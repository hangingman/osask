include depend.mak
include rules.mak

OSALINK = osalink1
OSALINK_OPT = osalink1.opt

# make sub directory modules
ANKFONT = ankfont0.bin
BASE = base.exe
BOOT = boot32.bin
SYSGGLIB = sysgg00.lib
SYSGGINC = sgg00lib

SUBDIRS = ankfont0 $(SYSGGINC)
SUBDIRS += $(ARCH)

TARGET = OSASK.EXE
BOOTIMAGE = $(TARGET:EXE=SYS)
EXE2SYS = imgtol

ARCH_OBJS = timerdrv.bin keymos0.bin fdcdrv0.bin vgadrv0.bin decode0.bin

ARCH_OBJS += $(VESA_OBJS)

ASK_OBJS =  init.bin tapi0.bin pioneer0.bin papi0.bin
C_OBJS = winman0.bin pokon0.bin
WINMAN_C_OBJS = kjpegls2.obj
WINMAN_NASK_OBJS = betaclip.obj bmp2beta.obj
WINMAN_OBJS = winman0.obj $(WINMAN_C_OBJS) $(WINMAN_NASK_OBJS)

SUB_OBJS = $(ANKFONT)

OBJS = $(SUB_OBJS) $(ASK_OBJS) $(C_OBJS) $(ARCH_OBJS)
LINK_OBJS = $(BASE) $(BOOT) $(OBJS:.bin=.tek)

INCPATH = ./include
LIBPATH = ./lib 
LIBS = guigui00.lib stdlib.lib $(SYSGGLIB)

POKON_STACK = 4k
POKON_MALLOC = 88k
POKON_FILE = 4096k
WINMAN_STACK = 8k
WINMAN_MALLOC = 4208k
WINMAN_FILE = 4096k

# prevent automatic deletion of an intermediate file
.SECONDARY : $(ASK_OBJS) $(ASK_OBJS:.bin=.3as) $(ASK_OBJS:.bin=.asm) \
	$(ASK_OBJS:.bin=.nas) $(ASK_OBJS:.bin=.i) $(ASK_OBJS:.bin=.obj) \
	$(ASK_OBJS:.bin=.exe) $(BASE:.exe=.asm) $(C_OBJS:.bin=.s) \
	$(C_OBJS:.bin=.nas) $(BOOT:.bin=.i) $(BOOT:.bin=.3as) \
	$(BOOT:.bin=.nas) $(WINMAN_C_OBJS:.obj=.s) $(WINMAN_C_OBJS:.obj=.nas) 


ALL :
	for %%i in ($(SUBDIRS)) do $(MAKE) -C %%i
	$(MAKE) $(TARGET)

$(TARGET) : $(OSALINK_OPT) $(LINK_OBJS)
	$(OSALINK)

$(OSALINK_OPT) : makefile depend.mak
	echo on > $(OSALINK_OPT)
	for %%i in ($(LINK_OBJS)) do echo %%i >> $(OSALINK_OPT)

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
	echo VMODE = y>> depend.mak
	echo CLGD543X = y>> depend.mak
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
	$(EXE2SYS) s $< $@ 2048

#$(BASE) : $(BASE:.exe=.nas)
#	$(NASK) $(NASK_FLAGS) $< $@ $*.lst 

pokon0.bim : pokon0.obj
	$(OBJ2BIM) @$(DEFAULT_RULE_FILE) out:$@ stack:$(POKON_STACK) $<

pokon0.bin : pokon0.bim
	$(BIM2BIN) malloc:$(POKON_MALLOC) file:$(POKON_FILE) input:$< output:$@

winman0.bim : $(WINMAN_OBJS)
	$(OBJ2BIM) @$(DEFAULT_RULE_FILE) out:$@ stack:$(WINMAN_STACK) $(WINMAN_OBJS)

winman0.bin : winman0.bim
	$(BIM2BIN) malloc:$(WINMAN_MALLOC) file:$(WINMAN_FILE) input:$< output:$@

%.i : %.ask .\depend.mak
	$(PREPROCESSOR) $(PREPROCESSOR_FLAGS) -o $*.i $<

%.3as : %.i
	$(ASKA) $< $@

%.nas : %.3as
	$(NASKCNV) $(NASKCNV_FLAGS) $< $@

%.bin : %.nas
	$(NASK) $(NASK_FLAGS) $< $@ $*.lst 

%.tek : %.bin
	$(BIM2BIN) $(BIM2BIN_FLAGS_TEK) input:$< output:$@

%.s : %.c .\depend.mak
	$(CC) $(CFLAGS) -I$(INCPATH) -I$(SYSGGINC) -o $@ $<

%.nas : %.s .\depend.mak
	$(GAS2NASK) $(GAS2NASK_FLAGS) $< $@

%.obj : %.nas .\depend.mak
	$(NASK) $(NASK_FLAGS) $< $@ $*.lst 

#%.obj : %.s .\depend.mak
#	$(ASM) $(ASM_FLAGS) -o $@ $<

#%.obj : %.c .\depend.mak
#	$(CC) $< $(CFLAGS) -I$(INCPATH) -I$(SYSGGINC)

#%.bin : %.exe # for ASM objs
#	$(BIM2BIN) input:$< output:$@ $(BIM2BIN_FLAGS_ASK)
#	$(EXE2BIN) $*.exe $@ $(EXE2BIN_FLAGS_ASK)

clean:
	for %%i in ($(SUBDIRS)) do $(MAKE) -i -C %%i clean
	del $(TARGET) $(BOOTIMAGE) $(BOOT:.bin=.o)
	for %%i in ($(OBJS)) do del %%i
	for %%i in ($(LINK_OBJS)) do del %%i
	for %%i in ($(BOOT:.bin=.lst)) do del %%i
	for %%i in ($(BOOT:.bin=.i)) do del %%i
	for %%i in ($(BOOT:.bin=.3as)) do del %%i
	for %%i in ($(BOOT:.bin=.nas)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.i)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.3as)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.nas)) do del %%i
#	for %%i in ($(ASK_OBJS:.bin=.asm)) do del %%i
#	for %%i in ($(ASK_OBJS:.bin=.obj)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.lst)) do del %%i
#	for %%i in ($(ASK_OBJS:.bin=.map)) do del %%i
#	for %%i in ($(ASK_OBJS:.bin=.exe)) do del %%i
	for %%i in ($(C_OBJS:.bin=.s)) do del %%i
	for %%i in ($(C_OBJS:.bin=.nas)) do del %%i
	for %%i in ($(C_OBJS:.bin=.obj)) do del %%i
	for %%i in ($(C_OBJS:.bin=.bim)) do del %%i
	for %%i in ($(C_OBJS:.bin=.lst)) do del %%i
	for %%i in ($(WINMAN_C_OBJS)) do del %%i
	for %%i in ($(WINMAN_C_OBJS:.obj=.s)) do del %%i
	for %%i in ($(WINMAN_C_OBJS:.obj=.nas)) do del %%i
	for %%i in ($(WINMAN_C_OBJS:.obj=.lst)) do del %%i
	for %%i in ($(WINMAN_NASK_OBJS)) do del %%i
	for %%i in ($(WINMAN_NASK_OBJS:.obj=.lst)) do del %%i

dist_clean:
	for %%i in ($(SUBDIRS)) do $(MAKE) -i -C %%i dist_clean
#	del $(TARGET) $(BOOTIMAGE)
	for %%i in ($(OBJS)) do del %%i
	for %%i in ($(LINK_OBJS)) do del %%i
	for %%i in ($(BOOT:.bin=.lst)) do del %%i
	for %%i in ($(BOOT:.bin=.i)) do del %%i
	for %%i in ($(BOOT:.bin=.3as)) do del %%i
	for %%i in ($(BOOT:.bin=.nas)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.i)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.3as)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.nas)) do del %%i
#	for %%i in ($(ASK_OBJS:.bin=.asm)) do del %%i
#	for %%i in ($(ASK_OBJS:.bin=.obj)) do del %%i
#	for %%i in ($(ASK_OBJS:.bin=.lst)) do del %%i
#	for %%i in ($(ASK_OBJS:.bin=.map)) do del %%i
#	for %%i in ($(ASK_OBJS:.bin=.exe)) do del %%i
	for %%i in ($(C_OBJS:.bin=.s)) do del %%i
	for %%i in ($(C_OBJS:.bin=.nas)) do del %%i
	for %%i in ($(C_OBJS:.bin=.obj)) do del %%i
	for %%i in ($(C_OBJS:.bin=.bim)) do del %%i
	for %%i in ($(WINMAN_C_OBJS)) do del %%i
	for %%i in ($(WINMAN_C_OBJS:.obj=.s)) do del %%i
	for %%i in ($(WINMAN_C_OBJS:.obj=.nas)) do del %%i
	for %%i in ($(WINMAN_C_OBJS:.obj=.lst)) do del %%i
	for %%i in ($(WINMAN_NASK_OBJS)) do del %%i
	for %%i in ($(WINMAN_NASK_OBJS:.obj=.lst)) do del %%i

# dependencies
pokon0.obj : pokon0.c pokon0.h