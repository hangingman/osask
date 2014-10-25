#include <iostream>
#include <fstream>
#include "others.hpp"


int GOLD_getsize(const UCHAR *name)
{
     std::ifstream in(static_cast<const char*>(filename), std::ifstream::ate | std::ifstream::binary);
     return in.tellg();
}

int GOLD_read(const UCHAR *name, int len, UCHAR *b0)
{
/**
	HANDLE h;
	int i;
	h = CreateFileA((char *) name, GENERIC_READ, FILE_SHARE_READ,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (h == INVALID_HANDLE_VALUE)
		goto err;
	ReadFile(h, b0, len, &i, NULL);
	CloseHandle(h);
	if (len != i)
		goto err;
	return 0;
err:
	return 1;
*/
}


