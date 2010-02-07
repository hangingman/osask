include ../z_tools/command.txt

TARGET = calendar
MODE   = g01
#OBJS   = $(TARGET).obj

#INCPATH    = ../z_tools/guigui01/
#include ../com_mak.txt



ALL:
	$(MAKE)	$(TARGET).$(MODE)

%.g01 : %.nh5
	$(TEKMIN1) -efg01-noadc $*.nh5 $*.g01



%.nh5 : %.nho
	$(BIM2BIN) -osacmp in:$*.nho out:$*.nh5


%.nho : %.org
	$(TEKMIN0) $*.org $*.nho



%.org : %.bim
	$(BIM2G01) $*.bim $*.org $*.rlm


%.bim : %.obj
	$(OBJ2BIM) rul:$(TOOLPATH)guigui01/guigui01.rul out:$*.bim stack:0 map:$*.map rlm:$*.rlm in:$*.obj


%.obj : %.nas
	$(NASK) in:$*.nas out:$*.obj lst:$*.lst




clean:
	-$(DELE) $(TARGET).lst
	-$(DELE) $(TARGET).bim
	-$(DELE) $(TARGET).map
	-$(DELE) $(TARGET).rlm
	-$(DELE) $(TARGET).g01


install:
	$(COPY) $(TARGET).$(MODE) ..\ok\
