/*
 * caldos.c
 *
 *  Copyright (c) 1991,  Microsoft Corporation
 *
 *  DESCRIPTION
 *
 *              This file is/was ported from ..\..\progman\pmdos.c
 *              It contains two routines used by
 *
 *  MODIFICATION HISTORY
 *      Initial Version: x/x/90 Author Unknown, since he didn't feel
 *                       like commenting the code...
 *
 *      NT 32b Version:  1/9/91 Jeff Pack
 *                       Intitial port to begin.
 *
 *  WARNING:  since this is NOT for DOS, I'm making it soley 32bit aware.
 *            Following functions not ported
 *              IsRemovable() is in pmcomman.c (already ifdef'd in asm code)
 *              IsRemote()  is in pmcomman.c   (ditto!)
 *
 */

#include "cal.h"
#include <string.h>
#include <time.h>

/*** GetCurDrive --     get current drive number.
 *
 *
 *
 * INT GetCurDrive(VOID)
 *
 * ENTRY -      VOID
 *
 * EXIT  -      INT CurrentDrive - drive number of current drive (0=a, etc).
 *
 * SYNOPSIS - calls GetCurrentDirectory, must parse returned string
 *            for either drive letter, or UNC path.  If UNC I gotta
 *            somehow, covert UNC path to drive letter to drive number.
 * WARNINGS - not DBCS aware!
 * EFFECTS  -
 *
 */

INT GetCurDrive(VOID)
{
    DWORD   nBufferLength = 128;
    DWORD   dwReturnCode;
    LPTSTR   lpszLocalBuffer;
    INT     iDriveNumber;

    /* alloc local, non-moveable, zero filled buffer */
    lpszLocalBuffer = (LPTSTR) LocalAlloc(LMEM_ZEROINIT, nBufferLength);
    if(lpszLocalBuffer == NULL){
        OutputDebugString(TEXT("<GetCurDrive> LocalAlloc FAILed\n"));
    }

GetCurDrive1:
    dwReturnCode = GetCurrentDirectory(nBufferLength, lpszLocalBuffer);

    /*
     * Failed for reason other than bufferlength too small
     */
    if(dwReturnCode == 0){
        OutputDebugString(TEXT("<GetCurDrive>  GetCurrentDirectory() FAILed\n"));
    }

    /*
     * test for success, if dwReturnCode is > buffer, then need
     * increase buffer
     */
    if(dwReturnCode > nBufferLength){
        lpszLocalBuffer = (LPTSTR) LocalReAlloc(lpszLocalBuffer,
                                               nBufferLength + 128,
                                               LMEM_ZEROINIT | LMEM_MOVEABLE);
        if(lpszLocalBuffer == NULL){
            OutputDebugString(TEXT("<GetCurDrive> LocalAlloc FAILed\n"));
        }
        else{
            nBufferLength += 128;
        }
        goto GetCurDrive1;
    }

    /*
     * Finally lpszLocalBuffer has string containing current directory.
     * Now must parse string for ":" or "\\" for drive letter or UNC
     * If : then get drive letter, and convert to number a=0, b=1, etc.
     * If \\ then gotta enumerate net drives, to learn what drive letter
     * corresponds to that UNC path.
     */

    /* check for drive letter */
    if(lpszLocalBuffer[1] == TEXT(':')){

        /* is drive letter, proceed */
        iDriveNumber = lpszLocalBuffer[1] - TEXT('A'); /* convert letter > number */
    }
    else{
        /* must be UNC path */

        /* BUG BUG need write code to convert UNC path   */
        OutputDebugString(TEXT("<GetCurDrive> Got UNC path, didnt expect, and no code!\n"));
    }

    LocalFree(lpszLocalBuffer);
    return(iDriveNumber);
}

/*** FDosDelete -- Delete named file.
 *
 * INT FDosDelete(LPSTR lpszFileToDelete)
 *
 * ENTRY -      LPSTR lpszFileToDelete - filename to delete.
 *
 * EXIT  -      INT xxx - returns (0) if success
 *
 * SYNOPSIS - calls win32 DeleteFile.
 * WARNINGS -
 * EFFECTS  -
 *
 */

INT FDosDelete(LPTSTR lpszFileToDelete)
{
    BOOL    bReturnCode;

    bReturnCode = DeleteFile(lpszFileToDelete);

    if(bReturnCode){
        return(0);
    }
    else{
        return(1);
    }
}


/*** FDosRename -- Rename file.
 *
 * INT FDosRename(LPSTR lpszOrgFileName, LPSTR lpszNewFileName)
 *
 * ENTRY -      LPSTR lpszOrgFileName - origianl filename.
 *              LPSTR lpszNewFileName - New filename.
 *
 * EXIT  -      INT xxx - returns (0) if success
 *
 * SYNOPSIS - calls win32 MoveFile.
 * WARNINGS -
 * EFFECTS  -
 *
 */

INT FDosRename(LPTSTR lpszOrgFileName, LPTSTR lpszNewFileName)
{

    BOOL    bReturnCode;

    /* rename file */
    bReturnCode = MoveFile(lpszOrgFileName, lpszNewFileName);

    if(bReturnCode){
        return(0);  /* success */
    }
    else{
        return(1);
    }
}


VOID ReadClock(D3*pd3, TM*pmin)
{
    time_t t;
    struct tm *ptm;

    time(&t);
    ptm = localtime(&t);
    *pmin = ptm->tm_min + ptm->tm_hour * 60;
    pd3->wMonth = ptm->tm_mon;

    //- ReadClock: fixed to be day of month 0-30.
    pd3->wDay =  ptm->tm_mday - 1;

    //- ReadClock: fixed to be years since 1980 instead of since 0.
    pd3->wYear = ptm->tm_year - 80;
}
