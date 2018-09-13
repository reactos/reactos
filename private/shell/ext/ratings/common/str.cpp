#include "npcommon.h"

// strstrf(str, srch)
//
// Returns a pointer to the first occurrence of srch within
// str (like strchrf, but search parameter is a string, not
// a single character).  Returns NULL if not found.
// REVIEW: simple algorithm here, but depending on usage,
// might be overkill to complicate it.

LPSTR WINAPI strstrf(LPCSTR lpString, LPCSTR lpSearch)
{
    INT cbSearch = strlenf(lpSearch);
    INT cbToSearch;
    LPSTR lp;

    // calculate the maximum distance to go -- the length
    // of the string to look in less the length of the
    // string to search for, since beyond that the string
    // being searched for would not fit.
    cbToSearch = strlenf(lpString) - cbSearch;
    if (cbToSearch < 0)
        return NULL;    /* string being searched is shorter */

    for (lp = (LPSTR)lpString; lp - lpString <= cbToSearch; ADVANCE(lp)) {
        if (strncmpf(lp, lpSearch, cbSearch) == 0)
            return lp;
    }

    return NULL;
}


// stristrf(str, srch)
//
// Returns a pointer to the first occurrence of srch within
// str, case-insensitive.  Returns NULL if not found.
// REVIEW: simple algorithm here, but depending on usage,
// might be overkill to complicate it.

LPSTR WINAPI stristrf(LPCSTR lpString, LPCSTR lpSearch)
{
    INT cbSearch = strlenf(lpSearch);
    INT cbToSearch;
    LPSTR lp;

    // calculate the maximum distance to go -- the length
    // of the string to look in less the length of the
    // string to search for, since beyond that the string
    // being searched for would not fit.
    cbToSearch = strlenf(lpString) - cbSearch;
    if (cbToSearch < 0)
        return NULL;    /* string being searched is shorter */

    for (lp = (LPSTR)lpString; lp - lpString <= cbToSearch; ADVANCE(lp)) {
        if (strnicmpf(lp, lpSearch, cbSearch) == 0)
            return lp;
    }

    return NULL;
}
