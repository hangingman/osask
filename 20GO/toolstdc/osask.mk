include ../include/rules.mak

all :
	$(MAKER) golib00 sjisconv gas2nask nask naskcnv0

golib :
	$(MAKER) golib00

clean :
	-$(DEL) *.obj
	-$(DEL) golib00
	-$(DEL) sjisconv
	-$(DEL) gas2nask
	-$(DEL) nask
	-$(DEL) naskcnv0


golib00 : golib00.obj Makefile ../include/rules.mak
	$(LINK) -o golib00 golib00.obj

sjisconv : sjisconv.obj Makefile ../include/rules.mak
	$(LINK) -o sjisconv sjisconv.obj

gas2nask : gas2nask.obj Makefile ../include/rules.mak
	$(LINK) -o gas2nask gas2nask.obj





nask : nask.obj Makefile ../include/rules.mak
	$(MAKEC) ../nasklib
	$(MAKEC) ../go_lib
	$(LINK) -o nask nask.obj ../nasklib/nasklib.lib \
		../go_lib/go_lib.lib ../go_lib/stdin.o

naskcnv0 : naskcnv0.obj Makefile ../include/rules.mak
	$(MAKEC) ../go_lib
	$(LINK) -o naskcnv0 naskcnv0.obj ../go_lib/go_lib.lib
