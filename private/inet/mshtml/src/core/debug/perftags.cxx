//+------------------------------------------------------------------------
//
//  File:       perftags.cxx
//
//  Contents:   Utilities for measuring perf
//
//-------------------------------------------------------------------------

#include "headers.hxx"

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

PERFTAG WINAPI
DbgExPerfRegister(char * szTag, char * szOwner, char * szDesc)
{
    EnterCriticalSection(&g_csLog);

    int cbTag   = lstrlenA(szTag) + 1;
    int cbOwner = lstrlenA(szOwner) + 1;
    int cbDesc  = lstrlenA(szDesc) + 1;
    LTAG * ptag  = (LTAG *)MemAlloc(offsetof(LTAG, ach) + cbTag + cbOwner + cbDesc);
    char * pch  = ptag->ach;

    ptag->ptagNext = g_ptagHead;
    ptag->fEnabled = (BYTE)!!GetPrivateProfileIntA("perftags", szTag, FALSE, "mshtmdbg.ini");

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

    return((PERFTAG)ptag);
}

void WINAPI
DbgExPerfLogFnList(PERFTAG tag, void * pvObj, const char * pchFmt, va_list va)
{
    LOGENTRY    le;
    UINT        cb;

    if (!tag || !((LTAG *)tag)->fEnabled || !g_fEnabled)
        return;

    le.pleNext = NULL;
    le.pvObject = pvObj;
    le.dwThread = GetCurrentThreadId();

    cb = wvsprintfA(le.ach, pchFmt, va);
 
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

void __cdecl
DbgExPerfLogFn(PERFTAG tag, void * pvObj, const char * pchFmt, ...)
{
    va_list va;
    va_start(va, pchFmt);
    DbgExPerfLogFnList(tag, pvObj, pchFmt, va);
    va_end(va);
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

static void PerfLogDumpRange(HANDLE hfile, LOGENTRY ** pple, UINT cle)
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

void WINAPI
DbgExPerfDump()
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

                PerfLogDumpRange(hfile, ppleBeg, pple - ppleBeg);
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
                PerfLogDumpRange(hfile, ppleBeg, pple - ppleBeg);
            }

            i -= 1;
            pple -= 1;
        }
    }

    hprintf(hfile, "------------------------------------------------------------------------------\r\n\r\n");

    CloseHandle(hfile);

    MemFree(rgple);

    DbgExPerfClear();

ret:
    LeaveCriticalSection(&g_csLog);
}

void WINAPI
DbgExPerfClear()
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

void WINAPI
DbgExPerfTags()
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

    DialogBoxA(g_hinstMain, MAKEINTRESOURCEA(IDD_PERFTAGS), NULL, PerfMonDlgProc);

    DeleteObject(ghbrBkgnd);
	ghThread = NULL;
    
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
    LONG    idx;
    LTAG *   ptag;
    RECT    rc;
    
    if (wNotify == LBN_DBLCLK)
    {
        idx = SendMessageA(hWnd, LB_GETCURSEL, 0, 0L);
        if (idx == CB_ERR)
            return;

        ptag = (LTAG *)SendMessageA(hWnd, LB_GETITEMDATA, (WPARAM)idx, 0L);
        if (ptag == (LTAG *)CB_ERR)
            return;
        
        ptag->fEnabled = !ptag->fEnabled;

        WritePrivateProfileStringA("perftags", ptag->pchTag,
            ptag->fEnabled ? "1" : "0", "\\mshtmdbg.ini");

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
                                ptag->fEnabled ? "1" : "0", "\\mshtmdbg.ini");
                        }
                    }

                    RefreshView(ghWnd);
                    break;
                }

                case IDC_CLEARLOG:
                    DbgExPerfClear();
                    break;

                case IDC_DUMPLOG:
                    DbgExPerfDump();
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

void
PerfProcessAttach()
{
    InitializeCriticalSection(&g_csLog);
    g_fAutoDump   = GetPrivateProfileIntA("perftags", "AutoDump", FALSE, "mshtmdbg.ini");
    g_fAutoOpen   = GetPrivateProfileIntA("perftags", "AutoOpen", FALSE, "mshtmdbg.ini");
    g_fAutoDelete = GetPrivateProfileIntA("perftags", "AutoDelete", TRUE, "mshtmdbg.ini");
    g_fSimpleDump = GetPrivateProfileIntA("perftags", "SimpleDump", TRUE, "mshtmdbg.ini");
    if (g_fAutoDump)
        g_fEnabled = TRUE;
    if (g_fAutoOpen)
        DbgExPerfTags();
}

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

        DbgExPerfDump();
    }
    if (ghWnd)
        SendMessageA(ghWnd, WM_CLOSE, 0, 0);
    DWORD dwTickEnd = GetTickCount() + 5000;
    while (ghThread && GetTickCount() < dwTickEnd)
		Sleep(50);

    DbgExPerfClear();
    
    for (LTAG * ptag = g_ptagHead; ptag; ptag = g_ptagHead)
    {
        g_ptagHead = ptag->ptagNext;
        MemFree(ptag);
    }

    DeleteCriticalSection(&g_csLog);
}

char * WINAPI
DbgExDecodeMessage(UINT msg)
{
    static char szBuf[12];

    *szBuf = '\0';
    
    switch (msg)
    {
        case WM_NULL:               return(" WM_NULL");
        case WM_CREATE:             return(" WM_CREATE");
        case WM_DESTROY:            return(" WM_DESTROY");
        case WM_MOVE:               return(" WM_MOVE");
        case WM_SIZE:               return(" WM_SIZE");
        case WM_ACTIVATE:           return(" WM_ACTIVATE");
        case WM_SETFOCUS:           return(" WM_SETFOCUS");
        case WM_KILLFOCUS:          return(" WM_KILLFOCUS");
        case WM_ENABLE:             return(" WM_ENABLE");
        case WM_SETREDRAW:          return(" WM_SETREDRAW");
        case WM_SETTEXT:            return(" WM_SETTEXT");
        case WM_GETTEXT:            return(" WM_GETTEXT");
        case WM_GETTEXTLENGTH:      return(" WM_GETTEXTLENGTH");
        case WM_PAINT:              return(" WM_PAINT");
        case WM_CLOSE:              return(" WM_CLOSE");
        case WM_QUERYENDSESSION:    return(" WM_QUERYENDSESSION");
        case WM_QUERYOPEN:          return(" WM_QUERYOPEN");
        case WM_ENDSESSION:         return(" WM_ENDSESSION");
        case WM_QUIT:               return(" WM_QUIT");
        case WM_ERASEBKGND:         return(" WM_ERASEBKGND");
        case WM_SYSCOLORCHANGE:     return(" WM_SYSCOLORCHANGE");
        case WM_SHOWWINDOW:         return(" WM_SHOWWINDOW");
        case WM_WININICHANGE:       return(" WM_WININICHANGE");
        case WM_DEVMODECHANGE:      return(" WM_DEVMODECHANGE");
        case WM_ACTIVATEAPP:        return(" WM_ACTIVATEAPP");
        case WM_FONTCHANGE:         return(" WM_FONTCHANGE");
        case WM_TIMECHANGE:         return(" WM_TIMECHANGE");
        case WM_CANCELMODE:         return(" WM_CANCELMODE");
        case WM_SETCURSOR:          return(" WM_SETCURSOR");
        case WM_MOUSEACTIVATE:      return(" WM_MOUSEACTIVATE");
        case WM_CHILDACTIVATE:      return(" WM_CHILDACTIVATE");
        case WM_QUEUESYNC:          return(" WM_QUEUESYNC");
        case WM_GETMINMAXINFO:      return(" WM_GETMINMAXINFO");
        case WM_PAINTICON:          return(" WM_PAINTICON");
        case WM_ICONERASEBKGND:     return(" WM_ICONERASEBKGND");
        case WM_NEXTDLGCTL:         return(" WM_NEXTDLGCTL");
        case WM_SPOOLERSTATUS:      return(" WM_SPOOLERSTATUS");
        case WM_DRAWITEM:           return(" WM_DRAWITEM");
        case WM_MEASUREITEM:        return(" WM_MEASUREITEM");
        case WM_DELETEITEM:         return(" WM_DELETEITEM");
        case WM_VKEYTOITEM:         return(" WM_VKEYTOITEM");
        case WM_CHARTOITEM:         return(" WM_CHARTOITEM");
        case WM_SETFONT:            return(" WM_SETFONT");
        case WM_GETFONT:            return(" WM_GETFONT");
        case WM_SETHOTKEY:          return(" WM_SETHOTKEY");
        case WM_GETHOTKEY:          return(" WM_GETHOTKEY");
        case WM_QUERYDRAGICON:      return(" WM_QUERYDRAGICON");
        case WM_COMPAREITEM:        return(" WM_COMPAREITEM");
        case WM_COMPACTING:         return(" WM_COMPACTING");
        case WM_COMMNOTIFY:         return(" WM_COMMNOTIFY");
        case WM_WINDOWPOSCHANGING:  return(" WM_WINDOWPOSCHANGING");
        case WM_WINDOWPOSCHANGED:   return(" WM_WINDOWPOSCHANGED");
        case WM_POWER:              return(" WM_POWER");
        case WM_COPYDATA:           return(" WM_COPYDATA");
        case WM_CANCELJOURNAL:      return(" WM_CANCELJOURNAL");
        case WM_NOTIFY:             return(" WM_NOTIFY");
        case WM_INPUTLANGCHANGEREQUEST: return(" WM_INPUTLANGCHANGEREQUEST");
        case WM_INPUTLANGCHANGE:    return(" WM_INPUTLANGCHANGE");
        case WM_TCARD:              return(" WM_TCARD");
        case WM_HELP:               return(" WM_HELP");
        case WM_USERCHANGED:        return(" WM_USERCHANGED");
        case WM_NOTIFYFORMAT:       return(" WM_NOTIFYFORMAT");
        case WM_CONTEXTMENU:        return(" WM_CONTEXTMENU");
        case WM_STYLECHANGING:      return(" WM_STYLECHANGING");
        case WM_STYLECHANGED:       return(" WM_STYLECHANGED");
        case WM_DISPLAYCHANGE:      return(" WM_DISPLAYCHANGE");
        case WM_GETICON:            return(" WM_GETICON");
        case WM_SETICON:            return(" WM_SETICON");
        case WM_NCCREATE:           return(" WM_NCCREATE");
        case WM_NCDESTROY:          return(" WM_NCDESTROY");
        case WM_NCCALCSIZE:         return(" WM_NCCALCSIZE");
        case WM_NCHITTEST:          return(" WM_NCHITTEST");
        case WM_NCPAINT:            return(" WM_NCPAINT");
        case WM_NCACTIVATE:         return(" WM_NCACTIVATE");
        case WM_GETDLGCODE:         return(" WM_GETDLGCODE");
        case WM_SYNCPAINT:          return(" WM_SYNCPAINT");
        case WM_NCMOUSEMOVE:        return(" WM_NCMOUSEMOVE");
        case WM_NCLBUTTONDOWN:      return(" WM_NCLBUTTONDOWN");
        case WM_NCLBUTTONUP:        return(" WM_NCLBUTTONUP");
        case WM_NCLBUTTONDBLCLK:    return(" WM_NCLBUTTONDBLCLK");
        case WM_NCRBUTTONDOWN:      return(" WM_NCRBUTTONDOWN");
        case WM_NCRBUTTONUP:        return(" WM_NCRBUTTONUP");
        case WM_NCRBUTTONDBLCLK:    return(" WM_NCRBUTTONDBLCLK");
        case WM_NCMBUTTONDOWN:      return(" WM_NCMBUTTONDOWN");
        case WM_NCMBUTTONUP:        return(" WM_NCMBUTTONUP");
        case WM_NCMBUTTONDBLCLK:    return(" WM_NCMBUTTONDBLCLK");
        case WM_KEYDOWN:            return(" WM_KEYDOWN");
        case WM_KEYUP:              return(" WM_KEYUP");
        case WM_CHAR:               return(" WM_CHAR");
        case WM_DEADCHAR:           return(" WM_DEADCHAR");
        case WM_SYSKEYDOWN:         return(" WM_SYSKEYDOWN");
        case WM_SYSKEYUP:           return(" WM_SYSKEYUP");
        case WM_SYSCHAR:            return(" WM_SYSCHAR");
        case WM_SYSDEADCHAR:        return(" WM_SYSDEADCHAR");
        case WM_IME_STARTCOMPOSITION:   return(" WM_IME_STARTCOMPOSITION");
        case WM_IME_ENDCOMPOSITION: return(" WM_IME_ENDCOMPOSITION");
        case WM_IME_COMPOSITION:    return(" WM_IME_COMPOSITION");
        case WM_INITDIALOG:         return(" WM_INITDIALOG");
        case WM_COMMAND:            return(" WM_COMMAND");
        case WM_SYSCOMMAND:         return(" WM_SYSCOMMAND");
        case WM_TIMER:              return(" WM_TIMER");
        case WM_HSCROLL:            return(" WM_HSCROLL");
        case WM_VSCROLL:            return(" WM_VSCROLL");
        case WM_INITMENU:           return(" WM_INITMENU");
        case WM_INITMENUPOPUP:      return(" WM_INITMENUPOPUP");
        case WM_MENUSELECT:         return(" WM_MENUSELECT");
        case WM_MENUCHAR:           return(" WM_MENUCHAR");
        case WM_ENTERIDLE:          return(" WM_ENTERIDLE");
        case WM_CTLCOLORMSGBOX:     return(" WM_CTLCOLORMSGBOX");
        case WM_CTLCOLOREDIT:       return(" WM_CTLCOLOREDIT");
        case WM_CTLCOLORLISTBOX:    return(" WM_CTLCOLORLISTBOX");
        case WM_CTLCOLORBTN:        return(" WM_CTLCOLORBTN");
        case WM_CTLCOLORDLG:        return(" WM_CTLCOLORDLG");
        case WM_CTLCOLORSCROLLBAR:  return(" WM_CTLCOLORSCROLLBAR");
        case WM_CTLCOLORSTATIC:     return(" WM_CTLCOLORSTATIC");
        case WM_MOUSEMOVE:          return(" WM_MOUSEMOVE");
        case WM_LBUTTONDOWN:        return(" WM_LBUTTONDOWN");
        case WM_LBUTTONUP:          return(" WM_LBUTTONUP");
        case WM_LBUTTONDBLCLK:      return(" WM_LBUTTONDBLCLK");
        case WM_RBUTTONDOWN:        return(" WM_RBUTTONDOWN");
        case WM_RBUTTONUP:          return(" WM_RBUTTONUP");
        case WM_RBUTTONDBLCLK:      return(" WM_RBUTTONDBLCLK");
        case WM_MBUTTONDOWN:        return(" WM_MBUTTONDOWN");
        case WM_MBUTTONUP:          return(" WM_MBUTTONUP");
        case WM_MBUTTONDBLCLK:      return(" WM_MBUTTONDBLCLK");
        case WM_MOUSEWHEEL:         return(" WM_MOUSEWHEEL");
        case WM_PARENTNOTIFY:       return(" WM_PARENTNOTIFY");
        case WM_ENTERMENULOOP:      return(" WM_ENTERMENULOOP");
        case WM_EXITMENULOOP:       return(" WM_EXITMENULOOP");
        case WM_NEXTMENU:           return(" WM_NEXTMENU");
        case WM_SIZING:             return(" WM_SIZING");
        case WM_CAPTURECHANGED:     return(" WM_CAPTURECHANGED");
        case WM_MOVING:             return(" WM_MOVING");
        case WM_POWERBROADCAST:     return(" WM_POWERBROADCAST");
        case WM_DEVICECHANGE:       return(" WM_DEVICECHANGE");
        case WM_MDICREATE:          return(" WM_MDICREATE");
        case WM_MDIDESTROY:         return(" WM_MDIDESTROY");
        case WM_MDIACTIVATE:        return(" WM_MDIACTIVATE");
        case WM_MDIRESTORE:         return(" WM_MDIRESTORE");
        case WM_MDINEXT:            return(" WM_MDINEXT");
        case WM_MDIMAXIMIZE:        return(" WM_MDIMAXIMIZE");
        case WM_MDITILE:            return(" WM_MDITILE");
        case WM_MDICASCADE:         return(" WM_MDICASCADE");
        case WM_MDIICONARRANGE:     return(" WM_MDIICONARRANGE");
        case WM_MDIGETACTIVE:       return(" WM_MDIGETACTIVE");
        case WM_MDISETMENU:         return(" WM_MDISETMENU");
        case WM_ENTERSIZEMOVE:      return(" WM_ENTERSIZEMOVE");
        case WM_EXITSIZEMOVE:       return(" WM_EXITSIZEMOVE");
        case WM_DROPFILES:          return(" WM_DROPFILES");
        case WM_MDIREFRESHMENU:     return(" WM_MDIREFRESHMENU");
        case WM_IME_SETCONTEXT:     return(" WM_IME_SETCONTEXT");
        case WM_IME_NOTIFY:         return(" WM_IME_NOTIFY");
        case WM_IME_CONTROL:        return(" WM_IME_CONTROL");
        case WM_IME_COMPOSITIONFULL:    return(" WM_IME_COMPOSITIONFULL");
        case WM_IME_SELECT:         return(" WM_IME_SELECT");
        case WM_IME_CHAR:           return(" WM_IME_CHAR");
        case WM_IME_KEYDOWN:        return(" WM_IME_KEYDOWN");
        case WM_IME_KEYUP:          return(" WM_IME_KEYUP");
        case WM_MOUSEHOVER:         return(" WM_MOUSEHOVER");
        case WM_MOUSELEAVE:         return(" WM_MOUSELEAVE");
        case WM_CUT:                return(" WM_CUT");
        case WM_COPY:               return(" WM_COPY");
        case WM_PASTE:              return(" WM_PASTE");
        case WM_CLEAR:              return(" WM_CLEAR");
        case WM_UNDO:               return(" WM_UNDO");
        case WM_RENDERFORMAT:       return(" WM_RENDERFORMAT");
        case WM_RENDERALLFORMATS:   return(" WM_RENDERALLFORMATS");
        case WM_DESTROYCLIPBOARD:   return(" WM_DESTROYCLIPBOARD");
        case WM_DRAWCLIPBOARD:      return(" WM_DRAWCLIPBOARD");
        case WM_PAINTCLIPBOARD:     return(" WM_PAINTCLIPBOARD");
        case WM_VSCROLLCLIPBOARD:   return(" WM_VSCROLLCLIPBOARD");
        case WM_SIZECLIPBOARD:      return(" WM_SIZECLIPBOARD");
        case WM_ASKCBFORMATNAME:    return(" WM_ASKCBFORMATNAME");
        case WM_CHANGECBCHAIN:      return(" WM_CHANGECBCHAIN");
        case WM_HSCROLLCLIPBOARD:   return(" WM_HSCROLLCLIPBOARD");
        case WM_QUERYNEWPALETTE:    return(" WM_QUERYNEWPALETTE");
        case WM_PALETTEISCHANGING:  return(" WM_PALETTEISCHANGING");
        case WM_PALETTECHANGED:     return(" WM_PALETTECHANGED");
        case WM_HOTKEY:             return(" WM_HOTKEY");
        case WM_PRINT:              return(" WM_PRINT");
        case WM_PRINTCLIENT:        return(" WM_PRINTCLIENT");
        case WM_USER:               return(" WM_USER");
        case WM_USER+1:             return(" WM_USER+1");
        case WM_USER+2:             return(" WM_USER+2");
        case WM_USER+3:             return(" WM_USER+3");
        case WM_USER+4:             return(" WM_USER+4");
        default:
            wsprintfA(szBuf, "0x%x", msg);
            break;
    }

    return(szBuf);
}
