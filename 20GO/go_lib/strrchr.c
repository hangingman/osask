//*****************************************************************************
// strrchr.c : string function
// 2002/02/04 by Gaku : this is rough sketch
//*****************************************************************************

#include <stddef.h>

//=============================================================================
// search the last occur of C in D
//=============================================================================
char* GO_strrchr (char *d, int c)
{
	char *tmp = d;

	while ('\0' != *d)
		d++;

	while (tmp <= d) {
		if (c == *d)
			return d;
		d--;
	}

	return NULL;
}
