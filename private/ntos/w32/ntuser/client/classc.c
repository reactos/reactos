/****************************** Module Header ******************************\
* Module Name: classc.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains
*
* History:
* 15-Dec-1993 JohnC      Pulled functions from user\server.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/*
 * These arrays are used by GetClassWord/Long.
 */

// !!! can't we get rid of this and just special case GCW_ATOM

CONST BYTE afClassDWord[] = {
     FIELD_SIZE(CLS, spicnSm),          // GCL_HICONSM       (-34)
     0,
     FIELD_SIZE(CLS, atomClassName),    // GCW_ATOM          (-32)
     0,
     0,
     0,
     0,
     0,
     FIELD_SIZE(CLS, style),            // GCL_STYLE         (-26)
     0,
     FIELD_SIZE(CLS, lpfnWndProc),      // GCL_WNDPROC       (-24)
     0,
     0,
     0,
     FIELD_SIZE(CLS, cbclsExtra),       // GCL_CBCLSEXTRA    (-20)
     0,
     FIELD_SIZE(CLS, cbwndExtra),       // GCL_CBWNDEXTRA    (-18)
     0,
     FIELD_SIZE(CLS, hModule),          // GCL_HMODULE       (-16)
     0,
     FIELD_SIZE(CLS, spicn),            // GCL_HICON         (-14)
     0,
     FIELD_SIZE(CLS, spcur),            // GCL_HCURSOR       (-12)
     0,
     FIELD_SIZE(CLS, hbrBackground),    // GCL_HBRBACKGROUND (-10)
     0,
     FIELD_SIZE(CLS, lpszMenuName)      // GCL_HMENUNAME      (-8)
};

CONST BYTE aiClassOffset[] = {
    FIELD_OFFSET(CLS, spicnSm),         // GCL_HICONSM
    0,
    FIELD_OFFSET(CLS, atomClassName),   // GCW_ATOM
    0,
    0,
    0,
    0,
    0,
    FIELD_OFFSET(CLS, style),           // GCL_STYLE
    0,
    FIELD_OFFSET(CLS, lpfnWndProc),     // GCL_WNDPROC
    0,
    0,
    0,
    FIELD_OFFSET(CLS, cbclsExtra),      // GCL_CBCLSEXTRA
    0,
    FIELD_OFFSET(CLS, cbwndExtra),      // GCL_CBWNDEXTRA
    0,
    FIELD_OFFSET(CLS, hModule),         // GCL_HMODULE
    0,
    FIELD_OFFSET(CLS, spicn),           // GCL_HICON
    0,
    FIELD_OFFSET(CLS, spcur),           // GCL_HCURSOR
    0,
    FIELD_OFFSET(CLS, hbrBackground),   // GCL_HBRBACKGROUND
    0,
    FIELD_OFFSET(CLS, lpszMenuName)     // GCL_MENUNAME
};

/*
 * INDEX_OFFSET must refer to the first entry of afClassDWord[]
 */
#define INDEX_OFFSET GCLP_HICONSM


/***************************************************************************\
* GetClassData
*
* GetClassWord and GetClassLong are now identical routines because they both
* can return DWORDs.  This single routine performs the work for them both
* by using two arrays; afClassDWord to determine whether the result should be
* a UINT or a DWORD, and aiClassOffset to find the correct offset into the
* CLS structure for a given GCL_ or GCL_ index.
*
* History:
* 11-19-90 darrinm      Wrote.
\***************************************************************************/

ULONG_PTR _GetClassData(
    PCLS pcls,
    PWND pwnd,   // used for transition to kernel-mode for GCL_WNDPROC
    int index,
    BOOL bAnsi)
{
    KERNEL_ULONG_PTR dwData;
    DWORD dwCPDType = 0;

    index -= INDEX_OFFSET;

    if (index < 0) {
        RIPERR0(ERROR_INVALID_INDEX, RIP_VERBOSE, "");
        return 0;
    }

    UserAssert(index >= 0);
    UserAssert(index < sizeof(afClassDWord));
    UserAssert(sizeof(afClassDWord) == sizeof(aiClassOffset));
    if (afClassDWord[index] == sizeof(DWORD)) {
        dwData = *(DWORD *)(((BYTE *)pcls) + aiClassOffset[index]);
    } else if (afClassDWord[index] == sizeof(KERNEL_ULONG_PTR)) {
        dwData = *(KERNEL_ULONG_PTR *)(((BYTE *)pcls) + aiClassOffset[index]);
    } else {
        dwData = (DWORD)*(WORD *)(((BYTE *)pcls) + aiClassOffset[index]);
    }

    index += INDEX_OFFSET;

    /*
     * If we're returning an icon or cursor handle, do the reverse
     * mapping here.
     */
    switch(index) {
    case GCLP_MENUNAME:
        if (IS_PTR(pcls->lpszMenuName)) {
            /*
             * The Menu Name is a real string: return the client-side address.
             * (If the class was registered by another app this returns an
             * address in that app's addr. space, but it's the best we can do)
             */
            dwData = bAnsi ?
                    (ULONG_PTR)pcls->lpszClientAnsiMenuName :
                    (ULONG_PTR)pcls->lpszClientUnicodeMenuName;
        }
        break;

    case GCLP_HICON:
    case GCLP_HCURSOR:
    case GCLP_HICONSM:
        /*
         * We have to go to the kernel to convert the pcursor to a handle because
         * cursors are allocated out of POOL, which is not accessable from the client.
         */
        if (dwData) {
            dwData = NtUserCallHwndParam(PtoH(pwnd), index, SFI_GETCLASSICOCUR);
        }
        break;

    case GCLP_WNDPROC:
        {

        /*
         * Always return the client wndproc in case this is a server
         * window class.
         */

        if (pcls->CSF_flags & CSF_SERVERSIDEPROC) {
            dwData = MapServerToClientPfn(dwData, bAnsi);
        } else {
            KERNEL_ULONG_PTR dwT = dwData;

            dwData = MapClientNeuterToClientPfn(pcls, dwT, bAnsi);

            /*
             * If the client mapping didn't change the window proc then see if
             * we need a callproc handle.
             */
            if (dwData == dwT) {
                /*
                 * Need to return a CallProc handle if there is an Ansi/Unicode mismatch
                 */
                if (bAnsi != !!(pcls->CSF_flags & CSF_ANSIPROC)) {
                    dwCPDType |= bAnsi ? CPD_ANSI_TO_UNICODE : CPD_UNICODE_TO_ANSI;
                }
            }
        }

        if (dwCPDType) {
            ULONG_PTR dwCPD;

            dwCPD = GetCPD(pwnd, dwCPDType | CPD_WNDTOCLS, KERNEL_ULONG_PTR_TO_ULONG_PTR(dwData));

            if (dwCPD) {
                dwData = dwCPD;
            } else {
                RIPMSG0(RIP_WARNING, "GetClassLong unable to alloc CPD returning handle\n");
            }
        }
        }
        break;

    case GCL_CBCLSEXTRA:
        if ((pcls->CSF_flags & CSF_WOWCLASS) && (pcls->CSF_flags & CSF_WOWEXTRA)) {
            /*
             * The 16-bit app changed its Extra bytes value.  Return the changed
             * value.  FritzS
             */

            return PWCFromPCLS(pcls)->iClsExtra;
        }
        else
            return pcls->cbclsExtra;

        break;

    /*
     * WOW uses a pointer straight into the class structure.
     */
    case GCLP_WOWWORDS:
        if (pcls->CSF_flags & CSF_WOWCLASS) {
            return ((ULONG_PTR)PWCFromPCLS(pcls));
        } else
            return 0;

    case GCL_STYLE:
        dwData &= CS_VALID;
        break;
    }

    return KERNEL_ULONG_PTR_TO_ULONG_PTR(dwData);
}



/***************************************************************************\
* _GetClassLong (API)
*
* Return a class long.  Positive index values return application class longs
* while negative index values return system class longs.  The negative
* indices are published in WINDOWS.H.
*
* History:
* 10-16-90 darrinm      Wrote.
\***************************************************************************/

ULONG_PTR _GetClassLongPtr(
    PWND pwnd,
    int index,
    BOOL bAnsi)
{
    PCLS pcls = REBASEALWAYS(pwnd, pcls);

    if (index < 0) {
        return _GetClassData(pcls, pwnd, index, bAnsi);
    } else {
        if (index + (int)sizeof(ULONG_PTR) > pcls->cbclsExtra) {
            RIPERR0(ERROR_INVALID_INDEX, RIP_VERBOSE, "");
            return 0;
        } else {
            ULONG_PTR UNALIGNED *pudw;
            pudw = (ULONG_PTR UNALIGNED *)((BYTE *)(pcls + 1) + index);
            return *pudw;
        }
    }
}

#ifdef _WIN64
DWORD _GetClassLong(
    PWND pwnd,
    int index,
    BOOL bAnsi)
{
    PCLS pcls = REBASEALWAYS(pwnd, pcls);

    if (index < 0) {
        if (index < INDEX_OFFSET || afClassDWord[index - INDEX_OFFSET] > sizeof(DWORD)) {
            RIPERR1(ERROR_INVALID_INDEX, RIP_WARNING, "GetClassLong: invalid index %d", index);
            return 0;
        }
        return (DWORD)_GetClassData(pcls, pwnd, index, bAnsi);
    } else {
        if (index + (int)sizeof(DWORD) > pcls->cbclsExtra) {
            RIPERR0(ERROR_INVALID_INDEX, RIP_VERBOSE, "");
            return 0;
        } else {
            DWORD UNALIGNED *pudw;
            pudw = (DWORD UNALIGNED *)((BYTE *)(pcls + 1) + index);
            return *pudw;
        }
    }
}
#endif

/***************************************************************************\
* GetClassWord (API)
*
* Return a class word.  Positive index values return application class words
* while negative index values return system class words.  The negative
* indices are published in WINDOWS.H.
*
* History:
* 10-16-90 darrinm      Wrote.
\***************************************************************************/

WORD GetClassWord(
    HWND hwnd,
    int index)
{
    PWND pwnd;
    PCLS pclsClient;

    pwnd = ValidateHwnd(hwnd);

    if (pwnd == NULL)
        return 0;

    pclsClient = (PCLS)REBASEALWAYS(pwnd, pcls);

    if (index == GCW_ATOM) {
        return (WORD)_GetClassData(pclsClient, pwnd, index, FALSE);
    } else {
        if ((index < 0) || (index + (int)sizeof(WORD) > pclsClient->cbclsExtra)) {
            RIPERR0(ERROR_INVALID_INDEX, RIP_VERBOSE, "");
            return 0;
        } else {
            WORD UNALIGNED *puw;
            puw = (WORD UNALIGNED *)((BYTE *)(pclsClient + 1) + index);
            return *puw;
        }
    }
}
