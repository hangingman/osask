#include <guigui01.h>

void G01Main()
{
	static unsigned char cmdlin[] = {
		0x86, 0x50, 0x8a, 0x8c, 0x40
	};
	int i, j;

	g01_setcmdlin(cmdlin);
	g01_getcmdlin_fopen_s_3_5(1);
	for (j = 0;; j++) {
		if (g01_getcmdlin_fopen_m_0_4(0, j) == 0)
			break;
		do {
			i = jg01_fread1_4(2 * 1024 * 1024, g01_bss1a1);
			jg01_fwrite1f_5(i, g01_bss1a1);
		} while (i != 0);
	}
	return;
}
