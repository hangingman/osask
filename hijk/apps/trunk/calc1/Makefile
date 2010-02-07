include ../z_tools/command.txt

TARGET = calc1
MODE   = g01
#OBJS   = $(TARGET).obj

#INCPATH    = ../z_tools/guigui01/





ALL:
	$(MAKE)	$(TARGET).$(MODE)

%.g01 : %.nas
	$(NASK) in:$*.nas out:$*.g01 lst:$*.lst

%.nas : %.3gs
	$(NASKCNV) -l -s in:$*.3gs out:$*.nas

%.3gs : %.ias
	$(ASKA) $*.ias $*.3gs

%.ias : %.ask
	$(CPP0) -P -I. -o $*.ias $*.ask


clean:
	-$(DELE) $(TARGET).ias
	-$(DELE) $(TARGET).3gs
	-$(DELE) $(TARGET).nas
	-$(DELE) $(TARGET).obj
	-$(DELE) $(TARGET).lst
	-$(DELE) $(TARGET).$(MODE)


install:
	$(COPY) $(TARGET).$(MODE) ..\ok\





