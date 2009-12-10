include ..\depend.mak
include ..\rules.mak

BASE = base.exe
BASE_OBJ = $(BASE:.exe=.obj)

ASK_OBJS =  timerdrv.bin keymos0.bin fdcdrv0.bin vgadrv0.bin decode0.bin

#ASK_OBJS += vesadrv0.bin 

C_OBJS = 

SUB_OBJS = 

OBJS = $(BASE) $(SUB_OBJS) $(ASK_OBJS) $(C_OBJS)

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
	$(BIM2BIN) input:$*.exe output:$@ $(BIM2BIN_FLAGS_ASK)
#	$(EXE2BIN) $*.exe $@ $(EXE2BIN_FLAGS_ASK)
	copy $@ ..

%.exe : %.obj makefile ..\depend.mak
	$(LINK) $*,$*,$*,nul,nul
	copy $@ ..

$(BASE_OBJ) : $(BASE_ASM) makefile ..\depend.mak
	$(ASM) $(ASM_FLAGS) /Fo$(BASE_OBJ) /Fl$(BASE:.exe=.lst) $(BASE_ASM)

clean:
	del $(BASE)
	for %%i in ($(BASE:.exe=.obj)) do del %%i
	for %%i in ($(BASE:.exe=.lst)) do del %%i
	for %%i in ($(BASE:.exe=.map)) do del %%i
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
	for %%i in ($(BASE:.exe=.obj)) do del %%i
	for %%i in ($(BASE:.exe=.lst)) do del %%i
	for %%i in ($(BASE:.exe=.map)) do del %%i
#	for %%i in ($(ASK_OBJS)) do del %%i
#	for %%i in ($(C_OBJS)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.i)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.3as)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.asm)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.obj)) do del %%i
#	for %%i in ($(ASK_OBJS:.bin=.lst)) do del %%i
#	for %%i in ($(ASK_OBJS:.bin=.map)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.exe)) do del %%i


