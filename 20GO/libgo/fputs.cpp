/* copyright(C) 2002 H.Kawai (under KL-01). */

#include <cstdio>
#include <cstring>
#include "go_stdio.hpp"

int GO_fputs(const char *s, GO_FILE *stream)
{
	unsigned int l = strlen(s);
	#warning "Fix me"
	//fwrite(s, 1, l, stream);
	return l;
}
