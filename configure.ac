#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.67])
AC_INIT([osask], [4.7.0], [newserver002@gmail.com])
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([-Wno-portability])
AC_CONFIG_SRCDIR([OSASK/winman0.c])
AC_CONFIG_HEADERS([config.h])
AC_USE_SYSTEM_EXTENSIONS
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP

# Checks for libraries.

# Checks for header files.
AC_FUNC_ALLOCA
AC_CHECK_HEADERS([limits.h malloc.h stddef.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT32_T
AC_FUNC_OBSTACK
AC_TYPE_OFF_T
AC_C_RESTRICT
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_CHECK_DECLS([sys_siglist])
AC_TYPE_UINT32_T
AC_LIBTOOL_WIN32_DLL
LT_INIT

# Set bool value for Makefile.am
AM_CONDITIONAL(USE_MAC_OSX, false)
AM_CONDITIONAL(USE_LINUX, false)
AM_CONDITIONAL(USE_WINDOWS, false)
AM_CONDITIONAL(USE_BSD, false)

# Compiler vendor check
AX_COMPILER_VENDOR
# Use C++11
AX_CXX_COMPILE_STDCXX_11([noext], [mandatory])
# Some compile flags
AX_CHECK_COMPILE_FLAG([-Wno-parentheses],[CXXFLAGS="$CXXFLAGS -Wno-parentheses"])

# Checks for HostOS
case "$host" in
        *-*-mingw*|*-*-cygwin*|*-*-msys*)
		AM_CONDITIONAL(USE_WINDOWS, true)
                ;;
        *-*-linux*)
		AM_CONDITIONAL(USE_LINUX, true)
                ;;
        *-*-darwin*)
		AM_CONDITIONAL(USE_MAC_OSX, true)
		if test "${ax_cv_c_compiler_vendor}x" = "clangx"; then
		   CXXFLAGS="$CXXFLAGS -stdlib=libc++"
		fi
                ;;
        *-*-*bsd*)
		AM_CONDITIONAL(USE_BSD, true)
                ;;
        *)
                AC_MSG_WARN([*** Please add $host to configure.ac checks!])
                ;;
esac

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC

# Generate base Makefile
AC_CONFIG_FILES([Makefile])

#
# Generate 20GO Makefiles
#
AC_CONFIG_FILES([20GO/Makefile
                 20GO/toolstdc/Makefile])
dnl              20GO/gcc/Makefile
dnl              20GO/go_lib/Makefile
dnl              20GO/intl/Makefile
dnl              20GO/libiberty/Makefile
dnl              20GO/drv_stdc/Makefile
dnl		 20GO/nasklib/Makefile
dnl		 20GO/w32clibc/Makefile])

#
# 28GO support, default is disable
#
AC_ARG_ENABLE([28go],
    AS_HELP_STRING([--enable-28go], 
    		   [Enable feature latest go tools]))
    AS_IF([test "x$enable_28go" = "xyes"], 
    	  [AC_DEFINE(USE_OSASK_28GO, 1, [when building feature latest go tools])
])

#
# 28GO is difficult to build, below files add later...
#

# osask is not yet build
# OSASK/Makefile
# OSASK/bochs/Makefile
# OSASK/nec98/makefile
# OSASK/pcat/Makefile
# OSASK/qemu/Makefile
# OSASK/sgg00lib/Makefile
# OSASK/towns/Makefile
# OSASK/vmware3/Makefile

# mystery zone...
# 28GO/bim2bin/dtk1s0c/Makefile
# 28GO/bim2bin/dtk1s0s/Makefile
# 28GO/bim2bin/dtk2s0f/Makefile
# 28GO/bim2bin/dtk2s0r/Makefile
# 28GO/bim2bin/dtk5f0d/Makefile
# 28GO/bim2bin/dtk5f0r/Makefile
# 28GO/bim2bin/dtk5s0d/Makefile
# 28GO/bim2bin/dtk5s0r/Makefile

if test "x$enable_28go" = "xyes"; then

AC_CONFIG_FILES([28GO/28GO_K/Makefile
                 28GO/aska/Makefile
                 28GO/28GO_K/gas2nask/Makefile
                 28GO/28GO_K/naskcnv0/Makefile
                 28GO/28GO_K/sjisconv/Makefile
                 28GO/28GO_K/golib00/Makefile
                 28GO/28GO_K/golibc/Makefile
                 28GO/28GO_K/nask/Makefile
                 28GO/28GO_K/akusA/Makefile
                 28GO/28GO_K/bim2g01/Makefile
                 28GO/28GO_K/bim2hrb/Makefile
                 28GO/28GO_K/bin2obj/Makefile
                 28GO/28GO_K/bin2ucc/Makefile
                 28GO/28GO_K/cpy/Makefile
                 28GO/28GO_K/cpyc/Makefile
                 28GO/28GO_K/doscmd/Makefile
                 28GO/28GO_K/drv_osa/Makefile
                 28GO/28GO_K/drv_stdc/Makefile
                 28GO/28GO_K/drv_w32/Makefile
                 28GO/28GO_K/g01libc0/Makefile
                 28GO/28GO_K/g01libc1/Makefile
                 28GO/28GO_K/gg00libc/Makefile
                 28GO/28GO_K/go_lib/Makefile
                 28GO/28GO_K/hrblib0a/Makefile
                 28GO/28GO_K/imgtol/Makefile
                 28GO/28GO_K/makefont/Makefile
                 28GO/28GO_K/mtek2org/Makefile
                 28GO/28GO_K/obj2bim/Makefile
                 28GO/28GO_K/osalink1/Makefile
                 28GO/28GO_K/rjcg01/Makefile
                 28GO/28GO_K/tekmin0/Makefile
                 28GO/28GO_K/tekmin1/Makefile
                 28GO/28GO_K/w32clibc/Makefile
                 28GO/Makefile
                 28GO/bim2bin/Makefile
                 28GO/edimg/Makefile
		 ])
fi

AC_OUTPUT
