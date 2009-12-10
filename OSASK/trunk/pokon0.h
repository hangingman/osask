#ifndef _POKON0_H
#define _POKON0_H

#define AUTO_MALLOC			0
#define NULL				0
#define SYSTEM_TIMER		0x01c0
#define LIST_HEIGHT			8
#define ext_EXE				('E' | ('X' << 8) | ('E' << 16))
#define ext_BIN				('B' | ('I' << 8) | ('N' << 16))
#define CONSOLESIZEX		40
#define CONSOLESIZEY		15
#define MAX_BANK			56
#define MAX_FILEBUF			64
#define MAX_SELECTOR		5
#define MAX_SELECTORWAIT	64
#define MAX_VMREF			64
#define JOBLIST_SIZE		16

/* key defines */
#define DEFSIG_EXT1     0x00001000
#define DEFSIG_MAKE     0x00000000
#define DEFSIG_BREAK    0x00004000
#define DEFSIG_REMAKE   0x00008000
#define DEFSIG_NOCAPS   0x00040000
#define DEFSIG_CAPS     0x04040000
#define DEFSIG_NOSHIFT  0x00100000
#define DEFSIG_SHIFT    0x10100000
#define DEFSIG_NOCTRL   0x00200000
#define DEFSIG_CTRL     0x20200000
#define DEFSIG_NOALT    0x00400000
#define DEFSIG_ALT      0x40400000

/* pokon status */
enum {
	STATUS_RUN_APPLICATION,
	STATUS_MAKE_PLAIN_BOOT_DISK,
	STATUS_MAKE_COMPRESSED_BOOT_DISK,
	FMODE3,
	FMODE4,
	FMODE5,
	FMODE6,
	STATUS_LOAD_BOOT_SECTOR_CODE_COMPLETE,/* 'S'と'Enter'と'F'と'R'しか入力できない */
	STATUS_WRITE_KERNEL_COMPLETE,
	STATUS_FORMAT_COMPLETE,
};

/* signal defines */
/* main signals */
#define NO_SIGNAL                               0x0000 /* no signal */
#define SIGNAL_REWIND                           0x0004 /* rewind */
#define SIGNAL_BOOT_COMPLETE                    99 /* boot完了 */
#define SIGNAL_TERMINATED_TASK                  0x0080 /* terminated task */
#define SIGNAL_REQUEST_DIALOG                   0x0084 /* ダイアログ要求シグナル */
#define SIGNAL_REQUEST_DIALOG2                  0x0088
#define	SIGNAL_FREE_FILES						0x008c /* ファイル開放要求(えせファイルシステム用) */
#define SIGNAL_RELOAD_FAT_COMPLETE              0x00a0 /* FAT再読み込み完了(Insert) */
#define SIGNAL_LOAD_APP_FILE_COMPLETE           0x00a4 /* ファイル読み込み完了(file load & execute) */
#define SIGNAL_CREATE_TASK_COMPLETE             0x00a8 /* タスク生成完了(create task) */
#define SIGNAL_LOAD_FILE_COMPLETE               0x00ac /* ファイル読み込み完了(file load) */
#define SIGNAL_LOAD_KERNEL_COMPLETE             0x00b0 /* ファイル読み込み完了(.EXE file load) */
#define SIGNAL_LOAD_BOOT_SECTOR_CODE_COMPLETE   0x00b4 /* ファイル読み込み完了(.EXE file load) */
#define SIGNAL_FORMAT_COMPLETE                  0x00b8 /* フォーマット完了 */
#define SIGNAL_WRITE_KERNEL_COMPLETE            0x00bc /* .EXE書き込み完了 */

/* action signals */
enum {
	SIGNAL_CURSOR_UP = 4,
	SIGNAL_CURSOR_DOWN,
	SIGNAL_ENTER = 6,
	SIGNAL_PAGE_UP = 10,
	SIGNAL_PAGE_DOWN,
	SIGNAL_TOP_OF_LIST = 12,
	SIGNAL_BOTTOM_OF_LIST,
	SIGNAL_DISK_CHANGED = 14,
	SIGNAL_LETTER_START = '!',
	SIGNAL_LETTER_END = 'Z',
	SIGNAL_WINDOW_CLOSE0 = 126,
	SIGNAL_WINDOW_CLOSE1 = 127,
};

/* command signals */
enum {
	COMMAND_SIGNAL_START = 0xc0,
	COMMAND_TO_FORMAT_MODE = COMMAND_SIGNAL_START,  /* to format-mode */
	COMMAND_TO_RUN_MODE,                            /* to run-mode */
	COMMAND_CHANGE_FORMAT_MODE,                     /* change format-mode */
	COMMAND_OPEN_CONSOLE,                           /* open console */
	COMMAND_OPEN_MONITOR,                           /* open monitor */
	COMMAND_SIGNAL_END = 0xff,
};

/* console signals ( + file selector signal start) */
enum {
	CONSOLE_SIGNAL_START = 256,
	CONSOLE_VRAM_ACCESS_ENABLE = CONSOLE_SIGNAL_START,
	CONSOLE_VRAM_ACCESS_DISABLE,
	CONSOLE_REDRAW_0,
	CONSOLE_REDRAW_1,
	CONSOLE_CHANGE_TITLE_COLOR = CONSOLE_SIGNAL_START + 5,
	CONSOLE_CLOSE_WINDOW,
	CONSOLE_CURSOR_BLINK = CONSOLE_SIGNAL_START + 31,
	CONSOLE_KEY_SIGNAL_START = CONSOLE_SIGNAL_START + ' ',
	CONSOLE_KEY_SIGNAL_END = CONSOLE_SIGNAL_START + 0x7f,
	CONSOLE_INPUT_ENTER = CONSOLE_SIGNAL_START + 0xa0,
	CONSOLE_INPUT_BACKSPACE = CONSOLE_SIGNAL_START + 0xa1,
	CONSOLE_SIGNAL_END = 512 - 1,
	FILE_SELECTOR_SIGNAL_START,
};

/* jobs */
#define JOB_INVALID_DISKCACHE           0x0004  /* invalid diskcache -> reload fat*/
#define JOB_LOAD_FILE_AND_EXECUTE       0x0008  /* file load & execute */
#define JOB_CREATE_TASK                 0x000c  /* create task */
#define JOB_LOAD_FILE                   0x0010  /* file load */
#define JOB_LOAD_FILE_AND_FORMAT        0x0014  /* file load & format (1) */

/* structs */
struct FILELIST {
	char name[11];
	struct SGG_FILELIST *ptr;
};

struct STR_BANK { /* 88bytes */
	int size, addr, tss;
	char name[12];
	struct {
		int global, inner;
	} Llv[8];
};

struct FILEBUF {
	int dirslot, linkcount, size, paddr;
};

struct FILESELWIN { /* 1つあたり、5.6KB必要 */
	struct LIB_WINDOW *window; /* (128B) */
	struct LIB_TEXTBOX *wintitle, *subtitle, *selector; /* (144B, 224B, 1088B) */
	struct FILELIST list[256] /* 4KB */, *lp;
	int ext, cur, winslot, sigbase;
	int task, mdlslot, num, siglen, sig[16];
};

struct SELECTORWAIT {
	int task, slot, bytes, ofs, sel;
};

struct STR_JOBLIST {
	int *list, free, *rp, *wp, now;
	int param[8];
	struct FILEBUF *fbuf;
	struct STR_BANK *bank;
	struct FILESELWIN *win;
	int dirslot;
};

struct VIRTUAL_MODULE_REFERENCE {
	int task, module_paddr;
};

static struct STR_CONSOLE {
	int curx, cury, col;
	struct LIB_WINDOW *win;
	unsigned char *buf;
	struct LIB_TEXTBOX *tbox, *title;
	int sleep, cursorflag, cursoractive;
};

/* functions */
void consoleout(char *s);
void open_console();

/* pokon console command */
int poko_memory(const char *cmdlin);
int poko_color(const char *cmdlin);
int poko_cls(const char *cmdlin);
int poko_mousespeed(const char *cmdlin);
int poko_setdefaultIL(const char *cmdlin);
int poko_tasklist(const char *cmdlin);
int poko_sendsignalU(const char *cmdlin);
int poko_LLlist(const char *cmdlin);
int poko_setIL(const char *cmdlin);
int poko_debug(const char *cmdlin);

/* */
void sgg_wm0s_sendto2_winman0(const int signal, const int param);

#if 0
void fileselect(struct FILESELWIN *win, int fileid);
#endif



#endif /* pokon0.h */
