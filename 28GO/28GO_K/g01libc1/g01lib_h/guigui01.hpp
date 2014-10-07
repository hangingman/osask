extern char g01_bss1a1[1], g01_bss1a2[1], g01_bss1a4[1], g01_bss1a8[1], g01_bss1a16[1];

#define g01_exit_success()			__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x40")
#define g01_exit_failure_int32(i)	__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x43,0x60"::"a"(i))
#define g01_putstr0(s)				__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x53,0x20"::"d"(s):"%esi","%edi","cc")	/* 50(6x)510(60)0_3 */
#define g01_putstr1(l, s)			__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x52,0x61,0x20"::"d"(s),"c"(l):"%esi","%edi","cc")	/* 50(6x)41601(60)0_3 */
#define g01_putc(c)					__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x55,0x16,0x00"::"a"(c):"%esi","%edi","cc")	/* 50{6}1600_3 */
#define g01_putstr0_exit1(s)		__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x35,0x32,0x43,0x10"::"d"(s):"%esi","%edi","cc")	/* 50(6x)510(60)0_3 */
#define g01_setcmdlin(p)			__asm__ __volatile__("movl %%eax,%%edi\n\tcall _g01_execcmd"::"a"(p):"%esi","%edi","cc")
#define g01_getcmdlin_exit1()		__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x86,0xbf")

#define g01_getcmdlin_fopen_s(i)
#define g01_getcmdlin_fopen_o(i)
#define g01_getcmdlin_fopen_m(i, j)

#define g01_getcmdlin_fopen_s_0_4(i)	__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x86,0x60,0x04"::"a"(i):"%esi","%edi","cc")
#define g01_getcmdlin_fopen_s_3_5(i)	__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x86,0x60,0x35"::"a"(i):"%esi","%edi","cc")

static inline int g01_getcmdlin_fopen_m_0_4(int i, int j)
{
	int f;
	__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x86,0x60,0x12,0x04":"=d"(f):"a"(i),"c"(j):"%esi","%edi","cc");
	return f;
}

static inline int g01_getcmdlin_fopen_o_0_4(int i)
{
	int f;
	__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x86,0x60,0x20,0x40":"=d"(f):"a"(i):"%esi","%edi","cc");
	return f;
}

static inline int g01_getcmdlin_fopen_o_3_5(int i)
{
	int f;
	__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x86,0x60,0x23,0x50":"=d"(f):"a"(i):"%esi","%edi","cc");
	return f;
}

static inline int g01_getcmdlin_flag_s(int i)
{
	int f;
	__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x86,0x60,0x00":"=a"(f):"a"(i):"%esi","%edi","cc");
	return f;
}

#define g01_getcmdlin_flag_o(i)				g01_getcmdlin_flag_s(i)
#define g01_getcmdlin_int_s(i)				g01_getcmdlin_flag_s(i)

static inline int g01_getcmdlin_int_o(int i, int d)
{
	int n;
	__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x86,0x60,0x12":"=d"(n):"a"(i),"d"(d):"%ecx","%esi","%edi","cc");
	return n;
}

#define g01_getcmdlin_str_s0(i, n, p)	__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x86, 0x61, 0x36, 0x02"::"c"(i),"a"(n),"d"(p):"%esi","%edi","cc","memory")
#define g01_getcmdlin_str_s0_0(n, p)	__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x86, 0x03, 0x60, 0x20"::"a"(n),"d"(p):"%esi","%edi","cc","memory")
//#define g01_getcmdlin_str_m0(i, j, n, p)	__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x86, 0x61, 0x36, 0x02"::"c"(i),"a"(n),"d"(p):"%esi","%edi","cc")

static inline int g01_getcmdlin_str_o0(int i, int n, void *p)
{
	int f;
	__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x86, 0x61, 0x03, 0x63, 0x20":"=a"(f):"c"(i),"b"(n),"d"(p):"%esi","%edi","cc","memory");
	return f;
}

static inline int g01_getcmdlin_str_m0_1(int j, int n, void *p)
{
	int f;
	__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x86, 0x11, 0x03, 0x63, 0x20":"=a"(f):"c"(j),"b"(n),"d"(p):"%esi","%edi","cc","memory");
	return f;
}

static inline int g01_getcmdlin_argc(int i)
{
	int f;
	__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x86,0xbe,0x60,0x00":"=a"(f):"a"(i):"%esi","%edi","cc");
	return f;
}

#define g01_getcmdlin_put0_s(i)		__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x86,0xbc,0x60"::"a"(i):"%esi","%edi","cc")
#define g01_getcmdlin_put1_s(i)		__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x86,0xbd,0x60"::"a"(i):"%esi","%edi","cc")
#define g01_getcmdlin_put0_m_exit1(i, j)	__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x38,0x6b,0xc6,0x01,0x43,0x10"::"a"(i),"c"(j))
#define g01_getcmdlin_put1_m_exit1(i, j)	__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x38,0x6b,0xd6,0x01,0x43,0x10"::"a"(i),"c"(j))

#if 0
static inline int g01_cmdlin3s(int l) {}
static inline int g01_cmdlin3m(int l, int i) {}
static inline int g01_cmdlin4s(int l) {}
static inline int g01_cmdlin4m(int l, int i) {}
static inline int g01_cmdlin5s(int l) {}
static inline int g01_cmdlin5m(int l, int i) {}
static inline int g01_cmdlin6s1(int l, int n, void *p) {}
static inline int g01_cmdlin6m1(int l, int i, int n, void *p) {}
#endif

#define	jg01_getcmdline0	jg01_getcmdlin1
#define	jg01_getcmdline1	jg01_getcmdlin0

static inline int jg01_getcmdlin0(int n, void *p)
{
	int l;
	__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x7b,0x99,0x80,0x10,0x26,0x01,0x20":"=c"(l):"a"(n),"d"(p):"%esi","%edi","cc","memory");
	return l;
}

#define jg01_getcmdlin1(n, p)		__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x7b,0x99,0x80,0x10,0x36,0x02"::"a"(n),"d"(p):"%esi","%edi","cc")
#define jg01_fclose(s)				__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x7b,0x99,0x80,0x20,0x60"::"a"(s):"%esi","%edi","cc")
#define jg01_fopen(m, s, p)			__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x7b,0x99,0x80,0x36,0x06,0x13,0x20"::"a"(m),"c"(s),"d"(p):"%esi","%edi","cc")
#define jg01_fopen01_4(p)			__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x7b,0x99,0x80,0x31,0x43,0x20"::"d"(p):"%esi","%edi","cc")
#define jg01_fopen19_5(p)			__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x7b,0x99,0x80,0x39,0x95,0x32"::"d"(p):"%esi","%edi","cc")

static inline int jg01_fread1(int s, int n, void *b)
{
	int l;
	__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x7b,0x99,0x80,0x40,0x61,0x26,0x01,0x20":"=c"(l):"a"(n),"c"(s),"d"(b):"%esi","%edi","cc","memory");
	return l;
}

#define jg01_fread0(s, n, b)		__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x7b,0x99,0x80,0x40,0x61,0x36,0x02"::"a"(n),"c"(s),"d"(b):"%esi","%edi","cc","memory")

static inline int jg01_fread1f(int s, int n, void *b)
{
	int l;
	__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x7b,0x99,0x80,0x42,0x61,0x26,0x01,0x20":"=c"(l):"a"(n),"c"(s),"d"(b):"%esi","%edi","cc","memory");
	return l;
}

#define jg01_fread0f(s, n, b)		__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x7b,0x99,0x80,0x42,0x61,0x36,0x02"::"a"(n),"c"(s),"d"(b):"%esi","%edi","cc","memory")

static inline int jg01_fread1_4(int n, void *b)
{
	int l;
	__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x7b,0x99,0x80,0x40,0x42,0x60,0x12":"=c"(l):"a"(n),"d"(b):"%esi","%edi","cc","memory");
	return l;
}

#define jg01_fread0_4(n, b)			__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x7b,0x99,0x80,0x40,0x43,0x60,0x20"::"a"(n),"d"(b):"%esi","%edi","cc","memory")

static inline int jg01_fread1f_4(int n, void *b)
{
	int l;
	__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x7b,0x99,0x80,0x42,0x42,0x60,0x12":"=c"(l):"a"(n),"d"(b):"%esi","%edi","cc","memory");
	return l;
}

#define jg01_fread0f_4(n, b)		__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x7b,0x99,0x80,0x42,0x43,0x60,0x20"::"a"(n),"d"(b):"%esi","%edi","cc","memory")

static inline int jg01_fopen01_4_fread1f_4(void *p, int n, void *b)
{
	int l;
	__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x37,0xb9,0x98,0x03,0x14,0x31,0x7b,0x99,0x80,0x42,0x42,0x60,0x12,0x30":"=c"(l):"a"(n),"c"(p),"d"(b):"%esi","%edi","cc","memory");
	return l;
}

#define jg01_fopen01_4_fread0f_4(p, n, b)	__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x37,0xb9,0x98,0x03,0x14,0x31,0x7b,0x99,0x80,0x42,0x43,0x60,0x23"::"a"(n),"c"(p),"d"(b):"%esi","%edi","cc","memory")

static inline int jg01_fwrite1(int s, int n, void *b)
{
	int l;
	__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x7b,0x99,0x80,0x41,0x61,0x26,0x01,0x20":"=c"(l):"a"(n),"c"(s),"d"(b):"%esi","%edi","cc");
	return l;
}

//#define jg01_fwrite0(s, b)		__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x7b,0x99,0x80,0x41,0x61,0x32"::"c"(s),"d"(b):"%esi","%edi","cc")
#define jg01_fwrite1f(s, n, b)	__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x7b,0x99,0x80,0x43,0x61,0x26,0x02"::"a"(n),"c"(s),"d"(b):"%esi","%edi","cc");
#define jg01_fwrite0f(s, b)		__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x7b,0x99,0x80,0x43,0x61,0x32"::"c"(s),"d"(b):"%esi","%edi","cc")

static inline int jg01_fwrite1_5(int n, void *b)
{
	int l;
	__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x7b,0x99,0x80,0x41,0x52,0x60,0x12":"=c"(l):"a"(n),"d"(b):"%esi","%edi","cc");
	return l;
}

//#define jg01_fwrite0_5(b)		__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x7b,0x99,0x80,0x41,0x53,0x20"::"d"(b):"%esi","%edi","cc")
#define jg01_fwrite1f_5(n, b)	__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x7b,0x99,0x80,0x43,0x52,0x60,0x20"::"a"(n),"d"(b):"%esi","%edi","cc");
#define jg01_fwrite0f_5(b)		__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x7b,0x99,0x80,0x43,0x53,0x20"::"d"(b):"%esi","%edi","cc")
#define jg01_fopen19_5_fwrite1f_5(p, n, b)	__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x37,0xb9,0x98,0x03,0x99,0x53,0x17,0xb9,0x98,0x04,0x35,0x26,0x02,0x30"::"a"(n),"c"(p),"d"(b):"%esi","%edi","cc");
#define jg01_fopen19_5_fwrite0f_5(p, b)	__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x37,0xb9,0x98,0x03,0x99,0x53,0x17,0xb9,0x98,0x04,0x35,0x32,0x30"::"c"(p),"d"(b):"%esi","%edi","cc")

static inline int jg01_tekgetsize(void *p)
{
	int r;
	__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x7b,0x99,0x80,0x87,0x12,0x00":"=a"(r):"d"(p):"%esi","%edi","cc");
	return r;
}

static inline int jg01_tekdecomp(int s, void *p, void *q)
{
	int r;
	__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x7b,0x99,0x80,0x87,0x26,0x12,0x00":"=a"(r):"c"(s),"d"(p),"a"(q):"%esi","%edi","cc","memory");
	return r;
}

static inline int jg01_rjc1_00s(int s, void *p)
{
	int r;
	__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x7b,0x99,0x80,0x87,0x31,0x26,0x12,0x00,0x61,0x00":"=a"(r):"c"(s),"d"(p):"%esi","%edi","cc","memory");
	return r;
}

static inline void *jg01_malloc(int n)
{
	void *p;
	__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x7b,0x99,0x80,0x88,0x06,0x12":"=d"(p):"c"(n):"%esi","%edi","cc");
	return p;
}

//#define jg01_testslot0(s, p)		g01_execcmd0(0x0698b947, 0x30220601, s, p)			/* 4_7b9_98_0_6 0_1_0_6_2_2_3_0 */
//#define jg01_tekgetsize(p, d)		g01_execcmd0(0x0898b947, 0x2206a178, p, d, 0x30)	/* 4_7b9_98_0_8 7_8a_1_0_6_2_2 3_0 */
//#define jg01_tekdecomp(s, p, q, r)	g01_execcmd0(0x0898b947, 0x42062179, s, p, q, r, 0x30)	/* 4_7b9_98_0_8 7_92_1_0_6_4_2 3_0 */
//#define jg01_mfree(b, p)			g01_execcmd0(0x0898b947, 0x23621090, b, p)			/* 4_7b9_98_0_8 9_0_1_0_6_2_2_3 */

#define jg01_sleep1(u, t)	__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x7b,0x99,0x80,0x8a,0x16,0x16,0x00"::"a"(t),"c"(u):"%esi","%edi","cc");

static inline int jg01_inkey2()
{
	int r;
	__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x7b,0x99,0x80,0x8b,0x20":"=a"(r)::"%esi","%edi","cc");
	return r;
}

static inline int jg01_inkey3()
{
	int r;
	__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x7b,0x99,0x80,0x8b,0x30":"=a"(r)::"%esi","%edi","cc");
	return r;
}

#define jg01_consctrl1(x, y)	__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x7b,0x99,0x80,0x8c,0x16,0x06,0x10"::"a"(x),"c"(y):"%esi","%edi","cc");
#define jg01_consctrl2(f, b)	__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x7b,0x99,0x80,0x8c,0x26,0x06,0x10"::"a"(f),"c"(b):"%esi","%edi","cc");
#define jg01_consctrl3()		__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x7b,0x99,0x80,0x8c,0x30":::"%esi","%edi","cc");
#define jg01_consctrl4(x, y)	__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x7b,0x99,0x80,0x8c,0x84,0x60,0x61,0x00"::"a"(x),"c"(y):"%esi","%edi","cc");

static inline int jg01_randomseed()
{
	int r;
	__asm__ __volatile__("call _g01_execcmd0\n\t.byte 0x7b,0x99,0x80,0x8d,0x00":"=a"(r)::"%esi","%edi","cc");
	return r;
}
