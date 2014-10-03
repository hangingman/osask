#include <guigui01.h>

unsigned char cmdusage[] = {
    0x86, 0x50,
    0x88, 0x8c,
    0x40
};

void G01Main()
{
	static char head[16] = {
		0x89, 0xff, 0xff, 0xff, 0x01, 0x00, 0x00, 0x00,
		0x4f, 0x53, 0x41, 0x53, 0x4b, 0x43, 0x4d, 0x50
	};
    unsigned char *tek = jg01_malloc(4 * 1024 * 1024), *org = jg01_malloc(8 * 1024 * 1024), *p;
    int i, j, k;
    g01_setcmdlin(cmdusage);
    g01_getcmdlin_fopen_s_0_4(0);
	i = jg01_fread1f_4(4 * 1024 * 1024 - 32, tek + 32);
	if (i < 4 || tek[0 + 32] != 0x47 || tek[1 + 32] != 0x01)
		goto cpy_only;
	if ((tek[2 + 32] & 0xf0) != 0x70) {
cpy_only:
    	g01_getcmdlin_fopen_s_3_5(1);
		jg01_fwrite1f_5(i, tek + 32);
		return;
	}
	if (tek[2 + 32] != 0x70) {
interr:
		g01_putstr0_exit1("Internal error");
	}
	org[0] = 0x47;
	org[1] = 0x01;
	if ((tek[3 + 32] & 0xf0) != 0x10)
		goto interr;
	for (j = 0; j < 16; j++)
		tek[j] = head[j];
	p = &tek[4 + 32];
	if ((*p & 0x80) == 0) {
		j = *p++;
	} else if ((*p & 0xc0) == 0x80) {
		j = (p[0] & 0x3f) << 8 | p[1];
		p += 2;
	} else if ((*p & 0xe0) == 0xc0) {
		j = (p[0] & 0x1f) << 16 | p[1] << 8 | p[2];
		p += 3;
	} else if ((*p & 0xf0) == 0xe0) {
		j = (p[0] & 0x0f) << 24 | p[1] << 16 | p[2] << 8 | p[3];
		p += 4;
	} else {
		goto interr;
	}
	j += 4;
	tek[16] = (j >> 27) & 0xfe;
	tek[17] = (j >> 20) & 0xfe;
	tek[18] = (j >> 13) & 0xfe;
	tek[19] = (j >>  6) & 0xfe;
	tek[20] = (j << 1 | 1) & 0xff;
	k = 21;
	if ((tek[3 + 32] &= 0x0f) != 0) {
		k++;
		if (tek[3 + 32] == 1)
			tek[21] = 0x15;
		if (tek[3 + 32] == 2)
			tek[21] = 0x19;
		if (tek[3 + 32] == 3)
			tek[21] = 0x21;
		if (tek[3 + 32] >= 4)
			goto interr;
	}
	do {
		tek[k++] = *p++;
	} while (p < &tek[32 + i]);
	if (jg01_tekdecomp(k, tek, org + 2) != 0)
		g01_putstr0_exit1("Broken .g01 format");
    g01_getcmdlin_fopen_s_3_5(1);
	jg01_fwrite1f_5(j + 2, org);
	return;
}
