#include <guigui01.h>
#include <stdio.h>

void G01Main()
{
	int c;
	char s[32];
	for (c = 1; c <= 15; c++) {
		jg01_consctrl2(c, 0);
		sprintf(s, "color %2d\n", c);
		g01_putstr0(s);
	}
	return;
}
