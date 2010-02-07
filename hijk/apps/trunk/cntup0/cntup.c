#include <guigui01.h>
#include <stdio.h>

void G01Main()
{
	int i;
	char s[16];
	for (i = 0; ; i++) {
		sprintf(s, "\r%d", i);
		g01_putstr0(s);
		jg01_sleep1(32, 1);
	}
	return;
}
