static void escape(char *s, char c)
{
	s[0] = '\\';
	s[3] = '0' + (c & 0x07);
	c >>= 3;
	s[2] = '0' + (c & 0x07);
	s[1] = '0' + (c >> 3);
	return;
}

static char *convmain(char *src0, char *src1, char *dest0, char *dest1, struct STR_FLAGS flags)
{
	char c;
	if (flags.opt[FLAG_S]) {
		while (src0 < src1) {
			c = *src0++;
			if (dest0 + 8 > dest1)
				return NULL;
			if (c <= 0x7f) {
				*dest0++ = c;
				continue;
			}
			if (0xa0 <= c && c <= 0xdf) { /* 半角かな */
escape1:
				escape(dest0, c);
				dest0 += 4;
				continue;
			}
			if (src0 >= src1)
				goto escape1;
			escape(dest0, c);
			dest0 += 4;
			c = *src0++;
			goto escape1;
		}
		return dest0;
	}
	if (flags.opt[FLAG_E]) {
		while (src0 < src1) {
			c = *src0++;
			if (dest0 + 8 > dest1)
				return NULL;
			if (c <= 0x7f) {
				*dest0++ = c;
				continue;
			}
			escape(dest0, c);
			dest0 += 4;
		}
		return dest0;
	}
	return NULL;
}
