/* copyright(C) 2002 H.Kawai (under KL-01). */

#include <cstdarg>
#include <cstdio>
#include <cstdlib>

#define MAXBUFSIZ		(64 * 1024)

int GO_printf(const char *format, ...)
{
	int i;
	va_list ap;
	char *s = reinterpret_cast<char*>(malloc(MAXBUFSIZ));

	va_start(ap, format);
	i = vsprintf(s, format, ap);
	va_end(ap);
	fputs(s, stdout);
	free(s);
	return i;
}

