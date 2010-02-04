# ÉâÉCÉuÉâÉäê∂ê¨ópMakefile

TARGET   = harilibc.lib
OBJS     = startup.obj

TOOLPATH = ../z_tools/
EFG      = $(TOOPLATH)efg01.exe
INCPATH  = ../z_tools/haribote/

SJISCONV = $(EFG) $(TOOLPATH)sjisconv sjis
CC1      = $(TOOLPATH)cc1.exe -I$(INCPATH) -Os -Wall -quiet
GAS2NASK = $(EFG) $(TOOLPATH)gas2nask -a
NASK     = $(EFG) $(TOOLPATH)nask
GOLIB    = $(EFG) $(TOOLPATH)golib00
MAKE     = $(TOOLPATH)make.exe -r
DELE     = del

ALL :
	$(MAKE) $(TARGET)

%.ca : %.c Makefile
	$(SJISCONV) in:$*.c out:$*.ca

%.gas : %.ca Makefile
	$(CC1) -o $*.gas $*.ca

%.nas : %.gas Makefile
	$(GAS2NASK) $*.gas $*.nas

%.obj : %.nas Makefile
	$(NASK) $*.nas $*.obj

$(TARGET) : $(OBJS) Makefile
	$(GOLIB) out:$(TARGET) $(OBJS)

clean :
	-$(DELE) *.obj

src_only :
	$(MAKE) clean
	-$(DELE) $(TARGET)
