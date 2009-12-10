/*
 * JPEG decoding engine for DCT-baseline
 *
 *      copyrights 2003 by nikq | nikq::club.
 *
 * history::
 * 2003/04/28 | added OSASK-GUI ( by H.Kawai )
 * 2003/05/12 | optimized DCT ( 20-bits fixed point, etc...) -> line 407-464
 *
 */

#define WINMAN0
#include "../math.h"

// ジグザグテーブル
static unsigned char zigzag_table[] = {
	 0, 1,  8,  16,  9,  2,  3, 10,
	17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63,
};

// ハフマンテーブル
typedef struct{
	int elem; //要素数
	unsigned short code[256];
	unsigned char  size[256];
	unsigned char  value[256];
} HUFF;

typedef struct{
    // SOF
    int width;
    int height;
    // MCU
    int mcu_width;
    int mcu_height;

    int max_h,max_v;
    int compo_count;
    int compo_id[3];
    int compo_sample[3];
    int compo_h[3];
    int compo_v[3];
    int compo_qt[3];

    // SOS
    int scan_count;
    int scan_id[3];
    int scan_ac[3];
    int scan_dc[3];
    int scan_h[3];  // サンプリング要素数
    int scan_v[3];  // サンプリング要素数
    int scan_qt[3]; // 量子化テーブルインデクス
    
    // DRI
    int interval;

    int mcu_buf[32*32*4]; //バッファ
    int *mcu_yuv[4];
    int mcu_preDC[3];
    
    // DQT
    int dqt[3][64];
    int n_dqt;
    
    // DHT
    HUFF huff[2][3];
    
    
    // FILE i/o
	unsigned char *fp, *fp1;
//    FILE *fp;
    unsigned long bit_buff;
    int bit_remain;

    int width_buf;

} JPEG;

// -------------------------- I/O ----------------------------

unsigned short get_bits(JPEG *jpeg, int bit)
{
	unsigned char  c, c2;
	unsigned short ret;
	unsigned long  buff;
	int remain;

	buff   = jpeg->bit_buff;
	remain = jpeg->bit_remain;

	while (remain <= 16) {
		if (jpeg->fp >= jpeg->fp1) {
			ret = 0;
			goto fin;
		}
		c = *jpeg->fp++;
		if (c == 0xff) { // マーカエラーを防ぐため、FF -> FF 00 にエスケープされてる
			if (jpeg->fp >= jpeg->fp1) {
				ret = 0;
				goto fin;
			}
			jpeg->fp++; /* 00をskip */
		}
		buff = (buff << 8) | c;
		remain += 8;
	}
	ret = (buff >> (remain - bit)) & ((1 << bit) - 1);
	remain -= bit;

	jpeg->bit_remain = remain;
	jpeg->bit_buff   = buff;
fin:
	return ret;
}

// ------------------------ JPEG セグメント実装 -----------------

// start of frame
int jpeg_sof(JPEG *jpeg)
{
	unsigned char c;
	int i, h, v, n;

	if (jpeg->fp + 8 > jpeg->fp1)
		goto err;
	/* fp[2] は bpp */
	jpeg->height = jpeg->fp[3] << 8 | jpeg->fp[4];
	jpeg->width  = jpeg->fp[5] << 8 | jpeg->fp[6];
	n = jpeg->compo_count = jpeg->fp[7]; // Num of compo, nf
	jpeg->fp += 8;
	if (jpeg->fp + n * 3 > jpeg->fp1)
		goto err;

	for (i = 0; i < n; i++) {
		jpeg->compo_id[i] = jpeg->fp[0];

		jpeg->compo_sample[i] = c = jpeg->fp[1];
		h = jpeg->compo_h[i] = (c >> 4) & 0x0f;
		v = jpeg->compo_v[i] = c & 0x0f;

		if (jpeg->max_h < h)
			jpeg->max_h = h;
		if (jpeg->max_v < v)
			jpeg->max_v = v;

        jpeg->compo_qt[i] = jpeg->fp[2];
		jpeg->fp += 3;
    }
    return 0;
err:
	jpeg->fp = jpeg->fp1;
	return 1;
}

// define quantize table
int jpeg_dqt(JPEG *jpeg)
{
	unsigned char c;
	int i, j, v, size;

	if (jpeg->fp + 2 > jpeg->fp1)
		goto err;
	size = (jpeg->fp[0] << 8 | jpeg->fp[1]) - 2;
	jpeg->fp += 2;
	if (jpeg->fp + size > jpeg->fp1)
		goto err;

	while (size > 0) {
		c = *jpeg->fp++;
		size--;
		j = c & 7;
		if (j > jpeg->n_dqt)
			jpeg->n_dqt = j;

 		if (c & 0xf8) {
			// 16 bit DQT
			for (i = 0; i < 64; i++) {
				jpeg->dqt[j][i] = jpeg->fp[0]; /* 下位は読み捨てている */
				jpeg->fp += 2;
			}
			size += -64 * 2;
		} else {
			//  8 bit DQT
			for (i = 0; i < 64; i++)
				jpeg->dqt[j][i] = *jpeg->fp++;
			size -= 64;
		}
	}
	return 0;
err:
	jpeg->fp = jpeg->fp1;
	return 1;
}

// define huffman table
int jpeg_dht(JPEG *jpeg)
{
	unsigned tc, th;
	unsigned code = 0;
	unsigned char val;
	int i, j, k, num, Li[17];
	int len, max_val;
	HUFF *table;

	if (jpeg->fp + 2 > jpeg->fp1)
		goto err;
	len = (jpeg->fp[0] << 8 | jpeg->fp[1]) - 2;
	jpeg->fp += 2;

	while (len > 0) {
		if (jpeg->fp + 17 > jpeg->fp1)
			goto err;
		val = jpeg->fp[0];

		tc = (val >> 4) & 0x0f; // テーブルクラス(DC/AC成分セレクタ)
		th =  val       & 0x0f; // テーブルヘッダ(何枚目のプレーンか)
		table = &(jpeg->huff[tc][th]);

		num = 0;
		k = 0;
		for (i = 1; i <= 16; i++) {
			Li[i] = jpeg->fp[i];
			num += Li[i];
            for (j = 0; j < Li[i]; j++)
                table->size[k++] = i;
		}
		table->elem = num;
		jpeg->fp += 17;

		k=0;
		code=0;
		i = table->size[0];
		while (k < num) {
			while (table->size[k] == i)
				table->code[k++] = code++;
			if (k >= num)
				break;
			do {
				code <<= 1;
				i++;
			} while (table->size[k] != i);
		}

		if (jpeg->fp + num > jpeg->fp1)
			goto err;
		for (k = 0; k < num; k++)
			table->value[k] = jpeg->fp[k];
		jpeg->fp += num;

        len -= 18 + num;
    }
    return 0;
err:
	jpeg->fp = jpeg->fp1;
	return 1;
}

void jpeg_idct_init(void);

void jpeg_init(JPEG *jpeg)
{
	jpeg_idct_init();
#if 0
	jpeg->width = 0;
	jpeg->mcu_preDC[0] = 0;
	jpeg->mcu_preDC[1] = 0;
	jpeg->mcu_preDC[2] = 0;
	jpeg->n_dqt = 0;
	jpeg->max_h = 0;
	jpeg->max_v = 0;
	jpeg->bit_remain = 0;
	jpeg->bit_buff   = 0;
	// DRIリセット無し
	jpeg->interval = 0;
#endif
	return;
}

int jpeg_header(JPEG *jpeg)
{
	unsigned char c;
	int r = 0, i;

	for (;;) {
		if (jpeg->fp + 2 > jpeg->fp1)
			goto err;
		if (jpeg->fp[0] != 0xff)
            goto err0;
		c = jpeg->fp[1];
		jpeg->fp += 2;
		if (c == 0xd8)
			continue; /* SOI */
		if (c == 0xd9)
			goto err; /* EOI */

        switch (c) {
        case 0xC0:
            jpeg_sof(jpeg);
            break;
        case 0xC4:
            jpeg_dht(jpeg);
            break;
        case 0xDB:
            jpeg_dqt(jpeg);
            break;
        case 0xDD: /* data restart interval */
			if (jpeg->fp + 4 > jpeg->fp1)
				goto err;
			jpeg->interval = jpeg->fp[2] << 8 | jpeg->fp[3];
			jpeg->fp += 4;
            break;
        case 0xDA: /* start of scan */
			if (jpeg->fp + 3 > jpeg->fp1)
				goto err;
			jpeg->scan_count = jpeg->fp[2];
			jpeg->fp += 3;
			if (jpeg->fp + jpeg->scan_count * 2 > jpeg->fp1)
				goto err;
			for (i = 0; i < jpeg->scan_count; i++) {
				jpeg->scan_id[i] = jpeg->fp[0];
				jpeg->scan_dc[i] = jpeg->fp[1] >> 4;   // DC Huffman Table
				jpeg->scan_ac[i] = jpeg->fp[1] & 0x0F; // AC Huffman Table
				jpeg->fp += 2;
			}
			jpeg->fp += 3; /* 3bytes skip */
            goto fin;
        default:
			/* 未対応 */
			if (jpeg->fp + 2 > jpeg->fp1)
				goto err;
			jpeg->fp += jpeg->fp[0] << 8 | jpeg->fp[1];
            break;
        }
    }
err:
	jpeg->fp = jpeg->fp1;
err0:
	r++;
fin:
	return r;
}

// MCU decode

// デコード
void jpeg_decode_init(JPEG *jpeg)
{
	int i, j;

	for (i = 0; i < jpeg->scan_count; i++) {
		// i:scan
		for (j = 0; j < jpeg->compo_count; j++) {
			// j:frame
			if (jpeg->scan_id[i] == jpeg->compo_id[j]) {
				jpeg->scan_h[i]  = jpeg->compo_h[j];
				jpeg->scan_v[i]  = jpeg->compo_v[j];
				jpeg->scan_qt[i] = jpeg->compo_qt[j];
                break;
			}
		}
	//	if (j >= jpeg->compo_count)
	//		return 1;
	}
	jpeg->mcu_width  = jpeg->max_h * 8;
	jpeg->mcu_height = jpeg->max_v * 8;
    
	for (i = 0; i < 32 * 32 * 4; i++)
		jpeg->mcu_buf[i] = 0x80;

	for (i = 0; i < jpeg->scan_count; i++)
		jpeg->mcu_yuv[i] = jpeg->mcu_buf + (i << 10);
	return;
}

// ハフマン 1シンボル復号
int jpeg_huff_decode(JPEG *jpeg,int tc,int th)
{
    HUFF *h = &(jpeg->huff[tc][th]);
    int code,size,k,v;
    code = 0;
    size = 0;
    k = 0;
    while( size < 16 ){
        size++;
        v = get_bits(jpeg,1);
        if(v < 0){
            return v;
        }
        code = (code << 1) | v;

        while(h->size[k] == size){
            if(h->code[k] == code){
                return h->value[k];
            }
            k++;
        }
    }

    return -1;
}

// 逆DCT（パク
// 浮動少数つかっちゃってるんで、どうにかしたい.

// patched by nikq :: fixed-int(20bit)
#define PI	3.14159265358979324

// ８×８個 の『基底画像』生成
static int base_img[64][64]; // 基底画像 ( [横周波数uπ][縦周波数vπ][横位相(M/8)][縦位相(N/8)]
static int cost[32]=
{  32768, 32138, 30274, 27246, 23170, 18205, 12540, 6393,
       0, -6393,-12540,-18205,-23170,-27246,-30274,-32138,
  -32768,-32138,-30274,-27246,-23170,-18205,-12540,-6393,
       0,  6393, 12540, 18205, 23170, 27246, 30274, 32138
};
void jpeg_idct_init(void)
{
    int u, v, m, n;
    int tmpm[8], tmpn[8];
    for (u = 0; u < 8; u++) {
        {
            int i=u, d=u*2;
            if (d == 0)
                i = 4;
            for (m = 0; m < 8; m++){
                tmpm[m] = cost[i]; // 横のCos波形
                i=(i+d)&31;
            }
        }
        for (v = 0; v < 8; v++) {
            {
                int i=v,d=v*2;
                if (d == 0)
                    i=4;
                for (n = 0; n < 8; n++){
                    tmpn[n] = cost[i]; // 縦のCos波形
                    i=(i+d)&31;
                }
            }
            // 掛け算して基底画像に
            for (m = 0; m < 8; m++) {
                for (n = 0; n < 8; n++) {
                    base_img[u * 8 + v][m * 8 + n] = (tmpm[m] * tmpn[n])>>15;
                }
            }
        }
    }
    return;
}

void jpeg_idct(int *block, int *dest)
{
    int i, j ,k;

    for (i = 0; i < 64; i++)
        dest[i] = 0;

    for (i = 0; i < 64; i++) {
        k = block[i];
        if(k) { //0係数はステ
            for (j = 0; j < 64; j++) {
                dest[j] += k * base_img[i][j];
            }
        }
    }
    // 固定小数点を元に戻す+ 4で割る
    for (i = 0; i < 64; i++)
        dest[i] >>= 17;
    return;
}

// 符号化された数値を元に戻す
int jpeg_get_value(JPEG *jpeg,int size)
{
	int val = 0;
    if (size) {
		val = get_bits(jpeg,size);
		if (val < 0)
			val = 0x10000 | (0 - val);
		else if (!(val & (1<<(size-1))))
			val -= (1 << size) - 1;
	}
    return val;
}

// ---- ブロックのデコード ---
// ハフマンデコード＋逆量子化＋逆ジグザグ
int jpeg_decode_huff(JPEG *jpeg,int scan,int *block)
{
    int size, len, val, run, index;
    int *pQt = (int *)(jpeg->dqt[jpeg->scan_qt[scan]]);
    
    // DC
    size = jpeg_huff_decode(jpeg,0,jpeg->scan_dc[scan]);
    if(size < 0)
        return 0;
    val = jpeg_get_value(jpeg,size);
    jpeg->mcu_preDC[scan] += val;
    block[0] = jpeg->mcu_preDC[scan] * pQt[0];

    //AC復号
    index = 1;
    while(index<64)
    {
        size = jpeg_huff_decode(jpeg,1,jpeg->scan_ac[scan]);
        if(size < 0)
            break;
        // EOB
        if(size == 0)
            break;
        
        // RLE
        run  = (size>>4)&0xF;
        size = size & 0x0F;
        
        val = jpeg_get_value(jpeg,size);
        if(val>=0x10000) {
            //マーカ発見
            return val;
        }

        // ZRL
        while (run-- > 0)
            block[ zigzag_table[index++] ] = 0;
        
        block[ zigzag_table[index] ] = val * pQt[index];
        index++;
    }
    while(index<64)
        block[zigzag_table[index++]]=0;
    return 0;
}

// ブロック (補間かけるには、ここで)
// リサンプリング
void jpeg_mcu_bitblt(int *src, int *dest, int width,
                     int x0, int y0, int x1, int y1)
{
	int w, h;
	int x, y, x2, y2;
	w = x1 - x0;
	h = y1 - y0;
	dest += y0 * width + x0;
	width -= w;

	for (y = 0; y < h; y++) {
		y2 = (y * 8 / h) * 8;
		for (x = 0; x < w; x++)
			*dest++ = src[y2 + (x * 8 / w)];
		dest += width;
	}
}

// MCU一個変換
int jpeg_decode_mcu(JPEG *jpeg)
{
	int scan, val;
	int unit, i, h, v;
	int *p, hh, vv;
	int block[64], dest[64];

	// mcu_width x mcu_heightサイズのブロックを展開
	for (scan = 0; scan < jpeg->scan_count; scan++) {
		hh = jpeg->scan_h[scan];
		vv = jpeg->scan_v[scan];
		for (v = 0; v < vv; v++) {
            for (h = 0; h < hh; h++) {
				// ブロック(8x8)のデコード
				val = jpeg_decode_huff(jpeg, scan, block);
			//	if(val>=0x10000){
			//		printf("marker found:%02x\n",val);
			//	}

				// 逆DCT
				jpeg_idct(block,dest);
				// リサンプリング

				// 書き込みバッファ
				p = jpeg->mcu_buf + (scan << 10);

                // 拡大転送
				jpeg_mcu_bitblt(dest, p, jpeg->mcu_width,
					jpeg->mcu_width * h / hh, jpeg->mcu_height * v / vv,
					jpeg->mcu_width * (h + 1) / hh, jpeg->mcu_height * (v + 1) / vv);
			}
		}
	}
}

// YCrCb=>RGB

int jpeg_decode_yuv(JPEG *jpeg, int h, int v, unsigned char *rgb)
{
	int x0, y0, x, y, x1, y1;
	int *py;
	int Y12, V;
	int mw, mh, w;
	int i;

	mw = jpeg->mcu_width;
	mh = jpeg->mcu_height;

	x0 = h * jpeg->max_h * 8;
	y0 = v * jpeg->max_v * 8;
	rgb += (y0 * jpeg->width_buf + x0) * 4;

	x1 = jpeg->width - x0;
	if (x1 > mw)
		x1 = mw;
	y1 = jpeg->height - y0;
	if (y1 > mh)
		y1 = mh;
    
	py = jpeg->mcu_buf;
	w = (jpeg->width_buf - x1) * 4;

	for (y = 0; y < y1; y++) {
		for (x = 0; x < x1; x++) {
			Y12 = py[0] << 12;
		//	U = py[1024]; /* pu */
			V = py[2048]; /* pv */

			/* blue */
			i = 128 + ((Y12 - V * 4 + py[1024] /* pu */ * 0x1C59) >> 12);
			if (i & 0xffffff00)
				i = (~i) >> 24;
			rgb[0] = i;

			/* green */
			i = 128 + ((Y12 - V * 0x0B6C) >> 12);
			if (i & 0xffffff00)
				i = (~i) >> 24;
			rgb[1] = i;

			/* red */
			i = 128 + ((Y12 + V * 0x166E) >> 12);
			if (i & 0xffffff00)
				i = (~i) >> 24;
			rgb[2] = i;
			py++;
			rgb += 4;
		}
		py += mw - x1;
		rgb += w;
	}
	return;
}

void jpeg_decode(JPEG *jpeg, unsigned char *rgb)
{
	int h_unit, v_unit;
	int mcu_count, h, v;
	int val;
	unsigned char m;

	// MCUサイズ計算
	jpeg_decode_init(jpeg);

	h_unit = (jpeg->width + jpeg->mcu_width - 1) / jpeg->mcu_width;
	v_unit = (jpeg->height + jpeg->mcu_height - 1) / jpeg->mcu_height;

	// 1ブロック展開するかもしれない
	mcu_count = 0;
	for (v = 0; v < v_unit; v++) {
		for (h = 0; h < h_unit; h++) {
			mcu_count++;
			jpeg_decode_mcu(jpeg);
			jpeg_decode_yuv(jpeg,h,v,rgb);
			if (jpeg->interval > 0 && mcu_count >= jpeg->interval) {
				// RSTmマーカをすっ飛ばす(FF hoge)
				// hogeは読み飛ばしてるので、FFも飛ばす
				jpeg->bit_remain -= (jpeg->bit_remain & 7);
				jpeg->bit_remain -= 8;
				jpeg->mcu_preDC[0] = 0;
				jpeg->mcu_preDC[1] = 0;
				jpeg->mcu_preDC[2] = 0;
				mcu_count = 0;
			}
		}
	}
    return;
}
#if !defined(WINMAN0)
/* stack:30k, malloc:7504k, mmarea:1024k */

#include <guigui00.h>

#define	AUTO_MALLOC	0
#define REWIND_CODE 	1

#define MAX_XSIZ		1600
#define MAX_YSIZ		1200
#define MAX_FSIZ		1024 * 1024

static int *signalbox0, *sig_ptr;
void initsignalbox();
const int getsignalw();

static int MALLOC_ADDR;
#define malloc(bytes)		(void *) (MALLOC_ADDR -= ((bytes) + 7) & ~7)
#define free(addr)			for (;;); /* freeがあっては困るので永久ループ */

void putstr(struct LIB_TEXTBOX *tbox, const char *s)
{
	lib_putstring_ASCII(0x0000, 0, 0, tbox, 0, 0, s);
	return;
}

void OsaskMain()
{
	struct LIB_WINDOW *window;
	struct LIB_TEXTBOX *wintitle, *tbox;
	struct LIB_GRAPHBOX *gbox;
    static JPEG jpeg;
	int i, filesize;
	int max[2];
	char *s;

	/* 何よりも最初にやるべきこと(lib_initよりも先！) */
	MALLOC_ADDR = lib_readCSd(0x0010);

	lib_init(AUTO_MALLOC);
	sig_ptr = signalbox0 = lib_opensignalbox(256, AUTO_MALLOC, 0, REWIND_CODE);
	window = lib_openwindow1(AUTO_MALLOC, 0x0200, 160, 16, 0x20, 8 - 7);
	wintitle = lib_opentextbox(0x1000, AUTO_MALLOC, 0,  7, 1,  0,  0, window, 0x00c0, 0);
	putstr(wintitle, "kjpegls");
	tbox = lib_opentextbox(0x0000, AUTO_MALLOC, 0, 20, 1,  0,  0, window, 0x00c0, 0);
	putstr(tbox, "select file");

	lib_initmodulehandle1(0x0210 /* slot */, 1 /* num */, 16 /* sig */);
	if (getsignalw() != 16) {
		/* エラーやキャンセル、もしくはウィンドウクローズ */
		lib_close(0); /* すぐにタスクを終了 */
		goto fin;
	}

	if ((filesize = lib_readmodulesize(0x0210)) > MAX_FSIZ) {
		s = "too large file";
		goto err;
	}
	jpeg.fp = (char *) lib_readCSd(0x0010 /* スタック領域の終わり == mapping領域の始まり */);
	lib_mapmodule(0x0000 /* opt */, 0x0210 /* slot */, 0x5 /* R-mode */, MAX_FSIZ, jpeg.fp, 0);
	jpeg.fp1 = jpeg.fp + filesize;
    jpeg_init(&jpeg);
   	jpeg_header(&jpeg);
	if (jpeg.width > MAX_XSIZ || jpeg.height > MAX_YSIZ) {
		s = "too large picture";
		goto err;
	}
	if (jpeg.width == 0) {
		s = "not support format";
		goto err;
	}

	putstr(tbox, "decoding...");
	gbox = malloc(sizeof (struct LIB_GRAPHBOX) + MAX_XSIZ * MAX_YSIZ * 4);
	jpeg.width_buf = jpeg.width;
    jpeg_decode(&jpeg, ((char *) gbox) + 64);
	lib_execcmd0(0x0014, 0x10, max, 0x000c, 0x0000);
	if (jpeg.width > max[0] || jpeg.height > max[1]) {
		s = "too small screen";
		goto err;
	}
	lib_closewindow(0, window);
	getsignalw();
	i = jpeg.width;
	if (i < 160)
		i = 160;
	lib_openwindow_nm(window, 0x0200, i, jpeg.height);
	lib_opentextbox_nm(0x1000, wintitle, 0,  7, 1,  0,  0, window, 0x00c0, 0);
	putstr(wintitle, "kjpegls");
	lib_opengraphbox_nm(0x1001, gbox, 4, 0, jpeg.width, jpeg.height, 0, 0, window);
fin:
	for (;;)
		getsignalw();
err:
	putstr(tbox, s);
	goto fin;
}

const int getsignalw()
{
    int signal;
    lib_waitsignal(0x0001, 0, 0);
    if (*sig_ptr == REWIND_CODE) {
		/* REWINDシグナルを受け取った */
		/* 直後の値の分だけシグナルを処理したことにして、ポインタを先頭に戻す */
        lib_waitsignal(0x0000, *(sig_ptr + 1), 0);
        sig_ptr = signalbox0;
    }
    signal = *sig_ptr;
    if (signal != 0) {
        sig_ptr++;
		/* １シグナル受け取ったことをライブラリに通知 */
        lib_waitsignal(0x0000, 1, 0);
    }
    return signal;
}
#endif
