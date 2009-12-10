include depend.mak
include rules.mak

OSALINK = osalink0
OSALINK_OPT = osalink0.opt

# make sub directory modules
ANKFONT = ankfont0.bin
BASE = base.exe
SYSGGLIB = sysgg00.lib
SYSGGINC = sgg00lib

SUBDIRS = ankfont0 $(SYSGGINC)
SUBDIRS += $(ARCH)

TARGET = OSASK.EXE

ARCH_OBJS = timerdrv.bin keymos0.bin fdcdrv0.bin vgadrv0.bin decode0.bin

ASK_OBJS =  init.bin tapi0.bin pioneer0.bin papi0.bin

#ASK_OBJS += vesadrv0.bin 

C_OBJS = winman0.bin pokon0.bin

SUB_OBJS = $(ANKFONT)

OBJS = $(BASE) $(SUB_OBJS) $(ASK_OBJS) $(C_OBJS) $(ARCH_OBJS)

INCPATH = ./include
LIBPATH = ./lib 
LIBS = guigui00.lib stdlib.lib $(SYSGGLIB)

# prevent automatic deletion of an intermediate file
.SECONDARY : $(ASK_OBJS:.bin=.3as) $(ASK_OBJS:.bin=.asm) $(ASK_OBJS:.bin=.i) $(ASK_OBJS:.bin=.obj) $(ASK_OBJS:.bin=.exe) $(BASE:.exe=.asm)

ALL :
	for %%i in ($(SUBDIRS)) do $(MAKE) -C %%i
	$(MAKE) $(TARGET)

$(TARGET) : $(OSALINK_OPT) $(OBJS)
	$(OSALINK)

$(OSALINK_OPT) : makefile
	echo on > $(OSALINK_OPT)
	for %%i in ($(OBJS)) do echo %%i >> $(OSALINK_OPT)

$(ANKFONT) :

pcat : FORCE
#	copy /y base.as base.asm
	echo ARCH = pcat>> depend.mak
	$(MAKE)

vmware : FORCE
#	copy /y base.as base.asm
	echo ARCH = pcat>> depend.mak
	echo VMWARE = y>> depend.mak
	$(MAKE)

bochs : FORCE
#	copy /y base.bch base.asm
	echo ARCH = pcat>> depend.mak
	echo BOCHS = 13>> depend.mak
	$(MAKE)

bochs12 : FORCE
#	copy /y base.bch base.asm
	echo ARCH = pcat>> depend.mak
	echo BOCHS = 12>> depend.mak
	$(MAKE)

towns : FORCE 
#	copy /y base.as base.asm
	echo ARCH = towns>> depend.mak
	echo TWSCRL1 = n>> depend.mak
	echo TWVSW = 1024>> depend.mak
	$(MAKE)

FORCE:
	echo on > depend.mak
	echo #DESIGN = WIN9X #Win9x style>> depend.mak
	echo #DESIGN = TMENU #T-OSASK>> depend.mak
	echo #DESIGN = CHO_OSASK #Cho-Kanji style OSASK>> depend.mak
	echo #DESIGN = NEWSTYLE #NWSOS style>> depend.mak

#$(BASE) : $(BASE:.exe=.obj)

pokon0.bim : pokon0.obj
	$(OBJ2BIM) -fixobj $< $(OBJ2BIM_ALIGN_FLAGS)
	$(OBJ2BIM) @$(DEFAULT_RULE_FILE) out:$@ stack:4k $<

pokon0.bin : pokon0.bim
	$(BIM2BIN) malloc:84k file:1024k input:$< output:$@

winman0.bim : winman0.obj
	$(OBJ2BIM) -fixobj $< $(OBJ2BIM_ALIGN_FLAGS)
	$(OBJ2BIM) @$(DEFAULT_RULE_FILE) out:$@ stack:4k $<

winman0.bin : winman0.bim
	$(BIM2BIN) malloc:620k file:768k input:$< output:$@

%.i : %.ask .\depend.mak
	$(PREPROCESSOR) $(PREPROCESSOR_FLAGS) $<

%.3as : %.i
	$(ASKA) $< $@

%.asm : %.3as
	$(MASMCNV) $(MASMCNV_FLAGS) $*.3as $*.asm

%.obj : %.asm
	$(ASM) $(ASM_FLAGS) /Fl$*.lst $*.asm

%.obj : %.c .\depend.mak
	$(CC) $< $(CFLAGS) -I$(INCPATH) -I$(SYSGGINC)

%.exe : %.obj # for ASKA objs
	$(LINK) $*,$*,$*,nul,nul

%.bin : %.exe # for ASM objs
	$(BIM2BIN) input:$< output:$@ $(BIM2BIN_FLAGS_ASK)
#	$(EXE2BIN) $*.exe $@ $(EXE2BIN_FLAGS_ASK)

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
	for %%i in ($(C_OBJS:.bin=.bim)) do del %%i

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

# dependencies
pokon0.obj : pokon0.c pokon0.h