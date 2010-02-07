#include <guigui01.h>
#include <stdio.h>

void G01Main()
{
	char *b = g01_bss1a1; /* 2MB */
	int argc = g01_getcmdlin_argc(1), i;
	sprintf(b, "argc = %d\n", argc);
	g01_putstr0(b);
	g01_putstr0("all = \"");
	g01_getcmdlin_str_s0_0(1024 * 1024, b);
	g01_putstr0(b);
	g01_putstr0("\"\n");
	for (i = 0; i < argc; i++) {
		sprintf(b, "argv[%03d] = \"", i);
		g01_putstr0(b);
		g01_getcmdlin_str_m0_1(i, 1024 * 1024, b);	/* argv[i] */
		g01_putstr0(b);
		g01_putstr0("\"\n");
	}
	return;
}
