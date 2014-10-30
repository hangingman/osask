//*****************************************************************************
// memcpy.c : memory function
// 2002/02/04 by Gaku : this is rough sketch
//*****************************************************************************

#include <cstddef>
#include "go_string.hpp"

//=============================================================================
// copy SZ bytes of S to D
//=============================================================================
void* GO_memcpy (void *d, const void *s, size_t sz)
{
	void *tmp = d;
	char *dp = (char*)d;
	char *sp = (char*)s;

	while (sz--)
		*dp++ = *sp++;

	return tmp;
}
