/* "osalink1.c":OSASK LINKプログラム version 2.0

	最初がBASE.EXE、その後は各種.BIN(.TEK)を想定している */



#include <guigui01.h>
#include <stdio.h>
#include <stdlib.h>

#define	OPTIONFILE	"OSALINK1.OPT"
#define	OUTPUTFILE	"OSASK.EXE"
#define	BUFSIZE		2 * 1024 * 1024



//オプションファイル情報の構造体の定義
struct optfiledata {
  int fnames;     //有効なファイル名の数
  unsigned char files[500];  //ファイル名。とりあえず500文字
  int hpoint[50];
  unsigned char *optname;
};



//関数のプロトタイプ宣言
char tolower_hide(char moto);
int readoptfile( struct optfiledata *optfiledata);
//const int script(char *opt, char *inp, char *out, char *helmes_buf);


void wordstore(char *p, const int w)
{
	p[0] =  w       & 0xff;
	p[1] = (w >> 8) & 0xff;
	return;
}

void dwordstore(char *p, const int d)
{
	p[0] =  d        & 0xff;
	p[1] = (d >>  8) & 0xff;
	p[2] = (d >> 16) & 0xff;
	p[3] = (d >> 24) & 0xff;
	return;
}

const int wordload(const unsigned char *p)
{
	return p[0] | p[1] << 8;
}

const int dwordload(const unsigned char *p)
{
	return p[0] | p[1] << 8 | p[2] << 16 | p[3] << 24;
}


//const int main(int argc, char **argv)
void G01Main()
{
	char error[100];

  //バージョン表記
  //  if ( argc == 2 ){
  //    if ( strcmp(argv[1],"-v") == 0 ){
  //      fprintf(stderr, "osalink1 hideyosi version 1.1\n");
  //      return 0;
  //    }
  //  }

  //FILE *fp0, *fp1;
	int i, i2,j, k, size, totalsize = 0;
	unsigned char fname[32], name[8], c;
	unsigned char fname2[32];


	//	unsigned char buf0[2*1024*1024];
	unsigned char *buf0 = jg01_malloc(BUFSIZE);
	if ( *buf0 == -1 ){
	  g01_putstr0("Mem Error!\n");
	  return;
	}


	unsigned char *buf = buf0, *buf1, *buf2, *buf3;

	unsigned char *optfile = OPTIONFILE, *outfile = OUTPUTFILE;

	//if (argc == 4)
	//	return script(argv[1], argv[2], argv[3], buf0);
	//if (argc >= 2)
	//	optfile = argv[1];
	//if (argc >= 3)
	//	outfile = argv[2];


	// OPTIONFILEの読み込みと各ファイルのサイズ取得
	//optionファイルの読み込みは関数化。構造体に内容を入れて
	//返してくる。


	//構造体を宣言
	struct optfiledata optfdata1;
	optfdata1.optname = OPTIONFILE;
	optfdata1.fnames = 3;

	i=readoptfile(&optfdata1);

		sprintf(error,"fnames=%d\n",optfdata1.fnames);
		g01_putstr0(error);



		//		for ( i = 0; i < optfdata1.fnames; i++){
		//		  g01_putstr0(optfdata1.files + optfdata1.hpoint[i] );
		//		  g01_putstr0("\n");
		//		}

	//ここで構造体にファイル名の配列情報が来ているはず・・・





	//fp0 = fopen(optfile, "r");
	//	if (fp0 == NULL) {
	//		fprintf(stderr, "Can't open \"%s\".\n", optfile);
	//		return 1;
	//	}














	for (i = 0; i < optfdata1.fnames; i++) {

	  //改行コードの引っこ抜き
	  // for ( i2 = 31; i2 != 0; i2--){
	  //	    if ( fname[i2] == 0x0a ) fname[i2] = 0x0;
	  //	    if ( fname[i2] == 0x0d ) fname[i2] = 0x0;
	  //	  }

	  //わっからん・・・こうやってコピーするとうまくいく・・・
	  //	  strcpy (fname2,fname);





	  //fp1 = fopen(fname2, "rb");
	  //	  jg01_fopen(1, 4, optfile);

	  g01_putstr0(optfdata1.files + optfdata1.hpoint[i]);
	  g01_putstr0(" Reading....");

	  jg01_fopen(1, 4, optfdata1.files + optfdata1.hpoint[i]);
	  size = jg01_fread1_4(BUFSIZE - totalsize, buf + totalsize);
	  
	  if (size == 0){
	    g01_putstr0("error\n");
	    return;
	  }

	  *fname = optfdata1.files + optfdata1.hpoint[i];


	  jg01_fclose(4);
	  g01_putstr0(" OK!");
	  sprintf(error,"%d bit readen\n",size);
	  g01_putstr0(error);

	  sprintf(error,"buf=%d\n",buf+totalsize);
	  g01_putstr0(error);



	  //		if (fp1 == NULL) {
err1:
	  //			fclose(fp0);
	  //			fprintf(stderr, "Can't open \"%s\".\n", fname2);
	  //			return 1;
	  //		}
	  //			fprintf(stderr, "reading %s \n", fname2);

		buf1 = buf + totalsize;
		//		size = fread(buf + totalsize, 1, BUFSIZE - totalsize, fp1);
		//		fclose(fp1);

	       
		if (size <= 0)
		  //	goto err1;
		  return 1;

		if (size >= BUFSIZE - totalsize)
		  //goto err1;
		  return 1;
		if (i == 0) {
			buf2 = buf + (wordload(buf + 0x204) << 4) - 16 + 0x220;
			goto skip; /* BASE.EXEは加工しない */
		}

		for (j = 0; j < 8; j++)
			name[j] = ' ';
		for (j = 0; j < 8 && fname[j] != '.'; j++)
			name[j] = tolower_hide(fname[j]);
		for (buf3 = buf2; *buf3; buf3 += 16) {
			c = 0;
			for (j = 0; j < 8; j++)
				c |= name[j] ^ buf3[j];
			if (c)
				continue;
			j = size;
			if (size > 20) {
			//	c = 0;
				for (k = 1; k < 16; k++)
					c |= buf1[k] ^ "\x82\xff\xff\xff\x01\x00\x00\x00OSASKCMP"[k];
				if (c == 0) {
					size -= 20;
					j = dwordload(buf1 + 16); /* 展開後のサイズ */
					for (k = 0; k < size; k++)
						buf1[k] = buf1[k + 20];
					buf3[0x0f] = 0x80; /* 圧縮フラグを立てる */
				}
			}
			dwordstore(buf3 + 0x08, j);
			wordstore(buf3 + 0x0c, (totalsize - 0x200) >> 4);
			break;
		}
skip:
		totalsize += size;
		while (totalsize & 0x0f)
			*(buf + totalsize++) = '\0'; /* パラグラフ単位のアライン */
	}
	//fclose(fp0);
	//ここでファイル名のループは終わり


	/* ヘッダ調整 */
	wordstore(buf + 0x02, totalsize & 0x01ff); // 最終ページサイズ
	wordstore(buf + 0x04, (totalsize + 0x1ff) >> 9); // ファイルページ数
	wordstore(buf + 0x0e, (totalsize - 0x200) >> 4); // 初期SS

	if (buf[0x0208] == 0x10 && buf[0x0209] == 0x89 && buf[0x020a] == 0x00) {
		/* OSASKのKHBIOS用スクリプトを発見 */
		size = (totalsize + (0x1ff - 0x200)) >> 9;
		buf[0x020b] =  size       & 0xff;
		buf[0x020c] = (size >> 8) & 0xff;

	}


	sprintf(error,"totalsize=%d",totalsize);
	g01_putstr0(error);


	/* 出力 */
	//	fp1 = fopen(outfile, "wb");
	//	fwrite(buf, 1, totalsize, fp1);
	//	fclose(fp1);
	//	return 0;

	jg01_fopen(0x19, 5,  outfile);
	jg01_fwrite1f_5(totalsize,buf);
	jg01_fclose(5);


}

char *skipspace(char *s, char *s1)
{
retry:
	while (s < s1 && *s <= ' ')
		s++;
	if (s + 1 < s1 && *s == '/' && *(s + 1) == '*') {
		s += 2;
		do {
			while (s < s1 && *s != '*')
				s++;
			if (s + 1 >= s1)
				return s1;
			s += 2;
		} while (*(s - 1) != '/');
		goto retry;
	}
	return s;
}

char *checktoken(char *s, char *s1)
/* これを呼ぶ前にskipspaceしておくこと */
{
	char *s0 = s;
	if (s < s1) {
		do {
			char c = *s;
			if (c <= ' ')
				break;
			if (c == ',')
				break;
			if (c == '(')
				break;
			if (c == ')')
				break;
			if (c == '{')
				break;
			if (c == '}')
				break;
			if (c == ';')
				break;
			if (c == '\x22')
				break;
			s++;
		} while (s < s1);
		if (s == s0)
			s++;
	}
	return s;
}

int getnum(char *s, char *s1)
/* 10進数と16進数のみ, しかも負の値は扱わない */
/* エラーだと-1を返す */
/* sはskipspace()しておくこと */
{
	int i = 0, base = 10, c;
	if (s >= s1)
		goto err;
	if (s + 2 < s1 && *s == '0' && tolower_hide(*(s + 1)) == 'x') {
		s += 2;
		base = 16;
	}
	do {
		c = tolower_hide(*s++);
		if (c < '0')
			goto err;
		c -= '0';
		if (c <= 9)
			goto skip;
		if (c < 'a' - '0')
			goto err;
		c -= 'a' - '0' - 10;
		if (c >= base)
			goto err;
	skip:
		i = i * base + c;
	} while (s < s1);
	return i;
err:
	return -1;
}

const char cmptoken(char *s, char *s1, char *t)
{
	do {
		if (s >= s1)
			break;
		if (*s++ != *t)
			break;
		t++;
	} while (*t);
	return *t;
}

char *get2prm(char *s, char *scr1, int *p0, int *p1)
/* "(p0, p1," */
{
	char *s1;

	s = skipspace(s, scr1);
	if (s >= scr1)
		goto err;
	s1 = checktoken(s, scr1);
	if (cmptoken(s, s1, "(") != 0)
		goto err;
	s = skipspace(s1, scr1);
	if (s >= scr1)
		goto err;
	s1 = checktoken(s, scr1);
	if ((*p0 = getnum(s, s1)) < 0)
		goto err;
	s = skipspace(s1, scr1);
	if (s >= scr1)
		goto err;
	s1 = checktoken(s, scr1);
	if (cmptoken(s, s1, ",") != 0)
		goto err;
	s = skipspace(s1, scr1);
	if (s >= scr1)
		goto err;
	s1 = checktoken(s, scr1);
	if ((*p1 = getnum(s, s1)) < 0)
		goto err;
	s = skipspace(s1, scr1);
	if (s >= scr1)
		goto err;
	s1 = checktoken(s, scr1);
	if (cmptoken(s, s1, ",") != 0)
		goto err;
	s = skipspace(s1, scr1);
	if (s >= scr1)
		goto err;
	return s;

err:
	return NULL;
}

char *get3prm(char *s, char *scr1, int *p0, int *p1, int *p2)
{
	char *s1;

	s = get2prm(s, scr1, p0, p1);
	if (s == NULL)
		goto err;
	s1 = checktoken(s, scr1);
	if ((*p2 = getnum(s, s1)) < 0)
		goto err;
	s = skipspace(s1, scr1);
	if (s >= scr1)
		goto err;
	s1 = checktoken(s, scr1);
	if (cmptoken(s, s1, ")") != 0)
		goto err;
	s = skipspace(s1, scr1);
	if (s >= scr1)
		goto err;
	s1 = checktoken(s, scr1);
	if (cmptoken(s, s1, ";") != 0)
		goto err;
	return s1;

err:
	return NULL;
}


//この関数は使われていないと思うんだが･･
/*
const int script(char *opt, char *inp, char *out, char *helmes_buf)
     //スクリプトが4KBを超えたら死にます
{


	unsigned char *buf = helmes_buf + 4 * 1024, *scr0 = helmes_buf, *scr1, *s, *s1;
		FILE *fp;
	int size, memofs, filofs;

		fp = fopen(opt, "r");

	if (fp == NULL)
		goto err;
	scr1 = scr0 + fread(scr0, 1, 4 * 1024, fp);
	fclose(fp);

	s = scr0;
	for (;;) {
		s = skipspace(s, scr1);
		if (s >= scr1)
			break;
		s1 = checktoken(s, scr1);
		if (cmptoken(s, s1, ";") == 0)
			continue;
		if (cmptoken(s, s1, "load") == 0) {
			s = get3prm(s1, scr1, &size, &memofs, &filofs);
			if (s == NULL)
				goto err;
			fp = fopen(inp, "rb");
			if (fp == NULL)
				goto err;
			fseek(fp, filofs, SEEK_SET);
			if (fread(buf + memofs, 1, size, fp) != size)
				goto err;
			fclose(fp);
			continue;
		}
		if (cmptoken(s, s1, "store") == 0) {
			s = get3prm(s1, scr1, &size, &memofs, &filofs);
			if (s == NULL)
				goto err;
			fp = fopen(out, "wb");
			if (fp == NULL)
				goto err;
			fseek(fp, filofs, SEEK_SET);
			if (fwrite(buf + memofs, 1, size, fp) != size)
				goto err;
			fclose(fp);
			continue;
		}
		if (cmptoken(s, s1, "fill") == 0) {
			s = get3prm(s1, scr1, &size, &memofs, &filofs);
			if (s == NULL)
				goto err;
			while (size > 0) {
				buf[memofs++] = (char) filofs;
				size--;
			}
			continue;
		}

#if 0
		if (cmptoken(s, s1, "if") == 0) {

			s = skipspace(s1, scr1);
			if (s >= scr1)
				goto err;
			s1 = checktoken(s, scr1);
			if (cmptoken(s, s1, "(") != 0) {
				goto err;
			s = skipspace(s1, scr1);
			if (s >= scr1)
				goto err;

		}
#endif

err:
		return 1;
	}
	return 0;
}

*/


char tolower_hide(char moto){
    if ( (moto > 0x41) && (moto < 0x5a ) ){
        return moto + 0x20;
    }
    else{
       return moto;
    }
}




//オプションファイルを読み込み、ファイル名の構造体を返す
int readoptfile(struct optfiledata *optdata1){

  int i,i2;
  int lfs;
  int fnames;


	jg01_fopen(1, 4, optdata1->optname);
	jg01_fread0_4(2 * 1024 * 1024, g01_bss1a1);
	if (strlen(g01_bss1a1) == 0) {
	  g01_putstr0("Can't open optfile\n");
	  return;
	}
  g01_putstr0("cccccccc\n");

	//まずは改行コードを一旦統一する
	for ( i = 0; i < strlen(g01_bss1a1); i++){
	  //とりあえずLFに統一するか・・・
	  if (g01_bss1a1[i] == 0x0D) g01_bss1a1[i] = 0x0A;
	  //これで、CRが全部LFに置換されるはず。
	}

	//先頭にある邪魔なLFなどを排除するため、開始地点を探っておく
	int stppoint; stppoint = 0;
	for ( i =0; i < strlen(g01_bss1a1); i++){
	  if ( g01_bss1a1[i] != 0x0A ){
	    stppoint = i;
	    break;
	  }
	}
	//これでもし先頭にLFがあっても全て飛ばした位置から開始できる


	//新しい配列変数を確保する。
	i2 = 0;
	lfs = 0;

	//	unsigned char files[strlen(g01_bss1a1)+1];
	//	unsigned char *files = tttttt.files;
	unsigned char *files = optdata1->files;

	for ( i = stppoint; i < strlen(g01_bss1a1); i++){
	  if ( g01_bss1a1[i] == 0x0A && g01_bss1a1[i-1] == 0x0A ){
	    //  連続したLFだったらシカトして進める。
	  }
	  else {
	    if ( g01_bss1a1[i] == 0x0A ){
	      files[i2] = 0;
	      lfs++;
	    }
	    else {
	      files[i2] = g01_bss1a1[i];
	    }
	    i2++;
	    //  LFを終端コードに置換
	  }
	}
	//最後に終端コードを打っておく
	files[i2+1] = 0;

	//エラーチェック用
	char error[100];
	//sprintf(error,"ii2=%d\n",ii2);
	//	g01_putstr0(error);
	//	g01_putstr0(files);


	//これで、0によって区切られた配列になったはず。
	//files配列内の、0の位置を記憶する配列を用意する。
	//厳密には0の前。行頭というべきか・・・
	//	int zeropoint[lfs + 1];
	//	int *zeropoint = optdata1->hpoint;
	//↑の処理で先頭は必ず行頭になってるはずなので・・・
	optdata1->hpoint[0]=0;
	fnames = 1;
	for (i = 1; i < i2 +1; i++){
	    if (files[i] != 0 && files[i-1] == 0){
	      //ここが行頭になるはず。位置を記憶
	      optdata1->hpoint[fnames] = i;
	      fnames++;
	    }
	}

	//これで、有効な行数はi3個。lfpointが行の先頭になっているはず。
		sprintf(error,"fnames=%d\n",fnames);
		g01_putstr0(error);
	//
		//				for ( i = 0; i < files; i++){
		//			g01_putstr0(files + optdata1->hpoint[i] );	g01_putstr0("\n");
		//					}

	//ここまででoptファイル読み込みの処理は終わった。
	//ファイルをクローズし、バッファも破棄する。
	jg01_fclose(4); 
	//	jg01_mfree(2*024*1024,buffer1);


	optdata1->fnames = fnames;



	return 0;








 }
