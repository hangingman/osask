include ../z_tools/command.txt

TARGET = hello
MODE   = g01
OBJS   = $(TARGET).obj

INCPATH    = $(TOOLPATH)guigui01/


ALL:
	$(MAKE)	$(TARGET).$(MODE)

%.g01 : %.nas
	$(NASK) in:$*.nas out:$*.g01 lst:$*.lst

clean:

	-$(DELE) $(TARGET).bim
	-$(DELE) $(TARGET).rlm
	-$(DELE) $(TARGET).ias
	-$(DELE) $(TARGET).lst
	-$(DELE) $(TARGET).g01

install:
	$(COPY) $(TARGET).$(MODE) ..\ok\