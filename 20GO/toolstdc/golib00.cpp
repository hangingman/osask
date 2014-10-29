/* "golib00.c":golib00wを標準ライブラリ仕様にしたもの */
/* copyright(C) 2003 川合秀実  KL-01 */

#include <cstdio>
#include <cstdlib>

#define NL			"\n"

struct str_obj {
	char *name;
	char *file0, *file1;
	int ofs;
};

struct str_label {
	char *s0;
	char *s1; /* point to '\0' */
	struct str_obj *obj;
};

struct stack_alloc {
	struct str_label label[32 * 1024];
	struct str_obj objs[16 * 1024];
	char filebuf[8 * 1024 * 1024];
	char iobuf[8 * 1024 * 1024];
	char for_align[16];
};

struct str_works {
	struct str_label *label0, *label1, *label;
	struct str_obj *objs0, *objs1, *objs;
	char *filebuf0, *filebuf1, *filebuf;
	char *iobuf0, *iobuf1;
	char *libname, *extname;
	int flags;
};

static void cmdline0(char *s0, char *s1, struct str_works *work);
static void libout(struct str_works *work);
unsigned int GO_strlen(const char* s);
int GOLD_write_t(const char* name, int len, const char* p0);

#include "../drv_stdc/others.hpp"

int main(int argc, char** argv)
{
	struct stack_alloc *pwork;
	struct str_works works;
	int i;

	pwork = (struct stack_alloc *) malloc(sizeof (struct stack_alloc));
	works.label = works.label0 = pwork->label;
	works.label1 = &pwork->label[sizeof (pwork->label) / sizeof(*pwork->label)];
	works.objs = works.objs0 = pwork->objs;
	works.objs1 = &pwork->objs[sizeof (pwork->objs) / sizeof(*pwork->objs)];
	works.filebuf = works.filebuf0 = pwork->filebuf;
	works.filebuf1 = &pwork->filebuf[sizeof (pwork->filebuf) / sizeof(*pwork->filebuf)];
	works.iobuf0 = pwork->iobuf;
	works.iobuf1 = &pwork->iobuf[sizeof (pwork->iobuf) / sizeof(*pwork->iobuf)];
	works.libname = works.extname = NULL;
	works.flags = 0;

	for (i = 1; i < argc; i++) {
		char* p0 = argv[i];
		cmdline0(p0, p0 + GO_strlen(p0), &works);
	}
	libout(&works);

	return 0;
}

#include "../drv_stdc/msgout_c.cpp"
#include "../drv_stdc/wfile_b.cpp"
#include "../drv_stdc/wfile_t.cpp"
#include "../funcs/gostrlen.cpp"
#include "../funcs/m_golib.cpp"
