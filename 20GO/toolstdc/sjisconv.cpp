/* "sjisconv"
	Copyright(C) 2003 H.Kawai

	usage : >sjisconv [-e] [-s] input-file output-file

	-s:ShiftJISモード
*/

#include <cstdio>
#include <cstdlib>

#define NL		"\n"
#define FLAG_E		0
#define FLAG_S		1

struct STR_FLAGS {
	char opt[3];
};

struct stack_alloc {
	char ibuf[8 * 1024 * 1024];	
	char obuf[8 * 1024 * 1024];
};

char* readfile(const char *name, char *b0, const char *b1);
unsigned int GO_strlen(const char *s);
void errout(const char *s);
void errout_s_NL(const char *s, const char *t);
static char *convmain(char *src0, char *src1, char *dest0, char *dest1, struct STR_FLAGS flags);
int GOLD_write_t(const char* name, int len, const char* p0);

#include "../drv_stdc/others.hpp"

int main(int argc, char **argv)
{
	struct stack_alloc *pwork;
	char *p0, *filename, *src1, i = 0;
	struct STR_FLAGS flags;

	pwork = (struct stack_alloc *) malloc(sizeof (struct stack_alloc));

	flags.opt[FLAG_E] = flags.opt[FLAG_S] = 0;

	while (--argc) {
		p0 = *++argv;
		if (*p0 == '-') {
			do {
				p0++;
				if (*p0 == 's')
					flags.opt[FLAG_S] = 1;
			} while (*p0 > ' ');
		} else {
			filename = p0;
			if (i == 0)
				src1 = readfile(filename, pwork->ibuf, pwork->ibuf + sizeof (pwork->ibuf));
			i++;
		}
	};
	if (i != 2) {
		errout("\"sjisconv\"  Copyright(C) 2003 H.Kawai" NL
			"usage : >sjisconv [-e] [-s] input-file output-file" NL
		);
	}
	src1 = convmain(pwork->ibuf, src1, pwork->obuf, pwork->obuf + sizeof (pwork->obuf), flags);
	if (src1 == NULL)
		errout("output filebuf over!" NL);
	if (GOLD_write_t(filename, src1 - pwork->obuf, pwork->obuf))
		errout_s_NL("can't write file: ", filename);

	return 0;
}

#include "../drv_stdc/msgout_c.cpp"
#include "../drv_stdc/wfile_t.cpp"
#include "../funcs/gostrlen.cpp"
#include "../funcs/m_sjiscv.cpp"
