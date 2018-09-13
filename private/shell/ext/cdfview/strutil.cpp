//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// string.cpp 
//
//   String functions used by cdfview that are not in shlwapi.h.
//
//   History:
//
//       5/15/97  edwardp   Created.
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//

#include "stdinc.h"

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** StrEqlA *** 
//
//   Compares two ANSI strings for equality.
//
////////////////////////////////////////////////////////////////////////////////
BOOL
StrEqlA(LPCSTR p1, LPCSTR p2)
{
    ASSERT(p1);
    ASSERT(p2);

    while ((*p1 == *p2) && *p1 && *p2)
    {
        p1++; p2++;
    }

    return (*p1 == *p2);
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** StrEqlW *** 
//
//   Compares two WIDE strings for equality.
//
////////////////////////////////////////////////////////////////////////////////
BOOL
StrEqlW(LPCWSTR p1, LPCWSTR p2)
{
    ASSERT(p1);
    ASSERT(p2);

    while ((*p1 == *p2) && *p1 && *p2)
    {
        p1++; p2++;
    }

    return (*p1 == *p2);
}
//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** StrLocallyDisplayable *** 
//
//   Determines if the given wide char string can be displayed on the current
//   system.
//
////////////////////////////////////////////////////////////////////////////////
BOOL
StrLocallyDisplayable(
    LPCWSTR pwsz
)
{
    ASSERT(pwsz);

    BOOL fRet;

    if (0 == WideCharToMultiByte(CP_ACP, 0, pwsz, -1, NULL, 0, NULL, &fRet))
        fRet = TRUE;

    return !fRet;
}

