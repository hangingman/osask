/* "pokon0.c":アプリケーションラウンチャー  ver.2.2
     copyright(C) 2001 小柳雅明, 川合秀実
    stack:4k malloc:92k file:0 */

/* バーチャルモジュールを開放せよ！ */

#include <guigui00.h>
#include <sysgg00.h>
/* sysggは、一般のアプリが利用してはいけないライブラリ
   仕様もかなり流動的 */
#include <stdlib.h>

#include "pokon0.h"

#define POKON_VERSION "pokon22"

#define POKO_VERSION "Heppoko-shell \"poko\" version 2.0\n    Copyright (C) 2002 H.Kawai(Kawaido)\n"
#define POKO_PROMPT "\npoko>"

/* pokon console error message */
enum {
	ERR_CODE_START = 1,
	ERR_BAD_COMMAND = ERR_CODE_START,
	ERR_ILLEGAL_PARAMETERS,
};

const static char *pokon_error_message[] = {
	"\nBad command.\n",
	"\nIllegal parameter(s).\n",
};

static struct STR_JOBLIST job;
static struct STR_VIEWER BINEDIT = { "BEDITC00BIN", 0, 0, 0, 0 };
static struct STR_VIEWER TXTEDIT = { "TEDITC00BIN", 2, 0x7f000001, 42, 0 };
static struct STR_VIEWER PICEDIT = { "BMPV06  BIN", 0, 0, 0, 0 };
static struct STR_VIEWER RESIZER = { "RESIZER0BIN", 0, 0, 0, 0 };

struct STR_BANK *banklist;
struct SGG_FILELIST *file;
struct FILEBUF *filebuf;
static int defaultIL = 2;
struct FILESELWIN *selwin;
struct SELECTORWAIT *selwait;
int selwaitcount = 0, selwincount = 1;
struct VIRTUAL_MODULE_REFERENCE *vmref0;
int need_wb;

static struct STR_CONSOLE console = { -1, 0, 0, NULL, NULL, NULL, NULL, 0, 0, 0 };

void writejob(int i)
{
	*(job.wp)++ = i;
	job.free--;
	if (job.wp == job.list + JOBLIST_SIZE)
		job.wp = job.list;
	return;
}

void writejob2(int i, int j)
{
	writejob(i);
	writejob(j);
	return;
}

const int readjob()
{
	int i = *(job.rp)++;
	job.free++;
	if (job.rp == job.list + JOBLIST_SIZE)
		job.rp = job.list;
	return i;
}

void sgg_loadfile2(const int file_id, const int fin_sig)
/* シグナルで、sizeとaddrを返す */
{
	sgg_execcmd0(0x0020, 0x80000000 + 8, 0x1247, 0x012c, file_id,
		0x4244 /* to pokon0 */, 0x7f000003, fin_sig, 0, 0, 0x0000);
	return;
}

void sendsignal1dw(int task, int sig)
{
	sgg_execcmd0(0x0020, 0x80000000 + 3, task | 0x0242, 0x7f000001, sig, 0x0000);
	return;
}

struct FILEBUF *searchfbuf(int fileid);
void selwinfin(int task, struct FILESELWIN *win, struct FILEBUF *fbuf, struct VIRTUAL_MODULE_REFERENCE *vmr);
struct FILEBUF *check_wb_cache(struct FILEBUF *fbuf);
int searchfid(const unsigned char *s);
struct STR_BANK *run_viewer(struct STR_VIEWER *viewer, int fid);
int searchfid0(const unsigned char *s);

void runjobnext()
{
	struct STR_JOBLIST *pjob = &job;
	struct FILEBUF *fbuf;
	int i;
	do {
		if ((pjob->now = *(pjob->rp)) == 0)
			return;

		readjob(); // から読み
		switch (pjob->now) {

		case JOB_INVALID_DISKCACHE:
			if (need_wb == 0) {
				sgg_format(0x0114, SIGNAL_RELOAD_FAT_COMPLETE);
				break;
			}
			pjob->now = 0;
			break;

		case JOB_LOAD_FILE_AND_EXECUTE:
			pjob->dirslot = readjob(); /* fileid */
			pjob->fbuf = (struct FILEBUF *) readjob(); /* fbuf */
			pjob->bank = (struct STR_BANK *) readjob(); /* bank */
			pjob->param[7] = 0;
	job_load_viewer:
			if (fbuf = searchfbuf(pjob->dirslot)) {
				/* 確保しておいたやつを開放 */
				pjob->fbuf->linkcount = 0;
				pjob->fbuf = fbuf;
				pjob->bank->size = fbuf->size;
				pjob->bank->addr = fbuf->paddr;
				fbuf->linkcount++;
				goto job_create_task2;
			}
			sgg_loadfile2(pjob->dirslot /* file id */, SIGNAL_LOAD_APP_FILE_COMPLETE);
			break;

		case JOB_CREATE_TASK:
			pjob->bank = (struct STR_BANK *) readjob(); /* bank */
			for (i = 0; i < 8; i++)
				pjob->param[i] = readjob();
	job_create_task2:
			sgg_createtask2(pjob->bank->size, pjob->bank->addr, SIGNAL_CREATE_TASK_COMPLETE);
			break;

		case JOB_LOAD_FILE:
			pjob->fbuf = (struct FILEBUF *) readjob(); /* fbuf */
			pjob->win = (struct FILESELWIN *) readjob(); /* win */
			pjob->param[0] = readjob(); /* tss */
			pjob->dirslot = readjob(); /* fileid */
			if (fbuf = searchfbuf(pjob->dirslot)) {
				/* 確保しておいたやつを開放 */
				pjob->fbuf->linkcount = 0;
				pjob->fbuf = fbuf;
				fbuf->linkcount++;
				selwinfin(pjob->param[0], pjob->win, fbuf, vmref0);
				pjob->win->mdlslot = -2;
				if (pjob->win->lp /* NULLならクローズ処理中 */) {
					pjob->win->task = 0;
					selwincount--;
				}
				pjob->now = 0;
				break;
			}
			sgg_loadfile2(pjob->dirslot /* file id */, SIGNAL_LOAD_FILE_COMPLETE);
			break;

		case JOB_LOAD_FILE_AND_FORMAT:
			pjob->param[0] = readjob(); /* fileid */
			sgg_loadfile2(pjob->param[0] /* file id */, SIGNAL_LOAD_KERNEL_COMPLETE);
			break;

		case JOB_VIEW_FILE:
			pjob->dirslot = readjob(); /* fileid */
			pjob->fbuf = (struct FILEBUF *) readjob(); /* fbuf */
			pjob->bank = (struct STR_BANK *) readjob(); /* bank */
			for (i = 0; i < 6; i++)
				pjob->param[i] = readjob();
			pjob->param[7] = 1;
			goto job_load_viewer;

		case JOB_CHECK_WB_CACHE:
			if (check_wb_cache(filebuf) == NULL) /* 終了したらNULLを返す */
				pjob->now = 0;
			break;

		case JOB_WRITEBACK_CACHE:
			sgg_execcmd0(0x0020, 0x80000000 + 6, 0x1245, 0x0140, 0x4243 /* to pokon0 */, 0x7f000002, SIGNAL_NO_WB_CACHE, 0, 0x0000);
			break;

		case JOB_INVALID_WB_CACHE:
			sgg_execcmd0(0x0020, 0x80000000 + 6, 0x1245, 0x0144, 0x4243 /* to pokon0 */, 0x7f000002, SIGNAL_NO_WB_CACHE, 0, 0x0000);
			break;

		case JOB_FREE_MEMORY:
			pjob->param[0] = readjob(); /* fileid */
			pjob->param[1] = readjob(); /* size */
			pjob->param[2] = readjob(); /* addr */
			sgg_execcmd0(0x0020, 0x80000000 + 5, 0x1244, 0x0134, pjob->param[0], pjob->param[1], pjob->param[2], 0x0000);
			pjob->now = 0;
			break;

		case JOB_CREATE_FILE:
			pjob->param[0] = readjob(); /* filename */
			pjob->param[1] = readjob();
			pjob->param[2] = readjob();
			pjob->param[6] = readjob(); /* task */
			pjob->param[7] = readjob(); /* signal */
			if (searchfid((char *) pjob->param) == 0) {
				sgg_execcmd0(0x0020, 0x80000000 + 9, 0x1248, 0x0148, pjob->param[0], pjob->param[1], pjob->param[2] >> 8,
					0x4243 /* to pokon0 */, 0x7f000002, SIGNAL_REFRESH_FLIST, 0, 0x0000);
			} else {
	errorsignal:
				pjob->now = 0;
				if (pjob->param[6])
					sendsignal1dw(pjob->param[6], pjob->param[7] + 1);
			}
			break;

		case JOB_RENAME_FILE:
			pjob->param[0] = readjob(); /* filename0 */
			pjob->param[1] = readjob();
			pjob->param[2] = readjob();
			pjob->param[3] = readjob(); /* filename1 */
			pjob->param[4] = readjob();
			pjob->param[5] = readjob();
			pjob->param[6] = readjob(); /* task */
			pjob->param[7] = readjob(); /* signal */
			if (searchfid((char *) &pjob->param[0]) != 0 && searchfid((char *) &pjob->param[3]) == 0) {
				sgg_execcmd0(0x0020, 0x80000000 + 12, 0x124b, 0x014c,
					pjob->param[0], pjob->param[1], pjob->param[2] >> 8,
					pjob->param[3], pjob->param[4], pjob->param[5] >> 8,
					0x4243 /* to pokon0 */, 0x7f000002, SIGNAL_REFRESH_FLIST, 0, 0x0000);
				break;
			}
			goto errorsignal;

		case JOB_RESIZE_FILE:
			pjob->param[0] = readjob(); /* filename */
			pjob->param[1] = readjob();
			pjob->param[2] = readjob();
			pjob->param[3] = readjob(); /* new-size */
			pjob->param[4] = readjob(); /* max-linkcount */
			pjob->param[6] = readjob(); /* task */
			pjob->param[7] = readjob(); /* signal */
			if ((i = searchfid((char *) &pjob->param[0])) != 0) {
				fbuf = searchfbuf(i);
				i = 0;
				if (fbuf)
					i = fbuf->linkcount;
				if (i <= pjob->param[4]) {
					sgg_execcmd0(0x0020, 0x80000000 + 10, 0x1249, 0x0150,
						pjob->param[0], pjob->param[1], pjob->param[2] >> 8, pjob->param[3], 
						0x4243 /* to pokon0 */, 0x7f000002, SIGNAL_REFRESH_FLIST0, 1 /* 正常終了が-1だから */, 0x0000);
					break;
				}
			}
			goto errorsignal;

		case JOB_DELETE_FILE:
			pjob->param[0] = readjob(); /* filename */
			pjob->param[1] = readjob();
			pjob->param[2] = readjob();
			pjob->param[4] = readjob(); /* max-linkcount */
			pjob->param[6] = readjob(); /* task */
			pjob->param[7] = readjob(); /* signal */
			if ((i = searchfid((char *) &pjob->param[0])) != 0) {
				fbuf = searchfbuf(i);
				i = 0;
				if (fbuf)
					i = fbuf->linkcount;
				if (i <= pjob->param[4]) {
					sgg_execcmd0(0x0020, 0x80000000 + 9, 0x1248, 0x0154,
						pjob->param[0], pjob->param[1], pjob->param[2] >> 8,
						0x4243 /* to pokon0 */, 0x7f000002, SIGNAL_REFRESH_FLIST, 0, 0x0000);
					break;
				}
			}
			goto errorsignal;


		}
	} while (pjob->now == 0);
	return;
}

void sgg_freememory2(const int fileid, const int size, const int addr)
{
	if (size > 0) {
		if (fileid == -1)
			sgg_execcmd0(0x0020, 0x80000000 + 5, 0x1244, 0x0134, -1, size, addr, 0x0000);
		else if (job.free >= 4) {
			/* 空きが十分にある */
			writejob2(JOB_FREE_MEMORY, fileid);
			writejob2(size, addr);
			*(job.wp) = 0; /* ストッパー */
	//	} else {
			/* どうすりゃいいんだ？ */
		}
	}
	return;
}

void putselector0(struct FILESELWIN *win, const int pos, const char *str)
{
	lib_putstring_ASCII(0x0000, 0, pos, win->selector, 0, 0, str);
	return;
}

void put_file(struct FILESELWIN *win, const char *name, const int pos, const int col)
{
	static char charbuf16[17] = "          .     ";
	int i;
	if (*name) {
		for (i = 0; i < 8; i++)
			charbuf16[2 + i] = name[i];
		for (/* i = 8 */; i < 11; i++)
			charbuf16[3 + i] = name[i];
	//	charbuf16[10] = '.';
		if (col)
			lib_putstring_ASCII(0x0001, 0, pos, win->selector, 15,  2, charbuf16);
		else
			lib_putstring_ASCII(0x0001, 0, pos, win->selector,  0, 15, charbuf16);
	} else
	//	lib_putstring_ASCII(0x0000, 0, pos, selector, 0, 0, "                ");
		putselector0(win, pos, "                ");
	return;
}

void list_set(struct FILESELWIN *win)
{
	int i, ext = win->ext;
	struct SGG_FILELIST *fp;
	struct FILELIST *lp, *list = win->list, *wp1, *wp2, tmp;

	// システムにファイルのリストを要求
	sgg_getfilelist(256, file, 0, 0);

	// アプリケーションがLIST_HEIGHT個以下のときのための処置
	for (i = 0; i <= LIST_HEIGHT; i++)
		list[i].name[0] = '\0';

	// 拡張子extのものだけを抽出してlistへ
	fp = file + 1; // 最初の要素はダミー
	lp = list;

	while (fp->name[0]) {
		if ((fp->type & 0x18) == 0 
			&& (ext == ext_ALL || (fp->name[8] | (fp->name[9] << 8) | (fp->name[10] << 16)) == ext)) {
			for (i = 0; i < 11; i++)
				lp->name[i] = fp->name[i];
			lp->ptr = fp;
			lp++;
		}
		fp++;
	}
	lp->name[0] = '\0';

	// sort by Koyanagi
	if (list[0].name[0] != '\0') {
		for (wp1 = list; wp1->name[0]; ++wp1) {
			for (wp2 = lp - 1; wp2 != wp1; --wp2) {
				// compare
				for (i = 0; i < 11; ++i) {
					if ((wp2 - 1)->name[i] > wp2->name[i]) {
						// swap and break
						tmp = *wp2;
						*wp2 = *(wp2 - 1);
						*(wp2 - 1) = tmp;
						break;
					} if ((wp2 - 1)->name[i] < wp2->name[i]) {
						break;
					}
				}
			}
		}
	}
	//
	win->lp = lp = list;
	for (i = 0; i < LIST_HEIGHT; i++)
 		put_file(win, lp[i].name, i, 0);

	if (list[0].name[0] == '\0') {
		// 選択可能なファイルが１つもない
		lib_putstring_ASCII(0x0000, 0, LIST_HEIGHT / 2 - 1, win->selector, 1, 0, "File not found! ");
		win->cur = -1;
		return;
	}
 	put_file(win, lp[0].name, 0, 1);
	win->cur = 0;
	return;
}

void putcursor()
{
	struct STR_CONSOLE *pcons = &console;
	pcons->sleep = 0;
	lib_putstring_ASCII(0x0001, pcons->curx, pcons->cury, pcons->tbox, 0,
		((pcons->col & pcons->cursorflag) | ((pcons->col >> 4) & ~(pcons->cursorflag))) & 0x0f, " ");
	return;
}

const int poko_cmdchk(const char *line, const char *cmd)
{
	while (*line == *cmd) {
		line++;
		cmd++;
	}
	if (*cmd == '\0' && *line == ' ')
		return 1;
	return 0;
}

void itoa10(unsigned int i, char *s)
{
	int j;
	s[11] = '\0';
	for (j = 10; j >= 0; j--) {
		s[j] = (i % 10) + '0';
		i /= 10;
	}
	for (j = 0; j < 10; j++) {
		if (s[j] != '0')
			break;
		s[j] = ' ';
	}
	return;
}


void openselwin(struct FILESELWIN *win, const char *title, const char *subtitle)
{
	static struct KEY_TABLE0 {
		int code;
		unsigned char opt, signum;
	} table[] = {
		{ 0x00ae /* cursor-up, down */,		1, SIGNAL_CURSOR_UP },
		{ 0x00a0 /* Enter */,				0, SIGNAL_ENTER },
		{ 0x00a8 /* page-up, down */,		1, SIGNAL_PAGE_UP },
		{ 0x00ac /* cursor-left, right */,	1, SIGNAL_PAGE_UP },
		{ 0x00a6 /* Home, End */,			1, SIGNAL_TOP_OF_LIST },
		{ 0x007010a4 /* Insert */,			0, SIGNAL_DISK_CHANGED },
		{ 0x007010a5 /* Delete */,			0, SIGNAL_START_WB },
		{ 0x107010a4 /* Shift+Insert */,	0, SIGNAL_FORCE_CHANGED },
		{ 0x107010a5 /* Shift+Delete */,	0, SIGNAL_CHECK_WB_CACHE },
		{ 0x30301000 | 'C' /* Shif+Ctrl */,	1, SIGNAL_CREATE_NEW },
		{ 0x30301000 | 'S' /* Shif+Ctrl */,	0, SIGNAL_RESIZE },
		{ SIGNAL_LETTER_START /* letters */,SIGNAL_LETTER_END - SIGNAL_LETTER_START,  SIGNAL_LETTER_START },
		{ 0,                               0,  0 }
	};
	struct KEY_TABLE0 *pkt;
	struct LIB_WINDOW *window;
	char *ss;

	window = win->window = lib_openwindow1(AUTO_MALLOC, win->winslot, 160, 40 + LIST_HEIGHT * 16, 0x28, win->sigbase + 120 /* +6 */);
	win->wintitle = lib_opentextbox(0x1000, AUTO_MALLOC,  0, 10, 1,  0,  0, window, 0x00c0, 0);
	win->subtitle = lib_opentextbox(0x0000, AUTO_MALLOC,  0, 20, 1,  0,  0, window, 0x00c0, 0);
	win->selector = lib_opentextbox(0x0001, AUTO_MALLOC, 15, 16, 8, 16, 32, window, 0x00c0, 0);
	lib_putstring_ASCII(0x0000, 0, 0, win->wintitle, 0, 0, title);
	lib_putstring_ASCII(0x0000, 0, 0, win->subtitle, need_wb & 9, 0, subtitle);

	for (pkt = table; pkt->code; pkt++)
		lib_definesignal1p0(pkt->opt, 0x0100, pkt->code, window, win->sigbase + pkt->signum);
	lib_definesignal0p0(0, 0, 0, 0);
	ss = win->subtitle_str;
	while (*ss++ = *subtitle++);
	return;
}


void gotsignal(int len)
{
	lib_waitsignal(0x0000, len, 0);
	return;
}

struct FILEBUF *searchfbuf(int fileid)
{
	struct FILEBUF *fbuf = filebuf;
	int i;

	for (i = 0; i < MAX_FILEBUF; i++) {
		if (fbuf[i].dirslot == fileid)
			return &fbuf[i];
	}
	return NULL;
}

struct FILEBUF *searchfrefbuf()
{
	struct FILEBUF *fbuf = filebuf;
	int i;
	for (i = 0; i < MAX_FILEBUF; i++) {
		if (fbuf[i].linkcount == 0)
			return &fbuf[i];
	}
	return NULL;
}

void selwinfin(int task, struct FILESELWIN *win, struct FILEBUF *fbuf, struct VIRTUAL_MODULE_REFERENCE *vmr)
{
	if (win->mdlslot == -1) {
		if (fbuf->linkcount == 0) {
			sgg_freememory2(fbuf->dirslot, fbuf->size, fbuf->paddr);
			fbuf->dirslot = -1;
		}
		return;
	}
	fbuf->linkcount++;
	for (; vmr->task; vmr++);
	vmr->task = task;
	vmr->module_paddr = fbuf->paddr;
	vmr->virtualmodule = sgg_createvirtualmodule(fbuf->size, fbuf->paddr);
	sgg_directwrite(0, 4, 0, win->mdlslot,
		(0x003c /* slot_sel */ | task << 8) + 0xf80000, (int) &vmr->virtualmodule, 0x000c);
	sendsignal1dw(task, win->sig[1]);
	return;
}

int searchfid(const unsigned char *s)
{
	static char str[11];
	int i;
	for (i = 0; i < 8; i++)
		str[i] = s[i];
	for (i = 0; i < 3; i++)
		str[8 + i] = s[9 + i];
	return searchfid0(str);
}

int searchfid0(const unsigned char *s)
{
	struct SGG_FILELIST *fp;
	unsigned char c;
	int i;

	for (fp = file + 1; fp->name[0]; fp++) {
		c = 0;
		for (i = 0; i < 11; i++)
			c |= s[i] - fp->name[i];
		if (c == 0)
			return fp->id;
	}
	return 0;
}

struct FILEBUF *check_wb_cache(struct FILEBUF *fbuf)
{
	struct FILEBUF *fbuf0 = filebuf + MAX_FILEBUF;
	while (fbuf != fbuf0) {
		if (fbuf->dirslot != -1 && fbuf->size != 0) {
			sgg_execcmd0(0x0020, 0x80000000 + 9, 0x1248,
				0x013c, fbuf->dirslot, fbuf->size, fbuf->paddr,
				0x4243 /* to pokon0 */, 0x7f000002, SIGNAL_CHECK_WB_CACHE_NEXT, fbuf + 1,
				0x0000);
			return fbuf;
		}
		fbuf++;
	}
	return NULL;
}

void OsaskMain()
{
	struct FILESELWIN *win;
	struct STR_JOBLIST *pjob = &job;
	struct STR_CONSOLE *pcons = &console;
	struct VIRTUAL_MODULE_REFERENCE *vmref;
	struct STR_OPEN_ORDER *order;

	int i, j, sig, *sb0, *sbp, tss, booting = 1, fmode = STATUS_RUN_APPLICATION;
	int *subcmd, bootflags = 0;
	struct STR_BANK *bank;
	struct FILEBUF *fbuf;
	struct SELECTORWAIT *swait;

	lib_init(AUTO_MALLOC);
	sgg_init(AUTO_MALLOC);

	sbp = sb0 = lib_opensignalbox(256, AUTO_MALLOC, 0, 4);

	file = (struct SGG_FILELIST *) malloc(256 * sizeof (struct SGG_FILELIST));
//	list = (struct FILELIST *) malloc(256 * sizeof (struct FILELIST));
	banklist = (struct STR_BANK *) malloc(MAX_BANK * sizeof (struct STR_BANK));
	filebuf = (struct FILEBUF *) malloc(MAX_FILEBUF * sizeof (struct FILEBUF));
	selwin = (struct FILESELWIN *) malloc(MAX_SELECTOR * sizeof (struct FILESELWIN));
	selwait = (struct SELECTORWAIT *) malloc(MAX_SELECTORWAIT * sizeof (struct SELECTORWAIT));
	vmref = (struct VIRTUAL_MODULE_REFERENCE *) malloc(MAX_VMREF * sizeof (struct VIRTUAL_MODULE_REFERENCE));
	vmref0 = vmref;
	subcmd = (int *) malloc(256 * sizeof (int));
	order = (struct STR_OPEN_ORDER *) malloc(MAX_ORDER * sizeof (struct STR_OPEN_ORDER));

	pjob->list = (int *) malloc(JOBLIST_SIZE * sizeof (int));
	*(pjob->rp = pjob->wp = pjob->list) = 0; /* たまった仕事はない */
	pjob->free = JOBLIST_SIZE - 1;

	pcons->win = (struct LIB_WINDOW *) malloc(sizeof (struct LIB_WINDOW));
	pcons->title = (struct LIB_TEXTBOX *) malloc(sizeof (struct LIB_TEXTBOX) + 16 * 8);
	pcons->tbox = (struct LIB_TEXTBOX *) malloc(sizeof (struct LIB_TEXTBOX) + CONSOLESIZEX * CONSOLESIZEY * 8);
	pcons->buf = (char *) malloc((CONSOLESIZEX + 2) * (CONSOLESIZEY + 1));

	win = selwin;
	for (i = 0; i < MAX_SELECTOR; i++) {
		win[i].task = 0;
		win[i].winslot = 0x0220 + i * 16;
		win[i].sigbase = 512 + 128 * i;
		win[i].window = NULL;
	}

	for (i = 0; i < MAX_SELECTORWAIT; i++)
		selwait[i].task = 0;

	for (i = 0; i < MAX_VMREF; i++)
		vmref[i].task = 0;

	for (i = 0; i < MAX_ORDER; i++)
		order[i].task = 0;

	openselwin(&selwin[0], POKON_VERSION, "< Run Application > ");

	lib_opentimer(SYSTEM_TIMER);
	lib_definesignal1p0(0, 0x0010 /* timer */, SYSTEM_TIMER, 0, CONSOLE_CURSOR_BLINK);

	/* キー操作を追加登録 */
	{
		static struct KEY_TABLE1 {
			int code;
			unsigned char opt, signum;
		} table[] = {
#if 0
			{ 'F' | DEFSIG_EXT1 | DEFSIG_NOSHIFT | DEFSIG_NOCTRL | DEFSIG_NOALT, 0, COMMAND_TO_FORMAT_MODE },
			{ 'R' | DEFSIG_EXT1 | DEFSIG_NOSHIFT | DEFSIG_NOCTRL | DEFSIG_NOALT, 0, COMMAND_TO_RUN_MODE },
			{ 'S' | DEFSIG_EXT1 | DEFSIG_NOSHIFT | DEFSIG_NOCTRL | DEFSIG_NOALT, 0, COMMAND_CHANGE_FORMAT_MODE },
			{ 'C' | DEFSIG_EXT1 | DEFSIG_NOSHIFT | DEFSIG_NOCTRL | DEFSIG_NOALT, 0, COMMAND_OPEN_CONSOLE },
			{ 'M' | DEFSIG_EXT1 | DEFSIG_NOSHIFT | DEFSIG_NOCTRL | DEFSIG_NOALT, 0, COMMAND_OPEN_MONITOR },
			{ 'B' | DEFSIG_EXT1 | DEFSIG_NOSHIFT | DEFSIG_NOCTRL | DEFSIG_NOALT, 0, COMMAND_BINEDIT },
			{ 'T' | DEFSIG_EXT1 | DEFSIG_NOSHIFT | DEFSIG_NOCTRL | DEFSIG_NOALT, 0, COMMAND_TXTEDIT },
#else
			{ 'F' | DEFSIG_EXT1 | DEFSIG_NOSHIFT | DEFSIG_CTRL | DEFSIG_NOALT, 0, COMMAND_TO_FORMAT_MODE },
			{ 'R' | DEFSIG_EXT1 | DEFSIG_NOSHIFT | DEFSIG_CTRL | DEFSIG_NOALT, 0, COMMAND_TO_RUN_MODE },
			{ 'S' | DEFSIG_EXT1 | DEFSIG_NOSHIFT | DEFSIG_CTRL | DEFSIG_NOALT, 0, COMMAND_CHANGE_FORMAT_MODE },
			{ 'C' | DEFSIG_EXT1 | DEFSIG_NOSHIFT | DEFSIG_CTRL | DEFSIG_NOALT, 0, COMMAND_OPEN_CONSOLE },
			{ 'M' | DEFSIG_EXT1 | DEFSIG_NOSHIFT | DEFSIG_CTRL | DEFSIG_NOALT, 0, COMMAND_OPEN_MONITOR },
			{ 'B' | DEFSIG_EXT1 | DEFSIG_NOSHIFT | DEFSIG_CTRL | DEFSIG_NOALT, 0, COMMAND_BINEDIT },
			{ 'T' | DEFSIG_EXT1 | DEFSIG_NOSHIFT | DEFSIG_CTRL | DEFSIG_NOALT, 0, COMMAND_TXTEDIT },
			{ 'F' | DEFSIG_EXT1 | DEFSIG_NOSHIFT | DEFSIG_NOCTRL | DEFSIG_ALT, 0, COMMAND_TO_FORMAT_MODE },
			{ 'R' | DEFSIG_EXT1 | DEFSIG_NOSHIFT | DEFSIG_NOCTRL | DEFSIG_ALT, 0, COMMAND_TO_RUN_MODE },
			{ 'S' | DEFSIG_EXT1 | DEFSIG_NOSHIFT | DEFSIG_NOCTRL | DEFSIG_ALT, 0, COMMAND_CHANGE_FORMAT_MODE },
			{ 'C' | DEFSIG_EXT1 | DEFSIG_NOSHIFT | DEFSIG_NOCTRL | DEFSIG_ALT, 0, COMMAND_OPEN_CONSOLE },
			{ 'M' | DEFSIG_EXT1 | DEFSIG_NOSHIFT | DEFSIG_NOCTRL | DEFSIG_ALT, 0, COMMAND_OPEN_MONITOR },
			{ 'B' | DEFSIG_EXT1 | DEFSIG_NOSHIFT | DEFSIG_NOCTRL | DEFSIG_ALT, 0, COMMAND_BINEDIT },
			{ 'T' | DEFSIG_EXT1 | DEFSIG_NOSHIFT | DEFSIG_NOCTRL | DEFSIG_ALT, 0, COMMAND_TXTEDIT },

#endif
			{ 0,                0, 0    }
		};
		struct KEY_TABLE1 *pkt;
		for (pkt = table; pkt->code; pkt++)
			lib_definesignal1p0(pkt->opt, 0x0100, pkt->code, selwin[0].window, pkt->signum);
		lib_definesignal0p0(0, 0, 0, 0);
	}

	for (i = 0; i < MAX_BANK; i++)
		banklist[i].size = banklist[i].tss = 0;

	for (i = 0; i < MAX_FILEBUF; i++) {
		filebuf[i].dirslot = -1;
		filebuf[i].linkcount = 0;
	}

	for (;;) {
		/* 全てのシグナルは、main()でやり取りする */
		sig = *sbp;
		win = selwin;
		if (sig < COMMAND_SIGNAL_START) {
			switch (sig) {
			case NO_SIGNAL:
				pcons->sleep = 1;
				lib_waitsignal(0x0001, 0, 0);
				continue;

			case SIGNAL_REWIND:
				gotsignal(*(sbp + 1));
				sbp = sb0;
				continue;

			case 98:
				gotsignal(1);
				sbp++;
				bootflags |= 0x02;
				if (bootflags == 0x03)
					goto bootflags_full;
				break;

			case SIGNAL_BOOT_COMPLETE:
				gotsignal(1);
				sbp++;
				bootflags |= 0x01;
				if (bootflags == 0x03) {
	bootflags_full:
					win[0].ext = ext_ALL;
					list_set(&win[0]);
					booting = 0;
				}
				break;

			case SIGNAL_TERMINATED_TASK:
				tss = sbp[1];
				gotsignal(2);
				sbp += 2;
				for (bank = banklist; bank->tss != tss; bank++);
				bank->size = bank->tss = 0;

				/* タスクtssへ通知しようと思っていたダイアログを全てクローズ */
				for (i = 1; i < MAX_SELECTOR; i++) {
					if (win[i].task == tss) {
						if (win[i].mdlslot > 0)
							win[i].mdlslot = -1;
						if (win[i].window != NULL && win[i].lp != NULL) {
							win[i].lp = NULL;
							lib_closewindow(0, win[i].window);
							win[i].mdlslot = -2;
						}
					}
				}
				/* 本来は、タスクを終了する前に、pokon0に通知があってしかるべき */
				/* そうでないと、終了したのにシグナルを送ってしまうという事が起こりうる */
				/* スロットのクローズを検出すれば、通知は可能 */

				for (fbuf = filebuf; fbuf->paddr != bank->addr || fbuf->linkcount <= 0; fbuf++);
				if (--fbuf->linkcount == 0) {
					sgg_freememory2(fbuf->dirslot, fbuf->size, fbuf->paddr);
					fbuf->dirslot = -1;
				}

				for (i = 0; i < MAX_ORDER; i++) {
					if (tss != order[i].task)
						continue;
					order[i].task = 0;
				}

		freefiles:
				for (i = 0; i < MAX_VMREF; i++) {
					if (tss != vmref[i].task)
						continue;
					j = vmref[i].module_paddr;
					vmref[i].task = 0;
					sgg_execcmd0(0x0074, 0, vmref[i].virtualmodule, 0x0000);
					for (fbuf = filebuf; fbuf->paddr != j || fbuf->linkcount <= 0; fbuf++);
						if (--fbuf->linkcount == 0) {
						sgg_freememory2(fbuf->dirslot, fbuf->size, fbuf->paddr);
						fbuf->dirslot = -1;
					}
				}
				break;

			case SIGNAL_REQUEST_DIALOG:
			case SIGNAL_REQUEST_DIALOG2:
				/* とりあえずバッファに入れておく */
				for (swait = selwait; swait->task; swait++); /* 行き過ぎる事は考えてない */
				selwaitcount++;
				swait->task    = sbp[1];
				swait->slot    = sbp[2];
				swait->bytes   = sbp[3];
				swait->ofs     = sbp[4];
				swait->sel     = sbp[5];
				gotsignal(6);
				sbp += 6;
				break;

			case SIGNAL_FREE_FILES:
				/* ファイル開放要求(えせファイルシステム用) */
				tss = sbp[1];
				gotsignal(2);
				sbp += 2;
				goto freefiles;

			case SIGNAL_RESIZE_FILE:
				/* ファイルサイズ変更要求 */
				/* cmd, virtualmodule, new-size, task, sig, slot */

				if (pjob->free >= 8 + 5 + 4) {
					/* 空きが十分にある */
					/* これ以降はかなりの手抜きがある(とりあえず動けばよいというレベル) */
					/* ディスクが交換されるかもしれないことに配慮していない */
					struct VIRTUAL_MODULE_REFERENCE *vmr;
					for (vmr = vmref; vmr->virtualmodule != sbp[1]; vmr++);
					for (fbuf = filebuf; fbuf->paddr != vmr->module_paddr || fbuf->linkcount <= 0; fbuf++);
					if ((i = fbuf->dirslot) != -1 && selwincount < MAX_SELECTOR && fbuf->linkcount == 1) {
						struct SGG_FILELIST *fp;
						for (fp = file + 1; fp->name[0]; fp++) {
							if (fp->id == i)
								goto resize_find;
						}
						goto resize_error;
	resize_find:
						for (win = &selwin[1]; win->task; win++);
						selwincount++;
						win->lp = win->list; /* window-close処理中でないことを明示 */
						win->task = sbp[3];
						win->mdlslot = sbp[5];
						win->siglen = 2;
						win->sig[1] = sbp[4];
						/* reloadのために、vmrefを捨てる */
						vmr->task = 0;
						sgg_execcmd0(0x0074, 0, vmr->virtualmodule, 0x0000);

						fbuf->linkcount = -1;
						fbuf->dirslot = -1;

						if (fbuf->size > 0) {
							writejob2(JOB_FREE_MEMORY, i); /* ライトバック＆メモリ開放 */
							writejob2(fbuf->size, fbuf->paddr);
						}

						writejob2(JOB_RESIZE_FILE, *((int *) &fp->name[0]));
						writejob2(*((int *) &fp->name[4]), (*((int *) &fp->name[8]) << 8) | '.');
						writejob2(sbp[2], 0 /* new-size, max-linkcount */);
						writejob2(0, 0 /* task, signal */);

						writejob2(JOB_LOAD_FILE, (int) fbuf);
						writejob2((int) win, win->task);
						writejob(i);

						*(pjob->wp) = 0; /* ストッパー */
						gotsignal(6);
						sbp += 6;
						break;
					}
				}
	resize_error:
				sendsignal1dw(sbp[3], sbp[4] + 1); /* error */
				gotsignal(6);
				sbp += 6;
				break;

			case SIGNAL_NEED_WB:
				/* ファイルキャッシュはライトバックが必要 */
				need_wb = -1;
		no_wb_cache2:
				gotsignal(2);
				sbp += 2;
				if (fmode == STATUS_RUN_APPLICATION) {
					for (i = 0; i < MAX_SELECTOR; i++) {
						if (win[i].window)
							lib_putstring_ASCII(0x0000, 0, 0, win[i].subtitle, need_wb & 9, 0, win[i].subtitle_str);
					}
				}
				break;

			case SIGNAL_NO_WB_CACHE:
				need_wb = 0;
				pjob->now = 0;
				goto no_wb_cache2;

			case SIGNAL_CHECK_WB_CACHE_NEXT:
				fbuf = (struct FILEBUF *) sbp[1];
				gotsignal(2);
				sbp += 2;
				if (check_wb_cache(fbuf) == NULL) /* 終了したらNULLを返す */
					pjob->now = 0;
				break;

			case SIGNAL_REFRESH_FLIST:
				if (pjob->param[6])
					sendsignal1dw(pjob->param[6], pjob->param[7] + (sbp[1] != 0));
				gotsignal(2);
				sbp += 2;
				goto refresh_selector;

			case SIGNAL_REFRESH_FLIST0:
				if (pjob->param[6])
					sendsignal1dw(pjob->param[6], pjob->param[7] + (sbp[1] != 0));
				gotsignal(2);
				sbp += 2;
				pjob->now = 0;
				break;

			case SIGNAL_RELOAD_FAT_COMPLETE:
				gotsignal(1);
				sbp++;
				for (i = 0; i < MAX_FILEBUF; i++)
					filebuf[i].dirslot = -1;
			refresh_selector:
				/* 全てのファイルセレクタを更新 */
				for (i = 0; i < MAX_SELECTOR; i++) {
					if (win[i].window)
						list_set(&win[i]);
				}
				pjob->now = 0;
				break;

			case SIGNAL_LOAD_APP_FILE_COMPLETE: /* ファイル読み込み完了(file load & execute) */
				fbuf = pjob->fbuf;
				bank = pjob->bank;
				fbuf->paddr = sbp[1];
				fbuf->size = sbp[2];
				gotsignal(3);
				sbp += 3;
				fbuf->linkcount = 0;
				if ((bank->size = fbuf->size) != -1 && pjob->free >= 2 + 8) {
					fbuf->linkcount = 1;
					fbuf->dirslot = pjob->dirslot;
					bank->addr = fbuf->paddr;
					/* 空きが十分にある */
					writejob2(JOB_CREATE_TASK, (int) bank);
					for (i = 0; i < 8; i++)
						writejob(pjob->param[i]);
					*(pjob->wp) = 0; /* ストッパー */
				}
				pjob->now = 0;
				break;

			case SIGNAL_CREATE_TASK_COMPLETE:
				bank = pjob->bank;
				bank->tss = tss = sbp[1];
				gotsignal(2);
				sbp += 2;
				if (tss == 0) {
					/* ロードした領域を解放 */
					for (fbuf = filebuf; fbuf->paddr != bank->addr; fbuf++);
					bank->size = 0;
					if (--fbuf->linkcount == 0) {
						sgg_freememory2(fbuf->dirslot, fbuf->size, fbuf->paddr);
						fbuf->dirslot = -1;
					}
				} else {
					sgg_settasklocallevel(tss,
						0 * 32 /* local level 0 (スリープレベル) */,
						27 * 64 + 0x0100 /* global level 27 (スリープ) */,
						-1 /* Inner level */
					);
					bank->Llv[0].global = 27;
					bank->Llv[0].inner  = -1;
					sgg_settasklocallevel(tss,
						1 * 32 /* local level 1 (起動・システム処理レベル) */,
						12 * 64 + 0x0100 /* global level 12 (一般アプリケーション) */,
						i = defaultIL /* Inner level */
					);
					bank->Llv[1].global = 12;
					bank->Llv[1].inner  = defaultIL;
					sgg_settasklocallevel(tss,
						2 * 32 /* local level 2 (通常処理レベル) */,
						12 * 64 + 0x0100 /* global level 12 (一般アプリケーション) */,
						i /* Inner level */
					);
					bank->Llv[2].global = 12;
					bank->Llv[2].inner  = i;
					sgg_runtask(tss, 1 * 32);
					if (pjob->param[7]) {
						/* tssとnumとfileidを登録する */
						for (i = 0; order[i].task; i++);
						order[i].task = tss;
						order[i].num = pjob->param[0];
						order[i].fileid = pjob->param[1];
						if (pjob->param[2]) {
							/* シグナルを送る */
							sendsignal1dw(tss, pjob->param[4]);
						}
					}
				}
				pjob->now = 0;
				break;

			case SIGNAL_LOAD_FILE_COMPLETE: /* ファイル読み込み完了(file load) */
				fbuf = pjob->fbuf;
				win = pjob->win;
				fbuf->paddr = sbp[1];
				fbuf->size = sbp[2];
				gotsignal(3);
				sbp += 3;
				fbuf->linkcount = 0;
				if (fbuf->size != -1) {
					fbuf->dirslot = pjob->dirslot;
					selwinfin(pjob->param[0], win, fbuf, vmref);
				} else {
					if (win->mdlslot != -1) {
						/* エラーシグナルを送信 */
						sendsignal1dw(pjob->param[0], win->sig[1] + 1);
					}
				}
				win->mdlslot = -2;
				if (win->lp /* NULLならクローズ処理中 */) {
					win->task = 0;
					selwincount--;
				}
				pjob->now = 0;
				break;

			case SIGNAL_LOAD_KERNEL_COMPLETE: /* ファイル読み込み完了(.EXE file load) */
				pjob->param[1] = sbp[2]; /* .EXE size */
				pjob->param[2] = sbp[1]; /* .EXE addr */
				gotsignal(3);
				sbp += 3;
				if (pjob->param[1] <= 0) {
					pjob->now = 0;
					break;
				}
				if ((i = pjob->param[3] = searchfid0("OSASKBS3BIN")) == 0) {
free_exe:
					sgg_freememory2(-1, pjob->param[1], pjob->param[2]);
					pjob->now = 0;
					break;
				}
				sgg_loadfile2(i /* file id */, SIGNAL_LOAD_BOOT_SECTOR_CODE_COMPLETE);
				break;

			case SIGNAL_LOAD_BOOT_SECTOR_CODE_COMPLETE: /* ファイル読み込み完了(.EXE file load) */
				pjob->param[4] = sbp[2]; /* BootSector size */
				pjob->param[5] = sbp[1]; /* BootSector addr */
				gotsignal(3);
				sbp += 3;
				if (pjob->param[4] <= 0)
					goto free_exe;
				for (i = 0; i < LIST_HEIGHT; i++)
					putselector0(win, i, "                ");
				putselector0(win, 1, "    Loaded.     ");
				putselector0(win, 3, " Change disks.  ");
				putselector0(win, 5, " Hit Enter key. ");
				fmode = STATUS_LOAD_BOOT_SECTOR_CODE_COMPLETE; /* 'S'と'Enter'と'F'と'R'しか入力できない */
				break;

			case SIGNAL_FORMAT_COMPLETE: /* フォーマット完了 */
				gotsignal(1);
				sbp++;
write_exe:
				fmode = STATUS_FORMAT_COMPLETE;
				putselector0(win, 1, " Writing        ");
				putselector0(win, 3, "   system image.");
				putselector0(win, 5, "  Please wait.  ");
				sgg_format2(0x0138 /* 1,440KBフォーマット用 圧縮 */,
					//	0x011c /* 1,760KBフォーマット用 非圧縮 */
					//	0x0128; /* 1,440KBフォーマット用 非圧縮 */
					pjob->param[4], pjob->param[5], pjob->param[1], pjob->param[2],
					SIGNAL_WRITE_KERNEL_COMPLETE /* finish signal */); // store system image
				break;

			case SIGNAL_WRITE_KERNEL_COMPLETE: /* .EXE書き込み完了 */
				gotsignal(1);
				sbp++;
				sgg_freememory2(-1, pjob->param[1], pjob->param[2]);
				sgg_freememory2(-1, pjob->param[4], pjob->param[5]);
				pjob->now = 0;
				fmode = STATUS_WRITE_KERNEL_COMPLETE;
				putselector0(win, 1, "   Completed.   ");
				putselector0(win, 3, " Change disks.  ");
				putselector0(win, 5, " Hit Alt+R key. ");
			//	break;
			}
		} else if (COMMAND_SIGNAL_START <= sig && sig < COMMAND_SIGNAL_END + 1) {
			/* selwin[0]に対する特別コマンド */
			gotsignal(1);
			sbp++;
			if (booting != 0 || fmode == STATUS_FORMAT_COMPLETE)
				continue; /* boot中は無視 */
			j = win[0].lp[win[0].cur].ptr->id;
			switch (sig) {
			case COMMAND_TO_FORMAT_MODE:
				j = 0;
				for (i = 1; i < MAX_SELECTOR; i++)
					j |= win[i].task;
				if (j != 0 || pcons->curx != -1 || need_wb != 0)
					break;
				if (fmode == STATUS_LOAD_BOOT_SECTOR_CODE_COMPLETE) {
					sgg_freememory2(-1, pjob->param[1], pjob->param[2]);
					sgg_freememory2(-1, pjob->param[4], pjob->param[5]);
					pjob->now = 0;
				}
				win[0].ext = ext_EXE;
				if (fmode == STATUS_WRITE_KERNEL_COMPLETE) {
				//	if (pjob->free >= 1) {
						/* 空きが十分にある */
						writejob(JOB_INVALID_DISKCACHE);
						*(pjob->wp) = 0; /* ストッパー */
						win->cur = -1;
				//	}
				} else {
					list_set(&win[0]);
				}
				fmode = STATUS_MAKE_PLAIN_BOOT_DISK;
				lib_putstring_ASCII(0x0000, 0, 0, win[0].subtitle, 0, 0, "< Load Systemimage >");
				break;

		/* フォーマットモードに入るには、他のファイルセレクタが全て閉じられていなければいけない */
		/* また、フォーマットモードに入っている間は、ファイルセレクタが開かない */

			case COMMAND_TO_RUN_MODE:
				if (fmode == STATUS_LOAD_BOOT_SECTOR_CODE_COMPLETE) {
					sgg_freememory2(-1, pjob->param[1], pjob->param[2]);
					sgg_freememory2(-1, pjob->param[4], pjob->param[5]);
					pjob->now = 0;
				}
				win[0].ext = ext_ALL;
				lib_putstring_ASCII(0x0000, 0, 0, win[0].subtitle, need_wb & 9, 0, "< Run Application > ");
				if (fmode == STATUS_WRITE_KERNEL_COMPLETE) {
				//	if (pjob->free >= 1) {
						/* 空きが十分にある */
						writejob(JOB_INVALID_DISKCACHE);
						*(pjob->wp) = 0; /* ストッパー */
						win->cur = -1;
				//	}
				} else {
					list_set(&win[0]);
				}
				fmode = STATUS_RUN_APPLICATION;
				/* 待機している要求があれば、ウィンドウを開く */
				break;

			case COMMAND_CHANGE_FORMAT_MODE:
				if (fmode == STATUS_MAKE_PLAIN_BOOT_DISK || fmode == STATUS_MAKE_COMPRESSED_BOOT_DISK) {
					fmode = STATUS_MAKE_PLAIN_BOOT_DISK + STATUS_MAKE_COMPRESSED_BOOT_DISK - fmode;
					lib_putstring_ASCII(0x0000, 0, 0, win[0].subtitle, (fmode - 1) * 9, 0, "< Load Systemimage >");
				} else if (fmode == STATUS_LOAD_BOOT_SECTOR_CODE_COMPLETE)
					goto write_exe;
				break;

			case COMMAND_OPEN_CONSOLE:
				if (pcons->curx == -1 && fmode == STATUS_RUN_APPLICATION)
					open_console();
				continue;

			case COMMAND_OPEN_MONITOR:
				continue;

			case COMMAND_BINEDIT:
				run_viewer(&BINEDIT, j);
				break;

			case COMMAND_TXTEDIT:
				run_viewer(&TXTEDIT, j);
				break;

		//	case 99:
		//		lib_putstring_ASCII(0x0000, 0, 0, mode,     0, 0, "< Error 99        > ");
		//		break;
			}
		} else if (CONSOLE_SIGNAL_START <= sig && sig < FILE_SELECTOR_SIGNAL_START) {
			/* console関係のシグナル */
			gotsignal(1);
			sbp++;
			if (CONSOLE_KEY_SIGNAL_START <= sig && sig <= CONSOLE_KEY_SIGNAL_END) {
				/* consoleへの1文字入力 */
				if (pcons->curx >= 0) {
					if (pcons->curx < CONSOLESIZEX - 1) {
						static char c[2] = { 0, 0 };
						c[0] = sig - CONSOLE_SIGNAL_START;
						consoleout(c);
					}
					if (pcons->cursoractive) {
						lib_settimer(0x0001, SYSTEM_TIMER);
						pcons->cursorflag = ~0;
						putcursor();
						lib_settimertime(0x0032, SYSTEM_TIMER, 0x80000000 /* 500ms */, 0, 0);
					}
				}
			} else {
				switch (sig) {
				case CONSOLE_VRAM_ACCESS_ENABLE /* VRAMアクセス許可 */:
					continue;

				case CONSOLE_VRAM_ACCESS_DISABLE /* VRAMアクセス禁止 */:
					lib_controlwindow(0x0100, pcons->win);
					continue;

				case CONSOLE_REDRAW_0:
				case CONSOLE_REDRAW_1:
					/* 再描画 */
					lib_controlwindow(0x0203, pcons->win);
					continue;

				case CONSOLE_CHANGE_TITLE_COLOR /* change console title color */:
					if (*sbp++ & 0x02) {
						if (!(pcons->cursoractive)) {
							lib_settimer(0x0001, SYSTEM_TIMER);
							pcons->cursoractive = 1;
							pcons->cursorflag = ~0;
							putcursor();
							lib_settimertime(0x0032, SYSTEM_TIMER, 0x80000000 /* 500ms */, 0, 0);
						}
					} else {
						if (pcons->cursoractive) {
							pcons->cursoractive = 0;
							pcons->cursorflag = 0;
							putcursor();
							lib_settimer(0x0001, SYSTEM_TIMER);
						}
					}
					gotsignal(1);
					continue;

				case CONSOLE_CLOSE_WINDOW /* close console window */:
					if (pcons->cursoractive) {
						pcons->cursoractive = 0;
						lib_settimer(0x0001, SYSTEM_TIMER);
					}
					lib_closewindow(0, pcons->win);
					pcons->curx = -1;
					continue;

				case CONSOLE_CURSOR_BLINK /* cursor blink */:
					if (pcons->sleep != 0 && pcons->cursoractive != 0) {
						pcons->cursorflag =~ pcons->cursorflag;
						putcursor();
						lib_settimertime(0x0012, SYSTEM_TIMER, 0x80000000 /* 500ms */, 0, 0);
					}
					continue;

				case CONSOLE_INPUT_ENTER /* consoleへのEnter入力 */:
					if (pcons->curx >= 0) {
						const char *p = pcons->buf + pcons->cury * (CONSOLESIZEX + 2) + 5;
						if (pcons->cursorflag != 0 && pcons->cursoractive != 0) {
							pcons->cursorflag = 0;
							putcursor();
						}
						while (*p == ' ')
							p++;
						if (*p) {
							int status = -ERR_BAD_COMMAND;
							static struct STR_POKON_CMDLIST {
								int (*fnc)(const char *);
								const char *cmd;
							} cmdlist[] = {
								poko_memory,		"memory",
								poko_color,			"color",
								poko_cls,			"cls",
								poko_mousespeed,	"mousespeed",
								poko_setdefaultIL,	"setdefaultIL",
								poko_tasklist,		"tasklist",
								poko_sendsignalU,	"sendsignalU",
								poko_LLlist,		"LLlist",
								poko_setIL,			"setIL",
								poko_create,		"create",
								poko_delete,		"delete",
								poko_rename,		"rename",
								poko_resize,		"resize",
								poko_nfname,		"nfname",
#if defined(DEBUG)
								poko_debug,			"debug",
#endif
								NULL,				NULL
							};
							struct STR_POKON_CMDLIST *pcmdlist = cmdlist;
							do {
								if (poko_cmdchk(p, pcmdlist->cmd)) {
									status = (*(pcmdlist->fnc))(p);
									break;
								}
							} while ((++pcmdlist)->fnc);
							if (status != 0) {
								consoleout(pokon_error_message[-status - ERR_CODE_START]);
							}
						}
				prompt:
						consoleout(POKO_PROMPT);
						if (pcons->cursoractive) {
							lib_settimer(0x0001, SYSTEM_TIMER);
							pcons->cursorflag = ~0;
							putcursor();
							lib_settimertime(0x0032, SYSTEM_TIMER, 0x80000000 /* 500ms */, 0, 0);
						}
					}
					break;

				case CONSOLE_INPUT_BACKSPACE /* consoleへのBackSpace入力 */:
					if (pcons->curx >= 0) {
						if (pcons->cursorflag != 0 && pcons->cursoractive != 0) {
							pcons->cursorflag = 0;
							putcursor();
						}
						if (pcons->curx > 5) {
							pcons->curx--;
							consoleout(" ");
							pcons->curx--;
						}
						if (pcons->cursoractive) {
							lib_settimer(0x0001, SYSTEM_TIMER);
							pcons->cursorflag = ~0;
							putcursor();
							lib_settimertime(0x0032, SYSTEM_TIMER, 0x80000000 /* 500ms */, 0, 0);
						}
					}
					continue;
				}
			}
		} else {
			/* ファイルセレクタへの一般シグナル */
			win = &selwin[(sig - FILE_SELECTOR_SIGNAL_START) >> 7];
			sig &= 0x7f;
			gotsignal(1);
			sbp++;
			if (fmode == STATUS_LOAD_BOOT_SECTOR_CODE_COMPLETE && sig == SIGNAL_ENTER) {
			//	putselector0(win, 1, " Writing        ");
				putselector0(win, 1, "  Formating...  ");
				putselector0(win, 3, "                ");
				sgg_format(0x0124 /* 1,440KBフォーマット */, SIGNAL_FORMAT_COMPLETE /* finish signal */); // format
				/* 1,760KBフォーマットと1,440KBフォーマットの混在モードは0x0118 */
				fmode = STATUS_FORMAT_COMPLETE;
				continue;
			}

			if (booting != 0 || fmode >= STATUS_LOAD_BOOT_SECTOR_CODE_COMPLETE)
				continue; /* boot中は無視 */
			if (win->window) {
				int cur = win->cur;
				struct FILELIST *lp = win->lp, *list = win->list;

				switch (sig) {
				case SIGNAL_CURSOR_UP:
					if (cur < 0 /* ファイルが１つもない */)
						continue;
					if (cur > 0) {
 						put_file(win, lp[cur].name, cur, 0);
						cur--;
 						put_file(win, lp[cur].name, cur, 1);
					} else if (lp > win->list) {
						lp--;
listup:
						for (i = 0; i < LIST_HEIGHT; i++) {
							if (i != cur)
								put_file(win, lp[i].name, i, 0);
							else
								put_file(win, lp[cur].name, cur, 1);
						}
					}
					break;

				case SIGNAL_CURSOR_DOWN:
				//	if (cur < 0 /* ファイルが１つもない */)
				//		break;
				//	ファイルがない場合、cur == -1 && lp[0].name[0] == '\0'
				//	なので、以下のifが成立しない。
					if (lp[cur + 1].name[0]) {
						if (cur < LIST_HEIGHT - 1) {
 							put_file(win, lp[cur].name, cur, 0);
							cur++;
	 						put_file(win, lp[cur].name, cur, 1);
						} else {
							lp++;
							goto listup;
						}
					}
					break;

				case SIGNAL_ENTER:
					if (cur < 0 /* ファイルが1つもない */)
						continue;
					if (win != selwin) { /* not pokon */
						/* ファイルをロードして、仮想モジュールを生成し、
							スロットを設定し、シグナルを発する */
						/* 最後に、ウィンドウを閉じる */
						if ((tss = win->task) == 0)
							break;
						if (fbuf = searchfbuf(lp[cur].ptr->id)) {
							win->lp = NULL;
							lib_closewindow(0, win->window);
							selwinfin(tss, win, fbuf, vmref);
							win->mdlslot = -2;
							break;
						}
						if ((fbuf = searchfrefbuf()) != NULL && pjob->free >= 5) {
							win->lp = NULL;
							lib_closewindow(0, win->window);
							/* 空きが十分にある */
							writejob2(JOB_LOAD_FILE, (int) fbuf);
							writejob2((int) win, tss);
							writejob(lp[cur].ptr->id);
							*(pjob->wp) = 0; /* ストッパー */
							bank->size = -1;
							fbuf->linkcount = -1;
						}
						break;
					}
					if (win->ext == ext_ALL) {
						/* ALLファイルモード */
						bank = banklist;
						for (i = 0; i < MAX_BANK; i++) {
							if (bank[i].size == 0)
								goto find_freebank;
						}
						break;
		find_freebank:
						bank += i;
						i = lp[cur].name[8] | lp[cur].name[9] << 8 | lp[cur].name[10] << 16;
						j = lp[cur].ptr->id;
						if (i == ('B' | 'I' << 8 | 'N' << 16)) {
							for (i = 0; i < 11; i++)
								bank->name[i] = lp[cur].name[i];
							bank->name[11] = '\0';
							if ((fbuf = searchfrefbuf()) == NULL)
								break;
							if (pjob->free >= 4) {
								/* 空きが十分にある */
								writejob2(JOB_LOAD_FILE_AND_EXECUTE, j);
								writejob2((int) fbuf, (int) bank);
								*(pjob->wp) = 0; /* ストッパー */
								bank->size = -1;
								fbuf->linkcount = -1;
							}
							break;
						}
						if (i == ('B' | 'M' << 8 | 'P' << 16)) {
							run_viewer(&PICEDIT, j);
							break;
						}
						if (i == ('T' | 'X' << 8 | 'T' << 16)) {
							run_viewer(&TXTEDIT, j);
							break;
						}
						run_viewer(&BINEDIT, j);
						break;
					}

					/* .EXEファイルモード */
					/* キャッシュチェックをしない */
					if (pjob->free >= 2) {
						/* 空きが十分にある */
						fmode = STATUS_FORMAT_COMPLETE;
						writejob2(JOB_LOAD_FILE_AND_FORMAT, lp[cur].ptr->id);
						*(pjob->wp) = 0; /* ストッパー */
					}
					break;

				case SIGNAL_PAGE_UP:
					if (cur < 0 /* ファイルが１つもない */)
						continue;
					if (lp >= list + LIST_HEIGHT)
						lp -= LIST_HEIGHT;
					else {
						lp = list;
						cur = 0;
					}
					goto listup;

				case SIGNAL_PAGE_DOWN:
					if (cur < 0 /* ファイルが１つもない */)
						continue;
					for (i = 1; lp[i].name[0] != '\0' && i < LIST_HEIGHT * 2; i++);
					if (i < LIST_HEIGHT) {
						// 全体が1画面分に満たなかった
						cur = i - 1;
						goto listup;
					} else if (i < LIST_HEIGHT * 2) {
						// 残りが1画面分に満たなかった
						lp += i - LIST_HEIGHT;
						cur = LIST_HEIGHT - 1;
						goto listup;
					}
					lp += LIST_HEIGHT;
					goto listup;

				case SIGNAL_TOP_OF_LIST:
					if (cur < 0 /* ファイルが１つもない */)
						break;
					lp = list;
					cur = 0;
					goto listup;

				case SIGNAL_BOTTOM_OF_LIST:
					if (cur < 0 /* ファイルが１つもない */)
						break;
					lp = list;
					for (i = 0; lp[i].name[0]; i++);
					if (i < LIST_HEIGHT) {
						// ファイル数が1画面分に満たなかった
						cur = i - 1;
					} else {
						lp += i - LIST_HEIGHT;
						cur = LIST_HEIGHT - 1;
					}
					goto listup;

				case SIGNAL_DISK_CHANGED:
					/* Insert */
					if (pjob->free >= 2) {
						/* 空きが十分にある */
						writejob2(JOB_CHECK_WB_CACHE, JOB_INVALID_DISKCACHE);
						*(pjob->wp) = 0; /* ストッパー */
					}
					break;

				case SIGNAL_START_WB:
					/* Delete */
					if (pjob->free >= 2) {
						/* 空きが十分にある */
						writejob2(JOB_CHECK_WB_CACHE, JOB_WRITEBACK_CACHE);
						*(pjob->wp) = 0; /* ストッパー */
					}
					break;

				case SIGNAL_FORCE_CHANGED:
					/* Shift+Insert */
					if (pjob->free >= 3) {
						/* 空きが十分にある */
						writejob2(JOB_CHECK_WB_CACHE, JOB_INVALID_WB_CACHE);
						writejob(JOB_INVALID_DISKCACHE);
						*(pjob->wp) = 0; /* ストッパー */
					}
					break;

				case SIGNAL_CHECK_WB_CACHE:
					/* Shift+Delete */
					if (pjob->free >= 1) {
						/* 空きが十分にある */
						writejob(JOB_CHECK_WB_CACHE);
						*(pjob->wp) = 0; /* ストッパー */
					}
					break;

				case SIGNAL_CREATE_NEW:
					if (pjob->free >= 6) {
						/* 空きが十分にある */
						writejob2(JOB_CREATE_FILE, 'N' | 'E' << 8 | 'W' << 16 | '_' << 24);
						writejob2('F' | 'I' << 8 | 'L' << 16 | 'E' << 24, '.' | ' ' << 8 | ' ' << 16 | ' ' << 24);
						writejob2(0, 0 /* task, signal */);
						*(pjob->wp) = 0; /* ストッパー */
					}
					break;

				case SIGNAL_DELETE_FILE:
					if (cur < 0 /* ファイルが1つもない */)
						continue;
					if (pjob->free >= 15) {
						/* 空きが十分にある */
						int name[3];
						char *cname = lp[cur].name;
						name[0] = cname[0] | cname[1] << 8 | cname[2] << 16 | cname[3] << 24;
						name[1] = cname[4] | cname[5] << 8 | cname[6] << 16 | cname[7] << 24;
						name[2] = '.' | cname[8] << 8 | cname[9] << 16 | cname[10] << 24;
						writejob2(JOB_RESIZE_FILE, name[0]);
						writejob2(name[1], name[2]);
						writejob2(0, 0 /* new-size, max-linkcount */);
						writejob2(0, 0 /* task, signal */);
						writejob2(JOB_DELETE_FILE, name[0]); /*  */
						writejob2(name[1], name[2]);
						writejob(0 /* max-linkcount */);
						writejob2(0, 0 /* task, signal */);
						*(pjob->wp) = 0; /* ストッパー */
					}
					break;

				case SIGNAL_RESIZE:
					run_viewer(&RESIZER, lp[cur].ptr->id);
					break;

				case SIGNAL_WINDOW_CLOSE0 /* close window */:
					if (i == 0)
						continue;
					/* キャンセルを通知して、閉じる */
					/* 待機しているものがあれば、応じる(応じるのは127を受け取ってからにする) */
					sendsignal1dw(win->task, win->sig[1] + 1 /* cancel */);
					win->lp = NULL;
					lib_closewindow(0, win->window);
					win->mdlslot = -2;
					continue;

				case SIGNAL_WINDOW_CLOSE1 /* closed window */:
					free(win->window);
					free(win->wintitle);
					free(win->subtitle);
					free(win->selector);
					win->window = NULL;
					win->lp = win->list;
					if (win->mdlslot == -2) {
						win->task = 0;
						selwincount--;
					}
					break;

				default: /* search filename */
					if (cur < 0 /* ファイルが１つもない */)
						break;
					/* search from current to bottom */
					lp = win->lp;
					for (i = cur + 1; lp[i].name[0]; i++) {
						if (lp[i].name[0] == sig) {
							cur = i;
							if (cur >= LIST_HEIGHT - 1) {
								if (lp[i+1].name[0] == '\0') {
								    i = cur - (LIST_HEIGHT - 1);
								    cur = LIST_HEIGHT - 1;
								    lp += i;
								} else {
								    i = cur - (LIST_HEIGHT - 2);
								    cur = LIST_HEIGHT - 2;/* 下から 2段目 */
								    lp += i;
								}
							}
							goto listup;
						}
					}
					/* search from top to current */
					if (lp[i].name[0] == '\0') {
						lp = list;
						for (i = 0; lp[i].name[0] && lp != win->lp + cur; i++) {
							if (lp[i].name[0] == sig) {
								cur = i;
								for (i = 1; i <= LIST_HEIGHT - 1; ++i) {
									if (lp[cur + i].name[0] == '\0')
										break;
								}
								j = LIST_HEIGHT - i;
								if (cur > j) {
									lp += cur - j;
									cur = j;
								}
								goto listup;
							}
						}
					}
					lp = win->lp;
				}
				win->cur = cur;
				win->lp = lp;
			}
		}

		if (booting)
			continue; /* boot中は無視 */

		while (selwaitcount != 0 && selwincount < MAX_SELECTOR && fmode == STATUS_RUN_APPLICATION) {
			static char t[24] = "< for ########     >";
			for (swait = selwait; swait->task == 0; swait++);
			for (win = &selwin[1]; win->task; win++);
			selwaitcount--;
			selwincount++;
			/* task, slot, num, siglen, sig[16]を取得 */
			win->lp = win->list; /* window-close処理中でないことを明示 */
			win->task = swait->task;
			win->mdlslot = swait->slot;
			sgg_debug00(0, swait->bytes, 0, swait->ofs,
				swait->sel | (win->task << 8) + 0xf80000, (const int) subcmd, 0x000c);
			swait->task = 0;
			if (subcmd[0] == 0xffffff01 /* num */) {
				win->num = subcmd[1];
				win->siglen = subcmd[3];
				win->sig[0] = subcmd[4];
				win->sig[1] = subcmd[5];
				for (i = 0; i < MAX_ORDER; i++) {
					if (order[i].task == win->task && order[i].num == win->num) {
						order[i].task = 0;
						i = order[i].fileid;
						goto open_redirect;
					}
				}
				for (bank = banklist; bank->size == 0 || bank->tss != win->task; bank++);
				for (i = 0; i < 8; i++)
					t[i + 6] = bank->name[i];
				openselwin(win, "(open)", t);
				win->ext = ext_ALL;
				list_set(win);
				continue;
			}
			if (subcmd[0] == 0xffffff03 /* direct-name */) {
				/* とりあえず、名前は12バイト固定(最後のバイトは00) */
				win->siglen = subcmd[6];
				win->sig[0] = subcmd[7];
				win->sig[1] = subcmd[8];
				if ((i = searchfid((unsigned char *) &subcmd[2])) == 0)
					goto error_sig;
				/* 見付かった */
open_redirect:
				if (fbuf = searchfbuf(i)) {
					selwinfin(win->task, win, fbuf, vmref);
					win->task = 0;
					selwincount--;
					continue;
				}
				if ((fbuf = searchfrefbuf()) != NULL && pjob->free >= 5) {
					/* 空きが十分にある */
					writejob2(JOB_LOAD_FILE, (int) fbuf);
					writejob2((int) win, win->task);
					writejob(i);
					*(pjob->wp) = 0; /* ストッパー */
					fbuf->linkcount = -1;
					continue;
				}
error_sig:
				/* エラーなのでslotを無効にしようかとも思ったが、そこまでやることもないよな */
#if 0
				k = (0x003c /* slot_sel */ | win->task << 8) + 0xf80000;
				sgg_directwrite(0, 4, 0, win->mdlslot, k, (int) &j, 0x000c);
#endif
				sendsignal1dw(win->task, win->sig[1] + 1);
				win->task = 0;
				selwincount--;
#if 0
				continue;
#endif
			}
		}

		if (pjob->now == 0)
			runjobnext();

	} /* for (;;) */
}

void open_monitor()
/*	モニターをオープンする */
{



}

void consoleout(char *s)
{
	struct STR_CONSOLE *pcons = &console;
	char *cbp = pcons->buf + pcons->cury * (CONSOLESIZEX + 2) + pcons->curx;
	while (*s) {
		pcons->buf[pcons->cury * (CONSOLESIZEX + 2) + (CONSOLESIZEX + 1)] = pcons->col;
		if (*s == '\n') {
			s++;
			lib_putstring_ASCII(0x0001, 0, pcons->cury,
				pcons->tbox, pcons->col & 0x0f, (pcons->col >> 4) & 0x0f,
				&((pcons->buf)[pcons->cury * (CONSOLESIZEX + 2) + 0]));
			pcons->cury++;
			pcons->curx = 0;
			if (pcons->cury == CONSOLESIZEY) {
				/* スクロールする */
				int i, j;
				char *bp = pcons->buf;
				bp[CONSOLESIZEY * (CONSOLESIZEX + 2) + (CONSOLESIZEX + 1)] = pcons->col;
				for (j = 0; j < CONSOLESIZEY; j++) {
					for (i = 0; i < CONSOLESIZEX + 2; i++)
						bp[i] = bp[i + (CONSOLESIZEX + 2)];
					lib_putstring_ASCII(0x0001, 0, j, pcons->tbox,
						bp[CONSOLESIZEX + 1] & 0x0f, (bp[CONSOLESIZEX + 1] >> 4) & 0x0f, bp);
					bp += CONSOLESIZEX + 2;
				}
				pcons->cury = CONSOLESIZEY - 1;
			}
			cbp = pcons->buf + pcons->cury * (CONSOLESIZEX + 2);
		} else {
			*cbp++ = *s++;
			pcons->curx++;
		}
	}
	lib_putstring_ASCII(0x0001, 0, pcons->cury,
		pcons->tbox, pcons->col & 0x0f, (pcons->col >> 4) & 0x0f,
		&((pcons->buf)[pcons->cury * (CONSOLESIZEX + 2) + 0]));
	return;
}

void open_console()
/*	コンソールをオープンする */
/*	カーソル点滅のために、setmodeも拾う */
/*	カーソル点滅のためのタイマーをイネーブルにする */
{
	struct STR_CONSOLE *pcons = &console;
	struct LIB_WINDOW *win = pcons->win;
	int i, j;
	char *bp;
	lib_openwindow1_nm(win, 0x0200, CONSOLESIZEX * 8, CONSOLESIZEY * 16, 0x0d, 256);
	lib_opentextbox_nm(0x1000, pcons->title, 0, 16,  1,  0,  0, win, 0x00c0, 0);
	lib_opentextbox_nm(0x0001, pcons->tbox,  0, CONSOLESIZEX, CONSOLESIZEY,  0,  0, win, 0x00c0, 0); // 5KB
	lib_putstring_ASCII(0x0000, 0, 0, pcons->title, 0, 0, POKON_VERSION" console");

	bp = pcons->buf;
	for (j = 0; j < CONSOLESIZEY + 1; j++) {
		for (i = 0; i < CONSOLESIZEX; i++) {
			*bp++ = ' ';
		}
		bp[0] = '\0';
		bp[1] = 0;
		bp += 2;
	}
	lib_definesignal1p0(0x7f - ' ', 0x0100, ' ',              win, CONSOLE_KEY_SIGNAL_START);
	lib_definesignal1p0(1,    0x0100, 0xa0 /* Enter */, win, CONSOLE_INPUT_ENTER);
	lib_definesignal0p0(0, 0, 0, 0);
	pcons->curx = pcons->cury = 0;
	pcons->col = 15;
	consoleout(POKO_VERSION);
	consoleout(POKO_PROMPT);
	if (pcons->cursoractive) {
		lib_settimer(0x0001, SYSTEM_TIMER);
		pcons->cursorflag = ~0;
		putcursor();
		lib_settimertime(0x0032, SYSTEM_TIMER, 0x80000000 /* 500ms */, 0, 0);
	}
	return;
}

const int console_getdec(const char **p)
{
	const char *cmdlin = *p;
	int dec = 0, mul = 10, sign = 1;
	while (*cmdlin == ' ')
		cmdlin++;
	if (*cmdlin == '+')
		cmdlin++;
	if (*cmdlin == '-') {
		cmdlin++;
		sign = -1;
	}
	if (*cmdlin == '0') {
		cmdlin++;
		if (*cmdlin == 'X' || *cmdlin == 'x') {
			mul = 16;
			cmdlin++;
		} else if (*cmdlin == 'B' || *cmdlin == 'b') {
			mul = 2;
			cmdlin++;
		} else if ('0' <= *cmdlin && *cmdlin <= '7') {
			mul = 8;
		}
	}
	for (;;) {
		unsigned char c = *cmdlin;
		if ('0' <= c && c <= '9')
			dec = dec * mul + c - '0';
		else if ('A' <= c && c <= 'F')
			dec = dec * mul + c - 'A' + 0xa;
		else if ('a' <= c && c <= 'f')
			dec = dec * mul + c - 'a' + 0xa;
		else if (c != '_')
			break;
		cmdlin++;
	}
	*p = cmdlin;
	return sign * dec;
}

int poko_memory(const char *cmdlin)
{
	static struct {
		int cmd, opt, mem20[4], mem24[4], mem32[4], eoc;
	} command = { 0x0034, 0, { 0 }, { 0 }, { 0 }, 0x0000 };
	char str[12];
	cmdlin += 6 /* "memory" */;
	while (*cmdlin == ' ')
		cmdlin++;
	if (*cmdlin) return -ERR_ILLEGAL_PARAMETERS;

	sgg_execcmd(&command);
	consoleout(       "\n20bit memory : ");
	itoa10(command.mem20[1 /* free */] >> 10, str);
	consoleout(str + 3);
	consoleout("KB free\n24bit memory : ");
	itoa10(command.mem24[1 /* free */] >> 10, str);
	consoleout(str + 3);
	consoleout("KB free\n32bit memory : ");
	itoa10(command.mem32[1 /* free */] >> 10, str);
	consoleout(str + 3);
	consoleout("KB free\n");
	return 0;
}

int poko_color(const char *cmdlin)
{
	int param0, param1 = console.col & 0xf0;
	cmdlin += 5 /* "color" */;
	while (*cmdlin == ' ')
		cmdlin++;
	if (*cmdlin == '\0') return -ERR_ILLEGAL_PARAMETERS;

	param0 = console_getdec(&cmdlin);
	while (*cmdlin == ' ')
		cmdlin++;
	if (*cmdlin) {
		param1 = console_getdec(&cmdlin) << 4;
		while (*cmdlin == ' ')
			cmdlin++;
		if (*cmdlin) return -ERR_ILLEGAL_PARAMETERS;
	}
	consoleout("\n");
	console.col = param0 | param1;
	return 0;
}

int poko_cls(const char *cmdlin)
{
	struct STR_CONSOLE *pcons = &console;
	int i, j;
	char *bp = pcons->buf;
	cmdlin += 3 /* "cls" */;
	while (*cmdlin == ' ')
		cmdlin++;
	if (*cmdlin) return -ERR_ILLEGAL_PARAMETERS;

	for (j = 0; j < CONSOLESIZEY; j++) {
		for (i = 0; i < CONSOLESIZEX + 2; i++)
			bp[i] = (pcons->buf)[i + (CONSOLESIZEX + 2) * CONSOLESIZEY];
		lib_putstring_ASCII(0x0001, 0, j, pcons->tbox,
			pcons->col & 0x0f, (pcons->col >> 4) & 0x0f, bp);
		bp += CONSOLESIZEX + 2;
	}
	pcons->cury = 0;
	return 0;
}

int poko_mousespeed(const char *cmdlin)
{
	int param;
	cmdlin += 10 /* "mousespeed" */;
	while (*cmdlin == ' ')
		cmdlin++;
	if (*cmdlin == '\0') {
#if 0
		char str[12], msg[5];
		itoa10(/* current mousespeed */, str);
		msg[0] = str[ 8]; /* 100の位 */
		msg[1] = str[ 9]; /*  10の位 */
		msg[2] = str[10]; /*   1の位 */
		msg[3] = '\n';
		msg[4] = '\0';
		consoleout(msg);
		return 0;
#else
		return -ERR_ILLEGAL_PARAMETERS;
#endif
	}

	param = console_getdec(&cmdlin);
	while (*cmdlin == ' ')
		cmdlin++;
	if (*cmdlin) return -ERR_ILLEGAL_PARAMETERS;

	sgg_wm0s_sendto2_winman0(0x6f6b6f70 + 0 /* mousespeed */, param);
	consoleout("\n");
	return 0;
}

int poko_setdefaultIL(const char *cmdlin)
{
	cmdlin += 12 /* "setdefaultIL" */;
	while (*cmdlin == ' ')
		cmdlin++;
	if (*cmdlin == '\0') return -ERR_ILLEGAL_PARAMETERS;

	defaultIL = console_getdec(&cmdlin);
	while (*cmdlin == ' ')
		cmdlin++;
	if (*cmdlin) return -ERR_ILLEGAL_PARAMETERS;
	consoleout("\n");
	return 0;
}

int poko_tasklist(const char *cmdlin)
{
	char str[12];
	static char msg[] = "000 name----\n";
	int i, j;
	struct STR_BANK *bank = banklist;
	cmdlin += 8 /* "tasklist" */;
	while (*cmdlin == ' ')
		cmdlin++;
	if (*cmdlin) return -ERR_ILLEGAL_PARAMETERS;
	consoleout("\n");
	for (i = 0; i < MAX_BANK; i++) {
		if (bank[i].size == 0)
			continue;
		itoa10(bank[i].tss >> 12, str);
		msg[0] = str[ 8]; /* 100の位 */
		msg[1] = str[ 9]; /*  10の位 */
		msg[2] = str[10]; /*   1の位 */
		for (j = 0; j < 8; j++)
			msg[4 + j] = bank[i].name[j];
		consoleout(msg);
	}
	return 0;
}

int poko_sendsignalU(const char *cmdlin)
{
	int task, sig;

	cmdlin += 11 /* "sendsignalU" */;
	while (*cmdlin == ' ')
		cmdlin++;
	if (*cmdlin == '\0') return -ERR_ILLEGAL_PARAMETERS;

	task = console_getdec(&cmdlin) * 4096 + 0x0242;
	while (*cmdlin == ' ')
		cmdlin++;
	if (*cmdlin == '\0') return -ERR_ILLEGAL_PARAMETERS;
	sig = console_getdec(&cmdlin);
	while (*cmdlin == ' ')
		cmdlin++;
	if (*cmdlin) return -ERR_ILLEGAL_PARAMETERS;
	sendsignal1dw(task, sig);
	consoleout("\n");
	return 0;
}

int poko_LLlist(const char *cmdlin)
{
	int task, i;
	struct STR_BANK *bank = banklist;
	cmdlin += 6 /* "LLlist" */;
	while (*cmdlin == ' ')
		cmdlin++;
	if (*cmdlin == '\0') return -ERR_ILLEGAL_PARAMETERS;
	task = console_getdec(&cmdlin) * 4096;
	for (i = 0; i < MAX_BANK; i++) {
		if (bank[i].size == 0)
			continue;
		if (bank[i].tss == task)
			goto find;
	}
	return -ERR_ILLEGAL_PARAMETERS;

find:
	bank += i;
	while (*cmdlin == ' ')
		cmdlin++;
	if (*cmdlin) return -ERR_ILLEGAL_PARAMETERS;
	consoleout("\nLL GL   IL\n");
	for (i = 0; i < 3; i++) {
		int global;
		char msg[16], str[16];
		msg[ 0] = i + '0';
		msg[ 1] = ' ';
		itoa10(global = bank->Llv[i].global, str);
		msg[ 2] = str[ 8];
		msg[ 3] = str[ 9];
		msg[ 4] = str[10];
		msg[ 5] = ' ';
		msg[ 6] = '-';
		msg[ 7] = '-';
		msg[ 8] = '-';
		msg[ 9] = '-';
		if (global == 12) {
			itoa10(bank->Llv[i].inner, str);
			msg[ 6] = str[ 7];
			msg[ 7] = str[ 8];
			msg[ 8] = str[ 9];
			msg[ 9] = str[10];
		}
		msg[10] = '\n';
		msg[11] = '\0';
		consoleout(msg);
	}
	return 0;
}

int poko_setIL(const char *cmdlin)
{
	int task, i, l;
	struct STR_BANK *bank = banklist;
	cmdlin += 5 /* "setIL" */;
	while (*cmdlin == ' ')
		cmdlin++;
	if (*cmdlin == '\0') return -ERR_ILLEGAL_PARAMETERS;
	task = console_getdec(&cmdlin) * 4096;
	for (i = 0; i < MAX_BANK; i++) {
		if (bank[i].size == 0)
			continue;
		if (bank[i].tss == task)
			goto find;
	}
	return -ERR_ILLEGAL_PARAMETERS;

find:
	bank += i;
	l = console_getdec(&cmdlin);
	while (*cmdlin == ' ')
		cmdlin++;
	if (*cmdlin != '\0' || l == 0) return -ERR_ILLEGAL_PARAMETERS;
	
	sgg_settasklocallevel(task,
		1 * 32 /* local level 1 (起動・システム処理レベル) */,
		12 * 64 + 0x0100 /* global level 12 (一般アプリケーション) */,
		l /* Inner level */
	);
	bank->Llv[1].global = 12;
	bank->Llv[1].inner  = l;
	sgg_settasklocallevel(task,
		2 * 32 /* local level 2 (通常処理レベル) */,
		16 * 64 /* global level 16 (一般アプリケーション) */,
		l /* Inner level */
	);
	bank->Llv[2].global = 12;
	bank->Llv[2].inner  = l;
	consoleout("\n");
	return 0;
}

const char *pokosub_getfilename(const char *p, char *s)
{
	int i, c;
	while (*p <= ' ' && *p != '\0')
		p++;
	s[0] = '\0';
	for (i = 1; i < 12; i++)
		s[i] = ' ';
	s[8] = '.';
	for (i = 0; i < 12; i++, p++) {
		c = *p;
		if (c <= ' ')
			break;
		if (c == '.')
			i = 8;
		if ('a' <= c && c <= 'z')
			c += 'A' - 'a';
		s[i] = c;
	}
	while (*p > ' ')
		p++;
	while (*p <= ' ' && *p != '\0')
		p++;
	return p;
}

int poko_create(const char *cmdlin)
{
	union {
		char s[12];
		int i[3];
	} filename = { 0 };
	struct STR_JOBLIST *pjob = &job;

	cmdlin = pokosub_getfilename(cmdlin + 6 /* "create" */, filename.s);
	if (filename.s[0] == '\0' || *cmdlin != '\0') return -ERR_ILLEGAL_PARAMETERS;
	if (pjob->free >= 6) {
		/* 空きが十分にある */
		writejob2(JOB_CREATE_FILE, filename.i[0]);
		writejob2(filename.i[1], filename.i[2]);
		writejob2(0, 0 /* task, signal */);
		*(pjob->wp) = 0; /* ストッパー */
	}
	consoleout("\n");
	return 0;
}

int poko_delete(const char *cmdlin)
{
	union {
		char s[12];
		int i[3];
	} filename = { 0 };
	struct STR_JOBLIST *pjob = &job;
	cmdlin = pokosub_getfilename(cmdlin + 6 /* "delete" */, filename.s);
	if (filename.s[0] == '\0' || *cmdlin != '\0') return -ERR_ILLEGAL_PARAMETERS;
	if (pjob->free >= 15) {
		/* 空きが十分にある */
		writejob2(JOB_RESIZE_FILE, filename.i[0]);
		writejob2(filename.i[1], filename.i[2]);
		writejob2(0, 0 /* new-size, max-linkcount */);
		writejob2(0, 0 /* task, signal */);
		writejob2(JOB_DELETE_FILE, filename.i[0]); /*  */
		writejob2(filename.i[1], filename.i[2]);
		writejob(0 /* max-linkcount */);
		writejob2(0, 0 /* task, signal */);
		*(pjob->wp) = 0; /* ストッパー */
	}
	consoleout("\n");
	return 0;
}

int poko_rename(const char *cmdlin)
{
	union {
		char s[12];
		int i[3];
	} filename0 = { 0 }, filename1 = { 0 };
	struct STR_JOBLIST *pjob = &job;
	cmdlin = pokosub_getfilename(cmdlin + 6 /* "rename" */, filename0.s);
	cmdlin = pokosub_getfilename(cmdlin, filename1.s);
	if (filename0.s[0] == '\0' || filename1.s[0] == '\0' || *cmdlin != '\0') return -ERR_ILLEGAL_PARAMETERS;
	if (pjob->free >= 9) {
		/* 空きが十分にある */
		writejob2(JOB_RENAME_FILE, filename0.i[0]);
		writejob2(filename0.i[1], filename0.i[2]);
		writejob2(filename1.i[0], filename1.i[1]);
		writejob(filename1.i[2]);
		writejob2(0, 0 /* task, signal */);
		*(pjob->wp) = 0; /* ストッパー */
	}
	consoleout("\n");
	return 0;
}

int poko_resize(const char *cmdlin)
{
	union {
		char s[12];
		int i[3];
	} filename = { 0 };
	struct STR_JOBLIST *pjob = &job;
	int size;
	cmdlin = pokosub_getfilename(cmdlin + 6 /* "resize" */, filename.s);
	if (filename.s[0] == '\0' || *cmdlin == '\0') return -ERR_ILLEGAL_PARAMETERS;
	size = console_getdec(&cmdlin);
	while (*cmdlin == ' ')
		cmdlin++;
	if (*cmdlin != '\0') return -ERR_ILLEGAL_PARAMETERS;
	if (pjob->free >= 8) {
		/* 空きが十分にある */
		writejob2(JOB_RESIZE_FILE, filename.i[0]);
		writejob2(filename.i[1], filename.i[2]);
		writejob2(size, 0 /* new-size, max-linkcount */);
		writejob2(0, 0 /* task, signal */);
		*(pjob->wp) = 0; /* ストッパー */
	}
	consoleout("\n");
	return 0;
}

int poko_nfname(const char *cmdlin)
{
	union {
		char s[12];
		int i[3];
	} filename = { 0 };
	struct STR_JOBLIST *pjob = &job;
	cmdlin = pokosub_getfilename(cmdlin + 6 /* "nfname" */, filename.s);
	if (filename.s[0] == '\0' || *cmdlin != '\0') return -ERR_ILLEGAL_PARAMETERS;
	if (pjob->free >= 9) {
		/* 空きが十分にある */
		writejob2(JOB_RENAME_FILE, 'N' | 'E' << 8 | 'W' << 16 | '_' << 24);
		writejob2('F' | 'I' << 8 | 'L' << 16 | 'E' << 24, '.' | ' ' << 8 | ' ' << 16 | ' ' << 24);
		writejob2(filename.i[0], filename.i[1]);
		writejob(filename.i[2]);
		writejob2(0, 0 /* task, signal */);
		*(pjob->wp) = 0; /* ストッパー */
	}
	consoleout("\n");
	return 0;
}

struct STR_BANK *run_viewer(struct STR_VIEWER *viewer, int fid)
{
	int i;
	struct STR_JOBLIST *pjob = &job;
	struct FILEBUF *fbuf;
	struct STR_BANK *bank = banklist;

	for (i = 0; i < MAX_BANK; i++) {
		if (bank[i].size == 0)
			goto find_freebank;
	}
error:
	return NULL;

find_freebank:
	bank += i;
	for (i = 0; i < 12; i++)
		bank->name[i] = viewer->binary[i];
//	bank->name[11] = '\0';
	if ((i = searchfid0(viewer->binary)) == NULL)
		goto error;
	if ((fbuf = searchfrefbuf()) == NULL)
		goto error;
	if (pjob->free >= 10) {
		/* 空きが十分にある */
		writejob2(JOB_VIEW_FILE, i);
		writejob2((int) fbuf, (int) bank);
		writejob2(1 /* num */, fid); /* 待機するやつ */
		writejob2(viewer->signal[0], viewer->signal[1]); /* シグナルボックスに溜め込むやつ */
		writejob2(viewer->signal[2], viewer->signal[3]);
		*(pjob->wp) = 0; /* ストッパー */
		bank->size = -1;
		fbuf->linkcount = -1;
		return bank;
	}
	return NULL;
}

#if 0

void poko_puthex2(int i)
{
	char str[3];
	str[0] = (i >> 4)   + '0';
	str[1] = (i & 0x0f) + '0';
	str[2] = '\0';
	if (str[0] > '9')
		str[0] += 'A' - 10 - '0';
	if (str[1] > '9')
		str[1] += 'A' - 10 - '0';
	consoleout(str);
	return;
}

int poko_debug(const char *cmdlin)
{
	static unsigned char *base = 0;
	int ofs;
	cmdlin += 6 /* "debug " */;
	while (*cmdlin == ' ')
		cmdlin++;
	if (*cmdlin == '\0') return -ERR_ILLEGAL_PARAMETERS;
	ofs = console_getdec(&cmdlin);
	while (*cmdlin == ' ')
		cmdlin++;
	if (*cmdlin) {
		poko_illegal_parameter_error();
		return;
	}
	consoleout("\n");
	poko_puthex2(base[ofs + 0]); consoleout(" ");
	poko_puthex2(base[ofs + 1]); consoleout(" ");
	poko_puthex2(base[ofs + 2]); consoleout(" ");
	poko_puthex2(base[ofs + 3]); consoleout(" ");
	poko_puthex2(base[ofs + 4]); consoleout(" ");
	poko_puthex2(base[ofs + 5]); consoleout(" ");
	poko_puthex2(base[ofs + 6]); consoleout(" ");
	poko_puthex2(base[ofs + 7]); consoleout("\n");
	return 0;
}

#endif
