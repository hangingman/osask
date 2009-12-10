/* "pokon0.c":アプリケーションラウンチャー  ver.2.5c
     copyright(C) 2002 小柳雅明, 川合秀実
    stack:4k malloc:84k file:1024k */

#include <guigui00.h>
#include <sysgg00.h>
/* sysggは、一般のアプリが利用してはいけないライブラリ
   仕様もかなり流動的 */
#include <stdlib.h>

#include "pokon0.h"

#define POKON_VERSION "pokon25"

#define POKO_VERSION "Heppoko-shell \"poko\" version 2.0\n    Copyright (C) 2002 OSASK Project\n"
#define POKO_PROMPT "\npoko>"

#define	FILEAREA		(1024 * 1024)

//static int MALLOC_ADDR;
#define MALLOC_ADDR			j
#define malloc(bytes)		(void *) (MALLOC_ADDR -= ((bytes) + 7) & ~7)
#define free(addr)			for (;;); /* freeがあっては困るので永久ループ */

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
static struct STR_VIEWER TXTEDIT = { "TEDITC01BIN", 2, 0x7f000001, 42, 0 };
static struct STR_VIEWER PICEDIT = { "BMPV06  BIN", 0, 0, 0, 0 };
static struct STR_VIEWER RESIZER = { "RESIZER0BIN", 0, 0, 0, 0 };
static struct STR_VIEWER HELPLAY = { "HELO    BIN", 0, 0, 0, 0 };
static struct STR_VIEWER MMLPLAY = { "MMLPLAY BIN", 0, 0, 0, 0 };
static struct STR_VIEWER MCOPY   = { "MCOPYC1 BIN", 0, 0, 0, 0 };
static struct STR_VIEWER CMPTEK0 = { "CMPTEK0 BIN", 0, 0, 0, 0 };

struct STR_BANK *banklist;
struct SGG_FILELIST *file;
struct FILEBUF *filebuf;
static int defaultIL = 2;
struct FILESELWIN *selwin0;
unsigned char *pselwincount;
struct VIRTUAL_MODULE_REFERENCE *vmref0;
int need_wb, readCSd10;
int sort_mode = DEFAULT_SORT_MODE;
signed char *pfmode;
signed char auto_decomp = 1;

//static struct STR_CONSOLE console = { -1, 0, 0, NULL, NULL, NULL, NULL, 0, 0, 0 };
static struct STR_CONSOLE *console;

int writejob_np(int n, int *p)
{
	struct STR_JOBLIST *pjob = &job;
	if (pjob->free >= n) {
		do {
			*(pjob->wp)++ = *p++;
			pjob->free--;
			if (pjob->wp == pjob->list + JOBLIST_SIZE)
				pjob->wp = pjob->list;
		} while (--n);
		*(pjob->wp) = 0; /* ストッパー */
		return 1;
	}
	return 0;
}

int writejob_n(int n, int p0, ...)
{
	return writejob_np(n, &p0);
}

int writejob_1(int p0)
{
	return writejob_n(1, p0);
}

int writejob_3p(int *p)
{
	return writejob_np(3, p);
}

const int readjob()
{
	struct STR_JOBLIST *pjob = &job;
	int i = *(pjob->rp)++;
	pjob->free++;
	if (pjob->rp == pjob->list + JOBLIST_SIZE)
		pjob->rp = pjob->list;
	return i;
}

#define	sgg_debug00(opt, bytes, reserve, src_ofs, src_sel, dest_ofs, dest_sel) \
	sgg_execcmd0(0x8010, (int) (opt), (int) (bytes), (int) (reserve), \
	(int) (src_ofs), (int) (src_sel), (int) (dest_ofs), (int) (dest_sel), \
	0x0000)

void sendsignal1dw(int task, int sig)
{
	sgg_execcmd0(0x0020, 0x80000000 + 3, task | 0x0242, 0x7f000001, sig, 0x0000);
	return;
}

void unlinkfbuf(struct FILEBUF *fbuf)
{
	int dirslot;
	if (--fbuf->linkcount == 0) {
		dirslot = fbuf->dirslot;
		if (fbuf->readonly)
			dirslot = -1;
		if (fbuf->size > 0) {
			if (dirslot == -1)
				sgg_execcmd0(0x0020, 0x80000000 + 5, 0x1244, 0x0134, -1, fbuf->size, fbuf->paddr, 0x0000);
			else if (writejob_n(4, JOB_FREE_MEMORY, dirslot, fbuf->size, fbuf->paddr) == 0) {
				/* どうすりゃいいんだ？ */
			}
		}
		fbuf->dirslot = -1;
		sgg_execcmd0(0x0074, 0, fbuf->virtualmodule, 0x0000);
	}
	return;
}

struct FILEBUF *searchfbuf(struct SGG_FILELIST *fp);
struct FILEBUF *check_wb_cache(struct FILEBUF *fbuf);
struct STR_BANK *run_viewer(struct STR_VIEWER *viewer, struct SGG_FILELIST *fp2);
struct SGG_FILELIST *searchfid1(const unsigned char *s);
struct FILEBUF *searchfrefbuf();

struct STR_BANK *searchfrebank()
{
	struct STR_BANK *bank = banklist;
	int i;

	for (i = 0; i < MAX_BANK; i++, bank++) {
		if (bank->tss == 0) {
			bank->tss = -1; /* リザーブマーク */
			return bank;
		}
	}
	return NULL;
}

struct SGG_FILELIST *searchfid(const unsigned char *s)
{
	char str[11];
	int i;
	for (i = 0; i < 11; i++) {
		if (i == 8)
			s++;
		str[i] = *s++;
	}
	return searchfid1(str);
}

struct SGG_FILELIST *searchfid1(const unsigned char *s)
{
	struct SGG_FILELIST *fp;
	unsigned char c;
	int i;

	for (fp = file + 1; fp->name[0]; fp++) {
		c = 0;
		for (i = 0; i < 11; i++)
			c |= s[i] - fp->name[i];
		if (c == 0)
			return fp;
	}
	return NULL;
}

void putselector0(struct FILESELWIN *win, const int pos, const char *str)
{
	lib_putstring_ASCII(0x0000, 0, pos, &win->selector.tbox, 0, 0, str);
	return;
}

void jsub_fbufready0(void *func);
int jsub_fbufready1(int *sbp);
void jsub_create_task0();
int jsub_create_task1(int *sbp);
void job_execute_file0(int cond);
void job_execute_file1(int cond);
void job_view_file0(int cond);
void job_view_file1(int cond);
void job_create_sysdisk0(int cond);
void job_create_sysdisk1(int cond);
void job_load_file0(int cond);
int job_resize_sub0(int *sbp);
void job_resize_sub1(int cond);
void job_resize_sub2(int cond);

#define	ppj(member)		(PPJ_ ## member)

void autoreadjob(int i, ...)
{
	int *p = &i;
	for (;;) {
		i = *p++;
		if (i == 0)
			return;
		((int *) &job)[i] = readjob();
	}
}

void runjobnext()
{
	struct STR_JOBLIST *pjob = &job;
	struct FILEBUF *fbuf;
	int i, j;
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
	job_end:
			pjob->now = 0;
			break;

		case JOB_LOAD_FILE_AND_EXECUTE:
			autoreadjob(ppj(fp), ppj(fbuf), ppj(bank), 0);
		//	pjob->retfunc = job_execute_file0;
		//	pjob->retfunc = job_view_file0;
			pjob->param[7] = 0;
			jsub_fbufready0(job_view_file0);
			break;

		case JOB_CREATE_TASK:
			pjob->bank = (struct STR_BANK *) readjob(); /* bank */
			for (i = 0; i < 8; i++)
				pjob->param[i] = readjob();
			jsub_create_task0();
			break;

		case JOB_LOAD_FILE:
			autoreadjob(ppj(fbuf), ppj(win), ppj(prm0) /* tss */, ppj(fp), 0);
		//	pjob->retfunc = job_load_file0;
			jsub_fbufready0(job_load_file0);
			break;

		case JOB_LOAD_FILE_AND_FORMAT:
			autoreadjob(ppj(fp), ppj(fbuf), ppj(prm1) /* fbuf */, 0);
			pjob->param[0] = (int) pjob->fbuf;
		//	pjob->retfunc = job_create_sysdisk0;
			pjob->win = &selwin0[0];
			jsub_fbufready0(job_create_sysdisk0);
			break;

		case JOB_VIEW_FILE:
			autoreadjob(ppj(fp), ppj(fbuf), ppj(bank), 0);
			for (i = 0; i < 6; i++)
				pjob->param[i] = readjob();
		//	pjob->retfunc = job_view_file0;
			pjob->param[7] = 1;
			jsub_fbufready0(job_view_file0);
			break;

		case JOB_CHECK_WB_CACHE:
			if (check_wb_cache(filebuf) == NULL) /* 終了したらNULLを返す */
				goto job_end;
			break;

		case JOB_WRITEBACK_CACHE:
			sgg_execcmd0(0x0020, 0x80000000 + 6, 0x1245, 0x0140, 0x4243 /* to pokon0 */, 0x7f000002, SIGNAL_NO_WB_CACHE, 0, 0x0000);
			break;

		case JOB_INVALID_WB_CACHE:
			sgg_execcmd0(0x0020, 0x80000000 + 6, 0x1245, 0x0144, 0x4243 /* to pokon0 */, 0x7f000002, SIGNAL_NO_WB_CACHE, 0, 0x0000);
			break;

		case JOB_FREE_MEMORY:
			autoreadjob(ppj(prm0), ppj(prm1), ppj(prm2), 0); /* fileid, size, addr */
			sgg_execcmd0(0x0020, 0x80000000 + 5, 0x1244, 0x0134, pjob->param[0], pjob->param[1], pjob->param[2], 0x0000);
			goto job_end;

		case JOB_CREATE_FILE:
			autoreadjob(ppj(prm0), ppj(prm1), ppj(prm2), ppj(prm6), ppj(prm7), 0);
				/* filename(0-2), task, signal */
			if (searchfid((char *) pjob->param) == NULL) {
				sgg_execcmd0(0x0020, 0x80000000 + 9, 0x1248, 0x0148, pjob->param[0], pjob->param[1], pjob->param[2] >> 8,
					0x4243 /* to pokon0 */, 0x7f000002, SIGNAL_REFRESH_FLIST, 0, 0x0000);
				break;
			}
	errorsignal:
			if (pjob->param[6])
				sendsignal1dw(pjob->param[6], pjob->param[7] + 1);
			goto job_end;

		case JOB_RENAME_FILE:
			for (i = 0; i < 8; i++)
				pjob->param[i] = readjob(); /* filename0(0-2), filename1(3-5), task, signal */
			if (searchfid((char *) &pjob->param[0]) == NULL)
				goto errorsignal;
			if (searchfid((char *) &pjob->param[3]) != NULL)
				goto errorsignal;
			sgg_execcmd0(0x0020, 0x80000000 + 12, 0x124b, 0x014c,
				pjob->param[0], pjob->param[1], pjob->param[2] >> 8,
				pjob->param[3], pjob->param[4], pjob->param[5] >> 8,
				0x4243 /* to pokon0 */, 0x7f000002, SIGNAL_REFRESH_FLIST, 0, 0x0000);
			break;

		case JOB_RESIZE_FILE:
			/* 2002.05.28 サイズ0のファイルを単純にリサイズするとごみを拾って混乱するバグを克服するために、
				2段階のサイズ変更を実施。0→1→目的サイズ。しかも1のときバイトを0xccにする。 */
			autoreadjob(ppj(prm0), ppj(prm1), ppj(prm2), ppj(prm3), ppj(prm4), ppj(prm6), ppj(prm7), 0);
				/* filename(0-2), new-size, max-linkcount, task, signal */
			if ((pjob->fp = searchfid((char *) &pjob->param[0])) == NULL)
				goto errorsignal;
			fbuf = searchfbuf(pjob->fp);
			j = pjob->param[3];
			i = 0;
			if (fbuf) {
				i = fbuf->linkcount;
				if (fbuf->readonly != 0 && j > 0)
					goto errorsignal;
			}
			if (i > pjob->param[4])
				goto errorsignal;
			if (j > 0 && fbuf == NULL) {
				pjob->fbuf = searchfrefbuf();
				jsub_fbufready0(job_resize_sub2);
				break;
			}
			i = SIGNAL_REFRESH_FLIST0;
			if (pjob->fp->size == 0) {
				i = SIGNAL_JSUB;
				j = 1;
			}
			pjob->jsubfunc = job_resize_sub0;
			sgg_execcmd0(0x0020, 0x80000000 + 10, 0x1249, 0x0150,
				pjob->param[0], pjob->param[1], pjob->param[2] >> 8, j, 
				0x4243 /* to pokon0 */, 0x7f000002, i, 1 /* 正常終了が-1だから */, 0x0000);
			break;

		case JOB_DELETE_FILE:
			autoreadjob(ppj(prm0), ppj(prm1), ppj(prm2), ppj(prm4), ppj(prm6), ppj(prm7), 0);
				/* filename(0-2), max-linkcount, task, signal */
			if ((pjob->fp = searchfid((char *) &pjob->param[0])) == NULL)
				goto errorsignal;
			fbuf = searchfbuf(pjob->fp);
			i = 0;
			if (fbuf)
				i = fbuf->linkcount;
			if (i > pjob->param[4])
				goto errorsignal;
			sgg_execcmd0(0x0020, 0x80000000 + 9, 0x1248, 0x0154,
				pjob->param[0], pjob->param[1], pjob->param[2] >> 8,
				0x4243 /* to pokon0 */, 0x7f000002, SIGNAL_REFRESH_FLIST, 0, 0x0000);
			break;
		}
	} while (pjob->now == 0);
	return;
}

/* jsub関数 */

void jsub_fbufready0(void *func)
/* pjob->fpで示されるファイルをpjob->fbufでアクセス可能な状態にする */
/* なお既にfbuf内に存在していた場合はpjob->fbufを書き換える(予備のものは自動開放) */
/* 完了したらpjob->retfuncを呼ぶ */
/* エラーのときはcond == 0になる。エラーの場合、unlinkの必要はない */
/* エラー時にはpjob->fbuf->linkcount = 0;になっているので、必要なら-1を再代入すること */
{
	struct STR_JOBLIST *pjob = &job;
	struct FILEBUF *fbuf;
	pjob->retfunc = func;
	if (fbuf = searchfbuf(pjob->fp)) {
		/* 確保しておいたやつを開放 */
		pjob->fbuf->linkcount = 0;
		pjob->fbuf = fbuf;
		fbuf->linkcount++;
		(*pjob->retfunc)(1); /* 成功 */
		return;
	}
	pjob->jsubfunc = jsub_fbufready1;
	sgg_execcmd0(0x0020, 0x80000000 + 8, 0x1247, 0x012c, pjob->fp->id,
		0x4244 /* to pokon0 */, 0x7f000003, SIGNAL_JSUB, 0, 0, 0x0000);
	return;
}

int jsub_fbufready1(int *sbp)
{
	struct STR_JOBLIST *pjob = &job;
	struct FILEBUF *fbuf = pjob->fbuf;
	int retcond = 0;
	int i, j = 0, size0, size1;
	fbuf->paddr = sbp[1];
	fbuf->size = sbp[2];
	fbuf->linkcount = 0;
	if (fbuf->size != -1) {
		fbuf->linkcount++;
		fbuf->dirslot = pjob->fp->id;

		if (auto_decomp) {
			size0 = fbuf->size;
			if (size0 < FILEAREA / 2 && size0 >= 20) {
				static unsigned char signature[16] = "\x82\xff\xff\xff\x01\x00\x00\x00OSASKCMP";
				unsigned char *fp0 = (char *) readCSd10;
				unsigned char *fp1 = fp0 + (size0 = ((size0 + 0xfff) & ~0xfff));
				sgg_execcmd0(0x0080, size0, fbuf->paddr, fp0, 0x0000); /* スロットを使わないマッピング */
				for (i = 0; i < 16; i++)
					j |= fp0[i] ^ signature[i];
				if (j == 0) {
					size1 = *((int *) (fp0 + 16));
					if (size0 + size1 <= FILEAREA) {
						if ((i = sgg_execcmd1(2 * 4 + 12, 0x0084, size1, 0, 0x0000)) != -1) { /* メモリをもらう */
							sgg_execcmd0(0x0080, size1, i, fp1, 0x0000); /* スロットを使わないマッピング */
							lib_decodetek0(size1, (int) (fp0 + 20), 0x000c, (int) fp1, 0x000c);
							/* ↓圧縮されたイメージを捨てる */
							sgg_execcmd0(0x0020, 0x80000000 + 5, 0x1244, 0x0134, -1, fbuf->size, fbuf->paddr, 0x0000);
							fbuf->readonly = 1;
							fbuf->paddr = i;
							fbuf->size = size1;
						}
					}
				}
				/* unmapする */
			}
		}

		fbuf->virtualmodule = sgg_createvirtualmodule(fbuf->size, fbuf->paddr);
		retcond++;
	}
	(*pjob->retfunc)(retcond);
	return 3; /* siglen */
}

void jsub_create_task0()
/* pjob->bankのみ参照 */
/* エラー時にはbank->tssが0になっているので、必要なら-1を代入すること */
{
	struct STR_JOBLIST *pjob = &job;
	struct FILEBUF *fbuf = pjob->bank->fbuf;
	pjob->jsubfunc = jsub_create_task1;
	sgg_createtask2(fbuf->size, fbuf->paddr, SIGNAL_JSUB);
	return;
}

int jsub_create_task1(int *sbp)
{
	struct STR_JOBLIST *pjob = &job;
	struct STR_BANK *bank = pjob->bank;
	int retcond = 0, tss, i;
	if (bank->tss = tss = sbp[1]) {
		retcond++;
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
		bank->Llv[1].inner  = i;
		sgg_settasklocallevel(tss,
			2 * 32 /* local level 2 (通常処理レベル) */,
			12 * 64 + 0x0100 /* global level 12 (一般アプリケーション) */,
			i /* Inner level */
		);
		bank->Llv[2].global = 12;
		bank->Llv[2].inner  = i;
		sgg_runtask(tss, 1 * 32);
	}
	(*pjob->retfunc)(retcond);
	return 2; /* siglen */
}

/* job関数 */

#if 0
void job_execute_file0(int cond)
{
	struct STR_JOBLIST *pjob = &job;
	if (cond == 0) { /* error */
	//	pjob->fbuf->linkcount = 0; /* 開放 */
		pjob->now = 0;
		return;
	}
	pjob->bank->fbuf = pjob->fbuf;
	pjob->retfunc = job_execute_file1;
	jsub_create_task0();
	return;
}

void job_execute_file1(int cond)
{
	struct STR_JOBLIST *pjob = &job;
	if (cond == 0) { /* error */
		unlinkfbuf(pjob->fbuf);
	//	pjob->bank->tss = 0; /* 開放 */
	}
	pjob->now = 0;
	return;
}
#endif

void job_view_file0(int cond)
{
	struct STR_JOBLIST *pjob = &job;
	if (cond == 0) { /* error */
	//	pjob->fbuf->linkcount = 0; /* 開放 */
		pjob->now = 0;
		return;
	}
	pjob->bank->fbuf = pjob->fbuf;
	pjob->retfunc = job_view_file1;
	jsub_create_task0();
	return;
}

void job_view_file1(int cond)
{
	struct STR_JOBLIST *pjob = &job;
	struct STR_OPEN_ORDER *order = pjob->order;
	if (cond == 0) { /* error */
		unlinkfbuf(pjob->fbuf);
	//	pjob->bank->tss = 0; /* 開放 */
	} else if (pjob->param[7]) {
		while (order->task);
		order->task = pjob->bank->tss;
		order->num = pjob->param[0];
		order->fp = (struct SGG_FILELIST *) pjob->param[1];
		if (pjob->param[2]) {
			/* シグナルを送る */
			sendsignal1dw(pjob->bank->tss, pjob->param[4]);
		}
	}
	pjob->now = 0;
	return;
}

void job_create_sysdisk0(int cond)
{
	struct STR_JOBLIST *pjob = &job;
	struct FILEBUF *fbuf = pjob->fbuf;
	pjob->fp = searchfid1("OSASKBS3BIN");
	if (cond == 0 || pjob->fp == NULL) { /* error */
	//	fbuf->linkcount = 0; /* 開放 */
		pjob->now = 0;
	} else {
		pjob->param[0] = (int) fbuf; /* キャッシュヒットかもしれないから更新 */
		pjob->fbuf = (struct FILEBUF *) pjob->param[1];
	//	pjob->retfunc = job_create_sysdisk1;
		jsub_fbufready0(job_create_sysdisk1);
	}
	return;

}

void job_create_sysdisk1(int cond)
{
	struct STR_JOBLIST *pjob = &job;
	struct FILESELWIN *win = pjob->win;
	int i;
	if (cond == 0) { /* error */
	//	pjob->fbuf->linkcount = 0; /* 開放 */
		unlinkfbuf((struct FILEBUF *) pjob->param[0]);
		pjob->now = 0;
	} else {
		pjob->param[1] = (int) pjob->fbuf; /* キャッシュヒットかもしれないから更新 */
		*pfmode = STATUS_LOAD_BOOT_SECTOR_CODE_COMPLETE; /* 'S'と'Enter'と'F'と'R'しか入力できない */
		for (i = 0; i < LIST_HEIGHT; i++)
			putselector0(win, i, "                ");
		putselector0(win, 1, "    Loaded.     ");
		putselector0(win, 3, " Change disks.  ");
		putselector0(win, 5, " Hit Enter key. ");
	}
	return;
}

void job_load_file0(int cond)
{
	struct STR_JOBLIST *pjob = &job;
	struct FILESELWIN *win = pjob->win;
	struct VIRTUAL_MODULE_REFERENCE *vmr = vmref0;
	struct FILEBUF *fbuf = pjob->fbuf;
	int i, task = pjob->param[0];
	if (task == 0) {
		/* resizeからの引継ぎ */
		task = win->task;
		win->sig[1] += pjob->param[7];
	}
	if (cond == 0) { /* error */
	//	fbuf->linkcount = 0; /* 開放 */
		if (win->mdlslot != -1) {
			/* エラーシグナルを送信 */
			sendsignal1dw(task, win->sig[1] + 1);
		}
	} else if (win->mdlslot == -1) {
		unlinkfbuf(fbuf);
	} else {
		for (i = 0; i < MAX_VMREF; i++, vmr++) {
			if (vmr->task == task && vmr->slot == win->mdlslot) {
				unlinkfbuf(vmr->fbuf);
				vmr->task = 0;
			}
		}
		for (vmr = vmref0; vmr->task; vmr++);
		vmr->task = task;
		vmr->fbuf = fbuf;
		vmr->slot = win->mdlslot;
		sgg_directwrite(0, 4, 0, win->mdlslot,
			(0x003c /* slot_sel */ | task << 8) + 0xf80000, (int) &fbuf->virtualmodule, 0x000c);
		sendsignal1dw(task, win->sig[1]);
	}
	win->mdlslot = -2;
	if (win->lp /* NULLならクローズ処理中 */) {
		win->task = 0;
		(*pselwincount)--;
	}
	pjob->now = 0;
	return;
}

int job_resize_sub0(int *sbp)
{
	struct STR_JOBLIST *pjob = &job;
	if (sbp[1] != 0) {
		/* リサイズ失敗 */
		pjob->param[7]++;
		if (pjob->param[6])
			sendsignal1dw(pjob->param[6], pjob->param[7]);
		sgg_getfilelist(256, file, 0, 0);
		pjob->now = 0;
	} else {
		pjob->fbuf = searchfrefbuf();
		jsub_fbufready0(job_resize_sub1);
	}
	return 2; /* siglen */
}

void job_resize_sub1(int cond)
{
	struct STR_JOBLIST *pjob = &job;
	struct FILEBUF *fbuf = pjob->fbuf;
	if (cond == 0) {
		/* リサイズ失敗(ロード失敗) */
	//	pjob->fbuf = 0; /* 開放 */
		pjob->param[7]++;
		if (pjob->param[6])
			sendsignal1dw(pjob->param[6], pjob->param[7]);
		sgg_getfilelist(256, file, 0, 0);
		pjob->now = 0;
	} else {
		unsigned char *fp0 = (char *) readCSd10;
		sgg_execcmd0(0x0080, FILEAREA, fbuf->paddr, fp0, 0x0000); /* スロットを使わないマッピング */
		*fp0 = 0xcc; /* 書き換え */
		/* unmapする */
		/* 手動unlink (writeback) */
		sgg_execcmd0(0x0020, 0x80000000 + 5, 0x1244, 0x0134, fbuf->dirslot, fbuf->size, fbuf->paddr, 0x0000);
		fbuf->dirslot = -1;
		fbuf->linkcount = 0; /* 開放 */
		sgg_execcmd0(0x0020, 0x80000000 + 10, 0x1249, 0x0150,
			pjob->param[0], pjob->param[1], pjob->param[2] >> 8, pjob->param[3], 
			0x4243 /* to pokon0 */, 0x7f000002, SIGNAL_REFRESH_FLIST0, 1 /* 正常終了が-1だから */, 0x0000);
	}
	return;
}

void job_resize_sub2(int cond)
{
	struct STR_JOBLIST *pjob = &job;
	if (cond == 0) {
		/* ロード失敗 */
	//	pjob->fbuf = 0; /* 開放 */
		/* アプリからではないのでシグナルは送らない */
		pjob->now = 0;
	} else {
		if (pjob->fbuf->readonly)
			pjob->now = 0;
		else {
			int i = SIGNAL_REFRESH_FLIST0, j = pjob->param[3];
			if (pjob->fp->size == 0) {
				i = SIGNAL_JSUB;
				j = 1;
			}
			pjob->jsubfunc = job_resize_sub0;
			sgg_execcmd0(0x0020, 0x80000000 + 10, 0x1249, 0x0150,
				pjob->param[0], pjob->param[1], pjob->param[2] >> 8, j, 
				0x4243 /* to pokon0 */, 0x7f000002, i, 1 /* 正常終了が-1だから */, 0x0000);
		}
		unlinkfbuf(pjob->fbuf);
	}
	return;
}


/////

void put_file(struct FILESELWIN *win, const char *name, const int pos, const int col)
{
	static char charbuf16[17] = "          .     ";
	static int color[2][2] = { 0, 15, 15, 2 };
	int i;
	if (*name) {
		for (i = 0; i < 8; i++)
			charbuf16[2 + i] = name[i];
		for (/* i = 8 */; i < 11; i++)
			charbuf16[3 + i] = name[i];
	//	charbuf16[10] = '.';
		lib_putstring_ASCII(0x0001, 0, pos, &win->selector.tbox, color[col][0], color[col][1], charbuf16);
	} else
	//	lib_putstring_ASCII(0x0000, 0, pos, selector, 0, 0, "                ");
		putselector0(win, pos, "                ");
	return;
}

void list_set(struct FILESELWIN *win)
{
	int i, ext = win->ext, j;
	struct SGG_FILELIST *fp;
	struct FILELIST *lp, *list = win->list, *wp1, *wp2, tmp;
	static int sort_order[2][11] = {
		{ 0, 1, 2,  3, 4, 5, 6, 7, 8, 9, 10 },
		{ 8, 9, 10, 0, 1, 2, 3, 4, 5, 6, 7  },
	};

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
					j = (int) ((wp2 - 1)->name[sort_order[sort_mode][i]])
						- (int) (wp2->name[sort_order[sort_mode][i]]);
					if (j == 0)
						continue;
					if (j > 0) {
						// swap and break
						tmp = *wp2;
						*wp2 = *(wp2 - 1);
						*(wp2 - 1) = tmp;
					}
					break;
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
		lib_putstring_ASCII(0x0000, 0, LIST_HEIGHT / 2 - 1, &win->selector.tbox, 1, 0, "File not found! ");
		win->cur = -1;
	} else {
 		put_file(win, lp[0].name, 0, 1);
		win->cur = 0;
	}
	return;
}

void putcursor()
{
	struct STR_CONSOLE *pcons = console;
	pcons->sleep = 0;
	lib_putstring_ASCII(0x0001, pcons->curx, pcons->cury, &pcons->tbox.tbox, 0,
		((pcons->col & pcons->cursorflag) | ((pcons->col >> 4) & ~(pcons->cursorflag))) & 0x0f, " ");
	return;
}

#if 0

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

#endif

void itoa10(unsigned int i, char *s)
{
	int j = 10;
	s[11] = '\0';
	do {
		s[j] = '0' + i % 10;
		j--;
	} while (i /= 10);
	while (j >= 0)
		s[j--] = ' ';
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
		{ DEFSIG_EXT1 | DEFSIG_NOSHIFT | DEFSIG_NOCTRL | DEFSIG_NOALT | 0xa4 /* Insert */,	0, SIGNAL_DISK_CHANGED },
		{ DEFSIG_EXT1 | DEFSIG_NOSHIFT | DEFSIG_NOCTRL | DEFSIG_NOALT | 0xa5 /* Delete */,	0, SIGNAL_START_WB },
		{ DEFSIG_EXT1 | DEFSIG_SHIFT | DEFSIG_NOCTRL | DEFSIG_NOALT | 0xa4 /* Shift+Insert */,	0, SIGNAL_FORCE_CHANGED },
		{ DEFSIG_EXT1 | DEFSIG_SHIFT | DEFSIG_NOCTRL | DEFSIG_NOALT | 0xa5 /* Shift+Delete */,	0, SIGNAL_CHECK_WB_CACHE },
		{ DEFSIG_EXT1 | DEFSIG_SHIFT | DEFSIG_CTRL | 'C' /* Shif+Ctrl */,	1, SIGNAL_CREATE_NEW },
		{ DEFSIG_EXT1 | DEFSIG_SHIFT | DEFSIG_CTRL | 'S' /* Shif+Ctrl */,	0, SIGNAL_RESIZE },
		{ DEFSIG_EXT1 | DEFSIG_NOSHIFT | DEFSIG_CTRL | DEFSIG_NOALT | 'S' /* Ctrl */,	0, SIGNAL_CHANGE_SORT_MODE },
		{ DEFSIG_EXT1 | DEFSIG_NOSHIFT | DEFSIG_NOCTRL | DEFSIG_ALT | 'S' /* Alt */,	0, SIGNAL_CHANGE_SORT_MODE },
		{ SIGNAL_LETTER_START /* letters */,SIGNAL_LETTER_END - SIGNAL_LETTER_START,  SIGNAL_LETTER_START },
		{ 0,                               0,  0 }
	};
	struct KEY_TABLE0 *pkt;
	struct LIB_WINDOW *window;
	char *ss;

	lib_openwindow1_nm(window = &win->window, win->winslot, 160, 40 + LIST_HEIGHT * 16, 0x28, win->sigbase + 120 /* +6 */);
	lib_opentextbox_nm(0x1000, &win->wintitle.tbox,  0, 10, 1,  0,  0, window, 0x00c0, 0);
	lib_opentextbox_nm(0x0000, &win->subtitle.tbox,  0, 20, 1,  0,  0, window, 0x00c0, 0);
	lib_opentextbox_nm(0x0001, &win->selector.tbox, 15, 16, 8, 16, 32, window, 0x00c0, 0);
	lib_putstring_ASCII(0x0000, 0, 0, &win->wintitle.tbox, 0, 0, title);
	lib_putstring_ASCII(0x0000, 0, 0, &win->subtitle.tbox, need_wb & 9, 0, subtitle);

	for (pkt = table; pkt->code; pkt++)
		lib_definesignal1p0(pkt->opt, 0x0100, pkt->code, window, win->sigbase + pkt->signum);
	lib_definesignal0p0(0, 0, 0, 0);
	ss = win->subtitle_str;
	while (*ss++ = *subtitle++);
	return;
}

struct FILEBUF *searchfbuf(struct SGG_FILELIST *fp)
{
	struct FILEBUF *fbuf = filebuf;
	int i;
	int fileid = fp->id;

	for (i = 0; i < MAX_FILEBUF; i++, fbuf++) {
		if (fbuf->dirslot == fileid)
			return fbuf;
	}
	return NULL;
}

struct FILEBUF *searchfrefbuf()
{
	struct FILEBUF *fbuf = filebuf;
	int i;
	for (i = 0; i < MAX_FILEBUF; i++, fbuf++) {
		if (fbuf->linkcount == 0) {
			fbuf->linkcount = -1; /* 予約マーク */
			fbuf->readonly = 0;
			return fbuf;
		}
	}
	return NULL;
}

struct FILEBUF *check_wb_cache(struct FILEBUF *fbuf)
{
	struct FILEBUF *fbuf0 = filebuf + MAX_FILEBUF;
	while (fbuf < fbuf0) {
		if (fbuf->dirslot != -1 && fbuf->size != 0 && fbuf->readonly == 0) {
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
	struct STR_CONSOLE *pcons;
	struct VIRTUAL_MODULE_REFERENCE *vmref;
	struct FILESELWIN *selwin;

	int i, j, sig, *sb0, *sbp, tss;
	int *subcmd;
	struct STR_BANK *bank;
	struct FILEBUF *fbuf, *fbuf0;
	struct SELECTORWAIT *swait;
	struct STR_OPEN_ORDER *order;
	struct SGG_FILELIST *fp;
	struct SELECTORWAIT *selwait;
	int selwaitcount = 0;
	signed char bootflags = 0;
	signed char fmode = STATUS_BOOTING;
	unsigned char selwincount = 1;

	MALLOC_ADDR = readCSd10 = lib_readCSd(0x0010);

	lib_init(AUTO_MALLOC);
	sgg_init(AUTO_MALLOC);

	sbp = sb0 = lib_opensignalbox(256, AUTO_MALLOC, 0, 4);
	pfmode = &fmode;
	pselwincount = &selwincount;

	file = (struct SGG_FILELIST *) malloc(256 * sizeof (struct SGG_FILELIST));
	banklist = (struct STR_BANK *) malloc(MAX_BANK * sizeof (struct STR_BANK));
	filebuf = fbuf0 = (struct FILEBUF *) malloc(MAX_FILEBUF * sizeof (struct FILEBUF));
	selwin0 = selwin = (struct FILESELWIN *) malloc(MAX_SELECTOR * sizeof (struct FILESELWIN));
	selwait = (struct SELECTORWAIT *) malloc(MAX_SELECTORWAIT * sizeof (struct SELECTORWAIT));
	vmref = (struct VIRTUAL_MODULE_REFERENCE *) malloc(MAX_VMREF * sizeof (struct VIRTUAL_MODULE_REFERENCE));
	vmref0 = vmref;
	subcmd = (int *) malloc(256 * sizeof (int));
	order = (struct STR_OPEN_ORDER *) malloc(MAX_ORDER * sizeof (struct STR_OPEN_ORDER));
	pjob->order = order; 
	pcons = console = malloc(sizeof (struct STR_CONSOLE));

	pjob->list = (int *) malloc(JOBLIST_SIZE * sizeof (int));
	*(pjob->rp = pjob->wp = pjob->list) = 0; /* たまった仕事はない */
	pjob->free = JOBLIST_SIZE - 1;

//	pcons->win = (struct LIB_WINDOW *) malloc(sizeof (struct LIB_WINDOW));
//	pcons->title = (struct LIB_TEXTBOX *) malloc(sizeof (struct LIB_TEXTBOX) + 16 * 8);
//	pcons->tbox = (struct LIB_TEXTBOX *) malloc(sizeof (struct LIB_TEXTBOX) + CONSOLESIZEX * CONSOLESIZEY * 8);
//	pcons->buf = (char *) malloc((CONSOLESIZEX + 2) * (CONSOLESIZEY + 1));
	pcons->curx = -1;

	win = selwin;
	for (i = 0; i < MAX_SELECTOR; i++, win++) {
		win->task = 0;
		win->winslot = 0x0220 + i * 16;
		win->sigbase = 512 + 128 * i;
		win->subtitle_str[0] = '\0';
	}
	win = selwin;

	for (i = 0; i < MAX_SELECTORWAIT; i++)
		selwait[i].task = 0;

	for (i = 0; i < MAX_VMREF; i++)
		vmref[i].task = 0;

	for (i = 0; i < MAX_ORDER; i++)
		order[i].task = 0;

	openselwin(&win[0], POKON_VERSION, "< Run Application > ");

	lib_opentimer(SYSTEM_TIMER);
	lib_definesignal1p0(0, 0x0010 /* timer */, SYSTEM_TIMER, 0, CONSOLE_CURSOR_BLINK);

	/* キー操作を追加登録 */
	{
		static struct KEY_TABLE1 {
			int code;
			unsigned char opt, signum;
		} table[] = {
			{ 'F' | DEFSIG_EXT1 | DEFSIG_NOSHIFT | DEFSIG_CTRL | DEFSIG_NOALT, 0, COMMAND_TO_FORMAT_MODE },
			{ 'R' | DEFSIG_EXT1 | DEFSIG_NOSHIFT | DEFSIG_CTRL | DEFSIG_NOALT, 0, COMMAND_TO_RUN_MODE },
			{ 'S' | DEFSIG_EXT1 | DEFSIG_NOSHIFT | DEFSIG_CTRL | DEFSIG_NOALT, 0, COMMAND_CHANGE_FORMAT_MODE },
			{ 'C' | DEFSIG_EXT1 | DEFSIG_NOSHIFT | DEFSIG_CTRL | DEFSIG_NOALT, 0, COMMAND_OPEN_CONSOLE },
			{ 'M' | DEFSIG_EXT1 | DEFSIG_NOSHIFT | DEFSIG_CTRL | DEFSIG_NOALT, 0, COMMAND_OPEN_MONITOR },
			{ 'B' | DEFSIG_EXT1 | DEFSIG_NOSHIFT | DEFSIG_CTRL | DEFSIG_NOALT, 0, COMMAND_BINEDIT },
			{ 'T' | DEFSIG_EXT1 | DEFSIG_NOSHIFT | DEFSIG_CTRL | DEFSIG_NOALT, 0, COMMAND_TXTEDIT },
			{ 'P' | DEFSIG_EXT1 | DEFSIG_NOSHIFT | DEFSIG_CTRL | DEFSIG_NOALT, 0, COMMAND_CMPTEK0 },
			{ 'U' | DEFSIG_EXT1 | DEFSIG_NOSHIFT | DEFSIG_CTRL | DEFSIG_NOALT, 0, COMMAND_MCOPY },
	//		{ 'F' | DEFSIG_EXT1 | DEFSIG_NOSHIFT | DEFSIG_NOCTRL | DEFSIG_ALT, 0, COMMAND_TO_FORMAT_MODE },
	//		{ 'R' | DEFSIG_EXT1 | DEFSIG_NOSHIFT | DEFSIG_NOCTRL | DEFSIG_ALT, 0, COMMAND_TO_RUN_MODE },
	//		{ 'S' | DEFSIG_EXT1 | DEFSIG_NOSHIFT | DEFSIG_NOCTRL | DEFSIG_ALT, 0, COMMAND_CHANGE_FORMAT_MODE },
	//		{ 'C' | DEFSIG_EXT1 | DEFSIG_NOSHIFT | DEFSIG_NOCTRL | DEFSIG_ALT, 0, COMMAND_OPEN_CONSOLE },
	//		{ 'M' | DEFSIG_EXT1 | DEFSIG_NOSHIFT | DEFSIG_NOCTRL | DEFSIG_ALT, 0, COMMAND_OPEN_MONITOR },
	//		{ 'B' | DEFSIG_EXT1 | DEFSIG_NOSHIFT | DEFSIG_NOCTRL | DEFSIG_ALT, 0, COMMAND_BINEDIT },
	//		{ 'T' | DEFSIG_EXT1 | DEFSIG_NOSHIFT | DEFSIG_NOCTRL | DEFSIG_ALT, 0, COMMAND_TXTEDIT },

			{ 0,                0, 0    }
		};
		struct KEY_TABLE1 *pkt;
		for (pkt = table; pkt->code; pkt++)
			lib_definesignal1p0(pkt->opt, 0x0100, pkt->code, &selwin[0].window, pkt->signum);
		lib_definesignal0p0(0, 0, 0, 0);
	}

	for (i = 0; i < MAX_BANK; i++)
		banklist[i].tss = 0;

	for (i = 0; i < MAX_FILEBUF; i++) {
		fbuf0[i].dirslot = -1;
		fbuf0[i].linkcount = 0;
	}

	for (;;) {
		unsigned char siglen = 1;
		/* 全てのシグナルは、main()でやり取りする */
		sig = *sbp;
		win = selwin;
		if (sig < COMMAND_SIGNAL_START) {
			switch (sig) {
			case NO_SIGNAL:
			//	siglen--; /* siglen = 0; */
				pcons->sleep = 1;
				lib_waitsignal(0x0001, 0, 0);
				continue;

			case SIGNAL_REWIND:
			//	siglen--; /* siglen = 0; */
				lib_waitsignal(0x0000, *(sbp + 1), 0);
				sbp = sb0;
				continue;

			case 98:
			//	siglen = 1;
				bootflags |= 0x02;
				goto bootflags_check;

			case SIGNAL_BOOT_COMPLETE:
			//	siglen = 1;
				bootflags |= 0x01;
	bootflags_check:
				if (bootflags == 0x03) {
					win[0].ext = ext_ALL;
					list_set(&win[0]);
					fmode = STATUS_RUN_APPLICATION;
				}
				break;

			case SIGNAL_TERMINATED_TASK:
				siglen++; /* siglen = 2; */
				tss = sbp[1];
				for (bank = banklist; bank->tss != tss; bank++);
				bank->tss = 0;

				/* タスクtssへ通知しようと思っていたダイアログを全てクローズ */
				for (i = 1; i < MAX_SELECTOR; i++) {
					if (win[i].task == tss) {
						if (win[i].mdlslot > 0)
							win[i].mdlslot = -1;
						if (win[i].subtitle_str[0] != 0 && win[i].lp != NULL) {
							win[i].lp = NULL;
							lib_closewindow(0, &win[i].window);
							win[i].mdlslot = -2;
						}
					}
				}
				/* 本来は、タスクを終了する前に、pokon0に通知があってしかるべき */
				/* そうでないと、終了したのにシグナルを送ってしまうという事が起こりうる */
				/* スロットのクローズを検出すれば、通知は可能 */

				for (i = 0; i < MAX_SELECTORWAIT; i++) {
					if (selwait[i].task == tss) {
						selwait[i].task = 0;
						selwaitcount--;
					}
				}

				unlinkfbuf(bank->fbuf);

				for (i = 0; i < MAX_ORDER; i++) {
					if (tss != order[i].task)
						continue;
					order[i].task = 0;
				}

		freefiles:
				for (i = 0; i < MAX_VMREF; i++, vmref++) {
					if (tss == vmref->task) {
						unlinkfbuf(vmref->fbuf);
						vmref->task = 0;
					}
				}
				vmref = vmref0;
				break;

			case SIGNAL_REQUEST_DIALOG:
			case SIGNAL_REQUEST_DIALOG2:
				/* とりあえずバッファに入れておく */
				siglen = 6;
				for (swait = selwait; swait->task; swait++); /* 行き過ぎる事は考えてない */
				selwaitcount++;
				swait->task    = sbp[1];
				swait->slot    = sbp[2];
				swait->bytes   = sbp[3];
				swait->ofs     = sbp[4];
				swait->sel     = sbp[5];
				break;

			case SIGNAL_FREE_FILES:
				/* ファイル開放要求(えせファイルシステム用) */
				siglen++; /* siglen = 2; */
				tss = sbp[1];
				goto freefiles;

			case SIGNAL_RESIZE_FILE:
				/* ファイルサイズ変更要求 */
				/* cmd, virtualmodule, new-size, task, sig, slot */
				siglen = 6;
				if (pjob->free >= 8 + 5 + 4) {
					/* 空きが十分にある */
					/* これ以降はかなりの手抜きがある(とりあえず動けばよいというレベル) */
					/* ディスクが交換されるかもしれないことに配慮していない */
					/*   →いや、ちゃんと配慮している */
					for (fbuf = fbuf0; fbuf->virtualmodule != sbp[1] || fbuf->linkcount <= 0; fbuf++);
					if ((i = fbuf->dirslot) != -1 && selwincount < MAX_SELECTOR && fbuf->linkcount == 1 && fbuf->readonly == 0) {
						struct VIRTUAL_MODULE_REFERENCE *vmr;
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
						for (vmr = vmref; vmr->fbuf != fbuf || vmr->task == 0; vmr++);
						vmr->task = 0;
						unlinkfbuf(fbuf);
						fbuf->linkcount = -1; /* このfbufをリザーブする */
						writejob_n(13, JOB_RESIZE_FILE, *((int *) &fp->name[0]),
							*((int *) &fp->name[4]), (*((int *) &fp->name[8]) << 8) | '.',
							sbp[2], 0 /* new-size, max-linkcount */, 0, 0 /* task, signal */,
							JOB_LOAD_FILE, (int) fbuf, (int) win, 0, fp);
						break;
					}
				}
	resize_error:
				sendsignal1dw(sbp[3], sbp[4] + 1); /* error */
				break;

			case SIGNAL_NEED_WB:
				/* ファイルキャッシュはライトバックが必要 */
				need_wb = -1;
		no_wb_cache2:
				siglen++; /* siglen = 2; */
				if (fmode == STATUS_RUN_APPLICATION) {
					for (i = 0; i < MAX_SELECTOR; i++) {
						if (win[i].subtitle_str[0])
							lib_putstring_ASCII(0x0000, 0, 0, &win[i].subtitle.tbox, need_wb & 9, 0, win[i].subtitle_str);
					}
				}
				break;

			case SIGNAL_JSUB:
				siglen = (*pjob->jsubfunc)(sbp);
				break;

			case SIGNAL_NO_WB_CACHE:
				need_wb = 0;
				pjob->now = 0;
				goto no_wb_cache2;

			case SIGNAL_CHECK_WB_CACHE_NEXT:
				siglen++; /* siglen = 2; */
				fbuf = (struct FILEBUF *) sbp[1];
				if (check_wb_cache(fbuf) == NULL) /* 終了したらNULLを返す */
					goto job_end;
				break;

			case SIGNAL_REFRESH_FLIST:
				siglen++; /* siglen = 2; */
				if (sbp[1])
					pjob->param[7]++;
				if (pjob->param[6])
					sendsignal1dw(pjob->param[6], pjob->param[7]);
				goto refresh_selector0;

			case SIGNAL_REFRESH_FLIST0:
				siglen++; /* siglen = 2; */
				if (sbp[1])
					pjob->param[7]++;
				if (pjob->param[6])
					sendsignal1dw(pjob->param[6], pjob->param[7]);
	refresh_filelist:
				sgg_getfilelist(256, file, 0, 0);
	job_end:
				pjob->now = 0;
				break;

			case SIGNAL_RELOAD_FAT_COMPLETE:
			//	siglen = 1;
				for (i = 0; i < MAX_FILEBUF; i++)
					fbuf0[i].dirslot = -1;
			refresh_selector0:
				pjob->now = 0;
			refresh_selector:
				/* 全てのファイルセレクタを更新 */
				for (i = 0; i < MAX_SELECTOR; i++) {
					if (win[i].subtitle_str[0])
						list_set(&win[i]);
				}
				break;

			case SIGNAL_FORMAT_COMPLETE: /* フォーマット完了 */
			//	siglen = 1;
write_exe:
				fmode = STATUS_FORMAT_COMPLETE;
				fbuf = (struct FILEBUF *) pjob->param[0];
				pjob->fbuf = (struct FILEBUF *) pjob->param[1];
				/* 何があってもキャッシュ無効 */
				for (i = 0; i < MAX_FILEBUF; i++)
					fbuf0[i].dirslot = -1;
				putselector0(win, 1, " Writing        ");
				putselector0(win, 3, "   system image.");
				putselector0(win, 5, "  Please wait.  ");
				sgg_format2(0x0138 /* 1,440KBフォーマット用 圧縮 */,
					//	0x011c /* 1,760KBフォーマット用 非圧縮 */
					//	0x0128; /* 1,440KBフォーマット用 非圧縮 */
					pjob->fbuf->size, pjob->fbuf->paddr, fbuf->size, fbuf->paddr,
					SIGNAL_WRITE_KERNEL_COMPLETE /* finish signal */); // store system image
				break;

			case SIGNAL_WRITE_KERNEL_COMPLETE: /* .EXE書き込み完了 */
			//	siglen = 1;
				unlinkfbuf((struct FILEBUF *) pjob->param[0]);
				unlinkfbuf((struct FILEBUF *) pjob->param[1]);
				putselector0(win, 1, "   Completed.   ");
				putselector0(win, 3, " Change disks.  ");
				putselector0(win, 5, " Hit Ctrl+R key.");
				fmode = STATUS_WRITE_KERNEL_COMPLETE;
				goto job_end;
			}
		} else if (COMMAND_SIGNAL_START <= sig && sig < COMMAND_SIGNAL_END + 1) {
			/* selwin[0]に対する特別コマンド */
		//	siglen = 1;
			if (fmode >= STATUS_FORMAT_COMPLETE)
				goto nextsig; /* boot中は無視 */
			fp = win[0].lp[win[0].cur].ptr;
			switch (sig) {
			case COMMAND_TO_FORMAT_MODE:
			//	j = 0;
			//	for (i = 1; i < MAX_SELECTOR; i++)
			//		j |= win[i].task;
				if (/* j != 0 || */ selwincount != 1 || pcons->curx != -1 || need_wb != 0)
					break;
				if (fmode == STATUS_LOAD_BOOT_SECTOR_CODE_COMPLETE) {
					unlinkfbuf((struct FILEBUF *) pjob->param[0]);
					unlinkfbuf((struct FILEBUF *) pjob->param[1]);
					pjob->now = 0;
				}
				win[0].ext = ext_EXE;
				if (fmode == STATUS_WRITE_KERNEL_COMPLETE) {
					/* 空きが十分にある */
					writejob_1(JOB_INVALID_DISKCACHE);
					win->cur = -1;
				} else {
					list_set(&win[0]);
				}
				fmode = STATUS_MAKE_PLAIN_BOOT_DISK;
				lib_putstring_ASCII(0x0000, 0, 0, &win[0].subtitle.tbox, 0, 0, "< Load Systemimage >");
				break;

		/* フォーマットモードに入るには、他のファイルセレクタが全て閉じられていなければいけない */
		/* また、フォーマットモードに入っている間は、ファイルセレクタが開かない */

			case COMMAND_TO_RUN_MODE:
				if (fmode == STATUS_LOAD_BOOT_SECTOR_CODE_COMPLETE) {
					unlinkfbuf((struct FILEBUF *) pjob->param[0]);
					unlinkfbuf((struct FILEBUF *) pjob->param[1]);
					pjob->now = 0;
				}
				win[0].ext = ext_ALL;
				if (fmode == STATUS_WRITE_KERNEL_COMPLETE) {
					writejob_1(JOB_INVALID_DISKCACHE);
					win->cur = -1;
				} else {
					list_set(&win[0]);
				}
				fmode = STATUS_RUN_APPLICATION;
				lib_putstring_ASCII(0x0000, 0, 0, &win[0].subtitle.tbox, need_wb & 9, 0, "< Run Application > ");
				/* 待機している要求があれば、ウィンドウを開く */
				break;

			case COMMAND_CHANGE_FORMAT_MODE:
				if (fmode == STATUS_LOAD_BOOT_SECTOR_CODE_COMPLETE)
					goto write_exe;
				if (fmode == STATUS_MAKE_PLAIN_BOOT_DISK || fmode == STATUS_MAKE_COMPRESSED_BOOT_DISK) {
					fmode = STATUS_MAKE_PLAIN_BOOT_DISK + STATUS_MAKE_COMPRESSED_BOOT_DISK - fmode;
					lib_putstring_ASCII(0x0000, 0, 0, &win[0].subtitle.tbox, (fmode - 1) * 9, 0, "< Load Systemimage >");
				}
				break;

			case COMMAND_OPEN_CONSOLE:
				if (pcons->curx == -1 && fmode == STATUS_RUN_APPLICATION)
					open_console();
				break;

			case COMMAND_OPEN_MONITOR:
				break;

			case COMMAND_BINEDIT:
				i = (int) &BINEDIT;
		runviewer_ij:
				run_viewer((void *) i, fp);
				break;

			case COMMAND_TXTEDIT:
				i = (int) &TXTEDIT;
				goto runviewer_ij;

			case COMMAND_CMPTEK0:
				i = (int) &CMPTEK0;
				goto runviewer_ij;

			case COMMAND_MCOPY:
				i = (int) &MCOPY;
				goto runviewer_ij;

		//	case 99:
		//		lib_putstring_ASCII(0x0000, 0, 0, mode,     0, 0, "< Error 99        > ");
		//		break;
			}
		} else if (CONSOLE_SIGNAL_START <= sig && sig < FILE_SELECTOR_SIGNAL_START) {
			/* console関係のシグナル */
		//	siglen = 1;
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
					break;

				case CONSOLE_VRAM_ACCESS_DISABLE /* VRAMアクセス禁止 */:
					lib_controlwindow(0x0100, &pcons->win);
					break;

				case CONSOLE_REDRAW_0:
				case CONSOLE_REDRAW_1:
					/* 再描画 */
					lib_controlwindow(0x0203, &pcons->win);
					break;

				case CONSOLE_CHANGE_TITLE_COLOR /* change console title color */:
					siglen++; /* siglen = 2; */
					if (sbp[1] & 0x02) {
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
					break;

				case CONSOLE_CLOSE_WINDOW /* close console window */:
					if (pcons->cursoractive) {
						pcons->cursoractive = 0;
						lib_settimer(0x0001, SYSTEM_TIMER);
					}
					lib_closewindow(0, &pcons->win);
					pcons->curx = -1;
					break;

				case CONSOLE_CURSOR_BLINK /* cursor blink */:
					if (pcons->sleep != 0 && pcons->cursoractive != 0) {
						pcons->cursorflag =~ pcons->cursorflag;
						putcursor();
						lib_settimertime(0x0012, SYSTEM_TIMER, 0x80000000 /* 500ms */, 0, 0);
					}
					break;

				case CONSOLE_INPUT_ENTER /* consoleへのEnter入力 */:
					if (pcons->curx < 0)
						break;
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
							char skip, prmflg;
						} cmdlist[] = {
							poko_memory,		"memory", 6, 0,
							poko_color,			"color", 5, 1,
							poko_cls,			"cls", 3, 0,
							poko_mousespeed,	"mousespeed", 10, 1,
							poko_setdefaultIL,	"setdefaultIL", 12, 1,
							poko_tasklist,		"tasklist", 8, 0,
							poko_sendsignalU,	"sendsignalU", 11, 1,
							poko_LLlist,		"LLlist", 6, 1,
							poko_setIL,			"setIL", 5, 1,
							poko_create,		"create", 6, 1,
							poko_delete,		"delete", 6, 1,
							poko_rename,		"rename", 6, 1,
							poko_resize,		"resize", 6, 1,
							poko_nfname,		"nfname", 6, 1,
							poko_autodecomp,	"autodecomp", 10, 1,
							poko_sortmode,		"sortmode", 8, 1,
#if defined(DEBUG)
							poko_debug,			"debug", 5, 1,
#endif
							NULL,				NULL, 0, 0
						};
						struct STR_POKON_CMDLIST *pcmdlist = cmdlist;
						do {
							const char *line = p, *cmd = pcmdlist->cmd;
							while (*line == *cmd) {
								line++;
								cmd++;
							}
							if (*cmd == '\0' && *line == ' ') {
								if (pcmdlist->skip) {
									p += pcmdlist->skip;
									while (*p == ' ')
										p++;
								}
								status = -ERR_ILLEGAL_PARAMETERS;
								if (pcmdlist->prmflg == 0) {
									if (*p != '\0')
										break;
								}
								if (pcmdlist->prmflg == 1) {
									if (*p == '\0')
										break;
								}
								status = (*(pcmdlist->fnc))(p);
								break;
							}
						} while ((++pcmdlist)->fnc);
						if (status < 0)
							consoleout(pokon_error_message[-status - ERR_CODE_START]);
						if (status == 1)
							consoleout("\n");
					}
			prompt:
					consoleout(POKO_PROMPT);
					if (pcons->cursoractive) {
						lib_settimer(0x0001, SYSTEM_TIMER);
						pcons->cursorflag = ~0;
						putcursor();
						lib_settimertime(0x0032, SYSTEM_TIMER, 0x80000000 /* 500ms */, 0, 0);
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
					break;
				}
			}
		} else {
			/* ファイルセレクタへの一般シグナル */
			win = &selwin[(sig - FILE_SELECTOR_SIGNAL_START) >> 7];
			sig &= 0x7f;
		//	siglen = 1;
			if (fmode == STATUS_LOAD_BOOT_SECTOR_CODE_COMPLETE && sig == SIGNAL_ENTER) {
			//	putselector0(win, 1, " Writing        ");
				putselector0(win, 1, "  Formating...  ");
				putselector0(win, 3, "                ");
				/* 何があってもキャッシュ無効 */
				for (i = 0; i < MAX_FILEBUF; i++)
					fbuf0[i].dirslot = -1;
				sgg_format(0x0124 /* 1,440KBフォーマット */, SIGNAL_FORMAT_COMPLETE /* finish signal */); // format
				/* 1,760KBフォーマットと1,440KBフォーマットの混在モードは0x0118 */
				fmode = STATUS_FORMAT_COMPLETE;
				goto nextsig;
			}

			if (fmode > STATUS_MAKE_COMPRESSED_BOOT_DISK)
				goto nextsig; /* boot中は無視 */
			if (win->subtitle_str[0]) {
				int cur = win->cur;
				struct FILELIST *lp = win->lp, *list = win->list;
				fp = lp[cur].ptr;

				switch (sig) {
				case SIGNAL_CURSOR_UP:
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
						break;
					if (win != selwin) { /* not pokon */
						/* ファイルをロードして、仮想モジュールを生成し、
							スロットを設定し、シグナルを発する */
						/* 最後に、ウィンドウを閉じる */
						if ((fbuf = searchfrefbuf()) != NULL && pjob->free >= 5) {
							lib_closewindow(0, &win->window);
							/* 空きが十分にある */
							writejob_n(5, JOB_LOAD_FILE, (int) fbuf, (int) win, win->task, (int) fp);
							lp = NULL;
						//	bank->size = -1; /* これは何だ？？？ */
						}
						break;
					}
					if (win->ext == ext_ALL) {
						/* ALLファイルモード */
						j = lp[cur].name[8] | lp[cur].name[9] << 8 | lp[cur].name[10] << 16;
						if (j == ('B' | 'I' << 8 | 'N' << 16)) {
							if ((bank = searchfrebank()) == NULL)
								break;
							for (i = 0; i < 11; i++)
								bank->name[i] = lp[cur].name[i];
							bank->name[11] = '\0';
							if ((fbuf = searchfrefbuf()) == NULL)
								break;
							writejob_n(4, JOB_LOAD_FILE_AND_EXECUTE, (int) fp,
								(int) fbuf, (int) bank);
							break;
						}
						i = (int) &BINEDIT;
						if (j == ('B' | 'M' << 8 | 'P' << 16))
							i = (int) &PICEDIT;
						if (j == ('T' | 'X' << 8 | 'T' << 16))
							i = (int) &TXTEDIT;
						if (j == ('H' | 'E' << 8 | 'L' << 16))
							i = (int) &HELPLAY;
						if (j == ('M' | 'M' << 8 | 'L' << 16))
							i = (int) &MMLPLAY;
						goto runviewer_ij;
					}

					/* .EXEファイルモード */
					/* キャッシュチェックをしない */
					if (pjob->free >= 4) {
						/* 空きが十分にある */
						fmode = STATUS_FORMAT_COMPLETE;
						if ((i = (int) searchfrefbuf()) == 0)
							break;
						if ((fbuf = searchfrefbuf()) == NULL)
							break;
						writejob_n(4, JOB_LOAD_FILE_AND_FORMAT,
							(int) fp, i, (int) fbuf);
					}
					break;

				case SIGNAL_PAGE_UP:
					if (cur < 0 /* ファイルが１つもない */)
						break;
					if (lp >= list + LIST_HEIGHT)
						lp -= LIST_HEIGHT;
					else {
						lp = list;
						cur = 0;
					}
					goto listup;

				case SIGNAL_PAGE_DOWN:
					if (cur < 0 /* ファイルが１つもない */)
						break;
					for (i = 1; lp[i].name[0] != '\0' && i < LIST_HEIGHT * 2; i++);
					if (i < LIST_HEIGHT) {
						// 全体が1画面分に満たなかった
						cur = i - 1;
						goto listup;
					}
					if (i < LIST_HEIGHT * 2) {
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
					writejob_n(2, JOB_CHECK_WB_CACHE, JOB_INVALID_DISKCACHE);
					break;

				case SIGNAL_START_WB:
					/* Delete */
					writejob_n(2, JOB_CHECK_WB_CACHE, JOB_WRITEBACK_CACHE);
					break;

				case SIGNAL_FORCE_CHANGED:
					/* Shift+Insert */
					writejob_n(3, JOB_CHECK_WB_CACHE, JOB_INVALID_WB_CACHE,
						JOB_INVALID_DISKCACHE);
					break;

				case SIGNAL_CHECK_WB_CACHE:
					/* Shift+Delete */
					writejob_1(JOB_CHECK_WB_CACHE);
					break;

				case SIGNAL_CREATE_NEW:
					writejob_n(6, JOB_CREATE_FILE, 'N' | 'E' << 8 | 'W' << 16 | '_' << 24,
						'F' | 'I' << 8 | 'L' << 16 | 'E' << 24, '.' | ' ' << 8 | ' ' << 16 | ' ' << 24,
						0, 0);
					break;

				case SIGNAL_DELETE_FILE:
					if (cur < 0 /* ファイルが1つもない */)
						break;
					if (pjob->free >= 15) {
						/* 空きが十分にある */
						int name[3];
						char *cname = lp[cur].name;
						name[0] = cname[0] | cname[1] << 8 | cname[2] << 16 | cname[3] << 24;
						name[1] = cname[4] | cname[5] << 8 | cname[6] << 16 | cname[7] << 24;
						name[2] = '.' | cname[8] << 8 | cname[9] << 16 | cname[10] << 24;
						writejob_1(JOB_RESIZE_FILE);
						writejob_3p(&name[0]);
						writejob_n(5, 0, 0, 0, 0, JOB_DELETE_FILE);
						writejob_3p(&name[0]);
						writejob_n(3, 0, 0, 0);
					}
					break;

				case SIGNAL_RESIZE:
					i = (int) &RESIZER;
					goto runviewer_ij;

				case SIGNAL_CHANGE_SORT_MODE:
					if (fmode == STATUS_RUN_APPLICATION) {
						sort_mode = (sort_mode + 1) % SORTS;
						/* 全てのファイルセレクタを更新 */
						goto refresh_selector;
				    }
				    goto nextsig;

				case SIGNAL_WINDOW_CLOSE0 /* close window */:
					if (i == 0)
						break;
					/* キャンセルを通知して、閉じる */
					/* 待機しているものがあれば、応じる(応じるのは127を受け取ってからにする) */
					sendsignal1dw(win->task, win->sig[1] + 1 /* cancel */);
					lp = NULL;
					lib_closewindow(0, &win->window);
					win->mdlslot = -2;
					break;

				case SIGNAL_WINDOW_CLOSE1 /* closed window */:
					win->subtitle_str[0] = '\0';
					lp = win->list;
					if (win->mdlslot == -2) {
						win->task = 0;
						selwincount--;
					}
					break;

				default: /* search filename */
					if (cur < 0 /* ファイルが１つもない */)
						break;
					/* search from current to bottom */
				//	lp = win->lp;
					for (i = cur + 1; lp[i].name[0]; i++) {
						if (lp[i].name[0] == sig) {
							cur = i;
							j = LIST_HEIGHT - 1;
							if (cur >= LIST_HEIGHT - 1) {
								if (lp[i + 1].name[0] != '\0')
									j--; /* 下から 2段目 */
					search_fixcur:
								lp += cur - j;
								cur = j;
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
								if (cur > j)
									goto search_fixcur;
								goto listup;
							}
						}
					}
					goto nextsig;
				}
				win->cur = cur;
				win->lp = lp;
			}
		}
nextsig:
		if (siglen) {
			lib_waitsignal(0x0000, siglen, 0);
			sbp += siglen;
		}

		while (*sbp == 0 && selwaitcount != 0 && selwincount < MAX_SELECTOR && fmode == STATUS_RUN_APPLICATION) {
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
						fp = order[i].fp;
						goto open_redirect;
					}
				}
				for (bank = banklist; bank->tss != win->task; bank++);
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
				if ((fp = searchfid((unsigned char *) &subcmd[2])) == NULL)
					goto error_sig;
open_redirect:
				/* 見付かった */
				if ((fbuf = searchfrefbuf()) != NULL &&
					writejob_n(5, JOB_LOAD_FILE, (int) fbuf, (int) win, win->task, fp))
					continue;

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

void putconsline(int y)
{
	struct STR_CONSOLE *pcons = console;
	char *bp = pcons->buf + y * (CONSOLESIZEX + 2);
	lib_putstring_ASCII(0x0001, 0, y, &pcons->tbox.tbox,
		bp[CONSOLESIZEX + 1] & 0x0f, (bp[CONSOLESIZEX + 1] >> 4) & 0x0f, bp);
	return;
}

void consoleout(char *s)
{
	struct STR_CONSOLE *pcons = console;
	char *cbp0 = pcons->buf + pcons->cury * (CONSOLESIZEX + 2);
	while (*s) {
		if (*s == '\n') {
			s++;
			cbp0[CONSOLESIZEX + 1] = pcons->col;
			putconsline(pcons->cury);
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
					putconsline(j);
					bp += CONSOLESIZEX + 2;
				}
				pcons->cury = CONSOLESIZEY - 1;
			}
			cbp0 = pcons->buf + pcons->cury * (CONSOLESIZEX + 2);
		} else {
			cbp0[pcons->curx++] = *s++;
		}
	}
	cbp0[CONSOLESIZEX + 1] = pcons->col;
	putconsline(pcons->cury);
	return;
}

void open_console()
/*	コンソールをオープンする */
/*	カーソル点滅のために、setmodeも拾う */
/*	カーソル点滅のためのタイマーをイネーブルにする */
{
	struct STR_CONSOLE *pcons = console;
	struct LIB_WINDOW *win = &pcons->win;
	int i, j;
	char *bp;
	lib_openwindow1_nm(win, 0x0200, CONSOLESIZEX * 8, CONSOLESIZEY * 16, 0x0d, 256);
	lib_opentextbox_nm(0x1000, &pcons->title.tbox, 0, 16,  1,  0,  0, win, 0x00c0, 0);
	lib_opentextbox_nm(0x0001, &pcons->tbox.tbox,  0, CONSOLESIZEX, CONSOLESIZEY,  0,  0, win, 0x00c0, 0); // 5KB
	lib_putstring_ASCII(0x0000, 0, 0, &pcons->title.tbox, 0, 0, POKON_VERSION" console");

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
	pcons->curx = pcons->cury = pcons->cursoractive = pcons->sleep = 0;
	pcons->col = 15;
//	consoleout(POKO_VERSION);
//	consoleout(POKO_PROMPT);
	consoleout(POKO_VERSION POKO_PROMPT);
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

const int cons_getdec_skpspc(const char **p)
{
	const char *s = *p;
	int i = console_getdec(&s);
	while (*s == ' ')
		s++;
	*p = s;
	return i;
}

int poko_memory(const char *cmdlin)
{
	static struct {
		int cmd, opt, mem20[4], mem24[4], mem32[4], eoc;
	} command = { 0x0034, 0, { 0 }, { 0 }, { 0 }, 0x0000 };
	char str[12];
//	if (*cmdlin) return -ERR_ILLEGAL_PARAMETERS;

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
	consoleout("KB free");
	return 1;
}

int poko_color(const char *cmdlin)
{
	int param0, param1 = console->col & 0xf0;
//	if (*cmdlin == '\0')
//		goto error;

	param0 = cons_getdec_skpspc(&cmdlin);
	if (*cmdlin) {
		param1 = cons_getdec_skpspc(&cmdlin) << 4;
		if (*cmdlin)
			goto error;
	}
	consoleout("\n");
	console->col = param0 | param1;
	return 0;
error:
	return -ERR_ILLEGAL_PARAMETERS;
}

int poko_cls(const char *cmdlin)
{
	struct STR_CONSOLE *pcons = console;
	int i, j;
	char *bp = pcons->buf;
//	if (*cmdlin)
//		return -ERR_ILLEGAL_PARAMETERS;
	for (j = 0; j < CONSOLESIZEY; j++) {
		for (i = 0; i < CONSOLESIZEX + 2; i++)
			bp[i] = (pcons->buf)[i + (CONSOLESIZEX + 2) * CONSOLESIZEY];
		putconsline(j);
		bp += CONSOLESIZEX + 2;
	}
	pcons->cury = 0;
	return 0;
}

int poko_mousespeed(const char *cmdlin)
{
	int param;
//	if (*cmdlin == '\0')
//		goto error;
	param = cons_getdec_skpspc(&cmdlin);
	if (*cmdlin)
		goto error;
	sgg_wm0s_sendto2_winman0(0x6f6b6f70 + 0 /* mousespeed */, param);
	return 1;
error:
	return -ERR_ILLEGAL_PARAMETERS;
}

int poko_setdefaultIL(const char *cmdlin)
{
//	if (*cmdlin == '\0')
//		goto error;
	defaultIL = cons_getdec_skpspc(&cmdlin);
	if (*cmdlin)
		goto error;
	return 1;
error:
	return -ERR_ILLEGAL_PARAMETERS;
}

int poko_tasklist(const char *cmdlin)
{
	char str[12];
	static char msg[] = "000 name----\n";
	int i, j;
	struct STR_BANK *bank = banklist;
//	if (*cmdlin)
//		return -ERR_ILLEGAL_PARAMETERS;
	consoleout("\n");
	for (i = 0; i < MAX_BANK; i++) {
		if (bank[i].tss <= 0)
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
//	if (*cmdlin == '\0')
//		goto error;
	task = cons_getdec_skpspc(&cmdlin) * 4096 + 0x0242;
	if (*cmdlin == '\0')
		goto error;
	sig = cons_getdec_skpspc(&cmdlin);
	if (*cmdlin)
		goto error;
	sendsignal1dw(task, sig);
	return 1;
error:
	return -ERR_ILLEGAL_PARAMETERS;
}

int poko_LLlist(const char *cmdlin)
{
	int task, i;
	struct STR_BANK *bank = banklist;
//	if (*cmdlin == '\0')
//		goto error;
	task = cons_getdec_skpspc(&cmdlin) * 4096;
	for (i = 0; i < MAX_BANK; i++) {
		if (bank[i].tss == task)
			goto find;
	}
error:
	return -ERR_ILLEGAL_PARAMETERS;

find:
	bank += i;
	if (*cmdlin)
		goto error;
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
//	if (*cmdlin == '\0')
//		goto error;
	task = cons_getdec_skpspc(&cmdlin) * 4096;
	for (i = 0; i < MAX_BANK; i++) {
		if (bank[i].tss == task)
			goto find;
	}
error:
	return -ERR_ILLEGAL_PARAMETERS;

find:
	bank += i;
	if (*cmdlin == '\0')
		goto error;
	l = cons_getdec_skpspc(&cmdlin);
	if (*cmdlin != '\0')
		goto error;
	if (l == 0)
		goto error;
	
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
	return 1;
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

	cmdlin = pokosub_getfilename(cmdlin, filename.s);
	if (filename.s[0] == '\0')
		goto error;
	if (*cmdlin != '\0')
		goto error;
	if (pjob->free >= 6) {
		/* 空きが十分にある */
		writejob_1(JOB_CREATE_FILE);
		writejob_3p(&filename.i[0]);
		writejob_n(2, 0, 0 /* task, signal */);
	}
	return 1;
error:
	return -ERR_ILLEGAL_PARAMETERS;
}

int poko_delete(const char *cmdlin)
{
	union {
		char s[12];
		int i[3];
	} filename = { 0 };
	struct STR_JOBLIST *pjob = &job;
	cmdlin = pokosub_getfilename(cmdlin, filename.s);
	if (filename.s[0] == '\0')
		goto error;
	if (*cmdlin != '\0')
		goto error;
	if (pjob->free >= 15) {
		/* 空きが十分にある */
		writejob_1(JOB_RESIZE_FILE);
		writejob_3p(&filename.i[0]);
		writejob_n(5, 0 /* new-size */, 0 /* max-linkcount */,
			0 /* task */, 0 /* signal */, JOB_DELETE_FILE);
		writejob_3p(&filename.i[0]);
		writejob_n(3, 0 /* max-linkcount */, 0 /* task */, 0 /* signal */);
	}
	return 1;
error:
	return -ERR_ILLEGAL_PARAMETERS;
}

int poko_rename(const char *cmdlin)
{
	union {
		char s[12];
		int i[3];
	} filename0 = { 0 }, filename1 = { 0 };
	struct STR_JOBLIST *pjob = &job;
	cmdlin = pokosub_getfilename(cmdlin, filename0.s);
	cmdlin = pokosub_getfilename(cmdlin, filename1.s);
	if (filename0.s[0] == '\0')
		goto error;
	if (filename1.s[0] == '\0')
		goto error;
	if (*cmdlin != '\0')
		goto error;
	if (pjob->free >= 9) {
		/* 空きが十分にある */
		writejob_1(JOB_RENAME_FILE);
		writejob_3p(&filename0.i[0]);
		writejob_3p(&filename1.i[0]);
		writejob_n(2, 0, 0);
	}
	return 1;
error:
	return -ERR_ILLEGAL_PARAMETERS;
}

int poko_resize(const char *cmdlin)
{
	union {
		char s[12];
		int i[3];
	} filename = { 0 };
	struct STR_JOBLIST *pjob = &job;
	int size;
	cmdlin = pokosub_getfilename(cmdlin, filename.s);
	if (filename.s[0] == '\0')
		goto error;
	if (*cmdlin == '\0')
		goto error;
	size = cons_getdec_skpspc(&cmdlin);
	if (*cmdlin != '\0')
		goto error;
	if (pjob->free >= 8) {
		/* 空きが十分にある */
		writejob_1(JOB_RESIZE_FILE);
		writejob_3p(&filename.i[0]);
		writejob_n(4, size, 0 /* max-linkcount */, 0 /* task */, 0 /* signal */);
	}
	return 1;
error:
	return -ERR_ILLEGAL_PARAMETERS;
}

int poko_nfname(const char *cmdlin)
{
	union {
		char s[12];
		int i[3];
	} filename = { 0 };
	struct STR_JOBLIST *pjob = &job;
	cmdlin = pokosub_getfilename(cmdlin, filename.s);
	if (filename.s[0] == '\0')
		goto error;
	if (*cmdlin != '\0')
		goto error;
	if (pjob->free >= 9) {
		/* 空きが十分にある */
		writejob_n(4, JOB_RENAME_FILE, 'N' | 'E' << 8 | 'W' << 16 | '_' << 24,
			'F' | 'I' << 8 | 'L' << 16 | 'E' << 24, '.' | ' ' << 8 | ' ' << 16 | ' ' << 24);
		writejob_3p(&filename.i[0]);
		writejob_n(2, 0, 0 /* task, signal */);
	}
	return 1;
error:
	return -ERR_ILLEGAL_PARAMETERS;
}

struct STR_BANK *run_viewer(struct STR_VIEWER *viewer, struct SGG_FILELIST *fp2)
{
	int i;
	struct STR_JOBLIST *pjob = &job;
	struct FILEBUF *fbuf;
	struct STR_BANK *bank;
	struct SGG_FILELIST *fp;

	if ((bank = searchfrebank()) == NULL)
		goto ret_null;
	for (i = 0; i < 12; i++)
		bank->name[i] = viewer->binary[i];
//	bank->name[11] = '\0';
	if ((fp = searchfid1(viewer->binary)) == NULL)
		goto error;
	if ((fbuf = searchfrefbuf()) == NULL)
		goto error;

	if (pjob->free >= 10) {
		/* 空きが十分にある */
		writejob_n(6, JOB_VIEW_FILE, (int) fp, (int) fbuf, (int) bank, 1 /* num */, fp2);
		writejob_np(4, &viewer->signal[0]);
		return bank;
	}
	fbuf->linkcount = 0;
error:
	bank->tss = 0;
ret_null:
	return NULL;
}

int poko_autodecomp(const char *cmdlin)
{
	int param;
//	if (*cmdlin == '\0')
//		goto error;
	param = cons_getdec_skpspc(&cmdlin);
	if (*cmdlin)
		goto error;
	auto_decomp = param;
	return 1;
error:
	return -ERR_ILLEGAL_PARAMETERS;
}

int poko_sortmode(const char *cmdlin)
{
	int param, i;

//	if (*cmdlin == '\0')
//		goto error;
	param = cons_getdec_skpspc(&cmdlin);
	if (*cmdlin)
		goto error;
	if (param < SORT_NAME)
		goto error;
	if (param >= SORTS)
		goto error;
	sort_mode = param;
	/* 全てのファイルセレクタを更新 */
	for (i = 0; i < MAX_SELECTOR; i++) {
		if (selwin0[i].subtitle_str[0])
			list_set(&selwin0[i]);
	}
	return 1;
error:
	return -ERR_ILLEGAL_PARAMETERS;
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
	int ofs, i;
//	if (*cmdlin == '\0')
//		goto error;
	ofs = cons_getdec_skpspc(&cmdlin);
	if (*cmdlin)
		goto error;
	for (i = 0; i < 8; i++) {
		poko_puthex2(base[ofs + i]);
		consoleout(" ");
	}
	return 1;
error:
	return -ERR_ILLEGAL_PARAMETERS;
}

#endif
