MAKE = make

ASKA = aska
ASM = ml
ASM_FLAGS = /c /Sa /Zm
MASMCNV = masmcnv2
MASMCNV_FLAGS = -l -s
LINK = link

CC = lcc
CFLAGS = -O -I.

LINK32 = link32
LINK32_OPT = /ALIGN:16 /BASE:0 /DRIVER /ENTRY:main

EXE2BIN = exe2bin0
EXE2BIN_FLAGS_ASK = -n
EXE2BIN_FLAGS_C = -t -s

TARGET = ankfont0.bin
SRC = fonttext.txt
DISTDIR = ..

# prevent automatic deletion of an intermediate file
.SECONDARY : $(SRC:.ask=.3as) $(SRC:.ask=.asm)

$(DISTDIR)\$(TARGET) : $(TARGET)
	copy $(TARGET) $(DISTDIR)

$(TARGET) : $(SRC) Makefile
	txt2bin.exe

clean:
	del $(TARGET)
