#include "stdafx.h"
#include "utils.h"

void lreverse( LPSTR s )
{
    LPSTR p1, p2;
    char c;

    p1 = s;
    p2 = (LPSTR)(p1+ lstrlen((LPSTR) p1 ) - 1 );

    while( p1 < p2 ) {
        c = *p1;
        *p1++ = *p2;
        *p2-- = c;
    }
}

void litoa( int n, LPSTR s )
{
    LPSTR pS;
    BOOL bNeg = FALSE;

    if (n < 0) {
        n = -n;
        bNeg = TRUE;
    }
    pS = s;
    do {
        *pS = n % 10 + '0';
        pS++;
    } while ( (n /= 10) != 0 );

    if (bNeg) {
        *pS = '-';
        pS++;
    }
    *pS = '\0';
    lreverse( s );
}