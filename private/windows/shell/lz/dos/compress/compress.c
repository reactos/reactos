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
#include <sys\types.h>
#include <sys\stat.h>

#include "..\..\libs\common.h"
#include "..\..\libs\buffers.h"
#include "..\..\libs\header.h"

#include "args.h"
#include "main.h"
#include "messages.h"

#include <diamondc.h>
#include "mydiam.h"

// Globals
///////////

CHAR ARG_PTR *pszInFileName,     // input file name
             *pszOutFileName,    // output file name
             *pszTargetName;     // target path name

TCHAR   ErrorMsg[1024];


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
         LoadString(NULL, SID_NO_OPEN_INPUT, ErrorMsg, 1024);
         printf(ErrorMsg, pszInFileName);
         break;

      case LZERROR_BADOUTHANDLE:
         LoadString(NULL, SID_NO_OPEN_OUTPUT, ErrorMsg, 1024);
         printf(ErrorMsg, pszOutFileName);
         break;

      case LZERROR_READ:
         LoadString(NULL, SID_NO_READ_INPUT, ErrorMsg, 1024);
         printf(ErrorMsg, pszInFileName);
         break;

      case LZERROR_WRITE:
         LoadString(NULL, SID_OUT_OF_SPACE, ErrorMsg, 1024);
         printf(ErrorMsg, pszOutFileName);
         break;

      case BLANK_ERROR:
         break;

      default:
         LoadString(NULL, SID_GEN_FAILURE, ErrorMsg, 1024);
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
      case NOTIFY_START_COMPRESS:
      {
         // Fail if the source and destination files are identical.
         if (ActuallyTheSameFile(pszSource, pszDest))
         {
            LoadString(NULL, SID_COLLISION, ErrorMsg, 1024);
            printf(ErrorMsg, pszSource);
            return(FALSE);
         }

         // Display start message.
         switch (byteAlgorithm)
         {
         case LZX_ALG:
             LoadString(
                NULL,
                SID_COMPRESSING_LZX,
                ErrorMsg,
                1024
                );
             printf(ErrorMsg, pszSource, pszDest,
                        CompressionMemoryFromTCOMP(DiamondCompressionType)
                        );
             break;

         case QUANTUM_ALG:
             LoadString(
                NULL,
                SID_COMPRESSING_QUANTUM,
                ErrorMsg,
                1024
                );
             printf(ErrorMsg, pszSource, pszDest,
                        CompressionLevelFromTCOMP(DiamondCompressionType),
                        CompressionMemoryFromTCOMP(DiamondCompressionType)
                        );
             break;

         default:
             LoadString(
                NULL,
                (byteAlgorithm == MSZIP_ALG) ? SID_COMPRESSING_MSZIP : SID_COMPRESSING,
                ErrorMsg,
                1024
                );
             printf(ErrorMsg, pszSource, pszDest);
         }
      }
         break;

      default:
         break;
   }

   return(TRUE);
}


//
//  static BOOL FileTimeIsNewer( const char* pszFile1, const char* pszFile2 );
//
//  Return value is TRUE if time stamp on pszFile1 is newer than the
//  time stamp on pszFile2.  If either of the two files do not exist,
//  the return value is also TRUE (for indicating that pszFile2 should
//  be update from pszFile1).  Otherwise, the return value is FALSE.
//

static BOOL FileTimeIsNewer( const char* pszFile1, const char* pszFile2 ) {

    struct _stat StatBufSource,
                 StatBufDest;

    if (( _stat( pszFile2, &StatBufDest   )) ||
        ( _stat( pszFile1, &StatBufSource )) ||
        ( StatBufSource.st_mtime > StatBufDest.st_mtime ))
        return TRUE;

    return FALSE;

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
       nTotalFiles = 0,
       nReturnCode = EXIT_SUCCESS;
   CHAR ARG_PTR pszDestFileName[MAX_PATH];
   CHAR chTargetFileName[ MAX_PATH ];
   LONG cblTotInSize = 0L,
        cblTotOutSize = 0L;

   PLZINFO pLZI;

   USHORT wLanguageId = LANGIDFROMLCID(GetThreadLocale());

   if ((LANG_JAPANESE == PRIMARYLANGID(wLanguageId)) ||
       (LANG_KOREAN   == PRIMARYLANGID(wLanguageId)) ||
       (LANG_CHINESE  == PRIMARYLANGID(wLanguageId)))
   {
      //
      // This used to be #ifdef DBCS. Now a runtime check.
      //
      DWORD dw = GetConsoleOutputCP();

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
                                SORT_DEFAULT ) );
             break;
      }
   }

   // Display sign-on banner.
   LoadString(NULL, SID_BANNER_TEXT, ErrorMsg, 1024);
   printf(ErrorMsg);

   // Parse command-line arguments.
   if (ParseArguments(argc, argv) != TRUE)
      return(EXIT_FAILURE);

   // Set up global target path name.
   pszTargetName = argv[iTarget];

   if (bDisplayHelp == TRUE)
   {
      // User asked for help.
      LoadString(NULL, SID_INSTRUCTIONS, ErrorMsg, 1024);
      printf(ErrorMsg);
      LoadString(NULL, SID_INSTRUCTIONS2, ErrorMsg, 1024);
      printf(ErrorMsg);
      LoadString(NULL, SID_INSTRUCTIONS3, ErrorMsg, 1024);
      printf(ErrorMsg);
      return(EXIT_SUCCESS);
   }

   // Check for command line problems.
   if (CheckArguments() == FALSE)
      return(EXIT_FAILURE);

   // Set up ring buffer and I/O buffers.
   pLZI = InitGlobalBuffersEx();
   if (!pLZI)
   {
      LoadString(NULL, SID_INSUFF_MEM, ErrorMsg, 1024);
      printf(ErrorMsg);
      return(EXIT_FAILURE);
   }

   // Process each source file.
   while ((iSourceFileName = GetNextFileArg(argv)) != FAIL)
   {
      // Set up global input file name.
      pszInFileName = CharLowerA(argv[iSourceFileName]);

      // Set up global output file name.
      MakeDestFileName(argv, pszDestFileName);
      pszOutFileName = CharLowerA(pszDestFileName);

      strcpy( chTargetFileName, pszOutFileName );

      if ( bDoRename )
          MakeCompressedName( chTargetFileName );

      if (( ! bUpdateOnly ) ||
          ( FileTimeIsNewer( pszInFileName, chTargetFileName ))) {

          if(DiamondCompressionType) {
             fError = DiamondCompressFile(ProcessNotification,pszInFileName,
                                            pszOutFileName,bDoRename,pLZI);
          } else {
             fError = Compress(ProcessNotification, pszInFileName,
                                 pszOutFileName, byteAlgorithm, bDoRename, pLZI);
          }

          if(fError != TRUE)
             // Deal with returned error codes.
             DisplayErrorMessage(nReturnCode = fError);
          else
          {
             nTotalFiles++;

             if (pLZI && pLZI->cblInSize && pLZI->cblOutSize) {

                // Keep track of cumulative statistics.
                cblTotInSize += pLZI->cblInSize;
                cblTotOutSize += pLZI->cblOutSize;

                // Display report for each file.
                LoadString(NULL, SID_FILE_REPORT, ErrorMsg, 1024);
                printf(ErrorMsg, pszInFileName, pLZI->cblInSize, pLZI->cblOutSize,
                   (INT)(100 - 100 * pLZI->cblOutSize / pLZI->cblInSize));

             }
             else {
                LoadString(NULL, SID_EMPTY_FILE_REPORT, ErrorMsg, 1024);
                printf(ErrorMsg, pszInFileName, 0, 0);
             }

          }
          // Separate individual file processing message blocks by a blank line.
          printf("\n");
      }

   }

   // Free memory used by ring buffer and I/O buffers.
   FreeGlobalBuffers(pLZI);

   // Display cumulative report for multiple files.
   if (nTotalFiles > 1) {

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

      LoadString(NULL, SID_TOTAL_REPORT, ErrorMsg, 1024);
      printf(ErrorMsg, nTotalFiles, cblTotInSize, cblTotOutSize,
             (INT)(100 - 100 * cblAdjOutSize / cblAdjInSize));
   }

   return(nReturnCode);
}

