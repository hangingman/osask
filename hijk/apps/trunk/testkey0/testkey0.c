#include <guigui01.h>
#include <stdio.h>

void G01Main()
{
	int i;
	char s[16];
	for (i = 0; ; i++) {
		sprintf(s, "\r%d", i);
		g01_putstr0(s);
		if (jg01_inkey2() != 0)
			break;
	}
	return;
}
