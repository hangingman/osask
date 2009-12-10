include ..\depend.mak
include ..\rules.mak

APP = base
TARGET = $(APP).exe
SRC = $(APP).3as
SRCS =  basehead.asm boot.asm init.3as basefoot.asm

DISTDIR = ..

# prevent automatic deletion of an intermediate file
.SECONDARY : $(SRC:.ask=.3as) $(SRC:.ask=.asm)

$(DISTDIR)\$(TARGET) : $(TARGET)
	copy $(TARGET) $(DISTDIR)

$(TARGET) : $(SRC) Makefile
	$(MASMCNV) $(MASMCNV_FLAGS) $< $(TARGET:.exe=.asm)
	$(ASM) $(ASM_FLAGS) /Fl$(APP).lst  $(TARGET:.exe=.asm)
	$(LINK) $(APP),$(APP),$(APP),nul,nul

$(SRC) : $(SRCS) Makefile
	echo on > $(SRC)
	for %%i in ($(SRCS)) do type %%i >> $(SRC)

init.3as : init.ask Makefile ..\depend.mak
	$(PREPROCESSOR) $(PREPROCESSOR_FLAGS) $<
	$(ASKA) init.i $@

clean:
	for %%i in ($(TARGET)) do del %%i
	for %%i in (init.i init.3as $(TARGET:.exe=.asm) $(TARGET:.exe=.obj)) do del %%i
	for %%i in ($(SRC) *.lst *.map) do del %%i

dist_clean:
#	for %%i in (*.i *.3as *.obj *.lst *.map *.exe) do del %%i
	for %%i in (*.i *.3as *.obj *.exe) do del %%i
