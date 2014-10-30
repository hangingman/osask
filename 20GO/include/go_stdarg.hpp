/* copyright(C) 2002 H.Kawai (under KL-01). */

#ifndef GO_STDARG_HPP_
#define GO_STDARG_HPP_

#include <cstdarg>
#include "go.hpp"

#ifndef va_start
   /** __builtin_va_start is deprecated gcc 4.0 later */
   #if __GNUC__ > 3
      #define va_start(v,l)	__builtin_va_start((v),l)
   #else
      #define va_start(v,l)	__builtin_stdarg_start((v),l)
   #endif
#endif

#ifndef va_end
   #define va_end		__builtin_va_end
#endif

#ifndef va_arg
   #define va_arg		__builtin_va_arg
#endif

#ifndef va_copy
   #define va_copy(d,s)		__builtin_va_copy((d),(s))
#endif

#define	va_list			__builtin_va_list

#endif /* GO_STDARG_HPP */
