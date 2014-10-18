/* copyright(C) 2002 H.Kawai (under KL-01). */

#include <stdarg.h>
#include <go_stdio.hpp>

int GO_sprintf(char *s, const char *format, ...)
{
	int i;
	va_list ap;

	va_start(ap, format);
	i = vsprintf(s, format, ap);
	va_end(ap);
	return i;
}

