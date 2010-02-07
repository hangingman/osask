#include <guigui01.h>
#include <stdio.h>

void G01Main()
{
	int x, y, c, k;
	x = 0;
	y = 0;
	c = 7;
	jg01_consctrl4(80, 25);
	jg01_consctrl2(7, 0);
	jg01_consctrl3();
    for (;;) {
		jg01_consctrl1(x, y); jg01_consctrl2(c, 0); g01_putc('@');
		jg01_sleep1(32 - 7, 1); /* 2^(-7) = 1/128 [sec] */
		k = jg01_inkey2();
		if (k == 0)
			continue;
		jg01_consctrl1(x, y); g01_putc(' ');
		if (k == 0x0d)
			break; /* EnterÇ≈èIóπ */
		if (k == '6' && x < 78) x++;
		if (k == '4' && x >  0) x--;
		if (k == '2' && y < 24) y++;
		if (k == '8' && y >  0) y--;
		if (k == 32) {
			c++;
			if (c == 8)
				c = 1;
		}
	}
	jg01_consctrl2(7, 0);
	jg01_consctrl1(0, 0);
	return;
}
