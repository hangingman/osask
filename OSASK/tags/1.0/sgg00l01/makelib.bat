ml /c /coff execcmd.asm
lcc init.c     -I. -O
rem lcc waitsig.c  -I. -O
rem lcc waitsigt.c -I. -O
rem lcc openwin.c  -I. -O
rem lcc opentext.c -I. -O
rem lcc putstr.c   -I. -O
rem lcc opensigb.c -I. -O
lcc fwinman.c -I. -O
lcc fpokon.c -I. -O
lcc fdebug.c -I. -O
lcclib -OUT:sysgg00.lib @sysgg00.opt
