MAKE = make

ASM = ml
ASM_FLAGS = /c /coff

CC = lcc

TARGET = sysgg00.lib
OPT = $(TARGET:.lib=.opt)

OBJS = execcmd.obj fwinman.obj fpokon.obj fdebug.obj \
		execcmd0.obj execcmd1.obj

INCPATH = ../include
DIST_DIR = ..\lib

ALL :
	make $(TARGET)

$(TARGET) : $(OBJS) $(OPT)
	lcclib -OUT:$@ @$(OPT)
	copy $@ $(DIST_DIR)

$(OPT) : Makefile
	echo on > $(OPT)
	for %%i in ($(OBJS)) do echo %%i >> $(OPT)

%.obj : %.c Makefile
	$(CC) $< -I$(INCPATH) -O
	obj2bim0 -fixobj $@ text_align:1 data_align:4 bss_align:4

%.obj : %.asm Makefile
	$(ASM) $(ASM_FLAGS) $<
	obj2bim0 -fixobj $@ text_align:1 data_align:4 bss_align:4

clean:
	del $(DIST_DIR)\$(TARGET)
	del $(TARGET)
	for %%i in ($(OBJS)) do del %%i
#	del *.lst
