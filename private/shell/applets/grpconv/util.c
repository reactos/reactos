//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
#include "grpconv.h"
#include "util.h"
#include "rcids.h"

//---------------------------------------------------------------------------
// Global to this file only.

const TCHAR g_szDot[] = TEXT(".");
const TCHAR g_szShellOpenCommand[] = TEXT("\\Shell\\Open\\Command");
const TCHAR c_szElipses[] = TEXT("...");
const TCHAR c_szSpace[] = TEXT(" ");
const TCHAR c_szUS[] = TEXT("_");

static BOOL g_fShowProgressDlg = FALSE;
HWND g_hwndProgress = NULL;     // Progress dialog.

//---------------------------------------------------------------------------
LRESULT CALLBACK ProgressWndProc(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        SetDlgItemText(hdlg, IDC_GROUPNAME, (LPTSTR)lparam);
        EnableMenuItem(GetSystemMenu(hdlg, FALSE), SC_CLOSE, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
        return TRUE;
    }

    return 0;
}

//---------------------------------------------------------------------------
void ShowProgressDlg(void)
{
    // Has someone tried to create the dialog but it isn't up yet?
    if (g_fShowUI && g_fShowProgressDlg && !g_hwndProgress)
    {
        // Yep.
        // NB We can handle this failing, we just try to carry on without
        // the dialog.
        g_hwndProgress = CreateDialog(g_hinst, MAKEINTRESOURCE(DLG_PROGRESS), NULL, ProgressWndProc);
    }
}

//---------------------------------------------------------------------------
void Group_CreateProgressDlg(void)
{
    // NB We just set a flag here, the first guy to try to set the
    // current progress actually puts up the dialag.
    g_fShowProgressDlg = TRUE;
}

//---------------------------------------------------------------------------
void Group_DestroyProgressDlg(void)
{
    if (g_hwndProgress)
    {
        DestroyWindow(g_hwndProgress);
        g_hwndProgress = NULL;
    }
    g_fShowProgressDlg = FALSE;
}

//---------------------------------------------------------------------------
// If the text is too long, lop off the end and stick on some elipses.
void Text_TruncateAndAddElipses(HWND hwnd, LPTSTR lpszText)
{
        RECT rcClient;
        SIZE sizeText;
        SIZE sizeElipses;
        HDC hdc;
        UINT cch;
        
        Assert(hwnd);
        Assert(lpszText);
        
        hdc = GetDC(hwnd);
        if (hdc)
        {
                GetClientRect(hwnd, &rcClient);
                GetTextExtentPoint(hdc, lpszText, lstrlen(lpszText), &sizeText);
                // Is the text too long?
                if (sizeText.cx > rcClient.right)
                {
                        // Yes, it is, clip it.
                        GetTextExtentPoint(hdc, c_szElipses, 3, &sizeElipses);
                        GetTextExtentExPoint(hdc, lpszText, lstrlen(lpszText), rcClient.right - sizeElipses.cx,
                                &cch, NULL, &sizeText);
                        lstrcpy(lpszText+cch, c_szElipses);
                }
                ReleaseDC(hwnd, hdc);
        }
}

//---------------------------------------------------------------------------
void Group_SetProgressDesc(UINT nID)
{
    TCHAR sz[MAX_PATH];

    ShowProgressDlg();
    if (g_hwndProgress)
    {
        LoadString(g_hinst, nID, sz, ARRAYSIZE(sz));
                SendDlgItemMessage(g_hwndProgress, IDC_STATIC, WM_SETTEXT, 0, (LPARAM)sz);
    }
}

//---------------------------------------------------------------------------
void Group_SetProgressNameAndRange(LPCTSTR lpszGroup, int iMax)
{
        TCHAR sz[MAX_PATH];
        TCHAR szNew[MAX_PATH];
        LPTSTR lpszName;
        MSG msg;
        static int cGen = 1;
        
        ShowProgressDlg();
        if (g_hwndProgress)
        {
                // DebugMsg(DM_TRACE, "gc.gspnar: Range 0 to %d", iMax);
                SendDlgItemMessage(g_hwndProgress, IDC_PROGRESS, PBM_SETRANGE, 0, MAKELPARAM(0, iMax));

                if (lpszGroup == (LPTSTR)-1)
                {
                        // Use some sensible name - Programs (x)
                        // where x = 1 to n, incremented each time this is
                        // called.
                        LoadString(g_hinst, IDS_GROUP, sz, sizeof(sz));
                        wsprintf(szNew, TEXT("%s (%d)"), sz, cGen++);
                        SetDlgItemText(g_hwndProgress, IDC_GROUPNAME, szNew);
                }
                else if (lpszGroup && *lpszGroup)
                {
                        lpszName = PathFindFileName(lpszGroup);
                        lstrcpy(sz, lpszName);
                        Text_TruncateAndAddElipses(GetDlgItem(g_hwndProgress, IDC_GROUPNAME), sz);
                        SetDlgItemText(g_hwndProgress, IDC_GROUPNAME, sz);
                }
                else
                {
                        // Use some sensible name.
                        LoadString(g_hinst, IDS_PROGRAMS, sz, ARRAYSIZE(sz));
                        SetDlgItemText(g_hwndProgress, IDC_GROUPNAME, sz);
                }
                
                // Let paints come in.
                while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                {
                    DispatchMessage(&msg);
                }
        }
}

//---------------------------------------------------------------------------
void Group_SetProgress(int i)
{
        MSG msg;

        ShowProgressDlg();
        if (g_hwndProgress)
        {               
                // DebugMsg(DM_TRACE, "gc.gsp: Progress %d", i);
                
            // Progman keeps trying to steal the focus...
                SetForegroundWindow(g_hwndProgress);
                SendDlgItemMessage(g_hwndProgress, IDC_PROGRESS, PBM_SETPOS, i, 0);
        }

        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
                DispatchMessage(&msg);
        }

}

#if 0
//---------------------------------------------------------------------------
BOOL WritePrivateProfileInt(LPCTSTR lpszSection, LPCTSTR lpszValue, int i, LPCTSTR lpszIniFile)
{
        TCHAR szBuf[CCHSZSHORT];

        wsprintf(szBuf, TEXT("%d"), i);
        return WritePrivateProfileString(lpszSection, lpszValue, szBuf, lpszIniFile);
}
#endif

//---------------------------------------------------------------------------
// Register an app as being able to handle a particular extension with the
// given internal type, human readble type and command.
// NB lpszExt doesn't need a dot.
// By default this won't overide something in the registration DB.
// Setting fOveride to TRUE will cause existing entries in the DB
// to be over written.
void ShellRegisterApp(LPCTSTR lpszExt, LPCTSTR lpszTypeKey,
    LPCTSTR lpszTypeValue, LPCTSTR lpszCommand, BOOL fOveride)
    {
    TCHAR szKey[CCHSZNORMAL];
    TCHAR szValue[CCHSZSHORT];
    LONG lcb;
    LONG lStatus;

    // Deal with the mapping from extension to TypeKey.
    lstrcpy(szKey, g_szDot);
    lstrcat(szKey, lpszExt);
    lcb = SIZEOF(szValue);
    lStatus = RegQueryValue(HKEY_CLASSES_ROOT, szKey, szValue, &lcb);
    // Is the extension not registered or do we even care?
    if (lStatus != ERROR_SUCCESS || fOveride)
        {
        // No, so register it.
        lstrcpy(szValue, lpszTypeKey);
        if (RegSetValue(HKEY_CLASSES_ROOT, szKey, REG_SZ, lpszTypeKey, 0) == ERROR_SUCCESS)
            {
//            DebugMsg(DM_TRACE, "gc.sra: Extension registered.");
            }
        else
            {
            DebugMsg(DM_ERROR, TEXT("gc.sra: Error registering extension."));
            }
        }

    // Deal with the mapping from TypeKey to TypeValue
    lcb = SIZEOF(szValue);
    lStatus = RegQueryValue(HKEY_CLASSES_ROOT, lpszTypeKey, szValue, &lcb);
    // Is the type not registered or do we even care?
    if (lStatus != ERROR_SUCCESS || fOveride)
        {
        // No, so register it.
        if (RegSetValue(HKEY_CLASSES_ROOT, lpszTypeKey, REG_SZ, lpszTypeValue, 0) == ERROR_SUCCESS)
            {
//            DebugMsg(DM_TRACE, "gc.sra: Type registered.");
            }
        else
            {
            DebugMsg(DM_ERROR, TEXT("gc.sra: Error registering type."));
            }
        }

    // Deal with adding the open command.
    lstrcpy(szKey, lpszTypeKey);
    lstrcat(szKey, g_szShellOpenCommand);
    lcb = SIZEOF(szValue);
    lStatus = RegQueryValue(HKEY_CLASSES_ROOT, szKey, szValue, &lcb);
    // Is the command not registered or do we even care?
    if (lStatus != ERROR_SUCCESS || fOveride)
        {
        // No, so register it.
        if (RegSetValue(HKEY_CLASSES_ROOT, szKey, REG_SZ, lpszCommand, 0) == ERROR_SUCCESS)
            {
//            DebugMsg(DM_TRACE, "gc.sra: Command registered.");
            }
        else
            {
            DebugMsg(DM_ERROR, TEXT("gc.sra: Error registering command."));
            }
        }
    }

#if 0
//-------------------------------------------------------------------------
// Do a unix(ish) gets(). This assumes bufferd i/o.
// Reads cb-1 characters (the last one will be a NULL) or up to and including
// the first NULL.
LPTSTR fgets(LPTSTR sz, WORD cb, int fh)
    {
    UINT i;

    // Leave room for the NULL.
    cb--;
    for (i=0; i<cb; i++)
        {
        _lread(fh, &sz[i], 1);
        // Check for a null.
        if (sz[i] == TEXT('\0'))
            return sz;
        }

    // Ran out of room.
    // NULL Terminate.
    sz[cb-1] = TEXT('\0');
    return sz;
    }
#else
//-------------------------------------------------------------------------
// Do a unix(ish) gets(). This assumes bufferd i/o.
// Reads cb-1 characters (the last one will be a NULL) or up to and including
// the first NULL.
#ifdef UNICODE
LPTSTR fgets(LPTSTR sz, DWORD count, HANDLE fh)
{
    DWORD cch;
    DWORD dwFilePointer, dwBytesRead;
    CHAR *AnsiString = NULL, *AnsiStringPointer, ch;
    LPTSTR retval = NULL;

    //
    // Allocate memory for the reading the ansi string from the stream
    //

    if ((AnsiString = (CHAR *)LocalAlloc(LPTR, count * SIZEOF(CHAR))) == NULL) {
        return(retval);
    }
    AnsiStringPointer = AnsiString;

    // Where are we?
    dwFilePointer = SetFilePointer(fh, 0, NULL, FILE_CURRENT);

    // Fill the buffer.
    ReadFile(fh, AnsiString, count, &dwBytesRead, NULL);

    // Always null the buffer.
    AnsiString[count-1] = '\0';

    // Convert the Ansi String to Unicode
    if (MultiByteToWideChar(
        CP_ACP,
        MB_PRECOMPOSED,
        AnsiString,
        -1,
        sz,
        count
        )  != 0) {
        retval = sz;
    }

    // If there was an earlied null we need to puke the rest 
    // back in to the stream?
    cch = lstrlenA(AnsiString);
    if (cch != count-1)
        SetFilePointer(fh, dwFilePointer+cch+1, NULL, FILE_BEGIN);

    // Do Cleanup
    if (AnsiString != NULL) {
        LocalFree(AnsiString);
    }

    return retval;
}
#else
LPTSTR fgets(LPTSTR sz, WORD cb, int fh)
{
    int cch;
    LONG lpos;

    // Where are we?
    lpos = _llseek(fh, 0, 1);
    // Fill the buffer.
    _lread(fh, sz, cb);
    // Always null the buffer.
    sz[cb-1] = TEXT('\0');
    // If there was an earlied null we need to puke the rest 
    // back in to the stream?
    cch = lstrlen(sz);
    if (cch != cb-1)
        _llseek(fh, lpos+cch+1, 0);
    return sz;
}
#endif
#endif

//---------------------------------------------------------------------------
// Put up a message box wsprintf style.
int MyMessageBox(HWND hwnd, UINT idTitle, UINT idMessage, LPCTSTR lpsz, UINT nStyle)
    {
    TCHAR szTempField[CCHSZNORMAL];
    TCHAR szTitle[CCHSZNORMAL];
    TCHAR szMessage[CCHSZNORMAL];
    int  iMsgResult;

    if (LoadString(g_hinst, idTitle, szTitle, ARRAYSIZE(szTitle)))
        {
        if (LoadString(g_hinst, idMessage, szTempField, ARRAYSIZE(szTempField)))
            {
            if (lpsz)
                wsprintf(szMessage, szTempField, (LPTSTR)lpsz);
            else
                lstrcpy(szMessage, szTempField);

            if (hwnd)
                hwnd = GetLastActivePopup(hwnd);

            iMsgResult = MessageBox(hwnd, szMessage, szTitle, nStyle);
            if (iMsgResult != -1)
                return iMsgResult;
            }
        }

    // Out of memory...
    DebugMsg(DM_ERROR, TEXT("MMB: Out of memory.\n\r"));
    return -1;
    }

//-------------------------------------------------------------------------
// Replace hash characters in a string with NULLS.
void ConvertHashesToNulls(LPTSTR p)
    {
    while (*p)
        {
        if (*p == TEXT('#'))
            {
            *p = TEXT('\0');
            // You can't do an AnsiNext on a NULL.
            // NB - we know this is a single byte.
            p++;
            }
        else
            p = CharNext(p);
        }
    }

//-------------------------------------------------------------------------
// Copy the directory component of a path into the given buffer.
// i.e. everything after the last slash and the slash itself for everything
// but the root.
// lpszDir is assumed to be as big as lpszPath.
void Path_GetDirectory(LPCTSTR lpszPath, LPTSTR lpszDir)
    {
    LPTSTR lpszFileName;
    UINT cb;

    // The default is a null.
    lpszDir[0] = TEXT('\0');

    // Copy over everything but the filename.
    lpszFileName = PathFindFileName(lpszPath);
    cb = (UINT)(lpszFileName-lpszPath);
    if (cb)
        {
        // REVIEW lstrcpyn seems to have a problem with a cb of 0;
        lstrcpyn(lpszDir, lpszPath, cb+1);

        // Remove the trailing slash if needed.
        if (!PathIsRoot(lpszDir))
            lpszDir[cb-1] = TEXT('\0');
        }
    }




//-------------------------------------------------------------------------
//
// internal CoCreateInstance.
//
// bind straight to shell232 DllGetClassObject()
// this is meant to skip all the CoCreateInstance stuff when we
// know the thing we are looking for is in shell232.dll.  this also
// makes things work if the registry is messed up
//
HRESULT ICoCreateInstance(REFCLSID rclsid, REFIID riid, LPVOID FAR* ppv)
{
    LPCLASSFACTORY pcf;
    HRESULT hres = SHDllGetClassObject(rclsid, &IID_IClassFactory, &pcf);
    if (SUCCEEDED(hres))
    {
        hres = pcf->lpVtbl->CreateInstance(pcf, NULL, riid, ppv);
        pcf->lpVtbl->Release(pcf);
    }
    return hres;
}

//-------------------------------------------------------------------------
LPTSTR _lstrcatn(LPTSTR lpszDest, LPCTSTR lpszSrc, UINT cbDest)
{
    UINT i;

    i = lstrlen(lpszDest);
    lstrcpyn(lpszDest+i, lpszSrc, cbDest-i);
    return lpszDest;
}

//-------------------------------------------------------------------------
// Simplified from shelldll. Keep sticking on numbers till the name is unique.
BOOL WINAPI MakeUniqueName(LPTSTR pszNewName, UINT cbNewName, LPCTSTR pszOldName,
    UINT nStart, PFNISUNIQUE pfnIsUnique, UINT nUser, BOOL fLFN)
{
    TCHAR szAddend[4];
    int cbAddend;
    int i;

    // Is it already unique?
    if ((*pfnIsUnique)(pszOldName, nUser))
    {
        lstrcpyn(pszNewName, pszOldName, cbNewName);
        return TRUE;
    }
    else
    {
        // NB Max is 100 identically names things but we should never
        // hit this as the max number of items in a progman group was 50.
        for (i=nStart; i<100; i++)
        {
            // Generate the addend.
            wsprintf(szAddend, TEXT("#%d"), i);
            cbAddend = lstrlen(szAddend);
            // Lotsa room?
            if ((UINT)(lstrlen(pszOldName)+cbAddend+1) > cbNewName)
            {
                // Nope.
                lstrcpyn(pszNewName, pszOldName, cbNewName);
                lstrcpy(pszNewName+(cbNewName-cbAddend), szAddend);
            }
            else
            {
                // Yep.
                lstrcpy(pszNewName, pszOldName);
                
                if (!fLFN)
                    lstrcat(pszNewName, c_szSpace);

                lstrcat(pszNewName, szAddend);
            }
            // Is it unique?
            if ((*pfnIsUnique)(pszNewName, nUser))
            {
                // Yep.
                return TRUE;
            }
        }
    }

    // Ooopsie.
    lstrcpyn(pszNewName, pszOldName, cbNewName);
    DebugMsg(DM_ERROR, TEXT("gp.mun: Unable to generate a unique name for %s."), pszOldName);
    return FALSE;
}

//-------------------------------------------------------------------------
// Simplified from shell.dll (For LFN things only).
BOOL WINAPI YetAnotherMakeUniqueName(LPTSTR pszNewName, UINT cbNewName, LPCTSTR pszOldName,
    PFNISUNIQUE pfnIsUnique, UINT n, BOOL fLFN)
{
    BOOL fRet = FALSE;
    TCHAR szTemp[MAX_PATH];

    // Is given name already unique?
    if ((*pfnIsUnique)(pszOldName, n))
    {
        // Yep,
        lstrcpyn(pszNewName, pszOldName, cbNewName);
    }
    else
    {
        if (fLFN)
        {
            // Try "another".
            LoadString(g_hinst, IDS_ANOTHER, szTemp, ARRAYSIZE(szTemp));
            _lstrcatn(szTemp, pszOldName, cbNewName);
            if (!(*pfnIsUnique)(szTemp, n))
            {
                // Nope, use the old technique of sticking on numbers.
                return MakeUniqueName(pszNewName, cbNewName, pszOldName, 3, pfnIsUnique, n, FALSE);
            }
            else
            {
                // Yep.
                lstrcpyn(pszNewName, szTemp, cbNewName);
            }
        }
        else
        {
            // Just stick on numbers.
            return MakeUniqueName(pszNewName, cbNewName, pszOldName, 2, pfnIsUnique, n, TRUE);
        }
    }
    // Name is unique.
    return TRUE;
}

//----------------------------------------------------------------------------
// Sort of a registry equivalent of the profile API's.
BOOL WINAPI Reg_Get(HKEY hkey, LPCTSTR pszSubKey, LPCTSTR pszValue, LPVOID pData, DWORD cbData)
{
    HKEY hkeyNew;
    BOOL fRet = FALSE;
    DWORD dwType;
    
    if (!GetSystemMetrics(SM_CLEANBOOT) && (RegOpenKey(hkey, pszSubKey, &hkeyNew) == ERROR_SUCCESS))
    {
        if (RegQueryValueEx(hkeyNew, (LPVOID)pszValue, 0, &dwType, pData, &cbData) == ERROR_SUCCESS)
        {
            fRet = TRUE;
        }
        RegCloseKey(hkeyNew);
    }
    return fRet;
}

//----------------------------------------------------------------------------
// Sort of a registry equivalent of the profile API's.
BOOL WINAPI Reg_Set(HKEY hkey, LPCTSTR pszSubKey, LPCTSTR pszValue, DWORD dwType, 
    LPVOID pData, DWORD cbData)
{
    HKEY hkeyNew;
    BOOL fRet = FALSE;

    if (pszSubKey)
    {
        if (RegCreateKey(hkey, pszSubKey, &hkeyNew) == ERROR_SUCCESS)
        {
            if (RegSetValueEx(hkeyNew, pszValue, 0, dwType, pData, cbData) == ERROR_SUCCESS)
            {
                fRet = TRUE;
            }
            RegCloseKey(hkeyNew);
        }
    }
    else
    {
        if (RegSetValueEx(hkey, pszValue, 0, dwType, pData, cbData) == ERROR_SUCCESS)
        {
            fRet = TRUE;
        }
    }
    return fRet;
}

//----------------------------------------------------------------------------
BOOL WINAPI Reg_SetDWord(HKEY hkey, LPCTSTR pszSubKey, LPCTSTR pszValue, DWORD dw)
{
    return Reg_Set(hkey, pszSubKey, pszValue, REG_DWORD, &dw, SIZEOF(dw));
}

//----------------------------------------------------------------------------
BOOL WINAPI Reg_GetDWord(HKEY hkey, LPCTSTR pszSubKey, LPCTSTR pszValue, LPDWORD pdw)
{
    return Reg_Get(hkey, pszSubKey, pszValue, pdw, SIZEOF(*pdw));
}

//----------------------------------------------------------------------------
void __cdecl _Log(LPCTSTR pszMsg, ...)
{
    TCHAR sz[2*MAX_PATH+40];  // Handles 2*largest path + slop for message
    va_list     vaListMarker;

    va_start(vaListMarker, pszMsg);

    if (g_hkeyGrpConv)
    {
        wvsprintf(sz, pszMsg, vaListMarker);
        Reg_SetString(g_hkeyGrpConv, NULL, TEXT("Log"), sz);
    }
}
