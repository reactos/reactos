#include "shellprv.h"
#pragma  hdrstop

BOOL _CopyCommand(LPCTSTR pszCommand, LPCTSTR pszDir, LPTSTR pszOut, DWORD cchOut)
{
    BOOL fRet = TRUE;
    // if it does not have quotes, try unquoted name to see if that works...
    if (pszCommand[0] != TEXT('"'))
    {
        if (UrlIs(pszCommand, URLIS_URL))
        {
            //  urls never have params...
            fRet = FALSE;
    
            if (UrlIs(pszCommand, URLIS_FILEURL))
                PathCreateFromUrl(pszCommand, pszOut, &cchOut, 0);
            else
                lstrcpyn(pszOut, pszCommand, cchOut);
        }
        else
        {
            lstrcpyn(pszOut, pszCommand, cchOut);
            PathQualifyDef(pszOut, pszDir, 0);

            if (PathFileExistsAndAttributes(pszOut, NULL))
            {
                fRet = FALSE;
            }
        }
    }

    //  we need to put the original command for args parsing
    if (fRet)
        lstrcpyn(pszOut, pszCommand, cchOut);

    return fRet;
}

BOOL _QualifyWorkingDir(LPCTSTR pszPath, LPTSTR pszDir, DWORD cchDir)
{
    // special case to make sure the working dir gets set right:
    //   1) no working dir specified
    //   2) a drive or a root path, or a relative path specified
    // derive the working dir from the qualified path. this is to make
    // sure the working dir for setup programs "A:setup" is set right

    if (StrChr(pszPath, TEXT('\\')) || StrChr(pszPath, TEXT(':')))
    {
        // build working dir based on qualified path
        lstrcpyn(pszDir, pszPath, cchDir);
        PathQualifyDef(pszDir, NULL, PQD_NOSTRIPDOTS);
        PathRemoveFileSpec(pszDir);
        return TRUE;
    }

    return FALSE;
}
    
// Run the thing, return TRUE if everything went OK
BOOL ShellExecCmdLine(HWND hwnd, LPCTSTR pszCommand, LPCTSTR pszDir,
        int nShow, LPCTSTR pszTitle, DWORD dwFlags)
{
    TCHAR szWD[MAX_PATH];
    TCHAR szFileName[MAX_PATH];
    LPTSTR pszArgs;
    SHELLEXECUTEINFO ExecInfo = {0};

    if (pszDir && *pszDir == TEXT('\0'))
        pszDir = NULL;

    if (_CopyCommand(pszCommand, pszDir, szFileName, SIZECHARS(szFileName)))
    {
        //  there might be args in that command
        pszArgs = PathGetArgs(szFileName);
        if (*pszArgs)
            *(pszArgs - 1) = TEXT('\0');
    }
    else
        pszArgs = NULL;

    PathUnquoteSpaces(szFileName);

    // this needs to be here.  app installs rely on the current directory
    // to be the directory with the setup.exe 
    if (!UrlIs(szFileName, URLIS_URL) 
    && ((dwFlags & SECL_USEFULLPATHDIR) || !pszDir))
    {
        if (_QualifyWorkingDir(szFileName, szWD, SIZECHARS(szWD)))
            pszDir = szWD;
    }


    FillExecInfo(ExecInfo, hwnd, NULL, szFileName, pszArgs, pszDir, nShow);
    ExecInfo.fMask = SEE_MASK_FLAG_DDEWAIT | SEE_MASK_DOENVSUBST;

    if (dwFlags & SECL_NO_UI)
        ExecInfo.fMask |= SEE_MASK_FLAG_NO_UI;

#ifdef WINNT        
    if (dwFlags & SECL_SEPARATE_VDM)
        ExecInfo.fMask |= SEE_MASK_FLAG_SEPVDM;
#endif

    return ShellExecuteEx(&ExecInfo);
}

