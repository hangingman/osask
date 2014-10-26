ACLOCAL_AMFLAGS = -I m4

#
# Depending on <guigui01.h> module list...
# Below modules can't build unless guigui01 will be built
#
GUIGUI_DEP_MODULES = \
	  bin2obj    \
	  cpyc	     \
	  bin2ucc    \
	  mtek2org   \
	  bim2g01    \
	  obj2bim    \
	  osalink1   \
	  rjcg01     \
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
# Important modules for osask ( Asuka, nask, naskcnv )
# Below modules must be built first
#
NASK_MODULES =       \
          gas2nask   \
	  naskcnv0   \
          sjisconv   \
	  golib00    \
	  golibc     \
	  nask

#
# Build module list
#
SUBDIRS = $(NASK_MODULES)       \
	  go_lib                \
	  drv_osa               \
	  drv_stdc              \
	  drv_w32               \
	  $(ASKA_DEP_MODULES)   \
	  $(GUIGUI_DEP_MODULES)

# for windows
if USE_WINDOWS
SUBDIRS += w32clibc  \
	   doscmd
endif
