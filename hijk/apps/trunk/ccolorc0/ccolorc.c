#include <guigui01.h>
#include <stdio.h>

static unsigned char cmdusg[] = {
	0x86, 0x50,
		0x10, 'f', 0x14, 't', 'e', 'x', 't',
		0x10, 'b', 0x14, 'b', 'a', 'c', 'k',
		0x40
};

void G01Main()
{
	int f, b;
	g01_setcmdlin(cmdusg);
	f = g01_getcmdlin_int_o(0, 7);
	b = g01_getcmdlin_int_o(1, 0);
	jg01_consctrl2(f, b);
	return;
}
