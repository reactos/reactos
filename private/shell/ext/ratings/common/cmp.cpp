#include "npcommon.h"

#ifdef strcmpf

#undef strcmpf
int WINAPI strcmpf(LPCSTR lpStr1, LPCSTR lpStr2)
{
    return lstrcmp(lpStr1, lpStr2);
}

#else

// strcmpf(str1, str2)
//
// Returns -1 if str1 is lexically less than str2
// Returns 0 if str1 is equal to str2
// Returns 1 if str1 is lexically greater than str2

int WINAPI strcmpf(LPCSTR lpStr1, LPCSTR lpStr2)
{
    for (; *lpStr1 && *lpStr2; ADVANCE(lpStr1), ADVANCE(lpStr2)) {
        UINT ch1, ch2;
        UINT nCmp;

        // for same-width chars, compare straight;
        // for DBC vs. SBC, compare 0xttll against 0x00ss
        ch1 = IS_LEAD_BYTE(*lpStr1) ? *(LPWORD)lpStr1 : *lpStr1;
        ch2 = IS_LEAD_BYTE(*lpStr2) ? *(LPWORD)lpStr2 : *lpStr2;

        if (ch1 > 0xff || ch2 > 0xff)
            nCmp = (ch1 < ch2) ? -1 : ((ch1 == ch2) ? 0 : 1);
        else
        {
            if (CollateTable[ch1] == CollateTable[ch2])
                nCmp = (ch1 < ch2) ? -1 : ((ch1 == ch2) ? 0 : 1);
            else
                nCmp = (CollateTable[ch1] < CollateTable[ch2]) ? -1 : 1;
        }

        if (nCmp != 0)
            return nCmp;
    }

    // end of one string or the other.  if different lengths,
    // shorter one must be lexically less, so it's ok to just
    // compare bytes.
    return (*lpStr1 > *lpStr2) ? -1 : (*lpStr1 == *lpStr2) ? 0 : 1;
}

#endif  /* ifndef strcmpf */


#ifdef stricmpf

#undef stricmpf
int WINAPI stricmpf(LPCSTR lpStr1, LPCSTR lpStr2)
{
    return lstrcmpi(lpStr1, lpStr2);
}

#else

// stricmpf(str1, str2)
//
// Returns -1 if str1 is lexically less than str2
// Returns 0 if str1 is equal to str2
// Returns 1 if str1 is lexically greater than str2
// All comparisons are case-insensitive

int WINAPI stricmpf(LPCSTR lpStr1, LPCSTR lpStr2)
{
    for (; *lpStr1 && *lpStr2; ADVANCE(lpStr1), ADVANCE(lpStr2)) {
        UINT ch1, ch2;
        UINT nCmp;

        // for same-width chars, compare straight;
        // for DBC vs. SBC, compare 0xttll against 0x00ss
        ch1 = IS_LEAD_BYTE(*lpStr1) ? *(LPWORD)lpStr1 : *lpStr1;
        ch2 = IS_LEAD_BYTE(*lpStr2) ? *(LPWORD)lpStr2 : *lpStr2;

        if (ch1 > 0xff || ch2 > 0xff)
            nCmp = (ch1 < ch2) ? -1 : ((ch1 == ch2) ? 0 : 1);
        else
        {
            ch1 = ToUpperCaseTable[ch1];
            ch2 = ToUpperCaseTable[ch2];
            if (CollateTable[ch1] == CollateTable[ch2])
                nCmp = 0;
            else
                nCmp = (CollateTable[ch1] < CollateTable[ch2]) ? -1 : 1;
        }

        if (nCmp != 0)
            return nCmp;
    }

    // end of one string or the other.  if different lengths,
    // shorter one must be lexically less, so it's ok to just
    // compare bytes.
    return (*lpStr1 > *lpStr2) ? -1 : (*lpStr1 == *lpStr2) ? 0 : 1;
}

#endif  /* ifndef stricmpf */


// strncmpf(str1, str2, cb)
//
// Returns -1 if str1 is lexically less than str2
// Returns 0 if str1 is equal to str2
// Returns 1 if str1 is lexically greater than str2
// At most cb bytes are compared before returning

int WINAPI strncmpf(LPCSTR lpStr1, LPCSTR lpStr2, UINT cb)
{
    LPCSTR lp1 = lpStr1;

    for (; *lp1 && *lpStr2; ADVANCE(lp1), ADVANCE(lpStr2)) {
        UINT ch1, ch2;
        UINT nCmp;

        // see if we've reached the byte limit.  only need
        // to compare one string for length, since if they
        // get out of sync (DBCS only), we will get a compare
        // error immediately.
        if ((UINT)(lp1 - lpStr1) >= cb)
            return 0;   // no failures, reached limit

        // for same-width chars, compare straight;
        // for DBC vs. SBC, compare 0xttll against 0x00ss
        ch1 = IS_LEAD_BYTE(*lp1) ? *(LPWORD)lp1 : *lp1;
        ch2 = IS_LEAD_BYTE(*lpStr2) ? *(LPWORD)lpStr2 : *lpStr2;

        nCmp = (ch1 < ch2) ? -1 : ((ch1 == ch2) ? 0 : 1);

        if (nCmp != 0)
            return nCmp;
    }

    // end of one string or the other.  check the length to see if
    // we have compared as many bytes as needed.
    if ((UINT)(lp1 - lpStr1) >= cb)
        return 0;   // no failures, reached limit

    // end of one string or the other.  if different lengths,
    // shorter one must be lexically less, so it's ok to just
    // compare bytes.
    return (*lp1 > *lpStr2) ? -1 : (*lp1 == *lpStr2) ? 0 : 1;
}


// strnicmpf(str1, str2, cb)
//
// Returns -1 if str1 is lexically less than str2
// Returns 0 if str1 is equal to str2
// Returns 1 if str1 is lexically greater than str2
// All comparisons are case-insensitive
// At most cb bytes are compared

int WINAPI strnicmpf(LPCSTR lpStr1, LPCSTR lpStr2, UINT cb)
{
    LPCSTR lp1 = lpStr1;

    for (; *lp1 && *lpStr2; ADVANCE(lp1), ADVANCE(lpStr2)) {
        UINT ch1, ch2;
        UINT nCmp;

        // see if we've reached the byte limit.  only need
        // to compare one string for length, since if they
        // get out of sync (DBCS only), we will get a compare
        // error immediately.
        if ((UINT)(lp1 - lpStr1) >= cb)
            return 0;   // no failures, reached limit

        // for same-width chars, compare straight;
        // for DBC vs. SBC, compare 0xttll against 0x00ss
        ch1 = IS_LEAD_BYTE(*lp1) ? *(LPWORD)lp1 : PtrToUlong(CharUpper((LPTSTR) *((BYTE *)lp1)));
        ch2 = IS_LEAD_BYTE(*lpStr2) ? *(LPWORD)lpStr2 : PtrToUlong(CharUpper((LPTSTR) *((BYTE *)lpStr2)));

        nCmp = (ch1 < ch2) ? -1 : ((ch1 == ch2) ? 0 : 1);

        if (nCmp != 0)
            return nCmp;
    }

    // end of one string or the other.  check the length to see if
    // we have compared as many bytes as needed.
    if ((UINT)(lp1 - lpStr1) >= cb)
        return 0;   // no failures, reached limit

    // end of one string or the other.  if different lengths,
    // shorter one must be lexically less, so it's ok to just
    // compare bytes.
    return (*lp1 > *lpStr2) ? -1 : (*lp1 == *lpStr2) ? 0 : 1;
}

