//+------------------------------------------------------------------------
//
//  Microsoft Windows
// Copyright (c) 1992 - 1999 Microsoft Corporation. All rights reserved.*///
//  File:       meter.cxx
//
//  Contents:   Performance metering
//
//-------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#ifndef X_RESOURCE_H_
#define X_RESOURCE_H_
#include "resource.h"
#endif

#ifndef X_STDDEF_H_
#define X_STDDEF_H_
#include <stddef.h>
#endif

#undef MtRegister
#undef MtAdd
#undef MtSet
#undef MtOpenMonitor
#undef MtLogClear
#undef MtLogDump

void __cdecl hprintf(HANDLE hfile, char * pchFmt, ...);

CRITICAL_SECTION    g_csMeter;
static BOOL         g_fBringToFront = FALSE;
static HWND         ghWnd           = NULL;
static HBRUSH       ghbrBkgnd       = NULL;
static HANDLE       ghThread        = NULL;
static BOOL         fDlgUp          = FALSE;
static DWORD        rgbWindowColor  = 0xFF000000;    // variables for the current
static DWORD        rgbHiliteColor  = 0xFF000000;    // system color settings.
static DWORD        rgbWindowText   = 0xFF000000;    // on a WM_SYSCOLORCHANGE
static DWORD        rgbHiliteText   = 0xFF000000;    // we check to see if we need
static DWORD        rgbGrayText     = 0xFF000000;    // to reload our bitmap.
static DWORD        rgbDDWindow     = 0xFF000000;    //
static DWORD        rgbDDHilite     = 0xFF000000;    // 0xFF000000 is an invalid RGB
static BOOL         g_fEnabled      = FALSE;
static BOOL         g_fAutoDump     = FALSE;
static BOOL         g_fAutoOpen     = FALSE;
static BOOL         g_fAutoDelete   = TRUE;
static BOOL         g_fSimpleDump   = TRUE;
static DWORD        g_dwItemCount   = 0;
static DWORD        g_cRefresh      = 0;
static BOOL         g_fLogging      = FALSE;
static LONG         g_lMtAddTrap    = 0;

struct MTAG
{
    MTAG *  pmtagNext;
    MTAG *  pmtagParent;
    char *  pchTag;
    char *  pchOwner;
    char *  pchDesc;
    LONG    lCntExc;
    LONG    lValExc;
    LONG    lCntInc;
    LONG    lValInc;
    DWORD   dwFlags;
    char    ach[1];
};

struct LENT
{
    FILETIME    ft;
    LONG        lCnt;
    LONG        lVal;
    MTAG *      pmtag;
};

#define LBLKENT     8192

struct LBLK
{
    LBLK *  plblkNext;
    LONG    cEnt;
    LENT    rglent[LBLKENT];
};

#define MTF_HASCHILDREN         0x00000001
#define MTF_VALUESDIRTY         0x00000002
#define MTF_COLLAPSED           0x00000004
#define MTF_BREAKPOINTEXCL      0x00000010
#define MTF_BREAKPOINTINCL      0x00000020

#define MTAG_MAXDESCSIZE        40

MTAG *  g_pmtagHead;
LBLK *  g_plblkHead;
LBLK *  g_plblkTail;
LONG    g_cLogEntry;
LONG    g_cMTagByName;
MTAG ** g_aMTagByName;
LONG    g_cMTagOrphanByParent;
MTAG ** g_aMTagOrphanByParent;

int
MTagCompareName(void * pv1, void *pv2)
{
    return(lstrcmpA(((MTAG *)pv1)->pchTag, (char *)pv2));
}

int
MTagCompareOwner(void * pv1, void * pv2)
{
    return(lstrcmpA(((MTAG *)pv1)->pchOwner, (char *)pv2));
}

BOOL
FindPtrArray(LONG * pcCount, void * pAry, int (*pfnCmp)(void *, void *), void * pvArg, int * piLocFind)
{
    int     iEntLow, iEntHigh, iEntMid, c;
	void **	ppvAry = *(void ***)pAry;
    void *  pvMid;
    BOOL    fResult;

    iEntLow  = 0;
    fResult  = FALSE;
    iEntHigh = *pcCount - 1;

    while (iEntLow <= iEntHigh)
    {
        iEntMid = (iEntLow + iEntHigh) >> 1;
        pvMid  = ppvAry[iEntMid];
        c = pfnCmp(pvMid, pvArg);
        if (c == 0)
        {
            iEntLow = iEntMid;
            fResult = TRUE;
            break;
        }
        else if (c < 0)
            iEntLow = iEntMid + 1;
        else
            iEntHigh = iEntMid - 1;
    }

    *piLocFind = iEntLow;
    return(fResult);
}

void
InsPtrArray(LONG * pcCount, void * ppvAry, int iIns, void * pvIns)
{
    if ((*pcCount & 255) == 0 && (*pcCount || *(void **)ppvAry == NULL))
    {
        void * pvNew = (void *)LocalAlloc(LMEM_FIXED, (*pcCount + 256) * sizeof(void *));

        if (pvNew == NULL)
            return;

        memcpy(pvNew, *(void **)ppvAry, *pcCount * sizeof(void *));

		if (*(void **)ppvAry)
			LocalFree(*(void **)ppvAry);

        *(void **)ppvAry = pvNew;
    }

    void ** ppvIns = *(void ***)ppvAry + iIns;

    memmove(ppvIns + 1, ppvIns, (*pcCount - iIns) * sizeof(void *));
    *ppvIns = pvIns;
    *pcCount += 1;
}

void
DelPtrArray(LONG * pcCount, void * ppvAry, int iDel, int cDel)
{
    void ** ppvDel = *(void ***)ppvAry + iDel;
    memmove(ppvDel, ppvDel + cDel, (*pcCount - (iDel + cDel)) * sizeof(void *));
    *pcCount -= cDel;
}

BOOL
FindMTagByName(char * pchName, int * piLoc)
{
    return(FindPtrArray(&g_cMTagByName, &g_aMTagByName, MTagCompareName, pchName, piLoc));
}

void
InsMTagByName(int iLocIns, MTAG * pmtagIns)
{
    InsPtrArray(&g_cMTagByName, &g_aMTagByName, iLocIns, pmtagIns);
}

BOOL
FindMTagOrphanByParent(char * pchOwner, BOOL fFirst, int * piLoc)
{
    BOOL fFound = FindPtrArray(&g_cMTagOrphanByParent, &g_aMTagOrphanByParent, MTagCompareOwner, pchOwner, piLoc);

    if (fFound && fFirst)
    {
        while (*piLoc > 0)
        {
            MTAG * pmtag = g_aMTagOrphanByParent[*piLoc - 1];
        
            if (lstrcmpA(pchOwner, pmtag->pchOwner) != 0)
                break;

            *piLoc -= 1;
        }
    }

    return(fFound);
}

void
InsMTagOrphan(MTAG * pmtagIns)
{
    int iLocIns;
    FindMTagOrphanByParent(pmtagIns->pchOwner, FALSE, &iLocIns);
    InsPtrArray(&g_cMTagOrphanByParent, &g_aMTagOrphanByParent, iLocIns, pmtagIns);
}

void
DelMTagOrphanRange(int iLocDel, int cLocDel)
{
    DelPtrArray(&g_cMTagOrphanByParent, &g_aMTagOrphanByParent, iLocDel, cLocDel);
}

LENT *
GetLogEntry()
{
    LBLK * plblk = g_plblkTail;

    if (!plblk || plblk->cEnt == LBLKENT)
    {
        plblk = (LBLK *)LocalAlloc(LMEM_FIXED, sizeof(LBLK));

        if (plblk == NULL)
        {
            return(NULL);
        }

        plblk->plblkNext = NULL;
        plblk->cEnt      = 0;

        if (g_plblkTail)
            g_plblkTail->plblkNext = plblk;
        if (g_plblkHead == NULL)
            g_plblkHead = plblk;

        g_plblkTail = plblk;
    }

    g_cLogEntry += 1;

    return(&plblk->rglent[plblk->cEnt++]);
}

int WINAPI
MtRegister(char * szTag, char * szOwner, char * szDesc)
{
    int cbTag   = lstrlenA(szTag) + 1;
    int cbOwner = lstrlenA(szOwner) + 1;
    int cbDesc  = lstrlenA(szDesc) + 1;
    MTAG * pmtag = NULL, * pmtagT, * pmtagP;
    int iLoc, cLoc;

    EnterCriticalSection(&g_csMeter);

    if (!FindMTagByName(szTag, &iLoc))
    {
        pmtag = (MTAG *)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, offsetof(MTAG, ach) + cbTag + cbOwner + cbDesc);

        pmtag->pmtagNext    = g_pmtagHead;
        g_pmtagHead         = pmtag;
        pmtag->pchTag       = pmtag->ach;
        pmtag->pchOwner     = pmtag->ach + cbTag;
        pmtag->pchDesc      = pmtag->pchOwner + cbOwner;

        lstrcpyA(pmtag->pchTag, szTag);
        lstrcpyA(pmtag->pchOwner, szOwner);
        lstrcpyA(pmtag->pchDesc, szDesc);

        InsMTagByName(iLoc, pmtag);

        if (FindMTagByName(szOwner, &iLoc))
        {
            pmtagT = g_aMTagByName[iLoc];

            pmtag->pmtagParent = pmtagT;

            if (!(pmtagT->dwFlags & MTF_HASCHILDREN))
            {
                pmtagT->dwFlags |= MTF_HASCHILDREN|MTF_COLLAPSED;
            }
        }
        else
        {
            InsMTagOrphan(pmtag);
        }

        if (FindMTagOrphanByParent(szTag, TRUE, &iLoc))
        {
            for (cLoc = 0; iLoc < g_cMTagOrphanByParent; ++iLoc, ++cLoc)
            {
                pmtagT = g_aMTagOrphanByParent[iLoc];

                if (lstrcmpA(pmtagT->pchOwner, szTag) == 0)
                {
                    pmtagT->pmtagParent = pmtag;
                    pmtag->dwFlags |= MTF_HASCHILDREN|MTF_COLLAPSED;

                    if (pmtagT->lCntInc || pmtagT->lValInc)
                    {
                        for (pmtagP = pmtag; pmtagP; pmtagP = pmtagP->pmtagParent)
                        {
                            pmtagP->lCntInc += pmtagT->lCntInc;
                            pmtagP->lValInc += pmtagT->lValInc;
                            pmtagP->dwFlags |= MTF_VALUESDIRTY;
                        }
                    }
                }
				else
				{
					break;
				}
            }

            DelMTagOrphanRange(iLoc - cLoc, cLoc);
        }
    }
	else
	{
		pmtag = g_aMTagByName[iLoc];
	}

    if (ghWnd)
    {
        g_cRefresh = 3;
    }

    LeaveCriticalSection(&g_csMeter);

    return(PtrToUlong(pmtag));
}

void
MtAdd(ULONG_PTR mt, LONG lCnt, LONG lVal)
{
    BOOL fBreak = FALSE;

    if (    (g_lMtAddTrap > 0 && lVal > g_lMtAddTrap)
        ||  (g_lMtAddTrap < 0 && lVal < g_lMtAddTrap))
    {
        fBreak = TRUE;
    }

    if (mt)
    {
        EnterCriticalSection(&g_csMeter);

        MTAG * pmtag = (MTAG *)mt;
        LENT * plent = g_fLogging ? GetLogEntry() : NULL;

        if (plent)
        {
            FILETIME ft;

            GetSystemTimeAsFileTime(&ft);
            FileTimeToLocalFileTime(&ft, &plent->ft);

            plent->pmtag = pmtag;
            plent->lCnt  = lCnt;
            plent->lVal  = lVal;
        }

        if (pmtag->dwFlags & MTF_BREAKPOINTEXCL)
        {
            fBreak = TRUE;
        }

        pmtag->lCntExc += lCnt;
        pmtag->lValExc += lVal;

        for (; pmtag; pmtag = pmtag->pmtagParent)
        {
            if (pmtag->dwFlags & MTF_BREAKPOINTINCL)
            {
                fBreak = TRUE;
            }

            pmtag->lCntInc += lCnt;
            pmtag->lValInc += lVal;
            pmtag->dwFlags |= MTF_VALUESDIRTY;
        }

        LeaveCriticalSection(&g_csMeter);
    }

    if (fBreak && !(GetAsyncKeyState(VK_CONTROL) & 0x8000))
    {
        DebugBreak();
    }
}

void
MtSet(int mt, LONG lCnt, LONG lVal)
{
    if (mt)
    {
        EnterCriticalSection(&g_csMeter);

        MTAG * pmtag = (MTAG *)mt;

        lVal = lVal < 0 ? 0 : lVal - pmtag->lValExc;
        lCnt = lCnt < 0 ? 0 : lCnt - pmtag->lCntExc;

        MtAdd(mt, lCnt, lVal);

        LeaveCriticalSection(&g_csMeter);
    }
}

char *
MtGetName(int mt)
{
    return(mt ? ((MTAG *)mt)->pchTag : "");
}

char *
MtGetDesc(int mt)
{
    return(mt ? ((MTAG *)mt)->pchDesc : "");
}

// Meter Monitor Window -------------------------------------------------------

DWORD WINAPI    MeterMonitorThread(LPVOID lpv);

BOOL WINAPI     MeterMonDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void            SetWindowTitle(HWND hWnd, LPSTR lpcText);
static void     RefreshView(HWND hWnd);
static void     MeterListNotify(WORD wNotify, HWND  hWnd);

void            MeasureItem(HWND hwnd, LPMEASUREITEMSTRUCT pmis);
static void     DrawItem(LPDRAWITEMSTRUCT pdis);
static void     OutTextFormat(LPDRAWITEMSTRUCT pdis);
static void     SetRGBValues(void);

void
MtDoOpenMonitor(BOOL fBringToFront)
{
    if (!ghThread)
    {
        EnterCriticalSection(&g_csMeter);

        if (!ghThread)
        {
            DWORD dwThreadId;

            fDlgUp = TRUE;
            g_fBringToFront = fBringToFront;

            ghThread = CreateThread((LPSECURITY_ATTRIBUTES) NULL, 0,
                (LPTHREAD_START_ROUTINE)MeterMonitorThread,
                NULL, 0, &dwThreadId);
        }

        LeaveCriticalSection(&g_csMeter);
    }
    else if (fBringToFront && ghWnd)
    {
        ShowWindow(ghWnd, SW_SHOWNORMAL);
        BringWindowToTop(ghWnd);
    }
}

void WINAPI
MtOpenMonitor()
{
    MtDoOpenMonitor(TRUE);
}

DWORD WINAPI
MeterMonitorThread(LPVOID)
{
    ghbrBkgnd = CreateSolidBrush(GetSysColor(COLOR_BACKGROUND));

    DialogBoxA(g_hinstMain, MAKEINTRESOURCEA(IDD_PERFMETER), NULL, (DLGPROC)MeterMonDlgProc);

    DeleteObject(ghbrBkgnd);
    CloseHandle(ghThread);
    ghThread = NULL;
    ExitThread(0);
    
    return 0;
}

int __cdecl MTagCompareDesc(const void * pv1, const void * pv2)
{
    MTAG * pmt1 = *(MTAG **)pv1;
    MTAG * pmt2 = *(MTAG **)pv2;
    return(lstrcmpA(pmt1->pchDesc, pmt2->pchDesc));
}

int __cdecl MTagCompareTag(const void * pv1, const void * pv2)
{
    MTAG * pmt1 = *(MTAG **)pv1;
    MTAG * pmt2 = *(MTAG **)pv2;
    return(lstrcmpA(pmt1->pchTag, pmt2->pchTag));
}

void
PopulateList(HWND hWnd, MTAG * pmtagPar)
{
    MTAG * apmtag[512], *pmtag, ** ppmtag;
    UINT   cpmtag = 0;

    for (pmtag = g_pmtagHead; pmtag; pmtag = pmtag->pmtagNext)
    {
        if (    pmtag->pmtagParent == pmtagPar
            &&  cpmtag < ARRAY_SIZE(apmtag))
        {
            apmtag[cpmtag++] = pmtag;
        }
    }

    if (cpmtag > 0)
    {
        qsort(apmtag, cpmtag, sizeof(MTAG *), MTagCompareDesc);

        for (ppmtag = apmtag; cpmtag > 0; --cpmtag, ++ppmtag)
        {
            g_dwItemCount += 1;
            SendDlgItemMessageA(hWnd, IDC_TAGLIST, LB_ADDSTRING, 0, (LPARAM)*ppmtag);

            if (g_dwItemCount != (DWORD)SendDlgItemMessageA(hWnd, IDC_TAGLIST, LB_GETCOUNT, 0, 0))
            {
                g_dwItemCount -= 1;
            }

            if (((*ppmtag)->dwFlags & (MTF_HASCHILDREN|MTF_COLLAPSED)) == MTF_HASCHILDREN)
            {
                g_dwItemCount += 1;
                SendDlgItemMessageA(hWnd, IDC_TAGLIST, LB_ADDSTRING, 0, (LPARAM)*ppmtag | 1L);
                PopulateList(hWnd, *ppmtag);
            }
        }
    }
}

void
DumpTag(HANDLE hfile, MTAG * pmtag, BOOL fExcl)
{
    LONG lCnt = fExcl ? pmtag->lCntExc : pmtag->lCntInc;
    LONG lVal = fExcl ? pmtag->lValExc : pmtag->lValInc;

    hprintf(hfile, "%s%s\t%s\t\"%s%s\"\t%ld\t%ld\r\n",
        pmtag->pchTag, fExcl ? "_excl" : "",
        fExcl ? pmtag->pchTag : pmtag->pchOwner,
        pmtag->pchDesc, fExcl ? " (Excl)" : "",
        lCnt, lVal);
}

void
DumpList(HANDLE hfile, MTAG * pmtagPar)
{
    MTAG *  apmtag[512], *pmtag, ** ppmtag;
    UINT    cpmtag = 0;

    for (pmtag = g_pmtagHead; pmtag; pmtag = pmtag->pmtagNext)
    {
        if (    pmtag->pmtagParent == pmtagPar
            &&  cpmtag < ARRAY_SIZE(apmtag))
        {
            apmtag[cpmtag++] = pmtag;
        }
    }

    if (cpmtag > 0)
    {
        qsort(apmtag, cpmtag, sizeof(MTAG *), MTagCompareTag);

        for (ppmtag = apmtag; cpmtag > 0; --cpmtag, ++ppmtag)
        {
            if ((*ppmtag)->lCntInc == 0 && (*ppmtag)->lValInc == 0)
                continue;

            DumpTag(hfile, *ppmtag, FALSE);

            if ((*ppmtag)->dwFlags & MTF_HASCHILDREN)
            {
                if ((*ppmtag)->lCntExc || (*ppmtag)->lValExc)
                {
                    DumpTag(hfile, *ppmtag, TRUE);
                }

                DumpList(hfile, *ppmtag);
            }
        }
    }
}

void WINAPI
MtLogClear()
{
    EnterCriticalSection(&g_csMeter);

    LBLK *  plblk = g_plblkHead;
    LBLK *  plblkNext;

    for (; plblk; plblk = plblkNext)
    {
        plblkNext = plblk->plblkNext;
        LocalFree(plblk);
    }

    g_plblkHead = NULL;
    g_plblkTail = NULL;
    g_cLogEntry = 0;

    LeaveCriticalSection(&g_csMeter);
}

void
DumpLog(HANDLE hfile)
{
    LBLK *  plblk = g_plblkHead;
    LENT *  plent;
    LONG    cEnt;
    LONG    lValSum = 0;

    for (; plblk; plblk = plblk->plblkNext)
    {
        for (cEnt = plblk->cEnt, plent = plblk->rglent; cEnt > 0; --cEnt, ++plent)
        {
            MTAG *      pmtag = plent->pmtag;
            SYSTEMTIME  st;

            FileTimeToSystemTime(&plent->ft, &st);

            lValSum += plent->lVal;

            hprintf(hfile, "%02d:%02d:%02d.%03d\t%s\t\"%s\"\t%ld\t%ld\t%ld\r\n",
                st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
                pmtag->pchTag, pmtag->pchDesc, plent->lCnt, plent->lVal, lValSum);
        }
    }
}

void WINAPI
MtLogDump(LPSTR pchFile)
{
    FILETIME ft1, ft2;
    SYSTEMTIME st;

    EnterCriticalSection(&g_csMeter);

    HANDLE hfile = INVALID_HANDLE_VALUE;

    hfile = CreateFileA(pchFile, GENERIC_WRITE,
                FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hfile == INVALID_HANDLE_VALUE)
        goto ret;

    SetFilePointer(hfile, 0, NULL, FILE_BEGIN);
    SetEndOfFile(hfile);
    GetSystemTimeAsFileTime(&ft1);
    FileTimeToLocalFileTime(&ft1, &ft2);
    FileTimeToSystemTime(&ft2, &st);
    hprintf(hfile, "PerfMeters Snapshot (%02d/%02d/%02d %02d:%02d:%02d)\r\n\r\n",
        st.wMonth, st.wDay, st.wYear % 100, st.wHour, st.wMinute, st.wSecond);
    DumpList(hfile, NULL);
    hprintf(hfile, "\r\n\r\nPerfMeters Log\r\n\r\n");
    DumpLog(hfile);

ret:
    if (hfile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hfile);
    }

    LeaveCriticalSection(&g_csMeter);
}

static void
RefreshView(HWND hWnd)
{
    EnterCriticalSection(&g_csMeter);
    SendDlgItemMessageA(hWnd, IDC_TAGLIST, LB_RESETCONTENT, 0, 0L);
    g_dwItemCount = 0;
    PopulateList(hWnd, NULL);
    LeaveCriticalSection(&g_csMeter);
}

static
void TagListNotify(WORD wNotify, HWND hWnd)
{
    LONG_PTR    idx;
    MTAG *  pmtag;
    BOOL    fExcl;
    RECT    rc;
    
    if (wNotify == LBN_DBLCLK)
    {
        idx = (LONG_PTR)SendMessageA(hWnd, LB_GETCURSEL, 0, (LPARAM)0);
        if (idx == CB_ERR)
            return;

        pmtag = (MTAG *)SendMessageA(hWnd, LB_GETITEMDATA, (WPARAM)idx, 0L);
        if (!pmtag || (LONG_PTR)pmtag == CB_ERR)
            return;
        
        fExcl = !!(((DWORD_PTR)pmtag) & 1L);
        pmtag = (MTAG *)((DWORD_PTR)pmtag & ~1L);

        if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
        {
            pmtag->dwFlags ^= (fExcl ? MTF_BREAKPOINTEXCL : MTF_BREAKPOINTINCL);
            
            if (SendMessageA(hWnd, LB_GETITEMRECT, (WPARAM)idx, (LPARAM)&rc) != LB_ERR)
                RedrawWindow(hWnd, &rc, NULL, RDW_INVALIDATE);
        }
        else if (!fExcl && (pmtag->dwFlags & MTF_HASCHILDREN))
        {
            LONG_PTR idxTop = (LONG_PTR)SendMessageA(hWnd, LB_GETTOPINDEX, 0, (LPARAM)0);
            pmtag->dwFlags ^= MTF_COLLAPSED;
            RefreshView(ghWnd);
            SendMessageA(hWnd, LB_SETTOPINDEX, (WPARAM)idxTop, 0);
        }
    }
}

static
void MtMeterClearChildren(MTAG * pmtag)
{
    for (MTAG * pmtagT = g_pmtagHead; pmtagT; pmtagT = pmtagT->pmtagNext)
    {
        if (pmtagT->pmtagParent == pmtag)
        {
            DWORD dwFlags = pmtagT->dwFlags & (MTF_BREAKPOINTINCL | MTF_BREAKPOINTEXCL);
            pmtagT->dwFlags &= ~(MTF_BREAKPOINTINCL | MTF_BREAKPOINTEXCL);
			// BUGBUG WIN64 - THIS IS PROBABLY WRONG AND NEEDS TO BE LOOKED AT!!!
            MtAdd((ULONG_PTR)pmtagT, -pmtagT->lCntExc, -pmtagT->lValExc);
            pmtagT->dwFlags |= dwFlags;

            if (pmtagT->lCntInc || pmtag->lValInc)
            {
                MtMeterClearChildren(pmtagT);
            }
        }
    }
}

static
void MtMeterClear()
{
	HWND	hwndList = GetDlgItem(ghWnd, IDC_TAGLIST);
	LONG_PTR   idx;
    MTAG *  pmtag;
    
    idx = (LONG_PTR)SendMessageA(hwndList, LB_GETCURSEL, 0, (LPARAM)0);
    if (idx == CB_ERR)
        return;

    pmtag = (MTAG *)SendMessageA(hwndList, LB_GETITEMDATA, (WPARAM)idx, 0L);
    if (!pmtag || pmtag == (MTAG *)CB_ERR)
        return;

    pmtag = (MTAG *)((DWORD_PTR)pmtag & ~1L);

    DWORD dwFlags = pmtag->dwFlags & (MTF_BREAKPOINTINCL | MTF_BREAKPOINTEXCL);
    pmtag->dwFlags &= ~(MTF_BREAKPOINTINCL | MTF_BREAKPOINTEXCL);
    MtAdd((ULONG_PTR)pmtag, -pmtag->lCntExc, -pmtag->lValExc);
    pmtag->dwFlags |= dwFlags;

    if (pmtag->lCntInc || pmtag->lValInc)
    {
        MtMeterClearChildren(pmtag);
    }
}

static
void OnTimer(HWND hwnd)
{
    EnterCriticalSection(&g_csMeter);

    HWND hwndList = GetDlgItem(hwnd, IDC_TAGLIST);
    LRESULT idx = SendMessageA(hwndList, LB_GETTOPINDEX, 0, 0);
    RECT rc;
    MTAG * pmtag;

    if (g_cRefresh > 0 && --g_cRefresh == 0)
    {
        RefreshView(hwnd);
    }
    else
    {
        while (SendMessageA(hwndList, LB_GETITEMRECT, (WPARAM)idx, (LPARAM)&rc) != LB_ERR)
        {
            pmtag = (MTAG *)SendMessageA(hwndList, LB_GETITEMDATA, (WPARAM)idx, 0L);
            if ((LONG_PTR)pmtag && (LONG_PTR)pmtag != CB_ERR)
			{
				if (pmtag->dwFlags & MTF_VALUESDIRTY)
				{
					pmtag->dwFlags &= ~MTF_VALUESDIRTY;
					RedrawWindow(hwndList, &rc, NULL, RDW_INVALIDATE);
				}
			}
            idx += 1;

            if (idx > 50)
            {
                break;
            }
        }
    }

    LeaveCriticalSection(&g_csMeter);
}

BOOL WINAPI
MeterMonDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LONG    idx = 0;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            ghWnd = hWnd;
			RefreshView(ghWnd);
            SetRGBValues();
            SetWindowTitle(hWnd, "PerfMeters - %s");
            SetWindowTitle(GetDlgItem(hWnd, IDC_STARTSTOP), g_fLogging ? "&Stop Log" : "&Start Log");
            ShowWindow(hWnd, g_fBringToFront ? SW_SHOWNORMAL : SW_SHOWMINNOACTIVE);
            SetTimer(hWnd, 1001, 1000, NULL);
            break;

        case WM_SYSCOLORCHANGE:
            SetRGBValues();
            g_cRefresh = 3;
            break;
            
        case WM_MEASUREITEM:
            MeasureItem(hWnd, (LPMEASUREITEMSTRUCT)lParam);
            break;
            
        case WM_DRAWITEM:
            DrawItem((LPDRAWITEMSTRUCT)lParam);
            break;
            
        case WM_PAINT:
            return FALSE;

        case WM_ERASEBKGND:
            return IsIconic(hWnd);

        case WM_QUERYENDSESSION:
            PostMessageA(hWnd, WM_CLOSE, 0, 0);
            break;

        case WM_CLOSE:
            fDlgUp = FALSE;
            KillTimer(ghWnd, 1001);
            return EndDialog(hWnd, 0);

        case WM_TIMER:
            OnTimer(ghWnd);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {                       
                case IDC_TAGLIST:
                    TagListNotify(HIWORD(wParam), (HWND)lParam);
                    break;

                case IDC_CLEARLOG:
                    MtLogClear();
                    break;

                case IDC_STARTSTOP:
                    SetWindowTitle((HWND)lParam, g_fLogging ? "&Start Log" : "&Stop Log");
                    g_fLogging = !g_fLogging;
                    break;

                case IDC_REFRESH:
                    g_cRefresh = 3;
                    break;

                case IDC_DUMPLOG:
                    MtLogDump("\\perfmetr.log");
                    break;

                case IDC_CLEARMETER:
                    MtMeterClear();
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

static char achSpaces[] = "                                                                ";

char *
GetSpaces(int n)
{
    return(&achSpaces[sizeof(achSpaces) - n]);
}

COLORREF
BkColorFromLevel(int cLevel, HDC hdc)
{
    if (GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE)
    {
        static COLORREF rgcr_pal[] = {
            0x20FFFFFF,
            0x2000FFFF,
            0x20F0CAA6,
            0x2000FF00,
            0x20C0C0C0
        };

        cLevel /= 2;
        cLevel %= 5;

        return(rgcr_pal[cLevel]);
    }
    else
    {
        static COLORREF rgcr_nopal[] = {
            0x20FFFFFF,
            0x20FFE0E0,
            0x20E0FFFF,
            0x20FFE0FF,
            0x20E0FFE0,
            0x20E0E0FF,
            0x20FFFFE0,
            0x20FFFF40,
            0x20E0E0E0
        };

        cLevel /= 2;
        cLevel %= 9;

        return(rgcr_nopal[cLevel]);
    }
}

char * CommaIze(LONG lVal, char * pszBuf)
{
    char ach[32];
    char * pszSrc = ach;
    char * pszDst = pszBuf;
    int c;

    wsprintfA(ach, "%ld", lVal);

    if (*pszSrc == '-')
    {
        *pszDst++ = *pszSrc++;
    }

    c = lstrlenA(pszSrc);

    while (*pszSrc)
    {
        *pszDst++ = *pszSrc++;
        c--;

        if (c && (c % 3) == 0)
        {
            *pszDst++ = ',';
        }
    }

    *pszDst = 0;

    return(pszBuf);
}

void DrawItem(LPDRAWITEMSTRUCT pdis)
{
    char ach[256], * pch = ach;
    char achDesc[256];
    int cchDesc;
    COLORREF crText = 0, crBack = 0;
    int cLevel = 0;

    if((int)pdis->itemID < 0)
        return;

    if((ODA_DRAWENTIRE | ODA_SELECT) & pdis->itemAction)
    {
        MTAG * pmtag = (MTAG *)(pdis->itemData & ~1L);
        BOOL fChildren  = !!(pmtag->dwFlags & MTF_HASCHILDREN);
        BOOL fForceExcl = !!(pdis->itemData & 1L);

        for (MTAG * pmtagT = pmtag->pmtagParent; pmtagT; pmtagT = pmtagT->pmtagParent)
        {
            cLevel += 2;
        }

        if (fForceExcl)
        {
            cLevel += 2;
        }

        cchDesc = lstrlenA(pmtag->pchDesc);

        if (cchDesc > sizeof(achDesc) - 1 - sizeof(" (Excl)"))
            cchDesc = sizeof(achDesc) - 1 - sizeof(" (Excl)");
        memcpy(achDesc, pmtag->pchDesc, cchDesc);

        if (fForceExcl)
            memcpy(achDesc + cchDesc, " (Excl)", 8);
        else
            achDesc[cchDesc] = 0;

        wsprintfA(ach, "%c %s%c%-55s",
            (pmtag->dwFlags & (fForceExcl ? MTF_BREAKPOINTEXCL : MTF_BREAKPOINTINCL)) ? 'B' : ' ',
            GetSpaces(cLevel),
            (fForceExcl || !fChildren) ? ' ' : (pmtag->dwFlags & MTF_COLLAPSED) ? '+' : '-',
            achDesc);

        LONG lCnt = (fForceExcl || !fChildren) ? pmtag->lCntExc : pmtag->lCntInc;
        LONG lVal = (fForceExcl || !fChildren) ? pmtag->lValExc : pmtag->lValInc;

        char achBuf1[32];
        char achBuf2[32];
        char achBuf3[32];

        wsprintfA(&ach[56], " %11s [%6s] %11s", CommaIze(lCnt, achBuf1),
            CommaIze(lCnt == 0 ? 0 : lVal / lCnt, achBuf2), CommaIze(lVal, achBuf3));

        if (pdis->itemState & ODS_SELECTED)
        {
            crText = SetTextColor(pdis->hDC, rgbHiliteText);
            crBack = SetBkColor(pdis->hDC, rgbHiliteColor);
            ExtTextOutA(pdis->hDC, pdis->rcItem.left, pdis->rcItem.top + 1,
                ETO_CLIPPED|ETO_OPAQUE, &pdis->rcItem, ach, lstrlenA(ach), NULL);
            SetTextColor(pdis->hDC, crText);
            SetBkColor(pdis->hDC, crBack);
        }
        else
        {
            crBack = SetBkColor(pdis->hDC, BkColorFromLevel(cLevel, pdis->hDC));
            ExtTextOutA(pdis->hDC, pdis->rcItem.left, pdis->rcItem.top + 1,
                ETO_CLIPPED|ETO_OPAQUE, &pdis->rcItem, ach, lstrlenA(ach), NULL);
            SetBkColor(pdis->hDC, crBack);
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
MeterProcessAttach()
{
    InitializeCriticalSection(&g_csMeter);

#if defined(RETAILBUILD) && defined(PERFMETER)
    g_fAutoOpen = TRUE;
#endif

    g_fAutoOpen = GetPrivateProfileIntA("perfmeter", "AutoOpen", g_fAutoOpen, "\\msxmldbg.ini");
    g_fLogging = GetPrivateProfileIntA("perfmeter", "AutoLog", FALSE, "\\msxmldbg.ini");
    g_fAutoDump = GetPrivateProfileIntA("perfmeter", "AutoDump", FALSE, "\\msxmldbg.ini");

    if (g_fAutoOpen)
    {
        MtDoOpenMonitor(FALSE);
    }
}
#ifdef BUILD_XMLDBG_AS_LIB
extern "C" 
#endif
void 
MeterProcessDetach()
{
    if (g_fAutoDump)
        MtLogDump("\\perfmetr.log");
    if (ghWnd)
        SendMessageA(ghWnd, WM_CLOSE, 0, 0);
    if (ghThread)
        WaitForSingleObject(ghThread, 5000);
    DeleteCriticalSection(&g_csMeter);
}
