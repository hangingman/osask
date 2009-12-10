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
void lib_execcmd0(const int cmd, ...);
const int lib_execcmd1(const int ret, const int cmd, ...);

void *malloc(const unsigned int nbytes);

#define	lib_init(work) \
	(struct LIB_WORK *) lib_execcmd1(1 * 4 + 12, 0x0004, \
	(work) ? (void *) (work) : malloc(sizeof (struct LIB_WORK)), 0x0000)

#define	lib_init_nm(work) \
	lib_execcmd0(0x0004, (void *) (work), 0x0000)

#define	lib_waitsignal(opt, signaldw, nest) \
	lib_execcmd0(0x0018, (int) (opt), (int) (signaldw), (int) (nest), 0x0000)

#define	lib_openwindow(window, slot, x_size, y_size) \
	(struct LIB_WINDOW *) lib_execcmd1(1 * 4 + 12, 0x0020, \
	(window) ? (void *) (window) : malloc(sizeof (struct LIB_WINDOW)), \
	(int) (slot), (int) (x_size), (int) (y_size), 0x0000)

#define	lib_openwindow_nm(window, slot, x_size, y_size) \
	lib_execcmd0(0x0020, (void *) (window), (int) (slot), (int) (x_size), \
	(int) (y_size), 0x0000)

#define	lib_opentextbox(opt, textbox, backcolor, x_size, y_size, x_pos, y_pos, window, charset, init_char) \
	(struct LIB_TEXTBOX *) lib_execcmd1(2 * 4 + 12, 0x0028, (int) (opt), \
	(textbox) ? (void *) (textbox) : malloc(sizeof (struct LIB_TEXTBOX) + 8 * (x_size) * (y_size)), \
	(int) (backcolor), (int) (x_size), (int) (y_size), (int) (x_pos), \
	(int) (y_pos), (void *) (window), (int) (charset), (int) (init_char), \
	0x0000)

#define	lib_opentextbox_nm(opt, textbox, backcolor, x_size, y_size, x_pos, y_pos, window, charset, init_char) \
	lib_execcmd0(0x0028, (int) (opt), (void *) (textbox), (int) (backcolor), \
	(int) (x_size), (int) (y_size), (int) (x_pos), (int) (y_pos), \
	(void *) (window), (int) (charset), (int) (init_char), 0x0000)

#define	lib_waitsignaltime(opt, signaldw, nest, time0, time1, time2) \
	lib_execcmd0(0x0018, (int) (opt), (int) (signaldw), (int) (nest), \
	(int) (time0), (int) (time1), (int) (time2), 0x0000)

#define	lib_opensignalbox(bytes, signalbox, eos, rewind) \
	(int *) lib_execcmd1(2 * 4 + 12, 0x0060, (int) (bytes), \
	(signalbox) ? (void *) (signalbox) : malloc(bytes), (int) (eos), \
	(int) (rewind), 0x0000)

#define	lib_opensignalbox_nm(bytes, signalbox, eos, rewind) \
	lib_execcmd0(0x0060, (int) (bytes), (void *) (signalbox), (int) (eos), \
	(int) (rewind), 0x0000)

#define	lib_definesignal0p0(opt, default_assign0, default_assign1, default_assign2) \
	lib_execcmd0(0x0068, (int) (opt), (int) (default_assign0), \
	(int) (default_assign1), (int) (default_assign2), 0, 0, 0x0000)

#define	lib_definesignal1p0(opt, default_assign0, default_assign1, default_assign2, signal) \
	lib_execcmd0(0x0068, (int) (opt), (int) (default_assign0), \
	(int) (default_assign1), (int) (default_assign2), 1, (int) (signal), \
	0, 0x0000)

#define	lib_opentimer(slot) \
	lib_execcmd0(0x0070, (int) (slot), 0x0000)

#define	lib_closetimer(slot) \
	lib_execcmd0(0x0074, (int) (slot), 0x0000)

#define	lib_settimertime(opt, slot, time0, time1, time2) \
	lib_execcmd0(0x0078, (int) (opt), (int) (slot), (int) (time0), \
	(int) (time1), (int) (time2), 0x0000)

#define	lib_settimer(opt, slot) \
	lib_execcmd0(0x0078, (int) (opt), (int) (slot), 0x0000)

#define	lib_opensoundtrack(slot) \
	lib_execcmd0(0x0080, (int) (slot), 0, 0x0000)

#define	lib_controlfreq(slot, freq) \
	lib_execcmd0(0x008c, (int) (slot), (int) (freq), 0x0000)

#define	lib_openwindow1(window, slot, x_size, y_size, flags, base) \
	(struct LIB_WINDOW *) lib_execcmd1(1 * 4 + 12, 0x0020, \
	(window) ? (void *) (window) : malloc(sizeof (struct LIB_WINDOW)), \
	(int) (slot) | 0x01, (int) (x_size), (int) (y_size), \
	0x01 | (int) (flags) << 8, (int) (base), 0x0000)

#define	lib_openwindow1_nm(window, slot, x_size, y_size, flags, base) \
	lib_execcmd0(0x0020, (void *) (window), (int) (slot) | 0x01, \
	(int) (x_size), (int) (y_size), 0x01 | (int) (flags) << 8, (int) (base), \
	0x0000)

#define	lib_closewindow(opt, window) \
	lib_execcmd0(0x0024, (int) (opt), (void *) (window), 0x0000)

#define	lib_controlwindow(opt, window) \
	lib_execcmd0(0x003c, (int) (opt), (void *) (window), 0x0000)

#define	lib_close(opt) \
	lib_execcmd0(0x0008, (int) (opt), 0x0000)

#define	lib_loadfontset(opt, slot, len, font) \
	lib_execcmd0(0x00e0, (int) (opt), (int) (slot), (int) (len), (int) (font), \
	0x000c, 0x0000)

#define	lib_loadfontset0(opt, slot) \
	lib_execcmd0(0x00e0, (int) (opt), (int) (slot), 0x0000)

#define	lib_makecharset(opt, charset, fontset, len, from, base) \
	lib_execcmd0(0x00e8, (int) (opt), (int) (charset), (int) (fontset), \
	(int) (len), (int) (from), (int) (base), 0x0000)

#define	lib_drawline(opt, window, color, x0, y0, x1, y1) \
	lib_execcmd0(0x0044, (int) (opt), (void *) (window), (int) (color), \
	(int) (x0), (int) (y0), (int) (x1), (int) (y1), 0x0000)

#define	lib_closetextbox(opt, textbox) \
	lib_execcmd0(0x002c, (int) (opt), (int) (textbox), 0x0000)

#define	lib_mapmodule(opt, slot, attr, size, addr, ofs) \
	lib_execcmd0(0x00c0, (int) (opt), (int) (slot), (int) (size), \
	(void *) (addr), 0x000c, (int) ((ofs) | (attr)), 0x0000)

#define	lib_unmapmodule(opt, size, addr) \
	lib_execcmd0(0x00c4, (int) (opt), (int) (size), (void *) (addr), 0x000c, \
	0x0000)

#define	lib_initmodulehandle(opt, slot) \
	lib_execcmd0(0x00a0, (int) (opt), (int) (slot), 0x0000)

//	struct LIB_WORK *lib_init(struct LIB_WORK *work);
//	void lib_waitsignal(const int opt, const int signaldw, const int nest);
//	struct LIB_WINDOW *lib_openwindow(struct LIB_WINDOW *window, const int slot, const int x_size, const int y_size);
//	struct LIB_TEXTBOX *lib_opentextbox(const int opt, struct LIB_TEXTBOX *textbox, const int backcolor,
//		const int x_size, const int y_size, const int x_pos, const int y_pos,
//		struct LIB_WINDOW *window, const int charset, const int init_char);
void lib_putstring_ASCII(const int opt, const int x_pos, const int y_pos,
	struct LIB_TEXTBOX *textbox, const int color, const int backcolor, const char *str);
//	void lib_waitsignaltime(const int opt, const int signaldw, const int nest,
//		const unsigned int time0, const unsigned int time1, const unsigned int time2);
//	int *lib_opensignalbox(const int bytes, int *signalbox, const int eos, const int rewind);
//	void lib_definesignal0p0(const int opt, const int default_assign0,
//		const int default_assign1, const int default_assign2);
//	void lib_definesignal1p0(const int opt, const int default_assign0,
//		const int default_assign1, struct LIB_WINDOW *default_assign2, const int signal);
//	void lib_opentimer(const int slot);
//	void lib_closetimer(const int slot);
//	void lib_settimertime(const int opt, const int slot,
//		const unsigned int time0, const unsigned int time1, const unsigned int time2);
//	void lib_settimer(const int opt, const int slot);
void lib_definesignalhandler(void (*lib_signalhandler)(struct LIB_SIGHNDLREG *));
//	void lib_opensoundtrack(const int slot);
//	void lib_controlfreq(const int slot, const int freq);
//	struct LIB_WINDOW *lib_openwindow1(struct LIB_WINDOW *window, const int slot,
//		const int x_size, const int y_size, const int flags, const int base);
//	void lib_closewindow(const int opt, struct LIB_WINDOW *window);
//	void lib_controlwindow(const int opt, struct LIB_WINDOW *window);
//	void lib_close(const int opt);
//	void lib_loadfontset(const int opt, const int slot, const int len, void *font);
//	void lib_loadfontset0(const int opt, const int slot);
//	void lib_makecharset(const int opt, const int charset, const int fontset,
//		const int len, const int from, const int base);
//	void lib_drawline(const int opt, struct LIB_WINDOW *window, const int color,
//		const int x0, const int y0, const int x1, const int y1);
//	void lib_closetextbox(const int opt, const struct LIB_TEXTBOX *textbox);
const int lib_readCSb(const int offset);
const int lib_readCSd(const int offset);
//	void lib_mapmodule(const int opt, const int slot, const int attr, const int size, void *addr, const int ofs);
//	void lib_unmapmodule(const int opt, const int size, void *addr);
void lib_steppath(const int opt, const int slot, const char *name);
//	void lib_initmodulehandle(const int opt, const int slot);
const int lib_readmodulesize(const int slot);
void lib_initmodulehandle1(const int slot, const int num, const int sig);

#endif
