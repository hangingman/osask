AUTOMAKE_OPTIONS        = color-tests subdir-objects

# include rules
include $(top_builddir)/rules.mk

#
# binary & sources
#
bin_PROGRAMS	= #golib00 sjisconv gas2nask nask naskcnv0

golib00_SOURCES = golib00.cpp
sjisconv_SOURCES= sjisconv.cpp
gas2nask_SOURCES= gas2nask.cpp

sjisconv_CXXFLAGS = -I$(top_builddir)/20GO/include -I$(top_builddir)/20GO/libnask

naskcnv0_SOURCES= naskcnv0.cpp
