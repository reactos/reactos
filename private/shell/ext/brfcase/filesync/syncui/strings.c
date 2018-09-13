//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1993-1994
//
// File: string.c
//
//  This files contains common string routines
//
// History:
//  10-09-93 ScottH     Created
//
//---------------------------------------------------------------------------

/////////////////////////////////////////////////////  INCLUDES

#include "brfprv.h"         // common headers
#include "strings.h"


#ifdef NOTUSED      
#pragma data_seg(DATASEG_PERINSTANCE)

static LPTSTR s_pszNextToken = NULL;        

#pragma data_seg()
#endif // NOTUSED


// Some of these are replacements for the C runtime routines.
//  This is so we don't have to link to the CRT libs.
//

// WARNING: all of these APIs do not setup DS, so you can not access
// any data in the default data seg of this DLL.
//
// do not create any global variables... talk to chrisg if you don't
// understand this


/*----------------------------------------------------------
Purpose: Case sensitive character comparison for DBCS

Returns: FALSE if they match, TRUE if no match
Cond:    --
*/
BOOL ChrCmp(
    WORD w1, 
    WORD wMatch)
    {
    /* Most of the time this won't match, so test it first for speed.
    */
    if (LOBYTE(w1) == LOBYTE(wMatch))
        {
        if (IsDBCSLeadByte(LOBYTE(w1)))
            {
            return(w1 != wMatch);
            }
        return FALSE;
        }
    return TRUE;
    }


#ifdef NOTUSED      // BUGBUG: this is not DBCS aware
/*----------------------------------------------------------
Purpose: strtok

         Swiped from the C 7.0 runtime sources.

Returns: 
Cond:    
*/
LPTSTR PUBLIC StrTok(
    LPTSTR psz,
    LPCTSTR rgchTokens)
    {
    TUCHAR map[32];
    LPTSTR pszToken;
    
    ZeroInit(map, map);

    do 
        {
        map[*rgchTokens >> 3] |= (1 << (*rgchTokens & 7));
        } while (*rgchTokens++);

    if (!psz)
        {
        ENTEREXCLUSIVE()
            {
            psz = s_pszNextToken;
            }
        LEAVEEXCLUSIVE()
        }

    while (map[*psz >> 3] & (1 << (*psz & 7)) && *psz)
        psz++;
    pszToken = psz;
    for (;; psz++)
        {
        if (map[*psz >> 3] & (1 << (*psz & 7)))
            {
            if (!*psz && psz == pszToken)
                return(NULL);
            if (*psz)
                *psz++ = TEXT('\0');

            ENTEREXCLUSIVE()
                {
                g_pszNextToken = psz;
                }
            LEAVEEXCLUSIVE()
            return pszToken;
            }
        }
    }
#endif


#if 0
/*----------------------------------------------------------
Purpose: Find first occurrence of character in string

Returns: Pointer to the first occurrence of ch in 
Cond:    --
*/
LPTSTR PUBLIC StrChr(
    LPCTSTR psz, 
    WORD wMatch)
    {
    for ( ; *psz; psz = CharNext(psz))
        {
        if (!ChrCmp(*(WORD  *)psz, wMatch))
            return (LPTSTR)psz;
        }
    return NULL;
    }
#endif

/*----------------------------------------------------------
Purpose: Get a string from the resource string table.  Returned
         ptr is a ptr to static memory.  The next call to this
         function will wipe out the prior contents.
Returns: Ptr to string
Cond:    --
*/
LPTSTR PUBLIC SzFromIDS(
    UINT ids,               // resource ID
    LPTSTR pszBuf,
    UINT cchBuf)           
    {
    ASSERT(pszBuf);

    *pszBuf = NULL_CHAR;
    LoadString(g_hinst, ids, pszBuf, cchBuf);
    return pszBuf;
    }


/*----------------------------------------------------------
Purpose: Formats a string by allocating a buffer and loading
         the given resource strings to compose the string.

Returns: the count of characters 

Cond:    Caller should free the allocated buffer using GFree.
*/
BOOL PUBLIC FmtString(
    LPCTSTR  * ppszBuf,
    UINT idsFmt,
    LPUINT rgids,
    UINT cids)
    {
    UINT cch = 0;
    UINT cchMax;
    LPTSTR pszBuf;

    ASSERT(ppszBuf);
    ASSERT(rgids);
    ASSERT(cids > 0);

    cchMax = (1+cids) * MAXPATHLEN;
    pszBuf = GAlloc(CbFromCch(cchMax));
    if (pszBuf)
        {
        // The first cids DWORDS are the addresses of the offset strings
        // in the buffer (passed to wvsprintf)
        LPBYTE pszMsgs = GAlloc((cids * sizeof(DWORD_PTR)) + (cids * CbFromCch(MAXPATHLEN)));
        if (pszMsgs)
            {
            TCHAR szFmt[MAXPATHLEN];
            DWORD_PTR *rgpsz = (DWORD_PTR*)pszMsgs;
            LPTSTR pszT = (LPTSTR)(pszMsgs + (cids * sizeof(DWORD_PTR)));
            UINT i;

            // Load the series of strings
            for (i = 0; i < cids; i++, pszT += MAXPATHLEN)
                {
                rgpsz[i] = (DWORD_PTR)pszT;
                SzFromIDS(rgids[i], pszT, MAXPATHLEN);
                }

            // Compose the string
            SzFromIDS(idsFmt, szFmt, ARRAYSIZE(szFmt));
            cch = FormatMessage(FORMAT_MESSAGE_FROM_STRING,
                          szFmt, 0, 0, pszBuf, cchMax, (va_list *)&rgpsz);
            ASSERT(cch <= cchMax);

            GFree(pszMsgs);
            }
        // pszBuf is freed by caller
        }

    *ppszBuf = pszBuf;
    return cch;
    }
