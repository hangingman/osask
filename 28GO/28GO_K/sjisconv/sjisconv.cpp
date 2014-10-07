/* "sjisconv"
	Copyright(C) 2009 H.Kawai

	usage : >sjisconv [-e] [-s] input-file output-file

	-s:ShiftJISÉÇÅ[Éh
*/

#include <guigui01.h>

typedef unsigned char UCHAR;

#define FLAG_E		0
#define FLAG_S		1

#define NULL	0

struct STR_FLAGS {
	UCHAR opt[4];
};

struct stack_alloc {
	UCHAR ibuf[8 * 1024 * 1024];	
	UCHAR obuf[8 * 1024 * 1024];
};

//static UCHAR *convmain(UCHAR *src0, UCHAR *src1, UCHAR *dest0, UCHAR *dest1, struct STR_FLAGS flags);

static inline void escape(UCHAR *s, UCHAR c)
{
	s[0] = '\\';
	s[3] = '0' + (c & 0x07);
	c >>= 3;
	s[2] = '0' + (c & 0x07);
	s[1] = '0' + (c >> 3);
	return;
}

void G01Main()
{
	static unsigned char cmdlin[] = {
		0x86, 0x52,  0x12, 'e', 'u', 'c', 0x20,
		0x13, 's', 'j', 'i', 's', 0x20, 0x88, 0x8c, 0x40	/* 1 3 "sjis" 2 0 1 2 "euc" 2 0 */
	};
	struct stack_alloc *pwork;
	UCHAR *p, *q, *q1, c;
	struct STR_FLAGS flags;

	g01_setcmdlin(cmdlin);
	pwork = jg01_malloc(sizeof (struct stack_alloc));
	flags.opt[FLAG_E] = g01_getcmdlin_flag_s(0);
	flags.opt[FLAG_S] = g01_getcmdlin_flag_s(1);
	if (flags.opt[FLAG_E] + flags.opt[FLAG_S] != 1) {
		g01_putstr0("\"sjisconv\"  Copyright(C) 2009 H.Kawai\n");
		g01_getcmdlin_exit1();
	}
	g01_getcmdlin_fopen_s_0_4(2);
	jg01_fread0f_4(sizeof (pwork->ibuf), pwork->ibuf);

//	src1 = convmain(pwork->ibuf, src1, pwork->obuf, pwork->obuf + sizeof (pwork->obuf), flags);

	p = pwork->ibuf;
	q = pwork->obuf;
	q1 = pwork->obuf + sizeof (pwork->obuf);
	while ((c = *p) != '\0') {
		p++;
		if (q + 8 > q1)
			g01_putstr0_exit1("output filebuf over!");
		if (c <= 0x7f) {
			*q++ = c;
			continue;
		}
		if (flags.opt[FLAG_S]) {
			if (0xa0 <= c && c <= 0xdf) { /* îºäpÇ©Ç» */
escape1:
				escape(q, c);
				q += 4;
				continue;
			}
			if (*p == '\0')
				goto escape1;
			escape(q, c);
			q += 4;
			c = *p++;
			goto escape1;
		}
		if (flags.opt[FLAG_E]) {
			escape(q, c);
			q += 4;
		}
	}

	g01_getcmdlin_fopen_s_3_5(3);
	jg01_fwrite1f_5(q - pwork->obuf, pwork->obuf);
	return;
}
