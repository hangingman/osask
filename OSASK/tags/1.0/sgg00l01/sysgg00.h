#ifndef __SYSGG00_H
#define __SYSGG00_H

struct SGG_WORK {
	int data[256 / 4];
};

struct SGG_FILELIST {
	int id;
	char name[11];
	char type;
};

struct SGG_WINDOW {
	int image[64 / 4];
	int handle;
};

void sgg_execcmd(void *EBX);
struct SGG_WORK *sgg_init(struct SGG_WORK *work);
const int sgg_getfilelist(const int size, struct SGG_FILELIST *fp, const int reserve0, const int reserve1);
void sgg_wm0s_movewindow(struct SGG_WINDOW *window, const int x, const int y);
void sgg_wm0s_setstatus(const struct SGG_WINDOW *window, const int status);
void sgg_wm0s_accessenable(const struct SGG_WINDOW *window);
void sgg_wm0s_accessdisable(const struct SGG_WINDOW *window);
void sgg_wm0s_redraw(const struct SGG_WINDOW *window);
void sgg_wm0_openwindow(struct SGG_WINDOW *window, const int handle);
const int sgg_wm0_win2sbox(const struct SGG_WINDOW *window);
void sgg_wm0_definesignal2(const int opt, const int device, const int code,
	const int signalbox, const int signal0, const int signal1);
void sgg_wm0_definesignal0(const int opt, const int device, const int code);
const int sgg_wm0_winsizex(const struct SGG_WINDOW *window);
const int sgg_wm0_winsizey(const struct SGG_WINDOW *window);
void sgg_wm0s_close(const struct SGG_WINDOW *window);
void sgg_wm0_setvideomode(const int mode, const int signal);
void sgg_wm0_gapicmd_0010_0000();
void sgg_wm0_gapicmd_001c_0004();
void sgg_wm0_gapicmd_001c_0020();
void sgg_wm0_putmouse(const int x, const int y);
void sgg_wm0_removemouse();
void sgg_wm0_movemouse(const int x, const int y);
void sgg_wm0_enablemouse();
void sgg_loadfile(const int mdl_ent, const int file_id, const int fin_sig);
void sgg_createtask(const int mdl_ent, const int fin_sig);
void sgg_settasklocallevel(const int task, const int local, const int global, const int inner);
void sgg_runtask(const int task, const int local);
void sgg_freememory(const int mdl_ent);
void sgg_format(const int sub_cmd, const int sig);
void sgg_debug00(const int opt, const int bytes, const int reserve,
	const int src_ofs, const int src_sel, const int dest_ofs, const int dest_sel);

#endif
