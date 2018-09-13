// installwv.cpp : Installs a file from a resource

#include "priv.h"
#include "installwv.h"
#include "regstr.h"
#ifndef _WIN64
#include <iert.h>
#endif

#define WINVER_KEY     "SOFTWARE\\Microsoft\\Windows\\CurrentVersion"
#define PROGFILES_KEY  "ProgramFilesDir"

#define IfFalseRet(val, hr) {if ((val) == 0) return (hr);}

HRESULT WriteResource(HANDLE hDestFile, HINSTANCE hResourceInst, HRSRC hRsrc)
{
    HGLOBAL hGlob;
    LPVOID  pTemp;
    ULONG   cbWritten;
    ULONG   cbResource;
    HRESULT hr = E_FAIL;

    // Load the resource
	hGlob = LoadResource(hResourceInst, hRsrc);
	if (EVAL(pTemp = LockResource(hGlob)))
    {
        // Write out the resource
        cbResource = SizeofResource(hResourceInst, hRsrc);
        if (EVAL(WriteFile(hDestFile, pTemp, cbResource, &cbWritten, NULL)))
            hr = S_OK;
    }

    return hr;
}

HRESULT GetInstallInfoFromResource(HINSTANCE hResourceInst, UINT uID, INSTALL_INFO *piiFile)
{
    TCHAR szEntry[MAX_PATH];
    HRESULT hr = E_FAIL;

    if (piiFile == NULL) 
        return E_INVALIDARG;

    // Get the string table entry
    piiFile->dwDestAttrib = FILE_ATTRIBUTE_HIDDEN;
    piiFile->szSource = NULL;
    piiFile->szDest = NULL;

    if (EVAL(LoadString(hResourceInst, uID, szEntry, ARRAYSIZE(szEntry))))    
    {
        // Are we at the end of the list (empty string)
        if (szEntry[0])
        {
            // No.
            piiFile->szSource = StrDup(szEntry);
            piiFile->szDest = StrDup(TEXT("ftp.htt"));
            hr = S_OK;
        }
        else
            hr = E_FAIL;
    }

    return hr;
}

HRESULT InstallInfoFreeMembers(INSTALL_INFO *piiFile)
{
    if (piiFile) {
        if (piiFile->szSource) {
            LocalFree(piiFile->szSource);
            piiFile->szSource = NULL;
        }

        if (piiFile->szDest) {
            LocalFree(piiFile->szDest);
            piiFile->szDest = NULL;
        }
    }

    return S_OK;
}


HRESULT InstallFilesFromResourceID(HINSTANCE hResourceInst, 
                                   UINT uID,                                
                                   LPTSTR pszDestDir)
{
    HRESULT hr = S_OK;
    INSTALL_INFO iiFile;

    if (!EVAL(pszDestDir))
        return E_INVALIDARG;

    if (S_OK == GetInstallInfoFromResource(hResourceInst, uID, &iiFile))
    {
        hr = InstallFileFromResource(hResourceInst, &iiFile, pszDestDir);
        InstallInfoFreeMembers(&iiFile);

        uID += 1;
    }

    return hr;
}

HRESULT InstallFileFromResource(HINSTANCE hResourceInst, 
                                INSTALL_INFO *piiFile, 
                                LPTSTR pszDestDir)
{
    HRESULT hr = E_FAIL;
    HRSRC   hRsrc;
    LPTSTR  lpszExt;

    // Find the resource
    if (EVAL(hRsrc = FindResource(hResourceInst, piiFile->szSource, RT_HTML)))
    {
        // Create destination file
        if (PathIsDirectory(pszDestDir))
        {
            TCHAR   szDestPath[MAX_PATH];

            StrNCpy(szDestPath, pszDestDir, MAX_PATH);
            if (PathAppend(szDestPath, piiFile->szDest))
            {
                HANDLE  hDestFile;

                SetFileAttributes(szDestPath, FILE_ATTRIBUTE_NORMAL); // Make sure we can overwrite
                hDestFile = CreateFile(szDestPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, piiFile->dwDestAttrib, NULL); 
                if (hDestFile != INVALID_HANDLE_VALUE) 
                {
                    // Write out the resource
                    hr = WriteResource(hDestFile, hResourceInst, hRsrc);
                    CloseHandle(hDestFile);

                    // Make sure the file attributes are set correctly.  Overwritting a file doesn't
                    // always update the attributes
                    SetFileAttributes(szDestPath, piiFile->dwDestAttrib);

                    if ((lpszExt = PathFindExtension(szDestPath)) && (StrCmpI(lpszExt, TEXT(".htt")) == 0))
                    {
                        hr = SHRegisterValidateTemplate(szDestPath, SHRVT_REGISTER);
                    }
                }
            }
        }
    }

    return hr;
}

HRESULT InstallWebViewFiles(HINSTANCE hInstResource)
{
    HRESULT hr = E_FAIL;
    TCHAR   szDestPath[MAX_PATH];

    // Set up %windir% path
    if (EVAL(SHGetSystemWindowsDirectory(szDestPath, ARRAYSIZE(szDestPath)) &&
             PathIsDirectory(szDestPath)))
    {
        // Install %windir%\web files
        if (EVAL(PathAppend(szDestPath, TEXT("Web"))))
        {
            if (PathIsDirectory(szDestPath) || CreateDirectory(szDestPath, NULL))
            {
                UINT idWebViewTemp = IDS_INSTALL_TEMPLATE;

                if (SHELL_VERSION_NT5 <= GetShellVersion())
                    idWebViewTemp = IDS_INSTALL_TEMPLATE_NT5;

                EVAL(SUCCEEDED(hr = InstallFilesFromResourceID(hInstResource, idWebViewTemp, szDestPath))); 
                EVAL(SetFileAttributes(szDestPath, FILE_ATTRIBUTE_SYSTEM));
            }
        }
    }
        
    return hr;
}
