..\z_tools\cpp0.exe -P -I. -o calc1.ias calc1.ask
..\z_tools\aska.exe calc1.ias calc1.3as
..\z_tools\efg01.exe ../z_tools/naskcnv0.g01 -l -s calc1.3as calc1.nas
..\z_tools\efg01.exe ../z_tools/nask.g01 calc1.nas calc1.org calc1.lst