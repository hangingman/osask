include ../z_tools/command.txt

TARGET = calc
MODE   = g01
OBJS   = $(TARGET).obj

INCPATH    = $(TOOLPATH)guigui01/

include $(TOOLPATH)com_mak.txt





install:
	$(COPY) $(TARGET).$(MODE) ..\ok\


clean:
	-$(DELE) *.bim
	-$(DELE) *.map
	-$(DELE) *.nh5
	-$(DELE) *.nho
	-$(DELE) *.obj
	-$(DELE) *.org
	-$(DELE) *.rlm
	-$(DELE) $(TARGET).$(MODE)

