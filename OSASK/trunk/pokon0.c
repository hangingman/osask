/* "pokon0.c":アプリケーションラウンチャー  ver.1.5
     copyright(C) 2001 川合秀実, 小柳雅明
    stack:4k malloc:72k file:0 */

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

struct VIRTUAL_MODULE_REFERENCE {
	int task, module_paddr;
} *vmref;

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

struct STR_BANK *banklist;
struct SGG_FILELIST *file;
struct LIB_TEXTBOX *console_txt;
struct LIB_WINDOW *console_win = NULL;
struct FILEBUF *filebuf;
static unsigned char *consolebuf = NULL;
static int console_curx, console_cury, console_col, defaultIL = 2, fmode = 0;
struct FILESELWIN *selwin;
struct SELECTORWAIT *selwait;
int selwaitcount = 0, selwincount = 1;
void consoleout(char *s);
void open_console();

void sgg_loadfile2(const int file_id, const int fin_sig)
/* シグナルで、sizeとaddrを返す */
{
	static struct {
		int cmd, opt, data[8];
		int eoc;
	} command = {
		0x0020, 0x80000000 + 8,
		0x1247, 0x012c, 0, 0x4244 /* to pokon0 */, 0x7f000003, 0, 0, 0,
		0x0000
	};
	command.data[2] = file_id;
	command.data[5] = fin_sig;
	sgg_execcmd(&command);
	return;
}

void sgg_createtask2(const int size, const int addr, const int fin_sig)
{
	static struct {
		int cmd, opt, data[8];
		int eoc;
	} command = {
		0x0020, 0x80000000 + 8,
		0x1247, 0x0130, 0, 0, 0x4243 /* to pokon0 */, 0x7f000002, 0, 0,
		0x0000
	};
	command.data[2] = size;
	command.data[3] = addr;
	command.data[6] = fin_sig;
	sgg_execcmd(&command);
	return;
}

void sgg_freememory2(const int size, const int addr)
{
	static struct {
		int cmd, opt, data[4];
		int eoc;
	} command = {
		0x0020, 0x80000000 + 4,
		0x1243, 0x0134, 0, 0,
		0x0000
	};
	command.data[2] = size;
	command.data[3] = addr;
	sgg_execcmd(&command);
	return;
}

void sgg_format2(const int sub_cmd, const int bsc_size, const int bsc_addr,
	const int exe_size, const int exe_addr, const int sig)
{
	static struct {
		int cmd, opt, data[10];
		int eoc;
	} command = {
		0x0020, 0x80000000 + 10,
		0x1249, 0, 0, 0, 0, 0, 0, 0x4242 /* to pokon0 */, 0x7f000001, 0,
		0x0000
	};

	command.data[1] = sub_cmd;
	command.data[3] = bsc_size;
	command.data[4] = bsc_addr;
	command.data[5] = exe_size;
	command.data[6] = exe_addr;
	command.data[9] = sig;

	sgg_execcmd(&command);
	return;
}

void sgg_directwrite(const int opt, const int bytes, const int reserve,
	const int src_ofs, const int src_sel, const int dest_ofs, const int dest_sel)
{
	static struct {
		int cmd, opt, bytes, reserve;
		int src_ofs, src_sel, dest_ofs, dest_sel;
		int eoc;
	} command = { 0x0078, 0, 0, 0, 0, 0, 0, 0, 0x0000 };

	command.opt = opt;
	command.bytes = bytes;
	command.reserve = reserve;
	command.src_ofs = src_ofs;
	command.src_sel = src_sel;
	command.dest_ofs = dest_ofs;
	command.dest_sel = dest_sel;

	sgg_execcmd(&command);
	return;
}

const int sgg_createvirtualmodule(const int size, const int addr)
{
	static struct {
		int cmd, opt, size, addr, addr64l, addr64h, eoc;
	} command = { 0x0070, 0, 0, 0, 0, 0, 0x0000 };

	command.size = size;
	command.addr64l = addr;

	sgg_execcmd(&command);
	return command.addr;
}

void putselector0(struct FILESELWIN *win, const int pos, const char *str)
{
	lib_putstring_ASCII(0x0000, 0, pos, win->selector, 0, 0, str);
	return;
}

void put_file(struct FILESELWIN *win, const char *name, const int pos, const int col)
{
	static char charbuf16[17] = "          .     ";
	if (*name) {
		charbuf16[2] = name[0];
		charbuf16[3] = name[1];
		charbuf16[4] = name[2];
		charbuf16[5] = name[3];
		charbuf16[6] = name[4];
		charbuf16[7] = name[5];
		charbuf16[8] = name[6];
		charbuf16[9] = name[7];
	//	charbuf16[10] = '.';
		charbuf16[11] = name[ 8];
		charbuf16[12] = name[ 9];
		charbuf16[13] = name[10];
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

int *sb0, *sbp, notasksignal = 0;
char sleep = 0, cursorflag = 0, cursoractive = 0;

const int wait99sub()
/* 画面更新を伴わないシグナルはここで全て処理してしまう */
{
	for (;;) {
		int sig = *sbp;
		if (sig == 0) {
			sleep = 1;
			lib_waitsignal(0x0001, 0, 0);
			return 0; /* 目覚めたら一度0を返す */
		} else if (sig == 1) {
			lib_waitsignal(0x0000, *(sbp + 1), 0);
			sbp = sb0;
		} else if (sig == 0x0080) {
			// タスク終了
			int i, j, tss;
			tss = sbp[1];
			lib_waitsignal(0x0000, 2, 0);
			sbp += 2;
			for (i = 0; banklist[i].tss != tss; i++);
			banklist[i].size = banklist[i].tss = 0;

			/* タスクtssへ通知しようと思っていたダイアログを全てクローズ */
			for (j = 1; j < MAX_SELECTOR; j++) {
				if (selwin[j].task == tss && selwin[j].window != NULL) {
					selwin[j].task = -1;
					lib_closewindow(0, selwin[j].window);
				}
			}
			/* 本来は、タスクを終了する前に、pokon0に通知があってしかるべき */
			/* そうでないと、終了したのにシグナルを送ってしまうという事が起こりうる */

			j = banklist[i].addr;
			for (i = 0; filebuf[i].paddr != j; i++);
			if (--filebuf[i].linkcount == 0) {
				sgg_freememory2(filebuf[i].size, j);
				filebuf[i].dirslot = -1;
			}
			for (i = 0; i < MAX_VMREF; i++) {
				if (tss != vmref[i].task)
					continue;
				j = vmref[i].module_paddr;
				vmref[i].task = 0;
				for (i = 0; filebuf[i].paddr != j; i++);
				if (--filebuf[i].linkcount == 0) {
					sgg_freememory2(filebuf[i].size, j);
					filebuf[i].dirslot = -1;
				}
			}
#if 0
check_notask:
			if (notasksignal) {
				int j = 0;
				for (i = 0; i < MAX_BANK; i++)
					j |= banklist[i].tss;
				if (j == 0) {
					static struct {
						int cmd, opt;
						int data[2];
						int eoc;
					} command = { 0x0020, 0x80000000 + 2, { 0x1240 + 1, 0xffffff02 }, 0x0000 };
					notasksignal = 0;
					sgg_execcmd(&command);
				}
			}
		} else if (sig == 0x0081) {
			/* カーネルからの要請。アプリケーションが全て終了したらシグナルを発する */
			notasksignal = 1;
			lib_waitsignal(0x0000, 1, 0);
			sbp++;
			goto check_notask;
#endif
		} else if (sig == 0x0084 || sig == 0x0088) {
			/* ダイアログ要求シグナル */
			/* とりあえずバッファに入れておく */
			int i;
			for (i = 0; selwait[i].task; i++); /* 行き過ぎる事は考えてない */
			selwaitcount++;
			selwait[i].task    = sbp[1];
			selwait[i].slot    = sbp[2];
			selwait[i].bytes   = sbp[3];
			selwait[i].ofs     = sbp[4];
			selwait[i].sel     = sbp[5];
			lib_waitsignal(0x0000, 6, 0);
			sbp += 6;
			return 0; /* 一度0を返す */
		} else if (sig >= 0x27f && (sig & 0x7f) == 0x7f) {
			/* closed window */
			int i = ((sig - 0x27f) >> 7) + 1;
			free(selwin[i].window);
			free(selwin[i].wintitle);
			free(selwin[i].subtitle);
			free(selwin[i].selector);
			selwin[i].window = NULL;
			selwincount--;
			sig = 0;
			goto skip0;
		} else {
skip0:
			lib_waitsignal(0x0000, 1, 0);
			sbp++;
			return sig;
		}
	}
}

void wait99()
{
	int sig;
	do {
		sig = wait99sub();
	} while (sig != 99);
	return;
}

void putcursor()
{
	sleep = 0;
	lib_putstring_ASCII(0x0001, console_curx, console_cury,
		console_txt, 0, ((console_col & cursorflag) | ((console_col >> 4) & ~cursorflag)) & 0x0f, " ");
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
	static struct {
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
	int i;

	win->window = lib_openwindow1(AUTO_MALLOC, win->winslot, 160, 40 + LIST_HEIGHT * 16, 0x28, win->sigbase + 120 /* +6 */);
	win->wintitle = lib_opentextbox(0x1000, AUTO_MALLOC,  0, 10, 1,  0,  0, win->window, 0x00c0, 0);
	win->subtitle = lib_opentextbox(0x0000, AUTO_MALLOC,  0, 20, 1,  0,  0, win->window, 0x00c0, 0);
	win->selector = lib_opentextbox(0x0001, AUTO_MALLOC, 15, 16, 8, 16, 32, win->window, 0x00c0, 0);
	lib_putstring_ASCII(0x0000, 0, 0, win->wintitle, 0, 0, title);
	lib_putstring_ASCII(0x0000, 0, 0, win->subtitle, 0, 0, subtitle);

	for (i = 0; table[i].code; i++)
		lib_definesignal1p0(table[i].opt, 0x0100, table[i].code, win->window, win->sigbase + table[i].signum);
	lib_definesignal0p0(0, 0, 0, 0);
	return;
}

const int fileselect(int i, struct FILESELWIN *win, int fileid);

static struct {
	int cmd, opt;
	int data[3];
	int eoc;
} send_command = { 0x0020, 0x80000000 + 3, { 0, 0x7f000001, 0 }, 0x0000 };

void main()
{
	struct FILESELWIN *win;

	int i, sig, bank;
	char charbuf16[17];

	lib_init(AUTO_MALLOC);
	sgg_init(AUTO_MALLOC);

	sbp = sb0 = lib_opensignalbox(256, AUTO_MALLOC, 0, 1);

	file = (struct SGG_FILELIST *) malloc(256 * sizeof (struct SGG_FILELIST));
//	list = (struct FILELIST *) malloc(256 * sizeof (struct FILELIST));
	banklist = (struct STR_BANK *) malloc(MAX_BANK * sizeof (struct STR_BANK));
	filebuf = (struct FILEBUF *) malloc(MAX_FILEBUF * sizeof (struct FILEBUF));
	selwin = (struct FILESELWIN *) malloc(MAX_SELECTOR * sizeof (struct FILESELWIN));
	selwait = (struct SELECTORWAIT *) malloc(MAX_SELECTORWAIT * sizeof (struct SELECTORWAIT));
	vmref = (struct VIRTUAL_MODULE_REFERENCE *) malloc(MAX_VMREF * sizeof (struct VIRTUAL_MODULE_REFERENCE));

	win = selwin;
	win[0].winslot = 0x0200;
	win[0].sigbase = 0;
	for (i = 1; i < MAX_SELECTOR; i++) {
		win[i].winslot = 0x0220 + (i - 1) * 16;
		win[i].sigbase = 512 + 128 * (i - 1);
		win[i].window = NULL;
	}

	for (i = 0; i < MAX_SELECTORWAIT; i++)
		selwait[i].task = 0;

	for (i = 0; i < MAX_VMREF; i++)
		vmref[i].task = 0;

	openselwin(&selwin[0], "pokon15", "< Run Application > ");

	lib_opentimer(SYSTEM_TIMER);
	lib_definesignal1p0(0, 0x0010 /* timer */, SYSTEM_TIMER, 0, 287);

	/* キー操作を追加登録 */
	{
		static struct {
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
		for (i = 0; table[i].code; i++)
			lib_definesignal1p0(table[i].opt, 0x0100, table[i].code, selwin[0].window, table[i].signum);
		lib_definesignal0p0(0, 0, 0, 0);
	}

	for (i = 0; i < MAX_BANK; i++)
		banklist[i].size = banklist[i].tss = 0;

	for (i = 0; i < MAX_FILEBUF; i++) {
		filebuf[i].dirslot = -1;
		filebuf[i].linkcount = 0;
	}

	wait99(); /* finish signalが来るまで待つ */
	selwin[0].ext = ext_BIN;
	list_set(&selwin[0]);

	for (;;) {
		do {
			sig = 0;
			if (selwaitcount == 0 || selwincount == MAX_SELECTOR || fmode != 0)
				sig = wait99sub();
			while (selwaitcount != 0 && selwincount < MAX_SELECTOR && fmode == 0) {
				int subcmd[256], j;
				static char t[24] = "< for ########     >";
				for (bank = 0; selwait[bank].task == 0; bank++);
				for (i = 1; selwin[i].window; i++);
				selwaitcount--;
				selwincount++;
				/* task, slot, num, siglen, sig[16]を取得 */
				selwin[i].task = selwait[bank].task;
				selwin[i].mdlslot = selwait[bank].slot;
				sgg_debug00(0, selwait[bank].bytes, 0,
					selwait[bank].ofs, selwait[bank].sel | (selwin[i].task << 8) + 0xf80000, (const int) subcmd, 0x000c);
				selwait[bank].task = 0;
				if (subcmd[0] == 0xffffff01 /* num */) {
					selwin[i].num = subcmd[1];
					selwin[i].siglen = subcmd[3];
					selwin[i].sig[0] = subcmd[4];
					selwin[i].sig[1] = subcmd[5];
					for (bank = 0; banklist[bank].size == 0 || banklist[bank].tss != selwin[i].task; bank++);
					for (j = 0; j < 8; j++)
						t[j + 6] = banklist[bank].name[j];
					openselwin(&selwin[i], "(open)", t);
					selwin[i].ext = -1;
					list_set(&selwin[i]);
				} else if (subcmd[0] == 0xffffff03 /* direct-name */) {
					/* とりあえず、名前は12バイト固定(最後のバイトは00) */
					struct SGG_FILELIST *fp;
					int j, k, fileid;
					unsigned char c;
					selwin[i].siglen = subcmd[6];
					selwin[i].sig[0] = subcmd[7];
					selwin[i].sig[1] = subcmd[8];
					for (fp = file + 1; fp->name[0]; fp++) {
						c = 0;
						for (j = 0; j < 8; j++)
							c |= ((unsigned char *) &subcmd[2])[j] - fp->name[j];
						for (j = 0; j < 3; j++)
							c |= ((unsigned char *) &subcmd[2])[j + 9] - fp->name[j + 8];
						if (c == 0)
							goto find;
					}
					goto error_sig;
find:
					fileid = fp->id;
					/* 見付かった */
					for (j = 0; j < MAX_FILEBUF; j++) {
						if (filebuf[j].dirslot != fileid)
							continue;
						filebuf[j].linkcount++;
						goto filebuf_hit1;
					}
					for (j = 0; j < MAX_FILEBUF; j++) {
						if (filebuf[j].linkcount)
							continue;
						goto find_freefilebuf1;
					}
					/* ここに来たらバッファ不足でロードできなかったことを意味する */
error_sig:
					k = (0x003c /* slot_sel */ | selwin[i].task << 8) + 0xf80000;
					sgg_directwrite(0, 4, 0, selwin[i].mdlslot, k, (int) &j, 0x000c);
					send_command.data[0] = selwin[i].task | 0x0242;
					send_command.data[2] = selwin[i].sig[1] + 1;
					sgg_execcmd(&send_command);
					selwin[i].task = 0;
					selwincount--;

find_freefilebuf1:
					sgg_loadfile2(fileid /* file id */,
						99 /* finish signal */
					);
					wait99(); // finish signalが来るまで待つ
					filebuf[j].size = sbp[0];
					filebuf[j].paddr = sbp[1];
					sbp += 2;
					lib_waitsignal(0x0000, 2, 0);
					if (filebuf[j].size == 0)
						goto error_sig; /* メモリが足りない */
					filebuf[j].dirslot = fileid;
					filebuf[j].linkcount = 1;
filebuf_hit1:
					for (k = 0; vmref[k].task != 0; k++);
					vmref[k].task = selwin[i].task;
					vmref[k].module_paddr = filebuf[j].paddr;
					j = sgg_createvirtualmodule(filebuf[j].size, filebuf[j].paddr);
					k = (0x003c /* slot_sel */ | selwin[i].task << 8) + 0xf80000;
					sgg_directwrite(0, 4, 0, selwin[i].mdlslot, k, (int) &j, 0x000c);
					send_command.data[0] = selwin[i].task | 0x0242;
					send_command.data[2] = selwin[i].sig[1];
					sgg_execcmd(&send_command);
					selwin[i].task = 0;
					selwincount--;
				}
			}
		} while (sig == 0);

		if (sig < 128) {
			i = 0;
			goto winselsig;
		}
		if (sig < 256)
			goto specialsig;
		if (sig < 512)
			goto consolesig;
		i = ((sig - 512) >> 7) + 1;
		sig &= 0x7f;
winselsig:
		win = &selwin[i];
		if (win->window) {
			int cur = win->cur;
			struct FILELIST *lp = win->lp, *list = win->list;
			switch (sig) {

			case 4 /* cursor-up */:
				if (cur < 0 /* ファイルが１つもない */)
					break;
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
					break;
				/* i : ウィンドウ番号 */
				/* win : &selwin[i] */
				sig = fileselect(i, win, lp[cur].ptr->id);
				if (sig == 0xc0)
					goto signal_c0;
				if (sig == 0xc1)
					goto signal_c1;
				break;

			case 10 /* page-up */:
				if (cur < 0 /* ファイルが１つもない */)
					break;
				if (lp >= list + LIST_HEIGHT)
					lp -= LIST_HEIGHT;
				else {
					lp = list;
					cur = 0;
				}
				goto listup;

			case 11 /* page-down */:
				if (cur < 0 /* ファイルが１つもない */)
					break;
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
				sgg_format(0x0114, 99 /* finish signal */); // invalid diskcache
				wait99(); // finish signalが来るまで待つ
				for (i = 0; i < MAX_FILEBUF; i++)
					filebuf[i].dirslot = -1;
				/* 全てのファイルセレクタを更新 */
				for (i = 0; i < MAX_SELECTOR; i++) {
					if (selwin[i].window)
						list_set(&selwin[i]);
				}
				cur = win->cur;
				lp = win->lp;
				break;

			case 126 /* close window */:
				if (i == 0)
					break;
				/* キャンセルを通知して、閉じる */
				/* 待機しているものがあれば、応じる(応じるのは127を受け取ってからにする) */
				send_command.data[0] = win->task | 0x0242;
				send_command.data[2] = win->sig[1] + 1 /* cancel */;
				sgg_execcmd(&send_command);
				win->task = -1;
				lib_closewindow(0, win->window);
				break;

		//	case 127 /* closed window */:
		//		free(win->window);
		//		win->window = NULL;
		//		selwincount--;
		//		break;
			}
			win->cur = cur;
			win->lp = lp;
		}
		continue;

specialsig:
		switch (sig) {
		case 0xc0 /* to format-mode */:
		signal_c0:
			sig = 0;
			for (i = 1; i < MAX_SELECTOR; i++)
				sig |= (int) selwin[i].window;
			if (sig)
				break;
			selwin[0].ext = ext_EXE;
			list_set(&selwin[0]);
			fmode = 1;
			goto signal_c2;
		//	lib_putstring_ASCII(0x0000, 0, 0, selwin[0].subtitle, (fmode - 1) * 9, 0, "< Load Systemimage >");
		//	break;

	/* フォーマットモードに入るには、他のファイルセレクタが全て閉じられていなければいけない */
	/* また、フォーマットモードに入っている間は、ファイルセレクタが開かない */

		case 0xc1 /* to run-mode */:
		signal_c1:
			selwin[0].ext = ext_BIN;
			lib_putstring_ASCII(0x0000, 0, 0, selwin[0].subtitle,     0, 0, "< Run Application > ");
			list_set(&selwin[0]);
			fmode = 0;
			/* 待機している要求があれば、ウィンドウを開く */
			break;

		case 0xc2 /* change format-mode */:
			if (fmode) {
				fmode = 3 - fmode;
		signal_c2:
				lib_putstring_ASCII(0x0000, 0, 0, selwin[0].subtitle, (fmode - 1) * 9, 0, "< Load Systemimage >");
			}
			break;

		case 0xc3 /* open console */:
			if (console_win == NULL)
				open_console();
			break;

		case 0xc4 /* open monitor */:
			break;

//		case 99:
//			lib_putstring_ASCII(0x0000, 0, 0, mode,     0, 0, "< Error 99        > ");
//			break;
		}
		continue;

consolesig:
		if (256 + ' ' <= sig && sig <= 256 + 0x7f) {
			/* consoleへの1文字入力 */
			if (console_win != NULL) {
				if (console_curx < CONSOLESIZEX - 1) {
					static char c[2] = { 0, 0 };
					c[0] = sig - 256;
					consoleout(c);
				}
				if (cursoractive) {
					lib_settimer(0x0001, SYSTEM_TIMER);
					cursorflag = ~0;
					putcursor();
					lib_settimertime(0x0032, SYSTEM_TIMER, 0x80000000 /* 500ms */, 0, 0);
				}
			}
		} else {
			switch (sig) {
			case 256 + 0 /* VRAMアクセス許可 */:
				break;

			case 256 + 1 /* VRAMアクセス禁止 */:
				lib_controlwindow(0x0100, console_win);
				break;

			case 256 + 2:
			case 256 + 3:
				/* 再描画 */
				lib_controlwindow(0x0203, console_win);
				break;

			case 256 + 5 /* change console title color */:
				if (*sbp++ & 0x02) {
					if (!cursoractive) {
						lib_settimer(0x0001, SYSTEM_TIMER);
						cursoractive = 1;
						cursorflag = ~0;
						putcursor();
						lib_settimertime(0x0032, SYSTEM_TIMER, 0x80000000 /* 500ms */, 0, 0);
					}
				} else {
					if (cursoractive) {
						cursoractive = 0;
						cursorflag = 0;
						putcursor();
						lib_settimer(0x0001, SYSTEM_TIMER);
					}
				}
				lib_waitsignal(0x0000, 1, 0);
				break;

			case 256 + 6 /* close console window */:
				lib_closewindow(0, console_win);
				console_win = NULL;
				if (cursoractive) {
					cursoractive = 0;
					lib_settimer(0x0001, SYSTEM_TIMER);
				}
				break;

			case 287 /* cursor blink */:
				if (sleep == 1 && cursoractive != 0) {
					cursorflag =~ cursorflag;
					putcursor();
					lib_settimertime(0x0012, SYSTEM_TIMER, 0x80000000 /* 500ms */, 0, 0);
				}
				break;

			case 256 + 0xa0 /* consoleへのEnter入力 */:
				if (console_win != NULL) {
					const char *p = consolebuf + console_cury * (CONSOLESIZEX + 2) + 5;
					if (cursorflag != 0 && cursoractive != 0) {
						cursorflag = 0;
						putcursor();
					}
					while (*p == ' ')
						p++;
					if (*p) {
						static struct {
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
						int i = 0;
						do {
							if (poko_cmdchk(p, cmdlist[i].cmd)) {
								(*cmdlist[i].fnc)(p);
								goto prompt;
							}
						} while (cmdlist[++i].fnc);
						consoleout("\nBad command.\n");
					}
			prompt:
					consoleout("\npoko>");
					if (cursoractive) {
						lib_settimer(0x0001, SYSTEM_TIMER);
						cursorflag = ~0;
						putcursor();
						lib_settimertime(0x0032, SYSTEM_TIMER, 0x80000000 /* 500ms */, 0, 0);
					}
				}
				break;

			case 256 + 0xa1 /* consoleへのBackSpace入力 */:
				if (console_win != NULL) {
					if (cursorflag != 0 && cursoractive != 0) {
						cursorflag = 0;
						putcursor();
					}
					if (console_curx > 5) {
						console_curx--;
						consoleout(" ");
						console_curx--;
					}
					if (cursoractive) {
						lib_settimer(0x0001, SYSTEM_TIMER);
						cursorflag = ~0;
						putcursor();
						lib_settimertime(0x0032, SYSTEM_TIMER, 0x80000000 /* 500ms */, 0, 0);
					}
				}
				break;
			}
		}
	}
}

const int fileselect(int i, struct FILESELWIN *win, int fileid)
{
	int sig;
	if (i) { /* not pokon */
		/* ファイルをロードして、仮想モジュールを生成し、
			スロットを設定し、シグナルを発する */
		/* 最後に、ウィンドウを閉じる */
		int j, tss;

		if (win->task == -1)
			return 0;

		tss = win->task;
		win->task = -1;
		lib_closewindow(0, win->window);

		for (i = 0; i < MAX_FILEBUF; i++) {
			if (filebuf[i].dirslot != fileid)
				continue;
			filebuf[i].linkcount++;
			goto filebuf_hit1;
		}
		for (i = 0; i < MAX_FILEBUF; i++) {
			if (filebuf[i].linkcount)
				continue;
			goto find_freefilebuf1;
		}
		return 0;
find_freefilebuf1:
		sgg_loadfile2(fileid /* file id */,
			99 /* finish signal */
		);
		wait99(); // finish signalが来るまで待つ
		filebuf[i].size = sbp[0];
		filebuf[i].paddr = sbp[1];
		sbp += 2;
		lib_waitsignal(0x0000, 2, 0);
		if (filebuf[i].size == 0)
			return 0;
		filebuf[i].dirslot = fileid;
		filebuf[i].linkcount = 1;
filebuf_hit1:
		for (j = 0; vmref[j].task != 0; j++);
		vmref[j].task = tss;
		vmref[j].module_paddr = filebuf[i].paddr;
		i = sgg_createvirtualmodule(filebuf[i].size, filebuf[i].paddr);
		sig = (0x003c /* slot_sel */ | tss << 8) + 0xf80000;
		sgg_directwrite(0, 4, 0, win->mdlslot, sig, (int) &i, 0x000c);
		send_command.data[0] = tss | 0x0242;
		send_command.data[2] = win->sig[1];
		sgg_execcmd(&send_command);
	//	win->task = -1;
	//	lib_closewindow(0, win->window);
		return 0;
	}
	/* pokon */
	if (win->ext == ext_BIN) {
		// .BINファイルモード
		int bank;
		char *s;
		for (bank = 0; bank < MAX_BANK; bank++) {
			if (banklist[bank].size == 0)
				goto find_freebank;
		}
		return 0;
find_freebank:
		for (i = 0; i < MAX_FILEBUF; i++) {
			if (filebuf[i].dirslot != fileid)
				continue;
			banklist[bank].size = filebuf[i].size;
			banklist[bank].addr = filebuf[i].paddr;
			filebuf[i].linkcount++;
			goto filebuf_hit;
		}
		for (i = 0; i < MAX_FILEBUF; i++) {
			if (filebuf[i].linkcount)
				continue;
			goto find_freefilebuf;
		}
		return 0;
find_freefilebuf:
		sgg_loadfile2(fileid /* file id */,
			99 /* finish signal */
		);
		wait99(); // finish signalが来るまで待つ
		banklist[bank].size = sbp[0];
		banklist[bank].addr = sbp[1];
		sbp += 2;
		lib_waitsignal(0x0000, 2, 0);
		if (banklist[bank].size == 0)
			return 0;
		filebuf[i].size = banklist[bank].size;
		filebuf[i].paddr = banklist[bank].addr;
		filebuf[i].dirslot = fileid;
		filebuf[i].linkcount = 1;
filebuf_hit:
		s = (win->lp)[win->cur].name;
		for (i = 0; i < 11; i++)
			banklist[bank].name[i] = s[i];
		banklist[bank].name[11] = '\0';

		sgg_createtask2(banklist[bank].size, banklist[bank].addr, 99 /* finish signal */);
		wait99(); // finish signalが来るまで待つ

		banklist[bank].tss = i = *sbp++;
		lib_waitsignal(0x0000, 1, 0);

		// 適正なGUIGUI00ファイルでない場合、タスクは生成されず、iは0。
		if (i) {
			sgg_settasklocallevel(i,
				0 * 32 /* local level 1 (スリープレベル) */,
				27 * 64 + 0x0100 /* global level 27 (スリープ) */,
				-1 /* Inner level */
			);
			banklist[bank].Llv[0].global = 27;
			banklist[bank].Llv[0].inner  = -1;
			sgg_settasklocallevel(i,
				1 * 32 /* local level 1 (起動・システム処理レベル) */,
				12 * 64 + 0x0100 /* global level 12 (一般アプリケーション) */,
				defaultIL /* Inner level */
			);
			banklist[bank].Llv[1].global = 12;
			banklist[bank].Llv[1].inner  = defaultIL;
			sgg_settasklocallevel(i,
				2 * 32 /* local level 2 (通常処理レベル) */,
				12 * 64 + 0x0100 /* global level 12 (一般アプリケーション) */,
				defaultIL /* Inner level */
			);
			banklist[bank].Llv[2].global = 12;
			banklist[bank].Llv[2].inner  = defaultIL;
			sgg_runtask(i, 1 * 32);
		} else {
			// ロードした領域を解放
			banklist[bank].size = 0;
			sig = banklist[bank].addr;
			for (i = 0; filebuf[i].paddr != sig; i++);
			if (--filebuf[i].linkcount == 0) {
				sgg_freememory2(filebuf[i].size, filebuf[i].paddr);
				filebuf[i].dirslot = -1;
			}
		}
	} else {
		// .EXEファイルモード
		int exe_size, exe_addr, bsc_size, bsc_addr;
		struct SGG_FILELIST *fp;

		sgg_loadfile2(fileid /* file id */,
			99 /* finish signal */
		);
		wait99(); // finish signalが来るまで待つ
		exe_size = sbp[0];
		exe_addr = sbp[1];
		sbp += 2;
		lib_waitsignal(0x0000, 2, 0);
		if (exe_size == 0)
			return 0;

		i = ('K' | ('B' << 8) | ('S' << 16) | ('1' << 24)); /* 1,440KB 非圧縮用 */
		if (fmode == 2)
		//	i = ('K' | ('B' << 8) | ('S' << 16) | ('0' << 24)); /* 1,760KB 非圧縮用 */
			i = ('K' | ('B' << 8) | ('S' << 16) | ('3' << 24)); /* 1,440KB 圧縮用 */

		for (fp = file + 1; fp->name[0]; fp++) {
			if ((fp->name[0] | (fp->name[1] << 8) | (fp->name[2] << 16) | (fp->name[3] << 24))
				== ('O' | ('S' << 8) | ('A' << 16) | ('S' << 24)) &&
			    (fp->name[4] | (fp->name[5] << 8) | (fp->name[6] << 16) | (fp->name[7] << 24))
				== i &&
			    (fp->name[8] | (fp->name[9] << 8) | (fp->name[10] << 16))
				== ('B' | ('I' << 8) | ('N' << 16))) {
				i = fp->id;
				break;
			}
		}
		if (fp->name[0] == 0)
			return 0; /* "OSASKBS0.BIN"～"OSASKBS3.BIN"が見つからなかった */

		sgg_loadfile2(i /* file id */, 99 /* finish signal */);
		wait99(); // finish signalが来るまで待つ
		bsc_size = sbp[0];
		bsc_addr = sbp[1];
		sbp += 2;
		lib_waitsignal(0x0000, 2, 0);
		if (bsc_size == 0)
			return 0;

		for (i = 0; i < LIST_HEIGHT; i++)
			putselector0(win, i, "                ");
		putselector0(win, 1, "    Loaded.     ");
		putselector0(win, 3, " Change disks.  ");
		putselector0(win, 5, " Hit Enter key. ");

		// Enter, 'F', 'R', 'S'を待つ
		do {
			sig = wait99sub();
		} while (sig != 6 && sig != 0xc0 && sig != 0xc1 && sig != 0xc2);

		if (sig == 0xc0 || sig == 0xc1)
			return sig;

		putselector0(win, 5, "  Please wait.  ");
		if (sig == 6) {
			putselector0(win, 1, "  Formating...  ");
			putselector0(win, 3, "                ");
			i = 0x0124; /* 1,440KBフォーマット */
		//	if (fmode)
		//		i = 0x0118; /* 1,760KBフォーマットと1,440KBフォーマットの混在 */
			sgg_format(i, 99 /* finish signal */); // format
			wait99(); // finish signalが来るまで待つ
		}
		putselector0(win, 1, " Writing        ");
		putselector0(win, 3, "   system image.");
		i = 0x0128; /* 1,440KBフォーマット用 非圧縮 */
		if (fmode == 2)
		//	i = 0x011c; /* 1,760KBフォーマット用 非圧縮 */
			i = 0x0138; /* 1,440KBフォーマット用 圧縮 */
		sgg_format2(i, bsc_size, bsc_addr, exe_size, exe_addr,
			99 /* finish signal */); // store system image
		wait99(); // finish signalが来るまで待つ

		// ロードした領域を解放
		sgg_freememory2(exe_size, exe_addr);
		sgg_freememory2(bsc_size, bsc_addr);

		putselector0(win, 1, "   Completed.   ");
		putselector0(win, 3, " Change disks.  ");
		putselector0(win, 5, "  Hit 'R' key.  ");

		// 'F', 'R'を待つ
		do {
			sig = wait99sub();
		} while (sig != 0xc0 && sig != 0xc1);

		sgg_format(0x0114, 99 /* finish signal */); // flush diskcache
		wait99(); // finish signalが来るまで待つ

	}
	return sig;
}


void open_monitor()
/*	モニターをオープンする */
{



}

void consoleout(char *s)
{
	char *cbp = consolebuf + console_cury * (CONSOLESIZEX + 2) + console_curx;
	while (*s) {
		consolebuf[console_cury * (CONSOLESIZEX + 2) + (CONSOLESIZEX + 1)] = console_col;
		if (*s == '\n') {
			s++;
			lib_putstring_ASCII(0x0001, 0, console_cury,
				console_txt, console_col & 0x0f, (console_col >> 4) & 0x0f,
				&consolebuf[console_cury * (CONSOLESIZEX + 2) + 0]);
			console_cury++;
			console_curx = 0;
			if (console_cury == CONSOLESIZEY) {
				/* スクロールする */
				int i, j;
				char *bp = consolebuf;
				consolebuf[CONSOLESIZEY * (CONSOLESIZEX + 2) + (CONSOLESIZEX + 1)] = console_col;
				for (j = 0; j < CONSOLESIZEY; j++) {
					for (i = 0; i < CONSOLESIZEX + 2; i++)
						bp[i] = bp[i + (CONSOLESIZEX + 2)];
					lib_putstring_ASCII(0x0001, 0, j, console_txt,
						bp[CONSOLESIZEX + 1] & 0x0f, (bp[CONSOLESIZEX + 1] >> 4) & 0x0f, bp);
					bp += CONSOLESIZEX + 2;
				}
				console_cury = CONSOLESIZEY - 1;
			}
			cbp = consolebuf + console_cury * (CONSOLESIZEX + 2);
		} else {
			*cbp++ = *s++;
			console_curx++;
		}
	}
	lib_putstring_ASCII(0x0001, 0, console_cury,
		console_txt, console_col & 0x0f, (console_col >> 4) & 0x0f,
		&consolebuf[console_cury * (CONSOLESIZEX + 2) + 0]);
	return;
}

void open_console()
/*	コンソールをオープンする */
/*	カーソル点滅のために、setmodeも拾う */
/*	カーソル点滅のためのタイマーをイネーブルにする */
{
	struct LIB_TEXTBOX *console_tit;
	int i, j;
	char *bp;
	console_win = lib_openwindow1(AUTO_MALLOC, 0x0210, CONSOLESIZEX * 8, CONSOLESIZEY * 16, 0x0d, 256);
	console_tit = lib_opentextbox(0x1000, AUTO_MALLOC,  0, 16,  1,  0,  0, console_win, 0x00c0, 0);
	console_txt = lib_opentextbox(0x0001, AUTO_MALLOC,  0, CONSOLESIZEX, CONSOLESIZEY,  0,  0, console_win, 0x00c0, 0); // 5KB
	lib_putstring_ASCII(0x0000, 0, 0, console_tit, 0, 0, "pokon15 console");

	if (consolebuf == NULL)
		consolebuf = (char *) malloc((CONSOLESIZEX + 2) * (CONSOLESIZEY + 1));
	bp = consolebuf;
	for (j = 0; j < CONSOLESIZEY + 1; j++) {
		for (i = 0; i < CONSOLESIZEX; i++) {
			*bp++ = ' ';
		}
		bp[0] = '\0';
		bp[1] = 0;
		bp += 2;
	}
	lib_definesignal1p0(0x5f, 0x0100, ' ',              console_win, 256 + ' ');
	lib_definesignal1p0(1,    0x0100, 0xa0 /* Enter */, console_win, 256 + 0xa0);
	lib_definesignal0p0(0, 0, 0, 0);
	console_curx = console_cury = 0;
	console_col = 15;
	consoleout("Heppoko-shell \"poko\" version 1.9\n    Copyright (C) 2001 H.Kawai(Kawaido)\n");
	consoleout("\npoko>");
	if (cursoractive) {
		lib_settimer(0x0001, SYSTEM_TIMER);
		cursorflag = ~0;
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
	int param0, param1 = console_col & 0xf0;
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
	console_col = param0 | param1;
	return;
}

void poko_cls(const char *cmdlin)
{
	int i, j;
	char *bp = consolebuf;
	cmdlin += 3 /* "cls" */;
	while (*cmdlin == ' ')
		cmdlin++;
	if (*cmdlin) {
		consoleout("\nIllegal parameter(s).\n");
		return;
	}
	for (j = 0; j < CONSOLESIZEY; j++) {
		for (i = 0; i < CONSOLESIZEX + 2; i++)
			bp[i] = consolebuf[i + (CONSOLESIZEX + 2) * CONSOLESIZEY];
		lib_putstring_ASCII(0x0001, 0, j, console_txt,
			console_col & 0x0f, (console_col >> 4) & 0x0f, bp);
		bp += CONSOLESIZEX + 2;
	}
	console_cury = 0;
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
	cmdlin += 8 /* "tasklist" */;
	while (*cmdlin == ' ')
		cmdlin++;
	if (*cmdlin) {
		consoleout("\nIllegal parameter(s).\n");
		return;
	}
	consoleout("\n");
	for (i = 0; i < MAX_BANK; i++) {
		if (banklist[i].size == 0)
			continue;
		itoa10(banklist[i].tss >> 12, str);
		msg[0] = str[ 8]; /* 100の位 */
		msg[1] = str[ 9]; /*  10の位 */
		msg[2] = str[10]; /*   1の位 */
		for (j = 0; j < 8; j++)
			msg[4 + j] = banklist[i].name[j];
		consoleout(msg);
	}
	return;
}

void poko_sendsignalU(const char *cmdlin)
{
	static struct {
		int cmd, opt;
		int data[3];
		int eoc;
	} command = { 0x0020, 0x80000000 + 3, { 0, 0x7f000001, 0 }, 0x0000 };

	cmdlin += 11 /* "sendsignalU" */;
	while (*cmdlin == ' ')
		cmdlin++;
	if (*cmdlin == '\0') {
		consoleout("\nIllegal parameter(s).\n");
		return;
	}
	command.data[0] = console_getdec(&cmdlin) * 4096 + 0x0242;
	while (*cmdlin == ' ')
		cmdlin++;
	if (*cmdlin == '\0') {
		consoleout("\nIllegal parameter(s).\n");
		return;
	}
	command.data[2] = console_getdec(&cmdlin);
	while (*cmdlin == ' ')
		cmdlin++;
	if (*cmdlin) {
		consoleout("\nIllegal parameter(s).\n");
		return;
	}
	sgg_execcmd(&command);
	consoleout("\n");
	return;
}

void poko_LLlist(const char *cmdlin)
{
	int task, i, l;
	cmdlin += 6 /* "LLlist" */;
	while (*cmdlin == ' ')
		cmdlin++;
	if (*cmdlin == '\0') {
		consoleout("\nIllegal parameter(s).\n");
		return;
	}
	task = console_getdec(&cmdlin) * 4096;
	for (i = 0; i < MAX_BANK; i++) {
		if (banklist[i].size == 0)
			continue;
		if (banklist[i].tss == task)
			goto find;
	}
	consoleout("\nIllegal parameter(s).\n");
	return;

find:
	while (*cmdlin == ' ')
		cmdlin++;
	if (*cmdlin) {
		consoleout("\nIllegal parameter(s).\n");
		return;
	}
	consoleout("\nLL GL   IL\n");
	for (l = 0; l < 3; l++) {
		int global;
		char msg[16], str[16];
		msg[ 0] = l + '0';
		msg[ 1] = ' ';
		itoa10(global = banklist[i].Llv[l].global, str);
		msg[ 2] = str[ 8];
		msg[ 3] = str[ 9];
		msg[ 4] = str[10];
		msg[ 5] = ' ';
		msg[ 6] = '-';
		msg[ 7] = '-';
		msg[ 8] = '-';
		msg[ 9] = '-';
		if (global == 12) {
			itoa10(banklist[i].Llv[l].inner, str);
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
	cmdlin += 5 /* "setIL" */;
	while (*cmdlin == ' ')
		cmdlin++;
	if (*cmdlin == '\0') {
		consoleout("\nIllegal parameter(s).\n");
		return;
	}
	task = console_getdec(&cmdlin) * 4096;
	for (i = 0; i < MAX_BANK; i++) {
		if (banklist[i].size == 0)
			continue;
		if (banklist[i].tss == task)
			goto find;
	}
	consoleout("\nIllegal parameter(s).\n");
	return;

find:
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
	banklist[i].Llv[1].global = 12;
	banklist[i].Llv[1].inner  = l;
	sgg_settasklocallevel(task,
		2 * 32 /* local level 2 (通常処理レベル) */,
		16 * 64 /* global level 16 (一般アプリケーション) */,
		l /* Inner level */
	);
	banklist[i].Llv[2].global = 12;
	banklist[i].Llv[2].inner  = l;
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
