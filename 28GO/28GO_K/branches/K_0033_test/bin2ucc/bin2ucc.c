typedef unsigned char UCHAR;

#include <guigui01.h>

#define MAXSIZ	8 * 1024 * 1024

#define CMDLIN_FLG_H	0
#define	CMDLIN_IN		1
#define	CMDLIN_OUT		2
#define CMDLIN_LABEL	3

static unsigned char cmdusg[] = {
	0x86, 0x51,
		0x11, '-', 'h', 0x20,
		0x88, 0x8c,
		0x04, 'l', 'a', 'b', 'e', 'l', 0x31, 0x01,
	0x40
};

void put8(unsigned char c)
{
	unsigned char s[4];
	s[0] = '0';
	s[1] = 'x';
	s[2] = ((c >> 4) & 0x0f) | '0';
	s[3] = ( c       & 0x0f) | '0';
	if (s[2] > '9')
		s[2] += 'a' - '0' - 10;
	if (s[3] > '9')
		s[3] += 'a' - '0' - 10;
	jg01_fwrite1f_5(4, s);
	return;
}

void G01Main()
{
	UCHAR *buf, l[12];
	int i, j;

	g01_setcmdlin(cmdusg);
	buf = jg01_malloc(MAXSIZ);
	g01_getcmdlin_str_s0(CMDLIN_LABEL, 12, l);
	if (*l == '\0')
		g01_putstr0_exit1("label error.");
	g01_getcmdlin_fopen_s_0_4(CMDLIN_IN);
	i = jg01_fread1f_4(MAXSIZ, buf);
	if (i == 0)
		g01_putstr0_exit1("input-file-size == 0.");
	g01_getcmdlin_fopen_s_3_5(CMDLIN_OUT);
	jg01_fwrite0f_5("unsigned char ");
	jg01_fwrite0f_5(l);
	jg01_fwrite0f_5("[] = {\n\t");
	if (g01_getcmdlin_flag_o(CMDLIN_FLG_H) != 0) {
		for (j = 0; j <= 16; j += 8) {
			put8((i >> j) & 0xff);
			jg01_fwrite0f_5(", ");
		}
		put8((i >> 24) & 0xff);
		jg01_fwrite0f_5(",\n\t");
	}
	for (j = 0; j < i - 1; j++) {
		put8(buf[j]);
		if (j > 0 && (j & 0x0f) == 0x0f)
			jg01_fwrite0f_5(",\n\t");
		else
			jg01_fwrite0f_5(", ");
	}
	put8(buf[j]);
	jg01_fwrite0f_5("\n};\n");
	return;
}
