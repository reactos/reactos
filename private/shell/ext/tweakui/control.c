/*
 * control - Dialog box property sheet for "control panel"
 */

#include "tweakui.h"

/*
 * cchFriendlyMax is used to hold control panel file descriptions.
 */
#define cchFriendlyMax 256

typedef struct CPL {            /* control panel */
    TCHAR tszFile[MAX_PATH];    /* File name */
} CPL, *PCPL;

#define icplPlvi(plvi) ((UINT)(plvi)->lParam)
#define pcplIcpl(icpl) (&pcpii->pcpl[icpl])
#define pcplPlvi(plvi) pcplIcpl(icplPlvi(plvi))

typedef struct CPII {
    Declare_Gxa(CPL, cpl);
} CPII, *PCPII;

CPII cpii;
#define pcpii (&cpii)

TCHAR g_tszBoring[64];

#pragma BEGIN_CONST_DATA

const static DWORD CODESEG rgdwHelp[] = {
        0,                      0,
};

/*****************************************************************************
 *
 *  Control_VerQueryDescription
 *
 *  Suck out the file description if we have one.
 *
 *****************************************************************************/

BOOL PASCAL
Control_VerQueryDescription(LPVOID pvData, UINT uiLang, UINT uiCharSet,
                            LPTSTR ptszBuf, UINT ctch)
{
    TCHAR tsz[128];
    LPWSTR pwsz;
    BOOL fRc;
    UINT cb;

    wsprintf(tsz, TEXT("\\StringFileInfo\\%04X%04X\\")
                  TEXT("FileDescription"), uiLang, uiCharSet);
    fRc = VerQueryValue(pvData, tsz, (LPVOID)&pwsz, &cb);

    if (fRc && cb > 0) {
#ifdef UNICODE
        lstrcpyn(ptszBuf, pwsz, ctch);
#else
        WideCharToMultiByte(CP_ACP, 0, pwsz, cb / cbX(WCHAR),
                            ptszBuf, MAX_PATH, NULL, NULL);
        lstrcpyn(ptszBuf, (LPSTR)pwsz, ctch);
#endif
    }

    return fRc;
}


/*****************************************************************************
 *
 *  Control_GetFileDescription
 *
 *  Suck out the file description if we have one.
 *
 *****************************************************************************/

void PASCAL
Control_GetFileDescription(LPTSTR ptszFile, LPTSTR ptszBuf, UINT ctch)
{
    DWORD dwHandle;
    UINT cb;

    /*
     *  Assume it doesn't work.
     */
    ptszBuf[0] = TEXT('\0');

    cb = GetFileVersionInfoSize(ptszFile, &dwHandle);
    if (cb) {
        LPVOID pvData = lAlloc(cb);
        if (pvData) {
            LPWORD rgwTrans;
            UINT uiSize;

            if (GetFileVersionInfo(ptszFile, dwHandle, cb, pvData) &&
                VerQueryValue(pvData, TEXT("\\VarFileInfo\\Translation"),
                              (LPVOID)&rgwTrans, &uiSize)) {
                if (Control_VerQueryDescription(pvData,
                                                rgwTrans[0], rgwTrans[1],
                                                ptszBuf, ctch) ||
                    /*
                     *  Lots of morons forget to set the language properly,
                     *  so we will try English/USA regardless.  And if that
                     *  doesn't work, try English/NULL because some people
                     *  do that too.
                     */
                    Control_VerQueryDescription(pvData,
                                                0x0409, 0x040E,
                                                ptszBuf, ctch) ||
                    Control_VerQueryDescription(pvData,
                                                0x0409, 0x0000,
                                                ptszBuf, ctch)) {
                }
            }
            lFree(pvData);
        }
    }
}

/*****************************************************************************
 *
 *  Control_GetShowState
 *
 *  Determine whether a *.CPL file is currently shown in the
 *  Control Panel.
 *
 *****************************************************************************/

BOOL PASCAL
Control_GetShowState(LPCTSTR ptszName)
{
    TCHAR tsz[10];

    GetPrivateProfileString(c_tszDontLoad, ptszName,
                            c_tszNil, tsz, cA(tsz), c_tszControlIni);

    return tsz[0] ? FALSE : TRUE;

}

/*****************************************************************************
 *
 *  Control_SetShowState
 *
 *  Set the registry/INI flag that tells us to hide/show the *.CPL file.
 *
 *****************************************************************************/

void PASCAL
Control_SetShowState(LPCTSTR ptszName, BOOL fShow)
{
    // BUGBUG -- string.c
    WritePrivateProfileString(c_tszDontLoad, ptszName,
                              fShow ? NULL : c_tszNo, c_tszControlIni);
}

/*****************************************************************************
 *
 *  Control_AddCpl
 *
 *  Add a single *.CPL to the list.
 *
 *  The display name is the CPL filename, followed by an optional
 *  parenthesized version description string.  For control panels
 *  that ship with Windows 95 whose version strings suck, we substitute
 *  our own.
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

LPCTSTR c_rgpszCpl[] = {
    c_tszAppWizCpl,
    c_tszDeskCpl,
    c_tszIntlCpl,
    c_tszMainCpl,
    c_tszTimeDateCpl,
};

int PASCAL
Control_AddCpl(HWND hwnd, LPTSTR ptszName)
{
    PCPL pcpl = Misc_AllocPx(&pcpii->gxa);
    TCHAR tszDesc[MAX_PATH];
    TCHAR tszFull[MAX_PATH + 3 + MAX_PATH]; /* 3 = " - " */

    lstrcpy(pcpl->tszFile, ptszFilenameCqn(ptszName));
    CharLower(pcpl->tszFile);

    Control_GetFileDescription(ptszName, tszDesc, MAX_PATH);

    /*
     *  Now clean up various weird cases.
     */
    if (lstrcmpi(tszDesc, g_tszBoring) == 0) {
        int i;
        for (i = 0; i < cA(c_rgpszCpl); i++) {
            if (lstrcmpi(pcpl->tszFile, c_rgpszCpl[i]) == 0) {
                LoadString(hinstCur, IDS_CPL_ADDRM + i, tszDesc, cA(tszDesc));
                break;
            }
        }
    }

    wsprintf(tszFull, TEXT("%s - %s"), pcpl->tszFile, tszDesc);

    return LV_AddItem(hwnd, pcpii->ccpl++, tszFull, -1,
                      Control_GetShowState(pcpl->tszFile));
}

/*****************************************************************************
 *
 *  Control_InitControls2
 *
 *  Enumerate all the special DLLs in the MMCPL section and add them
 *  to the list.  Note that the buffer sizes are exactly the same sizes
 *  that SHELL32 uses, so we won't miss anything that the shell itself
 *  doesn't.
 *
 *****************************************************************************/

void PASCAL
Control_InitControls2(HWND hwnd)
{
    LPTSTR ptsz;
    TCHAR tszKeys[512];
    TCHAR tszName[64];

    /* BUGBUG -- add string */
    GetPrivateProfileString(TEXT("MMCPL"), NULL, c_tszNil,
                            tszKeys, cA(tszKeys), c_tszControlIni);

    for (ptsz = tszKeys; ptsz[0]; ptsz += lstrlen(ptsz) + 1) {
        /*
         *  Some legacy keys to ignore:
         *
         *  NumApps=        <number of applets>
         *  H=              <height>
         *  W=              <width>
         *  X=              <x coordinate>
         *  Y=              <y coordinate>
         */
        /* BUGBUG -- Add string */
        if (lstrcmpi(ptsz, TEXT("NumApps")) == 0) {
            continue;
        }
        if (ptsz[1] == TEXT('\0')) {
            switch (ptsz[0]) {
            case 'H':
            case 'W':
            case 'X':
            case 'Y': continue;
            }
        }

        /*
         *  End of wacky legacy keys.
         */

        GetPrivateProfileString(TEXT("MMCPL"), ptsz, c_tszNil,
                                tszName, cA(tszName), c_tszControlIni);
        Control_AddCpl(hwnd, tszName);
    }
}

/*****************************************************************************
 *
 *  Control_InitControls3
 *
 *  Enumerate all the *.CPL files in the System folder
 *  and add them to the list.
 *
 *****************************************************************************/

void PASCAL
Control_InitControls3(HWND hwnd)
{
    WIN32_FIND_DATA wfd;
    HANDLE h;
    TCHAR tszSystemDir[MAX_PATH];
    TCHAR tszPrevDir[MAX_PATH];

    GetCurrentDirectory(cA(tszPrevDir), tszPrevDir); /* For restore */
    GetSystemDirectory(tszSystemDir, MAX_PATH);
    if (SetCurrentDirectory(tszSystemDir)) {

        h = FindFirstFile(c_tszStarCpl, &wfd);
        if (h != INVALID_HANDLE_VALUE) {
            do {
                Control_AddCpl(hwnd, wfd.cFileName);
            } while (FindNextFile(h, &wfd)) ;
            FindClose(h);
        }
        SetCurrentDirectory(tszPrevDir);
    }
}

/*****************************************************************************
 *
 *  Control_OnInitDialog
 *
 *  Enumerate all the *.CPL files and add them to the list.
 *
 *****************************************************************************/

BOOL PASCAL
Control_OnInitDialog(HWND hwnd)
{
    HCURSOR hcurPrev = GetCursor();
    SetCursor(LoadCursor(0, IDC_WAIT));

    ZeroMemory(pcpii, cbX(*pcpii));

    LoadString(hinstCur, IDS_CPL_BORING, g_tszBoring, cA(g_tszBoring));

    if (Misc_InitPgxa(&pcpii->gxa, cbX(CPL))) {
        Control_InitControls2(hwnd);
        Control_InitControls3(hwnd);
    }

    SetCursor(hcurPrev);
    return 1;
}

/*****************************************************************************
 *
 *  Control_OnDestroy
 *
 *  Free the memory we allocated.
 *
 *****************************************************************************/

BOOL PASCAL
Control_OnDestroy(HWND hdlg)
{
    Misc_FreePgxa(&pcpii->gxa);
    return 1;
}


/*****************************************************************************
 *
 *  Control_LV_Dirtify
 *
 *      Mark this item as having been changed during the property sheet
 *	page's lifetime.
 *
 *****************************************************************************/

void PASCAL
Control_LV_Dirtify(LPARAM icpl)
{
//    pcpii->pcpl[icpl].cplfl |= cplflEdited;
}

/*****************************************************************************
 *
 *  Control_OnApply
 *
 *      Write the changes out.
 *
 *****************************************************************************/

void PASCAL
Control_OnApply(HWND hdlg)
{
    HWND hwnd = GetDlgItem(hdlg, IDC_ICONLV);
    int cItems = ListView_GetItemCount(hwnd);
    BOOL fChanged = 0;
    LV_ITEM lvi;

    for (lvi.iItem = 0; lvi.iItem < cItems; lvi.iItem++) {
        PCPL pcpl;
        lvi.stateMask = LVIS_STATEIMAGEMASK;
        Misc_LV_GetItemInfo(hwnd, &lvi, lvi.iItem, LVIF_PARAM | LVIF_STATE);
        pcpl = pcplPlvi(&lvi);

        if (Control_GetShowState(pcpl->tszFile) != LV_IsChecked(&lvi)) {
            fChanged = 1;
            Control_SetShowState(pcpl->tszFile, LV_IsChecked(&lvi));
        }
    }

    if (fChanged) {
        PIDL pidlCpl;
        if (SHGetSpecialFolderLocation(hdlg, CSIDL_CONTROLS, &pidlCpl)) {
            SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_IDLIST, pidlCpl, 0L);
            Ole_Free(pidlCpl);
        }
    }
}

/*****************************************************************************
 *
 *  Oh yeah, we need this too.
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

LVV lvvControl = {
    0,                          /* Control_OnCommand */
    0,                          /* Control_LV_OnInitContextMenu */
    Control_LV_Dirtify,
    0,                          /* Control_LV_GetIcon */
    Control_OnInitDialog,
    Control_OnApply,
    Control_OnDestroy,
    0,                          /* Control_OnSelChange */
    7,                          /* iMenu */
    rgdwHelp,
    0,				/* Double-click action */
    lvvflCanCheck,              /* We need check boxes */
    {
	{ 0,			0 },
    }
};

#pragma END_CONST_DATA


/*****************************************************************************
 *
 *  Our window procedure.
 *
 *****************************************************************************/

BOOL EXPORT
Control_DlgProc(HWND hdlg, UINT wm, WPARAM wParam, LPARAM lParam)
{
    return LV_DlgProc(&lvvControl, hdlg, wm, wParam, lParam);
}
