//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1993-1994
//
// File: state.c
//
//  This file contains the state & .ini file routines 
//
// History:
//  08-06-93 ScottH     Transferred from twin code
//
//---------------------------------------------------------------------------

/////////////////////////////////////////////////////  INCLUDES

#include "brfprv.h"     // common headers

/////////////////////////////////////////////////////  TYPEDEFS

#ifdef DEBUG

// Some of the .ini processing code was pimped from the sync engine.
//

typedef struct _INIKEYHEADER
    {
    LPCTSTR pszSectionName;
    LPCTSTR pszKeyName;
    LPCTSTR pszDefaultRHS;
    } INIKEYHEADER;

typedef struct _BOOLINIKEY
    {
    INIKEYHEADER ikh;
    LPUINT puStorage;
    DWORD dwFlag;
    } BOOLINIKEY;

typedef struct _INTINIKEY
    {
    INIKEYHEADER ikh;
    LPUINT puStorage;
    } INTINIKEY;

#endif


// Some of these macros taken from prefs.h in Pen project...
//
#define PutIniIntCmp(idsSection, idsKey, nNewValue, nSave) \
    if ((nNewValue) != (nSave)) PutIniInt(idsSection, idsKey, nNewValue)

#define WritePrivateProfileInt(szApp, szKey, i, lpFileName) \
    {TCHAR sz[7]; \
    WritePrivateProfileString(szApp, szKey, SzFromInt(sz, i), lpFileName);}


/////////////////////////////////////////////////////  MODULE DATA

#ifdef DEBUG

#pragma data_seg(DATASEG_PERINSTANCE)

// Array of keys with Integer RHSs to be processed by ProcessIniFile() 

static INTINIKEY s_rgiik[] = 
    {
        {
        { c_szIniSecDebugUI,    c_szIniKeyTraceFlags, c_szZero },
        &g_uTraceFlags
        },

        {
        { c_szIniSecDebugUI,    c_szIniKeyDumpFlags, c_szZero },
        &g_uDumpFlags
        },

    };

// Array of keys with Boolean RHSs to be processed by ProcessIniFile() 

static BOOLINIKEY s_rgbik[] =
    {
        {
        { c_szIniSecDebugUI,    c_szIniKeyBreakOnOpen, c_szZero },
        &g_uBreakFlags,
        BF_ONOPEN
        },

        {
        { c_szIniSecDebugUI,    c_szIniKeyBreakOnClose, c_szZero },
        &g_uBreakFlags,
        BF_ONCLOSE
        },

        {
        { c_szIniSecDebugUI,    c_szIniKeyBreakOnRunOnce, c_szZero },
        &g_uBreakFlags,
        BF_ONRUNONCE
        },

        {
        { c_szIniSecDebugUI,    c_szIniKeyBreakOnValidate, c_szZero },
        &g_uBreakFlags,
        BF_ONVALIDATE
        },

        {
        { c_szIniSecDebugUI,    c_szIniKeyBreakOnThreadAtt, c_szZero },
        &g_uBreakFlags,
        BF_ONTHREADATT
        },

        {
        { c_szIniSecDebugUI,    c_szIniKeyBreakOnThreadDet, c_szZero },
        &g_uBreakFlags,
        BF_ONTHREADDET
        },

        {
        { c_szIniSecDebugUI,    c_szIniKeyBreakOnProcessAtt, c_szZero },
        &g_uBreakFlags,
        BF_ONPROCESSATT
        },

        {
        { c_szIniSecDebugUI,    c_szIniKeyBreakOnProcessDet, c_szZero },
        &g_uBreakFlags,
        BF_ONPROCESSDET
        },
    };

#pragma data_seg()


/* Boolean TRUE strings used by IsIniYes() (comparison is case-insensitive) */

static LPCTSTR s_rgpszTrue[] =
    {
    TEXT("1"),
    TEXT("On"),
    TEXT("True"),
    TEXT("Y"),
    TEXT("Yes")
    };

/* Boolean FALSE strings used by IsIniYes() (comparison is case-insensitive) */

static LPCTSTR s_rgpszFalse[] =
    {
    TEXT("0"),
    TEXT("Off"),
    TEXT("False"),
    TEXT("N"),
    TEXT("No")
    };

#endif  // DEBUG


/////////////////////////////////////////////////////  PROCEDURES


#ifdef DEBUG

/*----------------------------------------------------------
Purpose: Determines whether a string corresponds to a boolean
          TRUE value.
Returns: The boolean value (TRUE or FALSE)
Cond:    --
*/
BOOL PRIVATE IsIniYes(
    LPCTSTR psz)
    {
    int i;
    BOOL bNotFound = TRUE;
    BOOL bResult;

    ASSERT(psz); 

    /* Is the value TRUE? */

    for (i = 0; i < ARRAYSIZE(s_rgpszTrue); i++)
        {
        if (IsSzEqual(psz, s_rgpszTrue[i]))
            {
            bResult = TRUE;
            bNotFound = FALSE;
            break;
            }
        }

    /* Is the value FALSE? */

    if (bNotFound)
        {
        for (i = 0; i < ARRAYSIZE(s_rgpszFalse); i++)
            {
            if (IsSzEqual(psz, s_rgpszFalse[i]))
                {
                bResult = FALSE;
                bNotFound = FALSE;
                break;
                }
            }

        /* Is the value a known string? */

        if (bNotFound)
            {
            /* No.  Whine about it. */

            DEBUG_MSG(TF_WARNING, TEXT("IsIniYes() called on unknown Boolean RHS '%s'."), psz);
            bResult = FALSE;
            }
        }

    return bResult;
    }


/*----------------------------------------------------------
Purpose: Process keys with boolean RHSs.
Returns: --
Cond:    --
*/
void PRIVATE ProcessBooleans(void)
    {
    int i;

    for (i = 0; i < ARRAYSIZE(s_rgbik); i++)
        {
        DWORD dwcbKeyLen;
        TCHAR szRHS[MAXBUFLEN];
        BOOLINIKEY * pbik = &(s_rgbik[i]);
        LPCTSTR lpcszRHS;

        /* Look for key. */

        dwcbKeyLen = GetPrivateProfileString(pbik->ikh.pszSectionName,
                                   pbik->ikh.pszKeyName, TEXT(""), szRHS,
                                   ARRAYSIZE(szRHS), c_szIniFile);

        if (dwcbKeyLen)
            lpcszRHS = szRHS;
        else
            lpcszRHS = pbik->ikh.pszDefaultRHS;

        if (IsIniYes(lpcszRHS))
            {
            if (IsFlagClear(*(pbik->puStorage), pbik->dwFlag))
                DEBUG_MSG(TF_GENERAL, TEXT("ProcessIniFile(): %s set in %s![%s]."),
                         pbik->ikh.pszKeyName,
                         c_szIniFile,
                         pbik->ikh.pszSectionName);

            SetFlag(*(pbik->puStorage), pbik->dwFlag);
            }
        else
            {
            if (IsFlagSet(*(pbik->puStorage), pbik->dwFlag))
                DEBUG_MSG(TF_GENERAL, TEXT("ProcessIniFile(): %s cleared in %s![%s]."),
                         pbik->ikh.pszKeyName,
                         c_szIniFile,
                         pbik->ikh.pszSectionName);

            ClearFlag(*(pbik->puStorage), pbik->dwFlag);
            }
        }
    }


/*----------------------------------------------------------
Purpose: Process keys with integer RHSs.
Returns: --
Cond:    --
*/
void PRIVATE ProcessIntegers(void)
    {
    int i;

    for (i = 0; i < ARRAYSIZE(s_rgiik); i++)
        {
        DWORD dwcbKeyLen;
        TCHAR szRHS[MAXBUFLEN];
        INTINIKEY * piik = &(s_rgiik[i]);
        LPCTSTR lpcszRHS;

        /* Look for key. */

        dwcbKeyLen = GetPrivateProfileString(piik->ikh.pszSectionName,
                                   piik->ikh.pszKeyName, TEXT(""), szRHS,
                                   ARRAYSIZE(szRHS), c_szIniFile);

        if (dwcbKeyLen)
            lpcszRHS = szRHS;
        else
            lpcszRHS = piik->ikh.pszDefaultRHS;

        *(piik->puStorage) = AnsiToInt(lpcszRHS);

        DEBUG_MSG(TF_GENERAL, TEXT("ProcessIniFile(): %s set to %#04x."), 
                 piik->ikh.pszKeyName, *(piik->puStorage));
        }
    }

#endif

#ifdef DEBUG

/*----------------------------------------------------------
Purpose: Process initialization file
Returns: TRUE if initialization is successful
Cond:    --
*/
BOOL PUBLIC ProcessIniFile(void)
    {
    BOOL bResult = TRUE;

    // Currently, all integer keys are for DEBUG use only.
    //
    ProcessIntegers();

    // Currently, all boolean keys are for DEBUG use only.
    //
    ProcessBooleans();

    return bResult;
    }


/*----------------------------------------------------------
Purpose: Copy user settings to the .ini file
Returns: TRUE on success
Cond:    --
*/
BOOL PUBLIC CommitIniFile(void)
    {
    return TRUE;
    }

#endif

