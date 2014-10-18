/*
	コード生成クラス　〜generator.h + generator.cpp〜
*/
#ifndef	__GENERATOR_H
#define	__GENERATOR_H

#pragma warning(disable:4786)

#include <cstdio>
#include <cstdlib>
#include <string>
#include <iostream>

#include <string>
#include <list>
#include <map>
#include <stack>



#include "macro.hpp"
#include "module.hpp"
#include "scanner.hpp"
#include "table.hpp"
#include "textmodule.hpp"

class Generator{
  public:
	typedef	TextModule::LineData	LineData;

  private:
	Register		RegisterData;
	Segment			SegmentData;
	Tag			TagData;
	Label			LocalData;
	Label			GlobalData;
	bool			bExistMain;			// ソースファイル内にmain()が出てきたらtrue

	std::string			OutFileName;			// コードを出力するファイル名
	TextModule		OutFile;			// コード出力用ファイルモジュール
	Scanner*		scanner;			// Scannerのポインタ（Error()で使う）
	FILE*			lpLogFP;			// エラーメッセージ出力用FP
	int			nErrorCount;			// このソースで起こったエラーの数

	int			CheckPower2(int x);
	void			FlushStaticData(void);
	void			RegistBecome(Parameter& param1, Parameter& param2);
	HRESULT			RegisterDown(Parameter& param1, Parameter& param2);
	void			RegistAdd(Parameter& param1, Parameter& param2);
	void			RegistSub(Parameter& param1, Parameter& param2);
	void			RegistMul(Parameter& param1, Parameter& param2);
	void			RegistDiv(Parameter& param1, Parameter& param2);
	void			RegistAnd(Parameter& param1, Parameter& param2);
	void			RegistOr(Parameter& param1, Parameter& param2);
	void			RegistXor(Parameter& param1, Parameter& param2);
	void			RegistCmp(Parameter& param1, Parameter& param2);
	void			RegistShl(Parameter& param1, Parameter& param2);
	void			RegistShr(Parameter& param1, Parameter& param2);
	void			OpenSegment(SegmentList* segment);
	void			CloseSegment(SegmentList* segment);
	void			Error(const LPSTR str);	// エラーメッセージを表示

  public:
	SegmentList*	prevseg;		// 前に処理していたセグメントを入れる
	SegmentList*	seg;			// 現在処理中のセグメントを入れる

	void	debug(void);
	Generator();
	~Generator(){}
	
	void			BeginGenerate(std::string& outfilename, Scanner* s);
	void			EndGenerate(void);

	RegisterList*		FindRegisterList(const LPSTR key){ return RegisterData.Find(key); }
	void			AddSegmentList(LPSTR key, SegmentList* s){ SegmentData.Add(key, s); }
	SegmentList*		FindSegmentList(LPSTR key){ return SegmentData.Find(key); }
	void			AddTagList(LPSTR key, TagList* t){ TagData.Add(key, t); }
	TagList*		FindTagList(const LPSTR key){ return TagData.Find(key); }
	void			AddLocalLabelList(const LPSTR key, LabelList* l){ LocalData.Add(key, l); }
	LabelList*		FindLocalLabelList(LPSTR key){ return LocalData.Find(key); }
	void			ClearLocalLabelList(void){ LocalData.Clear(); }
	void			AddGlobalLabelList(LPSTR key, LabelList* l){ GlobalData.Add(key, l); }
	LabelList*		FindGlobalLabelList(LPSTR key){ return GlobalData.Find(key); }
	LabelList*		FindLabelList(LPSTR key);

	void			Seek(LineData linedata){ OutFile.SeekLine(linedata); }
	LineData		Reserve(){ return OutFile.ReserveLine(); }
	void			Erase(LineData linedata){ OutFile.EraseLine(linedata); }

	void			Param2LPSTR(LPSTR buf, Parameter& param);
	void			OutputMASM(const LPSTR label, const LPSTR command, const LPSTR param, const LPSTR comment);
	void			Call(Token ptype, LPSTR func);
	void			GlobalLabel(LPSTR label);
	void			LocalLabel(LPSTR label);
	void			Op1(Token command, Parameter& param);
	void			Op2(Token command, Parameter& param1, Parameter& param2);
	void			StartFunction(LPSTR funcname, Token ptype, SegmentList* segment);
	void			EndFunction(LPSTR funcname, int sysvarlocalvalue);
	void			RegistVariable(LPSTR varname, LabelList* label, LPSTR initialize);
	void			StartSegment(SegmentList* segment);
	void			EndSegment(void);
	void			AssumeSegment(SegmentList* seg, RegisterList* reg);
	void			Jump(CompareType cmptype, int labelcount);
	void			GlobalJump(CompareType cmptype, LPSTR globallabel);
	void			LocalJump(CompareType cmptype, LPSTR locallabel);
	void			Jump(CompareType cmptype, LPSTR locallabel);
	
	LPSTR			ConstString(LPSTR str);
	
	// 暫定コマンド
	void			Asmout(LPSTR buf);

	void			SetLogFile(FILE* fp){ lpLogFP = fp; }
	int			GetErrorCount(void){ return nErrorCount; }
};

#endif
