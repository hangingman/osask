/* "gas2nask" (.g01版)
	Copyright(C) 2009 H.Kawai

	usage : >gas2nask [-a] [-e] [-A] input-file output-file

	-a:.textのbalign2を削除
	-e:execcmd系のインライン展開
	-A:.dataのアライン異常を修正
*/

#include <guigui01.h>
#include <string.h>

typedef unsigned char UCHAR;
typedef unsigned int UINT;

static UCHAR *dest0_, *dest1_;

#define FLAG_a		0
#define FLAG_e		1
#define FLAG_A		2

#define	NL			"\n"
#define	LEN_NL		1

#define NULL		0

struct STR_FLAGS {
	UCHAR opt[3];
};

struct stack_alloc {
	UCHAR ibuf[8 * 1024 * 1024];
	UCHAR obuf[8 * 1024 * 1024];
};

#define CMDLIN_FLG_a	0
#define CMDLIN_FLG_e	1
#define CMDLIN_FLG_A	2
#define CMDLIN_OPT_T	3
#define CMDLIN_OPT_D	4
#define CMDLIN_OPT_B	5
#define CMDLIN_OPT_t	6
#define CMDLIN_OPT_d	7
#define CMDLIN_OPT_b	8
#define	CMDLIN_IN		9
#define	CMDLIN_OUT		10

static UCHAR cmdusg[] = {
	0x86, 0x5c, 0x09,
		0x11, '-', 'a', 0x20,
		0x11, '-', 'e', 0x20,
		0x11, '-', 'A', 0x20,
		0x10, 'T', 0x31, '#',
		0x10, 'D', 0x31, '#',
		0x10, 'B', 0x31, '#',
		0x10, 't', 0x31, '#',
		0x10, 'd', 0x31, '#',
		0x10, 'b', 0x31, '#', 0x87,
		0x88, 0x8c,
	0x40
};

static UCHAR *convmain(UCHAR *src0, UCHAR *src1, UCHAR *dest0, UCHAR *dest1, struct STR_FLAGS flags);

static char errflag = 0;

//static UCHAR *param_p[6];

void G01Main()
{
	struct stack_alloc *pwork;
	UCHAR *src1 = 0;
	struct STR_FLAGS flags;
	int j;

	g01_setcmdlin(cmdusg);
	pwork = jg01_malloc(sizeof (struct stack_alloc));

//	for (j = 0; j < 8; j++)
//		flags.opt[j] = 0;

	flags.opt[FLAG_a] = g01_getcmdlin_flag_s(CMDLIN_FLG_a);
	flags.opt[FLAG_e] = g01_getcmdlin_flag_s(CMDLIN_FLG_e);
	flags.opt[FLAG_A] = g01_getcmdlin_flag_s(CMDLIN_FLG_A);
	g01_getcmdlin_fopen_s_0_4(CMDLIN_IN);
	j = jg01_fread1f_4(sizeof (pwork->ibuf), pwork->ibuf);

	src1 = convmain(pwork->ibuf, pwork->ibuf + j, pwork->obuf, pwork->obuf + sizeof (pwork->obuf), flags);
	if (src1 == NULL)
		g01_putstr0_exit1("output filebuf over!");

	g01_getcmdlin_fopen_s_3_5(CMDLIN_OUT);
	jg01_fwrite1f_5(src1 - pwork->obuf, pwork->obuf);
	if (errflag != 0)
		g01_exit_failure_int32(errflag);
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

static UCHAR *checkparam(UCHAR *p);
static void convparam(UCHAR *p, int i);

static UCHAR *skipspace(UCHAR *p)
{
	while (*p <= ' ' && *p != '\n')
		p++;
	return p;
}

static UCHAR *getparam(UCHAR *p)
{
	if (*p != '\n') {
		do {
			p++;
		} while (*p != ',' && *p > ' ' && *p != ';');
	}
	return p;
}

static UCHAR *seek_token_end(UCHAR *s)
{
	while (*s != ':' && *s != '+' && *s > ' ' && *s != '(' && *s != '#' && *s != '*' && *s != '$'
			&& *s != ')' && *s != 0x22 && *s != ',' && *s != ';' && *s != '-')
		s++;
	return s;
}

static UCHAR *opcmp(UCHAR *q, UCHAR *s)
{
	int i;
	for (i = 0; i < 8; i++) {
		if (q[i] == ' ' && (s[i] <= ' ' || s[i] == ';'))
			goto match;
		if (q[i] != s[i])
			goto mismatch;
	}
	if (s[i] <= ' ' || s[i] == ';')
		goto match;
mismatch:
	return NULL;
match:
	return skipspace(&s[i]);
}

static char my_strcmp(const char *s, const char *t)
{
	while (*s == *t) {
		s++;
		t++;
		if (*t == '\0' && (*s <= ' ' || *s == ';'))
			return 1;
	}
	return 0;
}

static UCHAR *convmain(UCHAR *src0, UCHAR *src1, UCHAR *dest0, UCHAR *dest1, struct STR_FLAGS flags)
{
	UCHAR *p, *q, *r, *s, *t, *dld0s = 0, *dld0d = 0;
	UCHAR intext = 0, c, flag, flag1, flag2, dalgn0 = 1, dalgn1 = 1, strtmp[32];
	static UCHAR no_params[][16] = {
		"cld     CLD",
		"cltd    CDQ",
		"cmpsb   CMPSB",
		"cwtd    CWD",
		"cwtl    CWDE",
		"cbtw    CBW",
		"fabs    FABS",
		"fchs    FCHS",
		"fcos    FCOS",
		"fld1    FLD1",
		"fldz    FLDZ",
		"fsin    FSIN",
		"fsqrt   FSQRT",
		"fucom   FUCOM",
		"fucomp  FUCOMP",
		"fucompp FUCOMPP",
		"leave   LEAVE",
		"movsb   MOVSB",
		"movsl   MOVSD",
		"rep     REP",
		"repe    REPE",
		"repne   REPNE",
		"ret     RET",
		"sahf    SAHF",
		"scasb   SCASB",
		"stosb   STOSB",
		"stosl   STOSD",
		"\0"
	};
	static UCHAR one_param[][16] = {
		"0call    CALL",
		"1decb    DEC",
		"4decl    DEC",
		"2decw    DEC",
		"1divb    DIV",
		"4divl    DIV",
		"2divw    DIV",
		"8faddl   FADD",
		"4fadds   FADD",
		"8fdivl   FDIV",
		"4fdivs   FDIV",
		"8fdivrl  FDIVR",
		"4fdivrs  FDIVR",
		"4fidivl  FIDIV",
		"4fidivrl FIDIVR",
		"2filds   FILD",
		"4fildl   FILD",
		"8fildq   FILD",
		"4fimull  FIMUL",
		"2fistps  FISTP",
		"4fistpl  FISTP",
		"8fistpq  FISTP",
		"2fists   FIST",
		"4fistl   FIST",
		"3fld     FLD",
		"2fldcw   FLDCW",
		"8fldl    FLD",
		"4flds    FLD",
		"8fmull   FMUL",
		"4fmuls   FMUL",
		"2fnstcw  FNSTCW",
		"2fnstsw  FNSTSW",
		"8fstl    FST",
		"3fstp    FSTP",
		"8fstpl   FSTP",
		"4fstps   FSTP",
		"4fsts    FST",
		"8fsubl   FSUB",
		"8fsubrl  FSUBR",
		"4fsubrs  FSUBR",
		"3fucom   FUCOM",
		"3fucomp  FUCOMP",
		"3fxch    FXCH",
		"1idivb   IDIV",
		"4idivl   IDIV",
		"2idivw   IDIV",
		"1imulb   IMUL",
		"4imull   IMUL",
		"2imulw   IMUL",
		"1incb    INC",
		"4incl    INC",
		"2incw    INC",
		"0ja      JA",
		"0jae     JAE",
		"0jb      JB",
		"0jbe     JBE",
		"0je      JE",
		"0jg      JG",
		"0jge     JGE",
		"0jl      JL",
		"0jle     JLE",
		"0jmp     JMP",
		"0jne     JNE",
		"0jnp     JNP",
		"0jns     JNS",
		"0jp      JP",
		"0js      JS",
		"0loop    LOOP",
		"1mulb    MUL",
		"4mull    MUL",
		"2mulw    MUL",
		"1negb    NEG",
		"4negl    NEG",
		"2negw    NEG",
		"1notb    NOT",
		"4notl    NOT",
		"2notw    NOT",
		"4popl    POP",
		"4pushl   PUSH",
		"4ret     RET",
		"1seta    SETA",
		"1setae   SETAE",
		"1setb    SETB",
		"1setbe   SETBE",
		"1sete    SETE",
		"1setg    SETG",
		"1setge   SETGE",
		"1setl    SETL",
		"1setle   SETLE",
		"1setne   SETNE",
		"1setnp   SETNP",
		"1setns   SETNS",
		"1sets    SETS",
		"1setp    SETP",
		"\0"
	};
	static UCHAR one_shifts[][16] = {
		"1salb    SAL",
		"4sall    SAL",
		"2salw    SAL",
		"1sarb    SAR",
		"4sarl    SAR",
		"2sarw    SAR",
		"1shrb    SHR",
		"4shrl    SHR",
		"2shrw    SHR",
		"\0"
	};
	static UCHAR two_params[][16] = {
		"1addb    ADD",
		"4addl    ADD",
		"2addw    ADD",
		"1andb    AND",
		"4andl    AND",
		"2andw    AND",
		"1cmpb    CMP",
		"4cmpl    CMP",
		"2cmpw    CMP",
		"3fadd    FADD",
		"3faddp   FADDP",
		"3fdivp   FDIVRP",
		"3fdivrp  FDIVP",
		"3fmul    FMUL",
		"3fmulp   FMULP",
		"3fsubp   FSUBRP",
		"3fsubrp  FSUBP",
		"4imull   IMUL",
		"2imulw   IMUL",
		"1inb     IN",
		"4inl     IN",
		"2inw     IN",
		"4leal    LEA",
		"1movb    MOV",
		"4movl    MOV",
		"2movw    MOV",
		"1movsbl  MOVSX",
		"1movsbw  MOVSX",
		"2movswl  MOVSX",
		"1movzbl  MOVZX",
		"1movzbw  MOVZX",
		"2movzwl  MOVZX",
		"1orb     OR",
		"4orl     OR",
		"2orw     OR",
		"1outb    OUT",
		"4outl    OUT",
		"2outw    OUT",
		"1rolb    ROL",
		"4roll    ROL",
		"2rolw    ROL",
		"1salb    SAL",
		"4sall    SAL",
		"2salw    SAL",
		"1sarb    SAR",
		"4sarl    SAR",
		"2sarw    SAR",
		"4sbbl    SBB",
		"1shrb    SHR",
		"4shrl    SHR",
		"2shrw    SHR",
		"1subb    SUB",
		"4subl    SUB",
		"2subw    SUB",
		"1testb   TEST",
		"4testl   TEST",
		"2testw   TEST",
		"1xchgb   XCHG",
		"4xchgl   XCHG",
		"2xchgw   XCHG",
		"1xorb    XOR",
		"4xorl    XOR",
		"2xorw    XOR",
		"\0"
	};

	static UCHAR three_params[][16] = {
		"4imull   IMUL",
		"2imulw   IMUL",
		"4shldl   SHLD",
		"4shrdl   SHRD",
		"\0"
	};

	static UCHAR fdivfsub[][16] = {
		"1fdiv    FDIV",
		"0fdivr   FDIV",
		"1fsub    FSUB",
		"0fsubr   FSUB", /* GOには不要 */
		"\0"
	};

	int i;

	dest0_ = dest0;
	dest1_ = dest1;

	output0("[FORMAT \"WCOFF\"]" NL);
	output0(
		"[INSTRSET \"i486p\"]" NL
		"[OPTIMIZE 1]" NL
		"[OPTION 1]" NL
		"[BITS 32]" NL
	);

	if (g01_getcmdlin_str_o0(CMDLIN_OPT_T, 32, strtmp)) {
		output0("[SECTION .text align=");
		output0(strtmp);
		output0("]" NL);
	}
	if (g01_getcmdlin_str_o0(CMDLIN_OPT_D, 32, strtmp)) {
		output0("[SECTION .data align=");
		output0(strtmp);
		output0("]" NL);
	}
	if (g01_getcmdlin_str_o0(CMDLIN_OPT_B, 32, strtmp)) {
		output0("[SECTION .bss align=");
		output0(strtmp);
		output0("]" NL);
	}

	if (g01_getcmdlin_str_o0(CMDLIN_OPT_t, 32, strtmp)) {
		output0("[SECTION .text]" NL "\tALIGN\t");
		output0(strtmp);
		output0(NL);
	}
	if (g01_getcmdlin_str_o0(CMDLIN_OPT_d, 32, strtmp)) {
		output0("[SECTION .data]" NL "\tALIGNB\t");
		output0(strtmp);
		output0(NL);
	}
	if (g01_getcmdlin_str_o0(CMDLIN_OPT_b, 32, strtmp)) {
		output0("[SECTION .bss]" NL "\tALIGNB\t");
		output0(strtmp);
		output0(NL);
	}

	/* EXTERN抽出 */
	if (src0 > &src1[-1])
		goto no_src;
	if (src1[-1] != '\n') {
		errflag = 1;
		goto no_src;
	}
	/* アルゴリズム：トークンで区切る。_で始まるトークンを登録 */
	r = src0;
	p = dest0_ + (dest1 - dest0_) / 2;
	q = p;
	flag2 = 1;
	do {
		r = skipspace(r);
		s = seek_token_end(r);
		if (r[0] == '.') {
			if (r[1] == 't' && r[2] == 'e' && r[3] == 'x' && r[4] == 't' && r[5] <= ' ') {
				flag2 = 1;
				goto ext_skip2;
			}
			if (r[1] == 'd' && r[2] == 'a' && r[3] == 't' && r[4] == 'a' && r[5] <= ' ') {
				flag2 = 1;
				goto ext_skip2;
			}
			if (r[1] == 'b' && r[2] == 's' && r[3] == 's' && r[4] <= ' ') {
				flag2 = 1;
				goto ext_skip2;
			}
			if (r[1] == 's' && r[2] == 'e' && r[3] == 'c' && r[4] == 't' && r[5] == 'i' && r[6] == 'o' && r[7] == 'n' && r[8] <= ' ') {
				flag2 = 0;
				goto ext_skip2;
			}
		}
		if (r[0] == '#')
			goto ext_skip2;
		if (r[0] == '/')
			goto ext_skip2;
		if (flag2 == 0)
			goto ext_skip2;
		if (s - r != 0 && *s == ':' && *r == '_') {
			/* ラベル定義検出 */
			if (s - r > 255)
				goto ext_skip0;
			c = s - r;
			if (c == 0)
				goto ext_skip0;
			if (q > p) {
				t = p;
				do {
					t += 2;
					if (t[-2] != c) {
	ext_next_label2:
						t += t[-2];
						continue;
					}
					for (i = c - 1; i >= 0; i--) {
						if (r[i] != t[i])
							goto ext_next_label2;
					}
					t[-1] = 0x01; /* internal */
					goto ext_skip0; /* 二重登録回避 */
				} while (t < q);
				if (q + c + 2 > dest1)
					goto err;
				q[0] = c; /* len */ 
				q[1] = 0x01; /* internal */
				q += 2;
				do {
					*q++ = *r++;
				} while (--c);
			}
		}
ext_skip0:
		for (;;) {
			if (*s == 0x22) {
				do {
					s++;
					if (*s == '\\' && s[1] == 0x22)
						s += 2;
					if (*s == '\n')
						break;
				} while (*s != 0x22);
			}
			if (*s == ';')
				break;
			if (*s == '#')
				break;
			if (*s == '\r')
				break;
			if (*s == '\n')
				break;
			r = skipspace(s + 1);
			s = seek_token_end(r);
			if (*r == '_') {
				if (s - r > 255)
					goto ext_skip1; /* ながすぎ */
				c = s - r;
				if (q > p) {
					t = p;
					do {
						t += 2;
						if (t[-2] != c) {
		ext_next_label:
							t += t[-2];
							continue;
						}
						for (i = c - 1; i >= 0; i--) {
							if (r[i] != t[i])
								goto ext_next_label;
						}
						goto ext_skip1; /* 二重登録回避 */
					} while (t < q);
				}
				if (q + c + 2 > dest1)
					goto err;
				q[0] = c; /* len */ 
				q[1] = 0; /* external */
				q += 2;
				do {
					*q++ = *r++;
				} while (--c);
			}
ext_skip1:
			;
		}
ext_skip2:
		while (*s != '\n')
			s++;
		r = s + 1;
	} while (r < src1);
	if (q > p) {
		do {
			p += 2;
			if (p[-1] == 0x00) {
				if (dest0_ + 8 + p[-2] + LEN_NL > p)
					goto err;
				output0("\tEXTERN\t");
				output(p[-2], p);
				output0(NL);
			}
			p += p[-2];
		} while (p < q);
	}

	/* 一般変換 */
	flag2 = 1;
	do {
		/* 常に文末に'\n'があることを想定してよい */
		src0 = skipspace(src0);
retry1:
		/* 擬似命令 */
		if (*src0 == '.') {
			if (src0[1] == 'f' && src0[2] == 'i' && src0[3] == 'l' && src0[4] == 'e' && src0[5] <= ' ') {
				/* .file */
				src0 = skipspace(src0 + 5);
				if (*src0 != 0x22) {
	err_skip:
					g01_putstr0("skip:");
					p = src0;
					while (*p++ != '\n');
					g01_putstr1(p - src0, src0);
					errflag = 1;
					src0 = p;
					continue;
				}
				p = src0;
				do {
					p++;
				} while (*p != 0x22 && *p != '\n');
				output0("[FILE ");
				output(p - src0, src0);
				src0 = p;
				output0("\"]" NL);
	skip:
				while (*src0 != '\n')
					src0++;
				src0++;
				continue;
			}
			if (src0[1] == 't' && src0[2] == 'e' && src0[3] == 'x' && src0[4] == 't' && src0[5] <= ' ') {
				/* .text */
				intext = 1;
				flag2 = 1;
				output0("[SECTION .text]" NL);
				goto skip;
			}
			if (src0[1] == 'd' && src0[2] == 'a' && src0[3] == 't' && src0[4] == 'a' && src0[5] <= ' ') {
				/* .data */
				intext = 0;
				flag2 = 1;
				output0("[SECTION .data]" NL);
				dld0s = dld0d = 0;
				dalgn0 = dalgn1 = 1;
				goto skip;
			}
			if (src0[1] == 'b' && src0[2] == 's' && src0[3] == 's' && src0[4] <= ' ') {
				/* .bss */
				intext = 0;
				flag2 = 1;
				output0("[SECTION .bss]" NL);
				goto skip;
			}
			if (src0[1] == 'g' && src0[2] == 'l' && src0[3] == 'o' && src0[4] == 'b' && src0[5] == 'l' && src0[6] <= ' ') {
				/* .globl */
				src0 += 6;
				output0("\tGLOBAL\t");
				for (;;) {
					src0 = skipspace(src0);
					p = getparam(src0);
					output(p - src0, src0);
					if (*p != ',')
						break;
					output0(",");
					src0 = p + 1;
				}
				output0(NL);
				goto skip;
			}
			if (src0[1] == 's' && src0[2] == 'e' && src0[3] == 'c' && src0[4] == 't' && src0[5] == 'i' &&
				src0[6] == 'o' && src0[7] == 'n' && src0[8] <= ' ') {
				/* .section */
				flag2 = 0;
				goto skip;
			}
			if (src0[1] == 'd' && src0[2] == 'e' && src0[3] == 'f' && src0[4] <= ' ')
				goto skip; /* .def は飛ばす */
			if (src0[1] == 'p' && src0[2] == '2' && src0[3] == 'a' && src0[4] == 'l' && src0[5] == 'i' && src0[6] == 'g' &&
					src0[7] == 'n' && src0[8] <= ' ')
				goto skip; /* .p2align は飛ばす */
			if (flag2 == 0)
				goto skip;
			if (src0[1] == 'b' && src0[2] == 'a' && src0[3] == 'l' && src0[4] == 'i' && src0[5] == 'g' && src0[6] == 'n'  && src0[7] <= ' ') {
				/* .balign */
				src0 = skipspace(src0 + 7);
				p = getparam(src0);
				if (intext != 0 && flags.opt[FLAG_a] != 0 && (p - src0) == 1 && *src0 == '2')
					goto skip; 
				if (intext)
					output0("\tALIGN\t");
				else
					output0("\tALIGNB\t");
				output(p - src0, src0);
				src0 = p;
				output0(NL);
				goto skip;
			}
			if (src0[1] == 's' && src0[2] == 'p' && src0[3] == 'a' && src0[4] == 'c' && src0[5] == 'e' && src0[6] <= ' ') {
				/* .space */
				src0 = skipspace(src0 + 6);
				p = getparam(src0);
				output0("\tRESB\t");
				output(p - src0, src0);
				src0 = p;
				output0(NL);
				goto skip;
			}
			if (src0[1] == 'l' && src0[2] == 'o' && src0[3] == 'n' && src0[4] == 'g' && src0[5] <= ' ') {
				/* .long */
				if (flags.opt[FLAG_A] != 0 && intext == 0) {
					if (dld0s == 0) {
						dld0s = src0;
						dld0d = dest0_;
					}
					dalgn1 = 4;
					if (dalgn0 < 4) {
						src0 = dld0s;
						dest0_ = dld0d;
						output0("\tALIGNB\t4" NL);
						goto retry1;
					}
				}
				src0 += 5;
				output0("\tDD\t");
	dd_dw_db:
				for (;;) {
					src0 = skipspace(src0);
					p = src0;
					if (*src0 == '\n')
						break;
					do {
						p++;
					} while (*p != ',' && *p != ';' && *p != '\r' && *p != '\n');
					output(p - src0, src0);
					if (*p != ',')
						break;
					output0(",");
					src0 = p + 1;
				}
				output0(NL);
				goto skip;
			}
			if (src0[1] == 'b' && src0[2] == 'y' && src0[3] == 't' && src0[4] == 'e' && src0[5] <= ' ') {
				/* .byte */
				if (flags.opt[FLAG_A] != 0 && intext == 0) {
					if (dld0s == 0) {
						dld0s = src0;
						dld0d = dest0_;
					}
					dalgn1 = 1;
				}
				src0 += 5;
				output0("\tDB\t");
				goto dd_dw_db;
			}
			if (src0[1] == 'w' && src0[2] == 'o' && src0[3] == 'r' && src0[4] == 'd' && src0[5] <= ' ') {
				/* .word */
				if (flags.opt[FLAG_A] != 0 && intext == 0) {
					if (dld0s == 0) {
						dld0s = src0;
						dld0d = dest0_;
					}
					dalgn1 = 2;
					if (dalgn0 < 2) {
						src0 = dld0s;
						dest0_ = dld0d;
						output0("\tALIGNB\t2" NL);
						goto retry1;
					}
				}
				src0 += 5;
				output0("\tDW\t");
				goto dd_dw_db;
			}
			if (src0[1] == 'a' && src0[2] == 's' && src0[3] == 'c' && src0[4] == 'i' && src0[5] == 'i' && src0[6] <= ' ') {
				/* .ascii */
				if (flags.opt[FLAG_A] != 0 && intext == 0) {
					if (dld0s == 0) {
						dld0s = src0;
						dld0d = dest0_;
					}
					dalgn1 = 1;
				}
				src0 += 6;
				output0("\tDB\t");
				flag1 = 0;
				for (;;) {
					src0 = skipspace(src0);
					p = src0;
					if (*p++ != 0x22)
						break;
					flag = 0;
					for (;;) {
						c = *p++;
						if (c == 0x22)
							break;
						if (c == '\r')
							break; /* エラーだが面倒なので見逃してやる */
						if (c == '\n')
							break; /* エラーだが面倒なので見逃してやる */
						if (c != '\\') {
							if (flag == 0) {
								if (flag1)
									output0(",");
								output0("\x22");
								flag = 1;
							}
							output(1, &c);
							flag1 = 1;
							continue;
						}
						c = *p++;
						if (c == '\r')
							break; /* エラーだが面倒なので見逃してやる */
						if (c == '\n')
							break; /* エラーだが面倒なので見逃してやる */
						if (flag) {
							output0("\x22");
							flag = 0;
						}
						if (flag1)
							output0(",");
						if ('0' <= c && c <= '7') {
							c -= '0';
							if ('0' <= *p && *p <= '7')
								c = (c << 3) | (*p++ - '0');
							if ('0' <= *p && *p <= '7')
								c = (c << 3) | (*p++ - '0');
						}
						output0("0x");
						output(1, &"0123456789ABCDEF"[(c >> 4) & 0x0f]);
						output(1, &"0123456789ABCDEF"[c & 0x0f]);
						flag1 = 1;
					}
					if (flag) {
						output(1, "\x22");
					//	flag = 0;
					}
					src0 = skipspace(p);
					if (*src0 == '\n')
						break;
					if (*src0 == ';')
						break;
					if (*src0++ != ',')
						break; /* エラーだが面倒なので見逃してやる */
				}
				output0(NL);
				goto skip;
			}
			/* 該当なし */
			goto err_skip;
		}

		if (src0[0] == '#')
			goto skip;
		if (src0[0] == '/')
			goto skip;

		/* 通常命令 */
		if (flag2 == 0)
			goto skip;
retry:
		if (*src0 == '\n') {
			src0++;
			output0(NL);
			continue;
		}
		p = src0;
		do {
			p++;
		} while (*p > ' ' && *p != ';');
		if (p[-1] == ':') {
			/* ラベル定義 */
			if (flags.opt[FLAG_A] != 0 && intext == 0) {
				dld0s = src0;
				dld0d = dest0_;
				dalgn0 = dalgn1;
			}
			output(p - src0, src0);
			src0 = skipspace(p);
			goto retry;
		}
		output(1, "\t");

		if (p - src0 > 8)
			goto unknown;

		/* パラメータなし命令 */
		for (i = 0; q = &no_params[i][0], *q != '\0'; i++) {
			if ((r = opcmp(&q[0], src0)) != 0) {
				if (*r == '\n' || *r == ';') { /* パラメータがあればスルー */
					output0(&q[8]);
					output0(NL);
					goto skip;
				}
			}
		}

		if (flags.opt[FLAG_e]) {
			if ((r = opcmp("call ", src0)) != 0) {
				if (my_strcmp(r, "_lib_execcmd0")) {
					output0(
						"PUSH EBX" NL "LEA EBX,[ESP+4]" NL "CALL 0xc7:0" NL "POP EBX" NL
					);
					goto skip;
				}
				if (my_strcmp(r, "_lib_execcmd1")) {
					output0(
						"POP EAX" NL "PUSH EBX" NL "LEA EBX,[ESP+4]" NL "CALL 0xc7:0" NL
						"POP EBX" NL "PUSH EAX" NL "MOV EAX,[SS:ESP+EAX-8]" NL
					);
					goto skip;
				}
				if (my_strcmp(r, "_lib_execcmd2")) {
					output0(
						"POP EAX" NL "PUSH EBX" NL "LEA EBX,[ESP+4]" NL "CALL 0xc7:0" NL
						"POP EBX" NL "PUSH EAX" NL "MOV EAX,[SS:ESP+EAX]" NL
					);
					goto skip;
				}
			}
		}

		/* パラメータ1つ命令 */
		for (i = 0; q = &one_param[i][0], *q != '\0'; i++) {
			if ((r = opcmp(&q[1], src0)) != 0) {
				if (*r != '\n' && *r != ';') { /* パラメータがなければスルー */
					r = checkparam(r); /* コンマかセミコロン、\nまで読み飛ばす */
					if (r == NULL)
						break;
					if (*r == ',')
						break;
					output0(&q[9]);
					output0("\t");
					convparam(p, *q - '0');
					output0(NL);
					goto skip;
				}
			}
		}

		/* シフト系パラメータ1つ命令 */
		for (i = 0; q = &one_shifts[i][0], *q != '\0'; i++) {
			if ((r = opcmp(&q[1], src0)) != 0) {
				if (*r != '\n' && *r != ';') { /* パラメータがなければスルー */
					r = checkparam(r); /* コンマかセミコロン、\nまで読み飛ばす */
					if (r == NULL)
						break;
					if (*r == ',')
						break;
					output0(&q[9]);
					output0("\t");
					convparam(p, *q - '0');
					output0(",1" NL);
					goto skip;
				}
			}
		}

		/* パラメータ2つ命令(交換タイプ) */
		for (i = 0; q = &two_params[i][0], *q != '\0'; i++) {
			if ((r = opcmp(&q[1], src0)) != 0) {
				if (*r != '\n' && *r != ';') { /* パラメータがなければスルー */
					r = checkparam(p);
					if (r == NULL)
						break;
					if (*r != ',')
						break;
					s = checkparam(r + 1);
					if (s == NULL)
						break;
					if (*s == ',')
						break;
					output0(&q[9]);
					output0("\t");
					convparam(r + 1, *q - '0');
					output0(",");
					convparam(p, *q - '0');
					output0(NL);
					goto skip;
				}
			}
		}

		/* パラメータ3つ命令(逆順タイプ) */
		for (i = 0; q = &three_params[i][0], *q != '\0'; i++) {
			if ((r = opcmp(&q[1], src0)) != 0) {
				if (*r != '\n' && *r != ';') { /* パラメータがなければスルー */
					r = checkparam(r);
					if (r == NULL)
						break;
					if (*r != ',')
						break;
					s = checkparam(r + 1);
					if (s == NULL)
						break;
					if (*s != ',')
						break;
					t = checkparam(s + 1);
					if (t == NULL)
						break;
					if (*t == ',')
						break;
					output0(&q[9]);
					output0("\t");
					convparam(s + 1, *q - '0');
					output0(",");
					convparam(r + 1, *q - '0');
					output0(",");
					convparam(p, *q - '0');
					output0(NL);
					goto skip;
				}
			}
		}

		/* パラメータ2つ命令(交換タイプ, FDIV/FSUB専用) */
		for (i = 0; q = &fdivfsub[i][0], *q != '\0'; i++) {
			if ((r = opcmp(&q[1], src0)) != 0) {
				if (*r != '\n' && *r != ';') { /* パラメータがなければスルー */
					r = checkparam(r);
					if (r == NULL)
						break;
					if (*r != ',')
						break;
					s = checkparam(r + 1);
					if (s == NULL)
						break;
					if (*s == ',')
						break;
					t = skipspace(p);
					c = '0';
					output0(&q[9]);
					if (t[0] == '%' && t[1] == 's' && t[2] == 't' && t[3] != '(')
						c = '1';
					if (c == *q)
						output(1, "R");
					output0("\t");
					convparam(r + 1, 3);
					output0(",");
					convparam(p, 3);
					output0(NL);
					goto skip;
				}
			}
		}

unknown:
		/* 該当なし */
		output0(NL);
		goto err_skip;

	} while (src0 < src1);

no_src:
	return dest0_;
err:
	return NULL;
}

static UCHAR *checkparam(UCHAR *p)
{
	p = skipspace(p);
	if (*p == '%') { /* reg */
		p = skipspace(p + 1);
		if (*p == ',')
			goto err;
		if (*p == '\n')
			goto err;
		if (*p == ';')
			goto err;
		p = getparam(p);
		p = skipspace(p);
		goto fin;
	}

	if (*p == '$') { /* imm */
		p = skipspace(p + 1);
		if (*p == ',')
			goto err;
		if (*p == '\n')
			goto err;
		if (*p == ';')
			goto err;
		do {
			p++;
		} while (*p != '\n' && *p != ',' && *p != ';');
		goto fin;
	}

	if (*p == '\n')
		goto err;
	if (*p == ',')
		goto err;
	if (*p == ';')
		goto err;

	/* mem */
	if (*p != '(') {
		do {
			p++;
			if (*p == '\n')
				goto fin;
			if (*p == ',')
				goto fin;
			if (*p == ';')
				goto fin;
		} while (*p != '(');
	}
	p = skipspace(p + 1);
	if (*p == '%') { /* base */
		p = skipspace(p + 1);
		if (*p == ',')
			goto err;
		if (*p == ')')
			goto err;
		do {
			if (*p++ == '\n')
				goto err;
		} while (*p != ',' && *p != ')');
	}
	if (*p == ')') {
fin2:	
		p++;
		goto fin;
	}
	if (*p != ',')
		goto err;
	p = skipspace(p + 1);
	if (*p == ')')
		goto fin2;
	if (*p != '1' && *p != '%')
		goto err;
	if (*p == '%') {
		p = skipspace(p + 1);
		if (*p == ',')
			goto err;
		if (*p == ')')
			goto err;
		do {
			if (*p++ == '\n')
				goto err;
		} while (*p != ',' && *p != ')');
	}
	if (*p == '1')
		p = skipspace(p + 1);
	if (*p == ')')
		goto fin2;
	do {
		if (*p++ == '\n')
			goto err;
	} while (*p != ')');
	goto fin2;

err:
	p = NULL;
fin:
	return p;
}

static void convparam(UCHAR *p, int i)
{
	UCHAR c, *q;
retry:
	p = skipspace(p);
	if (*p == '%') { /* reg */
		p = skipspace(p + 1);
		if (p[0] == 's' && p[1] == 't' && p[2] != '(') {
			if (dest0_ + 3 >= dest1_)
				dest0_ = NULL;
			if (dest0_ == NULL)
				goto fin;
			p += 2;
			dest0_[0] = 'S';
			dest0_[1] = 'T';
			dest0_[2] = '0';
			dest0_ += 3;
		}
		for (;;) {
			c = *p++;
			if (c == ',')
				goto fin;
			if (c == ';')
				goto fin;
			if (c <= ' ')
				goto fin;
			if ('a' <= c && c <= 'z')
				c += 'A' - 'a';
			if (c == '(')
				continue;
			if (c == ')')
				continue;
			if (dest0_ >= dest1_)
				dest0_ = NULL;
			if (dest0_ == NULL)
				goto fin;
			*dest0_++ = c;
		}
	}
	if (*p == '$') { /* imm */
		p = skipspace(p + 1);
imm:
		for (;;) {
			c = *p++;
			if (c == ',')
				goto fin;
			if (c == ';')
				goto fin;
			if (c == '\n')
				goto fin;
			if (c == '\r')
				goto fin;
			if (dest0_ >= dest1_)
				dest0_ = NULL;
			if (dest0_ == NULL)
				goto fin;
			*dest0_++ = c;
		}
	}
	if (i == 0) { /* callやjmpなど */
		i += 4;
		if (*p == '*') {
			p++;
			goto retry;
		}
		q = p;
		for (;;) {
			if (*q == ',')
				break;
			if (*q == ';')
				break;
			if (*q == '\n')
				break;
			if (*q == '(')
				goto mem;
			q++;
		}
		goto imm;
	}
mem:
	/* mem */
	if (i == 1)
		output0("BYTE [");
	if (i == 2)
		output0("WORD [");
	if (i == 3)
		output0("[");
	if (i == 4)
		output0("DWORD [");
	if (i == 8)
		output0("QWORD [");
	if (i == 9)
		output0("TWORD [");
	c = 0;
	if (*p != '(') {
		do {
			c = *p++;
			if (c == ',')
				goto fin2;
			if (c == ';')
				goto fin2;
			if (c == '\n')
				goto fin2;
			if (c == '\r')
				goto fin2;
			if (dest0_ >= dest1_)
				dest0_ = NULL;
			if (dest0_ == NULL)
				goto fin;
			*dest0_++ = c;
		} while (*p != '(');
	}
	p = skipspace(p + 1);
	if (*p == '%') { /* base */
		p = skipspace(p + 1);
		if (c)
			output0("+");
		do {
			c = *p++;
			if ('a' <= c && c <= 'z')
				c += 'A' - 'a';
			if (dest0_ >= dest1_)
				dest0_ = NULL;
			if (dest0_ == NULL)
				goto fin;
			*dest0_++ = c;
		} while (*p != ',' && *p != ')');
	}
	if (*p == ')')
		goto fin2;
	p = skipspace(p + 1);
	if (*p == ')')
		goto fin2;
	if (*p == '%') { /* index */
		p = skipspace(p + 1);
		if (c)
			output0("+");
		do {
			c = *p++;
			if ('a' <= c && c <= 'z')
				c += 'A' - 'a';
			if (dest0_ >= dest1_)
				dest0_ = NULL;
			if (dest0_ == NULL)
				goto fin;
			*dest0_++ = c;
		} while (*p != ',' && *p != ')');
		output0("*");
		c = 1;
	}
	if (*p == '1')
		p = skipspace(p + 1);
	if (*p == ')') {
fin3:
		if (c == 1)
			output(1, "1");
		goto fin2;
	}
	p = skipspace(p + 1);
	if (*p == ')')
		goto fin3;
	do {
		if (dest0_ >= dest1_)
			dest0_ = NULL;
		if (dest0_ == NULL)
			goto fin;
		*dest0_++ = *p++;
	} while (*p != ')');
fin2:
	output0("]");
fin:
	return;
}
