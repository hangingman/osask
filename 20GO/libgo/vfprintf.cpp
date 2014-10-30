/* copyright(C) 2002 H.Kawai (under KL-01). */

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include "go_stdio.hpp"

#define MAXBUFSIZ		(64 * 1024)

int GO_vfprintf(GO_FILE *stream, const char *format, va_list arg)
{
	int i;
	char *s = reinterpret_cast<char*>(malloc(MAXBUFSIZ));

	i = vsprintf(s, format, arg);
	#warning "Fix me"
	//fputs(s, stream);
	free(s);
	return i;
}
