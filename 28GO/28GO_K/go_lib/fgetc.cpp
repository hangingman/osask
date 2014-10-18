/* copyright(C) 2002 H.Kawai (under KL-01). */

#include <go_stdio.hpp>

int GO_fgetc(GO_FILE *stream)
{
	int c = EOF;
	if (stream->p < stream->p1)
		c = *stream->p++;
	return c;
}
