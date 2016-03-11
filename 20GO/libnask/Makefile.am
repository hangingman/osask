# libnask
noinst_LTLIBRARIES	= libnask.la
libnask_la_LDFLAGS	= -no-undefined -version-info 1:0:0
libnask_la_SOURCES	= ll.cpp ll.hpp nask.cpp
libnask_la_CXXFLAGS     = -I../include

# test libnask
check_PROGRAMS		= nasktest
nasktest_SOURCES	= nasktest.cpp
nasktest_LDADD		= libnask.la

#TESTS_ENVIRONMENT	= ./nasktest
TESTS = nasktest
#XFAIL_TESTS = data2 data5
AUTOMAKE_OPTIONS = color-tests
