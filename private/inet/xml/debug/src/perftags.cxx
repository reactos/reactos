/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
//+------------------------------------------------------------------------
//
//  File:       perftags.cxx
//
//  Contents:   Utilities for measuring perf
//
//-------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#ifndef X_STDDEF_H_
#define X_STDDEF_H_
#include <stddef.h>
#endif

#ifndef X_RESOURCE_H_
#define X_RESOURCE_H_
#include "resource.h"
#endif

struct LOGENTRY
{
    LOGENTRY *  pleNext;
    __int64     t;
    void *      pvObject;
    DWORD       dwThread;
    char        ach[1024];
};

struct LOGBLOCK
{
    LOGBLOCK *  plbNext;
    char        ach[8192];
};

struct LTAG
{
    BOOL    fEnabled;       // This must be first!
    LTAG *   ptagNext;
    char *  pchTag;
    char *  pchOwner;
    char *  pchDesc;
    char    ach[1];
};

extern HINSTANCE  g_hinstMain;
static LTAG *      g_ptagHead = NULL;
static LOGBLOCK * g_plbHead = NULL;
static LOGBLOCK * g_plbTail = NULL;
static LOGENTRY * g_plePrev = NULL;
static LOGENTRY * g_pleNext = NULL;
static UINT       g_cbLeft  = 0;
CRITICAL_SECTION  g_csLog;
static HWND     ghWnd           = NULL;
static HBRUSH   ghbrBkgnd       = NULL;
static HANDLE   ghThread        = NULL;
static BOOL     fDlgUp          = FALSE;
static DWORD    rgbWindowColor  = 0xFF000000;    // variables for the current
static DWORD    rgbHiliteColor  = 0xFF000000;    // system color settings.
static DWORD    rgbWindowText   = 0xFF000000;    // on a WM_SYSCOLORCHANGE
static DWORD    rgbHiliteText   = 0xFF000000;    // we check to see if we need
static DWORD    rgbGrayText     = 0xFF000000;    // to reload our bitmap.
static DWORD    rgbDDWindow     = 0xFF000000;    //
static DWORD    rgbDDHilite     = 0xFF000000;    // 0xFF000000 is an invalid RGB
static BOOL     g_fEnabled      = FALSE;
static BOOL     g_fAutoDump     = FALSE;
static BOOL     g_fAutoOpen     = FALSE;
static BOOL     g_fAutoDelete   = TRUE;
static BOOL     g_fSimpleDump   = TRUE;

void __cdecl hprintf(HANDLE hfile, char * pchFmt, ...);
#undef MemAlloc
#define MemAlloc(cb)    LocalAlloc(LMEM_FIXED, (cb))
#undef MemFree
#define MemFree(pv)     LocalFree(pv)

INT_PTR WINAPI PerfRegister(char * szTag, char * szOwner, char * szDesc)
{
    EnterCriticalSection(&g_csLog);

    int cbTag   = lstrlenA(szTag) + 1;
    int cbOwner = lstrlenA(szOwner) + 1;
    int cbDesc  = lstrlenA(szDesc) + 1;
    LTAG * ptag  = (LTAG *)MemAlloc(offsetof(LTAG, ach) + cbTag + cbOwner + cbDesc);
    char * pch  = ptag->ach;

    ptag->ptagNext = g_ptagHead;
    ptag->fEnabled = (BYTE)!!GetPrivateProfileIntA("perftags", szTag, FALSE, "\\msxmldbg.ini");

    ptag->pchTag = pch;
    lstrcpyA(pch, szTag);
    pch += cbTag;
    
    ptag->pchOwner = pch;
    lstrcpyA(pch, szOwner);
    pch += cbOwner;

    ptag->pchDesc = pch;
    lstrcpyA(pch, szDesc);

    g_ptagHead = ptag;

    LeaveCriticalSection(&g_csLog);

    if (ghWnd)
    {
        PostMessageA(ghWnd, WM_COMMAND, IDC_REFRESH, 0);
    }

    return((INT_PTR)ptag);
}

void __cdecl PerfLogFn(int tag, void * pvObj, const char * pchFmt, ...)
{
    LOGENTRY    le;
    va_list     vl;
    UINT        cb;

    if (!tag || !((LTAG *)tag)->fEnabled || !g_fEnabled)
        return;

    le.pleNext = NULL;
    le.pvObject = pvObj;
    le.dwThread = GetCurrentThreadId();

    va_start(vl, pchFmt);
    cb = wvsprintfA(le.ach, pchFmt, vl);
    va_end(vl);

    cb = (cb + offsetof(LOGENTRY, ach) + 1);
    cb = (cb + 7) & ~7;

    EnterCriticalSection(&g_csLog);

    QueryPerformanceCounter((LARGE_INTEGER *)&le.t);

    if (cb > g_cbLeft)
    {
        LOGBLOCK * plb = (LOGBLOCK *)MemAlloc(sizeof(LOGBLOCK));

        if (plb == NULL)
            goto ret;

        plb->plbNext = NULL;

        if (g_plbTail)
        {
            g_plbTail->plbNext = plb;
            g_plbTail = plb;
        }
        else
        {
            g_plbHead = g_plbTail = plb;
        }

        g_pleNext = (LOGENTRY *)plb->ach;
        g_cbLeft  = sizeof(plb->ach);
    }

    if (g_plePrev)
        g_plePrev->pleNext = g_pleNext;

    g_plePrev = g_pleNext;
    g_pleNext = (LOGENTRY *)((BYTE *)g_plePrev + cb);
    g_cbLeft -= cb;
    memcpy(g_plePrev, &le, cb);

ret:
    LeaveCriticalSection(&g_csLog);
}

int __cdecl PerfLogCompareTime(const void * pv1, const void * pv2)
{
    __int64 t1 = (*(LOGENTRY **)pv1)->t;
    __int64 t2 = (*(LOGENTRY **)pv2)->t;
    return((t1 < t2) ? -1 : (t1 > t2) ? 1 : 0);
}

int __cdecl PerfLogCompareThread(const void * pv1, const void * pv2)
{
    DWORD dw1 = (*(LOGENTRY **)pv1)->dwThread;
    DWORD dw2 = (*(LOGENTRY **)pv2)->dwThread;
    return((dw1 < dw2) ? -1 : (dw1 > dw2) ? 1 : PerfLogCompareTime(pv1, pv2));
}

int __cdecl PerfLogCompareObject(const void * pv1, const void * pv2)
{
    DWORD_PTR dw1 = (DWORD_PTR)(*(LOGENTRY **)pv1)->pvObject;
    DWORD_PTR dw2 = (DWORD_PTR)(*(LOGENTRY **)pv2)->pvObject;
    return((dw1 < dw2) ? -1 : (dw1 > dw2) ? 1 : PerfLogCompareTime(pv1, pv2));
}

static void PerfLogDumpRange(HANDLE hfile, LOGENTRY ** pple, ULONG_PTR cle)
{
    __int64 tmin = (*pple)->t, tfrq, tnext;
    LOGENTRY * ple;
    int cLevel = 0;
    
    QueryPerformanceFrequency((LARGE_INTEGER *)&tfrq);

    hprintf(hfile, " TID         Time       Len      Object\r\n");

    for (; cle > 0; --cle, ++pple)
    {
        ple = *pple;
        tnext = (cle > 1) ? (*(pple + 1))->t : ple->t;

        hprintf(hfile, "%8lX %7ld.%ld (%6ld.%ld) %8lX ",
            ple->dwThread,
            (LONG)((ple->t - tmin) * 1000 / tfrq),
            (LONG)((ple->t - tmin) * 10000 / tfrq) % 10,
            (LONG)((tnext - ple->t) * 1000 / tfrq),
            (LONG)((tnext - ple->t) * 10000 / tfrq) % 10,
            ple->pvObject);

        if (ple->ach[0] == '-' && cLevel > 0)
            cLevel -= 1;

        for (int i = 0; i < cLevel; ++i)
            hprintf(hfile, "  ");

        hprintf(hfile, "%s\r\n", ple->ach);

        if (ple->ach[0] == '+')
            cLevel += 1;
    }

    hprintf(hfile, "\r\n");
}

char * GetPerfLogPath(char * pch)
{
    char    szModule[MAX_PATH];
    char *  pszModule;

    GetModuleFileNameA(NULL, szModule, MAX_PATH);
    
    pszModule = (LPSTR)(szModule + lstrlenA(szModule));
    
    while (*pszModule-- != '\\') ;

    pszModule += 2;
    
    CharUpperBuffA(pszModule, lstrlenA(pszModule));
    
    wsprintfA(pch, "\\%s-%lX.log", pszModule, GetCurrentProcessId());

    return(pch);
}

void PerfDump()
{
    EnterCriticalSection(&g_csLog);

    LOGENTRY *  pleHead = g_plbHead ? (LOGENTRY *)g_plbHead->ach : NULL;
    LOGENTRY *  ple, **pple;
    int         cle = 0;
    LOGENTRY ** rgple;
    char        achLog[MAX_PATH];
    HANDLE      hfile = CreateFileA(GetPerfLogPath(achLog), GENERIC_WRITE,
        FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hfile == INVALID_HANDLE_VALUE)
        goto ret;

    SetFilePointer(hfile, 0, NULL, FILE_END);

    for (ple = pleHead; ple; ple = ple->pleNext)
    {
        cle += 1;
    }

    if (cle == 0)
    {
        CloseHandle(hfile);
        goto ret;
    }

    rgple = (LOGENTRY **)MemAlloc(cle * sizeof(LOGENTRY *));

    for (ple = pleHead, pple = rgple; ple; ple = ple->pleNext, ++pple)
    {
        *pple = ple;
    }

    if (g_fSimpleDump)
    {
        hprintf(hfile, "+++ All Entries (as captured)\r\n\r\n");
        PerfLogDumpRange(hfile, rgple, cle);
    }
    else
    {
        int i;

        qsort(rgple, cle, sizeof(LOGENTRY *), PerfLogCompareTime);

        hprintf(hfile, "+++ All Entries\r\n\r\n");

        PerfLogDumpRange(hfile, rgple, cle);

        qsort(rgple, cle, sizeof(LOGENTRY *), PerfLogCompareThread);

        for (i = 0, pple = rgple; i < cle; ++i, ++pple)
        {
            LOGENTRY ** ppleBeg = pple;
            ple = *pple;

            for (++pple, ++i; i < cle; ++i, ++pple)
            {
                if ((*pple)->dwThread != ple->dwThread)
                    break;
            }

            if (pple - ppleBeg != cle)
            {
                hprintf(hfile, "+++ Entries On Thread %lX\r\n\r\n", ple->dwThread);

                PerfLogDumpRange(hfile, ppleBeg, (ULONG_PTR)pple - (ULONG_PTR)ppleBeg);
            }

            i -= 1;
            pple -= 1;
        }

        qsort(rgple, cle, sizeof(LOGENTRY *), PerfLogCompareObject);

        for (i = 0, pple = rgple; i < cle; ++i, ++pple)
        {
            LOGENTRY ** ppleBeg = pple;
            ple = *pple;

            for (++pple, ++i; i < cle; ++i, ++pple)
            {
                if ((*pple)->pvObject != ple->pvObject)
                    break;
            }

            if (pple - ppleBeg != cle)
            {
                hprintf(hfile, "+++ Entries On Object %lX\r\n\r\n", ple->pvObject);
                PerfLogDumpRange(hfile, ppleBeg, (ULONG_PTR)pple - (ULONG_PTR)ppleBeg);
            }

            i -= 1;
            pple -= 1;
        }
    }

    hprintf(hfile, "------------------------------------------------------------------------------\r\n\r\n");

    CloseHandle(hfile);

    MemFree(rgple);

    PerfClear();

ret:
    LeaveCriticalSection(&g_csLog);
}

void PerfClear()
{
    LOGBLOCK * plb, * plbNext;

    EnterCriticalSection(&g_csLog);

    for (plb = g_plbHead, plbNext = NULL; plb; plb = plbNext)
    {
        plbNext = plb->plbNext;
        MemFree(plb);
    }

    g_plbHead = NULL;
    g_plbTail = NULL;
    g_plePrev = NULL;
    g_pleNext = NULL;
    g_cbLeft  = 0;

    LeaveCriticalSection(&g_csLog);
}

//+---------------------------------------------------------------------------
//  Perf Monitor
//----------------------------------------------------------------------------

DWORD WINAPI
PerfMonitorThread(LPVOID lpv);

INT_PTR WINAPI
PerfMonDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void    SetWindowTitle(HWND hWnd, LPSTR lpcText);
void    RefreshView(HWND hWnd);
void    SetSumSelection(void);
void    TagListNotify(WORD wNotify, HWND  hWnd);

void    MeasureItem(HWND hwnd, LPMEASUREITEMSTRUCT pmis);
int     CompareItem(LPCOMPAREITEMSTRUCT pcis);
void    DrawItem(LPDRAWITEMSTRUCT pdis);
void    OutTextFormat(LPDRAWITEMSTRUCT pdis);
static void SetRGBValues(void);

//  Global Data

void PerfTags()
{
    EnterCriticalSection(&g_csLog);

    if (!ghThread)
    {
        DWORD dwThreadId;

        fDlgUp = TRUE;

        ghThread = CreateThread((LPSECURITY_ATTRIBUTES) NULL, 0,
            (LPTHREAD_START_ROUTINE)PerfMonitorThread,
            NULL, 0, &dwThreadId);
    }

    LeaveCriticalSection(&g_csLog);
}

DWORD WINAPI PerfMonitorThread(LPVOID)
{
    ghbrBkgnd = CreateSolidBrush(GetSysColor(COLOR_BACKGROUND));

    DialogBoxA(g_hinstMain, MAKEINTRESOURCEA(IDD_PERFTAGS), NULL, (DLGPROC)PerfMonDlgProc);

    DeleteObject(ghbrBkgnd);
    ExitThread(0);
    
    return 0;
}

void RefreshView(HWND hWnd)
{
    SendDlgItemMessageA(hWnd, IDC_TAGLIST, LB_RESETCONTENT, 0, 0L);

    for (LTAG * ptag = g_ptagHead; ptag; ptag = ptag->ptagNext)
    {
        SendDlgItemMessageA(hWnd, IDC_TAGLIST, LB_ADDSTRING, 0, (LPARAM)ptag);
    }
}

void TagListNotify(WORD wNotify, HWND hWnd)
{
    LONG_PTR    idx;
    LTAG *   ptag;
    RECT    rc;
    
    if (wNotify == LBN_DBLCLK)
    {
        idx = SendMessageA(hWnd, LB_GETCURSEL, 0, 0L);
        if (idx == CB_ERR)
            return;

        ptag = (LTAG *)SendMessageA(hWnd, LB_GETITEMDATA, (WPARAM)idx, 0L);
        if (ptag == (LTAG*)CB_ERR)
            return;
        
        ptag->fEnabled = !ptag->fEnabled;

        WritePrivateProfileStringA("perftags", ptag->pchTag,
            ptag->fEnabled ? "1" : "0", "\\msxmldbg.ini");

        if (SendMessageA(hWnd, LB_GETITEMRECT, (WPARAM)idx, (LPARAM)&rc) != LB_ERR)
        {
            RedrawWindow(hWnd, &rc, NULL, RDW_INVALIDATE);
        }
    }
}

INT_PTR WINAPI
PerfMonDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    char    achLog[MAX_PATH];
    LONG    idx = 0;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            ghWnd = hWnd;
            SetRGBValues();
            SetWindowTitle(hWnd, "PerfTags - %s");
            RefreshView(hWnd);
            ShowWindow(hWnd, SW_SHOWMINNOACTIVE);
            break;

        case WM_SYSCOLORCHANGE:
            SetRGBValues();
            RefreshView(hWnd);
            break;
            
        case WM_MEASUREITEM:
            MeasureItem(hWnd, (LPMEASUREITEMSTRUCT)lParam);
            break;
            
        case WM_DRAWITEM:
            DrawItem((LPDRAWITEMSTRUCT)lParam);
            break;
            
        case WM_COMPAREITEM:
            return CompareItem((COMPAREITEMSTRUCT *)lParam);
            
        case WM_PAINT:
            return FALSE;

        case WM_ERASEBKGND:
            return IsIconic(hWnd);

        case WM_QUERYENDSESSION:
            PostMessageA(hWnd, WM_CLOSE, 0, 0);
            break;

        case WM_CLOSE:
            fDlgUp = FALSE;
            return EndDialog(hWnd, 0);

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {                       
                case IDC_TAGLIST:
                    TagListNotify(HIWORD(wParam), (HWND)lParam);
                    break;

                case IDC_ENABLEALL:
                case IDC_DISABLEALL:
                {
                    for (LTAG * ptag = g_ptagHead; ptag; ptag = ptag->ptagNext)
                    {
                        if (ptag->fEnabled != !!(LOWORD(wParam) == IDC_ENABLEALL))
                        {
                            ptag->fEnabled = !ptag->fEnabled;
                            WritePrivateProfileStringA("perftags", ptag->pchTag,
                                ptag->fEnabled ? "1" : "0", "\\msxmldbg.ini");
                        }
                    }

                    RefreshView(ghWnd);
                    break;
                }

                case IDC_CLEARLOG:
                    PerfClear();
                    break;

                case IDC_DUMPLOG:
                    PerfDump();
                    break;

                case IDC_DELETELOG:
                    DeleteFileA(GetPerfLogPath(achLog));
                    break;

                case IDC_REFRESH:
                    RefreshView(ghWnd);
                    break;

                case IDC_STARTSTOP:
                    SetWindowTitle((HWND)lParam, g_fEnabled ? "Start" : "Stop");
                    g_fEnabled = !g_fEnabled;
                    break;

                default:
                    return FALSE;
            }
            break;
        default:
            return FALSE;
    }
    
    return TRUE;
}

int CompareItem(LPCOMPAREITEMSTRUCT pcis)
{
    LTAG * ptag1 = (LTAG *)pcis->itemData1;
    LTAG * ptag2 = (LTAG *)pcis->itemData2;
    int iCmp;
    
    iCmp = lstrcmpA(ptag1->pchOwner, ptag2->pchOwner);

    if (iCmp == 0)
        iCmp = lstrcmpA(ptag1->pchDesc, ptag2->pchDesc);

    if (iCmp < 0)
        iCmp = -1;
    else if (iCmp > 0)
        iCmp = 1;

    return(iCmp);
}

void DrawItem(LPDRAWITEMSTRUCT pdis)
{
    char ach[1024];
    COLORREF crText = 0, crBack = 0;
    LTAG * ptag = (LTAG *)pdis->itemData;

    if((int)pdis->itemID < 0)
        return;

    if((ODA_DRAWENTIRE | ODA_SELECT) & pdis->itemAction)
    {
        if(pdis->itemState & ODS_SELECTED)
        {
            crText = SetTextColor(pdis->hDC, rgbHiliteText);
            crBack = SetBkColor(pdis->hDC, rgbHiliteColor);
        }

        wsprintfA(ach, " %s %-14s %s", ptag->fEnabled ? "[X]" : "[ ]",
            ptag->pchOwner, ptag->pchDesc);

        ExtTextOutA(pdis->hDC, pdis->rcItem.left, pdis->rcItem.top + 1,
            ETO_OPAQUE | ETO_CLIPPED, &pdis->rcItem, ach, lstrlenA(ach), NULL);

        // Restore original colors if we changed them above.
        if(pdis->itemState & ODS_SELECTED)
        {
            SetTextColor(pdis->hDC, crText);
            SetBkColor(pdis->hDC,   crBack);
        }
    }

    if((ODA_FOCUS & pdis->itemAction) || (ODS_FOCUS & pdis->itemState))
        DrawFocusRect(pdis->hDC, &pdis->rcItem);
}

static void SetRGBValues(VOID)
{
    rgbWindowColor = GetSysColor(COLOR_WINDOW);
    rgbHiliteColor = GetSysColor(COLOR_HIGHLIGHT);
    rgbWindowText  = GetSysColor(COLOR_WINDOWTEXT);
    rgbHiliteText  = GetSysColor(COLOR_HIGHLIGHTTEXT);
    rgbGrayText    = GetSysColor(COLOR_GRAYTEXT);
}

#ifdef BUILD_XMLDBG_AS_LIB
extern "C" 
#endif
void
PerfProcessAttach()
{
    InitializeCriticalSection(&g_csLog);
    g_fAutoDump   = GetPrivateProfileIntA("perftags", "AutoDump", FALSE, "\\msxmldbg.ini");
    g_fAutoOpen   = GetPrivateProfileIntA("perftags", "AutoOpen", FALSE, "\\msxmldbg.ini");
    g_fAutoDelete = GetPrivateProfileIntA("perftags", "AutoDelete", TRUE, "\\msxmldbg.ini");
    g_fSimpleDump = GetPrivateProfileIntA("perftags", "SimpleDump", TRUE, "\\msxmldbg.ini");
    if (g_fAutoDump)
        g_fEnabled = TRUE;
    if (g_fAutoOpen)
        PerfTags();
}

#ifdef BUILD_XMLDBG_AS_LIB
extern "C" 
#endif
void
PerfProcessDetach()
{
    if (g_fAutoDump)
    {
        if (g_fAutoDelete)
        {
            char achLog[MAX_PATH];
            DeleteFileA(GetPerfLogPath(achLog));
        }

        PerfDump();
    }
    if (ghWnd)
        SendMessageA(ghWnd, WM_CLOSE, 0, 0);
    if (ghThread)
        WaitForSingleObject(ghThread, 5000);
    DeleteCriticalSection(&g_csLog);
}
