/* "pokon0.c":アプリケーションラウンチャー  ver.1.7
     copyright(C) 2001 川合秀実, 小柳雅明
    stack:4k malloc:76k file:0 */

#include <guigui00.h>
#include <sysgg00.h>
/* sysggは、一般のアプリが利用してはいけないライブラリ
   仕様もかなり流動的 */
#include <stdlib.h>

#define	AUTO_MALLOC			0
#define	NULL				0
#define	SYSTEM_TIMER		0x01c0
#define LIST_HEIGHT			8
#define ext_EXE				('E' | ('X' << 8) | ('E' << 16))
#define ext_BIN				('B' | ('I' << 8) | ('N' << 16))
#define	CONSOLESIZEX		40
#define	CONSOLESIZEY		15
#define	MAX_BANK			56
#define	MAX_FILEBUF			64
#define	MAX_SELECTOR		 5
#define	MAX_SELECTORWAIT	64
#define	MAX_VMREF			64
#define	JOBLIST_SIZE		16

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

static struct STR_JOBLIST {
	int *list, free, *rp, *wp, now;
	int param[8];
	struct FILEBUF *fbuf;
	struct STR_BANK *bank;
	struct FILESELWIN *win;
} job;

struct VIRTUAL_MODULE_REFERENCE {
	int task, module_paddr;
};

struct STR_BANK *banklist;
struct SGG_FILELIST *file;
struct FILEBUF *filebuf;
static int defaultIL = 2;
struct FILESELWIN *selwin;
struct SELECTORWAIT *selwait;
int selwaitcount = 0, selwincount = 1;

static struct STR_CONSOLE {
	int curx, cury, col;
	struct LIB_WINDOW *win;
	unsigned char *buf;
	struct LIB_TEXTBOX *tbox, *title;
	int sleep, cursorflag, cursoractive;
} console = { -1, 0, 0, NULL, NULL, NULL, NULL, 0, 0, 0 };

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

#define	sgg_createtask2(size, addr, fin_sig) \
	sgg_execcmd0(0x0020, 0x80000000 + 8, 0x1247, 0x0130, (int) (size), \
		(int) (addr), 0x4243 /* to pokon0 */, 0x7f000002, (int) (fin_sig), \
		0, 0x0000)

void runjobnext()
{
	struct STR_JOBLIST *pjob = &job;
//	int param[8], i;
	do {
		if ((pjob->now = *(pjob->rp)) == 0)
			return;

		readjob(); // から読み
		switch (pjob->now) {

		case 0x0004 /* invalid diskcache */:
			sgg_format(0x0114, 0x00a0 /* finish signal */);
			break;

		case 0x0008 /* file load & execute */:
			pjob->fbuf = (struct FILEBUF *) readjob(); /* fbuf */
			pjob->bank = (struct STR_BANK *) readjob(); /* bank */
			sgg_loadfile2(pjob->fbuf->dirslot /* file id */, 0x00a4 /* finish signal */);
			break;

		case 0x000c /* create task */:
			pjob->bank = (struct STR_BANK *) readjob(); /* bank */
			sgg_createtask2(pjob->bank->size, pjob->bank->addr, 0x00a8 /* finish signal */);
			break;

		case 0x0010  /* file load */:
			pjob->fbuf = (struct FILEBUF *) readjob(); /* fbuf */
			pjob->win = (struct FILESELWIN *) readjob(); /* win */
			pjob->param[0] = readjob(); /* tss */
			sgg_loadfile2(pjob->fbuf->dirslot /* file id */, 0x00ac /* finish signal */);
			break;

		case 0x0014 /* file load & format (1) */:
			pjob->param[0] = readjob(); /* fileid */
			sgg_loadfile2(pjob->param[0] /* file id */, 0x00b0 /* finish signal */);
		//	break;
		}
	} while (pjob->now == 0);
	return;
}

void consoleout(char *s);
void open_console();

void sgg_freememory2(const int size, const int addr)
{
	sgg_execcmd0(0x0020, 0x80000000 + 4, 0x1243, 0x0134, size, addr, 0x0000);
	return;
}

#define	sgg_format2(sub_cmd, bsc_size, bsc_addr, exe_size, exe_addr, sig) \
	sgg_execcmd0(0x0020, 0x80000000 + 10, 0x1249, (int) (sub_cmd), 0, \
		(int) (bsc_size), (int) (bsc_addr), (int) (exe_size), \
		(int) (exe_addr), 0x4242 /* to pokon0 */, 0x7f000001, (int) (sig), \
		0x0000)

#define sgg_directwrite(opt, bytes, reserve, src_ofs, src_sel, dest_ofs, dest_sel) \
	sgg_execcmd0(0x0078, (int) (opt), (int) (bytes), (int) (reserve), \
		(int) (src_ofs), (int) (src_sel), (int) (dest_ofs), (int) (dest_sel), \
		0x0000)

#define	sgg_createvirtualmodule(size, addr) \
	(int) sgg_execcmd1(3 * 4 + 12, 0x0070, 0, (int) (size), 0, (int) (addr), \
	0, 0x0000)

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
			&& ((fp->name[8] | (fp->name[9] << 8) | (fp->name[10] << 16)) == ext || ext == -1)) {
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

void poko_memory(const char *cmdlin);
void poko_color(const char *cmdlin);
void poko_cls(const char *cmdlin);
void poko_mousespeed(const char *cmdlin);
void poko_setdefaultIL(const char *cmdlin);
void poko_tasklist(const char *cmdlin);
void poko_sendsignalU(const char *cmdlin);
void poko_LLlist(const char *cmdlin);
void poko_setIL(const char *cmdlin);
void poko_debug(const char *cmdlin);

void sgg_wm0s_sendto2_winman0(const int signal, const int param);

void openselwin(struct FILESELWIN *win, const char *title, const char *subtitle)
{
	static struct KEY_TABLE0 {
		int code;
		unsigned char opt, signum;
	} table[] = {
		{ 0x00ae /* cursor-up, down */,    1,  4 },
		{ 0x00a0 /* Enter */,              0,  6 },
		{ 0x00a8 /* page-up, down */,      1, 10 },
		{ 0x00ac /* cursor-left, right */, 1, 10 },
		{ 0x00a6 /* Home, End */,          1, 12 },
		{ 0x00a4 /* Insert */,             0, 14 },
		{ 0,                               0,  0 }
	};
	struct KEY_TABLE0 *pkt;
	struct LIB_WINDOW *window;

	window = win->window = lib_openwindow1(AUTO_MALLOC, win->winslot, 160, 40 + LIST_HEIGHT * 16, 0x28, win->sigbase + 120 /* +6 */);
	win->wintitle = lib_opentextbox(0x1000, AUTO_MALLOC,  0, 10, 1,  0,  0, window, 0x00c0, 0);
	win->subtitle = lib_opentextbox(0x0000, AUTO_MALLOC,  0, 20, 1,  0,  0, window, 0x00c0, 0);
	win->selector = lib_opentextbox(0x0001, AUTO_MALLOC, 15, 16, 8, 16, 32, window, 0x00c0, 0);
	lib_putstring_ASCII(0x0000, 0, 0, win->wintitle, 0, 0, title);
	lib_putstring_ASCII(0x0000, 0, 0, win->subtitle, 0, 0, subtitle);

	for (pkt = table; pkt->code; pkt++)
		lib_definesignal1p0(pkt->opt, 0x0100, pkt->code, window, win->sigbase + pkt->signum);
	lib_definesignal0p0(0, 0, 0, 0);
	return;
}

//void fileselect(struct FILESELWIN *win, int fileid);

void sendsignal1dw(int task, int sig)
{
	sgg_execcmd0(0x0020, 0x80000000 + 3, task | 0x0242, 0x7f000001, sig, 0x0000);
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
	int i;
	if (win->mdlslot == -1) {
		if (fbuf->linkcount == 0) {
			sgg_freememory2(fbuf->size, fbuf->paddr);
			fbuf->dirslot = -1;
		}
		return;
	}
	fbuf->linkcount++;
	for (; vmr->task; vmr++);
	vmr->task = task;
	vmr->module_paddr = fbuf->paddr;
	i = sgg_createvirtualmodule(fbuf->size, fbuf->paddr);
	sgg_directwrite(0, 4, 0, win->mdlslot,
		(0x003c /* slot_sel */ | task << 8) + 0xf80000, (int) &i, 0x000c);
	sendsignal1dw(task, win->sig[1]);
	return;
}

int searchfid(const unsigned char *s)
{
	struct SGG_FILELIST *fp;
	unsigned char c;
	int i;

	for (fp = file + 1; fp->name[0]; fp++) {
		c = 0;
		for (i = 0; i < 8; i++)
			c |= s[i] - fp->name[i];
		for (i = 0; i < 3; i++)
			c |= s[i + 9] - fp->name[i + 8];
		if (c == 0)
			return fp->id;
	}
	return 0;
}

void main()
{
	struct FILESELWIN *win;
	struct STR_JOBLIST *pjob = &job;
	struct STR_CONSOLE *pcons = &console;
	struct VIRTUAL_MODULE_REFERENCE *vmref;

	int i, j, sig, *sb0, *sbp, tss, booting = 1, fmode = 0;
	int *subcmd;
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
	subcmd = (int *) malloc(256 * sizeof (int));

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

	openselwin(&selwin[0], "pokon17", "< Run Application > ");

	lib_opentimer(SYSTEM_TIMER);
	lib_definesignal1p0(0, 0x0010 /* timer */, SYSTEM_TIMER, 0, 287);

	/* キー操作を追加登録 */
	{
		static struct KEY_TABLE1 {
			int code;
			unsigned char opt, signum;
		} table[] = {
			{ 'F' | 0x00701000, 0, 0xc0 },
			{ 'R' | 0x00701000, 0, 0xc1 },
			{ 'S' | 0x00701000, 0, 0xc2 },
			{ 'C' | 0x00701000, 0, 0xc3 },
			{ 'M' | 0x00701000, 0, 0xc4 },
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
		if (sig < 0x00c0) {
			switch (sig) {
			case 0x0000 /* no signal */:
				pcons->sleep = 1;
				lib_waitsignal(0x0001, 0, 0);
				continue;

			case 0x0004 /* rewind */:
				gotsignal(*(sbp + 1));
				sbp = sb0;
				continue;

			case     99 /* boot完了 */:
				gotsignal(1);
				sbp++;
				win[0].ext = ext_BIN;
				list_set(&win[0]);
				booting = 0;
				break;

			case 0x0080 /* terminated task */:
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

				for (fbuf = filebuf; fbuf->paddr != bank->addr; fbuf++);
				if (--fbuf->linkcount == 0) {
					sgg_freememory2(fbuf->size, fbuf->paddr);
					fbuf->dirslot = -1;
				}
				for (i = 0; i < MAX_VMREF; i++) {
					if (tss != vmref[i].task)
						continue;
					j = vmref[i].module_paddr;
					vmref[i].task = 0;
					for (fbuf = filebuf; fbuf->paddr != j; fbuf++);
						if (--fbuf->linkcount == 0) {
						sgg_freememory2(fbuf->size, fbuf->paddr);
						fbuf->dirslot = -1;
					}
				}
				break;

			case 0x0084 /* ダイアログ要求シグナル */:
			case 0x0088:
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

			case 0x00a0 /* FAT再読み込み完了(Insert) */:
				gotsignal(1);
				sbp++;
				for (i = 0; i < MAX_FILEBUF; i++)
					filebuf[i].dirslot = -1;
				/* 全てのファイルセレクタを更新 */
				for (i = 0; i < MAX_SELECTOR; i++) {
					if (win[i].window)
						list_set(&win[i]);
				}
				pjob->now = 0;
				break;

			case 0x00a4 /* ファイル読み込み完了(file load & execute) */:
				fbuf = pjob->fbuf;
				bank = pjob->bank;
				fbuf->size = sbp[1];
				fbuf->paddr = sbp[2];
				gotsignal(3);
				sbp += 3;
				fbuf->linkcount = 0;
				if ((bank->size = fbuf->size) != 0 && pjob->free >= 2) {
					fbuf->linkcount = 1;
					bank->addr = fbuf->paddr;
					/* 空きが十分にある */
				//	writejob(0x000c /* create task */);
				//	writejob((int) bank);
					writejob2(0x000c /* create task */, (int) bank);
					*(pjob->wp) = 0; /* ストッパー */
				}
				pjob->now = 0;
				break;

			case 0x00a8 /* タスク生成完了(create task) */:
				bank = pjob->bank;
				bank->tss = tss = sbp[1];
				gotsignal(2);
				sbp += 2;
				if (tss == 0) {
					/* ロードした領域を解放 */
					for (fbuf = filebuf; fbuf->paddr != bank->addr; fbuf++);
					bank->size = 0;
					if (--fbuf->linkcount == 0) {
						sgg_freememory2(fbuf->size, fbuf->paddr);
						fbuf->dirslot = -1;
					}
				} else {
					sgg_settasklocallevel(tss,
						0 * 32 /* local level 1 (スリープレベル) */,
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
				}
				pjob->now = 0;
				break;

			case 0x00ac /* ファイル読み込み完了(file load) */:
				fbuf = pjob->fbuf;
				win = pjob->win;
				fbuf->size = sbp[1];
				fbuf->paddr = sbp[2];
				gotsignal(3);
				sbp += 3;
				fbuf->linkcount = 0;
				if (fbuf->size)
					selwinfin(pjob->param[0], win, fbuf, vmref);
				else {
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

			case 0x00b0 /* ファイル読み込み完了(.EXE file load) */:
				pjob->param[0] = sbp[1]; /* .EXE size */
				pjob->param[1] = sbp[2]; /* .EXE addr */
				gotsignal(3);
				sbp += 3;
				if (pjob->param[0] == 0) {
					pjob->now = 0;
					break;
				}
				if ((i = searchfid("OSASKBS3.BIN")) == 0) {
free_exe:
					sgg_freememory2(pjob->param[0], pjob->param[1]);
					pjob->now = 0;
					break;
				}
				sgg_loadfile2(i /* file id */, 0x00b4 /* finish signal */);
				break;

			case 0x00b4 /* ファイル読み込み完了(.EXE file load) */:
				pjob->param[2] = sbp[1]; /* BootSector size */
				pjob->param[3] = sbp[2]; /* BootSector addr */
				gotsignal(3);
				sbp += 3;
				if (pjob->param[2] == 0)
					goto free_exe;
				for (i = 0; i < LIST_HEIGHT; i++)
					putselector0(win, i, "                ");
				putselector0(win, 1, "    Loaded.     ");
				putselector0(win, 3, " Change disks.  ");
				putselector0(win, 5, " Hit Enter key. ");
				fmode = 7; /* 'S'と'Enter'と'F'と'R'しか入力できない */
				break;

			case 0x00b8 /* フォーマット完了 */:
				gotsignal(1);
				sbp++;
write_exe:
				fmode = 9;
				putselector0(win, 1, " Writing        ");
				putselector0(win, 3, "   system image.");
				putselector0(win, 5, "  Please wait.  ");
				sgg_format2(0x0138 /* 1,440KBフォーマット用 圧縮 */,
					//	0x011c /* 1,760KBフォーマット用 非圧縮 */
					//	0x0128; /* 1,440KBフォーマット用 非圧縮 */
					pjob->param[2], pjob->param[3], pjob->param[0], pjob->param[1],
					0x00bc /* finish signal */); // store system image
				break;

			case 0x00bc /* .EXE書き込み完了 */:
				gotsignal(1);
				sbp++;
				sgg_freememory2(pjob->param[0], pjob->param[1]);
				sgg_freememory2(pjob->param[2], pjob->param[3]);
				pjob->now = 0;
				fmode = 8;
				putselector0(win, 1, "   Completed.   ");
				putselector0(win, 3, " Change disks.  ");
				putselector0(win, 5, "  Hit 'R' key.  ");
			//	break;
			}
		} else if (0x00c0 <= sig && sig < 256) {
			/* selwin[0]に対する特別コマンド */
			gotsignal(1);
			sbp++;
			if (booting != 0 || fmode == 9)
				continue; /* boot中は無視 */
			switch (sig) {
			case 0xc0 /* to format-mode */:
				j = 0;
				for (i = 1; i < MAX_SELECTOR; i++)
					j |= win[i].task;
				if (j != 0 || pcons->curx != -1)
					break;
				if (fmode == 7) {
					sgg_freememory2(pjob->param[0], pjob->param[1]);
					sgg_freememory2(pjob->param[2], pjob->param[3]);
					pjob->now = 0;
				}
				win[0].ext = ext_EXE;
				if (fmode == 8) {
				//	if (pjob->free >= 1) {
						/* 空きが十分にある */
						writejob(0x0004 /* invalid diskcache */);
						*(pjob->wp) = 0; /* ストッパー */
						win->cur = -1;
				//	}
				} else {
					list_set(&win[0]);
				}
				fmode = 1;
				lib_putstring_ASCII(0x0000, 0, 0, win[0].subtitle, 0, 0, "< Load Systemimage >");
				break;

		/* フォーマットモードに入るには、他のファイルセレクタが全て閉じられていなければいけない */
		/* また、フォーマットモードに入っている間は、ファイルセレクタが開かない */

			case 0xc1 /* to run-mode */:
				if (fmode == 7) {
					sgg_freememory2(pjob->param[0], pjob->param[1]);
					sgg_freememory2(pjob->param[2], pjob->param[3]);
					pjob->now = 0;
				}
				win[0].ext = ext_BIN;
				lib_putstring_ASCII(0x0000, 0, 0, win[0].subtitle,     0, 0, "< Run Application > ");
				if (fmode == 8) {
				//	if (pjob->free >= 1) {
						/* 空きが十分にある */
						writejob(0x0004 /* invalid diskcache */);
						*(pjob->wp) = 0; /* ストッパー */
						win->cur = -1;
				//	}
				} else {
					list_set(&win[0]);
				}
				fmode = 0;
				/* 待機している要求があれば、ウィンドウを開く */
				break;

			case 0xc2 /* change format-mode */:
				if (fmode == 1 || fmode == 2) {
					fmode = 3 - fmode;
					lib_putstring_ASCII(0x0000, 0, 0, win[0].subtitle, (fmode - 1) * 9, 0, "< Load Systemimage >");
				} else if (fmode == 7)
					goto write_exe;
				break;

			case 0xc3 /* open console */:
				if (pcons->curx == -1 && fmode == 0)
					open_console();
				continue;

			case 0xc4 /* open monitor */:
				continue;

		//	case 99:
		//		lib_putstring_ASCII(0x0000, 0, 0, mode,     0, 0, "< Error 99        > ");
		//		break;
			}
		} else if (256 <= sig && sig < 512) {
			/* console関係のシグナル */
			gotsignal(1);
			sbp++;
			if (256 + ' ' <= sig && sig <= 256 + 0x7f) {
				/* consoleへの1文字入力 */
				if (pcons->curx >= 0) {
					if (pcons->curx < CONSOLESIZEX - 1) {
						static char c[2] = { 0, 0 };
						c[0] = sig - 256;
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
				case 256 + 0 /* VRAMアクセス許可 */:
					continue;

				case 256 + 1 /* VRAMアクセス禁止 */:
					lib_controlwindow(0x0100, pcons->win);
					continue;

				case 256 + 2:
				case 256 + 3:
					/* 再描画 */
					lib_controlwindow(0x0203, pcons->win);
					continue;

				case 256 + 5 /* change console title color */:
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

				case 256 + 6 /* close console window */:
					if (pcons->cursoractive) {
						pcons->cursoractive = 0;
						lib_settimer(0x0001, SYSTEM_TIMER);
					}
					lib_closewindow(0, pcons->win);
					pcons->curx = -1;
					continue;

				case 287 /* cursor blink */:
					if (pcons->sleep != 0 && pcons->cursoractive != 0) {
						pcons->cursorflag =~ pcons->cursorflag;
						putcursor();
						lib_settimertime(0x0012, SYSTEM_TIMER, 0x80000000 /* 500ms */, 0, 0);
					}
					continue;

				case 256 + 0xa0 /* consoleへのEnter入力 */:
					if (pcons->curx >= 0) {
						const char *p = pcons->buf + pcons->cury * (CONSOLESIZEX + 2) + 5;
						if (pcons->cursorflag != 0 && pcons->cursoractive != 0) {
							pcons->cursorflag = 0;
							putcursor();
						}
						while (*p == ' ')
							p++;
						if (*p) {
							static struct STR_POKON_CMDLIST {
								void (*fnc)(const char *);
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
							//	poko_debug,			"debug",
								NULL,				NULL
							};
							struct STR_POKON_CMDLIST *pcmdlist = cmdlist;
							do {
								if (poko_cmdchk(p, pcmdlist->cmd)) {
									(*(pcmdlist->fnc))(p);
									goto prompt;
								}
							} while ((++pcmdlist)->fnc);
							consoleout("\nBad command.\n");
						}
				prompt:
						consoleout("\npoko>");
						if (pcons->cursoractive) {
							lib_settimer(0x0001, SYSTEM_TIMER);
							pcons->cursorflag = ~0;
							putcursor();
							lib_settimertime(0x0032, SYSTEM_TIMER, 0x80000000 /* 500ms */, 0, 0);
						}
					}
					break;

				case 256 + 0xa1 /* consoleへのBackSpace入力 */:
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
			win = &selwin[(sig - 512) >> 7];
			sig &= 0x7f;
			gotsignal(1);
			sbp++;
			if (fmode == 7 && sig == 6) {
				putselector0(win, 1, " Writing        ");
				putselector0(win, 1, "  Formating...  ");
				putselector0(win, 3, "                ");
				sgg_format(0x0124 /* 1,440KBフォーマット */, 0x00b8 /* finish signal */); // format
				/* 1,760KBフォーマットと1,440KBフォーマットの混在モードは0x0118 */
				fmode = 9;
				continue;
			}

			if (booting != 0 || fmode >= 7)
				continue; /* boot中は無視 */
			if (win->window) {
				int cur = win->cur;
				struct FILELIST *lp = win->lp, *list = win->list;

				switch (sig) {
				case 4 /* cursor-up */:
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

				case 5 /* cursor-down */:
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

				case 6 /* Enter */:
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
						if ((fbuf = searchfrefbuf()) != NULL && pjob->free >= 3) {
							win->lp = NULL;
							lib_closewindow(0, win->window);
							/* 空きが十分にある */
						//	writejob(0x0010 /* file load */);
						//	writejob((int) fbuf);
						//	writejob((int) win);
						//	writejob(tss);
							writejob2(0x0010 /* file load */, (int) fbuf);
							writejob2((int) win, tss);
							*(pjob->wp) = 0; /* ストッパー */
							bank->size = -1;
							fbuf->linkcount = -1;
							fbuf->dirslot = lp[cur].ptr->id;
						}
						break;
					}
					if (win->ext == ext_BIN) {
						/* .BINファイルモード */
						bank = banklist;
						for (i = 0; i < MAX_BANK; i++) {
							if (bank[i].size == 0)
								goto find_freebank;
						}
						break;
		find_freebank:
						bank += i;
						for (i = 0; i < 11; i++)
							bank->name[i] = lp[cur].name[i];
						bank->name[11] = '\0';
						if (fbuf = searchfbuf(lp[cur].ptr->id)) {
							if (pjob->free >= 2) {
								/* 空きが十分にある */
								bank->size = fbuf->size;
								bank->addr = fbuf->paddr;
								fbuf->linkcount++;
							//	writejob(0x000c /* create task */);
							//	writejob((int) bank);
								writejob2(0x000c /* create task */, (int) bank);
								*(pjob->wp) = 0; /* ストッパー */
							}
							break;
						}
						if ((fbuf = searchfrefbuf()) == NULL)
							break;
						if (pjob->free >= 3) {
							/* 空きが十分にある */
							writejob(0x0008 /* file load & execute */);
						//	writejob((int) fbuf);
						//	writejob((int) bank);
							writejob2((int) fbuf, (int) bank);
							*(pjob->wp) = 0; /* ストッパー */
							bank->size = -1;
							fbuf->linkcount = -1;
							fbuf->dirslot = lp[cur].ptr->id;
						}
						break;
					}

					/* .EXEファイルモード */
					/* キャッシュチェックをしない */
					if (pjob->free >= 2) {
						/* 空きが十分にある */
						fmode = 9;
					//	writejob(0x0014 /* file load & format (1) */);
					//	writejob(lp[cur].ptr->id);
						writejob2(0x0014 /* file load & format (1) */, lp[cur].ptr->id);
						*(pjob->wp) = 0; /* ストッパー */
					}
					break;

				case 10 /* page-up */:
					if (cur < 0 /* ファイルが１つもない */)
						continue;
					if (lp >= list + LIST_HEIGHT)
						lp -= LIST_HEIGHT;
					else {
						lp = list;
						cur = 0;
					}
					goto listup;

				case 11 /* page-down */:
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

				case 12 /* Home */:
					if (cur < 0 /* ファイルが１つもない */)
						break;
					lp = list;
					cur = 0;
					goto listup;

				case 13 /* End */:
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

				case 14 /* Insert */:
					if (pjob->free >= 1) {
						/* 空きが十分にある */
						writejob(0x0004 /* invalid diskcache */);
						*(pjob->wp) = 0; /* ストッパー */
					}
					break;

				case 126 /* close window */:
					if (i == 0)
						continue;
					/* キャンセルを通知して、閉じる */
					/* 待機しているものがあれば、応じる(応じるのは127を受け取ってからにする) */
					sendsignal1dw(win->task, win->sig[1] + 1 /* cancel */);
					win->lp = NULL;
					lib_closewindow(0, win->window);
					win->mdlslot = -2;
					continue;

				case 127 /* closed window */:
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
				}
				win->cur = cur;
				win->lp = lp;
			}
		}

		if (booting)
			continue; /* boot中は無視 */

		while (selwaitcount != 0 && selwincount < MAX_SELECTOR && fmode == 0) {
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
				for (bank = banklist; bank->size == 0 || bank->tss != win->task; bank++);
				for (j = 0; j < 8; j++)
					t[j + 6] = bank->name[j];
				openselwin(win, "(open)", t);
				win->ext = -1;
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
				if (fbuf = searchfbuf(i)) {
					selwinfin(win->task, win, fbuf, vmref);
					win->task = 0;
					selwincount--;
					continue;
				}
				if ((fbuf = searchfrefbuf()) != NULL && pjob->free >= 3) {
					/* 空きが十分にある */
				//	writejob(0x0010 /* file load */);
				//	writejob((int) fbuf);
				//	writejob((int) win);
				//	writejob(win->task);
					writejob2(0x0010 /* file load */, (int) fbuf);
					writejob2((int) win, win->task);
					*(pjob->wp) = 0; /* ストッパー */
					fbuf->linkcount = -1;
					fbuf->dirslot = i;
					continue;
				}
error_sig:
				/* エラーなのでslotを無効にしようかとも思ったが、そこまでやることもないよな */
			//	k = (0x003c /* slot_sel */ | win->task << 8) + 0xf80000;
			//	sgg_directwrite(0, 4, 0, win->mdlslot, k, (int) &j, 0x000c);
				sendsignal1dw(win->task, win->sig[1] + 1);
				win->task = 0;
				selwincount--;
			//	continue;
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
	lib_putstring_ASCII(0x0000, 0, 0, pcons->title, 0, 0, "pokon17 console");

	bp = pcons->buf;
	for (j = 0; j < CONSOLESIZEY + 1; j++) {
		for (i = 0; i < CONSOLESIZEX; i++) {
			*bp++ = ' ';
		}
		bp[0] = '\0';
		bp[1] = 0;
		bp += 2;
	}
	lib_definesignal1p0(0x5f, 0x0100, ' ',              win, 256 + ' ');
	lib_definesignal1p0(1,    0x0100, 0xa0 /* Enter */, win, 256 + 0xa0);
	lib_definesignal0p0(0, 0, 0, 0);
	pcons->curx = pcons->cury = 0;
	pcons->col = 15;
	consoleout("Heppoko-shell \"poko\" version 1.9\n    Copyright (C) 2001 H.Kawai(Kawaido)\n");
	consoleout("\npoko>");
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

void poko_memory(const char *cmdlin)
{
	static struct {
		int cmd, opt, mem20[4], mem24[4], mem32[4], eoc;
	} command = { 0x0034, 0, { 0 }, { 0 }, { 0 }, 0x0000 };
	char str[12];
	cmdlin += 6 /* "memory" */;
	while (*cmdlin == ' ')
		cmdlin++;
	if (*cmdlin) {
		consoleout("\nIllegal parameter(s).\n");
		return;
	}
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
	return;
}

void poko_color(const char *cmdlin)
{
	int param0, param1 = console.col & 0xf0;
	cmdlin += 5 /* "color" */;
	while (*cmdlin == ' ')
		cmdlin++;
	if (*cmdlin == '\0') {
		consoleout("\nIllegal parameter(s).\n");
		return;
	}
	param0 = console_getdec(&cmdlin);
	while (*cmdlin == ' ')
		cmdlin++;
	if (*cmdlin) {
		param1 = console_getdec(&cmdlin) << 4;
		while (*cmdlin == ' ')
			cmdlin++;
		if (*cmdlin) {
			consoleout("\nIllegal parameter(s).\n");
			return;
		}
	}
	consoleout("\n");
	console.col = param0 | param1;
	return;
}

void poko_cls(const char *cmdlin)
{
	struct STR_CONSOLE *pcons = &console;
	int i, j;
	char *bp = pcons->buf;
	cmdlin += 3 /* "cls" */;
	while (*cmdlin == ' ')
		cmdlin++;
	if (*cmdlin) {
		consoleout("\nIllegal parameter(s).\n");
		return;
	}
	for (j = 0; j < CONSOLESIZEY; j++) {
		for (i = 0; i < CONSOLESIZEX + 2; i++)
			bp[i] = (pcons->buf)[i + (CONSOLESIZEX + 2) * CONSOLESIZEY];
		lib_putstring_ASCII(0x0001, 0, j, pcons->tbox,
			pcons->col & 0x0f, (pcons->col >> 4) & 0x0f, bp);
		bp += CONSOLESIZEX + 2;
	}
	pcons->cury = 0;
	return;
}

void poko_mousespeed(const char *cmdlin)
{
	int param;
	cmdlin += 10 /* "mousespeed" */;
	while (*cmdlin == ' ')
		cmdlin++;
	if (*cmdlin == '\0') {
		consoleout("\nIllegal parameter(s).\n");
		return;
	}
	param = console_getdec(&cmdlin);
	while (*cmdlin == ' ')
		cmdlin++;
	if (*cmdlin) {
		consoleout("\nIllegal parameter(s).\n");
		return;
	}
	sgg_wm0s_sendto2_winman0(0x6f6b6f70 + 0 /* mousespeed */, param);
	consoleout("\n");
	return;
}

void poko_setdefaultIL(const char *cmdlin)
{
	cmdlin += 12 /* "setdefaultIL" */;
	while (*cmdlin == ' ')
		cmdlin++;
	if (*cmdlin == '\0') {
		consoleout("\nIllegal parameter(s).\n");
		return;
	}
	defaultIL = console_getdec(&cmdlin);
	while (*cmdlin == ' ')
		cmdlin++;
	if (*cmdlin) {
		consoleout("\nIllegal parameter(s).\n");
		return;
	}
	consoleout("\n");
	return;
}

void poko_tasklist(const char *cmdlin)
{
	char str[12];
	static char msg[] = "000 name----\n";
	int i, j;
	struct STR_BANK *bank = banklist;
	cmdlin += 8 /* "tasklist" */;
	while (*cmdlin == ' ')
		cmdlin++;
	if (*cmdlin) {
		consoleout("\nIllegal parameter(s).\n");
		return;
	}
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
	return;
}

void poko_sendsignalU(const char *cmdlin)
{
//	static struct {
//		int cmd, opt;
//		int data[3];
//		int eoc;
//	} command = { 0x0020, 0x80000000 + 3, { 0, 0x7f000001, 0 }, 0x0000 };

	int task, sig;

	cmdlin += 11 /* "sendsignalU" */;
	while (*cmdlin == ' ')
		cmdlin++;
	if (*cmdlin == '\0') {
		consoleout("\nIllegal parameter(s).\n");
		return;
	}
	task = console_getdec(&cmdlin) * 4096 + 0x0242;
	while (*cmdlin == ' ')
		cmdlin++;
	if (*cmdlin == '\0') {
		consoleout("\nIllegal parameter(s).\n");
		return;
	}
	sig = console_getdec(&cmdlin);
	while (*cmdlin == ' ')
		cmdlin++;
	if (*cmdlin) {
		consoleout("\nIllegal parameter(s).\n");
		return;
	}
//	sgg_execcmd(&command);
	sendsignal1dw(task, sig);
	consoleout("\n");
	return;
}

void poko_LLlist(const char *cmdlin)
{
	int task, i;
	struct STR_BANK *bank = banklist;
	cmdlin += 6 /* "LLlist" */;
	while (*cmdlin == ' ')
		cmdlin++;
	if (*cmdlin == '\0') {
		consoleout("\nIllegal parameter(s).\n");
		return;
	}
	task = console_getdec(&cmdlin) * 4096;
	for (i = 0; i < MAX_BANK; i++) {
		if (bank[i].size == 0)
			continue;
		if (bank[i].tss == task)
			goto find;
	}
	consoleout("\nIllegal parameter(s).\n");
	return;

find:
	bank += i;
	while (*cmdlin == ' ')
		cmdlin++;
	if (*cmdlin) {
		consoleout("\nIllegal parameter(s).\n");
		return;
	}
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
	return;
}

void poko_setIL(const char *cmdlin)
{
	int task, i, l;
	struct STR_BANK *bank = banklist;
	cmdlin += 5 /* "setIL" */;
	while (*cmdlin == ' ')
		cmdlin++;
	if (*cmdlin == '\0') {
		consoleout("\nIllegal parameter(s).\n");
		return;
	}
	task = console_getdec(&cmdlin) * 4096;
	for (i = 0; i < MAX_BANK; i++) {
		if (bank[i].size == 0)
			continue;
		if (bank[i].tss == task)
			goto find;
	}
	consoleout("\nIllegal parameter(s).\n");
	return;

find:
	bank += i;
	l = console_getdec(&cmdlin);
	while (*cmdlin == ' ')
		cmdlin++;
	if (*cmdlin != '\0' || l == 0) {
		consoleout("\nIllegal parameter(s).\n");
		return;
	}
	
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
	return;
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

void poko_debug(const char *cmdlin)
{
	static unsigned char *base = 0;
	int ofs;
	cmdlin += 6 /* "debug " */;
	while (*cmdlin == ' ')
		cmdlin++;
	if (*cmdlin == '\0') {
		consoleout("\nIllegal parameter(s).\n");
		return;
	}
	ofs = console_getdec(&cmdlin);
	while (*cmdlin == ' ')
		cmdlin++;
	if (*cmdlin) {
		consoleout("\nIllegal parameter(s).\n");
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
	return;
}

#endif

/*
	フォーマット以外は全て完璧。
*/
