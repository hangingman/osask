#ifndef LL_HPP_
#define LL_HPP_

#include <cstdio>
#include <cstring>
#include <iostream>
#include <memory>
#include <array>
#include <sstream>
#include <bitset>

#if (DEBUG)
	#include "go.hpp"
#endif

typedef unsigned char UCHAR;
typedef unsigned int  UINT;

// on 64 bit sizeof(char*) is 8 and sizeof(int) is 4
// on 32 bit sizeof(char*) is 4 and sizeof(int) is 4
typedef union {
	UINT integer;
	UCHAR byte[4];
} nask32bitInt;

// See also: http://en.cppreference.com/w/cpp/string/byte/memcpy
static nask32bitInt* ucharToNask32bitIntPtr(UCHAR* uchar) {
	std::unique_ptr<nask32bitInt> t(new nask32bitInt[sizeof uchar / 4]);
	std::memcpy(t->byte, uchar, sizeof t->byte);
	return t.get();
};

/** For debug log. Usage ./configure CXXFLAGS=-DDEBUG_BUILD; make */
#if defined(DEBUG) && defined(__GNUC__)
#  define LOG_DEBUG(fmt, ...) printf("%s:%d %s(): " fmt, __FILE__, __LINE__, __func__, ## __VA_ARGS__)
#else
#  define LOG_DEBUG(x, ...) do {} while (0)
#endif

#if defined(TRACE) && defined(__GNUC__)
#  define LOG_TRACE(fmt, ...) printf("%s:%d %s(): " fmt, __FILE__, __LINE__, __func__, ## __VA_ARGS__)
#else
#  define LOG_TRACE(x, ...) do {} while (0)
#endif

/** Always trace log */
#define LOG_INFO(fmt, ...) printf("%s(): " fmt, __func__, ## __VA_ARGS__)

#include  <iomanip>

template <class T, size_t dim>
static void append_buf(std::array<T, dim>& arr, std::stringstream& buf) {
	for (int i = 0; i < dim; i++) {
		buf << "0x";
		buf << std::setfill('0') << std::setw(2) << std::hex;
		buf << static_cast<UINT>(arr[i]);
		if (i+1 != dim) { buf << ',' << ' '; }
	}
}

template <class T, size_t dim>
static void append_buf_pretty(const char* name, std::array<T, dim>& arr, std::stringstream& buf) {
	buf << name << ": [ ";
	append_buf(arr, buf);
	buf << " ]" << std::endl;
}

template <class T>
static std::string dump_ptr(const char* name, T* src) {

	std::stringstream buf;
	buf << name;
	buf << " = [ ";
	while( *src != 0x00 ) {
		buf << "0x" << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(*src);
		buf << ", ";
		src++;
        }
	buf << "]";
	return buf.str();
}

static std::string dump_ptr(const char* name, nask32bitInt* src) {

	std::stringstream buf;
	buf << name;
	buf << " = [ ";
	//while( (*src).byte[0] != 0x00 ) {
	for(int i = 0; i < 8; i++) {
		buf << "0x";
		buf << std::setfill('0') << std::setw(2) << std::hex
		    << static_cast<UINT>((*src).byte[0])
		    << ", 0x";
		buf << std::setfill('0') << std::setw(2) << std::hex
		    << static_cast<UINT>((*src).byte[1])
		    << ", 0x";
		buf << std::setfill('0') << std::setw(2) << std::hex
		    << static_cast<UINT>((*src).byte[2])
		    << ", 0x";
		buf << std::setfill('0') << std::setw(2) << std::hex
		    << static_cast<UINT>((*src).byte[3])
		    << ", ";
		src++;
	}
	buf << "]";
	return buf.str();
}

template <class T>
static std::string dump_bit(T src) {

	std::stringstream buf;
	buf << static_cast<std::bitset<8> >(src);
	return buf.str();
}

static std::string dump_getparam(std::string& bit) {

	std::stringstream buf;
	std::string data_width = bit.substr(0,3);
	std::string data_type  = bit.substr(3,2);
	std::string data_range = bit.substr(5,2);
	std::string use_dollar = bit.substr(7,1);

	buf << bit << std::endl;
	buf << "datawidth:\t" << std::bitset<12>(data_width).to_ulong() << "byte" << std::endl;
	int tp = std::bitset<8>(data_type).to_ulong();
	int rg = std::bitset<8>(data_range).to_ulong();

	switch (tp) {
	case 0:
		buf << "type:\t\treg" << std::endl;
		break;
	case 1:
		buf << "type:\t\tmem" << std::endl;
		break;
	case 2:
		buf << "type:\t\timm" << std::endl;
		break;
	}

	switch (rg) {
	case 0:
		buf << "range:\t\tdefault" << std::endl;
		break;
	case 1:
		buf << "range:\t\tshort" << std::endl;
		break;
	case 2:
		buf << "range:\t\tnear" << std::endl;
		break;
	case 3:
		buf << "range:\t\tfar" << std::endl;
		break;
	}

	switch (std::bitset<4>(use_dollar).to_ulong()) {
		case 0:
			buf << "use_dollar:\tfalse" << std::endl;
			break;
		case 1:
			buf << "use_dollar:\ttrue" << std::endl;
			break;
	}

	return buf.str();
}

static std::string dump_argv(char** argv) {

	std::stringstream buf;
	buf << '{';

	while( *argv != NULL ){
		buf << *argv;
		buf << ", ";
		argv++;
        }
	buf << '}';
	return buf.str();
}

constexpr unsigned int INVALID_DELTA = 0x40000000;
constexpr size_t       MAXSIGMAS     = 4;

constexpr unsigned int VFLG_ERROR    = 0x01; /* エラー */
constexpr unsigned int VFLG_SLFREF   = 0x02; /* 自己参照エラー */
constexpr unsigned int VFLG_UNDEF    = 0x04; /* 未定義エラー */
constexpr unsigned int VFLG_EXTERN   = 0x10; /* 外部ラベル */
constexpr unsigned int VFLG_CALC     = 0x20; /* 計算中 */
constexpr unsigned int VFLG_ENABLE   = 0x40; /* STR_LABELで有効なことを示す */

static constexpr int nask_maxlabels  = 64 * 1024; /* 64K個(LL:88*64k) */

struct STR_SIGMA {
	int scale;
	unsigned int subsect, terms;
     	STR_SIGMA& operator=(std::unique_ptr<STR_SIGMA>& value) {
		this->scale = value->scale;
		this->subsect = value->subsect;
		this->terms = value->terms;
		return *this;
     	}
};

struct STR_VALUE {
	int min;
	unsigned int delta, flags;
	std::array<int, 2> scale;
	std::array<unsigned int, 2> label;
	std::array<struct STR_SIGMA, MAXSIGMAS> sigma;

     	STR_VALUE& operator=(std::unique_ptr<STR_VALUE>& value) {
		this->min = value->min;
		this->delta = value->delta;
		this->flags = value->flags;
		this->scale = value->scale;
		this->label = value->label;
		this->sigma = value->sigma;
		return *this;
	}
	//STR_VALUE& assign_sigma(size_t index, std::unique_ptr<STR_SIGMA>& value) {
	// 	this->sigma[index] = value;
	// 	return *this;
	//}
};

struct STR_LABEL {
	struct STR_VALUE value;
	UCHAR *define; /* これがNULLだと、extlabel */
	//STR_LABEL& assign_value(std::unique_ptr<STR_VALUE>& value) {
	// 	this->value = value;
	// 	return *this;
	//}
};

struct STR_SUBSECTION {
	unsigned int min, delta, unsolved; /* unsolved == 0 なら最適化の必要なし */
	UCHAR *sect0, *sect1;
};

static struct STR_LABEL* label0;
static struct STR_SUBSECTION* subsect0;

static std::array<UCHAR, 7> table98typlen = { 0x38, 0x38, 0x39, 0x39, 0x3b, 0x3b, 0x38 };
static std::array<UCHAR, 7> table98range  = { 0x00, 0x02, 0x00, 0x03, 0x00, 0x03, 0x03 };

struct STR_LL_VB {
	UCHAR *expr, typlen, range;
};

UCHAR *LL_skip_expr(UCHAR *p);
UCHAR *LL_skip_mc30(UCHAR *s, UCHAR *bytes, char flag);
UCHAR *LL_skipcode(UCHAR *p);

unsigned int solve_subsection(struct STR_SUBSECTION *subsect, char force);
UCHAR *skip_mc30(UCHAR *s, UCHAR *bytes, char flag);
void init_value(STR_VALUE* value);
void calcsigma(std::unique_ptr<STR_VALUE>& value);
void addsigma(std::unique_ptr<STR_VALUE>& value, struct STR_SIGMA sigma);
void calc_value0(std::unique_ptr<STR_VALUE>& value, UCHAR **pexpr);

/* ラベルの定義方法:
	一般式
	80 variable-sum, 0000bbaa(aa:項数-1, bb:最初の番号),
	84〜87 sum, i - 1, expr, expr, ...

  ・80〜8f:LLが内部処理用に使う
	80〜83:variable参照(1〜4バイト)
	88〜8f:sum(variable), (1〜4, 1〜4) : 最初は項数-1, 次は最初の番号
		{ "|>", 12, 18 }, { "&>", 12, 17 },
		{ "<<", 12, 16 }, { ">>", 12, 17 },
		{ "//", 14,  9 }, { "%%", 14, 10 },
		{ "+",  13,  4 }, { "-",  13,  5 },
		{ "*",  14,  6 }, { "/",  14,  7 },
		{ "%",  14,  8 }, { "^",   7, 14 },
		{ "&",   8, 12 }, { "|",   6, 13 },
		{ "",    0,  0 }

	s+
	s-
	s~

	+, -, *, <<, /u, %u, /s, %s
	>>u, >>s, &, |, ^


	< 文法 >

最初はヘッダ。
・ヘッダサイズ(DW) = 12
・バージョンコード(DW)
・ラベル総数(DW)


  ・38:式の値をDBにして設置
  ・39:式の値をDWにして設置
  ・3a:式の値を3バイトで設置
  ・3b:式の値をDDにして設置
  以下、・3fまである。
  ・40〜47:ブロックコマンド。if文などの後続文をブロック化する(2〜9)。
  ・48:バイトブロック（ブロック長がバイトで後続）。
  ・49:ワードブロック。
  ・4a:バイトブロック。
  ・4b:ダブルワードブロック。
  ・4c:排他的if開始。
  ・4d:選択的if開始。
  ・4e:選択的バウンダリーif開始。変数設定の直後、バウンダリー値が続く。
  ・4f:endif。
  排他的ifは、endifが来るまでいくつも並べられる。endifが来るまで、
  全てelse-ifとして扱われる。最後にelseを作りたければ、条件を定数1にせよ。
  ・ターミネーターはラベル定義で0xffffffff。

  ・58:ORG

  ・60:アライン。バイトの埋め方は個別に設定する。・・・これは排他的ifでも記述できる。

  ・70〜77:可変長バイト宣言(文法上では40〜4bが後続することを許すが、サポートしていない。許されるのは30〜3f)
  ・78〜7f:可変長バイト参照

  ・80〜8f:LLが内部処理用に使う
	80〜83:variable参照(1〜4バイト)
	88〜8f:sum(variable), (1〜4, 1〜4) : 最初は項数-1, 次は最初の番号


  if文中では、可変長バイト宣言しかできない。

	ORGについて。本来引数は式であってよいが、このバージョンでは定数式を仮定している。




*/

/* ibuf, obufの基礎構造 */
/* シグネチャー8バイト, 総長4バイト, リザーブ4バイト */
/* メインデータレングス4B, メインデータスタート4B */

/* ↑こういうややこしいことはやらない */
/* スキップコマンドを作って対処する */

#endif /* LL_HPP_ */
