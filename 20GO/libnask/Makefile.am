# automake
AUTOMAKE_OPTIONS        = color-tests subdir-objects

# for md5
SUBDIR = data

# include rules
include $(top_builddir)/rules.mk

# libnask
noinst_LTLIBRARIES	= libnask.la
libnask_la_LDFLAGS	= -no-undefined -version-info 1:0:0
libnask_la_SOURCES	= ll.cpp ll.hpp libnask.cpp libnask.hpp
libnask_la_CXXFLAGS     = -I../include

# nask
bin_PROGRAMS		= nask$(EXEEXT)
nask_SOURCES		= nask.cpp
nask_CXXFLAGS		= -I$(top_builddir)/20GO/include -I$(top_builddir)/20GO/libnask
nask_LDADD		= $(top_builddir)/20GO/libnask/libnask.la $(top_builddir)/20GO/libgo/libgo.la

# test libnask
check_PROGRAMS		= nask_unit_test$(EXEEXT)
nask_unit_test_SOURCES	= nask_unit_test.cpp
nask_unit_test_CXXFLAGS	= -I$(top_builddir)/20GO/include
nask_unit_test_LDADD	= libnask.la

# functional test
TESTS = nask_unit_test nask_functional_test.sh
