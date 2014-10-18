/* copyright(C) 2002 H.Kawai (under KL-01). */

#include <go_stdlib.hpp>	/* strtod */
#include <go_stdio.hpp>	/* NULL */

double GO_atof(const char *s)
{
	return strtod(s, (char **) NULL);
}
