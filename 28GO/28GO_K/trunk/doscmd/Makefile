include ../z_tools/command.txt

TARGET     = doscmd
MODE       = exe
OBJS       = $(TARGET).obj
STACKSIZE  = 0x1000000 # 16MB (default)

INCPATH    = ../z_tools/win32/
CC1OPT     =

RULEFILE = ../z_tools/guigui00/guigui00.rul

LIBPATH  = $(INCPATH)
LIBS     = $(LIBPATH)libmingw.lib

# 以上の項目はあなたのディレクトリ構成にあわせて書き換える

ALL:
	$(MAKE) $(TARGET).$(MODE)

%.ca : %.c Makefile
	$(SJISCONV) in:$*.c out:$*.ca

%.gas : %.ca Makefile
	$(CC1) -I$(INCPATH) $(CC1OPT) -Os -Wall -quiet -o $*.gas $*.ca

%.nas : %.gas Makefile
	$(GAS2NASK) $*.gas $*.nas

%.obj : %.nas Makefile
	$(NASK) $*.nas $*.obj

%.ias : %.ask Makefile
	$(CPP0) -o $*.ias $*.ask

%.3as : %.ias Makefile
	$(ASKA) $*.ias $*.3as

%.nas : %.3as Makefile
	$(NASKCNV) $*.3as $*.nas

%.lst : %.nas Makefile
	$(NASK) $*.nas $*.obj $*.lst

$(TARGET).exe : $(OBJS) Makefile
	$(LINK) -o $(TARGET).exe -Map $(TARGET).map $(OBJS) $(LIBS)

#$(TARGET).exe : $(OBJS) Makefile
#	link32.exe /opt:nowin98 /merge:.data=.text /merge:.rdata=.text \
#		/section:.text,erw /SUBSYSTEM:CONSOLE \
#		/DEFAULTLIB:$(TOOLPATH)\lnk32lib\kernel32.lib \
#		/MAP:$(TARGET).map $(OBJS)

clean :
	-$(DELE) *.obj
	-$(DELE) $(TARGET).map

src_only :
	$(MAKE) clean
	-$(DELE) $(TARGET).$(MODE)
