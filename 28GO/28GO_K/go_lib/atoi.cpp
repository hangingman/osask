/* copyright(C) 2002 H.Kawai (under KL-01). */

#include <go_stdlib.hpp>	/* strtol */
#include <go_stdio.hpp>	/* NULL */

int GO_atoi(const char *s)
{
	return (int) strtol(s, (char **) NULL, 10);
}
