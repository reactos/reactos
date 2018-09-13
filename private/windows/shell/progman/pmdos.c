/*
 * pmdos.c
 *
 *  Copyright (c) 1991,  Microsoft Corporation
 *
 *  DESCRIPTION
 *
 *              This file is for support of program manager under NT Windows.
 *              This file is/was ported from pmdos.asm (program manager).
 *              It was in x86 asm code, and now is in ansi C.
 *              Some functions will be removed, due to they are only needed
 *              by DOS Windows.
 *
 *  MODIFICATION HISTORY
 *      Initial Version: x/x/90 Author Unknown, since he didn't feel
 *                                                              like commenting the code...
 *
 *      NT 32b Version:  1/9/91 Jeff Pack
 *                                                              Intitial port to begin.
 *
 *  WARNING:  since this is NOT for DOS, I'm making it soley 32bit aware.
 *                        Following functions not ported
 *                                      IsRemovable() is in pmcomman.c (already ifdef'd in asm code)
 *                                      IsRemote()  is in pmcomman.c   (ditto!)
 *
 */


#include <io.h>
#include <string.h>
#include <ctype.h>
#include <windows.h>
#include <port1632.h>

#if DBG

#define KdPrint(_x_) OutputDebugStringA _x_;

#else

#define KdPrint(_x_)

#endif

#define LOCALBUFFERSIZE 128


/*** FileTime --        Gets time of last modification.
 *
 *
 *
 * DWORD FileTime(HFILE hFile)
 *
 * ENTRY -      int hFile       - file handle to access
 *
 * EXIT  -      LPWORD   - which is gotten from lpTimeStamp = 0 (ERROR).
 *                                         or lpTimeStamp != 0 (value of timestamp)
 *
 * SYNOPSIS -  calls GetFileTime() to get timestamp. If error, then
 *                              lpTimeStamp = 0, else contains TimeStamp for file.
 * WARNINGS -
 * EFFECTS  -
 *
 */

DWORD FileTime(
    HFILE hFile)
{
    BOOL            bReturnCode;
    FILETIME        CreationTime;
    FILETIME        LastAccessTime;
    FILETIME        LastWriteTime;
    WORD            FatTime;
    WORD            FatDate;

    bReturnCode = GetFileTime((HANDLE)hFile, &CreationTime, &LastAccessTime,
        &LastWriteTime);

    /*
    * Test return code
    */
    if (bReturnCode == FALSE) {
            return 0;               /*set to zero, for error*/
    }

    /*
     * Now convert 64bit time to DOS 16bit time
     */
        FileTimeToDosDateTime( &LastWriteTime, &FatDate, &FatTime);
        return FatTime;
}


/*** IsReadOnly --      determines if file is readonly or not.
 *
 *
 *
 * BOOL IsReadOnly(LPSTR lpszFileString)
 *
 * ENTRY -      LPSTR lpszFileString    - file name to use
 *
 * EXIT  -      BOOL xxx - returns (0) = not readonly  (1) = read only
 *                                         or lpTimeStamp != 0 (value of timestamp)
 *
 * SYNOPSIS -  calls GetAttributes, then tests if file is read only.
 * WARNINGS -
 * EFFECTS  -
 *
 */


BOOL IsReadOnly(LPTSTR lpszFileString)
{

        DWORD   dwReturnedAttributes;
        LPTSTR   lpszLocalBuffer;                                /*local buffer for AnsiToOem()*/
        DWORD   nBufferLength;

        nBufferLength = lstrlen(lpszFileString) + 1;
        /*alloc local, non-moveable, zero filled buffer*/
        lpszLocalBuffer = (LPTSTR)LocalAlloc(0, sizeof(TCHAR)*nBufferLength);
        if(lpszLocalBuffer == NULL){
                KdPrint(("<IsReadOnly> LocalAlloc FAILed\n"));
        }

        lstrcpy(lpszLocalBuffer,lpszFileString);

        /*get attributes of filestring*/
        dwReturnedAttributes = GetFileAttributes(lpszLocalBuffer);
        if(dwReturnedAttributes == -1){
            KdPrint(("<IsReadOnly> - GetFileAttributes() FAILed!\n"));
            LocalFree(lpszLocalBuffer);
            return FALSE;
        } else {
                /*AND with read_only attribute*/
                dwReturnedAttributes = dwReturnedAttributes & FILE_ATTRIBUTE_READONLY;
                switch(dwReturnedAttributes){

                        case FILE_ATTRIBUTE_READONLY:
                                LocalFree(lpszLocalBuffer);
                                return TRUE;
                                break;

                        default:
                                LocalFree(lpszLocalBuffer);
                                return FALSE;
                }

        }

}

/*** GetDOSErrorCode -- returns extended error code
 *
 *
 *
 * DWORD GetDOSErrorCode(VOID)
 *
 * ENTRY -      VOID
 *
 * EXIT  -      DWORD - returned extended code.
 *
 * SYNOPSIS - calls GetLastError() to get error code from OS
 * WARNINGS -
 * EFFECTS  -
 *
 */

DWORD GetDOSErrorCode(VOID)
{

        return( (int) GetLastError());

        /*BUG BUG, pmgseg.c uses this from _lcreat() to determine if returned
                5 (access denied) or 13 (invalid_data).  So this need be tested
                to see if win32 returns these.*/

}

/*** DosDelete -- Delete named file.
 *
 * int DosDelete(LPSTR lpszFileToDelete)
 *
 * ENTRY -      LPSTR lpszFileToDelete - filename to delete.
 *
 * EXIT  -      int xxx - returns (0) if success
 *
 * SYNOPSIS - calls win32 DeleteFile.
 * WARNINGS -
 * EFFECTS  -
 *
 */

int DosDelete(LPTSTR lpszFileToDelete)
{

        BOOL    bReturnCode;
        LPTSTR   lpszLocalBuffer;                                /*local buffer for AnsiToOem()*/
        DWORD   nBufferLength;

        nBufferLength = lstrlen(lpszFileToDelete) + 1;
        /*alloc local, non-moveable, zero filled buffer*/
        lpszLocalBuffer = (LPTSTR)LocalAlloc(0, sizeof(TCHAR)*nBufferLength);
        if(lpszLocalBuffer == NULL){
                KdPrint(("<DosDelete> LocalAlloc FAILed\n"));
        }


        lstrcpy(lpszLocalBuffer,lpszFileToDelete);


        bReturnCode = DeleteFile(lpszLocalBuffer);
        LocalFree(lpszLocalBuffer);
        if(bReturnCode){
                return(0);
        }
        else{
                return(1);
        }
}

/*** DosRename -- Rename file.
 *
 * int DosRename(LPSTR lpszOrgFileName, LPSTR lpszNewFileName)
 *
 * ENTRY -      LPSTR lpszOrgFileName - origianl filename.
 *                      LPSTR lpszNewFileName - New filename.
 *
 * EXIT  -      int xxx - returns (0) if success
 *
 * SYNOPSIS - calls win32 MoveFile.
 * WARNINGS -
 * EFFECTS  -
 *
 */

int DosRename(LPTSTR lpszOrgFileName, LPTSTR lpszNewFileName)
{

        BOOL    bReturnCode;
        LPTSTR   lpszLocalBuffer;                                /*local buffer for AnsiToOem()*/
        LPTSTR   lpszLocalBuffer1;                               /*local buffer for AnsiToOem()*/
        DWORD   nBufferLength;
        DWORD   nBufferLength1;

        nBufferLength = lstrlen(lpszOrgFileName) + 1;
        nBufferLength1 = lstrlen(lpszNewFileName) + 1;
        /*alloc local, non-moveable, zero filled buffer*/
        lpszLocalBuffer = (LPTSTR)LocalAlloc(0, sizeof(TCHAR)*nBufferLength);
        if(lpszLocalBuffer == NULL){
                KdPrint(("<DosRename> LocalAlloc FAILed\n"));
        }
        lpszLocalBuffer1 = (LPTSTR)LocalAlloc(0, sizeof(TCHAR)*nBufferLength1);
        if(lpszLocalBuffer1 == NULL){
                KdPrint(("<DosRename> LocalAlloc FAILed\n"));
        }


        lstrcpy(lpszLocalBuffer,lpszOrgFileName);
        lstrcpy(lpszLocalBuffer1,lpszNewFileName);

        /*rename file*/
        bReturnCode = MoveFile(lpszLocalBuffer, lpszLocalBuffer1);

        LocalFree(lpszLocalBuffer);
        LocalFree(lpszLocalBuffer1);

        if(bReturnCode){
                return(0);
        }
        else{
                return(1);
        }
}

#if 0
#ifdef i386

//
// Returns true if the application exe type is a DOS binary.
//

BOOL IsDOSApplication(LPTSTR lpPath)
{
    DWORD dwBinaryType;
    BOOL bRet;

    bRet = GetBinaryType(lpPath, &dwBinaryType);
    if (bRet) {
        if (dwBinaryType != SCS_DOS_BINARY) {
            bRet = FALSE;
        }
    }
    return(bRet);
}

//
// this routine translates path characters into _ characters because
// the NT registry apis do not allow the creation of keys with
// names that contain path characters.  it allocates a buffer that
// must be freed.
//

LPTSTR TranslateConsoleTitle(LPTSTR ConsoleTitle)
{
    int ConsoleTitleLength,i;
    LPTSTR TranslatedConsoleTitle,Tmp;

    ConsoleTitleLength = lstrlen(ConsoleTitle) + 1;
    Tmp = TranslatedConsoleTitle = (LPTSTR)LocalAlloc(LMEM_FIXED,ConsoleTitleLength * sizeof(TCHAR));
    if (TranslatedConsoleTitle == NULL) {
        return NULL;
    }
    for (i=0;i<ConsoleTitleLength;i++) {
        if (*ConsoleTitle == TEXT('\\')) {
            *TranslatedConsoleTitle++ = (TCHAR)TEXT('_');
            ConsoleTitle++;
        } else {
            *TranslatedConsoleTitle++ = *ConsoleTitle++;
        }
    }
    return Tmp;
}

// DOS apps are no longer set to fullscreen by default in progman
//  5-3-93 johannec (bug 8343)

#define CONSOLE_REGISTRY_STRING TEXT("Console")
#define CONSOLE_REGISTRY_FULLSCR TEXT("FullScreen")


BOOL SetDOSApplicationToFullScreen(LPTSTR lpTitle)
{
    HKEY hkeyConsole,hkeyTitle;
    LPTSTR lpTranslatedTitle;
    DWORD Error;
    DWORD dwFullScreen = 1;

    Error = RegOpenKeyEx(HKEY_CURRENT_USER,
                         CONSOLE_REGISTRY_STRING,
                         0,
                         KEY_READ | KEY_WRITE,
                         &hkeyConsole);

    if (Error) {
        return FALSE;
    }

    lpTranslatedTitle = TranslateConsoleTitle(lpTitle);
    if (lpTranslatedTitle == NULL) {
        return FALSE;
    }

    Error = RegCreateKey(hkeyConsole,
                           lpTranslatedTitle,
                           &hkeyTitle);
    LocalFree(lpTranslatedTitle);
    if (Error) {
        RegCloseKey(hkeyConsole);
        return FALSE;
    }
    Error = RegSetValueEx(hkeyTitle,
                        CONSOLE_REGISTRY_FULLSCR,
                        0,
                        REG_DWORD,
                        (LPBYTE)&dwFullScreen,
                        sizeof(dwFullScreen));
    RegCloseKey(hkeyTitle);
    RegCloseKey(hkeyConsole);

    return(!Error);
}
#endif

#endif
