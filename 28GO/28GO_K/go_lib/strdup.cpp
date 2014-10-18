/* copyright(C) 2002 H.Kawai (under KL-01). */

#include <go_stdlib.hpp>
#include <string.h>

char *GO_strdup(const char *cs)
{
     	char *t = reinterpret_cast<char*>(
	     malloc(strlen(cs) + 1)
	);
	
	if (t)
		strcpy(t, cs);
	return t;
}
