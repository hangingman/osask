#include <guigui01.h>
#include <stdio.h>

void G01Main()
{
	int x, y, u, v, r, k, s;
	char b[32];
	jg01_consctrl4(80, 25);
	jg01_consctrl2(7, 0);
	jg01_consctrl3();
	x = 39;
	y = 11;
	u = 1;
	v = -1;
	s = 0;
	r = 37;
	jg01_consctrl2(0, 7);
	for (k = 0; k <= 79; k++)
		g01_putc(' ');
	for (k = 1; k <= 22; k++) {
		jg01_consctrl1(0, k);
		g01_putc(' ');
		jg01_consctrl1(79, k);
		g01_putc(' ');
	}

	for (;;) {
		jg01_consctrl1(0, 0);
		jg01_consctrl2(1, 7);
		sprintf(b, "score = %d", s);
		g01_putstr0(b);
		jg01_consctrl1(x, y);
		jg01_consctrl2(6, 0);
		g01_putc('O');
		jg01_consctrl1(r, 23);
		jg01_consctrl2(7, 0);
		g01_putstr0("#####");
		if (y >= 23)
			break;
		jg01_sleep1(32 - 4, 1); /* 2^(-4) = 1/16 [sec] */
		k = jg01_inkey2();
		while (jg01_inkey2() != 0);
		jg01_consctrl1(x, y);
		g01_putc(' ');
		if (x <= 1)
			u = +1;
		if (x >= 78)
			u = -1;
		if (y <= 1)
			v = +1;
		if (y == 22) {
			if (r - 1 <= x && x <= r + 5) {
				v = -1;
				s++;
			}
		}
		x += u;
		y += v;
		if (k > 0) {
			jg01_consctrl1(r, 23);
			g01_putstr0("     ");
		}
		if (k == '6' && r < 73)
			r += 2;
		if (k == '4' && r >  1)
			r -= 2;
	}
}
