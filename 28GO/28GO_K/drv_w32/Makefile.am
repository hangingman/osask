noinst_LTLIBRARIES	= libdrvw32.la
libdrvw32_la_LDFLAGS 	= -no-undefined -version-info 1:0:0
libdrvw32_la_SOURCES	= \
cc1main.cpp		  \
cc1pmain.cpp		  \
cpp0main.cpp		  \
cc1drv.cpp

# for windows
if USE_WINDOWS
libdrvw32_la_SOURCES +=   \
wfile_b.cpp		  \
msgout.cpp		  \
drv_w32.cpp
endif
