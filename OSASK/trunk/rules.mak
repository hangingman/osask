#compilers, linkers
MAKE = make

ASKA = aska
PREPROCESSOR = lcc -EP
PREPROCESSOR_FLAGS =
ASM = ml
ASM_FLAGS = /c /Sa /Zm
MASMCNV = masmcnv2
MASMCNV_FLAGS = -l -s
LINK = link

CC = lcc
CFLAGS = -O

LINK32 = link32
LINK32_FLAGS = /ALIGN:16 /BASE:0 /DRIVER /ENTRY:main

EXE2BIN = exe2bin1
EXE2BIN_FLAGS_ASK = -n
EXE2BIN_FLAGS_C = -t -s

# architecture dependency
ifeq ($(ARCH),towns)
PREPROCESSOR_FLAGS += -DTOWNS
CFLAGS += -DTOWNS
endif
ifeq ($(ARCH),pcat)
PREPROCESSOR_FLAGS += -DPCAT
CFLAGS += -DPCAT
endif
ifeq ($(VMWARE),y)
PREPROCESSOR_FLAGS += -DVMWARE
CFLAGS += -DVMWARE
endif

