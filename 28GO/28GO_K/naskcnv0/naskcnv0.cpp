/* "naskcnv0"
	Copyright(C) 2009 H.Kawai

	usage : >naskcnv0 [-l] [-s] [-w] input-file output-file

	-l:leaをmovに変換
	-s:余計なdword、word、byteを削除
*/

#include <guigui01.h>
#include <string.h>

typedef unsigned char UCHAR;
typedef unsigned int UINT;

#define	NL			"\n"
#define	LEN_NL		1

#define FLAG_L		0
#define FLAG_S		1
#define FLAG_W		2

#define CMDLIN_FLG_L	0
#define CMDLIN_FLG_S	1
#define CMDLIN_FLG_W	2
#define	CMDLIN_IN		3
#define	CMDLIN_OUT		4

static UCHAR cmdusg[] = {
	0x86, 0x53,
		0x11, '-', 'l', 0x20,
		0x11, '-', 's', 0x20,
		0x11, '-', 'w', 0x20,
		0x88, 0x8c,
	0x40
};

static UCHAR *dest0_, *dest1_;

#define NULL	0

struct STR_FLAGS {
	UCHAR opt[3];
};

struct stack_alloc {
	UCHAR ibuf[8 * 1024 * 1024];	
	UCHAR obuf[8 * 1024 * 1024];
};

static UCHAR *convmain(UCHAR *src0, UCHAR *src1, UCHAR *dest0, UCHAR *dest1, struct STR_FLAGS flags);

char *strchr(char *d, int c);

void G01Main()
{
	struct stack_alloc *pwork;
	UCHAR *src1;
	struct STR_FLAGS flags;

	g01_setcmdlin(cmdusg);
	pwork = jg01_malloc(sizeof (struct stack_alloc));
	flags.opt[FLAG_L] = g01_getcmdlin_flag_s(CMDLIN_FLG_L);
	flags.opt[FLAG_S] = g01_getcmdlin_flag_s(CMDLIN_FLG_S);
	flags.opt[FLAG_W] = g01_getcmdlin_flag_s(CMDLIN_FLG_W);
	g01_getcmdlin_fopen_s_0_4(CMDLIN_IN);
	src1 = pwork->ibuf + jg01_fread1f_4(sizeof (pwork->ibuf), pwork->ibuf);
	src1 = convmain(pwork->ibuf, src1, pwork->obuf, pwork->obuf + sizeof (pwork->obuf), flags);
	if (src1 == NULL)
		g01_putstr0_exit1("output filebuf over!");

	g01_getcmdlin_fopen_s_3_5(CMDLIN_OUT);
	jg01_fwrite1f_5(src1 - pwork->obuf, pwork->obuf);
	return;
}

static void output(UINT l, UCHAR *s)
{
	if (l) {
		if (dest0_ + l >= dest1_)
			dest0_ = NULL;
		if (dest0_) {
			do {
				*dest0_++ = *s++;
			} while (--l);
		}
	}
	return;
}

static void output0(UCHAR *s)
{
	output(strlen(s), s);
	return;
}

char *wordsrch(char *s, const char *t);
char *cwordsrch(char *s, const char *t);
void cnv_lea(char *p);

static char leaopt = 0;

UCHAR *convmain(UCHAR *src0, UCHAR *src1, UCHAR *dest0, UCHAR *dest1, struct STR_FLAGS flags)
{
	UCHAR *p, *q;
	int /* obj = 0, */ i /* , c */;
	UCHAR farproc = 0;
	static char *eraselist[] = {
		"ENDP", "ENDS", "END", "ASSUME", NULL
	};
	static UCHAR linebuf0[1000];
	UCHAR *linebuf = linebuf0;

	leaopt = flags.opt[FLAG_L];
	dest0_ = dest0;
	dest1_ = dest1;

	output0((flags.opt[FLAG_W]) ? "[FORMAT \"WCOFF\"]" NL : "[FORMAT \"BIN\"  ]" NL);
	output0(
		"[INSTRSET \"i486p\"]" NL
		"[OPTIMIZE 1]" NL
		"[OPTION 1]" NL
		"[BITS 32]" NL
	);

	for (;;) {
		p = src0;
		for (;;) {
			if (p >= src1)
				break;
			if (*p++ == '\n')
				break;
		}
		if (p == src0)
			break;
		if (p - src0 > (int) (sizeof linebuf0) - 1) {
			/* 長すぎる...処理に困るのでとりあえず素通りする */
			output(p - src0, src0);
			src0 = p;
			continue;
		}
		q = linebuf;
		do {
			*q++ = *src0++;
		} while (src0 < p);
		*q = '\0';
		if (strchr(linebuf, '\"'))
			goto output_; // 変換しない

		// 文中に「"」が無かったので、遠慮なく変換

		// segment文検出
		if (cwordsrch(linebuf, "SEGMENT")) {
			output0(cwordsrch(linebuf, "CODE")
				? "[SECTION .text]" NL : "[SECTION .data]" NL);
			continue;
		}

		// proc文検出
		if ((p = cwordsrch(linebuf, "PROC")) != 0) {
			farproc = (cwordsrch(p, "FAR") != NULL);
			for (p = linebuf; *p <= ' '; p++);
			while (*p > ' ')
				p++;
			p[0] = ':';
			#if (LEN_NL == 1)
				p[1] = '\n';
				p[2] = '\0';
			#else
				p[1] = '\r';
				p[2] = '\n';
				p[3] = '\0';
			#endif
		//	goto output_; // 他の変換はもうしない
		}

		for (i = 0; eraselist[i] != 0; i++) {
			if (cwordsrch(linebuf, eraselist[i]))
				goto noout; // 一切出力しない
		}

		// ret文検出
		if ((p = cwordsrch(linebuf, "RET")) != 0) {
			p += 3;
			for (q = p; *q; q++);
			while (p <= q) {
				*(q + 1) = *q;
				q--;
			}
			p[-3] = 'R';
			p[-2] = 'E';
			p[-1] = 'T';
			p[ 0] = farproc ? 'F' : 'N';
		}

		// ローカルラベル変換
		while ((p = strchr(linebuf, '#')) != 0) {
			*p = '.';
		}

		// LEA文検出
		if ((p = cwordsrch(linebuf, "LEA")) != 0)
			cnv_lea(p);

		/* 簡易判定方法でパラメータを検出 */
		/* ・最後に":"が付いているニーモニックはラベル宣言と解釈 */
		p = linebuf;
		do {
			while (*p != '\0' && *p <= ' ')
				p++;
			while (*p > ' ')
				p++;
			if (*p == '\0')
				break;
		} while (p[-1] == ':');
		if (*p != '\0') {
			while ((q = cwordsrch(p, "OR")) != 0) {
				q[0] = '|';
				q[1] = ' ';
			}
			while ((q = cwordsrch(p, "AND")) != 0) {
				q[0] = '&';
				q[1] = ' ';
				q[2] = ' ';
			}
			while ((q = cwordsrch(p, "XOR")) != 0) {
				q[0] = '^';
				q[1] = ' ';
				q[2] = ' ';
			}
			while ((q = cwordsrch(p, "NOT")) != 0) {
				q[0] = '~';
				q[1] = ' ';
				q[2] = ' ';
			}
		}

		// ptr消去
		while ((p = cwordsrch(linebuf, "PTR")) != 0) {
			p[0] = p[1] = p[2] = ' ';
		}

		// offset消去
		while ((p = cwordsrch(linebuf, "OFFSET")) != 0) {
			p[0] = p[1] = p[2] = p[3] = p[4] = p[5] = ' ';
		}

		// dword, word, byte消去 (大文字は残す)
		if (flags.opt[FLAG_S] != 0 && strchr(linebuf, '[') == NULL) {
			while ((p = wordsrch(linebuf, "dword")) != 0) {
				p[0] = ' ';
			}
			while ((p = wordsrch(linebuf, "word")) != 0) {
				p[0] = p[1] = p[2] = p[3] = ' ';
			}
			while ((p = wordsrch(linebuf, "byte")) != 0) {
				p[0] = p[1] = p[2] = p[3] = ' ';
			}
		}

output_:
		output0(linebuf);
noout:
		;
	}
	output0(NL);
	return dest0_;
}

void cnv_lea(char *p)
// LEA文検出
{
	char *q;

	// LEA文からセグメントオーバーライドプリフィクスを取り除く
	if ((q = strstr(p + 3, "S:[")) || (q = strstr(p + 3, "s:["))) {
		q[-1] = ' '; // 'E', 'C', 'S', 'D', 'F', or 'G'
		q[ 0] = ' '; // 'S'
		q[ 1] = ' '; // ':'
	}

	// LEA文で、定数MOVに変換可能なら変換する
	if (leaopt != 0 && (q = strchr(p + 3, '[')) != 0) {
		char *q0 = q++;
		do {
			while ((*q == '+' || *q == '-' || *q == '*'
				|| *q == '/' || *q == '(' || *q == ')'
				|| *q <= ' ') && *p != '\0')
				q++;
			if ('0' <= *q && *q <= '9') {
				while (*q > ' ' && *q != '+' && *q != '-'
					&& *q != '*' && *q != '/' && *q != ']')
					q++;
			} else
				break;
		} while (*q != ']' && *q != '\0');
		if (*q == ']') {
			p[0] = 'M';
			p[1] = 'O';
			p[2] = 'V';
			*q0 = ' ';
			*q = ' ';
		}
	}

	// LEA文中の「dword」、「word」、「byte」の消去
	if ((q = cwordsrch(p, "DWORD")) != 0) {
		q[4] = ' ';
		goto LEA_space4;
	}
	if ((q = cwordsrch(p, "WORD")) != 0)
		goto LEA_space4;
	if ((q = cwordsrch(p, "BYTE")) != 0) {
LEA_space4:
		q[0] = ' ';
		q[1] = ' ';
		q[2] = ' ';
		q[3] = ' ';
	}
	return;
}

char *wordsrch(char *s, const char *t)
// sの中にtがあるかどうかを調べる
// strstrとの違いは、wordsrchが単語単位で検索することである
{
	char *p = s, c;
	int l = strlen(t);

	for (p = s; (p = strstr(p, t)) != 0; p += l) {

		// 単語の左は語の切れ目か？
		if (p > s) {
			c = p[-1];
			if (c > ' ' && c != ',')
				continue;
		}

		// 単語の右は語の切れ目か？
		c = p[l];
		if (c <= ' ' || c == ',')
			return p;
	}

	return NULL;
}

unsigned char tolower(unsigned char c)
{
	return ('A' <= c && c <= 'Z') ? c + ('a' - 'A') : c;
}

char *cwordsrch(char *s, const char *c)
// 大文字を指定すれば小文字でも探す
{
	char *r, *p, l[100];

	if ((r = wordsrch(s, c)) == NULL) {
		for (p = l; (*p++ = tolower(*c++)) != 0; );
		r = wordsrch(s, l);
	}
	return r;
}

char *strchr(char *d, int c)
{
	while (c != *d) {
		if ('\0' == *d++)
			return NULL;
	}
	return d;
}
