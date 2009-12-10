include depend.mak
include rules.mak

OSALINK = osalink0
OSALINK_OPT = osalink0.opt

# make sub directory modules
ANKFONT = ankfont0.bin
BASE = base.exe
SYSGGLIB = sysgg00.lib
SYSGGINC = sgg00lib

SUBDIRS = ankfont base $(SYSGGINC)
SUBDIRS += $(ARCH)

TARGET = OSASK.EXE

ARCH_OBJS = timerdrv.bin keymos0.bin fdcdrv0.bin vgadrv0.bin decode0.bin

ASK_OBJS =  tapi0.bin pioneer0.bin papi0.bin

#ASK_OBJS += vesadrv0.bin 

C_OBJS = winman0.bin pokon0.bin

SUB_OBJS = $(BASE) $(ANKFONT)

OBJS = $(SUB_OBJS) $(ASK_OBJS) $(C_OBJS) $(ARCH_OBJS)

INCPATH = ./include
LIBPATH = ./lib 
LIBS = guigui00.lib stdlib.lib $(SYSGGLIB)

ALL :
	for %%i in ($(SUBDIRS)) do $(MAKE) -C %%i
	make $(TARGET)

$(TARGET) : $(OSALINK_OPT) $(OBJS)
	$(OSALINK)

$(OSALINK_OPT) : Makefile
	echo on > $(OSALINK_OPT)
	for %%i in ($(OBJS)) do echo %%i >> $(OSALINK_OPT)

$(BASE) :

$(ANKFONT) :

pcat : FORCE
	echo ARCH = pcat> depend.mak
	make

vmware : FORCE
	echo ARCH = pcat> depend.mak
	echo VMWARE = y>> depend.mak
	make

towns : FORCE 
	echo ARCH = towns> depend.mak
	make

FORCE:

%.bin : %.ask Makefile depend.mak
	$(PREPROCESSOR) $(PREPROCESSOR_FLAGS) $<
	$(ASKA) $*.i $*.3as
	$(MASMCNV) $(MASMCNV_FLAGS) $*.3as $*.asm
	$(ASM) $(ASM_FLAGS) /Fl$*.lst $*.asm
	$(LINK) $*,$*,$*,nul,nul
	$(EXE2BIN) $*.exe $@ $(EXE2BIN_FLAGS_ASK)

%.exe : %.c Makefile depend.mak
	$(CC) $< $(CFLAGS) -I$(INCPATH) -I$(SYSGGINC)
	$(LINK32) $(LINK32_FLAGS) /LIBPATH:$(LIBPATH) $*.obj $(LIBS)

pokon0.bin : pokon0.exe Makefile
	$(EXE2BIN) $< $@ -s 40k

winman0.bin : winman0.exe Makefile
	$(EXE2BIN) $< $@ -s 96k

clean:
	for %%i in ($(SUBDIRS)) do $(MAKE) -i -C %%i clean
	del $(TARGET)
	for %%i in ($(OBJS)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.i)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.3as)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.asm)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.obj)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.lst)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.map)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.exe)) do del %%i
	for %%i in ($(C_OBJS:.bin=.obj)) do del %%i
	for %%i in ($(C_OBJS:.bin=.exe)) do del %%i

dist_clean:
	for %%i in ($(SUBDIRS)) do $(MAKE) -i -C %%i dist_clean
#	del $(TARGET)
	for %%i in ($(OBJS)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.i)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.3as)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.asm)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.obj)) do del %%i
#	for %%i in ($(ASK_OBJS:.bin=.lst)) do del %%i
#	for %%i in ($(ASK_OBJS:.bin=.map)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.exe)) do del %%i
	for %%i in ($(C_OBJS:.bin=.obj)) do del %%i
	for %%i in ($(C_OBJS:.bin=.exe)) do del %%i

