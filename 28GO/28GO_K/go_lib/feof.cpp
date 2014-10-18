/* copyright(C) 2002 H.Kawai (under KL-01). */

#include <go_stdio.hpp>

int GO_feof(GO_FILE *stream)
{
	return stream->p >= stream->p1;
}
