#include <guigui01.h>
#include <stdio.h>
#include <stdlib.h>

void mprintf(const char *f, ...);	/* mini-printf */

void G01Main()
{
    int a = 0, k, x = 0, y = 0, p = 0, s = 200;
	srand(jg01_randomseed());
	jg01_consctrl4(80, 25);
	jg01_consctrl2(7, 0);
	jg01_consctrl3();
	for (;;) {
		if (y == 0) {
			a = (rand() % 36) + '0';
			if (a > '9')
				a += 'A' - '0' - 10;
			x = rand() % 79;
		}
		jg01_consctrl1(0, 0);
		mprintf("score = %d  speed = %d  ", p, s);
		jg01_consctrl2(6, 0);
		jg01_consctrl1(x, y);
		g01_putc(a);
		jg01_consctrl2(7, 0);
		if (y >= 22)
			break;
		jg01_sleep1(32 - 10, s);
		jg01_consctrl1(x, y);
		g01_putc(' ');
		k = jg01_inkey2();
		if ('a' <= k && k <= 'z')
			k += 'A' - 'a';
		if (k == a) {
			p += 22 - y;
			s--;
			if (s < 10)
				s = 10;
			y = 0;
		} else {
			if (k != 0) {
				p -= 22 - y;
				if (p < 0)
					p = 0;
			}
			y++;
		}
	}
//	g01_putc('\n');
	return;
}
