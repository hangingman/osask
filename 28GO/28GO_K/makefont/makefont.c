#include <guigui01.h>

void G01Main()
{
	static unsigned char cmdlin[] = {
		0x86, 0x50, 0x88, 0x8c, 0x40
	};
	char s[8];
	int i, j, c;

	g01_setcmdlin(cmdlin);
	g01_getcmdlin_fopen_s_0_4(0);
	g01_getcmdlin_fopen_s_3_5(1);
	for (;;) {
		j = 0;
		do {
			if (jg01_fread1_4(1, &c) == 0)
				return;
			if (j < 8)
				s[j] = c;
			j++;
		} while (c != '\n');
		if (s[0] == ' ' || s[0] == '*' || s[0] == '.') {
			for (i = j = 0; j < 8; j++)
				i |= (s[j] == '*') << (7 - j);
			jg01_fwrite1f_5(1, &i);
		}
	}
}
