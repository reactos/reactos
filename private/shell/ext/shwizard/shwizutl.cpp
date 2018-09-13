#include "shwizard.h"

#include <tchar.h>
#include <shlguid.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <shguidp.h>
#include <shellp.h>

#define STR_SHELLCLASSINFO          TEXT(".ShellClassInfo")
#define STR_HTMLINFOTIPFILE (SZ_CANBEUNICODE TEXT("HTMLInfoTipFile"))
#define STR_WEBVIEWTEMPLATE_KEY     TEXT("WebViewTemplate")
#define STR_PERSISTMONIKER_VERSION  TEXT("IE4")

#define FILE_PROTOCOL               TEXT("file://")
#define FILE_FOLDER_SETTINGS        TEXT("Folder Settings")
#define FILE_FOLDER_SETTINGS_SHORT  TEXT("FldSet")
#define FILE_BACKGROUND_IMAGE       TEXT("Background")
#define FILE_BACKGROUND_IMAGE_SHORT TEXT("Backgrnd")
#define FILE_HTMLINFOTIP            TEXT("Comment.htt")
#define FILE_WEBVIEW_TEMPLATE       TEXT("Folder.htt")

const WCHAR g_wchUnicodeBOFMarker = 0xfeff; // Little endian unicode Byte Order Mark.First byte:0xff, Second byte: 0xfe.


// Forward declarations
HRESULT ReadWebViewTemplate(LPCTSTR pszIniFile, LPCTSTR pszSection, LPTSTR pszWebViewTemplate, int cchWebViewTemplate);
void DisplayPicture(LPCTSTR lpszImageFileName, HWND hwndSubclassed, LPCSTR lpszStartHtml, LPCSTR lpszEndHtml);

#ifdef UNICODE
DWORD SHGetIniStringUTF7(LPCWSTR pwszSection, LPCWSTR pwszKey, LPWSTR pBuf, DWORD nSize, LPCWSTR pwszFile)
{
    DWORD dwRet;
    if (pwszKey && *pwszKey == SZ_CANBEUNICODE[0])
    {
        dwRet = SHGetIniString(pwszSection, pwszKey+1, pBuf, nSize, pwszFile);
    }
    else
    {
        dwRet = GetPrivateProfileString(pwszSection, pwszKey, TEXT(""), pBuf, nSize, pwszFile);
    }
    return dwRet;
}

BOOL SHSetIniStringUTF7(LPCWSTR pwszSection, LPCWSTR pwszKey, LPCWSTR pwszString, LPCWSTR pwszFile)
{
    BOOL bRet;
    if (pwszKey && *pwszKey == SZ_CANBEUNICODE[0])
    {
        bRet = SHSetIniString(pwszSection, pwszKey+1, pwszString, pwszFile);
    }
    else
    {
        bRet = WritePrivateProfileString(pwszSection, pwszKey, pwszString, pwszFile);
    }
    return bRet;
}
#endif

// Displays an error message box.
void DisplayFolderCustomizationError(UINT nError)
{
    TCHAR szTitle[MAX_PATH];
    LoadString(g_hAppInst, IDS_TITLE, szTitle, ARRAYSIZE(szTitle));

    TCHAR szMessage[MAX_PATH];
    LoadString(g_hAppInst, nError, szMessage, ARRAYSIZE(szMessage));

    MessageBox(NULL, szMessage, szTitle, MB_OK | MB_SETFOREGROUND | MB_ICONSTOP);
}

// The pszGUID buffer should be atleast GUIDSTR_MAX chars long.
void TCharStringFromGUID(const GUID& guid, TCHAR* pszGUID)
{
    OLECHAR wszOleGUID[GUIDSTR_MAX];
    StringFromGUID2(guid, wszOleGUID, ARRAYSIZE(wszOleGUID));
    SHUnicodeToTChar(wszOleGUID, pszGUID, GUIDSTR_MAX);
}

HRESULT EnsureFSObjectPresenceWithErrorUI(LPCTSTR pszFile, DWORD dwAccess, DWORD dwAttributes)
{
    HRESULT hres = E_FAIL;
    if (dwAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        if (PathIsDirectory(pszFile)    // A dir with the same name already exists. Good.
                || CreateDirectory(pszFile, NULL))  // We can create it now.
        {
            hres = S_OK;
        }
    }
    else
    {
        HANDLE hFile = CreateFile(pszFile, dwAccess, 0, NULL, OPEN_ALWAYS, dwAttributes, NULL);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            CloseHandle(hFile);
            hres = S_OK;
        }
    }
    if (FAILED(hres))
    {
        // Report the error.
        DisplayFolderCustomizationError(IDS_ACCESSERROR);
    }
    return hres;
}

void GetRelativePath(LPCTSTR pszDir, LPTSTR pszPath)
{
    TCHAR szTemp[MAX_PATH];
    PathRelativePathTo(szTemp, pszDir, FILE_ATTRIBUTE_DIRECTORY, pszPath, 0);
    PathCanonicalize(pszPath, szTemp);
}

HRESULT EnsureDirResourceObjectPresenceWithErrorUI(LPCTSTR pszDir, LPTSTR pszResourceObjectPath)
{
    BOOL    fSuccess;
    
    // Make sure there is FILE_FOLDER_SETTINGS folder within pszDir.
    lstrcpyn(pszResourceObjectPath, pszDir, MAX_PATH);
    if (IsLFNDrive(pszResourceObjectPath))
    {
        fSuccess = PathAppend(pszResourceObjectPath, FILE_FOLDER_SETTINGS);
    }
    else
    {
        fSuccess = PathAppend(pszResourceObjectPath, FILE_FOLDER_SETTINGS_SHORT);
    }

    if (!fSuccess)
        return E_FAIL;
    
    HRESULT  hres = EnsureFSObjectPresenceWithErrorUI(pszResourceObjectPath, GENERIC_WRITE,
            FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_DIRECTORY);
    if (SUCCEEDED(hres))
    {
        // Make sure FILE_FOLDER_SETTINGS is a super hidden folder.
        if(!SetFileAttributes(pszResourceObjectPath, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM))
            hres = E_FAIL;
    }
    return hres;
}

HRESULT MakeFileLocalCopy(LPCTSTR pszSrcFilePath, LPCTSTR pszFolderPath, LPTSTR pszDestFile)
{
    HRESULT hres = E_FAIL;
    TCHAR szTemp[MAX_PATH];
    // The local-copy should go in the directory resource object.
    if (SUCCEEDED(EnsureDirResourceObjectPresenceWithErrorUI(pszFolderPath, szTemp)))
    {
        //PathAppend can fail if the path is too deep!
        if(PathAppend(szTemp, pszDestFile) &&
          (lstrcmpi(pszSrcFilePath, szTemp) != 0))
        {
            SetFileAttributes(szTemp, FILE_ATTRIBUTE_NORMAL);   // Make sure it is writable.
            if (CopyFile(pszSrcFilePath, szTemp, FALSE))
            {
                lstrcpy(pszDestFile, szTemp);
                hres = S_OK;
            }
            else
            {
                int i = GetLastError();
                // Report the error.
                DisplayFolderCustomizationError(IDS_ACCESSERROR);
            }
        }
    }
    return hres;
}

HRESULT UpdateLVBackgroundImageSettings(LPCTSTR pszBackgroundImage, LPCTSTR pszFolderPath, LPCTSTR pszIni)
{
    TCHAR szGUID[GUIDSTR_MAX];
    TCharStringFromGUID(VID_FolderState, szGUID);

    WritePrivateProfileString(TEXT("ExtShellFolderViews"), TEXT("Default"), NULL, pszIni);
    WritePrivateProfileString(TEXT("ExtShellFolderViews"), szGUID, szGUID, pszIni);
    WritePrivateProfileString(szGUID, TEXT("Attributes"), TEXT("1"), pszIni);
    // Write out the relative path for the background image file in the ini file
    TCHAR szRelativePath[MAX_PATH];
    lstrcpyn(szRelativePath, pszBackgroundImage, ARRAYSIZE(szRelativePath));
    GetRelativePath(pszFolderPath, szRelativePath);
    SHSetIniString(szGUID, TEXT("IconArea_Image"), szRelativePath, pszIni);
    return S_OK;
}

BOOL IsBackgroundImageSet()
{
    TCHAR szGUID[GUIDSTR_MAX];
    TCharStringFromGUID(VID_FolderState, szGUID);
    TCHAR szNone[MAX_PATH];
    LoadString(g_hAppInst, IDS_ENTRY_NONE, szNone, ARRAYSIZE(szNone));
    TCHAR szTemp[30];
    return SHGetIniString(szGUID, TEXT("IconArea_Image"), szTemp, ARRAYSIZE(szTemp), g_szIniFile)
            && (lstrcmpi(szTemp, szNone) != 0);
}

void RemoveBackgroundImage()
{
    TCHAR szNone[MAX_PATH];
    LoadString(g_hAppInst, IDS_ENTRY_NONE, szNone, ARRAYSIZE(szNone));
    UpdateChangeBitmap(szNone);
}

BOOL IsIconTextColorSet()
{
    TCHAR szGUID[GUIDSTR_MAX];
    TCharStringFromGUID(VID_FolderState, szGUID);

    TCHAR szTemp[30];
    return GetPrivateProfileString(szGUID, TEXT("IconArea_Text"), TEXT(""),
            szTemp, ARRAYSIZE(szTemp), g_szIniFile)
            || GetPrivateProfileString(szGUID, TEXT("IconArea_TextBackground"), TEXT(""),
            szTemp, ARRAYSIZE(szTemp), g_szIniFile);
}

void RestoreIconTextColor()
{
    TCHAR szGUID[GUIDSTR_MAX];
    TCharStringFromGUID(VID_FolderState, szGUID);

    WritePrivateProfileString(szGUID, TEXT("IconArea_Text"), NULL, g_szIniFile);
    WritePrivateProfileString(szGUID, TEXT("IconArea_TextBackground"), NULL, g_szIniFile);
}

BOOL IsFolderCommentSet()
{
    TCHAR szTemp[30];
    return SHGetIniStringUTF7(STR_SHELLCLASSINFO, STR_HTMLINFOTIPFILE, szTemp, ARRAYSIZE(szTemp), g_szIniFile) > 0;
}

void RemoveFolderComment()
{
    UpdateComment(NULL);
}

BOOL IsWebViewTemplateSet()
{
    TCHAR szVID[GUIDSTR_MAX];
    TCharStringFromGUID(VID_WebView, szVID);
    TCHAR szTemp[30];
    return SUCCEEDED(ReadWebViewTemplate(g_szIniFile, szVID, szTemp, ARRAYSIZE(szTemp)));
}

void UpdateChangeBitmap(LPCTSTR pszImageFile)
{
    // Ensure that the INI file is present and we can get write access.
    if (SUCCEEDED(EnsureFSObjectPresenceWithErrorUI(g_szIniFile, GENERIC_WRITE, FILE_ATTRIBUTE_NORMAL)))
    {
        TCHAR szGUID[GUIDSTR_MAX];
        TCharStringFromGUID(VID_FolderState, szGUID);

        TCHAR szNone[MAX_PATH];
        LoadString(g_hAppInst, IDS_ENTRY_NONE, szNone, ARRAYSIZE(szNone));

        TCHAR szTemp2[MAX_PATH];
        SHGetIniString(szGUID, TEXT("IconArea_Image"), szTemp2, ARRAYSIZE(szTemp2), g_szIniFile);
        if (!*szTemp2)
            lstrcpy(szTemp2, szNone);
        PathCombine(szTemp2, g_szCurFolder, szTemp2);

        if (lstrcmpi(pszImageFile, szNone) == 0)    // Remove the entry for the background image file from the ini file.
        {
            SHSetIniString(szGUID, TEXT("IconArea_Image"), NULL, g_szIniFile);
        }
        else
        {
            TCHAR szTemp[MAX_PATH];
            // Get the dest file name
            if (IsLFNDrive(g_szCurFolder))
            {
                lstrcpyn(szTemp, FILE_BACKGROUND_IMAGE, ARRAYSIZE(szTemp));
            }
            else
            {
                lstrcpyn(szTemp, FILE_BACKGROUND_IMAGE_SHORT, ARRAYSIZE(szTemp));
            }
            TCHAR* pszExt = PathFindExtension(pszImageFile);
            if (pszExt)
            {
                StrCatBuff(szTemp, pszExt, ARRAYSIZE(szTemp));
            }
            // The local-copy of the background image should go in the directory resource object.
            if (SUCCEEDED(MakeFileLocalCopy(pszImageFile, g_szCurFolder, szTemp)))
            {
                // Delete the old image file if it is not the same as the new one.
                if (lstrcmpi(szTemp2, szTemp) != 0)
                {
                    DeleteFile(szTemp2);
                }
                UpdateLVBackgroundImageSettings(szTemp, g_szCurFolder, g_szIniFile);
            }
        }
        UpdateGlobalFolderInfo(UPDATE_BITMAP, szGUID);
    }
}

const LPCTSTR c_szWebViewTemplateVersions[] =
{
    STR_WEBVIEWTEMPLATE_KEY TEXT(".NT5"),
    TEXT("PersistMoniker"),
};

HRESULT RemoveWebViewTemplateSettings()
{
    SHFOLDERCUSTOMSETTINGS fcs = {sizeof(fcs), FCSM_WEBVIEWTEMPLATE, 0};
    fcs.pszWebViewTemplate = NULL;   // template path
    return SHGetSetFolderCustomSettings(&fcs, g_szCurFolder, FCS_FORCEWRITE);
}

HRESULT ReadWebViewTemplate(LPCTSTR pszIniFile, LPCTSTR pszSection, LPTSTR pszWebViewTemplate, int cchWebViewTemplate)
{
    SHFOLDERCUSTOMSETTINGS fcs = {sizeof(fcs), FCSM_WEBVIEWTEMPLATE, 0};
    fcs.pszWebViewTemplate = pszWebViewTemplate;
    fcs.cchWebViewTemplate = cchWebViewTemplate;
    return SHGetSetFolderCustomSettings(&fcs, g_szCurFolder, FCS_READ);
}

HRESULT GetWebViewTemplateKeyVersion(LPCTSTR pszKey, LPTSTR pszKeyVersion, int cchKeyVersion)
{
    HRESULT hr = E_INVALIDARG;
    if (pszKeyVersion)
    {
        pszKeyVersion[0] = TEXT('\0');

        TCHAR szTemp[MAX_PATH];
        wnsprintf(szTemp, ARRAYSIZE(szTemp), TEXT("%s\\%s"), REG_WEBVIEW_TEMPLATES, pszKey);

        HKEY hkeyTemplate;
        if (pszKey && pszKey[0] && RegOpenKeyEx(HKEY_LOCAL_MACHINE, szTemp, 0, KEY_READ, &hkeyTemplate) == ERROR_SUCCESS)
        {
            TCHAR szTemp2[MAX_PATH];
            DWORD cbTemp2 = sizeof(szTemp2);
            if (RegQueryValueEx(hkeyTemplate, REG_VAL_VERSION, NULL, NULL, (LPBYTE)szTemp2, &cbTemp2) == ERROR_SUCCESS
                    && StrCmpNI(szTemp2, STR_PERSISTMONIKER_VERSION, ARRAYSIZE(STR_PERSISTMONIKER_VERSION)) != 0)
            {
                lstrcpyn(pszKeyVersion, szTemp2, cchKeyVersion);
            }
            RegCloseKey(hkeyTemplate);
        }
        hr = S_OK;
    }
    return hr;
}

TCHAR const c_szWebDir[] = TEXT("%WebDir%");
BOOL SubstituteWebDir(LPTSTR pszFile, int cch)
{
    BOOL fRet = FALSE;
    TCHAR szWebDirPath[MAX_PATH];
    if (SUCCEEDED(SHGetWebFolderFilePath(FILE_WEBVIEW_TEMPLATE, szWebDirPath, ARRAYSIZE(szWebDirPath))))
    {
        PathRemoveFileSpec(szWebDirPath);

        LPTSTR pszTemp = StrStrI(pszFile, szWebDirPath);
        if (pszTemp)
        {
            StrCpy(pszTemp, c_szWebDir);
            PathAppend(pszTemp, pszTemp + lstrlen(szWebDirPath));
            fRet = TRUE;
        }
    }
    return fRet;
}

HRESULT UpdateWebViewTemplateSettings(LPCTSTR pszKey, LPCTSTR pszWebViewTemplatePath, LPCTSTR pszPreviewBitmap, LPCTSTR pszIni)
{
    TCHAR szVID[GUIDSTR_MAX];
    TCharStringFromGUID(VID_WebView, szVID);

    TCHAR szTemp[MAX_PATH];
    StrCpyN(szTemp, pszWebViewTemplatePath, ARRAYSIZE(szTemp));

    TCHAR szTemp2[MAX_PATH];
    GetWebViewTemplateKeyVersion(pszKey, szTemp2, ARRAYSIZE(szTemp2));

    // Update the Ini file.
    SHFOLDERCUSTOMSETTINGS fcs = {sizeof(fcs), FCSM_WEBVIEWTEMPLATE, 0};
    fcs.pszWebViewTemplate = szTemp;
    fcs.pszWebViewTemplateVersion = szTemp2;
    HRESULT hres = SHGetSetFolderCustomSettings(&fcs, g_szCurFolder, FCS_FORCEWRITE);

    // Update the template preview.
    if (SUCCEEDED(hres) && pszPreviewBitmap && pszPreviewBitmap[0])
    {
        lstrcpyn(szTemp2, pszPreviewBitmap, ARRAYSIZE(szTemp2));
        if (!SubstituteWebDir(szTemp2, ARRAYSIZE(szTemp2)))
        {
            PathUnExpandEnvStrings(pszPreviewBitmap, szTemp2, ARRAYSIZE(szTemp2));  // Encode the path with env vars
        }
        WritePrivateProfileString(szVID, TEXT("PersistMonikerPreview"), szTemp2, pszIni);
    }
    return hres;
}

void UpdateAddWebView()
{
    // Make sure FILE_FOLDER_SETTINGS is present and we can get write access.
    if (SUCCEEDED(EnsureFSObjectPresenceWithErrorUI(g_szIniFile, GENERIC_WRITE, FILE_ATTRIBUTE_NORMAL)))
    {
        // Copy over the supporting files as well.
        HRESULT hres = S_OK;
        HKEY hkeyTemplate;
        TCHAR szTemp[MAX_PATH], szTemp2[MAX_PATH];
        wnsprintf(szTemp, ARRAYSIZE(szTemp), TEXT("%s\\%s"), REG_WEBVIEW_TEMPLATES, g_szKey);
        // Make sure this is a template from the registry
        if (g_szKey[0] && RegOpenKeyEx(HKEY_LOCAL_MACHINE, szTemp, 0, KEY_READ, &hkeyTemplate) == ERROR_SUCCESS)
        {
            // Now, copy the supporting files for the template, into FILE_FOLDER_SETTINGS.
            HKEY hkeySupportingFiles;
            if (RegOpenKeyEx(hkeyTemplate, REG_SUPPORTINGFILES, 0, KEY_READ, &hkeySupportingFiles) == ERROR_SUCCESS)
            {
                TCHAR szTemp3[50]; // 50 is a reasonable limit on a file name.
                // The local-copies of the template resources should go in the directory resource object.
                if (SUCCEEDED(EnsureDirResourceObjectPresenceWithErrorUI(g_szCurFolder, szTemp2)))
                {
                    DWORD cchTemp3 = ARRAYSIZE(szTemp3), cbTemp = sizeof(szTemp);
                    for (int i = 0; RegEnumValue(hkeySupportingFiles, i, szTemp3, &cchTemp3,
                            NULL, NULL, (LPBYTE)szTemp, &cbTemp) == ERROR_SUCCESS;
                            cchTemp3 = ARRAYSIZE(szTemp3), cbTemp = sizeof(szTemp), i++)
                    {
                        TCHAR szDestFilePath[MAX_PATH];
                        PathCombine(szDestFilePath, szTemp2, szTemp3);
                        SetFileAttributes(szDestFilePath, FILE_ATTRIBUTE_NORMAL);   // Make sure it is writable.
                        if (!CopyFile(szTemp, szDestFilePath, FALSE))
                        {
                            // Report the error.
                            DisplayFolderCustomizationError(IDS_ACCESSERROR);
                            hres = E_FAIL;
                            break;
                        }
                    }
                }
                RegCloseKey(hkeySupportingFiles);
            }
            RegCloseKey(hkeyTemplate);
        }
        if (SUCCEEDED(hres))
        {
            if (g_iFlagA != SYSTEM_TEMPLATE)
            {
                lstrcpy(szTemp, FILE_WEBVIEW_TEMPLATE);
                hres = MakeFileLocalCopy(g_szFullHTMLFile, g_szCurFolder, szTemp);
            }
            if (SUCCEEDED(hres) && g_szKey[0])
            {
                if (IsLFNDrive(g_szIniFile))
                {
                    lstrcpyn(szTemp, FILE_PROTOCOL FILE_FOLDER_SETTINGS TEXT("\\") FILE_WEBVIEW_TEMPLATE, ARRAYSIZE(szTemp));
                }
                else
                {
                    lstrcpyn(szTemp, FILE_PROTOCOL FILE_FOLDER_SETTINGS_SHORT TEXT("\\") FILE_WEBVIEW_TEMPLATE, ARRAYSIZE(szTemp));
                }
                hres = UpdateWebViewTemplateSettings(g_szKey, (g_iFlagA == SYSTEM_TEMPLATE) ? g_szTemplateFile : szTemp, g_szPreviewBitmapFile, g_szIniFile);
            }
        }
        if (g_iFlagA != SYSTEM_TEMPLATE)
        {
            DeleteFile(g_szFullHTMLFile);   // Delete this temp file
        }
        if (SUCCEEDED(hres))
        {
            UpdateGlobalFolderInfo(UPDATE_WEB_VIEW, NULL);
        }
    }
}

void GetTemporaryTemplatePath(LPTSTR pszTemplate)
{
    GetTempPath(MAX_PATH, pszTemplate);
    PathAppend(pszTemplate, FILE_WEBVIEW_TEMPLATE);
}

void ChooseShortcutColor (HWND hWnd, SHORTCUTCOLOR *sccColor)
{
    CHOOSECOLOR ccColors;
    memset(&ccColors, 0, sizeof(CHOOSECOLOR));

    ccColors.hwndOwner      = hWnd;
    ccColors.lStructSize    = sizeof(CHOOSECOLOR);
    ccColors.rgbResult      = sccColor->crColor;
    ccColors.lpCustColors   = g_crCustomColors;
    ccColors.Flags          = CC_RGBINIT | CC_SOLIDCOLOR; // | CC_PREVENTFULLOPEN;
    ccColors.lCustData      = 0L;

    if (ChooseColor(&ccColors))
    {
        sccColor->iChanged = TRUE;
        sccColor->crColor = ccColors.rgbResult;
        InvalidateRect(NULL, NULL, FALSE);
    }

}


void UpdateShortcutColors (LPCTSTR szGUID)
{
    TCHAR szTempVal[20];
    BOOL bEnsureFolderStateSettings = FALSE;

    if (ShortcutColorText.iChanged)
    {
        wnsprintf(szTempVal, ARRAYSIZE(szTempVal), TEXT("0x%8.8X"), ShortcutColorText.crColor);
        WritePrivateProfileString(szGUID, TEXT("IconArea_Text"), szTempVal, g_szIniFile);
        bEnsureFolderStateSettings = TRUE;
    }

    if (ShortcutColorBkgnd.iChanged)
    {
        wnsprintf(szTempVal, ARRAYSIZE(szTempVal), TEXT("0x%8.8X"), ShortcutColorBkgnd.crColor);
        WritePrivateProfileString(szGUID, TEXT("IconArea_TextBackground"), szTempVal, g_szIniFile);
        bEnsureFolderStateSettings = TRUE;
    }
    else
    {
        WritePrivateProfileString(szGUID, TEXT("IconArea_TextBackground"), NULL, g_szIniFile);
    }
    if (bEnsureFolderStateSettings)
    {
        WritePrivateProfileString(TEXT("ExtShellFolderViews"), szGUID, szGUID, g_szIniFile);
        WritePrivateProfileString(szGUID, TEXT("Attributes"), TEXT("1"), g_szIniFile);
    }
}

void SetRequiredAttributes()
{
    SetFileAttributes(g_szIniFile, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);

    // PathMakeSystemFolder() special cases the windir and system dir
    PathMakeSystemFolder(g_szCurFolder);
}

void UpdateGlobalFolderInfo (int flag, LPCTSTR szGUID)
{
    SetRequiredAttributes();
    if (flag == UPDATE_BITMAP && szGUID)
        UpdateShortcutColors(szGUID);
    else
        SetFileAttributes(g_szFullHTMLFile, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);

    WritePrivateProfileString(STR_SHELLCLASSINFO, TEXT("ConfirmFileOp"), TEXT("0"), g_szIniFile);
}

BOOL CopyTemplate()
{
    DWORD buffer_size = 0, bytes_processed;
    HANDLE hTemplateSource, hTemplateTarget;
    void *file_buffer = NULL;

    if (!TemplateExists(g_szTemplateFile))
    {
        RestoreMasterTemplate(g_szTemplateFile);
    }

    hTemplateSource = CreateFile(g_szTemplateFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hTemplateSource == INVALID_HANDLE_VALUE)
        return(FALSE);

    // CreateFile() fails if the file already exists and has the RO bit set. So, first set the attribute to normal.
    SetFileAttributes(g_szFullHTMLFile, FILE_ATTRIBUTE_NORMAL);

    hTemplateTarget = CreateFile(g_szFullHTMLFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hTemplateTarget == INVALID_HANDLE_VALUE)
    {
        DWORD dwError = GetLastError();
        CloseHandle(hTemplateSource);
        return(FALSE);
    }
    
    if ((file_buffer = malloc((buffer_size = GetFileSize(hTemplateSource, NULL)))) == (void *)NULL)
    {
        CloseHandle(hTemplateSource);
        CloseHandle(hTemplateTarget);
        return(FALSE);
    }

    ReadFile(hTemplateSource, file_buffer, buffer_size, &bytes_processed, NULL);
    WriteFile(hTemplateTarget, file_buffer, buffer_size, &bytes_processed, NULL);

    free(file_buffer);

    CloseHandle(hTemplateSource);
    CloseHandle(hTemplateTarget);

    g_bTemplateCopied = TRUE;

    return(TRUE);
}


BOOL TemplateExists (LPCTSTR szTemplatePath)
{
    return GetFileAttributes(szTemplatePath) != 0xFFFFFFFF;
}


void LaunchRegisteredEditor (void)
{
    //Launch Editor
    SHELLEXECUTEINFO rgInfo;
    rgInfo.cbSize = sizeof(rgInfo);
    rgInfo.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI | SEE_MASK_FLAG_DDEWAIT;
    rgInfo.hwnd = NULL;
    rgInfo.lpVerb = TEXT("edit");
    rgInfo.lpFile = g_szFullHTMLFile;
    rgInfo.lpParameters = NULL;
    rgInfo.lpDirectory = NULL;
    rgInfo.nShow = SW_NORMAL;

    SetFileAttributes(g_szFullHTMLFile, FILE_ATTRIBUTE_NORMAL);  // Originally done for FPE, now do this for TPVs (Third Party Vendors)

    if (ShellExecuteEx(&rgInfo)) {
        while (MsgWaitForMultipleObjects(1, &rgInfo.hProcess, FALSE, INFINITE, QS_ALLINPUT) == (WAIT_OBJECT_0 + 1)) {
            MSG msg;
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) { 
                if (msg.message == WM_PAINT)  
                    DispatchMessage(&msg); 
            }
        }
        CloseHandle(rgInfo.hProcess);
    } else
        LaunchNotepad();            // resort to notepad if the registered app couldn't be launched ...

    SetFileAttributes(g_szFullHTMLFile, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
    SHRegisterValidateTemplate(g_szFullHTMLFile, SHRVT_REGISTER);
}


void LaunchNotepad (void)
{
    TCHAR szTemp[MAX_PATH];

    LoadString(g_hAppInst, IDS_NOTEPAD, szTemp, ARRAYSIZE(szTemp));

    lstrcat(szTemp, g_szFullHTMLFile);

    STARTUPINFO si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(STARTUPINFO);
    si.lpReserved = NULL;
    si.lpReserved2 = NULL;
    si.cbReserved2 = 0;
    si.lpDesktop = NULL;
    si.dwFlags = 0;
    
    CreateProcess(NULL, szTemp, NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);
    CloseHandle(pi.hThread);

    while (MsgWaitForMultipleObjects(1, &pi.hProcess, FALSE, INFINITE, QS_ALLINPUT) == (WAIT_OBJECT_0 + 1)) {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) { 
            if (msg.message == WM_PAINT)  
                DispatchMessage(&msg); 
        }
    }

    CloseHandle(pi.hProcess);

}


void RestoreMasterTemplate (LPCTSTR szTemplatePath)
{
    HINSTANCE hInstance;
    HRSRC     hRsrc;
    HGLOBAL   hGlobal;
    HANDLE    hTemplateTarget;
    DWORD     bytes_processed;
    void      *buffer;
    TCHAR     szTemp[MAX_PATH];

    LoadString(g_hAppInst, IDS_WEBVIEW_DLL, szTemp, ARRAYSIZE(szTemp));
    hInstance = LoadLibraryEx(szTemp, NULL, LOAD_LIBRARY_AS_DATAFILE);

    LoadString(g_hAppInst, IDS_FOLDER, szTemp, ARRAYSIZE(szTemp));
    hRsrc = FindResource(hInstance, szTemp, RT_HTML);

    hGlobal = LoadResource(hInstance, hRsrc);
    buffer = LockResource(hGlobal);

    hTemplateTarget = CreateFile(szTemplatePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hTemplateTarget == INVALID_HANDLE_VALUE)
        return;

    WriteFile(hTemplateTarget, buffer, SizeofResource(hInstance, hRsrc), &bytes_processed, NULL);
    CloseHandle(hTemplateTarget);

}


#define MAX_HTML_ESCAPE_SEQUENCE 8  // longest string representation of a 16 bit integer is 65535.  So, entire composite escape string has:
                                    // 2 for "&#" + maximum of 5 digits + 1 for ";" = 8 characters

/*
 * UnicodeToHTMLEscapeStringAnsi
 *
 * Takes a unicode string as the input source and translates it into an ansi string that mshtml can process.  Characters > 127 will be
 * translated into an html escape sequence that has the following syntax:  "&#xxxxx;" where xxxxx is the string representation of the decimal
 * integer which is the value for the unicode character.  In this manner we are able to generate HTML text which represent UNICODE characters.
 */
void UnicodeToHTMLEscapeStringAnsi(LPCWSTR pwszSrc, LPSTR pszDest, int cchDest)
{
    while (*pwszSrc && (cchDest > MAX_HTML_ESCAPE_SEQUENCE))
    {
        int iLen;
        ULONG ul = MAKELONG(*pwszSrc, 0);

        // We can optimize the common ansi characters to avoid generating the long escape sequence.  This allows us to fit
        // longer paths in the buffer.
        if (ul < 128)
        {
            *pszDest = (CHAR)*pwszSrc;
            iLen = 1;
        }
        else
        {
            iLen = wsprintfA(pszDest, "&#%lu;", ul);
        }
        pszDest += iLen;
        cchDest -= iLen;
        pwszSrc++;
    }
    *pszDest = 0;
}

/************************************************************
 void DisplayPicture(LPCTSTR lpszImageFileName, HWND hwndSubclassed, LPCSTR lpszStartHtml, LPCSTR lpszEndHtml):
 Displays the file mentioned in hwnd, which should have been subclassed with ThumbNailSubClassWndProc.
 
 Parameters: lpszImageFileName: Name of the file displayed
             hwndSubclassed:    Handle to the window in which the image is displayed.
                                This should have been subclassed with ThumbNailSubClassWndProc
             lpszStartHtml:     The start string for the generated HTML file
                If lpszImageFileName is not NULL, it is inserted in the HTML file, after lpszStartHtml
             lpszEndHtml:       The end string for the generated HTML file
 Return Value : void
 ************************************************************/
 
void DisplayPicture(LPCTSTR lpszImageFileName, HWND hwndSubclassed, LPCSTR lpszStartHtml, LPCSTR lpszEndHtml)
{
    TCHAR szTitle[MAX_PATH];
    TCHAR szMessage[MAX_PATH];
    TCHAR szTempFileName[MAX_PATH];
    static unsigned int uiTempFileNo = 0;

    GetTempPath(ARRAYSIZE(szMessage), szMessage);
    GetLongPathName(szMessage, szTempFileName, ARRAYSIZE(szTempFileName));
    wnsprintf(szTitle, ARRAYSIZE(szTitle), TEXT("shwiz%u.htm"), uiTempFileNo++ % 20);
    PathAppend(szTempFileName, szTitle);

    HANDLE  hFile;
    hFile = CreateFile(szTempFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        // report the error ..
        LoadString(g_hAppInst, IDS_TITLE, szTitle, ARRAYSIZE(szTitle));
        LoadString(g_hAppInst, IDS_ERR_FILEOPEN, szMessage, ARRAYSIZE(szMessage));
        MessageBox(NULL, szMessage, szTitle, MB_OK | MB_SETFOREGROUND | MB_ICONSTOP);
    }
    else
    {
        TCHAR szTemp[MAX_PATH];
        if (lpszImageFileName)
        {
            lstrcpyn(szTemp, lpszImageFileName, ARRAYSIZE(szTemp));
        }
        else
        {
            szTemp[0] = TEXT('\0');
        }

        CHAR szTemp2[MAX_PATH];
#ifdef UNICODE
        UnicodeToHTMLEscapeStringAnsi(szTemp, szTemp2, ARRAYSIZE(szTemp2));
#else
        SHTCharToAnsi(szTemp, szTemp2, ARRAYSIZE(szTemp2));
#endif
        DWORD dwBytesProcessed;
        WriteFile(hFile, lpszStartHtml, lstrlenA(lpszStartHtml), &dwBytesProcessed, NULL);
        WriteFile(hFile, szTemp2, lstrlenA(szTemp2), &dwBytesProcessed, NULL);
        WriteFile(hFile, lpszEndHtml, lstrlenA(lpszEndHtml), &dwBytesProcessed, NULL);
        CloseHandle(hFile);

        ShowPreview(szTempFileName, hwndSubclassed);
    }
}


void DisplayBackground(LPCTSTR lpszImageFileName, HWND hwndSubclassed)
{
    // The following variables must not be Unicode since HTML files cannot be Unicode
    char szStartHtml[] = "<HTML><BODY BACKGROUND=\"";
    char szEndHtml[] = "\"><BR CLEAR=ALL></BR></BODY></HTML>";
    DisplayPicture(lpszImageFileName, hwndSubclassed, szStartHtml, szEndHtml);
}


void DisplayPreview(LPCTSTR lpszImageFileName, HWND hwndSubclassed)
{
    RECT rcClient;
    GetClientRect(hwndSubclassed, &rcClient);

    // The following variables must not be Unicode since HTML files cannot be Unicode
    //bugbug - should change #c0c0c0 below to reflect user's system 3dface color
    char szStartHtml[] = "<HTML><BODY style=\"border:none;\" topmargin=0 leftmargin=0 rightmargin=0 bottommargin=0>\r\n<IMG src=\"";
    char szEndHtml[] = "\" style=\"position: absolute; left:0; top:0; width: 100%; height: 100%;\">\r\n</BODY></HTML>";
    DisplayPicture(lpszImageFileName, hwndSubclassed, szStartHtml, szEndHtml);
}


void InstallFileFromResource(LPCTSTR pszSource, LPCTSTR pszDestFile)
{
    HRSRC hRsrc;

    hRsrc = FindResource(g_hAppInst, pszSource, RT_HTML);
    if (hRsrc)
    {
        HANDLE hDestFile = CreateFile(pszDestFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL); 
        if (hDestFile != INVALID_HANDLE_VALUE)
        {
            HGLOBAL hGlob = LoadResource(g_hAppInst, hRsrc);
            LPVOID pTemp = LockResource(hGlob);
            if (pTemp)
            {
                ULONG cbWritten, cbResource = SizeofResource(g_hAppInst, hRsrc);
                WriteFile(hDestFile, pTemp, cbResource, &cbWritten, NULL);
            }
            CloseHandle(hDestFile);
        }
    }
}

void InstallUnknownHTML(LPTSTR pszTempFileName, int cchTempFileName, BOOL bForce)
{
    TCHAR szTempFileName[MAX_PATH];
    if (!pszTempFileName || cchTempFileName <= 0)
    {
        pszTempFileName = szTempFileName;
        cchTempFileName = ARRAYSIZE(szTempFileName);
    }
    TCHAR szUnknown[] = TEXT("unknown.htm");
    GetTempPath(cchTempFileName, pszTempFileName);
    PathAppend(pszTempFileName, szUnknown);

    // Do this only if pszDestFile doesn't already exist or we are forced to install
    if (bForce || GetFileAttributes(pszTempFileName) == 0xFFFFFFFF)
    {
        InstallFileFromResource(szUnknown, pszTempFileName);
    }
}


void DisplayUnknown(HWND hwndSubclassed)
{
    TCHAR szTempFileName[MAX_PATH];
    InstallUnknownHTML(szTempFileName, ARRAYSIZE(szTempFileName), FALSE);
    ShowPreview(szTempFileName, hwndSubclassed);
}


// Displays a preview of an empty html in hwnd,
// which should have been subclassed with ThumbNailSubClassWndProc.
// Parameters: lpszImageFileName: Name of the file displayed
//             hwndSubclassed:    Handle to the window in which the image is displayed.
//                                This should have been subclassed with ThumbNailSubClassWndProc
// Return Value : void
void DisplayNone(HWND hwndSubclassed)
{
    // The following variables must not be Unicode since HTML files cannot be Unicode
    //bugbug - should change #c0c0c0 below to reflect user's system 3dface color
    char szStartHtml[] = "<HTML><BODY BGCOLOR=#C0C0C0>";
    char szEndHtml[] = "</BODY></HTML>";
    DisplayPicture(NULL, hwndSubclassed, szStartHtml, szEndHtml);
}


// Shows the preview of the HtmlFile lpszFileName in hwnd,
// which should have been subclassed with ThumbNailSubClassWndProc.
void ShowPreview(LPCTSTR lpszFileName, HWND hwndSubclassed)
{
    SendMessage(hwndSubclassed, WM_SETIMAGE, 0, (LPARAM)lpszFileName);
}


// Subclassing hwnd with ThumbNailSubClassWndProc. The original WndProc is saved in g_lpThumbnailWndProc
void Subclass(HWND hwnd)
{
    if (!g_lpThumbnailWndProc)  // It is not subclassed by anyone else.
    {
        // Subclass hwnd
        g_lpThumbnailWndProc = (WNDPROC)GetWindowLongPtr(hwnd, GWLP_WNDPROC);
        if (g_lpThumbnailWndProc)
        {
            SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)ThumbNailSubClassWndProc);
            // Initialize the subclass
            SendMessage(hwnd, WM_INITSUBPROC, 0, 0);
        }
    }
}


// Resets subclassing. The original WndProc should be in g_lpThumbnailWndProc
void Unsubclass(HWND hwnd)
{
    if (g_lpThumbnailWndProc)  // It is subclassed.
    {
        SendMessage(hwnd, WM_UNINITSUBPROC, 0, 0);
        SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)g_lpThumbnailWndProc);
        g_lpThumbnailWndProc = NULL;
    }
}


void ExpandWebDir(LPTSTR pszFile, int cch)
{
    //Check if the given string has %WebDir%
    LPTSTR psz = StrChr(pszFile, TEXT('%'));
    if (psz)
    {
        if (!StrCmpNI(psz, c_szWebDir, ARRAYSIZE(c_szWebDir) - 1))
        {
            LPTSTR pszFileName = PathFindFileName(pszFile);
            if (pszFileName && (pszFileName != psz))
            {
                TCHAR szTempBuff[MAX_PATH];
                StrCpyN(szTempBuff, pszFileName, ARRAYSIZE(szTempBuff));
                SHGetWebFolderFilePath(szTempBuff, pszFile, cch);
            }
        }
    }
}

// Returns: PM_NONE if g_szIniFile does not contain a WebViewTemplate
//          PM_LOCAL if g_szIniFile contains a local folder.htt as the WebViewTemplate
//          PM_REMOTE if g_szIniFile contains a remote template as the WebViewTemplate
// If pszFileName is not NULL and cchFileName > 0, then the WebViewTemplate is returned in pszFileName and it's
// PreviewBitmapFileName is returned in pszPreviewFileName
int HasPersistMoniker(LPTSTR pszFileName, int cchFileName, LPTSTR pszPreviewFileName, int cchPreviewFileName)
{
    int iRetVal = PM_NONE;
    TCHAR szTemp[MAX_PATH];
    TCHAR szVID[GUIDSTR_MAX];

    TCharStringFromGUID(VID_WebView, szVID);

    TCHAR szFileName[MAX_PATH];
    if (!pszFileName || cchFileName <= 0)
    {
        pszFileName = szFileName;
        cchFileName = ARRAYSIZE(szFileName);
    }
    HRESULT hr = ReadWebViewTemplate(g_szIniFile, szVID, pszFileName, cchFileName);
    if (pszPreviewFileName && cchPreviewFileName > 0)
    {
        GetPrivateProfileString(szVID, TEXT("PersistMonikerPreview"), TEXT(""), szTemp, ARRAYSIZE(szTemp), g_szIniFile);
        ExpandEnvironmentStrings(szTemp, pszPreviewFileName, cchPreviewFileName);   // This is a path, so expand the env vars in it
        ExpandWebDir(pszPreviewFileName, cchPreviewFileName);
    }
    if (SUCCEEDED(hr) && pszFileName[0])
    {
        lstrcpyn(szTemp, pszFileName, ARRAYSIZE(szTemp));
        LPTSTR pszTemp = szTemp;
        if (StrCmpNI(FILE_PROTOCOL, pszTemp, 7) == 0) // ARRAYSIZE(TEXT("file://"))
        {
            pszTemp += 7;   // ARRAYSIZE(TEXT("file://"))
        }
        // handle relative references...
        PathCombine(pszTemp, g_szCurFolder, pszTemp);

        TCHAR szTemp2[MAX_PATH];
        lstrcpyn(szTemp2, g_szCurFolder, ARRAYSIZE(szTemp2));
        if (IsLFNDrive(szTemp2))
        {
            PathAppend(szTemp2, FILE_FOLDER_SETTINGS);
        }
        else
        {
            PathAppend(szTemp2, FILE_FOLDER_SETTINGS_SHORT);
        }
        PathAppend(szTemp2, FILE_WEBVIEW_TEMPLATE);
        if (lstrcmpi(pszTemp, szTemp2) == 0)
        {
            iRetVal = PM_LOCAL;
            lstrcpyn(pszFileName, szTemp2, cchFileName);
        }
        else
        {
            iRetVal = PM_REMOTE;
        }
    }
    return iRetVal;
}


const TCHAR c_szExploreClass[]  = TEXT("ExploreWClass");
const TCHAR c_szIExploreClass[] = TEXT("IEFrame");
const TCHAR c_szCabinetClass[]  = 
#ifdef IE3CLASSNAME
    TEXT("IEFrame");
#else
    TEXT("CabinetWClass");
#endif
const TCHAR c_szDesktopClass[]  = TEXT("Progman");

BOOL IsFolderWindow(HWND hwnd)
{
    TCHAR szClass[32];

    GetClassName(hwnd, szClass, ARRAYSIZE(szClass));
    return (lstrcmp(szClass, c_szCabinetClass) == 0) || (lstrcmp(szClass, c_szIExploreClass) == 0);
}

BOOL IsNamedWindow(HWND hwnd, LPCTSTR pszClass)
{
    TCHAR szClass[32];

    GetClassName(hwnd, szClass, ARRAYSIZE(szClass));
    return lstrcmp(szClass, pszClass) == 0;
}

BOOL IsExplorerWindow(HWND hwnd)
{
    return IsNamedWindow(hwnd, c_szExploreClass);
}

BOOL IsDesktopWindow(HWND hwnd)

{
    return IsNamedWindow(hwnd, c_szDesktopClass);
}

BOOL CALLBACK Folder_UpdateWebView(HWND hwnd, LPARAM lParam)
{
    if (IsFolderWindow(hwnd) || IsExplorerWindow(hwnd))
    {
        PostMessage(hwnd, WM_COMMAND, SFVIDM_MISC_SETWEBVIEW, lParam);
    }
    return TRUE;
}

#define SHGETSETSETTINGSORDINAL 68  // shell32.dll ordinal for SHGetSetSettings

typedef void (WINAPI *SHGETSETSETTINGSFN)(LPSHELLSTATE pss, DWORD dwMask, BOOL bSet);

BOOL ProdWebViewOn(HWND hwndOwner)
{
    BOOL fRet = TRUE;
    HINSTANCE hShell32 = LoadLibrary(TEXT("shell32.dll"));
    SHGETSETSETTINGSFN pfn = (SHGETSETSETTINGSFN)GetProcAddress(hShell32, (LPCSTR)MAKEINTRESOURCE(SHGETSETSETTINGSORDINAL));
    if (pfn)   // Webview is off. Ask if they want to turn it on.
    {
        SHELLSTATE  ss = {0};
        pfn(&ss, 0, TRUE);   // Force a refresh
        pfn(&ss, SSF_WEBVIEW, FALSE);
        if (!ss.fWebView)
        {
            TCHAR szTitle[MAX_PATH], szMessage[MAX_PATH];
            LoadString(g_hAppInst, IDS_TITLE, szTitle, ARRAYSIZE(szTitle));
            LoadString(g_hAppInst, IDS_WEBVIEW_ON, szMessage, ARRAYSIZE(szMessage));
            if (MessageBox(hwndOwner, szMessage, szTitle, MB_OKCANCEL | MB_SETFOREGROUND
                    | MB_ICONQUESTION | MB_DEFBUTTON1) != IDOK)
            {
                fRet = FALSE;
            }
            else
            {
                ss.fWebView = TRUE;
                pfn(&ss, SSF_WEBVIEW, TRUE);
                EnumWindows(Folder_UpdateWebView, (LPARAM)TRUE);
            }
        }
    }
    return fRet;
}

#ifndef FCIDM_REFRESH
#define FCIDM_REFRESH   0xA220
#endif

BOOL CALLBACK Folder_UpdateAll(HWND hwnd, LPARAM lParam)
{
    if (IsFolderWindow(hwnd) || IsExplorerWindow(hwnd) || IsDesktopWindow(hwnd))
    {
        PostMessage(hwnd, WM_COMMAND, FCIDM_REFRESH, NULL);
    }
    return TRUE;
}

void ForceShellToRefresh(void)
{
    SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH, g_szCurFolder, NULL);
    EnumWindows(Folder_UpdateAll, NULL);
}

const TCHAR g_szCommentWrapBegin[] = TEXT("<body style=\"background:infobackground; color:infotext; margin-top:1; font:menu\">\n");
const TCHAR g_szCommentWrapEnd[] = TEXT("</body>");

HRESULT UpdateComment(LPCTSTR pszHTMLComment)
{
    HRESULT hr = E_FAIL;
    if (pszHTMLComment && pszHTMLComment[0])
    {
        TCHAR szTemp[MAX_PATH];
        if (SUCCEEDED(EnsureDirResourceObjectPresenceWithErrorUI(g_szCurFolder, szTemp))
                && PathAppend(szTemp, FILE_HTMLINFOTIP))
        {
            HANDLE hCommentFile = CreateFile(szTemp, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hCommentFile != INVALID_HANDLE_VALUE)
            {
                DWORD cbWritten;
                DWORD cchToWrite = lstrlen(pszHTMLComment) + 1;
                BOOL bContinue = TRUE;
#ifdef UNICODE
                bContinue = WriteFile(hCommentFile, (LPCSTR)&g_wchUnicodeBOFMarker, SIZEOF(g_wchUnicodeBOFMarker), &cbWritten, NULL);
#endif
                if (bContinue && WriteFile(hCommentFile, (LPCVOID)g_szCommentWrapBegin, lstrlen(g_szCommentWrapBegin) * SIZEOF(TCHAR), &cbWritten, NULL)
                        && WriteFile(hCommentFile, (LPCVOID)pszHTMLComment, lstrlen(pszHTMLComment) * SIZEOF(TCHAR), &cbWritten, NULL)
                        && WriteFile(hCommentFile, (LPCVOID)g_szCommentWrapEnd, lstrlen(g_szCommentWrapEnd) * SIZEOF(TCHAR), &cbWritten, NULL))
                {
                    if (IsLFNDrive(g_szIniFile))
                    {
                        lstrcpyn(szTemp, FILE_PROTOCOL FILE_FOLDER_SETTINGS TEXT("\\") FILE_HTMLINFOTIP, ARRAYSIZE(szTemp));
                    }
                    else
                    {
                        lstrcpyn(szTemp, FILE_PROTOCOL FILE_FOLDER_SETTINGS_SHORT TEXT("\\") FILE_HTMLINFOTIP, ARRAYSIZE(szTemp));
                    }
                    if (SHSetIniStringUTF7(STR_SHELLCLASSINFO, STR_HTMLINFOTIPFILE, szTemp , g_szIniFile))
                    {
                        hr = S_OK;
                    }
                }
                CloseHandle(hCommentFile);
            }
        }
        UpdateGlobalFolderInfo(UPDATE_COMMENT, NULL);
    }
    else
    {   // Have to remove comment.htt & HTMLInfoTipFile from desktop.ini
        TCHAR szHTMLInfoTipFile[MAX_PATH];
        SHGetIniStringUTF7(STR_SHELLCLASSINFO, STR_HTMLINFOTIPFILE, szHTMLInfoTipFile, ARRAYSIZE(szHTMLInfoTipFile), g_szIniFile);
        LPTSTR pszHTMLInfoTipFile = szHTMLInfoTipFile;
        if (StrCmpNI(FILE_PROTOCOL, pszHTMLInfoTipFile, 7) == 0) // ARRAYSIZE(TEXT("file://"))
        {
            pszHTMLInfoTipFile += 7;   // ARRAYSIZE(TEXT("file://"))
        }
        // handle relative references...
        PathCombine(pszHTMLInfoTipFile, g_szCurFolder, pszHTMLInfoTipFile);
        DeleteFile(pszHTMLInfoTipFile);
        SHSetIniStringUTF7(STR_SHELLCLASSINFO, STR_HTMLINFOTIPFILE, NULL, g_szIniFile);
        hr = S_OK;
    }
    return hr;
}

HRESULT GetCurrentComment(LPTSTR pszHTMLComment, int cchHTMLComment)
{
    HRESULT hr = E_FAIL;
    TCHAR szTemp[MAX_PATH];
    if (!pszHTMLComment || (cchHTMLComment <= 0))
    {
        hr = E_INVALIDARG;
    }
    else if (SHGetIniStringUTF7(STR_SHELLCLASSINFO, STR_HTMLINFOTIPFILE, szTemp, ARRAYSIZE(szTemp), g_szIniFile) > 0)
    {
        TCHAR szHTMLInfoTipFile[MAX_PATH];
        ExpandEnvironmentStrings(szTemp, szHTMLInfoTipFile, ARRAYSIZE(szHTMLInfoTipFile));   // This is a path, so expand the env vars in it
        ExpandWebDir(szHTMLInfoTipFile, ARRAYSIZE(szHTMLInfoTipFile));
        LPTSTR pszHTMLInfoTipFile = szHTMLInfoTipFile;
        if (StrCmpNI(FILE_PROTOCOL, pszHTMLInfoTipFile, 7) == 0) // ARRAYSIZE(TEXT("file://"))
        {
            pszHTMLInfoTipFile += 7;   // ARRAYSIZE(TEXT("file://"))
        }
        // handle relative references...
        PathCombine(pszHTMLInfoTipFile, g_szCurFolder, pszHTMLInfoTipFile);

        HANDLE hCommentFile = CreateFile(pszHTMLInfoTipFile, GENERIC_READ, FILE_SHARE_READ, 
                NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if (hCommentFile != INVALID_HANDLE_VALUE)
        {
            WCHAR wch;
            DWORD cbRead = 0;
            
            if (ReadFile(hCommentFile, (LPVOID)&wch, SIZEOF(wch), &cbRead, NULL)
                    && (cbRead == SIZEOF(wch)))
            {
                DWORD cbToRead;
                BOOL bFoundCommentStart = FALSE;
                if (wch == g_wchUnicodeBOFMarker)
                {
                    cbToRead = (cchHTMLComment - 1) * SIZEOF(WCHAR);
                    LPWSTR pwsz = (LPWSTR)LocalAlloc(LPTR, cbToRead + (1 * SIZEOF(WCHAR)));
                    if (!pwsz)
                    {
                        hr = E_OUTOFMEMORY;
                    }
                    else
                    {
                        WCHAR wszTemp[ARRAYSIZE(g_szCommentWrapBegin)];
                        if (ReadFile(hCommentFile, (LPVOID)&wszTemp, (ARRAYSIZE(wszTemp) - 1) * SIZEOF(WCHAR), &cbRead, NULL)
                                && (cbRead == (ARRAYSIZE(wszTemp) - 1) * SIZEOF(WCHAR)))
                        {
                            wszTemp[ARRAYSIZE(wszTemp) - 1] = L'\0';

                            WCHAR wszCommentWrapBegin[ARRAYSIZE(g_szCommentWrapBegin)];
                            SHTCharToUnicode(g_szCommentWrapBegin, wszCommentWrapBegin, ARRAYSIZE(wszCommentWrapBegin));
                            
                            if (StrCmpIW(wszTemp, wszCommentWrapBegin) != 0)
                            {
                                //Seek to the point after the Unicode file marker
                                SetFilePointer(hCommentFile, SIZEOF(g_wchUnicodeBOFMarker), NULL, FILE_BEGIN);
                            }
                            else
                            {
                                bFoundCommentStart = TRUE;
                            }
                        }
                        if (ReadFile(hCommentFile, (LPVOID)pwsz, cbToRead, &cbRead, NULL))
                        {
                            int iEnd = (int)cbRead / SIZEOF(WCHAR);
                            if (bFoundCommentStart)
                            {
                                pwsz[iEnd] = L'\0';

                                WCHAR wszCommentWrapEnd[ARRAYSIZE(g_szCommentWrapEnd)];
                                SHTCharToUnicode(g_szCommentWrapEnd, wszCommentWrapEnd, ARRAYSIZE(wszCommentWrapEnd));

                                if (StrCmpIW(&pwsz[iEnd - lstrlen(g_szCommentWrapEnd)], wszCommentWrapEnd) == 0)
                                {
                                    iEnd -= lstrlen(g_szCommentWrapEnd);
                                }
                            }
                            pwsz[iEnd] = L'\0';
                            SHUnicodeToTChar(pwsz, pszHTMLComment, cchHTMLComment);
                            hr = S_OK;
                        }
                        LocalFree(pwsz);
                    }
                }
                else
                {
                    // Anything other than the little endian unicode file is treated as ansi.
                    SetFilePointer(hCommentFile, 0L, NULL, FILE_BEGIN);   //Seek to the begining of the file again
                    cbToRead = (cchHTMLComment - 1) * SIZEOF(CHAR);
                    LPSTR psz = (LPSTR)LocalAlloc(LPTR, cbToRead + (1 * SIZEOF(CHAR)));
                    if (!psz)
                    {
                        hr = E_OUTOFMEMORY;
                    }
                    else
                    {
                        CHAR szTemp2[ARRAYSIZE(g_szCommentWrapBegin)];
                        if (ReadFile(hCommentFile, (LPVOID)&szTemp2, (ARRAYSIZE(szTemp2) - 1) * SIZEOF(CHAR), &cbRead, NULL)
                                && (cbRead == (ARRAYSIZE(szTemp2) - 1) * SIZEOF(CHAR)))
                        {
                            szTemp2[ARRAYSIZE(szTemp2) - 1] = L'\0';
                            
                            CHAR szCommentWrapBegin[ARRAYSIZE(g_szCommentWrapBegin)];
                            SHTCharToAnsi(g_szCommentWrapBegin, szCommentWrapBegin, ARRAYSIZE(szCommentWrapBegin));
                            
                            if (StrCmpIA(szTemp2, szCommentWrapBegin) != 0)
                            {
                                //Seek to the begining of the file again
                                SetFilePointer(hCommentFile, 0L, NULL, FILE_BEGIN);
                            }
                            else
                            {
                                bFoundCommentStart = TRUE;
                            }
                        }
                        if (ReadFile(hCommentFile, (LPVOID)psz, cbToRead, &cbRead, NULL))
                        {
                            int iEnd = (int)cbRead / SIZEOF(CHAR);
                            if (bFoundCommentStart)
                            {
                                psz[iEnd] = L'\0';

                                CHAR szCommentWrapEnd[ARRAYSIZE(g_szCommentWrapEnd)];
                                SHTCharToAnsi(g_szCommentWrapEnd, szCommentWrapEnd, ARRAYSIZE(szCommentWrapEnd));

                                if (StrCmpIA(&psz[iEnd - lstrlen(g_szCommentWrapEnd)], szCommentWrapEnd) == 0)
                                {
                                    iEnd -= lstrlen(g_szCommentWrapEnd);
                                }
                            }
                            psz[iEnd] = L'\0';
                            SHAnsiToTChar(psz, pszHTMLComment, cchHTMLComment);
                            hr = S_OK;
                        }
                        LocalFree(psz);
                    }
                }
            }
            CloseHandle(hCommentFile);
        }
    }
    if (FAILED(hr) && (hr != E_INVALIDARG))
    {
        pszHTMLComment[0] = TEXT('\0');
        hr = S_OK;
    }
    return hr;
}

