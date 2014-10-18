/* copyright(C) 2002 H.Kawai (under KL-01). */

#include <go_stdio.hpp>
#include <go_stdlib.hpp>

/* 読んだ文字しか読み戻せないバージョン */
/* 本来のungetcとは仕様が異なる */

int GO_ungetc(int c, GO_FILE *stream)
{
	if (stream->p0 < stream->p && stream->p[-1] == c) {
		stream->p--;
		return c;
	}
	fputs("GO_ungetc:error!\n", stderr);
	abort();
}
