/* copyright(C) 2002 H.Kawai (under KL-01). */

#include <cstdio>
#include "go_stdio.hpp"

int GO_fclose(GO_FILE *stream)
{
	if (stream) {
		if (stream->dummy != ~0) {
			GOL_close((GOL_FILE *) stream->dummy);
			GOL_sysfree(stream);
ret0:
			return 0;
		}
		
		if (stream == &GO_stdout)
			goto ret0;
		if (stream == &GO_stderr)
			goto ret0;
	}
	return EOF;
}
