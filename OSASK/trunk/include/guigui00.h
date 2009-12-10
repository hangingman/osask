#ifndef __GUIGUI00_H
#define __GUIGUI00_H

struct LIB_WORK {
	int data[256 / 4];
};

struct LIB_WINDOW {
	int data[128 / 4];
};

struct LIB_TEXTBOX {
	int data[64 / 4];
};

struct LIB_SIGHNDLREG {
	int ES, DS, FS, GS;
	int EDI, ESI, EBP, ESP;
	int EBX, EDX, ECX, EAX;
	int EIP, CS, EFLAGS;
};

void lib_execcmd(void *EBX);
struct LIB_WORK *lib_init(struct LIB_WORK *work);
void lib_waitsignal(const int opt, const int signaldw, const int nest);
struct LIB_WINDOW *lib_openwindow(struct LIB_WINDOW *window, const int slot, const int x_size, const int y_size);
struct LIB_TEXTBOX *lib_opentextbox(const int opt, struct LIB_TEXTBOX *textbox, const int backcolor,
	const int x_size, const int y_size, const int x_pos, const int y_pos,
	struct LIB_WINDOW *window, const int charset, const int init_char);
void lib_putstring_ASCII(const int opt, const int x_pos, const int y_pos,
	struct LIB_TEXTBOX *textbox, const int color, const int backcolor, const char *str);
void lib_waitsignaltime(const int opt, const int signaldw, const int nest,
	const unsigned int time0, const unsigned int time1, const unsigned int time2);
int *lib_opensignalbox(const int bytes, int *signalbox, const int eom, const int rewind);
void lib_definesignal0p0(const int opt, const int default_assign0,
	const int default_assign1, const int default_assign2);
void lib_definesignal1p0(const int opt, const int default_assign0,
	const int default_assign1, struct LIB_WINDOW *default_assign2, const int signal);
void lib_opentimer(const int slot);
void lib_closetimer(const int slot);
void lib_settimertime(const int opt, const int slot,
	const unsigned int time0, const unsigned int time1, const unsigned int time2);
void lib_settimer(const int opt, const int slot);
void lib_definesignalhandler(void (*lib_signalhandler)(struct LIB_SIGHNDLREG *));
void lib_opensoundtrack(const int slot);
void lib_controlfreq(const int slot, const int freq);
struct LIB_WINDOW *lib_openwindow1(struct LIB_WINDOW *window, const int slot,
	const int x_size, const int y_size, const int flags, const int base);
void lib_closewindow(const int opt, struct LIB_WINDOW *window);
void lib_controlwindow(const int opt, struct LIB_WINDOW *window);
void lib_close(const int opt);
void lib_loadfontset(const int opt, const int slot, const int len, void *font);
void lib_loadfontset0(const int opt, const int slot);
void lib_makecharset(const int opt, const int charset, const int fontset,
	const int len, const int from, const int base);
void lib_drawline(const int opt, struct LIB_WINDOW *window, const int color,
	const int x0, const int y0, const int x1, const int y1);
void lib_closetextbox(const int opt, const struct LIB_TEXTBOX *textbox);

#endif
