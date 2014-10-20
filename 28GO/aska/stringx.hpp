/*
  文字列操作クラス　〜stringx.h〜		Ver.[2000/02/11]
*/
#ifndef	__STRINGX_H
#define	__STRINGX_H

#pragma warning(disable:4786)

#include <cstdio>
#include <cstdlib>
#include <ctype.h>
#include <list>
#include <map>
#include <stack>
#include <string>

class stringx {

public:
     typedef std::string::size_type size_type;

protected:
     std::string	str;

// 文字列操作スタティックルーチン群
public:
     static size_type	strlen(const char* s);

// std::string操作ルーチン群
public:
     void		set(std::string s){ str=s; }
     std::string	get(){ return str; }
     stringx&		operator=(std::string& s){ set(s); return *this; }
     stringx&		operator=(char* s){ set(s); return *this; }
     std::string	operator()(){ return get(); }
     operator 		std::string(){ return get(); }
	
     std::string	substr(size_type pos, size_type n);
     std::string	copy(size_type pos, size_type n){ return substr(pos, n); }
     std::string	cut(size_type pos, size_type n);
     std::string	left(size_type pos){ return substr(0, pos); }
     std::string	right(size_type pos){ return substr(pos+1, str.size()-pos-1); }
     std::string	tolower();
     std::string	toupper();
     size_type		lastdelimiter(const char* s);
     long		tolong(){ return atol(str.c_str()); }
     double		todouble(){ return atof(str.c_str()); }
};

#endif
