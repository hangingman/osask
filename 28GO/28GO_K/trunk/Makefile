TOOLPATH = ./z_tools/

all:
#	$(TOOLPATH)make -C gcc
	$(TOOLPATH)make -C toolw32
	$(TOOLPATH)golib00.exe out:w32clibc/libmingw.lib libmingw/libmingw_o/_chkstk.o libmingw/libmingw_o/dt.o libmingw/libmingw_o/dh.o libmingw/libmingw_o/ds00027.o libmingw/libmingw_o/ds00053.o libmingw/libmingw_o/ds00130.o libmingw/libmingw_o/ds00207.o libmingw/libmingw_o/ds00277.o libmingw/libmingw_o/ds00342.o libmingw/libmingw_o/ds00535.o libmingw/libmingw_o/ds00724.o libmingw/libmingw_o/ds00323.o libmingw/libmingw_o/ds00415.o libmingw/libmingw_o/ds00421.o libmingw/libmingw_o/ds00424.o libmingw/libmingw_o/ds00175.o libmingw/libmingw_o/ds00611.o libmingw/libmingw_o/ds00088.o libmingw/libmingw_o/ds00070.o libmingw/libmingw_o/ds00272.o libmingw/libmingw_o/ds00727.o
	$(TOOLPATH)make -C akusA
	$(TOOLPATH)make -C bin2obj
	$(TOOLPATH)make -C doscmd
	$(TOOLPATH)make -C makefont
	$(TOOLPATH)make -C hrblib0a


clean :
	$(TOOLPATH)make -C gcc       clean
	$(TOOLPATH)make -C toolstdc  clean
	$(TOOLPATH)make -C toolw32   clean
	$(TOOLPATH)make -C nasklib   clean
	$(TOOLPATH)make -C go_lib    clean
	$(TOOLPATH)make -C libiberty clean
	$(TOOLPATH)make -C intl      clean
	$(TOOLPATH)make -C cp        clean
	$(TOOLPATH)make -C drv_w32   clean
	$(TOOLPATH)make -C drv_stdc  clean
	$(TOOLPATH)make -C drv_osa   clean
	$(TOOLPATH)make -C golibc    clean
	$(TOOLPATH)make -C w32clibc  clean
