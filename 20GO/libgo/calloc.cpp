/* copyright(C) 2002 H.Kawai (under KL-01). */

#include <cstdlib>
#include <cstring>

void *GO_calloc(size_t nobj, size_t size)
{
	void *p;
	size *= nobj;
	if (p = malloc(size))
		memset(p, 0, size);
	return p;
}
