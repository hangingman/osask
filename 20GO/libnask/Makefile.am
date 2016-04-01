# automake
AUTOMAKE_OPTIONS        = color-tests subdir-objects

# include rules
include $(top_builddir)/rules.mk

# libnask
noinst_LTLIBRARIES	= libnask.la
libnask_la_LDFLAGS	= -no-undefined -version-info 1:0:0
libnask_la_SOURCES	= ll.cpp ll.hpp libnask.cpp libnask.hpp
libnask_la_CXXFLAGS     = -I../include

# nask
bin_PROGRAMS		= nask
nask_SOURCES		= nask.cpp
nask_CXXFLAGS		= -I$(top_builddir)/20GO/include -I$(top_builddir)/20GO/libnask
nask_LDADD		= $(top_builddir)/20GO/libnask/libnask.la $(top_builddir)/20GO/libgo/libgo.la

# test libnask
check_PROGRAMS		= nasktest
nasktest_SOURCES	= nasktest.cpp
nasktest_LDADD		= libnask.la

#TESTS_ENVIRONMENT	= ./nasktest
TESTS = nasktest
#XFAIL_TESTS = data2 data5
