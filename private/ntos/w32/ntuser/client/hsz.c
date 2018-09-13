/****************************** Module Header ******************************\
* Module Name: hsz.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* HSZ.C - DDEML String handle functions
*
* History:
* 10-28-91 Sanfords Created
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* DdeCreateStringHandle (DDEML API)
*
* Description:
* Create an HSZ from a string.
*
* History:
* 11-1-91 sanfords Created.
\***************************************************************************/
HSZ DdeCreateStringHandleA(
DWORD idInst,
LPCSTR psz,
int iCodePage)
{
    if (iCodePage == 0) {
        iCodePage = CP_WINANSI;
    }
    return (InternalDdeCreateStringHandle(idInst, (PVOID)psz, iCodePage));
}


HSZ DdeCreateStringHandleW(
DWORD idInst,
LPCWSTR psz,
int iCodePage)
{
    if (iCodePage == 0) {
        iCodePage = CP_WINUNICODE;
    }
    return (InternalDdeCreateStringHandle(idInst, (PVOID)psz, iCodePage));
}



HSZ InternalDdeCreateStringHandle(
DWORD idInst,
PVOID psz,
int iCodePage)
{
    PCL_INSTANCE_INFO pcii;
    HSZ hszRet = 0;
    int cb;
    WCHAR szw[256];

    EnterDDECrit;

    pcii = ValidateInstance((HANDLE)LongToHandle( idInst ));
    if (pcii == NULL) {
        BestSetLastDDEMLError(DMLERR_INVALIDPARAMETER);
        goto Exit;
    }

    switch (iCodePage) {
    case CP_WINANSI:
        if (*(LPSTR)psz == '\0') {
            goto Exit;
        }
        hszRet = NORMAL_HSZ_FROM_LATOM(AddAtomA((LPSTR)psz));
        break;

    default:

        /*
         * Convert psz to unicode and fall through.
         */
        cb = sizeof(szw) /  sizeof(WCHAR);
#ifdef LATER
        MultiByteToWideChar((UINT)iCodePage, MB_PRECOMPOSED,
                            (LPSTR)psz, -1, szw, cb);
#endif
        psz = &szw[0];


    case CP_WINUNICODE:
        if (*(LPWSTR)psz == L'\0') {
            goto Exit;
        }
        hszRet = NORMAL_HSZ_FROM_LATOM(AddAtomW((LPWSTR)psz));
        break;
    }
    MONHSZ(pcii, hszRet, MH_CREATE);

Exit:
    LeaveDDECrit;
    return (hszRet);
}



/***************************************************************************\
* DdeQueryString (DDEML API)
*
* Description:
* Recall the string associated with an HSZ.
*
* History:
* 11-1-91 sanfords Created.
\***************************************************************************/
DWORD DdeQueryStringA(
DWORD idInst,
HSZ hsz,
LPSTR psz,
DWORD cchMax,
INT iCodePage)
{
    if (iCodePage == 0) {
        iCodePage = CP_WINANSI;
    }
    return (InternalDdeQueryString(idInst, hsz, psz, cchMax, iCodePage));
}


DWORD DdeQueryStringW(
DWORD idInst,
HSZ hsz,
LPWSTR psz,
DWORD cchMax,
INT iCodePage)
{
    if (iCodePage == 0) {
        iCodePage = CP_WINUNICODE;
    }
    return (InternalDdeQueryString(idInst, hsz, psz, cchMax * sizeof(WCHAR), iCodePage));
}


DWORD InternalDdeQueryString(
DWORD idInst,
HSZ hsz,
PVOID psz,
DWORD cbMax,
INT iCodePage)
{
    PCL_INSTANCE_INFO pcii;
    DWORD dwRet = 0;
    WCHAR szw[256];
// BOOL fDefUsed; // LATER

    EnterDDECrit;

    pcii = ValidateInstance((HANDLE)LongToHandle( idInst ));
    if (pcii == NULL) {
        BestSetLastDDEMLError(DMLERR_INVALIDPARAMETER);
        goto Exit;
    }

    if (ValidateHSZ(hsz) == HSZT_INVALID) {
        SetLastDDEMLError(pcii, DMLERR_INVALIDPARAMETER);
        goto Exit;
    }

    if (LATOM_FROM_HSZ(hsz) == 0) {
        if (iCodePage == CP_WINUNICODE) {
            if (psz != NULL) {
                *(LPWSTR)psz = L'\0';
            }
            dwRet = sizeof(WCHAR);
            goto Exit;
        } else {
            if (psz != NULL) {
                *(LPSTR)psz = '\0';
            }
            dwRet = sizeof(CHAR);
            goto Exit;
        }
    }

    if (psz == NULL) {
        cbMax = sizeof(szw);
        psz = (PVOID)szw;
    }

    switch (iCodePage) {
    case CP_WINANSI:
        dwRet = GetAtomNameA(LATOM_FROM_HSZ(hsz), psz, cbMax);
        break;

    default:
        dwRet = GetAtomNameW(LATOM_FROM_HSZ(hsz), (LPWSTR)psz, cbMax / sizeof(WCHAR));
        if (iCodePage != CP_WINUNICODE) {

            /*
             * convert psz to the appropriate codepage and count the
             * characters(ie BYTES for DBCS!) to alter dwRet.
             */
#ifdef LATER
            // Does this routine work in place? (i.e. input and output buffer the same).
            WideCharToMultiByte((UINT)iCodePage, 0, szw,
                    sizeof(szw) /  sizeof(WCHAR),
                    (LPSTR)psz, cbMax, NULL, &fDefUsed);
#endif
            dwRet = cbMax + 1;
        }
        break;
    }

Exit:
    LeaveDDECrit;
    return (dwRet);
}



/***************************************************************************\
* DdeFreeStringHandle (DDEML API)
*
* Description:
* Decrement the use count of an HSZ.
*
* History:
* 11-1-91 sanfords Created.
\***************************************************************************/
BOOL DdeFreeStringHandle(
DWORD idInst,
HSZ hsz)
{
    PCL_INSTANCE_INFO pcii;
    BOOL fRet = FALSE;

    EnterDDECrit;

    pcii = ValidateInstance((HANDLE)LongToHandle( idInst ));
    if (pcii == NULL) {
        BestSetLastDDEMLError(DMLERR_INVALIDPARAMETER);
        goto Exit;
    }

    if (ValidateHSZ(hsz) == HSZT_INVALID) {
        SetLastDDEMLError(pcii, DMLERR_INVALIDPARAMETER);
        goto Exit;
    }

    MONHSZ(pcii, hsz, MH_DELETE);
    fRet = TRUE;
    if (LATOM_FROM_HSZ(hsz) != 0) {
        if (DeleteAtom(LATOM_FROM_HSZ(hsz))) {
            SetLastDDEMLError(pcii, DMLERR_INVALIDPARAMETER);
            fRet = FALSE;
        }
    }

Exit:
    LeaveDDECrit;
    return (fRet);
}



/***************************************************************************\
* DdeKeepStringHandle (DDEML API)
*
* Description:
* Increments the use count of an HSZ.
*
* History:
* 11-1-91 sanfords Created.
\***************************************************************************/
BOOL DdeKeepStringHandle(
DWORD idInst,
HSZ hsz)
{
    PCL_INSTANCE_INFO pcii;
    BOOL fRet = FALSE;

    EnterDDECrit;

    pcii = ValidateInstance((HANDLE)LongToHandle( idInst ));
    if (pcii == NULL) {
        BestSetLastDDEMLError(DMLERR_INVALIDPARAMETER);
        goto Exit;
    }

    if (ValidateHSZ(hsz) == HSZT_INVALID) {
        SetLastDDEMLError(pcii, DMLERR_INVALIDPARAMETER);
        goto Exit;
    }

    if (LATOM_FROM_HSZ(hsz) == 0) {
        fRet = TRUE;
        goto Exit;
    }
    MONHSZ(pcii, hsz, MH_KEEP);
    fRet = IncLocalAtomCount(LATOM_FROM_HSZ(hsz)) ? TRUE : FALSE;

Exit:
    LeaveDDECrit;
    return (fRet);
}



/***************************************************************************\
* DdeCmpStringHandles (DDEML API)
*
* Description:
* Useless comparison of hszs. Provided for case sensitivity expandability.
* Direct comparison of hszs would be a case sensitive comparison while
* using this function would be case-insensitive. For now both ways are ==.
*
* History:
* 11-1-91 sanfords Created.
\***************************************************************************/
int DdeCmpStringHandles(
HSZ hsz1,
HSZ hsz2)
{
    if (hsz2 > hsz1) {
        return (-1);
    } else if (hsz2 < hsz1) {
        return (1);
    } else {
        return (0);
    }
}


/***************************************************************************\
* ValidateHSZ
*
* Description:
* Verifies the probability of a reasonable hsz
*
* History:
* 11-1-91 sanfords Created.
\***************************************************************************/
DWORD ValidateHSZ(
HSZ hsz)
{
    if (hsz == 0) {
        return (HSZT_NORMAL);
    }
    if (LOWORD((ULONG_PTR)hsz) < 0xC000) {
        return (HSZT_INVALID);
    }
    if (HIWORD((ULONG_PTR)hsz) == 0) {
        return (HSZT_NORMAL);
    }
    if (HIWORD((ULONG_PTR)hsz) == 1) {
        return (HSZT_INST_SPECIFIC);
    }
    return (HSZT_INVALID);
}

/***************************************************************************\
* MakeInstSpecificAtom
*
* Description:
* Creates a new atom that has hwnd imbeded into it.
*
* History:
* 11-1-91 sanfords Created.
\***************************************************************************/
LATOM MakeInstSpecificAtom(
LATOM la,
HWND hwnd)
{
    WCHAR sz[256];
    LPWSTR psz;

    if (GetAtomName(la, sz, 256) == 0) {
        return (0);
    }
#ifdef UNICODE
    psz = sz + wcslen(sz);
#else
    psz = sz + strlen(sz);
#endif
    wsprintf(psz, TEXT("(%#p)"), hwnd);
    la = AddAtom(sz);
    return (la);
}



/***************************************************************************\
* ParseInstSpecificAtom
*
* Description:
* Extracts the hwnd value out of the atom.
*
* History:
* 11-1-91 sanfords Created.
\***************************************************************************/
HWND ParseInstSpecificAtom(
LATOM la,
LATOM *plaNormal)
{
    CHAR sz[256];
    LPSTR pszHwnd;
    HWND hwnd;

    /*
     * LATER- NEED TO MAKE THIS UNICODE BASED WHEN WE GET A SCANF WE CAN USE
     */
    if (GetAtomNameA(la, sz, 256) == 0) {
        return (0);
    }
    pszHwnd = strrchr(sz, '(');
    if (pszHwnd == NULL) {
        return (0);
    }
    if (sscanf(pszHwnd, "(%#p)", &hwnd) != 1) {
        return (0);
    }
    if (plaNormal != NULL) {
        *pszHwnd = '\0';
        *plaNormal = AddAtomA(sz);
    }
    return (hwnd);
}




/***************************************************************************\
* LocalToGlobalAtom
*
* Description:
* Converts a Local Atom to a Global Atom
*
* History:
* 12-1-91 sanfords Created.
\***************************************************************************/
GATOM LocalToGlobalAtom(
LATOM la)
{
    WCHAR sz[256];

    if (la == 0) {
        return (0);
    }
    if (GetAtomName((ATOM)la, sz, 256) == 0) {
        RIPMSG0(RIP_WARNING, "LocalToGlobalAtom out of memory");
        return (0);
    }
    return ((GATOM)GlobalAddAtom(sz));
}



/***************************************************************************\
* GlobalToLocalAtom
*
* Description:
* Converts a Global Atom to a Local Atom
*
* History:
* 12-1-91 sanfords Created.
\***************************************************************************/
LATOM GlobalToLocalAtom(
GATOM ga)
{
    WCHAR sz[256];

    if (ga == 0) {
        return (0);
    }
    if (GlobalGetAtomName((ATOM)ga, sz, 256) == 0) {
        RIPMSG0(RIP_WARNING, "GlobalToLocalAtom out of memory");
        return (0);
    }
    return ((LATOM)AddAtom(sz));
}


/***************************************************************************\
* IncGlobalAtomCount
*
* Description:
* Duplicates an atom.
*
*
* History:
* 1-22-91 sanfords Created.
\***************************************************************************/
GATOM IncGlobalAtomCount(
GATOM ga)
{
    WCHAR sz[256];

    if (ga == 0) {
        return (0);
    }
    if (GlobalGetAtomName(ga, sz, 256) == 0) {
        RIPMSG0(RIP_WARNING, "IncGlobalAtomCount out of memory");
        return (0);
    }
    return ((GATOM)GlobalAddAtom(sz));
}


/***************************************************************************\
* IncGlobalAtomCount
*
* Description:
* Duplicates an atom.
*
*
* History:
* 1-22-91 sanfords Created.
\***************************************************************************/
LATOM IncLocalAtomCount(
LATOM la)
{
    WCHAR sz[256];

    if (la == 0) {
        return (0);
    }
    if (GetAtomName(la, sz, 256) == 0) {
        RIPMSG0(RIP_WARNING, "IncLocalAtomCount out of memory");
        return (0);
    }
    return ((LATOM)AddAtom(sz));
}
