/* "golib00.c":golib00を.g01化したもの */
/* copyright(C) 2009 川合秀実  KL-01 */

#include <guigui01.h>
#include <string.h>

typedef unsigned char UCHAR;

#define NL			"\n"
#define NULL	0

struct str_obj {
	UCHAR *name;
	UCHAR *file0, *file1;
	int ofs;
};

struct str_label {
	UCHAR *s0;
	UCHAR *s1; /* point to '\0' */
	struct str_obj *obj;
};

struct stack_alloc {
	struct str_label label[32 * 1024];
	struct str_obj objs[16 * 1024];
	UCHAR filebuf[8 * 1024 * 1024];
	UCHAR iobuf[8 * 1024 * 1024];
	UCHAR cmdlin[64 * 1024];
	UCHAR for_align[16];
};

struct str_works {
	struct str_label *label0, *label1, *label;
	struct str_obj *objs0, *objs1, *objs;
	UCHAR *filebuf0, *filebuf1, *filebuf;
	UCHAR *iobuf0, *iobuf1;
	UCHAR *libname, *extname;
	int flags;
};

static void cmdline0(UCHAR *s0, UCHAR *s1, struct str_works *work);
static void libout(struct str_works *work);

void G01Main()
{
	struct stack_alloc *pwork;
	struct str_works works;
	UCHAR *p0;
	int i;

	pwork = jg01_malloc(sizeof (struct stack_alloc));
//	my_getcmdlin0(64 * 1024, p0 = pwork->cmdlin);
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

	p0 = pwork->cmdlin;
	for (i = 1; g01_getcmdlin_str_m0_1(i, 64 * 1024, p0) != 0; i++) {
		if (p0[3] == '=')
			p0[3] = ':';
		cmdline0(p0, p0 + strlen(p0), &works);
	}
	libout(&works);

	return;
}

static void cmdline(UCHAR *s0, UCHAR *s1, struct str_works *work);
static UCHAR *copystr(UCHAR *s0, UCHAR *s1, struct str_works *work);
static void registlabel(struct str_obj *obj, struct str_works *work);
static int getdec(const UCHAR *p);
static int get32l(const UCHAR *p);
static int get32b(const UCHAR *p);

static UCHAR *copystr(UCHAR *s0, UCHAR *s1, struct str_works *work)
{
	UCHAR *r = work->filebuf, *p = r;
	if (r + (s1 - s0) + 1 > work->filebuf1)
		g01_putstr0_exit1("filebuf over!");
	while (s0 < s1)
		*p++ = *s0++;
	*p++ = '\0';
	work->filebuf = p;
	return r;
}

static void cmdline0(UCHAR *s0, UCHAR *s1, struct str_works *work)
{
	UCHAR *t, *p, *q, *r, *s;
	int len = s1 - s0;
	int i, j;
	struct str_obj *obj, *obj0;
	if (len <= 0)
		return;
	if (len > 4 && s0[0] == 'o' && s0[1] == 'p' && s0[2] == 't' && s0[3] == ':' && (s0 <= work->iobuf0 || work->iobuf1 <= s0)) {
		s1 = copystr(&s0[4], s1, work);
		s1 = work->iobuf0 + jg01_fopen01_4_fread1f_4(s1, j = work->iobuf1 - work->iobuf0, work->iobuf0);
		cmdline(work->iobuf0, s1, work);
		return;
	}
	if (len > 4 && s0[0] == 'o' && s0[1] == 'u' && s0[2] == 't' && s0[3] == ':' && work->libname == NULL) {
		work->libname = copystr(&s0[4], s1, work);
		return;
	}
	if (len > 4 && s0[0] == 'e' && s0[1] == 'x' && s0[2] == 't' && s0[3] == ':' && work->extname == NULL) {
		work->extname = copystr(&s0[4], s1, work);
		return;
	}
	if (len == 4 && s0[0] == 'l' && s0[1] == 's' && s0[2] == 't' && s0[3] == ':') {
		work->flags |= 1;
		return;
	}
	obj = work->objs;
	if (obj >= work->objs1) {
too_many_object:
		g01_putstr0_exit1("too many object file!");
	}
	obj->name = copystr(s0, s1, work);
	obj->file0 = work->filebuf;
	i = jg01_fopen01_4_fread1f_4(obj->name, j = work->filebuf1 - obj->file0, obj->file0);
	work->filebuf = obj->file1 = obj->file0 + i;
	t = obj->name + strlen(obj->name);
	while (obj->name < t) {
		if (t[-1] == '/')
			break;
		if (t[-1] == '\\')
			break;
		t--;
	}
	obj->name = t;
	if (*obj->file0 == 'L') {
		/* COFF */
		registlabel(obj, work);
		work->objs++;
		return;
	}
	if (*obj->file0 != '!') {
		g01_putstr0("unknown file type: ");
		g01_putstr0_exit1(obj->name);
	}
	/* ライブラリのロード */
	p = obj->file0;
	s = obj->file1;
	t = &p[0x44];
	obj0 = obj;
	for (j = get32b(t); j > 0; j--) {
		t += 4;
		r = p + get32b(t);
		q = r + 0x3c;
		for (obj = obj0; obj < work->objs; obj++) {
			if (q == obj->file0)
				goto skip;
		}
		while (r < q + 15 && *r != '/')
			r++;
		obj->name = copystr(q - 0x3c, r, work);
		if (*q != 'L')
			g01_putstr0_exit1("Internal error : unknown library format (2)");
		obj->file0 = q;
		obj->file1 = obj->file0 + getdec(&q[-0x0c]);
		if (obj->file1 > s)
			g01_putstr0_exit1("Internal error : unknown library format (3)");
		registlabel(obj, work);
		work->objs = ++obj;
skip:
		if (obj >= work->objs1)
			goto too_many_object;
	}
	return;
}

static void cmdline(UCHAR *s0, UCHAR *s1, struct str_works *work)
{
	UCHAR *t;
	for (;;) {
		while (s0 < s1 && *s0 <= ' ')
			s0++;
		if (s1 <= s0)
			break;
		t = s0;
		while (t < s1 && *t > ' ')
			t++;
		cmdline0(s0, t, work);
		s0 = t;
	}
	return;
}

static int get32l(const UCHAR *p)
{
	return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

static int get32b(const UCHAR *p)
{
	return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

static void registlabel(struct str_obj *obj, struct str_works *work)
{
	UCHAR *q = obj->file0 + get32l(&obj->file0[0x08]), *s;
	UCHAR *s0 = obj->file0 + get32l(&obj->file0[0x08])
				+ get32l(&obj->file0[0x0c]) * 0x12;
	UCHAR c;
	int i, j, sec, k, m;
	struct str_label *l;
	for (i = get32l(&obj->file0[0x0c]); i > 0; i -= j, q += j * 0x12) {
		j = q[0x11] /* numaux */ + 1;
		sec = q[0x0c];
		if (sec == 0)
			continue;
		if (sec > 0x80)
			continue;
		if (q[0x10] != 0x02 /* && q[0x10] != 0x06 */)
			continue;
		s = s0 + get32l(&q[0x04]);
		if (q[0x00] | q[0x01] | q[0x02] | q[0x03]) {
			s = q;
			if (q[7])
				s = copystr(&q[0], &q[8], work);
		}
		for (k = 0; s[k]; k++);
		for (l = work->label0; l < work->label; l++) {
			if (l->s1 - l->s0 == k) {
				c = 0;
				for (m = 0; m < k; m++)
					c |= s[m] ^ l->s0[m];
				if (c == 0) {
					g01_putstr0("redefine symbol: ");
					g01_putstr0_exit1(s);
				}
			}
		}
		if (l >= work->label1)
			g01_putstr0_exit1("too many symbols!");
		work->label++;
		l->s0 = s;
		l->s1 = s + k;
		l->obj = obj;
	}
}

UCHAR *iobuf_p;
UCHAR *iobuf_p1;

static void putbuf(int len, const UCHAR *data)
{
	if (iobuf_p + len > iobuf_p1)
		g01_putstr0_exit1("iobuf over!");
	while (len--)
		*iobuf_p++ = *data++;
	return;
}

static UCHAR *puttag()
{
	UCHAR *p = iobuf_p;
	static UCHAR tag[60] =
		"/               " "0               "
		"        0       " "0         `\n";
	putbuf(60, tag);
	return p;
}

static void putdec(UCHAR *p, int i)
{
	UCHAR dec[10], *q = dec + 10;
	do {
		*--q = (i % 10) + '0';
	} while (i /= 10);
	do {
		*p++ = *q++;
	} while (q < dec + 10);
	return;
}

static void put32b(int i)
{
	UCHAR tmp[4];
	tmp[0] = (i >> 24) & 0xff;
	tmp[1] = (i >> 16) & 0xff;
	tmp[2] = (i >>  8) & 0xff;
	tmp[3] =  i        & 0xff;
	putbuf(4, tmp);
}

static void put32l(int i)
{
	UCHAR tmp[4];
	tmp[0] =  i        & 0xff;
	tmp[1] = (i >>  8) & 0xff;
	tmp[2] = (i >> 16) & 0xff;
	tmp[3] = (i >> 24) & 0xff;
	putbuf(4, tmp);
}

static void put16l(int i)
{
	UCHAR tmp[2];
	tmp[0] =  i        & 0xff;
	tmp[1] = (i >>  8) & 0xff;
	putbuf(2, tmp);
}

static void libout(struct str_works *work)
{
	int len, pass, i, j;
	UCHAR *p;
	iobuf_p1 = work->iobuf1;
	if (work->objs0 == work->objs) {
		g01_putstr0_exit1(	"usage : >golib00 [obj/lib-files] [commnad]" NL
							"   command : out=lib-file   ext=obj-file" NL
							"             lst=           ext=*  (all-obj)"
		);
	}
	if (work->libname) {
		for (pass = 0; pass < 2; pass++) {
			iobuf_p = work->iobuf0;
			putbuf(0x08, "!<arch>\n");

			p = puttag();
			put32b(len = work->label - work->label0);
			for (i = 0; i < len; i++)
				put32b(work->label0[i].obj->ofs);
			for (i = 0; i < len; i++)
				putbuf(work->label0[i].s1 - work->label0[i].s0 + 1, work->label0[i].s0);
			if ((iobuf_p - work->iobuf0) & 1)
				putbuf(1, "\0");
			putdec(p + 48, iobuf_p - p - 60);

			p = puttag();
			put32l(len = work->objs - work->objs0);
			for (i = 0; i < len; i++)
				put32l(work->objs0[i].ofs);
			put32l(len = work->label - work->label0);
			/* 本来はソートして出力するのだが、手抜きでソートしていない */
			for (i = 0; i < len; i++)
				put16l(work->label0[i].obj - work->objs0 + 1);
			for (i = 0; i < len; i++)
				putbuf(work->label0[i].s1 - work->label0[i].s0 + 1, work->label0[i].s0);
			if ((iobuf_p - work->iobuf0) & 1)
				putbuf(1, "\0");
			putdec(p + 48, iobuf_p - p - 60);

			p = puttag();
			p[1] = '/';
			len = work->objs - work->objs0;
			for (i = 0; i < len; i++) {
				work->objs0[i].ofs = iobuf_p - work->iobuf0;
				p = iobuf_p;
				putbuf(j = strlen(work->objs0[i].name), work->objs0[i].name);
				if (j > 15) {
					iobuf_p = p + 15;
					j = 15;
				}
				putbuf(16 - j, "/               ");
				putbuf(44, "0                       0       0         `\n");
				putbuf(work->objs0[i].file1 - work->objs0[i].file0, work->objs0[i].file0);
				if ((iobuf_p - work->iobuf0) & 1)
					putbuf(1, "\0");
				putdec(p + 48, iobuf_p - p - 60);
			}
		}
		jg01_fopen19_5_fwrite1f_5(work->libname, iobuf_p - work->iobuf0, work->iobuf0)
	}
	if (work->flags & 1) {
		len = work->objs - work->objs0;
		pass = work->label - work->label0;
		for (i = 0; i < len; i++) {
			g01_putstr0(work->objs0[i].name);
			g01_putc('\n');
			for (j = 0; j < pass; j++) {
				if (work->label0[j].obj == &work->objs0[i]) {
					g01_putc('\t');
					g01_putstr0(work->label0[j].s0);
					g01_putc('\n');
				}
			}
			g01_putc('\n');
		}
	}
	if (work->extname) {
		len = work->objs - work->objs0;
		if (work->extname[0] == '*' && work->extname[1] == '\0') {
			for (i = 0; i < len; i++)
				jg01_fopen19_5_fwrite1f_5(work->objs0[i].name, work->objs0[i].file1 - work->objs0[i].file0, work->objs0[i].file0);
			goto fin;
		}
		for (i = 0; i < len; i++) {
			for (j = 0; work->extname[j] == work->objs0[i].name[j]; j++) {
				if (work->extname[j] == 0) {
					jg01_fopen19_5_fwrite1f_5(work->extname, work->objs0[i].file1 - work->objs0[i].file0, work->objs0[i].file0);
					goto fin;
				}
			}
		}
	}
fin:
	return;
}

static int getdec(const UCHAR *p)
{
	int i = 0;
	while (*p == ' ')
		p++;
	while ('0' <= *p && *p <= '9')
		i = i * 10 + (*p++ - '0');
	return i;
}
