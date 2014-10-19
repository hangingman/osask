# programs
bin_PROGRAMS   = nask

#
# nask
#
nask_SOURCES	= \
mmmalloc.cpp	  \
Nasuka.cpp	  \
Nasuka_sub.cpp	  \
ll_lib.cpp	  \
ll.cpp

nask_CXXFLAGS   = -I$(top_builddir)/28GO/28GO_K/include \
		  -I$(top_builddir)/28GO/28GO_K/g01libc1/g01lib_h
nask_LDADD	= ../drv_stdc/libdrvstdc.la ../go_lib/libgolib.la
