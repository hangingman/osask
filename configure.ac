#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([osask], [4.7.0], [newserver002@gmail.com])
AM_INIT_AUTOMAKE
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
AC_CHECK_HEADERS([langinfo.h libintl.h limits.h locale.h malloc.h stddef.h stdlib.h string.h unistd.h utime.h wchar.h wctype.h])

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
LT_INIT

# Checks for library functions.
AM_GNU_GETTEXT_VERSION([0.18])
AC_FUNC_ERROR_AT_LINE
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC
AC_FUNC_MBRTOWC
AC_FUNC_MKTIME
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_FUNC_STRTOD
AC_CHECK_FUNCS([__argz_count __argz_next __argz_stringify btowc bzero getcwd getpagesize memchr memmove mempcpy memset munmap nl_langinfo re_comp regcomp setenv setlocale sqrt stpcpy strcasecmp strchr strcspn strdup strerror strncasecmp strpbrk strrchr strspn strstr strtol strtoul tzset utime])

#
# Below files add later...
#

# GCC is not yet build...
# 28GO/28GO_G/gcc/Makefile
# 28GO/28GO_G/intl/Makefile
# 28GO/28GO_G/libiberty/Makefile

# osask is not yet build
# OSASK/Makefile
# OSASK/bochs/Makefile
# OSASK/nec98/makefile
# OSASK/pcat/Makefile
# OSASK/qemu/Makefile
# OSASK/sgg00lib/Makefile
# OSASK/towns/Makefile
# OSASK/vmware3/Makefile

AC_CONFIG_FILES([28GO/28GO_K/Makefile
                 28GO/28GO_K/Nasuka/Makefile
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
                 28GO/28GO_K/gas2nask/Makefile
                 28GO/28GO_K/gg00libc/Makefile
                 28GO/28GO_K/go_lib/Makefile
                 28GO/28GO_K/golib00/Makefile
                 28GO/28GO_K/golibc/Makefile
                 28GO/28GO_K/hrblib0a/Makefile
                 28GO/28GO_K/imgtol/Makefile
                 28GO/28GO_K/makefont/Makefile
                 28GO/28GO_K/mtek2org/Makefile
                 28GO/28GO_K/naskcnv0/Makefile
                 28GO/28GO_K/obj2bim/Makefile
                 28GO/28GO_K/osalink1/Makefile
                 28GO/28GO_K/rjcg01/Makefile
                 28GO/28GO_K/sjisconv/Makefile
                 28GO/28GO_K/tekmin0/Makefile
                 28GO/28GO_K/tekmin1/Makefile
                 28GO/28GO_K/w32clibc/Makefile
                 28GO/Asuka/Makefile
                 28GO/Makefile
                 28GO/bim2bin/Makefile
                 28GO/bim2bin/dtk1s0c/Makefile
                 28GO/bim2bin/dtk1s0s/Makefile
                 28GO/bim2bin/dtk2s0f/Makefile
                 28GO/bim2bin/dtk2s0r/Makefile
                 28GO/bim2bin/dtk5f0d/Makefile
                 28GO/bim2bin/dtk5f0r/Makefile
                 28GO/bim2bin/dtk5s0d/Makefile
                 28GO/bim2bin/dtk5s0r/Makefile
                 28GO/edimg/Makefile
                 Makefile])
AC_OUTPUT
