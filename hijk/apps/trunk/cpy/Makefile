include ../z_tools/command.txt

TARGET = cpy
MODE   = g01
#OBJS   = $(TARGET).obj

#INCPATH    = ../z_tools/guigui01/


all:
	$(MAKE) $(TARGET).$(MODE)



%.g01 : %.nas
	$(NASK) in:cpy.nas out:$*.g01 lst:$*.lst

%.nas : %.3as
	$(NASKCNV) -l -s in:$*.3as out:$*.nas

%.3as : %.ias
	$(ASKA) $*.ias $*.3as

%.ias : %.ask
	$(CPP0) -P -I. -o $*.ias $*.ask



clean:
	-$(DELE) $(TARGET).map
	-$(DELE) $(TARGET).bim
	-$(DELE) $(TARGET).rlm
	-$(DELE) $(TARGET).ias
	-$(DELE) $(TARGET).lst
	-$(DELE) $(TARGET).g01


install:
	$(COPY) $(TARGET).$(MODE) ..\ok\
