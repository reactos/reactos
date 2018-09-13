//
// processes the version property sheet page.
//

#include "shellprv.h"
#pragma  hdrstop

#ifndef WIN32
#include <ver.h>
#include <version.h>
#endif

#include "help.h"

#define DWORDUP(x)              (((x)+3)&~3)
#define VerKeyToValue(lpKey)    (lpKey + DWORDUP(lstrlen(lpKey)+1))

#pragma warning(disable: 4200)   // zero size array in struct

// magic undoced explort from version.dll

STDAPI_(BOOL) VerQueryValueIndexW(const void *pBlock, LPTSTR lpSubBlock, DWORD dwIndex, void **ppBuffer, void **ppValue, PUINT puLen);

#ifdef UNICODE
#define VerQueryValueIndex VerQueryValueIndexW
#endif

typedef struct 
{
    WORD wTotLen;
    WORD wValLen;
    TCHAR szKey[];
} SHELLVERBLOCK, *LPSHELLVERBLOCK;

// Following code is copied from fileman\wfdlgs2.c


//    The following data structure associates a version stamp datum
//    name (which is not localized) with a string ID.  This is so we
//    can show translations of these names to the user.
struct vertbl {
    TCHAR const *pszName;
    short idString;
};

//   Note that version stamp datum names are NEVER internationalized,
//   so the following literal strings are just fine.
const struct vertbl vernames[] = {
    { TEXT("FileVersion"),              IDD_VERSION_FILEVERSION },
    { TEXT("LegalCopyright"),   IDD_VERSION_COPYRIGHT },
    { TEXT("FileDescription"),  IDD_VERSION_DESCRIPTION },
    { TEXT("Comments"),                 IDS_VN_COMMENTS },
    { TEXT("CompanyName"),              IDS_VN_COMPANYNAME },
    { TEXT("InternalName"),             IDS_VN_INTERNALNAME },
    { TEXT("LegalTrademarks"),  IDS_VN_LEGALTRADEMARKS },
    { TEXT("OriginalFilename"), IDS_VN_ORIGINALFILENAME },
    { TEXT("PrivateBuild"),             IDS_VN_PRIVATEBUILD },
    { TEXT("ProductName"),              IDS_VN_PRODUCTNAME },
    { TEXT("ProductVersion"),           IDS_VN_PRODUCTVERSION },
    { TEXT("SpecialBuild"),             IDS_VN_SPECIALBUILD }
};

#define NUM_SPECIAL_STRINGS     3


typedef struct { // vp
    PROPSHEETPAGE psp;
    HWND hDlg;
    LPTSTR pVerBuffer;          /* pointer to version data */
    TCHAR szVersionKey[60];     /* big enough for anything we need */
    struct _VERXLATE
    {
        WORD wLanguage;
        WORD wCodePage;
    } *lpXlate;                     /* ptr to translations data */
    int cXlate;                 /* count of translations */
    LPTSTR pszXlate;
    int cchXlateString;
    TCHAR szFile[MAX_PATH];
} VERPROPSHEETPAGE, * LPVERPROPSHEETPAGE;


#define VER_KEY_END     25      /* length of "\StringFileInfo\xxxxyyyy\" */
                                /* (not localized) */
#define MAXMESSAGELEN   (50 + MAX_PATH * 2)


/*
    Gets a particular datum about a file.  The file's version info
    should have already been loaded by GetVersionInfo.  If no datum
    by the specified name is available, NULL is returned.  The name
    specified should be just the name of the item itself;  it will
    be concatenated onto "\StringFileInfo\xxxxyyyy\" automatically.

    Version datum names are not localized, so it's OK to pass literals
    such as "FileVersion" to this function.

    Note that since the returned datum is in a global memory block,
    the return value of this function is LPSTR, not PSTR.
*/
LPTSTR GetVersionDatum(LPVERPROPSHEETPAGE pvp, LPCTSTR pszName)
{
    UINT cbValue = 0;
    LPTSTR lpValue;

    if (!pvp->pVerBuffer)
        return NULL;

    lstrcpy(pvp->szVersionKey + VER_KEY_END, pszName);

    VerQueryValue(pvp->pVerBuffer, pvp->szVersionKey, (void **)&lpValue, &cbValue);

    return (cbValue != 0) ? lpValue : NULL;
}

/*
    Frees global version data about a file.  After this call, all
    GetVersionDatum calls will return NULL.  To avoid memory leaks,
    always call this before the main properties dialog exits.
*/
void FreeVersionInfo(LPVERPROPSHEETPAGE pvp)
{
    if (pvp->pVerBuffer) 
    {
        GlobalFree(pvp->pVerBuffer);
        pvp->pVerBuffer = NULL;
    }
    if (pvp->pszXlate) 
    {
        LocalFree((HLOCAL)(HANDLE)pvp->pszXlate);
        pvp->pszXlate = NULL;
    }

    pvp->lpXlate = NULL;
}

/*
    Initialize version information for the properties dialog.  The
    above global variables are initialized by this function, and
    remain valid (for the specified file only) until FreeVersionInfo
    is called.

    The first language we try will be the first item in the
    "\VarFileInfo\Translations" section;  if there's nothing there,
    we try the one coded into the IDS_FILEVERSIONKEY resource string.
    If we can't even load that, we just use English (040904E4).  We
    also try English with a null codepage (04090000) since many apps
    were stamped according to an old spec which specified this as
    the required language instead of 040904E4.

    GetVersionInfo returns TRUE if the version info was read OK,
    otherwise FALSE.  If the return is FALSE, the buffer may still
    have been allocated;  always call FreeVersionInfo to be safe.

    pszPath is modified by this call (pszName is appended).
*/
BOOL GetVersionInfo(LPVERPROPSHEETPAGE pvp, LPCTSTR pszPath)
{
    UINT cbValue = 0;
    LPTSTR pszValue = NULL;
    DWORD dwHandle;             /* version subsystem handle */
    DWORD dwVersionSize;        /* size of the version data */

    FreeVersionInfo(pvp);       /* free old version buffer */

    // cast const -> non const for bad API def
    dwVersionSize = GetFileVersionInfoSize((LPTSTR)pszPath, &dwHandle);

    if (dwVersionSize == 0L)
        return FALSE;           /* no version info */

    pvp->pVerBuffer = GlobalAlloc(GPTR, dwVersionSize);
    if (pvp->pVerBuffer == NULL)
        return FALSE;

    // cast const -> non const for bad API def
    
    if (!GetFileVersionInfo((LPTSTR)pszPath, dwHandle, dwVersionSize, pvp->pVerBuffer))
    {
        return FALSE;
    }

    // Look for translations
    if (VerQueryValue(pvp->pVerBuffer, TEXT("\\VarFileInfo\\Translation"), (void **)&pvp->lpXlate, &cbValue)
                && cbValue)
    {
        pvp->cXlate = cbValue / SIZEOF(DWORD);
        pvp->cchXlateString = pvp->cXlate * 64;  /* figure 64 chars per lang name */
        pvp->pszXlate = (LPTSTR)(void*)LocalAlloc(LPTR, pvp->cchXlateString*SIZEOF(TCHAR));
        // failure of above will be handled later
    }
    else
    {
        pvp->lpXlate = NULL;
    }

    // Try same language as this program
    if (LoadString(HINST_THISDLL, IDS_VN_FILEVERSIONKEY, pvp->szVersionKey, ARRAYSIZE(pvp->szVersionKey)))
    {
        if (GetVersionDatum(pvp, vernames[0].pszName))
        {
            return TRUE;
        }
    }

    // Try first language this supports
    if (pvp->lpXlate)
    {
        wsprintf(pvp->szVersionKey, TEXT("\\StringFileInfo\\%04X%04X\\"),
                pvp->lpXlate[0].wLanguage, pvp->lpXlate[0].wCodePage);
        if (GetVersionDatum(pvp, vernames[0].pszName))  /* a required field */
        {
            return TRUE;
        }
    }

#ifdef UNICODE
    // try English, unicode code page
    lstrcpy(pvp->szVersionKey, TEXT("\\StringFileInfo\\040904B0\\"));
    if (GetVersionDatum(pvp, vernames[0].pszName))
    {
        return TRUE;
    }
#endif

    // try English
    lstrcpy(pvp->szVersionKey, TEXT("\\StringFileInfo\\040904E4\\"));
    if (GetVersionDatum(pvp, vernames[0].pszName))
    {
        return TRUE;
    }

    // try English, null codepage
    lstrcpy(pvp->szVersionKey, TEXT("\\StringFileInfo\\04090000\\"));
    if (GetVersionDatum(pvp, vernames[0].pszName))
    {
        return TRUE;
    }

    // Could not find FileVersion info in a reasonable format
    return FALSE;
}


/*
    Fills the version key listbox with all available keys in the
    StringFileInfo block, and sets the version value text to the
    value of the first item.
*/
#ifndef WINNT
#define NOQUERYVALUEINDEX
#endif

void FillVersionList(LPVERPROPSHEETPAGE pvp)
{
    LPTSTR pszName;
    LPTSTR pszValue;
    TCHAR szStringBase[VER_KEY_END+1];
    int i, j, idx;
    HWND hwndLB;
    TCHAR szMessage[MAXMESSAGELEN+1];
    UINT uOffset, cbValue;
#ifdef NOQUERYVALUEINDEX
    LPSHELLVERBLOCK pszBlock, pszEnd;      /* will point to a block */
#endif
    
    hwndLB = GetDlgItem(pvp->hDlg, IDD_VERSION_KEY);
    
    ListBox_ResetContent(hwndLB);
    for (i=0; i<NUM_SPECIAL_STRINGS; ++i)
    {
        SetDlgItemText(pvp->hDlg, vernames[i].idString, szNULL);
    }
    
    pvp->szVersionKey[VER_KEY_END] = 0;        /* don't copy too much */
    lstrcpy(szStringBase, pvp->szVersionKey);   /* copy to our buffer */
    szStringBase[VER_KEY_END - 1] = 0; /* strip the backslash */
    
    // Note: The Nt Version of version.dll has other exports.  If/When they are
    // available in Win version then we can remove this section...
#ifdef NOQUERYVALUEINDEX
    if (!VerQueryValue(pvp->pVerBuffer, szStringBase, (void **)&pszBlock, &cbValue))
    {
        goto NoVersionStrings;
    }
    
    /*
    Now pszBlock points to where the value of the StringFileInfo\xxxxyyyy
    block would be.  Add on the value length (which should be 0) and
    dword padding, and we're pointing at the child blocks.
    
      We also get the total size of the StringFileInfo block.  We do this
      by backing up 16 bytes:  4 for the two words (block len, value len),
      the size of "xxxxyyyy", and dword padding.  This is fetching the
      block length of the Var block.  We subtract 16 from that length
      because it includes the name of the Var block itself, etc.
    */
    
    pszBlock = (LPSHELLVERBLOCK)((LPSTR)pszBlock - (DWORDUP(sizeof("040904e4")) + sizeof(SHELLVERBLOCK)));
    pszEnd   = (LPSHELLVERBLOCK)((LPSTR)pszBlock + pszBlock->wTotLen);
    
    pszBlock = (LPSHELLVERBLOCK)((LPSTR)pszBlock + DWORDUP(sizeof("040904e4")) + sizeof(SHELLVERBLOCK) +
        DWORDUP(pszBlock->wValLen));
#endif // NOQUERYVALUEINDEX
    
    //
    // Now iterate through all of the strings
    //
    for (j = 0; ; j++)
    {
#ifdef NOQUERYVALUEINDEX
        if (pszBlock >= pszEnd)
            break;
        pszName = pszBlock->szKey;
        pszValue = VerKeyToValue(pszName);
#else
        if (!VerQueryValueIndex(pvp->pVerBuffer, szStringBase, j, &pszName, &pszValue, &cbValue))
            break;
#endif
        
        for (i = 0; i < ARRAYSIZE(vernames); i++)
        {
            if (!lstrcmp(vernames[i].pszName, pszName))
            {
                break;
            }
        }
        
        if (i < NUM_SPECIAL_STRINGS)
        {
            VS_FIXEDFILEINFO *pffi;
            if ((vernames[i].idString == IDD_VERSION_FILEVERSION) &&
                VerQueryValue(pvp->pVerBuffer, TEXT("\\"), (void **)&pffi, &cbValue) && cbValue)
            {
                TCHAR szString[128];

                // display the binary version info, not the useless 
                // string version (that can be out of sync)

                wnsprintf(szString, ARRAYSIZE(szString), TEXT("%d.%d.%d.%d"), 
                    HIWORD(pffi->dwFileVersionMS),
                    LOWORD(pffi->dwFileVersionMS),
                    HIWORD(pffi->dwFileVersionLS),
                    LOWORD(pffi->dwFileVersionLS));
                SetDlgItemText(pvp->hDlg, vernames[i].idString, szString);
            }
            else
                SetDlgItemText(pvp->hDlg, vernames[i].idString, pszValue);
        }
        else
        {
            if (i == ARRAYSIZE(vernames) ||
                !LoadString(HINST_THISDLL, vernames[i].idString, szMessage, ARRAYSIZE(szMessage)))
            {
                lstrcpy(szMessage, pszName);
            }
            
            idx = ListBox_AddString(hwndLB, szMessage);
            if (idx != LB_ERR)
            {
                ListBox_SetItemData(hwndLB, idx, (DWORD_PTR)pszValue);
            }
        }
#ifdef NOQUERYVALUEINDEX
        pszBlock = (LPSHELLVERBLOCK)((LPSTR)pszBlock + DWORDUP(pszBlock->wTotLen));
#endif
    }
    
    // Now look at the \VarFileInfo\Translations section and add an
    // item for the language(s) this file supports.
#ifdef NOQUERYVALUEINDEX
NoVersionStrings:
#endif
    
    if (pvp->lpXlate == NULL || pvp->pszXlate == NULL)
        return;
    
    if (!LoadString(HINST_THISDLL, (pvp->cXlate == 1) ? IDS_VN_LANGUAGE : IDS_VN_LANGUAGES,
        szMessage, ARRAYSIZE(szMessage)))
        return;
    
    idx = ListBox_AddString(hwndLB, szMessage);
    if (idx == LB_ERR)
        return;
    
    pvp->pszXlate[0] = 0;
    uOffset = 0;
    for (i = 0; i < pvp->cXlate; i++) {
        if (uOffset + 2 > (UINT)pvp->cchXlateString)
            break;
        if (i != 0) {
            lstrcat(pvp->pszXlate, TEXT(", "));
            uOffset += 2;       // skip over ", "
        }
        if (VerLanguageName(pvp->lpXlate[i].wLanguage, pvp->pszXlate + uOffset, pvp->cchXlateString - uOffset) >
            (DWORD)(pvp->cchXlateString - uOffset))
            break;
        uOffset += lstrlen(pvp->pszXlate + uOffset);
    }
    pvp->pszXlate[pvp->cchXlateString - 1] = 0;
    ListBox_SetItemData(hwndLB, idx, (LPARAM)(LPTSTR)pvp->pszXlate);
    ListBox_SetCurSel(hwndLB, 0);
    
    FORWARD_WM_COMMAND(pvp->hDlg, IDD_VERSION_KEY, hwndLB, LBN_SELCHANGE, PostMessage);
}


//
// Function:    _UpdateVersionPrsht, private
//
// Descriptions:
//   This function fills fields of the "version" dialog box (a page of
//  a property sheet) with attributes of the associated file.
//
// Returns:
//  TRUE, if successfully done; FALSE, otherwise.
//
// History:
//  01-06-93 Shrikant   Created
//
BOOL _UpdateVersionPrsht(LPVERPROPSHEETPAGE pvp)
{
    if (GetVersionInfo(pvp, pvp->szFile))           /* changes szPath */
        FillVersionList(pvp);

    return TRUE;
}


void _VersionPrshtCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
        LPTSTR pszValue;
        int idx;

        switch (id)
        {
        case IDD_VERSION_KEY:
                if (codeNotify != LBN_SELCHANGE)
                {
                        break;
                }

                idx = ListBox_GetCurSel(hwndCtl);
                pszValue = (LPTSTR)ListBox_GetItemData(hwndCtl, idx);
                if (pszValue)
                {
                        SetDlgItemText(hwnd, IDD_VERSION_VALUE, pszValue);
                }
                break;
        }
}

// Array for context help:
static const DWORD aVersionHelpIds[] = {
    IDD_VERSION_FILEVERSION, IDH_FPROP_VER_ABOUT,
    IDD_VERSION_DESCRIPTION, IDH_FPROP_VER_ABOUT,
    IDD_VERSION_COPYRIGHT,   IDH_FPROP_VER_ABOUT,
    IDD_VERSION_FRAME,       IDH_FPROP_VER_INFO,
    IDD_VERSION_KEY,         IDH_FPROP_VER_INFO,
    IDD_VERSION_VALUE,       IDH_FPROP_VER_INFO,

    0, 0
};

BOOL_PTR CALLBACK _VersionPrshtDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    LPVERPROPSHEETPAGE pvp = (LPVERPROPSHEETPAGE)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (uMessage)
    {
    case WM_INITDIALOG:
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        pvp = (LPVERPROPSHEETPAGE)lParam;
        pvp->hDlg = hDlg;
        break;

    case WM_DESTROY:
        FreeVersionInfo(pvp);   // free anything we created
        break;

    case WM_HELP:
        WinHelp((HWND) ((LPHELPINFO) lParam)->hItemHandle, NULL, HELP_WM_HELP,
            (ULONG_PTR) (LPTSTR) aVersionHelpIds);
        break;

    case WM_CONTEXTMENU:      // right mouse click
        WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU,
            (ULONG_PTR) (LPTSTR) aVersionHelpIds);
        break;

    case WM_NOTIFY:
        switch (((NMHDR *)lParam)->code)
        {
        case PSN_SETACTIVE:
            _UpdateVersionPrsht(pvp);
            break;
        }
        break;

    case WM_COMMAND:
        HANDLE_WM_COMMAND(hDlg, wParam, lParam, _VersionPrshtCommand);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}


//
// Descriptions:
//   This function creates a property sheet for the "version" page
//  which shows version information.
//
void AddVersionPage(LPCTSTR pszFile, LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam)
{
    DWORD dwAttr, dwVerLen, dwVerHandle;
    VERPROPSHEETPAGE vp = {0};


#ifndef WINNT
    // Note Win9x version.dll can not currently handle long file names
    GetShortPathName(pszFile, vp.szFile, ARRAYSIZE(vp.szFile));
#else
    lstrcpyn(vp.szFile, pszFile, ARRAYSIZE(vp.szFile));
#endif

    // REVIEW: dwVerHandle is unused, and can be NULL on NT.  If the same 
    // is true on Win9x, we can delete the dwVerHandle local var
    dwAttr = GetFileAttributes(vp.szFile);
    if (0xFFFFFFFF != dwAttr && 0 == (dwAttr & FILE_ATTRIBUTE_OFFLINE) /*avoid HSM recall*/)
    {
        dwVerLen = GetFileVersionInfoSize(vp.szFile, &dwVerHandle);
        if (dwVerLen) 
        {
            HPROPSHEETPAGE hpage;

            vp.psp.dwSize = SIZEOF(VERPROPSHEETPAGE);     // extra data
            vp.psp.dwFlags = PSP_DEFAULT;
            vp.psp.hInstance = HINST_THISDLL;
            vp.psp.pszTemplate = MAKEINTRESOURCE(DLG_VERSION);
            vp.psp.pfnDlgProc = _VersionPrshtDlgProc;

            hpage = CreatePropertySheetPage(&vp.psp);
            if (hpage)
                if (!pfnAddPage(hpage, lParam))
                    DestroyPropertySheetPage(hpage);
        }
    }
}
