MAKE = make

ASKA = aska
PREPROCESSOR = lcc -EP
MASMCNV = masmcnv2
MASMCNV_FLAGS = -l -s
ASM = ml
ASM_FLAGS = /Sa /Zm /c
LINK = link

CC = lcc
CFLAGS = -O
LINK32 = link32
LINK32_FLAGS = /ALIGN:16 /BASE:0 /DRIVER /ENTRY:main

EXE2BIN = exe2bin0
EXE2BIN_FLAGS_ASK = -n
EXE2BIN_FLAGS_C = 

OSALINK = osalink0
OSALINK_OPT = osalink0.opt

# make sub directory modules
ANKFONT = ankfont0.bin
BASE = base.exe
SYSGGLIB = sysgg00.lib
SYSGGINC = sgg00l01

SUBDIRS = ankfont base $(SYSGGINC)

TARGET = OSASK.EXE

ASK_OBJS =  tapi0.bin timerdrv.bin keymos0.bin fdcdrv0.bin \
    pioneer0.bin vgadrv0.bin decode0.bin

#ASK_OBJS += vesadrv0.bin 

C_OBJS = winman0.bin pokon0.bin

SUB_OBJS = $(BASE) $(ANKFONT)

OBJS = $(SUB_OBJS) $(ASK_OBJS) $(C_OBJS)

INCPATH = ./include
LIBPATH = ./lib 
LIBS = guigui00.lib stdlib.lib $(SYSGGLIB)

ALL :
	for %%i in ($(SUBDIRS)) do $(MAKE) -C %%i
	make $(TARGET)

$(TARGET) : $(OSALINK_OPT) $(OBJS)
	$(OSALINK)

$(OSALINK_OPT) : Makefile
	echo on > $(OSALINK_OPT)
	for %%i in ($(OBJS)) do echo %%i >> $(OSALINK_OPT)

$(BASE) :

$(ANKFONT) :

%.bin : %.ask Makefile
	$(PREPROCESSOR) $<
	$(ASKA) $*.i $*.3as
	$(MASMCNV) $(MASMCNV_FLAGS) $*.3as $*.asm
	$(ASM) $(ASM_FLAGS) /Fl$*.lst $*.asm
	$(LINK) $*,$*,$*,nul,nul
	$(EXE2BIN) $*.exe $@ $(EXE2BIN_FLAGS_ASK)

%.exe : %.c Makefile
	$(CC) $< $(CFLAGS) -I$(INCPATH) -I$(SYSGGINC)
	$(LINK32) $(LINK32_FLAGS) /LIBPATH:$(LIBPATH) $*.obj $(LIBS)

pokon0.bin : pokon0.exe Makefile
	$(EXE2BIN) $< $@ -s 20k

winman0.bin : winman0.exe Makefile
	$(EXE2BIN) $< $@ -s 24k

clean:
	for %%i in ($(SUBDIRS)) do $(MAKE) -i -C %%i clean
	del $(TARGET)
	del $(OSALINK_OPT)
	for %%i in ($(ASK_OBJS)) do del %%i
	for %%i in ($(C_OBJS)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.i)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.3as)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.asm)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.obj)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.lst)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.map)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.exe)) do del %%i
	for %%i in ($(C_OBJS:.bin=.obj)) do del %%i
	for %%i in ($(C_OBJS:.bin=.exe)) do del %%i

dist_clean:
	for %%i in ($(SUBDIRS)) do $(MAKE) -i -C %%i dist_clean
#	del $(TARGET)
#	del $(OSALINK_OPT)
	for %%i in ($(ASK_OBJS)) do del %%i
	for %%i in ($(C_OBJS)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.i)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.3as)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.asm)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.obj)) do del %%i
#	for %%i in ($(ASK_OBJS:.bin=.lst)) do del %%i
#	for %%i in ($(ASK_OBJS:.bin=.map)) do del %%i
	for %%i in ($(ASK_OBJS:.bin=.exe)) do del %%i
	for %%i in ($(C_OBJS:.bin=.obj)) do del %%i
	for %%i in ($(C_OBJS:.bin=.exe)) do del %%i
