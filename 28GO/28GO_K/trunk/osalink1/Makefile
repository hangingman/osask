include ../z_tools/command.txt

INCPATH = $(TOOLPATH)win32/

TARGET = osalink1
MODE = exe


LD = $(TOOLPATH)ld.exe

DEL = del



all : osalink1.exe


$(TARGET).$(MODE) : $(TARGET).obj
	$(LD) -s -Bdynamic --stack 0x1000000 -o $@ -Map $(TARGET).map $< $(INCPATH)w32clibc.lib $(INCPATH)golibc.lib $(INCPATH)libmingw.lib

#		18MB(apl) + 16MB(sys)




$(TARGET).obj : $(TARGET).nas
	$(NASK) $< $(TARGET).obj



$(TARGET).nas : $(TARGET).gas
	$(GAS2NASK) -a $< $@


$(TARGET).gas : $(TARGET).ca
	$(CC1) -I$(INCPATH) -Dmain=main0 -Os -Wall -quiet -o $@ $<



$(TARGET).ca : $(TARGET).c
	$(SJISCONV) in:$< out:$@





clean :
	-$(DEL) *.ca
	-$(DEL) *.gas
	-$(DEL) *.nas
	-$(DEL) *.map
	-$(DEL) *.obj
	-$(DEL) *.exe
