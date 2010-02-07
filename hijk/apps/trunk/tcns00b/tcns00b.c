#include <guigui01.h>
#include <stdio.h>

void mprintf(const char *f, ...);

void G01Main()
{
	int c;
	for (c = 1; c <= 15; c++) {
		jg01_consctrl2(c, 0);
		mprintf("color %d\n", c);
	}
	return;
}
