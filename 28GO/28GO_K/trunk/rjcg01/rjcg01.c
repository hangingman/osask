#include <guigui01.h>

unsigned char cmdusage[] = {
    0x86, 0x50,
    0x88, 0x8c,
    0x40
};

void G01Main()
{
    unsigned char *buf = g01_bss1a1;
    int i, j;
    g01_setcmdlin(cmdusage);
    g01_getcmdlin_fopen_s_0_4(0);
	i = jg01_fread1f_4(2 * 1024 * 1024, buf);
	if (i < 3 || buf[0] != 0x47 || buf[1] != 0x01)
		g01_putstr0_exit1("Not .g01 file");
	if ((buf[2] & 0xf8) != 0 || (buf[2] & 0x04) == 0)
		g01_putstr0_exit1("Format error");
	j = (buf[2] & 1) << 1;
	jg01_rjc1_00s(i - 3 + j, buf + 3 - j);
	buf[2] &= ~0x04;
    g01_getcmdlin_fopen_s_3_5(1);
	jg01_fwrite1f_5(i, buf);
	return;
}
