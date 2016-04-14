/* copyright(C) 2003 H.Kawai (under KL-01). */

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include "ll.hpp"
#include "libnask.hpp"
#include "go_string.hpp"
#include "../drv_stdc/others.hpp"

static constexpr size_t SIZ_STDOUT = (16 * 1024);
static constexpr size_t SIZ_STDERR = (16 * 1024);
static constexpr size_t SIZ_WORK   = (8 * 1024 * 1024);
static constexpr size_t SIZ_SYSWRK = (1024 * 1024);
static constexpr size_t MAX_SRCSIZ = (2 * 1024 * 1024);
static constexpr size_t MAX_TMPSIZ = (4 * 1024 * 1024);
static constexpr size_t MAX_BINSIZ = (2 * 1024 * 1024);
static constexpr size_t MAX_LSTSIZ = (4 * 1024 * 1024);

typedef unsigned char UCHAR;
#define	NL			"\n"

typedef struct GO_STR_FILE {
	UCHAR *p0, *p1, *p;
	int dummy;
} GO_FILE;

extern GO_FILE GO_stdin, GO_stdout, GO_stderr;
extern struct GOL_STR_MEMMAN GOL_memman, GOL_sysman;
int GOL_retcode;

UCHAR *GOL_work0;
UCHAR *LL(UCHAR *src0, UCHAR *src1, UCHAR *dest0, UCHAR *dest1);
UCHAR *nask(UCHAR *src0, UCHAR *src1, UCHAR *dest0, UCHAR *dest1);
int before_nask_process(int argc, UCHAR **argv, UCHAR *src0);
int nask_main(int argc, UCHAR **argv);
void *GOL_memmaninit(struct GOL_STR_MEMMAN *man, size_t size, void *p);
void GOL_sysabort(unsigned char termcode);
void errmsgout(const UCHAR* s);
void errmsgout(const char* s);
void errmsgout_s_NL(const UCHAR *msg, const UCHAR *s);
void errmsgout_s_NL(const char* msg, const char* s);

struct bss_alloc {
	UCHAR _stdout[SIZ_STDOUT];
	UCHAR _stderr[SIZ_STDERR];
	UCHAR work[SIZ_WORK];
	UCHAR work1[MAX_SRCSIZ + MAX_TMPSIZ + MAX_BINSIZ + MAX_LSTSIZ];
};

// メイン関数の定義は上書きできない
int main(int argc, char** argv)
{
	LOG_DEBUG("nask argc: %d, argv:%s\n", argc, dump_argv(argv).c_str());
	nask_main(argc, reinterpret_cast<UCHAR**>(argv));
}

int nask_main(int argc, UCHAR **argv)
{
	std::unique_ptr<struct bss_alloc> bss0(new bss_alloc());
	GO_stdout.p0 = GO_stdout.p = bss0->_stdout;
	GO_stdout.p1 = GO_stdout.p0 + SIZ_STDOUT;
	GO_stdout.dummy = ~0;
	GO_stderr.p0 = GO_stderr.p = bss0->_stderr;
	GO_stderr.p1 = GO_stderr.p0 + (SIZ_STDERR - 128); /* わざと少し小さくしておく */
	GO_stderr.dummy = ~0;
	GOL_memmaninit(&GOL_memman, SIZ_WORK, GOL_work0 = bss0->work);

	GOL_retcode = before_nask_process(argc, argv, bss0->work1);
	LOG_DEBUG("nask GOL_retcode: %d", GOL_retcode);
	/* バッファを出力 */
	GOL_sysabort(0);
	return 0; /* ダミー */
}

// なんでこういうことするかなあ？コードを再利用する場合、ライブラリを作るべきですよ？
#include "../drv_stdc/others.cpp"
#include "../drv_stdc/wfile_b.cpp"
#include "../drv_stdc/wfile_t.cpp"

void errmsgout_s_NL(const char* msg, const UCHAR* s)
{
	return errmsgout_s_NL(reinterpret_cast<const UCHAR*>(msg), s);
}

void errmsgout(const char* s)
{
	return errmsgout(reinterpret_cast<const UCHAR*>(s));
}

void errmsgout(const UCHAR *s)
{
	int l =	GO_strlen(s);
	char flag = 0;
	GO_FILE *stream = &GO_stderr;
	if (l >= stream->p1 - stream->p) {
		l = stream->p1 - stream->p;
		flag++;
	}
	if (l > 0) {
		std::cerr << s << std::endl;
	}
	if (flag) {
		GOL_sysabort(3 /* GO_TERM_ERROVER */);
	}
	return;
}

void errmsgout_s_NL(const UCHAR *msg, const UCHAR *s)
{
	std::cerr << msg << s << std::endl;
	return;
}

int before_nask_process(int argc, UCHAR **argv, UCHAR *src0)
{
	UCHAR *src1, *dest0, *dest1;
	UCHAR *tmp0, *tmp1, *list0, *list1;
	int len;
	int nask_errors = 0;

	tmp0 = src0 + MAX_SRCSIZ;
	dest0 = tmp0 + MAX_TMPSIZ;
	list0 = dest0 + MAX_BINSIZ;
	if (argc < 2 || argc > 4) {
		errmsgout("usage : >nask source [object/binary] [list]");
		return 16;
	}
	len = GOLD_getsize(argv[1]);
	LOG_DEBUG("GOLD_getsize(file): %d\n", len);
	if (len == -1) {
		errmsgout_s_NL("NASK : can't open ", argv[1]);
		return 17;
	}
	if (len > MAX_SRCSIZ) {
		errmsgout_s_NL("NASK : too large ", argv[1]);
		return 17;
	}
	if (GOLD_read(argv[1], len, src0)) {
		errmsgout_s_NL("NASK : can't read ", argv[1]);
		return 17;
	}
	src1 = src0 + len;

	LOG_DEBUG("call nask...\n");
	list1 = nask(src0, src1, list0, list0 + MAX_LSTSIZ);
	LOG_DEBUG("nask loaded src: %s\n", dump_ptr("src0", src0).c_str());

	if (list1 == NULL) {
over_listbuf:
		errmsgout("NASK : LSTBUF is not enough" NL);
		return 19;
	}

	LOG_DEBUG("call LL...\n");
	tmp1 = LL(list0, list1, tmp0, tmp0 + MAX_TMPSIZ);
	LOG_DEBUG("LL loaded tmp0: %s\n", dump_ptr("tmp0", tmp0).c_str());

	if (tmp1 == NULL) {
over_tmpbuf:
		errmsgout("NASK : TMPBUF is not enough" NL);
		return 19;
	}

	LOG_DEBUG("call output...\n");
	LOG_DEBUG("output loaded dest0: %s\n", dump_ptr("dest0", dest0).c_str());
	dest1 = output(tmp0, tmp1, dest0, dest0 + MAX_BINSIZ, list0, list0 + MAX_LSTSIZ - 2, nask_errors);
	LOG_DEBUG("output modified dest0: %s\n", dump_ptr("dest0", dest0).c_str());
	LOG_DEBUG("output generated dest1: %s\n", dump_ptr("dest1", dest1).c_str());

	if (dest1 == NULL) {
		errmsgout("NASK : BINBUF is not enough" NL);
		return 19;
	}
	if (argc == 4) {
		tmp1 = list0;
		if (list0[MAX_LSTSIZ - 1])
			goto over_listbuf;
		while (*tmp1 != '\0' && tmp1 < list0 + MAX_LSTSIZ)
			tmp1++;
		if (GOLD_write_t(argv[3], tmp1 - list0, list0)) {
			errmsgout("NASK : list output error" NL);
			return 20;
		}
	}
	if (argc >= 3) {
		if (dest1 < dest0) {
			dest1 = dest0;
		}
		if (GOLD_write_b(argv[2], dest1 - dest0, dest0)) {
			errmsgout("NASK : object/binary output error" NL);
			return 21;
		}
	}

	if (nask_errors) {
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
	static const char *termmsg[] = {
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
	if (GO_stderr.p - GO_stderr.p0)
		GOLD_write_t(NULL, GO_stderr.p - GO_stderr.p0, GO_stderr.p0);
	GOLD_exit((termcode == 0) ? GOL_retcode : 1);
}
