/* "winman0.c":ぐいぐい仕様ウィンドウマネージャー ver.1.4
		copyright(C) 2001 川合秀実
    stack:4k malloc:92k file:768k */

/* プリプロセッサのオプションで、-DPCATか-DTOWNSを指定すること */

#include <guigui00.h>
#include <sysgg00.h>
/* sysggは、一般のアプリが利用してはいけないライブラリ
   仕様もかなり流動的 */
#include <stdlib.h>

#define	AUTO_MALLOC			   0
#define NULL				   0
#define	MAX_WINDOWS		 	  80		// 8.1KB
#define JOBLIST_SIZE		 256		// 1KB
#define	MAX_SOUNDTRACK		  16		// 0.5KB
#define	DEFSIGBUFSIZ		2048

#define WINFLG_MUSTREDRAW		0x80000000	/* bit31 */
#define WINFLG_OVERRIDEDISABLED	0x01000000	/* bit24 */
#define	WINFLG_WAITREDRAW		0x00000200	/* bit 9 */
#define	WINFLG_WAITDISABLE		0x00000100	/* bit 8 */

//	#define	DEBUG	1

struct DEFINESIGNAL { // 32bytes
	int win, opt, dev, cod, len, sig[3];
};

struct WM0_WINDOW {	// total 108bytes
//	struct DEFINESIGNAL defsig[29]; // 928bytes
	struct SGG_WINDOW sgg; // 68bytes
//	struct DEFINESIGNAL *ds1;
	int condition, x0, y0, x1, y1, job_flag0, job_flag1;
	int tx0, ty0, tx1, ty1; /* ウィンドウ移動のためのタブ */
	int flags;
	struct WM0_WINDOW *up, *down;
};

struct SOUNDTRACK {
	int sigbox, sigbase, slot, reserve[5];
};

struct WM0_WINDOW *window, *top = NULL, *unuse = NULL, *iactive = NULL, *pokon0 = NULL;
int *joblist, jobfree, *jobrp, *jobwp, jobnow = 0;
void (*jobfunc)(int, int);
int x2 = 0, y2, mx = 0x80000000, my, mbutton = 0;
int fromboot = 0;
struct SOUNDTRACK *sndtrk_buf, *sndtrk_active = NULL;
struct WM0_WINDOW *job_win;
int job_count, job_int0, job_movewin4_ready = 0, mousescale = 1;
int job_movewin_x, job_movewin_y, job_movewin_x0, job_movewin_y0;
struct DEFINESIGNAL *defsigbuf;

void init_screen(const int x, const int y);
struct WM0_WINDOW *handle2window(const int handle);
void chain_unuse(struct WM0_WINDOW *win);
struct WM0_WINDOW *get_unuse();
void mousesignal(const unsigned int header, int dx, int dy);
void writejob(int i);
void writejob2(int i, int j);
void runjobnext();
void job_openwin0(struct WM0_WINDOW *win);
void redirect_input(struct WM0_WINDOW *win);
void job_activewin0(struct WM0_WINDOW *win);
void job_movewin0(struct WM0_WINDOW *win);
void job_movewin1(const int cmd, const int handle);
void job_movewin2();
void job_movewin3();
void job_movewin4(int sig);
void job_movewin4m(int x, int y);
void job_closewin0(struct WM0_WINDOW *win0);
void job_closewin1(const int cmd, const int handle);
void job_general1();
void job_general2(const int cmd, const int handle);
void job_openvgadriver(const int drv);
void job_setvgamode0(const int mode);
void job_setvgamode1(const int cmd, const int handle);
void job_setvgamode2();
void job_setvgamode3(const int sig, const int result);
void job_loadfont0(int fonttype, int tss, int sig);
void job_loadfont1(int flag, int dmy);
void job_loadfont2();
void job_loadfont3(int flag, int dmy);

void free_sndtrk(struct SOUNDTRACK *sndtrk);
struct SOUNDTRACK *alloc_sndtrk();
void send_signal2dw(const int sigbox, const int data0, const int data1);
void send_signal3dw(const int sigbox, const int data0, const int data1, const int data2);

void lib_drawletters_ASCII(const int opt, const int win, const int charset, const int x0, const int y0,
	const int color, const int backcolor, const char *str);
void debug_bin2hex(unsigned int i, unsigned char *s);

void sgg_wm0_definesignal3(const int opt, const int device, int keycode,
	const int sig0, const int sig1, const int sig2);
void sgg_wm0_definesignal3sub(const int keycode);
void sgg_wm0_definesignal3sub2(const int rawcode, const int shiftmap);
void sgg_wm0_definesignal3sub3(int rawcode, const int shiftmap);
void sgg_wm0_definesignal3com();


/* キー操作：
      F9:一番下のウィンドウへ
      F10:上から２番目のウィンドウを選択
      F11:ウィンドウの移動
      F12:ウィンドウクローズ */

//int allclose = 0;

static int tapisigvec[] = {
	0x006c, 6 * 4, 0x011c /* cmd fot tapi */, 0, 0, 0x0000, 0x0000
};

void main()
{
	int *signal, *signal0, i, j;
	struct WM0_WINDOW *win;

	#if (defined(TOWNS))
		static int TOWNS_mouseinit[] = {
			0x0064, 17 * 4, 0x0030 /* SetMouseParam */, 0x020d,
			20 /* サンプリングレート 20ミリ秒 */,
			0, 0, /* TAPIのシグナル処理ベクタ(TAPI_SignalMessageTimer) */
			0x3245, 0x7f000004, 0x73756f6d, 0,
			0x000d0019 /* wait0=25, wait1=13 */, 0x0f3f0f0f /* strobe */,
			0x00000000, 0x000000, 0x0f0f0f3f, 0x00000030,
			0 /* eoc */
		};
	#endif

	lib_init(AUTO_MALLOC);
	sgg_init(AUTO_MALLOC);

	signal = signal0 = lib_opensignalbox(256 * 4, AUTO_MALLOC, 0, 4); // 1KB

	// ウィンドウを確保して、全て未使用ウインドウとして登録
	window = (struct WM0_WINDOW *) malloc(MAX_WINDOWS * sizeof (struct WM0_WINDOW));
	for (i = 0; i < MAX_WINDOWS; i++)
		chain_unuse(&window[i]);

	// サウンドトラック用バッファの初期化
	sndtrk_buf = (struct SOUNDTRACK *) malloc(MAX_SOUNDTRACK * sizeof (struct SOUNDTRACK));
	for (i = 0; i < MAX_SOUNDTRACK; i++)
		free_sndtrk(&sndtrk_buf[i]);

	joblist = (int *) malloc(JOBLIST_SIZE * sizeof (int));
	*(jobrp = jobwp = joblist) = 0; /* たまった仕事はない */
	jobfree = JOBLIST_SIZE - 1;

	defsigbuf = (struct DEFINESIGNAL *) malloc (DEFSIGBUFSIZ * sizeof (struct DEFINESIGNAL));
	for (i = 0; i < DEFSIGBUFSIZ - 1; i++)
		defsigbuf[i].win = NULL;
	defsigbuf[DEFSIGBUFSIZ - 1].win = -1;

	sgg_execcmd(tapisigvec);

	#if (defined(TOWNS))
		TOWNS_mouseinit[5] = tapisigvec[3];
		TOWNS_mouseinit[6] = tapisigvec[4];
		sgg_execcmd(TOWNS_mouseinit);
	#endif

	for (;;) {
		win = top;
		switch (signal[0]) {
		case 0x0000:
			lib_waitsignal(0x0001, 0, 0);
			break;

		case 0x0004 /* rewind */:
			lib_waitsignal(0x0000, signal[1], 0);
			signal = signal0;
			break;

		case 0x0010:
			/* 初期化要請 */
			signal++;
			lib_waitsignal(0x0000, 1, 0);

			/* ジョブリストにこの要求を入れる */
			if (jobfree >= 4) {
				/* 空きが十分にある */
				#if (defined(PCAT))
					writejob2(0x0030 /* open VGA driver */, 0x0000);
					writejob2(0x0034 /* change VGA mode */, 0x0012);
		fin_wrtjob:
					*jobwp = 0; /* ストッパー */
					if (jobnow == 0)
						runjobnext();
				#endif
				#if (defined(TOWNS))
					writejob2(0x0030 /* open VGA driver */, 0x0000);
					writejob2(0x0034 /* change VGA mode */, 0x0000);
		fin_wrtjob:
					*jobwp = 0; /* ストッパー */
					if (jobnow == 0)
						runjobnext();
				#endif
			}
			break;

		case 0x0018:
			/* from boot */
			fromboot = signal[1];
			signal += 2;
			lib_waitsignal(0x0000, 2, 0);
			break;

		case 0x001c:
			/* 終了要請 */
#if 0
			if (signal[1] == 4) {
				/* close all-window(含むpokon0) */
				signal += 2;
				lib_waitsignal(0x0000, 2, 0);
	close_all:
				allclose = 0;
				if (top == pokon0 && top->down == pokon0)
					break;
				allclose = 1;
				win = top;
				if (win == pokon0)
					win = win->down;
				sgg_wm0s_close(&win->sgg);
				closewin = win;
				break;
			}
#endif
			goto mikannsei;
			break;

		case 0x0020:
			/* ウィンドウオープン要請(handle) */
			win = get_unuse();
			win->flags = 0;
			sgg_wm0_openwindow(&win->sgg, signal[1]);
			signal += 2;
			lib_waitsignal(0x0000, 2, 0);
		//	win->ds1 = win->defsig;

			/* ジョブリストにこの要求を入れる */
			if (jobfree >= 2) {
				// 空きが十分にある
			//	writejob(0x0020 /* open */);
			//	writejob((int) win);
				writejob2(0x0020 /* open */, (int) win);
			//	*jobwp = 0; /* ストッパー */
			//	if (jobnow == 0)
			//		runjobnext();
				goto fin_wrtjob;
			}
			break;

		case 0x0024:
			/* ウィンドウクローズ要請(handle) */
			win = handle2window(signal[1]);
			signal += 2;
			lib_waitsignal(0x0000, 2, 0);
			/* ジョブリストにこの要求を入れる */
			if ((win->flags & 0x01) == 0 && jobfree >= 2) {
				// 空きが十分にある
				win->flags |= 0x01; /* クローズ処理中 */
			//	writejob(0x002c /* close */);
			//	writejob((int) win);
				writejob2(0x002c /* close */, (int) win);
			//	*jobwp = 0; /* ストッパー */
			//	if (jobnow == 0)
			//		runjobnext();
				goto fin_wrtjob;
			}
			break;

		case 0x0028:
			/* ウィンドウアクティブ要請(opt, handle) */
			goto mikannsei;
			break;

		case 0x002c:
			/* ウィンドウ連動デバイス指定
			    (opt,  win-handle, reserve(signalebox),
			       default-device, default-code, len(2), 0x7f000001, signal) */

			win = handle2window(signal[2]);
			{
				struct DEFINESIGNAL *dsp;
				for (dsp = defsigbuf; dsp->win != NULL && dsp->win != -1; dsp++);
				if (dsp->win == NULL) {
					dsp->win = (int) win;
					dsp->opt = signal[1];
					dsp->dev = signal[4];
					dsp->cod = signal[5];
					dsp->len = 2;
					dsp->sig[0] = signal[7];
					dsp->sig[1] = signal[8];
					if (iactive == win) {
						int sigbox = sgg_wm0_win2sbox(&win->sgg);
						sgg_wm0_definesignal3(dsp->opt, dsp->dev,
							dsp->cod, sigbox, dsp->sig[0], dsp->sig[1]);
					}
				}
			}
			signal += 9;
			lib_waitsignal(0x0000, 9, 0);
			break;

		case 0x0040: /* open sound track (slot, tss, signal-base, reserve0, reserve1)
			   受理したことを知らせるために、シグナルで応答する */
			{
				struct SOUNDTRACK *sndtrk = alloc_sndtrk();
				sndtrk->sigbox  = signal[2 /* tss */] + 0x0240;
				sndtrk->slot    = signal[1 /* slot */];
				sndtrk->sigbase = signal[3 /* signal-base */];
				signal += 6;
				lib_waitsignal(0x0000, 6, 0);
				/* ハンドル番号の対応づけを通達 */
				send_signal3dw(sndtrk->sigbox, sndtrk->sigbase + 0, sndtrk->slot, (int) sndtrk);
				if (sndtrk_active == NULL) {
					sndtrk_active = sndtrk;
					/* アクティブシグナルを送る */
					send_signal2dw(sndtrk->sigbox, sndtrk->sigbase + 8 /* enable */, sndtrk->slot);
				}
			}
			break;

		case 0x0044: /* close sound track (handle) */
			{
				struct SOUNDTRACK *sndtrk = (struct SOUNDTRACK *) signal[1];
				signal += 2;
				lib_waitsignal(0x0000, 2, 0);
				/* close完了をしらせる */
				send_signal2dw(sndtrk->sigbox, sndtrk->sigbase + 4 /* close */, sndtrk->slot);
				free_sndtrk(sndtrk);
				if (sndtrk == sndtrk_active) {
					/* 違うやつをアクティブにする */
					sndtrk = NULL;
					for (i = 0; i < MAX_SOUNDTRACK; i++) {
						if (sndtrk_buf[i].sigbox) {
							sndtrk = &sndtrk_buf[i];
							break;
						}
					}
					if (sndtrk_active = sndtrk) {
						/* アクティブシグナルを送る */
						send_signal2dw(sndtrk->sigbox, sndtrk->sigbase + 8 /* enable */, sndtrk->slot);
					}
				}
			}
			break;

		case 0x0048: /* load external font (font-type, tss, len, sig) */
			/* ジョブリストにこの要求を入れる */
			if (jobfree >= 4) {
				/* 空きが十分にある */
				writejob2(0x0038 /* loadfont */, signal[1] /* type */);
				writejob2(signal[2] /* tss */, signal[4] /* sig */);
				*jobwp = 0; /* ストッパー */
				if (jobnow == 0)
					runjobnext();
			}
			signal += 5;
			lib_waitsignal(0x0000, 5, 0);
			break;

		case 0x0050:
		case 0x0051:
		case 0x0052:
		case 0x0053:
		case 0x0054: /* 0x005fまではリザーブ */
			(*jobfunc)(signal[0] - 0x0050, 0);
			signal++;
			lib_waitsignal(0x0000, 1, 0);
			if (jobnow == 0)
				runjobnext();
			break;

		case 0x0014: /* 画面モード変更完了(result) */
		case 0x00c0: /* 更新停止シグナル(handle) */
		case 0x00c4: /* 描画完了シグナル(handle) */
			i = signal[0];
			j = signal[1];
			signal += 2;
			lib_waitsignal(0x0000, 2, 0);
			(*jobfunc)(i, j);
			if (jobnow == 0)
				runjobnext();
			break;

		case 0x00d0:
		case 0x00d1:
		case 0x00d2:
		case 0x00d3:
		case 0x00f0:
			i = signal[0];
			signal++;
			lib_waitsignal(0x0000, 1, 0);
			job_movewin4(i);
			break;


		case 0x0200 /* active bottom window */:
			signal++;
			lib_waitsignal(0x0000, 1, 0);
			/* ジョブリストにこの要求を入れる */
			if ((win->flags & 0x01) == 0 && jobfree >= 2) {
				/* 空きが十分にある */
			//	writejob(0x0024 /* active */);
			//	writejob((int) top->up);
				writejob2(0x0024 /* active */, (int) win /* top */ ->up);
			//	*jobwp = 0; /* ストッパー */
			//	if (jobnow == 0)
			//		runjobnext();
				goto fin_wrtjob;
			}
			break;

		case 0x0201 /* active second window */:
			signal++;
			lib_waitsignal(0x0000, 1, 0);
			/* ジョブリストにこの要求を入れる */
			if ((win->flags & 0x01) == 0 && jobfree >= 2) {
				/* 空きが十分にある */
			//	writejob(0x0024 /* active */);
			//	writejob((int) top->down);
				writejob2(0x0024 /* active */, (int) win /* top */ ->down);
			//	*jobwp = 0; /* ストッパー */
			//	if (jobnow == 0)
			//		runjobnext();
				goto fin_wrtjob;
			}
			break;

		case 0x0202 /* move window */:
			signal++;
			lib_waitsignal(0x0000, 1, 0);
			/* ジョブリストにこの要求を入れる */
			if ((win->flags & 0x01) == 0 && jobfree >= 2) {
				// 空きが十分にある
			//	writejob(0x0028 /* move by keyboard */);
			//	writejob((int) top);
				writejob2(0x0028 /* move by keyboard */, (int) win /* top */);
			//	*jobwp = 0; /* ストッパー */
			//	if (jobnow == 0)
			//		runjobnext();
				goto fin_wrtjob;
			}
			break;

		case 0x0203 /* close window */:
			signal++;
			lib_waitsignal(0x0000, 1, 0);
			if (win /* top */ != pokon0 && (win->flags & 0x01) == 0)
				sgg_wm0s_close(&win /* top */ ->sgg);
			break;

		case 0x0204 /* VGA mode 0x0012 */:
			signal++;
			lib_waitsignal(0x0000, 1, 0);

			/* ジョブリストにこの要求を入れる */
			if (jobfree >= 2) {
				/* 空きが十分にある */
				#if (defined(PCAT))
				//	writejob(0x0034 /* change VGA mode */);
				//	writejob(0x0012);
					writejob2(0x0034 /* change VGA mode */, 0x0012);
				//	*jobwp = 0; /* ストッパー */
				//	if (jobnow == 0)
				//		runjobnext();
					goto fin_wrtjob;
				#endif
				#if (defined(TOWNS))
				//	writejob(0x0034 /* change VGA mode */);
				//	writejob(0x0000);
					writejob2(0x0034 /* change VGA mode */, 0x0000);
				//	*jobwp = 0; /* ストッパー */
				//	if (jobnow == 0)
				//		runjobnext();
					goto fin_wrtjob;
				#endif
			}
			break;

		case 0x0205 /* VESA mode 0x0102 */:
			signal++;
			lib_waitsignal(0x0000, 1, 0);

			/* ジョブリストにこの要求を入れる */
			if (jobfree >= 2) {
				/* 空きが十分にある */
				#if (defined(PCAT))
				//	writejob(0x0034 /* change VGA mode */);
				//	writejob(0x0102);
					writejob2(0x0034 /* change VGA mode */, 0x0102);
				//	*jobwp = 0; /* ストッパー */
				//	if (jobnow == 0)
				//		runjobnext();
					goto fin_wrtjob;
				#endif
				#if (defined(TOWNS))
				//	writejob(0x0034 /* change VGA mode */);
				//	writejob(0x0001);
					writejob2(0x0034 /* change VGA mode */, 0x0001);
				//	*jobwp = 0; /* ストッパー */
				//	if (jobnow == 0)
				//		runjobnext();
					goto fin_wrtjob;
				#endif
			}
			break;

		case 0x0240 /* load JPN16$.FNT */:
			signal++;
			lib_waitsignal(0x0000, 1, 0);

			/* ジョブリストにこの要求を入れる */
			if (jobfree >= 4) {
				/* 空きが十分にある */
				writejob2(0x0038 /* loadfont */, 0x11 /* type */);
				writejob2(0, 0);
			//	*jobwp = 0; /* ストッパー */
			//	if (jobnow == 0)
			//		runjobnext();
				goto fin_wrtjob;
			}
			break;

		case 0x73756f6d /* from mouse */:
			#if (defined(TOWNS))
				if (mx != 0x80000000) {
					mousesignal(((signal[3] >> 4) & 0x03) ^ 0x03,
						- (signed char) (((signal[3] >> 20) & 0xf0) | ((signal[3] >> 16) & 0x0f)),
						- (signed char) (((signal[3] >>  4) & 0xf0) | ( signal[3]        & 0x0f)));
				}
				signal += 4;
				lib_waitsignal(0x0000, 4, 0);
				break;
			#endif
			#if (defined(PCAT))
				if (mx != 0x80000000)
					mousesignal(signal[1], (signed short) (signal[2] & 0xffff), (signed short) (signal[2] >> 16));
				signal += 3;
				lib_waitsignal(0x0000, 3, 0);
				break;
			#endif

		case 0x6f6b6f70 + 0 /* mousespeed */:
			mousescale = signal[1];
			signal += 2;
			lib_waitsignal(0x0000, 2, 0);
			break;

		default:
		mikannsei:
		//	lib_drawline(0x0020, (void *) -1, 0, 0, 0, 15, 15); /* ここに来たことを知らせる */
			signal++;
			lib_waitsignal(0x0000, 1, 0);
		}
	}
}

void init_screen(const int x, const int y)
{
	lib_drawline(0x0020, (void *) -1,  6,      0,      0, x -  1, y - 29);
	lib_drawline(0x0020, (void *) -1,  8,      0, y - 28, x -  1, y - 28);
	lib_drawline(0x0020, (void *) -1, 15,      0, y - 27, x -  1, y - 27);
	lib_drawline(0x0020, (void *) -1,  8,      0, y - 26, x -  1, y -  1);
	lib_drawline(0x0020, (void *) -1, 15,      3, y - 24,     59, y - 24);
	lib_drawline(0x0020, (void *) -1, 15,      2, y - 24,      2, y -  4);
	lib_drawline(0x0020, (void *) -1,  7,      3, y -  4,     59, y -  4);
	lib_drawline(0x0020, (void *) -1,  7,     59, y - 23,     59, y -  5);
	lib_drawline(0x0020, (void *) -1,  0,      2, y -  3,     59, y -  3);
	lib_drawline(0x0020, (void *) -1,  0,     60, y - 24,     60, y -  3);
	lib_drawline(0x0020, (void *) -1,  7, x - 47, y - 24, x -  4, y - 24);
	lib_drawline(0x0020, (void *) -1,  7, x - 47, y - 23, x - 47, y -  4);
	lib_drawline(0x0020, (void *) -1, 15, x - 47, y -  3, x -  4, y -  3);
	lib_drawline(0x0020, (void *) -1, 15, x -  3, y - 24, x -  3, y -  3);
	sgg_wm0_putmouse(mx, my);
	return;
}

struct WM0_WINDOW *handle2window(const int handle)
{
	// topの中から探してもいい
	int i;
	for (i = 0; i < MAX_WINDOWS; i++) {
		if (window[i].sgg.handle == handle)
			return &(window[i]);
	}
	return NULL;
}

void chain_unuse(struct WM0_WINDOW *win)
{
	// unuseは一番上
	// winは一番下に追加
	win->sgg.handle = 0; // handle2windowが間違って検出しないために
	if (unuse) {
		struct WM0_WINDOW *bottom;
		bottom = unuse->up;
		win->down = unuse;
		win->up = bottom;
		unuse->up = win;
		bottom->down = win;
	} else {
		unuse = win;
		win->up = win;
		win->down = win;
	}
	return;
}

struct WM0_WINDOW *get_unuse()
{
	// 一番上からとってくる
	struct WM0_WINDOW *win = unuse;
	struct WM0_WINDOW *bottom = unuse->up;
	unuse = unuse->down;
	unuse->up = bottom;
	bottom->down = unuse;
	if (win == unuse)
		unuse = NULL;
	return win;
}

static enum {
	NO_PRESS, CLOSE_BUTTON, TITLE_BAR
} press_pos = NO_PRESS;
static struct WM0_WINDOW *press_win;
static int press_mx0, press_my0;

void mousesignal(const unsigned int header, int dx, int dy)
{
	dx *= mousescale;
	dy *= mousescale;
	if ((header >> 28) == 0x0 /* normal mode */) {
		// マウス状態変更
		int ox = mx, oy = my;
		if (job_movewin4_ready != 0 && (dx | dy) != 0) {
			if (press_pos == NO_PRESS) {
				int xsize = job_win->x1 - job_win->x0;
				int ysize = job_win->y1 - job_win->y0;
				if (job_movewin_x <= mx && mx < job_movewin_x + xsize &&
					job_movewin_y <= my && my < job_movewin_y + ysize) {
					if (press_mx0 < 0) {
						press_mx0 = mx;
						press_my0 = my;
					}
					job_movewin4m(mx + dx, my + dy);
				} else {
					press_mx0 = mx = (job_win->x0 + job_win->x1) / 2;
					press_my0 = my = job_win->y0 + 12;
					dx = dy = 0;
				}
			} else if (press_pos == TITLE_BAR && press_win == job_win)
				job_movewin4m(mx + dx, my + dy);
		}
		mx += dx;
		my += dy;
		if (mx < 0)
			mx = 0;
		if (mx >= x2)
			mx = x2 - 1;
		if (my < 0)
			my = 0;
		if (my >= y2 - 15)
			my = y2 - 16;
		if (mx != ox || my != oy)
			sgg_wm0_movemouse(mx, my);
		if (mbutton != (header & 0x07)) {
			struct WM0_WINDOW *win;
			// マウスのボタン状態が変化
			// bit0:left
			// bit1:right
			// bit2:middle
			// bit3:always 1
			// bit4:reserve(dx bit8)
			// bit5:reserve(dy bit8)
#if 0
			// どのwindowをクリックしたのかを検出
			if (win = top) {
				do {
					if (win->x0 <= mx && mx < win->x1 && win->y0 <= my && my < win->y1)
						goto found_window;
					win = win->down;
				} while (win != top);
				win = NULL;
			}
			if (win != NULL) {
found_window:

/*
	tx0 = x0 +  3
	ty0 = y0 +  3
	tx1 = x1 -  4
	ty1 = y0 + 21
*/
				if (win == top) {
					/* ウィンドウタイトル部分のプレスか？ */
						if (win->tx0 <= mx && mx < win->tx1 && win->ty0 <= my && my < win->ty0) {
							/* title-barをプレスした */
							if (jobfree >= 2) {
								/* 空きが十分にある */
								press_win = win;
								press_pos = TITLE_BAR;
								press_mx0 = mx;
								press_my0 = my;
								writejob(0x0028 /* move */);
								writejob((int) win);
								*jobwp = 0; /* ストッパー */
							//	if (jobnow == 0)
							//		runjobnext();
							}
						}
#endif

			if ((mbutton & 0x01) == 0x00 && (header & 0x01) == 0x01) {
				// 左ボタンが押された

				if (job_movewin4_ready != 0 && press_pos == NO_PRESS) {
					job_movewin4(0x00f0 /* Enter */);
					goto skip;
				}

				// どのwindowをクリックしたのかを検出
				if (win = top) {
					do {
						if (win->x0 <= mx && mx < win->x1 && win->y0 <= my && my < win->y1)
							goto found_window;
						win = win->down;
					} while (win != top);
					win = NULL;
				}
				if (win != NULL) {
		found_window:
					if (win == top) {
						// アクティブなウィンドウの中にマウスがある
						if (win->x1 - 21 <= mx && mx < win->x1 - 5 && win->y0 + 5 <= my && my < win->y0 + 19) {
							// close buttonをプレスした
							press_win = win;
							press_pos = CLOSE_BUTTON;
						} else if (win->x0 + 3 <= mx && mx < win->x1 - 4 && win->y0 + 3 <= my && my < win->y0 + 21) {
							/* title-barをプレスした */
							if (jobfree >= 2) {
								/* 空きが十分にある */
								press_win = win;
								press_pos = TITLE_BAR;
								press_mx0 = mx;
								press_my0 = my;
								writejob(0x0028 /* move */);
								writejob((int) win);
								*jobwp = 0; /* ストッパー */
							//	if (jobnow == 0)
							//		runjobnext();
							}
						}
					} else {
						/* ジョブリストにウィンドウアクティブ要求を入れる */
						if (jobfree >= 2) {
							/* 空きが十分にある */
							writejob(0x0024 /* active */);
							writejob((int) win);
							*jobwp = 0; /* ストッパー */
						}
					}
				}
			} else if ((mbutton & 0x01) == 0x01 && (header & 0x01) == 0x00) {
				/* 左ボタンがはなされた */
				switch (press_pos) {
				case CLOSE_BUTTON:
					if (press_win->x1 - 21 <= mx && mx < press_win->x1 - 5 &&
						press_win->y0 + 5 <= my && my < press_win->y0 + 19) {
						if (press_win != pokon0)
							sgg_wm0s_close(&press_win->sgg);
					}
					break;

				case TITLE_BAR:
					if (press_win == job_win && job_movewin4_ready != 0) {
						/* 移動先確定シグナルを送る */
						job_movewin4(0x00f0 /* Enter */);
					}
				//	break;

				}
				press_pos = NO_PRESS;
			}
skip:
			mbutton = header & 0x07;
		}
	} else if ((header >> 28) == 0xa /* extmode byte2 */) {
		/* マウスリセット */
		mbutton = 0;
		sgg_wm0_enablemouse();
		#if (defined(DEBUG))
			lib_drawletters_ASCII(1, -1, 0xc0,  0, 0, 15, 0, "mouse reset");
		#endif
	} else {
		/* mikannsei */
	}
	/* 溜まったジョブがあれば、実行する */
	if (jobnow == 0)
		runjobnext();
	return;
}

void writejob(int i)
{
	*jobwp++ = i;
	jobfree--;
	if (jobwp == joblist + JOBLIST_SIZE)
		jobwp = joblist;
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
	int i = *jobrp++;
	jobfree++;
	if (jobrp == joblist + JOBLIST_SIZE)
		jobrp = joblist;
	return i;
}

void runjobnext()
{
	do {
		if ((jobnow = *jobrp) == 0)
			return;

		readjob(); // から読み
		switch (jobnow) {

		case 0x0020 /* open new window */:
			job_openwin0((struct WM0_WINDOW *) readjob());
			break;

		case 0x0024 /* active window */:
			job_activewin0((struct WM0_WINDOW *) readjob());
			break;

		case 0x0028 /* move window by keyboard */:
			job_movewin0((struct WM0_WINDOW *) readjob());
			break;

		case 0x002c /* close window */:
			job_closewin0((struct WM0_WINDOW *) readjob());
			break;

		case 0x0030 /* open VGA driver */:
			job_openvgadriver(readjob());
			break;

		case 0x0034 /* set VGA mode */:
			job_setvgamode0(readjob());
			break;

		case 0x0038 /* load external font */:
			job_loadfont0(readjob(), readjob(), readjob());
		//	break;
		}
	} while (jobnow == 0);
	return;
}

const int overrapwin(const struct WM0_WINDOW *win0, const struct WM0_WINDOW *win1)
{
	return win0->x0 < win1->x1 && win1->x0 < win0->x1 && win0->y0 < win1->y1 && win1->y0 < win0->y1;
}

void job_openwin0(struct WM0_WINDOW *win)
{
	int xsize = sgg_wm0_winsizex(&win->sgg);
	int ysize = sgg_wm0_winsizey(&win->sgg);

	// まず、座標を決める
	if (top == NULL) {
		// for pokon0
		win->x0 = 16;
		win->y0 = 32;
		pokon0 = win;
	} else {
		win->x0 = (x2 - xsize) / 2;
		win->x0 &= ~0x07; // オープン位置を8の倍数になるように調整
		win->y0 = (y2 - 28 - ysize) / 2;
	}

	// 各種パラメーターの初期化
	win->x1 = win->x0 + xsize;
	win->y1 = win->y0 + ysize;
	win->job_flag0 = WINFLG_MUSTREDRAW | WINFLG_OVERRIDEDISABLED; // override-disable & must-redraw

	// 入力アクティブを変更
	redirect_input(win); // この関数は、ウィンドウ制御はしない
	iactive = win;

	// 接続
	if (top == NULL)
		win->up = win->down = top = win;
	else {
		struct WM0_WINDOW *bottom;
		win->up = bottom = top->up;
		win->down = top;
		top->up = win;
		bottom->down = win;
		top = win;
	}

	sgg_wm0s_movewindow(&win->sgg, win->x0, win->y0);
	sgg_wm0s_setstatus(&win->sgg, win->condition = 0x03 /* bit0:表示, bit1:入力 */);
	job_general1();
	return;
}

void redirect_input(struct WM0_WINDOW *win)
{
	// キー入力シグナルの対応づけを初期化
//	sgg_wm0_definesignal0(255, 0x0100, 0);
	sgg_wm0_definesignal3com();

	// winman0のキー操作を登録(F9～F12)
	sgg_wm0_definesignal3(3, 0x0100, 0x00701089 /* F9～F12 */,
		0x3240 /* winman0 signalbox */, 0x7f000001, 0x0200);

	sgg_wm0_definesignal3(1, 0x0100, 0x00701081 /* F1 */,
		0x3240 /* winman0 signalbox */, 0x7f000001, 0x0204);

	sgg_wm0_definesignal3(0, 0x0100, 0x00701085 /* F5 */,
		0x3240 /* winman0 signalbox */, 0x7f000001, 0x0240); /* JPN16$.FNTの即時ロード */

	if (win) {
		struct DEFINESIGNAL *dsp;
		int sigbox = sgg_wm0_win2sbox(&win->sgg);
		for (dsp = defsigbuf; dsp->win != -1; dsp++) {
			if (dsp->win == (int) win) {
				if (dsp->len == 2) {
					sgg_wm0_definesignal3(dsp->opt, dsp->dev,
						dsp->cod, sigbox, dsp->sig[0], dsp->sig[1]);
				}
			}
		}
	}
	return;
}

void job_activewin0(struct WM0_WINDOW *win)
{
	int x0, y0, x1, y1;
	struct WM0_WINDOW *win_up, *win_down;

	if (top == win) {
		/* topとiactiveは常に等しい */
		jobnow = 0;
		return;
	}

	/* winをリストから一度切り離す */
	win_up = win->up;
	win_down = win->down;
	win_up->down = win_down;
	win_down->up = win_up;

	/* 再接続 */
	win->up = win_down = top->up;
	win->down = top;
	top->up = win;
	win_down->down = win;
	top = win;

	/* 入力アクティブを変更 */
	redirect_input(win); /* この関数は、ウィンドウ制御はしない */
	iactive = win;
	win = top;
	do {
		win->job_flag0 = 0;
	} while ((win = win->down) != top);
	job_general1();
	return;
}

void job_movewin0(struct WM0_WINDOW *win)
{
	// winは、必ずtopで、かつ、iactiveであること

	job_win = top /* win */;

	// キー入力シグナルの対応づけを初期化
//	sgg_wm0_definesignal0(255, 0x0100, 0);
	sgg_wm0_definesignal3com();

	// とりあえず、全てのウィンドウの画面更新権を一時的に剥奪する
	job_count = 0;
	win = top;
	jobfunc = &job_movewin1;
	do {
		win->job_flag0 = 0x01;
		if (win->condition & 0x01) {
			job_count++; // disable
			sgg_wm0s_accessdisable(&win->sgg);
		}
		win = win->down;
	} while (win != top);

//	以下は成立しない(最低一つは出力アクティブなウィンドウが存在するから)
//	if (job_count == 0) {
//		job_movewin2(); // すぐにウィンドウ枠表示へ
//	}

	return;
}

void job_movewin1(const int cmd, const int handle)
{
	if (cmd != 0x00c0) {
		#if (defined(DEBUG))
			unsigned char s[12];
			s[8] = '\0';
			debug_bin2hex(cmd, s);    lib_drawletters_ASCII(1, -1, 0xc0,  0, 0, 13, 0, s);
			debug_bin2hex(handle, s); lib_drawletters_ASCII(1, -1, 0xc0, 80, 0, 13, 0, s);
		#endif
		return;
	}
	// 0x00c0しかこない
	if (--job_count == 0)
		job_movewin2();
	return;
}

void job_movewin2()
{
	// シグナルを宣言(0x00d0～0x00d3, 0x00f0)
	sgg_wm0_definesignal3(3, 0x0100, 0x00ac /* left */,
		0x3240 /* winman0 signalbox */, 0x7f000001, 0x00d0);
	sgg_wm0_definesignal3(0, 0x0100, 0x00a0 /* Enter */,
		0x3240 /* winman0 signalbox */, 0x7f000001, 0x00f0);

	// 枠を描く
	job_movewin_x0 = job_movewin_x = job_win->x0;
	job_movewin_y0 = job_movewin_y = job_win->y0;
	job_movewin3();
	job_movewin4_ready = 1;
	if (press_pos == NO_PRESS) {
		if (job_movewin_x > mx || mx >= job_win->x1 || job_movewin_y > my || my >= job_win->y1) {
			press_mx0 = mx = (job_win->x0 + job_win->x1) / 2;
			press_my0 = my = job_win->y0 + 12;
			sgg_wm0_movemouse(mx, my);
		}
	}
	return;
}

void job_movewin3()
{
	int x0, y0, x1, y1;
	struct WM0_WINDOW *win = job_win;

	x0 = job_movewin_x;
	y0 = job_movewin_y;
	x1 = x0 + win->x1 - win->x0 - 1;
	y1 = y0 + win->y1 - win->y0 - 1;

	lib_drawline(0x00e0, (void *) -1, 9, x0,     y0,     x1 - 3, y0 + 2);
	lib_drawline(0x00e0, (void *) -1, 9, x0 + 3, y1 - 2, x1,     y1    );
	lib_drawline(0x00e0, (void *) -1, 9, x0,     y0 + 3, x0 + 2, y1    );
	lib_drawline(0x00e0, (void *) -1, 9, x1 - 2, y0,     x1,     y1 - 3);
	return;
}

void job_movewin4(int sig)
{
	struct WM0_WINDOW *win0 = job_win;

	int x0 = job_movewin_x;
	int y0 = job_movewin_y;
	int xsize = win0->x1 - win0->x0;
	int ysize = win0->y1 - win0->y0;

	#if (defined(TOWNS))
		if (sig == 0x00d0 && x0 >= 4)
			x0 -= 4;
		if (sig == 0x00d1 && x0 + xsize <= x2 - 4)
			x0 += 4;
		if (sig == 0x00d2 && y0 >= 4)
			y0 -= 4;
		if (sig == 0x00d3 && y0 + ysize <= y2 - 32)
			y0 += 4;
	#endif
	#if (defined(PCAT))
		if (sig == 0x00d0 && x0 >= 8)
			x0 -= 8;
		if (sig == 0x00d1 && x0 + xsize <= x2 - 8)
			x0 += 8;
		if (sig == 0x00d2 && y0 >= 8)
			y0 -= 8;
		if (sig == 0x00d3 && y0 + ysize <= y2 - 36)
			y0 += 8;
	#endif

	if ((x0 - job_movewin_x) | (y0 - job_movewin_y)) {
		if (press_pos == NO_PRESS || press_pos == TITLE_BAR) {
			mx += x0 - job_movewin_x;
			my += y0 - job_movewin_y;
			sgg_wm0_movemouse(mx, my);
		}
		job_movewin3();
		job_movewin_x = x0;
		job_movewin_y = y0;
		job_movewin3();
	}

	if (sig == 0x00f0) {
		job_movewin4_ready = 0;
		press_mx0 = -1;
		job_movewin3();
		struct WM0_WINDOW *win1;
		win0->job_flag0 = WINFLG_MUSTREDRAW | WINFLG_OVERRIDEDISABLED; // override-disabled & must-redraw
		win1 = win0 /* top */->down;
		do {
			int flag0 = WINFLG_OVERRIDEDISABLED; // override-disabled
			if (overrapwin(win0, win1))
				flag0 = WINFLG_MUSTREDRAW | WINFLG_OVERRIDEDISABLED; // override-disabled & must-redraw
			win1->job_flag0 = flag0;
		} while ((win1 = win1->down) != win0);

		// ウィンドウを消す
		lib_drawline(0x0020, (void *) -1, 6, win0->x0, win0->y0, win0->x1 - 1, win0->y1 - 1);

		win0->x0 = x0;
		win0->y0 = y0;
		win0->x1 = x0 + xsize;
		win0->y1 = y0 + ysize;
		sgg_wm0s_movewindow(&win0->sgg, win0->x0, win0->y0);
		redirect_input(win0);
		job_general1();
	}
	return;
}

void job_movewin4m(int x, int y)
{
	struct WM0_WINDOW *win0 = job_win;

	#if (defined(TOWNS))
		int x0 = job_movewin_x0 + (x - press_mx0) & ~3;
		int y0 = job_movewin_y0 + y - press_my0;
		int xsize = win0->x1 - win0->x0;
		int ysize = win0->y1 - win0->y0;
	#endif
	#if (defined(PCAT))
		int x0 = job_movewin_x0 + (x - press_mx0) & ~7;
		int y0 = job_movewin_y0 + y - press_my0;
		int xsize = win0->x1 - win0->x0;
		int ysize = win0->y1 - win0->y0;
	#endif

	if (x0 < 0)
		x0 = 0;
	if (x0 > x2 - xsize)
		x0 = x2 - xsize;
	if (y0 < 0)
		y0 = 0;
	if (y0 > y2 - ysize - 28)
		y0 = y2 - ysize - 28;
	if ((x0 - job_movewin_x) | (y0 - job_movewin_y)) {
		job_movewin3();
		job_movewin_x = x0;
		job_movewin_y = y0;
		job_movewin3();
	}
	return;
}

void job_closewin0(struct WM0_WINDOW *win0)
// このウィンドウは既にaccessdisableになっていることが前提
{
	struct WM0_WINDOW *win1, *win_up, *win_down;

	/* winをリストから切り離す */
	win_up = win0->up;
	win_down = win0->down;
	win_up->down = win_down;
	win_down->up = win_up;	

	sgg_wm0s_windowclosed(&win0->sgg);

	struct DEFINESIGNAL *dsp;
	for (dsp = defsigbuf; dsp->win != -1; dsp++) {
		if (dsp->win == (int) win0)
			dsp->win = NULL;
	}

	job_count = 0;
	jobfunc = &job_closewin1;

#if 0
	if (allclose) {
		lib_drawline(0x0020, (void *) -1, 6, win0->x0, win0->y0, win0->x1 - 1, win0->y1 - 1);
		chain_unuse(win0);
		allclose--;
		return;
	}
#endif

	if (win0 == top) {
		top = win_down;
		if (win_up == win0) {
			top = NULL;
			redirect_input(0);
			iactive = NULL;
			goto no_window;
		}
		redirect_input(top);
		iactive = top;
	}

	win1 = top;
	do {
		int flag0 = 0;
		if (overrapwin(win0, win1)) {
			flag0 = WINFLG_MUSTREDRAW | WINFLG_OVERRIDEDISABLED;
			if (win1->condition & 0x01) {
				sgg_wm0s_accessdisable(&win1->sgg);
				job_count++;
				flag0 = WINFLG_MUSTREDRAW | WINFLG_OVERRIDEDISABLED | WINFLG_WAITDISABLE;
			}
		}
		win1->job_flag0 = flag0;
	} while ((win1 = win1->down) != top);

no_window:

	/* ウィンドウを消す */
	lib_drawline(0x0020, (void *) -1, 6, win0->x0, win0->y0, win0->x1 - 1, win0->y1 - 1);
	chain_unuse(win0);
	if (job_count == 0)
		job_general1();
	return;
}

void job_closewin1(const int cmd, const int handle)
{
	struct WM0_WINDOW *win = handle2window(handle);

	if (cmd != 0x00c0 || (win->job_flag0 & WINFLG_WAITDISABLE) == 0) {
		#if (defined(DEBUG))
			unsigned char s[12];
			s[8] = '\0';
			debug_bin2hex(cmd, s);    lib_drawletters_ASCII(1, -1, 0xc0,  0, 0, 14, 0, s);
			debug_bin2hex(handle, s); lib_drawletters_ASCII(1, -1, 0xc0, 80, 0, 14, 0, s);
		#endif
		return;
	}
	win->job_flag0 &= ~WINFLG_WAITDISABLE;
	job_count--;
	if (job_count == 0)
		job_general1();
	return;
}

// ウィンドウ消去の場合、消したいウィンドウと重なっているものを全て"must-redraw"にしたあと
// ウィンドウを消し、あとは自動に任せる

// ウィンドウオープンの場合、追加して、後は自動に任せる

// ウィンドウ移動の場合、移動元と重なっているものを全て"must-redraw"にしたあと
// もといた場所を消し、あとは自動に任せる


void job_general1()
/* condition.bit 0 ... 0:accessdisable 1:accessenable
   condition.bit 1 ... 0:inputdisable(not active) 1:inputenable(active)

   job_flag0.bit 0 ... new condition.bit 0(auto-set)
   job_flag0.bit 1 ... new condition.bit 1(auto-set)
   job_flag0.bit 8 ... disable-accept waiting
   job_flag0.bit 9 ... redraw finish waiting
   job_flag0.bit24 ... 0:normal 1:override-accessdisabled
   job_flag0.bit31 ... 0:normal 1:must-redraw */
{
	struct WM0_WINDOW *win0, *win1, *bottom, *top_ = top /* 高速化、コンパクト化のため */;
	int flag0;

	if (top_ == NULL) {
		jobnow = 0;
	//	jobfunc = NULL;
		return;
	}

	/* accessenable & not input active */
	win0 = top_;
	do {
		flag0 = win0->job_flag0;
		flag0 |=  0x0001; /* accessenable */
		flag0 &= ~0x0302; /* not-input-active & no-waiting */
		if ((win0->condition & 0x01) == 0)
			flag0 |= WINFLG_OVERRIDEDISABLED; /* override-disabled */
		win0->job_flag0 = flag0;
	} while ((win0 = win0->down) != top_);

	/* 上から見ていって、重なっているものはaccessdisable */
	win0 = top_;
	do {
		for (win1 = win0->down; win1 != top_; win1 = win1->down) {
			if (win1->job_flag0 & 0x01) { /* このif文がなくても実行結果は変わらないが、高速化のため */
				if (overrapwin(win0, win1))
					win1->job_flag0 &= ~0x01; /* accessdisable */
			}
		}
	} while ((win0 = win0->down) != top_);

	/* topはjob_flag0.bit 1 = 1; */
	top_->job_flag0 |= 0x02; /* input-active */

	/* 下から見ていって、conditionが変化していたらjob_flag0.bit31 = 1;
	     job_flag0.bit31 == 1なら自分に重なっている上のやつ全てもjob_flag0.bit31 = 1; */
	win0 = bottom = top_->up; /* 一番上の上は、一番下 */
	do {
		flag0 = win0->job_flag0;
		if (win0->condition != (flag0 & 0x03) ||
			(flag0 & (WINFLG_OVERRIDEDISABLED | 0x01)) == (WINFLG_OVERRIDEDISABLED | 0x01))
			win0->job_flag0 = (flag0 |= WINFLG_MUSTREDRAW);
		if (flag0 & WINFLG_MUSTREDRAW) {
			for (win1 = win0->up; win1 != bottom; win1 = win1->up) {
				if ((win1->job_flag0 & WINFLG_MUSTREDRAW) == 0) { /* このif文がなくても実行結果は変わらないが、高速化のため */
					if (overrapwin(win0, win1))
						win1->job_flag0 |= WINFLG_MUSTREDRAW; /* must-redraw */
				}
			}
		}
	} while ((win0 = win0->down) != bottom);

	/* redrawを予定しているウィンドウで、他のredraw予定ウィンドウとオーバーラップしているものは、
		全てoverride-accessdisabledにする */
	job_count = 0;
	jobfunc = &job_general2;
	win0 = top_;
	do {
		if (win0->job_flag0 & WINFLG_MUSTREDRAW) {
			for (win1 = win0->down; win1 != win0; win1 = win1->down) {
				if (win1->job_flag0 & WINFLG_MUSTREDRAW) {
					if (overrapwin(win0, win1)) {
						if ((win0->job_flag0 & WINFLG_OVERRIDEDISABLED) == 0) {
							sgg_wm0s_accessdisable(&win0->sgg);
							win0->job_flag0 |= WINFLG_OVERRIDEDISABLED | WINFLG_WAITDISABLE;
							job_count++;
						}
						if ((win1->job_flag0 & (WINFLG_MUSTREDRAW | WINFLG_OVERRIDEDISABLED)) == WINFLG_MUSTREDRAW) {
							sgg_wm0s_accessdisable(&win1->sgg);
							win1->job_flag0 |= WINFLG_OVERRIDEDISABLED | WINFLG_WAITDISABLE;
							job_count++;
						}
					}
				}
			}
		}
	} while ((win0 = win0->down) != top_);
	job_win = NULL;
	if (job_count == 0)
		job_general2(0, 0);
	return;
}

void job_general2(const int cmd, const int handle)
{
	struct WM0_WINDOW *win;
	int flag0;

	if (cmd != 0 || handle != 0) {
		win = handle2window(handle);
		if (cmd == 0x00c0 /* 更新停止受理シグナル */) {
			if (win->job_flag0 & WINFLG_WAITDISABLE)
				win->job_flag0 &= ~WINFLG_WAITDISABLE;
			else {
				#if (defined(DEBUG))
					unsigned char s[12];
				#endif
error:
				#if (defined(DEBUG))
					s[8] = '\0';
					debug_bin2hex(cmd, s);    lib_drawletters_ASCII(1, -1, 0xc0,  0, 0, 15, 0, s);
					debug_bin2hex(handle, s); lib_drawletters_ASCII(1, -1, 0xc0, 80, 0, 15, 0, s);
				#endif
				return;
			}
		} else if (cmd == 0x00c4 /* 描画完了シグナル */) {
			if (win->job_flag0 & WINFLG_WAITREDRAW)
				win->job_flag0 &= ~WINFLG_WAITREDRAW;
			else
				goto error;
		}
		if (--job_count)
			return;
	}

	win = job_win;

	if (win == top && win->job_flag0 == 0) {
		/* １周目でwin->job_flag0が0になることもありうる */
		// 全ての作業が完了
		jobnow = 0;
	//	jobfunc = NULL;
		return;
	}
	if (win == NULL)
		win = top;

	do {
		win = win->up;
		flag0 = win->job_flag0;
		win->job_flag0 = 0;
		if ((flag0 & (WINFLG_OVERRIDEDISABLED | 0x01)) == (WINFLG_OVERRIDEDISABLED | 0x01) && x2 != 0)
			sgg_wm0s_accessenable(&win->sgg);
		if (flag0 & WINFLG_MUSTREDRAW) {
			job_win = win;
			if ((flag0 & (WINFLG_OVERRIDEDISABLED | 0x01)) == 0) {
				sgg_wm0s_accessdisable(&win->sgg);
				win->job_flag0 |= WINFLG_WAITDISABLE;
				job_count = 1;
			}
			if ((flag0 & 0x03) != win->condition)
				sgg_wm0s_setstatus(&win->sgg, win->condition = (flag0 & 0x03));
			sgg_wm0s_redraw(&win->sgg);
			win->job_flag0 |= WINFLG_WAITREDRAW;
			job_count++;
			return;
		}
	} while (win != top);
	jobnow = 0;
//	jobfunc = NULL;
	return;
}

void free_sndtrk(struct SOUNDTRACK *sndtrk)
{
	sndtrk->sigbox = 0;
	return;
}

struct SOUNDTRACK *alloc_sndtrk()
{
	int i;
	for (i = 0; i < MAX_SOUNDTRACK; i++) {
		if (sndtrk_buf[i].sigbox == 0)
			return &sndtrk_buf[i];
	}
	return NULL;
}

void send_signal2dw(const int sigbox, const int data0, const int data1)
{
#if 0
	static struct {
		int cmd, opt;
		int data[3];
		int eoc;
	} command = { 0x0020, 0x80000000 + 3, { 0 }, 0x0000 };

	command.data[0] = sigbox | 2;
	command.data[1] = data0;
	command.data[2] = data1;

	sgg_execcmd(&command);
	return;
#endif

	sgg_execcmd0(0x0020, 0x80000000 + 3, sigbox | 2, data0, data1, 0x000);
	return;
}

void send_signal3dw(const int sigbox, const int data0, const int data1, const int data2)
{
#if 0
	static struct {
		int cmd, opt;
		int data[4];
		int eoc;
	} command = { 0x0020, 0x80000000 + 4, { 0 }, 0x0000 };

	command.data[0] = sigbox | 3;
	command.data[1] = data0;
	command.data[2] = data1;
	command.data[3] = data2;

	sgg_execcmd(&command);
	return;
#endif
	sgg_execcmd0(0x0020, 0x80000000 + 4, sigbox | 3, data0, data1, data2, 0x000);
	return;
}

void gapi_loadankfont();

void job_openvgadriver(const int drv)
{
	// closeする時にすべてのウィンドウはdisableになっているはずなので、
	// ここではdisableにはしない
	sgg_wm0_gapicmd_0010_0000();
	gapi_loadankfont();
	jobnow = 0;
	return;
}

void job_setvgamode0(const int mode)
{
	struct WM0_WINDOW *win;
	job_int0 = mode;

	if (mx != 0x80000000) {
		sgg_wm0_removemouse();
	} else
		mx = my = 1;

	// とりあえず、全てのウィンドウの画面更新権を一時的に剥奪する
	job_count = 0;
	jobfunc = &job_setvgamode1;
	if (win = top) {
		do {
			win->job_flag0 = (WINFLG_MUSTREDRAW | WINFLG_OVERRIDEDISABLED); // override-accessdisabled & must-redraw
			if ((win->condition & 0x01) != 0 && x2 != 0) {
				job_count++; // disable
				sgg_wm0s_accessdisable(&win->sgg);
			}
		} while ((win = win->down) != top);
	}

	if (job_count == 0) {
		job_setvgamode2(); // すぐにディスプレーモード変更へ
	}

	return;
}

void job_setvgamode1(const int cmd, const int handle)
{
	// 0x00c0しかこない
	if (--job_count == 0)
		job_setvgamode2();
	return;
}

void job_setvgamode2()
{
	#if (defined(TOWNS))
#if 0
		int mode = job_int0;
		switch (mode) {
		case 0x0000:
			x2 = 1024;
			y2 = 480;
			/* 画面モード0設定(640x480) */
			sgg_execcmd0(0x0050, 7 * 4, 0x001c, 0, 0x0020, 0, 0x0000, 0x0000);
			break;

		case 0x0001:
			x2 = 1024;
			y2 = 512;
			/* 画面モード1設定(768x512) */
			sgg_execcmd0(0x0050, 7 * 4, 0x001c, 0, 0x0020, 1, 0x0000, 0x0000);

		}
#endif
			x2 = 1024;
			y2 = 512;
			/* 画面モード0設定(640x480) */
			/* 画面モード1設定(768x512) */
			sgg_execcmd0(0x0050, 7 * 4, 0x001c, 0, 0x0020, job_int0 /* mode */, 0x0000, 0x0000);


		sgg_wm0_gapicmd_001c_0004(); /* ハードウェア初期化 */
		init_screen(x2, y2);
		job_general1();
		return;
	#endif

	#if (defined(PCAT))
		#if (defined(BOCHS))
			x2 = 640; /* Bochsは仮想画面が使えない */
			y2 = 480;
		#else
			x2 = 800;
			y2 = 600;
		#endif

		if (fromboot & 0x0001) {
			/* 普通の方法が使えない */
			/* (仮想86モードでのVGAモード切り換えがうまく行かない) */
		//	x2 = 640;
		//	y2 = 480;
		//	sgg_wm0_gapicmd_001c_0020(); // 画面モード設定(640x480)
			sgg_execcmd0(0x0050, 7 * 4, 0x001c, 0, 0x0020, 0x0012, 0x0000, 0x0000);
			sgg_wm0_gapicmd_001c_0004(); // ハードウェア初期化
			init_screen(x2, y2);
			job_general1();
			return;
		}

#if 0
		int mode = job_int0;
		switch (mode) {
		case 0x0012:
			x2 = 640;
			y2 = 480;
			break;

		case 0x0102:
			x2 = 800;
			y2 = 600;

		}
#endif
		jobfunc = &job_setvgamode3;
		sgg_wm0_setvideomode(job_int0 /* mode */, 0x0014);
		return;
	#endif
}

#if (defined(PCAT))

void job_setvgamode3(const int sig, const int result)
{
	// 0x0014しかこない
	if (result == 0) {
		sgg_execcmd0(0x0050, 7 * 4, 0x001c, 0, 0x0020, job_int0 | 0x01000000, 0x0000, 0x0000);
		sgg_wm0_gapicmd_001c_0004(); // ハードウェア初期化
		init_screen(x2, y2);
		job_general1();
		return;
	}

	// VESAのノンサポートなどにより、画面モード切り換え失敗
//	x2 = 640;
//	y2 = 480;
//	jobfunc = &job_setvgamode3;
	sgg_wm0_setvideomode(0x0012 /* VGA */, 0x0014);
	return;
}

#endif

int job_fonttss, job_sig, job_fontflag = 0;

void job_loadfont0(int fonttype, int tss, int sig)
{
	job_sig = sig;
	job_fonttss = tss;
	if (job_fontflag & 0x01) {
		job_loadfont1(0, 0);
		return;
	}
	/* ロードコード */
	lib_initmodulehandle0(0x000c, 0x0200); /* machine-dirに初期化 */
	jobfunc = &job_loadfont1;
	lib_steppath0(0, 0x0200, "JPN16V00.FNT", 0x0050 /* sig */);
	return;
}

void job_loadfont1(int flag, int dmy)
{
	if ((job_fontflag & 0x01) == 0 && flag == 0 /* ロード成功 */) {
		char *fp = (char *) lib_readCSd(0x0010 /* malloc領域の終わり == mapping領域の始まり */);
		lib_mapmodule(0x0000, 0x0200, 0x5, 256 * 1024, fp, 0);

		/* ここでslot:0x0210にgapidataを配備 */
		/* ロードが終わってから配備するのが重要で、そうでないとgapidataの拡張が終わってないかもしれないから */
		sgg_execcmd0(0x007c, 0, 0x0210, 0x0000);

		lib_mapmodule(0x0000, 0x0210, 0x7 /* R/W */, 304 * 1024, fp + 256 * 1024, (8 + 16) * 1024);
		lib_decodetek0(304 * 1024, (int) fp, 0x000c, (int) fp + 256 * 1024, 0x000c);
		lib_unmapmodule(0, 768 * 1024, fp);
		job_fontflag |= 0x01;
		send_signal3dw(0x4000 /* pokon0 */ | 0x240, 0x7f000002,
			0x008c /* SIGNAL_FREE_FILES */, 0x3000 /* winman0 */);
	}
//	if (job_fonttss)
//		send_signal2dw(job_fonttss | 0x240, 0x7f000001, job_sig);
//	jobnow = 0;
//	jobfunc = NULL;
	job_loadfont2();
	return;
}

void job_loadfont2()
{
	if (job_fontflag & 0x02) {
		job_loadfont3(0, 0);
		return;
	}
	/* ロードコード */
	lib_initmodulehandle0(0x000c, 0x0200); /* machine-dirに初期化 */
	jobfunc = &job_loadfont3;
	lib_steppath0(0, 0x0200, "KOR16V00.FNT", 0x0050 /* sig */);
	return;
}

void job_loadfont3(int flag, int dmy)
{
	if ((job_fontflag & 0x02) == 0 && flag == 0 /* ロード成功 */) {
		char *fp = (char *) lib_readCSd(0x0010 /* malloc領域の終わり == mapping領域の始まり */);
		lib_mapmodule(0x0000, 0x0200, 0x5, 256 * 1024, fp, 0);

		/* ここでslot:0x0210にgapidataを配備 */
		/* ロードが終わってから配備するのが重要で、そうでないとgapidataの拡張が終わってないかもしれないから */
		sgg_execcmd0(0x007c, 0, 0x0210, 0x0000);

		lib_mapmodule(0x0000, 0x0210, 0x7 /* R/W */, 288 * 1024, fp + 256 * 1024, (8 + 320) * 1024);
		lib_decodetek0(288 * 1024, (int) fp, 0x000c, (int) fp + 256 * 1024, 0x000c);
		lib_unmapmodule(0, 768 * 1024, fp);
		job_fontflag |= 0x02;
		send_signal3dw(0x4000 /* pokon0 */ | 0x240, 0x7f000002,
			0x008c /* SIGNAL_FREE_FILES */, 0x3000 /* winman0 */);
	}
	if (job_fonttss) {
		send_signal2dw(job_fonttss | 0x240, 0x7f000001, job_sig);
		send_signal2dw(job_fonttss | 0x240, 0x000000cc, 0); /* to pioneer0 */
	}
	jobnow = 0;
//	jobfunc = NULL;
	return;
}

#if (defined(DEBUG))

void lib_drawletters_ASCII(const int opt, const int win, const int charset, const int x0, const int y0,
	const int color, const int backcolor, const char *str)
{
	struct COMMAND {
		int cmd; /* 0x0048 */
		int opt /* 必ず0x0001にする */;
		int window, charset /* 必ず0x00c0にする */;
		int x0, y0 /* dot単位 */;
		int color, backcolor;
		int length;
	//	int letters[0];
		int eoc;
    };

	const unsigned char *s;
	int *t;
	int length;
	
	struct COMMAND *command;

	// strの長さを調べる
	for (s = (const unsigned char *) str; *s++; );

	if (length = s - (const unsigned char *) str - 1) {
		command = malloc(sizeof (struct COMMAND) + length * 4);

		command->cmd = 0x0048;
		command->opt = opt;
		command->window = win;
		command->charset = charset;
		command->x0 = x0;
		command->y0 = y0;
		command->color = color;
		command->backcolor = backcolor;
		command->length = length;

		s = (const unsigned char *) str;
		t = &command->eoc;
		while (*t++ = *s++);

		lib_execcmd(command);
		free(command);
	}
	return;
}

void debug_bin2hex(unsigned int i, unsigned char *s)
{
	int j;
	unsigned char c;
	s += 7;
	for (j = 0; j < 8; j++) {
		c = (i & 0x0f) | '0';
		i >>= 4;
		if (c > '9')
			c += 'A' - '0' - 10;
		*s = c;
		s--;
	}
	return;
}

#endif

static int defsig_signal[4] /* , defsig_opt */; /* サイズ縮小のため */

void sgg_wm0_definesignal3(const int opt, const int device, int keycode,
	const int sig0, const int sig1, const int sig2)
{
	int i = opt & 0x0fff;
	int phase = (keycode >> 14) & 0x3;
//	defsig_opt = opt;
	defsig_signal[0] = sig0 | 0x02;
	defsig_signal[1] = sig1;
	defsig_signal[2] = sig2;
	do {
		sgg_wm0_definesignal3sub(keycode);
		if (opt & 0x00008000)
			defsig_signal[2] += 3;
		defsig_signal[2]++;
		if (opt & 0x00ff0000) {
			do {
				phase++;
				if ((phase &= 0x3) == 0) {
					keycode &= 0xffff3fff;
					keycode++;
				} else
					keycode += 0x00004000;
			} while (((opt >> phase) & 0x00010000) == 0);
		} else
			keycode++;
	} while (i--);
	return;
/*

・keysignal定義について
  opt	bit 0-11 : 連続定義シグナル数
		bit15	 : interval(+1か+4か)
		bit16-19 : make/break/remake/overbreak
				   これは、拡張モードへ移行するために必要
				   すべてを0にすると、make|remakeというふうに扱われる
				   もちろん、インクリメントの方針としても利用される
				   スタートは、keycode内のbit30-31に書かれている
*/

}

void sgg_wm0_definesignal3sub(const int keycode)
/* 連続指定をサポートしない */
/* デコードのみ(make/break/remakeの判定はしない) */
{
	#define	NOSHIFT		0	/* 0x0000c070 */
	#define	SHIFT		1	/* 0x0010c070 */
	#define	IGSHIFT		2	/* 0x0000c060 */
	#define	CAPLKON		3	/* 0x0004c074, 0x0010c074 */
	#define	CAPLKOF		4	/* 0x0000c074, 0x0014c074 */
	#define	NUMLKON		5	/* 0x0002c072, 0x0010c072 */
	#define	NUMLKOF		6	/* 0x0000c072, 0x0012c072 */
	#define ALT			7   /* 0x0040c070 */
	static struct {
		int type0, type1;
	} shifttype[] = {
		{ 0x0000c070, 0 },
		{ 0x0010c070, 0 },
		{ 0x0000c060, 0 },
		{ 0x0004c074, 0x0010c074 },
		{ 0x0000c074, 0x0014c074 },
		{ 0x0002c072, 0x0010c072 },
		{ 0x0000c072, 0x0012c072 },
		{ 0x0040c070, 0 }
	};
	/* 入力方法テーブル(2通りまでサポート) */
	static struct KEYTABLE {
		unsigned char rawcode0, shifttype0;
		unsigned char rawcode1, shifttype1;
	} table[] = {
		#if (defined(PCAT))
			{ 0x39, NOSHIFT, 0xff, 0xff    } /* ' ' */,
			{ 0x02, SHIFT,   0xff, 0xff    } /* '!' */,
			{ 0x03, SHIFT,   0xff, 0xff    } /* '\x22' */,
			{ 0x04, SHIFT,   0xff, 0xff    } /* '#' */,
			{ 0x05, SHIFT,   0xff, 0xff    } /* '%' */,
			{ 0x06, SHIFT,   0xff, 0xff    } /* '$' */,
			{ 0x07, SHIFT,   0xff, 0xff    } /* '&' */,
			{ 0x08, SHIFT,   0xff, 0xff    } /* '\x27' */,
			{ 0x09, SHIFT,   0xff, 0xff    } /* '(' */,
			{ 0x0a, SHIFT,   0xff, 0xff    } /* ')' */,
			{ 0x28, SHIFT,   0x37, IGSHIFT } /* '*' */,
			{ 0x27, SHIFT,   0x4e, IGSHIFT } /* '+' */,
			{ 0x33, NOSHIFT, 0xff, 0xff    } /* ',' */,
			{ 0x0c, NOSHIFT, 0x4a, IGSHIFT } /* '-' */,
			{ 0x34, NOSHIFT, 0x53, NUMLKON } /* '.' */,
			{ 0x35, NOSHIFT, 0xb5, IGSHIFT } /* '/' */,
			{ 0x0b, NOSHIFT, 0x52, NUMLKON } /* '0' */,
			{ 0x02, NOSHIFT, 0x4f, NUMLKON } /* '1' */,
			{ 0x03, NOSHIFT, 0x50, NUMLKON } /* '2' */,
			{ 0x04, NOSHIFT, 0x51, NUMLKON } /* '3' */,
			{ 0x05, NOSHIFT, 0x4b, NUMLKON } /* '4' */,
			{ 0x06, NOSHIFT, 0x4c, NUMLKON } /* '5' */,
			{ 0x07, NOSHIFT, 0x4d, NUMLKON } /* '6' */,
			{ 0x08, NOSHIFT, 0x47, NUMLKON } /* '7' */,
			{ 0x09, NOSHIFT, 0x48, NUMLKON } /* '8' */,
			{ 0x0a, NOSHIFT, 0x49, NUMLKON } /* '9' */,
			{ 0x28, NOSHIFT, 0xff, 0xff    } /* ':' */,
			{ 0x27, NOSHIFT, 0xff, 0xff    } /* ';' */,
			{ 0x33, SHIFT,   0xff, 0xff    } /* '<' */,
			{ 0x0c, SHIFT,   0xff, 0xff    } /* '=' */,
			{ 0x34, SHIFT,   0xff, 0xff    } /* '>' */,
			{ 0x35, SHIFT,   0xff, 0xff    } /* '?' */,
			{ 0x1a, NOSHIFT, 0xff, 0xff    } /* '@' */,
			{ 0x1e, CAPLKON, 0xff, 0xff    } /* 'A' */,
			{ 0x30, CAPLKON, 0xff, 0xff    } /* 'B' */,
			{ 0x2e, CAPLKON, 0xff, 0xff    } /* 'C' */,
			{ 0x20, CAPLKON, 0xff, 0xff    } /* 'D' */,
			{ 0x12, CAPLKON, 0xff, 0xff    } /* 'E' */,
			{ 0x21, CAPLKON, 0xff, 0xff    } /* 'F' */,
			{ 0x22, CAPLKON, 0xff, 0xff    } /* 'G' */,
			{ 0x23, CAPLKON, 0xff, 0xff    } /* 'H' */,
			{ 0x17, CAPLKON, 0xff, 0xff    } /* 'I' */,
			{ 0x24, CAPLKON, 0xff, 0xff    } /* 'J' */,
			{ 0x25, CAPLKON, 0xff, 0xff    } /* 'K' */,
			{ 0x26, CAPLKON, 0xff, 0xff    } /* 'L' */,
			{ 0x32, CAPLKON, 0xff, 0xff    } /* 'M' */,
			{ 0x31, CAPLKON, 0xff, 0xff    } /* 'N' */,
			{ 0x18, CAPLKON, 0xff, 0xff    } /* 'O' */,
			{ 0x19, CAPLKON, 0xff, 0xff    } /* 'P' */,
			{ 0x10, CAPLKON, 0xff, 0xff    } /* 'Q' */,
			{ 0x13, CAPLKON, 0xff, 0xff    } /* 'R' */,
			{ 0x1f, CAPLKON, 0xff, 0xff    } /* 'S' */,
			{ 0x14, CAPLKON, 0xff, 0xff    } /* 'T' */,
			{ 0x16, CAPLKON, 0xff, 0xff    } /* 'U' */,
			{ 0x2f, CAPLKON, 0xff, 0xff    } /* 'V' */,
			{ 0x11, CAPLKON, 0xff, 0xff    } /* 'W' */,
			{ 0x2d, CAPLKON, 0xff, 0xff    } /* 'X' */,
			{ 0x15, CAPLKON, 0xff, 0xff    } /* 'Y' */,
			{ 0x2c, CAPLKON, 0xff, 0xff    } /* 'Z' */,
			{ 0x1b, NOSHIFT, 0xff, 0xff    } /* '[' */,
			{ 0x7d, NOSHIFT, 0x73, NOSHIFT } /* '\' */,
			{ 0x2b, NOSHIFT, 0xff, 0xff    } /* ']' */,
			{ 0x0d, NOSHIFT, 0xff, 0xff    } /* '^' */,
			{ 0x73, SHIFT,   0xff, 0xff    } /* '_' */,
			{ 0x1a, SHIFT,   0xff, 0xff    } /* '`' */,
			{ 0x1e, CAPLKOF, 0xff, 0xff    } /* 'a' */,
			{ 0x30, CAPLKOF, 0xff, 0xff    } /* 'b' */,
			{ 0x2e, CAPLKOF, 0xff, 0xff    } /* 'c' */,
			{ 0x20, CAPLKOF, 0xff, 0xff    } /* 'd' */,
			{ 0x12, CAPLKOF, 0xff, 0xff    } /* 'e' */,
			{ 0x21, CAPLKOF, 0xff, 0xff    } /* 'f' */,
			{ 0x22, CAPLKOF, 0xff, 0xff    } /* 'g' */,
			{ 0x23, CAPLKOF, 0xff, 0xff    } /* 'h' */,
			{ 0x17, CAPLKOF, 0xff, 0xff    } /* 'i' */,
			{ 0x24, CAPLKOF, 0xff, 0xff    } /* 'j' */,
			{ 0x25, CAPLKOF, 0xff, 0xff    } /* 'k' */,
			{ 0x26, CAPLKOF, 0xff, 0xff    } /* 'l' */,
			{ 0x32, CAPLKOF, 0xff, 0xff    } /* 'm' */,
			{ 0x31, CAPLKOF, 0xff, 0xff    } /* 'n' */,
			{ 0x18, CAPLKOF, 0xff, 0xff    } /* 'o' */,
			{ 0x19, CAPLKOF, 0xff, 0xff    } /* 'p' */,
			{ 0x10, CAPLKOF, 0xff, 0xff    } /* 'q' */,
			{ 0x13, CAPLKOF, 0xff, 0xff    } /* 'r' */,
			{ 0x1f, CAPLKOF, 0xff, 0xff    } /* 's' */,
			{ 0x14, CAPLKOF, 0xff, 0xff    } /* 't' */,
			{ 0x16, CAPLKOF, 0xff, 0xff    } /* 'u' */,
			{ 0x2f, CAPLKOF, 0xff, 0xff    } /* 'v' */,
			{ 0x11, CAPLKOF, 0xff, 0xff    } /* 'w' */,
			{ 0x2d, CAPLKOF, 0xff, 0xff    } /* 'x' */,
			{ 0x15, CAPLKOF, 0xff, 0xff    } /* 'y' */,
			{ 0x2c, CAPLKOF, 0xff, 0xff    } /* 'z' */,
			{ 0x1b, SHIFT,   0xff, 0xff    } /* '{' */,
			{ 0x7d, SHIFT,   0xff, 0xff    } /* '|' */,
			{ 0x2b, SHIFT,   0xff, 0xff    } /* '}' */,
			{ 0x0d, SHIFT,   0x0b, SHIFT   } /* '~' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\x7f' */,
			{ 0x01, NOSHIFT, 0xff, 0xff    } /* Esc */,
			{ 0x3b, NOSHIFT, 0xff, 0xff    } /* F1 */,
			{ 0x3c, NOSHIFT, 0xff, 0xff    } /* F2 */,
			{ 0x3d, NOSHIFT, 0xff, 0xff    } /* F3 */,
			{ 0x3e, NOSHIFT, 0xff, 0xff    } /* F4 */,
			{ 0x3f, NOSHIFT, 0xff, 0xff    } /* F5 */,
			{ 0x40, NOSHIFT, 0xff, 0xff    } /* F6 */,
			{ 0x41, NOSHIFT, 0xff, 0xff    } /* F7 */,
			{ 0x42, NOSHIFT, 0xff, 0xff    } /* F8 */,
			{ 0x43, NOSHIFT, 0xff, 0xff    } /* F9 */,
			{ 0x44, NOSHIFT, 0xff, 0xff    } /* F10 */,
			{ 0x57, NOSHIFT, 0xff, 0xff    } /* F11 */,
			{ 0x58, NOSHIFT, 0xff, 0xff    } /* F12 */,
			{ 0xff, 0xff,    0xff, 0xff    } /* F13 */,
			{ 0xff, 0xff,    0xff, 0xff    } /* F14 */,
			{ 0xff, 0xff,    0xff, 0xff    } /* F15 */,
			{ 0xff, 0xff,    0xff, 0xff    } /* F16 */,
			{ 0xff, 0xff,    0xff, 0xff    } /* F17 */,
			{ 0xff, 0xff,    0xff, 0xff    } /* F18 */,
			{ 0xff, 0xff,    0xff, 0xff    } /* F19 */,
			{ 0xff, 0xff,    0xff, 0xff    } /* F20 */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\x95' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\x96' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\x97' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\x98' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\x99' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\x9a' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\x9b' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\x9c' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\x9d' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\x9e' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\x9f' */,
			{ 0x1c, NOSHIFT, 0x9c, NOSHIFT } /* Enter */,
			{ 0x0e, NOSHIFT, 0xff, 0xff    } /* BackSpace */,
			{ 0x0f, NOSHIFT, 0xff, 0xff    } /* Tab */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xa3' */,
			{ 0xd2, NOSHIFT, 0x52, NUMLKOF } /* Insert */,
			{ 0xd3, NOSHIFT, 0x53, NUMLKOF } /* Delete */,
			{ 0xc7, NOSHIFT, 0x47, NUMLKOF } /* Home */,
			{ 0xcf, NOSHIFT, 0x4f, NUMLKOF } /* End */,
			{ 0xc9, NOSHIFT, 0x49, NUMLKOF } /* PageUp */,
			{ 0xd1, NOSHIFT, 0x51, NUMLKOF } /* PageDown */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xaa' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xab' */,
			{ 0xcb, NOSHIFT, 0x4b, NUMLKOF } /* Left */,
			{ 0xcd, NOSHIFT, 0x4d, NUMLKOF } /* Right */,
			{ 0xc8, NOSHIFT, 0x48, NUMLKOF } /* Up */,
			{ 0xd0, NOSHIFT, 0x50, NUMLKOF } /* Down */,
			{ 0x46, NOSHIFT, 0xff, 0xff    } /* ScrollLock */,
			{ 0x45, NOSHIFT, 0xff, 0xff    } /* NumLock */,
			{ 0x3a, SHIFT,   0xff, 0xff    } /* CapsLock */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xb3' */,
			{ 0x2a, 0xfe,    0x36, 0xfe    } /* Shift */,
			{ 0x1d, 0xfe,    0x9d, 0xfe    } /* Ctrl */,
			{ 0x38, 0xfe,    0xb8, 0xfe    } /* Alt */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xb7' */,
			{ 0xb7, NOSHIFT, 0xff, 0xff    } /* PrintScreen */,
			{ 0xff, NOSHIFT, 0xff, 0xff    } /* Pause */,
			{ 0xc6, NOSHIFT, 0xff, 0xff    } /* Break(ALT?) */,
			{ 0x54, NOSHIFT, 0xff, 0xff    } /* SysRq(ALT?) */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xbc' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xbd' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xbe' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xbf' */,
			{ 0xdb, NOSHIFT, 0xdc, NOSHIFT } /* Windows */,
			{ 0xdd, NOSHIFT, 0xff, 0xff    } /* Menu */,
			{ 0xff, 0xff,    0xff, 0xff    } /* Power */,
			{ 0xff, 0xff,    0xff, 0xff    } /* Sleep */,
			{ 0xff, 0xff,    0xff, 0xff    } /* Wake */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xc5' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xc6' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xc7' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xc8' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xc9' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xca' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xcb' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xcc' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xcd' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xce' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xcf' */,
			{ 0x29, NOSHIFT, 0xff, 0xff    } /* Zenkaku */,
			{ 0x7b, NOSHIFT, 0xff, 0xff    } /* Muhenkan */,
			{ 0x79, NOSHIFT, 0xff, 0xff    } /* Henkan */,
			{ 0x70, NOSHIFT, 0xff, 0xff    } /* Hiragana */,
			{ 0x70, SHIFT,   0xff, 0xff    } /* Katakana */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xd5' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xd6' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xd7' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xd8' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xd9' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xda' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xdb' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xdc' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xdd' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xde' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xdf' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xe0' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xe1' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xe2' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xe3' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xe4' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xe5' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xe6' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xe7' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xe8' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xe9' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xea' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xeb' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xec' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xed' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xee' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xef' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xf0' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xf1' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xf2' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xf3' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xf4' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xf5' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xf6' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xf7' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xf8' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xf9' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xfa' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xfb' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xfc' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xfd' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xfe' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xff' */
		#endif
		#if (defined(TOWNS))
			{ 0x35, NOSHIFT, 0xff, 0xff    } /* ' ' */,
			{ 0x02, SHIFT,   0xff, 0xff    } /* '!' */,
			{ 0x03, SHIFT,   0x34, NOSHIFT } /* '\x22' */,
			{ 0x04, SHIFT,   0xff, 0xff    } /* '#' */,
			{ 0x05, SHIFT,   0xff, 0xff    } /* '%' */,
			{ 0x06, SHIFT,   0xff, 0xff    } /* '$' */,
			{ 0x07, SHIFT,   0xff, 0xff    } /* '&' */,
			{ 0x08, SHIFT,   0xff, 0xff    } /* '\x27' */,
			{ 0x09, SHIFT,   0xff, 0xff    } /* '(' */,
			{ 0x0a, SHIFT,   0xff, 0xff    } /* ')' */,
			{ 0x28, SHIFT,   0x36, NOSHIFT } /* '*' */,
			{ 0x27, SHIFT,   0x38, NOSHIFT } /* '+' */,
			{ 0x31, NOSHIFT, 0xff, 0xff    } /* ',' */,
			{ 0x0c, NOSHIFT, 0x39, NOSHIFT } /* '-' */,
			{ 0x32, NOSHIFT, 0x47, NOSHIFT } /* '.' */,
			{ 0x33, NOSHIFT, 0x37, NOSHIFT } /* '/' */,
			{ 0x0b, NOSHIFT, 0x46, NOSHIFT } /* '0' */,
			{ 0x02, NOSHIFT, 0x42, NOSHIFT } /* '1' */,
			{ 0x03, NOSHIFT, 0x43, NOSHIFT } /* '2' */,
			{ 0x04, NOSHIFT, 0x44, NOSHIFT } /* '3' */,
			{ 0x05, NOSHIFT, 0x3e, NOSHIFT } /* '4' */,
			{ 0x06, NOSHIFT, 0x3f, NOSHIFT } /* '5' */,
			{ 0x07, NOSHIFT, 0x40, NOSHIFT } /* '6' */,
			{ 0x08, NOSHIFT, 0x3a, NOSHIFT } /* '7' */,
			{ 0x09, NOSHIFT, 0x3b, NOSHIFT } /* '8' */,
			{ 0x0a, NOSHIFT, 0x3c, NOSHIFT } /* '9' */,
			{ 0x28, NOSHIFT, 0xff, 0xff    } /* ':' */,
			{ 0x27, NOSHIFT, 0xff, 0xff    } /* ';' */,
			{ 0x31, SHIFT,   0xff, 0xff    } /* '<' */,
			{ 0x0c, SHIFT,   0x3d, NOSHIFT } /* '=' */,
			{ 0x32, SHIFT,   0xff, 0xff    } /* '>' */,
			{ 0x33, SHIFT,   0xff, 0xff    } /* '?' */,
			{ 0x1b, NOSHIFT, 0xff, 0xff    } /* '@' */,
			{ 0x1e, CAPLKON, 0xff, 0xff    } /* 'A' */,
			{ 0x2e, CAPLKON, 0xff, 0xff    } /* 'B' */,
			{ 0x2c, CAPLKON, 0xff, 0xff    } /* 'C' */,
			{ 0x20, CAPLKON, 0xff, 0xff    } /* 'D' */,
			{ 0x13, CAPLKON, 0xff, 0xff    } /* 'E' */,
			{ 0x21, CAPLKON, 0xff, 0xff    } /* 'F' */,
			{ 0x22, CAPLKON, 0xff, 0xff    } /* 'G' */,
			{ 0x23, CAPLKON, 0xff, 0xff    } /* 'H' */,
			{ 0x18, CAPLKON, 0xff, 0xff    } /* 'I' */,
			{ 0x24, CAPLKON, 0xff, 0xff    } /* 'J' */,
			{ 0x25, CAPLKON, 0xff, 0xff    } /* 'K' */,
			{ 0x26, CAPLKON, 0xff, 0xff    } /* 'L' */,
			{ 0x30, CAPLKON, 0xff, 0xff    } /* 'M' */,
			{ 0x2f, CAPLKON, 0xff, 0xff    } /* 'N' */,
			{ 0x19, CAPLKON, 0xff, 0xff    } /* 'O' */,
			{ 0x1a, CAPLKON, 0xff, 0xff    } /* 'P' */,
			{ 0x11, CAPLKON, 0xff, 0xff    } /* 'Q' */,
			{ 0x14, CAPLKON, 0xff, 0xff    } /* 'R' */,
			{ 0x1f, CAPLKON, 0xff, 0xff    } /* 'S' */,
			{ 0x15, CAPLKON, 0xff, 0xff    } /* 'T' */,
			{ 0x17, CAPLKON, 0xff, 0xff    } /* 'U' */,
			{ 0x2d, CAPLKON, 0xff, 0xff    } /* 'V' */,
			{ 0x12, CAPLKON, 0xff, 0xff    } /* 'W' */,
			{ 0x2b, CAPLKON, 0xff, 0xff    } /* 'X' */,
			{ 0x16, CAPLKON, 0xff, 0xff    } /* 'Y' */,
			{ 0x2a, CAPLKON, 0xff, 0xff    } /* 'Z' */,
			{ 0x1c, NOSHIFT, 0xff, 0xff    } /* '[' */,
			{ 0x0e, NOSHIFT, 0xff, 0xff    } /* '\' */,
			{ 0x29, NOSHIFT, 0xff, 0xff    } /* ']' */,
			{ 0x0d, NOSHIFT, 0xff, 0xff    } /* '^' */,
			{ 0x34, SHIFT,   0xff, 0xff    } /* '_' */,
			{ 0x1b, SHIFT,   0xff, 0xff    } /* '`' */,
			{ 0x1e, CAPLKOF, 0xff, 0xff    } /* 'a' */,
			{ 0x2e, CAPLKOF, 0xff, 0xff    } /* 'b' */,
			{ 0x2c, CAPLKOF, 0xff, 0xff    } /* 'c' */,
			{ 0x20, CAPLKOF, 0xff, 0xff    } /* 'd' */,
			{ 0x13, CAPLKOF, 0xff, 0xff    } /* 'e' */,
			{ 0x21, CAPLKOF, 0xff, 0xff    } /* 'f' */,
			{ 0x22, CAPLKOF, 0xff, 0xff    } /* 'g' */,
			{ 0x23, CAPLKOF, 0xff, 0xff    } /* 'h' */,
			{ 0x18, CAPLKOF, 0xff, 0xff    } /* 'i' */,
			{ 0x24, CAPLKOF, 0xff, 0xff    } /* 'j' */,
			{ 0x25, CAPLKOF, 0xff, 0xff    } /* 'k' */,
			{ 0x26, CAPLKOF, 0xff, 0xff    } /* 'l' */,
			{ 0x30, CAPLKOF, 0xff, 0xff    } /* 'm' */,
			{ 0x2f, CAPLKOF, 0xff, 0xff    } /* 'n' */,
			{ 0x19, CAPLKOF, 0xff, 0xff    } /* 'o' */,
			{ 0x1a, CAPLKOF, 0xff, 0xff    } /* 'p' */,
			{ 0x11, CAPLKOF, 0xff, 0xff    } /* 'q' */,
			{ 0x14, CAPLKOF, 0xff, 0xff    } /* 'r' */,
			{ 0x1f, CAPLKOF, 0xff, 0xff    } /* 's' */,
			{ 0x15, CAPLKOF, 0xff, 0xff    } /* 't' */,
			{ 0x17, CAPLKOF, 0xff, 0xff    } /* 'u' */,
			{ 0x2d, CAPLKOF, 0xff, 0xff    } /* 'v' */,
			{ 0x12, CAPLKOF, 0xff, 0xff    } /* 'w' */,
			{ 0x2b, CAPLKOF, 0xff, 0xff    } /* 'x' */,
			{ 0x16, CAPLKOF, 0xff, 0xff    } /* 'y' */,
			{ 0x2a, CAPLKOF, 0xff, 0xff    } /* 'z' */,
			{ 0x1c, SHIFT,   0xff, 0xff    } /* '{' */,
			{ 0x0e, SHIFT,   0xff, 0xff    } /* '|' */,
			{ 0x29, SHIFT,   0xff, 0xff    } /* '}' */,
			{ 0x0d, SHIFT,   0xff, 0xff    } /* '~' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\x7f' */,
			{ 0x01, NOSHIFT, 0xff, 0xff    } /* Esc */,
			{ 0x5d, NOSHIFT, 0xff, 0xff    } /* F1 */,
			{ 0x5e, NOSHIFT, 0xff, 0xff    } /* F2 */,
			{ 0x5f, NOSHIFT, 0xff, 0xff    } /* F3 */,
			{ 0x60, NOSHIFT, 0xff, 0xff    } /* F4 */,
			{ 0x61, NOSHIFT, 0xff, 0xff    } /* F5 */,
			{ 0x62, NOSHIFT, 0xff, 0xff    } /* F6 */,
			{ 0x63, NOSHIFT, 0xff, 0xff    } /* F7 */,
			{ 0x64, NOSHIFT, 0xff, 0xff    } /* F8 */,
			{ 0x65, NOSHIFT, 0xff, 0xff    } /* F9 */,
			{ 0x66, NOSHIFT, 0xff, 0xff    } /* F10 */,
			{ 0x69, NOSHIFT, 0xff, 0xff    } /* F11 */,
			{ 0x5b, NOSHIFT, 0xff, 0xff    } /* F12 */,
			{ 0x74, NOSHIFT, 0xff, 0xff    } /* F13 */,
			{ 0x75, NOSHIFT, 0xff, 0xff    } /* F14 */,
			{ 0x76, NOSHIFT, 0xff, 0xff    } /* F15 */,
			{ 0x77, NOSHIFT, 0xff, 0xff    } /* F16 */,
			{ 0x78, NOSHIFT, 0xff, 0xff    } /* F17 */,
			{ 0x79, NOSHIFT, 0xff, 0xff    } /* F18 */,
			{ 0x7a, NOSHIFT, 0xff, 0xff    } /* F19 */,
			{ 0x7b, NOSHIFT, 0xff, 0xff    } /* F20 */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\x95' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\x96' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\x97' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\x98' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\x99' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\x9a' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\x9b' */,
			{ 0xf4, NOSHIFT, 0xff, 0xff    } /* EXT1(F28) */,
			{ 0xf8, NOSHIFT, 0xff, 0xff    } /* EXT2(F29) */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\x9e' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\x9f' */,
			{ 0x1d, NOSHIFT, 0x45, NOSHIFT } /* Enter */,
			{ 0x0f, NOSHIFT, 0xff, 0xff    } /* BackSpace */,
			{ 0x10, NOSHIFT, 0xff, 0xff    } /* Tab */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xa3' */,
			{ 0x48, NOSHIFT, 0xff, 0xff    } /* Insert */,
			{ 0x4b, NOSHIFT, 0xff, 0xff    } /* Delete */,
			{ 0x4e, NOSHIFT, 0xe1, NOSHIFT } /* Home */,
			{ 0xe2, NOSHIFT, 0xff, 0xff    } /* End */,
			{ 0x6e, NOSHIFT, 0xff, 0xff    } /* PageUp */,
			{ 0x70, NOSHIFT, 0xff, 0xff    } /* PageDown */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xaa' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xab' */,
			{ 0x4f, NOSHIFT, 0xff, 0xff    } /* Left */,
			{ 0x51, NOSHIFT, 0xff, 0xff    } /* Right */,
			{ 0x4d, NOSHIFT, 0xff, 0xff    } /* Up */,
			{ 0x50, NOSHIFT, 0xff, 0xff    } /* Down */,
			{ 0xe0, NOSHIFT, 0xff, 0xff    } /* ScrollLock */,
			{ 0xff, 0xff,    0xff, 0xff    } /* NumLock */,
			{ 0x55, NOSHIFT, 0xff, 0xff    } /* CapsLock */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xb3' */,
			{ 0x53, 0xfe,    0xff, 0xff    } /* Shift */,
			{ 0x52, 0xfe,    0xff, 0xff    } /* Ctrl */,
			{ 0x5c, 0xfe,    0x72, 0xfe    } /* Alt */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xb7' */,
			{ 0x7d, NOSHIFT, 0xff, 0xff    } /* COPY(PrintScreen) */,
			{ 0xff, 0xff,    0xff, 0xff    } /* Pause */,
			{ 0x7c, NOSHIFT, 0xff, 0xff    } /* Break */,
			{ 0xdd, NOSHIFT, 0xff, 0xff    } /* SysRq */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xbc' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xbd' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xbe' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xbf' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* Windows */,
			{ 0xff, 0xff,    0xff, 0xff    } /* Menu */,
			{ 0xff, 0xff,    0xff, 0xff    } /* Power */,
			{ 0xff, 0xff,    0xff, 0xff    } /* Sleep */,
			{ 0xff, 0xff,    0xff, 0xff    } /* Wake */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xc5' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xc6' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xc7' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xc8' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xc9' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xca' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xcb' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xcc' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xcd' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xce' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xcf' */,
			{ 0x71, NOSHIFT, 0xff, 0xff    } /* Zenkaku */,
			{ 0x57, NOSHIFT, 0xff, 0xff    } /* Muhenkan */,
			{ 0x58, NOSHIFT, 0xff, 0xff    } /* Henkan */,
			{ 0x56, NOSHIFT, 0xff, 0xff    } /* Hiragana */,
			{ 0x5a, NOSHIFT, 0xff, 0xff    } /* Katakana */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xd5' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xd6' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xd7' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xd8' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xd9' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xda' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xdb' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xdc' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xdd' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xde' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xdf' */,
/* e0～f7は、割り当てるのが面倒になった機種固有のマイナーキー */
			{ 0x72, NOSHIFT, 0xff, 0xff    } /* 取消 */,
			{ 0x73, NOSHIFT, 0xff, 0xff    } /* 実行 */,
			{ 0x59, NOSHIFT, 0xff, 0xff    } /* かな漢字 */,
			{ 0x4a, NOSHIFT, 0xff, 0xff    } /* 000 */,
			{ 0x6b, NOSHIFT, 0xff, 0xff    } /* 漢字辞書 */,
			{ 0x6c, NOSHIFT, 0xff, 0xff    } /* 単語抹消 */,
			{ 0x6d, NOSHIFT, 0xff, 0xff    } /* 単語登録 */,
			{ 0x6a, NOSHIFT, 0xff, 0xff    } /* 英字 */,
			{ 0x6f, NOSHIFT, 0xff, 0xff    } /* カタカナ/英小文字 */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xe9' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xea' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xeb' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xec' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xed' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xee' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xef' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xf0' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xf1' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xf2' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xf3' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xf4' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xf5' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xf6' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xf7' */,
/* f8～ffは、マイナーシフトキー */
			{ 0x67, NOSHIFT, 0x68, NOSHIFT } /* 親指シフト('\xf8') */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xf9' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xfa' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xfb' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xfc' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xfd' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xfe' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xff' */
		#endif
	};

	if ((keycode & 0xfffff000) == 0) {
		/* 従来通りの指定 */
		if (' ' <= keycode && keycode <= 0xff) {
			struct KEYTABLE *kt = &table[keycode - ' '];
			int rawcode, st, shiftmap;
			if ((st = kt->shifttype0) < 0xf0) {
				rawcode = kt->rawcode0;
				shiftmap = shifttype[st].type0;
				sgg_wm0_definesignal3sub3(rawcode, shiftmap);
				sgg_wm0_definesignal3sub3(rawcode, shiftmap | 0x80000000);
				if (shiftmap = shifttype[st].type1) {
					sgg_wm0_definesignal3sub3(rawcode, shiftmap);
					sgg_wm0_definesignal3sub3(rawcode, shiftmap | 0x80000000);
				}
				if ((st = kt->shifttype1) < 0xf0) {
					rawcode = kt->rawcode1;
					shiftmap = shifttype[st].type0;
					sgg_wm0_definesignal3sub3(rawcode, shiftmap);
					sgg_wm0_definesignal3sub3(rawcode, shiftmap | 0x80000000);
					if (shiftmap = shifttype[st].type1) {
						sgg_wm0_definesignal3sub3(rawcode, shiftmap);
						sgg_wm0_definesignal3sub3(rawcode, shiftmap | 0x80000000);
					}
				}
			}
		}
	} else {
		int shiftmap = ((keycode >> 16) & 0x000000ff) | ((keycode >> 8) & 0x00ff0000)
					  | ((keycode << 16) & 0xc0000000) | 0x0000c000;
		int rawcode = keycode & 0x00003fff;
		if (' ' + 0x1000 <= rawcode && rawcode <= 0x10ff) {
			struct KEYTABLE *kt = &table[rawcode - (' ' + 0x1000)];
			if (kt->shifttype0 != 0xff)
				sgg_wm0_definesignal3sub3(kt->rawcode0, shiftmap);
		} else if (' ' + 0x2000 <= rawcode && rawcode <= 0x20ff) {
			struct KEYTABLE *kt = &table[rawcode - (' ' + 0x2000)];
			if (kt->shifttype1 != 0xff)
				sgg_wm0_definesignal3sub3(kt->rawcode1, shiftmap);
		}
	}
	return;
}

/*
void sgg_wm0_definesignal3sub2(const int rawcode, const int shiftmap)
{
	if ((defsig_opt & 0x00ff0000) == 0)
		sgg_wm0_definesignal3sub3(rawcode, shiftmap | 0x80000000);
	sgg_wm0_definesignal3sub3(rawcode, shiftmap);
	return;
}
*/

void sgg_wm0_definesignal3sub3(int rawcode, const int shiftmap)
{
	if (rawcode == 0xff)
		rawcode = 0x100;
	sgg_execcmd0(0x0068, 12 * 4,
			0x010c   /* define */,
			7        /* opt(len) */,
			rawcode  /* rawcode */,
			shiftmap /* shiftmap */,
			tapisigvec[3] /* vector(ofs) */,
			0x03030000 | tapisigvec[4] /* vector(sel), cmd, len */,
			defsig_signal[0] /* signal[0] */,
			defsig_signal[1] /* signal[1] */,
			defsig_signal[2] /* signal[2] */,
			0x0000 /* EOC */,
		0);

#if 0
	/* cmd, opt(len), rawコード, shift-lock-bitmap(mask, equal), subcmd,... */
	static struct {
		int cmd, length;
		int deccmd[10];
		int eoc;
	} command = {
		0x0068, 12 * 4, {
			0x010c /* define */,
			7      /* opt(len) */,
			0      /* rawcode */,
			0      /* shiftmap */,
			0      /* vector(ofs) */,
			0      /* vector(sel), cmd, len */,
			0      /* signal[0] */,
			0      /* signal[1] */,
			0      /* signal[2] */,
			0x0000 /* EOC */
		}, 0 /* EOC */
	};
	command.deccmd[2] = rawcode;
	command.deccmd[3] = shiftmap;
	command.deccmd[4] = tapisigvec[3] /* ofs */;
	command.deccmd[5] = 0x03030000 | tapisigvec[4] /* sel */;
	command.deccmd[6] = defsig_signal[0];
	command.deccmd[7] = defsig_signal[1];
	command.deccmd[8] = defsig_signal[2];
	sgg_execcmd(&command);
#endif
	return;
}

void sgg_wm0_definesignal3com()
{
	#if (defined(PCAT))
		static struct {
			int cmd, length;
			int deccmd[6 * 16 + 3];
			int eoc;
		} command = {
			0x0068, 101 * 4, {
				0x0110     /* clear */,
				0          /* opt */,

				0x010c     /* define */,
				4          /* opt(len) */,
				0x46       /* rawcode(ScrollLock) */,
				0x0000c070 /* shiftmap */,
				0x0001     /* xor bit */,
				0x00060000 /* cmd(xor) */,

				0x010c     /* define */,
				4          /* opt(len) */,
				0x45       /* rawcode(NumLock) */,
				0x0000c070 /* shiftmap */,
				0x0002     /* xor bit */,
				0x00060000 /* cmd(xor) */,

				0x010c     /* define */,
				4          /* opt(len) */,
				0x3a       /* rawcode(CapsLock) */,
				0x0010c070 /* shiftmap */,
				0x0004     /* xor bit */,
				0x00060000 /* cmd(xor) */,

				0x010c     /* define */,
				4          /* opt(len) */,
				0x2a       /* rawcode(left-Shift) */,
				0x0000c000 /* shiftmap */,
				0x0010     /* or bit */,
				0x00040000 /* cmd(or) */,

				0x010c     /* define */,
				4          /* opt(len) */,
				0x2a       /* rawcode(left-Shift) */,
				0x4000c000 /* shiftmap */,
				~0x0010    /* and bit */,
				0x00050000 /* cmd(and) */,

				0x010c     /* define */,
				4          /* opt(len) */,
				0x36       /* rawcode(right-Shift) */,
				0x0000c000 /* shiftmap */,
				0x0010     /* or bit */,
				0x00040000 /* cmd(or) */,

				0x010c     /* define */,
				4          /* opt(len) */,
				0x36       /* rawcode(right-Shift) */,
				0x4000c000 /* shiftmap */,
				~0x0010    /* and bit */,
				0x00050000 /* cmd(and) */,

				0x010c     /* define */,
				4          /* opt(len) */,
				0x1d       /* rawcode(left-Ctrl) */,
				0x0000c000 /* shiftmap */,
				0x0020     /* or bit */,
				0x00040000 /* cmd(or) */,

				0x010c     /* define */,
				4          /* opt(len) */,
				0x1d       /* rawcode(left-Ctrl) */,
				0x4000c000 /* shiftmap */,
				~0x0020    /* and bit */,
				0x00050000 /* cmd(and) */,

				0x010c     /* define */,
				4          /* opt(len) */,
				0x9d       /* rawcode(right-Ctrl) */,
				0x0000c000 /* shiftmap */,
				0x0020     /* or bit */,
				0x00040000 /* cmd(or) */,

				0x010c     /* define */,
				4          /* opt(len) */,
				0x9d       /* rawcode(right-Ctrl) */,
				0x4000c000 /* shiftmap */,
				~0x0020    /* and bit */,
				0x00050000 /* cmd(and) */,

				0x010c     /* define */,
				4          /* opt(len) */,
				0x38       /* rawcode(left-Alt) */,
				0x0000c000 /* shiftmap */,
				0x0040     /* or bit */,
				0x00040000 /* cmd(or) */,

				0x010c     /* define */,
				4          /* opt(len) */,
				0x38       /* rawcode(left-Alt) */,
				0x4000c000 /* shiftmap */,
				~0x0040    /* and bit */,
				0x00050000 /* cmd(and) */,

				0x010c     /* define */,
				4          /* opt(len) */,
				0xb8       /* rawcode(right-Alt) */,
				0x0000c000 /* shiftmap */,
				0x0040     /* or bit */,
				0x00040000 /* cmd(or) */,

				0x010c     /* define */,
				4          /* opt(len) */,
				0xb8       /* rawcode(right-Alt) */,
				0x4000c000 /* shiftmap */,
				~0x0040    /* and bit */,
				0x00050000 /* cmd(and) */,

				0x010c     /* define */,
				4          /* opt(len) */,
				0xd3       /* rawcode(Deletele) */,
				0x0060c070 /* shiftmap */,
				0          /* reserve */,
				0x00380000 /* cmd(reset) */,

				0x0000 /* EOC */
			}, 0x0000 /* EOC */
		};
	#endif
	#if (defined(TOWNS))
		static struct {
			int cmd, length;
			int deccmd[6 * 10 + 3];
			int eoc;
		} command = {
			0x0068, 65 * 4, {
				0x0110     /* clear */,
				0          /* opt */,

				0x010c     /* define */,
				4          /* opt(len) */,
				0x55       /* rawcode(CapsLock) */,
				0x0000c070 /* shiftmap */,
				0x0004     /* xor bit */,
				0x00060000 /* cmd(xor) */,

				0x010c     /* define */,
				4          /* opt(len) */,
				0x53       /* rawcode(Shift) */,
				0x0000c000 /* shiftmap */,
				0x0010     /* or bit */,
				0x00040000 /* cmd(or) */,

				0x010c     /* define */,
				4          /* opt(len) */,
				0x53       /* rawcode(Shift) */,
				0x4000c000 /* shiftmap */,
				~0x0010    /* and bit */,
				0x00050000 /* cmd(and) */,

				0x010c     /* define */,
				4          /* opt(len) */,
				0x52       /* rawcode(Ctrl) */,
				0x0000c000 /* shiftmap */,
				0x0020     /* or bit */,
				0x00040000 /* cmd(or) */,

				0x010c     /* define */,
				4          /* opt(len) */,
				0x52       /* rawcode(Ctrl) */,
				0x4000c000 /* shiftmap */,
				~0x0020    /* and bit */,
				0x00050000 /* cmd(and) */,

				0x010c     /* define */,
				4          /* opt(len) */,
				0x5c       /* rawcode(Alt) */,
				0x0000c000 /* shiftmap */,
				0x0040     /* or bit */,
				0x00040000 /* cmd(or) */,

				0x010c     /* define */,
				4          /* opt(len) */,
				0x5c       /* rawcode(Alt) */,
				0x4000c000 /* shiftmap */,
				~0x0040    /* and bit */,
				0x00050000 /* cmd(and) */,

				0x010c     /* define */,
				4          /* opt(len) */,
				0x72       /* rawcode(取消) */,
				0x0000c000 /* shiftmap */,
				0x0040     /* or bit */,
				0x00040000 /* cmd(or) */,

				0x010c     /* define */,
				4          /* opt(len) */,
				0x72       /* rawcode(取消) */,
				0x4000c000 /* shiftmap */,
				~0x0040    /* and bit */,
				0x00050000 /* cmd(and) */,

				0x010c     /* define */,
				4          /* opt(len) */,
				0x4b       /* rawcode(Deletele) */,
				0x0060c070 /* shiftmap */,
				0          /* reserve */,
				0x00380000 /* cmd(reset) */,

				0x0000 /* EOC */
			}, 0x0000 /* EOC */
		};
	#endif

	sgg_execcmd(&command);
	return;
}

/*
・keysignal定義について
  opt	bit 0-11 : 連続定義シグナル数
		bit15	 : interval(+1か+4か)
		bit16-17 : make/break/remake/overbreak
		bit20-23 : インクリメント時のmake/break/remake/overbreak
				   オール0かつインクリメント、というのは許さない
					bit16-23が0の場合、従来通りの扱いになる

  keycode bit12-14 : 000:キーコード指定
					 001:コードではなくキー指定（第一キー）
					 010:コードではなくキー指定（第二キー）
					 100:コードではなくキー指定（第三キー）
		  bit15    : bit16-31は有効(bit12-14がノンゼロなら有効に決まっているじゃないか)
		  bit16-23 : shiftマスク
		  bit24-31 : shift比較
*/

void gapi_loadankfont()
{
	static struct {
		int cmd, length;
		int gapicmd[15];
		int eoc;
	} command = {
		0x0050, 17 * 4, {
			0x0104 /* loadfont */,
			0x0000 /* opt */,
			0x0001 /* type */,
			0x0100 /* len */,
			0x1000 /* to */,
			0x0000 /* from(ofs) */,
			7 * 8  /* from(sel) */,
			0x0104 /* loadfont */,
			0x0000 /* opt */,
			0x0001 /* type */,
			0x0100 /* len */,
			0x2000 /* to */,
			0x0000 /* from(ofs) */,
			7 * 8  /* from(sel) */,
			0x0000 /* EOC */
		}, 0 /* EOC */
	};
	sgg_execcmd(&command);
	return;
}
