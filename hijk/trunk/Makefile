TARGET     = hijk
MODE       = exe
OBJS       = $(TARGET).obj tek.obj naskfunc.obj
#STACKSIZE  = 0x1c00000 # 28MB (default + 12MB)

#INCPATH    = ../z_tools/win32/
#CC1OPT     = -Dmain=main0
#include ../z_tools/com_mak.txt


all :
	make $(TARGET).$(MODE)


$(TARGET).$(MODE) : $(TARGET).c tek.c _gasfunc.s
	gcc -Os -DUSE_WIN32 -o $(TARGET) $(TARGET).c tek.c _gasfunc.s
	strip $(TARGET).$(MODE)
#	upx -9 $(TARGET).$(MODE)


#Linuxなどの場合：
#	（gccの3.x系で）
#    >gcc -Os -DUSE_POSIX -o efg01 efg01.c tek.c gasfunc.s
#    >strip efg01
#    >upx -9 efg01




clean :
	$(RM) $(TARGET).$(MODE)