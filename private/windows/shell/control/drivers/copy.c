/*
 *  copy.c - Copy routine for WinDosSetup
 *  Todd Laney
 *
 *  Modification History:
 *
 *  6/03/91 Vlads        Change copy process to incorporate new Install API
 *
 *  3/24/89  Toddla      Wrote it
 *
 *
 *  notes:
 *   we now use the LZCopy stuff for compression
 *   we now set the crit error handler ourselves so CHECKFLOPPY is
 *   NOT defined
 */

#include <windows.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <mmsystem.h>

#include "drivers.h"
#include "sulib.h"
//#include <ver.h>


#define MAX_COPY_ATTEMPTS  15

/*
 *  Maximum number of install disks we support
 */

#define MAX_DISKS 100

/*
 *  Flags for VerInstallFile
 */

#define FORCEABLE_FLAGS  (VIF_MISMATCH + VIF_SRCOLD + VIF_DIFFLANG + VIF_DIFFTYPE + VIF_DIFFCODEPG )

/**********************************************************************
 *
 * Local function prototypes.
 *
 **********************************************************************/

 // Retrieve disk path for logical disk

 BOOL GetDiskPath(LPSTR Disk, LPSTR szPath);

 // Convert VIF_... to ERROR... return codes

 UINT ConvertFlagToValue(DWORD dwFlags);

 // Do the work of trying to copy a file

 LONG TryCopy(LPSTR    szSrc,     // Full source file path
              LPSTR    szLogSrc,  // Logical source name
              LPSTR    szDestPath,// Destination path
              FPFNCOPY fpfnCopy); // Callback routine

 #ifdef CHECK_FLOPPY
 BOOL NEAR IsDiskInDrive(int iDisk);
 #endif

 // GLOBAL VARIABLES

 //  directory where windows will be setup to

 char szSetupPath[MAX_PATH];

 // directory where the root of the setup disks are!

 char szDiskPath[MAX_PATH];

 // Name of driver being copied (or oemsetup.inf)

 char szDrv[120];

/*
 *  global vars used by DosCopy
 */
 static LPSTR    lpBuf = NULL;   // copy buffer
 static int      iBuf = 0;       // usage count
 static UINT     nBufSize;
 BOOL     bRetry = FALSE;
 BOOL     bQueryExist;


 BOOL DefCopyCallback(int msg, DWORD n, LPSTR szFile)
 {
     return FC_IGNORE;
 }



/*  UINT FileCopy (szSource, szDir, fpfnCopy, UINT fCopy)
 *
 *  This function will copy a group of files to a single destination
 *
 *  ENTRY:
 *
 *  szSourc      : pointer to a SETUP.INF section
 *  szDir        : pointer to a string containing the target DIR
 *  fpfnCopy     : callback function used to notify called of copy status
 *  fCopy        : flags
 *
 *      FC_SECTION            - szSource is a section name
 *      FC_LIST               - szSource is a pointer to a char **foo;
 *      FC_LISTTYPE           - szSource is a pointer to a char *foo[];
 *      FC_FILE               - szSource is a file name.
 *      FC_QUALIFIED          - szSource is a fully qualified file name.
 *      FC_DEST_QUALIFIED     - szDir is fully qualified. Don't expand this.
 *      FC_CALLBACK_WITH_VER  - call back if file exists and report version information.
 *
 *  NOTES:
 *      if szSource points to a string of the form '#name' the section
 *      named by 'name' will be used as the source files
 *
 *      the first field of each line in the secion is used as the name of the
 *      source file.  A file name has the following form:
 *
 *          #:name
 *
 *          #       - Disk number containing file 1-9,A-Z
 *          name    - name of the file, may be a wild card expression
 *
 *  Format for copy status function
 *
 *  BOOL FAR PASCAL CopyStatus(int msg, int n, LPSTR szFile)
 *
 *      msg:
 *          COPY_ERROR          error occured while copying file(s)
 *                              n      is the DOS error number
 *                              szFile is the file that got the error
 *                              return: TRUE ok, FALSE abort copy
 *
 *          COPY_STATUS         Called each time a new file is copied
 *                              n      is the percent done
 *                              szFile is the file being copied
 *                              return: TRUE ok, FALSE abort copy
 *
 *          COPY_INSERTDISK     Please tell the user to insert a disk
 *                              n      is the disk needed ('1' - '9')
 *                              return: TRUE try again, FALSE abort copy
 *
 *          COPY_QUERYCOPY      Should this file be copied?
 *                              n      line index in SETUP.INF section (0 based)
 *                              szFile is the line from section
 *                              return: TRUE copy it, FALSE dont copy
 *
 *          COPY_START          Sent before any files are copied
 *
 *          COPY_END            Sent after all files have been copied
 *                              n   is dos error if copy failed
 *
 *          COPY_EXISTS         Sent if the FC_CALL_ON_EXIST bit was set
 *                              and the file exists at the destination
 *                              given for the filecopy.
 *
 *
 *  EXIT: returns TRUE if successful, FALSE if failure.
 *
 */

UINT FileCopy (LPSTR szSource, LPSTR szDir, FPFNCOPY fpfnCopy, UINT fCopy)
{
   int   err = ERROR_SUCCESS;     // Return code from this routine

   char  szPath[MAX_PATH];
   char  szLogSrc[MAX_PATH];
   char  szSrc[MAX_PATH];

   LPSTR pFileBegin;              // First file

   LPSTR * List;                  // Handle lists of files
   LPSTR * ListHead;

   int   nDisk;                   // The disk we're on

   int   cntFiles = 0;            // How many files we've got to do

   if (fpfnCopy == NULL) {
      fpfnCopy = DefCopyCallback;
   }

   if (!szSource || !*szSource || !szDir || !*szDir) {
      return ERROR_FILE_NOT_FOUND;
   }


  /*
   *  fix up the drive in the destination
   */

   if ( fCopy & FC_DEST_QUALIFIED ) {
      lstrcpy(szPath, szDir);
      fCopy &= ~FC_DEST_QUALIFIED;
   } else {
      ExpandFileName(szDir, szPath);
   }

   if (szSource[0] == '#' && fCopy == FC_FILE) {
       fCopy = FC_SECTION;
       ++szSource;
   }

   switch (fCopy) {
       case FC_SECTION:
       {
           szSource = infFindSection(NULL,szSource);

          /*
           * We are called even when the section doesn't exist
           */

           if (szSource == NULL) {
               return ERROR_SUCCESS;
           }

           fCopy = FC_LIST;
       }
       // fall through to FC_LIST

       case FC_LIST:
          pFileBegin = szSource;
          cntFiles = infLineCount(szSource);
          break;

       case FC_LISTTYPE:
          ListHead = List = (LPSTR far *)szSource;
          pFileBegin = *ListHead;
          while ( *List++ )           // Count files to be copied.
             ++cntFiles;
          break;

       case FC_FILE:
       case FC_QUALIFIED:
       default:
          pFileBegin = szSource;
          cntFiles = 1;
    }

  /*
   *  walk all files in the list and call TryCopy ....
   *
   *  NOTES:
   *      we must walk file list sorted by disk number.
   *      we should use the disk that is currently inserted.
   *      we should do a find first/find next on the files????
   *      we need to check for errors.
   *      we need to ask the user to insert disk in drive.
   *
   */

   (*fpfnCopy)(COPY_START,0,NULL);

  /*
   *  Go through all possible disks: 1 to 100 and A to Z (26)
   */

   for (nDisk = 1;
        err == ERROR_SUCCESS && (cntFiles > 0) &&
            (nDisk <= MAX_DISKS + 'Z' - 'A' + 1);
        nDisk++)
   {
      char Disk[10];              // Maximum string is "100:"
      LPSTR pFile;
      int FileNumber;             // Which file in the list we're on
                                  // (to pass to callback)

      pFile      = pFileBegin;    // Start at first file
      List       = ListHead;      // Handled chained lists
      FileNumber = 0;             // Informational for callback - gives
                                  // which file in list we're on
     /*
      *  Work out the string representing our disk letter
      */

      if (nDisk > MAX_DISKS) {
          Disk[0] = 'A' + nDisk - MAX_DISKS - 1;
          Disk[1] = '\0';
      } else {
          _itoa(nDisk, Disk, 10);
      }

      strcat(Disk, ":");

      for (;
           err == ERROR_SUCCESS && pFile;
           FileNumber++,
           pFile = fCopy == FC_LISTTYPE ? *(++List) :
                   fCopy == FC_LIST ? infNextLine(pFile) :
                   NULL)
      {
        /*
         *  We have to reset high bit of first byte because it could be set
         *  by translating service in OEM setup to show that file name was
         *  mapped
         */

         *pFile = toascii(*pFile);


        /*
         *  should we copy this file?
         *  copy the files in disk order.
         */

         if (_strnicmp(pFile, Disk, strlen(Disk)) == 0 || // File has disk
                                                         // number and we're
                                                         // on that disk
             RemoveDiskId(pFile) == pFile &&
                nDisk == 1 && *pFile ||                  // First disk and
                                                         // no disk number

             fCopy == FC_QUALIFIED) {                    // Fully qualified


            /*
             * done with a file. decrement count.
             */

             cntFiles--;

             lstrcpy(szDrv, RemoveDiskId(pFile));

             switch ((*fpfnCopy)(COPY_QUERYCOPY, FileNumber, pFile))
             {
                 case CopyCurrent:                // Skip

                         continue;

                 case CopyNeither:

                         err = ERROR_FILE_EXISTS; // File already exists

                 case CopyNew:
                         break;

                 default:
                         break;

             }

            /*
             *  Pick up bad return code from switch
             */

             if (err != ERROR_SUCCESS) {
                 break;
             }

            /*
             *  now we convert logical dest into a physical
             *    (unless FC_QUALIFIED)
             */

             infParseField(pFile, 1, szLogSrc);    // logical source

             if ( fCopy != FC_QUALIFIED ) {
                ExpandFileName(szLogSrc, szSrc); // full physical source
             } else {
                lstrcpy(szSrc,szLogSrc);
             }


            /*
             *  Attempt copy
             */

             err = TryCopy(szSrc,      // Qualified Source file
                           szLogSrc,   // Logical source file name (with disk #)
                           szPath,     // Path for directory to install in
                           fpfnCopy);  // Copy callback function

            /*
             *  If failed to find file try the windows directory
             */

             if (err != ERROR_SUCCESS) {
                 break;
             }

         } /* End if dor if DoCopy */
      }
   }

   (*fpfnCopy)(COPY_END,err,NULL);

   return err;
}

/**********************************************************************
 *
 *  TryCopy
 *
 *  Copy a single file from source to destination using the VerInstallFile
 *  API - interpreting the return code as :
 *
 *    ERROR_SUCCESS  - OK
 *    Other          - failure type
 *
 **********************************************************************/

LONG TryCopy(LPSTR    szSrc,      // Full expanded source file path
             LPSTR    szLogSrc,   // Logical source name
             LPSTR    szDestPath, // Destination path
             FPFNCOPY fpfnCopy)   // Callback routine

{
    DWORD wTmpLen;
    DWORD dwRetFlags;
    char  szTempFile[MAX_PATH];
    char  szErrFile[MAX_PATH];
    char  DriversPath[MAX_PATH];
    BOOL  bRetVal;               // Return code from callback
    LPSTR szFile;
    char  szSrcPath[MAX_PATH];
    int   iAttemptCount;
    WORD  wVerFlags;
    LONG  err;

   /*
    *  Fix up destination if file is a kernel driver
    */

    if (IsFileKernelDriver(szSrc)) {
        strcpy(DriversPath, szDestPath);
        strcat(DriversPath, "\\drivers");
        szDestPath = DriversPath;
    }

   /*
    *  Create file name from current string
    */

    szFile = FileName(szSrc);
    lstrcpy(szSrcPath, szSrc);
    StripPathName(szSrcPath);

    for(iAttemptCount = 0, wVerFlags = 0 ;
        iAttemptCount <= MAX_COPY_ATTEMPTS;
        iAttemptCount++) {

        HCURSOR  hcurPrev;             // Saved cursor state

        // Central operation - attempt to install file szFile in directory
        // pointed by szPath from directory pointed by szSrc
        // If operation will fail but with possibility to force install
        // in last parameter buffer we will have temporary file name ==>
        // therefore we can avoid excessive copying.
        // NOTE: now szFile consists of only file name and other buffers
        // only path names.

        wTmpLen = MAX_PATH;

        hcurPrev = SetCursor(LoadCursor(NULL,IDC_WAIT));
        dwRetFlags = VerInstallFile(wVerFlags,
                                    (LPSTR) szFile,
                                    (LPSTR) szFile,
                                    (LPSTR) szSrcPath,
                                    (LPSTR) szDestPath,
                                    (LPSTR) szDestPath,
                                    (LPSTR) szTempFile,
                                    (LPDWORD) &wTmpLen);
        SetCursor(hcurPrev);

       /*
        *  Operation failed if at least one bit of return flags is non-zero
        *  That is unusual but defined so in Version API.
        */

        if ( !dwRetFlags )
            return ERROR_SUCCESS;    // If no errors - goto next file


       /*
        *  If flag MISMATCH is set - install can be forced and we have
        *  temporary file in destination subdirectory
        */

        if ( dwRetFlags  &  VIF_MISMATCH ) {

            if ( dwRetFlags & VIF_SRCOLD ) {

              /*
               *  If we need not call back with question - automatically
               *  force install with same parameters.
               *  michaele, *only* if src file is *newer* than dst file
               */

               DeleteFile(szTempFile);

               return ERROR_SUCCESS;
            }

           /*
            *  If we need not call back with question - automatically
            *  force install with same parameters.
            */

            wVerFlags |= VIFF_FORCEINSTALL;
            iAttemptCount--;             // Make sure we get another go.
            continue;

        }   /* End if MISMATCH */

       /*
        *  If real error occured - call back with error file info
        *  In all dialogs we use our error codes - so I will convert
        *  flags returned from Ver API to ours.
        */

        err = ConvertFlagToValue(dwRetFlags);


       /*
        *  If source path or file is nor readable - try to change disk
        */

        if ( dwRetFlags & VIF_CANNOTREADSRC )
        {
          /*
           *  Now new path in szSrc so I deleted logic for creating it
           */

           if (RemoveDiskId(szLogSrc) == szLogSrc)

             /*
              *  if disk # not provided, default to 1
              */

              bRetVal = (*fpfnCopy)(COPY_INSERTDISK, (DWORD)"1", szSrcPath);
           else
              bRetVal = (*fpfnCopy)(COPY_INSERTDISK, (DWORD)szLogSrc, szSrcPath);


           switch (bRetVal)
              {
              case FC_RETRY:
                  continue;              // and try again...

              case FC_ABORT:
                  return ERROR_FILE_NOT_FOUND;

              case FC_IGNORE:
                  break;
              }
        }

        ExpandFileName(szLogSrc, szErrFile);

#if WINDOWSDIR

        if (!*bWindowsDir  &&
            err != FC_ERROR_LOADED_DRIVER &&
            err != ERROR_DISK_FULL)
        {
            GetWindowsDirectory(szPath, MAX_PATH);
            *bWindowsDir = TRUE;
            continue;
        }

#endif // WINDOWSDIR

        switch ((*fpfnCopy)(COPY_ERROR, err, szErrFile)) {

            case FC_IGNORE:
                return ERROR_SUCCESS;

            case FC_RETRY:
                break;

            case FC_ABORT:
                return ERROR_FILE_NOT_FOUND;
        }
    } // End of attempts

    return err;
}

/*  BOOL GetDiskPath(Disk, szPath)
 *
 *  This function will retrive the full path name for a logical disk
 *
 *  The code reads the [disks] section of SETUP.INF and looks for
 *  n = path where n is the disk char.  NOTE the disk '0' defaults to
 *  the root windows directory.
 *
 *  ENTRY:
 *
 *  cDisk        : what disk to find 0-9,A-Z
 *  szPath       : buffer to hold disk path
 *
 *  Returns :
 *     TRUE if a disk path was found
 *     FALSE if there was no disk specified (ie no ':'
 *
 */

BOOL GetDiskPath(LPSTR Disk, LPSTR szPath)
{
   char    ach[MAX_PATH];
   char    szBuf[MAX_PATH];
   int i;


  /*
   *  Check to see if there is actually a disk id.
   *  If not return FALSE
   */

   if (RemoveDiskId(Disk) == Disk) {
       return FALSE;
   }

  /*
   *  Create our copy of the disk id
   */

   for (i = 0; Disk[i] != ':'; i++) {
       ach[i] = Disk[i];
   }
   ach[i] = '\0';


  /*
   *  Zero disk letter means windows setup directory
   */

   if (_stricmp(ach, "0") == 0) {

      /*
       * return the windows setup directory
       */

       lstrcpy(szPath,szSetupPath);
       return TRUE;
   }

  /*
   *  now look in the [disks] section for a full path name
   *
   *  This is a pretty bogus concept and is not supported
   *  in win 32 style disks section [Source Media Descriptions]
   */

   if ( !infGetProfileString(NULL,DISK_SECT,ach,szPath) &&
        !infGetProfileString(NULL,OEMDISK_SECT,ach,szPath)) {

       lstrcpy(szPath, szDiskPath);
   } else {
       infParseField(szPath,1,szPath);

      /*
       *  is the path relative? is so prepend the szDiskPath
       */

       if (szPath[0] == '.' || szPath[0] == '\0') {
           lstrcpy(szBuf,szDiskPath);
           catpath(szBuf,szPath);
           lstrcpy(szPath,szBuf);
       }

   }

   return TRUE;
}


/*  BOOL FAR PASCAL ExpandFileName(LPSTR szFile, LPSTR szPath)
 *
 *  This function will retrive the full path name for a file
 *  it will expand, logical disk letters to pyshical ones
 *  will use current disk and directory if non specifed.
 *
 *  if the drive specifed is 0-9, it will expand the drive into a
 *  full pathname using GetDiskPath()
 *
 *  IE  0:system ==>  c:windows\system
 *      1:foo.txt     a:\foo.txt
 *
 *  ENTRY:
 *
 *  szFile       : File name to expand
 *  szPath       : buffer to hold full file name
 *
 */
BOOL ExpandFileName(LPSTR szFile, LPSTR szPath)
{
   char    szBuf[MAX_PATH*2];

   if (GetDiskPath(szFile, szBuf)) {
       lstrcpy(szPath,szBuf);
       if (szFile[2])
          catpath(szPath,szFile + 2);
   } else {
       lstrcpy(szPath,szFile);
   }
   return TRUE;
}




void catpath(LPSTR path, LPSTR sz)
{
   //
   // Remove any drive letters from the directory to append
   //
   sz = RemoveDiskId(sz);

   //
   // Remove any current directories ".\" from directory to append
   //
   while (sz[0] == '.' && SLASH(sz[1]))
      sz += 2;

   //
   // Dont append a NULL string or a single "."
   //
   if (*sz && ! (sz[0] == '.' && sz[1] == 0))
   {
      // Add a slash separator if necessary.
      if ((! SLASH(path[lstrlen(path) - 1])) &&    // slash at end of path
          ((path[lstrlen(path) - 1]) != ':') &&    // colon at end of path
          (! SLASH(sz[0])))                        // slash at beginning of file
         lstrcat(path, CHSEPSTR);

      lstrcat(path, sz);
   }
}

/*
 *  Return a pointer to the file name part of a string
 */

LPSTR FileName(LPSTR szPath)
{
   LPSTR   sz;

   for (sz=szPath; *sz; sz++)
      ;

   for (; sz>=szPath && !SLASH(*sz) && *sz!=':'; sz--)
      ;

   return ++sz;
}

/*
 *  Return the portion of a file name following the disk (ie anything
 *  before the colon).
 *  If there is no colon just return a pointer to the original string
 */

LPSTR RemoveDiskId(LPSTR szPath)
{
   LPSTR sz;

   for (sz = szPath; *sz; sz++) {
       if (*sz == ':') {
           return sz + 1;
       }
   }

   return szPath;
}

LPSTR StripPathName(LPSTR szPath)
{
    LPSTR   sz;

    sz = FileName(szPath);

    if (sz > szPath+1 && SLASH(sz[-1]) && sz[-2] != ':')
       sz--;

    *sz = 0;
    return szPath;
}

/*
 *  See if a file is a kernel driver.  Unfortunately the VersionInfo APIs
 *  don't seem coded up to take care of this at the moment so we just check
 *  to see if the file extension is ".SYS"
 */

 BOOL IsFileKernelDriver(LPSTR szPath)
 {
     char drive[MAX_PATH];
     char dir[MAX_PATH];
     char fname[MAX_PATH];
     char ext[MAX_PATH];

     _splitpath(szPath, drive, dir, fname, ext);
     return !_stricmp(ext, ".sys");
 }


/**************************************************************************
 *
 * This function converts returned flags from Ver API to the numerical
 * error codes used in SETUP.
 *
 ***************************************************************************/

UINT ConvertFlagToValue(DWORD dwFlags)
{
    if ( ! dwFlags  )
       return(NO_ERROR);
    if ( dwFlags & VIF_CANNOTREADSRC )
       return(ERROR_FILE_NOT_FOUND);
    if ( dwFlags & VIF_OUTOFMEMORY )
       return(ERROR_OUTOFMEMORY);
    if ( dwFlags & VIF_ACCESSVIOLATION )
       return(ERROR_ACCESS_DENIED);
    if ( dwFlags & VIF_SHARINGVIOLATION )
       return(ERROR_SHARING_VIOLATION);
    if ( dwFlags & VIF_FILEINUSE)
       return(FC_ERROR_LOADED_DRIVER);

    return(ERROR_CANNOT_COPY);    // General error
}



#ifdef CHECK_FLOPPY
/*--------------------------------------------------------------------------

  IsValidDiskette() -

--------------------------------------------------------------------------*/

#define CBSECTORSIZE   512
#define INT13_READ   2

BOOL IsValidDiskette(int iDrive)
{
   char       buf[CBSECTORSIZE];

   iDrive |= 0x0020;   // make lower case

   iDrive -= 'a';   // A = 0, B = 1, etc. for BIOS stuff

   return MyReadWriteSector(buf, INT13_READ, iDrive, 0, 0, 1);
}



/*  BOOL IsDiskInDrive(char cDisk)
 *
 *  Is the specifed disk in the drive
 *
 *  ENTRY:
 *
 *  cDisk        : what disk required to be in the drive (logical)
 *
 *  return TRUE if the specifed disk is in the drive
 *         FALSE if the wrong disk is in the drive or disk error
 *
 */
BOOL IsDiskInDrive(int iDisk)
{

   if ((iDisk  >= 'A' && iDisk <= 'Z') ||
      (iDisk  >= 'a' && iDisk <= 'z'))
      {
      if (DosRemoveable(iDisk))
         {
         if (!IsValidDiskette(iDisk))
            return FALSE;
         }
      return TRUE;
      }
   return TRUE;   // for non drive letters assume a path
                  // and thus always in.
}

#endif
