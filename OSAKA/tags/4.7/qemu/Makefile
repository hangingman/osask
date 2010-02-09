# Makefile for OSAKA/qemu
#  ver. 0.0

TOOLPATH = ../../z_tools/
INCPATH  = ../../z_tools/guigui00/

include ../make_inc.txt
include ../make_com.txt

MKFL = Makefile ../Makefile ../make_inc.txt ../make_com.txt

ALL :
	$(MAKE) OSAKA.EXE

OSAKA.EXE : $(MKFL) osalink1.opt timerdrv.tek
	$(MAKE) -C .. at
	$(COPY) ..\osaka_at.exe osaka_at.exe
	$(OSALINK)
	$(COPY) OSASK.EXE OSAKA.EXE
	$(DELE) OSASK.EXE
	$(DELE) osaka_at.exe

clean :
	-$(DELE) OSA*.EXE
