/* copyright(C) 2002 H.Kawai (under KL-01). */

/* #include <stdarg.h> */

#if (!defined(STDARG_H)) & 1

#define STDARG_H	1

#if (defined(__cplusplus))
	extern "C" {
#endif

#include "go_lib.hpp"

/** __builtin_va_start is deprecated gcc 4.0 later */
#if __GNUC__ > 3
   #define va_start(v,l)	__builtin_va_start((v),l)
#else
   #define va_start(v,l)	__builtin_stdarg_start((v),l)
#endif

#define va_end			__builtin_va_end
#define va_arg			__builtin_va_arg
#define va_copy(d,s)	__builtin_va_copy((d),(s))
#define	va_list			__builtin_va_list

#if (defined(__cplusplus))
	}
#endif

#endif