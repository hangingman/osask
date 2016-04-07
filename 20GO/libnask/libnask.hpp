/* "nask.c" */
/* copyright(C) 2003 H.Kawai(川合秀実) */
/*   [OSASK 3978], [OSASK 3979]で光成さんの指摘を大いに参考にしました */
/*	小柳さんのstring0に関する指摘も参考にしました */

#include <cstdlib>	/* malloc/free */
#include <cstdint>
#include "ll.hpp"

#if (DEBUG)
	#include "go.hpp"
	#include "../include/stdio.h"
#endif

static constexpr unsigned int nask_LABELBUFSIZ = 256 * 1024;
static constexpr unsigned int OPCLENMAX    = 12; /* 足りなくなったら12にしてください */
static constexpr unsigned int MAX_SECTIONS = 8;
static constexpr unsigned int E_LABEL0     = 16;
static constexpr int nask_L_LABEL0         = 16384; /* externラベルは16300個程度使える */
static constexpr unsigned int MAX_LISTLEN  = 32;

// リマークNL(f8) : ラインスタート, 4バイトのレングス, 4バイトのポインタ バイト列を並べる
// リマークADR(e0) : アドレス出力
// リマークBY(e1) : 1バイト出力
// リマークWD(e2) : 2バイト出力
// リマーク3B(e3) : 3バイト出力
// リマークDW(e4) : 4バイト出力
// リマーク[BY](e5) : 1バイト出力[]つき
// リマーク[WD](e6) : 2バイト出力[]つき
// リマーク[3B](e7) : 3バイト出力[]つき
// リマーク[DW](e8) : 4バイト出力[]つき

constexpr UINT REM_ADDR      = 0xe0;
constexpr UINT REM_BYTE      = 0xe1; /* 廃止 */
constexpr UINT REM_WORD      = 0xe2; /* 廃止 */
constexpr UINT REM_DWRD      = 0xe4; /* 廃止 */
constexpr UINT REM_ADDR_ERR  = 0xe5;
constexpr UINT REM_RANGE_ERR = 0xe8;
constexpr UINT REM_3B	     = 0xf1;
constexpr UINT REM_4B	     = 0xf2;
constexpr UINT REM_8B	     = 0xf6;
constexpr UINT SHORT_DB0     = 0x30;
constexpr UINT SHORT_DB1     = 0x31;
constexpr UINT SHORT_DB2     = 0x32;
constexpr UINT SHORT_DB4     = 0x34;
constexpr UINT EXPR_MAXSIZ   = 2048;
constexpr UINT EXPR_MAXLEN   = 1000;

static void setdec(unsigned int i, int n, UCHAR *s);
static void sethex0(unsigned int i, int n, UCHAR *s);

static void *cmalloc(int size)
{
	std::unique_ptr<char[]> p (new char[size]);
	for (int i = 0; i < size; i++)
		p[i] = 0;

	return p.get();
}

struct INST_TABLE {
	std::array<UCHAR, OPCLENMAX> opecode;
	UINT support;
	std::array<UCHAR, 8> param;
};

struct STR_SECTION {
	UINT dollar_label0; /* $ */
	UINT dollar_label1; /* ..$ */
	UINT dollar_label2; /* $$ */
	int total_len;
	UCHAR *p0, *p; /* ソート用のポインタ */
	UCHAR name[17], name_len;
	signed char align0, align1; /* -1は未設定 */
};

struct STR_OUTPUT_SECTION {
	UCHAR *p, *d0, *reloc_p;
	int addr, relocs;
	UCHAR align, flags;
};

extern int nask_errors;

constexpr UCHAR SUP_8086        = 0x000000ff; /* bit 0 */
constexpr UCHAR SUP_80186	= 0x000000fe; /* bit 1 */
constexpr UCHAR SUP_80286	= 0x000000fc; /* bit 2 */
constexpr UCHAR SUP_80286P	= 0x000000a8; /* bit 3 */
constexpr UCHAR SUP_i386	= 0x000000f0; /* bit 4 */
constexpr UCHAR SUP_i386P	= 0x000000a0; /* bit 5 */
constexpr UCHAR SUP_i486	= 0x000000c0; /* bit 6 */
constexpr UCHAR SUP_i486P	= 0x00000080; /* bit 7 */
constexpr UCHAR SUP_Pentium     = 0;
constexpr UCHAR SUP_Pentium2    = 0;
constexpr UCHAR SUP_Pentium3    = 0;
constexpr UCHAR SUP_Pentium4    = 0;

constexpr UCHAR PREFIX          = 0x01;       /* param[1]がプリフィックス番号 */
constexpr UCHAR NO_PARAM	= 0x02;       /* param[1]の下位4bitがオペコードバイト数 */
constexpr UCHAR OPE_MR		= 0x03;       /* mem/reg,reg型 */ /* [1]:datawidth, [2]:len */
constexpr UCHAR OPE_RM		= 0x04;       /* reg,mem/reg型 */
constexpr UCHAR OPE_M		= 0x05;       /* mem/reg型 */
constexpr UCHAR OPE_SHIFT       = 0x06;       /* ROL, ROR, RCL, RCR, SHL, SAL, SHR, SAR */
constexpr UCHAR OPE_RET         = 0x07;       /* RET, RETF, RETN */
constexpr UCHAR OPE_AAMD        = 0x08;	      /* AAM, AAD */
constexpr UCHAR OPE_INT	        = 0x09;       /* INT */
constexpr UCHAR OPE_PUSH	= 0x0a;       /* INC, DEC, PUSH, POP */
constexpr UCHAR OPE_MOV		= 0x0b;       /* MOV */
constexpr UCHAR OPE_ADD		= 0x0c;       /* ADD, OR, ADC, SBB, AND, SUB, XOR, CMP */
constexpr UCHAR OPE_XCHG	= 0x0d;       /* XCHG */
constexpr UCHAR OPE_INOUT	= 0x0e;       /* IN, OUT */
constexpr UCHAR OPE_IMUL	= 0x0f;       /* IMUL */
constexpr UCHAR OPE_TEST	= 0x10;       /* TEST */
constexpr UCHAR OPE_MOVZX	= 0x11;       /* MOVSX, MOVZX */
constexpr UCHAR OPE_SHLD	= 0x12;       /* SHLD, SHRD */
constexpr UCHAR OPE_LOOP	= 0x13;       /* LOOPcc, JCXZ */
constexpr UCHAR OPE_JCC		= 0x14;       /* Jcc */
constexpr UCHAR OPE_BT		= 0x15;       /* BT, BTC, BTR, BTS */
constexpr UCHAR OPE_ENTER	= 0x16;       /* ENTER */
constexpr UCHAR OPE_ALIGN	= 0x17;       /* ALIGN, ALIGNB */
constexpr UCHAR OPE_FPU		= 0x30;
constexpr UCHAR OPE_FPUP	= 0x31;
constexpr UCHAR OPE_FSTSW	= 0x32;
constexpr UCHAR OPE_FXCH	= 0x33;
constexpr UCHAR OPE_ORG		= 0x3d;       /* ORG */
constexpr UCHAR OPE_RESB	= 0x3e;       /* RESB, RESW, RESD, RESQ, REST */
constexpr UCHAR OPE_EQU		= 0x3f;

constexpr UCHAR OPE_JMP	        = 0x40;       /* CALL, JMP */
constexpr UCHAR OPE_GLOBAL	= 0x44;	      /* GLOBAL, EXTERN */
constexpr UCHAR OPE_TIMES	= 0x47;	      /* TIMES */
constexpr UCHAR OPE_DB		= 0x48;	      /* DB, DW, DD, DQ, DT */
constexpr UCHAR OPE_END	        = 0x49;

/* NO_PARAM用 */
constexpr UCHAR OPE16           = 0x10;
constexpr UCHAR OPE32           = 0x20;
constexpr UCHAR DEF_DS          = 0x40;
/* param[1]のbit4 : ope32 */
/* param[1]のbit5 : ope16 */
/* param[1]のbit6 : デフォルトプリフィックスDS */
/* param[1]のbit7 : デフォルトプリフィックスSS */

static constexpr std::array<UCHAR, 65> table_prms = {
	0, 0, 0 /* NO_PARAM */, 2 /* OPE_MR */, 2 /* OPE_RM */,
	1 /* OPE_M */, 2 /* OPE_SHIFT */, 9 /* OPE_RET */, 9 /* OPE_AAMD */,
	1 /* OPE_INT */, 1 /* OPE_PUSH */, 2 /* OPE_MOV */, 2 /* OPE_ADD */,
	2 /* OPE_XCHG */, 2 /* OPE_INOUT */, 9 /* OPE_IMUL */, 2 /* OPE_TEST */,
	2 /* OPE_MOVZX */, 3 /* OPE_SHLD */, 9 /* OPE_LOOP */, 1 /* OPE_JCC */,
	2 /* OPE_BT */, 2 /* OPE_ENTER */, 1 /* OPE_ALIGN */, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,	0, 0, 0, 0, 0, 0, 0, 0,
	9 /* OPE_FPU */, 9 /* OPE_FPUP */, 1 /* OPE_FSTSW */, 9 /* OPE_FXCH */,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 1 /* OPE_ORG */, 1 /* OPE_RESB */,
	1 /* OPE_EQU */
};

struct STR_DECODE {
	UCHAR *label, *param;
	struct INST_TABLE *instr;
	std::array<UINT, 3> prm_t;
	std::array<UCHAR*, 3> prm_p;
	int prefix;
	std::array<int, 3> gparam;
	std::array<int, 3> gvalue;
	int gp_mem, gp_reg;
	struct STR_SECTION* sectable;
	UCHAR error, flag /* , dollar */;
};
/* flagのbit0はmem/regがregかどうかをあらわす */

struct STR_TERM {
	int term_type;
	int value;
};

struct STR_OFSEXPR {
	int scale[2], disp;
	unsigned char reg[2], dispflag; /* 0xffのとき、unknown, regが127以下なら、スケール無し */
	unsigned char err;
};

struct STR_DEC_EXPR_STATUS {
	UINT support;
	int glabel_len;
	UCHAR *glabel;
	signed char datawidth; /* -1(default), 1(byte), 2(word), 4(dword) */
	signed char seg_override; /* -1(default), 0〜5 */
	signed char range; /* -1(default), 0(short), 1(near), 2(far) */
	char nosplit; /* 0(default), 1(nosplit) */
	char use_dollar;  /* 0(no use), 1(use) */
	char option;
	char to_flag;
	UINT dollar_label0; /* $ */
	UINT dollar_label1; /* ..$ */
	UINT dollar_label2; /* $$ */
};

struct STR_STATUS {
	UCHAR *src1; /* ファイル終端ポインタ */
	UINT support, file_len;
	char bits, optimize, format, option;
	struct STR_DEC_EXPR_STATUS expr_status;
	struct STR_OFSEXPR ofsexpr;
	struct STR_TERM *expression, *mem_expr;
	UCHAR *file_p;
};

struct STR_IFDEFBUF {
	/* 条件付き定義用バッファ構造体 */
	UCHAR *bp, *bp0, *bp1; /* range-error用バッファ */

	/* bit0-4:バイト数, bit7:exprフラグ, bit5-6:レンジチェック */
	std::unique_ptr<UCHAR []> vb{ new UCHAR[12]() };
	std::unique_ptr<int []> dat{ new int[12]() };
	std::unique_ptr<UCHAR* []> expr{ new UCHAR*[12] };

	// Proper way to create unique_ptr that holds an allocated array
	// http://stackoverflow.com/a/21377382/2565527
};

static UCHAR *labelbuf0, *labelbuf;
static UCHAR *locallabelbuf0 /* 256bytes */, *locallabelbuf;
static int nextlabelid;
static const char *ERRMSG[] = {
	"      >> [ERROR #001] syntax error.\n",
	"      >> [ERROR #002] parameter error.\n",
	"      >> [ERROR #003] data size error.\n",
	"      >> [ERROR #004] data type error.\n",
	"      >> [ERROR #005] addressing error.\n",
	"      >> [ERROR #006] TIMES error.\n",
	"      >> [ERROR #007] label definition error.\n",
	"      >> [ERROR #008] data range error.\n",
	"      >> [ERROR #009] expression error.\n",	/* 不定値エラー(delta != 0) */
	"      >> [ERROR #010] expression error.\n",
	"      >> [ERROR #011] expression error.\n",
	"      >> [ERROR #012] expression error.\n" /* 未定義ラベル参照 */
};

namespace libnask {

     static constexpr UCHAR header[140] = {
	/* file header */
	  0x4c, 0x01, /* signature */
	  0x03, 0x00, /* sections == 3 */
	  0, 0, 0, 0, /* time & date */
	  0, 0, 0, 0, /* +0x08: symboltable */
	  0, 0, 0, 0, /* +0x0c: sizeof (symboltable) / 18 */
	  0x00, 0x00, /* no optional header */
	  0x00, 0x00, /* flags */

	/* section header (.text) */
	  '.', 't', 'e', 'x', 't', 0, 0, 0, /* name */
	  0, 0, 0, 0, /* paddr (section_text - section_text) */
	  0, 0, 0, 0, /* vaddr == 0 */
	  0, 0, 0, 0, /* +0x24: sizeof (section_text) */
	  0, 0, 0, 0, /* +0x28: section_text */
	  0, 0, 0, 0, /* +0x2c: reloctab_text */
	  0, 0, 0, 0, /* line number == 0 */
	  0, 0, /* +0x34: sizeof (reloctab_text) / 10 */
	  0, 0, /* sizeof (line_number) == 0 */
	  0x20, 0x00, 0x10, 0x60, /* +0x38: flags, default_align = 1 */

	/* section header (.data) */
	  '.', 'd', 'a', 't', 'a', 0, 0, 0, /* name */
	  0, 0, 0, 0, /* paddr (section_data - section_text) */
	  0, 0, 0, 0, /* vaddr == 0 */
	  0, 0, 0, 0, /* +0x4c: sizeof (section_data) */
	  0, 0, 0, 0, /* +0x50: section_data */
	  0, 0, 0, 0, /* +0x54: reloctab_data */
	  0, 0, 0, 0, /* line number == 0 */
	  0, 0, /* +0x5c: sizeof (reloctab_data) / 10 */
	  0, 0, /* sizeof (line_number) == 0 */
	  0x40, 0x00, 0x10, 0xc0, /* +0x60: flags, default_align = 1 */

	/* section header (.bss) */
	  '.', 'b', 's', 's', 0, 0, 0, 0, /* name */
	  0, 0, 0, 0, /* paddr (section_bss - section_text) */
	  0, 0, 0, 0, /* vaddr == 0 */
	  0, 0, 0, 0, /* +0x74: sizeof (section_bss) */
	  0, 0, 0, 0, /* section_bss == 0 */
	  0, 0, 0, 0, /* reloctab_bss == 0 */
	  0, 0, 0, 0, /* line number == 0 */
	  0, 0, /* sizeof (reloctab_data) / 10 == 0 */
	  0, 0, /* sizeof (line_number) == 0 */
	  0x80, 0x00, 0x10, 0xc0 /* +0x88: flags, default_align = 1 */
     };

     static constexpr UCHAR common_symbols0[18 * 1 - 1] = {
	  '.', 'f', 'i', 'l', 'e', 0, 0, 0, /* name */
	  0, 0, 0, 0, /* value */
	  0xfe, 0xff, /* debugging symbol */
	  0, 0, /* T_NULL */
	  103 /* , 0 */ /* file name, numaux = 0 */
     };

     static constexpr UCHAR common_symbols1[18 * 6] = {
	  '.', 't', 'e', 'x', 't', 0, 0, 0, /* name */
	  0, 0, 0, 0, /* value */
	  0x01, 0x00, /* section 1 */
	  0, 0, /* T_NULL */
	  3, 1, /* private symbol, numaux = 1 */
	  0, 0, 0, 0, /* +0x12: sizeof (section_text) */
	  0, 0, /* +0x16: sizeof (reloctab_text) / 10 */
	  0, 0, /* sizeof (line_number) == 0 */
	  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	  '.', 'd', 'a', 't', 'a', 0, 0, 0, /* name */
	  0, 0, 0, 0, /* value */
	  0x02, 0x00, /* section 2 */
	  0, 0, /* T_NULL */
	  3, 1, /* private symbol, numaux = 1 */
	  0, 0, 0, 0, /* +0x36: sizeof (section_text) */
	  0, 0, /* +0x3a: sizeof (reloctab_text) / 10 */
	  0, 0, /* sizeof (line_number) == 0 */
	  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	  '.', 'b', 's', 's', 0, 0, 0, 0, /* name */
	  0, 0, 0, 0, /* value */
	  0x03, 0x00, /* section 3 */
	  0, 0, /* T_NULL */
	  3, 1, /* private symbol, numaux = 1 */
	  0, 0, 0, 0, /* +0x5a: sizeof (section_bss) */
	  0, 0, /* sizeof (reloctab_text) / 10 == 0 */
	  0, 0, /* sizeof (line_number) == 0 */
	  0, 0, 0, 0, 0, 0, 0, 0, 0, 0
     };
}

UCHAR check_alignments(std::unique_ptr<STR_OUTPUT_SECTION[]>& sectable,
		       UCHAR *src1, UCHAR *srcp, UCHAR *file_p, int file_len, int g_symbols, int e_symbols, UCHAR file_aux);

UCHAR* skipspace(UCHAR *s, UCHAR *t);
UCHAR* putimm(int i, UCHAR *p);
UCHAR* decoder(struct STR_STATUS *status, UCHAR *src, struct STR_DECODE *decode);
UCHAR* putprefix(UCHAR *dest0, UCHAR *dest1, int prefix, int bits, int opt);
void put4b(UINT i, UCHAR *p);
UINT get4b(UCHAR *p);
struct STR_TERM *decode_expr(UCHAR **ps, UCHAR *s1, struct STR_TERM *expr, int *priority, struct STR_DEC_EXPR_STATUS *status);
void calc_ofsexpr(struct STR_OFSEXPR *ofsexpr, struct STR_TERM **pexpr, char nosplit);
int getparam(UCHAR **ps, UCHAR *s1, int *p, struct STR_TERM *expression, struct STR_TERM *mem_expr,
	struct STR_OFSEXPR *ofsexpr, struct STR_DEC_EXPR_STATUS *status);
int testmem(struct STR_OFSEXPR *ofsexpr, int gparam, struct STR_STATUS *status, int *prefix);
void putmodrm(std::unique_ptr<STR_IFDEFBUF>& ifdef, int tmret, int gparam,
	struct STR_STATUS *status, /* struct STR_OFSEXPR *ofsexpr, */ int ttt);
int microcode90(std::unique_ptr<STR_IFDEFBUF>& ifdef, struct STR_TERM *expr, int *def, signed char dsiz);
int microcode91(std::unique_ptr<STR_IFDEFBUF>& ifdef, struct STR_TERM *expr, int *def, signed char dsiz);
int microcode94(std::unique_ptr<STR_IFDEFBUF>& ifdef, struct STR_TERM *expr, int *def);
int defnumexpr(std::unique_ptr<STR_IFDEFBUF>& ifdef, struct STR_TERM *expr, UCHAR vb, UCHAR def);
int getparam0(UCHAR *s, struct STR_STATUS *status);
int getconst(UCHAR **ps, struct STR_STATUS *status, int *p);
int testmem0(struct STR_STATUS *status, int gparam, int *prefix);
int label2id(int len, UCHAR *label, int extflag);
UCHAR *id2label(int id);
UCHAR *put_expr(UCHAR *s, struct STR_TERM **pexpr);
UCHAR *flush_bp(int len, UCHAR *buf, UCHAR *dest0, UCHAR *dest1, std::unique_ptr<STR_IFDEFBUF>& ifdef);
struct STR_TERM *rel_expr(struct STR_TERM *expr, struct STR_DEC_EXPR_STATUS *status);
UCHAR *LL_skip_expr(UCHAR *p);
UCHAR *LL_skipcode(UCHAR *p);
UCHAR *output(UCHAR *src0, UCHAR *src1, UCHAR *dest0, UCHAR *dest1, UCHAR *list0, UCHAR *list1, int nask_errors);
UCHAR *output_error(UCHAR *list0, UCHAR *list1, UCHAR *dest);

#define	defnumconst(ifdef, imm, virbyte, typecode) ifdef->vb[(virbyte) & 0x07] = typecode; ifdef->dat[(virbyte) & 0x07] = imm
