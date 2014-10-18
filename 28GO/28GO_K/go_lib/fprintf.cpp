/* copyright(C) 2002 H.Kawai (under KL-01). */

#include <go_stdarg.hpp>
#include <go_stdio.hpp>
#include <go_stdlib.hpp>

#define MAXBUFSIZ		(64 * 1024)

int GO_fprintf(FILE *stream, const char *format, ...)
{
	int i;
	va_list ap;
	char *s = reinterpret_cast<char*>(malloc(MAXBUFSIZ));

	va_start(ap, format);
	i = vsprintf(s, format, ap);
	va_end(ap);
	fputs(s, stream);
	free(s);
	return i;
}

