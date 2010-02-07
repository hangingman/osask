..\z_tools\cpp0.exe -P -I. -o hexdump.ias hexdump.ask
..\z_tools\aska.exe hexdump.ias hexdump.3as
..\z_tools\efg01.exe ../z_tools/naskcnv0.g01 -l -s hexdump.3as hexdump.nas
..\z_tools\efg01.exe ../z_tools/nask.g01 hexdump.nas hexdump.org hexdump.lst
..\z_tools\efg01.exe ../z_tools/rjcg01.g01 hexdump.org hexdump.rjc
..\z_tools\efg01.exe ../z_tools/tekmin0.g01 hexdump.rjc hexdump.nho
..\z_tools\bim2bin.exe -osacmp eopt:@ eprm:@ in:hexdump.nho out:hexdump.nh5
..\z_tools\efg01.exe ../z_tools/tekmin1.g01 -efg01-noadc hexdump.nh5 hexdump.g01