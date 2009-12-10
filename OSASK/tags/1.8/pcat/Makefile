include ..\depend.mak
include ..\rules.mak

ASK_OBJS =  timerdrv.bin keymos0.bin fdcdrv0.bin vgadrv0.bin decode0.bin

#ASK_OBJS += vesadrv0.bin 

C_OBJS = 

SUB_OBJS = 

OBJS = $(SUB_OBJS) $(ASK_OBJS) $(C_OBJS)

INCPATH = ../include
LIBPATH = ../lib 
LIBS = guigui00.lib stdlib.lib $(SYSGGLIB)

DISTDIR = ..

ALL : $(OBJS)
#	for %%i in ($(SUBDIRS)) do $(MAKE) -C %%i
#	make $(TARGET)

%.bin : %.ask makefile ..\depend.mak
	$(PREPROCESSOR) $(PREPROCESSOR_FLAGS) $<
	$(ASKA) $*.i $*.3as
	$(MASMCNV) $(MASMCNV_FLAGS) $*.3as $*.asm
	$(ASM) $(ASM_FLAGS) /Fl$*.lst $*.asm
	$(LINK) $*,$*,$*,nul,nul
	$(EXE2BIN) $*.exe $@ $(EXE2BIN_FLAGS_ASK)
	copy $@ ..

%.exe : %.c makefile ..\depend.mak
	$(CC) $< $(CFLAGS) -I$(INCPATH) -I$(SYSGGINC)
	$(LINK32) $(LINK32_FLAGS) /LIBPATH:$(LIBPATH) $*.obj $(LIBS)
	copy $@ ..

clean:
	for %%i in ($(ASK_OBJS)) do del %%i
#	for %%i in ($(C_OBJS)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.i)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.3as)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.asm)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.obj)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.lst)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.map)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.exe)) do del %%i

dist_clean:
#	for %%i in ($(ASK_OBJS)) do del %%i
#	for %%i in ($(C_OBJS)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.i)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.3as)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.asm)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.obj)) do del %%i
#	for %%i in ($(ASK_OBJS:.bin=.lst)) do del %%i
#	for %%i in ($(ASK_OBJS:.bin=.map)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.exe)) do del %%i


