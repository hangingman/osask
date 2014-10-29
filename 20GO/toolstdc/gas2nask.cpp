/* "gas2nask" (stdc版)
	Copyright(C) 2003 H.Kawai

	usage : >gas2nask [-a] input-file output-file

	-a:.textのbalign2を削除
*/

#include <cstdlib>
#include <cstdio>
#include <cstring>

typedef unsigned int UINT;

static char *dest0_, *dest1_;

#define FLAG_t		0
#define FLAG_d		1
#define FLAG_b		2
#define FLAG_T		3
#define FLAG_D		4
#define FLAG_B		5
#define FLAG_a		6

#define	NL		"\n"
#define	LEN_NL		1

struct STR_FLAGS {
	char opt[8];
};

struct stack_alloc {
	char ibuf[8 * 1024 * 1024];	
	char obuf[8 * 1024 * 1024];
};

char *readfile(const char *name, char *b0, const char *b1);
static void errout(const char *s);
static void errout_s_NL(const char *s, const char *t);
static char *convmain(char *src0, char *src1, char *dest0, char *dest1, struct STR_FLAGS flags);
unsigned int GO_strlen(const char *s);
int GOLD_write_t(const char* name, int len, const char* p0);

static char errflag = 0;

static char *param_p[6];

#include "../drv_stdc/others.hpp"

int main(int argc, char **argv)
{
	struct stack_alloc *pwork;
	char *p0, *f, *src1, *filename, i = 0;
	struct STR_FLAGS flags;
	int j;

	pwork = (struct stack_alloc *) malloc(sizeof (struct stack_alloc));

	for (j = 0; j < 8; j++)
		flags.opt[j] = 0;

	while (--argc) {
		p0 = *++argv;
		if (*p0 == '-') {
			while (*++p0) {
				if (*p0 == 'a')
					flags.opt[FLAG_a] = 1;
				if (*p0 == 'd') {
					j = FLAG_d;
	getparam:
					do {
						p0++;
					} while (*p0 != '\0' && *p0 <= ' ');
					if (*p0 == '\0') {
						p0 = *++argv;
						argc--;
						if (argc > 1)
							goto getparam;
						flags.opt[j] = 0;
						goto skip;
					}
					param_p[j] = p0;
					while ('0' <= *p0 && *p0 <= '9')
						p0++;
					flags.opt[j] = p0 - param_p[j];
					p0--; /* 次のループのため */
					continue;
				}
				if (*p0 == 'b') {
					j = FLAG_b;
					goto getparam;
				}
				if (*p0 == 't') {
					j = FLAG_t;
					goto getparam;
				}
				if (*p0 == 'D') {
					j = FLAG_D;
					goto getparam;
				}
				if (*p0 == 'B') {
					j = FLAG_B;
					goto getparam;
				}
				if (*p0 == 'T') {
					j = FLAG_T;
					goto getparam;
				}
			}
		} else {
			filename = p0;
			if (i == 0)
				src1 = readfile(filename, pwork->ibuf, pwork->ibuf + sizeof (pwork->ibuf));
			i++;
		}
	}
skip:
	if (i != 2) {
		errout("\"gas2nask\"  Copyright(C) 2003 H.Kawai" NL
			"usage : >gas2nask [-a] input-file output-file" NL);
	}
	src1 = convmain(pwork->ibuf, src1, pwork->obuf, pwork->obuf + sizeof (pwork->obuf), flags);
	if (src1 == NULL)
		errout("output filebuf over!" NL);
	if (GOLD_write_t(filename, src1 - pwork->obuf, pwork->obuf))
		errout_s_NL("can't write file: ", filename);

	return errflag;
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

void msgout0(int len, char *s)
{
	GOLD_write_t(NULL, len, s);
	return;
}

#include "../drv_stdc/msgout_c.cpp"
#include "../drv_stdc/wfile_t.cpp"
#include "../funcs/gostrlen.cpp"
#include "../funcs/m_gasnas.cpp"
