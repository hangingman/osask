/* copyright(C) 2008 H.Kawai (under KL-01). */

#include <guigui01.h>
#include <string.h>
#include "go_lib.h"
//#include <stdio.h>
//#include <stdlib.h>

#define SIZ_STDOUT			(16 * 1024)
#define SIZ_STDERR			(16 * 1024)
#define SIZ_WORK			(8 * 1024 * 1024)
#define SIZ_SYSWRK			(1024 * 1024)

#define	MAX_SRCSIZ		(2 * 1024 * 1024)
#define	MAX_TMPSIZ		(4 * 1024 * 1024)
#define	MAX_BINSIZ		(2 * 1024 * 1024)
#define	MAX_LSTSIZ		(4 * 1024 * 1024)

//typedef unsigned char UCHAR;

#define	NL			"\n"

typedef struct GO_STR_FILE {
	UCHAR *p0, *p1, *p;
	int dummy;
} GO_FILE;

GO_FILE GO_stdin, GO_stdout, GO_stderr;
struct GOL_STR_MEMMAN GOL_memman /* , GOL_sysman */;
int GOL_retcode;

UCHAR *GOL_work0;
int main1(UCHAR *src0);
void GOL_sysabort(unsigned char termcode);
void *GOL_memmaninit(struct GOL_STR_MEMMAN *man, size_t size, void *p);

struct bss_alloc {
	UCHAR _stdout[SIZ_STDOUT];
	UCHAR _stderr[SIZ_STDERR];
//	UCHAR syswrk[SIZ_SYSWRK];
	UCHAR work[SIZ_WORK];
	UCHAR work1[MAX_SRCSIZ + MAX_TMPSIZ + MAX_BINSIZ + MAX_LSTSIZ];
	UCHAR cmdlin[64 * 1024];
};

#define	CMDLIN_IN		0
#define	CMDLIN_OUT		1
#define	CMDLIN_LST		2

static UCHAR cmdusg[] = {
	0x86, 0x50,
		0x88, 0x8d, 
		0x12, 'l', 's', 't', 0x08, 0x64, 'l', 'i', 's', 't', '-', 0x02
};

void G01Main()
{
	struct bss_alloc *bss0;

	g01_setcmdlin(cmdusg);

	bss0 = jg01_malloc(sizeof (struct bss_alloc));

	GO_stdout.p0 = GO_stdout.p = bss0->_stdout;
	GO_stdout.p1 = GO_stdout.p0 + SIZ_STDOUT;
	GO_stdout.dummy = ~0;
	GO_stderr.p0 = GO_stderr.p = bss0->_stderr;
	GO_stderr.p1 = GO_stderr.p0 + (SIZ_STDERR - 128); /* わざと少し小さくしておく */
	GO_stderr.dummy = ~0;
//	GOL_memmaninit(&GOL_sysman, SIZ_SYSWRK, bss0->syswrk);
	GOL_memmaninit(&GOL_memman, SIZ_WORK, GOL_work0 = bss0->work);

	GOL_retcode = main1(bss0->work1);
	/* バッファを出力 */
	GOL_sysabort(0);
}

UCHAR *nask(UCHAR *src0, UCHAR *src1, UCHAR *dest0, UCHAR *dest1);
UCHAR *LL(UCHAR *src0, UCHAR *src1, UCHAR *dest0, UCHAR *dest1);
UCHAR *output(UCHAR *src0, UCHAR *src1, UCHAR *dest0, UCHAR *dest1, UCHAR *list0, UCHAR *list1);
void *GO_memcpy(void *s, const void *ct, unsigned int n);

void errmsgout(const UCHAR *s)
{
	int l =	strlen(s);
	char flag = 0;
	GO_FILE *stream = &GO_stderr;
	if (l >= stream->p1 - stream->p) {
		l = stream->p1 - stream->p;
		flag++;
	}
	if (l > 0) {
		memcpy(stream->p, s, l);
		stream->p += l;
	}
	if (flag)
		GOL_sysabort(3 /* GO_TERM_ERROVER */);
	return;
}

int nask_errors = 0;

int main1(UCHAR *src0)
{
	UCHAR *src1, *dest0, *dest1;
	UCHAR *tmp0, *tmp1, *list0, *list1;
	int len;

	tmp0 = src0 + MAX_SRCSIZ;
	dest0 = tmp0 + MAX_TMPSIZ;
	list0 = dest0 + MAX_BINSIZ;

	g01_getcmdlin_fopen_s_0_4(CMDLIN_IN);
	len = jg01_fread1f_4(MAX_SRCSIZ, src0);
	src1 = src0 + len;

	list1 = nask(src0, src1, list0, list0 + MAX_LSTSIZ);
	if (list1 == NULL) {
over_listbuf:
		errmsgout("NASK : LSTBUF is not enough" NL);
		return 19;
	}

	tmp1 = LL(list0, list1, tmp0, tmp0 + MAX_TMPSIZ);
	if (tmp1 == NULL) {
//over_tmpbuf:
		errmsgout("NASK : TMPBUF is not enough" NL);
		return 19;
	}

	dest1 = output(tmp0, tmp1, dest0, dest0 + MAX_BINSIZ, list0, list0 + MAX_LSTSIZ - 2);
	if (dest1 == NULL) {
		errmsgout("NASK : BINBUF is not enough" NL);
		return 19;
	}
	if (g01_getcmdlin_fopen_o_3_5(CMDLIN_LST) != 0) {
		tmp1 = list0;
		if (list0[MAX_LSTSIZ - 1])
			goto over_listbuf;
		while (*tmp1 != '\0' && tmp1 < list0 + MAX_LSTSIZ)
			tmp1++;
		jg01_fwrite1f_5(tmp1 - list0, list0);
	}
	if (g01_getcmdlin_fopen_o_3_5(CMDLIN_OUT) != 0) {
		if (dest1 < dest0)
			dest1 = dest0;
		jg01_fwrite1f_5(dest1 - dest0, dest0);
	}

	if (nask_errors) {
//		GO_spritf(src0, "NASK : %d errors." NL, nask_errors);
//		errmsgout(src0);
		UCHAR strbuf[16];
		len = nask_errors;
		errmsgout("NASK : ");
		src1 = &strbuf[15];
		*src1 = '\0';
		do {
			*--src1 = (len % 10) + '0';
		} while (len /= 10);
		errmsgout(src1);
		errmsgout(" errors." NL);
		return 1;
	}

	return 0;
}

void GOL_sysabort(unsigned char termcode)
{
	static char *termmsg[] = {
		"",
		"[TERM_WORKOVER]\n",
		"[TERM_OUTOVER]\n",
		"[TERM_ERROVER]\n",
		"[TERM_BUGTRAP]\n",
		"[TERM_SYSRESOVER]\n",
		"[TERM_ABORT]\n"
	};

	GO_stderr.p1 += 128; /* 予備に取っておいた分を復活 */
	/* バッファを出力 */
	if (termcode <= 6)
		errmsgout(termmsg[termcode]);
	if (GO_stderr.p > GO_stderr.p0)
		g01_putstr1(GO_stderr.p - GO_stderr.p0, GO_stderr.p0);
	if (termcode == 0) {
		if (GOL_retcode == 0)
			g01_exit_success();
		else
			g01_exit_failure_int32(GOL_retcode);
	}
	g01_exit_failure_int32(1);
}
