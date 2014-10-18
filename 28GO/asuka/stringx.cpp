#include "stringx.hpp"

stringx::size_type	stringx::strlen(const char* s){
	int i;
	for(i = 0; s[i] != 0; i++);
	return i;
}

std::string	stringx::substr(size_type pos, size_type n){
	std::string				tmp;
	std::string::iterator	itr = str.begin() + pos;
	tmp.insert(tmp.begin(), itr, itr + n);
	return tmp;
}

std::string	stringx::cut(size_type pos, size_type n){
	std::string				tmp;
	std::string::iterator	itr = str.begin() + pos;
	tmp = substr(pos, n);
	str.erase(itr, itr + n);
	return tmp;
}

std::string	stringx::tolower(){
	std::string				tmp;
	std::string::iterator	itr, itr2;
	for(itr = str.begin(), itr2 = tmp.begin(); itr != str.end(); itr++, itr2++){
		if(*itr >= 'A' && *itr <= 'Z'){
			*itr2 = *itr + 0x20;		// 大文字→小文字変換
		}
	}
	return tmp;
}

std::string	stringx::toupper(){
	std::string				tmp;
	std::string::iterator	itr, itr2;
	for(itr = str.begin(), itr2 = tmp.begin(); itr != str.end(); itr++, itr2++){
		if(*itr >= 'a' && *itr <= 'z'){
			*itr2 = *itr - 0x20;		// 小文字→大文字変換
		}
	}
	return tmp;
}

std::string::size_type	stringx::lastdelimiter(const char* s){
	std::string::iterator	itr;
	int			i, n = strlen(s), last = -1;
	for(itr = str.begin(); itr != str.end(); itr++){
		for(i = 0; i < n; i++){
			if(*itr == s[i]) last = itr - str.begin();
		}
	}
	return last;
}
