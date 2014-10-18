/* copyright(C) 2002 H.Kawai (under KL-01). */

#include <go_stdlib.hpp>
#include <go_string.hpp>

void *GO_calloc(size_t nobj, size_t size)
{
	void *p;
	size *= nobj;
	if (p = malloc(size))
		memset(p, 0, size);
	return p;
}
