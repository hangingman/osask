/* copyright(C) 2002 H.Kawai (under KL-01). */

#include <go_stdio.hpp>
#include <go_string.hpp>

int GO_fputs(const char *s, GO_FILE *stream)
{
	unsigned int l = strlen(s);
	fwrite(s, 1, l, stream);
	return l;
}
