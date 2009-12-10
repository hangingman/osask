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
BOOTIMAGE = $(TARGET:EXE=SYS)
EXE2SYS = exe2sys

ARCH_OBJS = timerdrv.bin keymos0.bin fdcdrv0.bin vgadrv0.bin decode0.bin

ASK_OBJS =  init.bin tapi0.bin pioneer0.bin papi0.bin

#ASK_OBJS += vesadrv0.bin 

C_OBJS = winman0.bin pokon0.bin

SUB_OBJS = $(ANKFONT)

OBJS = $(BASE) $(SUB_OBJS) $(ASK_OBJS) $(C_OBJS) $(ARCH_OBJS)

INCPATH = ./include
LIBPATH = ./lib 
LIBS = guigui00.lib stdlib.lib $(SYSGGLIB)

POKON_STACK = 4k
POKON_MALLOC = 84k
POKON_FILE = 1024k
WINMAN_STACK = 4k
WINMAN_MALLOC = 624k
WINMAN_FILE = 768k

# prevent automatic deletion of an intermediate file
.SECONDARY : $(ASK_OBJS:.bin=.3as) $(ASK_OBJS:.bin=.asm) $(ASK_OBJS:.bin=.nas) $(ASK_OBJS:.bin=.i) $(ASK_OBJS:.bin=.obj) $(ASK_OBJS:.bin=.exe) $(BASE:.exe=.asm)

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
	echo ARCH = pcat>> depend.mak
	$(MAKE)

vmware : FORCE
	echo ARCH = pcat>> depend.mak
	echo VMWARE = y>> depend.mak
	$(MAKE)

bochs : FORCE
	echo ARCH = pcat>> depend.mak
	echo BOCHS = 13>> depend.mak
	$(MAKE)

bochs12 : FORCE
	echo ARCH = pcat>> depend.mak
	echo BOCHS = 12>> depend.mak
	$(MAKE)

towns : FORCE 
	echo ARCH = towns>> depend.mak
	echo TWSCRL1 = n>> depend.mak
	echo TWVSW = 1024>> depend.mak
	$(MAKE)

nec98 : FORCE 
	echo ARCH = nec98>> depend.mak
	$(MAKE)

FORCE:
	echo on > depend.mak
	echo #DESIGN = WIN9X#       Win9x style>> depend.mak
	echo #DESIGN = WIN31#       Win3.1 style>> depend.mak
	echo #DESIGN = TMENU#       T-OSASK>> depend.mak
	echo #DESIGN = CHO_OSASK#   Cho-Kanji style OSASK>> depend.mak
	echo #DESIGN = NEWSTYLE#    NWSOS style>> depend.mak

bootimg : $(BOOTIMAGE)

$(BOOTIMAGE) : $(TARGET)
	$(EXE2SYS) $< $@	

#$(BASE) : $(BASE:.exe=.nas)
#	$(NASK) $(NASK_FLAGS) $< $@ $*.lst 

pokon0.bim : pokon0.obj
	$(OBJ2BIM) -fixobj $< $(OBJ2BIM_ALIGN_FLAGS)
	$(OBJ2BIM) @$(DEFAULT_RULE_FILE) out:$@ stack:$(POKON_STACK) $<

pokon0.bin : pokon0.bim
	$(BIM2BIN) malloc:$(POKON_MALLOC) file:$(POKON_FILE) input:$< output:$@

winman0.bim : winman0.obj
	$(OBJ2BIM) -fixobj $< $(OBJ2BIM_ALIGN_FLAGS)
	$(OBJ2BIM) @$(DEFAULT_RULE_FILE) out:$@ stack:$(WINMAN_STACK) $<

winman0.bin : winman0.bim
	$(BIM2BIN) malloc:$(WINMAN_MALLOC) file:$(WINMAN_FILE) input:$< output:$@

%.i : %.ask .\depend.mak
	$(PREPROCESSOR) $(PREPROCESSOR_FLAGS) $<

%.3as : %.i
	$(ASKA) $< $@

%.nas : %.3as
	$(NASKCNV) $(NASKCNV_FLAGS) $< $@

%.bin : %.nas
	$(NASK) $(NASK_FLAGS) $< $@ $*.lst 

%.obj : %.c .\depend.mak
	$(CC) $< $(CFLAGS) -I$(INCPATH) -I$(SYSGGINC)

#%.bin : %.exe # for ASM objs
#	$(BIM2BIN) input:$< output:$@ $(BIM2BIN_FLAGS_ASK)
#	$(EXE2BIN) $*.exe $@ $(EXE2BIN_FLAGS_ASK)

clean:
	for %%i in ($(SUBDIRS)) do $(MAKE) -i -C %%i clean
	del $(TARGET) $(BOOTIMAGE)
	for %%i in ($(OBJS)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.i)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.3as)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.nas)) do del %%i
#	for %%i in ($(ASK_OBJS:.bin=.asm)) do del %%i
#	for %%i in ($(ASK_OBJS:.bin=.obj)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.lst)) do del %%i
#	for %%i in ($(ASK_OBJS:.bin=.map)) do del %%i
#	for %%i in ($(ASK_OBJS:.bin=.exe)) do del %%i
	for %%i in ($(C_OBJS:.bin=.obj)) do del %%i
	for %%i in ($(C_OBJS:.bin=.bim)) do del %%i

dist_clean:
	for %%i in ($(SUBDIRS)) do $(MAKE) -i -C %%i dist_clean
#	del $(TARGET)
	for %%i in ($(OBJS)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.i)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.3as)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.nas)) do del %%i
#	for %%i in ($(ASK_OBJS:.bin=.asm)) do del %%i
#	for %%i in ($(ASK_OBJS:.bin=.obj)) do del %%i
#	for %%i in ($(ASK_OBJS:.bin=.lst)) do del %%i
#	for %%i in ($(ASK_OBJS:.bin=.map)) do del %%i
#	for %%i in ($(ASK_OBJS:.bin=.exe)) do del %%i
	for %%i in ($(C_OBJS:.bin=.obj)) do del %%i
	for %%i in ($(C_OBJS:.bin=.exe)) do del %%i

# dependencies
pokon0.obj : pokon0.c pokon0.h