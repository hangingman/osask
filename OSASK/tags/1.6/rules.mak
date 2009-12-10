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

EXE2BIN = exe2bin2
EXE2BIN_FLAGS_ASK = -n
EXE2BIN_FLAGS_C = -t -s

OBJ2BIM = obj2bim0
BIM2BIN = bim2bin0

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

