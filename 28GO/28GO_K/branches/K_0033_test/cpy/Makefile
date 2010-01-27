include ../z_tools/command.txt

TARGET = cpy
MODE   = g01
#OBJS   = $(TARGET).obj

#INCPATH    = ../z_tools/guigui01/
#include ../z_tools/com_mak.txt

ALL :
	$(MAKE)	$(TARGET).$(MODE)

$(TARGET).$(MODE) : $(TARGET).nas
	$(NASK) in:$< out:$(TARGET).$(MODE) lst:$(TARGET).lst

$(TARGET).nas : $(TARGET).3as
	$(NASKCNV) -l -s in:$< out:$@

$(TARGET).3as : $(TARGET).ias
	$(ASKA) $< $@

$(TARGET).ias : $(TARGET).ask
	$(CPP0) -P -I. -o $@ $<


clean:
	del *.3as
	del *.ias
	del *.nas
	del *.lst


