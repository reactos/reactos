#include "npcommon.h"

void InitSpn(
    char *abBits,
    char *abDBCBits,
    LPCSTR lpSpn
)
{
    LPCSTR lpCur;

    ::memset(abBits, '\0', 256/8);
    if (::fDBCSEnabled)
        ::memset(abDBCBits, '\0', 256/8);

    for (lpCur = lpSpn; *lpCur; ADVANCE(lpCur)) {
        if (IS_LEAD_BYTE(*lpCur)) {
            char chXOR = *lpCur ^ *(lpCur+1);
            SPN_SET(abDBCBits, chXOR);
        }
        else
            SPN_SET(abBits, *lpCur);
    }
}

// strspn(str, spn)
//
// Returns count of leading characters in str which exist
// in spn;  equivalent to returning the index of the first
// character which is not in spn.

UINT WINAPI strspnf(LPCSTR lpString, LPCSTR lpSpn)
{
    char abBits[256/8];
    char abDBCBits[256/8];
    LPCSTR lpCur;

    InitSpn(abBits, abDBCBits, lpSpn);

    for (lpCur = lpString; *lpCur; ADVANCE(lpCur)) {
        if (IS_LEAD_BYTE(*lpCur)) {
            char chXOR = *lpCur ^ *(lpCur + 1);
            if (!SPN_TEST(abDBCBits, chXOR) ||
                (strchrf(lpSpn, *(LPWORD)lpCur) == NULL))
                break;
        }
        else if (!SPN_TEST(abBits, *lpCur))
            break;
    }

    return (UINT) (lpCur - lpString);
}


// strcspn(str, spn)
//
// Returns count of leading characters in str which do not
// exist in spn;  equivalent to returning the index of the
// first character which is in spn.

UINT WINAPI strcspnf(LPCSTR lpString, LPCSTR lpSpn)
{
    char abBits[256/8];
    char abDBCBits[256/8];
    LPCSTR lpCur;

    InitSpn(abBits, abDBCBits, lpSpn);

    for (lpCur = lpString; *lpCur; ADVANCE(lpCur)) {
        if (IS_LEAD_BYTE(*lpCur)) {
            char chXOR = *lpCur ^ *(lpCur + 1);
            if (SPN_TEST(abDBCBits, chXOR) &&
                (strchrf(lpSpn, *(LPWORD)lpCur) != NULL))
                break;
        }
        else if (SPN_TEST(abBits, *lpCur))
            break;
    }
    return (UINT)(lpCur-lpString);
}

