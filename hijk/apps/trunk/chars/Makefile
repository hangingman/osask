include ../z_tools/command.txt

TARGET = chars
MODE   = g01
#OBJS   = $(TARGET).obj

#INCPATH    = ../z_tools/guigui01/
#include ../com_mak.txt

all:
	$(NASK) chars.nas chars.g01 chars.lst


clean:
	-$(DELE) $(TARGET).map
	-$(DELE) $(TARGET).bim
	-$(DELE) $(TARGET).rlm
	-$(DELE) $(TARGET).lst
	-$(DELE) $(TARGET).g01

install:
	$(COPY) $(TARGET).$(MODE) ..\ok\

