MAKE = make

ASKA = aska
PREPROCESSOR = lcc -EP
ASM = ml
ASM_FLAGS = /c /coff
MASMCNV = masmcnv2
MASMCNV_FLAGS = -l -s
LINK = link

CC = lcc
CFLAGS = -O -I.
AR = lcclib

LINK32 = link32
LINK32_OPT = /ALIGN:16 /BASE:0 /DRIVER /ENTRY:main 

EXE2BIN = exe2bin0
EXE2BIN_FLAGS_ASM = -n
EXE2BIN_FLAGS_C = -t -s

TARGET = sysgg00.lib
INCFILE = $(TARGET:.lib=.h)
ASM_SRC = execcmd.asm 
C_SRCS = init.c fwinman.c fpokon.c fdebug.c
#C_SRCS += waitsig.c waitsigt.c openwin.c opentext.c putstr.c opensigb.c

OBJS = $(ASM_SRC:.asm=.obj) $(C_SRCS:.c=.obj)

OPT = $(TARGET:.lib=.opt)

DIST_DIR = ..\lib

# prevent automatic deletion of an intermediate file
.SECONDARY : $(SRC:.ask=.3as) $(SRC:.ask=.asm)

$(TARGET) : $(OBJS) $(OPT)
	$(AR) -OUT:$(TARGET) @$(OPT)
	copy $@ $(DIST_DIR)

$(OPT) : Makefile
	echo on > $(OPT)
	for %%i in ($(OBJS)) do echo %%i >> $(OPT)

%.obj : %.asm Makefile
	$(ASM) $(ASM_FLAGS) /Fl$*.lst $<

%.obj : %.c Makefile
	$(CC) $< $(CFLAGS)

%.c : $(INCFILE) Makefile

clean:
	del $(DIST_DIR)\$(TARGET)
	del $(TARGET)
	for %%i in ($(OBJS)) do del %%i
	del *.lst

dist_clean:
	for %%i in ($(OBJS)) do del %%i
	del *.lst
