/* copyright(C) 2002 H.Kawai (under KL-01). */

#include <cstdio>
#include <cstdlib>
#include "go_stdio.hpp"

int GO_fputc(int c, GO_FILE *stream)
{
	if (stream->p >= stream->p1)
		abort();
	*stream->p++ = c;
	/* GOL_debuglog(1, &c); */
	return (unsigned char) c;
}
