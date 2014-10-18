/*
	ファイルパス管理クラス　〜filepath.h〜		Ver.[2000/02/11]
*/
#ifndef	__FILEPATH_H
#define	__FILEPATH_H

#pragma warning(disable:4786)

#include <cstdio>
#include <cstdlib>
#include <list>
#include <map>
#include <stack>

#include "stringx.hpp"

class filepath{
  public:
	std::string	drive;
	std::string	path;
	std::string	name;
	std::string	extention;

	void	set(std::string&);
	void	set(char* s){ std::string t=s; set(t);}
	filepath&	operator=(std::string& s){ set(s); return *this; }
	filepath&	operator=(char* s){ set(s); return *this; }
	std::string	operator()(){ return getfullname(); }
			operator std::string(){ return getfullname(); }
	std::string	getfullpath();
	std::string	getfilename();
	std::string	getfullname();
};

#endif
