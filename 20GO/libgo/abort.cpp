/* copyright(C) 2002 H.Kawai (under KL-01). */

#include <cstdlib>
#include "go.hpp"

void GO_abort(void)
{
	GOL_sysabort(GO_TERM_ABORT);
}
