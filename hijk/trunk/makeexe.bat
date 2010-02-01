..\z_tools\efg01.exe ../z_tools/sjisconv.g01 sjis in:efg01.c out:efg01.ca
..\z_tools\cc1.exe -I../z_tools/win32/ -Dmain=main0 -DUSE_INCLUDE_G01 -Os -Wall -quiet -o efg01.gas efg01.ca
..\z_tools\efg01.exe ../z_tools/gas2nask.g01 -a D:4 -A in:efg01.gas out:efg01.nas
..\z_tools\efg01.exe ../z_tools/nask.g01 in:efg01.nas out:efg01.obj
..\z_tools\efg01.exe ../z_tools/sjisconv.g01 sjis in:tek.c out:tek.ca
..\z_tools\cc1.exe -I../z_tools/win32/ -Dmain=main0 -Os -Wall -quiet -o tek.gas tek.ca
..\z_tools\efg01.exe ../z_tools/gas2nask.g01 -a D:4 -A in:tek.gas out:tek.nas
..\z_tools\efg01.exe ../z_tools/nask.g01 in:tek.nas out:tek.obj
..\z_tools\efg01.exe ../z_tools/nask.g01 in:naskfunc.nas out:naskfunc.obj
..\z_tools\efg01.exe -noadc ../z_tools/bin2obj.g01 -h in:%1.g01 out:inclg01.obj label:_inclg01
..\z_tools\ld.exe -s -Bdynamic --stack 0x1c00000  -o %1.exe -Map %1.map efg01.obj tek.obj naskfunc.obj inclg01.obj ../z_tools/win32/w32clibc.lib ../z_tools/win32/golibc.lib ../z_tools/win32/libmingw.lib
..\z_tools\upx.exe -9 %1.exe