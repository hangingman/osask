include ../z_tools/command.txt


all:
	$(MAKE) imgtol.com

%.ias : %.ask
	$(TOOLPATH)28GOcpp0 -P -I. -o $*.ias $*.ask

%.3as : %.ias
	$(ASKA) $*.ias $*.3as

%.nas : %.3as
	$(NASKCNV) -l -s $*.3as $*.nas

%.com : %.nas
	$(NASK) $*.nas $*.org $*.lst
	$(BIM2BIN) -osacmp -l2d3 in:$*.org out:$*.ld3
	comcom $*.ld3 $*.com


clean:
	$(DELE) *.lst
	$(DELE) *.ld3
	$(DELE) *.org