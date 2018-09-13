/****************************** Module Header ******************************\
* Module Name: clres.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Resource Loading/Creation Routines
*
* History:
* 24-Sep-1990 MikeKe    From win30
* 19-Sep-1995 ChrisWil  Win95/NT merge.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/*
 * Constants.
 */
#define BPP01_MAXCOLORS     2
#define BPP04_MAXCOLORS    16
#define BPP08_MAXCOLORS   256

#define RESCLR_BLACK      0x00000000
#define RESCLR_WHITE      0x00FFFFFF

typedef struct {
    ACCEL accel;
    WORD  padding;
} RESOURCE_ACCEL, *PRESOURCE_ACCEL;

/*
 * Bitmap resource IDs
 */
#define BMR_ICON    1
#define BMR_BITMAP  2
#define BMR_CURSOR  3

typedef struct _OLDCURSOR {
    BYTE bType;
    BYTE bFormat;
    WORD xHotSpot;  // 0 for icons
    WORD yHotSpot;  // 0 for icons
    WORD cx;
    WORD cy;
    WORD cxBytes;
    WORD wReserved2;
    BYTE abBitmap[1];
} OLDCURSOR, *POLDCURSOR;
typedef OLDCURSOR UNALIGNED *UPOLDCURSOR;

/*
 * Local Macros.
 */
#define GETINITDC() \
    (gfSystemInitialized ? NtUserGetDC(NULL) : CreateDCW(L"DISPLAY", L"", NULL, NULL))

#define RELEASEINITDC(hdc) \
    (gfSystemInitialized ? ReleaseDC(NULL, hdc) : DeleteDC(hdc))

#define ISRIFFFORMAT(p) \
    (((UNALIGNED RTAG *)(p))->ckID == FOURCC_RIFF)

#define MR_FAILFOR40    0x01
#define MR_MONOCHROME   0x02


typedef struct tagMAPRES {
    WORD idDisp;                // display driver ID
    WORD idUser;                // USER ID
    BYTE bFlags;                // Flags
    BYTE bReserved;             // unused
} MAPRES, *LPMAPRES, *PMAPRES;


HBITMAP CopyBmp(HBITMAP hbmpOrg, int cxNew, int cyNew, UINT LR_flags);

/***************************************************************************\
* SplFindResource
*
* Check whether the hInstance passed is that of the present display driver;
* if so, it will call the GetDriverResourceId() in the display to allow
* it to map the given id/name to a new id/name.  Then it will call
* FindResource9) in KERNEL.
*
* 13-Nov-1995 SanfordS  Added mapping for DEFAULT constants.
\***************************************************************************/

HANDLE SplFindResource(
    HINSTANCE hmod,
    LPCWSTR   lpName,
    LPCWSTR   lpType)
{
    return FINDRESOURCEW(hmod, lpName, lpType);
}

/***************************************************************************\
* SplFreeResource
*
* Really frees a resource that is shared (won't be touched again unless
* LR_COPYFROMRESOURCE is used) or system.
*
* 13-Nov-1995 SanfordS  Added mapping for DEFAULT constants.
\***************************************************************************/

VOID SplFreeResource(
    HANDLE    hRes,
    HINSTANCE hmod,
    UINT      lrFlags)
{
    if (!FREERESOURCE(hRes, hmod) &&
        ((hmod == hmodUser) || (lrFlags & LR_SHARED))) {

        FREERESOURCE(hRes, hmod);
    }
}

/***********************************************************************\
* WowGetModuleFileName
*
* This converts a WOW or non-WOW module handle to a string form that
* can be restored even for WOW handles.
*
* Returns: fSuccess
*
* 29-Nov-1995 SanfordS  Created.
\***********************************************************************/

BOOL WowGetModuleFileName(
    HMODULE hModule,
    LPWSTR  pwsz,
    DWORD   cchMax)
{
    if (!GetModuleFileName(hModule, pwsz, cchMax)) {

        if (cchMax < 10) {
            RIPMSG0(RIP_WARNING, "WowGetModuleFileName: exceeded Char-Max");
            return FALSE;
        }

        wsprintf(pwsz, TEXT("\001%08lx"), hModule);
    }

    return TRUE;
}

/***********************************************************************\
* WowGetModuleHandle
*
* This restores the string form of a module handle created by
* WowGetModuleFileName to the original handle.
*
* Returns: fSuccess
*
* 29-Nov-1995 Created   SanfordS
\***********************************************************************/

HMODULE WowGetModuleHandle(
    LPCWSTR pwsz)
{
    HMODULE hMod = NULL;
    DWORD   digit;

    if (pwsz[0] == TEXT('\001')) {

        /*
         * Cant seem to link to swscanf without CRT0 problems so just
         * do it by hand.
         */
        while (*(++pwsz)) {

            if (*pwsz == TEXT(' '))
                continue;

            digit = *pwsz - TEXT('0');

            if (digit > 9)
                digit += (DWORD)(TEXT('0') - TEXT('a') + 10);

            (ULONG_PTR)hMod <<= 4;
            (ULONG_PTR)hMod += digit;
        }

    } else {

        hMod = GetModuleHandle(pwsz);
    }

    return hMod;
}

/***************************************************************************\
* CreateAcceleratorTableA (API)
*
* Creates an accel table, returns handle to accel table.
*
* 02-May-1991 ScottLu   Created.
\***************************************************************************/

HACCEL WINAPI CreateAcceleratorTableA(
    LPACCEL paccel,
    int     cAccel)
{
    int     nAccel = cAccel;
    LPACCEL pAccelT = paccel;

    /*
     * Convert any character keys from ANSI to Unicode.
     */
    while (nAccel--) {

        if ((pAccelT->fVirt & FVIRTKEY) == 0) {

            if (!NT_SUCCESS(RtlMultiByteToUnicodeN((LPWSTR)&(pAccelT->key),
                                                   sizeof(WCHAR),
                                                   NULL,
                                                   (LPSTR)&(pAccelT->key),
                                                   sizeof(CHAR)))) {
                pAccelT->key = 0xFFFF;
            }
        }

        pAccelT++;
    }

    return NtUserCreateAcceleratorTable(paccel, cAccel);
}

/***************************************************************************\
* CopyAcceleratorTableA (API)
*
* Copies an accel table
*
* 02-May-1991 ScottLu   Created.
\***************************************************************************/

int CopyAcceleratorTableA(
    HACCEL hacc,
    LPACCEL paccel,
    int length)
{
    int retval;

    retval = NtUserCopyAcceleratorTable(hacc, paccel, length);

    /*
     * If we are doing a copy and we succeeded then convert the accelerator
     */
    if ((paccel != NULL) && (retval > 0)) {

        /*
         * Translate UNICODE character keys to ANSI
         */
        int nAccel = retval;
        LPACCEL pAccelT = paccel;

        while (nAccel--) {
            if ((pAccelT->fVirt & FVIRTKEY) == 0) {
                if (!NT_SUCCESS(RtlUnicodeToMultiByteN((PCHAR)&(pAccelT->key),
                                                       sizeof(WCHAR),
                                                       NULL,
                                                       (PWSTR)&(pAccelT->key),
                                                        sizeof(pAccelT->key)))) {
                        pAccelT->key = 0;
                    }
                }
            pAccelT++;
        }
    }

    return retval;
}

/***************************************************************************\
* FindAccResource
*
* Resource accelerator tables are to be loaded only once to be compatible
*  with Win95. So we keep track of the addresses we've loaded tables from
*  and the corresponding handle.
*
* This function finds an entry in the table. It returns the address
*  of the pacNext pointer that contains the requested entry.
*
* 01/31/97 GerardoB     Created.
\***************************************************************************/
PACCELCACHE * FindAccResource (HACCEL hAccel, PVOID pRes)
{
     /************************************
     * The caller must own gcsAccelCache *
     *************************************/

    PACCELCACHE * ppacNext = &gpac;
    PACCELCACHE pac;

    /*
     * This is meant to search by handle or by pointer, not both
     * So at least one of the parameters must be NULL.
     */
    UserAssert(!(hAccel && pRes));
    /*
     * Walk the table
     */
    while (*ppacNext != NULL) {
        pac = *ppacNext;
        if ((pac->pRes == pRes) || (pac->hAccel == hAccel)) {
            /*
            * Found it. Validate this entry before returning.
            */
            UserAssert(pac->dwLockCount != 0);
            UserAssert(HMValidateHandleNoDesktop(pac->hAccel, TYPE_ACCELTABLE));
            break;
        }

        ppacNext = &(pac->pacNext);
    }

    return ppacNext;
}
/***************************************************************************\
* AddAccResource
*
* This is called everytime LoadAcc loads a new table. It adds an
*  entry (handle and resource address) to the global list and
*  sets the lock count to 1
*
* 01/31/97 GerardoB     Created.
\***************************************************************************/
void AddAccResource (HACCEL hAccel, PVOID pRes)
{
    PACCELCACHE pac;

    UserAssert(HMValidateHandleNoDesktop(hAccel, TYPE_ACCELTABLE));
    UserAssert(pRes != NULL);

    /*
     * Allocate and initialize a new entry.
     */
    pac = (PACCELCACHE)LocalAlloc(LPTR, sizeof(ACCELCACHE));
    if (pac != NULL) {
        pac->dwLockCount = 1;
        pac->hAccel = hAccel;
        pac->pRes = pRes;

        /*
         * Make it the new head of the list
         */
        RtlEnterCriticalSection(&gcsAccelCache);
            pac->pacNext = gpac;
            gpac = pac;
        RtlLeaveCriticalSection(&gcsAccelCache);

    }
}
/***************************************************************************\
* DestroyAcceleratorTable
*
* 01/31/97 GerardoB     Created.
\***************************************************************************/
BOOL DestroyAcceleratorTable (HACCEL hAccel)
{
    BOOL fUnlocked = TRUE;
    PACCELCACHE *ppacNext, pac;

    /*
     * If we added this table to our list, decrement the lock count
     */
    RtlEnterCriticalSection(&gcsAccelCache);
        ppacNext = FindAccResource(hAccel, NULL);
        if (*ppacNext != NULL) {
            pac = *ppacNext;
            /*
             * Found it. Decrement lock count.
             */
            UserAssert(pac->dwLockCount != 0);
            fUnlocked = (--pac->dwLockCount == 0);
            /*
             * If noboby else wants this around, unlink it and nuke it.
             */
            if (fUnlocked) {
                *ppacNext = pac->pacNext;
                LocalFree(pac);
            }
        }
    RtlLeaveCriticalSection(&gcsAccelCache);

    /*
     * If not totally deref'ed, return FALSE (win95 compat).
     */
    if (fUnlocked) {
        return NtUserDestroyAcceleratorTable(hAccel);
    } else {
        return FALSE;
    }
}
/***************************************************************************\
* LoadAcc (Worker)
*
* This is the worker-routine for loading accelerator tables.
*
\***************************************************************************/

#define FACCEL_VALID (FALT | FCONTROL | FNOINVERT | FSHIFT | FVIRTKEY | FLASTKEY)

HANDLE LoadAcc(
    HINSTANCE hmod,
    HANDLE    hrl)
{
    PACCELCACHE * ppacNext;
    HANDLE handle = NULL;

    if (hrl != NULL) {

        if (hrl = LOADRESOURCE(hmod, hrl)) {

            PRESOURCE_ACCEL paccel;

            if ((paccel = (PRESOURCE_ACCEL)LOCKRESOURCE(hrl, hmod)) != NULL) {

                int nAccel = 0;
                int i;
                LPACCEL paccelT;

                /*
                 * Check if we've already loaded accelerators from this
                 *  same address
                 */
                RtlEnterCriticalSection(&gcsAccelCache);
                    ppacNext = FindAccResource(NULL, paccel);
                    if (*ppacNext != NULL) {
                        (*ppacNext)->dwLockCount++;
                        handle = (*ppacNext)->hAccel;
                    }
                RtlLeaveCriticalSection(&gcsAccelCache);
                /*
                 * If we found this table on the global list,
                 *  return the same handle (Win95 compat)
                 */
                if (handle != NULL) {
                    goto UnlockAndFree;
                }

                while (!((paccel[nAccel].accel.fVirt) & FLASTKEY)) {

                    if (paccel[nAccel].accel.fVirt & ~FACCEL_VALID) {
                        RIPMSG0(RIP_WARNING, "LoadAcc: Invalid Parameter");
                        goto UnlockAndFree;
                    }

                    nAccel++;
                }

                if (paccel[nAccel].accel.fVirt & ~FACCEL_VALID) {
                    RIPMSG0(RIP_WARNING, "LoadAcc: Invalid Parameter");
                    goto UnlockAndFree;
                }

                /*
                 * Since the accelerator table is coming from a resource, each
                 * element has an extra WORD of padding which we strip here
                 * to conform with the public (and internal) ACCEL structure.
                 */
                paccelT = UserLocalAlloc(0, sizeof(ACCEL) * (nAccel + 1));
                if (paccelT == NULL) {
                    goto UnlockAndFree;
                }
                for (i = 0; i < nAccel + 1; i++) {
                    paccelT[i] = paccel[i].accel;
                }

                handle = NtUserCreateAcceleratorTable(paccelT,
                                                      nAccel + 1);

                UserLocalFree(paccelT);

                /*
                 * Add this handle/address to the global table so
                 *  we won't load it twice.
                 */
                if (handle != NULL) {
                    AddAccResource(handle, paccel);
                }
UnlockAndFree:

                UNLOCKRESOURCE(hrl, hmod);
            }

            FREERESOURCE(hrl, hmod);
        }
    }

    return handle;
}

/***************************************************************************\
* LoadAcceleratorsA (API)
* LoadAcceleratorsW (API)
*
*
* 24-Sep-1990 MikeKe    From Win30
\***************************************************************************/

HACCEL WINAPI LoadAcceleratorsA(
    HINSTANCE hmod,
    LPCSTR    lpAccName)
{
    HANDLE hRes;

    hRes = FINDRESOURCEA((HANDLE)hmod, lpAccName, (LPSTR)RT_ACCELERATOR);

    return (HACCEL)LoadAcc(hmod, hRes);
}

HACCEL WINAPI LoadAcceleratorsW(
    HINSTANCE hmod,
    LPCWSTR   lpAccName)
{
    HANDLE hRes;

    hRes = FINDRESOURCEW((HANDLE)hmod, lpAccName, RT_ACCELERATOR);

    return (HACCEL)LoadAcc(hmod, hRes);
}

/***************************************************************************\
* LoadStringA (API)
* LoadStringW (API)
*
*
* 05-Apr-1991 ScottLu   Fixed to work with client/server.
\***************************************************************************/

int WINAPI LoadStringA(
    HINSTANCE hmod,
    UINT      wID,
    LPSTR     lpAnsiBuffer,
    int       cchBufferMax)
{
    LPWSTR          lpUniBuffer;
    INT             cchUnicode;
    INT             cbAnsi = 0;

    /*
     * LoadStringOrError appends a NULL but does not include it in the
     * return count-of-bytes
     */
    cchUnicode = LoadStringOrError((HANDLE)hmod,
                                      wID,
                                      (LPWSTR)&lpUniBuffer,
                                      0,
                                      0);

    if (cchUnicode) {

        cbAnsi = WCSToMB(lpUniBuffer,
                         cchUnicode,
                         &lpAnsiBuffer,
                         cchBufferMax - 1,
                         FALSE);

        cbAnsi = min(cbAnsi, cchBufferMax - 1);
    }

    /*
     * Append a NULL but do not include it in the count returned
     */
    lpAnsiBuffer[cbAnsi] = 0;
    return cbAnsi;
}

int WINAPI LoadStringW(
    HINSTANCE hmod,
    UINT      wID,
    LPWSTR    lpBuffer,
    int       cchBufferMax)
{
    return LoadStringOrError((HANDLE)hmod,
                                wID,
                                lpBuffer,
                                cchBufferMax,
                                0);
}

/***************************************************************************\
* SkipIDorString
*
* Skips string (or ID) and returns the next aligned WORD.
*
\***************************************************************************/

PBYTE SkipIDorString(
    LPBYTE pb)
{
    if (*((LPWORD)pb) == 0xFFFF)
        return (pb + 4);

    while (*((PWCHAR)pb)++ != 0);

    return pb;
}

/***************************************************************************\
* GetSizeDialogTemplate
*
* This gets called by thank produced stubs. It returns the size of a
* dialog template.
*
* 07-Apr-1991 ScottLu   Created.
\***************************************************************************/

DWORD GetSizeDialogTemplate(
    HINSTANCE      hmod,
    LPCDLGTEMPLATE pdt)
{
    UINT           cdit;
    LPBYTE         pb;
    BOOL           fChicago;
    LPDLGTEMPLATE2 pdt2;

    if (HIWORD(pdt->style) == 0xFFFF) {

        pdt2 = (LPDLGTEMPLATE2)pdt;
        fChicago = TRUE;

        /*
         * Fail if the app is passing invalid style bits.
         */
        if (pdt2->style & ~(DS_VALID40 | 0xffff0000)) {
            RIPMSG0(RIP_WARNING, "Bad dialog style bits - please remove");
            return 0;
        }

        pb = (LPBYTE)(((LPDLGTEMPLATE2)pdt) + 1);

    } else {

        fChicago = FALSE;

        /*
         * Check if invalid style bits are being passed. Fail if the app
         * is a new app ( >= VER40).
         * This is to ensure that we are compatible with Chicago.
         */
        if ((pdt->style & ~(DS_VALID40 | 0xffff0000)) &&
                (GETEXPWINVER(hmod) >= VER40)) {

            /*
             * It's a new app with invalid style bits - fail.
             */
            RIPMSG0(RIP_WARNING, "Bad dialog style bits - please remove");
            return 0;
        }

        pb = (LPBYTE)(pdt + 1);
    }

    /*
     * If there is a menu ordinal, add 4 bytes skip it. Otherwise it is a
     * string or just a 0.
     */
    pb = SkipIDorString(pb);

    /*
     * Skip window class and window text, adjust to next word boundary.
     */
    pb = SkipIDorString(pb);
    pb = SkipIDorString(pb);

    /*
     * Skip font type, size and name, adjust to next dword boundary.
     */
    if ((fChicago ? pdt2->style : pdt->style) & DS_SETFONT) {
        pb += fChicago ? sizeof(DWORD) + sizeof(WORD): sizeof(WORD);
        pb = SkipIDorString(pb);
    }
    pb = (LPBYTE)(((ULONG_PTR)pb + 3) & ~3);

    /*
     * Loop through dialog items now...
     */
    cdit = fChicago ? pdt2->cDlgItems : pdt->cdit;

    while (cdit-- != 0) {

        UINT cbCreateParams;

        pb += fChicago ? sizeof(DLGITEMTEMPLATE2) : sizeof(DLGITEMTEMPLATE);

        /*
         * Skip the dialog control class name.
         */
        pb = SkipIDorString(pb);

        /*
         * Look at window text now.
         */
        pb = SkipIDorString(pb);

        cbCreateParams = *((LPWORD)pb);

        /*
         * skip any CreateParams which include the generated size WORD.
         */
        if (cbCreateParams)
            pb += cbCreateParams;

        pb += sizeof(WORD);

        /*
         * Point at the next dialog item. (DWORD aligned)
         */
        pb = (LPBYTE)(((ULONG_PTR)pb + 3) & ~3);
    }

    /*
     * Return template size.
     */
    return (DWORD)(pb - (LPBYTE)pdt);
}

/***************************************************************************\
* DialogBoxIndirectParamA (API)
* DialogBoxIndirectParamW (API)
*
* Creates the dialog and goes into a modal loop processing input for it.
*
* 05-Apr-1991 ScottLu   Created.
\***************************************************************************/

INT_PTR WINAPI DialogBoxIndirectParamA(
    HINSTANCE       hmod,
    LPCDLGTEMPLATEA lpDlgTemplate,
    HWND            hwndOwner,
    DLGPROC         lpDialogFunc,
    LPARAM          dwInitParam)
{
    return DialogBoxIndirectParamAorW(hmod,
                                      (LPCDLGTEMPLATEW)lpDlgTemplate,
                                      hwndOwner,
                                      lpDialogFunc,
                                      dwInitParam,
                                      SCDLG_ANSI);
}

INT_PTR WINAPI DialogBoxIndirectParamW(
    HINSTANCE       hmod,
    LPCDLGTEMPLATEW lpDlgTemplate,
    HWND            hwndOwner,
    DLGPROC         lpDialogFunc,
    LPARAM          dwInitParam)
{
    return DialogBoxIndirectParamAorW(hmod,
                                      lpDlgTemplate,
                                      hwndOwner,
                                      lpDialogFunc,
                                      dwInitParam,
                                      0);
}

INT_PTR WINAPI DialogBoxIndirectParamAorW(
    HINSTANCE       hmod,
    LPCDLGTEMPLATEW lpDlgTemplate,
    HWND            hwndOwner,
    DLGPROC         lpDialogFunc,
    LPARAM          dwInitParam,
    UINT            fAnsiFlags)
{
    DWORD cb;

    /*
     * The server routine destroys the menu if it fails.
     */
    cb = GetSizeDialogTemplate(hmod, lpDlgTemplate);

    if (!cb) {
        RIPMSG0(RIP_WARNING, "DialogBoxIndirectParam: Invalid Paramter");
        return -1;
    }

    return InternalDialogBox(hmod,
                            (LPDLGTEMPLATE)lpDlgTemplate,
                            hwndOwner,
                            lpDialogFunc,
                            dwInitParam,
                            SCDLG_CLIENT | (fAnsiFlags & (SCDLG_ANSI | SCDLG_16BIT)));
}

/***************************************************************************\
* CreateDialogIndirectParamA (API)
* CreateDialogIndirectParamW (API)
*
* Creates a dialog given a template and return s the window handle.
* fAnsi determines if the dialog has an ANSI or UNICODE lpDialogFunc
*
* 05-Apr-1991 ScottLu   Created.
\***************************************************************************/

HWND WINAPI CreateDialogIndirectParamA(
    HINSTANCE       hmod,
    LPCDLGTEMPLATEA lpDlgTemplate,
    HWND            hwndOwner,
    DLGPROC         lpDialogFunc,
    LPARAM          dwInitParam)
{
    return CreateDialogIndirectParamAorW(hmod,
                                         (LPCDLGTEMPLATE)lpDlgTemplate,
                                         hwndOwner,
                                         lpDialogFunc,
                                         dwInitParam,
                                         SCDLG_ANSI);
}

HWND WINAPI CreateDialogIndirectParamW(
    HINSTANCE       hmod,
    LPCDLGTEMPLATEW lpDlgTemplate,
    HWND            hwndOwner,
    DLGPROC         lpDialogFunc,
    LPARAM          dwInitParam)
{
    return CreateDialogIndirectParamAorW(hmod,
                                         (LPCDLGTEMPLATE)lpDlgTemplate,
                                         hwndOwner,
                                         lpDialogFunc,
                                         dwInitParam,
                                         0);
}

HWND WINAPI CreateDialogIndirectParamAorW(
    HANDLE         hmod,
    LPCDLGTEMPLATE lpDlgTemplate,
    HWND           hwndOwner,
    DLGPROC        lpDialogFunc,
    LPARAM         dwInitParam,
    UINT           fAnsi)
{
    DWORD cb;
    HWND  hwndRet;

    /*
     * The server routine destroys the menu if it fails.
     */
    cb = GetSizeDialogTemplate(hmod, lpDlgTemplate);

    if (!cb) {
        RIPMSG0(RIP_WARNING, "CreateDialogIndirect: Invalid Parameter");
        return NULL;
    }

    hwndRet = InternalCreateDialog(hmod,
                                   (LPDLGTEMPLATE)lpDlgTemplate,
                                   cb,
                                   hwndOwner,
                                   lpDialogFunc,
                                   dwInitParam,
                                   SCDLG_CLIENT | (fAnsi & (SCDLG_ANSI|SCDLG_16BIT)));

    return hwndRet;
}

/***************************************************************************\
* DialogBoxParamA (API)
* DialogBoxParamW (API)
*
* Loads the resource, creates the dialog and goes into a modal loop processing
* input for it.
*
* 05-Apr-1991 ScottLu   Created.
\***************************************************************************/

INT_PTR WINAPI DialogBoxParamA(
    HINSTANCE hmod,
    LPCSTR    lpName,
    HWND      hwndOwner,
    DLGPROC   lpDialogFunc,
    LPARAM    dwInitParam)
{
    HANDLE h;
    PVOID  p;
    INT_PTR i = -1;

    if (h = FINDRESOURCEA(hmod, (LPSTR)lpName, (LPSTR)RT_DIALOG)) {

        if (h = LOADRESOURCE(hmod, h)) {

            if (p = LOCKRESOURCE(h, hmod)) {

                i = DialogBoxIndirectParamAorW(hmod,
                                               p,
                                               hwndOwner,
                                               lpDialogFunc,
                                               dwInitParam,
                                               SCDLG_ANSI);

                UNLOCKRESOURCE(h, hmod);
            }

            FREERESOURCE(h, hmod);
        }
    }

    return i;
}

INT_PTR WINAPI DialogBoxParamW(
    HINSTANCE hmod,
    LPCWSTR   lpName,
    HWND      hwndOwner,
    DLGPROC   lpDialogFunc,
    LPARAM    dwInitParam)
{
    HANDLE h;
    PVOID  p;
    INT_PTR i = -1;

    UserAssert(LOWORD(hmod) == 0); // This should never be a WOW module

    if (h = FINDRESOURCEW(hmod, lpName, RT_DIALOG)) {

        if (p = LoadResource(hmod, h)) {

            i = DialogBoxIndirectParamAorW(hmod,
                                           p,
                                           hwndOwner,
                                           lpDialogFunc,
                                           dwInitParam,
                                           0);
        }
    }

    return i;
}

/***************************************************************************\
* CreateDialogParamA (API)
* CreateDialogParamW (API)
*
* Loads the resource, creates a dialog from that template, return s the
* window handle.
*
* 05-Apr-1991 ScottLu   Created.
\***************************************************************************/

HWND WINAPI CreateDialogParamA(
    HINSTANCE hmod,
    LPCSTR    lpName,
    HWND      hwndOwner,
    DLGPROC   lpDialogFunc,
    LPARAM    dwInitParam)
{
    HANDLE         h;
    LPDLGTEMPLATEA p;
    HWND           hwnd = NULL;

    if (h = FINDRESOURCEA(hmod, lpName, (LPSTR)RT_DIALOG)) {

        if (h = LOADRESOURCE(hmod, h)) {

            if (p = (LPDLGTEMPLATEA)LOCKRESOURCE(h, hmod)) {

                hwnd = CreateDialogIndirectParamAorW(hmod,
                                                     (LPCDLGTEMPLATE)p,
                                                     hwndOwner,
                                                     lpDialogFunc,
                                                     dwInitParam,
                                                     SCDLG_ANSI);

                UNLOCKRESOURCE(h, hmod);
            }

            FREERESOURCE(h, hmod);
        }
    }

    return hwnd;
}

HWND WINAPI CreateDialogParamW(
    HINSTANCE hmod,
    LPCWSTR   lpName,
    HWND      hwndOwner,
    DLGPROC   lpDialogFunc,
    LPARAM    dwInitParam)
{
    HANDLE h;
    PVOID  p;
    HWND   hwnd = NULL;

    if (h = FINDRESOURCEW(hmod, lpName, RT_DIALOG)) {

        if (h = LOADRESOURCE(hmod, h)) {

            if (p = LOCKRESOURCE(h, hmod)) {

                hwnd = CreateDialogIndirectParamAorW(hmod,
                                                     p,
                                                     hwndOwner,
                                                     lpDialogFunc,
                                                     dwInitParam,
                                                     0);

                UNLOCKRESOURCE(h, hmod);
            }

            FREERESOURCE(h, hmod);
        }
    }

    return hwnd;
}

/***************************************************************************\
* DestroyCursor (API)
*
* Client wrapper for NtUserDestroyCursor.
*
* 28-Nov-1994 JimA      Created.
\***************************************************************************/

BOOL WINAPI DestroyCursor(
    HCURSOR hcur)
{
    return NtUserDestroyCursor(hcur, CURSOR_CALLFROMCLIENT);
}

/***************************************************************************\
* CreateIcoCur
*
*
\***************************************************************************/

HICON CreateIcoCur(
    PCURSORDATA lpi)
{
    HCURSOR hcur;

    UserAssert(lpi->hbmColor || lpi->hbmMask);

    hcur = (HCURSOR)NtUserCallOneParam((lpi->CURSORF_flags & CURSORF_GLOBAL),
                                       SFI__CREATEEMPTYCURSOROBJECT);

    if (hcur == NULL)
        return NULL;

#if DBG
    {
        BITMAP bmMask;
        BITMAP bmColor;

        UserAssert(GetObject(lpi->hbmMask, sizeof(BITMAP), &bmMask));
        
        /* Bug 252902 - joejo
         * Since the width and height of the mask bitmap is set below
         * we really don't need to assert on the width/height check. Throwing
         * a warning should be good enough.
         */
        if (bmMask.bmWidth != (LONG)lpi->cx) {
           RIPMSG1(RIP_WARNING, "Mask width not equal to requested width: lpi %#p", lpi);
        }

        if (bmMask.bmHeight != (LONG)lpi->cy) {
           RIPMSG1(RIP_WARNING, "Mask height not equal to requested height: lpi %#p", lpi);
        }

        if (lpi->hbmColor) {
            UserAssert(GetObject(lpi->hbmColor, sizeof(BITMAP), &bmColor));
            UserAssert(bmMask.bmHeight == bmColor.bmHeight * 2);
            UserAssert(bmMask.bmWidth  == bmColor.bmWidth);
        }
    }
#endif

    if (_SetCursorIconData(hcur, lpi))
        return hcur;

    NtUserDestroyCursor(hcur, CURSOR_ALWAYSDESTROY);

    return NULL;
}

/***************************************************************************\
* CreateIcoCurIndirect
*
*
\***************************************************************************/

HCURSOR CreateIcoCurIndirect(
    PCURSORDATA pcurCreate,
    UINT        cPlanes,
    UINT        cBitsPixel,
    CONST BYTE  *lpANDbits,
    CONST BYTE  *lpXORbits)
{
    int     cbBits;
    HCURSOR hcurNew;
    BOOL    bColor;
    UINT    cx;
    UINT    cy;
    LPBYTE  pBits = NULL;

    /*
     * Allocate CURSOR structure.
     */
    hcurNew = (HCURSOR)NtUserCallOneParam(0, SFI__CREATEEMPTYCURSOROBJECT);

    if (hcurNew == NULL)
        return NULL;

    /*
     * If there is no Color bitmap, create a single buffer that contains both
     * the AND and XOR bits.  The AND bitmap is always MonoChrome
     */
    bColor = (cPlanes | cBitsPixel) > 1;

    if (!bColor) {

        cbBits = (((pcurCreate->cx + 0x0F) & ~0x0F) >> 3) * pcurCreate->cy;

        pBits = (LPBYTE)UserLocalAlloc(HEAP_ZERO_MEMORY, (cbBits * 2));

        if (pBits == NULL) {
            NtUserDestroyCursor(hcurNew, CURSOR_ALWAYSDESTROY);
            return NULL;
        }

        RtlCopyMemory(pBits, lpANDbits, cbBits);
        RtlCopyMemory(pBits + cbBits, lpXORbits, cbBits);
        lpANDbits = pBits;
    }

    /*
     * Create hbmMask (its always MonoChrome)
     */
    cx = pcurCreate->cx;
    cy = pcurCreate->cy * 2;

    pcurCreate->hbmMask = CreateBitmap(cx, cy, 1, 1, lpANDbits);

    if (pcurCreate->hbmMask == NULL) {

        /*
         * If this is a COLOR icon/cursor, lpANDBits doesn't need to be
         * pcurCreate->cy * 2; indeed, we don't use this double height at all.
         * This is a bug that will be fixed post 4.0.
         * For now, let's try to handle the case where the CreateBitmap call
         * failed because the caller didn't pass in a double height AND mask
         * (Win95 doesn't have this bug)
         */
        if (bColor) {

            RIPMSG0(RIP_WARNING, "CreateIcoCurIndirect: Retrying hbmMask creation.");

            cbBits = (((pcurCreate->cx + 0x0F) & ~0x0F) >> 3) * pcurCreate->cy;
            pBits = (LPBYTE)UserLocalAlloc(HEAP_ZERO_MEMORY, cbBits*2);

            if (pBits == NULL) {
                NtUserDestroyCursor(hcurNew, CURSOR_ALWAYSDESTROY);
                return NULL;
            }

            RtlCopyMemory(pBits, lpANDbits, cbBits);
            pcurCreate->hbmMask = CreateBitmap(cx, cy, 1, 1, pBits);
            UserLocalFree(pBits);

            pBits = NULL;
        }

        if (pcurCreate->hbmMask == NULL) {

            /*
             * CreateBitmap() failed.  Clean-up and get out of here.
             */
            NtUserDestroyCursor(hcurNew, CURSOR_ALWAYSDESTROY);

            if (pBits != NULL)
                UserLocalFree(pBits);

            return NULL;
        }
    }

    /*
     * Create hbmColor or NULL it so that CallOEMCursor doesn't think we are
     * color.
     */
    if (bColor) {
        pcurCreate->hbmColor = CreateBitmap(cx,
                                            cy / 2,
                                            cPlanes,
                                            cBitsPixel,
                                            lpXORbits);

        if (pcurCreate->hbmColor == NULL) {

            /*
             * CreateBitmap() failed.  Clean-up and get out of here.
             */
            DeleteObject(pcurCreate->hbmMask);
            NtUserDestroyCursor(hcurNew, CURSOR_ALWAYSDESTROY);
            return NULL;
        }

        pcurCreate->bpp = (cPlanes * cBitsPixel);

    } else {
        pcurCreate->hbmColor = NULL;
        pcurCreate->bpp      = 1;
    }

    /*
     * Load contents into the cursor/icon object
     */
    pcurCreate->cy            = cy;
    pcurCreate->lpModName     = NULL;
    pcurCreate->lpName        = NULL;
    pcurCreate->rt            = 0;
    pcurCreate->CURSORF_flags = 0;

    if (_SetCursorIconData(hcurNew, pcurCreate)) {
        if (pBits != NULL)
            UserLocalFree(pBits);
        return hcurNew;
    }

    /*
     * Could not set up cursor/icon, so free resources.
     */
    NtUserDestroyCursor(hcurNew, CURSOR_ALWAYSDESTROY);
    DeleteObject(pcurCreate->hbmMask);

    if (pcurCreate->hbmColor)
        DeleteObject(pcurCreate->hbmColor);
    if (pBits != NULL)
        UserLocalFree(pBits);

    return NULL;
}

/***************************************************************************\
* CreateCursor (API)
*
* History:
* 26-Feb-1991 MikeKe    Created.
* 01-Aug-1991 IanJa     Init cur.pszModname or DestroyCursor will work
\***************************************************************************/

HCURSOR WINAPI CreateCursor(
    HINSTANCE hModule,
    int       iXhotspot,
    int       iYhotspot,
    int       iWidth,
    int       iHeight,
    LPBYTE    lpANDplane,
    LPBYTE    lpXORplane)
{
    CURSORDATA cur;
    UNREFERENCED_PARAMETER(hModule);

    if ((iXhotspot < 0) || (iXhotspot > iWidth) ||
        (iYhotspot < 0) || (iYhotspot > iHeight)) {
        return 0;
    }

    RtlZeroMemory(&cur, sizeof(cur));
    cur.xHotspot = (SHORT)iXhotspot;
    cur.yHotspot = (SHORT)iYhotspot;
    cur.cx       = (DWORD)iWidth;
    cur.cy       = (DWORD)iHeight;

    return CreateIcoCurIndirect(&cur, 1, 1, lpANDplane, lpXORplane);
}

/***************************************************************************\
* CreateIcon (API)
*
* History:
* 26-Feb-1991 MikeKe    Created.
* 01-Aug-1991 IanJa     Init cur.pszModname so DestroyIcon will work
\***************************************************************************/

HICON WINAPI CreateIcon(
    HINSTANCE  hModule,
    int        iWidth,
    int        iHeight,
    BYTE       planes,
    BYTE       bpp,
    CONST BYTE *lpANDplane,
    CONST BYTE *lpXORplane)
{
    CURSORDATA cur;
    UNREFERENCED_PARAMETER(hModule);

    RtlZeroMemory(&cur, sizeof(cur));
    cur.xHotspot = (SHORT)(iWidth / 2);
    cur.yHotspot = (SHORT)(iHeight / 2);
    cur.cx       = (DWORD)iWidth;
    cur.cy       = (DWORD)iHeight;

    return CreateIcoCurIndirect(&cur, planes, bpp, lpANDplane, lpXORplane);
}

/***************************************************************************\
* CreateIconIndirect (API)
*
* Creates an icon or cursor from an ICONINFO structure. Does not destroy
* cursor/icon bitmaps.
*
* 24-Jul-1991 ScottLu   Created.
\***************************************************************************/
HICON WINAPI CreateIconIndirect(
    PICONINFO piconinfo)
{
    HCURSOR    hcur;
    CURSORDATA cur;
    BITMAP     bmMask;
    BITMAP     bmColor;
    HBITMAP    hbmpBits2, hbmpMem;
    HDC        hdcMem;

    /*
     * Make sure the bitmaps are real, and get their dimensions.
     */
    if (!GetObjectW(piconinfo->hbmMask, sizeof(BITMAP), &bmMask))
        return NULL;

    if (piconinfo->hbmColor != NULL) {
        if (!GetObjectW(piconinfo->hbmColor, sizeof(BITMAP), &bmColor))
            return NULL;
    }

    /*
     * Allocate CURSOR structure.
     */
    hcur = (HCURSOR)NtUserCallOneParam(0, SFI__CREATEEMPTYCURSOROBJECT);
    if (hcur == NULL)
        return NULL;

    /*
     * Internally, USER stores the height as 2 icons high - because when
     * loading bits from a resource, in both b/w and color icons, the
     * bits are stored on top of one another (AND/XOR mask, AND/COLOR bitmap).
     * When bitmaps are passed in to CreateIconIndirect(), they are passed
     * as two bitmaps in the color case, and one bitmap (with the stacked
     * masks) in the black and white case.  Adjust cur.cy so it is 2 icons
     * high in both cases.
     */

    RtlZeroMemory(&cur, sizeof(cur));
    cur.cx = bmMask.bmWidth;

    if (piconinfo->hbmColor == NULL) {

        cur.cy  = bmMask.bmHeight;
        cur.bpp = 1;

    } else {
        cur.cy       = bmMask.bmHeight * 2;
        cur.bpp      = (DWORD)(bmColor.bmBitsPixel * bmColor.bmPlanes);
        cur.hbmColor = CopyBmp(piconinfo->hbmColor, 0, 0, LR_DEFAULTCOLOR);

        if (cur.hbmColor == NULL) {
            RIPMSG0(RIP_WARNING, "CreateIconIndirect: Failed to copy piconinfo->hbmColor");
            goto CleanUp;
        }
    }

    /*
     * hbmMask must always be double height, even for color icons.
     * So cy might be equal to bmMask.bmHeight * 2 at this point.
     * If this is the case, the second half of hbmMask won't be initilized;
     * nobody is supposed to use it but GDI expects it there when checking the
     * bitmap dimensions (for cursors)
     */
    cur.hbmMask  =  CreateBitmap(cur.cx, cur.cy, 1, 1, NULL);

    if (cur.hbmMask == NULL) {
        RIPMSG0(RIP_WARNING, "CreateIconIndirect: Failed to create cur.hbmMask");
        goto CleanUp;
    }

    RtlEnterCriticalSection(&gcsHdc);


    if (hdcMem = CreateCompatibleDC (ghdcBits2)) {

        hbmpMem = SelectObject(hdcMem, cur.hbmMask);
        hbmpBits2 = SelectObject(ghdcBits2, piconinfo->hbmMask);

        BitBlt(hdcMem,
               0,
               0,
               bmMask.bmWidth,
               bmMask.bmHeight,
               ghdcBits2,
               0,
               0,
               SRCCOPY);

        SelectObject(hdcMem, hbmpMem);
        SelectObject(ghdcBits2, hbmpBits2);
        DeleteDC (hdcMem);

    } else {

        RtlLeaveCriticalSection(&gcsHdc);
        RIPMSG0(RIP_WARNING, "CreateIconIndirect: CreateCompatibleDC failed");
        goto CleanUp;
    }

    RtlLeaveCriticalSection(&gcsHdc);

    /*
     * rt and Hotspot
     */
    if (piconinfo->fIcon) {
        cur.rt        = PTR_TO_ID(RT_ICON);
        cur.xHotspot = (SHORT)(cur.cx / 2);
        cur.yHotspot = (SHORT)(cur.cy / 4);
    } else {
        cur.rt        = PTR_TO_ID(RT_CURSOR);
        cur.xHotspot = ((SHORT)piconinfo->xHotspot);
        cur.yHotspot = ((SHORT)piconinfo->yHotspot);
    }


    if (_SetCursorIconData(hcur, &cur)) {
        return hcur;
    }

CleanUp:
    /*
     * Note that if this fails, the bitmaps have NOT been made public.
     */
    if (cur.hbmMask != NULL) {
        DeleteObject(cur.hbmMask);
    }
    if (cur.hbmColor != NULL) {
        DeleteObject(cur.hbmColor);
    }

    NtUserDestroyCursor(hcur, CURSOR_ALWAYSDESTROY);
    return NULL;
}

/***************************************************************************\
* GetIconInfo (API)
*
* Returns icon information, including bitmaps.
*
* 24-Jul-1991 ScottLu   Created.
\***************************************************************************/

BOOL WINAPI GetIconInfo(
    HICON     hicon,
    PICONINFO piconinfo)
{
    return NtUserGetIconInfo(hicon, piconinfo, NULL, NULL, NULL, FALSE);
}

/***************************************************************************\
* GetCursorFrameInfo (API)
*
* Returns cursor information.
*
* 24-Jul-1991 ScottLu   Created.
\***************************************************************************/

HCURSOR WINAPI GetCursorFrameInfo(
    HCURSOR hcur,
    LPWSTR  lpName,
    int     iFrame,
    LPDWORD pjifRate,
    LPINT   pccur)
{
    /*
     * Caller wants us to return the version of this cursor that is stored
     * in the display driver.
     */
    if (hcur == NULL) {

        return LoadIcoCur(NULL,
                          lpName,
                          RT_CURSOR,
                          0,
                          0,
                          LR_DEFAULTSIZE);
    }

    return NtUserGetCursorFrameInfo(hcur, iFrame, pjifRate, pccur);
}

/***************************************************************************\
* _FreeResource   (API)
* _LockResource   (API)
* _UnlockResource (API)
*
* These are dummy routines that need to exist for the apfnResCallNative
* array, which is used when calling the run-time libraries.
*
\***************************************************************************/

BOOL WINAPI _FreeResource(
    HANDLE    hResData,
    HINSTANCE hModule)
{
    UNREFERENCED_PARAMETER(hResData);
    UNREFERENCED_PARAMETER(hModule);

    return FALSE;
}

LPSTR WINAPI _LockResource(
    HANDLE    hResData,
    HINSTANCE hModule)
{
    UNREFERENCED_PARAMETER(hModule);

    return (LPSTR)(hResData);
}

BOOL WINAPI _UnlockResource(
    HANDLE    hResData,
    HINSTANCE hModule)
{
    UNREFERENCED_PARAMETER(hResData);
    UNREFERENCED_PARAMETER(hModule);

    return TRUE;
}

/***************************************************************************\
* LookupIconIdFromDirectory (API)
*
* This searches through an icon directory for the icon that best fits the
* current display device.
*
* 24-07-1991 ScottLu    Created.
\***************************************************************************/

int WINAPI LookupIconIdFromDirectory(
    PBYTE presbits,
    BOOL  fIcon)
{
    return LookupIconIdFromDirectoryEx(presbits, fIcon, 0, 0, 0);
}

/***************************************************************************\
* LookupIconIdFromDirectoryEx (API)
*
*
\***************************************************************************/

int WINAPI LookupIconIdFromDirectoryEx(
    PBYTE           presbits,
    BOOL            fIcon,
    int             cxDesired,
    int             cyDesired,
    UINT            LR_flags)
{
    ConnectIfNecessary();

    return RtlGetIdFromDirectory(presbits,
                                 fIcon,
                                 cxDesired,
                                 cyDesired,
                                 LR_flags,
                                 NULL);
}
/***************************************************************************\
* LoadCursorIconFromResource (API)
*
* Loads animated icon/cursor from a pointer to a resource
*
* 02-20-1996 GerardoB    Created.
\***************************************************************************/
HANDLE LoadCursorIconFromResource(
    PBYTE   presbits,
    LPCWSTR lpName,
    int     cxDesired,
    int     cyDesired,
    UINT    LR_flags)
{
    BOOL     fAni;
    FILEINFO fi;
    LPWSTR   lpwszRT;

    fi.pFileMap = presbits;
    fi.pFilePtr = fi.pFileMap;
    fi.pFileEnd = fi.pFileMap + sizeof (RTAG) + ((RTAG *)presbits)->ckSize;
    fi.pszName  = lpName;

    return LoadCursorIconFromFileMap(&fi,
                                     &lpwszRT,
                                     cxDesired,
                                     cyDesired,
                                     LR_flags,
                                     &fAni);
}
/***************************************************************************\
* CreateIconFromResource (API)
*
* Takes resource bits and creates either an icon or cursor.
*
* 24-07-1991 ScottLu    Created.
\***************************************************************************/

HICON WINAPI CreateIconFromResource(
    PBYTE presbits,
    DWORD dwResSize,
    BOOL  fIcon,
    DWORD dwVer)
{
    return CreateIconFromResourceEx(presbits,
                                    dwResSize,
                                    fIcon,
                                    dwVer,
                                    0,
                                    0,
                                    LR_DEFAULTSIZE | LR_SHARED);
}

/***************************************************************************\
* CreateIconFromResourceEx (API)
*
* Takes resource bits and creates either an icon or cursor.
*
* 30-Aug-1994 FritzS    Created
\***************************************************************************/

HICON WINAPI CreateIconFromResourceEx(
    PBYTE presbits,
    DWORD dwResSize,
    BOOL  fIcon,
    DWORD dwVer,
    int   cxDesired,
    int   cyDesired,
    UINT  LR_flags)
{
    UNREFERENCED_PARAMETER(dwResSize);

    /*
     * NT Specific code to validate the version.
     */
    if ((dwVer < 0x00020000) || (dwVer > 0x00030000)) {
        RIPMSG0(RIP_WARNING, "CreateIconFromResourceEx: Invalid Paramter");
        return NULL;
    }

    /*
     * Set desired size of resource based on flags and/or true
     * dimensions passed in.
     */
    cxDesired = GetIcoCurWidth(cxDesired , fIcon, LR_flags, 0);
    cyDesired = GetIcoCurHeight(cyDesired, fIcon, LR_flags, 0);

    if (ISRIFFFORMAT(presbits)) {
        return LoadCursorIconFromResource (presbits, NULL, cxDesired, cyDesired, LR_flags);
    } else {
        return ConvertDIBIcon((LPBITMAPINFOHEADER)presbits,
                              NULL,
                              NULL,
                              fIcon,
                              cxDesired,
                              cyDesired,
                              LR_flags);
    }
}

/***************************************************************************\
* Convert1BppToMonoBitmap
*
* This routine converts a 1bpp bitmap to a true monochrome surface.  This
* is done for bitmaps which need to do foreground/background color matching
* at output time.  Otherwise, a 1bpp will just match to its palette.
*
* NOTE: This routine deletes the original bitmap if successful.  If failure
*       we'll return the original bitmap.
*
* History:
* 17-Apr-1996 ChrisWil  Created
\***************************************************************************/

HBITMAP Convert1BppToMonoBitmap(
    HDC     hdcSrc,
    HBITMAP hbm1Bpp)
{
    HBITMAP hbmMono = hbm1Bpp;
    HBITMAP hbmDst;
    HBITMAP hbmS;
    HBITMAP hbmD;
    HDC     hdcDst;
    BITMAP  bm;

    if (hdcDst = CreateCompatibleDC(hdcSrc)) {

        GetObject(hbm1Bpp, sizeof(BITMAP), &bm);

        if (hbmDst = CreateBitmap(bm.bmWidth, bm.bmHeight, 1, 1, NULL)) {

            hbmS = SelectBitmap(hdcSrc, hbm1Bpp);
            hbmD = SelectBitmap(hdcDst, hbmDst);

            BitBlt(hdcDst,
                   0,
                   0,
                   bm.bmWidth,
                   bm.bmHeight,
                   hdcSrc,
                   0,
                   0,
                   SRCCOPY);

            SelectBitmap(hdcSrc, hbmS);
            SelectBitmap(hdcDst, hbmD);

            hbmMono = hbmDst;
            DeleteObject(hbm1Bpp);
        }

        DeleteDC(hdcDst);
    }

    return hbmMono;
}

/***************************************************************************\
* CreateScreenBitmap
*
* This routine creates a screen bitmap.  We use the CreateDIBitmap call
* to do compatible color-matching with Win95.  Also, note that this
* routine takes in WORD aligned bits.
*
\***************************************************************************/

HBITMAP CreateScreenBitmap(
    int    cx,
    int    cy,
    UINT   planes,
    UINT   bpp,
    LPSTR  lpBits,
    LPBOOL pf1Bpp)
{
    HDC     hdcScreen;
    HBITMAP hbm = NULL;
    DWORD   dwCount;

    static struct {
        BITMAPINFOHEADER bi;
        DWORD            ct[16];
    } dib4Vga = {{sizeof(BITMAPINFOHEADER),
                  0,
                  0,
                  1,
                  4,
                  BI_RGB,
                  0,
                  0,
                  0,
                  16,
                  0
                 },
                 {0x00000000,
                  0x00800000,
                  0x00008000,
                  0x00808000,
                  0x00000080,
                  0x00800080,
                  0x00008080,
                  0x00C0C0C0,
                  0x00808080,
                  0x00FF0000,
                  0x0000FF00,
                  0x00FFFF00,
                  0x000000FF,
                  0x00FF00FF,
                  0x0000FFFF,
                  0x00FFFFFF
                 }
                };

    static struct {
        BITMAPINFOHEADER bi;
        DWORD            ct[2];
    } dib1Vga = {{sizeof(BITMAPINFOHEADER),
                  0,
                  0,
                  1,
                  1,
                  BI_RGB,
                  0,
                  0,
                  0,
                  2,
                  0
                 },
                 {0x00000000,
                  0x00FFFFFF
                 }
                };


    /*
     * Create the surface.
     */
    if (hdcScreen = GETINITDC()) {

        /*
         * This appears to mess up color to mono conversion by losing all
         * the data and forcing all non-forground colors to black.
         * (try copyimage with IDC_WARNING_DEFAULT)
         * This is what win95 does but their system works.  The scary thing
         * (according to marke) is that win95 may have changed GDI to make
         * this work.
         *
         * In order to get nearest-color-matching compatible with Win95,
         * we're going to need to use the CreateDIBitmap() for mono-surfaces.
         * This code-path will do nearest-color, rather than color-matching.
         */
        if ((bpp == 1) && (planes == 1)) {

            dib1Vga.bi.biWidth  = cx;
            dib1Vga.bi.biHeight = cy;

            hbm = CreateDIBitmap(hdcScreen,
                                 (LPBITMAPINFOHEADER)&dib1Vga,
                                 CBM_CREATEDIB,
                                 NULL,
                                 (LPBITMAPINFO)&dib1Vga,
                                 DIB_RGB_COLORS);

            *pf1Bpp = TRUE;

        } else {

            if (((planes == 0) || (planes == gpsi->Planes)) &&
                ((bpp == 0) || (bpp == gpsi->BitsPixel))) {

                hbm = CreateCompatibleBitmap(hdcScreen, cx, cy);

            } else {

                dib4Vga.bi.biBitCount = planes * bpp ? planes * bpp : gpsi->BitCount;

#if 0 // We use to do the dib-section create, but this breaks icons
      // when they are made public (can't make a dibsection public). So
      // we now wil create this as a real-dib.
      //
                {
                DWORD dwDummy;

                dib4Vga.bi.biWidth    =  cx;
                dib4Vga.bi.biHeight   = -cy;     // top-down DIB (like a DDB)

                hbm = CreateDIBSection(hdcScreen,
                                       (LPBITMAPINFO)&dib4Vga,
                                       DIB_RGB_COLORS,
                                       (LPVOID)&dwDummy,
                                       0,
                                       0);
                }
#else
                dib4Vga.bi.biWidth  = cx;
                dib4Vga.bi.biHeight = cy;

                hbm = CreateDIBitmap(hdcScreen,
                                     (LPBITMAPINFOHEADER)&dib4Vga,
                                     CBM_CREATEDIB,
                                     NULL,
                                     (LPBITMAPINFO)&dib4Vga,
                                     DIB_RGB_COLORS);
#endif
            }
        }

        RELEASEINITDC(hdcScreen);
    }

    if (hbm && lpBits) {

        BITMAP bm;

        GetObject(hbm, sizeof(BITMAP), &bm);
        dwCount = (DWORD)(UINT)(bm.bmWidthBytes * bm.bmPlanes) * (DWORD)(UINT)cy;
        SetBitmapBits(hbm, dwCount, lpBits);
    }

    return hbm;
}

/***************************************************************************\
* LoadBmp (Worker)
*
* This routine decides whether the bitmap to be loaded is in old or new (DIB)
* format and calls appropriate handlers.
*
* History:
* 24-Sep-1990 MikeKe    From Win30.
* 18-Jun-1991 ChuckWh   Added local bitmap handle support.
* 05-Sep-1995 ChrisWil  Port/Change for Chicago functionality.
\***************************************************************************/

HBITMAP LoadBmp(
    HINSTANCE hmod,
    LPCWSTR   lpName,
    int       cxDesired,
    int       cyDesired,
    UINT      flags)
{
    HBITMAP hbmp = NULL;
    BOOL    fFree = FALSE;
    BOOL    f1Bpp = FALSE;

/***************************************************************************\
* Bitmap Resource Table
*
* As of WIN4.0, most system bitmaps are rendered instead of grabbed from the
* display driver.  However, a lot of apps, especially those that fake their
* own MDI, do LoadBitmap(NULL, OBM_...) to grab a system bitmap.  So we
* hook those requests here and copy our rendered bitmaps into a newly-
* created bitmap.  Note that this is actually faster than loading from a
* resource table!
*
* BOGUS -- give 'em old close buttons, not new cool X's
*
\***************************************************************************/
#define MAX_BMPMAP  32

    CONST static MAPRES MapOemBmp[MAX_BMPMAP] = {

        {OBM_BTNCORNERS , OBI_RADIOMASK      ,               },
        {OBM_BTSIZE     , OBI_NCGRIP         ,               },
        {OBM_CHECK      , OBI_MENUCHECK      , MR_MONOCHROME },
        {OBM_CHECKBOXES , OBI_CHECK          ,               },
        {OBM_COMBO      , OBI_DNARROW        ,               },
        {OBM_DNARROW    , OBI_DNARROW        ,               },
        {OBM_DNARROWD   , OBI_DNARROW_D      ,               },
        {OBM_DNARROWI   , OBI_DNARROW_I      ,               },
        {OBM_LFARROW    , OBI_LFARROW        ,               },
        {OBM_LFARROWD   , OBI_LFARROW_D      ,               },
        {OBM_LFARROWI   , OBI_LFARROW_I      ,               },

        /*
         * Use MONO bitmaps in future once flat/mono controls are worked out.
         */
        {OBM_OLD_DNARROW, OBI_DNARROW        , MR_FAILFOR40  },
        {OBM_OLD_LFARROW, OBI_LFARROW        , MR_FAILFOR40  },
        {OBM_OLD_REDUCE , OBI_REDUCE_MBAR    , MR_FAILFOR40  },
        {OBM_OLD_RESTORE, OBI_RESTORE_MBAR   , MR_FAILFOR40  },
        {OBM_OLD_RGARROW, OBI_RGARROW        , MR_FAILFOR40  },
        {OBM_OLD_UPARROW, OBI_UPARROW        , MR_FAILFOR40  },
        {OBM_OLD_ZOOM   , OBI_ZOOM           , MR_FAILFOR40  },

        {OBM_MNARROW    , OBI_MENUARROW      , MR_MONOCHROME },
        {OBM_REDUCE     , OBI_REDUCE_MBAR    ,               },
        {OBM_REDUCED    , OBI_REDUCE_MBAR_D  ,               },
        {OBM_RESTORE    , OBI_RESTORE_MBAR   ,               },
        {OBM_RESTORED   , OBI_RESTORE_MBAR_D ,               },
        {OBM_RGARROW    , OBI_RGARROW        ,               },
        {OBM_RGARROWD   , OBI_RGARROW_D      ,               },
        {OBM_RGARROWI   , OBI_RGARROW_I      ,               },
        {OBM_SIZE       , OBI_NCGRIP         ,               },
        {OBM_UPARROW    , OBI_UPARROW        ,               },
        {OBM_UPARROWD   , OBI_UPARROW_D      ,               },
        {OBM_UPARROWI   , OBI_UPARROW_I      ,               },
        {OBM_ZOOM       , OBI_ZOOM           ,               },
        {OBM_ZOOMD      , OBI_ZOOM_D         ,               }
    };


    /*
     * If hmod is valid, load the client-side bits.
     */
    if (hmod == NULL) {

        HBITMAP hOldBmp;
        WORD    bm;
        WORD    wID;
        BOOL    fCombo;
        BOOL    fCheckBoxes;
        int     i;
        RECT    rc;
        BOOL    fSysMenu = FALSE;
        BOOL    fMenu = FALSE;
        BOOL    fMono = FALSE;

        hmod = hmodUser;

        /*
         * Since the resource is coming from USER32, we only
         * deal with ID types.
         */
        wID = PTR_TO_ID(lpName);

        switch(wID) {
        case OBM_OLD_CLOSE:
            if (GETAPPVER() >= VER40)
                goto FailOldLoad;

            /*
             * fall through to the Close case.
             */

        case OBM_CLOSE:
            /* the new look for the system menu is to use the window's
             * class icon -- but since here we don't know which window
             * they'll be using this for, fall back on the good ole'
             * windows logo icon
             */
            cxDesired = (SYSMET(CXMENUSIZE) + SYSMET(CXEDGE)) * 2;
            cyDesired = SYSMET(CYMENUSIZE) + (2 * SYSMET(CYEDGE));
            fSysMenu  = TRUE;
            break;

        case OBM_TRUETYPE: {

                PVOID  p;
                HANDLE h;
                int    nOffset;

                /*
                 * Offset into resource.
                 */
                if (gpsi->dmLogPixels == 120) {
                    nOffset = OFFSET_120_DPI;
                } else {
                    nOffset = OFFSET_96_DPI;
                }

                lpName = (LPWSTR)(MAX_RESOURCE_INDEX -
                        ((ULONG_PTR)lpName) + nOffset);

                if (h = FINDRESOURCEW(hmod, (LPWSTR)lpName, RT_BITMAP)) {

                    if (h = LOADRESOURCE(hmod, h)) {

                        if (p = LOCKRESOURCE(h, hmod)) {


                            hbmp = (HBITMAP)ObjectFromDIBResource(hmod,
                                                                  lpName,
                                                                  RT_BITMAP,
                                                                  cxDesired,
                                                                  cyDesired,
                                                                  flags);

                            UNLOCKRESOURCE(h, hmod);
                        }

                        FREERESOURCE(h, hmod);
                    }
                }

                goto LoadBmpDone;
            }
            break;

        default:
            fCombo      = (wID == OBM_COMBO);
            fCheckBoxes = (wID == OBM_CHECKBOXES);

            /*
             * hard loop to check for mapping.
             */
            for (i=0; (i < MAX_BMPMAP) && (MapOemBmp[i].idDisp != wID); i++);

            if (i == MAX_BMPMAP)
                goto LoadForReal;

            if ((MapOemBmp[i].bFlags & MR_FAILFOR40) &&
                    (GETAPPVER() >= VER40)) {

FailOldLoad:
                RIPMSG0(RIP_WARNING, "LoadBitmap: old IDs not allowed for 4.0 apps");
                return NULL;
            }

            if (MapOemBmp[i].bFlags & MR_MONOCHROME)
                fMono = TRUE;

            bm = MapOemBmp[i].idUser;

            if ((bm == OBI_REDUCE_MBAR) || (bm == OBI_RESTORE_MBAR))
                fMenu = TRUE;

            cxDesired = gpsi->oembmi[bm].cx;
            cyDesired = gpsi->oembmi[bm].cy;

            if (fMenu)
                cyDesired += (2 * SYSMET(CYEDGE));

            if (fCheckBoxes) {
                cxDesired *= NUM_BUTTON_STATES;
                cyDesired *= NUM_BUTTON_TYPES;
            } else if (fCombo) {
                cxDesired -= (2 * SYSMET(CXEDGE));
                cyDesired -= (2 * SYSMET(CYEDGE));
            }
            break;
        }

        /*
         * Creates DIB section or color compatible.
         */
        if (fMono) {

            /*
             * Create mono-bitmaps as DIBs on NT.  On Win95 this is
             * called as:
             *
             *   hbmp = CreateBitmap(cxDesired, cyDesired, 1, 1, NULL);
             *
             * However, due to color-matching differences, we need to
             * use dibs to get the nearest-color-matching.  At the
             * end of this routine we will convert to a true-mono so that
             * foreground/background matching can be performed normally.
             */
            hbmp = CreateScreenBitmap(cxDesired, cyDesired, 1, 1, NULL, &f1Bpp);

        } else {

            hbmp = CreateScreenBitmap(cxDesired, cyDesired, 0, 0, NULL, &f1Bpp);
        }

        if (hbmp == NULL)
            goto LoadBmpDone;

        RtlEnterCriticalSection(&gcsHdc);
        hOldBmp = SelectBitmap(ghdcBits2, hbmp);
        UserAssert(GetBkColor(ghdcBits2) == RGB(255,255,255));
        UserAssert(GetTextColor(ghdcBits2) == RGB(0, 0, 0));

        rc.top    = 0;
        rc.left   = 0;
        rc.bottom = cyDesired;
        rc.right  = cxDesired;

        if (fMono) {
            PatBlt(ghdcBits2, 0, 0, cxDesired, cyDesired, WHITENESS);
        } else {
            FillRect(ghdcBits2,
                     &rc,
                     ((fMenu | fSysMenu) ? SYSHBR(MENU) : SYSHBR(WINDOW)));
        }

        if (fSysMenu) {
            int x = SYSMET(CXEDGE);
            int i;

            cxDesired /= 2;

            for (i=0; i < 2; i++) {

                DrawIconEx(ghdcBits2,
                           x,
                           SYSMET(CYEDGE),
                           gpsi->hIconSmWindows,
                           cxDesired - 2 * SYSMET(CXEDGE),
                           SYSMET(CYMENUSIZE) - SYSMET(CYEDGE),
                           0,
                           NULL,
                           DI_NORMAL);

                x += cxDesired;
            }

        } else if (fCombo) {

            /*
             * Revisit when we start using TTF -- that'll take care of
             * this hack.
             */
            rc.top     = -SYSMET(CYEDGE);
            rc.bottom +=  SYSMET(CYEDGE);
            rc.left    = -SYSMET(CXEDGE);
            rc.right  +=  SYSMET(CXEDGE);

            DrawFrameControl(ghdcBits2,
                             &rc,
                             DFC_SCROLL,
                             DFCS_SCROLLDOWN);

        } else if (fCheckBoxes) {

            int   wType;
            int   wState;
            int   x;
            DWORD clrTextSave;
            DWORD clrBkSave;
            int   y = 0;

            for (wType=0; wType < NUM_BUTTON_TYPES; wType++) {

                x = 0;

                cxDesired = gpsi->oembmi[bm].cx;
                cyDesired = gpsi->oembmi[bm].cy;

                if (wType == 1) {

                    /*
                     * BOGUS UGLINESS -- will be fixed once the Graphics dudes
                     * get me the icon TTF -- I'll revisit this then and make
                     * REAL
                     */
                    clrTextSave = SetTextColor(ghdcBits2, RESCLR_BLACK);
                    clrBkSave   = SetBkColor  (ghdcBits2, RESCLR_WHITE);

                    for (wState = 0; wState < NUM_BUTTON_STATES; wState++) {

                        NtUserBitBltSysBmp(ghdcBits2,
                                           x,
                                           y,
                                           cxDesired,
                                           cyDesired,
                                           gpsi->oembmi[OBI_RADIOMASK].x,
                                           gpsi->oembmi[OBI_RADIOMASK].y,
                                           SRCAND);

                        NtUserBitBltSysBmp(ghdcBits2,
                                           x,
                                           y,
                                           cxDesired,
                                           cyDesired,
                                           gpsi->oembmi[bm].x,
                                           gpsi->oembmi[bm].y,
                                           SRCINVERT);
                        x += cxDesired;
                        bm++;
                    }

                    SetTextColor(ghdcBits2, clrTextSave);
                    SetBkColor(ghdcBits2, clrBkSave);

                } else {

                    for (wState=0; wState < NUM_BUTTON_STATES; wState++) {

                        BitBltSysBmp(ghdcBits2, x, y, bm);
                        x += cxDesired;
                        bm++;
                    }

                    /*
                     * Skip OBI_*_CDI.
                     */
                    bm++;
                }

                y += cyDesired;
            }

        } else {

            BitBltSysBmp(ghdcBits2, 0, fMenu ? SYSMET(CYEDGE) : 0, bm);
        }

        SelectBitmap(ghdcBits2, hOldBmp);

        /*
         * If the bitmap was created as a 1bpp, we need to convert to a
         * true mono-bitmap.  GDI performs different color-matching depending
         * upon this case.
         */
        if (f1Bpp && hbmp)
            hbmp = Convert1BppToMonoBitmap(ghdcBits2, hbmp);

        RtlLeaveCriticalSection(&gcsHdc);

    } else {

LoadForReal:

        hbmp = (HBITMAP)ObjectFromDIBResource(hmod,
                                              lpName,
                                              RT_BITMAP,
                                              cxDesired,
                                              cyDesired,
                                              flags);
    }

LoadBmpDone:

    return hbmp;
}

/***************************************************************************\
* LoadBitmapA (API)
* LoadBitmapW (API)
*
* Loads a bitmap from client.  If hmod == NULL, loads a bitmap from the
* system.
*
\***************************************************************************/

HBITMAP WINAPI LoadBitmapA(
    HINSTANCE hmod,
    LPCSTR    lpName)
{
    LPWSTR  lpUniName;
    HBITMAP hRet;

    if (ID(lpName))
        return LoadBmp(hmod, (LPCWSTR)lpName, 0, 0, 0);

    if (!MBToWCS(lpName, -1, &lpUniName, -1, TRUE))
        return NULL;

    hRet = LoadBmp(hmod, lpUniName, 0, 0, 0);

    UserLocalFree(lpUniName);

    return hRet;
}

HBITMAP WINAPI LoadBitmapW(
    HINSTANCE hmod,
    LPCWSTR   lpName)
{
    return LoadBmp(hmod, lpName, 0, 0, 0);
}

/***************************************************************************\
* LoadCursorA (API)
* LoadCursorW (API)
*
* Loads a cursor from client.  If hmod == NULL, loads a cursor from the
* system.
*
* 05-Apr-1991 ScottLu   Rewrote to work with client server.
\***************************************************************************/

HCURSOR WINAPI LoadCursorA(
    HINSTANCE hmod,
    LPCSTR    lpName)
{
    HCURSOR hRet;
    LPWSTR  lpUniName;

    if (ID(lpName))
        return LoadCursorW(hmod, (LPWSTR)lpName);

    if (!MBToWCS(lpName, -1, &lpUniName, -1, TRUE))
        return NULL;

    hRet = LoadCursorW(hmod, lpUniName);

    UserLocalFree(lpUniName);

    return hRet;
}

HCURSOR WINAPI LoadCursorW(
    HINSTANCE hmod,
    LPCWSTR   lpName)
{

    return LoadIcoCur(hmod,
                      lpName,
                      RT_CURSOR,
                      0,
                      0,
                      LR_DEFAULTSIZE | LR_SHARED);

}

/***************************************************************************\
* LoadIconA (API)
* LoadIconW (API)
*
* Loads an icon from client.  If hmod == NULL, loads an icon from the
* system.
*
* 05-Apr-1991 ScottLu   Rewrote to work with client server.
\***************************************************************************/

HICON WINAPI LoadIconA(
    HINSTANCE hmod,
    LPCSTR    lpName)
{
    HICON  hRet;
    LPWSTR lpUniName;

    if (ID(lpName))
        return LoadIconW(hmod, (LPWSTR)lpName);

    if (!MBToWCS(lpName, -1, &lpUniName, -1, TRUE))
        return NULL;

    hRet = LoadIconW(hmod, lpUniName);

    UserLocalFree(lpUniName);

    return hRet;
}

HICON WINAPI LoadIconW(
    HINSTANCE hmod,
    LPCWSTR   lpName)
{
    return LoadIcoCur(hmod,
                      lpName,
                      RT_ICON,
                      0,
                      0,
                      LR_DEFAULTSIZE | LR_SHARED);
}

/***************************************************************************\
* LoadImageA (API)
* LoadImageW (API)
*
* Loads a bitmap, icon or cursor resource from client.  If hmod == NULL,
* then it will load from system-resources.
*
\***************************************************************************/

HANDLE WINAPI LoadImageA(
    HINSTANCE hmod,
    LPCSTR    lpName,
    UINT      type,
    int       cxDesired,
    int       cyDesired,
    UINT      flags)
{
    LPWSTR lpUniName;
    HANDLE hRet;

    if (ID(lpName))
        return LoadImageW(hmod,
                          (LPCWSTR)lpName,
                          type,
                          cxDesired,
                          cyDesired,
                          flags);

    if (!MBToWCS(lpName, -1, &lpUniName, -1, TRUE))
        return NULL;

    hRet = LoadImageW(hmod, lpUniName, type, cxDesired, cyDesired, flags);

    UserLocalFree(lpUniName);

    return hRet;
}

HANDLE WINAPI LoadImageW(
    HINSTANCE hmod,
    LPCWSTR   lpName,
    UINT      IMAGE_code,
    int       cxDesired,
    int       cyDesired,
    UINT      flags)
{
    /*
     * If we specified LR_LOADFROMFILE, then we can tweak the
     * flags to turn off LR_SHARED.
     */
    if (flags & LR_LOADFROMFILE)
        flags &= ~LR_SHARED;

    switch (IMAGE_code) {
    case IMAGE_BITMAP:
        return (HANDLE)LoadBmp(hmod, lpName, cxDesired, cyDesired, flags);

    case IMAGE_CURSOR:
#if 0 //CHRISWIL : oemInfo.fColorCursors doesn't exist on NT.
        if (!oemInfo.fColorCursors)
            flags |= LR_MONOCHROME;
#endif

    case IMAGE_ICON:

        /*
         * On WinNT 3.51, an app can successfully load a
         * USER icon without specifying LR_SHARED. We enable
         * these apps to succeed, but make 4.0 apps conform to
         * Windows95 behavior.
         */

        if (!hmod && GETEXPWINVER(NULL) < VER40) {
            flags |= LR_SHARED;
        }

        return (HANDLE)LoadIcoCur(hmod,
                                  lpName,
                                  ((IMAGE_code == IMAGE_ICON) ? RT_ICON : RT_CURSOR),
                                  cxDesired,
                                  cyDesired,
                                  flags);

    default:
        RIPMSG0(RIP_WARNING, "LoadImage: invalid IMAGE_code");
        return NULL;
    }
}

/***************************************************************************\
* GetIconIdEx
*
* This one accepts width, height, and other flags.  Just not exported right
* now.
*
\***************************************************************************/

UINT GetIconIdEx(
    HINSTANCE hmod,
    HANDLE    hrsd,
    LPCWSTR   lpszType,
    DWORD     cxDesired,
    DWORD     cyDesired,
    UINT      LR_flags)
{
    int         idIcon = 0;
    LPNEWHEADER lpnh;

    if (lpnh = (LPNEWHEADER)LOCKRESOURCE(hrsd, hmod)) {

        /*
         * Do a sanity check on this data structure.  Otherwise we'll GP FAULT
         * when extracting an icon from a corrupted area.  Fix for B#9290.
         * SANKAR, 08/13/91
         */
        if ((lpnh->Reserved == 0) &&
            ((lpnh->ResType == IMAGE_ICON) || (lpnh->ResType == IMAGE_CURSOR))) {

            idIcon = LookupIconIdFromDirectoryEx((PBYTE)lpnh,
                                                 (lpszType == RT_ICON),
                                                 cxDesired,
                                                 cyDesired,
                                                 LR_flags);
        }

        UNLOCKRESOURCE(hrsd, hmod);
    }

    return idIcon;
}

/***************************************************************************\
* LoadDib (Worker)
*
* This is the worker-routine for loading a resource and returning a handle
* to the object as a dib.
*
\***************************************************************************/

HANDLE LoadDIB(
    HINSTANCE hmod,
    LPCWSTR   lpName,
    LPWSTR    type,
    DWORD     cxDesired,
    DWORD     cyDesired,
    UINT      LR_flags)
{
    HANDLE  hDir;
    UINT    idIcon;
    LPWSTR  lpszGroupType;
    HANDLE  hRes = NULL;

    switch (PTR_TO_ID(type)) {

    case PTR_TO_ID(RT_ICON):
    case PTR_TO_ID(RT_CURSOR):

        lpszGroupType = RT_GROUP_CURSOR + (type - RT_CURSOR);

        /*
         * For WOW support, OIC_ICON and OIC_SIZE need to be supported.
         * Since these resources match other existing resources, we map
         * them here so we produce results that emulates
         * behavor as if we had the actual resources in USER.
         *
         * Note that obsolete mapping of lpName in LoadIcoCur prevents
         * win4.0 apps from getting here.
         */
        if (hmod == hmodUser) {

            switch ((ULONG_PTR)lpName) {
            case OCR_SIZE:
                lpName = (LPCWSTR)OCR_SIZEALL_DEFAULT;
                break;

            case OCR_ICON:
                lpName = (LPCWSTR)OCR_ICON_DEFAULT;
                break;
            }
        }
        /*
         * The resource is actually a directory which contains multiple
         * individual image resources we must choose from.
         * Locate the directory
         */
        if (hDir = SplFindResource(hmod, lpName, (LPCWSTR)lpszGroupType)) {

            /*
             * Load the directory.
             */
            if (hDir = LOADRESOURCE(hmod, hDir)) {

                /*
                 * Get the name of the best individual image.
                 */
                if (idIcon = GetIconIdEx(hmod,
                                         hDir,
                                         type,
                                         cxDesired,
                                         cyDesired,
                                         LR_flags)) {

                    /*
                     * NOTE: Don't free the directory resource!!! - ChipA.
                     * We can't call SplFindResource here, because idIcon
                     * is internal to us and GetDriverResourceId()
                     * doesn't know how to map it.
                     */
                    hRes = FINDRESOURCEW(hmod, MAKEINTRESOURCE(idIcon), type);
                }

                /*
                 * BOGUS:
                 * It would be very cool if we could loop through all the
                 * items in the directory and free 'em too.  Free the ones
                 * except for the one we're about to load, that is.
                 *
                 * Free directory resources TWICE so they get really freed.
                 */
                SplFreeResource(hDir, hmod, LR_flags);
            }
        } else {
            /*
             * Failed to load a regular icon\cursor.
             * Try to load an animated icon/cursor with the same name
             */
            hRes = SplFindResource(hmod, lpName,
                    PTR_TO_ID(type) == PTR_TO_ID(RT_CURSOR) ? RT_ANICURSOR : RT_ANIICON);
        }
        break;

    case PTR_TO_ID(RT_BITMAP):
        hRes = SplFindResource(hmod, lpName, RT_BITMAP);
        break;

    default:
        RIPMSG0(RIP_WARNING, "LoadDIB: Invalid resource type");
        break;
    }

    if (hRes)
        hRes = LOADRESOURCE(hmod, hRes);

    return hRes;
}

/***************************************************************************\
* LoadIcoCur (Worker)
*
*
\***************************************************************************/

HICON LoadIcoCur(
    HINSTANCE hmod,
    LPCWSTR   pszResName,
    LPWSTR    type,
    DWORD     cxDesired,
    DWORD     cyDesired,
    UINT      LR_flags)
{
    HICON     hico;
    LPWSTR    pszModName;
    WCHAR     achModName[MAX_PATH];

    ConnectIfNecessary();

    /*
     * Setup module name and handles for lookup.
     */
    if (hmod == NULL)  {

        hmod = hmodUser;
        pszModName = szUSER32;

    } else {

        WowGetModuleFileName(hmod,
                             achModName,
                             sizeof(achModName) / sizeof(WCHAR));

        pszModName = achModName;
    }

    if (LR_flags & LR_CREATEDIBSECTION)
        LR_flags = (LR_flags & ~LR_CREATEDIBSECTION) | LR_CREATEREALDIB;

    /*
     * Setup defaults.
     */
    if ((hmod == hmodUser) && !IS_PTR(pszResName)) {

        int      imapMax;
        LPMAPRES lpMapRes;

        /*
         * Map some old OEM IDs for people.
         */
        if (type == RT_ICON) {

            static MAPRES MapOemOic[] = {
                {OCR_ICOCUR, OIC_WINLOGO, MR_FAILFOR40}
            };

            lpMapRes = MapOemOic;
            imapMax  = 1;

        } else {

            static MAPRES MapOemOcr[] = {
                {OCR_ICON, OCR_ICON, MR_FAILFOR40},
                {OCR_SIZE, OCR_SIZE, MR_FAILFOR40}
            };

            lpMapRes = MapOemOcr;
            imapMax  = 2;
        }

        while (--imapMax >= 0) {

            if (lpMapRes->idDisp == PTR_TO_ID(pszResName)) {

                if ((lpMapRes->bFlags & MR_FAILFOR40) &&
                    GETAPPVER() >= VER40) {

                    RIPMSG1(RIP_WARNING,
                          "LoadIcoCur: Old ID 0x%x not allowed for 4.0 apps",
                          PTR_TO_ID(pszResName));

                    return NULL;
                }

                pszResName = MAKEINTRESOURCE(lpMapRes->idUser);
                break;
            }

            ++lpMapRes;
        }
    }

    /*
     * Determine size of requested object.
     */
    cxDesired = GetIcoCurWidth(cxDesired , (type == RT_ICON), LR_flags, 0);
    cyDesired = GetIcoCurHeight(cyDesired, (type == RT_ICON), LR_flags, 0);

    /*
     * See if this is a cached icon/cursor, and grab it if we have one
     * already.
     */
    if (LR_flags & LR_SHARED) {

        CURSORFIND cfSearch;

        /*
         * Note that win95 fails to load any USER resources unless
         * LR_SHARED is specified - so we do too.  Also, win95 will
         * ignore your cx, cy and LR_flag parameters and just give
         * you whats in the cache so we do too.
         * A shame but thats life...
         *
         * Setup search criteria.  Since this is a load, we will have
         * no source-cursor to lookup.  Find something respectable.
         */
        cfSearch.hcur = (HCURSOR)NULL;
        cfSearch.rt   = PtrToUlong(type);

        if (hmod == hmodUser) {

            cfSearch.cx  = 0;
            cfSearch.cy  = 0;
            cfSearch.bpp = 0;

        } else {

            cfSearch.cx  = cxDesired;
            cfSearch.cy  = cyDesired;

/*
 * On NT we have a more strict cache-lookup.  By passing in (zero), we
 * will tell the cache-lookup to ignore the bpp.  This fixes a problem
 * in Crayola Art Studio where the coloring-book cursor was being created
 * as an invisible cursor.  This lookup is compatible with Win95.
 */
#if 0
            cfSearch.bpp = GetIcoCurBpp(LR_flags);
#else
            cfSearch.bpp = 0;
#endif
        }

        hico = FindExistingCursorIcon(pszModName, pszResName, &cfSearch);

        if (hico != NULL)
            goto IcoCurFound;
    }

#ifdef LATER // SanfordS
    /*
     * We need to handle the case where a configurable icon has been
     * loaded from some arbitrary module or file and someone now wants
     * to load the same thing in a different size or color content.
     *
     * A cheezier alternative is to just call CopyImage on what we
     * found.
     */
    if (hmod == hmodUser) {
        hico = FindExistingCursorIcon(NULL,
                                      szUSER,
                                      type,
                                      pszResName,
                                      0,
                                      0,
                                      0);
        if (hico != NULL) {
            /*
             * Find out where the original came from and load it.
             * This may require some redesign to remember the
             * filename that LR_LOADFROMFILE images came from.
             */
            _GetIconInfo(....);
            return LoadIcoCur(....);
        }
    }
#endif

    hico = (HICON)ObjectFromDIBResource(hmod,
                                        pszResName,
                                        type,
                                        cxDesired,
                                        cyDesired,
                                        LR_flags);

IcoCurFound:

    return hico;
}

/***************************************************************************\
* ObjectFromDIBResource
*
*
\***************************************************************************/
HANDLE ObjectFromDIBResource(
    HINSTANCE hmod,
    LPCWSTR   lpName,
    LPWSTR    type,
    DWORD     cxDesired,
    DWORD     cyDesired,
    UINT      LR_flags)
{
    HANDLE  hObj = NULL;

    if (LR_flags & LR_LOADFROMFILE) {

        hObj = RtlLoadObjectFromDIBFile(lpName,
                                        type,
                                        cxDesired,
                                        cyDesired,
                                        LR_flags);
    } else {

        HANDLE hdib;

        hdib = LoadDIB(hmod, lpName, type, cxDesired, cyDesired, LR_flags);

        if (hdib != NULL) {

            LPBITMAPINFOHEADER lpbih;

            /*
             * We cast the resource-bits to a BITMAPINFOHEADER.  If the
             * resource is a CURSOR type, then there are actually two
             * WORDs preceeding the BITMAPINFOHDEADER indicating the
             * hot-spot.  Be careful in assuming you have a real
             * dib in this case.
             */
            if(lpbih = (LPBITMAPINFOHEADER)LOCKRESOURCE(hdib, hmod)) {

                switch (PTR_TO_ID(type)) {
                case PTR_TO_ID(RT_BITMAP):
                    /*
                     * Create a physical bitmap from the DIB.
                     */
                    hObj = ConvertDIBBitmap(lpbih,
                                            cxDesired,
                                            cyDesired,
                                            LR_flags,
                                            NULL,
                                            NULL);
                    break;

                case PTR_TO_ID(RT_ICON):
                case PTR_TO_ID(RT_CURSOR):
                case PTR_TO_ID(RT_ANICURSOR):
                case PTR_TO_ID(RT_ANIICON):
                    /*
                     * Animated icon\cursors resources use the RIFF format
                     */
                    if (ISRIFFFORMAT(lpbih)) {
                        hObj = LoadCursorIconFromResource ((PBYTE)lpbih, lpName, cxDesired, cyDesired, LR_flags);
                    } else {
                        /*
                         * Create the object from the DIB.
                         */
                        hObj = ConvertDIBIcon(lpbih,
                                              hmod,
                                              lpName,
                                              (type == RT_ICON),
                                              cxDesired,
                                              cyDesired,
                                              LR_flags);
                    }
                    break;
                }

                UNLOCKRESOURCE(hdib, hmod);
            }

            /*
             * DO THIS TWICE!  The resource compiler always makes icon images
             * (RT_ICON) in a group icon discardable, whether the group dude
             * is or not!  So the first free won't really free the thing;
             * it'll just set the ref count to 0 and let the discard logic
             * go on its merry way.
             *
             * We take care of shared guys, so we don't need this dib no more.
             * Don't need this DIB no more no more, no more no more no more
             * don't need this DIB no more.
             */
            SplFreeResource(hdib, hmod, LR_flags);
        }
    }

    return hObj;
}

/***************************************************************************\
* BitmapFromDIB
*
* Creates a bitmap-handle from a DIB-Spec.  This function supports the
* LR_CREATEDIBSECTION flag, sets proper color depth, and stretches the
* DIBs as requested.
*
\***************************************************************************/

HBITMAP BitmapFromDIB(
    int          cxNew,
    int          cyNew,
    WORD         bPlanesNew,
    WORD         bBitsPixelNew,
    UINT         LR_flags,
    int          cxOld,
    int          cyOld,
    LPSTR        lpBits,
    LPBITMAPINFO lpbi,
    HPALETTE     hpal)
{
    HBITMAP hbmpNew = NULL;
    BOOL    fStretch;
    BOOL    f1Bpp = FALSE;

    RtlEnterCriticalSection(&gcsHdc);

    if (cxNew == 0)
        cxNew = cxOld;

    if (cyNew == 0)
        cyNew = cyOld;

    fStretch = ((cxNew != cxOld) || (cyNew != cyOld));

    /*
     * If LR_flags indicate DIB-Section, then return that as the
     * bitmap handle.
     */
    if (LR_flags & (LR_CREATEDIBSECTION | LR_CREATEREALDIB)) {

        int   cxTemp;
        int   cyTemp;
        BOOL  fOldFormat;
        LPVOID dwDummy;
        DWORD dwTemp;

#define lpbch ((LPBITMAPCOREHEADER)lpbi)

        fOldFormat = ((WORD)lpbi->bmiHeader.biSize == sizeof(BITMAPCOREHEADER));

        if (fOldFormat) {

            cxTemp = lpbch->bcWidth;
            cyTemp = lpbch->bcHeight;

            lpbch->bcWidth  = (WORD)cxNew;
            lpbch->bcHeight = (WORD)cyNew;

        } else {

            cxTemp = lpbi->bmiHeader.biWidth;
            cyTemp = lpbi->bmiHeader.biHeight;
            dwTemp = lpbi->bmiHeader.biCompression;

            lpbi->bmiHeader.biWidth  = cxNew;
            lpbi->bmiHeader.biHeight = cyNew;

            if (dwTemp != BI_BITFIELDS)
                lpbi->bmiHeader.biCompression = BI_RGB;
        }

        if (LR_flags & LR_CREATEREALDIB) {
            hbmpNew = CreateDIBitmap(ghdcBits2,
                                     (LPBITMAPINFOHEADER)lpbi,
                                     CBM_CREATEDIB,
                                     NULL,
                                     lpbi,
                                     DIB_RGB_COLORS);
        } else {
            hbmpNew = CreateDIBSection(ghdcBits2,
                                       lpbi,
                                       DIB_RGB_COLORS,
                                       &dwDummy,
                                       0,
                                       0);
        }

        if (fOldFormat) {
            lpbch->bcWidth  = (WORD)cxTemp;
            lpbch->bcHeight = (WORD)cyTemp;
        } else {
            lpbi->bmiHeader.biWidth       = cxTemp;
            lpbi->bmiHeader.biHeight      = cyTemp;
            lpbi->bmiHeader.biCompression = dwTemp;
        }
#undef lpbch
    }

    if (hbmpNew == NULL) {

        hbmpNew = CreateScreenBitmap(cxNew,
                                     cyNew,
                                     bPlanesNew,
                                     bBitsPixelNew,
                                     NULL,
                                     &f1Bpp);
    }

    if (hbmpNew) {

        int     nStretchMode;
        DWORD   rgbBk;
        DWORD   rgbText;
        HBITMAP hbmpT;
        BOOL    fFail;

        /*
         * We need to select in appropriate bitmap immediately!  That way,
         * if we need to handle palette realization, the color matching
         * will work properly.
         */
        hbmpT = SelectBitmap(ghdcBits2, hbmpNew);

        /*
         * Setup for stretching
         */
        if (fStretch) {
            nStretchMode = SetBestStretchMode(ghdcBits2,
                                              bPlanesNew,
                                              bBitsPixelNew);
        }

        rgbBk   = SetBkColor(ghdcBits2, RESCLR_WHITE);
        rgbText = SetTextColor(ghdcBits2, RESCLR_BLACK);

        /*
         * Realize the palette.
         */
        if (hpal) {
#if DBG
            UserAssert(TEST_PUSIF(PUSIF_PALETTEDISPLAY));
#endif // DBG

            hpal = SelectPalette(ghdcBits2, hpal, FALSE);
            RealizePalette(ghdcBits2);
        }

        if (fStretch) {

            fFail = SmartStretchDIBits(ghdcBits2,
                               0,
                               0,
                               cxNew,
                               cyNew,
                               0,
                               0,
                               cxOld,
                               cyOld,
                               lpBits,
                               lpbi,
                               DIB_RGB_COLORS,
                               SRCCOPY) <= 0;
        } else {

            fFail = SetDIBits(ghdcBits2,
                      hbmpNew,
                      0,
                      cyNew,
                      lpBits,
                      lpbi,
                      DIB_RGB_COLORS) <= 0;
        }

        /*
         * Unrealize the palette
         */
        if (hpal) {
            SelectPalette(ghdcBits2, hpal, TRUE);
            RealizePalette(ghdcBits2);
        }

        /*
         * Cleanup after stretching
         */
        SetTextColor(ghdcBits2, rgbText);
        SetBkColor(ghdcBits2, rgbBk);
        if (fStretch)
            SetStretchBltMode(ghdcBits2, nStretchMode);

        SelectBitmap(ghdcBits2, hbmpT);

        /*
         * If the SetDIBits() of StretchDIBits() failed, it is probably because
         * GDI or the driver did not like the DIB format.  This may happen if
         * the file is truncated and we are using a memory mapped file to read
         * the DIB in.  In this case, an exception gets thrown in GDI, that it
         * traps and will return failure from the GDI call.
         */

        if (fFail) {
            DeleteObject(hbmpNew);
            hbmpNew = NULL;
        }
    }

    /*
     * If the bitmap was created as a 1bpp, we need to convert to a
     * true mono-bitmap.  GDI performs different color-matching depending
     * upon this case.
     */
    if (f1Bpp && hbmpNew)
        hbmpNew = Convert1BppToMonoBitmap(ghdcBits2, hbmpNew);

    RtlLeaveCriticalSection(&gcsHdc);
    return hbmpNew;
}

/***************************************************************************\
* HowManyColors
*
*
\***************************************************************************/

DWORD HowManyColors(
    IN  UPBITMAPINFOHEADER upbih,
    IN  BOOL               fOldFormat,
    OUT OPTIONAL LPBYTE    *ppColorTable)
{
#define upbch ((UPBITMAPCOREHEADER)upbih)

    if (fOldFormat) {
        if (ppColorTable != NULL) {
            *ppColorTable = (LPBYTE)(upbch + 1);
        }
        if (upbch->bcBitCount <= 8)
            return (1 << upbch->bcBitCount);
    } else {
        if (ppColorTable != NULL) {
            *ppColorTable = (LPBYTE)(upbih + 1);
        }
        if (upbih->biClrUsed)
            return (DWORD)upbih->biClrUsed;
        else if (upbih->biBitCount <= 8)
            return (1 << upbih->biBitCount);
        else if ((upbih->biBitCount == 16) || (upbih->biBitCount == 32))
            return 3;
    }
    return 0;

#undef upbch
}

/***************************************************************************\
* ChangeDibColors
*
* Given a DIB, processes LR_MONOCHROME, LR_LOADTRANSPARENT and
* LR_LOADMAP3DCOLORS flags on the given header and colortable.
*
*
\***************************************************************************/

VOID ChangeDibColors(
    IN LPBITMAPINFOHEADER lpbih,
    IN UINT               LR_flags)
{
    LPDWORD lpColorTable;
    DWORD  rgb;
    UINT   iColor;
    UINT   cColors;

    cColors = HowManyColors(lpbih, FALSE, &(LPBYTE)lpColorTable);

    /*
     * NT Bug 366661: Don't check the color count here b/c we will do different
     * things depending on what type of change we are performing.  For example,
     * when loading hi-color/true-color icons, we always need to do the 
     * monochrome conversion in order to properly get an icon-mask.
     */

    /*
     * LR_MONOCHROME is the only option that handles PM dibs.
     */
    if (LR_flags & LR_MONOCHROME) {
        /*
         * LR_MONOCHROME is the only option that handles PM dibs.
         *
         * DO THIS NO MATTER WHETHER WE HAVE A COLOR TABLE!  We need 
         * to do this for mono conversion and for > 8 BPP 
         * icons/cursors.  In CopyDibHdr, we already made a copy of 
         * the header big enough to hold 2 colors even on 16 and 24 
         * BPP images.
         */

        lpbih->biBitCount = lpbih->biPlanes = 1;
        lpColorTable[0] = RESCLR_BLACK;
        lpColorTable[1] = RESCLR_WHITE;
    } else if (LR_flags & LR_LOADTRANSPARENT) {

        LPBYTE pb;

        /*
         * No color table!  Do nothing.
         */
        if (cColors == 0) {
            RIPMSG0(RIP_WARNING, "ChangeDibColors: DIB doesn't have a color table");
            return;
        }

        pb = (LPBYTE)(lpColorTable + cColors);

        /*
         * Change the first pixel's color table entry to RGB_WINDOW
         * Gosh, I love small-endian
         */
        if (lpbih->biCompression == 0)
            iColor = (UINT)pb[0];
        else
            /*
             * RLE bitmap, will start with cnt,clr  or  0,cnt,clr
             */
            iColor = (UINT)(pb[0] == 0 ? pb[2] : pb[1]);

        switch (cColors) {
        case BPP01_MAXCOLORS:
            iColor &= 0x01;
            break;

        case BPP04_MAXCOLORS:
            iColor &= 0x0F;
            break;

        case BPP08_MAXCOLORS:
            iColor &= 0xFF;
            break;
        }

        rgb = (LR_flags & LR_LOADMAP3DCOLORS ? SYSRGB(3DFACE) : SYSRGB(WINDOW));

        lpColorTable[iColor] = RGBX(rgb);

    } else  if (LR_flags & LR_LOADMAP3DCOLORS) {

        /*
         * Fix up the color table, mapping shades of grey to the current
         * 3D colors.
         */
        for (iColor = 0; iColor < cColors; iColor++) {

            switch (*lpColorTable & 0x00FFFFFF) {

            case RGBX(RGB(223, 223, 223)):
                rgb = SYSRGB(3DLIGHT);
                goto ChangeColor;

            case RGBX(RGB(192, 192, 192)):
                rgb = SYSRGB(3DFACE);
                goto ChangeColor;

            case RGBX(RGB(128, 128, 128)):
                rgb = SYSRGB(3DSHADOW);

                /*
                 * NOTE: byte-order is different in DIBs than in RGBs
                 */
ChangeColor:
                *lpColorTable = RGBX(rgb);
                break;
            }
            lpColorTable++;
        }
    }
}

/***************************************************************************\
* ConvertDIBIcon
*
* Called when a cursor/icon in DIB format is loaded.  This converts the
* cursor/icon into the old format and returns the resource handle.  IE,
* grabs the DIB bits and transforms them into physical bitmap bits.
*
*
* DIB Formats for icons/cursors 101
*
* Old Win 3.0 format icons/cursors start with an OLDICON/OLDCURSOR header
* followed by a double high monochrome DIB.  The height refered to in the
* header is the icon/cursor height, not the DIB height which is twice as
* high.  The XOR mask is in the first-half of the DIB bits.
*
* Old PM format icons/cursors start with a BITMAPCOREHEADER and
* are identical to the current win 3.1/NT format thereafter.
*
* Current NT/Chicago/Win 3.1 format icons/cursors start with
* a BITAMPINFOHEADER.  The height of this header refers to the height
* of the first bitmap which may either be color or truely monochrome.
* If its color, it is followed by the monochrome AND mask bits imediately
* after the color bits.  If it is truely monochrome, the AND and XOR
* masks are totally contained in the first DIB bits and no more bits
* follow.
*
* 5-Oct-1994 SanfordS   Recreated
\***************************************************************************/

HICON ConvertDIBIcon(
    LPBITMAPINFOHEADER lpbih,
    HINSTANCE          hmod,
    LPCWSTR            lpName,
    BOOL               fIcon,
    DWORD              cxNew,
    DWORD              cyNew,
    UINT               LR_flags)
{
    LPBITMAPINFOHEADER lpbihNew = NULL;
    LPSTR              lpBitsNextMask = NULL;
    HICON              hicoNew = NULL;
    BOOL               fOldFormat = FALSE;
    CURSORDATA         cur;
    WCHAR              achModName[MAX_PATH];

    /*
     * Because Icons/Cursors always get public bitmaps, we cannot use
     * LR_CREATEDIBSECTION on them.
     */
    if (LR_flags & LR_CREATEDIBSECTION)
        LR_flags = (LR_flags & ~LR_CREATEDIBSECTION) | LR_CREATEREALDIB;

    RtlZeroMemory(&cur, sizeof(cur));

    if (!fIcon) {
        /*
         * Cursors have an extra two words preceeding the BITMAPINFOHEADER
         * indicating the hot-spot.  After doing the increments, the
         * pointer should be at the dib-header.
         */
        cur.xHotspot = (short)(int)*(((LPWORD)lpbih)++);
        cur.yHotspot = (short)(int)*(((LPWORD)lpbih)++);
    }

    /*
     * Get the XOR/Color mask.
     * The XOR bits are first in the DIB because the header info
     * pertains to them.
     * The AND mask is always monochrome.
     */
    lpBitsNextMask = NULL;  // not passing lpBits in.
    cur.hbmColor = ConvertDIBBitmap(lpbih,
                                    cxNew,
                                    cyNew,
                                    LR_flags,
                                    &lpbihNew,
                                    &lpBitsNextMask);
    if (cur.hbmColor == NULL)
        return NULL;

    if (hmod == NULL) {
        cur.lpModName = NULL;
    } else {
        cur.CURSORF_flags = CURSORF_FROMRESOURCE;
        if (hmod == hmodUser) {
            cur.lpModName     = szUSER32;
        } else  {
            WowGetModuleFileName(hmod,
                              achModName,
                              sizeof(achModName) / sizeof(WCHAR));
            cur.lpModName = achModName;
        }
    }
    cur.rt     = (fIcon ? PTR_TO_ID(RT_ICON) : PTR_TO_ID(RT_CURSOR));
    cur.lpName = (LPWSTR)lpName;
    cur.bpp    = lpbihNew->biBitCount * lpbihNew->biPlanes;

    if (cxNew == 0)
        cxNew = lpbihNew->biWidth;

    if (cyNew == 0)
        cyNew = lpbihNew->biHeight / 2;

    if (!fIcon) {

        cur.xHotspot = MultDiv(cur.xHotspot,
                               cxNew,
                               lpbihNew->biWidth);
        cur.yHotspot = MultDiv(cur.yHotspot,
                               cyNew,
                               lpbihNew->biHeight / 2);
    } else {

        /*
         * For an icon the hot spot is the center of the icon
         */
        cur.xHotspot = (INT)(cxNew) / 2;
        cur.yHotspot = (INT)(cyNew) / 2;
    }

    /*
     * Setup header for monochrome DIB.  Note that we use the COPY.
     */
    ChangeDibColors(lpbihNew, LR_MONOCHROME);

    if (lpBitsNextMask != NULL) {
        cur.hbmMask = BitmapFromDIB(cxNew,
                                    cyNew * 2,
                                    1,
                                    1,
                                    0,
                                    lpbihNew->biWidth,
                                    lpbihNew->biHeight,
                                    lpBitsNextMask,
                                    (LPBITMAPINFO)lpbihNew,
                                    NULL);
        if (cur.hbmMask == NULL) {
            DeleteObject(cur.hbmColor);
            UserLocalFree(lpbihNew);
            return NULL;
        }

    } else {
        cur.hbmMask = cur.hbmColor;
        cur.hbmColor = NULL;
    }

    cur.cx = cxNew;
    cur.cy = cyNew * 2;

    /*
     * Free our dib header copy allocated by ConvertDIBBitmap
     */
    UserLocalFree(lpbihNew);

    if (LR_flags & LR_SHARED)
        cur.CURSORF_flags |= CURSORF_LRSHARED;

    if (LR_flags & LR_GLOBAL)
        cur.CURSORF_flags |= CURSORF_GLOBAL;

    if (LR_flags & LR_ACONFRAME)
        cur.CURSORF_flags |= CURSORF_ACONFRAME;

    return CreateIcoCur(&cur);
}

/***************************************************************************\
* TrulyMonochrome
*
* Checks to see if a DIB colro table is truly monochrome.  ie: the color
* table has black & white entries only.
*
\***************************************************************************/

BOOL TrulyMonochrome(
    LPVOID lpColorTable,
    BOOL   fOldFormat)
{
    #define lpRGB  ((UNALIGNED LONG *)lpColorTable)
    #define lpRGBw ((UNALIGNED WORD *)lpColorTable)

    if (fOldFormat) {

        /*
         * Honey - its triplets.
         */
        if (lpRGBw[0] == 0x0000)
            return (lpRGBw[1] == 0xFF00) && (lpRGBw[2] == 0xFFFF);
        else if (lpRGBw[0] == 0xFFFF)
            return (lpRGBw[1] == 0x00FF) && (lpRGBw[2] == 0x0000);

    } else {

        /*
         * Honey - its quadruplets!
         */
        if (lpRGB[0] == RESCLR_BLACK)
            return (lpRGB[1] == RESCLR_WHITE);
        else if (lpRGB[0] == RESCLR_WHITE)
            return (lpRGB[1] == RESCLR_BLACK);
    }

    #undef lpRGB
    #undef lpRGBw

    return FALSE;
}

/***************************************************************************\
* CopyDibHdr
*
* Copies and converts a DIB resource header
*
* Handles conversion of OLDICON, OLDCURSOR and BITMAPCOREHEADER
* structures to BITMAPINFOHEADER headers.
*
* Note: fSingleHeightMasks is set for OLDICON and OLDCURSOR formats.
*       This identifies that a monochrome AND/Color mask
*       is NOT double height as it is in the newer formats.
*
* NOTE:  On the off chance that LR_LOADTRANSPARENT is used, we want to
*     copy a DWORD of the bits.  Since DIB bits are DWORD aligned, we know
*     at least a DWORD is there, even if the thing is a 1x1 mono bmp.
*
* The returned buffer is allocated in this function and needs to be
* freed by the caller.
*
* 22-Oct-1995 SanfordS  Revised
\***************************************************************************/

LPBITMAPINFOHEADER CopyDibHdr(
    IN  UPBITMAPINFOHEADER upbih,
    OUT LPSTR             *lplpBits,
    OUT LPBOOL             lpfMono)
{

#define upbch ((UPBITMAPCOREHEADER)upbih)

    DWORD              cColors;
    DWORD              i;
    LPBITMAPINFOHEADER lpbihNew;
    DWORD              cbAlloc;
    LPBYTE             lpColorTable;
    struct  {
        BITMAPINFOHEADER   bih;
        DWORD              rgb[256];
        DWORD              dwBuffer;
    } Fake;

    switch (upbih->biSize) {
    case sizeof(BITMAPINFOHEADER):
        /*
         * Cool.  No conversion needed.
         */
        cColors   = HowManyColors(upbih, FALSE, &lpColorTable);
        *lplpBits = (LPSTR)(((LPDWORD)lpColorTable) + cColors);
        break;

    case sizeof(BITMAPCOREHEADER):
        /*
         * Convert the BITMAPCOREHEADER to a BITMAPINFOHEADER
         */
        Fake.bih.biSize          = sizeof(BITMAPINFOHEADER);
        Fake.bih.biWidth         = upbch->bcWidth;
        Fake.bih.biHeight        = upbch->bcHeight;
        Fake.bih.biPlanes        = upbch->bcPlanes;
        Fake.bih.biBitCount      = upbch->bcBitCount;
        Fake.bih.biCompression   =
        Fake.bih.biXPelsPerMeter =
        Fake.bih.biYPelsPerMeter =
        Fake.bih.biClrImportant  = 0;
        Fake.bih.biClrUsed       = cColors =
                                   HowManyColors(upbih, TRUE, &lpColorTable);
        Fake.bih.biSizeImage     =
                BitmapWidth(Fake.bih.biWidth, Fake.bih.biBitCount) *
                            Fake.bih.biHeight;
        /*
         * Copy and convert tripplet color table to rgbQuad color table.
         */
        for (i = 0; i < cColors; i++, lpColorTable += 3) {

            Fake.rgb[i] = lpColorTable[0]        +
                          (lpColorTable[1] << 8) +
                          (lpColorTable[2] << 16);
        }

        Fake.rgb[i] = *(DWORD UNALIGNED *)lpColorTable;  // For LR_LOADTRANSPARENT
        upbih       = (UPBITMAPINFOHEADER)&Fake;
        *lplpBits   = lpColorTable;
        break;

    default:

#define upOldIcoCur ((UPOLDCURSOR)upbih)

        if (upOldIcoCur->bType == BMR_ICON ||
                upOldIcoCur->bType == BMR_CURSOR) {
            /*
             * Convert OLDICON/OLDCURSOR header to BITMAPINFHEADER
             */
            RIPMSG0(RIP_WARNING, "USER32:Converting a OLD header. - email sanfords if you see this");
            Fake.bih.biSize          = sizeof(BITMAPINFOHEADER);
            Fake.bih.biWidth         = upOldIcoCur->cx;
            Fake.bih.biHeight        = upOldIcoCur->cy * 2;
            Fake.bih.biPlanes        =
            Fake.bih.biBitCount      = 1;
            Fake.bih.biCompression   =
            Fake.bih.biXPelsPerMeter =
            Fake.bih.biYPelsPerMeter =
            Fake.bih.biClrImportant  = 0;
            Fake.bih.biClrUsed       = cColors = BPP01_MAXCOLORS;
            Fake.bih.biSizeImage     =
                    BitmapWidth(upOldIcoCur->cx, 1) * upOldIcoCur->cy;
            Fake.rgb[0]              = RESCLR_BLACK;
            Fake.rgb[1]              = RESCLR_WHITE;
            upbih                    = (LPBITMAPINFOHEADER)&Fake;
            *lplpBits                = upOldIcoCur->abBitmap;
            Fake.rgb[2]              = *((LPDWORD)*lplpBits);  // For LR_LOADTRANSPARENT

        } else {

            RIPMSG0(RIP_WARNING, "ConvertDIBBitmap: not a valid format");
            return NULL;
        }

#undef pOldIcoCur

        break;
    }

    *lpfMono = (cColors == BPP01_MAXCOLORS) &&
            TrulyMonochrome((LPBYTE)upbih + sizeof(BITMAPINFOHEADER), FALSE);

    cbAlloc = sizeof(BITMAPINFOHEADER) + (cColors * sizeof(RGBQUAD)) + 4;

    if (lpbihNew = UserLocalAlloc(0, cbAlloc))
        RtlCopyMemory(lpbihNew, upbih, cbAlloc);

    return lpbihNew;

#undef upbch

}

/***************************************************************************\
* ConvertDIBBitmap
*
* This takes a BITMAPCOREHEADER, OLDICON, OLDCURSOR or BITMAPINFOHEADER DIB
* specification and creates a physical object from it.
* Handles Color fixups, DIB sections, color depth, and stretching options.
*
* Passes back: (if lplpbih is not NULL)
*   lplpbih = copy of given header converted to BITMAPINFOHEADER form.
*   lplpBits = pointer to next mask bits, or NULL if no second mask.
*   Caller must free lplpbih returned.
*
* If lplpBits is not NULL and points to a non-NULL value, it supplies
* the location of the DIB bits allowing the header to be from a different
* location.
*
* 04-Oct-1995 SanfordS  Recreated.
\***************************************************************************/

HBITMAP ConvertDIBBitmap(
    IN  UPBITMAPINFOHEADER           upbih,
    IN  DWORD                        cxDesired,
    IN  DWORD                        cyDesired,
    IN  UINT                         LR_flags,
    OUT OPTIONAL LPBITMAPINFOHEADER *lplpbih,
    IN OUT OPTIONAL LPSTR           *lplpBits)
{
    LPBITMAPINFOHEADER lpbihNew;
    BOOL               fMono, fMonoGiven;
    BYTE               bPlanesDesired;
    BYTE               bppDesired;
    LPSTR              lpBits;
    HBITMAP            hBmpRet;

    /*
     * Make a copy of the DIB-Header.  This returns a pointer
     * which was allocated, so it must be freed later.
     * The also converts the header to BITMAPINFOHEADER format.
     */
    if ((lpbihNew = CopyDibHdr(upbih, &lpBits, &fMono)) == NULL)
        return NULL;

    /*
     * When loading a DIB file, we may need to use a different
     * bits pointer.  See RtlRes.c/RtlLoadObjectFromDIBFile.
     */
    if (lplpBits && *lplpBits)
        lpBits = *lplpBits;

    fMonoGiven = fMono;

    if (!fMono) {

        if (LR_flags & (LR_LOADTRANSPARENT | LR_LOADMAP3DCOLORS))
            ChangeDibColors(lpbihNew, LR_flags & ~LR_MONOCHROME);

        bPlanesDesired = gpsi->Planes;
        bppDesired     = gpsi->BitsPixel;
        fMono          = LR_flags & LR_MONOCHROME;
    }

    if (fMono) {
        bPlanesDesired =
        bppDesired     = 1;
    }

    /*
     * HACK area
     */
    if (lplpbih != NULL) {

        /*
         * pass back the translated/copied header
         */
        *lplpbih = lpbihNew;

        /*
         * When loading icon/cursors on a system with multiple monitors
         * with different color depths, always convert to VGA color.
         */
        if (!fMono && !SYSMET(SAMEDISPLAYFORMAT)) {
            bPlanesDesired = 1;
            bppDesired = 4;
        }

        /*
         * Return a ponter to the bits following this set of bits
         * if there are any there.
         *
         * Note that the header given with an ICON DIB always reflects
         * twice the height of the icon desired but the COLOR bitmap
         * (if there is one) will only be half that high.  We need to
         * fixup cyDesired for monochrome icons so that the mask isnt
         * stretched to half the height its supposed to be.  Color
         * bitmaps, however, must have the header corrected to reflect
         * the bits actual height which is half what the header said.
         * The correction must later be backed out so that the returned
         * header reflects the dimensions of the XOR mask that immediately
         * follows the color mask.
         */
        if (fMonoGiven) {

            *lplpBits = NULL;

            if (cyDesired)
                cyDesired <<= 1;    // mono icon bitmaps are double high.

        } else {

            UserAssert(!(lpbihNew->biHeight & 1));
            lpbihNew->biHeight >>= 1;  // color icon headers are off by 2

            /*
             * Gross calculation!  We subtract the XOR part of the mask
             * for this calculation so that we submit a double-high mask.
             * The first half of this is garbage, but for icons its not
             * used.  This may be a bug for cursor use of icons.
             */
            *lplpBits = lpBits +
                    (BitmapWidth(lpbihNew->biWidth, lpbihNew->biBitCount) -
                    BitmapWidth(lpbihNew->biWidth, 1)) *
                    lpbihNew->biHeight;
        }
    }

    if (cxDesired == 0)
        cxDesired = lpbihNew->biWidth;

    if (cyDesired == 0)
        cyDesired = lpbihNew->biHeight;

    hBmpRet = BitmapFromDIB(cxDesired,
                            cyDesired,
                            bPlanesDesired,
                            bppDesired,
                            LR_flags,
                            lpbihNew->biWidth,
                            lpbihNew->biHeight,
                            lpBits,
                            (LPBITMAPINFO)lpbihNew,
                            NULL);

    if (lplpbih == NULL || hBmpRet == NULL) {
        UserLocalFree(lpbihNew);
    } else if (!fMonoGiven) {
        lpbihNew->biHeight <<= 1;   // restore header for next mask
    }

    return hBmpRet;
}

/***************************************************************************\
* MyAbs
*
* Calcules my weighted absolute value of the difference between 2 nums.
* This of course normalizes values to >= zero.  But it also doubles them
* if valueHave < valueWant.  This is because you get worse results trying
* to extrapolate from less info up then interpolating from more info down.
* I paid $150 to take the SAT.  I'm damned well going to get my vocab's
* money worth.
*
\***************************************************************************/

UINT MyAbs(
    int valueHave,
    int valueWant)
{
    int diff = (valueHave - valueWant);

    if (diff < 0)
        diff = 2 * (-diff);

    return (UINT)diff;
}

/***************************************************************************\
* Magnitude
*
* Used by the color-delta calculations.  The reason is that num colors is
* always a power of 2.  So we use the log 2 of the want vs. have values
* to avoid having weirdly huge sets.
*
\***************************************************************************/

UINT Magnitude(
    int nValue)
{
    if (nValue < 4)
        return 1;
    else if (nValue < 8)
        return 2;
    else if (nValue < 16)
        return 3;
    else if (nValue < 256)
        return 4;
    else
        return 8;
}

/***************************************************************************\
* MatchImage
*
* This function takes LPINTs for width & height in case of "real size".
* For this option, we use dimensions of 1st icon in resdir as size to
* load, instead of system metrics.
*
* Returns a number that measures how "far away" the given image is
* from a desired one.  The value is 0 for an exact match.  Note that our
* formula has the following properties:
*     (1) Differences in width/height count much more than differences in
*         color format.
*     (2) Fewer colors give a smaller difference than more
*     (3) Bigger images are better than smaller, since shrinking produces
*             better results than stretching.
*
* The formula is the sum of the following terms:
*     Log2(colors wanted) - Log2(colors really), times -2 if the image
*         has more colors than we'd like.  This is because we will lose
*         information when converting to fewer colors, like 16 color to
*         monochrome.
*     Log2(width really) - Log2(width wanted), times -2 if the image is
*         narrower than what we'd like.  This is because we will get a
*         better result when consolidating more information into a smaller
*         space, than when extrapolating from less information to more.
*     Log2(height really) - Log2(height wanted), times -2 if the image is
*         shorter than what we'd like.  This is for the same reason as
*         the width.
*
* Let's step through an example.  Suppose we want a 16 color, 32x32 image,
* and are choosing from the following list:
*     16 color, 64x64 image
*     16 color, 16x16 image
*      8 color, 32x32 image
*      2 color, 32x32 image
*
* We'd prefer the images in the following order:
*      8 color, 32x32         : Match value is 0 + 0 + 1     == 1
*     16 color, 64x64         : Match value is 1 + 1 + 0     == 2
*      2 color, 32x32         : Match value is 0 + 0 + 3     == 3
*     16 color, 16x16         : Match value is 2*1 + 2*1 + 0 == 4
*
\***************************************************************************/

UINT MatchImage(
    LPRESDIR lprd,
    LPINT    lpcxWant,
    LPINT    lpcyWant,
    UINT     uColorsWant,
    BOOL     fIcon)
{
    UINT uColorsNew;
    int  cxNew;
    int  cyNew;

    cxNew = lprd->Icon.Width;
    cyNew = lprd->Icon.Height;

    if (fIcon) {
        uColorsNew = lprd->Icon.ColorCount;
    } else {
        cyNew >>= 1;
        uColorsNew = BPP01_MAXCOLORS;
    }

    /*
     * 0 really means maximum size (256) or colors (256).
     */
    if (!cxNew)
        cxNew = BPP08_MAXCOLORS;

    if (!*lpcxWant)
        *lpcxWant = cxNew;

    if (!cyNew)
        cyNew = BPP08_MAXCOLORS;

    if (!*lpcyWant)
        *lpcyWant = cyNew;

    if (!uColorsNew)
        uColorsNew = BPP08_MAXCOLORS;

    /*
     * Here are the rules for our "match" formula:
     *      (1) A close size match is much preferable to a color match
     *      (2) Fewer colors are better than more
     *      (3) Bigger icons are better than smaller
     *
     * The color count, width, and height are powers of 2.  So we use Magnitude()
     * which calculates the order of magnitude in base 2.
     */
#if 0 //CHRISWIL: Not in Win95

    return (MagnitudeDiff(uColorsWant, uColorsNew) +
            MagnitudeDiff(cxNew, *lpcxWant) +
            MagnitudeDiff(cyNew, *lpcyWant));

#else

    return( 2*MyAbs(Magnitude(uColorsWant), Magnitude(uColorsNew)) +
              MyAbs(cxNew, *lpcxWant) +
              MyAbs(cyNew, *lpcyWant));
#endif
}

/***************************************************************************\
* GetBestImage
*
* Among the different forms of images, choose the one that best matches the
* color format & dimensions of the request.  We try to match the size, then
* the color info.  So we find the item that
*
* (1) Has closest dimensions (smaller or bigger equally good) to minimize
*     the width, height difference
* (2) Has best colors.  We favor less over more colors.
*
* If we find an identical match, we return immediately.
*
\***************************************************************************/

UINT GetBestImage(
    LPRESDIR lprd,
    UINT     uCount,
    int      cxDesired,
    int      cyDesired,
    UINT     bpp,
    BOOL     fIcon)
{
    UINT uIndex;
    UINT uT;
    UINT uIndexBest = 0;
    UINT uBest = (UINT)-1;


    /*
     * Get desired number of colors in # value, not bits value.  Note that
     * we do NOT allow you to have  16- or 32- or 24- bit color icons.
     *
     * the icon resources can be 16, 24, 32 bpp, but the restable only has
     * a color count, so a HiColor icon would have a max value in the
     * restable.  we treat a 0 in the color count as "max colors"
     */
    if (bpp == 0)
        bpp = (UINT)gpsi->BitCount;

    if (bpp > 8)
        bpp = 8;

    bpp = 1 << bpp;

    /*
     * Loop through resource entries, saving the "closest" item so far.  Most
     * of the real work is in MatchImage(), which uses a fabricated formula
     * to give us the results that we desire.  Namely, an image as close in
     * size to what we want preferring bigger over smaller, then an image
     * with the right color format
     */
    for (uIndex = 0; uIndex < uCount; uIndex++, lprd++) {

        /*
         * Get "matching" value.  How close are we to what we want?
         */
        uT = MatchImage(lprd, &cxDesired, &cyDesired, bpp, fIcon);

        if (!uT) {

            /*
             * We've found an exact match!
             */
            return uIndex;

        } else if (uT < uBest) {

            /*
             * We've found a better match than the current alternative.
             */
            uBest = uT;
            uIndexBest = uIndex;
        }
    }

    return uIndexBest;
}

/***************************************************************************\
* GetIcoCurWidth
*
* When zero is passed in for a dimension, calculates what size we should
* really used.  Done in a couple o' places, so made it a FN().
*
\***************************************************************************/

_inline DWORD GetIcoCurWidth(
    DWORD cxOrg,
    BOOL  fIcon,
    UINT  lrFlags,
    DWORD cxDes)
{
    if (cxOrg) {
        return cxOrg;
    } else if (lrFlags & LR_DEFAULTSIZE) {
        return (fIcon ? SYSMET(CXICON) : SYSMET(CXCURSOR));
    } else {
        return cxDes;
    }
}

/***************************************************************************\
* GetIcoCurHeight
*
* Vertical counterpart to GetWidth().
*
\***************************************************************************/

_inline DWORD GetIcoCurHeight(
    DWORD cyOrg,
    BOOL  fIcon,
    UINT  lrFlags,
    DWORD cyDes)
{
    if (cyOrg) {
        return cyOrg;
    } else if (lrFlags & LR_DEFAULTSIZE) {
        return (fIcon ? SYSMET(CYICON) : SYSMET(CYCURSOR));
    } else {
        return cyDes;
    }
}

/***************************************************************************\
* GetIcoCurBpp
*
* Returns best match Bpp based on lr-flags.
*
\***************************************************************************/

_inline DWORD GetIcoCurBpp(
    UINT lrFlags)
{
    if (lrFlags & LR_MONOCHROME) {

#if DBG
        if (lrFlags & LR_VGACOLOR) {
            RIPMSG0(RIP_WARNING, "lrFlags has both MONOCHROME and VGACOLOR; assuming MONOCHROME");
        }
#endif
        return 1;

    } else if (
            TEST_PUSIF(PUSIF_PALETTEDISPLAY) ||
            (lrFlags & LR_VGACOLOR) ||
            !SYSMET(SAMEDISPLAYFORMAT)) {

        return 4;
    } else {
        return 0;
    }
}

/***************************************************************************\
* WOWFindResourceExWCover
*
* The WOW FindResource routines expect an ansi string so we have to
* convert the calling string IFF it is not an ID
*
\***************************************************************************/

HANDLE WOWFindResourceExWCover(
    HANDLE  hmod,
    LPCWSTR rt,
    LPCWSTR lpUniName,
    WORD    LangId)
{
    LPSTR  lpAnsiName;
    HANDLE hRes;

    if (ID(lpUniName))
        return FINDRESOURCEEXA(hmod, (LPSTR)lpUniName, (LPSTR)rt, LangId);

    /*
     * Otherwise convert the name of the menu then call LoadMenu
     */
    if (!WCSToMB(lpUniName, -1, &lpAnsiName, -1, TRUE))
        return NULL;

    hRes = FINDRESOURCEEXA(hmod, lpAnsiName, (LPSTR)rt, LangId);

    UserLocalFree(lpAnsiName);

    return hRes;
}

/***************************************************************************\
* WOWLoadBitmapA
*
*
\***************************************************************************/

HBITMAP WOWLoadBitmapA(
    HINSTANCE hmod,
    LPCSTR    lpName,
    LPBYTE    pResData,
    DWORD     cbResData)
{
    LPWSTR  lpUniName;
    HBITMAP hRet;

    UNREFERENCED_PARAMETER(cbResData);

    if (pResData == NULL) {

        if (ID(lpName))
            return LoadBmp(hmod, (LPCWSTR)lpName, 0, 0, 0);

        if (!MBToWCS(lpName, -1, &lpUniName, -1, TRUE))
            return NULL;

        hRet = LoadBmp(hmod, lpUniName, 0, 0, 0);

        UserLocalFree(lpUniName);

    } else {

        hRet = ConvertDIBBitmap((LPBITMAPINFOHEADER)pResData,
                                0,
                                0,
                                LR_DEFAULTSIZE,
                                NULL,
                                NULL);
    }

    return hRet;
}

/***************************************************************************\
* WOWServerLoadCreateCursorIcon
*
*
\***************************************************************************/

HICON WowServerLoadCreateCursorIcon(
    HANDLE  hmod,
    LPWSTR  pszModName,
    DWORD   dwExpWinVer,
    LPCWSTR lpName,
    DWORD   cb,
    PVOID   pResData,
    LPWSTR  type,
    BOOL    fClient)
{
    HICON hRet;
    BOOL  fIcon = (type == RT_ICON);
    UINT  LR_Flags = LR_SHARED;

    UNREFERENCED_PARAMETER(pszModName);
    UNREFERENCED_PARAMETER(dwExpWinVer);
    UNREFERENCED_PARAMETER(cb);
    UNREFERENCED_PARAMETER(fClient);

    if (!fIcon)
        LR_Flags |= LR_MONOCHROME;

    if (pResData == NULL) {

        hRet = LoadIcoCur(hmod,
                          lpName,
                          type,
                          0,
                          0,
                          LR_Flags | LR_DEFAULTSIZE);

    } else {

        hRet = ConvertDIBIcon((LPBITMAPINFOHEADER)pResData,
                              hmod,
                              lpName,
                              fIcon,
                              GetIcoCurWidth(0 , fIcon, LR_DEFAULTSIZE, 0),
                              GetIcoCurHeight(0, fIcon, LR_DEFAULTSIZE, 0),
                              LR_Flags);
    }

    return hRet;
}

/***************************************************************************\
* WOWServerLoadCreateMenu
*
*
\***************************************************************************/
HMENU WowServerLoadCreateMenu(
    HANDLE hMod,
    LPCSTR lpName,
    CONST  LPMENUTEMPLATE pmt,
    DWORD  cb,
    BOOL   fCallClient)
{
    UNREFERENCED_PARAMETER(cb);
    UNREFERENCED_PARAMETER(fCallClient);

    if (pmt == NULL) {
        return LoadMenuA(hMod, lpName);
    } else
        return CreateMenuFromResource(pmt);
}

/***********************************************************************\
* DIBFromBitmap()
*
*  Creates a memory block with DIB information from a physical bitmap tagged
*  to a specific DC.
*
*  A DIB block consists of a BITMAPINFOHEADER + RGB colors + DIB bits.
*
* Returns: UserLocalAlloc pointer to DIB info.
*
* 03-Nov-1995 SanfordS  Created.
\***********************************************************************/

PVOID DIBFromBitmap(
    HBITMAP hbmp,
    HDC     hdc)
{
    BITMAP             bmp;
    LPBITMAPINFOHEADER lpbi;
    DWORD              cbBits;
    DWORD              cbPalette;
    DWORD              cbTotal;
    WORD               cBits;

    UserAssert(hbmp);
    UserAssert(hdc);

    if (GetObject(hbmp, sizeof(BITMAP), &bmp) == 0)
        return NULL;

    cBits = ((WORD)bmp.bmPlanes * (WORD)bmp.bmBitsPixel);

TrySmallerDIB:

    cbBits = (DWORD)WIDTHBYTES((WORD)bmp.bmWidth * cBits) * (DWORD)bmp.bmHeight;

    cbPalette = 0;
    if (cBits <= 8)
        cbPalette = (1 << cBits) * sizeof(RGBQUAD);
    else
        cbPalette = 3 * sizeof(RGBQUAD);

    cbTotal  = sizeof(BITMAPINFOHEADER) + cbPalette + cbBits;
    lpbi = (LPBITMAPINFOHEADER)UserLocalAlloc(HEAP_ZERO_MEMORY, cbTotal);
    if (lpbi == NULL) {

        /*
         * Try a smaller DIB, if we can.  We can't if the DIB is mono.
         */
        switch (cBits) {
        case 4:
            cBits = 1;
            break;

        case 8:
            cBits = 4;
            break;

        case 16:
            cBits = 8;
            break;

        case 24:
            cBits = 16;
            break;

        case 32:
            cBits = 24;
            break;

        default:
            return NULL;   // 1 or wierd.
        }

        RIPMSG1(RIP_WARNING, "Not enough memory to create large color DIB, trying %d bpp.", cBits);
        goto TrySmallerDIB;
    }

    RtlZeroMemory(lpbi, sizeof(BITMAPINFOHEADER));
    lpbi->biSize        = sizeof(BITMAPINFOHEADER);
    lpbi->biWidth       = bmp.bmWidth;
    lpbi->biHeight      = bmp.bmHeight;
    lpbi->biPlanes      = 1;
    lpbi->biBitCount    = cBits;

    /*
     * Get old bitmap's DIB bits, using the current DC.
     */
    GetDIBits(hdc,
              hbmp,
              0,
              lpbi->biHeight,
              ((LPSTR)lpbi) + lpbi->biSize + cbPalette,
              (LPBITMAPINFO)lpbi,
              DIB_RGB_COLORS);

    lpbi->biClrUsed   = cbPalette / sizeof(RGBQUAD);
    lpbi->biSizeImage = cbBits;

    return lpbi;
}

/***************************************************************************\
* CopyBmp
*
* Creates a new bitmap and copies the given bitmap to the new one,
* stretching and color-converting the bits if desired.
*
* 03-Nov-1995 SanfordS  Created.
\***************************************************************************/

HBITMAP CopyBmp(
    HBITMAP hbmpOrg,
    int     cxNew,
    int     cyNew,
    UINT    LR_flags)
{
    HBITMAP hbmNew = NULL;
    LPBITMAPINFOHEADER pdib;

    RtlEnterCriticalSection(&gcsHdc);

    if (pdib = DIBFromBitmap(hbmpOrg, ghdcBits2)) {

#if 0  // Win-9x comments this code out
        if (LR_flags & LR_COPYRETURNORG) {

            DWORD bpp = GetIcoCurBpp(LR_flags);

            if ((cxNew == 0 || cxNew == pdib->biWidth)  &&
                (cyNew == 0 || cyNew == pdib->biHeight) &&
                (bpp == 0 || bpp == pdib->biBitCount)) {

                hbmNew = hbmpOrg;
            }
        }

        if (hbmNew == NULL)
            hbmNew = ConvertDIBBitmap(pdib, cxNew, cyNew, LR_flags, NULL, NULL);
#endif

        hbmNew = ConvertDIBBitmap(pdib, cxNew, cyNew, LR_flags, NULL, NULL);

        UserLocalFree(pdib);
    }

    RtlLeaveCriticalSection(&gcsHdc);

    if ((LR_flags & LR_COPYDELETEORG) && hbmNew && (hbmNew != hbmpOrg))
        DeleteObject(hbmpOrg);

    return hbmNew;
}

/***********************************************************************\
* CopyImageFromRes
*
* This is used by the LR_COPYFROMRESOURCE option.  We assume that the
* icon/cursor passed in is among the process list of loaded shared
* icons.  If we find it there, we can attempt to load the icon from
* the resource to get an image that looks better than a stretched or
* compressed one.
*
* That way we will not stretch a 32x32 icon to 16x16 if someone added
* a 16x16 image to their class icon--a simple way for apps to jazz up
* their appearance.
*
* 12-Mar-1996 ChrisWil  Created.
\***********************************************************************/

HICON CopyImageFromRes(
    LPWSTR      pszModName,
    LPWSTR      pszResName,
    PCURSORFIND pcfSearch,
    UINT        LR_flags)
{
    HINSTANCE hmod;
    HICON     hicoDst = NULL;

    /*
     * Override the search-criteria if this is the user-module.  By
     * setting these to zero, we are basically saying "don't care" for
     * these attributes.
     */
    hmod = (pszModName ? WowGetModuleHandle(pszModName) : hmodUser);

    if (hmod == hmodUser) {

        pcfSearch->cx  = 0;
        pcfSearch->cy  = 0;
        pcfSearch->bpp = 0;

        pszModName = szUSER32;
    }

    /*
     * If a resource has been found with this name/bpp, then attempt
     * to load the resource with the desired dimensions.
     */
    if (FindExistingCursorIcon(pszModName, pszResName, pcfSearch)) {

        hicoDst = LoadIcoCur(hmod,
                             pszResName,
                             (LPWSTR)ULongToPtr( pcfSearch->rt ),
                             pcfSearch->cx,
                             pcfSearch->cy,
                             LR_flags);
    }

    return hicoDst;
}

/***********************************************************************\
*  CopyIcoCur()
*
*  Allocates a new icon resource and transmogrifies the old icon into the
*  newly desired format.
*
*  Note that if we have to stretch the icon, the hotspot area changes.  For
*  icons, the hotspot is set to be the middle of the icon.
*
* Returns:
*
* 01-Nov-1995 SanfordS  Created.
* 12-Mar-1996 ChrisWil  Added lookup for existing icon/cursor.
\***********************************************************************/

HICON CopyIcoCur(
    HICON hicoSrc,
    BOOL  fIcon,
    int   cxNew,
    int   cyNew,
    UINT  LR_flags)
{
    HBITMAP        hbmMaskNew;
    HBITMAP        hbmColorNew;
    int            cx;
    int            cy;
    DWORD          bpp;
    DWORD          bppDesired;
    HICON          hicoDst = NULL;
    ICONINFO       ii;
    CURSORDATA     cur;
    UNICODE_STRING strModName;
    UNICODE_STRING strResName;
    WCHAR          awszModName[MAX_PATH];
    WCHAR          awszResName[MAX_PATH];

    /*
     * Extract needed info from existing icon/cursor from the kernel
     */
    if (!NtUserGetIconSize(hicoSrc, 0, &cx, &cy))
        return NULL;

    cy >>= 1;

    if (LR_flags & LR_CREATEDIBSECTION)
        LR_flags = (LR_flags & ~LR_CREATEDIBSECTION) | LR_CREATEREALDIB;

    /*
     * Setup unicode-strings for calls to kernel-side.
     */
    strModName.Length        = 0;
    strModName.MaximumLength = MAX_PATH;
    strModName.Buffer        = awszModName;

    strResName.Length        = 0;
    strResName.MaximumLength = MAX_PATH;
    strResName.Buffer        = awszResName;

    /*
     * Note: this creates copies of hbmMask and hbmColor that need to be
     * freed before we leave.
     */
    if (!NtUserGetIconInfo(hicoSrc,
                           &ii,
                           &strModName,
                           &strResName,
                           &bpp,
                           TRUE)) {

        return NULL;
    }

    cxNew = GetIcoCurWidth(cxNew, fIcon, LR_flags, cx);
    cyNew = GetIcoCurHeight(cyNew, fIcon, LR_flags, cy);

    if (LR_flags & LR_COPYFROMRESOURCE) {

        CURSORFIND cfSearch;
        LPWSTR     pszModName;

        /*
         * Setup the search criteria.
         */
        cfSearch.hcur = hicoSrc;
        cfSearch.rt   = PtrToUlong((fIcon ? RT_ICON : RT_CURSOR));
        cfSearch.cx   = cxNew;
        cfSearch.cy   = cyNew;
        cfSearch.bpp  = bpp;

        /*
         * Copy the image.  This performs a lookup for the hicoSrc.  If
         * it is not found in the process and shared caches, then we
         * will proceed with copying the hicoSrc.  If an icon is found
         * in the cache, then we will attempt to reload the image for
         * the best resolution possible.
         */
        pszModName = (strModName.Length ? strModName.Buffer : NULL);

        hicoDst = CopyImageFromRes(pszModName,
                                   strResName.Buffer,
                                   &cfSearch,
                                   LR_flags);

        if (hicoDst)
            goto CleanupExit;
    }

    bppDesired = GetIcoCurBpp(LR_flags);

    if ((cxNew != cx) ||
        (cyNew != cy) ||
        ((bpp != 1) && (bppDesired != 0) && (bppDesired != bpp))) {

        /*
         * Since we have to stretch or maybe fixup the colors just get
         * the DIB bits and let ConverDIBBitmap do all the magic.
         */
        hbmMaskNew = CopyBmp(ii.hbmMask, cxNew, cyNew * 2, LR_MONOCHROME);

        if (hbmMaskNew == NULL)
            goto CleanupExit;

        hbmColorNew = NULL;

        if (ii.hbmColor) {

            hbmColorNew = CopyBmp(ii.hbmColor, cxNew, cyNew, LR_flags);

            if (hbmColorNew == NULL) {
                DeleteObject(hbmMaskNew);
                goto CleanupExit;
            }
        }

        /*
         * Replace ii.hbmxxx guys with our fixed up copies and delete the old.
         */
        DeleteObject(ii.hbmMask);
        ii.hbmMask = hbmMaskNew;

        if (ii.hbmColor && (ii.hbmColor != hbmColorNew)) {
            DeleteObject(ii.hbmColor);
            ii.hbmColor = hbmColorNew;
        }

        /*
         * tweak the hotspots for changes in size.
         */
        if (cxNew != cx)
            ii.xHotspot = MultDiv(ii.xHotspot, cxNew, cx);

        if (cyNew != cy)
            ii.yHotspot = MultDiv(ii.yHotspot, cyNew, cy);

    } else if (LR_flags & LR_COPYRETURNORG) {

        hicoDst = hicoSrc;

CleanupExit:

        /*
         * Free up the bitmaps which were created by GetIconInfo().
         */
        DeleteObject(ii.hbmMask);

        if (ii.hbmColor)
            DeleteObject(ii.hbmColor);

        goto Exit;
    }

    /*
     * Build the icon/cursor object from the info.  The bitmaps
     * are not freed in this case.
     */
    hicoDst = (HICON)NtUserCallOneParam(0, SFI__CREATEEMPTYCURSOROBJECT);

    if (hicoDst == NULL)
        goto CleanupExit;

    RtlZeroMemory(&cur, sizeof(cur));
    cur.lpName    = strResName.Length ? strResName.Buffer : NULL;
    cur.lpModName = strModName.Length ? strModName.Buffer : NULL;
    cur.rt        = ii.fIcon ? PTR_TO_ID(RT_ICON) : PTR_TO_ID(RT_CURSOR);
    cur.bpp       = bpp;
    cur.cx        = cxNew;
    cur.cy        = cyNew * 2;
    cur.xHotspot  = (short)ii.xHotspot;
    cur.yHotspot  = (short)ii.yHotspot;
    cur.hbmMask   = ii.hbmMask;
    cur.hbmColor  = ii.hbmColor;

    if (!_SetCursorIconData(hicoDst, &cur)) {
        NtUserDestroyCursor(hicoDst, CURSOR_ALWAYSDESTROY);
        return NULL;
    }

Exit:

    /*
     * destroy the original if asked to.
     */
    if (hicoDst != hicoSrc && (LR_flags & LR_COPYDELETEORG))
        DestroyCursor(hicoSrc);

    return hicoDst;
}

/***********************************************************************\
* CopyImage
*
* Allocates a new icon resource and copies the attributes of the old icon
* to the new icon.
*
* Returns: hIconNew
*
* 01-Nov-1995 SanfordS  Created.
\***********************************************************************/

HANDLE WINAPI CopyImage(
    HANDLE hImage,
    UINT   IMAGE_flag,
    int    cxNew,
    int    cyNew,
    UINT   LR_flags)
{
    if (LR_flags & ~LR_VALID) {
        RIPERR0(ERROR_INVALID_PARAMETER, RIP_WARNING, "CopyImage: bad LR_flags.");
        return NULL;
    }

    return InternalCopyImage(hImage, IMAGE_flag, cxNew, cyNew, LR_flags);
}

/***********************************************************************\
* InternalCopyImage
*
* Performs the copyimage work.  This is called from the callback-thunk.
*
\***********************************************************************/

HANDLE InternalCopyImage(
    HANDLE hImage,
    UINT   IMAGE_flag,
    int    cxNew,
    int    cyNew,
    UINT   LR_flags)
{
    switch (IMAGE_flag) {

    case IMAGE_BITMAP:
        if (GetObjectType(hImage) != OBJ_BITMAP) {
            RIPMSG0(RIP_ERROR, "CopyImage: invalid bitmap");
            return NULL;
        }

        return (HICON)CopyBmp(hImage, cxNew, cyNew, LR_flags);

    case IMAGE_CURSOR:
    case IMAGE_ICON:

        return CopyIcoCur(hImage,
                          (IMAGE_flag == IMAGE_ICON),
                          cxNew,
                          cyNew,
                          LR_flags);
    }

    RIPERR0(ERROR_INVALID_PARAMETER, RIP_WARNING, "CopyImage: bad IMAGE_flag.");

    return NULL;
}

/***************************************************************************\
* RtlGetIdFromDirectory
*
* History:
* 06-Apr-1991 ScottLu   Cleaned up, make work with client/server.
* 16-Nov-1995 SanfordS  Now uses LookupIconIdFromDirectoryEx
\***************************************************************************/

int RtlGetIdFromDirectory(
    PBYTE  presbits,
    BOOL   fIcon,
    int    cxDesired,
    int    cyDesired,
    DWORD  LR_flags,
    PDWORD pdwResSize)
{
    LPNEWHEADER lpnh;
    LPRESDIR    lprsd;
    UINT        iImage;
    UINT        cImage;
    UINT        bpp;

    /*
     * Make sure this is pointing to valid resource bits.
     */
    if (presbits == NULL)
        return 0;

    lpnh = (LPNEWHEADER)presbits;

    /*
     * Fill in defaults.
     */
    cxDesired = GetIcoCurWidth(cxDesired, fIcon, LR_flags, 0);
    cyDesired = GetIcoCurHeight(cyDesired, fIcon, LR_flags, 0);

    bpp = GetIcoCurBpp(LR_flags);

    /*
     * We'll use the first image in the directory if we can't find one
     * that's appropriate.
     */
    cImage = lpnh->ResCount;
    lprsd  = (LPRESDIR)(lpnh + 1);

    iImage = GetBestImage(lprsd, cImage, cxDesired, cyDesired, bpp, fIcon);

    if (iImage == cImage)
        iImage = 0;

    if (pdwResSize != NULL)
        *pdwResSize = (lprsd + iImage)->BytesInRes;

    return ((LPRESDIR)(lprsd + iImage))->idIcon;
}
