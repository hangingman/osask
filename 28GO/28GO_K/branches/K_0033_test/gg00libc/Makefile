include ../z_tools/command.txt

TARGET     = gg00libc
STACKSIZE  =
MALLOCSIZE =
MMAREA     =
OBJS       = decl2d3.obj dectek0.obj defsghdl.obj execcmd.obj execcmd0.obj \
	execcmd1.obj initmdh1.obj malloc.obj putsjis0.obj putstr.obj readcsb.obj \
	readcsd.obj readmdsz.obj settime2.obj stppth0.obj alloca.obj \
	idiv.obj imul.obj imuldiv.obj

# 以上5つはソースごとに書き換える
# OBJSを書き換えると分割コンパイル対応
 

INCPATH  = ../
RULEFILE =


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
	$(NASKCNV) in:$*.3as out:$*.nas

%.lst : %.nas Makefile
	$(NASK) $*.nas $*.obj $*.lst

$(TARGET).bim : $(OBJS) Makefile
	$(OBJ2BIM) @$(RULEFILE) out:$(TARGET).bim stack:$(STACKSIZE) map:$(TARGET).map $(OBJS)

%.bin : %.bim Makefile
	$(BIM2BIN) in:$*.bim out:$*.org malloc:$(MALLOCSIZE) mmarea:$(MMAREA)
	$(BIM2BIN) in:$*.org out:$*.bin -osacmp -tek0

$(TARGET).lib : $(OBJS) Makefile
	$(GOLIB) out=$@ $(OBJS)

clean :
	$(DELE) *.obj
	$(DELE) $(TARGET).bim
	$(DELE) $(TARGET).map
	$(DELE) $(TARGET).org
