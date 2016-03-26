AUTOMAKE_OPTIONS        = color-tests subdir-objects

# include rules
include $(top_builddir)/rules.mk

#
# binary & sources
#
bin_PROGRAMS	= nask #golib00 sjisconv gas2nask nask naskcnv0

golib00_SOURCES = golib00.cpp
sjisconv_SOURCES= sjisconv.cpp
gas2nask_SOURCES= gas2nask.cpp

nask_SOURCES	= nask.cpp
nask_CXXFLAGS   = -I../include
nask_LDADD	= ../libnask/libnask.la ../libgo/libgo.la

naskcnv0_SOURCES= naskcnv0.cpp
