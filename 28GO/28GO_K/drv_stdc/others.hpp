#ifndef OTHERS_HPP_
#define OTHERS_HPP_

#define GOLD_write_t	GOLD_write_b
#define GOLD_exit	ExitProcess

/** ファイルサイズ取得 */
int GOLD_getsize(const UCHAR *name);

/** ファイル読み込み、バイナリモード、
    サイズを呼び出し側で直前にファイルチェックしていて、
    ファイルサイズぴったりを要求してくる                 */
int GOLD_read(const UCHAR *name, int len, UCHAR *b0);

#endif /* OTHERS_HPP */
