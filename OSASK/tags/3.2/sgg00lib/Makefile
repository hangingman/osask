include ..\depend.mak
include ..\rules.mak

#ASM_FLAGS = /c /coff

NASKCNV_FLAGS += -w # for WCOFF

TARGET = sysgg00.lib
OPT = $(TARGET:.lib=.opt)

ASK_OBJS = execcmd.obj execcmd0.obj execcmd1.obj
C_OBJS = fwinman.obj fpokon.obj fdebug.obj

OBJS = $(ASK_OBJS) $(C_OBJS)

INCPATH = ../include
SYSGGINC = ../include
DIST_DIR = ..\lib

# prevent automatic deletion of an intermediate file
.SECONDARY : $(ASK_OBJS:.obj=.3as) $(ASK_OBJS:.obj=.asm) $(ASK_OBJS:.obj=.nas) $(ASK_OBJS:.obj=.i) $(ASK_OBJS:.obj=.obj) $(ASK_OBJS:.obj=.exe) $(C_OBJS:.obj=.s) $(C_OBJS:.obj=.nas)

ALL : $(TARGET)

$(TARGET) : $(OBJS) $(OPT)
#	$(LCCLIB) -OUT:$@ @$(OPT)
	$(GOLIB) out:$@ $(OBJS)
	copy $@ $(DIST_DIR)

$(OPT) : makefile
	echo on > $(OPT)
	for %%i in ($(OBJS)) do echo %%i >> $(OPT)

%.s : %.c ..\depend.mak
	$(CC) $(CFLAGS) -I$(INCPATH) -I$(SYSGGINC) -o $@ $<

%.nas : %.s ..\depend.mak
	$(GAS2NASK) $(GAS2NASK_FLAGS) $< $@

%.obj : %.nas ..\depend.mak
	$(NASK) $(NASK_FLAGS) $< $@ $*.lst 

%.i : %.ask makefile ..\depend.mak coffhead.ask
	$(PREPROCESSOR) $(PREPROCESSOR_FLAGS) -o $@ $<

%.3as : %.i
	$(ASKA) $< $@

%.nas : %.3as
	$(NASKCNV) $(NASKCNV_FLAGS) $< $@

%.obj : %.nas
	$(NASK) $(NASK_FLAGS) $< $@ $*.lst 

#%.obj : %.asm makefile
#	$(ASM) $(ASM_FLAGS) $<
#	$(OBJ2BIM) -fixobj $@ text_align:1 data_align:4 bss_align:4

clean:
	del $(DIST_DIR)\$(TARGET)
	del $(TARGET)
	for %%i in ($(OBJS)) do del %%i
	for %%i in ($(C_OBJS:.obj=.s)) do del %%i
	for %%i in ($(C_OBJS:.obj=.nas)) do del %%i
	for %%i in ($(C_OBJS:.obj=.lst)) do del %%i
	for %%i in ($(ASK_OBJS:.obj=.i)) do del %%i
	for %%i in ($(ASK_OBJS:.obj=.3as)) do del %%i
	for %%i in ($(ASK_OBJS:.obj=.nas)) do del %%i
#	for %%i in ($(ASK_OBJS:.obj=.asm)) do del %%i
#	for %%i in ($(ASK_OBJS:.obj=.obj)) do del %%i
	for %%i in ($(ASK_OBJS:.obj=.lst)) do del %%i
#	for %%i in ($(ASK_OBJS:.obj=.map)) do del %%i
#	del *.lst

