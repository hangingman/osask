#include <guigui01.h>

#define CMDLIN_Y	0
#define	CMDLIN_M	1

static unsigned char cmdusg[] = {
	0x86, 0x50,
		0x00, 'y', 0x14, 'y', 'e', 'a', 'r',
		0x10, 'm', 0x18, 0x54, 'm', 'o', 'n', 't', 'h'
};

void putdec2(int i)
{
	g01_putc(' ');
	g01_putc((i > 9) ? i / 10 + '0' : ' ');
	g01_putc(i % 10 + '0');
	return;
}

void G01Main()
{
	static char table0[] = {  1,  4,  3,  6,  1,  4,  6,  2,  5,  0,  3,  5 };
	static char table1[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	int y, m, m1, w, d;
	g01_setcmdlin(cmdusg);
	y  = g01_getcmdlin_int_s(CMDLIN_Y);
	m  = g01_getcmdlin_int_o(CMDLIN_M,  1) - 1;
	m1 = g01_getcmdlin_int_o(CMDLIN_M, 12);
	if (m < 0 || m > 11 || y < 100 || y > 9999)
		g01_getcmdlin_exit1();
	table1[1] += (((y & 3) == 0 && y % 100 != 0) || y % 400 == 0);
	for (; m < m1; m++) {
		g01_putstr0("\n         ");
		g01_putstr1(3, &"JanFebMarAprMayJunJulAugSepOctNovDec"[m * 3]);
		putdec2(y / 100);
		w = y % 100;
		g01_putc(w / 10 + '0');
		g01_putc(w % 10 + '0');
		g01_putstr0("\n\nSun Mon Tue Wed Thu Fri Sat\n");
		w = y - (m <= 1);
		w = (w + (w >> 2) - w / 100 + w / 400 + table0[m]) % 7;
		for (d = 0; d < (w << 2); d++)
			g01_putc(' ');
		for (d = 1; d <= table1[m]; d++) {
			putdec2(d);
			w = (w + 1) % 7;
			g01_putc((w != 0) ? ' ' : '\n');
		}
		if (w != 0)
			g01_putc('\n');
	}
	g01_putc('\n');
	return;
}
