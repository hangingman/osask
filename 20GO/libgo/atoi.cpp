/* copyright(C) 2002 H.Kawai (under KL-01). */

#include <cstdlib>	/* strtol */
#include <cstdio>	/* NULL */

int GO_atoi(const char *s)
{
	return (int) strtol(s, (char **) NULL, 10);
}
