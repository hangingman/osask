TARGET     = w32clibc
STACKSIZE  =
MALLOCSIZE =
MMAREA     =
OBJS       = startup.obj fputs.obj puts.obj fprintf.obj printf.obj vprintf.obj \
			 vfprintf.obj fwrite.obj stdin.obj malloc.obj free.obj fclose.obj \
			 fflush.obj fopen.obj fread.obj fseek.obj ftell.obj remove.obj \
			 fputc.obj clearerr.obj rewind.obj fgetc.obj feof.obj fgets.obj \
			 gets.obj ungetc.obj exit.obj system.obj

# 以上5つはソースごとに書き換える
# OBJSを書き換えると分割コンパイル対応
 
TOOLPATH =
INCPATH  = ..
RULEFILE = f:/osask/inclib/guigui00.rul
DLLRULE  = $(TOOLPATH)dllrule0.rul
MAKE     = $(TOOLPATH)make -r
SJISCONV = $(TOOLPATH)sjisconv -s
CC1      = $(TOOLPATH)cc1 -I$(INCPATH) -Os -quiet -W -Wall -Werror
GAS2NASK = $(TOOLPATH)gas2nask -a
NASK     = $(TOOLPATH)nask
OBJ2BIM  = $(TOOLPATH)obj2bim3
BIM2BIN  = $(TOOLPATH)bim2bin3
CPP0     = $(TOOLPATH)cpp0 -P -I$(INCPATH)
ASKA     = $(TOOLPATH)aska
NASKCNV  = $(TOOLPATH)naskcnv0 -l -s -w
GOLIB    = $(TOOLPATH)golib00w
DELE     = del

# 以上の項目はあなたのディレクトリ構成にあわせて書き換える

ALL :
	$(MAKE) $(TARGET).lib

%.ca : %.c Makefile
	$(SJISCONV) $*.c $*.ca

%.gas : %.ca Makefile
	$(CC1) -o $*.gas $*.ca

%.nas : %.gas Makefile
	$(GAS2NASK) $*.gas $*.nas

%.obj : %.nas Makefile
	$(NASK) $*.nas $*.obj

%.ias : %.ask Makefile
	$(CPP0) -o $*.ias $*.ask

%.3as : %.ias Makefile
	$(ASKA) $*.ias $*.3as

%.nas : %.3as Makefile
	$(NASKCNV) $*.3as $*.nas

%.lst : %.nas Makefile
	$(NASK) $*.nas $*.obj $*.lst

$(TARGET).bim : $(OBJS) Makefile
	$(OBJ2BIM) @$(RULEFILE) out:$(TARGET).bim stack:$(STACKSIZE) map:$(TARGET).map $(OBJS)

%.bin : %.bim Makefile
	$(BIM2BIN) in:$*.bim out:$*.org malloc:$(MALLOCSIZE) mmarea:$(MMAREA)
	$(BIM2BIN) -osacmp -tek0 in:$*.org out:$*.bin

$(TARGET).lib : $(OBJS) Makefile
	$(GOLIB) out:$@ $(OBJS)

$(TARGET).dll : $(TARGET).bim Makefile
	$(OBJ2BIM) @$(DLLRULE) out:$(TARGET).bim map:$(TARGET).map $(OBJS)
	$(BIM2BIN) -osacmp -tek0 in:$(TARGET).bim out:$(TARGET).dll

clean :
	-$(DELE) *.obj
	-$(DELE) $(TARGET).bim
	-$(DELE) $(TARGET).map
	-$(DELE) $(TARGET).org
	-$(DELE) $(TARGET).lib
