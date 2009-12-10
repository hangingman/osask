// "pokon0.c":アプリケーションラウンチャー  ver.0.8
//   copyright(C) 2000 川合秀実, 小柳雅明
//  exe2bin0 pokon0 -s 20k

#include <guigui00.h>
#include <sysgg00.h>
// sysggは、一般のアプリが利用してはいけないライブラリ
// 仕様もかなり流動的
#include <stdlib.h>

#define	AUTO_MALLOC	0
#define LIST_HEIGHT	8
#define ext_EXE		('E' | ('X' << 8) | ('E' << 16))
#define ext_BIN		('B' | ('I' << 8) | ('N' << 16))

struct FILELIST {
	char name[11];
	struct SGG_FILELIST *ptr;
};

unsigned int counter = 0, banklist[8];
struct SGG_FILELIST *file;
struct FILELIST *list, *lp;

struct LIB_TEXTBOX *selector;

void put_file(const char *name, const int pos, const int col)
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
			lib_putstring_ASCII(0x0001, 0, pos, selector, 15,  2, charbuf16);
		else
			lib_putstring_ASCII(0x0001, 0, pos, selector,  0, 15, charbuf16);
	} else
		lib_putstring_ASCII(0x0000, 0, pos, selector, 0, 0, "                ");
	return;
}

const int list_set(const int ext)
{
	int i;
	struct SGG_FILELIST *fp;
	struct FILELIST *wp1, *wp2, tmp;

	// システムにファイルのリストを要求
	sgg_getfilelist(256, file, 0, 0);

	// アプリケーションがLIST_HEIGHT個以下のときのための処置
	for (i = 0; i <= LIST_HEIGHT; i++)
		list[i].name[0] = '\0';

	// 拡張子extのものだけを抽出してlistへ
	fp = file + 1; // 最初の要素はダミー
	lp = list;

	while (fp->name[0]) {
		if ((fp->name[8] | (fp->name[9] << 8) | (fp->name[10] << 16)) == ext) {
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
	lp = list;
	for (i = 0; i < LIST_HEIGHT; i++)
 		put_file(lp[i].name, i, 0);

	if (list[0].name[0] == '\0') {
		// 選択可能なファイルが１つもない
		lib_putstring_ASCII(0x0000, 0, LIST_HEIGHT / 2 - 1, selector, 1, 0, "File not found! ");
		return -1;
	}
 	put_file(lp[0].name, 0, 1);
	return 0;
}

int *sb0, *sbp;

void wait99sub()
{
	if (*sbp == 0)
		lib_waitsignal(0x0001, 0, 0);
	else if (*sbp == 1) {
		lib_waitsignal(0x0000, *(sbp + 1), 0);
		sbp = sb0;
	} else if (*sbp == 0x0080) {
		// タスク終了
		int i;
		for (i = 0; banklist[i] != sbp[1]; i++);
		banklist[i] = 0;
		sgg_freememory(0x0220 /* "empty00" */ + i * 16);
		lib_waitsignal(0x0000, 2, 0);
		sbp += 2;
	} else {
		lib_waitsignal(0x0000, 1, 0);
		sbp++;
	}
	return;
}

void wait99()
{
	while (*sbp != 99)
		wait99sub();
	lib_waitsignal(0x0000, 1, 0); // 99の分を処理
	sbp++;
	return;
}

void main()
{
	struct LIB_WINDOW *window;
	struct LIB_TEXTBOX *wintitle, *mode;
	char charbuf16[17];
	int cur, i, sig, bank, fmode = 0, ext;
	struct SGG_FILELIST *fp;

	lib_init(AUTO_MALLOC);
        sgg_init(AUTO_MALLOC);

	sbp = sb0 = lib_opensignalbox(256, AUTO_MALLOC, 0, 1);

	file = (struct SGG_FILELIST *) malloc(256 * sizeof (struct SGG_FILELIST));
	list = (struct FILELIST *) malloc(256 * sizeof (struct FILELIST));

	window = lib_openwindow(AUTO_MALLOC, 0x0200, 160, 40 + LIST_HEIGHT * 16);
	wintitle = lib_opentextbox(0x1000, AUTO_MALLOC,  0,  7, 1,  0,  0, window, 0x00c0, 0);
	mode     = lib_opentextbox(0x0000, AUTO_MALLOC,  0, 20, 1,  0,  0, window, 0x00c0, 0); // 256bytes
	selector = lib_opentextbox(0x0001, AUTO_MALLOC, 15, 16, 8, 16, 32, window, 0x00c0, 0); // 1.1KB

	lib_putstring_ASCII(0x0000, 0, 0, wintitle, 0, 0, "pokon08");

	// キー操作を登録
	lib_definesignal1p0(1, 0x0100, 0x00ae /* cursor-up */,   window,  4);
//	lib_definesignal1p0(0, 0x0100, 0x00af /* cursor-down */, window,  5);
	lib_definesignal1p0(0, 0x0100, 0x00a0 /* Enter */,       window,  6);
	lib_definesignal1p0(0, 0x0100, 'F',                      window,  7);
	lib_definesignal1p0(0, 0x0100, 'f',                      window,  7);
	lib_definesignal1p0(0, 0x0100, 'R',                      window,  8);
	lib_definesignal1p0(0, 0x0100, 'r',                      window,  8);
	lib_definesignal1p0(0, 0x0100, 'S',                      window,  9);
	lib_definesignal1p0(0, 0x0100, 's',                      window,  9);
	lib_definesignal1p0(1, 0x0100, 0x00a8 /* page-up */,     window, 10);
//	lib_definesignal1p0(0, 0x0100, 0x00a9 /* page-down */,   window, 11);
	lib_definesignal1p0(1, 0x0100, 0x00ac /* cursor-left */, window, 10);
//	lib_definesignal1p0(0, 0x0100, 0x00ad /* cursor-right */,window, 11);
	lib_definesignal1p0(1, 0x0100, 0x00a6 /* Home */,        window, 12);
//	lib_definesignal1p0(0, 0x0100, 0x00a7 /* End */,         window, 13);
	lib_definesignal1p0(0, 0x0100, 0x00a4 /* Insert */,      window, 14);
	lib_definesignal0p0(0, 0, 0, 0);

	lib_putstring_ASCII(0x0000, 0, 0, mode,     0, 0, "< Run Application > ");
	cur = list_set(ext = ext_BIN);

	for (i = 0; i < 8; i++)
		banklist[i] = 0;

	for (;;) {
		do {
			sig = *sbp;
			wait99sub();
		} while (sig == 0);
  
		switch (sig) {

		case 4 /* cursor-up */:
			if (cur < 0 /* ファイルが１つもない */)
				break;
			if (cur > 0) {
 				put_file(lp[cur].name, cur, 0);
				cur--;
 				put_file(lp[cur].name, cur, 1);
			} else if (lp > list) {
				lp--;
listup:
				for (i = 0; i < LIST_HEIGHT; i++) {
					if (i != cur)
						put_file(lp[i].name, i, 0);
					else
						put_file(lp[cur].name, cur, 1);
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
 					put_file(lp[cur].name, cur, 0);
					cur++;
 					put_file(lp[cur].name, cur, 1);
				} else {
					lp++;
					goto listup;
				}
			}
			break;

		case 6 /* Enter */:
			if (cur < 0 /* ファイルが1つもない */)
				break;
			if (ext == ext_BIN) {
				// .BINファイルモード
				for (bank = 2; bank < 8; bank++) {
					if (banklist[bank] == 0)
						goto find_freebank;
				}
				if (banklist[0] == 0)
					bank = 0;
				else if (banklist[1] == 0)
					bank = 1;
				else
					break;
find_freebank:
				sgg_loadfile(0x0220 /* load to "empty00" */ + bank * 16,
					lp[cur].ptr->id /* file id */,
					99 /* finish signal */
				);
				wait99(); // finish signalが来るまで待つ

				sgg_createtask(0x0220 /* "empty00" */ + bank * 16, 99 /* finish signal */);
				wait99(); // finish signalが来るまで待つ

				banklist[bank] = i = *sbp++;
				lib_waitsignal(0x0000, 1, 0);

				// 適正なGUIGUI00ファイルでない場合、タスクは生成されず、iは0。
				if (i) {
					sgg_settasklocallevel(i,
						1 * 32 /* local level 1 (起動・システム処理レベル) */,
						16 * 64 /* gloval level 16 (一般アプリケーション) */,
						 2 /* Inner level */
					);
					sgg_settasklocallevel(i,
						2 * 32 /* local level 2 (通常処理レベル) */,
						16 * 64 /* gloval level 16 (一般アプリケーション) */,
						 2 /* Inner level */
					);
					sgg_runtask(i, 1 * 32);
				} else {
					// ロードした領域を解放
					sgg_freememory(0x0220 /* "empty00" */ + bank * 16);
				}
			} else if (/* ext == ext_EXE && */ (banklist[0] | banklist[1]) == 0) {
				// .EXEファイルモード
				sgg_loadfile(0x0220 /* load to "empty00" */,
					lp[cur].ptr->id /* file id */,
					99 /* finish signal */
				);
				wait99(); // finish signalが来るまで待つ

				i = ('K' | ('B' << 8) | ('S' << 16) | ('1' << 24));
				if (fmode)
					i = ('K' | ('B' << 8) | ('S' << 16) | ('0' << 24));

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
				if (fp->name[0]) {
					sgg_loadfile(0x0230 /* load to "empty01" */,
						i /* file id */,
						99 /* finish signal */
					);
				} else {
					// "OSASKBS0.BIN"が見つからなかった
					break;
				}
				wait99(); // finish signalが来るまで待つ

				for (i = 0; i < LIST_HEIGHT; i++)
					lib_putstring_ASCII(0x0000, 0, i, selector, 0, 0, "                ");
				lib_putstring_ASCII(0x0000, 0, 1, selector, 0, 0, "    Loaded.     ");
				lib_putstring_ASCII(0x0000, 0, 3, selector, 0, 0, " Change disks.  ");
				lib_putstring_ASCII(0x0000, 0, 5, selector, 0, 0, " Hit Enter key. ");

				// 6, 7, 8, 9を待つ
				while (*sbp != 6 && *sbp != 7 && *sbp != 8 && *sbp != 9)
					wait99sub();
				sig = *sbp++;
				lib_waitsignal(0x0000, 1, 0);

				if (sig == 7)
					goto signal_7;
				if (sig == 8)
					goto signal_8;

				lib_putstring_ASCII(0x0000, 0, 5, selector, 0, 0, "  Please wait.  ");
				if (sig == 6) {
					lib_putstring_ASCII(0x0000, 0, 1, selector, 0, 0, "  Formating...  ");
					lib_putstring_ASCII(0x0000, 0, 3, selector, 0, 0, "                ");
				//	lib_putstring_ASCII(0x0000, 0, 5, selector, 0, 0, "  Please wait.  ");
					i = 0x0124;
					if (fmode)
						i = 0x0118;
					sgg_format(i, 99 /* finish signal */); // format
					wait99(); // finish signalが来るまで待つ
				}
				lib_putstring_ASCII(0x0000, 0, 1, selector, 0, 0, " Writing        ");
				lib_putstring_ASCII(0x0000, 0, 3, selector, 0, 0, "   system image.");
			//	lib_putstring_ASCII(0x0000, 0, 5, selector, 0, 0, "  Please wait.  ");
				i = 0x0128;
				if (fmode)
					i = 0x011c;
				sgg_format(i, 99 /* finish signal */); // store system image
				wait99(); // finish signalが来るまで待つ

				// ロードした領域を解放
				sgg_freememory(0x0220 /* "empty00" */);
				sgg_freememory(0x0230 /* "empty01" */);

				lib_putstring_ASCII(0x0000, 0, 1, selector, 0, 0, "   Completed.   ");
				lib_putstring_ASCII(0x0000, 0, 3, selector, 0, 0, " Change disks.  ");
				lib_putstring_ASCII(0x0000, 0, 5, selector, 0, 0, "  Hit 'R' key.  ");

				// 7, 8を待つ
				while (*sbp != 7 && *sbp != 8)
					wait99sub();
				sig = *sbp++;
				lib_waitsignal(0x0000, 1, 0);

				sgg_format(0x0114, 99 /* finish signal */); // flush diskcache
				wait99(); // finish signalが来るまで待つ

				if (sig == 7)
					goto signal_7;
			//	if (sig == 8)
					goto signal_8;
			}
			break;

		case 7 /* to format-mode */:
		signal_7:
			lib_putstring_ASCII(0x0000, 0, 0, mode, fmode, 0, "< Load Systemimage >");
			cur = list_set(ext = ext_EXE);
			break;

		case 8 /* to run-mode */:
		signal_8:
			lib_putstring_ASCII(0x0000, 0, 0, mode,     0, 0, "< Run Application > ");
			cur = list_set(ext = ext_BIN);
			break;

		case 9 /* change format-mode */:
			if (ext == ext_EXE) {
				fmode ^= 0x01;
				lib_putstring_ASCII(0x0000, 0, 0, mode, fmode * 9, 0, "< Load Systemimage >");
			}
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
			sgg_format(0x0114, 99 /* finish signal */); // flush diskcache
			wait99(); // finish signalが来るまで待つ
			cur = list_set(ext);
			break;

//		case 99:
//			lib_putstring_ASCII(0x0000, 0, 0, mode,     0, 0, "< Error 99        > ");
//			sbp++;
//			break;
		}
	}
}
