/*
 * Edit Control Test for ReactOS, quick n' dirty. There you go
 * This source code is in the PUBLIC DOMAIN and has NO WARRANTY.
 * by Waldo Alvarez Cañizares <wac at ghost.matcom.uh.cu>, June 22, 2003.
 */

#include <windows.h>

static const char hexvals[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
VOID  htoa (unsigned int val, char *buf)
{
   int i;
   buf += 7;
   
   for (i=0;i<8;i++)
       {
            *buf-- = hexvals[val & 0x0000000F];
            val = val >> 4;
       }
}


VOID strcpy_(char *dst, const char *src)
{
	const char* p = src;
	while ((*dst++ = *p++)) {}
}

VOID strcpyw_(wchar_t* dst,wchar_t* src)
{
    const wchar_t* p = src;
    while ((*dst++ = *p++)) {}
}
