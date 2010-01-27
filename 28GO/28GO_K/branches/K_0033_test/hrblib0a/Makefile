# ÉâÉCÉuÉâÉäê∂ê¨ópMakefile
include ../z_tools/command.txt

TARGET   = harilibc.lib
OBJS     = startup.obj

INCPATH  = ../z_tools/haribote/


ALL :
	$(MAKE) $(TARGET)

%.ca : %.c Makefile
	$(SJISCONV) in:$*.c out:$*.ca

%.gas : %.ca Makefile
	$(CC1) -I$(INCPATH) -Os -Wall -quiet -o $*.gas $*.ca

%.nas : %.gas Makefile
	$(GAS2NASK) -a $*.gas $*.nas

%.obj : %.nas Makefile
	$(NASK) $*.nas $*.obj

$(TARGET) : $(OBJS) Makefile
	$(GOLIB) out=$(TARGET) $(OBJS)

clean :
	-$(DELE) *.obj

src_only :
	$(MAKE) clean
	-$(DELE) $(TARGET)
