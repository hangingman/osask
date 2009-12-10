#compilers, linkers
#
#absolute path
# NOTE end with "\\"
#BIN_PATH = G:\OSASK\BIN\\

MAKE = $(BIN_PATH)make

ASKA = $(BIN_PATH)aska
PREPROCESSOR = $(BIN_PATH)lcc -EP
PREPROCESSOR_FLAGS =
#ASM = $(BIN_PATH)ml
#ASM_FLAGS = /c /Sa /Zm
#MASMCNV = $(BIN_PATH)masmcnv2
#MASMCNV_FLAGS = -l -s

NASK = $(BIN_PATH)nask
NASK_FLAGS =
NASKCNV = $(BIN_PATH)naskcnv0
NASKCNV_FLAGS = -l -s

LINK = $(BIN_PATH)link

LCCLIB = $(BIN_PATH)lcclib
CC = $(BIN_PATH)lcc
CFLAGS = -O

LINK32 = $(BIN_PATH)link32
LINK32_FLAGS = /ALIGN:16 /BASE:0 /DRIVER /ENTRY:main

EXE2BIN = $(BIN_PATH)exe2bin2
EXE2BIN_FLAGS_ASK = -n
EXE2BIN_FLAGS_C = -t -s

OBJ2BIM = $(BIN_PATH)obj2bim3
BIM2BIN = $(BIN_PATH)bim2bin3
BIM2BIN_FLAGS_ASK = -exe512 -simple
BIM2BIN_FLAGS_TEK = -osacmp

OBJ2BIM_ALIGN_FLAGS = text_align:1 data_align:4 bss_align:4
DEFAULT_RULE_FILE = osask.rul

# architecture dependency
# for FM-TOWNS
ifeq ($(ARCH),towns)
#BASE_ASM = base.asm
BASE_NAS = base.nas
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
#BASE_ASM = base.asm
BASE_NAS = base.nas
PREPROCESSOR_FLAGS += -DPCAT
CFLAGS += -DPCAT
ifeq ($(VMWARE),y)
PREPROCESSOR_FLAGS += -DVMWARE
CFLAGS += -DVMWARE
endif
ifeq ($(BOCHS),13)
#BASE_ASM = base_bch.asm
BASE_NAS = base_bch.nas
PREPROCESSOR_FLAGS += -DBOCHS
CFLAGS += -DBOCHS
endif
ifeq ($(BOCHS),12)
#BASE_ASM = base_bch.asm
BASE_NAS = base_bch.nas
PREPROCESSOR_FLAGS += -DBOCHS -DNOHLT
CFLAGS += -DBOCHS -DNOHLT
endif
endif
# for NEC PC-98x1
ifeq ($(ARCH),nec98)
BASE_NAS = base.nas
PREPROCESSOR_FLAGS += -DNEC98
CFLAGS += -DNEC98
endif
# design $(DESIGN)
MMI = WIN9X
ifeq ($(DESIGN),WIN9X)
MMI = WIN9X
endif
ifeq ($(DESIGN),WIN31)
MMI = WIN31
endif
ifeq ($(DESIGN),TMENU)
MMI = TMENU
endif
ifeq ($(DESIGN),CHO_OSASK)
MMI = CHO_OSASK
endif
ifeq ($(DESIGN),NEWSTYLE)
MMI = NEWSTYLE
endif
PREPROCESSOR_FLAGS += -D$(MMI)
CFLAGS += -D$(MMI)
