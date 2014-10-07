#include <guigui01.h>

unsigned char cmdusage[] = {
    0x86, 0x52,
	0x12, 'm', 'o', 'd', 0x11, '#',
	0x12, 't', 'y', 'p', 0x11, '#',
    0x88, 0x8c,
    0x40
};

void G01Main()
{
    unsigned char *buf = g01_bss1a1, *p = buf + 16, *q, *r, mod, typ, flag = 0, first;
    int i, j;
    g01_setcmdlin(cmdusage);
	mod = g01_getcmdlin_int_o(0, 0);	/* .g01Œ^ */
	typ = g01_getcmdlin_int_o(1, 1);	/* tek5Œ^ */
    g01_getcmdlin_fopen_s_0_4(2);
	i = jg01_fread1f_4(2 * 1024 * 1024, buf);
	if (i < 17 || buf[1] != 0xff || buf[8] != 'O')
		g01_putstr0_exit1("Not tek file");
	j &= 0;
	do {
		j = j << 7 | (*p++) >> 1;
	} while ((p[-1] & 1) == 0);
	first = *p;
	j -= 4;
	if (j < 0)
		g01_putstr0_exit1("Too small input file");
	q = buf + 13;
	if (first == 0x15)
		flag = 1;
	if (first == 0x19)
		flag = 2;
	if (first == 0x21)
		flag = 3;
	q -= (j == 0x7f) + (flag == 0);
	p += (flag != 0);
	q[0] = 0x47;
	q[1] = 0x01;
	q[2] = 0x70 | mod;
	q[3] = typ << 4 | flag;
	r = q + 5;
	if (j < 0x7f) {
		q[4] = j;
	//	r = q + 5;
	} else if (j <= 0x3fff) {
		q[4] = 0x80 | j >> 8;
		q[5] = j & 0xff;
		r = q + 6;
	} else if (j <= 0x1fffff) {	/* 2M-1 */
		q[4] = 0xc0 | j >> 16;
		q[5] = (j >> 8) & 0xff;
		q[6] = j & 0xff;
		r = q + 7;
	} else {
		g01_putstr0_exit1("Internal error");	/* Šî–{“I‚É‚±‚±‚É‚Í—ˆ‚È‚¢‚ª«—ˆŠg’£‚µ‚½‚Æ‚«—p */
	}
	if (r < p) {
		for (j = 0; j > q - r; j--)
			r[j] = r[j - 1];
		q++;
	//	r++;
	}
    g01_getcmdlin_fopen_s_3_5(3);
	jg01_fwrite1f_5(buf + i - q, q);
	return;
}
