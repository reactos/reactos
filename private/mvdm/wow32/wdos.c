/* wdos.c - DOS realted functions for WOW
 *
 * Modification History
 *
 * Sudeepb 23-Aug-1991 Created
 */

#include "precomp.h"
#pragma hdrstop
#include "curdir.h"


MODNAME(wdos.c);

ULONG demClientErrorEx (HANDLE hFile, CHAR chDrive, BOOL bSetRegs);

extern DOSWOWDATA DosWowData;
extern PWORD16 pCurTDB, pCurDirOwner;

//
// This is our local array of current directory strings. A particular entry
// is only used if the directory becomes longer than the old MS-DOS limit
// of 67 characters.
//
#define MAX_DOS_DRIVES 26
LPSTR CurDirs[MAX_DOS_DRIVES] = {NULL};


VOID DosWowUpdateTDBDir(UCHAR Drive, LPSTR pszDir);
VOID DosWowUpdateCDSDir(UCHAR Drive, LPSTR pszDir);

#ifdef DEBUG
VOID __cdecl Dumpdir(LPSTR pszfn, LPSTR pszDir, ...);
#else
#define Dumpdir //
#endif

//
// modify this to change all the current directory spew's logging level
//
#define CURDIR_LOGLEVEL 4

/* First, a brief explanation on Windows and current directory
 *
 * 1. Windows keeps a single current directory (and drive off course) per
 *    application. All the "other" drives are shared between the apps and
 *    current dirs on them could change without further notice, such that
 *    if app "Abra" has c as it's current drive and "c:\foo" is it's current
 *    dir, and another app "Cadabra" has d as it's current drive and "d:\bar"
 *    as it's current dir, then this is what apps get in return to the respective
 *    system calls:
 *    App     Call                 Param      result
 *    Cadabra GetCurrentDirectory  c:         c:\foo
 *    Abra    SetCurrentDirectory  c:\foobar
 *    Cadabra GetCurrentDirectory  c:         c:\foobar
 *    Abra    SetCurrentDirectory  d:\test
 *    Abra    GetCurrentDirectory  d:         d:\test
 *    Cadabra GetCurrentDirectory  d:         d:\bar   <- d is it's current drive!
 *
 * 2. Windows is a "non-preemptive" multitasking OS. Remember that for later.
 *
 * 3. Tasks are id'd by their respective TDB's which have these interesting
 *    members (see tdb16.h for the complete list):
 *
 *    TDB_Drive
 *    TDB_LFNDirectory
 *
 *    when the high bit of the TDB_Drive is set (TDB_DIR_VALID) -- TDB_LFNDirectory
 *    is a valid current directory for the TDB_Drive (which is app's current
 *    drive). The drive itself (0-based drive number) is stored in
 *    TDB_Drive & ~TDB_DIR_VALID
 *
 * 4. Who touches TDB_Drive ?
 *    SaveState code -- which is called when the task is being switched away *from*
 *    it looks to see if info on current drive and directory in TDB is stale (via
 *    the TDB_DIR_VALID bit) and calls GetDefaultDrive and GetCurrentDirectory to
 *    make sure what's in TDB is valid
 *
 * 5. Task switching
 *    When task resumes it's running due to explanation above -- it has valid
 *    TDB_Drive in it. When the very first call to the relevant i21 is being
 *    made -- kernel looks at the owner of the current drive (kernel variable)
 *    and if some other task owns the current drive/directory -- it makes calls
 *    to wow to set current drive/dir from the TDB (which is sure valid at
 *    this point). Current Drive owner is set to the current task so that
 *    the next time around this call is not performed -- and since windows does
 *    not allow task preemptions -- any calls to set drive/directory are not
 *    reflected upon tdb up until the task switch time.
 *
 * 6. WOW considerations
 *    We in WOW have a great deal of hassle due to a number of APIs that are
 *    not called from i21 handler but rather deal with file i/o and other
 *    issues that depend upon Win32 current directory. Lo and behold we have
 *    an UpdateDosCurrentDirectory call that we make before and after the call
 *    to certain Win32 apis (which were found by trial and error)
 *    The logic inside is that we always try to keep as much sync as possible
 *    between TDB, CDS and Win32.
 *
 * 7. CurDirs
 *    CDS can only accomodate current dirs which are up to 64 chars in length
 *    hence there is an array of CurDirs which is filled on a per-need basis
 *    for those drives that have curdir lengths > 64+3 chars
 *
 * 8. Belief
 *    I profoundly believe that the above information is sufficient by large
 *    to successfully troubleshoot all the "current directory" issues that may
 *    arise :-)
 *
 * 9. Thanks
 *    Goes to Neil and Dave for all the insight and patience that made all these
 *    wonderful discoveries possible.
 *
 * -- VadimB, This day -- July, the 28th 1997
 */

/* GetCurrentDir - Updatess current dir in CDS structure
 *
 * Entry - pcds    = pointer to CDS
 *         chDrive = Physical Drive in question (0, 1 ...)
 *
 * Exit
 *      SUCCESS - returns TRUE
 *
 *      FAILURE - returns FALSE
 */
BOOL GetCurrentDir (PCDS pcds, UCHAR Drive)
{
    static CHAR  EnvVar[] = "=?:";
    DWORD EnvVarLen;
    BOOL bStatus = TRUE;
    UCHAR FixedCount;
    int i;
    PCDS pcdstemp;

    FixedCount = *(PUCHAR) DosWowData.lpCDSCount;
    //
    // from Macro.Asm in DOS:
    // ; Sudeepb 20-Dec-1991 ; Added for redirected drives
    // ; We always sync the redirected drives. Local drives are sync
    // ; as per the curdir_tosync flag and SCS_ToSync
    //

    if (*(PUCHAR)DosWowData.lpSCS_ToSync) {

#ifdef FE_SB
        if (GetSystemDefaultLangID() == MAKELANGID(LANG_JAPANESE,SUBLANG_DEFAULT)) {
            PCDS_JPN pcdstemp_jpn;

            pcdstemp_jpn = (PCDS_JPN) DosWowData.lpCDSFixedTable;
            for (i=0;i < (int)FixedCount; i++, pcdstemp_jpn++)
                pcdstemp_jpn->CurDirJPN_Flags |= CURDIR_TOSYNC;
        }
        else {
            pcdstemp = (PCDS) DosWowData.lpCDSFixedTable;
            for (i=0;i < (int)FixedCount; i++, pcdstemp++)
                pcdstemp->CurDir_Flags |= CURDIR_TOSYNC;
        }
#else
        pcdstemp = (PCDS) DosWowData.lpCDSFixedTable;
        for (i=0;i < (int)FixedCount; i++, pcdstemp++)
            pcdstemp->CurDir_Flags |= CURDIR_TOSYNC;
#endif

        // Mark tosync in network drive as well
        pcdstemp = (PCDS)DosWowData.lpCDSBuffer;
        pcdstemp->CurDir_Flags |= CURDIR_TOSYNC;

        *(PUCHAR)DosWowData.lpSCS_ToSync = 0;
    }

    // If CDS needs to be synched or if the requested drive is different
    // then the the drive being used by NetCDS go refresh the CDS.
    if ((pcds->CurDir_Flags & CURDIR_TOSYNC) ||
        ((Drive >= FixedCount) && (pcds->CurDir_Text[0] != (Drive + 'A') &&
                                   pcds->CurDir_Text[0] != (Drive + 'a')))) {
        // validate media
        EnvVar[1] = Drive + 'A';
        if((EnvVarLen = GetEnvironmentVariableOem (EnvVar, (LPSTR)pcds,
                                                MAXIMUM_VDM_CURRENT_DIR+3)) == 0){

        // if its not in env then and drive exist then we have'nt
        // yet touched it.

            pcds->CurDir_Text[0] = EnvVar[1];
            pcds->CurDir_Text[1] = ':';
            pcds->CurDir_Text[2] = '\\';
            pcds->CurDir_Text[3] = 0;
            SetEnvironmentVariableOem ((LPSTR)EnvVar,(LPSTR)pcds);
        }

        if (EnvVarLen > MAXIMUM_VDM_CURRENT_DIR+3) {
            //
            // The current directory on this drive is too long to fit in the
            // cds. That's ok for a win16 app in general, since it won't be
            // using the cds in this case. But just to be more robust, put
            // a valid directory in the cds instead of just truncating it on
            // the off chance that it gets used.
            //
            pcds->CurDir_Text[0] = EnvVar[1];
            pcds->CurDir_Text[1] = ':';
            pcds->CurDir_Text[2] = '\\';
            pcds->CurDir_Text[3] = 0;
        }

        pcds->CurDir_Flags &= 0xFFFF - CURDIR_TOSYNC;
        pcds->CurDir_End = 2;

    }

    if (!bStatus) {

        *(PUCHAR)DosWowData.lpDrvErr = ERROR_INVALID_DRIVE;
    }

    return (bStatus);

}

/* SetCurrentDir - Set the current directory
 *
 *
 * Entry - lpBuf   = pointer to string specifying new directory
 *         chDrive = Physical Drive in question (0, 1 ...)
 *
 * Exit
 *     SUCCESS returns TRUE
 *     FAILURE returns FALSE
 *
 */

BOOL SetCurrentDir (LPSTR lpBuf, UCHAR Drive)
{
    static CHAR EnvVar[] = "=?:";
    CHAR chDrive = Drive + 'A';
    BOOL bRet;

    // ok -- we are setting the current directory ONLY if the drive
    // is the current drive for the app

    if (*(PUCHAR)DosWowData.lpCurDrv == Drive) { // if on the current drive--go win32
       bRet = SetCurrentDirectoryOem(lpBuf);
    }
    else {  // verify it's a valid dir
       DWORD dwAttributes;

       dwAttributes = GetFileAttributesOem(lpBuf);
       bRet = (0xffffffff != dwAttributes) && (dwAttributes & FILE_ATTRIBUTE_DIRECTORY);
    }

    if (!bRet) {
       demClientErrorEx(INVALID_HANDLE_VALUE, chDrive, FALSE);
       return(FALSE);
    }

    EnvVar[1] = chDrive;
    bRet = SetEnvironmentVariableOem((LPSTR)EnvVar,lpBuf);

    return (bRet);
}


/* QueryCurrentDir - Verifies current dir provided in CDS structure
 *                      for $CURRENT_DIR
 *
 * First Validates Media, if invalid -> i24 error
 * Next  Validates Path, if invalid set path to root (not an error)
 *
 * Entry - Client (DS:SI) Buffer to CDS path to verify
 *     Client (AL)    Physical Drive in question (A=0, B=1, ...)
 *
 * Exit
 *     SUCCESS
 *       Client (CY) = 0
 *
 *         FAILURE
 *           Client (CY) = 1 , I24 drive invalid
 */
BOOL QueryCurrentDir (PCDS pcds, UCHAR Drive)
{
    DWORD dw;
    CHAR  chDrive;
    static CHAR  pPath[]="?:\\";
    static CHAR  EnvVar[] = "=?:";

    // validate media
    chDrive = Drive + 'A';
    pPath[0] = chDrive;
    dw = GetFileAttributesOem(pPath);
    if (dw == 0xFFFFFFFF || !(dw & FILE_ATTRIBUTE_DIRECTORY))
      {
        demClientErrorEx(INVALID_HANDLE_VALUE, chDrive, FALSE);
        return (FALSE);
        }

    // if invalid path, set path to the root
    // reset CDS, and win32 env for win32
    dw = GetFileAttributesOem(pcds->CurDir_Text);
    if (dw == 0xFFFFFFFF || !(dw & FILE_ATTRIBUTE_DIRECTORY))
      {
        strcpy(pcds->CurDir_Text, pPath);
        pcds->CurDir_End = 2;
        EnvVar[1] = chDrive;
        SetEnvironmentVariableOem(EnvVar,pPath);
        }

    return (TRUE);
}


/* strcpyCDS - copies CDS paths
 *
 *  This routine emulates how DOS was coping the directory path. It is
 *  unclear if it is still necessary to do it this way.
 *
 * Entry -
 *
 * Exit
 *     SUCCESS
 *
 *         FAILURE
 */
VOID strcpyCDS (PCDS source, LPSTR dest)
{
#ifdef FE_SB   // for DBCS Directory name by v-hidekk 1994.5.23
    unsigned char ch;
    unsigned char ch2;
#else // !FE_SB
    char ch;
#endif // !FE_SB
    int index;

    index = source->CurDir_End;

    if (source->CurDir_Text[index]=='\\')
        index++;
#ifdef FE_SB  //for DBCS Directory by v-hidekk 1994.5.23

// BUGBUG -- the code below is not equivalent to the code in Else clause
// we need to check for 0x05 character preceded by '\\' and replace it
// wth 0xE5

    while (ch = source->CurDir_Text[index]) {
        if (IsDBCSLeadByte(ch) ) {
            if( ch2 = source->CurDir_Text[index+1] ) {
                *dest++ = ch;
                *dest++ = ch2;
                index+=2;
            }
            else {
                index++;
            }
        }
        else {
            *dest++ = (UCHAR)toupper(ch);
            index++;
        }
    }


#else // !FE_SB

    while (ch = source->CurDir_Text[index]) {

        if ((ch == 0x05) && (source->CurDir_Text[index-1] == '\\')) {
            ch = (CHAR) 0xE5;
        }

        *dest++ = toupper(ch);
        index++;
    }
#endif // !FE_SB

    *dest = ch;                                 // trailing zero

}


/* GetCDSFromDrv - Updates current dir in CDS structure
 *
 * Entry - Drive    = Physical Drive in question (0, 1 ...)
 *
 * Exit
 *      SUCCESS - returns v86 pointer to CDS structure in DOS
 *
 *      FAILURE - returns 0
 */

PCDS GetCDSFromDrv (UCHAR Drive)
{
    PCDS  pCDS = NULL;
    static CHAR  pPath[]="?:\\";
    CHAR  chDrive;
     //
    // Is Drive valid?
    //

    if (Drive >= *(PUCHAR)DosWowData.lpCDSCount) {

        if (Drive <= 25) {

            chDrive = Drive + 'A';
            pPath[0] = chDrive;

            //
            // test to see if non-fixed/floppy drive exists
            //

            if ((*(PUCHAR)DosWowData.lpCurDrv == Drive) ||
                (GetDriveType(pPath) > 1)) {

                //
                // Network drive
                //

                pCDS = (PCDS) DosWowData.lpCDSBuffer;
            }

        }

    } else {

#if NEC_98
        //  This is updated a current dir in lpCDSBuffer or lpCDSFixedTable.
        //  Then, The drive is checked  the drive type and lpCDSCount(continuation drive).
        if (Drive <= 25)
        {
            chDrive = Drive + 'A';
            pPath[0] = chDrive;

            switch(GetDriveType(pPath))
            {
                case DRIVE_REMOTE:
                // NetWorkDrive
                    pCDS = (PCDS) DosWowData.lpCDSBuffer;
                    break;

                case DRIVE_REMOVABLE:
                case DRIVE_FIXED:
                case DRIVE_CDROM:
                case DRIVE_RAMDISK:
                    pCDS = (PCDS) DosWowData.lpCDSFixedTable;
                    pCDS = (PCDS)((ULONG)pCDS + (Drive*sizeof(CDS_JPN)));
                    break;
            }
        }
#else   // NEC_98
        chDrive = Drive + 'A';
        pPath[0] = chDrive;
        if ((Drive != 1) || (DRIVE_REMOVABLE == GetDriveType(pPath))) {

            //
            // Drive defined in fixed table
            //

            pCDS = (PCDS) DosWowData.lpCDSFixedTable;
#ifdef FE_SB
            if (GetSystemDefaultLangID() == MAKELANGID(LANG_JAPANESE,SUBLANG_DEFAULT)) {
                pCDS = (PCDS)((ULONG)pCDS + (Drive*sizeof(CDS_JPN)));
            }
            else
                pCDS = (PCDS)((ULONG)pCDS + (Drive*sizeof(CDS)));
#else
            pCDS = (PCDS)((ULONG)pCDS + (Drive*sizeof(CDS)));
#endif
        }
#endif  // NEC_98

    }

    return (pCDS);
}


/* DosWowSetDefaultDrive - Emulate DOS set default drive call
 *
 * Entry -
 *  BYTE  DriveNum;    = drive number to switch to
 *
 * Exit
 *       returns client AX
 *
 */

ULONG DosWowSetDefaultDrive(UCHAR Drive)
{
    PCDS pCDS;

    if (NULL != (pCDS = GetCDSFromDrv (Drive))) {

        if (GetCurrentDir (pCDS, Drive)) {

            if (*(PUCHAR)DosWowData.lpCurDrv != Drive) {

                // The upper bit in the TDB_Drive byte is used to indicate
                // that the current drive and directory information in the
                // TDB is stale. Turn it off here.

                // what is the curdir for this drive ?
                //

                CHAR  szPath[MAX_PATH] = "?:\\";
                PTDB  pTDB;

                if (*pCurTDB) {
                   pTDB = (PTDB)SEGPTR(*pCurTDB,0);
                   if (TDB_SIGNATURE == pTDB->TDB_sig) {
                      if ((pTDB->TDB_Drive & TDB_DIR_VALID) &&
                          (Drive == (pTDB->TDB_Drive & ~TDB_DIR_VALID))) {
                         // update cds with current stuff here

                         szPath[0] = 'A' + Drive;
                         strcpy(&szPath[2], pTDB->TDB_LFNDirectory);
                         // this call also updates the current dos drive
                         DosWowUpdateCDSDir(Drive, szPath);
                         Dumpdir("SetDefaultDrive(TDB->CDS): Drive %x", szPath, (UINT)Drive);
                         return(Drive);
                      }
                   }
                }

                szPath[0] = Drive + 'A';

                if ((Drive<MAX_DOS_DRIVES) && CurDirs[Drive]) {
                   strcpy(&szPath[3], CurDirs[Drive]);
                }
                else { // grab one from CDS
                   strcpyCDS(pCDS, &szPath[3]);
                }

                // update TDB to be in-sync with the cds

                Dumpdir("SetDefaultDrive(CDS->TDB): Drive %x", szPath, (UINT)Drive);
                *(PUCHAR)DosWowData.lpCurDrv = Drive;
                DosWowUpdateTDBDir(Drive, szPath);

            }

        }
    }

    return (*(PUCHAR)DosWowData.lpCurDrv);

}


#ifdef DEBUG

VOID __cdecl
Dumpdir(LPSTR pszfn, LPSTR pszDir, ...)
{

   PTDB pTDB;
   char szMod[9];
   char s[256];
   va_list va;

   if (NULL != WOW32_strchr(pszfn, '%')) {
      va_start(va, pszDir);
      wvsprintf(s, pszfn, va);
      va_end(va);
      pszfn = s;
   }

   LOGDEBUG(CURDIR_LOGLEVEL, ("%s: ", pszfn));

   if (*pCurTDB) {

      pTDB = (PTDB)SEGPTR(*pCurTDB,0);
      if (NULL != pTDB && TDB_SIGNATURE == pTDB->TDB_sig) {

         WOW32_strncpy(szMod, pTDB->TDB_ModName, 8);
         szMod[8] = '\0';
         LOGDEBUG(CURDIR_LOGLEVEL, ("CurTDB: %x (%s) ", (DWORD)*pCurTDB, szMod));
         LOGDEBUG(CURDIR_LOGLEVEL, ("Drv %x Dir %s\n", (DWORD)pTDB->TDB_Drive, pTDB->TDB_LFNDirectory));

      }
   }
   else {
      LOGDEBUG(CURDIR_LOGLEVEL, ("CurTDB: NULL\n"));
   }

   LOGDEBUG(CURDIR_LOGLEVEL, ("%s: ", pszfn));

   if (*pCurDirOwner) {

      pTDB = (PTDB)SEGPTR(*pCurDirOwner,0);
      if (NULL != pTDB && TDB_SIGNATURE == pTDB->TDB_sig) {

         WOW32_strncpy(szMod, pTDB->TDB_ModName, 8);
         szMod[8] = '\0';
         LOGDEBUG(CURDIR_LOGLEVEL, ("CurDirOwn: %x (%s) ", (DWORD)*pCurDirOwner, szMod));
         LOGDEBUG(CURDIR_LOGLEVEL, ("Drive %x Dir %s\n", (DWORD)pTDB->TDB_Drive, pTDB->TDB_LFNDirectory));

      }

   }
   else {
      LOGDEBUG(CURDIR_LOGLEVEL, ("CurDirOwn: NULL\n"));
   }

   if (NULL != pszDir) {
      LOGDEBUG(CURDIR_LOGLEVEL, ("%s: %s\n", pszfn, pszDir));
   }

}
#endif

// returns: current directory as done from the root

BOOL DosWowGetTDBDir(UCHAR Drive, LPSTR pCurrentDirectory)
{
   PTDB pTDB;

   if (*pCurTDB) {
      pTDB = (PTDB)SEGPTR(*pCurTDB,0);
      if (TDB_SIGNATURE == pTDB->TDB_sig &&
            (pTDB->TDB_Drive & TDB_DIR_VALID) &&
            ((pTDB->TDB_Drive & ~TDB_DIR_VALID) == Drive)) {
         strcpy(pCurrentDirectory, &pTDB->TDB_LFNDirectory[1]);
         // upper-case directory name
         WOW32_strupr(pCurrentDirectory);
         Dumpdir("DosWowGetTDBDir(CurTDB): Drive %x", pCurrentDirectory, (UINT)Drive);
         return(TRUE);
      }
   }
   return(FALSE);
}



/* DosWowGetCurrentDirectory - Emulate DOS Get current Directory call
 *
 *
 * Entry -
 *    Drive - Drive number for directory request
 *    pszDir- pointer to receive directory (MUST BE OF SIZE MAX_PATH)
 *
 * Exit
 *     SUCCESS
 *       0
 *
 *     FAILURE
 *       system status code
 *
 */
ULONG DosWowGetCurrentDirectory(UCHAR Drive, LPSTR pszDir)
{
    PCDS pCDS;
    DWORD dwRet = 0xFFFF000F;       // assume error

    //
    // Handle default drive value of 0
    //

    if (Drive == 0) {
       Drive = *(PUCHAR)DosWowData.lpCurDrv;
    } else {
       Drive--;
    }

    if (DosWowGetTDBDir(Drive, pszDir)) {
       return(0);
    }

    //
    // If the path has grown larger than the old MS-DOS path size, then
    // get the directory from our own private array.
    //
    if ((Drive<MAX_DOS_DRIVES) && CurDirs[Drive]) {
        strcpy(pszDir, CurDirs[Drive]);
        Dumpdir("GetCurrentDirectory(CurDirs): Drive %x", pszDir, (UINT)Drive);
        return 0;
    }

    if (NULL != (pCDS = GetCDSFromDrv (Drive))) {

        if (GetCurrentDir (pCDS, Drive)) {
            // for removable media we need to check that media is present.
            // fixed disks, network drives and CDROM drives are fixed drives in
            // DOS. sudeepb 30-Dec-1993
            if (!(pCDS->CurDir_Flags & CURDIR_NT_FIX)) {
                if(QueryCurrentDir (pCDS, Drive) == FALSE)
                    return (dwRet);         // fail
            }
            strcpyCDS(pCDS, pszDir);
            dwRet = 0;
        }
    }

    Dumpdir("GetCurrentDirectory: Drive %x", pszDir, (UINT)Drive);
    return (dwRet);

}

// updates current directory in CDS for the specified drive
//

VOID DosWowUpdateCDSDir(UCHAR Drive, LPSTR pszDir)
{
   PCDS pCDS;

   if (NULL != (pCDS = GetCDSFromDrv(Drive))) {
      // cds retrieved successfully

      // now for this drive -- validate

      if (strlen(pszDir) > MAXIMUM_VDM_CURRENT_DIR+3) {
         if ((!CurDirs[Drive]) &&
              (NULL == (CurDirs[Drive] = malloc_w(MAX_PATH)))) {
            return;
         }

         strcpy(CurDirs[Drive], &pszDir[3]);
         // put a valid directory in cds just for robustness' sake
         WOW32_strncpy(&pCDS->CurDir_Text[0], pszDir, 3);
         pCDS->CurDir_Text[3] = 0;
      } else {
         if (CurDirs[Drive]) {
            free_w(CurDirs[Drive]);
            CurDirs[Drive] = NULL;
         }
         strcpy(&pCDS->CurDir_Text[0], pszDir);
      }

      *(PUCHAR)DosWowData.lpCurDrv = Drive;

   }

}

// updates current task's tdb with the current drive and directory information
//
//

VOID DosWowUpdateTDBDir(UCHAR Drive, LPSTR pszDir)
{
   PTDB pTDB;

   if (*pCurTDB) {

      pTDB = (PTDB)SEGPTR(*pCurTDB,0);
      if (TDB_SIGNATURE == pTDB->TDB_sig) {

         // So TDB should be updated IF the current drive is
         // indeed the drive we're updating a directory for

         if (*(PUCHAR)DosWowData.lpCurDrv == Drive) { // or valid and it's current drive

            pTDB->TDB_Drive = Drive | TDB_DIR_VALID;
            strcpy(pTDB->TDB_LFNDirectory, pszDir+2);
            *pCurDirOwner = *pCurTDB;
         }

      }

   }
}


/* DosWowSetCurrentDirectory - Emulate DOS Set current Directory call
 *
 *
 * Entry -
 *    lpDosDirectory - pointer to new DOS directory
 *
 * Exit
 *     SUCCESS
 *       0
 *
 *     FAILURE
 *       system status code
 *
 */

extern NTSTATUS demSetCurrentDirectoryLCDS(UCHAR, LPSTR);

ULONG DosWowSetCurrentDirectory(LPSTR pszDir)
{
    PCDS pCDS;
    UCHAR Drive;
    LPTSTR pLast;
    PSTR lpDirName;
    UCHAR szPath[MAX_PATH];
    DWORD dwRet = 0xFFFF0003;       // assume error
    static CHAR  EnvVar[] = "=?:";
    BOOL  ItsANamedPipe = FALSE;

    if (':' == pszDir[1]) {
        Drive = toupper(pszDir[0]) - 'A';
    } else {
        if (IS_ASCII_PATH_SEPARATOR(pszDir[0]) &&
            IS_ASCII_PATH_SEPARATOR(pszDir[1])) {
            return dwRet;       // can't update dos curdir with UNC
        }
        Drive = *(PUCHAR)DosWowData.lpCurDrv;
    }


    if (NULL != (pCDS = GetCDSFromDrv (Drive))) {

        lpDirName = NormalizeDosPath(pszDir, Drive, &ItsANamedPipe);

        GetFullPathNameOem(lpDirName, MAX_PATH, szPath, &pLast);

        if (SetCurrentDir(szPath, Drive)) {

            //
            // If the directory is growing larger than the old MS-DOS max,
            // then remember the path in our own array. If it is shrinking,
            // then free up the string we allocated earlier.
            //
            if (strlen(szPath) > MAXIMUM_VDM_CURRENT_DIR+3) {
                if ((!CurDirs[Drive]) &&
                    (NULL == (CurDirs[Drive] = malloc_w(MAX_PATH)))) {
                    return dwRet;
                }
                strcpy(CurDirs[Drive], &szPath[3]);
                // put a valid directory in cds just for robustness' sake
                WOW32_strncpy(&pCDS->CurDir_Text[0], szPath, 3);
                pCDS->CurDir_Text[3] = 0;
            } else {
                if (CurDirs[Drive]) {
                    free_w(CurDirs[Drive]);
                    CurDirs[Drive] = NULL;
                }
                strcpy(&pCDS->CurDir_Text[0], szPath);
            }

            dwRet = 0;

            //
            // Update kernel16's "directory owner" with the current TDB.
            //
            Dumpdir("SetCurrentDirectory", szPath);
            DosWowUpdateTDBDir(Drive, szPath);

            // now update dem
            demSetCurrentDirectoryLCDS(Drive, szPath);

        }

    }

    return (dwRet);
}


//*****************************************************************************
// UpdateDosCurrentDirectory -
//
// Entry -
//    fDir - specifies which directory should be updated
//
// Exit -
//    TRUE if the update was successful, FALSE otherwise
//
// Notes:
//
// There are actually three different current directories:
// - The WIN32 current directory (this is really the one that counts)
// - The DOS current directory, kept on a per drive basis
// - The TASK current directory, kept in the TDB of a win16 task
//
// It is the responsibility of this routine to effectively copy the contents
// of one of these directories into another. From where to where is determined
// by the passed parameter, so it is the caller's responsibility to be sure
// what exactly needs to be sync'd up with what.
//
//*****************************************************************************

BOOL UpdateDosCurrentDirectory(UDCDFUNC fDir)
{
    LONG   lReturn = (LONG)FALSE;

    switch(fDir)  {

        case DIR_DOS_TO_NT: {

            UCHAR szPath[MAX_PATH] = "?:\\";
            PTDB pTDB;

            WOW32ASSERT(DosWowData.lpCurDrv != (ULONG) NULL);

            Dumpdir("UpdateDosCurrentDir DOS->NT", NULL);
            if ((*pCurTDB) && (*pCurDirOwner != *pCurTDB)) {

                pTDB = (PTDB)SEGPTR(*pCurTDB,0);

                if ((TDB_SIGNATURE == pTDB->TDB_sig) &&
                    (pTDB->TDB_Drive & TDB_DIR_VALID)) {

                    szPath[0] = 'A' + (pTDB->TDB_Drive & ~TDB_DIR_VALID);
                    strcpy(&szPath[2], pTDB->TDB_LFNDirectory);

                    LOGDEBUG(CURDIR_LOGLEVEL, ("UpdateDosCurrentDirectory: DOS->NT %s, case 1\n", szPath));
                    if (SetCurrentDirectoryOem(szPath)) {
                       // update cds and the current drive all at the same time
                       DosWowUpdateCDSDir((UCHAR)(pTDB->TDB_Drive & ~TDB_DIR_VALID), szPath);

                       // set the new curdir owner
                       *pCurDirOwner = *pCurTDB;
                    }
                    break;          // EXIT case
                }
            }


            szPath[0] = *(PUCHAR)DosWowData.lpCurDrv + 'A';

            if (CurDirs[*(PUCHAR)DosWowData.lpCurDrv]) {

                strcpy(&szPath[3], CurDirs[*(PUCHAR)DosWowData.lpCurDrv]);
                LOGDEBUG(CURDIR_LOGLEVEL, ("UpdateDosCurrentDirectory: DOS->NT %s, case 2\n", szPath));
                DosWowUpdateTDBDir(*(PUCHAR)DosWowData.lpCurDrv, szPath);

                SetCurrentDirectoryOem(CurDirs[*(PUCHAR)DosWowData.lpCurDrv]);
                lReturn = TRUE;
                break;
            }

            if (DosWowGetCurrentDirectory(0, &szPath[3])) {
                LOGDEBUG(LOG_ERROR, ("DowWowGetCurrentDirectory failed\n"));
            } else {

                // set the current directory owner so that when the
                // task switch occurs -- i21 handler knows to set
                // the current dir
                LOGDEBUG(CURDIR_LOGLEVEL, ("UpdateDosCurrentDirectory: DOS->NT %s, case 3\n", szPath));
                DosWowUpdateTDBDir(*(PUCHAR)DosWowData.lpCurDrv, szPath);

                SetCurrentDirectoryOem(szPath);
                lReturn = TRUE;
            }
            break;
        }

        case DIR_NT_TO_DOS: {

            UCHAR szPath[MAX_PATH];

            if (!GetCurrentDirectoryOem(MAX_PATH, szPath)) {

                LOGDEBUG(LOG_ERROR, ("DowWowSetCurrentDirectory failed\n"));

            } else {

                Dumpdir("UpdateDosCurrentDirectory NT->DOS", szPath);
                LOGDEBUG(LOG_WARNING, ("UpdateDosCurrentDirectory NT->DOS: %s\n", &szPath[0]));
                if (szPath[1] == ':') {
                    DosWowSetDefaultDrive((UCHAR) (toupper(szPath[0]) - 'A'));
                    DosWowSetCurrentDirectory(szPath);
                    lReturn = TRUE;
                }

            }
            break;
        }

    }
    return (BOOL)lReturn;
}

/***************************************************************************

 Stub entry points (called by KRNL386, 286 via thunks)

 ***************************************************************************/


/* WK32SetDefaultDrive - Emulate DOS set default drive call
 *
 * Entry -
 *  BYTE  DriveNum;    = drive number to switch to
 *
 * Exit
 *       returns client AX
 *
 */

ULONG FASTCALL WK32SetDefaultDrive(PVDMFRAME pFrame)
{
    PWOWSETDEFAULTDRIVE16   parg16;
    UCHAR Drive;

    GETARGPTR(pFrame, sizeof(WOWSETDEFAULTDRIVE16), parg16);

    Drive = (UCHAR) parg16->wDriveNum;

    FREEARGPTR(parg16);

    return (DosWowSetDefaultDrive (Drive));

}


/* WK32SetCurrentDirectory - Emulate DOS set current Directory call
 *
 * Entry -
 *    DWORD lpDosData    = pointer to DosWowData structure in DOS
 *    parg16->lpDosDirectory - pointer to real mode DOS pdb variable
 *    parg16->wNewDirectory  - 16-bit pmode selector for new Directory
 *
 * Exit
 *     SUCCESS
 *       0
 *
 *     FAILURE
 *       system status code
 *
 */
ULONG FASTCALL WK32SetCurrentDirectory(PVDMFRAME pFrame)
{

    PWOWSETCURRENTDIRECTORY16   parg16;
    LPSTR pszDir;
    ULONG dwRet;

    GETARGPTR(pFrame, sizeof(WOWSETCURRENTDIRECTORY16), parg16);
    GETVDMPTR(parg16->lpCurDir, 4, pszDir);
    FREEARGPTR(parg16);

    dwRet = DosWowSetCurrentDirectory (pszDir);

    FREEVDMPTR(pszDir);
    return(dwRet);

}


/* WK32GetCurrentDirectory - Emulate DOS Get current Directory call
 *
 *
 * Entry -
 *    DWORD lpDosData    = pointer to DosWowData structure in DOS
 *    parg16->lpCurDir  - pointer to buffer to receive directory
 *    parg16->wDriveNum - Drive number requested
 *                        Upper bit (0x80) is set if the caller wants long path
 *
 * Exit
 *     SUCCESS
 *       0
 *
 *     FAILURE
 *       DOS error code (000f)
 *
 */
ULONG FASTCALL WK32GetCurrentDirectory(PVDMFRAME pFrame)
{
    PWOWGETCURRENTDIRECTORY16   parg16;
    LPSTR pszDir;
    UCHAR Drive;
    ULONG dwRet;

    GETARGPTR(pFrame, sizeof(WOWGETCURRENTDIRECTORY16), parg16);
    GETVDMPTR(parg16->lpCurDir, 4, pszDir);
    Drive = (UCHAR) parg16->wDriveNum;
    FREEARGPTR(parg16);

    if (Drive<0x80) {
        UCHAR ChkDrive;

        //
        // Normal GetCurrentDirectory call.
        // If the path has grown larger than the old MS-DOS path size, then
        // return error, just like on win95.
        //

        if (Drive == 0) {
            ChkDrive = *(PUCHAR)DosWowData.lpCurDrv;
        } else {
            ChkDrive = Drive-1;
        }
        if ((Drive<MAX_DOS_DRIVES) && CurDirs[ChkDrive]) {
            return 0xFFFF000F;
        }

    } else {

        //
        // the caller wants the long path path
        //

        Drive &= 0x7f;
    }

    dwRet = DosWowGetCurrentDirectory (Drive, pszDir);

    FREEVDMPTR(pszDir);
    return(dwRet);

}

/* WK32GetCurrentDate - Emulate DOS Get current Date call
 *
 *
 * Entry -
 *
 * Exit
 *    return value is packed with date information
 *
 */
ULONG FASTCALL WK32GetCurrentDate(PVDMFRAME pFrame)
{
    SYSTEMTIME systemtime;

    UNREFERENCED_PARAMETER(pFrame);

    GetLocalTime(&systemtime);

    return ((DWORD) (systemtime.wYear  << 16 |
                     systemtime.wDay   << 8  |
                     systemtime.wMonth << 4  |
                     systemtime.wDayOfWeek
                     ));

}


#if 0
/* The following routine will probably never be used because we allow
   WOW apps to set a local time within the WOW. So we really want apps
   that read the time with int21 to go down to DOS where this local time
   is kept. But if we ever want to return the win32 time, then this
   routine will do it. */
/* WK32GetCurrentTime - Emulate DOS Get current Time call
 *
 *
 * Entry -
 *
 * Exit
 *    return value is packed with time information
 *
 */
ULONG FASTCALL WK32GetCurrentTime(PVDMFRAME pFrame)
{
    SYSTEMTIME systemtime;

    UNREFERENCED_PARAMETER(pFrame);

    GetLocalTime(&systemtime);

    return ((DWORD) (systemtime.wHour   << 24 |
                     systemtime.wMinute << 16 |
                     systemtime.wSecond << 8  |
                     systemtime.wMilliseconds/10
                     ));

}
#endif

/* WK32DeviceIOCTL - Emulate misc. DOS IOCTLs
 *
 * Entry -
 *  BYTE  DriveNum;    = drive number
 *
 * Exit
 *       returns client AX
 *
 */

ULONG FASTCALL WK32DeviceIOCTL(PVDMFRAME pFrame)
{
    PWOWDEVICEIOCTL16   parg16;
    UCHAR Drive;
    UCHAR Cmd;
    DWORD dwReturn = 0xFFFF0001;        // error invalid function
    UINT uiDriveStatus;
    static CHAR  pPath[]="?:\\";

    GETARGPTR(pFrame, sizeof(WOWDEVICEIOCTL16), parg16);

    Cmd = (UCHAR) parg16->wCmd;
    Drive = (UCHAR) parg16->wDriveNum;

    FREEARGPTR(parg16);

    if (Cmd != 8) {                     // Does Device Use Removeable Media
        return (dwReturn);
    }

    if (Drive == 0) {
        Drive = *(PUCHAR)DosWowData.lpCurDrv;
    } else {
        Drive--;
    }

    pPath[0] = Drive + 'A';
    uiDriveStatus = GetDriveType(pPath);

    if ((uiDriveStatus == 0) || (uiDriveStatus == 1)) {
        return (0xFFFF000F);            // error invalid drive
    }

    if (uiDriveStatus == DRIVE_REMOVABLE) {
        dwReturn = 0;
    } else {
        dwReturn = 1;
    }

    return (dwReturn);

}


BOOL DosWowDoDirectHDPopup(VOID)
{
   BOOL fNoPopupFlag;

   fNoPopupFlag = !!(CURRENTPTD()->dwWOWCompatFlagsEx & WOWCFEX_NODIRECTHDPOPUP);
   LOGDEBUG(0, ("direct hd access popup flag: %s\n", fNoPopupFlag ? "TRUE" : "FALSE"));
   return(!fNoPopupFlag);
}


