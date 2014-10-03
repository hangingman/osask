#include <guigui01.h>

typedef unsigned char UCHAR;
int get32(const UCHAR *p);
void put32(UCHAR *p, int i);

#define MAXSIZ	2 * 1024 * 1024

#define	CMDLIN_IN		0
#define	CMDLIN_OUT		1
#define	CMDLIN_HEAP		2
#define	CMDLIN_MMA		3

static UCHAR cmdusg[] = {
	0x86, 0x50,
		0x88, 0x8c, 
		0x03, 'h', 'e', 'a', 'p', 0x11, '#',
		0x12, 'm', 'm', 'a', 0x11, '#',
	0x40
};

void G01Main()
{
	UCHAR *fbuf = g01_bss1a1;
	int heap_siz, mmarea, fsiz, dsize, dofs, stksiz, wrksiz, entry, bsssiz;
	int heap_adr, i;
	static UCHAR sign[4] = "Hari";

	g01_setcmdlin(cmdusg);

	/* パラメータの取得 */
	heap_siz = g01_getcmdlin_int_s(CMDLIN_HEAP);
	mmarea   = g01_getcmdlin_int_o(CMDLIN_MMA, 0);

	/* ファイル読み込み */
	g01_getcmdlin_fopen_s_0_4(CMDLIN_IN);
	fsiz = jg01_fread1f_4(MAXSIZ, fbuf);

	/* ヘッダ確認 */
	if (get32(&fbuf[4]) != 0x24) {	/* ファイル中の.textスタートアドレス */
err_form:
		g01_putstr0_exit1("bim file format error");
	}
	if (get32(&fbuf[8]) != 0x24)	/* メモリロード時の.textスタートアドレス */
		goto err_form;
	dsize  = get32(&fbuf[12]);	/* .dataセクションサイズ */
	dofs   = get32(&fbuf[16]);	/* ファイルのどこに.dataセクションがあるか */
	stksiz = get32(&fbuf[20]);	/* スタックサイズ */
	entry  = get32(&fbuf[24]);	/* エントリポイント */
	bsssiz = get32(&fbuf[28]);	/* bssサイズ */

	/* ヘッダ生成 */
	heap_adr = stksiz + dsize + bsssiz;
	heap_adr = (heap_adr + 0xf) & 0xfffffff0; /* 16バイト単位に切り上げ */
	wrksiz = heap_adr + heap_siz;
	wrksiz = (wrksiz + 0xfff) & 0xfffff000; /* 4KB単位に切り上げ */
	put32(&fbuf[ 0], wrksiz);
	for (i = 0; i < 4; i++)
		fbuf[4 + i] = sign[i];
	put32(&fbuf[ 8], mmarea);
	put32(&fbuf[12], stksiz);
	put32(&fbuf[16], dsize);
	put32(&fbuf[20], dofs);
	put32(&fbuf[24], 0xe9000000);
	put32(&fbuf[28], entry - 0x20);
	put32(&fbuf[32], heap_adr);

	/* ファイル書き込み */
	g01_getcmdlin_fopen_s_3_5(CMDLIN_OUT);
	jg01_fwrite1f_5(fsiz, fbuf);
	return;
}

int get32(const UCHAR *p)
{
	return p[0] | p[1] << 8 | p[2] << 16 | p[3] << 24;
}

void put32(UCHAR *p, int i)
{
	p[0] =  i        & 0xff;
	p[1] = (i >>  8) & 0xff;
	p[2] = (i >> 16) & 0xff;
	p[3] = (i >> 24) & 0xff;
	return;
}

/*

memo

[ .bimファイルの構造 ]

+ 0 : .textサイズ
+ 4 : ファイル中の.textスタートアドレス（0x24）
+ 8 : メモリロード時の.textスタートアドレス（0x24）
+12 : .dataサイズ
+16 : ファイル中の.dataスタートアドレス
+20 : メモリロード時の.dataスタートアドレス
+24 : エントリポイント
+28 : bss領域のバイト数
+36 : コード

[ .hrbファイルの構造 ]

+ 0 : stack+.data+heap の大きさ（4KBの倍数）
+ 4 : シグネチャ "Hari"
+ 8 : mmarea の大きさ（4KBの倍数）
+12 : スタック初期値＆.data転送先
+16 : .dataのサイズ
+20 : .dataの初期値列がファイルのどこにあるか
+24 : 0xe9000000
+28 : エントリアドレス-0x20
+32 : heap領域（malloc領域）開始アドレス

*/
