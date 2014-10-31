/* "naskcnv0"
	Copyright(C) 2003 H.Kawai

	usage : >naskcnv0 [-l] [-s] [-w] input-file output-file

	-l:leaをmovに変換
	-s:余計なdword、word、byteを削除
*/

#include <cstdio>
#include <cstdlib>
#include <cstring>

char *GO_strstr(const char *cs, const char *ct);
unsigned int GO_strlen(const char *cs);
char *GO_strchr(const char *cs, int c);
int GOLD_write_t(const char* name, int len, const char* p0);

typedef unsigned int UINT;

#define	NL		"\n"
#define	LEN_NL		1

static char *dest0_, *dest1_;

#define FLAG_W		0
#define FLAG_L		1
#define FLAG_S		2

struct STR_FLAGS {
	char opt[3];
};

struct stack_alloc {
	char ibuf[8 * 1024 * 1024];	
	char obuf[8 * 1024 * 1024];
};

char* readfile(const char *name, char *b0, const char *b1);
static void errout(const char *s);
static void errout_s_NL(const char *s, const char *t);
static char *convmain(char *src0, char *src1, char *dest0, char *dest1, struct STR_FLAGS flags);

#include "../drv_stdc/others.hpp"

int main(int argc, char **argv)
{
	struct stack_alloc *pwork;
	char *p0, *filename, *src1, i = 0;
	struct STR_FLAGS flags;
	int j;

	pwork = (struct stack_alloc *) malloc(sizeof (struct stack_alloc));

	for (j = 0; j < 3; j++)
		flags.opt[j] = 0;

	while (--argc) {
		p0 = *++argv;
		if (*p0 == '-') {
			do {
				p0++;
				if (*p0 == 'w')
					flags.opt[FLAG_W] = 1;
				if (*p0 == 'l')
					flags.opt[FLAG_L] = 1;
				if (*p0 == 's')
					flags.opt[FLAG_S] = 1;
			} while (*p0 > ' ');
		} else {
			filename = p0;
			if (i == 0)
				src1 = readfile(filename, pwork->ibuf, pwork->ibuf + sizeof (pwork->ibuf));
			i++;
		}
	}
	if (i != 2) {
		errout("\"naskcnv0\"  Copyright(C) 2003 H.Kawai" NL
			"usage : >naskcnv0 [-l] [-s] [-w] input-file output-file" NL
		);
	}
	src1 = convmain(pwork->ibuf, src1, pwork->obuf, pwork->obuf + sizeof (pwork->obuf), flags);
	if (src1 == NULL)
		errout("output filebuf over!" NL);
	if (GOLD_write_t(filename, src1 - pwork->obuf, pwork->obuf))
		errout_s_NL("can't write file: ", filename);

	return 0;
}

static void output(UINT l, const char *s)
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

#include "../drv_stdc/msgout_c.cpp"
#include "../drv_stdc/wfile_t.cpp"
#include "../funcs/m_naskcv.cpp"
