include ../z_tools/command.txt

TARGET = hexdump
MODE   = g01
#OBJS   = $(TARGET).obj

#INCPATH    = $(TOOLPATH)guigui01/
INCPATH    = .


ALL:
	$(MAKE)	$(TARGET).$(MODE)


%.g01 : %.nh5
	$(TEKMIN1) -efg01-noadc $*.nh5 $*.g01

%.nh5 : %.nho
	$(BIM2BIN) -osacmp in:$*.nho out:$*.nh5

%.nho : %.rjc
	$(TEKMIN0) $*.rjc $*.nho


%.rjc : %.org
	$(EFG) $(TOOLPATH)rjcg01 $*.org $*.rjc

%.org : %.nas
	$(NASK) in:$*.nas out:$*.org lst:$*.lst

%.nas : %.3as
	$(EFG) $(TOOLPATH)naskcnv0 -l -s in:$*.3as out:$*.nas

%.3as : %.ias
	$(ASKA) $*.ias $*.3as

%.ias : %.ask
	$(CPP0) -o $*.ias $*.ask




clean:

	-$(DELE) $(TARGET).bim
	-$(DELE) $(TARGET).rlm
	-$(DELE) $(TARGET).ias
	-$(DELE) $(TARGET).3as
	-$(DELE) $(TARGET).nas
	-$(DELE) $(TARGET).org
	-$(DELE) $(TARGET).rjc
	-$(DELE) $(TARGET).lst
	-$(DELE) $(TARGET).$(MODE)


install:
	$(COPY) $(TARGET).$(MODE) ..\ok\