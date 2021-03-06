/*
  ファイルモジュール化クラス　〜module.h + module.cpp〜
*/
#ifndef	__MODULE_H
#define	__MODULE_H

#pragma warning(disable:4786)

#ifdef _WIN32
#include <io.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <map>
#include <stack>

#include "macro.hpp"

class Module {

     LPVOID		lpMdlAdr;		// モジュールアドレス
     DWORD		dwMdlSize;		// モジュールサイズ
     LPSTR		lpMdlPos;		// ReadLineの処理位置
     std::string	FileName;		// モジュール化しているファイル名
     std::string	MakeFullPath(std::string& filename);		// ディレクトリを補う

public:
     Module(void){ lpMdlAdr=NULL; dwMdlSize=0; lpMdlPos=NULL; }
     ~Module(){ Release(); }
#ifdef WINVC
     void	Release(void){ DELETEPTR_SAFE(lpMdlAdr); dwMdlSize=0; lpMdlPos=NULL; }
#else
     void	Release(void){
/**       TODO:
	  生ポインタは使用しないようにする
	  DELETEPTR_SAFE(    lpMdlAdr    );
	  dwMdlSize=0;
	  lpMdlPos=NULL;
*/
     }
#endif

     std::string	GetFileName(void){ return FileName; }
     LPVOID	GetModuleAddress(void){ return lpMdlAdr; }
     DWORD	GetModuleSize(void){ return dwMdlSize; }

     // 戻り値 0:正常終了  1:fileopen失敗  3:メモリ確保失敗  4:読み込み失敗  5:fileclose失敗
     HRESULT ReadFile(std::string& filename);
     // 戻り値 0:正常終了  1:EOF  2:バッファあふれ  3:ファイル読み込んでない
     HRESULT ReadLine(LPSTR buf);

private:
     std::ifstream::pos_type filelength(const char* filename);

};

//現段階ではディレクトリの追加などはしていない。そのうち
//includeディレクトリを追加して、順に検索していく方式にしたい。

#endif
