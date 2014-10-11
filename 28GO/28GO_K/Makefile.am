ACLOCAL_AMFLAGS = -I m4

#
# Depending on <guigui01.h> module list...
# Below modules can't build unless guigui01 will be built
#
GUIGUI_DEP_MODULES = \
	  bin2obj    \
	  cpyc	     \
	  akusA	     \
	  bin2ucc    \
	  mtek2org   \
	  nask       \
	  bim2g01    \
	  naskcnv0   \
	  obj2bim    \
	  sjisconv   \
	  golib00    \
	  osalink1   \
	  rjcg01     \
	  gas2nask   \
	  makefont   \
	  tekmin0    \
	  tekmin1    \
	  bim2hrb
#
# Depending on Asuka module list...
# Below modules can't build unless Asuka will be built
#
ASKA_DEP_MODULES =   \
	  gg00libc   \
	  imgtol     \
	  g01libc0   \
	  cpy	     \
	  g01libc1

#
# Build module list
#
SUBDIRS = drv_osa               \
	  go_lib                \
	  drv_stdc              \
	  drv_w32               \
	  $(ASKA_DEP_MODULES)   \
	  $(GUIGUI_DEP_MODULES)

# for windows
if USE_WINDOWS
SUBDIRS += w32clibc  \
	   doscmd
endif
