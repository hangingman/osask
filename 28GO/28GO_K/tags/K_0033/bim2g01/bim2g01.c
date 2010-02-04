#include <guigui01.h>

typedef unsigned char UCHAR;
void putqlink(UCHAR **pp, int *flags, UCHAR *q, int qf, int sec, int unit, int *table);
void putnum_u(UCHAR **pp, int *flags, int d);
void putnum_flush(UCHAR **p, int *flags);
void putimg(UCHAR **pp, int *flags, UCHAR *q, int qf, int size, UCHAR *r);
//int getnum(const UCHAR *p);
int getnum0(const UCHAR **pp);
int get32(const UCHAR *p);
void put32(UCHAR *p, int i);

#define MAXSIZ	4 * 1024 * 1024

#define	CMDLIN_RJC		0
#define	CMDLIN_IN		1
#define	CMDLIN_OUT		2
#define	CMDLIN_RLM		3

static UCHAR cmdusg[] = {
	0x86, 0x51,
		0x12, 'r', 'j', 'c', 0x11, '0',
		0x88, 0x8c, 
		0x12, 'r', 'l', 'm', 0x03, 0x01, '-', 0x02,
	0x40
};

void G01Main()
{
	UCHAR *fbuf, *p, *p0, *q;
	int /* heap_siz, mmarea, */ fsiz, csize, dsize, dofs, stksiz, /* wrksiz, */ entry, bsssiz;
	int /* heap_adr, */ i, flags = 0, qf, rjc_flag;
	int *reloc_table_all;

	g01_setcmdlin(cmdusg);
	fbuf = jg01_malloc(MAXSIZ);
	reloc_table_all = jg01_malloc(4 * 64 * 1024);

//	heap_siz = getnum(argv[3]);
//	mmarea = 0;
	rjc_flag = g01_getcmdlin_int_o(CMDLIN_RJC, 1);
//	if (argc >= 6)
//		mmarea = getnum(argv[5]);

	/* reloc-memo */
	for (i = 0; i < 4; i++)
		reloc_table_all[i] = -1;
	if (g01_getcmdlin_fopen_o_0_4(CMDLIN_RLM) != 0) {
		jg01_fread0f_4(MAXSIZ, fbuf);
		p = fbuf;
		qf = 0;
		for (i = 0; i < 4; i++) {
			while (*p != 0 && *p != '{')
				p++;
			if (*p != '{') {
				g01_putstr0_exit1("reloc-memo error");
			}
			p++;
			if (qf >= 64 * 1024 - 4) {
				g01_putstr0_exit1("reloc-buf over error");
			}
			for (;; qf++) {
				while (0 < *p && *p <= ' ')
					p++;
				if (*p == '}')
					break;
				if (*p < '0' || '9' < *p) {
 					g01_putstr0_exit1("reloc-memo error");
				}
				reloc_table_all[qf] = getnum0((const UCHAR **) &p);
				while (0 < *p && *p <= ' ')
					p++;
				while (*p == '+') {
					p++;
					while (0 < *p && *p <= ' ')
						p++;
					reloc_table_all[qf] += getnum0((const UCHAR **) &p);
					while (0 < *p && *p <= ' ')
						p++;
				}
				if (*p == ',')
					p++;
			}
			reloc_table_all[qf++] = -1;
		}
	}

	/* ファイル読み込み */
	g01_getcmdlin_fopen_s_0_4(CMDLIN_IN);
	fsiz = jg01_fread1f_4(MAXSIZ, fbuf);

	/* ヘッダ確認 */
	if (get32(&fbuf[4]) != 0x20) {	/* ファイル中の.textスタートアドレス */
err_form:
		g01_putstr0_exit1("bim file format error");
	}
	if (get32(&fbuf[8]) != 0x0)	/* メモリロード時の.textスタートアドレス */
		goto err_form;
	csize  = get32(&fbuf[ 0]);
	dsize  = get32(&fbuf[12]);	/* .dataセクションサイズ */
	dofs   = get32(&fbuf[16]);	/* ファイルのどこに.dataセクションがあるか */
	stksiz = get32(&fbuf[20]);	/* スタックサイズ */
	entry  = get32(&fbuf[24]);	/* エントリポイント */
	bsssiz = get32(&fbuf[28]);	/* bssサイズ */
	if ((entry | bsssiz) != 0)
		goto err_form;
	if (fbuf[(0x20-1)+csize] == 0xc3)
		csize--;
	for (i = 0; i < 32; i++) {
		if (dsize > 0 && fbuf[dofs+dsize-1] == 0x00)
			dsize--;
	}

	/* g01生成 */
	i = (fsiz + 255) & -256;
	p0 = p = fbuf + i;
	p[0] = 'G';
	p[1] = 0x01;
	p[2] = 0x10;
	p += 3;
	flags |= 1;

	if (rjc_flag != 0) {
		i = jg01_rjc1_00s(csize, &fbuf[0x20]);
	//	if (i == 0)
	//		rjc_flag = 0;
	}
	if (rjc_flag == 0) {
		putnum_u(&p, &flags, 3);
		putnum_u(&p, &flags, 4);
		p0[2] += 0x10;
	}

	/* .textセクション */
	putnum_u(&p, &flags, 0x0d);
	q = p;
	qf = flags;
	i = 0;
	putnum_u(&p, &flags, 1);
	putimg(&p, &flags, q, qf, csize, &fbuf[0x20]);
	putqlink(&p, &flags, q, qf, 0, 1, &reloc_table_all[0]);
	while (reloc_table_all[i] != -1)
		i++;
	i++;
	putqlink(&p, &flags, q, qf, 1, 1, &reloc_table_all[i]);

	/* .dataセクション */
	if (dsize > 0) {
		p0[2] += 0x10;
		putnum_u(&p, &flags, 0x0d);
		q = p;
		qf = flags;
		putnum_u(&p, &flags, 1);
		putimg(&p, &flags, q, qf, dsize, &fbuf[dofs]);
		while (reloc_table_all[i] != -1)
			i++;
		i++;
		putqlink(&p, &flags, q, qf, 0, 4, &reloc_table_all[i]);
		while (reloc_table_all[i] != -1)
			i++;
		i++;
		putqlink(&p, &flags, q, qf, 1, 4, &reloc_table_all[i]);
	}
	putnum_flush(&p, &flags);

	/* ファイル書き込み */
	g01_getcmdlin_fopen_s_3_5(CMDLIN_OUT);
	jg01_fwrite1f_5(p - p0, p0);
	return;
}

void putnum_u(UCHAR **pp, int *flags, int d)
{
	UCHAR buf[32], *p = *pp;
	int i = 0, j;
	if ((*flags & 1) != 0) {
		buf[0] = (*--p) >> 4;
		i++;
		*flags &= ~1;
	}
	if (d < 0) {
		g01_putstr0_exit1("error");
	}
	if (d <= 0x6)
		buf[i++] = d;
	else if (d <= 0x3f) {
		buf[i + 0] = 0x8 | d >> 4;
		buf[i + 1] = d & 0x0f;
		i += 2;
	} else if (d <= 0x1ff) {
		buf[i + 0] = 0xc | d >> 8;
		buf[i + 1] = (d >> 4) & 0x0f;
		buf[i + 2] = d & 0x0f;
		i += 3;
	} else if (d <= 0xfff) {
		buf[i + 0] = 0xe;
		buf[i + 1] = (d >> 8) & 0x0f;
		buf[i + 2] = (d >> 4) & 0x0f;
		buf[i + 3] = d & 0x0f;
		i += 4;
	} else if (d <= 0x7fff) {
		buf[i + 0] = 0xf;
		buf[i + 1] = (d >> 12) & 0x0f;
		buf[i + 2] = (d >>  8) & 0x0f;
		buf[i + 3] = (d >>  4) & 0x0f;
		buf[i + 4] = d & 0x0f;
		i += 5;
	} else if (d <= 0x3ffff) {
		buf[i + 0] = 0xf;
		buf[i + 1] = ((d >> 16) & 0x0f) | 0x8;
		buf[i + 2] =  (d >> 12) & 0x0f;
		buf[i + 3] =  (d >>  8) & 0x0f;
		buf[i + 4] =  (d >>  4) & 0x0f;
		buf[i + 5] = d & 0x0f;
		i += 6;
	} else if (d <= 0x1fffff) {
		buf[i + 0] = 0xf;
		buf[i + 1] = ((d >> 20) & 0x0f) | 0xc;
		buf[i + 2] =  (d >> 16) & 0x0f;
		buf[i + 3] =  (d >> 12) & 0x0f;
		buf[i + 4] =  (d >>  8) & 0x0f;
		buf[i + 5] =  (d >>  4) & 0x0f;
		buf[i + 6] = d & 0x0f;
		i += 7;
	} else if (d <= 0xffffff) {
		buf[i + 0] = 0xf;
		buf[i + 1] = 0xe;
		buf[i + 2] = (d >> 20) & 0x0f;
		buf[i + 3] = (d >> 16) & 0x0f;
		buf[i + 4] = (d >> 12) & 0x0f;
		buf[i + 5] = (d >>  8) & 0x0f;
		buf[i + 6] = (d >>  4) & 0x0f;
		buf[i + 7] = d & 0x0f;
		i += 8;
	} else {
		buf[i + 0] = 0x7;
		buf[i + 1] = 0x4;
		buf[i + 2] = (d >> 28) & 0x0f;
		buf[i + 3] = (d >> 24) & 0x0f;
		buf[i + 4] = (d >> 20) & 0x0f;
		buf[i + 5] = (d >> 16) & 0x0f;
		buf[i + 6] = (d >> 12) & 0x0f;
		buf[i + 7] = (d >>  8) & 0x0f;
		buf[i + 8] = (d >>  4) & 0x0f;
		buf[i + 9] = d & 0x0f;
		i += 10;
	}
	buf[i] = 0;
	for (j = 0; j < i; j += 2)
		*p++ = buf[j + 0] << 4 | buf[j + 1];
	*pp = p;
	*flags |= i & 1;
	return;
}

void putnum_flush(UCHAR **p, int *flags)
{
	if ((*flags & 1) != 0)
		*flags &= ~1;
	return;
}

void putimg(UCHAR **pp, int *flags, UCHAR *q, int qf, int size, UCHAR *r)
{
	UCHAR *p = *pp, *p0 = p;
	int i = *flags;

	for (;;) {
		putnum_u(&p, flags, 0x14);
		putnum_u(&p, flags, size * 2);
		if ((*flags & 1) == 0)
			break;
		p = p0;
		*flags = i;
		putnum_u(&p, flags, 0);
		if (qf == 0)
			*q += 0x10;
		else
			q[-1]++;

	}
	for (i = 0; i < size; i++)
		p[i] = r[i];
	*pp = p + i;
	return;
}

void putqlink(UCHAR **pp, int *flags, UCHAR *q, int qf, int sec, int unit, int *table)
{
	int i, j = 0;	

	for (i = 0; table[i] >= 0; i++);
	if (i > 0) {
		if (qf == 0)
			*q += 0x10;
		else
			q[-1]++;
		putnum_u(pp, flags, 0x19);
		putnum_u(pp, flags, i + 1);
		putnum_u(pp, flags, sec);
		for (i = 0; table[i] >= 0; i++) {
			if (((table[i] - j) & (unit - 1)) != 0) {
				g01_putstr0_exit1("putqlink: unit error");
			}
			putnum_u(pp, flags, (table[i] - j) / unit);
			j = table[i] + 4;
		}
	}
	return;
}

int getnum0(const UCHAR **pp)
{
	int i = 0, base = 10, sign = 1;
	const UCHAR *p = *pp;
	UCHAR c;
	if (*p == '-') {
		p++;
		sign = -1;
	}
	if (*p == '0') {
		p++;
		base = 8;
		c = *p;
		if (c >= 'a')
			c -= 'a' - 'A';
		if (c == 'X') {
			p++;
			base = 16;
		}
		if (c == 'O') {
			p++;
			base = 8;
		}
		if (c == 'B') {
			p++;
			base = 2;
		}
	}
	for (;;) {
		c = *p++;
		if ('0' <= c && c <= '9')
			c -= '0'; 
		else if ('A' <= c && c <= 'F')
			c -= 'A' - 10;
		else if ('a' <= c && c <= 'f')
			c -= 'a' - 10;
		else
			break;
		if (c >= base)
			break;
		i = i * base + c;
	}
	if (c >= 'a')
		c -= 'a' - 'A';
	if (c == 'K')
		i <<= 10;
	if (c == 'M')
		i <<= 20;
	if (c == 'G')
		i <<= 30;
	*pp = p;
	return i * sign;
}

int get32(const UCHAR *p)
{
	return p[0] | p[1] << 8 | p[2] << 16 | p[3] << 24;
}

void put32(UCHAR *p, int i)
{
	p[0] =  i        & 0xff;
	p[1] = (i >>  8) & 0xff;
	p[2] = (i >> 16) & 0xff;
	p[3] = (i >> 24) & 0xff;
	return;
}

/*

memo

[ .bimファイルの構造 ]

+ 0 : .textサイズ
+ 4 : ファイル中の.textスタートアドレス（0x24）
+ 8 : メモリロード時の.textスタートアドレス（0x24）
+12 : .dataサイズ
+16 : ファイル中の.dataスタートアドレス
+20 : メモリロード時の.dataスタートアドレス
+24 : エントリポイント
+28 : bss領域のバイト数
+32 : コード

*/
