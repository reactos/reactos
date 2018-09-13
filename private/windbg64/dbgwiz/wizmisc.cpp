#include "precomp.hxx"
#pragma hdrstop

static TCHAR szInitialDir_BrowseForDir[_MAX_PATH] = {0};


int 
CALLBACK
BrowseCallbackProc(
    HWND hwnd,
    UINT uMsg,
    LPARAM lp, 
    LPARAM pData
    ) 
{       
    TCHAR sz[_MAX_PATH] = {0};

    switch(uMsg) {
    case BFFM_INITIALIZED:
        // LParam is TRUE since you are passing a path.
        // It would be FALSE if you were passing a pidl.
        SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM) szInitialDir_BrowseForDir);
        break;            

    case BFFM_SELCHANGED:
        // Set the status window to the currently selected path.
        if (SHGetPathFromIDList((LPITEMIDLIST) lp, sz)) {
            SendMessage(hwnd, BFFM_SETSTATUSTEXT, 0, (LPARAM) sz);
        }
        break;

    default:
        break;         
    }         

    return 0;      
}

void
BrowseForDir(
    HWND hDlg,
    int nPath_EditCtrlId
    )
{  
    Assert(hDlg);

    LPMALLOC pMalloc;
    BROWSEINFO   bi = {0};
    LPITEMIDLIST pidl;

    // Get a starting directory
    GetDlgItemText(hDlg, nPath_EditCtrlId, szInitialDir_BrowseForDir, sizeof(szInitialDir_BrowseForDir));

    Assert(SHGetMalloc(&pMalloc) == NOERROR);

    bi.hwndOwner      = hDlg;    
    //bi.pidlRoot       = NULL;
    //bi.pszDisplayName = szPath;    
    //bi.lpszTitle      = pszTitle;
    bi.ulFlags        = BIF_BROWSEINCLUDEFILES | BIF_RETURNONLYFSDIRS | BIF_STATUSTEXT;
    bi.lpfn           = BrowseCallbackProc;
    //bi.lParam         = (LPARAM) szInitialDirectory;

    pidl = SHBrowseForFolder(&bi);    
    

    if (pidl) { 
        // Retrieve the full path and set the edit ctrl text
        char szPath[_MAX_PATH] = {0};
        
        Assert(SHGetPathFromIDList(pidl, szPath));
        Assert(strlen(szPath) < sizeof(szPath));

        SetDlgItemText(hDlg, nPath_EditCtrlId, szPath); 
 
        // Free the PIDL returned by SHBrowseForFolder. 
        pMalloc->Free(pidl); 
    } 

    pMalloc->Release();
}

tagIE_3_OPENFILENAMEA *
Create_IE3_OpenFileName(
    HWND hDlg,
    PSTR pszPath,
    DWORD dwPathSize,
    PSTR pszInitialDir,
    PSTR pszFileFilter
    )
{
    tagIE_3_OPENFILENAMEA * pofn = (tagIE_3_OPENFILENAMEA *)
        calloc(sizeof(tagIE_3_OPENFILENAMEA), 1);

    if (pofn) {
        pofn->lStructSize = sizeof(*pofn);
        pofn->hwndOwner = hDlg; 
        //pofn->hInstance; 
        pofn->lpstrFilter = pszFileFilter; 
        //pofn->lpstrCustomFilter; 
        //pofn->nMaxCustFilter; 
        //pofn->nFilterIndex; 
        pofn->lpstrFile = pszPath; 
        pofn->nMaxFile = dwPathSize; 
        //pofn->lpstrFileTitle; 
        //pofn->nMaxFileTitle; 
        pofn->lpstrInitialDir = pszInitialDir;
        //pofn->lpstrTitle; 
        pofn->Flags = OFN_LONGNAMES | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
        //pofn->nFileOffset; 
        //pofn->nFileExtension; 
        //pofn->lpstrDefExt; 
        //pofn->lCustData; 
        //pofn->lpfnHook; 
        //pofn->lpTemplateName; 
    }

    return pofn;
}

BOOL
SimpleSaveFileDlg(
    HWND hDlg,
    PSTR pszPath,
    DWORD dwPathSize,
    PSTR pszInitialDir,
    PSTR pszFileFilter
    )
{
    Assert(hDlg);


    if (!pszPath) {
        dwPathSize = 0;
    }

    tagIE_3_OPENFILENAMEA * pofn = Create_IE3_OpenFileName(hDlg, 
                                                           pszPath,
                                                           dwPathSize, 
                                                           pszInitialDir, 
                                                           pszFileFilter);
    if (pofn) {
        
        BOOL bRet;
        
        bRet = GetSaveFileName( (OPENFILENAME *) pofn);
        free(pofn);

        return bRet;

    } else {
        return FALSE;
    }
}


void
SaveFileDlg(
    HWND hDlg,
    PSTR pszPath,
    DWORD dwPathSize,
    PSTR pszInitialDir,
    PSTR pszFileFilter,
    int nPath_EditCtrlId
    )
{
    Assert(hDlg);

    char szTmpPath[_MAX_PATH] = {0};
    char szCurDir[_MAX_PATH] = {0};

    if (pszPath) {
        strcpy(szTmpPath, pszPath);
    }

    if (pszInitialDir) {
        strncpy(szCurDir, pszInitialDir, sizeof(szCurDir) -1);
    } else {
        Assert(GetCurrentDirectory(sizeof(szCurDir), szCurDir));
    }

    if (SimpleSaveFileDlg(hDlg, szTmpPath, sizeof(szTmpPath), szCurDir, pszFileFilter)) {
        SetDlgItemText(hDlg, nPath_EditCtrlId, szTmpPath);
        if (pszPath) {
            strncpy(pszPath, szTmpPath, dwPathSize-1);
            pszPath[dwPathSize-1] = NULL;
        }
    }
}


BOOL
SimpleBrowseForFile(
    HWND hDlg,
    PSTR pszPath,
    DWORD dwPathSize,
    PSTR pszInitialDir,
    PSTR pszFileFilter
    )
{
    Assert(hDlg);


    if (pszPath) {
        *pszPath = NULL;
    } else {
        dwPathSize = 0;
    }

    tagIE_3_OPENFILENAMEA * pofn = Create_IE3_OpenFileName(hDlg, 
                                                           pszPath,
                                                           dwPathSize, 
                                                           pszInitialDir, 
                                                           pszFileFilter);

    if (pofn) {
        
        BOOL bRet;
        
        pofn->Flags |= OFN_FILEMUSTEXIST;
        bRet = GetOpenFileName( (OPENFILENAME *) pofn);
        free(pofn);

        return bRet;

    } else {
        return FALSE;
    }
}


void
BrowseForFile(
    HWND hDlg,
    PSTR pszInitialDir,
    PSTR pszFileFilter,
    int nPath_EditCtrlId
    )
{
    Assert(hDlg);

    char szPath[_MAX_PATH] = {0};
    char szCurDir[_MAX_PATH] = {0};

    if (pszInitialDir) {
        strncpy(szCurDir, pszInitialDir, sizeof(szCurDir) -1);
    } else {
        Assert(GetCurrentDirectory(sizeof(szCurDir), szCurDir));
    }

    if (SimpleBrowseForFile(hDlg, szPath, sizeof(szPath), szCurDir, pszFileFilter)) {
        SetDlgItemText(hDlg, nPath_EditCtrlId, szPath);
    }
}


BOOL 
CreateLink(
    HWND hwnd,
    LPCTSTR lptszTargetPathOrig,       // target exe path
    LPCTSTR lptszArguments,        // exe arguments
    LPCTSTR lptszDesc,
    LPCTSTR lptszLinkPath          // shortcut path
    )     
{
    Assert(hwnd);

    IShellLink*   pShellLink   = NULL;
    IPersistFile* pPersistFile = NULL;
    TCHAR szTargetPath[_MAX_PATH] = {0};
    HRESULT hResult;

    // Remove "" from the targetpath
    if ('"' == *lptszTargetPathOrig) {
        _tcscpy(szTargetPath, lptszTargetPathOrig+1);
        szTargetPath[_tcslen(szTargetPath) -1] = 0;
    } else {
        _tcscpy(szTargetPath, lptszTargetPathOrig);
    }

    hResult = CoInitialize( NULL );   
    Assert(ERROR_SUCCESS == hResult);

    hResult = CoCreateInstance( CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, 
        IID_IShellLink, (LPVOID*)&pShellLink);
    Assert(ERROR_SUCCESS == hResult);

    hResult = pShellLink->QueryInterface( IID_IPersistFile, (LPVOID*)&pPersistFile );
    Assert(ERROR_SUCCESS == hResult);



    do {
        hResult = pShellLink->SetPath( szTargetPath );
        if (ERROR_SUCCESS != hResult) {
            
            PSTR pszLastError = WKSP_FormatLastErrorMessage();
            int nRes = WKSP_CustMsgBox(MB_ICONHAND | MB_TASKMODAL | MB_RETRYCANCEL, 
                NULL, IDS_RETRY_CREATING_SHORTCUT, pszLastError);
            
            if (pszLastError) {
                free(pszLastError);
            }

            if (IDRETRY != nRes) {
                goto EXIT;
            }
        }
    } while (ERROR_SUCCESS != hResult);

    hResult = pShellLink->SetArguments( lptszArguments );
    Assert(ERROR_SUCCESS == hResult);

    hResult = pShellLink->SetDescription(lptszDesc);
    Assert(ERROR_SUCCESS == hResult);
    
    if ( SUCCEEDED( hResult ) ) {
        LPCOLESTR lposzBuffer;
        OLECHAR   oBuffer[_MAX_PATH];
        
#ifdef UNICODE
        lposzBuffer = lptszLinkPath;
#else
        MultiByteToWideChar( CP_ACP, 0x0000, lptszLinkPath, -1, oBuffer, _MAX_PATH );
        lposzBuffer = oBuffer;
#endif
        
        hResult = pPersistFile->Save( lposzBuffer, TRUE );
        Assert(ERROR_SUCCESS == hResult);
        
    }

EXIT:    
    if (pPersistFile) {
        pPersistFile->Release();
    }

    if (pShellLink) {
        pShellLink->Release(); 
    }
    
    CoUninitialize();
    
    return SUCCEEDED( hResult );
}


BOOL
SearchPromptForExePath(
    HWND hwnd,
    PSTR pszFileName,
    PSTR & pszCompletePathToFile
    )
{
    Assert(hwnd);

    char szCurDir[_MAX_PATH] = {0};
    char szFilePath[_MAX_PATH] = {0};
    DWORD dwSize;


    dwSize = SearchPath(NULL, pszFileName, NULL, sizeof(szFilePath), szFilePath, NULL);
    Assert(dwSize < sizeof(szFilePath));

    if (dwSize) {
        // Found it
        pszCompletePathToFile = NT4SafeStrDup(szFilePath);
        return TRUE;
    }

    //
    // Didn't find it, ask the user to find it.
    //
    Assert(GetCurrentDirectory(sizeof(szCurDir), szCurDir));

    if (SimpleBrowseForFile(hwnd, szFilePath, sizeof(szFilePath), szCurDir, pszFileName)) {
        pszCompletePathToFile = NT4SafeStrDup(szFilePath);
        return TRUE;
    }

    return FALSE;
}


//**************************************************************************** 
// // BOOL FGetDirectory(LPTSTR szDir) // 
//****************************************************************************  
BOOL 
FGetDirectory(
    LPTSTR pszDir
    ) 
{ 
    BOOL  fRet;
    
    LPITEMIDLIST pidlRoot; 
    LPMALLOC lpMalloc;  
    
    if (0 != SHGetSpecialFolderLocation(HWND_DESKTOP, CSIDL_COMMON_DESKTOPDIRECTORY, &pidlRoot)) {
        return FALSE; 
    }

    if (NULL == pidlRoot) {
        return FALSE;  
    }

    fRet = SHGetPathFromIDList(pidlRoot, pszDir); 
    
    // Get the shell's allocator to free PIDLs 
    if (!SHGetMalloc(&lpMalloc) && (NULL != lpMalloc)) { 
        if (NULL != pidlRoot) { 
            lpMalloc->Free(pidlRoot); 
        }  
        lpMalloc->Release(); 
    }  
    return fRet; 
}   

 
PSTR
CreateLinkOnDesktop(
    HWND hwnd,
    PSTR pszShortcutName,
    BOOL bCreateLink
    )
{
    Assert(hwnd);

    char szExe[_MAX_PATH] = {0};
    char szArgs[_MAX_PATH * 10] = {0};
    char szLink[_MAX_PATH] = {0};
    char szDesc[_MAX_PATH] = {0};

    strcpy(szExe, "\"");

    // Determine the exe: windbg or windbgrm
    if (g_CfgData.m_bRunningOnHost) {
        
        // Create the exe path
        PSTR pszFile = "WINDBG.EXE";
        PSTR pszCompletePathToFile;
        BOOL bSuccess;
        do {

            bSuccess = SearchPromptForExePath(hwnd, pszFile, pszCompletePathToFile);

            if (!bSuccess && IDYES != WKSP_CustMsgBox(MB_ICONHAND | MB_TASKMODAL | MB_YESNO, 
                NULL, IDS_WINDBG_NEEDS_TO_BE_LOCATED, pszFile, pszFile)) {
                    
                return NULL;
            }
        } while (!bSuccess);

        strcat(szExe, pszCompletePathToFile);
        strcpy(szDesc, pszFile);

        free(pszCompletePathToFile);

    } else {
        // Runnning on the target
        if (enumREMOTE_USERMODE == g_CfgData.m_DebugMode) {
                
            // If we are on the target and doing user mode debugging
            // we must be doing remote debugging.
            //
            // Point the shortcut to windbgrm
            PSTR pszFile = "WINDBGRM.EXE";
            PSTR pszCompletePathToFile;
            BOOL bSuccess;
            do {

                bSuccess = SearchPromptForExePath(hwnd, pszFile, pszCompletePathToFile);

                if (!bSuccess && IDRETRY != WKSP_CustMsgBox(MB_ICONHAND | MB_TASKMODAL | MB_YESNO, 
                    NULL, IDS_WINDBG_NEEDS_TO_BE_LOCATED, pszFile, pszFile)) {
                    
                    return NULL;
                }
            } while (!bSuccess);

            strcat(szExe, pszCompletePathToFile);
            strcpy(szDesc, pszFile);

            free(pszCompletePathToFile);

        } else {
            // No link will be created.
            // We are doing kernel debugging.
            return NULL;
        }
    }

    strcat(szExe, "\"");

    // Create the arguments
    if (!g_CfgData.m_bRunningOnHost && enumREMOTE_USERMODE == g_CfgData.m_DebugMode) {
        // Add the windbgrm arguments
        if (g_CfgData.m_bSerial) {
            strcat(szArgs, "-n \"Ser");
            strcat(szArgs, BaudRateBitFlagToText(g_CfgData.m_dwBaudRate));
            strcat(szArgs, "\"");
        } else {
            // Network connection
            strcat(szArgs, "-n \"Pipes\"");
        }
    } else {
        //
        // Add the sym path
        if (!g_SymPaths.IsEmpty()) {
            strcat(szArgs, "-y \"");
            while (!g_SymPaths.IsEmpty()) {
                PSTR pszSymPath = g_SymPaths.GetHeadData();
                g_SymPaths.RemoveHead();
                Assert(pszSymPath);

                strcat(szArgs, pszSymPath);
                strcat(szArgs, ";");

                free(pszSymPath);
            }

            // Remove trailing ";" sympath
            szArgs[strlen(szArgs) -1] = NULL;
            strcat(szArgs, "\" ");
        }

        //
        // Add workspace name
        strcat(szArgs, "-w \"");
        strcat(szArgs, g_CfgData.m_pszShortCutName);
        strcat(szArgs, "\" ");

        //
        // Add the name of the executable
        if (enumKERNELMODE == g_CfgData.m_DebugMode) {
            strcat(szArgs, "ntoskrnl.exe ");
        } else if (enumDUMP == g_CfgData.m_DebugMode) {
            strcat(szArgs, "-z \"");
            strcat(szArgs, g_CfgData.m_pszDumpFileName);
            strcat(szArgs, "\" ");
        } else {
            if (g_CfgData.m_bLaunchNewProcess) {
                if (g_CfgData.m_pszExecutableName && *g_CfgData.m_pszExecutableName) {
                    strcat(szArgs, g_CfgData.m_pszExecutableName);
                    strcat(szArgs, " ");
                } else {
                    strcat(szArgs, "-Ae ");
                }
            } else {
                strcat(szArgs, "-Ap ");
            }
        }
    }

    // Get the path to the desktop
#if 0
    {
        HKEY hkey = NULL;
        char szBuf[_MAX_PATH] = {0};

        if (RegOpenKeyEx(HKEY_CURRENT_USER, 
            "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", 
            0, KEY_READ, &hkey) != ERROR_SUCCESS) {

            return NULL;

        } else {

            DWORD dwType;
            DWORD dwSize = sizeof(szLink);

            Assert(ERROR_SUCCESS == RegQueryValueEx(hkey, "Desktop", NULL, &dwType,
                (PUCHAR) &szBuf, &dwSize));
        
            RegCloseKey(hkey);

            sprintf(szLink, "%s\\%s %s.lnk", szBuf, szDesc, g_CfgData.m_pszShortCutName);
 
            if (CreateLink(hwnd, szExe, szArgs, szDesc, szLink)) {
                return NT4SafeStrDup(szLink);
            } else {
                return NULL;
            }
        }
    }
#else
    {
        if (bCreateLink) {
            char szBuf[_MAX_PATH] = {0};
            
            if (!FGetDirectory(szBuf)) {
                return NULL;
            } else {
                sprintf(szLink, "%s\\%s %s.lnk", szBuf, szDesc, g_CfgData.m_pszShortCutName);
                
                if (CreateLink(hwnd, szExe, szArgs, szDesc, szLink)) {
                    return NT4SafeStrDup(szLink);
                } else {
                    return NULL;
                }
            } 
        } else {
            UINT uLen = strlen(szExe) + strlen(szArgs) + 2;
            PSTR psz = (PSTR) malloc(uLen);
            
            sprintf(psz, "%s %s", szExe, szArgs);
            Assert(strlen(psz) < uLen);
            
            return psz;
        }
    }
#endif
}

void
DeleteLinkOnDesktop(
    HWND hwnd, 
    PSTR pszDeskTopShortcut_FullPath
    )
{
    Assert(hwnd);
    if (!pszDeskTopShortcut_FullPath) {
        return;
    }

    if (!DeleteFile(pszDeskTopShortcut_FullPath)) {
        PSTR pszErr = WKSP_FormatLastErrorMessage();
        WKSP_MsgBox(NULL, IDS_ERROR_DELETE_SHORTCUT, pszErr);
        free(pszErr);
    }
}

void
SpawnMunger()
{
    // You must be on the target and doing kernel debugging in order spawn munger.
    if (!(!g_CfgData.m_bRunningOnHost && enumKERNELMODE == g_CfgData.m_DebugMode)) {
        return;
    }

    PSTR pszAppName = "dbconfig.exe";
    char szCmdLine[_MAX_PATH];

    sprintf(szCmdLine, "%s /d /q /a %s /b %s", pszAppName, g_CfgData.m_pszCommPortName, 
        BaudRateBitFlagToText(g_CfgData.m_dwBaudRate));
    Assert(strlen(szCmdLine) < sizeof(szCmdLine));

    if (WinExec(szCmdLine, SW_SHOWNORMAL) <= 31) {
        WKSP_MsgBox(NULL, IDS_ERROR_COULDNT_EXEC_MUNGER, g_CfgData.m_pszCommPortName,
            BaudRateBitFlagToText(g_CfgData.m_dwBaudRate));
    }
}


int
DirectoryExists(
    PCSTR pszDirOriginal
    )
/*++
Returns:
    1 - dir exists.
    -1 - file exists with the same name.
    0 - neither a file nor a dir exists with that name
--*/
{
    PSTR pszDir = NT4SafeStrDup(pszDirOriginal);
    Assert(pszDir);

    // remove any trailing '\\'
    {
        for (PSTR psz = pszDir + strlen(pszDir) -1; '\\' == *psz; psz--) {
            *psz = NULL;
        }
    }

    WIN32_FIND_DATA wfd;
    HANDLE h = FindFirstFile(pszDir, &wfd);

    free( (PVOID) pszDir);

    if (INVALID_HANDLE_VALUE == h) {
        // Cool, it doesn't exist.
        return 0;
    } else {
        Assert(FindClose(h));
    }
    if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        return 1;
    } else {
        return -1;
    }
}

BOOL
CreateDirIfNotExist(
    PCSTR pszDir
    )
{
    if (DirectoryExists(pszDir)) {
        return TRUE;
    } else {
        if (CreateDirectory(pszDir, NULL)) {
            return TRUE;
        } else {
            return FALSE;
        }
    }
}

BOOL
CreateDirTree(
    PCSTR pszPathOriginal
    )
{
    char szTarg[_MAX_PATH * 2] = {0};
    char szDrive[_MAX_DRIVE];
    char szPath[_MAX_PATH];
    char szFName[_MAX_FNAME];
    char szExt[_MAX_EXT];

    PSTR pszDelims = "\\";

    PSTR pszDup = (PSTR) malloc(strlen(pszPathOriginal) + 2);
    Assert(pszDup);
    strcpy(pszDup, pszPathOriginal);
    strcat(pszDup, "\\");

    BOOL bNetworkName = !strncmp("\\\\", pszDup, 2);
  
    _splitpath(pszDup, szDrive, szPath, szFName, szExt);
    Assert(!*szFName);
    Assert(!*szExt);

    if (bNetworkName) {
        Assert(!*szDrive);
        strcat(szTarg, "\\");
    } else {
        strcat(szTarg, szDrive);
    }

    int nPasses = 0;
    for (PSTR psz = strtok(szPath, pszDelims); psz; psz = strtok(NULL, pszDelims)) {
        strcat(szTarg, "\\");
        strcat(szTarg, psz);

        if (!bNetworkName || (bNetworkName && nPasses >= 2)) {
            if (!CreateDirIfNotExist(szTarg)) {
                return FALSE;
            }
        } else {
            nPasses++;
        }
    }

    free( (PVOID) pszDup);

    return TRUE;
}

BOOL
IsCommPortAlreadyTheDebugPort(
    PSTR pszPortNameToTest
    )
/*++
    Stolen from Munger.c
--*/
{
    Assert(pszPortNameToTest);

    //
    // Names used in boot.ini parsing
    //
    const TCHAR szBootIni[]     = TEXT("c:\\boot.ini");
    const TCHAR szFlexBoot[]    = TEXT("flexboot");
    const TCHAR szMultiBoot[]   = TEXT("multiboot");
    const TCHAR szBootLdr[]     = TEXT("boot loader");
    const TCHAR szTimeout[]     = TEXT("timeout");
    const TCHAR szDefault[]     = TEXT("default");
    const TCHAR szOS[]          = TEXT("operating systems");

    const   int BUFZ = 4096;
    const TCHAR   *pszBoot = NULL;
    TCHAR   szTemp[MAX_PATH] = {0};
    TCHAR   szKeyName[BUFZ] = {0};
    TCHAR   szBuffer[BUFZ] = {0};
    int     i, n;

#if defined(MIPS) || defined(_ALPHA_) || defined(_PPC_)
    return FALSE;

    // Way too many functions to import
#if 0
    ////////////////////////////////////////////////////////////////////
    //  Read info from NVRAM environment variables
    ////////////////////////////////////////////////////////////////////
    
    fCanUpdateNVRAM = FALSE;
    if (hmodSetupDll = LoadLibrary(TEXT("setupdll"))) {
        if (fpGetNVRAMvar = (GETNVRAMPROC)GetProcAddress(hmodSetupDll, "GetNVRAMVar")) {
            if (fCanUpdateNVRAM = GetRGSZEnvVar (&CPEBOSLoadIdentifier, "LOADIDENTIFIER")) {
                GetRGSZEnvVar (&CPEBOSLoadOptions,"OSLOADOPTIONS");
                GetRGSZEnvVar (&CPEBOSLoadPaths, "OSLOADFILENAME");
                NumBootOptions = CPEBOSLoadIdentifier.cEntries;
                for (i = 0; i < CPEBOSLoadIdentifier.cEntries; i++) {
                    strcpy(BootOptions[i].Identifier, CPEBOSLoadIdentifier.pszVars[i]);
                    if (i<CPEBOSLoadPaths.cEntries) {
                        strcpy(BootOptions[i].Path, CPEBOSLoadPaths.pszVars[i]);
                    } else {
                        BootOptions[i].Path[0]='\0';
                    }
                    if (i<CPEBOSLoadOptions.cEntries) {
                        strcpy(BootOptions[i].OtherOptions, CPEBOSLoadOptions.pszVars[i]);
                    } else {
                        BootOptions[i].OtherOptions[0]='\0';
                    }
                    BootOptions[i].DebugPort=-1;
                    BootOptions[i].BaudRate=-1;
                    BootOptions[i].DebugType=-1;
                    BootOptions[i].Modified=FALSE;
                    _strlwr(BootOptions[i].OtherOptions);
                    ParseBootOption(&(BootOptions[i]));
                    if (-1==BootOptions[i].DebugType) {
                        BootOptions[i].DebugType=IDO_RBNO;
                    }
                }
                Curr_Munge.BootIndex=0;
            } else {
                return FALSE;
            }
        }
        FreeLibrary (hmodSetupDll);
    }
#endif

#else  //  X86 (not MIPS || ALPHA || PPC)
    
    //
    // Find System Partition for boot.ini
    //
#ifndef _ALPHA_
#if 0
    // TODO - kcarlos
    // This function should be exported and then called, way too
    // much code to copy paste.
    szBootIni[0]=x86DetermineSystemPartition( NULL );
#endif
#endif
    
    //
    //  Get correct Boot Drive - this was added because after someone
    //  boots the system, they can ghost or change the drive letter
    //  of their boot drive from "c:" to something else.
    //
    // (not worth it for this util)
    //
    //szBootIni[0] = GetBootDrive();
    //
    //
    //  Determine which section [boot loader]
    //                          [flexboot]
    //                       or [multiboot] is in file
    //
    
    n = GetPrivateProfileString (szBootLdr, NULL, NULL, szTemp,
        sizeof(szTemp), szBootIni);
    if (n != 0) {
        pszBoot = szBootLdr;
    } else {
        n = GetPrivateProfileString (szFlexBoot, NULL, NULL, szTemp,
            sizeof(szTemp), szBootIni);
        if (n != 0) {
            pszBoot = szFlexBoot;
        } else {
            pszBoot = szMultiBoot;
        }
    }
    
    //  Get info under [*pszBoot] section - timeout & default OS path
    
    GetPrivateProfileString (pszBoot, szDefault, NULL, szTemp,
        sizeof(szTemp), szBootIni);
    

    //
    // Get the boot entry
    //
    GetPrivateProfileString(szOS, szTemp, NULL, szKeyName, BUFZ, szBootIni);
    if (!*szKeyName) {
        return FALSE;
    } else {
        _strlwr(szKeyName);
        if (!strstr(szKeyName, "debug")) {
            // No debugging
            return FALSE;
        } else {
            // We are doing kernel debugging. Which port is in use?
            PSTR pszPort = strstr(szKeyName, "debugport=");
            if (!pszPort) {
                // None specified, the default port is 2
                if (!_stricmp(pszPortNameToTest, "com2")) {
                    return TRUE;
                } else {
                    return FALSE;
                }
            } else {
                pszPort += strlen("debugport=");
                if (!_stricmp(pszPort, pszPortNameToTest)) {
                    return TRUE;
                } else {
                    return FALSE;
                }
            }
        }
    }
#endif
}

void
EnumComPorts(
    PCOMPORT_INFO pComInfo, 
    DWORD dwSizeOfArray,
    DWORD & dwNumComPortsFound
    )
{
    Assert(pComInfo);
    Assert(dwSizeOfArray);

    ZeroMemory(pComInfo, sizeof(COMPORT_INFO) * dwSizeOfArray);

    const int nMAX_STRING_SIZE = 256;
    HKEY hkey;
    DWORD dwMaxValueNameLen;
    DWORD dwNumPortsPresent = 0;

    dwNumComPortsFound = 0;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Hardware\\DeviceMap\\SerialComm",
        0, 
        KEY_ALL_ACCESS, //KEY_READ, 
        &hkey ) != ERROR_SUCCESS ) {

        return;
    }

    WKSP_RegKeyValueInfo(hkey, NULL,
        &dwNumPortsPresent, NULL, &dwMaxValueNameLen);

    Assert(dwMaxValueNameLen < nMAX_STRING_SIZE);
    Assert(dwNumPortsPresent < dwSizeOfArray);

    for (DWORD dw = 0; dw < dwNumPortsPresent; dw++) {

        DWORD dwType;
        char szTmp[nMAX_STRING_SIZE] = {0};
        DWORD dwTmp = sizeof(szTmp);
        char szData[nMAX_STRING_SIZE] = {0};
        DWORD dwDataSize = sizeof(szData);

        Assert(RegEnumValue(hkey, dw, szTmp, &dwTmp, 0, &dwType,
            (PUCHAR) szData, &dwDataSize ) == ERROR_SUCCESS);

        sprintf(szTmp, "\\\\.\\%s", szData );
        HANDLE hCommDev = CreateFile(
                         szTmp,
                         GENERIC_READ | GENERIC_WRITE,
                         0,
                         NULL,
                         OPEN_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL
                         );

        if (hCommDev == INVALID_HANDLE_VALUE) {
            // Is it in use by the OS as a debug port right now?
            if (!IsCommPortAlreadyTheDebugPort(szData)) {
                // It is being used by something else, skip it
                continue;
            }
            
            hCommDev = CreateFile(
                             szTmp,
                             0, // Just query the thing, don't try to open it
                             0,
                             NULL,
                             OPEN_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL
                             );
        }


        if (hCommDev != INVALID_HANDLE_VALUE) {
            // We were able to open it.
            pComInfo[dwNumComPortsFound].dwNum = strtoul(&szData[3], NULL, 0);
            strcpy(pComInfo[dwNumComPortsFound].szSymName, szData);
        
            COMMPROP cmmp;
            Assert(GetCommProperties(hCommDev, &cmmp));
        
            pComInfo[dwNumComPortsFound].dwSettableBaud = cmmp.dwSettableBaud;

            dwNumComPortsFound++;

            // Comm port is avaliable
            CloseHandle( hCommDev );
        }
    }

    RegCloseKey( hkey );
}



void
CreateUniqueDestPath()
{
/*
    srand( (unsigned) time( NULL ) );

    // Loop until this succeeds
    while (TRUE) {
        long lTime = time(NULL);
        int nExt = rand() % 0x1000;

        sprintf(g_CfgData.m_szSubDirName, "%x.%x", lTime, nExt);

        char szNewDir[_MAX_PATH];
        strcpy(szNewDir, g_CfgData.m_szBaseSymPath);
        strcat(szNewDir, "\\");
        strcat(szNewDir, g_CfgData.m_szSubDirName);

        WIN32_FIND_DATA wfd;
        HANDLE h = FindFirstFile(szNewDir, &wfd);
        if (INVALID_HANDLE_VALUE == h) {
            // Cool, it doesn't exist. We have a new name
            return;
        } else {
            Assert(FindClose(h));
        }
    }
*/
}

void
StripWhiteSpace(
    char * const pszStart
    )
/*++
Description:
    Strips traling and leading white space.
--*/
{
    Assert(pszStart);
    PSTR psz = NULL;

    // Trim trialing white space
    for (psz = pszStart + strlen(pszStart) -1;
        psz >= pszStart && isspace(*psz); psz--) {

        *psz = NULL;
    }

    // Trim leading white space
    for (psz = pszStart; *psz && isspace(*psz); psz++) {
        // Intentionally empty
    }

    if (!*psz) {
        // empty string
        *pszStart = NULL;
    } else {
        memmove(pszStart, psz, strlen(psz) +1);
    }
}

void
GuessAtSymSrc()
{
    char szSP[_MAX_PATH] = {0};
    char szOS[_MAX_PATH] = {0};
    PSTR psz = "\\\\ntbuilds\\";

    if (!_strnicmp(psz, g_CfgData.m_szInstallSrcPath, strlen(psz))) {
        // internal. Installed from NTbuilds
        strcpy(szOS, "\\\\ntstress\\symbols\\");

        switch (g_CfgData.m_si.wProcessorArchitecture) {
        default:
            Assert(0);
            break;
            
        case PROCESSOR_ARCHITECTURE_INTEL:
            strcat(szOS, "x86\\");
            break;
            
        case PROCESSOR_ARCHITECTURE_MIPS:
            strcat(szOS, "mips\\");
            break;
            
        case PROCESSOR_ARCHITECTURE_ALPHA:
            strcat(szOS, "alpha\\");
            break;
            
        case PROCESSOR_ARCHITECTURE_PPC:
            strcat(szOS, "ppc\\");
            break;
        }

        // Build number
        psz = szOS + strlen(szOS);
        sprintf(psz, "%lu", g_CfgData.m_osi.dwBuildNumber);

        // Copy the OS path to the SP (service pack) path
        strcpy(szSP, szOS);

        // Add the service pack number
        if (g_CfgData.m_dwServicePack) {
            psz = szSP + strlen(szSP);
            sprintf(psz, "sp%lu", g_CfgData.m_dwServicePack);
        }

        // Update both paths
        {
            PSTR pszChecked = WKSP_DynaLoadString(g_hInst, IDS_OS_DESC_CHECKED);
            if (!strcmp(pszChecked, g_CfgData.m_pszFreeCheckedBuild)) {
                strcat(szOS, ".chk\\");
                strcat(szSP, ".chk\\");
            } else {
                strcat(szOS, "\\");
                strcat(szSP, "\\");
            }
        }


        // Update both paths
        strcat(szOS, "symbols\\");
        strcat(szSP, "symbols\\");

    } else if (':' == g_CfgData.m_szInstallSrcPath[1]) {
        strncpy(szOS, g_CfgData.m_szInstallSrcPath, 2);
        strcat(szOS, "\\support\\debug\\");

        switch (g_CfgData.m_si.wProcessorArchitecture) {
        default:
            Assert(0);
            break;
            
        case PROCESSOR_ARCHITECTURE_INTEL:
            strcat(szOS, "i386\\");
            break;
            
        case PROCESSOR_ARCHITECTURE_MIPS:
            strcat(szOS, "mips\\");
            break;
            
        case PROCESSOR_ARCHITECTURE_ALPHA:
            strcat(szOS, "alpha\\");
            break;
            
        case PROCESSOR_ARCHITECTURE_PPC:
            strcat(szOS, "ppc\\");
            break;
        }
        
        strcat(szOS, "symbols\\");
        // Copy the OS path to the SP (service pack) path
        strcpy(szSP, szOS);
    } else {
        // The OS was install from some location, but not anything
        // we're prepared to deal with, so simply copy the install 
        // path.
        strcat(szOS, g_CfgData.m_szInstallSrcPath);
        // Copy the OS path to the SP (service pack) path
        strcpy(szSP, szOS);
    }
    
    Assert(strlen(szOS) < sizeof(szOS));
    Assert(strlen(szSP) < sizeof(szSP));

    if (g_CfgData.m_pszOS_SymPath) {
        free(g_CfgData.m_pszOS_SymPath);
    }
    g_CfgData.m_pszOS_SymPath = NT4SafeStrDup(szOS);

    if (g_CfgData.m_pszSP_SymPath) {
        free(g_CfgData.m_pszSP_SymPath);
    }
    g_CfgData.m_pszSP_SymPath = NT4SafeStrDup(szSP);
}

void
CleanupAfterWizard(
    HWND hwnd
    )
{
    DeleteLinkOnDesktop(hwnd, g_CfgData.m_pszDeskTopShortcut_FullPath);
}

BOOL
GetDiskInfo(
    PSTR pszRoot, 
    PSTR & pszVolumeLabel, 
    DWORD & dwSerialNum, 
    UINT & uDriveType
    )
{
    BOOL bDeleteRootName = FALSE;
    DWORD dwMaxCompLen;
    DWORD dwFileSysFlags;

    if (NULL == pszRoot || !*pszRoot) {
        return FALSE;
    }

    // If pszRoot is a UNC name, you must follow it with an 
    // additional backslash. For example, you would specify 
    // \\MyServer\MyShare as \\MyServer\MyShare\. 
    if (!strncmp(pszRoot, "\\\\", 2)) {

        bDeleteRootName = TRUE;

        PSTR psz = (PSTR) calloc(strlen(pszRoot) +2, 1);
        strcpy(psz, pszRoot);
        strcat(psz, "\\");
        pszRoot = psz;
     
        // Only want the computer and share name
        psz += 2; // Skip the '\\\\'

        // Skip comp name
        psz = strchr(psz, '\\');
        Assert(psz);
        psz++;

        // Go to the end of the share name
        psz = strchr(psz, '\\');
        if (!psz) {
            free(pszRoot);
            return FALSE;
        }
        *(++psz) = NULL;
    
    } else if ('\\' == *pszRoot) {
        bDeleteRootName = TRUE;
        pszRoot = NT4SafeStrDup("\\");
    } else if (isalpha(*pszRoot) && ':' == *(pszRoot+1)) {

        // BUGBUG - kcarlos - Does not handle '..\..\test'
        bDeleteRootName = TRUE;
        // The most we have to copy 3 "c:\"
        PSTR psz = (PSTR) calloc(4, 1);
        strncpy(psz, pszRoot, 3);
        pszRoot = psz;
    }

    // Reset values
    if (pszVolumeLabel) {
        free(pszVolumeLabel);
        pszVolumeLabel = NULL;
    }
    pszVolumeLabel = (PSTR) calloc(_MAX_PATH, 1);
    dwSerialNum = 0;
    uDriveType = GetDriveType(pszRoot);

    BOOL bSuccess = GetVolumeInformation(pszRoot, pszVolumeLabel, _MAX_PATH,
        &dwSerialNum, &dwMaxCompLen, &dwFileSysFlags, NULL, 0);

    if (bDeleteRootName) {
        free(pszRoot);
    }

    return bSuccess;
}


BOOL
AreAnyOfTheFilesCompressed(
    BOOL bWarnUser,
    PSTR pszDirPath
    )
{
    if (bWarnUser) {
        PSTR pszWarning = WKSP_DynaLoadString(g_hInst, IDS_WARNING);
        WKSP_MsgBox(pszWarning, IDS_WARN_CHECK_IF_DECOMPRESSION_OF_SYMBOLS_REQUIRED);
        free(pszWarning);
    }

    BOOL bAnyCompressedFiles = FALSE;
    WIN32_FIND_DATA FindFileData;    
    HANDLE hFind;
    PSTR pszFileMask = (PSTR) calloc(_MAX_PATH * 2, 1);
    
    strcpy(pszFileMask, pszDirPath);
    strcat(pszFileMask, "\\");
    strcat(pszFileMask, "*.??_"); 

    //
    // Any compressed files in this directory?
    hFind = FindFirstFile(pszFileMask, &FindFileData);
    if (INVALID_HANDLE_VALUE != hFind) {
        bAnyCompressedFiles = TRUE;
        goto CLEANUP;
    }

    //
    // None, let's recurse
    strcpy(pszFileMask, pszDirPath);
    strcat(pszFileMask, "\\");
    strcat(pszFileMask, "*"); 

    hFind = FindFirstFile(pszFileMask, &FindFileData);
    if (INVALID_HANDLE_VALUE != hFind) {
        BOOL bContinue;
        do {
            if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY
                && strcmp(FindFileData.cFileName, ".")
                && strcmp(FindFileData.cFileName, "..") ) {

                // recurse into this directory
                strcpy(pszFileMask, pszDirPath);
                strcat(pszFileMask, "\\");
                strcat(pszFileMask, FindFileData.cFileName);

                if (AreAnyOfTheFilesCompressed(FALSE, pszFileMask)) {
                    bAnyCompressedFiles = TRUE;
                    goto CLEANUP;
                }
            }
            bContinue = FindNextFile(hFind, &FindFileData);
        } while (bContinue);
    }

CLEANUP:
    free(pszFileMask);

    if (INVALID_HANDLE_VALUE != hFind) {
        FindClose(hFind);
    }

    return bAnyCompressedFiles;
}

PTSTR
NT4SafeStrDup(
    PCTSTR psz
    )
/*++
Routine Description:
    MSVCRT.DLL from 1381 will AV when _strdup is passed a NULL
    argument.

--*/
{
    if (!psz) {
        return NULL;
    }

    return _strdup(psz);
}

