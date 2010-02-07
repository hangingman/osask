#include <guigui01.h>
#include <stdio.h>

void G01Main()
{
	int i;
	for (;;) {
		i = jg01_inkey3();
		if (i == 0x0d)
			break;
		if ('0' <= i && i <= '9')
			i = ((i - '0' + 1) % 10) + '0';
		if ('A' <= i && i <= 'Z')
 			i = ((i - 'A' + 1) % 26) + 'A';
		if ('a' <= i && i <= 'z')
 			i = ((i - 'a' + 1) % 26) + 'a';
		g01_putc(i);
	}
	g01_putc('\n');
	return;
}
