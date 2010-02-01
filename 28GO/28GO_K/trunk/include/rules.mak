include ../z_tools/command.txt


MAKER = $(MAKE) -r
MAKEC = $(MAKE) -C
LINK = $(LD) -s -Bdynamic
LINK_ADDLIB = $(TOOLPATH)win32/libmingw.lib

GODRV = ../drv_w32/drv_w32.obj
GODRVDIR = ../drv_w32
LINKOPT_CPP0 = --stack 0x1a00000
LINKOPT_CC1  = --stack 0x3600000
LINKOPT_CC1P = --stack 0x3600000

%.s : %.c Makefile ../include/rules.mak
	$(CC1) -include ../include/gccdef.h -I../include -Os -quiet -o $*.s $*.c

%.nas : %.s Makefile ../include/rules.mak
	$(GAS2NASK) -a $*.s $*.nas

%.o : %.nas Makefile ../include/rules.mak
	$(NASK) $*.nas $*.o

%.obj : %.nas Makefile ../include/rules.mak
	$(NASK) $*.nas $*.obj
