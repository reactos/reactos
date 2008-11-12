#ifndef _DDRAWTESTLIST_H
#define _DDRAWTESTLIST_H

#include "ddrawapi.h"
void dump_ddrawi_directdraw_int(LPDDRAWI_DIRECTDRAW_INT lpDraw_int);

/* dump all data struct when this is trun onm usefull when u debug ddraw.dll */
#define DUMP_ON 1

/* include the tests */
#include "tests/Test_DirectDrawCreateEx.c"








/* The List of tests */
TESTENTRY TestList[] =
{
    { L"DirectDrawCreateEx", Test_DirectDrawCreateEx }
};

/* The function that gives us the number of tests */
INT NumTests(void)
{
	return sizeof(TestList) / sizeof(TESTENTRY);
}

/* old debug macro and dump data */


void dump_ddrawi_directdraw_int(LPDDRAWI_DIRECTDRAW_INT lpDraw_int)
{
    printf("%08lx pIntDirectDraw7->lpVtbl : 0x%p\n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_INT, lpVtbl), lpDraw_int->lpVtbl);
    printf("%08lx pIntDirectDraw7->lpLcl : 0x%p\n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_INT, lpLcl), lpDraw_int->lpLcl );
    printf("%08lx pIntDirectDraw7->lpLink : 0x%p\n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_INT, lpLink), lpDraw_int->lpLink );
    printf("%08lx pIntDirectDraw7->dwIntRefCnt : 0x%08lx \n", FIELD_OFFSET(DDRAWI_DIRECTDRAW_INT, dwIntRefCnt), lpDraw_int->dwIntRefCnt );
}
#endif

/* EOF */
