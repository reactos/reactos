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


#ifndef ORGCODE
#include <io.h>
#include <string.h>
#include <ctype.h>
#endif
#include "windows.h"
#include <port1632.h>

BOOL PathType(LPSTR);
DWORD FileTime(HFILE);
DWORD GetDOSErrorCode(VOID);
int GetCurrentDrive(VOID);
int w_GetCurrentDirectory(int, LPSTR);
int w_SetCurrentDirectory(LPSTR);
int DosDelete(LPSTR);
LPSTR lmemmove(LPSTR, LPSTR, WORD);
BOOL  FAR PASCAL IsRemoteDrive(int);
BOOL  FAR PASCAL IsRemovableDrive(int);

#define LOCALBUFFERSIZE 128


/*** PathType --        Determines if string denotes a directory or not.
 *
 *
 *
 * BOOL PathType(LPSTR pszFileString)
 *
 * ENTRY -      LPSTR pszFileString     - pointer to string to use to determine if directory
 *                                                               or not.
 *                                                window, with focus.
 * EXIT  -      int iReturnValue        - 2 = is directory   1 = Is Not directory
 *
 * SYNOPSIS -  This function takes a pointer to a string, calls OS to determine
 *                              if string is, or is not a directory.
 * WARNINGS -  Cna't even see where this is called!
 * EFFECTS  -
 *
 */


BOOL PathType(LPSTR lpszFileString)
{

        LPSTR   lpszLocalBuffer;                                /*local buffer for AnsiToOem()*/
        DWORD   dwReturnedAttributes;
        DWORD   nBufferLength;

        nBufferLength = strlen(lpszFileString) + 1;
        /*alloc local, non-moveable, zero filled buffer*/
        lpszLocalBuffer = LocalAlloc(LMEM_ZEROINIT, nBufferLength);
        if(lpszLocalBuffer == NULL){
#if DBG
                OutputDebugString("<PathType> LocalAlloc FAILed\n");
#endif /* DBG */
                return 0;
        }

        AnsiToOem(lpszFileString, lpszLocalBuffer);

        /*get attributes of filestring*/
        dwReturnedAttributes = GetFileAttributes(lpszLocalBuffer);
        if(dwReturnedAttributes == -1){
#if DBG
                OutputDebugString("<PathType> - GetFileAttributes() FAILed!\n");
#endif /* DBG */
                LocalFree(lpszLocalBuffer);
                return(0);
        }
        else{
                /*and with directory attribute*/
                dwReturnedAttributes = dwReturnedAttributes & FILE_ATTRIBUTE_DIRECTORY;
                switch(dwReturnedAttributes){

                        case FILE_ATTRIBUTE_DIRECTORY:
                                LocalFree(lpszLocalBuffer);
                                return(2);
                                break;

                        default:
                                LocalFree(lpszLocalBuffer);
                                return(1);
                }

        }

}


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


BOOL IsReadOnly(LPSTR lpszFileString)
{

        DWORD   dwReturnedAttributes;
        LPSTR   lpszLocalBuffer;                                /*local buffer for AnsiToOem()*/
        DWORD   nBufferLength;

        nBufferLength = strlen(lpszFileString) + 1;
        /*alloc local, non-moveable, zero filled buffer*/
        lpszLocalBuffer = LocalAlloc(LMEM_ZEROINIT, nBufferLength);
        if(lpszLocalBuffer == NULL){
#if DBG
                OutputDebugString("<IsReadOnly> LocalAlloc FAILed\n");
#endif /* DBG */
                return 0;
        }

        AnsiToOem(lpszFileString, lpszLocalBuffer);

        /*get attributes of filestring*/
        dwReturnedAttributes = GetFileAttributes(lpszLocalBuffer);
        if(dwReturnedAttributes == -1){
#if DBG
            OutputDebugString("<IsReadOnly> - GetFileAttributes() FAILed!\n");
#endif /* DBG */
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

/*** GetCurrentDrive -- get current drive number.
 *
 *
 *
 * int GetCurrentDrive(VOID)
 *
 * ENTRY -      VOID
 *
 * EXIT  -      int CurrentDrive - drive number of current drive (0=a, etc).
 *
 * SYNOPSIS - calls GetCurrentDirectory, must parse returned string
 *                              for either drive letter, or UNC path.  If UNC I gotta
 *                              somehow, covert UNC path to drive letter to drive number.
 * WARNINGS -
 * EFFECTS  -
 *
 */

int GetCurrentDrive(VOID)
{


        /*BUG BUG, not DBCS aware!*/

        DWORD   nBufferLength = LOCALBUFFERSIZE;
        DWORD   dwReturnCode;
        LPSTR   lpszLocalBuffer;
        int             iDriveNumber;

        /*alloc local, non-moveable, zero filled buffer*/
        lpszLocalBuffer = LocalAlloc(LMEM_ZEROINIT, nBufferLength);
        if(lpszLocalBuffer == NULL){
#if DBG
                OutputDebugString("<GetCurrentDrive> LocalAlloc FAILed\n");
#endif /* DBG */
                return 0;
        }

GetCurDrive1:
        dwReturnCode = GetCurrentDirectory(nBufferLength, lpszLocalBuffer);

        /*failed for reason other than bufferlength too small*/
        if(dwReturnCode == 0){
#if DBG
                OutputDebugString("<GetCurrentDrive>  GetCurrentDirectory() FAILed\n");
#endif /* DBG */
                return 0;
        }
        /*test for success, if dwReturnCode is > buffer, then need increase buffer*/
        if(dwReturnCode > nBufferLength){
                lpszLocalBuffer = LocalReAlloc(lpszLocalBuffer, nBufferLength + LOCALBUFFERSIZE, LMEM_ZEROINIT | LMEM_MOVEABLE);
                if(lpszLocalBuffer == NULL){
#if DBG
                        OutputDebugString("<GetCurrentDrive> LocalAlloc FAILed\n");
#endif /* DBG */
                        return 0;
                }
                else{
                        nBufferLength += LOCALBUFFERSIZE;
                }
                goto GetCurDrive1;
        }

        /*finally lpszLocalBuffer has string containing current directory*/
        /* now must parse string for ":" or "\\" for drive letter or UNC*/
        /*if : then get drive letter, and convert to number a=0, b=1, etc.*/
        /*if \\ then gotta enumerate net drives, to learn what drive letter*/
        /*corresponds to that UNC path*/

        /*check for drive letter*/
        if(lpszLocalBuffer[1] == ':'){
                /*is drive letter, proceed*/
                if(isupper(lpszLocalBuffer[0])){
                        iDriveNumber = lpszLocalBuffer[0] - 'A';        /*convert letter > number*/
                }
                else{
                        iDriveNumber = lpszLocalBuffer[0] - 'a';        /*convert letter > number*/
                }
        }
        else{
                /*must be UNC path*/

                /*BUG BUG need write code to convert UNC path   */
#if DBG
                OutputDebugString("<GetCurrentDrive> Got UNC path, didnt expect, and no code!\n");
#endif /* DBG */
        }

        LocalFree(lpszLocalBuffer);
        return(iDriveNumber);
}

/*** SetCurrentDrive -- set current drive.
 *
 *
 *
 * int SetCurrentDrive(int iDrive)
 *
 * ENTRY -      int iDrive - drive number to set as current drive
 *
 * EXIT  -      int xxx - under DOS would have returned # of logical drives.
 *              I can do this, but it's not used, if fact, no error
 *              checks are done on this return value.
 *
 * SYNOPSIS - calls SetCurrentDirectory to set current drive.
 * WARNINGS -  ALWAYS sets to root directory, since can't get cur dir
 *                              on other than current working drive.
 * EFFECTS  -
 *
 */

int SetCurrentDrive(int iDrive)
{

        char    cLocalBuffer[LOCALBUFFERSIZE] = "C:\\";
        char    cDriveLetter;

        /*convert drive number (zero based) to letter*/
        cDriveLetter = (char) iDrive + (char)'A';
        cLocalBuffer[0] = cDriveLetter;         /*set new drive in string*/

        if(!SetCurrentDirectory(cLocalBuffer)){
                /*call failed*/
#if DBG
                OutputDebugString("<SetCurrentDrive> SetCurrentDirectory FAILed!\n");
#endif /* DBG */
                return 0;
        }
        return(0);
}

/*** w_GetCurrentDirectory -- GetCurrent Working Directory
 *
 *
 *
 * int w_GetCurrentDirectory(int iDrive, LPSTR lpszCurrentDirectory)
 *
 * ENTRY -      int iDrive - drive number to use as current drive.
 *                      LPSTR lpszCurrentDirectory - pointer to return data to.
 *
 * EXIT  -      int iReturnCode - returns (0) if success
 *                      LPSTR lpszCurrentDirectory - has curretn directory.
 *
 * SYNOPSIS - calls GetCurrentDirectory to get current directory.
 *                              the original asm code, checked idrive for zero, if so
 *                              then calls GetCurrentDrive.  Under win32, is not neccessary,
 *                              since GetCUrrentDirectory()     returns current drive.
 *                              Since it checks this, it means then that other than current
 *                              drive can be checked, yet win32 doesnt allow this, so I have to
 *                              code in a debug check, if iDrive != current drive.
 * WARNINGS -   win32 doesn't allow multiple cur dirs across drives.
 * EFFECTS  -
 *
 */

int w_GetCurrentDirectory(int iDrive, LPSTR lpszCurrentDirectory)
{


        /*first see if iDrive == 0, if so then only need call GetCurrentDirectory*/
        /*if non-zero, then could be current drive, OR another drive.*/
        /*THIS IS NOT ALLOWED!*/

        /*BUG BUG, not DBCS aware!*/

        DWORD   nBufferLength = LOCALBUFFERSIZE;
        DWORD   dwReturnCode;
        LPSTR   lpszLocalBuffer;
        int             iDriveNumber;


        /*alloc local, non-moveable, zero filled buffer*/
        lpszLocalBuffer = LocalAlloc(LMEM_ZEROINIT, nBufferLength);
        if(lpszLocalBuffer == NULL){
#if DBG
                OutputDebugString("<w_GetCurrentDirectory> LocalAlloc FAILed\n");
#endif /* DBG */
                return(1);
        }

GetCurDir1:
        dwReturnCode = GetCurrentDirectory(nBufferLength, lpszLocalBuffer);

        /*failed for reason other than bufferlength too small*/
        if(dwReturnCode == 0){
#if DBG
                OutputDebugString("<w_GetCurrentDirectory>  GetCurrentDirectory() FAILed\n");
#endif /* DBG */
                LocalFree(lpszLocalBuffer);
                return(1);
        }
        /*test for success, if dwReturnCode is > buffer, then need increase buffer*/
        if(dwReturnCode > nBufferLength){
                lpszLocalBuffer = LocalReAlloc(lpszLocalBuffer, nBufferLength + LOCALBUFFERSIZE, LMEM_ZEROINIT | LMEM_MOVEABLE);
                if(lpszLocalBuffer == NULL){
#if DBG
                        OutputDebugString("<w_GetCurrentDirectory> LocalAlloc FAILed\n");
#endif /* DBG */
                        LocalFree(lpszLocalBuffer);
                        return(1);
                }
                else{
                        nBufferLength += LOCALBUFFERSIZE;
                }
                goto GetCurDir1;
        }

        /*now I have string that contains EITHER current drive in a drive letter*/
        /*or current drive by a UNC name*/
        /*BUG BUG UNC name check uncoded, since I have to go from UNC name to drive letter*/

        /*debug code, to make sure iDrive == current drive*/
        /*see if drive letter based string*/
        if(lpszLocalBuffer[1] == ':'){
                /*is Drive letter based!*/
                /*never know case of returned string from kernel*/
                if(isupper(lpszLocalBuffer[0])){
                        iDriveNumber = lpszLocalBuffer[0] - 'A';
                }
                else{
                        iDriveNumber = lpszLocalBuffer[0] - 'a';
                }
                /*DEBUG make sure that we are indeed setting a new drive */
                /* remember that iDrive == 0 means use current drive!*/
                if(iDrive == iDriveNumber || iDrive == 0){
                        /*is current drive and drive letter based, set to after "x:\"*/
                        strcpy(lpszCurrentDirectory, lpszLocalBuffer);  /*copy directory to pointer*/
                }
                else{   /* is different drive, or not using current drive (== 0)*/
                        SetCurrentDrive(iDriveNumber);  /*set new drive "<iDrive>:\"   */
                        /*now that new drive/dir is set, return current dir*/
                        /* BUG BUG, because setting drive, overides cur dir, I return*/
                        /* "<newdrive>:\"     */
                        strcpy(lpszCurrentDirectory, "c:\\");
                        lpszCurrentDirectory[0]  = (char) (iDriveNumber + 'a'); /*set new drive*/
                }
        }
        else{
                /*is NOT drive letter based*/
                /* BUG BUG need write code to parse UNC, and return only the path*/

                /* BUG BUGalso need check to see if iDrive == UNC drive, so I gotta*/
                /* convert UNC path to drive, and compare*/

#if DBG
                OutputDebugString("<w_GetCurrentDirectory> Took path for UNC, and no code!\n");
#endif /* DBG */
                LocalFree(lpszLocalBuffer);
                return(1);
        }

        LocalFree(lpszLocalBuffer);
        return(0);                      /*success*/
}

/*** w_SetCurrentDirectory -- SetCurrent Working Directory and drive
 *
 * int w_SetCurrentDirectory(LPSTR lpszCurrentDirectory)
 *
 * ENTRY -      LPSTR lpszCurrentDirectory - string to set current drive/dir to
 *
 * EXIT  -      int iReturnCode - returns (0) if success
 *
 * SYNOPSIS - calls SetCurrentDirectory to set current directory and drive.
 * WARNINGS -
 * EFFECTS  -
 *
 */

int w_SetCurrentDirectory(LPSTR lpszCurrentDirectory)
{

        DWORD   dwReturnCode;

        dwReturnCode = SetCurrentDirectory(lpszCurrentDirectory);
        if(dwReturnCode == 0){
#if DBG
                OutputDebugString("<w_SetCurrentDirectory> SetCurrentDirectory FAILed!\n");
#endif /* DBG */
                return(1);
        }

        return(0);                      /*success*/
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

int DosDelete(LPSTR lpszFileToDelete)
{

        BOOL    bReturnCode;
        LPSTR   lpszLocalBuffer;                                /*local buffer for AnsiToOem()*/
        DWORD   nBufferLength;

        nBufferLength = strlen(lpszFileToDelete) + 1;
        /*alloc local, non-moveable, zero filled buffer*/
        lpszLocalBuffer = LocalAlloc(LMEM_ZEROINIT, nBufferLength);
        if(lpszLocalBuffer == NULL){
#if DBG
                OutputDebugString("<DosDelete> LocalAlloc FAILed\n");
#endif /* DBG */
                return 1;
        }


        AnsiToOem(lpszFileToDelete, lpszLocalBuffer);


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

int DosRename(LPSTR lpszOrgFileName, LPSTR lpszNewFileName)
{

        BOOL    bReturnCode;
        LPSTR   lpszLocalBuffer;                                /*local buffer for AnsiToOem()*/
        LPSTR   lpszLocalBuffer1;                               /*local buffer for AnsiToOem()*/
        DWORD   nBufferLength;
        DWORD   nBufferLength1;

        nBufferLength = strlen(lpszOrgFileName) + 1;
        nBufferLength1 = strlen(lpszNewFileName) + 1;
        /*alloc local, non-moveable, zero filled buffer*/
        lpszLocalBuffer = LocalAlloc(LMEM_ZEROINIT, nBufferLength);
        if(lpszLocalBuffer == NULL){
#if DBG
                OutputDebugString("<DosRename> LocalAlloc FAILed\n");
#endif /* DBG */
                return 1;
        }
        lpszLocalBuffer1 = LocalAlloc(LMEM_ZEROINIT, nBufferLength1);
        if(lpszLocalBuffer1 == NULL){
                OutputDebugString("<DosRename> LocalAlloc FAILed\n");
        }


        AnsiToOem(lpszOrgFileName, lpszLocalBuffer);
        AnsiToOem(lpszNewFileName, lpszLocalBuffer1);

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

/*** lmemmove -- move memory.
 *
 * LPSTR lmemmove(LPSTR lpszDst, LPSTR lpszSrc, WORD wCount)
 *
 * ENTRY -      LPSTR lpszDst - destination
 *                      LPSTR lpszSrc - source
 *                      WORD wCount     - number of chars to move.
 *
 * EXIT  -      LPSTR lpszDst - returns lpszDst.
 *
 * SYNOPSIS - calls c runtime.  Done cause they hacked lmemove to asm.
 * WARNINGS -
 * EFFECTS  -
 *
 */

LPSTR lmemmove(LPSTR lpszDst, LPSTR lpszSrc, WORD wCount)
{


        return(memmove(lpszDst, lpszSrc, wCount));

}

