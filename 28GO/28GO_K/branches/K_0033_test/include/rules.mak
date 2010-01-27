TOOLPATH = ../z_tools/
EFG = $(TARGET)efg01

CPP0 = $(TOOLPATH)28GOcpp0
CC1 = $(TOOLPATH)28GOcc1

MAKE = $(TOOLPATH)make.exe
GAS2NASK = $(EFG) $(TOOLPATH)gas2nask
NASK = $(EFG) $(TOOLPATH)Nasuka
LIBRARIAN = $(EFG) $(TOOLPATH)golib00
LD = $(TOOLPATH)ld.exe
DEL = del

ASKA = aska.exe
NASKCNV = $(EFG) $(TOOLPATH)naskcnv0

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
