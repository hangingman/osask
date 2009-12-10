ml /c /coff execcmd.asm
lcc init.c     -Ih:\osask\pioneer0\lccinc -O
lcc fwinman.c -Ih:\osask\pioneer0\lccinc -O
lcc fpokon.c -Ih:\osask\pioneer0\lccinc -O
lcc fdebug.c -Ih:\osask\pioneer0\lccinc -O
lcclib -OUT:sysgg00.lib @sysgg00.opt
