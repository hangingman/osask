#include <guigui01.h>
#include <stdio.h>
#include <stdlib.h>

void mprintf(const char *f, ...);	/* mini-printf */

void G01Main()
{
    int k, x, y, u = 0, v = 0, p = 0, s = 0;
	char f[80][25];
	srand(jg01_randomseed());
	jg01_consctrl4(80, 25);
	jg01_consctrl2(7, 0);
	jg01_consctrl3();
	for (y = 0; y <= 21; y++) {
		jg01_consctrl1(0, y);
		for (x = 0; x <= 79; x++) {
			if (1 <= x && x <= 78 && 1 <= y && y <= 20) {
				f[x][y] = 0;
				g01_putc(' ');
			} else {
				f[x][y] = 1;
				g01_putc('#');
			}
		}
	}
	jg01_consctrl2(6, 0);
	for (k = 0; k < 20; k++) {
		x = (rand() % 78) + 1;
		y = (rand() % 20) + 1;
		jg01_consctrl1(x, y);
		g01_putc('*');
		f[x][y] = 2;
	}

	x = 38;
	y = 10;
	for (;;) {
		jg01_consctrl1(10, 0);
		jg01_consctrl2(2, 0);
		mprintf(" score = %d  stars = %d ", p, s);
		jg01_consctrl1(x, y);
		jg01_consctrl2(3, 0);
		g01_putc('@');
		f[x][y] = 1;
		jg01_sleep1(32 - 10, 50);
		k = jg01_inkey2();
		if (k == '8') { u =  0; v = -1; }
		if (k == '6') { u = +1; v =  0; }
		if (k == '4') { u = -1; v =  0; }
		if (k == '2') { u =  0; v = +1; }
		x += u;
		y += v;
		if ((u | v) != 0) {
			p++;
			if (f[x][y] == 2) {
				p += 100;
				s++;
			}
			if (f[x][y] == 1)
				break;
		}
	}
	jg01_consctrl1(0, 21);
	jg01_consctrl2(7, 0);
//	g01_putc('\n');
	return;
}
