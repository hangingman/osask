rem ..\z_tools\make.exe %1 %2 %3 %4 %5 %6 %7 %8 %9
..\z_tools\efg01.exe ../z_tools/nask.g01 in:calendar.nas out:calendar.obj lst:calendar.lst
..\z_tools\efg01.exe ../z_tools/obj2bim.g01 rul:../z_tools/guigui01/guigui01.rul out:calendar.bim stack:0 map:calendar.map rlm:calendar.rlm calendar.obj
..\z_tools\efg01.exe ../z_tools/bim2g01.g01 calendar.bim calendar.org calendar.rlm
..\z_tools\efg01.exe ../z_tools/tekmin0.g01 calendar.org calendar.nho
..\z_tools\bim2bin.exe -osacmp in:calendar.nho out:calendar.nh5 eopt:@ eprm:@
..\z_tools\efg01.exe ../z_tools/tekmin1.g01 -efg01-noadc calendar.nh5 calendar.g01

