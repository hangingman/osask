#include <guigui01.h>

void G01Main()
{
	static unsigned char msg[] = {
		'h' | (('w' << 1) & 0x80), 'e' | (('w' << 2) & 0x80), 'l' | (('w' << 3) & 0x80),
		'l' | (('w' << 4) & 0x80), 'o' | (('w' << 5) & 0x80), ',' | (('w' << 6) & 0x80),
		' ' | (('w' << 7) & 0x80),
		'o', 'r', 'l', 'd', 0x00
	};
	g01_putstr0(msg);
	return;
}
