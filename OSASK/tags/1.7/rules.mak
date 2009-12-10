#compilers, linkers
#
#absolute path
# NOTE end with "\\"
#BIN_PATH = G:\OSASK\BIN\\

MAKE = $(BIN_PATH)make

ASKA = $(BIN_PATH)aska
PREPROCESSOR = $(BIN_PATH)lcc -EP
PREPROCESSOR_FLAGS =
ASM = $(BIN_PATH)ml
ASM_FLAGS = /c /Sa /Zm
MASMCNV = $(BIN_PATH)masmcnv2
MASMCNV_FLAGS = -l -s
LINK = $(BIN_PATH)link

LCCLIB = $(BIN_PATH)lcclib
CC = $(BIN_PATH)lcc
CFLAGS = -O

LINK32 = $(BIN_PATH)link32
LINK32_FLAGS = /ALIGN:16 /BASE:0 /DRIVER /ENTRY:main

EXE2BIN = $(BIN_PATH)exe2bin2
EXE2BIN_FLAGS_ASK = -n
EXE2BIN_FLAGS_C = -t -s

OBJ2BIM = $(BIN_PATH)obj2bim0
BIM2BIN = $(BIN_PATH)bim2bin0

OBJ2BIM_ALIGN_FLAGS = text_align:1 data_align:4 bss_align:4
DEFAULT_RULE_FILE = osask.rul

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

