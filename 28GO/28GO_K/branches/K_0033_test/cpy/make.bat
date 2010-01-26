rem ..\z_tools\make.exe %1 %2 %3 %4 %5 %6 %7 %8 %9

..\z_tools\cpp0.exe -P -I. -o cpy.ias cpy.ask
..\z_tools\aska.exe cpy.ias cpy.3as
..\z_tools\efg01.exe ../z_tools/naskcnv0.g01 -l -s in:cpy.3as out:cpy.nas
..\z_tools\efg01.exe ../z_tools/nask.g01 in:cpy.nas out:cpy.org lst:cpy.lst