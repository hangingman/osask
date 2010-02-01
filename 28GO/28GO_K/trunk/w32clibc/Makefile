include ../z_tools/command.txt

TARGET     = w32clibc
MODE       = lib
OBJS       = startup.obj fputs.obj puts.obj fprintf.obj printf.obj vprintf.obj \
			 vfprintf.obj fwrite.obj stdin.obj malloc.obj free.obj fclose.obj \
			 fflush.obj fopen.obj fread.obj fseek.obj ftell.obj remove.obj \
			 fputc.obj clearerr.obj rewind.obj fgetc.obj feof.obj fgets.obj \
			 gets.obj ungetc.obj exit.obj system.obj

INCPATH    = ../z_tools/win32/
CC1OPT     =
#include ../z_tools/com_mak.txt


# osa_dir—pMakefile


.SUFFIXES : .obj .nas
.SUFFIXES : .nas .gas
.SUFFIXES : .gas .ca
.SUFFIXES : .ca .c





all :
	$(MAKE) $(TARGET).$(MODE)


$(TARGET).$(MODE) : $(OBJS)
	$(GOLIB) out=$@ $(OBJS)




.nas.obj : $@.nas Makefile
	$(NASK) in:$*.nas out:$*.obj


.gas.nas : $@.gas Makefile
	$(GAS2NASK) in:$*.gas out:$*.nas


.ca.gas : $@.ca Makefile
	$(CC1) -o $*.gas $*.ca


.c.ca : $@.c Makefile
	$(SJISCONV) in:$*.c out:$*.ca


clean :
	-$(DELE) *.obj
	-$(DELE) *.gas
	-$(DELE) *.nas
	-$(DELE) *.ca


