include ..\depend.mak
include ..\rules.mak

ASM_FLAGS = /c /coff

TARGET = sysgg00.lib
OPT = $(TARGET:.lib=.opt)

OBJS = execcmd.obj fwinman.obj fpokon.obj fdebug.obj \
		execcmd0.obj execcmd1.obj

INCPATH = ../include
DIST_DIR = ..\lib

ALL : $(TARGET)

$(TARGET) : $(OBJS) $(OPT)
	$(LCCLIB) -OUT:$@ @$(OPT)
	copy $@ $(DIST_DIR)

$(OPT) : makefile
	echo on > $(OPT)
	for %%i in ($(OBJS)) do echo %%i >> $(OPT)

%.obj : %.c makefile
	$(CC) $< -I$(INCPATH) -O
	$(OBJ2BIM) -fixobj $@ text_align:1 data_align:4 bss_align:4

%.obj : %.asm makefile
	$(ASM) $(ASM_FLAGS) $<
	$(OBJ2BIM) -fixobj $@ text_align:1 data_align:4 bss_align:4

clean:
	del $(DIST_DIR)\$(TARGET)
	del $(TARGET)
	for %%i in ($(OBJS)) do del %%i
#	del *.lst
