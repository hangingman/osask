/* Executer For Guigui01 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char UCHAR;

#if (!defined(USE_WIN32))
	#define USE_WIN32	0
#endif

#if (!defined(USE_POSIX))
	#define USE_POSIX	0
#endif

#if (USE_WIN32)
	#include <windows.h>
	#include <conio.h>
	#include <time.h>
#endif

#if (USE_POSIX)
	#if (!defined(_POSIX_C_SOURCE))
		#define _POSIX_C_SOURCE	199309
	#endif
	#include <time.h>
	#include <termios.h>
	#include <signal.h>
#endif

#define DEBUG_MODE			0

#define READ_EH4_BUFLEN		16

struct READ_EH4 {	/* 構造体名、関数名はEH4になっているが実際はGH4 */
	UCHAR buf[READ_EH4_BUFLEN], *p;
	int r, w, l;
};

struct READ_API {
	struct READ_EH4 reh4, reh4_b;
	int rep, mod, mod0, flags, *reg32, *st0;
	char gosub, term;
};

struct STR_SLOT {
	int t, p[3];
};

struct STR_SLOT slot[64];

void init_reh4(struct READ_EH4 *reh4, UCHAR *p);
int getnum_u(struct READ_EH4 *reh4, char f);
int debug_dump_tag(struct READ_EH4 *reh4, int trm);
void dump_g01(struct READ_EH4 *reh4, int trm);
void skip_tags(struct READ_EH4 *reh4);
int load_g01(struct READ_EH4 *reh4, UCHAR *p1, UCHAR *code0, UCHAR *work0, int *esi0);
void init_ra(struct READ_API *ra, UCHAR *p);
int getnum_api(struct READ_API *ra, char f);

void start_app(int *esi);
void asm_api();
void asm_end();
void c_api();
int rjc(UCHAR *p0, UCHAR *p1, int ofs0, int ofs, int ofs1, int mode);
int inkey();
void consctrl1(int x, int y);
void consctrl2(int f, int b);
void consctrl3();
void consctrl4(int x, int y);

#if (USE_POSIX)
void end_app(void);
void signal_handler(int sig);
static struct termios save_term;
static struct termios temp_term;
#endif

int main_argc, main_flags;
UCHAR **main_argv;

int tek_getsize(UCHAR *p);
int tek_decomp(unsigned char *p, char *q, int size);

#if (defined(USE_INCLUDE_G01))
	extern UCHAR inclg01[5];
#endif

void esi20()
{
	puts("use CALL([ESI+20])!");
	exit(1);
}

struct FUNC06STR_SUB00 {
	UCHAR tag[32], usg[32], def[32], typ, flg, dmy_c[2];
	int def_int, dmy_i[6];
	/* 0:file-path, 1:int, 2:flag, 3:str */
};

struct FUNC06STR {
	struct FUNC06STR_SUB00 usg[64];
	int def_arg0, flags, usgs;
	UCHAR *argv0, argv_f[256], argv_all[64 * 1024];
};

static struct FUNC06STR *func06str;

void func06_init();
void func06_putusage();
void func06_setup();
int my_strtol(UCHAR *p, UCHAR **pp);
int getreg32idx(struct READ_API *ra);
int read_reg32(struct READ_API *ra, int i);
void write_reg32(struct READ_API *ra, int i, int v);

int main(int argc, UCHAR **argv)
{
	UCHAR *g01_0 = malloc(1024 * 1024), *g01_1, *p;
	UCHAR *code0 = malloc(1024 * 1024);
	UCHAR *work0 = malloc(4 * 1024 * 1024);	/* 1MB+1MB+2MB */
	UCHAR *argv0 = *argv, *tmp = malloc(64 * 1024);
	FILE *fp;
	struct READ_EH4 reh4;
	int entry, work_esi[9];
//int i;

	#if (!defined(USE_INCLUDE_G01))
		for (;;) {
			argv++;
			argc--;
			if (argc == 0)
				goto cantopen;
			if (**argv != '-')
				break;
			if (strcmp(*argv, "-lc") == 0)
				 main_flags |= 1;	/* lesser compatible mode */
			if (strcmp(*argv, "-noadc") == 0)
				 main_flags |= 2;	/* no auto de-compress mode */
		}
	#endif
	main_argc = argc;
	main_argv = argv;

	/* *argvを読み込んで実行 */
	func06_init();
	#if (defined(USE_INCLUDE_G01))
		work_esi[0] = inclg01[0] | inclg01[1] << 8 | inclg01[2] << 16 | inclg01[3] << 24;
		for (entry = 0; entry < work_esi[0]; entry++)
			g01_0[entry] = (inclg01 + 4)[entry];
		g01_1 = g01_0 + work_esi[0];
	#else
		fp = fopen(*argv, "rb");
		if (fp == NULL) {
			sprintf(tmp, "%s.g01", *argv);
			fp = fopen(tmp, "rb");
			if (fp == NULL) {
				sprintf(tmp, argv0);
				entry = strlen(tmp);
				while (entry > 0 && tmp[entry - 1] != '\\' && tmp[entry - 1] != '/')
					entry--;
				sprintf(tmp + entry, *argv);
				fp = fopen(tmp, "rb");
				if (fp == NULL) {
					sprintf(tmp + entry, "%s.g01", *argv);
					fp = fopen(tmp, "rb");
					if (fp == NULL) {
cantopen:
					  //						puts("Can't open app-file.");
					  puts("hijk Ver 1.7\n\tCan't open app-file.");
						return 1;
					}
				}
			}
		}
		g01_1 = g01_0 + fread(g01_0, 1, 1024 * 1024 - 16, fp);
		fclose(fp);
	#endif
	if ((work_esi[0] = tek_getsize(g01_0)) > 0) {
		if (tek_decomp(g01_0, code0, g01_1 - g01_0) == 0) {
			for (entry = 0; entry < work_esi[0]; entry++)
				g01_0[entry] = code0[entry];
			g01_1 = g01_0 + work_esi[0];
		}
	}
//	*g01_1 = 0x9f;	/* 31:end */

	if (g01_0[0] != 'G' || g01_0[1] != 0x01) {
		puts("Not GUIGUI01 file.");
		return 1;
	}
	#if (DEBUG_MODE)
		init_reh4(&reh4, &g01_0[2]);
		while (debug_dump_tag(&reh4, 31) != 31);
		putchar('\n');
		putchar('\n');
	#endif

	#if (DEBUG_MODE)
		init_reh4(&reh4, &g01_0[2]);
	//	dump_g01(&reh4, 31);
		putchar('\n');
	#endif

	if (g01_0[2] == 0x70) {
		if ((g01_0[3] & 0xf0) != 0x10) {
unkwn:
			puts("Unknown .g01 format");
			exit(1);
		}
		for (entry = 0; entry < 16; entry++) {
			static char head[16] = {
				0x89, 0xff, 0xff, 0xff, 0x01, 0x00, 0x00, 0x00,
				0x4f, 0x53, 0x41, 0x53, 0x4b, 0x43, 0x4d, 0x50
			};
			code0[entry] = head[entry];
		}
		p = &g01_0[4];
		if ((*p & 0x80) == 0) {
			entry = *p++;
		} else if ((*p & 0xc0) == 0x80) {
			entry = (p[0] & 0x3f) << 8 | p[1];
			p += 2;
		} else if ((*p & 0xe0) == 0xc0) {
			entry = (p[0] & 0x1f) << 16 | p[1] << 8 | p[2];
			p += 3;
		} else if ((*p & 0xf0) == 0xe0) {
			entry = (p[0] & 0x0f) << 24 | p[1] << 16 | p[2] << 8 | p[3];
			p += 4;
		} else {
			goto unkwn;
		}
		entry += 4;
		code0[16] = (entry >> 27) & 0xfe;
		code0[17] = (entry >> 20) & 0xfe;
		code0[18] = (entry >> 13) & 0xfe;
		code0[19] = (entry >>  6) & 0xfe;
		code0[20] = (entry << 1 | 1) & 0xff;
		work_esi[0] = entry;
		entry = 21;
		if ((g01_0[3] &= 0x0f) != 0) {
			entry++;
			if (g01_0[3] == 1)
				code0[21] = 0x15;
			if (g01_0[3] == 2)
				code0[21] = 0x19;
			if (g01_0[3] == 3)
				code0[21] = 0x21;
			if (g01_0[3] >= 4) {
				puts("Unknown .g01 format");
				exit(1);
			}
		}
		do {
			code0[entry++] = *p++;
		} while (p < g01_1);
		if (tek_decomp(code0, g01_0 + 2, entry) != 0) {
			puts("Broken .g01 format");
			exit(1);
		}
		g01_1 = g01_0 + 2 + work_esi[0];
	}

	init_reh4(&reh4, &g01_0[2]);
	work_esi[0] = (int) &asm_api;
	work_esi[1] = 0; /* 未定, mma? */
	work_esi[2] = 0;
	work_esi[3] = (int) work0 + 2 * 1024 * 1024;	/* edxからデフォルト2MBは自由に使える。つまりbssのデフォルトサイズが2MBみたいなもの */
	work_esi[4] = (int) code0;
	work_esi[5] = (int) work0 + 1024 * 1024 - 0x70;
	work_esi[6] = 0; /* 未定, mma? */
	work_esi[7] = (int) work_esi;
	work_esi[8] = 0;
	/* edxは一般にはアラインされたbss終了アドレス。いやそれはおかしい。edxはbss開始アドレス。bssサイズがなければ2MB */
	/* edxは.dataの末端というか.bssの先頭 */
	/* malloc域として使うのなら、edxに足せばいい。 */
	/* データ開始アドレスは、ESP+0x70 */
	entry = load_g01(&reh4, g01_1, code0, work0, work_esi);

	*(int *) &work0[1024 * 1024 - 0x74] = (int) code0 + entry;
	*(int *) &work0[1024 * 1024 - 0x70] = (int) asm_end;
	/* -0x70 - -0x40 くらいはミニプログラムのためにあけてやる。これはst0[]で使いやすいからな */


//for (i=0;i < 50; i++) printf("%02X ", code0[i]);

	#if (USE_POSIX)
		if(tcgetattr(fileno(stdin), &save_term) != 0) {
			puts("tcgetattr error");
			exit(1);
		}
		temp_term = save_term;
		temp_term.c_iflag &= IGNCR;
		temp_term.c_lflag &= ~ICANON;
		temp_term.c_lflag &= ~ECHO;
		temp_term.c_cc[VMIN] = 0;
		temp_term.c_cc[VTIME] = 0;
		if(tcsetattr(fileno(stdin), TCSANOW, &temp_term) != 0) {
			puts("tcsetattr error");
			exit(1);
		}
	signal(SIGINT , signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGTSTP, signal_handler);
	signal(SIGCONT, signal_handler);
	atexit(end_app);
	#endif
	/* setjump */
	start_app(work_esi);

	return 0;
}

void init_reh4(struct READ_EH4 *reh4, UCHAR *p)
{
	int i;
	for (i = 0; i < READ_EH4_BUFLEN; i += 2) {
		reh4->buf[i + 0] = *p >> 4;
		reh4->buf[i + 1] = *p & 0x0f;
		p++;
	}
	reh4->p = p;
	reh4->r = 0;
	reh4->w = 0;
	reh4->l = READ_EH4_BUFLEN;
	return;
}

void fill_reh4(struct READ_EH4 *reh4)
{
	while (reh4->l <= READ_EH4_BUFLEN - 2) {
		reh4->buf[reh4->w + 0] = *(reh4->p) >> 4;
		reh4->buf[reh4->w + 1] = *(reh4->p) & 0x0f;
		reh4->l += 2;
		reh4->p++;
		reh4->w = (reh4->w + 2) & (READ_EH4_BUFLEN - 1);
	}
	return;
}

int getlen(struct READ_EH4 *reh4)
{
	UCHAR c;
	fill_reh4(reh4);
	c = reh4->buf[reh4->r];
	if ((c & 0x8) == 0)
		return 1;
	if ((c & 0xc) == 0x8)
		return 2;
	if ((c & 0xe) == 0xc)
		return 3;
	if (c == 0xe)
		return 4;
	c = reh4->buf[(reh4->r + 1) & (READ_EH4_BUFLEN - 1)];
	if ((c & 0x8) == 0)
		return 5;
	if ((c & 0xc) == 0x8)
		return 6;
	if ((c & 0xe) == 0xc)
		return 7;
	if (c == 0xe)
		return 8;
	c = reh4->buf[(reh4->r + 2) & (READ_EH4_BUFLEN - 1)];
	if ((c & 0x8) == 0)
		return 16;
	if ((c & 0xc) == 0x8)
		return 20;
	if ((c & 0xe) == 0xc)
		return 24;
	if (c == 0xe)
		return 32;
	puts("getlen: too long.");
	exit(1);
}

int getnum_u0(struct READ_EH4 *reh4)
{
	return getnum_u(reh4, 0);
}

int getnum_u1(struct READ_EH4 *reh4)
{
	return getnum_u(reh4, 1);
}

int getnum_u(struct READ_EH4 *reh4, char f)
{
	static unsigned int mask[] = {
		0x7, 0x3f, 0x1ff, 0xfff, 0x7fff, 0x3ffff, 0x1fffff, 0xffffff, 0, 0xffffffff, 0,	/* 4-44 */
		0xffffffff, 0, 0xffffffff, 0, 0xffffffff, 0, 0, 0, 0xffffffff, 0, 0, 0, /* 48-92 */
		0xffffffff, 0, 0, 0, 0xffffffff /* 96-128 */
	};
	int l = getlen(reh4), j = l, i;
	if (f != 0 && reh4->buf[reh4->r] == 6 /* && l == 1 */) {
		reh4->r = (reh4->r + 1) & (READ_EH4_BUFLEN - 1);
		reh4->l--;
		return -1;
	}
	if (reh4->buf[reh4->r] == 7 /* && l == 1 */) {
		while (reh4->buf[reh4->r] == 7) {
			reh4->r = (reh4->r + 1) & (READ_EH4_BUFLEN - 1);
			reh4->l--;
			fill_reh4(reh4);
		}
		j = l = getlen(reh4);
		if (l < 8) {	/* special */
			if (l == 1) {
				j = reh4->buf[reh4->r] << 1;	/* 4のとき8、5のとき10、6のとき12 */
				l = j + 2;
				reh4->r = (reh4->r + 1) & (READ_EH4_BUFLEN - 1);
				reh4->l--;
				fill_reh4(reh4);
				if (j == 0 >> 1)
					return -1; /* inf / null-ptr */
			} else if (l == 2) {
				i = reh4->buf[reh4->r] << 4 | reh4->buf[(reh4->r + 1) & (READ_EH4_BUFLEN - 1)];
				reh4->r = (reh4->r + 2) & (READ_EH4_BUFLEN - 1);
				reh4->l -= 2;
				fill_reh4(reh4);
				if (i == 0xb8) {
					j = getnum_u0(reh4);
					return getnum_u0(reh4) << j;
				} else if (i == 0xb9) {
					#if (DEBUG_MODE)
						printf("getnum_u: 7B9 ");
					#endif
					j = getnum_u0(reh4);
					return getnum_u(reh4, 0) << j | ((1 << j) - 1);
				} else if (i == 0xba) {
					j = getnum_u0(reh4);
					l = 10;
				} else if (i == 0xbb) {
					puts("error-5");
					exit(1);
				} else if (i == 0xbc) {
					i =  reh4->buf[reh4->r] << 4;
					i |= reh4->buf[(reh4->r + 1) & (READ_EH4_BUFLEN - 1)];
					i |= reh4->buf[(reh4->r + 2) & (READ_EH4_BUFLEN - 1)] << 12;
					i |= reh4->buf[(reh4->r + 3) & (READ_EH4_BUFLEN - 1)] <<  8;
					reh4->r = (reh4->r + 4) & (READ_EH4_BUFLEN - 1);
					reh4->l -= 4;
					return i;
				} else if (i == 0xbd) {
					i =  reh4->buf[reh4->r] << 4;
					i |= reh4->buf[(reh4->r + 1) & (READ_EH4_BUFLEN - 1)];
					i |= reh4->buf[(reh4->r + 2) & (READ_EH4_BUFLEN - 1)] << 12;
					i |= reh4->buf[(reh4->r + 3) & (READ_EH4_BUFLEN - 1)] <<  8;
					i |= reh4->buf[(reh4->r + 4) & (READ_EH4_BUFLEN - 1)] << 20;
					i |= reh4->buf[(reh4->r + 5) & (READ_EH4_BUFLEN - 1)] << 16;
					i |= reh4->buf[(reh4->r + 6) & (READ_EH4_BUFLEN - 1)] << 28;
					i |= reh4->buf[(reh4->r + 7) & (READ_EH4_BUFLEN - 1)] << 24;
					reh4->r = (reh4->r + 8) & (READ_EH4_BUFLEN - 1);
					reh4->l -= 8;
					return i;
				} else if (i == 0xbe) {
					i =  reh4->buf[reh4->r] << 4;
					i |= reh4->buf[(reh4->r + 1) & (READ_EH4_BUFLEN - 1)];
					i |= reh4->buf[(reh4->r + 2) & (READ_EH4_BUFLEN - 1)] << 12;
					i |= reh4->buf[(reh4->r + 3) & (READ_EH4_BUFLEN - 1)] <<  8;
					i |= reh4->buf[(reh4->r + 4) & (READ_EH4_BUFLEN - 1)] << 20;
					i |= reh4->buf[(reh4->r + 5) & (READ_EH4_BUFLEN - 1)] << 16;
					i |= reh4->buf[(reh4->r + 6) & (READ_EH4_BUFLEN - 1)] << 28;
					i |= reh4->buf[(reh4->r + 7) & (READ_EH4_BUFLEN - 1)] << 24;
					reh4->r = (reh4->r + 16) & (READ_EH4_BUFLEN - 1);
					reh4->l -= 16;
					return i;
				} else {
					puts("error-6");
					exit(1);
				}
			} else {
				puts("error-4");
				exit(1);
			}
		}
	}
	i = 0;
	do {
		i = i << 4 | reh4->buf[reh4->r];
		reh4->r = (reh4->r + 1) & (READ_EH4_BUFLEN - 1);
		reh4->l--;
	} while (--j);
	return i & mask[l - 1];
}

#if (DEBUG_MODE)

int debug_dump_tag(struct READ_EH4 *reh4, int trm)
/* G01のdump用 */
{
	int i = getnum_u0(reh4), l;
	if (i == trm)
		return i;
	if (i == 0) {	/* NOP */
		printf("[0] ");
	}
	if (1 <= i && i <= 7) {		/* 1-7 */
		printf("[%X %X] ", i, getnum_u0(reh4));
	}
	if (8 <= i && i <= 15) {	/* 8-15 */
		printf("[%X (%X) ", i, l = getnum_u0(reh4));
		while (l > 0) {
			debug_dump_tag(reh4, trm);
			l--;
		}
		printf("\b] ");
	}
	if (16 <= i && i <= 23) {
		printf("[%X (%X) ", i, l = getnum_u0(reh4));
		while (l > 0) {
			fill_reh4(reh4);
			printf("%X", reh4->buf[reh4->r]);
			reh4->r = (reh4->r + 1) & (READ_EH4_BUFLEN - 1);
			reh4->l--;
			l--;
		}
		printf("] ");
	}
	if (24 <= i && i <= 31) {
		printf("[%X (%X) ", i, l = getnum_u0(reh4));
		while (l > 0) {
			printf("%X ", getnum_u0(reh4));
			l--;
		}
		printf("] ");
	}
	return i;
}

void dump_g01(struct READ_EH4 *reh4, int trm)
{
	int stack_unit = 1 << 12, stack_size = 1024 * 1024 >> 12;
	int heap_unit = 1 << 12, heap_size = 2 * 1024 * 1024 >> 12;
	int file_access_level = 0, last_section = -1;
	static char *secname[] = { ".text", ".data", ".bss" };
	int flags = 0, i, j, k, l;

	for (;;) {
		i = getnum_u0(reh4);
		if (i == trm)
			return;
		if (i == 0)
			continue;
		if (i == 3)
			file_access_level = getnum_u0(reh4) >> 1;
		else if (i == 4) {
			stack_unit = 4096 << (getnum_u0(reh4) << 1);
			stack_size = 1;
		} else if (i == 5)
			stack_size = getnum_u0(reh4) + 1;
		else if (i == 8) {
			heap_unit = 4096 << (getnum_u0(reh4) << 1);
			heap_size = 1;
		} else if (i == 9)
			heap_size = getnum_u0(reh4) + 1;
		else if (i == 12) {	/* skip (comment out) */
			j = getnum_u0(reh4);
			while (j > 0) {
				skip_tags(reh4);
				j--;
			}
		} else if (i == 13) {	/* section */
			int sectype = last_section + 1, align = 1 << 12, entry = 0, size = 0;
			if ((flags & 1) == 0) {
				printf("file_access_level:%d\n", file_access_level);
				printf("stack:0x%X\n", stack_unit * stack_size);
				printf("heap: 0x%X\n", heap_unit * heap_size);
				flags |= 1;
			}
			j = getnum_u0(reh4);
			while (j > 0) {
				k = getnum_u0(reh4);
				if (k == 0)
					;
				else if (k == 1)
					sectype = last_section = getnum_u0(reh4);
				else if (k == 2)
					align = 1 << getnum_u0(reh4);
				else if (k == 4)
					entry = size = getnum_u0(reh4);
				else if (k == 20) {
					size = l = getnum_u0(reh4);
					while (l > 0) {
						fill_reh4(reh4);
						reh4->r = (reh4->r + 1) & (READ_EH4_BUFLEN - 1);
						reh4->l--;
						l--;
					}
					size >>= 1;
				} else {
					puts("error");
					exit(1);
				}
				j--;
			}
			printf("secton-type: %s\n", secname[sectype]);
			printf("  align:%d\n", align);
			printf("  size:%d\n", size);
			if (sectype == 0)
				printf("  entry-point:0x%X\n", entry);
		} else {
			puts("error");
			exit(1);
		}
	}
	return;
}

#endif

void skip_tags(struct READ_EH4 *reh4)
{
	int i = getnum_u0(reh4), j;
	if (1 <= i && i <= 11)
		getnum_u(reh4, 0);
	if (12 <= i && i <= 15) {
		j = getnum_u0(reh4);
		while (j > 0) {
			skip_tags(reh4);
			j--;
		}
	}
	if (16 <= i && i <= 23) {
		j = getnum_u0(reh4);
		while (j > 0) {
			fill_reh4(reh4);
			reh4->r = (reh4->r + 1) & (READ_EH4_BUFLEN - 1);
			reh4->l--;
			j--;
		}
	}
	if (24 <= i && i <= 31) {
		j = getnum_u0(reh4);
		while (j > 0) {
			getnum_u0(reh4);
			j--;
		}
	}
	return;
}

static char exit_nl = 1, errmod = 1, bufmod = 1, last_putc = 0; /* できるだけcuiで対話して解決 */
	/* 0:エラー返す, 1:エラー返さない・解決しない（エラーメッセージ）, 2:黙ってできる範囲でのみ解決, 3:cui, 4:gui */
	/* bufmod=0:打ち切り, 1:エラー返さない・解決しない（エラーメッセージ）, 2:黙ってできる範囲でのみ解決, 3:cui, 4:gui */
	/* eui=0:システム任せ, 1:沈黙, 2:cui, 3:gui */

/*
一時設定、永久設定
fwriteは分割読み込みを意図しているのか、一括読み込みを意図しているのかを示すオプションを
*/

int load_g01(struct READ_EH4 *reh4, UCHAR *p1, UCHAR *code0, UCHAR *work0, int *esi0)
{
	int stack_unit = 4096, stack_size = 1024 * 1024 / 4096;
	int heap_unit = 4096, heap_size = 2 * 1024 * 1024 / 4096;
	int mma_unit = 4096, mma_size = 1 * 1024 * 1024 / 4096;
	int file_access_level = 0, last_section = -1;
	int flags = 0, i, j, k, l, m, n, rjc_flag = 1, ii;
	int entry = 0, size;
	UCHAR *code00 = code0, *data00 = work0 + 1024 * 1024;

	for (l = 0; l < 32; l++)
		work0[l] = 0x00;

	ii = getnum_u0(reh4);
	if (ii == 0) {
		flags = getnum_u0(reh4) ^ 6;	/* bit0:CALL(EBP)自動挿入, bit1: 自動改行, bit2: rjc */
//printf("flags=0x%X\n", flags);
		rjc_flag = (flags >> 2) & 1;
		exit_nl = (flags >> 1) & 1;
		ii = (flags >> 3) & 3;
		if (ii > 0) {
			if (ii <= 2) {
				esi0[1] = getnum_u0(reh4);	/* 今のところ単純定数のみ */
				if (ii == 2)
					esi0[2] = getnum_u0(reh4);	/* 今のところ単純定数のみ */
//printf("eax0=0x%X\n", esi0[1]);
//printf("ecx0=0x%X\n", esi0[2]);
			} else {
				puts("reg32-init: internal error");
				exit(1);
			}
		}
		reh4->p -= reh4->l >> 1;
//printf("next=0x%X\n", *reh4->p);
	//	reh4->w = reh4->r;
	//	reh4->l &= 1;
	//	reh4->w = (reh4->w + reh4->l) & (READ_EH4_BUFLEN - 1);
		if ((flags & 1) != 0) {
			code0[0] = 0xff;
			code0[1] = 0x16;
			code0 += 2;
		}
		while (reh4->p < p1)
			*code0++ = *(reh4->p)++;
		size = code0 - code00;
		if (rjc_flag != 0)
			rjc(code00, code0, 0, 0, size, 0);
		*code0++ = 0xc3;
		goto fin;
	}
	for (; ii > 0; ii--) {
		i = getnum_u0(reh4);
	//	if (i == trm)
	//		return entry;
		if (i == 0)
			continue;
		if (i == 3) {
			file_access_level = getnum_u0(reh4) ^ 6;
			rjc_flag = (file_access_level >> 2) & 1;
			exit_nl = (file_access_level >> 1) & 1;
			file_access_level >>= 5;
		} else if (i == 4) {
			stack_unit = 4096 << (getnum_u0(reh4) << 1);
			stack_size = 1;
		} else if (i == 5)
			stack_size = getnum_u0(reh4) + 1;
		else if (i == 8) {
			heap_unit = 4096 << (getnum_u0(reh4) << 1);
			heap_size = 1;
		} else if (i == 9)
			heap_size = getnum_u0(reh4) + 1;
		else if (i == 10) {
			mma_unit = 4096 << (getnum_u0(reh4) << 1);
			mma_size = 1;
		} else if (i == 11)
			mma_size = getnum_u0(reh4) + 1;
		else if (i == 12) {	/* skip (comment out) */
			j = getnum_u0(reh4);
			while (j > 0) {
				skip_tags(reh4);
				j--;
			}
		} else if (i == 13) {	/* section */
			int sectype = ++last_section, align = 1 << 12, size = 0;
			j = getnum_u0(reh4);
			if ((flags & 1) == 0) {
			//	work0 += stack_unit * stack_size;
				work0 += 1024 * 1024;
				flags |= 1;
			}
			while (j > 0) {
				k = getnum_u0(reh4);
				if (k == 0)
					;
				else if (k == 1)
					sectype = last_section = getnum_u0(reh4);
				else if (k == 2)
					align = 1 << getnum_u0(reh4);
				else if (k == 4) {
					if (sectype == 0)
						entry = getnum_u0(reh4);
					else
						size = getnum_u0(reh4);	/* bss用 */
				} else if (k == 20) {
					size = l = getnum_u0(reh4);
					l >>= 1;
					if (sectype == 0) {
						while (l > 0) {
							fill_reh4(reh4);
							*code0++ = reh4->buf[reh4->r] << 4 | reh4->buf[(reh4->r + 1) & (READ_EH4_BUFLEN - 1)];
							reh4->r = (reh4->r + 2) & (READ_EH4_BUFLEN - 1);
							reh4->l -= 2;
							l--;
						}
						if (rjc_flag != 0)
							rjc(code0 - size / 2, code0, 0, 0, size / 2, 0);
						*code0++ = 0xc3;
					}
					if (sectype == 1) {
						while (l > 0) {
							fill_reh4(reh4);
							*work0++ = reh4->buf[reh4->r] << 4 | reh4->buf[(reh4->r + 1) & (READ_EH4_BUFLEN - 1)];
							reh4->r = (reh4->r + 2) & (READ_EH4_BUFLEN - 1);
							reh4->l -= 2;
							l--;
						}
						for (l = 0; l < 32; l++)
							*work0++ = 0x00;
					}
					size >>= 1;
				} else if (k == 25) {	/* 19.quick-links */
					l = getnum_u0(reh4) - 1;
					m = getnum_u0(reh4);
					if (m == 0)
						m = (int) code00;
					else if (m == 1)
						m = (int) data00;
					else {
						puts("error");
						exit(1);
					}
					n = 0;
					while (l > 0) {
						n += getnum_u0(reh4);
						if (sectype == 0) {
							*(int *) &code00[n] += m;
							n += 4;
						} else if (sectype == 1) {
							*(int *) &data00[n * 4] += m;
							n++;
						} else {
							puts("error-1");
							exit(1);
						}
						l--;
					}
				} else {
					puts("error-2");
					exit(1);
				}
				j--;
			}
		} else {
			puts("error-3");
			exit(1);
		}
	}
fin:
	return entry;
}

void init_ra(struct READ_API *ra, UCHAR *p)
{
	init_reh4(&ra->reh4, p);
	ra->rep = 0x7fffffff;
	ra->mod = 0;
	ra->mod0 = 0;
	ra->flags = 0;
	ra->gosub = 0;
	ra->term = 0;
	return;
}

void ungetnum_gh4(struct READ_EH4 *reh4, char g)
{
	if (reh4->l > READ_EH4_BUFLEN - 1) {
		reh4->l -= 2;
		reh4->p--;
		reh4->w = (reh4->w + (READ_EH4_BUFLEN - 2)) & (READ_EH4_BUFLEN - 1);
	}
	reh4->r = (reh4->r + (READ_EH4_BUFLEN - 1)) & (READ_EH4_BUFLEN - 1);
	reh4->buf[reh4->r] = g;
	reh4->l++;
	return;
}

char test_sig4(struct READ_API *ra, char i)
{
	if (ra->mod == 0) {
		if (ra->reh4.l == 0)
			fill_reh4(&ra->reh4);
		if (ra->reh4.buf[ra->reh4.r] == i) {	/* デフォルトキャンセルは、4bitの5 */
			ra->reh4.r = (ra->reh4.r + 1) & (READ_EH4_BUFLEN - 1);
			ra->reh4.l--;
			return -1;
		}
	}
	return 0;
}

void insert_6_0(struct READ_API *ra)
/* 6_0 */
{
	if (ra->mod == 0 && test_sig4(ra, 4) == 0) {	/* デフォルトキャンセルは、4bitの4 */
//		ungetnum_gh4(&ra->reh4, 0x0);
		ungetnum_gh4(&ra->reh4, 0x6);
	}
	return;
}

void insert_6x(struct READ_API *ra)
/* 6_504 */
{
	if (ra->mod == 0 && test_sig4(ra, 5) == 0) {		/* デフォルトキャンセルは、4bitの5 */
		ungetnum_gh4(&ra->reh4, 0x8);
		ungetnum_gh4(&ra->reh4, 0xe);
		ungetnum_gh4(&ra->reh4, 0xd);
		ungetnum_gh4(&ra->reh4, 0x6);
	}
	return;
}

void insert_6y(struct READ_API *ra)
/* 6_505 */
{
	if (ra->mod == 0 && test_sig4(ra, 5) == 0) {		/* デフォルトキャンセルは、4bitの5 */
		ungetnum_gh4(&ra->reh4, 0x9);
		ungetnum_gh4(&ra->reh4, 0xe);
		ungetnum_gh4(&ra->reh4, 0xd);
		ungetnum_gh4(&ra->reh4, 0x6);
	}
	return;
}

void insert_mini(struct READ_API *ra, char s)
/* s */
{
	if (ra->mod == 0 && test_sig4(ra, 5) == 0)		/* デフォルトキャンセルは、4bitの5 */
		ungetnum_gh4(&ra->reh4, s);
	return;
}

static UCHAR osask7 = 0, osask7_buf;

void enable_osask7()
{
	osask7 = 1;
	osask7_buf = 1;
	return;
}

void disable_osask7()
{
	osask7 = 0;
	return;
}

int getnum_api(struct READ_API *ra, char f)
/* 将来はバッファリングなどを活用して高速化 */
/* f=1:個数フィールド, f=2:ターミネータ通知 */
{
	int i, j, k;
	if (osask7 != 0 && osask7_buf >= 0x80) {
		i = osask7_buf & 0x7f;
		osask7_buf = 1;
		if (i != 0)
			osask7 |= 0x02;
		if ((osask7 & 0x02) != 0) {
			if (ra->term != 0) {
				if (i == ra->rep) {
					ra->term = 0;
					ra->mod = ra->mod0;
					ra->rep = 0x7fffffff;
					if (ra->gosub != 0)
						ra->reh4 = ra->reh4_b;
					if (f != 9)
						goto retry;
					return -2; /* term */
				}
			} else {
				ra->rep--;
				if (ra->rep == 0) {	/* gosubからの帰還 */
			//		ra->term = 0;
					ra->mod = ra->mod0;
					ra->rep = 0x7fffffff;
					if (ra->gosub != 0)
						ra->reh4 = ra->reh4_b;
				}
			}
			goto fin1;
		}
	}

retry:
	if (ra->mod == 0) {
#if 0
if (ra->reh4.buf[ra->reh4.r] == 6) {
printf("l=%d ", ra->reh4.l);
printf("next: %X ", ra->reh4.buf[ra->reh4.r]);
printf("%X ", ra->reh4.buf[(ra->reh4.r + 1) & (READ_EH4_BUFLEN - 1)]);
if (ra->reh4.l == 1) { puts("fill"); fill_reh4(&ra->reh4);
printf("l=%d ", ra->reh4.l);
printf("next: %X ", ra->reh4.buf[ra->reh4.r]);
printf("%X ", ra->reh4.buf[(ra->reh4.r + 1) & (READ_EH4_BUFLEN - 1)]);
}}
#endif
		j = getlen(&ra->reh4);
		i = getnum_u1(&ra->reh4);
		if (f == 0) {
			if (0x3c <= i && i <= 0x3f && j == 2)
				i = 0x40 << (i - 0x3c);
			if (0x1f0 <= i && i <= 0x1ff && j == 3)
				i = 0x400 << (i - 0x1f0);
			if (0xfc0 <= i && i <= 0xfc5 && j == 4)
				i = 0x4000000 << (i - 0xfc0);
		}
		if (i == -1) {
			i = getreg32idx(ra);
//printf("6_%d", i);
			if (i <= 0x3f || i >= 0x7ffffff4)
				i = read_reg32(ra, i);
			else if (i == 0x1e8) {
				j = getnum_api(ra, 0);
				ra->gosub = 1;
				ra->mod0 = ra->mod;
				ra->term = j & 1;
				insert_mini(ra, 1);
				k = getnum_api(ra, 0);
//printf("_%d_%d", j, k);
				if (k == 3)
					k++;
				if (ra->term != 0 && k == 1)
					insert_mini(ra, 0);	/* byteだとterm-zeroがデフォルトになる */
				i = getnum_api(ra, 0);
//printf("_%d", i);
				if (j <= 1)
					ra->gosub = 0;
				else if (j <= 3) {
					insert_6_0(ra);
					j = getnum_api(ra, 0);
//printf("_%d", j);
					ra->reh4_b = ra->reh4;
					init_reh4(&ra->reh4, (UCHAR *) j);
				} else {
					puts("getnum_api: internal error: 6x...");
					exit(1);
				}
//putchar(';');
//putchar(' ');
				ra->rep = i;
				ra->mod = k * 8;
				if (ra->term != 0)
					i = -2;
				else {
					ra->rep++;
#if 0
					if (ra->rep == 0) {
				//		ra->term = 0;
						ra->mod = ra->mod0;
						ra->rep = 0x7fffffff;
						if (ra->gosub != 0)
							ra->reh4 = ra->reh4_b;
					}
#endif
				}
			} else {
				puts("getnum_api: internal error: 6...");
printf("i=%d", i);
				exit(1);
			}
		}
	} else if (ra->mod == 8) {
		ra->reh4.p -= ra->reh4.l >> 1;
		ra->reh4.w = ra->reh4.r;
		ra->reh4.l &= 1;
		ra->reh4.w = (ra->reh4.w + ra->reh4.l) & (READ_EH4_BUFLEN - 1);
		i = *(ra->reh4.p)++;
	} else if (ra->mod == 16) {
		fill_reh4(&ra->reh4);
		while ((((int) ra->reh4.p) & 1) != 0) {
			ra->reh4.p--;
			ra->reh4.l -= 2;
			ra->reh4.w = (ra->reh4.w - 2 + READ_EH4_BUFLEN) & (READ_EH4_BUFLEN - 1);
		}
		while (ra->reh4.l >= 4) {
			ra->reh4.p -= 2;
			ra->reh4.l -= 4;
			ra->reh4.w = (ra->reh4.w - 4 + READ_EH4_BUFLEN) & (READ_EH4_BUFLEN - 1);
		}
		i = *(unsigned short *) ra->reh4.p;
		ra->reh4.p += 2;
	} else if (ra->mod == 32) {
		fill_reh4(&ra->reh4);
		while ((((int) ra->reh4.p) & 3) != 0) {
			ra->reh4.p--;
			ra->reh4.l -= 2;
			ra->reh4.w = (ra->reh4.w - 2 + READ_EH4_BUFLEN) & (READ_EH4_BUFLEN - 1);
		}
		while (ra->reh4.l >= 8) {
			ra->reh4.p -= 4;
			ra->reh4.l -= 8;
			ra->reh4.w = (ra->reh4.w - 8 + READ_EH4_BUFLEN) & (READ_EH4_BUFLEN - 1);
		}
		i = *(int *) ra->reh4.p;
		ra->reh4.p += 4;
	} else {
		puts("getnum_api: error");
		exit(1);
	}
	if (ra->term != 0) {
		if (i == ra->rep) {
			ra->term = 0;
			ra->mod = ra->mod0;
			ra->rep = 0x7fffffff;
			if (ra->gosub != 0)
				ra->reh4 = ra->reh4_b;
			if (f != 9)
				goto retry;
			return -2; /* term */
		}
	} else {
		ra->rep--;
		if (ra->rep == 0) {	/* gosubからの帰還 */
	//		ra->term = 0;
			ra->mod = ra->mod0;
			ra->rep = 0x7fffffff;
			if (ra->gosub != 0)
				ra->reh4 = ra->reh4_b;
		}
	}
fin:
	if (osask7 != 0) {
		osask7_buf = osask7_buf << 1 | ((i >> 7) & 1);
		i &= 0x7f;
fin1:
		if (0x10 <= i && i <= 0x19)
			i += 0x20;
		if (0x1a <= i && i <= 0x1f)
			i += 'A' - 0x1a;
	}
	#if (DEBUG_MODE)
		printf("%d ", i);
	#endif
	return i;

#if 0
retry:
	if (ra->rep == 0) {
		ra->mod = ra->mod0;
		do {
			i = getnum_u1(&ra->reh4);
		} while (i == 0);
		if (i == 1) {
			/* mode */
			#if (DEBUG_MODE)
				puts("getnum_api: mode");
			#endif
			j = getnum_u(&ra->reh4);
			if ((j & 1) != 0) {
				puts("getnum_api: error");
				exit(1);
			}
			if ((j & 8) != 0) {
				puts("getnum_api: error");
				exit(1);
			}
			k = getnum_u(&ra->reh4);
			if (k > 0)
				k = ((j >> 7) + 1) << (k - 1);
			ra->mod = k;
			if ((j & 4) != 0)
				ra->mod0 = k;
			#if (DEBUG_MODE)
				printf("api: mod%d ", ra->mod);
			#endif
			i = getnum_u(&ra->reh4);
			if (i == 0)
				goto retry;
			ra->rep = i;
			#if (DEBUG_MODE)
				printf("api: modrep%d ", ra->rep);
			#endif
			if ((j & 2) != 0)
				goto fin;
		} else {
			ra->rep = i - 1;
			#if (DEBUG_MODE)
				printf("api: rep%d ", ra->rep);
			#endif
		}
	}
#endif
}

void c_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax, int eip)
{
	UCHAR *cmd = (UCHAR *) edi, *p, *q, /* *r, */ tmpbuf[32], f3 = 1;
	struct READ_API ra;
	int i, j, k, l, m, n, o, h, edi0 = edi;
	if (edi == 0)
		cmd = (UCHAR *) eip;
	init_ra(&ra, cmd);
	ra.reg32 = &edi;
	ra.st0 = &eip;
	for (;;) {
		if (f3 == 0)
			goto fin;
		f3--;
		i = getnum_api(&ra, 0);
		if (i == 0)
			continue;
//printf("i=%d ", i);
		if (i == 3) {
			if (f3 == 0) {
				f3 = 0xff;
				continue;
			}
fin:
			if (edi0 == 0)
				eip = (int) ra.reh4.p - (ra.reh4.l >> 1);
		//	i = (&eip)[2]; /* これがないとなぜか落ちる（CPUの隠れた仕様？：デバックでprintfしてて偶然発見） */
			return;
		}
		if (i == 4) {	/* 今のnaskfuncではRETで戻ったときのESI値は不定 */
			if (exit_nl != 0 && last_putc != 0x0a && last_putc != 0)
				putchar('\n');
			i = getnum_api(&ra, 0);
			if (i == 0)
				exit(0);
			if (i == 1)
				exit(1);
			if (i == 3)	/* 数字を返す異常終了 */
				exit(getnum_api(&ra, 0));
			puts("c_api: func4: error");
			exit(1);
		}
		if (i == 5) {
			if (ra.mod == 0 && test_sig4(&ra, 4) == 0)		/* デフォルトキャンセルは、4bitの4 */
				ungetnum_gh4(&ra.reh4, 0);
			if (getnum_api(&ra, 0) != 0) {
				puts("c_api: func5: error");
				exit(1);
			}
			insert_6x(&ra);
			j = getnum_api(&ra, 1);
enable_osask7();
			if (j >= 0) {
//printf("func5: l=%d ", j);
				while (j > 0) {
					putchar(last_putc = getnum_api(&ra, 0));
					j--;
				}
			} else {
				while ((j = getnum_api(&ra, 9)) >= 0)
					putchar(last_putc = j);
			}
disable_osask7();
			fflush(stdout);
		} else if (i == 6) {
			if (test_sig4(&ra, 5) != 0) {
				if ((func06str->flags & 1) == 0) {
					func06str->flags |= 1;
						func06str->def_arg0 = 0x7fffffff;
					if (test_sig4(&ra, 5) == 0)
						func06str->def_arg0 = getnum_api(&ra, 0);
					for (j = 0;; j++) {
						i = getnum_api(&ra, 0);
						if (i == 4) {
							func06str->usgs = j;
							break;
						}
						if (j == 64) {
							puts("func6: internal error (2)");
							exit(1);
						}
						if (i == 5)
							i = 0xfff0 << 2 | 0;
						if (i == 6)
							i = 0xfff1 << 2 | 2;
						if (i == 7) {
							/* 改行 */
							if (j == 0) {
								puts("func6: internal error (3)");
								exit(1);
							}
							j--;
							for (i = 0; func06str->usg[j].usg[i] != '\0'; i++);
							func06str->usg[j].usg[i] = 0x1f;
							func06str->usg[j].usg[i + 1] = '\0';
							continue;
						}
						func06str->usg[j].flg = i & 3;
						i >>= 2;
						if (i == 0) {
							k = getnum_api(&ra, 0) + 1;
							ra.reh4.p -= ra.reh4.l >> 1;
							ra.reh4.l &= 1;
							ra.reh4.w = (ra.reh4.r + ra.reh4.l) & (READ_EH4_BUFLEN - 1);
							for (l = 0; l < k; l++)
								func06str->usg[j].tag[l] = *(ra.reh4.p)++;
							func06str->usg[j].tag[l] = '\0';
							func06str->usg[j].typ = getnum_api(&ra, 0);
							k = getnum_api(&ra, 0);
							ra.reh4.p -= ra.reh4.l >> 1;
							ra.reh4.l &= 1;
							ra.reh4.w = (ra.reh4.r + ra.reh4.l) & (READ_EH4_BUFLEN - 1);
							for (l = 0; l < k; l++)
								func06str->usg[j].usg[l] = *(ra.reh4.p)++;
							func06str->usg[j].usg[l] = '\0';
						} else if (i == 2) {	/* in:input-file */
							func06str->usg[j].tag[0] = 'i';
							func06str->usg[j].tag[1] = 'n';
							func06str->usg[j].tag[2] = '\0';
							func06str->usg[j].typ = 0;
							func06str->usg[j].usg[0] = 0x01;
							func06str->usg[j].usg[1] = 'p';
							func06str->usg[j].usg[2] = 'u';
							func06str->usg[j].usg[3] = 't';
							func06str->usg[j].usg[4] = '-';
							func06str->usg[j].usg[5] = 0x02;
							func06str->usg[j].usg[6] = '\0';
						} else if (i == 3) {	/* out:output-file */
							func06str->usg[j].tag[0] = 'o';
							func06str->usg[j].tag[1] = 'u';
							func06str->usg[j].tag[2] = 't';
							func06str->usg[j].tag[3] = '\0';
							func06str->usg[j].typ = 0;
							func06str->usg[j].usg[0] = 0x01;
							func06str->usg[j].usg[1] = 'p';
							func06str->usg[j].usg[2] = 'u';
							func06str->usg[j].usg[3] = 't';
							func06str->usg[j].usg[4] = '-';
							func06str->usg[j].usg[5] = 0x02;
							func06str->usg[j].usg[6] = '\0';
						} else if (i == 0xfff0) {	/*  */
							puts("func6: internal error (6)");
							exit(1);
						} else if (i == 0xfff1) {	/*  */
							puts("func6: internal error (5)");
							exit(1);
						} else {
							puts("func6: internal error (4)");
							printf("i=%d j=%d\n", i, j);
							exit(1);
						}
						if (test_sig4(&ra, 5) != 0) { /* デフォルト値設定 */
							puts("func6: internal error (7)");
							exit(1);
						}
					}
			//		func06_putusage();
					func06_setup();
				} else {
					func06_putusage();
				}
			} else {
				if ((func06str->flags & 1) == 0) {
					func06str->flags |= 1;
					func06_setup();
				}
				i = getnum_api(&ra, 1);
				if (i == 0x3f) {
					func06_putusage();
					exit(1);
				}
				if (i == 0x3e) {
					i = getnum_api(&ra, 1);
					j = getreg32idx(&ra);
					for (l &= 0, k = 1; func06str->argv_f[k] != 0xff; k++) {
						if ((func06str->argv_f[k] & 0x7f) == i)
							l++;
					}
					if (func06str->usg[i].typ == 0xff)
						l = 1;
					if (func06str->usg[i].typ == 0xfe) {
						for (l = 1, k = 1; func06str->argv_f[k] != 0xff; k++) {
							if ((func06str->argv_f[k] & 0x80) != 0)
								l++;
						}
					}
					write_reg32(&ra, j, l);
					continue;
				}
				j &= 0;
				k = 1;
				if (0x3c <= i && i <= 0x3d) {	/* argの内容を加工せずに出力（エラー用）, 3dは整理してから表示&タグは出ない */
					l = i;
					i = getnum_api(&ra, 1);
					if (i >= func06str->usgs) {
						puts("func6: internal error (9)");
						exit(1);
					}
					if ((func06str->usg[i].flg & 2) != 0) {
						insert_6_0(&ra);
						j = getnum_api(&ra, 0);
					}
					if (func06str->usg[i].typ == 0xff) {	/* コマンドライン全体型 */
						printf(func06str->argv_all);
						last_putc = 1;
						continue;
					} else if (func06str->usg[i].typ == 0xfe) {
						/* コマンドライン部分型 */
						p = func06str->argv0;
						if (j > 0) {
							do {
								while (k < main_argc && (func06str->argv_f[k] & 0x80) == 0)
									k++;
								k++;
							} while (--j > 0);
							k--;
							p = NULL;
							if (k < main_argc)
								p = main_argv[k];
						}
						if (p != NULL) {
							printf(p);
							last_putc = 1;
						}
						continue;
					}
					k = 1;
					do {
						while (k < main_argc && (func06str->argv_f[k] & 0x7f) != i)
							k++;
						k++;
					} while (--j >= 0);
					k--;
					p = NULL;
					if (k < main_argc) {
						p = main_argv[k];
						if ((func06str->argv_f[k] & 0x80) == 0 && l == 0x3d)
							p += strlen(func06str->usg[i].tag) + 1;
					}
					if (p != NULL) {
						printf(p);
						last_putc = 1;
					}
					continue;
				}
				if (i >= func06str->usgs) {
					puts("func6: internal error (8)");
					exit(1);
				}
				if ((func06str->usg[i].flg & 2) != 0) {
					insert_6_0(&ra);
					j = getnum_api(&ra, 0);
				}
				if (func06str->usg[i].typ == 0xff) {	/* コマンドライン全体型 */
					p = func06str->argv_all;
					goto func06_typ03;
				} else if (func06str->usg[i].typ == 0xfe) {
					/* コマンドライン部分型 */
					p = func06str->argv0;
					if (j > 0) {
						do {
							while (k < main_argc && (func06str->argv_f[k] & 0x80) == 0)
								k++;
							k++;
						} while (--j > 0);
						k--;
						p = NULL;
						if (k < main_argc)
							p = main_argv[k];
					}
					j = getreg32idx(&ra);
					k &= 0;
					if (p != NULL)
						k |= 1;
					write_reg32(&ra, j, k);
					goto func06_typ03;
				}
				k = 1;
				do {
					while (k < main_argc && (func06str->argv_f[k] & 0x7f) != i)
						k++;
					k++;
				} while (--j >= 0);
				k--;
				p = NULL;
				if (k < main_argc) {
					p = main_argv[k];
					if ((func06str->argv_f[k] & 0x80) == 0)
						p += strlen(func06str->usg[i].tag) + 1;
				}
				if ((func06str->usg[i].flg & 3) != 0 /* || func06str->usg[i].typ == 2 */) {
					j = getreg32idx(&ra);
					k &= 0;
					if (p != NULL)
						k |= 1;
					write_reg32(&ra, j, k);
				}
				if (func06str->usg[i].typ == 0) {
					i = getnum_api(&ra, 0);	/* opt */
					j = getnum_api(&ra, 0);	/* slot */

if (p != NULL && i != 0 && i != 3) printf("debug: cmdlin_fopen: %d %d %s\n", i, j, p);

					if (p != NULL)
						goto fopen_i_j_p;
				} else if (func06str->usg[i].typ == 1) {
					j = getreg32idx(&ra);
					if (p != NULL) {
						k = my_strtol(p, NULL);
						write_reg32(&ra, j, k);
					}
				} else if (func06str->usg[i].typ == 2) {
					/* 何もしない、フラグは必ず省略可能フラグが1 */
				} else if (func06str->usg[i].typ == 3) {
func06_typ03:
					/*  = 2 max ? ? */
					/*  = 3 max ? */
					j = getnum_api(&ra, 0);
					if (j < 2 || 3 < j) {
						puts("func6: mode != 2/3");
						exit(1);
					}
					m = getnum_api(&ra, 0); /* max */
					l |= -1;
					if (j == 2)
						l = getreg32idx(&ra);
					insert_6_0(&ra);
					q = (void *) getnum_api(&ra, 0);
					if (p != NULL) {
						if ((k = strlen(p)) >= m) {
							if (last_putc != '\n' && last_putc != 0)
								putchar('\n');
							if (m < 48)
								printf("Too long Command line (\"%s\" max:%d)\n", p, m);
							else
								printf("Too long Command line (max:%d)\n", m);
							exit(1);
						}
						while (*p != '\0')
							*q++ = *p++;
						if (l < 0)
							*q = '\0';
						if (l >= 0)
							write_reg32(&ra, l, k);
					}
				} else {
					puts("func6: internal error (0)");
					exit(1);
				}
			}
		} else if (i == 0x00ffffff) {
//printf("junk: ");
			i = getnum_api(&ra, 0);
//printf("%d ", i);
			if (i == 1) {
				/* レガシーコマンドライン取得 */
				/* 1つの文字列で与えられる */
				/* エンコードはIBM-US */
				/* opt バッファ最大, バッファ使用量, バッファの中身 */
				/* これはそういうもの。この文脈で6_505がくるとcの前に最大サイズが入る */
				/* そしてgh4モードのときに限りこの6_505は省略できる。 */
				/* 6_505以外を使いたいときは、6を置け。そうすればあとは自由だ */

				/* 典型例 0 (6_505_)2_(1)_max_(6_0_)?(reg32/st[]で受ける)_(6_0_)? */
				/*  = 0 2 max ? ? */
				/*    0 2 max 6 6 3 3 0 ? ? */
				/* 典型例 0 (6_505_)3_(1)_max_(0)_(6_0_)? */
				/*  = 0 3 max ? */

				/* 以下古い */
				/* opt, [書き込み即値], バッファ最大サイズ, バッファアドレス */
				/* 6_5...じゃだめなのか？ ほんとはそのほうがいい */
				/* でこれだと基本形は opt, [書き込み即値], バッファ最大サイズ, バッファの中身 になる */
				/* しかしいつも6でエスケープするのが無駄なので、6を省略。むしろ6が来たら通常へ。 */
				/* そうすると(6)6を表現できないが、それは(6)66で。 */

				if ((func06str->flags & 1) == 0) {
					func06str->flags |= 1;
					func06_setup();
				}

				if (getnum_api(&ra, 0) != 0 && ra.mod != 0) {
					puts("jg01_getcmdlin: opt!=0");
					exit(1);
				}
				k = getnum_api(&ra, 0);
				if (k < 2 || 3 < k) {
					puts("jg01_getcmdlin: mode!=2_ or 3_");
					exit(1);
				}
				insert_mini(&ra, 1);
				if (getnum_api(&ra, 0) != 1) {
					printf("jg01_getcmdlin: mode!=%d_1", k);
					exit(1);
				}
				j = getnum_api(&ra, 0);	/* max */
				/* 本来はここで4がきていないことを確認するべき */
				i |= -1;
				if (k == 2)
					i = getnum_api(&ra, 0); /* *len */
				insert_6_0(&ra);
				p = (UCHAR *) getnum_api(&ra, 0);
				if ((k = strlen(func06str->argv_all)) >= j) {
					if (last_putc != '\n' && last_putc != 0)
						putchar('\n');
					printf("Too long Command line (max:%d)\n", j);
					exit(1);
				}
				q = func06str->argv_all;
				while (*q != '\0')
					*p++ = *q++;
				if (i < 0)
					*p = '\0';

#if 0
				q = main_argv[0];
				while (*q != 0)
					q++;
				q--;
				while (q > main_argv[0] && q[-1] != '\\' && q[-1] != '/')
					q--;
				k = 0;
				l = 1;
				for (;;) {
					for (r = q; *r != 0 && *r != 0x20; r++);
					if (*r != 0) {
						if (j > k)
							p[k] = 0x22;
						k++;
					}
					while (*q != 0) {
						if (j > k)
							p[k] = *q;
						q++;
						k++;
					}
					if (*r != 0) {
						if (j > k)
							p[k] = 0x22;
						k++;
					}
					if (main_argc <= l)
						break;
 					if (j > k)
						p[k] = 0x20;
					k++;
					q = main_argv[l++];
				}
				if (j > k)
					p[k] = 0;	/* これはサービス。この動作は保証されない */
				if (bufmod != 0 && ((i >= 0 && k >= j) || (i < 0 && k >= j - 1))) {
					if (last_putc != '\n' && last_putc != 0)
						putchar('\n');
					printf("Too long Command line (max:%d)\n", k);
					exit(1);
				}
#endif
				if (i >= 0) {
					if (i < 8)
						ra.reg32[7 - i] = k;
					else
						ra.st0[i - 8] = k;
				}
			} else if (i == 3) {
				/* まだtekの自動展開を入れてない（p[1]とかを使う）, -nodecmp */
				i = getnum_api(&ra, 0) >> 3;	/* mode/opt */
				j = getnum_api(&ra, 0);
				if (getnum_api(&ra, 0) != 3) {
					puts("jg01_fopen: mode!=3_");
					exit(1);
				}
				insert_6_0(&ra);
				p = (UCHAR *) getnum_api(&ra, 0);
				if (j < 4 || j > 63) {
					printf("c_api: junk_fopen: slot over %d\n", j);
					exit(1);
				}
#if 0
				if (p[m] != 0) {
					puts("c_api: junk_fopen: mikan error");
					exit(1);
				}
#endif
	fopen_i_j_p:
				if (slot[j].t == 2) {
					if (slot[j].p[2] == 0)
						fclose((FILE *) slot[j].p[0]);
					else {
						if (slot[j].p[1] > 0)
							free((void *) slot[j].p[2]);
					}
					slot[j].t = 0;
				}
				if (i == 0) {
					slot[j].t = 2;
					slot[j].p[0] = (int) fopen(p, "rb");
					slot[j].p[1] = slot[j].p[2] = 0;
					if (slot[j].p[0] == 0) {
						slot[j].t = 0;
						if (errmod != 0) {
							if (last_putc != '\n' && last_putc != 0)
								putchar('\n');
							printf("File read open error: %s\n", p);
							exit(1);
						}
					} else if ((main_flags & 2) == 0) {
						m = fread(tmpbuf, 1, 32, (FILE *) slot[j].p[0]);
						fseek((FILE *) slot[j].p[0], 0, SEEK_SET);
						for (; m < 32; m++)
							tmpbuf[m] = 0;
						if ((m = tek_getsize(tmpbuf)) > 0) {
							fseek((FILE *) slot[j].p[0], 0, SEEK_END);
							k = ftell((FILE *) slot[j].p[0]);
							fseek((FILE *) slot[j].p[0], 0, SEEK_SET);
							p = malloc(m);
							q = malloc(k);
							if (p == NULL || q == NULL) {
								if (p != NULL)
									free(p);
								if (q != NULL)
									free(q);
							} else {
								if (fread(q, 1, k, (FILE *) slot[j].p[0]) != k) {
				decomperr:
									free(p);
									free(q);
									fseek((FILE *) slot[j].p[0], 0, SEEK_SET);
								} else {
									if (m > 0 && tek_decomp(q, p, k) != 0)
										goto decomperr;
									free(q);
									fclose((FILE *) slot[j].p[0]);
									slot[j].p[1] = m;		/* fsiz */
									slot[j].p[2] = (int) p;	/* fbuf */
									slot[j].p[0] = 0;		/* fpos */
								}
							}
						}
					}
				} else if (i == 3) {
					slot[j].t = 2;
					slot[j].p[0] = (int) fopen(p, "wb");
					if (slot[j].p[0] == 0) {
						slot[j].t = 0;
						if (errmod != 0) {
							if (last_putc != '\n' && last_putc != 0)
								putchar('\n');
							printf("File write open error: %s\n", p);
							exit(1);
						}
					}
				} else {
					puts("c_api: junk_fopen: error");
					exit(1);
				}
			} else if (i == 2) {
				getnum_api(&ra, 0); /* optのskip, いろんなタイプのslotをクローズさせる, 連続closeなど */
					/* 個数指定がbit0, タイプ比較がbit1 */
				j = getnum_api(&ra, 0);
				if (j < 4 || j > 63) {
					printf("c_api: junk_fclose: slot over %d\n", j);
					exit(1);
				}
				if (slot[j].t == 2) {
					if (slot[j].p[2] == 0)
						fclose((FILE *) slot[j].p[0]);
					else {
						if (slot[j].p[1] > 0)
							free((void *) slot[j].p[2]);
					}
					slot[i].t = 0;
				}
			} else if (i == 4) {
				i = getnum_api(&ra, 0);	/* opt: bit0 r/w, bit1 partial/full */
				j = getnum_api(&ra, 0);	/* slot */
				n = getnum_api(&ra, 0);	/* 2 or 3 */
				/* fread1:   0 s 2 n (6_0) l b */
				/* fread0:   0 s 3 n         b */
				/* fread1f:  2 s 2 n (6_0) l b */
				/* fread0f:  2 s 3 n         b */
				/* fwrite1:  1 s 2 n (6_0) l b */
				/* fwrite0:  1 s 3   (6_0) l b */
				/* fwrite1f: 3 s 2 n         b */
				/* fwrite0f: 3 s 3           b */
				k = -1;
				if ((i & 1) == 0 || n == 2)
					k = getnum_api(&ra, 0);	/* n */
				o = -1;
				if (((i & 1) == 0 && n == 2) || i == 1)
					o = getnum_api(&ra, 0);	/* *len */
				insert_6_0(&ra);
				p = (void *) getnum_api(&ra, 0);	/* p */
				if (k < 0)
					k = strlen(p);
				if (j < 4 || j > 63) {
					printf("c_api: junk_freadwrite: slot over %d\n", j);
					exit(1);
				}
				m |= -1;
				if (slot[j].t == 2) {
					m &= 0;
					if ((i & 1) == 0) {	/* fread */
						if (n == 3)
							k--;
						if (slot[j].p[2] == 0) {
						//	if (!feof((FILE *) slot[j].p[0]))	/* tolsetのwin32のfeofはバグがある模様 */
								m = fread(p, 1, k, (FILE *) slot[j].p[0]);
						} else {
							h = k;
							while (h > 0 && slot[j].p[0] < slot[j].p[1]) {
								p[m++] = ((char *) slot[j].p[2])[slot[j].p[0]++];
								h--;
							}
						}
						if (n == 3)
							p[m] = 0x00;
						if ((i & 2) != 0 && m >= k) {
							if (last_putc != '\n' && last_putc != 0)
								putchar('\n');
							printf("File read buffer full (max:%d): (filepath)\n", k);
							exit(1);							
						}
//printf("fread: %d(%d)\n", m, k);
					} else {	/* fwrite */
						if (k > 0) {
							m = fwrite(p, 1, k, (FILE *) slot[j].p[0]);
							if ((i & 2) != 0 && m < k) {
								if (last_putc != '\n' && last_putc != 0)
									putchar('\n');
								puts("File write error: (filepath)");
								exit(1);							
							}
						}
//printf("fwrite: %d\n", m);
					}
				}
				if (o >= 0) {
					if (o < 8)
						ra.reg32[7 - o] = m;
					else
						ra.st0[o - 8] = m;
				}
#if 0
			} else if (i == 6) {
				i = getnum_api(&ra);	/* bit0-2 : pの長さ */
				j = getnum_api(&ra);
				if (j < 4 || j > 63) {
					printf("c_api: junk_testslot: slot over %d\n", j);
					exit(1);
				}
				p = (void *) getnum_api(&ra);	/* p */
				*(char *) p = slot[j].t;
#endif
			} else if (i == 7) {
				i = getnum_api(&ra, 0);
				if (i == 1) {
					insert_6_0(&ra);
					j = tek_getsize((char *) getnum_api(&ra, 0));
				} else if (i == 2) {
					i = getnum_api(&ra, 0); /* tekサイズ */
					insert_6_0(&ra);
					j = getnum_api(&ra, 0); /* tekポインタ */
					insert_6_0(&ra);
					j = tek_decomp((char *) j, (char *) getnum_api(&ra, 0), i);
				} else if (i == 3) {
					i = getnum_api(&ra, 0);
					if (getnum_api(&ra, 0) != 2) {
						puts("c_api: rjc error");
						exit(1);
					}
					j = getnum_api(&ra, 0);
					insert_6_0(&ra);
					p = (char *) getnum_api(&ra, 0);
					k = getnum_api(&ra, 0);
					l = getnum_api(&ra, 0);
					j = rjc(p, p + j, k, k + l, k + getnum_api(&ra, 0), i);
				} else {
					printf("c_api: junk_tek: error i=%d\n", i);
					exit(1);
				}
				k = getnum_api(&ra, 0);
				if (k >= 0) {
					if (k < 8)
						ra.reg32[7 - k] = j;
					else
						ra.st0[k - 8] = j;
				}
			} else if (i == 8) {
				/* malloc: 0 n (6_0_)p */
				getnum_api(&ra, 0);
				i = getnum_api(&ra, 0);
				j = getnum_api(&ra, 0);
				q = malloc(i);
				if (q == NULL) {
					printf("Out of memory (need:%d)\n", i);
					exit(1);
				}
				if (j < 8)
					ra.reg32[7 - j] = (int) q;
				else
					ra.st0[j - 8] = (int) q;
			} else if (i == 9) {
				getnum_api(&ra, 0);
				getnum_api(&ra, 0);
				getnum_api(&ra, 0);
			} else if (i == 10) {
				getnum_api(&ra, 0);
				i = getnum_api(&ra, 0) - 32;	/* 分解能(1/4294967296が基準) */
				j = getnum_api(&ra, 0);
				#if (USE_WIN32)
					j *= 1000;
					if (i > 0)
						j <<= i;
					if (i < 0)
						j = (j + ~(-1 << (- i))) >> (- i);
					Sleep(j);
				#elif (USE_POSIX)
				{
					struct timespec ts0, ts1;
					j *= 1000;
					if (i > 0)
						j <<= i;
					if (i < 0)
						j = (j + ~(-1 << (- i))) >> (- i);
					ts0.tv_sec = j / 1000;
					ts0.tv_nsec = (j % 1000) * 1000000;
//printf("%d %d %d\n", j, ts0.tv_sec, ts0.tv_nsec);
//exit(1);
					while (nanosleep(&ts0, &ts1) != 0)
						ts0 = ts1;
				}
				#else
					puts("c_api: junk_sleep: not supported");
					exit(1);
				#endif
			} else if (i == 11) {
				#if (USE_WIN32 || USE_POSIX)
					i = getnum_api(&ra, 0);
					j = inkey();
					k = getreg32idx(&ra);
					if (i < 2 || i > 3) {
						printf("c_api: junk_inkey: bad option (%d)\n", i);
						exit(1);
					}
					if (i == 3) {
						while (j == 0) {
							#if (USE_WIN32)
								Sleep(10);
							#elif (USE_POSIX)
								struct timespec ts;
								ts.tv_sec = 0;
								ts.tv_nsec = 10000000;
								nanosleep(&ts, NULL);
							#endif
							j = inkey();
						}
					}
					write_reg32(&ra, k, j);
				#endif
			} else if (i == 12) {
				for (;;) {
					i = getnum_api(&ra, 0);
					if (i == 0)
						break;
					if (i == 1) {
						j = getnum_api(&ra, 0);
						consctrl1(j, getnum_api(&ra, 0));
					}
					if (i == 2) {
						j = getnum_api(&ra, 0);
						consctrl2(j, getnum_api(&ra, 0));
					}
					if (i == 3)
						consctrl3();
					if (i == 4) {
						j = getnum_api(&ra, 0);
						consctrl4(j, getnum_api(&ra, 0));
					}
				}
			} else if (i == 13) {
				#if (USE_WIN32 || USE_POSIX)
					time_t timer;
					getnum_api(&ra, 0);
					k = getreg32idx(&ra);
					time(&timer);
					write_reg32(&ra, k, (int) timer);
				#endif
			} else {
				printf("c_api: junk_error i=%d\n", i);
				exit(1);
			}
		} else {
			printf("c_api: error i=%d\n", i);
			exit(1);
		}
	}
}

int rjc(UCHAR *p0, UCHAR *p1, int ofs0, int ofs, int ofs1, int mode)
/* mode:	0:decode, 1:encode */
{
	UCHAR *p = p0, *pp = p0 - 4;
	int i, j, k, m = 0;
	while (p < p1) {
		if (0xe8 <= *p && *p <= 0xe9 && &p[4] < p1) {	/* e8 (call32), e9 (jmp32) */
	r32:
			p++;
			if (p - pp < 4)
				continue;
			i = p[0] | p[1] << 8 | p[2] << 16 | p[3] << 24;
			k = i;
			j = (p - p0) + ofs + 4; /* 相対アドレス基点 */
			pp = p;
			if (i == 0 || i == 0x80808080) {
				i ^= 0x80808080;
			//	puts("rjc: warning: extend convert enabled");
				/*
					  この拡張はabcdw006で導入された。それ以前のものについては、
					この条件に当てはまるパターンが存在しないだろうということで、
					互換性に問題なしとしている。この警告は古いbim2g01で生成された
					バイナリにのみ意味があり、古いバイナリでこれが出たら互換性に
					問題があることを示している。
					  のちにテストしたところ、以前のものについてもこの条件に当て
					はまるパターンは存在していた。golib00, obj2bim, naskが該当。

					この拡張が導入されたいきさつ：
					  現在はDLLをサポートしていないので影響はないのだが、将来DLL等を
					サポートした際には、.textセクション内で E8 00 00 00 00 という
					コードがそれなりの頻度で出現する可能性が高い。これをこの拡張
					変換なしで処理すると、それぞれのアドレスは出現位置によって
					異なる数値に変換されることになり、これは圧縮効率を低下させる
					要因となる。
					  ということで、従来のrjc変換の前に00000000と80808080とを交換する
					ことで、ほぼ確実にrjc変換対象から除外し、すべて同じ値に
					変換されることを期待する。ちなみにrjcの後にまた00000000と80808080
					とを交換する。これで0は0のままになる可能性が高い。
					  80808080にした理由だが、もっともrjc変換から除外される可能性が
					高いのは80000000であり、しかしこれはefg01の圧縮が効きにくい可能性が
					あるので、80808080にした。
				*/
			}
			if (mode) { /* encode */
				if (ofs0 - j <= i && i < ofs1 - j)
					i += j;
				else if (ofs1 - j <= i && i < ofs1)
					i -= ofs1 - ofs0;
			} else { /* decode */
				if (ofs0 <= i && i < ofs1)
					i -= j;
				else if (ofs0 - j <= i && i < ofs0)
					i += ofs1 - ofs0;
			}
			if (i == 0 || i == 0x80808080) {
				if (i == 0)
					puts("rjc: warning: extend convert enabled"); /* 上記コメント参照 */
				i ^= 0x80808080;
			}
			if (i != k) {
				p[0] =  i        & 0xff;
				p[1] = (i >>  8) & 0xff;
				p[2] = (i >> 16) & 0xff;
				p[3] = (i >> 24) & 0xff;
				p += 4;
				m++;
			}
			continue;
		}
		p++;
		if (p[-1] == 0x0f && &p[4] < p1 && (p[0] & 0xf0) == 0x80)	/* 0f 8x (jcc32) */
			goto r32;
	}
	return m;
}

void func06_init()
{
	UCHAR *p;
	func06str = malloc(sizeof (struct FUNC06STR));
	func06str->flags &= 0;
	func06str->usgs = 2;
	func06str->def_arg0 = 1;
	func06str->usg[0].tag[0] = '\0';
	func06str->usg[0].flg = 0;
	func06str->usg[0].usg[0] = '\0';
	func06str->usg[0].typ = 0xff;
	func06str->usg[1].tag[0] = '\0';
	func06str->usg[1].flg = 3;
	func06str->usg[1].usg[0] = '\0';
	func06str->usg[1].typ = 0xfe;	/* 3:str */
	for (func06str->argv0 = p = *main_argv; *p != '\0'; p++) {
		if (*p == '\\' || *p == '/')
			func06str->argv0 = p + 1;
	}
	return;
}

char func06_putusage_sub(int i, char f, char g)
{
	int j;
	char c, d = 0;
	g |= func06str->usg[i].flg;
	if ((g & 1) != 0)
		putchar('[');
	if (func06str->usg[i].typ != 2) {
		if (f)
			putchar('[');
	}
	printf(func06str->usg[i].tag);
	if (func06str->usg[i].typ != 2) {
		putchar(':');
		if (f)
			putchar(']');
	}
	for (j = 0; (c = func06str->usg[i].usg[j]) != '\0'; j++) {
		if (c >= ' ')
			putchar(c);
		else if (c == 0x01)
			printf(func06str->usg[i].tag);
		else if (c == 0x02)
			printf("file");
		else if (c == 0x1f)
			d = 1;
		else {
			puts("func06_putusage_sub: error");
			printf("c=0x%02x\n", c);
			exit(1);
		}
	}
	if ((g & 1) != 0)
		putchar(']');
	return d;
}

void func06_putusage()
{
	int i;
	char f = 0, g;
	printf("usage>%s", func06str->argv0);
	if (func06str->usg[0].tag[0] == '\0') {
		puts("   - legacy-mode -");
		return;
	}
	for (i = 0; i < func06str->usgs; i++) {
		if (i == func06str->def_arg0)
			f = 1;
		putchar(' ');
		g = func06_putusage_sub(i, f, 0);
		if ((func06str->usg[i].flg & 2) != 0) {
			putchar(' ');
			func06_putusage_sub(i, f, 1);
			printf("...");
			f = 0;
		}
		if (g)
			printf("\n        ");
	}
	putchar('\n');
	return;
}

void func06_setup()
{
	UCHAR *p, *q = func06str->argv_all, *q0, f;
	int i, j, k;
	p = func06str->argv0;
	while (*p != '\0')
		*q++ = *p++;
	*q++ = ' ';
	for (i = 1; i < main_argc; i++) {
		func06str->argv_f[i] = 0x7e;
		p = main_argv[i];
		for (j = 0; j < func06str->usgs; j++) {
			for (k = 0; func06str->usg[j].tag[k] != '\0' && p[k] != '\0'; k++) {
				if (func06str->usg[j].tag[k] != p[k])
					break;
			}
			if (func06str->usg[j].tag[k] == '\0' && (p[k] == ':' || (p[k] == '\0' && func06str->usg[j].typ == 2))) {
				func06str->argv_f[i] = j;
				func06str->usg[j].flg |= 4;
				break;
			}
			if (strcmp(p, "-efg01-lc") == 0) {
				func06str->argv_f[i] = 0x7d;
				main_flags |= 1;	/* lesser compatible mode */
				break;
			}
			if (strcmp(p, "-efg01-noadc") == 0) {
				func06str->argv_f[i] = 0x7d;
				main_flags |= 2;	/* no auto de-compress mode */
				break;
			}
			if (strcmp(p, "-usage") == 0) {
				main_flags |= 4;
				break;
			}
		}
		f = 0;
		q0 = q;
		if (func06str->argv_f[i] != 0x7d) {
retry:
			p = main_argv[i];
			while (*p != '\0') {
				if (*p == ':') {
					if (p[1] == ':')
						p++;
					else {
						f = 0;
						q = q0;
						break;
					}
				}
				if (*p == ' ' && f == 0) {
					f = 1;
					q = q0;
					*q++ = 0x22;
					goto retry;
				}
				*q++ = *p++;
			}
			if (f != 0)
				*q++ = 0x22;
			if (q > q0) {
				*q++ = ' ';
				func06str->argv_f[i] |= 0x80;
			}
		}
	}
	func06str->argv_f[i] = 0xff;
	q[-1] = '\0';

	j = func06str->def_arg0;
	for (i = 1; i < main_argc; i++) {
		if (func06str->argv_f[i] != 0xfe)
			continue;
		for (;;) {
			if ((func06str->usg[j].flg & 4) == 0)
				break;
			if ((func06str->usg[j].flg & 2) != 0)
				break;
			if (j >= func06str->usgs)
				break;
			j++;
		}
		func06str->usg[j].flg |= 4;
		func06str->argv_f[i] = (func06str->argv_f[i] & 0x80) | j;
	}

	if ((main_flags & 4) != 0) {
		func06_putusage();
		exit(0);
	}

	for (i = 0; i < func06str->usgs; i++) {
		if ((func06str->usg[i].flg & 5) == 0 && func06str->usg[i].typ < 0xfe) {	/* 省略不能なのに該当なし */
			func06_putusage();
			exit(1);
		}
	}

#if 0
	for (i = 0;; i++) {
		printf("%02X ", func06str->argv_f[i]);
		if (func06str->argv_f[i] == 0xff)
			break;
	}
	putchar('\n');
#endif

	return;
}

#define INVALID		-0x7fffffff

char *calc_skipspace(char *p)
{
	for (; *p == ' '; p++) { }	/* スペースを読み飛ばす */
	return p;
}

int calc_getnum(char **pp, int priority)
{
	char *p = *pp;
	int i = INVALID, j;
	p = calc_skipspace(p);

	/* 単項演算子 */
	if (*p == '+') {
		p = calc_skipspace(p + 1);
		i = calc_getnum(&p, 0);
	} else if (*p == '-') {
		p = calc_skipspace(p + 1);
		i = calc_getnum(&p, 0);
		if (i != INVALID) {
			i = - i;
		}
	} else if (*p == '~') {
		p = calc_skipspace(p + 1);
		i = calc_getnum(&p, 0);
		if (i != INVALID) {
			i = ~i;
		}
	} else if (*p == '(') {	/* かっこ */
		p = calc_skipspace(p + 1);
		i = calc_getnum(&p, 9);
		if (*p == ')') {
			p = calc_skipspace(p + 1);
		} else {
			i = INVALID;
		}
	} else if ('0' <= *p && *p <= '9') { /* 数値 */
		i = strtol(p, (void *) /* (const char **) */ &p, 0);
		if (*p == 'k' || *p == 'K') {
			i <<= 10;
			p++;
		}
		if (*p == 'M') {
			i <<= 20;
			p++;
		}
		if (*p == 'G') {
			i <<= 30;
			p++;
		}
	} else { /* エラー */
		i = INVALID;
	}

	/* 二項演算子 */
	for (;;) {
		if (i == INVALID) {
			break;
		}
		p = calc_skipspace(p);
		if (*p == '+' && priority > 2) {
			p = calc_skipspace(p + 1);
			j = calc_getnum(&p, 2);
			if (j != INVALID) {
				i += j;
			} else {
				i = INVALID;
			}
		} else if (*p == '-' && priority > 2) {
			p = calc_skipspace(p + 1);
			j = calc_getnum(&p, 2);
			if (j != INVALID) {
				i -= j;
			} else {
				i = INVALID;
			}
		} else if (*p == '*' && priority > 1) {
			p = calc_skipspace(p + 1);
			j = calc_getnum(&p, 1);
			if (j != INVALID) {
				i *= j;
			} else {
				i = INVALID;
			}
		} else if (*p == '/' && priority > 1) {
			p = calc_skipspace(p + 1);
			j = calc_getnum(&p, 1);
			if (j != INVALID && j != 0) {
				i /= j;
			} else {
				i = INVALID;
			}
		} else if (*p == '%' && priority > 1) {
			p = calc_skipspace(p + 1);
			j = calc_getnum(&p, 1);
			if (j != INVALID && j != 0) {
				i %= j;
			} else {
				i = INVALID;
			}
		} else if (*p == '<' && p[1] == '<' && priority > 3) {
			p = calc_skipspace(p + 2);
			j = calc_getnum(&p, 3);
			if (j != INVALID && j != 0) {
				i <<= j;
			} else {
				i = INVALID;
			}
		} else if (*p == '>' && p[1] == '>' && priority > 3) {
			p = calc_skipspace(p + 2);
			j = calc_getnum(&p, 3);
			if (j != INVALID && j != 0) {
				i >>= j;
			} else {
				i = INVALID;
			}
		} else if (*p == '&' && priority > 4) {
			p = calc_skipspace(p + 1);
			j = calc_getnum(&p, 4);
			if (j != INVALID) {
				i &= j;
			} else {
				i = INVALID;
			}
		} else if (*p == '^' && priority > 5) {
			p = calc_skipspace(p + 1);
			j = calc_getnum(&p, 5);
			if (j != INVALID) {
				i ^= j;
			} else {
				i = INVALID;
			}
		} else if (*p == '|' && priority > 6) {
			p = calc_skipspace(p + 1);
			j = calc_getnum(&p, 6);
			if (j != INVALID) {
				i |= j;
			} else {
				i = INVALID;
			}
		} else {
			break;
		}
	}
	p = calc_skipspace(p);
	*pp = p;
	return i;
}

int my_strtol(UCHAR *p, UCHAR **pp)
{
	int i = calc_getnum((void *) &p, 9);
	if (pp != NULL)
		*pp = p;
	return i;
}

int getreg32idx(struct READ_API *ra)
{
	int i, l;
	l &= 0;
	if (ra->mod == 0)
		l = getlen(&ra->reh4);
	i = getnum_api(ra, 1);
	if (l == 2 && 0x30 <= i && i <= 0x3f)
		i = 0x7ffffff0 | (i & 0xf);
		if (i == 0x35) {
			/* charで4バイト取ってきて、raに格納 */
			puts("getreg32idx: internal error");
			exit(1);
		}
	return i;
}

int read_reg32(struct READ_API *ra, int i)
{
	int v = 0;
	if (i < 8)
		v = ra->reg32[7 - i];
	else if (i < 0x7ffffff0)
		v = ra->st0[i - 8];
	else if (i == 0x7ffffff4)
		;
	else if (0x7ffffff8 <= i && i <= 0x7ffffffb)
		v = *(UCHAR *) &ra->reg32[7 - (i & 3)];
	else if (0x7ffffffc <= i && i <= 0x7fffffff)
		v = *(1 + (UCHAR *) &ra->reg32[7 - (i & 3)]);
	else {
		puts("read_reg32: internal error");
		exit(1);
	}
	return v;
}

void write_reg32(struct READ_API *ra, int i, int v)
{
	if (i < 8)
		ra->reg32[7 - i] = v;
	else if (i < 0x7ffffff0)
		ra->st0[i - 8] = v;
	else if (i == 0x7ffffff4)
		;
	else if (0x7ffffff8 <= i && i <= 0x7ffffffb)
		*(char *) &ra->reg32[7 - (i & 3)] = v;
	else if (0x7ffffffc <= i && i <= 0x7fffffff)
		*(1 + (char *) &ra->reg32[7 - (i & 3)]) = v;
	else {
		puts("write_reg32: internal error");
		exit(1);
	}
	return;
}

int inkey()
{
	#if (USE_WIN32)
		int i;
		if (_kbhit() == 0)
			return 0;
		i = _getch();
		if ((0x20 <= i && i <= 0x7e) || i == '\r' || i == '\t' || i == '\b')
			return i;
		while (_kbhit())
			_getch();
	#elif (USE_POSIX)
		char i;
		do {
			read(fileno(stdin), &i, 1);
			if ((0x20 <= i && i <= 0x7e) || i == '\r' || i == '\t' || i == '\b')
				return i;
		} while(i != 0);
	#endif
	return 0;
}

void consctrl1(int x, int y)
{
	#if (USE_WIN32)
		COORD c;
		HANDLE h;
		h = GetStdHandle(STD_OUTPUT_HANDLE);
		c.X = x;
		c.Y = y;
		SetConsoleCursorPosition(h, c);
	#elif (USE_POSIX)
		printf("\033[%d;%dH", y + 1, x + 1);
	#endif
	return;
}

void consctrl2(int f, int b)
{
	#if (USE_WIN32)
		static char t[] = {
			 0,  9, 10, 11, 12, 13, 14, 15,  8,  1,  2,  3,  4,  5,  6,  7
		};
		HANDLE h;
		h = GetStdHandle(STD_OUTPUT_HANDLE);
		if (0 <= f && f <= 15 && 0 <= b && b <= 15)
			SetConsoleTextAttribute(h, t[f] + t[b] * 16);
	#elif (USE_POSIX)
		static char t[] = {
			 0,  4,  2,  6,  1,  5,  3,  7
		};
		if (0 <= f && f <= 15 && 0 <= b && b <= 15)
			printf("\033[3%d;4%dm", t[f & 7], t[b & 7]);
	#endif
	return;
}

void consctrl3()
{
	#if (USE_WIN32)
		COORD c;
		HANDLE h;
		DWORD dmy, attr;
		CONSOLE_SCREEN_BUFFER_INFO sbinf;
		h = GetStdHandle(STD_OUTPUT_HANDLE);
		GetConsoleScreenBufferInfo(h, &sbinf);
		c.X = 0;
		c.Y = 0;
		if (FillConsoleOutputCharacter(h, ' ', sbinf.dwSize.X * sbinf.dwSize.Y, c, &dmy) == 0)
			return;
		SetConsoleCursorPosition(h, c);
		printf(" \r");
		if (ReadConsoleOutputAttribute(h, (WORD *) &attr, 1, c, &dmy) == 0)
			return;
		if (FillConsoleOutputAttribute(h, attr, sbinf.dwSize.X * sbinf.dwSize.Y, c, &dmy) == 0)
	#elif (USE_POSIX)
		int x, y;
		printf("\033[2J\033[1;1H");
		for (y = 0; y < 25; y++) {
			printf("\033[%d;1H", y + 1);
			for (x = 0; x < 79; x++)
				putchar(' ');
			if (y < 24)
				putchar(' ');
		}
		printf("\033[1;1H");
	#endif
	return;
}

void consctrl4(int x, int y)
{
	#if (USE_WIN32 || USE_POSIX)
		if (x > 80 && y > 25) {
			printf("consctrl1: console size over (%d,%d)\n", x, y);
			exit(1);
		}
	#else
		puts("consctrl1: not  supported");
		exit(1);
	#endif
	return;
}

#if (USE_POSIX)

void signal_handler(int sig)
{
	switch(sig) {
		case SIGINT:
			exit(1);
		case SIGTERM:
			exit(143);
		case SIGTSTP:
			tcsetattr(fileno(stdin), TCSANOW, &save_term);
			raise(SIGSTOP);
			break;
		case SIGCONT:
			tcsetattr(fileno(stdin), TCSANOW, &temp_term);
			break;
	}
}

void end_app()
{
	tcsetattr(fileno(stdin), TCSANOW, &save_term);
}
#endif

