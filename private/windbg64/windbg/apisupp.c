/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    apisupp.c

Abstract:

    This file contains the set of routines which preform the interfacing
    between the debugger dlls and the shell exe.

Author:

    Jim Schaad (jimsch)

Environment:

    Win32 - User

--*/

/**************************** INCLUDES **********************************/

#include "precomp.h"
#pragma hdrstop


extern ULONG ulPseudo[];

/**********************************************************************/

extern char     in386mode;
extern LPSHF    Lpshf;
extern CXF      CxfIp;

char    is_assign;
LPTD    LptdFuncEval = NULL;

/**********************************************************************/

BOOL EstablishConnection(LPSTR);

BOOL SYSetProfileString(LPCSTR szName, LPCSTR szValue);
BOOL SYGetProfileString(LPCSTR szName, LPSTR szValue, ULONG cbValue,
                        ULONG * pcbRet);


/**********************************************************************/

// these are used only by Symbol serve routines
//       GetSymbolFileFromServer
//       CloseSymbolServer

HINSTANCE              ghSrv = 0;
CHAR                   gszSrvName[_MAX_PATH * 2];
LPSTR                  gszSrvParams = NULL; 
PSYMBOLSERVERPROC      gfnSymbolServer = NULL;
PSYMBOLSERVERCLOSEPROC gfnSymbolServerClose = NULL;

/**********************************************************************/

char *
__cdecl
t_itoa(
    int a,
    char * b,
    int c
    )
{
    return _itoa(a, b, c);
}

char * _cdecl t_ultoa(ulong a, char * b, int c)
{
    return _ultoa(a, b, c);
}


int
__cdecl
d_sprintf(
    char * a,
    const char * b,
    ...
    )
{
    char rgch[100];

    int returnvalue;
    va_list va_arg;
    va_start(va_arg, b);

    strcpy(rgch, b);
    returnvalue = vsprintf(a, (char *) rgch, va_arg) ;

    va_end(va_arg);
    return returnvalue;
}                                       /* d_sprintf() */

int
__cdecl
d_eprintf(
    const char * a,
    char * b,
    char * c,
    int d
    )
{
    char rgch1[100];
    char rgch2[100];
    char rgch3[100];
    char rgch4[100];

    strcpy(rgch1, a);
    strcpy(rgch2, b);
    strcpy(rgch3, c);

    sprintf(rgch4, rgch1, rgch2, rgch3, d);
    DebugBreak();
    return( strlen(rgch4) );
}                                       /* d_eprintf() */

TCHAR OutQ[10*2048]={0}; // Max 10 dprintfs in queue
ULONG indx=0;

void
DPRINTF(
    LPSTR fmt,
    ...
    ) {
    TCHAR *buff= &OutQ[indx*2048];
    va_list args;
    DWORD cb;

    va_start(args, fmt);

    indx = (indx + 1) % 10;
    cb = _vsnprintf(buff, 2047, fmt, args);
    buff[cb] = '\0';
    PostMessage( Views[cmdView].hwndClient, WU_LOG_REMOTE_MSG, TRUE, (LPARAM)buff );
    va_end(args);
}

//
// Symbol server stuff
//      

void
CloseSymbolServer(
    VOID
    )
{
    if (!ghSrv) 
        return;

    if (gfnSymbolServerClose)
        gfnSymbolServerClose();

    FreeLibrary(ghSrv);

    ghSrv = 0;
    *gszSrvName = 0;
    gszSrvParams = NULL;
    gfnSymbolServer = NULL;
    gfnSymbolServerClose = NULL;
}


BOOL
GetSymbolFileFromServer(
    IN  LPCSTR ServerInfo, 
    IN  LPCSTR FileName, 
    IN  DWORD  num1,
    IN  DWORD  num2,
    IN  DWORD  num3,
    OUT LPSTR FilePath
    )
{
    // initialize server, if needed

    if (!ghSrv) {
        ghSrv = (HINSTANCE) INVALID_HANDLE_VALUE;
        strcpy(gszSrvName, &ServerInfo[7]);
        if (!*gszSrvName) 
            return FALSE;
		// BUGBUG: gszSrvParams is no longer needed because it is 
        // now implemented in the local variable, 'params'
        // Still, we need to zero out this location to get gszSrvName.
        gszSrvParams = (PCHAR) strchr((PUCHAR) gszSrvName, '*');
        if (!gszSrvParams ) 
            return FALSE;
        *gszSrvParams++ = '\0';
        ghSrv = LoadLibrary(gszSrvName);
        if (ghSrv) {
            gfnSymbolServer = (PSYMBOLSERVERPROC)GetProcAddress(ghSrv, "SymbolServer");
            if (!gfnSymbolServer) {
                FreeLibrary(ghSrv);
                ghSrv = (HINSTANCE) INVALID_HANDLE_VALUE;
            }
            gfnSymbolServerClose = (PSYMBOLSERVERCLOSEPROC)GetProcAddress(ghSrv, "SymbolServerClose");
        } else {
            ghSrv = (HINSTANCE) INVALID_HANDLE_VALUE;
        }
    }

    // bail, if we have no valid server

    if (ghSrv == INVALID_HANDLE_VALUE)
        return FALSE;
	
		strcpy(gszSrvName, &ServerInfo[7]);
		gszSrvParams = (PCHAR) strchr((PUCHAR) gszSrvName, '*');
	  	*gszSrvParams++ = '\0';
    
    return gfnSymbolServer(gszSrvParams, FileName, num1, num2, num3, FilePath);
}


int
OSDAPI
LBQuit(
    DWORD ui
    )
{
    return 0;
}                               /* LBQuit() */

int
OSDAPI
AssertOut(
    LPSTR lszMsg,
    LPSTR lszFile,
    DWORD iln
    )
{
    ShowAssert(lszMsg, iln, lszFile);
    return TRUE;
}                                       /* AssertOut() */

VOID
DLoadedSymbols(
    SHE   she,
    LSZ   pszPath
    )
{
    HEXE    emi;
    LPSTR   pszErrText;
    LPSTR   pszModName;
    DWORD   dwLen;


    emi = SHGethExeFromName((char *)pszPath);
    Assert(emi != 0);

    ModListModLoad( SHGetExeName( emi ), she );
    if (she != sheSuppressSyms && g_contWorkspace_WkSp.m_bVerbose) {
        pszErrText = SHLszGetErrorText(she);
        if (she == sheNoSymbols) {
            pszModName = SHGetExeName( emi );
        } else {
            pszModName = SHGetSymFName( emi );
        }
        if (NULL == pszModName || 0 == strlen(pszModName)) {
            // Let's at least print something out.
            Assert(!"A function maimed the name of the module. Ignorable error.");
            pszModName = pszPath;
        }

        dwLen = 32;

        if (pszErrText) {
            dwLen += strlen(pszErrText);
        }
        dwLen += strlen(pszModName);

        LPSTR pszBuf = (LPSTR) malloc( dwLen );
        Assert(pszBuf);
        sprintf( pszBuf, "Module Load: %s  (%s)\r\n", pszModName, pszErrText ? pszErrText : "" );
        PostMessage( Views[cmdView].hwndClient, WU_LOG_REMOTE_MSG, TRUE, (LPARAM)pszBuf );
    }
    return;
}

BOOL 
SYGetDefaultShe( 
    LSZ pszModuleName, 
    SHE *pShe
    )
/*++
Description:
    This is a callback for the back end to determine whether symbols
    should be loaded at startup or defered until needed.

Arguments:
    pszModuleName - if NULL we are getting the default symbol loading behavior
            
            If not NULL, then the front end is being queryed as to whether the
            symbol loading behavior is different than the default behavior. 
            
            This is not a hack but the design of the debugger. Previously, 
            all module loading behavior could be specified by the user, but 
            this was causing some serious problems since users could never 
            really figure out what or why some symbols were loaded at specific
            times.
            
    pShe -  Indicates when the symbols should be loaded:
                sheNone - Load symbols for this module at startup
                sheDefer - Defer symbols loading until the module is needed.
                sheSuppress - Suppress symbol loading for this module.

Returns:
    TRUE - success
    FALSE - failure
--*/
{
    TCHAR szFName[_MAX_FNAME];
    TCHAR szExt[_MAX_EXT];

    if ( pszModuleName ) {

        // Load these symbols at startup because the kernel debugger needs them.
        
        _tsplitpath( pszModuleName, NULL, NULL, szFName, szExt );

        if ( !_tcsicmp( szFName, _T("nt") ) ||
            ( !_tcsicmp( szFName, _T("hal") ) && !_tcsicmp( szExt, _T(".dll") ) )
            ) {
        
            *pShe = sheNone; // sheNone - means load at startup
            return TRUE;

        }
    
    }
        
    return ModListGetDefaultShe( pszModuleName, pShe );
}

void
LBLog(
    LSZ lsz
)
{
    OutputDebugString(lsz);
    return;
}                                       /* LBLog() */

/**********************************************************************/

PVOID
MHAlloc(
    size_t cb
    )
{
    return malloc(cb);
}                                       /* MHAlloc() */

PVOID
MHAllocHuge(
    LONG l,
    UINT ui
    )
{
    return MHAlloc(l * ui );
}                                       /* MHAllocHuge() */

PVOID
MHRealloc(
    PVOID lpv,
    size_t cb
    )
{
    return realloc(lpv, cb);
}                                       /* MHRealloc() */

LPTSTR
MHStrdup(
    LPCTSTR p
    )
{
    return _tcsdup(p);
}

void
MHFree(
    PVOID lpv
    )
{
    free(lpv);
}                                       /* MHFree() */

/**********************************************************************/

DWORD
OSDAPI
DHGetNumber(
    LPSTR String,
    LPLONG Result
    )
/*++

Routine Description:

    Evaluate an expression, returning a long.


Arguments:

    lpv - supplies pointer to the string to convert to a number.

    result - returns the value of the expression as a long

Returns:

   the error code returned by CPGetCastNbr

--*/
{
    return (DWORD) CPGetCastNbr ( String,
                                  T_LONG,
                                  radix,
                                  fCaseSensitive,
                                  &CxfIp,
                                  (LPSTR) Result,
                                  NULL,
                                  g_contWorkspace_WkSp.m_bMasmEval
                                  );
}

HDEP
MMAllocHmem(
    size_t cb
    )
{
    return (HDEP) calloc(1, cb);
}                                       /* MMAllocHmem() */

HDEP
MMReallocHmem(
    HDEP hmem,
    size_t cb
    )
{
    LPBYTE p = (LPBYTE)hmem;
    size_t s = p ? _msize(p) : 0;

    if (cb > s) {
        p = (LPBYTE) realloc(p, cb);
        if (p) {
            ZeroMemory(p+s, cb-s);
        }
    }
    return (HDEP)p;
}                                       /* MMReallocHmem() */

VOID
MMFreeHmem(
    HDEP hmem
    )
{
    free((LPVOID)hmem);
}                                       /* MMFreeHmem() */

LPVOID
MMLpvLockMb(
    HDEP hmem
    )
{
    return (LPVOID)hmem;
}                                       /* MMLpvLockMb() */

void
MMbUnlockMb(
    HDEP hmem
    )
{
    return;
}                                       /* MMbUnlockMb() */

/**********************************************************************/

HANDLE
SYOpen(
    LSZ lsz
    )
{
    DWORD dwShareAttributes;

    EstablishConnection(lsz);

    if (OsVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {
        dwShareAttributes = (FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE);
    } else {
        dwShareAttributes = (FILE_SHARE_READ | FILE_SHARE_WRITE);
    }

    return CreateFile(lsz,
                     GENERIC_READ,
                     dwShareAttributes,
                     NULL,
                     OPEN_EXISTING,
                     0,
                     NULL);
}                                       /* SYOpen() */

VOID
SYClose(
    HANDLE hFile
    )
{
    CloseHandle((HANDLE) hFile);
    return;
}                                       /* SYClose() */

UINT
SYRead(
    HANDLE  hFile,
    LPB     pch,
    UINT    cb
    )
{
    DWORD dwBytesRead;
    ReadFile(hFile, pch, cb, &dwBytesRead, NULL);
    return (dwBytesRead);
}                                       /* SYRead() */

long
SYLseek(
    HANDLE  hFile,
    LONG    cbOff,
    UINT    iOrigin
    )
{
    return SetFilePointer(hFile, cbOff, NULL, iOrigin);
}                                       /* SYLseek() */

long
SYTell(
    HANDLE  hFile
    )
{
    return SetFilePointer(hFile, 0, NULL, FILE_CURRENT);
}                                       /* SYTell() */


static LPSTR LpBaseDir = NULL;

VOID
SetFindExeBaseName(
    LPSTR lpName
    )
{
    LPSTR p;

    if (LpBaseDir) {
        free(LpBaseDir);
        LpBaseDir = 0;
    }

    if (lpName) {
        // search from tail for '\\'
        if ( p = strrchr(lpName, '\\') ) {
            if (p == lpName || (p == (lpName + 2) && lpName[1] == ':')) {
              if (!IsDBCSLeadByte(*lpName))
                p += 1;
            }
        } else if ( lpName[0] != '\0' && lpName[1] == ':' ) {
          if (!IsDBCSLeadByte(*lpName))
            p = lpName + 2;
        }
        if (p) {
            LpBaseDir = (LPSTR) malloc((size_t)(p - lpName) + 1);
            Assert(LpBaseDir);
            strncpy(LpBaseDir, lpName, (size_t)(p - lpName));
            LpBaseDir[p-lpName] = '\0';
        }
    }
}


BOOL
IsExistingConnection(
    LPSTR RemoteName
    )
{
    DWORD        rc;
    HANDLE       hEnum;
    DWORD        Entries;
    NETRESOURCE  *nrr = NULL;
    DWORD        cb;
    DWORD        i;
    DWORD        ss;
    BOOL         rval = FALSE;


    rc = WNetOpenEnum( RESOURCE_CONNECTED, RESOURCETYPE_ANY, 0, NULL, &hEnum );
    if (rc != NO_ERROR) {
        return FALSE;
    }

    ss = 0;
    cb = 64 * 1024;
    nrr = (NETRESOURCE *) calloc( cb, sizeof(char));
    Assert(nrr);

    while( TRUE ) {
        Entries = (DWORD)-1;
        rc = WNetEnumResource( hEnum, &Entries, nrr, &cb );
        if (rc == ERROR_NO_MORE_ITEMS) {
            break;
        } else if (rc == ERROR_MORE_DATA) {
            cb += 16;
            nrr = (NETRESOURCE *) realloc( nrr, cb );
            Assert(nrr);
            ZeroMemory( nrr, cb );
            continue;
        } else if (rc != NO_ERROR) {
            break;
        }
        for (i=0; i<Entries; i++) {
            if (_stricmp( nrr[i].lpRemoteName, RemoteName ) == 0) {
                rval = TRUE;
                break;
            }
        }
    }

    free( nrr );
    WNetCloseEnum( hEnum );

    return rval;
}


BOOL
EstablishConnection(
    LPSTR FileName
    )
{
    NETRESOURCE  nr;
    DWORD        rc;
    DWORD        i;
    LPSTR        RemoteName;
    LPSTR        p;



    if ((FileName[0] != '\\') || (FileName[1] != '\\')) {
        return FALSE;
    }
    p = (LPSTR) strchr( (LPBYTE) &FileName[2], '\\' );
    if (!p) {
        //
        // malformed name
        //
        return FALSE;
    }
    p = (LPSTR) strchr( (LPBYTE) p+1, '\\' );
    if (!p) {
        p = &FileName[strlen(FileName)];
    }
    i = (DWORD)(p - FileName);
    RemoteName = (LPSTR) malloc( i + 4 );
    Assert(RemoteName);
    strncpy( RemoteName, FileName, i );
    RemoteName[i] = 0;

    if (IsExistingConnection( RemoteName )) {
        free( RemoteName );
        return TRUE;
    }

    nr.dwScope        = 0;
    nr.dwType         = RESOURCETYPE_DISK;
    nr.dwDisplayType  = 0;
    nr.dwUsage        = 0;
    nr.lpLocalName    = NULL;
    nr.lpRemoteName   = RemoteName;
    nr.lpComment      = NULL;
    nr.lpProvider     = NULL;

    rc = WNetAddConnection2( &nr, NULL, NULL, 0 );
    if (rc != NO_ERROR) {
        free( RemoteName );
        return FALSE;
    }

    free( RemoteName );
    return TRUE;
}

void
DeleteSymFileLoadErrList(
    SYM_FILE_LOAD_ERR * const pListHead
    )
{
    Assert(pListHead);

    PSYM_FILE_LOAD_ERR pCur = pListHead;

    do {
        if (pCur->hSymFile) {
            CloseHandle(pCur->hSymFile);
            pCur->hSymFile = NULL;
        }
        if (pCur->pszExtendedSHE) {
            free(pCur->pszExtendedSHE);
            pCur->pszExtendedSHE = NULL;
        }
        if (pCur->pszSHE) {
            free(pCur->pszSHE);
            pCur->pszSHE = NULL;
        }

        if (pListHead != pCur) {
            RemoveEntryList( (PLIST_ENTRY) pCur );
            free(pCur);
        }

        pCur = pListHead->Flink;
    } while ( !IsListEmpty( (PLIST_ENTRY) pListHead ) );
}


BOOL
CALLBACK
FindDebugFileCallback(
    HANDLE hFileHandle,
    LPSTR pszFileName,
    PVOID pvCallerData
    )
/*++
Routine Description:

Arguments:
    FileHandle - Handle to a symbol file

    FileName - path to the symbol file

    CallerData - Validation data....
--*/
{
    Assert(hFileHandle);
    Assert(pszFileName);
    Assert(pvCallerData);

    SHE                 sheTmp;
    PFIND_SYM_FILE      pFindSymFileData = (PFIND_SYM_FILE) pvCallerData;
    SYM_FILE_LOAD_ERR   TmpLoadErr = {0};


    Assert(_tcslen(pszFileName) < sizeof(TmpLoadErr.szSymFilePath) / sizeof(TCHAR));

    //
    // Make sure that the head of the list has been initialized.
    //
    Assert(pFindSymFileData->LoadErr.Flink);
    Assert(pFindSymFileData->LoadErr.Blink);
    Assert(pFindSymFileData->pfnValidateExe);

    //
    // Copy data into the TmpLoadErr struct
    //
    _tcscpy(TmpLoadErr.szSymFilePath, pszFileName);
    TmpLoadErr.hSymFile = NULL;
    TmpLoadErr.she = pFindSymFileData->pfnValidateExe( hFileHandle,
                                                      pFindSymFileData->pVldChk);

    //
    // Use a temp value for the switch so we don't have to use a goto, or some uglier
    // hack. We don't want to modify 'TmpLoadErr.she' because we need to preserve the
    // actual sym handler error.
    sheTmp = TmpLoadErr.she;

    // Ignore checksum errors for NTDLL.DLL and KERNEL32.DLL
    if (sheBadChecksum == sheTmp) {
        TCHAR szFName[_MAX_FNAME];
        TCHAR szExt[_MAX_EXT];

        _splitpath(pFindSymFileData->szImageFilePath, NULL, NULL, szFName, szExt);

        if ( !_tcsicmp(szExt, ".dll") ) {
            // We have a dll. Is it ntdll or kernel32?
            if ( !_tcsicmp(szFName, "ntdll") || !_tcsicmp(szFName, "kernel32") ) {
                // Lie, and say that there were no errors loading either one of these
                sheTmp = sheNone;
            }
        }
    }

    switch ( sheTmp ) {
    default:
        // Since we are returning FALSE, DbgHelp will close the handle to this file,
        // and we need to keep it open, so let's duplicate the file handle
        Dbg(DuplicateHandle(GetCurrentProcess(),
                                 hFileHandle,
                                 GetCurrentProcess(),
                                 &TmpLoadErr.hSymFile,
                                 0,
                                 TRUE,
                                 DUPLICATE_SAME_ACCESS
                                 ));

        // Append all of the symbol files that were examined, and any load errors
        if (sheBadChecksum == TmpLoadErr.she) {

            // extended error text associated with this error.
            TmpLoadErr.pszExtendedSHE = WKSP_DynaLoadStringWithArgs(g_hInst,
                                        DBG_Checksum_Mismatch,
                                        pFindSymFileData->pVldChk->SymCheckSum,
                                        pFindSymFileData->pVldChk->ImgCheckSum
                                        );

        } else if (sheBadTimeStamp == TmpLoadErr.she) {

            // extended error text associated with this error.
            TmpLoadErr.pszExtendedSHE = WKSP_DynaLoadStringWithArgs(g_hInst,
                                        DBG_Timestamp_Mismatch,
                                        pFindSymFileData->pVldChk->SymTimeDateStamp,
                                        pFindSymFileData->pVldChk->ImgTimeDateStamp
                                        );
        }

        TmpLoadErr.pszSHE = _strdup(SHLszGetErrorText(TmpLoadErr.she));

        // Do we need to create a new list entry?
        if (!pFindSymFileData->LoadErr.hSymFile) {
            // The list head hasn't been used, use it now.
            pFindSymFileData->LoadErr = TmpLoadErr;
            InitializeListHead((PLIST_ENTRY) &pFindSymFileData->LoadErr);
        } else {
            // The list head has been used, create a new entry.
            PSYM_FILE_LOAD_ERR pLoadErr = (PSYM_FILE_LOAD_ERR)
                malloc( sizeof(SYM_FILE_LOAD_ERR) );

            *pLoadErr = TmpLoadErr;
            InsertTailList((PLIST_ENTRY) &pFindSymFileData->LoadErr, (PLIST_ENTRY) pLoadErr);
        }

        // File did not contain symbolic info, keep searching.
        pFindSymFileData->bFound = FALSE;
        return FALSE; // keep searching

    case sheNoSymbols:
        // File did not contain symbolic info, keep searching. If it doesn't have any symbolic
        // info, don't list, it won't do us any good anyways.
        pFindSymFileData->bFound = FALSE;

        // Let DbgHelp close the file handle

        return FALSE; // keep searching

    case sheNone:
        // We found the correct symbols. We can stop searching.
        // Now we have to delete the entire list of previously
        // tested symbols.
        pFindSymFileData->bFound = TRUE;

        // If we return TRUE, DbgHelp will not close the file handle
        TmpLoadErr.hSymFile = hFileHandle;

        // Iinform the user of any error codes.
        if (sheBadChecksum == TmpLoadErr.she) {

                        // extended error text associated with this error.
            TmpLoadErr.pszExtendedSHE = WKSP_DynaLoadStringWithArgs(g_hInst,
                                        DBG_Checksum_Mismatch,
                                        pFindSymFileData->pVldChk->SymCheckSum,
                                        pFindSymFileData->pVldChk->ImgCheckSum
                                        );

        } else if (sheBadTimeStamp == TmpLoadErr.she) {

                        // extended error text associated with this error.
            TmpLoadErr.pszExtendedSHE = WKSP_DynaLoadStringWithArgs(g_hInst,
                                        DBG_Timestamp_Mismatch,
                                        pFindSymFileData->pVldChk->SymTimeDateStamp,
                                        pFindSymFileData->pVldChk->ImgTimeDateStamp
                                        );
        }

        TmpLoadErr.pszSHE = _strdup(SHLszGetErrorText(TmpLoadErr.she));

        // Delete the entire list.
        DeleteSymFileLoadErrList(&pFindSymFileData->LoadErr);

        // The copy the symbol data.
        pFindSymFileData->LoadErr = TmpLoadErr;
        InitializeListHead( (PLIST_ENTRY) &pFindSymFileData->LoadErr );

        return TRUE; // Stop searching
    }
}

BOOL
SYIgnoreAllSymbolErrors(
    VOID
    )
{
    return g_contWorkspace_WkSp.m_bIgnoreAllSymbolErrors;
}

LPSTR
ExpandPath(
    LPSTR lpPath
    )
{
    LPSTR   p, newpath, p1, p2, p3;
    CHAR    envvar[MAX_PATH];
    CHAR    envstr[MAX_PATH];
    ULONG   i, PathMax;

    if (!lpPath) {
        return(NULL);
    }

    p = lpPath;
    PathMax = strlen(lpPath) + MAX_PATH + 1;
    p2 = newpath = (LPSTR) MHAlloc( PathMax );

    if (!newpath) {
        return(NULL);
    }

    while( p && *p) {
        if (*p == '%') {
            i = 0;
            p++;
            while (p && *p && *p != '%') {
                envvar[i++] = *p++;
            }
            p++;
            envvar[i] = '\0';
            p1 = envstr;
            *p1 = 0;
            GetEnvironmentVariable( envvar, p1, MAX_PATH );
            while (p1 && *p1) {
                *p2++ = *p1++;
                if (p2 >= newpath + PathMax) {
                    PathMax += MAX_PATH;
                    p3 = (PTSTR) MHRealloc(newpath, PathMax);
                    if (!p3) {
                        MHFree(newpath);
                        return(NULL);
                    } else {
                        p2 = p3 + (p2 - newpath);
                        newpath = p3;
                    }
                }
            }
        }
        *p2++ = *p++;
        if (p2 >= newpath + PathMax) {
            PathMax += MAX_PATH;
            p3 = (PTSTR) MHRealloc(newpath, PathMax);
            if (!p3) {
                MHFree(newpath);
                return(NULL);
            } else {
                p2 = p3 + (p2 - newpath);
                newpath = p3;
            }
        }
    }
    *p2 = '\0';

    return newpath;
}



HANDLE
FindExecutableImageEx2(
    LPSTR FileName,
    LPSTR SymbolPath,
    LPSTR ImageFilePath,
    PFIND_EXE_FILE_CALLBACK Callback,
    PVOID CallerData
    )
{
    PTSTR Start;
    PTSTR End;
    HANDLE FileHandle = NULL;
    TCHAR DirectoryPath[ MAX_PATH ];
    PTSTR NewSymbolPath = NULL;
    FIND_SYM_FILE *pFileData;
    CHAR Drive[_MAX_DRIVE], Dir[_MAX_DIR], SubDirPart[_MAX_DIR], FilePart[_MAX_FNAME], Ext[_MAX_EXT];
    BOOL symsrv=TRUE;

    __try {
        __try {
            _splitpath(FileName, Drive, Dir, FilePart, Ext);
            
            NewSymbolPath = ExpandPath(SymbolPath);

            if (GetFullPathName( FileName, MAX_PATH, ImageFilePath, &Start )) {
                //DPRINTF(NULL, "FindExecutableImageEx-> Looking for %s... ", ImageFilePath);
                FileHandle = CreateFile( ImageFilePath,
                                         GENERIC_READ,
                                         OsVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT ? (FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE) : (FILE_SHARE_READ | FILE_SHARE_WRITE),
                                         NULL,
                                         OPEN_EXISTING,
                                         0,
                                         NULL
                                       );

                if (FileHandle != INVALID_HANDLE_VALUE) {
                    if (Callback) {
                        if (!Callback(FileHandle, ImageFilePath, CallerData)) {
                            //EPRINTF(NULL, "mismatched timestamp\n");
                            CloseHandle(FileHandle);
                            FileHandle = INVALID_HANDLE_VALUE;
                        }
                    }
                    if (FileHandle != INVALID_HANDLE_VALUE) {
                        //EPRINTF(NULL, "OK\n");
                        MHFree( NewSymbolPath );
                        return FileHandle;
                    }
                } else {
                    //EPRINTF(NULL, "no file\n");
                }
            }

            Start = NewSymbolPath;
            while (Start && *Start != '\0') {
                if (End = _tcschr( Start, ';' )) {
                    int Len = (int)(End - Start);
                    Len = min(Len, sizeof(DirectoryPath)-1);

                    strncpy( DirectoryPath, Start, Len );
                    DirectoryPath[ Len ] = '\0';
                    End += 1;
                } else {
                    strcpy( (PCHAR) DirectoryPath, Start );
                }

                if (!_strnicmp(DirectoryPath, "SYMSRV*", 7)) {
                    TCHAR FilePath[_MAX_PATH];

//                    DPRINTF("FindExecutableImageEx-> Searching %s for %s \r\n", DirectoryPath, FileName);
                    
                    *ImageFilePath = 0;
                    pFileData = (FIND_SYM_FILE *)CallerData;
                    if (symsrv && pFileData) {
                        ULONG ImageSize=pFileData->pVldChk->ImgSize;

//                        DPRINTF("TS %x, Sz %x ... \r\n", pFileData->pVldChk->ImgTimeDateStamp, pFileData->pVldChk->ImgSize);
                        strcpy(FilePath, FilePart);
                        strcat(FilePath, ".dbg");
                        GetSymbolFileFromServer(DirectoryPath,
                                                FilePath,
                                                pFileData->pVldChk->ImgTimeDateStamp,
                                                pFileData->pVldChk->ImgSize,
                                                0,
                                                ImageFilePath);
                        symsrv = FALSE;
//                        DPRINTF("FindExecutableImageEx-> Opening %s... \r\n", ImageFilePath);
                        if (ImageFilePath[0]) {
//                            DPRINTF("found \n");
                            goto found;
                        }
                    }
//                        goto next;
                } else if (SearchTreeForFile( (PCHAR) DirectoryPath, FileName, ImageFilePath )) {
                    //DPRINTF(NULL, "FindExecutableImageEx-> Searching %s for %s... ", DirectoryPath, FileName);
                    
                    //EPRINTF(NULL, "found\n");
                    //DPRINTF(NULL, "FindExecutableImageEx-> Opening %s... ", ImageFilePath);
found:
                    pFileData = (FIND_SYM_FILE *)CallerData;
                    if (symsrv && pFileData) {
//                        DPRINTF("TS %x, Sz %x ... \r\n", pFileData->pVldChk->ImgTimeDateStamp, pFileData->pVldChk->ImgSize);
                    }
                    FileHandle = CreateFile( ImageFilePath,
                                             GENERIC_READ,
                                             OsVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT ? (FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE) : (FILE_SHARE_READ | FILE_SHARE_WRITE),
                                             NULL,
                                             OPEN_EXISTING,
                                             0,
                                             NULL
                                           );

                    if (FileHandle != INVALID_HANDLE_VALUE) {
                        if (Callback) {
                            if (!Callback(FileHandle, ImageFilePath, CallerData)) {
                                //EPRINTF(NULL, "mismatched timestamp\n");
                                CloseHandle(FileHandle);
                                FileHandle = INVALID_HANDLE_VALUE;
                            }
                        }
                        if (FileHandle != INVALID_HANDLE_VALUE) {
                            //EPRINTF(NULL, "OK\n");
                            MHFree( NewSymbolPath );
                            return FileHandle;
                        }
                    } else {
                        //EPRINTF(NULL, "no file\n");
                    }
                } else {
                    //EPRINTF(NULL, "no file\n");
                }

//next:
                Start = End;
				symsrv = TRUE;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            SetLastError(ERROR_INVALID_PARAMETER);
        }

        ImageFilePath[0] = '\0';

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    if (FileHandle) {
        CloseHandle(FileHandle);
    }

    if (NewSymbolPath) {
        MHFree( NewSymbolPath );
    }

    return NULL;
}


HANDLE
SYFindExeFile(
    PTSTR             pszFile,
    PTSTR             pszFound,
    UINT              cchFound,
    PVLDCHK           pVldChk,
    PFNVALIDATEEXE    pfnValidateExe,
    SHE              *pSheLoadError
    )
/*++
Arguments:
    pszFile     - Image file we want to find symbols for.

    pszFound    - Path to the symbol file that will be used for
                    the image.

    cchFound    - Size of pszFound

    pVldChk - Data used by the validation function to
                    match the image against the symbol file.

    pfnValidateExe - Validation function. Uses the validation data
                    to match the image with its respective symbol
                    file

    pSheLoadError - Any load error that was found while trying
                    to match the symbol file to the image.

Returns
    Success - File handle to the open symbol file.
    Failure - (-1)
--*/
{
    Assert(pfnValidateExe);
    Assert(pszFile);
    Assert(pszFound);
    Assert(cchFound);

    PTSTR                       pzSearchPath = NULL;
    TCHAR                       szFname[_MAX_FNAME];
    TCHAR                       szExt[_MAX_EXT];
    TCHAR                       szDrive[_MAX_DRIVE];
    TCHAR                       szDir[_MAX_DIR];
    TCHAR                       szFileName[MAX_PATH];
    SHE                         she;
    TCHAR                       rgch[MAX_PATH];
    TCHAR                       szBuf[MAX_PATH];
    DWORD                       len;
    DWORD                       cSP;
    HANDLE                      rVal = INVALID_HANDLE_VALUE; // Not found
    FIND_SYM_FILE               FindSymFileData = {0};
    SYM_FILE_LOAD_ERR           TmpLoadErr = {0};
    PSYM_FILE_LOAD_ERR          pCurLoadErr = NULL;


    // Initialize
    InitializeListHead( (PLIST_ENTRY) &FindSymFileData.LoadErr );

    if (!pszFile || !*pszFile) {
        rVal = INVALID_HANDLE_VALUE;
        goto finished_searching;
    }

    if (*pszFile == '#') {
        pszFile += 3;
    }

    _tsplitpath( pszFile, szDrive, szDir, szFname, szExt);
    sprintf( szFileName, "%s%s", szFname, szExt );

    cSP = ModListGetSearchPath(NULL, 0) + strlen(szDrive) + strlen(szDir) + 2;
    if (!cSP) {
        pzSearchPath = _strdup("");
    } else {
        pzSearchPath = (LPSTR) malloc(cSP);
        Assert(pzSearchPath);
        sprintf( pzSearchPath, "%s%s", szDrive, szDir );
        len = strlen(pzSearchPath);
        if (len) {
            pzSearchPath[len++] = ';';
            pzSearchPath[len] = 0;
        }
        ModListGetSearchPath( &pzSearchPath[len], cSP-len );
        len = strlen(pzSearchPath);
        Assert(len < cSP);
        if (*pzSearchPath && pzSearchPath[len-1] == ';') {
            //
            //  Kill the trailing semi-colon
            //
            pzSearchPath[len-1] = 0;
        }
    }

    FindSymFileData.pfnValidateExe = pfnValidateExe;
    FindSymFileData.pVldChk = pVldChk;
    _tcscpy(FindSymFileData.szImageFilePath, szFileName);

    sprintf( szFileName, "%s%s", szFname, szExt );
    // Does the image contain debug info?
    FindExecutableImageEx(szFileName,
                          pzSearchPath, 
                          rgch,
                          FindDebugFileCallback, 
                          &FindSymFileData
                          );
    if (FindSymFileData.bFound) {
        // The image contains debug info
        goto done_found;
    }
    //
    // The image didn't contain debug info, search the sym path
    //
    FindDebugInfoFileEx(szFileName, 
                        pzSearchPath, 
                        rgch,
                        FindDebugFileCallback, 
                        &FindSymFileData
                        );

    if (!FindSymFileData.bFound) {
        FindExecutableImageEx2(szFileName,
                               pzSearchPath, 
                               rgch,
                               FindDebugFileCallback, 
                               &FindSymFileData
                               );
    }

finished_searching:
    if (!FindSymFileData.bFound) {
        // None of the symbols on the search path are valid
        if (g_contWorkspace_WkSp.m_bIgnoreAllSymbolErrors) {

            // Don't prompt the user.
            // Load the first symbol with one of the following ignorable errors:
            //  sheBadChecksum, sheBadTimeStamp, sheCouldntReadImageHeader

            pCurLoadErr = &FindSymFileData.LoadErr;
            do {
                if (sheBadChecksum == pCurLoadErr->she
                    || sheBadTimeStamp == pCurLoadErr->she
                    || sheCouldntReadImageHeader == pCurLoadErr->she) {

                    // Store the sym file we picked.
                    TmpLoadErr = *pCurLoadErr;
                    // Zero these values out so they don't get freed/closed.
                    pCurLoadErr->hSymFile = NULL;
                    pCurLoadErr->pszExtendedSHE = NULL;
                    pCurLoadErr->pszSHE = NULL;

                    // Free the list
                    DeleteSymFileLoadErrList(&FindSymFileData.LoadErr);

                    // Place the data in the list head
                    FindSymFileData.LoadErr = TmpLoadErr;

                    FindSymFileData.bFound = TRUE;
                    goto done_found;
                }
                pCurLoadErr = pCurLoadErr->Flink;
            } while (&FindSymFileData.LoadErr != pCurLoadErr);

            // We didn't find any sym file with an ignorable load error.
            rVal = INVALID_HANDLE_VALUE;

            // Free the list
            DeleteSymFileLoadErrList(&FindSymFileData.LoadErr);

        } else {
            if (!g_contWorkspace_WkSp.m_bBrowseForSymsOnSymLoadErrors) {
                // User doesn't want to be bothered with browsing
                rVal = INVALID_HANDLE_VALUE;

                // Free the list
                DeleteSymFileLoadErrList(&FindSymFileData.LoadErr);

            } else {

                INT_PTR nRes = ExeBrowseBadSym(&FindSymFileData);

                if (IDCANCEL == nRes) {

                    // User didn't select or specify file
                    rVal = INVALID_HANDLE_VALUE;
                    FindSymFileData.bFound = FALSE;

                    // Free the list
                    DeleteSymFileLoadErrList(&FindSymFileData.LoadErr);

                } else if (IDC_BUT_LOAD == nRes || IDC_BUT_BROWSE == nRes) {
                    // Make sure it has been initialized
                    Assert(OsVersionInfo.dwOSVersionInfoSize);

                    //
                    // At this point all we have is the file name. And it is stored in
                    // FindSymFileData.LoadErr.szSymFilePath
                    //

                    
                    // Free the list
                    DeleteSymFileLoadErrList(&FindSymFileData.LoadErr);

                    // Now open the file
                    FindSymFileData.LoadErr.hSymFile =
                                          SYOpen(FindSymFileData.LoadErr.szSymFilePath);
                    
                    if (INVALID_HANDLE_VALUE == FindSymFileData.LoadErr.hSymFile) {

                        //
                        // Couldn't open the symbol file.
                        // 
                        FindSymFileData.LoadErr.she = sheFileOpen;
                        FindSymFileData.bFound = FALSE;

                    } else {

                        _tsplitpath(FindSymFileData.LoadErr.szSymFilePath,
                                    NULL,
                                    NULL,
                                    NULL,
                                    szExt
                                    );
                        if ( !_tcsicmp(".pdb", szExt) ) {

                            //
                            // Ignore trying to validate PDB files. Just mark them
                            // as being ok. The function that called this one will
                            // correctly validate the PDB.
                            //
                            FindSymFileData.LoadErr.she = sheNone;

                        } else {
                            
                            //
                            // User wants us to try another file
                            // Valid sym file? 
                            //
                            FindDebugFileCallback(FindSymFileData.LoadErr.hSymFile,
                                                  FindSymFileData.LoadErr.szSymFilePath, 
                                                  &FindSymFileData
                                                  );
                        }

                        // Just to humor the user, we're going to load this file
                        // regardless of any errors.
                        FindSymFileData.bFound = TRUE;

                    }

                } else {
                    Assert(0);
                }
            }
        }
    }

done_found:
   //  DPRINTF("Mod %s : TS %x, Sz %x ... \r\n", pszFile, FindSymFileData.pVldChk->ImgTimeDateStamp, FindSymFileData.pVldChk->ImgSize);
    if (FindSymFileData.bFound) {
        PTSTR msgBuf = (PTSTR) malloc( MAX_VAR_MSG_TXT * sizeof(TCHAR) );

//        DPRINTF("found ");
        sprintf( msgBuf, 
                "%s for %s (%s%s%s)\n",
                FindSymFileData.LoadErr.szSymFilePath,
                pszFile,
                FindSymFileData.LoadErr.pszSHE ? FindSymFileData.LoadErr.pszSHE : "",
                // Add a space between the error msg and the extended info
                FindSymFileData.LoadErr.pszExtendedSHE ? " " : "",
                FindSymFileData.LoadErr.pszExtendedSHE ? FindSymFileData.LoadErr.pszExtendedSHE : ""
                );
        PostMessage( Views[cmdView].hwndClient, WU_LOG_REMOTE_MSG, TRUE, (LPARAM)msgBuf );

        rVal = FindSymFileData.LoadErr.hSymFile;
        strncpy(pszFound, FindSymFileData.LoadErr.szSymFilePath, cchFound -1);
        pszFound[cchFound -1] = NULL;
    }

    if (pSheLoadError) {
        *pSheLoadError = FindSymFileData.LoadErr.she;
    }

    if (pzSearchPath) {
        free(pzSearchPath);
    }

    // Zero these values out so they don't get freed/closed.
    FindSymFileData.LoadErr.hSymFile = NULL;

    // Free the list
    DeleteSymFileLoadErrList(&FindSymFileData.LoadErr);
                        
    return rVal;
}


/**********************************************************************/

XOSD
SYUnFixupAddr(
    LPADDR lpaddr
    )
{
    if (LppdCur == NULL) {
        return xosdNone;
    }

    return OSDUnFixupAddr(LppdCur->hpid, NULL, lpaddr);
}

XOSD
SYFixupAddr(
    LPADDR paddr
    )
{
    if (LppdCur == NULL) {
        return xosdNone;
    }

    return OSDFixupAddr(LppdCur->hpid, NULL, paddr);
}

XOSD
SYSanitizeAddr(
    LPADDR paddr
    )
{
    XOSD xosd;
    emiAddr(*paddr) = 0;
    if ((xosd = SYUnFixupAddr(paddr)) != xosdNone) {
        return xosd;
    }
    return SYFixupAddr(paddr);
}

UINT SYProcessor(VOID)
{
    long l = 0;

    Assert(LppdCur);

    OSDGetDebugMetric ( LppdCur->hpid, 0, mtrcProcessorType, &l );

    return (int) l;
}                                       /* SYProcessor() */

void
SYSetEmi (
    HPID hpid,
    HTID htid,
    LPADDR lpaddr
    )
{
    Assert ( ! ADDR_IS_LI ( *lpaddr ) );

    OSDSetEmi(hpid, htid, lpaddr);

    return;
}                                       /* SYSetEmi() */

BOOL
SYGetAddr(
    HFRAME  hframe,
    LPADDR  lpaddr,
    ADR     addrType
    )
{
    if ((LppdCur == NULL) || (hframe == NULL)) {
        return FALSE;
    }

    return OSDGetAddr ( LppdCur->hpid, (HTID) hframe, addrType,
                       lpaddr) == xosdNone;
}                                       /* SYGetAddr() */

XOSD
SYGetMemInfo(
    LPMEMINFO lpmi
    )
{
    return OSDGetMemoryInformation(LppdCur->hpid, NULL, lpmi);
}

/**********************************************************************/

UINT
WINAPI
DHGetDebuggeeBytes(
    ADDR addr,
    UINT cb,
    void * lpb
    )
{
    int         terrno = errno;
    XOSD        xosd;
    DWORD       cbT;


    if ((LppdCur == NULL) || (LptdCur == NULL)) {    //Sanity check
        return 0;
    }

    if ( ADDR_IS_LI ( addr ) ) {
        OSDFixupAddr (LppdCur->hpid, LptdCur->htid, &addr );
    }
    if (OSDReadMemory(LppdCur->hpid, LptdCur->htid, &addr, lpb, cb, &cbT)
                                                                 != xosdNone) {
        cbT = 0;
    }

    errno = terrno;
    return cbT;
}                                       /* DHGetDebuggeeBytes() */

/***    DHSetDebuggeeBytes
 **
 **  Synopsis:
 **     uint = DHSetDebuggeeBytes(addr, cb, lpb)
 **
 **  Entry:
 **     addr    - address to write the bytes at
 **     cb      - count of bytes to be written
 **     lpb     - pointer to buffer of bytes to be written
 **
 **  Returns:
 **     count of bytes actually written if positive otherwise an
 **     XOSD error code.
 **
 **  Description:
 **     This function will write bytes into the debuggees memory space.
 **     To do this it uses OSDebug functions
 */


UINT
WINAPI
DHSetDebuggeeBytes(
    ADDR addr,
    UINT cb,
    void * lpb
    )
{
    int         terrno = errno;
    DWORD       cbT = 0;
    XOSD xosd;

    if ((LppdCur == NULL) || (LptdCur == NULL)) {    //Sanity check
        return cbT;
    }


    if ( ADDR_IS_LI ( addr ) ) {
        SYFixupAddr ( &addr );
    }

    xosd = OSDWriteMemory(LppdCur->hpid, LptdCur->htid, &addr, lpb, cb, &cbT);

    errno = terrno;
    if (xosd == xosdNone) {
        return cbT;
    } else {
        return 0;
    }

}                                       /* DHSetDebuggeeBytes() */

/***    DHGetReg
 **
 **  Synopsis:
 **     pshreg = DHGetReg(pshreg, pcxt, pframe)
 **
 **  Entry:
 **     pshreg  The register structure. The member hReg must contain
 **             the handle to the register to get.
 **     pCxt    The context packet to use.
 **
 **  Returns:
 **     pshreg if successful, NULL if the call could not be completed.
 **
 **  Description:
 **
 **     Currently the 8087 registers are not implemented. In the future
 **     only ST0 will be implemented.
 **
 */

PSHREG
DHGetReg(
    PSHREG pShreg,
    PCXT pCxt,
    HFRAME hFrame
    )
{
    HPID hpid;
    HTID htid;

    if (pShreg->hReg >= CV_REG_PSEUDO1 && pShreg->hReg <= CV_REG_PSEUDO9) {

        pShreg->Byte4 = ulPseudo[pShreg->hReg - CV_REG_PSEUDO1];

        return pShreg;

    } else if (LppdCur && LptdCur) {

        if (hFrame) {
            hpid = LppdCur->hpid;
            htid = (HTID) hFrame;
        } else {
            hpid = LppdCur->hpid;
            htid = LptdCur->htid;
        }

        if (OSDReadRegister( hpid,
                             htid,
                             pShreg->hReg,
                             &pShreg->Byte1 ) == xosdNone) {

            return ( pShreg );

        } else {
            return NULL;
        }
        
    } else {
        
        return NULL;
        
    }
}                                       /* DHGetReg() */

/***    DHSetReg
 **
 **  Synopsis:
 **     pshreg = DHSetReg( pReg, pCxt, pframe )
 **
 **  Entry:
 **     pReg    - Register description to be read
 **     pCxt    - context to use reading the register
 **     pframe
 **
 **  Returns:
 **     pointer to register description
 **
 **  Description:
 **
 */

PSHREG
DHSetReg (
    PSHREG pReg,
    PCXT pCxt,
    HFRAME hFrame
    )
{
    HPID hpid;
    HTID htid;

    Unreferenced( pCxt );

    if (pReg->hReg >= CV_REG_PSEUDO1 && pReg->hReg <= CV_REG_PSEUDO9) {

        ulPseudo[pReg->hReg - CV_REG_PSEUDO1] = pReg->Byte4;
        return ( pReg );

    }

    if ( LppdCur && LptdCur ) {

        if (hFrame) {
            hpid = LppdCur->hpid;
            htid = (HTID) hFrame;
        } else {
            hpid = LppdCur->hpid;
            htid = LptdCur->htid;
        }

        if (OSDWriteRegister(hpid, htid, pReg->hReg, (void *) &pReg->Byte1 )
                == xosdNone) {
            return ( pReg );
        } else {
            return NULL;
        }

    } else {
        return NULL;
    }
}                                       /* DHSetReg() */




BOOL
DHSetupExecute(
    LPHDEP lphdep
    )

/*++

Routine Description:

    This function is called from the expression evaluator to setup things
    up for a function evaluation call.  This request is passed on to the
    execution model.

Arguments:

    lphdep      - Supples a pointer to where a handle is returned

Return Value:

    TRUE if something fails

--*/

{
    if (LptdCur == NULL) {
        return xosdUnknown;
    }

    LptdCur->fInFuncEval = TRUE;
    LptdFuncEval = LptdCur;
    return OSDSetupExecute( LppdCur->hpid, LptdCur->htid, lphdep ) != xosdNone;
}                               /* DHSetupExecute() */



BOOL
DHStartExecute(
    HDEP     hdep,
    LPADDR   lpaddr,
    BOOL     fIgnoreEvents,
    SHCALL   shcall
    )

/*++

Routine Description:

    This function is called when the expression evaluator starts
    the debugging running a function evaluation.  It must be preceded
    by a call to DHSetupExecute.

Arguments:

    hdep        - Supplies the handle to the Execute Function object
    lpaddr      - Supplies the address to start execution at
    fIgnoreEvents - Supplies
    fFarRet     - Supplies TRUE if a return should be executed

Return Value:

    TRUE if something fails

--*/

{
    XOSD        xosd;
    MSG         msg;
    BOOL        fTmp;

    if (!LptdFuncEval) {
        return 1;
    }

    xosd = OSDStartExecute(LptdFuncEval->lppd->hpid, hdep, lpaddr,
                           fIgnoreEvents, shcall == SHFar);

    if (xosd != xosdNone) {
        return 1;
    }

    fTmp = SetAutoRunSuppress(TRUE);

    while (GetMessage(&msg, NULL, 0, 0)) {
        ProcessQCQPMessage(&msg);
        if (!LptdFuncEval) {
            xosd = 1;
            break;
        }
        if ((msg.message == DBG_REFRESH) &&
            (msg.wParam == dbcExecuteDone)) {
            xosd = (XOSD) msg.lParam;
            break;
        }
    }
    SetAutoRunSuppress(fTmp);

    return xosd != xosdNone;
}                               /* DHStartExecute() */



BOOL
DHCleanUpExecute(
    HDEP   hdep
    )

/*++

Routine Description:

    This function is used to clean up after doing a function evalution.

Arguments:

    hdep        - Supplies the handle to the function evaluation object

Return Value:

    TRUE if something fails

--*/

{
    if (!LptdFuncEval) {
        return TRUE;
    }
    LptdFuncEval->fInFuncEval = FALSE;
    return (OSDCleanUpExecute( LptdFuncEval->lppd->hpid, hdep ) != xosdNone);
}                               /* DHCleanUpExecute() */


/**********************************************************************/

// kcarlos
// BUGBUG
// dead code
/*
LSZ
FullPath(
    LSZ  lszBuf,
    LSZ  lszRel,
    UINT cbBuf
    )
{
    char    szBuf[500];
    char    szRel[500];
    char *  szRet;

    strcpy( szRel, lszRel );
    if ( szRet = _fullpath( (char *) szBuf, szRel, (size_t)cbBuf ) ) {
        strcpy( lszBuf, szBuf );
    }
    return (LSZ)szRet;
}
*/

// kcarlos
// BUGBUG
// dead code
/*
void
MakePath(
    LSZ  lszPath,
    LSZ  lszDrive,
    LSZ  lszDir,
    LSZ  lszFName,
    LSZ  lszExt
    )
{
    char    szPath[500];
    char    szDrive[256];
    char    szDir[256];
    char    szFName[256];
    char    szExt[256];

    strcpy( szDrive, lszDrive );
    strcpy( szDir, lszDir );
    strcpy( szFName, lszFName );
    strcpy( szExt, lszExt );

    _makepath( szPath, szDrive, szDir, szFName, szExt );
    strcpy( lszPath, szPath );
}
*/


// kcarlos
// BUGBUG
// dead code
/*
UINT
__cdecl
OurSprintf(
    LSZ  lszBuf,
    LSZ  lszFmt,
    ...
    )
{
    va_list val;
    WORD    wRet;
    char    szBuf[256];
    char    szFmt[256];

    strcpy( szFmt, lszFmt );

    va_start( val, lszFmt );
    wRet = (WORD)vsprintf( szBuf, szFmt, val );
    va_end( val );

    strcpy( lszBuf, szBuf );
    return wRet;
}
*/


// kcarlos
// BUGBUG
// dead code
/*
void
SearchEnv(
    LSZ  lszFile,
    LSZ  lszVar,
    LSZ  lszPath
    )
{
    char    szFile[ 256 ];
    char    szVar[ 256 ];
    char    szPath[ 256 ];

    strcpy( szFile, lszFile );
    strcpy( szVar, lszVar );
    _searchenv( szFile, szVar, szPath );
    strcpy( lszPath, szPath );
}
*/

// kcarlos
// BUGBUG
// dead code
/*
void
SplitPath(
    LSZ  lsz1,
    LSZ  lsz2,
    LSZ  lsz3,
    LSZ  lsz4,
    LSZ  lsz5
    )
{
#define _MAX_CVPATH     255
#define _MAX_CVDRIVE    3
#define _MAX_CVDIR      255
#define _MAX_CVFNAME    255
#define _MAX_CVEXT      255

    char    sz1[ _MAX_CVPATH ];
    char    sz2[ _MAX_CVDRIVE ];
    char    sz3[ _MAX_CVDIR ];
    char    sz4[ _MAX_CVFNAME];
    char    sz5[ _MAX_CVEXT];

    strcpy( sz1, lsz1 );
    _splitpath( sz1, sz2, sz3, sz4, sz5 );
    if (lsz2 != NULL) strcpy( lsz2, sz2 );
    if (lsz3 != NULL) strcpy( lsz3, sz3 );
    if (lsz4 != NULL) strcpy( lsz4, sz4 );
    if (lsz5 != NULL) strcpy( lsz5, sz5 );

    return;
}
*/


LPSTR
FormatSymbol(
    HSYM   hsym,
    PCXT   lpcxt
    )
{
    HDEP            hstr;
    char            szContext[512];
    char            szStr[512];
    char            szUndecStr[256];


    EEFormatCXTFromPCXT( lpcxt, &hstr, g_contWorkspace_WkSp.m_bShortContext );
    if (g_contWorkspace_WkSp.m_bShortContext) {
        strcpy( szContext, (LPSTR)MMLpvLockMb(hstr) );
        // We check this after the fact to avoid any side affects
        //  in calling MMLpvLockMb twice.
        Assert(strlen(szContext) < sizeof(szContext));
    } else {
        BPShortenContext( (LPSTR)MMLpvLockMb(hstr), szContext );
    }
    MMbUnlockMb(hstr);
    EEFreeStr(hstr);

    FormatHSym(hsym, lpcxt, szStr);
    if (*szStr == '?') {
        if (UnDecorateSymbolName( szStr,
                                  szUndecStr,
                                  sizeof(szUndecStr),
                                  UNDNAME_COMPLETE                |
                                  UNDNAME_NO_LEADING_UNDERSCORES  |
                                  UNDNAME_NO_MS_KEYWORDS          |
                                  UNDNAME_NO_FUNCTION_RETURNS     |
                                  UNDNAME_NO_ALLOCATION_MODEL     |
                                  UNDNAME_NO_ALLOCATION_LANGUAGE  |
                                  UNDNAME_NO_MS_THISTYPE          |
                                  UNDNAME_NO_CV_THISTYPE          |
                                  UNDNAME_NO_THISTYPE             |
                                  UNDNAME_NO_ACCESS_SPECIFIERS    |
                                  UNDNAME_NO_THROW_SIGNATURES     |
                                  UNDNAME_NO_MEMBER_TYPE          |
                                  UNDNAME_NO_RETURN_UDT_MODEL     |
                                  UNDNAME_NO_ARGUMENTS            |
                                  UNDNAME_NO_SPECIAL_SYMS         |
                                  UNDNAME_NAME_ONLY
                                )) {
            strcat(szContext,szUndecStr);
        }
    } else {
        strcat(szContext,szStr);
    }

    return _strdup(szContext);
}

BOOL
GetNearestSymbolInfo(
    LPADDR        addr,
    LPNEARESTSYM  lpnsym
    )
{
    ADDR            addr1 = {0};
    HDEP            hsyml;
    PHSL_HEAD       lphsymhead;
    PHSL_LIST       lphsyml;
    UINT            n;
    DWORDLONG       dwOff;
    DWORDLONG       dwOffP;
    DWORDLONG       dwOffN;
    DWORDLONG       dwAddrP;
    DWORDLONG       dwAddrN;
    EESTATUS        eest;
    HEXE            hexe;


    ZeroMemory(&lpnsym->cxt, sizeof(CXT));
    SYUnFixupAddr(addr);
    SHSetCxt(addr, &lpnsym->cxt);

    hexe = (HEXE)emiAddr(*addr);
    if ( hexe && (HPID)hexe != LppdCur->hpid ) {
        SHWantSymbols(hexe);
    }

    hsyml = 0;
    eest = EEGetHSYMList(&hsyml, &lpnsym->cxt, HSYMR_public, NULL, TRUE);
    if (eest != EENOERROR) {
        return FALSE;
    }

    lphsymhead = (PHSL_HEAD) MMLpvLockMb ( hsyml );
    lphsyml = (PHSL_LIST)(lphsymhead + 1);

    dwOffP = _UI64_MAX;
    dwOffN = _UI64_MAX;
    dwAddrP = _UI64_MAX;
    dwAddrN = _UI64_MAX;

    SYFixupAddr(addr);
    addr1 = *addr;
    for ( n = 0; n < (UINT)lphsyml->symbolcnt; n++ ) {

        if (SHAddrFromHsym(&addr1, lphsyml->hSym[n])) {
            SYFixupAddr(&addr1);
            if (GetAddrSeg(addr1) != GetAddrSeg(*addr)) {
                continue;
            }
            if (GetAddrOff(addr1) <= GetAddrOff(*addr)) {
                dwOff = GetAddrOff(*addr) - GetAddrOff(addr1);
                if (dwOff < dwOffP) {
                    dwOffP = dwOff;
                    dwAddrP = GetAddrOff(addr1);
                    lpnsym->hsymP = lphsyml->hSym[n];
                    lpnsym->addrP = addr1;
                }
            } else {
                dwOff = GetAddrOff(addr1) - GetAddrOff(*addr);
                if (dwOff < dwOffN) {
                    dwOffN = dwOff;
                    dwAddrN = GetAddrOff(addr1);
                    lpnsym->hsymN = lphsyml->hSym[n];
                    lpnsym->addrN = addr1;
                }
            }
        }

    }

    MMbUnlockMb(hsyml);
    MMFreeHmem(hsyml);

    return TRUE;
}

LPSTR
GetNearestSymbolFromAddr(
    LPADDR lpaddr,
    LPADDR lpAddrRet
    )
{
    NEARESTSYM      nsym;


    ZeroMemory( &nsym, sizeof(nsym) );

    if (!GetNearestSymbolInfo( lpaddr, &nsym )) {
        return NULL;
    }

    if (!nsym.hsymP) {
        return NULL;
    }

    *lpAddrRet = nsym.addrP;
    SYFixupAddr(lpAddrRet);

    return FormatSymbol( nsym.hsymP, &nsym.cxt );
}

BOOL
GetUnicodeStrings(
    VOID
    )
{
    return g_contWorkspace_WkSp.m_bUnicodeIsDefault;
}

/**********************************************************************/

/*
 **     Set up the structures needed for OSDebug
 **
 **     dbf     - This structure contains a set of callback routines
 **             to be used by the OSDebug modules to get certian
 **             services
 */

DBF     Dbf = {
    MHAlloc,                            /* MHAlloc                      */
    MHRealloc,                          /* MHRealloc                    */
    MHFree,                             /* MHFree                       */

//(HLLI (WINAPI *)( UINT, LLF, LPFNKILLNODE, LPFNFCMPNODE ))
    (LPLLINIT) LLPlliInit,                         /* LLInit                       */
//(HLLE (WINAPI *)( HLLI ))
    (LPLLCREATE) LLPlleCreate,                       /* LLCreate                     */
//(VOID (WINAPI *)( HLLI, HLLE ))
    (LPLLADD) LLAddPlleToLl,                      /* LLAdd                        */
//(VOID (WINAPI *)( HLLI, HLLE, DWORD ))
    (LPLLINSERT) LLInsertPlleInLl,                   /* LLInsert                     */
//(BOOL (WINAPI *)( HLLI, HLLE ))
    (LPLLDELETE) LLFDeletePlleFromLl,                /* LLDelete                     */
//(HLLE (WINAPI *)( HLLI, HLLE ))
    (LPLLNEXT) LLPlleFindNext,                     /* LLNext                       */
//(LONG (WINAPI *)( HLLI ))
    (LPLLDESTROY) LLChlleDestroyLl,                   /* LLDestroy                    */
//(HLLE (WINAPI *)( HLLI, HLLE, LPV, DWORD ))
    (LPLLFIND) LLPlleFindLpv,                      /* LLFind                       */
    (LPLLSIZE) LLChlleInLl,                        /* LLSize */
//(LPVOID (WINAPI *)( HLLE ))
    (LPLLLOCK) LLLpvFromPlle,                      /* LLLock                       */
//(VOID (WINAPI *)( HLLE ))
    (LPLLUNLOCK) LLUnlockPlle,                       /* LLUnlock                     */
//(HLLE (WINAPI *)( HLLI ))
    (LPLLLAST) LLPlleGetLast,                      /* LLLast                       */
//(VOID (WINAPI *)( HLLI, HLLE ))
    (LPLLADDHEAD) LLPlleAddToHeadOfLI,                /* LLAddHead                    */
//(BOOL (WINAPI *)( HLLI, HLLE ))
    (LPLLREMOVE) LLFRemovePlleFromLl,                /* LLRemove                     */


    AssertOut,                          /* LBAssert                     */
    LBQuit,                             /* LBQuit                       */
    NULL,                               /* SHGetSymbol               (1)*/
    NULL,                               /* SHGetPublicAddr           (1)*/
    NULL,                               /* SHAddrToPublicName           */
    NULL,                               /* SHGetDebugData               */
//  NULL,                               /* SHLocateSymbolFile           */
    NULL,                               /* SHLpGSNGetTable (see FLoadEmTl)*/
    NULL,                               /* SHWantSymbols                */

    DHGetNumber,                        /* DHGetNumber                  */

    NULL,                               /* GetTargetProcessor */
//    NULL,                               /* GetSet */
    /* End of structure         */
};

KNF     Knf = {
    sizeof(KNF),
    MHAlloc,                            /* MHAlloc                      */
    MHRealloc,                          /* MHRealloc                    */
    MHFree,                             /* MHFree                       */
    MHAllocHuge,                        /* MHAllocHuge                  */
    MHFree,                             /* MHFreeHuge                   */
    MMAllocHmem,                        /* MMAllocHmem                  */
    MMFreeHmem,                         /* MMFreeHmem                   */
    MMLpvLockMb,                        /* MMLock                       */
    MMbUnlockMb,                        /* MMUnlock                     */
//(HLLI (WINAPI *)( DWORD, LLF, LPFNKILLNODE, LPFNFCMPNODE ))
    (LPLLINIT) LLPlliInit,                         /* LLInit                       */
//(HLLE (WINAPI *)( HLLI ))
    (LPLLCREATE) LLPlleCreate,                       /* LLCreate                     */
//(VOID (WINAPI *)( HLLI, HLLE ))
    (LPLLADD) LLAddPlleToLl,                      /* LLAdd                        */
//(VOID (WINAPI *)( HLLI, HLLE ))
    (LPLLADDHEAD) LLPlleAddToHeadOfLI,                /* LLAddHead                    */
//(VOID (WINAPI *)( HLLI, HLLE, DWORD ))
    (LPLLINSERT) LLInsertPlleInLl,                   /* LLInsert                     */
//(BOOL (WINAPI *)( HLLI, HLLE ))
    (LPLLDELETE) LLFDeletePlleFromLl,                /* LLDelete                     */
//(BOOL (WINAPI *)( HLLI, HLLE ))
    (LPLLREMOVE) LLFRemovePlleFromLl,                /* LLRemove                     */
//(LONG (WINAPI *)( HLLI ))
    (LPLLDESTROY) LLChlleDestroyLl,                   /* LLDestroy                    */
//(HLLE (WINAPI *)( HLLI, HLLE ))
    (LPLLNEXT) LLPlleFindNext,                     /* LLNext                       */
//(HLLE (WINAPI *)( HLLI, HLLE, LPV, DWORD ))
    (LPLLFIND) LLPlleFindLpv,                      /* LLFind                       */
//(HLLE (WINAPI *)( HLLI ))
    (LPLLLAST) LLPlleGetLast,                      /* LLLast                       */
//(DWORD (WINAPI *)( HLLI ))
    (LPLLSIZE) LLChlleInLl,                        /* LLSize                       */
//(LPVOID (WINAPI *)( HLLE ))
    (LPLLLOCK) LLLpvFromPlle,                      /* LLLock                       */
//(VOID (WINAPI *)( HLLE ))
    (LPLLUNLOCK) LLUnlockPlle,                       /* LLUnlock                     */
//(BOOL (WINAPI *)(LPCH, LPCH, DWORD))
    AssertOut,                          /* LPPrintf                     */
//(BOOL (WINAPI *)(DWORD))
    LBQuit,                             /* LPQuit                       */
    SYOpen,                             /* SYOpen                       */
    SYClose,                            /* SYClose                      */
    SYRead,                             /* SYRead                   */
    SYLseek,                            /* SYSeek                       */
    SYFixupAddr,                        /* SYFixupAddr                  */
    SYUnFixupAddr,                      /* SYUnFixupAddr                */

    SYProcessor,                        /* SYProcessor                  */
    //NULL,

    // SYFIsOverlayLoaded,                 /* SYFIsOverlayLoaded           */
    // SearchEnv,                          /* searchenv                    */
    // OurSprintf,                         /* sprintf                      */
    // SplitPath,                          /* splitpath                    */
    // FullPath,                           /* fullpath                     */
    // MakePath,                           /* makepath                     */
    // OurStat,                            /* stat                         */
    // LBLog,                              /* LBLog                        */
    SYTell,                             /* SYTell                       */
    SYFindExeFile,                      /* SYFindExeFile                */
    GetSymbolFileFromServer,            /* GetSymbolFileFromServer      */              
	SYIgnoreAllSymbolErrors,            /* SYIgnoreAllSymbolErrors      */
    DLoadedSymbols,                     /* LoadedSymbols                */
    SYGetDefaultShe,                    /* SYGetDefaultShe              */
    NULL,                               // SYFindDebugInfoFile
    NULL,                               // GetRegistryRoot
    SYSetProfileString,                 // SetProfileString
    SYGetProfileString,                 // GetProfileString
};    /* End of structure             */

CRF Crf = {
    NULL,                               /* int                    */
    t_ultoa,                            /* ultoa                        */
    t_itoa,                             /* _itoa                         */
    NULL,                               /* ltoa                         */
    d_eprintf,                          /* eprintf                      */
    d_sprintf                           /* sprintf                      */
};                                      /* End of structure             */

CVF Cvf = {
    MHAlloc,                            /* MHlpvAlloc                   */
    MHFree,                             /* MHFreeLpv                    */
    NULL,                               /* SHGetNextExe              (1) */
    NULL,                               /* SHHEXEFromHMOD            (1)*/
    NULL,                               /* SHGetNextMod              (1)*/
    NULL,                               /* SHGetCXTFromHMOD          (1)*/
    NULL,                               /* SHGetCXTFromHEXE          (1)*/
    NULL,                               /* SHSetCXT                  (1)*/
    NULL,                               /* SHSetCXTMod               (1)*/
    NULL,                               /* SHFindNameInGlobal        (1)*/
    NULL,                               /* SHFindNameInContext       (1)*/
    NULL,                               /* SHGoToParent              (1)*/
    NULL,                               /* SHHSYMFromCXT                */
    NULL,                               /* SHNextHsym                (1)*/
    CLGetFuncCXF,                       /* SHGetFuncCXF                 */
    NULL,                               /* SHGetModName                 */
    NULL,                               /* SHGetExeName                 */
    NULL,                               /* SHGetModNameFromHexe         */
    NULL,                               /* SHGetSymFName                */
    NULL,                               /* SHGethExeFromName         (1)*/
    NULL,                               /* SHGethExeFromModuleName   (1)*/
    NULL,                               /* SHGetNearestHSYM          (1)*/
    NULL,                               /* SHIsInProlog              (1)*/
    NULL,                               /* SHIsADDRInCXT                */
    NULL,                               /* SHModelFromAddr           (1)*/
    NULL,                               /* SHFindSLink32                */

    NULL,                               /* SLLineFromAddr               */
    NULL,                               /* SLFLineToAddr             (1)*/
    NULL,                               /* SLNameFromHsf                */
    NULL,                               /* SLHmdsFromHsf                */
    NULL,                               /* SLHsfFromPcxt             (1)*/
    NULL,                               /* SLHsfFromFile             (1)*/

    NULL,                               /* PHGetNearestHSYM             */
    NULL,                               /* PHFindNameInPublics       (1)*/
    NULL,                               /* THGetTypeFromIndex        (1)*/
    NULL,                               /* THGetNextType                */
    MMAllocHmem,                        /* MHMemAllocate                */
    MMReallocHmem,                      /* MHMemReAlloc                 */
    MMFreeHmem,                         /* MHMemFree                    */
    MMLpvLockMb,                        /* MHMemLock                    */
    MMbUnlockMb,                        /* MHMemUnLock                  */
    NULL,                               /* MHIsMemLocked                */
    NULL,                               /* MHOmfLock                    */
    NULL,                               /* MHOmfUnlock                  */
    NULL,                               /* DHExecProc                   */
    DHGetDebuggeeBytes,                 /* DHGetDebuggeeBytes           */
    DHSetDebuggeeBytes,                 /* DHPutDebuggeeBytes           */
    DHGetReg,                           /* DHGetReg                     */
    DHSetReg,                           /* DHSetReg                     */
    NULL,                               /* DHSaveReg                    */
    NULL,                               /* DHRestoreReg                 */
    &in386mode,                         /* in386mode                    */
    &is_assign,                         /* is_assign                    */
//(int (OSDAPI *)( UINT ))
    LBQuit,                             /* quit                         */
    NULL,                               /* pArrayDefault ???            */
    NULL,                               /* pSHCompareRE                 */
    SYFixupAddr,                        /* SHFixupAddr                  */
    SYUnFixupAddr,                      /* pSHUnFixupAddr               */
    NULL,                               /* pCVfnCmp                     */
    NULL,                               /* pCVtdCmp                     */
    NULL,                               /* pCVcsCmp                     */
//(int (OSDAPI *)( LPSTR, LPSTR, UINT))
    AssertOut,                          /* assert                       */
    DHSetupExecute,                     /* DHSetupExecute               */
    DHCleanUpExecute,                   /* DHCleanUpExecute             */
    DHStartExecute,                     /* DHStartExecute               */
    NULL,                               /* SHFindNameInTypes            */
    NULL,                               /* SYProcessor                  */
    NULL,                               /* THAreTypesEqual              */
    NULL,                               /* GetTargetProcessor           */
    GetUnicodeStrings,                  /* GetUnicodeStrings            */
    SYGetAddr,                          /* pSYGetAddr                   */
    NULL,                               /* pSYSetAddr                   */
    SYGetMemInfo,                       /* pSYGetMemInfo                */
    NULL,                               /* SHWantSymbols                */

};                                      /* End of structure             */

LPSHF   Lpshf;

/***    CopyShToEe
 **
 **  Synopsis:
 **     void = CopyShToEe()
 **
 **  Entry:
 **     none
 **
 **  Returns:
 **     Nothing
 **
 **  Description:
 **     Copy function pointers from the Symbol handler API to the
 **     Expression evaluator API
 **
 */

void CopyShToEe()
{
    Cvf.pSHGetNextExe             = Lpshf->pSHGetNextExe;
    Cvf.pSHHexeFromHmod           = Lpshf->pSHHexeFromHmod;
    Cvf.pSHGetNextMod             = Lpshf->pSHGetNextMod;
    Cvf.pSHGetCxtFromHmod         = Lpshf->pSHGetCxtFromHmod;
    Cvf.pSHGetCxtFromHexe         = Lpshf->pSHGetCxtFromHexe;
    Cvf.pSHSetCxt                 = Lpshf->pSHSetCxt;
    Cvf.pSHFindNameInGlobal       = Lpshf->pSHFindNameInGlobal;
    Cvf.pSHFindNameInContext      = Lpshf->pSHFindNameInContext;
    Cvf.pSHGoToParent             = Lpshf->pSHGoToParent;
    Cvf.pSHNextHsym               = Lpshf->pSHNextHsym;
    Cvf.pSHGethExeFromName        = Lpshf->pSHGethExeFromName;
    Cvf.pSHGethExeFromModuleName  = Lpshf->pSHGethExeFromModuleName;
    Cvf.pSHGetNearestHsym         = Lpshf->pSHGetNearestHsym;
    Cvf.pSHIsInProlog             = Lpshf->pSHIsInProlog;
    Cvf.pSHModelFromAddr          = Lpshf->pSHModelFromAddr;
    Cvf.pSLFLineToAddr            = Lpshf->pSLFLineToAddr;
    Cvf.pSLHsfFromPcxt            = Lpshf->pSLHsfFromPcxt;
    Cvf.pSLHsfFromFile            = Lpshf->pSLHsfFromFile;
    Cvf.pPHFindNameInPublics      = Lpshf->pPHFindNameInPublics;
    Cvf.pTHGetTypeFromIndex       = Lpshf->pTHGetTypeFromIndex;
    Cvf.pSHCompareRE              = Lpshf->pSHCompareRE;
    Cvf.pSHGetExeName             = Lpshf->pSHGetExeName;
    Cvf.pSHGetModNameFromHexe     = Lpshf->pSHGetModNameFromHexe;
    Cvf.pSHGetSymFName            = Lpshf->pSHGetSymFName;
    Cvf.pSLNameFromHsf            = Lpshf->pSLNameFromHsf;
    Cvf.pSHWantSymbols            = Lpshf->pSHWantSymbols;
    Cvf.pTHAreTypesEqual          = Lpshf->pTHAreTypesEqual;
    Cvf.pSHFindNameInTypes        = Lpshf->pSHFindNameInTypes;
    Cvf.pSHSetCxtMod              = Lpshf->pSHSetCxtMod;

    Dbf.lpfnSHGetSymbol           = Lpshf->pSHGetSymbol;
    Dbf.lpfnSHGetPublicAddr       = Lpshf->pSHGetPublicAddr;
    Dbf.lpfnSHLpGSNGetTable       = Lpshf->pSHLpGSNGetTable;
    Dbf.lpfnSHGetDebugData        = Lpshf->pSHGetDebugData;
//  Dbf.lpfnSHFindSymbol          = Lpshf->pSHFindSymbol;
    Dbf.lpfnSHAddrToPublicName    = GetNearestSymbolFromAddr;
    Dbf.lpfnSHWantSymbols         = Lpshf->pSHWantSymbols;
//  Dbf.lpfnSHLocateSymbolFile    = Lpshf->pSHLocateSymbolFile;

    return;
}                                       /* CopyShToEe() */
