include ../z_tools/command.txt

TARGET = testkey1
MODE   = g01
OBJS   = $(TARGET).obj

INCPATH    = $(TOOLPATH)guigui01/

include $(TOOLPATH)com_mak.txt


install:
	$(COPY) $(TARGET).$(MODE) ..\ok\



clean:
	-$(DELE) *.map
	-$(DELE) *.nh5
	-$(DELE) *.nho
	-$(DELE) *.obj
	-$(DELE) *.org
	-$(DELE) *.rlm
	-$(DELE) *.bim
	-$(DELE) $(TARGET).$(MODE)

