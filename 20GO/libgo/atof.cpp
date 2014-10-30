/* copyright(C) 2002 H.Kawai (under KL-01). */

#include <cstdlib>	/* strtod */
#include <cstdio>	/* NULL */

double GO_atof(const char *s)
{
	return strtod(s, (char **) NULL);
}
