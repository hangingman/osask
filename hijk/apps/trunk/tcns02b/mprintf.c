#include <guigui01.h>
#include <stdarg.h>

void mprintf(const char *f, ...)
{
	va_list ap;
	int i, j;
	char s[16];

	va_start(ap, f);
	while (*f != '\0') {
		if (*f != '%') {
//letter:
			g01_putc(*f++);
		} else {
			f++;
		//	if (*f == '%')
		//		goto letter;
			if (*f == 'd') {
				i = va_arg(ap, int);
				if (i < 0) {
					g01_putc('-');
					i = - i;
				}
				s[15] = '\0';
				j = 15;
				do {
					j--;
					s[j] = (i % 10) + '0';
					i /= 10;
				} while (i > 0);
				g01_putstr0(&s[j]);
				f++;
			}
		}
	}
	va_end(ap);
	return;
}
