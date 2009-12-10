/* "winman0.c":ぐいぐい仕様ウィンドウマネージャー ver.2.2
		copyright(C) 2002 川合秀実, I.Tak., 小柳雅明, KIYOTO
    stack:4k malloc:620k file:768k */

#define WALLPAPERMAXSIZE	(512 * 1024)

/* プリプロセッサのオプションで、-DPCATか-DTOWNSを指定すること */

#include <guigui00.h>
#include <sysgg00.h>
/* sysggは、一般のアプリが利用してはいけないライブラリ
   仕様もかなり流動的 */
#include <stdlib.h>

//static int MALLOC_ADDR;
#define MALLOC_ADDR			j
#define malloc(bytes)		(void *) (MALLOC_ADDR -= ((bytes) + 7) & ~7)
#define free(addr)			for (;;); /* freeがあっては困るので永久ループ */

#define	AUTO_MALLOC			   0
#define NULL				   0
#define	MAX_WINDOWS		 	  80		// 8.1KB
#define JOBLIST_SIZE		 256		// 1KB
#define	MAX_SOUNDTRACK		  16		// 0.5KB
#define	DEFSIGBUFSIZ		2048

#define	BACKCOLOR			   6

#if (defined(WIN9X))
	#define	RESERVELINE0		   0
	#define	RESERVELINE1		  28
#elif (defined(TMENU))
	#define	RESERVELINE0		  20
	#define	RESERVELINE1		  28
#elif (defined(CHO_OSASK))
	#define	RESERVELINE0		   0
	#define	RESERVELINE1		  20
#elif (defined(NEWSTYLE))
	#define	RESERVELINE0		   0
	#define	RESERVELINE1		   0
#elif (defined(WIN31))
	#define	RESERVELINE0		   0
	#define	RESERVELINE1		   0
#endif

#if (defined(PCAT) || defined(NEC98))
	#define	MOVEUNIT			   8
#endif
#if (defined(TOWNS))
	#define	MOVEUNIT			   4
#endif

#if (defined(TOWNS))
	#if (!defined(TWVSW))
		#define	TWVSW		1024
	#endif
#endif

#define WINFLG_MUSTREDRAW		0x80000000	/* bit31 */
#define WINFLG_OVERRIDEDISABLED	0x01000000	/* bit24 */
#define	WINFLG_WAITREDRAW		0x00000200	/* bit 9 */
#define	WINFLG_WAITDISABLE		0x00000100	/* bit 8 */

//	#define	DEBUG	1

#define	sgg_debug00(opt, bytes, reserve, src_ofs, src_sel, dest_ofs, dest_sel) \
	sgg_execcmd0(0x8010, (int) (opt), (int) (bytes), (int) (reserve), \
	(int) (src_ofs), (int) (src_sel), (int) (dest_ofs), (int) (dest_sel), \
	0x0000)

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

static struct STR_JOB {
	int now, movewin4_ready, fontflag;
	int *list, free, *rp, *wp;
	int count, int0;
	int movewin_x, movewin_y, movewin_x0, movewin_y0;
	int readCSd10;
	void (*func)(int, int);
	struct WM0_WINDOW *win;
	int fonttss, sig;
} job = { 0 /* now */, 0 /* movewin4_ready */, /* fontflag */ };

unsigned char *wallpaper, wallpaper_exist;
struct WM0_WINDOW *window, *top = NULL, *unuse = NULL, *iactive = NULL, *pokon0 = NULL;
int x2 = 0, y2, mx = 0x80000000, my, mbutton = 0;
int fromboot = 0, mousescale = 1;
struct SOUNDTRACK *sndtrk_buf, *sndtrk_active = NULL;
struct DEFINESIGNAL *defsigbuf;

void init_screen(const int x, const int y);
struct WM0_WINDOW *handle2window(const int handle);
void chain_unuse(struct WM0_WINDOW *win);
struct WM0_WINDOW *get_unuse();
void mousesignal(const unsigned int header, int dx, int dy);
int writejob_n(int n, int p0,...);
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

#ifdef TOWNS
void job_savevram0(void);
void job_savevram1(int flag, int dmy);
void job_savevram2(int flag, int dmy);
#endif
void job_openwallpaper(void);
void job_loadwallpaper(int flag, int dmy);
void putwallpaper(int x0, int y0, int x1, int y1);

//void free_sndtrk(struct SOUNDTRACK *sndtrk);

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

void OsaskMain()
{
	int *signal, *signal0, i, j;
	struct WM0_WINDOW *win;
	struct STR_JOB *pjob = &job;

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
	#if (defined(NEC98))
		static int NEC98_mouseinit[] = {
			0x0060, 4 * 4, 0x0120 /* EnableMouse */, 0, 0
		};
	#endif

	MALLOC_ADDR = pjob->readCSd10 = lib_readCSd(0x0010);

	lib_init(AUTO_MALLOC);
	sgg_init(AUTO_MALLOC);

	signal = signal0 = lib_opensignalbox(256 * 4, AUTO_MALLOC, 0, 4); // 1KB

	// ウィンドウを確保して、全て未使用ウインドウとして登録
	window = (struct WM0_WINDOW *) malloc(MAX_WINDOWS * sizeof (struct WM0_WINDOW));
	for (i = 0; i < MAX_WINDOWS; i++) {
		window[i].sgg.handle = 0;
		chain_unuse(&window[i]);
	}

	// サウンドトラック用バッファの初期化
	sndtrk_buf = (struct SOUNDTRACK *) malloc(MAX_SOUNDTRACK * sizeof (struct SOUNDTRACK));
	for (i = 0; i < MAX_SOUNDTRACK; i++)
	//	free_sndtrk(&sndtrk_buf[i]);
		sndtrk_buf[i].sigbox = 0;

	pjob->list = (int *) malloc(JOBLIST_SIZE * sizeof (int));
	*(pjob->rp = pjob->wp = pjob->list) = 0; /* たまった仕事はない */
	pjob->free = JOBLIST_SIZE - 1;

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
	#if (defined(NEC98))
		sgg_execcmd(NEC98_mouseinit);
	#endif

	wallpaper = malloc(WALLPAPERMAXSIZE);

	for (;;) {
		unsigned char siglen = 1;
		win = top;
		struct SOUNDTRACK *sndtrk;
		switch (i = signal[0]) {
		case 0x0000:
		//	siglen--; /* siglen = 0; */
			lib_waitsignal(0x0001, 0, 0);
			continue;

		case 0x0004 /* rewind */:
		//	siglen--; /* siglen = 0; */
			lib_waitsignal(0x0000, signal[1], 0);
			signal = signal0;
			continue;

		case 0x0010:
			/* 初期化要請 */
		//	siglen = 1;
			#if (defined(PCAT))
				writejob_n(4, 0x0030 /* open VGA driver */, 0x0000,
					0x0034 /* change VGA mode */, 0x0012);
			#elif (defined(TOWNS))
				writejob_n(4, 0x0030 /* open VGA driver */, 0x0000,
					0x0034 /* change VGA mode */, 0x0000);
			#elif (defined(NEC98))
				writejob_n(4, 0x0030 /* open VGA driver */, 0x0000,
					0x0034 /* change VGA mode */, 0x0000);
			#endif
		fin_wrtjob:
			*pjob->wp = 0; /* ストッパー */
		check_jobnext:
			if (pjob->now == 0)
				runjobnext();
			break;

		case 0x0018:
			/* from boot */
			siglen++; /* siglen = 2; */
			fromboot = signal[1];
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

		case 0x0020:
			/* ウィンドウオープン要請(handle) */
			win = get_unuse();
			win->flags = 0;
			sgg_wm0_openwindow(&win->sgg, signal[1]);
			siglen++; /* siglen = 2; */
		//	win->ds1 = win->defsig;
			writejob_n(2, 0x0020 /* open */, (int) win);
			goto fin_wrtjob;

		case 0x0024:
			/* ウィンドウクローズ要請(handle) */
			win = handle2window(signal[1]);
			siglen++; /* siglen = 2; */
			if ((win->flags & 0x01) == 0) {
				win->flags |= 0x01; /* クローズ処理中 */
				writejob_n(2, 0x002c /* close */, (int) win);
				goto fin_wrtjob;
			}
			break;

		case 0x0028:
			/* ウィンドウアクティブ要請(opt, handle) */
			goto mikannsei;

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
			siglen = 9;
			break;

		case 0x0040: /* open sound track (slot, tss, signal-base, reserve0, reserve1)
			   受理したことを知らせるために、シグナルで応答する */
		//	sndtrk = alloc_sndtrk();
			for (sndtrk = sndtrk_buf; sndtrk->sigbox != 0; sndtrk++);
			sndtrk->sigbox  = signal[2 /* tss */] + 0x0240;
			sndtrk->slot    = signal[1 /* slot */];
			sndtrk->sigbase = signal[3 /* signal-base */];
			siglen = 6;
			/* ハンドル番号の対応づけを通達 */
			send_signal3dw(sndtrk->sigbox, sndtrk->sigbase + 0, sndtrk->slot, (int) sndtrk);
			if (sndtrk_active == NULL) {
				sndtrk_active = sndtrk;
				/* アクティブシグナルを送る */
	sndtrk_sendactsig:
				send_signal2dw(sndtrk->sigbox, sndtrk->sigbase + 8 /* enable */, sndtrk->slot);
			}
			break;

		case 0x0044: /* close sound track (handle) */
			sndtrk = (struct SOUNDTRACK *) signal[1];
			siglen++; /* siglen = 2; */
			/* close完了をしらせる */
			send_signal2dw(sndtrk->sigbox, sndtrk->sigbase + 4 /* close */, sndtrk->slot);
		//	free_sndtrk(sndtrk);
			sndtrk->sigbox = 0;
			if (sndtrk == sndtrk_active) {
				/* 違うやつをアクティブにする */
				sndtrk = NULL;
				for (i = 0; i < MAX_SOUNDTRACK; i++) {
					if (sndtrk_buf[i].sigbox) {
						sndtrk = &sndtrk_buf[i];
						break;
					}
				}
				if (sndtrk_active = sndtrk)
					goto sndtrk_sendactsig; /* アクティブシグナルを送る */
			}
			break;

		case 0x0048: /* load external font (font-type, tss, len, sig) */
			siglen = 5;
			writejob_n(4, 0x0038 /* loadfont */, signal[1] /* type */,
				signal[2] /* tss */, signal[4] /* sig */);
			goto fin_wrtjob;

		case 0x0050:
		case 0x0051:
		case 0x0052:
		case 0x0053:
		case 0x0054: /* 0x005fまではリザーブ */
			(*pjob->func)(i - 0x0050, 0);
		//	siglen = 1;
			goto check_jobnext;

		case 0x0014: /* 画面モード変更完了(result) */
		case 0x00c0: /* 更新停止シグナル(handle) */
		case 0x00c4: /* 描画完了シグナル(handle) */
		//	i = signal[0];
			j = signal[1];
			siglen++; /* siglen = 2; */
			(*pjob->func)(i, j);
			goto check_jobnext;

		case 0x00d0:
		case 0x00d1:
		case 0x00d2:
		case 0x00d3:
		case 0x00f0:
		//	i = signal[0];
		//	siglen = 1;
			job_movewin4(i);
			break;


		case 0x0200 /* active bottom window */:
		//	siglen = 1;
			if ((win->flags & 0x01) == 0) {
				writejob_n(2, 0x0024 /* active */, (int) win /* top */ ->up);
				goto fin_wrtjob;
			}
			break;

		case 0x0201 /* active second window */:
		//	siglen = 1;
			if ((win->flags & 0x01) == 0) {
				writejob_n(2, 0x0024 /* active */, (int) win /* top */ ->down);
				goto fin_wrtjob;
			}
			break;

		case 0x0202 /* move window */:
		//	siglen = 1;
			if ((win->flags & 0x01) == 0) {
				writejob_n(2, 0x0028 /* move by keyboard */, (int) win /* top */);
				goto fin_wrtjob;
			}
			break;

		case 0x0203 /* close window */:
		//	siglen = 1;
			if (win /* top */ != pokon0 && (win->flags & 0x01) == 0)
				sgg_wm0s_close(&win /* top */ ->sgg);
			break;

		case 0x0204 /* VGA mode 0x0012 */:
		//	siglen = 1;
			#if (defined(PCAT))
				writejob_n(2, 0x0034 /* change VGA mode */, 0x0012);
			#elif (defined(TOWNS))
				writejob_n(2, 0x0034 /* change VGA mode */, 0x0000);
			#elif (defined(NEC98))
				writejob_n(2, 0x0034 /* change VGA mode */, 0x0000);
			#endif
			goto fin_wrtjob;

		case 0x0205 /* VESA mode 0x0102 */:
		//	siglen = 1;
			#if (defined(PCAT))
				writejob_n(2, 0x0034 /* change VGA mode */, 0x0102);
			#endif
			#if (defined(TOWNS))
				writejob_n(2, 0x0034 /* change VGA mode */, 0x0001);
			#endif
			goto fin_wrtjob;

		case 0x0240 /* load JPN16$.FNT */:
		//	siglen = 1;
			writejob_n(4, 0x0038 /* loadfont */, 0x11 /* type */, 0, 0);
			goto fin_wrtjob;

#if (defined(TOWNS))

		case 0x0244 /* capture */:
		//	siglen = 1;
			writejob_n(1, 0x003c /* capture */);
			goto fin_wrtjob;
#endif

		case 0x0248 /* load wallpaper */:
		//	siglen = 1;
			writejob_n(1, 0x0040 /* load wallpaper */);
			goto fin_wrtjob;

#if (defined(NEC98))
		case 0x0999 /* mouse move to (0, 400-16) */:
			//	siglen = 1;
			if (pjob->now == 0 && mx != 0x80000000)
				sgg_wm0_movemouse(mx = 0, my = 400-16);
			break;

#endif

		case 0x73756f6d /* from mouse */:
			#if (defined(TOWNS))
				if (mx != 0x80000000) {
					mousesignal(((signal[3] >> 4) & 0x03) ^ 0x03,
						- (signed char) (((signal[3] >> 20) & 0xf0) | ((signal[3] >> 16) & 0x0f)),
						- (signed char) (((signal[3] >>  4) & 0xf0) | ( signal[3]        & 0x0f)));
				}
				siglen = 4;
			#endif
			#if (defined(PCAT))
				if (mx != 0x80000000)
					mousesignal(signal[1], (signed short) (signal[2] & 0xffff), (signed short) (signal[2] >> 16));
				siglen = 3;
			#endif
			#if (defined(NEC98))
				if (mx != 0x80000000)
					mousesignal(signal[1] & 0xff,
						  (signed char) ((signal[1] >>  8) & 0xff),
						  (signed char) ((signal[1] >> 16) & 0xff));
				siglen = 2;
			#endif

			break;

		case 0x6f6b6f70 + 0 /* mousespeed */:
			mousescale = signal[1];
			siglen++; /* siglen = 2; */
			break;

		default:
		mikannsei:
		//	lib_drawline(0x0020, (void *) -1, 0, 0, 0, 15, 15); /* ここに来たことを知らせる */
		//	siglen = 1;
			;
		}
		if (siglen) {
			signal += siglen;
			lib_waitsignal(0x0000, siglen, 0);
		}
	}
}

void init_screen(const int x, const int y)
{
	struct STR_BGV {
		signed char col, x0, y0, x1, y1;
	};
	#if (defined(WIN9X))
		static struct STR_BGV linedata[] = {
			{  8,   0, -28,  -1, -28 },
			{ 15,   0, -27,  -1, -27 },
			{  8,   0, -26,  -1,  -1 },
			{ 15,   3, -24,  59, -24 },
			{ 15,   2, -24,   2,  -4 },
			{  7,   3,  -4,  59,  -4 },
			{  7,  59, -23,  59,  -5 },
			{  0,   2,  -3,  59,  -3 },
			{  0,  60, -24,  60,  -3 },
			{  7, -47, -24,  -4, -24 },
			{  7, -47, -23, -47,  -4 },
			{ 15, -47,  -3,  -4,  -3 },
			{ 15,  -3, -24,  -3,  -3 },
			{ -1,   0,   0,  -1, -29 }		/* for wallpaper */
		};
	#elif (defined(TMENU))
		#define TMGUIB2(c, x, y, w, h) \
			{ 15, x,         y,         x + w - 1, y         }, \
			{ 15, x,         y + 1,     x + w - 2, y + 1     }, \
			{  0, x + w - 1, y + 1,     x + w - 1, y + 1     }, \
			{ 15, x,         y + 2,     x + 1,     y + h - 2 }, \
			{  c, x + 2,     y + 2,     x + w - 3, y + h - 3 }, \
			{  0, x + w - 2, y + 2,     x + w - 1, y + h - 3 }, \
			{  0, x + 2,     y + h - 2, x + w - 1, y + h - 2 }, \
			{ 15, x,         y + h - 1, x,         y + h - 1 }, \
			{  0, x + 1,     y + h - 1, x + w - 1, y + h - 1 }
		#define TMGUIBF1(x, y, w, h) \
			{ 15, x,         y,         x + w - 1, y         }, \
			{ 15, x,         y + 1,     x,         y + h - 1 }, \
			{  0, x + w - 1, y + 1,     x + w - 1, y + h - 2 }, \
			{  0, x + 1,     y + h - 1, x + w - 1, y + h - 1 }
		#define BOXFRAME(c, x0, y0, x1, y1)	\
			{  c, x0,        y0,        x1,        y0        }, \
			{  c, x0,        y0 + 1,    x0,        y1 - 1    }, \
			{  c, x1,        y0 + 1,    x1,        y1 - 1    }, \
			{  c, x0,        y1,        x1,        y1        }
		static struct STR_BGV linedata[] = {
			TMGUIB2(0, 0, 0, 96, 20),			/* FM TOWNS button */
			{ 7, 96, 0, -1, 19 },				/* title bar */
			TMGUIB2(0, -28, 0, 20, 20),			/* exit button */
			BOXFRAME(0, 0, -28, -1, -1),
			{ 7, 1, -27, -2, -2 },				/* bar */
			TMGUIBF1( 6, -24, 90, 20),			/* "start" frame */
			{ -1, 0, 20, -1, -29 }				/* for wallpaper */
		};
		#undef	TMGUIB2
		#undef	TMGUIBF1
		#undef	BOXFRAME
	#elif (defined(CHO_OSASK))
		static struct STR_BGV linedata[] = {
			{  0,   0, -20,  -1, -20 },
			{ 15,   0, -19,  -1, -19 },
			{  7,   0, -18,  -1,  -1 },		/* System line(?) */
			{  0, -82, -19, -82,  -1 },
			{ 15, -81, -18, -81,  -1 },
			{ -1,   0,   0,  -1, -21 },		/* for wallpaper */
		};
	#elif (defined(NEWSTYLE))
		static struct STR_BGV linedata[] = {
			{ -1,   0,   0,  -1,  -1 }		/* for wallpaper */
		};
	#elif (defined(WIN31))
		static struct STR_BGV linedata[] = {
			{ -1,   0,   0,  -1,  -1 }		/* for wallpaper */
		};
	#endif

	struct STR_BGV *p;
	int x0, y0, x1, y1;
	for (p = linedata; ; p++) {
		if ((x0 = p->x0) < 0)
			x0 += x;
		if ((y0 = p->y0) < 0)
			y0 += y;
		if ((x1 = p->x1) < 0)
			x1 += x;
		if ((y1 = p->y1) < 0)
			y1 += y;
		if (p->col < 0) break;
		lib_drawline(0x0020, (void *) -1, p->col, x0, y0, x1, y1);
	}
	putwallpaper(x0, y0, x1 + 1, y1 + 1);

	#if (defined(TMENU))
		static int tosask[] = {
			0x00000000, 0x00000000, 0x00000000, 0x0f0f0f0f, 0x0000000f, 
			0x00000000, 0x0f0f0f0f, 0x0f0f0f0f, 0x000f0f0f, 0x00000000, 
			0x000f0000, 0x00000000, 0x00000000, 0x0f0f0f0f, 0x0f0f0f0f, 
			0x000f0f0f, 0x00000f0f, 0x00000000, 0x0f000000, 0x0e0e0e0f, 
			0x00000000, 0x00000000, 0x0f0f0000, 0x0f0f0f0f, 0x000f0f0f, 
			0x0f000000, 0x0f0f0f0f, 0x0f0f0f0f, 0x000f0f0f, 0x00000000, 
			0x0f0f0f00, 0x00000000, 0x0f000000, 0x0f0f0f0f, 0x0f0f0f0f, 
			0x000f0f0f, 0x00000f0f, 0x00000000, 0x0f0f0f00, 0x0e0e0e0f, 
			0x00000000, 0x00000000, 0x0f0f0f00, 0x00000000, 0x0f0f0f00, 
			0x0f0f0000, 0x0000000f, 0x00000000, 0x00000000, 0x00000000, 
			0x0f0f0f00, 0x00000000, 0x0f0f0000, 0x0000000f, 0x00000000, 
			0x00000000, 0x00000f0f, 0x0f000000, 0x0f0f0f0f, 0x0e0e0e00, 
			0x00000000, 0x00000000, 0x000f0f00, 0x00000000, 0x0f0f0000, 
			0x0f0f0000, 0x0000000f, 0x00000000, 0x00000000, 0x00000000, 
			0x0f000f0f, 0x0000000f, 0x0f0f0000, 0x0000000f, 0x00000000, 
			0x00000000, 0x00000f0f, 0x0f0f0f00, 0x00000f0f, 0x0e0e0e00, 
			0x00000000, 0x00000000, 0x00000f0f, 0x00000000, 0x0f000000, 
			0x0f00000f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0000000f, 0x00000000, 
			0x0f000f0f, 0x0000000f, 0x0f000000, 0x0f0f0f0f, 0x0f0f0f0f, 
			0x0000000f, 0x0f0f0f0f, 0x0f0f0f0f, 0x00000000, 0x0e0e0e00, 
			0x00000000, 0x00000000, 0x00000f0f, 0x00000000, 0x0f000000, 
			0x0000000f, 0x0f0f0f0f, 0x0f0f0f0f, 0x00000f0f, 0x0f000000, 
			0x0000000f, 0x00000f0f, 0x00000000, 0x0f0f0f0f, 0x0f0f0f0f, 
			0x00000f0f, 0x0f0f0f0f, 0x000f0f0f, 0x00000000, 0x0e0e0e00, 
			0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 
			0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 
			0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 
			0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x0e0e0e00, 
			0x0f0f0f0f, 0x00000f0f, 0x00000f0f, 0x00000000, 0x0f000000, 
			0x0000000f, 0x00000000, 0x00000000, 0x000f0f0f, 0x0f0f0000, 
			0x0f0f0f0f, 0x000f0f0f, 0x00000000, 0x00000000, 0x00000000, 
			0x000f0f0f, 0x00000f0f, 0x0f0f0000, 0x0000000f, 0x0e0e0e00, 
			0x0f0f000f, 0x00000f00, 0x00000f0f, 0x00000000, 0x0f000000, 
			0x0000000f, 0x00000000, 0x00000000, 0x000f0f00, 0x0f0f0000, 
			0x0f0f0f0f, 0x000f0f0f, 0x00000000, 0x00000000, 0x00000000, 
			0x000f0f00, 0x00000f0f, 0x0f000000, 0x00000f0f, 0x0e0e0e00, 
			0x0f0f0000, 0x00000000, 0x000f0f00, 0x00000000, 0x0f0f0000, 
			0x00000000, 0x00000000, 0x00000000, 0x000f0f00, 0x000f0f00, 
			0x00000000, 0x0f0f0000, 0x00000000, 0x00000000, 0x00000000, 
			0x000f0f00, 0x00000f0f, 0x00000000, 0x000f0f0f, 0x0e0e0e00, 
			0x0f0f0000, 0x00000000, 0x0f0f0f00, 0x00000000, 0x0f0f0f00, 
			0x00000000, 0x00000000, 0x00000000, 0x000f0f0f, 0x000f0f00, 
			0x00000000, 0x0f0f0000, 0x00000000, 0x00000000, 0x00000000, 
			0x000f0f0f, 0x00000f0f, 0x00000000, 0x0f0f0f00, 0x0e0e0e00, 
			0x0f0f0000, 0x00000000, 0x0f0f0000, 0x0f0f0f0f, 0x000f0f0f, 
			0x0f0f0000, 0x0f0f0f0f, 0x0f0f0f0f, 0x00000f0f, 0x00000f0f, 
			0x00000000, 0x0f000000, 0x0f0f000f, 0x0f0f0f0f, 0x0f0f0f0f, 
			0x00000f0f, 0x00000f0f, 0x00000000, 0x0f0f0000, 0x0e0e0e0f, 
			0x0f0f0f00, 0x0000000f, 0x00000000, 0x0f0f0f0f, 0x0000000f, 
			0x0f0f0000, 0x0f0f0f0f, 0x0f0f0f0f, 0x0000000f, 0x00000f0f, 
			0x00000000, 0x0f000000, 0x0f0f000f, 0x0f0f0f0f, 0x0f0f0f0f, 
			0x0000000f, 0x00000f0f, 0x00000000, 0x0f000000, 0x0e0e0e0f 
		};
		static int exitdoor[] = {
			0x00000000, 0x000f0f0f, 0x00000000, 
			0x00000000, 0x000f0f0f, 0x00000000, 
			0x00000000, 0x000f0f0f, 0x00000000, 
			0x00000000, 0x000f0f0f, 0x00000000, 
			0x00000000, 0x00000000, 0x00000000, 
			0x00000000, 0x000f0f0f, 0x00000000, 
			0x0f000000, 0x0f0f0f0f, 0x00000000, 
			0x0f000000, 0x0f0f0f0f, 0x00000000, 
			0x0f0f0000, 0x0f0f0f0f, 0x0000000f, 
			0x0f0f0000, 0x0f0f0f0f, 0x0000000f, 
			0x0f0f0f00, 0x0f0f0f0f, 0x00000f0f, 
			0x0f0f0f00, 0x0f0f0f0f, 0x00000f0f, 
			0x0f0f0f0f, 0x0f0f0f0f, 0x000f0f0f, 
			0x0f0f0f0f, 0x0f0f0f0f, 0x000f0f0f, 
		};
		static int askcube[] = {
			0x07070707, 0x09000707, 0x07070700, 0x07070707,
			0x07070707, 0x09090900, 0x07000909, 0x07070707,
			0x09000707, 0x01090909, 0x09090900, 0x07070700,
			0x09090007, 0x09090101, 0x09090009, 0x07070009,

			0x01000300, 0x01000000, 0x01090909, 0x07000e00,
			0x0b0b0300, 0x09010000, 0x00000909, 0x07000e00,
			0x0b0b0b00, 0x01000b0b, 0x00000e00, 0x07000e0e,
			0x000b0b00, 0x000b0b00, 0x00000e0e, 0x0700000e,
	
			0x0b0b0000, 0x000b0003, 0x0e000e0e, 0x07000000,
			0x03000b00, 0x00000b0b, 0x0e0e0e0e, 0x07000e0e,
			0x000b0b00, 0x000b0b00, 0x00000e0e, 0x07000e0e,
			0x0b0b0007, 0x000b0b0b, 0x00000e0e, 0x0707000e,

			0x03000707, 0x000b0b0b, 0x00000e0e, 0x07070700,
			0x07070707, 0x00030300, 0x07000e0e, 0x07070707,
			0x07070707, 0x00000707, 0x07070700, 0x07070707
		};
		lib_putblock1((void *) -1,      8,      4, 77, 13, 3, &tosask);
		lib_putblock1((void *) -1, x - 24,      3, 11, 14, 1, &exitdoor);
		lib_putblock1((void *) -1,      8, y - 21, 15, 15, 1, &askcube);
	#endif

	sgg_wm0_putmouse(mx, my);
	return;
}

struct WM0_WINDOW *handle2window(const int handle)
{
	// topの中から探してもいい
	int i;
	struct WM0_WINDOW *win = window;
	for (i = 0; i < MAX_WINDOWS; i++, win++) {
		if (win->sgg.handle == handle)
			return win;
	}
	return NULL;
}

void chain_unuse(struct WM0_WINDOW *win)
{
	// unuseは一番上
	// winは一番下に追加
	if (win->sgg.handle) {
		/* (int) [stack_sel:handle] = 0; */
		/* init.askに、該当のハンドルがフリーであることを教えるため */
		static int zero = 0;
		sgg_directwrite(0, 4, 0, win->sgg.handle, 0x01280030 /* stack_sel */, &zero, 0x000c);
		win->sgg.handle = 0; // handle2windowが間違って検出しないために
	}
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

static struct STR_PRESS {
	enum {
		NO_PRESS, CLOSE_BUTTON, TITLE_BAR
	} pos;
	struct WM0_WINDOW *win;
	int mx0, my0;
} press0 = { NO_PRESS }; 

void mousesignal(const unsigned int header, int dx, int dy)
{
	struct STR_JOB *pjob = &job;
	struct STR_PRESS *press = &press0;
	int _mx = mx, _my = my;
	dx *= mousescale;
	dy *= mousescale;
	if ((header >> 28) == 0x0 /* normal mode */) {
		// マウス状態変更
		int ox = _mx, oy = _my;
		_mx += dx;
		_my += dy;
		if (_mx < 0)
			_mx = 0;
		if (_mx >= x2)
			_mx = x2 - 1;
		if (_my < 0)
			_my = 0;
		if (_my >= y2 - 15)
			_my = y2 - 16;
		if (_mx != ox || _my != oy) {
			mx = _mx;
			my = _my;
			sgg_wm0_movemouse(_mx, _my);
			if (pjob->movewin4_ready) {
				if (press->pos == NO_PRESS || (press->pos == TITLE_BAR && press->win == pjob->win))
					job_movewin4m(_mx, _my);
			}
		}
		if (mbutton != (header & 0x07)) {
			struct WM0_WINDOW *win;
			// マウスのボタン状態が変化
			// bit0:left
			// bit1:right
			// bit2:middle
			// bit3:always 1
			// bit4:reserve(dx bit8)
			// bit5:reserve(dy bit8)

			if ((mbutton & 0x01) == 0x00 && (header & 0x01) == 0x01) {
				// 左ボタンが押された
				if (pjob->movewin4_ready != 0 && press->pos == NO_PRESS) {
					job_movewin4(0x00f0 /* Enter */);
					goto skip;
				}
				// どのwindowをクリックしたのかを検出
				if (win = top) {
					do {
						if (win->x0 <= _mx && _mx < win->x1 && win->y0 <= _my && _my < win->y1)
							goto found_window;
						win = win->down;
					} while (win != top);
					win = NULL;
				}
				if (win != NULL) {
		found_window:
					if (win == top) {
						// アクティブなウィンドウの中にマウスがある
						#if (defined(WIN9X))
							if (win->x1 - 21 <= _mx && _mx < win->x1 - 5 && win->y0 + 5 <= _my && _my < win->y0 + 19) {
								// close buttonをプレスした
								press->win = win;
								press->pos = CLOSE_BUTTON;
							} else if (win->x0 + 3 <= _mx && _mx < win->x1 - 4 && win->y0 + 3 <= _my && _my < win->y0 + 21) {
								/* title-barをプレスした */
								if (pjob->free >= 2) {
									/* 空きが十分にある */
									press->win = win;
									press->pos = TITLE_BAR;
									press->mx0 = _mx;
									press->my0 = _my;
									writejob_n(2, 0x0028 /* move */, (int) win);
								}
							}
						#elif (defined(TMENU) || defined(WIN31))
							if (win->x0 + 2 <= _mx && _mx < win->x0 + 20 && win->y0 + 3 <= _my && _my < win->y0 + 21) {
								// close buttonをプレスした
								press->win = win;
								press->pos = CLOSE_BUTTON;
							} else if (win->x0 + 1 <= _mx && _mx < win->x1 - 1 && win->y0 + 1 <= _my && _my < win->y0 + 21) {
								/* title-barをプレスした */
								if (pjob->free >= 2) {
									/* 空きが十分にある */
									press->win = win;
									press->pos = TITLE_BAR;
									press->mx0 = _mx;
									press->my0 = _my;
									writejob_n(2, 0x0028 /* move */, (int) win);
								}
							}
						#elif (defined(CHO_OSASK))
							if (win->x0 + 4 <= _mx && _mx < win->x0 + 20 && win->y0 + 4 <= _my && _my < win->y0 + 20) {
								// close buttonをプレスした
								press->win = win;
								press->pos = CLOSE_BUTTON;
							} else if (win->x0 + 1 <= _mx && _mx < win->x1 - 1 && win->y0 + 1 <= _my && _my < win->y0 + 21) {
								/* title-barをプレスした */
								if (pjob->free >= 2) {
									/* 空きが十分にある */
									press->win = win;
									press->pos = TITLE_BAR;
									press->mx0 = _mx;
									press->my0 = _my;
									writejob_n(2, 0x0028 /* move */, (int) win);
								}
							}
						#elif (defined(NEWSTYLE)) /* クローズボタンはありません */
							if (win->x0 + 1 <= _mx && _mx < win->x1 - 1 && win->y0 + 1 <= _my && _my < win->y0 + 21) {
								/* title-barをプレスした */
								if (pjob->free >= 2) {
									/* 空きが十分にある */
									press->win = win;
									press->pos = TITLE_BAR;
									press->mx0 = _mx;
									press->my0 = _my;
									writejob_n(2, 0x0028 /* move */, (int) win);
								}
							}
						#endif
					} else {
						writejob_n(2, 0x0024 /* active */, (int) win);
					}
				}
			} else if ((mbutton & 0x01) == 0x01 && (header & 0x01) == 0x00) {
				/* 左ボタンがはなされた */
				switch (press->pos) {
				case CLOSE_BUTTON:
					#if (defined(WIN9X))
						if (press->win->x1 - 21 <= _mx && _mx < press->win->x1 - 5 &&
							press->win->y0 + 5 <= _my && _my < press->win->y0 + 19) {
							if (press->win != pokon0)
								sgg_wm0s_close(&press->win->sgg);
						}
					#elif (defined(TMENU) || defined(WIN31))
						if (press->win->x0 + 2 <= _mx && _mx < press->win->x0 + 20 &&
							press->win->y0 + 3 <= _my && _my < press->win->y0 + 21) {
							if (press->win != pokon0)
								sgg_wm0s_close(&press->win->sgg);
						}
					#elif (defined(CHO_OSASK))
						if (press->win->x0 + 4 <= _mx && _mx < press->win->x0 + 20 &&
							press->win->y0 + 4 <= _my && _my < press->win->y0 + 20) {
							if (press->win != pokon0)
								sgg_wm0s_close(&press->win->sgg);
						}
					#endif
					break;

				case TITLE_BAR:
					if (press->win == pjob->win && pjob->movewin4_ready != 0) {
						/* 移動先確定シグナルを送る */
						job_movewin4(0x00f0 /* Enter */);
					}
				//	break;

				}
				press->pos = NO_PRESS;
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
	if (pjob->now == 0)
		runjobnext();
	return;
}

int writejob_n(int n, int p0,...)
{
	struct STR_JOB *pjob = &job;
	if (pjob->free >= n) {
		int *p = &p0;
		do {
			*pjob->wp++ = *p++;
			pjob->free--;
			if (pjob->wp == pjob->list + JOBLIST_SIZE)
				pjob->wp = pjob->list;
		} while (--n);
		*pjob->wp = 0;
		return 1;
	}
	return 0;
}

const int readjob()
{
	struct STR_JOB *pjob = &job;
	int i = *pjob->rp++;
	pjob->free++;
	if (pjob->rp == pjob->list + JOBLIST_SIZE)
		pjob->rp = pjob->list;
	return i;
}

void runjobnext()
{
	struct STR_JOB *pjob = &job;
	int i, j;
	void (*func1)(int);

	do {
		if ((pjob->now = *pjob->rp) == 0)
			return;

		readjob(); // から読み
		func1 = NULL;
		switch (pjob->now) {
		case 0x0020 /* open new window */:
		//	job_openwin0((struct WM0_WINDOW *) readjob());
			func1 = job_openwin0;
			break;

		case 0x0024 /* active window */:
		//	job_activewin0((struct WM0_WINDOW *) readjob());
			func1 = job_activewin0;
			break;

		case 0x0028 /* move window by keyboard */:
		//	job_movewin0((struct WM0_WINDOW *) readjob());
			func1 = job_movewin0;
			break;

		case 0x002c /* close window */:
		//	job_closewin0((struct WM0_WINDOW *) readjob());
			func1 = job_closewin0;
			break;

		case 0x0030 /* open VGA driver */:
		//	job_openvgadriver(readjob());
			func1 = job_openvgadriver;
			break;

		case 0x0034 /* set VGA mode */:
		//	job_setvgamode0(readjob());
			func1 = job_setvgamode0;
			break;

		case 0x0038 /* load external font */:
			i = readjob();
			j = readjob();
			job_loadfont0(i, j, readjob());
			break;

#if (defined(TOWNS))
		case 0x003c /* capture */:
		    job_savevram0();
		    break;
#endif
		case 0x0040 /* load wallpaper */:
			job_openwallpaper();
			break;
		}
		if (func1)
			(*func1)(readjob());
	} while (pjob->now == 0);
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
		win->x0 &= ~(MOVEUNIT - 1); // オープン位置を8の倍数になるように調整
		win->y0 = RESERVELINE0 + (y2 - (RESERVELINE0 + RESERVELINE1) - ysize) / 2;
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
	// for WIN9X & WIN31 by koyanagi
	#if (defined(WIN9X) || defined(WIN31))
	sgg_wm0_definesignal3(0, 0x0100, 0x407010a2 /* ALT+TAB */,
	      0x3240 /* winman0 signalbox */, 0x7f000001, 0x0200);/* switch */
	sgg_wm0_definesignal3(0, 0x0100, 0x40701084 /* ALT+F4 */,
	      0x3240 /* winman0 signalbox */, 0x7f000001, 0x0203);/* close */
	#endif

	sgg_wm0_definesignal3(1, 0x0100, 0x00701081 /* F1 */,
		0x3240 /* winman0 signalbox */, 0x7f000001, 0x0204);

	sgg_wm0_definesignal3(0, 0x0100, 0x00701085 /* F5 */,
		0x3240 /* winman0 signalbox */, 0x7f000001, 0x0240); /* JPN16$.FNTの即時ロード */

	#if (defined(TOWNS))
		sgg_wm0_definesignal3(0, 0x0100, 0x00701086 /* F6 */,
			0x3240 /* winman0 signalbox */, 0x7f000001, 0x0244); /* CAPTURE */
	#endif
	
	sgg_wm0_definesignal3(0, 0x0100, 0x00701087 /* F7 */,
		0x3240 /* winman0 signalbox */, 0x7f000001, 0x0248); /* load wallpaper */

	#if (defined(NEC98))
		sgg_wm0_definesignal3(1, 0x0100, 0x20701089 /* Ctrl+F9～F10 */,
			0x3240 /* winman0 signalbox */, 0x7f000001, 0x0202);
		sgg_wm0_definesignal3(0, 0x0100, 0x20701081 /* Ctrl+F1 */,
			0x3240 /* winman0 signalbox */, 0x7f000001, 0x0999);
	#endif

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
		job.now = 0;
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
	struct STR_JOB *pjob = &job;

	pjob->win = top /* win */;

	// キー入力シグナルの対応づけを初期化
//	sgg_wm0_definesignal0(255, 0x0100, 0);
	sgg_wm0_definesignal3com();

	// とりあえず、全てのウィンドウの画面更新権を一時的に剥奪する
	pjob->count = 0;
	win = top;
	pjob->func = job_movewin1;
	do {
		win->job_flag0 = 0x01;
		if (win->condition & 0x01) {
			pjob->count++; // disable
			sgg_wm0s_accessdisable(&win->sgg);
		}
		win = win->down;
	} while (win != top);

//	以下は成立しない(最低一つは出力アクティブなウィンドウが存在するから)
//	if (pjob->count == 0) {
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
	if (--job.count == 0)
		job_movewin2();
	return;
}

void job_movewin2()
{
	struct STR_JOB *pjob = &job;
	struct STR_PRESS *press = &press0;
	struct WM0_WINDOW *win = pjob->win;
	// シグナルを宣言(0x00d0～0x00d3, 0x00f0)
	sgg_wm0_definesignal3(3, 0x0100, 0x00ac /* left */,
		0x3240 /* winman0 signalbox */, 0x7f000001, 0x00d0);
	sgg_wm0_definesignal3(0, 0x0100, 0x00a0 /* Enter */,
		0x3240 /* winman0 signalbox */, 0x7f000001, 0x00f0);

	// 枠を描く
	pjob->movewin_x0 = pjob->movewin_x = win->x0;
	pjob->movewin_y0 = pjob->movewin_y = win->y0;
	job_movewin3();
	pjob->movewin4_ready = 1;
	if (press->pos == NO_PRESS) {
		if (pjob->movewin_x > mx || mx >= win->x1 || pjob->movewin_y > my || my >= win->y1) {
			mx = (win->x0 + win->x1) / 2;
			my = win->y0 + 12;
			sgg_wm0_movemouse(mx, my);
		}
		press->mx0 = mx;
		press->my0 = my;
	}
	return;
}

void job_movewin3()
{
	struct STR_JOB *pjob = &job;
	int x0, y0, x1, y1;
	struct WM0_WINDOW *win = pjob->win;

	x0 = pjob->movewin_x;
	y0 = pjob->movewin_y;
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
	struct STR_JOB *pjob = &job;
	struct WM0_WINDOW *win0 = pjob->win;
	struct STR_PRESS *press = &press0;

	int x0 = pjob->movewin_x;
	int y0 = pjob->movewin_y;
	int xsize = win0->x1 - win0->x0;
	int ysize = win0->y1 - win0->y0;

	if (sig == 0x00d0 && x0 >= MOVEUNIT)
		x0 -= MOVEUNIT;
	if (sig == 0x00d1 && x0 + xsize <= x2 - 4)
		x0 += MOVEUNIT;
	if (sig == 0x00d2 && y0 >= MOVEUNIT + RESERVELINE0)
		y0 -= MOVEUNIT;
	if (sig == 0x00d3 && y0 + ysize <= y2 - (RESERVELINE1 + MOVEUNIT))
		y0 += MOVEUNIT;

	if ((x0 - pjob->movewin_x) | (y0 - pjob->movewin_y)) {
		if (press->pos == NO_PRESS || press->pos == TITLE_BAR) {
			mx += x0 - pjob->movewin_x;
			my += y0 - pjob->movewin_y;
			sgg_wm0_movemouse(mx, my);
		}
		job_movewin3();
		pjob->movewin_x = x0;
		pjob->movewin_y = y0;
		job_movewin3();
	}

	if (sig == 0x00f0) {
		pjob->movewin4_ready = 0;
		press->mx0 = -1;
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
		putwallpaper(win0->x0, win0->y0, win0->x1, win0->y1);

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
	struct STR_JOB *pjob = &job;
	struct WM0_WINDOW *win0 = pjob->win;

	int x0 = pjob->movewin_x0 + (x - press0.mx0) & ~(MOVEUNIT - 1);
	int y0 = pjob->movewin_y0 + y - press0.my0;
	int xsize = win0->x1 - win0->x0;
	int ysize = win0->y1 - win0->y0;

	if (x0 < 0)
		x0 = 0;
	if (x0 > x2 - xsize)
		x0 = (x2 - xsize) & ~(MOVEUNIT - 1);
	if (y0 < RESERVELINE0)
		y0 = RESERVELINE0;
	if (y0 > y2 - ysize - RESERVELINE1)
		y0 = y2 - ysize - RESERVELINE1;
	if ((x0 - pjob->movewin_x) | (y0 - pjob->movewin_y)) {
		job_movewin3();
		pjob->movewin_x = x0;
		pjob->movewin_y = y0;
		job_movewin3();
	}
	return;
}

void job_closewin0(struct WM0_WINDOW *win0)
// このウィンドウは既にaccessdisableになっていることが前提
{
	struct STR_JOB *pjob = &job;
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

	pjob->count = 0;
	pjob->func = job_closewin1;

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
				pjob->count++;
				flag0 = WINFLG_MUSTREDRAW | WINFLG_OVERRIDEDISABLED | WINFLG_WAITDISABLE;
			}
		}
		win1->job_flag0 = flag0;
	} while ((win1 = win1->down) != top);

no_window:

	/* ウィンドウを消す */
	putwallpaper(win0->x0, win0->y0, win0->x1, win0->y1);
	chain_unuse(win0);
	if (pjob->count == 0)
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
	if (--job.count == 0)
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
	struct STR_JOB *pjob = &job;
	struct WM0_WINDOW *win0, *win1, *bottom, *top_ = top /* 高速化、コンパクト化のため */;
	int flag0;

	if (top_ == NULL) {
		pjob->now = 0;
	//	pjob->func = NULL;
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
	pjob->count = 0;
	pjob->func = job_general2;
	win0 = top_;
	do {
		if (win0->job_flag0 & WINFLG_MUSTREDRAW) {
			for (win1 = win0->down; win1 != win0; win1 = win1->down) {
				if (win1->job_flag0 & WINFLG_MUSTREDRAW) {
					if (overrapwin(win0, win1)) {
						if ((win0->job_flag0 & WINFLG_OVERRIDEDISABLED) == 0) {
							sgg_wm0s_accessdisable(&win0->sgg);
							win0->job_flag0 |= WINFLG_OVERRIDEDISABLED | WINFLG_WAITDISABLE;
							pjob->count++;
						}
						if ((win1->job_flag0 & (WINFLG_MUSTREDRAW | WINFLG_OVERRIDEDISABLED)) == WINFLG_MUSTREDRAW) {
							sgg_wm0s_accessdisable(&win1->sgg);
							win1->job_flag0 |= WINFLG_OVERRIDEDISABLED | WINFLG_WAITDISABLE;
							pjob->count++;
						}
					}
				}
			}
		}
	} while ((win0 = win0->down) != top_);
	pjob->win = NULL;
	if (pjob->count == 0)
		job_general2(0, 0);
	return;
}

void job_general2(const int cmd, const int handle)
{
	struct STR_JOB *pjob = &job;
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
		if (--pjob->count)
			return;
	}

	win = pjob->win;

	if (win == top && win->job_flag0 == 0)
		goto fin;
		/* １周目でwin->job_flag0が0になることもありうる */
		// 全ての作業が完了
	if (win == NULL)
		win = top;

	do {
		win = win->up;
		flag0 = win->job_flag0;
		win->job_flag0 = 0;
		if ((flag0 & (WINFLG_OVERRIDEDISABLED | 0x01)) == (WINFLG_OVERRIDEDISABLED | 0x01) && x2 != 0)
			sgg_wm0s_accessenable(&win->sgg);
		if (flag0 & WINFLG_MUSTREDRAW) {
			pjob->win = win;
			if ((flag0 & (WINFLG_OVERRIDEDISABLED | 0x01)) == 0) {
				sgg_wm0s_accessdisable(&win->sgg);
				win->job_flag0 |= WINFLG_WAITDISABLE;
				pjob->count = 1;
			}
			if ((flag0 & 0x03) != win->condition)
				sgg_wm0s_setstatus(&win->sgg, win->condition = (flag0 & 0x03));
			sgg_wm0s_redraw(&win->sgg);
			win->job_flag0 |= WINFLG_WAITREDRAW;
			pjob->count++;
			return;
		}
	} while (win != top);
fin:
	pjob->now = 0;
//	pjob->func = NULL;
	return;
}

#if 0

void free_sndtrk(struct SOUNDTRACK *sndtrk)
{
	sndtrk->sigbox = 0;
	return;
}

struct SOUNDTRACK *alloc_sndtrk()
{
	struct SOUNDTRACK *sndtrk = sndtrk_buf;
	int i;
	for (i = 0; i < MAX_SOUNDTRACK; i++, sndtrk++) {
		if (sndtrk->sigbox == 0)
			return sndtrk;
	}
	return NULL;
}

#endif

void send_signal2dw(const int sigbox, const int data0, const int data1)
{
	sgg_execcmd0(0x0020, 0x80000000 + 3, sigbox | 2, data0, data1, 0x000);
	return;
}

void send_signal3dw(const int sigbox, const int data0, const int data1, const int data2)
{
	sgg_execcmd0(0x0020, 0x80000000 + 4, sigbox | 3, data0, data1, data2, 0x000);
	return;
}

void gapi_loadankfont();

void job_openvgadriver(const int drv)
{
	#define	S	((((((((0
	#define	O	* 2 + 1)
	#define	_	* 2 + 0)
	#define	T	, ((((((((0

	static union {
		struct {
			unsigned char p0[32], p1[32];
		};
		int align;
	} mcursor = {
		#if (defined(WIN9X))
			0xc0, 0x00, 0xa0, 0x00, 0x90, 0x00, 0x88, 0x00,
			0x84, 0x00, 0x82, 0x00, 0x81, 0x00, 0x80, 0x80,
			0x83, 0x00, 0x84, 0x00, 0xa2, 0x00, 0xd2, 0x00,
			0x09, 0x00, 0x09, 0x00, 0x04, 0x80, 0x03, 0x00,

			0x00, 0x00, 0x40, 0x00, 0x60, 0x00, 0x70, 0x00,
			0x78, 0x00, 0x7c, 0x00, 0x7e, 0x00, 0x7f, 0x00,
			0x7c, 0x00, 0x78, 0x00, 0x5c, 0x00, 0x0c, 0x00,
			0x06, 0x00, 0x06, 0x00, 0x03, 0x00, 0x00, 0x00
#if 0
			S O O _ _ _ _ _ _ T _ _ _ _ _ _ _ _,
			S O _ O _ _ _ _ _ T _ _ _ _ _ _ _ _,
			S O _ _ O _ _ _ _ T _ _ _ _ _ _ _ _,
			S O _ _ _ O _ _ _ T _ _ _ _ _ _ _ _,
			S O _ _ _ _ O _ _ T _ _ _ _ _ _ _ _,
			S O _ _ _ _ _ O _ T _ _ _ _ _ _ _ _,
			S O _ _ _ _ _ _ O T _ _ _ _ _ _ _ _,
			S O _ _ _ _ _ _ _ T O _ _ _ _ _ _ _,
			S O _ _ _ _ _ O O T _ _ _ _ _ _ _ _,
			S O _ _ _ _ O _ _ T _ _ _ _ _ _ _ _,
			S O _ O _ _ _ O _ T _ _ _ _ _ _ _ _,
			S O O _ O _ _ O _ T _ _ _ _ _ _ _ _,
			S _ _ _ _ O _ _ O T _ _ _ _ _ _ _ _,
			S _ _ _ _ O _ _ O T _ _ _ _ _ _ _ _,
			S _ _ _ _ _ O _ _ T O _ _ _ _ _ _ _,
			S _ _ _ _ _ _ O O T _ _ _ _ _ _ _ _,

			S _ _ _ _ _ _ _ _ T _ _ _ _ _ _ _ _,
			S _ O _ _ _ _ _ _ T _ _ _ _ _ _ _ _,
			S _ O O _ _ _ _ _ T _ _ _ _ _ _ _ _,
			S _ O O O _ _ _ _ T _ _ _ _ _ _ _ _,
			S _ O O O O _ _ _ T _ _ _ _ _ _ _ _,
			S _ O O O O O _ _ T _ _ _ _ _ _ _ _,
			S _ O O O O O O _ T _ _ _ _ _ _ _ _,
			S _ O O O O O O O T _ _ _ _ _ _ _ _,

			S _ O O O O O _ _ T _ _ _ _ _ _ _ _,
			S _ O O O O _ _ _ T _ _ _ _ _ _ _ _,
			S _ O _ O O O _ _ T _ _ _ _ _ _ _ _,
			S _ _ _ _ O O _ _ T _ _ _ _ _ _ _ _,
			S _ _ _ _ _ O O _ T _ _ _ _ _ _ _ _,
			S _ _ _ _ _ O O _ T _ _ _ _ _ _ _ _,
			S _ _ _ _ _ _ O O T _ _ _ _ _ _ _ _,
			S _ _ _ _ _ _ _ _ T _ _ _ _ _ _ _ _
#endif
		#elif (defined(TMENU))
			/* オリジナルマウスカーソルパターン(16x16, mono) by I.Tak. */
			/* TOWNS の内臓に入ってるものに似せていますがフルスクラッチです。*/
			0x80, 0x00, 0xc0, 0x00, 0xa0, 0x00, 0x90, 0x00,
			0xc8, 0x00, 0xa4, 0x00, 0xc2, 0x00, 0xa1, 0x00,
			0xd0, 0x80, 0xa3, 0xc0, 0xd7, 0x00, 0xbc, 0x00,
			0xfa, 0x00, 0xca, 0x00, 0x8e, 0x00, 0x06, 0x00,

			0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x60, 0x00,
			0x30, 0x00, 0x58, 0x00, 0x3c, 0x00, 0x5e, 0x00,
			0x2f, 0x00, 0x5c, 0x00, 0x28, 0x00, 0x48, 0x00,
			0x04, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00
#if 0
			S O _ _ _ _ _ _ _ T _ _ _ _ _ _ _ _,
			S O O _ _ _ _ _ _ T _ _ _ _ _ _ _ _,
			S O _ O _ _ _ _ _ T _ _ _ _ _ _ _ _,
			S O _ _ O _ _ _ _ T _ _ _ _ _ _ _ _,
			S O O _ _ O _ _ _ T _ _ _ _ _ _ _ _,
			S O _ O _ _ O _ _ T _ _ _ _ _ _ _ _,
			S O O _ _ _ _ O _ T _ _ _ _ _ _ _ _,
			S O _ O _ _ _ _ O T _ _ _ _ _ _ _ _,
			S O O _ O _ _ _ _ T O _ _ _ _ _ _ _,
			S O _ O _ _ _ O O T O O _ _ _ _ _ _,
			S O O _ O _ O O O T _ _ _ _ _ _ _ _,
			S O _ O O _ O O _ T _ _ _ _ _ _ _ _,
			S O O O O O _ O _ T _ _ _ _ _ _ _ _,
			S O O _ _ O _ O _ T _ _ _ _ _ _ _ _,
			S O _ _ _ O O O _ T _ _ _ _ _ _ _ _,
			S _ _ _ _ _ O O _ T _ _ _ _ _ _ _ _,

			S _ _ _ _ _ _ _ _ T _ _ _ _ _ _ _ _,
			S _ _ _ _ _ _ _ _ T _ _ _ _ _ _ _ _,
			S _ O _ _ _ _ _ _ T _ _ _ _ _ _ _ _,
			S _ O O _ _ _ _ _ T _ _ _ _ _ _ _ _,
			S _ _ O O _ _ _ _ T _ _ _ _ _ _ _ _,
			S _ O _ O O _ _ _ T _ _ _ _ _ _ _ _,
			S _ _ O O O O _ _ T _ _ _ _ _ _ _ _,
			S _ O _ O O O O _ T _ _ _ _ _ _ _ _,
			S _ _ O _ O O O O T _ _ _ _ _ _ _ _,
			S _ O _ O O O _ _ T _ _ _ _ _ _ _ _,
			S _ _ O _ O _ _ _ T _ _ _ _ _ _ _ _,
			S _ O _ _ O _ _ _ T _ _ _ _ _ _ _ _,
			S _ _ _ _ _ O _ _ T _ _ _ _ _ _ _ _,
			S _ _ _ _ _ O _ _ T _ _ _ _ _ _ _ _,
			S _ _ _ _ _ _ _ _ T _ _ _ _ _ _ _ _,
			S _ _ _ _ _ _ _ _ T _ _ _ _ _ _ _ _
#endif
		#elif (defined(CHO_OSASK))
			/* CHO Kanji like cursor made by I.Tak. */
			0xe0, 0x00, 0x90, 0x00, 0x90, 0x00, 0x48, 0x00,
			0x48, 0x00, 0x25, 0xe0, 0x26, 0x18, 0x12, 0x08,
			0x30, 0x04, 0x28, 0x04, 0x28, 0x04, 0x20, 0x0e,
			0x10, 0x3e, 0x0c, 0xf8, 0x03, 0xe0, 0x03, 0x80,

			0x00, 0x00, 0x60, 0x00, 0x60, 0x00, 0x30, 0x00,
			0x30, 0x00, 0x18, 0x00, 0x19, 0xe0, 0x0d, 0xf0,
			0x0f, 0xf8, 0x17, 0xf8, 0x17, 0xf8, 0x1f, 0xf0,
			0x0f, 0xc0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00
#if 0
			S O O O _ _ _ _ _ T _ _ _ _ _ _ _ _,
			S O _ _ O _ _ _ _ T _ _ _ _ _ _ _ _,
			S O _ _ O _ _ _ _ T _ _ _ _ _ _ _ _,
			S _ O _ _ O _ _ _ T _ _ _ _ _ _ _ _,
			S _ O _ _ O _ _ _ T _ _ _ _ _ _ _ _,
			S _ _ O _ _ O _ O T O O O _ _ _ _ _,
			S _ _ O _ _ O O _ T _ _ _ O O _ _ _,
			S _ _ _ O _ _ O _ T _ _ _ _ O _ _ _,
			S _ _ O O _ _ _ _ T _ _ _ _ _ O _ _,
			S _ _ O _ O _ _ _ T _ _ _ _ _ O _ _,
			S _ _ O _ O _ _ _ T _ _ _ _ _ O _ _,
			S _ _ O _ _ _ _ _ T _ _ _ _ O O O _,
			S _ _ _ O _ _ _ _ T _ _ O O O O O _,
			S _ _ _ _ O O _ _ T O O O O O _ _ _,
			S _ _ _ _ _ _ O O T O O O _ _ _ _ _,
			S _ _ _ _ _ _ O O T O _ _ _ _ _ _ _
#endif
		#elif (defined(NEWSTYLE))
			/* NEWSTYLE Cursor made by I.Tak. */
			0xc0, 0x00, 0xb0, 0x00, 0x4e, 0x00, 0x42, 0x00,
			0x24, 0x00, 0x2a, 0x00, 0x35, 0x00, 0x02, 0x80,
			0x01, 0x40, 0x00, 0xa0, 0x00, 0x50, 0x00, 0x28,
			0x00, 0x14, 0x00, 0x0a, 0x00, 0x05, 0x00, 0x02,

			0x00, 0x00, 0x40, 0x00, 0x30, 0x00, 0x3c, 0x00,
			0x18, 0x00, 0x14, 0x00, 0x02, 0x00, 0x01, 0x00,
			0x00, 0x80, 0x00, 0x40, 0x00, 0x20, 0x00, 0x10,
			0x00, 0x08, 0x00, 0x04, 0x00, 0x02, 0x00, 0x00
#if 0
			S O O _ _ _ _ _ _ T _ _ _ _ _ _ _ _,
			S O _ O O _ _ _ _ T _ _ _ _ _ _ _ _,
			S _ O _ _ O O O _ T _ _ _ _ _ _ _ _,
			S _ O _ _ _ _ O _ T _ _ _ _ _ _ _ _,
			S _ _ O _ _ O _ _ T _ _ _ _ _ _ _ _,
			S _ _ O _ O _ O _ T _ _ _ _ _ _ _ _,
			S _ _ O O _ O _ O T _ _ _ _ _ _ _ _,
			S _ _ _ _ _ _ O _ T O _ _ _ _ _ _ _,
			S _ _ _ _ _ _ _ O T _ O _ _ _ _ _ _,
			S _ _ _ _ _ _ _ _ T O _ O _ _ _ _ _,
			S _ _ _ _ _ _ _ _ T _ O _ O _ _ _ _,
			S _ _ _ _ _ _ _ _ T _ _ O _ O _ _ _,
			S _ _ _ _ _ _ _ _ T _ _ _ O _ O _ _,
			S _ _ _ _ _ _ _ _ T _ _ _ _ O _ O _,
			S _ _ _ _ _ _ _ _ T _ _ _ _ _ O _ O,
			S _ _ _ _ _ _ _ _ T _ _ _ _ _ _ O _,

			S _ _ _ _ _ _ _ _ T _ _ _ _ _ _ _ _,
			S _ O _ _ _ _ _ _ T _ _ _ _ _ _ _ _,
			S _ _ O O _ _ _ _ T _ _ _ _ _ _ _ _,
			S _ _ O O O O _ _ T _ _ _ _ _ _ _ _,
			S _ _ _ O O _ _ _ T _ _ _ _ _ _ _ _,
			S _ _ _ O _ O _ _ T _ _ _ _ _ _ _ _,
			S _ _ _ _ _ _ O _ T _ _ _ _ _ _ _ _,
			S _ _ _ _ _ _ _ O T _ _ _ _ _ _ _ _,
			S _ _ _ _ _ _ _ _ T O _ _ _ _ _ _ _,
			S _ _ _ _ _ _ _ _ T _ O _ _ _ _ _ _,
			S _ _ _ _ _ _ _ _ T _ _ O _ _ _ _ _,
			S _ _ _ _ _ _ _ _ T _ _ _ O _ _ _ _,
			S _ _ _ _ _ _ _ _ T _ _ _ _ O _ _ _,
			S _ _ _ _ _ _ _ _ T _ _ _ _ _ O _ _,
			S _ _ _ _ _ _ _ _ T _ _ _ _ _ _ O _,
			S _ _ _ _ _ _ _ _ T _ _ _ _ _ _ _ _
#endif
		#elif (defined(WIN31))
			/* 改造済みマウスカーソルパターン(16x16, mono) by 聖人 */
			/* 少しはWin3.1の雰囲気出るかなーなんて… */
			/* 16進に変換するの面倒くさい。今までのほうがよかった… */
			0xc0, 0x00, 0xa0, 0x00, 0x90, 0x00, 0x88, 0x00,
			0x84, 0x00, 0x82, 0x00, 0x81, 0x00, 0x80, 0x80,
			0x80, 0x40, 0x83, 0xc0, 0x99, 0x00, 0xa9, 0x00,
			0x44, 0x80, 0x04, 0x80, 0x03, 0x80, 0x00, 0x00,

			0x00, 0x00, 0x40, 0x00, 0x60, 0x00, 0x70, 0x00,
			0x78, 0x00, 0x7c, 0x00, 0x7e, 0x00, 0x7f, 0x00,
			0x7f, 0x80, 0x7c, 0x00, 0x66, 0x00, 0x46, 0x00,
			0x03, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00
#if 0
			S 0 0 _ _ _ _ _ _ T _ _ _ _ _ _ _ _,
			S 0 _ 0 _ _ _ _ _ T _ _ _ _ _ _ _ _,
			S 0 _ _ 0 _ _ _ _ T _ _ _ _ _ _ _ _,
			S 0 _ _ _ 0 _ _ _ T _ _ _ _ _ _ _ _,
			S 0 _ _ _ _ 0 _ _ T _ _ _ _ _ _ _ _,
			S 0 _ _ _ _ _ 0 _ T _ _ _ _ _ _ _ _,
			S 0 _ _ _ _ _ _ 0 T _ _ _ _ _ _ _ _,
			S 0 _ _ _ _ _ _ _ T 0 _ _ _ _ _ _ _,
			S 0 _ _ _ _ _ _ _ T _ 0 _ _ _ _ _ _,
			S 0 _ _ _ _ _ 0 0 T 0 0 _ _ _ _ _ _,
			S 0 _ _ 0 0 _ _ 0 T _ _ _ _ _ _ _ _,
			S 0 _ 0 _ 0 _ _ 0 T _ _ _ _ _ _ _ _,
			S _ 0 _ _ _ 0 _ _ T 0 _ _ _ _ _ _ _,
			S _ _ _ _ _ 0 _ _ T 0 _ _ _ _ _ _ _,
			S _ _ _ _ _ _ 0 0 T _ _ _ _ _ _ _ _,
			S _ _ _ _ _ _ _ _ T _ _ _ _ _ _ _ _,

			S _ _ _ _ _ _ _ _ T _ _ _ _ _ _ _ _,
			S _ 0 _ _ _ _ _ _ T _ _ _ _ _ _ _ _,
			S _ 0 0 _ _ _ _ _ T _ _ _ _ _ _ _ _,
			S _ 0 0 0 _ _ _ _ T _ _ _ _ _ _ _ _,
			S _ 0 0 0 0 _ _ _ T _ _ _ _ _ _ _ _,
			S _ 0 0 0 0 0 _ _ T _ _ _ _ _ _ _ _,
			S _ 0 0 0 0 0 0 _ T _ _ _ _ _ _ _ _,
			S _ 0 0 0 0 0 0 0 T _ _ _ _ _ _ _ _,
			S _ 0 0 0 0 0 0 0 T 0 _ _ _ _ _ _ _,
			S _ 0 0 0 0 0 _ _ T _ _ _ _ _ _ _ _,
			S _ 0 0 _ _ 0 0 _ T _ _ _ _ _ _ _ _,
			S _ 0 _ _ _ 0 0 _ T _ _ _ _ _ _ _ _,
			S _ _ _ _ _ _ 0 0 T _ _ _ _ _ _ _ _,
			S _ _ _ _ _ _ 0 0 T _ _ _ _ _ _ _ _,
			S _ _ _ _ _ _ _ _ T _ _ _ _ _ _ _ _,
			S _ _ _ _ _ _ _ _ T _ _ _ _ _ _ _ _
#endif
		#endif
	};

	#undef	S
	#undef	O
	#undef	_
	#undef	T

	// closeする時にすべてのウィンドウはdisableになっているはずなので、
	// ここではdisableにはしない
	sgg_wm0_gapicmd_0010_0000();
	gapi_loadankfont();

	/* マウスパターン転送 */
	sgg_execcmd0(0x0050, 7 * 4, 0x0180, 0, 0x0001, &mcursor, 0x000c, 0x0000);

	job.now = 0;
	return;
}

void job_setvgamode0(const int mode)
{
	struct STR_JOB *pjob = &job;
	struct WM0_WINDOW *win;
	pjob->int0 = mode;

	if (mx != 0x80000000) {
		sgg_wm0_removemouse();
	} else
		mx = my = 1;

	// とりあえず、全てのウィンドウの画面更新権を一時的に剥奪する
	pjob->count = 0;
	pjob->func = job_setvgamode1;
	if (win = top) {
		do {
			win->job_flag0 = (WINFLG_MUSTREDRAW | WINFLG_OVERRIDEDISABLED); // override-accessdisabled & must-redraw
			if ((win->condition & 0x01) != 0 && x2 != 0) {
				pjob->count++; // disable
				sgg_wm0s_accessdisable(&win->sgg);
			}
		} while ((win = win->down) != top);
	}

	if (pjob->count == 0)
		job_setvgamode2(); // すぐにディスプレーモード変更へ

	return;
}

void job_setvgamode1(const int cmd, const int handle)
{
	// 0x00c0しかこない
	if (--job.count == 0)
		job_setvgamode2();
	return;
}

void job_setvgamode2()
{
	#if (defined(TOWNS))
#if 0
		int mode = job.int0;
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
		x2 = TWVSW;
		y2 = 512*1024/TWVSW;
		/* 画面モード0設定(640x480) */
		/* 画面モード1設定(768x512) */
		sgg_execcmd0(0x0050, 7 * 4, 0x001c, 0, 0x0020, job.int0 /* mode */, 0x0000, 0x0000);

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
		job.func = &job_setvgamode3;
		sgg_wm0_setvideomode(job.int0 /* mode */, 0x0014);
		return;
	#endif

	#if (defined(NEC98))
		x2 = 640;
		y2 = 400;
		sgg_execcmd0(0x0050, 7 * 4, 0x001c, 0, 0x0020, job.int0 /* mode */, 0x0000, 0x0000);
		sgg_wm0_gapicmd_001c_0004(); /* ハードウェア初期化 */
		init_screen(x2, y2);
		job_general1();
		return;
	#endif
}

#if (defined(PCAT))

void job_setvgamode3(const int sig, const int result)
{
	// 0x0014しかこない
	if (result == 0) {
		sgg_execcmd0(0x0050, 7 * 4, 0x001c, 0, 0x0020, job.int0 | 0x01000000, 0x0000, 0x0000);
		sgg_wm0_gapicmd_001c_0004(); // ハードウェア初期化
		init_screen(x2, y2);
		job_general1();
		return;
	}

	// VESAのノンサポートなどにより、画面モード切り換え失敗
//	x2 = 640;
//	y2 = 480;
//	job.func = job_setvgamode3;
	sgg_wm0_setvideomode(0x0012 /* VGA */, 0x0014);
	return;
}

#endif

void job_loadfont0(int fonttype, int tss, int sig)
{
	struct STR_JOB *pjob = &job;
	pjob->sig = sig;
	pjob->fonttss = tss;
	if (pjob->fontflag & 0x01) {
		job_loadfont1(0, 0);
		return;
	}
	/* ロードコード */
	lib_initmodulehandle0(0x000c, 0x0200); /* machine-dirに初期化 */
	pjob->func = job_loadfont1;
	lib_steppath0(0, 0x0200, "JPN16V00.FNT", 0x0050 /* sig */);
	return;
}

void job_loadfont1(int flag, int dmy)
{
	struct STR_JOB *pjob = &job;
	if ((pjob->fontflag & 0x01) == 0 && flag == 0 /* ロード成功 */) {
		char *fp = (char *) pjob->readCSd10 /* malloc領域の終わり == mapping領域の始まり */;
		lib_mapmodule(0x0000, 0x0200, 0x5, 256 * 1024, fp, 0);

		/* ここでslot:0x0210にgapidataを配備 */
		/* ロードが終わってから配備するのが重要で、そうでないとgapidataの拡張が終わってないかもしれないから */
		sgg_execcmd0(0x007c, 0, 0x0210, 0x0000);

		lib_mapmodule(0x0000, 0x0210, 0x7 /* R/W */, 304 * 1024, fp + 256 * 1024, (8 + 16) * 1024);
		lib_decodetek0(304 * 1024, (int) fp, 0x000c, (int) fp + 256 * 1024, 0x000c);
		lib_unmapmodule(0, 768 * 1024, fp);
		pjob->fontflag |= 0x01;
		send_signal3dw(0x4000 /* pokon0 */ | 0x240, 0x7f000002,
			0x008c /* SIGNAL_FREE_FILES */, 0x3000 /* winman0 */);
	}
//	if (pjob->fonttss)
//		send_signal2dw(job_fonttss | 0x240, 0x7f000001, job_sig);
//	pjob->now = 0;
//	pjob->func = NULL;
	job_loadfont2();
	return;
}

void job_loadfont2()
{
	if (job.fontflag & 0x02) {
		job_loadfont3(0, 0);
		return;
	}
	/* ロードコード */
	lib_initmodulehandle0(0x000c, 0x0200); /* machine-dirに初期化 */
	job.func = job_loadfont3;
	lib_steppath0(0, 0x0200, "KOR16V00.FNT", 0x0050 /* sig */);
	return;
}

void job_loadfont3(int flag, int dmy)
{
	struct STR_JOB *pjob = &job;
	if ((pjob->fontflag & 0x02) == 0 && flag == 0 /* ロード成功 */) {
		char *fp = (char *) pjob->readCSd10 /* malloc領域の終わり == mapping領域の始まり */;
		lib_mapmodule(0x0000, 0x0200, 0x5, 256 * 1024, fp, 0);

		/* ここでslot:0x0210にgapidataを配備 */
		/* ロードが終わってから配備するのが重要で、そうでないとgapidataの拡張が終わってないかもしれないから */
		sgg_execcmd0(0x007c, 0, 0x0210, 0x0000);

		lib_mapmodule(0x0000, 0x0210, 0x7 /* R/W */, 288 * 1024, fp + 256 * 1024, (8 + 320) * 1024);
		lib_decodetek0(288 * 1024, (int) fp, 0x000c, (int) fp + 256 * 1024, 0x000c);
		lib_unmapmodule(0, 768 * 1024, fp);
		pjob->fontflag |= 0x02;
		send_signal3dw(0x4000 /* pokon0 */ | 0x240, 0x7f000002,
			0x008c /* SIGNAL_FREE_FILES */, 0x3000 /* winman0 */);
	}
	if (pjob->fonttss) {
		send_signal2dw(pjob->fonttss | 0x240, 0x7f000001, pjob->sig);
		send_signal2dw(pjob->fonttss | 0x240, 0x000000cc, 0); /* to pioneer0 */
	}
	pjob->now = 0;
//	pjob->func = NULL;
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
		int letters[80];
		int eoc;
    } command;

	const unsigned char *s;
	int *t;
	int length;
	
	// strの長さを調べる
	for (s = (const unsigned char *) str; *s++; );

	if (length = s - (const unsigned char *) str - 1) {
		command.cmd = 0x0048;
		command.opt = opt;
		command.window = win;
		command.charset = charset;
		command.x0 = x0;
		command.y0 = y0;
		command.color = color;
		command.backcolor = backcolor;
		command.length = length;

		s = (const unsigned char *) str;
		t = command.letters;
		while (*t++ = *s++);

		lib_execcmd(command);
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
			{ 0x39, IGSHIFT, 0xff, 0xff    } /* ' ' */,
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
			{ 0x1c, IGSHIFT, 0x9c, IGSHIFT } /* Enter */,
			{ 0x0e, IGSHIFT, 0xff, 0xff    } /* BackSpace */,
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
			{ 0x35, IGSHIFT, 0xff, 0xff    } /* ' ' */,
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
			{ 0x1d, IGSHIFT, 0x45, IGSHIFT } /* Enter */,
			{ 0x0f, IGSHIFT, 0xff, 0xff    } /* BackSpace */,
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
		#if (defined(NEC98))
			{ 0x34, IGSHIFT, 0xff, 0xff    } /* ' ' */,
			{ 0x01, SHIFT,   0xff, 0xff    } /* '!' */,
			{ 0x02, SHIFT,   0xff, 0xff    } /* '\x22' */,
			{ 0x03, SHIFT,   0xff, 0xff    } /* '#' */,
			{ 0x04, SHIFT,   0xff, 0xff    } /* '%' */,
			{ 0x05, SHIFT,   0xff, 0xff    } /* '$' */,
			{ 0x06, SHIFT,   0xff, 0xff    } /* '&' */,
			{ 0x07, SHIFT,   0xff, 0xff    } /* '\x27' */,
			{ 0x08, SHIFT,   0xff, 0xff    } /* '(' */,
			{ 0x09, SHIFT,   0xff, 0xff    } /* ')' */,
			{ 0x27, SHIFT,   0x45, NOSHIFT } /* '*' */,
			{ 0x26, SHIFT,   0x49, NOSHIFT } /* '+' */,
			{ 0x30, NOSHIFT, 0x4f, NOSHIFT } /* ',' */,
			{ 0x0b, NOSHIFT, 0x40, NOSHIFT } /* '-' */,
			{ 0x31, NOSHIFT, 0x50, NOSHIFT } /* '.' */,
			{ 0x32, NOSHIFT, 0x41, NOSHIFT } /* '/' */,
			{ 0x0a, NOSHIFT, 0x4e, NOSHIFT } /* '0' */,
			{ 0x01, NOSHIFT, 0x4a, NOSHIFT } /* '1' */,
			{ 0x02, NOSHIFT, 0x4b, NOSHIFT } /* '2' */,
			{ 0x03, NOSHIFT, 0x4c, NOSHIFT } /* '3' */,
			{ 0x04, NOSHIFT, 0x46, NOSHIFT } /* '4' */,
			{ 0x05, NOSHIFT, 0x47, NOSHIFT } /* '5' */,
			{ 0x06, NOSHIFT, 0x48, NOSHIFT } /* '6' */,
			{ 0x07, NOSHIFT, 0x42, NOSHIFT } /* '7' */,
			{ 0x08, NOSHIFT, 0x43, NOSHIFT } /* '8' */,
			{ 0x09, NOSHIFT, 0x44, NOSHIFT } /* '9' */,
			{ 0x27, NOSHIFT, 0xff, 0xff    } /* ':' */,
			{ 0x26, NOSHIFT, 0xff, 0xff    } /* ';' */,
			{ 0x30, SHIFT,   0xff, 0xff    } /* '<' */,
			{ 0x0b, SHIFT,   0x4d, NOSHIFT } /* '=' */,
			{ 0x31, SHIFT,   0xff, 0xff    } /* '>' */,
			{ 0x32, SHIFT,   0xff, 0xff    } /* '?' */,
			{ 0x1a, NOSHIFT, 0xff, 0xff    } /* '@' */,
			{ 0x1d, CAPLKON, 0xff, 0xff    } /* 'A' */,
			{ 0x2d, CAPLKON, 0xff, 0xff    } /* 'B' */,
			{ 0x2b, CAPLKON, 0xff, 0xff    } /* 'C' */,
			{ 0x1f, CAPLKON, 0xff, 0xff    } /* 'D' */,
			{ 0x12, CAPLKON, 0xff, 0xff    } /* 'E' */,
			{ 0x20, CAPLKON, 0xff, 0xff    } /* 'F' */,
			{ 0x21, CAPLKON, 0xff, 0xff    } /* 'G' */,
			{ 0x22, CAPLKON, 0xff, 0xff    } /* 'H' */,
			{ 0x17, CAPLKON, 0xff, 0xff    } /* 'I' */,
			{ 0x23, CAPLKON, 0xff, 0xff    } /* 'J' */,
			{ 0x24, CAPLKON, 0xff, 0xff    } /* 'K' */,
			{ 0x25, CAPLKON, 0xff, 0xff    } /* 'L' */,
			{ 0x2f, CAPLKON, 0xff, 0xff    } /* 'M' */,
			{ 0x2e, CAPLKON, 0xff, 0xff    } /* 'N' */,
			{ 0x18, CAPLKON, 0xff, 0xff    } /* 'O' */,
			{ 0x19, CAPLKON, 0xff, 0xff    } /* 'P' */,
			{ 0x10, CAPLKON, 0xff, 0xff    } /* 'Q' */,
			{ 0x13, CAPLKON, 0xff, 0xff    } /* 'R' */,
			{ 0x1e, CAPLKON, 0xff, 0xff    } /* 'S' */,
			{ 0x14, CAPLKON, 0xff, 0xff    } /* 'T' */,
			{ 0x16, CAPLKON, 0xff, 0xff    } /* 'U' */,
			{ 0x2c, CAPLKON, 0xff, 0xff    } /* 'V' */,
			{ 0x11, CAPLKON, 0xff, 0xff    } /* 'W' */,
			{ 0x2a, CAPLKON, 0xff, 0xff    } /* 'X' */,
			{ 0x15, CAPLKON, 0xff, 0xff    } /* 'Y' */,
			{ 0x29, CAPLKON, 0xff, 0xff    } /* 'Z' */,
			{ 0x1b, NOSHIFT, 0xff, 0xff    } /* '[' */,
			{ 0x0d, NOSHIFT, 0xff, 0xff    } /* '\' */,
			{ 0x28, NOSHIFT, 0xff, 0xff    } /* ']' */,
			{ 0x0c, NOSHIFT, 0xff, 0xff    } /* '^' */,
			{ 0x33, SHIFT,   0xff, 0xff    } /* '_' */,
			{ 0x0c, SHIFT,   0xff, 0xff    } /* '`' */,
			{ 0x1d, CAPLKOF, 0xff, 0xff    } /* 'a' */,
			{ 0x2d, CAPLKOF, 0xff, 0xff    } /* 'b' */,
			{ 0x2b, CAPLKOF, 0xff, 0xff    } /* 'c' */,
			{ 0x1f, CAPLKOF, 0xff, 0xff    } /* 'd' */,
			{ 0x12, CAPLKOF, 0xff, 0xff    } /* 'e' */,
			{ 0x20, CAPLKOF, 0xff, 0xff    } /* 'f' */,
			{ 0x21, CAPLKOF, 0xff, 0xff    } /* 'g' */,
			{ 0x22, CAPLKOF, 0xff, 0xff    } /* 'h' */,
			{ 0x17, CAPLKOF, 0xff, 0xff    } /* 'i' */,
			{ 0x23, CAPLKOF, 0xff, 0xff    } /* 'j' */,
			{ 0x24, CAPLKOF, 0xff, 0xff    } /* 'k' */,
			{ 0x25, CAPLKOF, 0xff, 0xff    } /* 'l' */,
			{ 0x2f, CAPLKOF, 0xff, 0xff    } /* 'm' */,
			{ 0x2e, CAPLKOF, 0xff, 0xff    } /* 'n' */,
			{ 0x18, CAPLKOF, 0xff, 0xff    } /* 'o' */,
			{ 0x19, CAPLKOF, 0xff, 0xff    } /* 'p' */,
			{ 0x10, CAPLKOF, 0xff, 0xff    } /* 'q' */,
			{ 0x13, CAPLKOF, 0xff, 0xff    } /* 'r' */,
			{ 0x1e, CAPLKOF, 0xff, 0xff    } /* 's' */,
			{ 0x14, CAPLKOF, 0xff, 0xff    } /* 't' */,
			{ 0x16, CAPLKOF, 0xff, 0xff    } /* 'u' */,
			{ 0x2c, CAPLKOF, 0xff, 0xff    } /* 'v' */,
			{ 0x11, CAPLKOF, 0xff, 0xff    } /* 'w' */,
			{ 0x2a, CAPLKOF, 0xff, 0xff    } /* 'x' */,
			{ 0x15, CAPLKOF, 0xff, 0xff    } /* 'y' */,
			{ 0x29, CAPLKOF, 0xff, 0xff    } /* 'z' */,
			{ 0x1b, SHIFT,   0xff, 0xff    } /* '{' */,
			{ 0x0d, SHIFT,   0xff, 0xff    } /* '|' */,
			{ 0x28, SHIFT,   0xff, 0xff    } /* '}' */,
			{ 0x1a, SHIFT,   0xff, 0xff    } /* '~' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\x7f' */,
			{ 0x00, NOSHIFT, 0xff, 0xff    } /* Esc */,
			{ 0x62, NOSHIFT, 0xff, 0xff    } /* F1 */,
			{ 0x63, NOSHIFT, 0xff, 0xff    } /* F2 */,
			{ 0x64, NOSHIFT, 0xff, 0xff    } /* F3 */,
			{ 0x65, NOSHIFT, 0xff, 0xff    } /* F4 */,
			{ 0x66, NOSHIFT, 0xff, 0xff    } /* F5 */,
			{ 0x67, NOSHIFT, 0xff, 0xff    } /* F6 */,
			{ 0x68, NOSHIFT, 0xff, 0xff    } /* F7 */,
			{ 0x69, NOSHIFT, 0xff, 0xff    } /* F8 */,
			{ 0x6a, NOSHIFT, 0xff, 0xff    } /* F9 */,
			{ 0x6b, NOSHIFT, 0xff, 0xff    } /* F10 */,
			{ 0x52, NOSHIFT, 0xff, 0xff    } /* F11 */,
			{ 0x53, NOSHIFT, 0xff, 0xff    } /* F12 */,
			{ 0x54, NOSHIFT, 0xff, 0xff    } /* F13 */,
			{ 0x55, NOSHIFT, 0xff, 0xff    } /* F14 */,
			{ 0x56, NOSHIFT, 0xff, 0xff    } /* F15 */,
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
			{ 0xff, 0xff,    0xff, 0xff    } /* EXT1(F28) */,
			{ 0xff, 0xff,    0xff, 0xff    } /* EXT2(F29) */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\x9e' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\x9f' */,
			{ 0x1c, IGSHIFT, 0xff, 0xff    } /* Enter */,
			{ 0x0e, IGSHIFT, 0xff, 0xff    } /* BackSpace */,
			{ 0x0f, NOSHIFT, 0xff, 0xff    } /* Tab */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xa3' */,
			{ 0x38, NOSHIFT, 0xff, 0xff    } /* Insert */,
			{ 0x39, NOSHIFT, 0xff, 0xff    } /* Delete */,
			{ 0x3e, NOSHIFT, 0x5e, NOSHIFT } /* Home */,
			{ 0x3f, NOSHIFT, 0xff, 0xff    } /* End */,
			{ 0x37, NOSHIFT, 0xff, 0xff    } /* PageUp */,
			{ 0x36, NOSHIFT, 0xff, 0xff    } /* PageDown */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xaa' */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xab' */,
			{ 0x3b, NOSHIFT, 0xff, 0xff    } /* Left */,
			{ 0x3c, NOSHIFT, 0xff, 0xff    } /* Right */,
			{ 0x3a, NOSHIFT, 0xff, 0xff    } /* Up */,
			{ 0x3d, NOSHIFT, 0xff, 0xff    } /* Down */,
			{ 0xff, 0xff,    0xff, 0xff    } /* ScrollLock */,
			{ 0xff, 0xff,    0xff, 0xff    } /* NumLock */,
			{ 0x71, NOSHIFT, 0xff, 0xff    } /* CapsLock */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xb3' */,
			{ 0x70, 0xfe,    0xff, 0xff    } /* Shift */,
			{ 0x74, 0xfe,    0xff, 0xff    } /* Ctrl */,
			{ 0x73, 0xfe,    0x72, 0xfe    } /* Alt */,
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xb7' */,
			{ 0x61, NOSHIFT, 0xff, 0xff    } /* COPY(PrintScreen) */,
			{ 0xff, 0xff,    0xff, 0xff    } /* Pause */,
			{ 0x60, NOSHIFT, 0xff, 0xff    } /* STOP(Break) */,
			{ 0xff, 0xff,    0xff, 0xff    } /* SysRq */,
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
			{ 0xff, 0xff,    0xff, 0xff    } /* Zenkaku */,
			{ 0xff, 0xff,    0xff, 0xff    } /* Muhenkan */,
			{ 0xff, 0xff,    0xff, 0xff    } /* Henkan */,
			{ 0xff, 0xff,    0xff, 0xff    } /* Hiragana */,
			{ 0xff, 0xff,    0xff, 0xff    } /* Katakana */,
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
/* f8～ffは、マイナーシフトキー */
			{ 0xff, 0xff,    0xff, 0xff    } /* '\xf8' */,
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
	#if (defined(NEC98))
		static struct {
			int cmd, length;
			int deccmd[6 * 9 + 3];
			int eoc;
		} command = {
			0x0068, 59 * 4, {
				0x0110     /* clear */,
				0          /* opt */,

				0x010c     /* define */,
				4          /* opt(len) */,
				0x71       /* rawcode(CapsLock) */,
				0x0000c070 /* shiftmap */,
				0x0004     /* or bit */,
				0x00040000 /* cmd(or) */,

				0x010c     /* define */,
				4          /* opt(len) */,
				0x71       /* rawcode(CapsLock) */,
				0x4000c070 /* shiftmap */,
				~0x0004    /* and bit */,
				0x00050000 /* cmd(and) */,

				0x010c     /* define */,
				4          /* opt(len) */,
				0x70       /* rawcode(Shift) */,
				0x0000c000 /* shiftmap */,
				0x0010     /* or bit */,
				0x00040000 /* cmd(or) */,

				0x010c     /* define */,
				4          /* opt(len) */,
				0x70       /* rawcode(Shift) */,
				0x4000c000 /* shiftmap */,
				~0x0010    /* and bit */,
				0x00050000 /* cmd(and) */,

				0x010c     /* define */,
				4          /* opt(len) */,
				0x74       /* rawcode(Ctrl) */,
				0x0000c000 /* shiftmap */,
				0x0020     /* or bit */,
				0x00040000 /* cmd(or) */,

				0x010c     /* define */,
				4          /* opt(len) */,
				0x74       /* rawcode(Ctrl) */,
				0x4000c000 /* shiftmap */,
				~0x0020    /* and bit */,
				0x00050000 /* cmd(and) */,

				0x010c     /* define */,
				4          /* opt(len) */,
				0x73       /* rawcode(Alt) */,
				0x0000c000 /* shiftmap */,
				0x0040     /* or bit */,
				0x00040000 /* cmd(or) */,

				0x010c     /* define */,
				4          /* opt(len) */,
				0x73       /* rawcode(Alt) */,
				0x4000c000 /* shiftmap */,
				~0x0040    /* and bit */,
				0x00050000 /* cmd(and) */,

				0x010c     /* define */,
				4          /* opt(len) */,
				0x39       /* rawcode(Deletele) */,
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

#if (defined(TOWNS))

void job_savevram0()
{
    /* 保存先用意 */
    lib_initmodulehandle0(0x0008, 0x0200); /* user-dirに初期化 */
    job.func = &job_savevram1;
    lib_steppath0(0, 0x0200, "SCRNSHOT.TIF", 0x0050 /* sig */);
    return;
    /* SCRNSHOT.TIFは514KB以上になります */
    /* 事前にモジュールを作っておいて（resizeしなくてよい）ください */
}

void job_savevram1(int flag, int dmy)
{
    if (flag == 0) { /* open成功 */
		lib_resizemodule(0, 0x0200, x2 * y2 + 2048, 0x0050 /* sig */);
		job.func = &job_savevram2;
	} else {
		job.now = 0;
	}
	return;
}

static unsigned char tiffhead[] = {
	0x49,0x49,0x2A,0x00,0x08,0x00,0x00,0x00,0x11,0x00,0xFE,0x00,0x04,0x00,0x01,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x03,0x00,0x01,0x00,0x00,0x00,0x00,0x04,
	0x00,0x00,0x01,0x01,0x03,0x00,0x01,0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x02,0x01,
	0x03,0x00,0x01,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x03,0x01,0x03,0x00,0x01,0x00,
	0x00,0x00,0x01,0x00,0x00,0x00,0x06,0x01,0x03,0x00,0x01,0x00,0x00,0x00,0x03,0x00,
	0x00,0x00,0x0A,0x01,0x03,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x11,0x01,
	0x04,0x00,0x01,0x00,0x00,0x00,0x00,0x08,0x00,0x00,0x15,0x01,0x03,0x00,0x01,0x00,
	0x00,0x00,0x01,0x00,0x00,0x00,0x16,0x01,0x04,0x00,0x01,0x00,0x00,0x00,0x5E,0x02,
	0x00,0x00,0x17,0x01,0x04,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x08,0x00,0x19,0x01,
	0x03,0x00,0x01,0x00,0x00,0x00,0xFF,0x00,0x00,0x00,0x1A,0x01,0x05,0x00,0x01,0x00,
	0x00,0x00,0xF0,0x01,0x00,0x00,0x1B,0x01,0x05,0x00,0x01,0x00,0x00,0x00,0xF8,0x01,
	0x00,0x00,0x1C,0x01,0x03,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x40,0x01,
	0x03,0x00,0x00,0x03,0x00,0x00,0x00,0x02,0x00,0x00,0x31,0x01,0x02,0x00,0x1C,0x00,
	0x00,0x00,0x80,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x4F,0x53,0x41,0x53,0x4B,0x2F,0x54,0x4F,0x57,0x4E,0x53,0x20,0x67,0x65,0x6E,0x65,
	0x72,0x61,0x74,0x65,0x64,0x20,0x54,0x49,0x46,0x46,0x2E,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x4B,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x4B,0x00,0x00,0x00,0x01,0x00,0x00,0x00
};

void job_savevram2(int flag, int dmy)
{
	struct STR_PALETTE {
		unsigned char r, g, b;
	};
	static struct STR_PALETTE palette[16] = {
		{ 0x00, 0x00, 0x00 }, { 0x84, 0x00, 0x00 },
		{ 0x00, 0x84, 0x00 }, { 0x84, 0x84, 0x00 },
		{ 0x00, 0x00, 0x84 }, { 0x84, 0x00, 0x84 },
		{ 0x00, 0x84, 0x84 }, { 0x84, 0x84, 0x84 },
		{ 0xc6, 0xc6, 0xc6 }, { 0xff, 0x00, 0x00 },
		{ 0x00, 0xff, 0x00 }, { 0xff, 0xff, 0x00 },
		{ 0x00, 0x00, 0xff }, { 0xff, 0x00, 0xff },
		{ 0x00, 0xff, 0xff }, { 0xff, 0xff, 0xff }
	};
	int i;

	if (flag == 0) { /* resize 成功 */
		if (lib_readmodulesize(0x0200) >= x2 * y2 + 2048) {
			char *fp = (char *) job.readCSd10;
			struct STR_PALETTE *pal = palette;
			lib_mapmodule(0x0000, 0x0200, 0x7, 514 * 1024, fp, 0);
			for (i = 0; i < 512 / 4; i++)
				 ((int *) fp)[i] = ((int *) tiffhead)[i];
			*(int *) &fp[0x1e] = x2;
			*(int *) &fp[0x2a] = y2;
			*(int *) &fp[0x8a] = x2 * y2;
			do {
				((int *) fp)[i] = 0;
				i++;
			} while (i < 2048 / 4);
			for (i = 0; i < 16; i++, pal++) {
				fp[i * 2 + 0x0200 + 0] = pal->r;
				fp[i * 2 + 0x0200 + 1] = pal->r;
				fp[i * 2 + 0x0400 + 0] = pal->g;
				fp[i * 2 + 0x0400 + 1] = pal->g;
				fp[i * 2 + 0x0600 + 0] = pal->b;
				fp[i * 2 + 0x0600 + 1] = pal->b;
			}
			sgg_debug00(0, x2*y2, 0, 0xe0000000, 0x01280008,
				(const int) fp + 2048, 0x000c);
            lib_unmapmodule(0, 514 * 1024, fp);
            send_signal3dw(0x4000 /* pokon0 */ | 0x240, 0x7f000002,
			   0x008c /* SIGNAL_FREE_FILES */, 0x3000 /* winman0 */);
        }
    }
    job.now = 0;
    return;
}

#if 0

void job_savevram0()
{
    /* 保存先用意 */
    lib_initmodulehandle0(0x0008, 0x0200); /* user-dirに初期化 */
    job.func = &job_savevram1;
    lib_steppath0(0, 0x0200, "VRAMIMAG.BIN", 0x0050 /* sig */);
    return;
    /* VRAMIMAG.BINは512KB以上のサイズのファイル */
    /* 事前にcreateしてresizeしておいてください */
}

void job_savevram1(int flag, int dmy)
{
    if (flag == 0) { /* open成功 */
        if (lib_readmodulesize(0x0200) >= 512 * 1024) {
            char *fp = (char *) job.readCSd10;
            lib_mapmodule(0x0000, 0x0200, 0x7, 512 * 1024, fp, 0);
            sgg_debug00(0, 512 * 1024, 0, 0xe0000000, 0x01280008,
			(const int) fp, 0x000c);
            lib_unmapmodule(0, 512 * 1024, fp);
            send_signal3dw(0x4000 /* pokon0 */ | 0x240, 0x7f000002,
			   0x008c /* SIGNAL_FREE_FILES */, 0x3000 /* winman0 */);
        }
    }
    job.now = 0;
    return;
}

#endif

#endif

void job_openwallpaper()
/* 2002.05.27 川合 : 壁紙表示がトグルになるように変更 */
{
	if (wallpaper_exist == 0) {
		lib_initmodulehandle0(0x0008, 0x0200);
		job.func = &job_loadwallpaper;
		lib_steppath0(0, 0x0200, "OSASK   .BMP", 0x0050 /* sig */);
		return;
		/* OSASK.BMP is wallpaper. */
	}
	job_loadwallpaper(1, 0);
	return;
}

int bmp2beta(unsigned char *s, char *d, int w,int h, int ms);

void job_loadwallpaper(int flag, int dmy)
/* 2002.05.27 川合 : わずかに改良 */
{
	struct WM0_WINDOW *win;
	int i;

	wallpaper_exist = 0;
	if (flag == 0) {	/* opening succeeded. */
		char *fp = (char *)lib_readCSd(0x10);
		for (i = 0; i < WALLPAPERMAXSIZE / 4; i++)
			*((int *) wallpaper + i) = BACKCOLOR | BACKCOLOR << 8
				| BACKCOLOR << 16 | BACKCOLOR << 24;	/* ゴミが出るので初期化しておく */
		lib_mapmodule(0x0000, 0x0200, 0x5, 768*1024, fp, 0);
		wallpaper_exist = 
			bmp2beta(fp, wallpaper, x2, y2, lib_readmodulesize(0x0200));
		lib_unmapmodule(0, 768 * 1024, fp);
		send_signal3dw(0x4000 /* pokon0 */ | 0x240, 0x7f000002,
			0x008c /* SIGNAL_FREE_FILES */, 0x3000 /* winman0 */);
	}

	/* 全ウィンドウ再描画 */
	if (mx != 0x80000000) {
		sgg_wm0_removemouse();
	} else
		mx = my = 1;

	init_screen(x2, y2);
	if (win = top) {
		do {
			win->job_flag0 |= WINFLG_MUSTREDRAW;
		} while ((win = win->down) != top);
	}
	job_general1();

//	job.now = 0; /* これはjob_general1()に含まれる */
	return;
}

void putwallpaper(int x0, int y0, int x1, int y1)
/* 2002.05.27 川合 : x1, y1の値の扱いを仕様変更 */
{
	if (wallpaper_exist)
		lib_putblock1((void *) -1, x0, y0, x1 - x0, y1 - y0,
			x2 - (x1 - x0), &wallpaper[x2 * y0 + x0]);
	else
		lib_drawline(0x0020, (void *) -1, BACKCOLOR, x0, y0, x1 - 1, y1 - 1);
	return;
}

int bmp2beta(unsigned char *src, char *dest, int bw, int bh, int mappedsize)
/* 2002.05.27 川合 : うまくいったら非零を返すように仕様変更 */
/*		ついでにセンタリングもつけた */
{
	int bmpW, bmpH, bpp, i;

	/* Minimal size check */
	if (mappedsize < 28)				/* OS2-BMP's header size */
		goto error;

	/* Header check and MAC */
	if (*(short *) src != 0x4d42) {		/* "BM" */
		mappedsize += -128;				/* for MAC BMP */
		if (mappedsize < 28)
			goto error;
		src += 128;
		if (*(short *) src != 0x4d42)
			goto error;
	}

	/* Is module lacked? */
	if (mappedsize < *(int *) (src+2))
		goto error;

	/* OS/2 and MS formats */
	i = *(int *) (src+14);
	if (i == 40 && *(int *) (src+30) != 0)	/* compressed */
		goto error;

	if (i != 40 && i != 12)
		goto error;

	bmpW = *(int *) (src+18);
	bmpH = *(int *) (src+22);
	bpp = *(short *) (src+28);
	src += *(int *) (src+10);

	/* センタリング */
	if (bmpW < bw)
		dest += (bw - bmpW) >> 1;
	if (bmpH < bh - (RESERVELINE0 + RESERVELINE1))
		dest += bw * (RESERVELINE0 + (bh - bmpH - (RESERVELINE0 + RESERVELINE1)) >> 1);

	if (bpp == 1) {
		int lw,y;

		bmpW = (bmpW + 31) & ~31;
		lw = ((bw < bmpW) ? bw : bmpW) / 8; /* limit loop count */
		bmpW /= 8;
		src += (bmpH - 1) * bmpW;

		for (y = (bh < bmpH) ? bh : bmpH; y > 0; y--) {
			int x;
			for (x = 0; x < lw; x++) {
				unsigned char t = src[x];

				dest[x * 8 + 0] = ((t >> 7) & 0x01) + 7;
				dest[x * 8 + 1] = ((t >> 6) & 0x01) + 7;
				dest[x * 8 + 2] = ((t >> 5) & 0x01) + 7;
				dest[x * 8 + 3] = ((t >> 4) & 0x01) + 7;
				dest[x * 8 + 4] = ((t >> 3) & 0x01) + 7;
				dest[x * 8 + 5] = ((t >> 2) & 0x01) + 7;
				dest[x * 8 + 6] = ((t >> 1) & 0x01) + 7;
				dest[x * 8 + 7] = ( t       & 0x01) + 7;
			}
			src -= bmpW; dest += bw;
		}
	} else {
		int lw, y;

		bmpW = (bmpW + 7) & ~7;
		lw = ((bw < bmpW) ? bw : bmpW) / 8; /* limit loop count */
		bmpW /= 2;
		src += (bmpH - 1) * bmpW;

		for (y = (bh < bmpH) ? bh : bmpH; y > 0; y--) {
			int x;
			for (x = 0; x < lw; x++) {
				unsigned int t = *((int *) src + x);

				dest[x * 8 + 0] = (t >>  4) & 0x0f;
				dest[x * 8 + 1] =  t        & 0x0f;
				dest[x * 8 + 2] = (t >> 12) & 0x0f;
				dest[x * 8 + 3] = (t >>  8) & 0x0f;
				dest[x * 8 + 4] = (t >> 20) & 0x0f;
				dest[x * 8 + 5] = (t >> 16) & 0x0f;
				dest[x * 8 + 6] = (t >> 28) & 0x0f;
				dest[x * 8 + 7] = (t >> 24) & 0x0f;
			}
			src -= bmpW; dest += bw;
		}
	}
	return 1;
error:
	return 0;
}
