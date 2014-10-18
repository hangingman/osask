/* copyright(C) 2002 H.Kawai (under KL-01). */

#include <go_stdarg.hpp>
#include <go_stdio.hpp>
#include <go_stdlib.hpp>

#define MAXBUFSIZ		(64 * 1024)

int GO_vfprintf(GO_FILE *stream, const char *format, va_list arg)
{
	int i;
	char *s = reinterpret_cast<char*>(
	     malloc(MAXBUFSIZ)
	);
	
	i = vsprintf(s, format, arg);
	fputs(s, stream);
	free(s);
	return i;
}
