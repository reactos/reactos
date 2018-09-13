#include <windows.h>
#include <port1632.h>
#include "recordll.h"
#include "recorder.h"


LPVOID lmemcpy(LPVOID dst, LPVOID src, INT count)
{
LPSTR bsrc = (LPSTR)src;
LPSTR bdst = (LPSTR)dst;

	if (bdst <= bsrc || bdst >= (bsrc + count)) {
		/*
		 * Non-Overlapping Buffers
		 * copy from lower addresses to higher addresses
		 */
		while (count--)
			*bdst++ = *bsrc++;
		}
	else {
		/*
		 * Overlapping Buffers
		 * copy from higher addresses to lower addresses
		 */
		bdst += count - 1;
		bsrc += count - 1;

		while (count--)
			*bdst-- = *bsrc--;
		}

	return(dst);
}


INT APIENTRY lstrncmpi( LPSTR lpstr1, LPSTR lpstr2, INT ci )
{
INT iret=0;
CHAR c1, c2;
LPSTR lpc1=&c1;
LPSTR lpc2=&c2;

    while( ci-- ) {

	c1 = *lpstr1;
	c2 = *lpstr2;

	AnsiUpperBuff(lpc1, 1);
	AnsiUpperBuff(lpc2, 1);

	if (c1 < c2) iret--;
	if (c1 > c2) iret++;

	if ( iret ) break;	/* If we got the info, outta here */

	lpstr1 = AnsiNext( lpstr1 );
	lpstr2 = AnsiNext( lpstr2 );
	}

    return iret;

}


INT APIENTRY lstrlen( LPSTR lpstr )
{
INT i=0;
LPSTR lpStart=lpstr;

    while(  *lpstr != '\0' ) {
	lpstr = AnsiNext( lpstr );
	i++;
	}

    lpstr = lpStart;

    return i;

}


LPSTR APIENTRY lstrcpy( LPSTR lpdest, LPSTR lpsrc )
{
LPSTR lpdestStart=lpdest;
LPSTR lpsrcStart=lpsrc;

    while( *lpsrc != '\0' ) {
	*lpdest = *lpsrc;
	 lpsrc	= AnsiNext( lpsrc );
	 lpdest = AnsiNext( lpdest );
	 }

    *lpdest = '\0';

    lpdest = lpdestStart;
    lpsrc  = lpsrcStart;

    return lpdest;

}
