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

OBJ2BIM = $(BIN_PATH)obj2bim1
BIM2BIN = $(BIN_PATH)bim2bin1
BIM2BIN_FLAGS_ASK = -exe512 -simple

OBJ2BIM_ALIGN_FLAGS = text_align:1 data_align:4 bss_align:4
DEFAULT_RULE_FILE = osask.rul

# architecture dependency
# for FM-TOWNS
ifeq ($(ARCH),towns)
BASE_ASM = base.asm
PREPROCESSOR_FLAGS += -DTOWNS
CFLAGS += -DTOWNS
# enable 1-pixel scroll
ifeq ($(TWSCRL1),y)
PREPROCESSOR_FLAGS += -DTWSCRL1
CFLAGS += -DTWSCRL1
endif
# set "TOWNS Vertial Screen Width"
ifneq ($(TWVSW),)
PREPROCESSOR_FLAGS += -DTWVSW=$(TWVSW)
CFLAGS += -DTWVSW=$(TWVSW)
else
PREPROCESSOR_FLAGS += -DTWVSW=1024
CFLAGS += -DTWVSW=1024
endif
endif
# for PC/AT
ifeq ($(ARCH),pcat)
BASE_ASM = base.asm
PREPROCESSOR_FLAGS += -DPCAT
CFLAGS += -DPCAT
ifeq ($(VMWARE),y)
PREPROCESSOR_FLAGS += -DVMWARE
CFLAGS += -DVMWARE
endif
ifeq ($(BOCHS),13)
BASE_ASM = base_bch.asm
PREPROCESSOR_FLAGS += -DBOCHS
CFLAGS += -DBOCHS
endif
ifeq ($(BOCHS),12)
BASE_ASM = base_bch.asm
PREPROCESSOR_FLAGS += -DBOCHS -DNOHLT
CFLAGS += -DBOCHS -DNOHLT
endif
endif
# design $(DESIGN)
ifeq ($(WIN9X),y)
DESIGN = WIN9X
endif
ifeq ($(TMENU),y)
DESIGN = TMENU
endif
ifeq ($(CHO_OSASK),y)
DESIGN = CHO_OSASK
endif
ifeq ($(NEWSTYLE),y)
DESIGN = NEWSTYLE
endif
ifeq ($(DESIGN),)
DESIGN = WIN9X
endif
PREPROCESSOR_FLAGS += -D$(DESIGN)
CFLAGS += -D$(DESIGN)
