MAKE = make

ASKA = aska
PREPROCESSOR = lcc -EP
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

OSALINK = osalink0
OSALINK_OPT = OSALINK0.OPT


APP = base
TARGET = $(APP).exe
SRC = $(APP).3as
SRCS =  basehead.asm boot.asm init.3as basefoot.asm

DISTDIR = ..

# prevent automatic deletion of an intermediate file
.SECONDARY : $(SRC:.ask=.3as) $(SRC:.ask=.asm)

$(TARGET) : $(SRC) Makefile
	$(MASMCNV) $(MASMCNV_FLAGS) $< $(TARGET:.exe=.asm)
	$(ASM) $(ASM_FLAGS) /Fl$(APP).lst  $(TARGET:.exe=.asm)
	$(LINK) $(APP),$(APP),$(APP),nul,nul
	copy $@ $(DISTDIR)

$(SRC) : $(SRCS) Makefile
	echo on > $(SRC)
	for %%i in ($(SRCS)) do type %%i >> $(SRC)

init.3as : init.ask Makefile
	$(ASKA) $< $@

clean:
	for %%i in ($(DISTDIR)\$(TARGET) $(TARGET)) do del %%i
	for %%i in (init.3as $(TARGET:.exe=.asm) $(TARGET:.exe=.obj)) do del %%i
	for %%i in ($(SRC) *.lst *.map) do del %%i

dist_clean:
#	for %%i in (*.3as *.obj *.lst *.map *.exe) do del %%i
	for %%i in (*.3as *.obj *.exe) do del %%i
