/*
** main.c - Main module for DOS command-line LZA file compression / expansion
**          programs.
**
** Author: DavidDi
**
** This module is compiled twice - once for COMPRESS (COMPRESS defined) and
** once for EXPAND (COMPRESS not defined).
*/


// Headers
///////////

#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include "..\..\libs\common.h"
#include "..\..\libs\buffers.h"
#include "..\..\libs\header.h"

#include "args.h"
#include "main.h"
#include "messages.h"

//
// diamond routines
//
#include "mydiam.h"

// Globals
///////////

CHAR ARG_PTR *pszInFileName,     // input file name
             *pszOutFileName,    // output file name
             *pszTargetName;     // target path name

TCHAR   ErrorMsg[2048];

BOOL    bContainsMultipleFiles;  // Is source file a multi-file CAB?
INT     nLocalFiles, nTotalFiles = 0;  // number of files listed/expanded


// Module Variables
////////////////////

#ifndef COMPRESS
static BOOL bCopyingFile;        // Is current file being copied or expanded?
#endif


// Local Prototypes
////////////////////

static VOID DisplayErrorMessage(INT fError);
static VOID MakeDestFileName(CHAR ARG_PTR *argv[], CHAR ARG_PTR *pszDest);
static BOOL GetCanonicalName(LPSTR lpszFileName, LPSTR lpszCanonicalBuf);
static BOOL ActuallyTheSameFile(CHAR ARG_PTR *pszFile1,
                                CHAR ARG_PTR *pszFile2);
static BOOL ProcessNotification(CHAR ARG_PTR *pszSource,
                                CHAR ARG_PTR *pszDest, WORD wNotification);


/*
** static void DisplayErrorMessage(int fError);
**
** Display error message for given error condition.
**
** Arguments:  LZERROR_ code
**
** Returns:    void
**
** Globals:    none
*/
static VOID DisplayErrorMessage(INT fError)
{

   switch(fError)
   {
      case LZERROR_BADINHANDLE:
         LoadString(NULL, SID_NO_OPEN_INPUT, ErrorMsg, 2048);
         // WARNING: Cannot call CharToOemW  with src=dest
         CharToOem(ErrorMsg, ErrorMsg);
         printf(ErrorMsg, pszInFileName);
         break;

      case LZERROR_BADOUTHANDLE:
         LoadString(NULL, SID_NO_OPEN_OUTPUT, ErrorMsg, 2048);
         // WARNING: Cannot call CharToOemW  with src=dest
         CharToOem(ErrorMsg, ErrorMsg);
         printf(ErrorMsg, pszOutFileName);
         break;

      case LZERROR_READ:
         LoadString(NULL, SID_FORMAT_ERROR, ErrorMsg, 2048);
         // WARNING: Cannot call CharToOemW  with src=dest
         CharToOem(ErrorMsg, ErrorMsg);
         printf(ErrorMsg, pszInFileName);
         break;

      case LZERROR_WRITE:
         LoadString(NULL, SID_OUT_OF_SPACE, ErrorMsg, 2048);
         // WARNING: Cannot call CharToOemW  with src=dest
         CharToOem(ErrorMsg, ErrorMsg);
         printf(ErrorMsg, pszOutFileName);
         break;

      case LZERROR_UNKNOWNALG:
         LoadString(NULL, SID_UNKNOWN_ALG, ErrorMsg, 2048);
         // WARNING: Cannot call CharToOemW  with src=dest
         CharToOem(ErrorMsg, ErrorMsg);
         printf(ErrorMsg, pszInFileName);
         break;

      case BLANK_ERROR:
         break;

      default:
         LoadString(NULL, SID_GEN_FAILURE, ErrorMsg, 2048);
         // WARNING: Cannot call CharToOemW  with src=dest
         CharToOem(ErrorMsg, ErrorMsg);
         printf(ErrorMsg, pszInFileName, pszOutFileName);
         break;
   }
}


/*
** static void MakeDestFileName(char ARG_PTR *argv[], char ARG_PTR *pszDest);
**
** Create the appropriate destination file name.
**
** Arguments:  argv    - like argument to main()
**             pszDest - pointer to destination file name buffer to be filled
**                       in
**
** Returns:    void
**
** Globals:    none
*/
static VOID MakeDestFileName(CHAR ARG_PTR *argv[], CHAR ARG_PTR *pszDest)
{
   CHAR ARG_PTR *pszDestFile;

   if (nNumFileSpecs == 2 && bTargetIsDir == FALSE && bDoRename == FALSE)
      // Compress a single input file to a single output file.  N.b., we must
      // be careful to eat up the output file name command-line argument so
      // it doesn't get processed like another input file!
      STRCPY(pszDest, argv[GetNextFileArg(argv)]);
   else if (bTargetIsDir == TRUE)
   {
      // Prepend output file name with destination directory path name.
      STRCPY(pszDest, pszTargetName);

      // Isolate source file name from source file specification.
      pszDestFile = ExtractFileName(pszInFileName);

      // Add destination file name to destination directory path
      // specification.
      MakePathName(pszDest, pszDestFile);
   }
   else
      // Destination file name same as source file name.  N.b., this is an
      // error condition if (bDoRename == FALSE).
      STRCPY(pszDest, pszInFileName);
}


/*
** static BOOL GetCanonicalName(LPSTR lpszFileName, LPSTR lpszCanonicalBuf);
**
** Gets the canonical name for a given file specification.
**
** Arguments:  pszFileName    - file specification
**             szCanonicalBuf - buffer to be filled with canonical name
**
** Returns:    TRUE if successful.  FALSE if unsuccessful.
**
** N.b., szCanonicalBuf must be at least 128 bytes long.  The contents of
** szCanonicalBuf are only defined if the funstion returns TRUE.
**
*/
static BOOL GetCanonicalName(LPSTR lpszFileName, LPSTR lpszCanonicalBuf)
{
   BOOL bRetVal = FALSE;
   LPSTR lpszLastComp;

   return((BOOL) GetFullPathName(lpszFileName, MAX_PATH, lpszCanonicalBuf,  &lpszLastComp));
}


/*
** static BOOL ActuallyTheSameFile(char ARG_PTR *pszFile1,
**                                 char ARG_PTR *pszFile2);
**
** Checks to see if two file specifications point to the same physical file.
**
** Arguments:  pszFile1 - first file specification
**             pszFile2 - second file specification
**
** Returns:    BOOL - TRUE if the file specifications point to the same
**                    physical file.  FALSE if not.
**
** Globals:    none
*/
static BOOL ActuallyTheSameFile(CHAR ARG_PTR *pszFile1,
                                CHAR ARG_PTR *pszFile2)
{
   CHAR szCanonicalName1[MAX_PATH],
        szCanonicalName2[MAX_PATH];

   if (GetCanonicalName(pszFile1, szCanonicalName1) &&
       GetCanonicalName(pszFile2, szCanonicalName2))
   {
      if (! lstrcmpiA(szCanonicalName1, szCanonicalName2))
         return(TRUE);
   }

   return(FALSE);
}


/*
** static BOOL ProcessNotification(char ARG_PTR *pszSource,
**                                 char ARG_PTR *pszDest,
**                                 WORD wNotification);
**
** Callback function during file processing.
**
** Arguments:  pszSource     - source file name
**             pszDest       - destination file name
**             wNotification - process type query
**
** Returns:    BOOL - (wNotification == NOTIFY_START_*):
**                         TRUE if the source file should be "processed" into
**                         the destination file.  FALSE if not.
**                    else
**                         TRUE.
**
** Globals:    none
*/
static BOOL ProcessNotification(CHAR ARG_PTR *pszSource,
                                CHAR ARG_PTR *pszDest, WORD wNotification)
{
   switch(wNotification)
   {
      case NOTIFY_START_EXPAND:
      case NOTIFY_START_COPY:
      {
         // If we're listing files, display the name then tell caller to skip.

         if (bDoListFiles == TRUE)
         {
            PSTR p;

            //
            // Display just the base name from the target.  The prefix of the
            // target path is garbage (the source path.)
            //
            if(p = StringRevChar(pszDest,'\\')) {
               p++;
            } else {
               p = pszDest;
            }

            LoadString(NULL, SID_LISTING, ErrorMsg, 2048);
            CharToOem(ErrorMsg, ErrorMsg);
            printf(ErrorMsg, pszSource, p);

            nLocalFiles++;    // count files listed
            nTotalFiles++;    // count files listed

            return(FALSE);    // always skip file
         }

         // Fail if the source and destination files are identical.
         if (ActuallyTheSameFile(pszSource, pszDest))
         {
            LoadString(NULL, SID_COLLISION, ErrorMsg, 2048);
            // WARNING: Cannot call CharToOemW  with src=dest
            CharToOem(ErrorMsg, ErrorMsg);
            printf(ErrorMsg, pszSource);
            return(FALSE);
         }

         nLocalFiles++;    // count files expanded
         nTotalFiles++;    // count files expanded

         // Display start message.
         if (wNotification == NOTIFY_START_EXPAND) {
            LoadString(NULL, SID_EXPANDING, ErrorMsg, 2048);
            // WARNING: Cannot call CharToOemW  with src=dest
            CharToOem(ErrorMsg, ErrorMsg);
            printf(ErrorMsg, pszSource, pszDest);
         }
         else // NOTIFY_START_COPY
         {
            bCopyingFile = TRUE;
            LoadString(NULL, SID_COPYING, ErrorMsg, 2048);
            // WARNING: Cannot call CharToOemW  with src=dest
            CharToOem(ErrorMsg, ErrorMsg);
            printf(ErrorMsg, pszSource, pszDest);
         }
         break;
      }

      default:
         break;
   }

   return(TRUE);
}


/*
** int main(int argc, char *argv[]);
**
** Run command-line file compression program.
**
** Arguments:  figure it out
**
** Returns:    int - EXIT_SUCCESS if compression finished successfully,
**                   EXIT_FAILURE if not.
**
** Globals:    none
*/
INT __cdecl main(INT argc, CHAR *argv[])
{
   INT iSourceFileName,
       fError,
       nReturnCode = EXIT_SUCCESS;
   CHAR ARG_PTR pszDestFileName[MAX_PATH];
   LONG cblTotInSize = 0L,
        cblTotOutSize = 0L;
   PLZINFO pLZI;
   BOOL fIsDiamondFile;
   CHAR ARG_PTR *pszFilesSpec;
   BOOL fReportStats = TRUE;  // cleared if any multi-file CABs or listing

   USHORT wLanguageId = LANGIDFROMLCID(GetThreadLocale());

   if ((LANG_JAPANESE == PRIMARYLANGID(wLanguageId)) ||
       (LANG_KOREAN   == PRIMARYLANGID(wLanguageId)) ||
       (LANG_CHINESE  == PRIMARYLANGID(wLanguageId)))
   {
       //
       // This used to be #ifdef DBCS.  Now a runtime check.
       //
       DWORD dw = GetConsoleOutputCP();

       // WARNING: in product 1.1 we need to uncomment the SetConsole above
       // LoadString will return Ansi and printf will just pass it on
       // This will let cmd interpret the characters it gets.
       //   SetConsoleOutputCP(GetACP());

       switch (dw) {
          case 932:
          case 936:
          case 949:
          case 950:
             SetThreadLocale(MAKELCID(
                                MAKELANGID(
                                   PRIMARYLANGID(GetSystemDefaultLangID()),
                                   SUBLANG_ENGLISH_US),
                                SORT_DEFAULT));
             break;
          default:
             SetThreadLocale(MAKELCID(
                                MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                                SORT_DEFAULT));
             break;
       }
   }

   // Display sign-on banner.
   LoadString(NULL, SID_BANNER_TEXT, ErrorMsg, 2048);
   // WARNING: Cannot call CharToOemW  with src=dest
   CharToOem(ErrorMsg, ErrorMsg);
   printf(ErrorMsg);           

   // Parse command-line arguments.
   if (ParseArguments(argc, argv) != TRUE)
      return(EXIT_FAILURE);

   // Set up global target path name.
   pszTargetName = argv[iTarget];

   if (bDisplayHelp == TRUE)
   {
      // User asked for help.
      LoadString(NULL, SID_INSTRUCTIONS, ErrorMsg, 2048);
      // WARNING: Cannot call CharToOemW  with src=dest
      CharToOem(ErrorMsg, ErrorMsg);
      printf(ErrorMsg);
      return(EXIT_SUCCESS);
   }

   // Check for command line problems.
   if (CheckArguments() == FALSE)
      return(EXIT_FAILURE);

   // Set up ring buffer and I/O buffers.
   pLZI = InitGlobalBuffersEx();
   if (!pLZI || !InitDiamond())
   {
      LoadString(NULL, SID_INSUFF_MEM, ErrorMsg, 2048);
      // WARNING: Cannot call CharToOemW  with src=dest
      CharToOem(ErrorMsg, ErrorMsg);
      printf(ErrorMsg);
      return(EXIT_FAILURE);
   }

   // Process each source file.
   while ((iSourceFileName = GetNextFileArg(argv)) != FAIL)
   {
      nLocalFiles = 0;
      pLZI->cblOutSize = 0;

      // Set up global input file name.
      pszInFileName = CharLowerA(argv[iSourceFileName]);

      // Set up global output file name.
      MakeDestFileName(argv, pszDestFileName);
      pszOutFileName = CharLowerA(pszDestFileName);

      // Assume current file will be expanded.  The ProcessNotification()
      // callback will change this module global to TRUE if the file is being
      // copied instead of expanded.
      bCopyingFile = FALSE;

      //
      // Determine whether the file was compressed with diamond.
      // If so, we need to expand it specially.
      //
      fIsDiamondFile = IsDiamondFile(pszInFileName, &bContainsMultipleFiles);
      if (fIsDiamondFile) {
         
         if (bContainsMultipleFiles) {

            if (nNumFileSpecs == 1 && (bDoListFiles == FALSE)) {

               //
               // The source file is a multi-file CAB, and is the only file
               // on the command line.  We'll require an explicit filespec
               // which names the file(s) desired from within the CAB.
               // The Files specification may contain wildcards.
               //
               // If the user included multiple source files on the command
               // line, we'll assume they're expecting a lot of output, and
               // we'll default to all files.  It would be pointless to put
               // up a message anyway, since the screen will be scrolling.
               //

               if (pszSelectiveFilesSpec == NULL) {

                  LoadString(NULL, SID_FILESPEC_REQUIRED, ErrorMsg, 2048);
                  // WARNING: Cannot call CharToOemW  with src=dest
                  CharToOem(ErrorMsg, ErrorMsg);
                  printf(ErrorMsg);
                  continue;   // skip this (the only) source file
               }
            }

            if (!bTargetIsDir && (bDoListFiles == FALSE)) {

               //
               // The source file is a multi-file CAB, and the target is
               // a single file.  Now this just isn't going to work.  We'll
               // display this warning, and hope the user notices it.  If
               // a multiple sources were specified, they're going to get
               // a mess anyway.  We just won't contribute to it.
               //

               LoadString(NULL, SID_DEST_REQUIRED, ErrorMsg, 2048);
               // WARNING: Cannot call CharToOemW  with src=dest
               CharToOem(ErrorMsg, ErrorMsg);
               printf(ErrorMsg, pszInFileName);
               continue;   // skip this source file
            }

            pszFilesSpec = pszSelectiveFilesSpec;

            //
            // Don't try to interpret final stats if multi-file CABs are seen.
            // (Because of selective extract and other issues, you'll get silly
            // reports like "20000000 bytes expanded to 16320 bytes".)
            //

            fReportStats = FALSE;

         } else {

            //
            // Legacy: no selective expand from single-file CABs
            //

            pszFilesSpec = NULL;
         }

         //
         // Make sure renaming is ON if this is a multi-file CAB.
         //

         fError = ExpandDiamondFile(ProcessNotification,pszInFileName,
                           pszOutFileName,(bDoRename || bContainsMultipleFiles),
                           pszFilesSpec,pLZI);
      } else {
         fError = Expand(ProcessNotification, pszInFileName,
                           pszOutFileName, bDoRename, pLZI);
      }

      if (fError != TRUE) {
         // Deal with returned error codes.
         DisplayErrorMessage(nReturnCode = fError);

      } else if (bContainsMultipleFiles) {

         if (nLocalFiles == 0) {

            LoadString(NULL, SID_NO_MATCHES, ErrorMsg, 2048);
            // WARNING: Cannot call CharToOemW  with src=dest
            CharToOem(ErrorMsg, ErrorMsg);
            printf(ErrorMsg, pszInFileName, pszSelectiveFilesSpec);
         }

      } else {

         if (pLZI && pLZI->cblInSize && pLZI->cblOutSize) {

            // Keep track of cumulative statistics.
            cblTotInSize += pLZI->cblInSize;
            cblTotOutSize += pLZI->cblOutSize;

            if (bCopyingFile) {
               LoadString(NULL, SID_COPY_REPORT, ErrorMsg, 2048);
               // WARNING: Cannot call CharToOemW  with src=dest
               CharToOem(ErrorMsg, ErrorMsg);
               printf(ErrorMsg, pszInFileName, pLZI->cblInSize);
            }
            else {
               LoadString(NULL, SID_FILE_REPORT, ErrorMsg, 2048);
               // WARNING: Cannot call CharToOemW  with src=dest
               CharToOem(ErrorMsg, ErrorMsg);
               printf(ErrorMsg, pszInFileName, pLZI->cblInSize, pLZI->cblOutSize,
                      (INT)(100 * pLZI->cblOutSize / pLZI->cblInSize - 100));
            }
         }
      }

      // Separate individual file processing message blocks by a blank line.
      printf("\n");
   }

   // Free memory used by ring buffer and I/O buffers.
   FreeGlobalBuffers(pLZI);

   TermDiamond();

   if (!fReportStats || bDoListFiles) {

      if (nTotalFiles > 1) {

         LoadString(NULL, SID_TOTAL_COUNT, ErrorMsg, 2048);
         // WARNING: Cannot call CharToOemW  with src=dest
         CharToOem(ErrorMsg, ErrorMsg);
         printf(ErrorMsg, nTotalFiles);
      }

   } else {

      // Display cumulative report for multiple files.
      if ((nTotalFiles > 1) && (cblTotInSize != 0)) {

         // Scale results to get accurate %
         LONG cblAdjInSize = cblTotInSize,
              cblAdjOutSize = cblTotOutSize;
         while (cblAdjInSize > 100000) {
           cblAdjInSize /= 2;
           cblAdjOutSize /= 2;
           }
         cblAdjOutSize += (cblAdjInSize / 200);    // round off (+0.5%)
         if (cblAdjOutSize < 0) {
           cblAdjOutSize = 0;
           }

         LoadString(NULL, SID_TOTAL_REPORT, ErrorMsg, 2048);
         // WARNING: Cannot call CharToOemW  with src=dest
         CharToOem(ErrorMsg, ErrorMsg);
         printf(ErrorMsg, nTotalFiles, cblTotInSize, cblTotOutSize,
                (INT)(100 * cblAdjOutSize / cblAdjInSize - 100));
      }

   }

   return(nReturnCode);
}

