/*
** args.c - Command-line argument manipulation functions.
**
** Author:  DavidDi
**
** N.b., setargv.obj must be linked with this module for the command-line
** parsing to function properly.
*/


// Headers
///////////

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "..\..\libs\common.h"

#include "args.h"
#include "main.h"
#include "messages.h"

extern BOOL PathType(LPSTR lpszFileString);   /* WIN32 MOD*/

// Globals
///////////

// All the globals defined in this module are set by ParseArguments().

BOOL bDoRename,      // flag for performing compressed file renaming
     bDisplayHelp,   // flag for displaying help information
     bTargetIsDir;   // flag telling whether or not files are being
                     // compressed to a directory

INT nNumFileSpecs,   // number of non-switch, non-directory command-line
                     // arguments, assumed to be file specifications
    iTarget;         // argv[] index of target directory argument, or FAIL if
                     // none present

BOOL bDoListFiles;   // flag for displaying list of files from a CAB
                     // (instead of actually expanding them)
CHAR ARG_PTR *pszSelectiveFilesSpec; // name of file(s) to expand from a CAB


/*
** BOOL ParseArguments(int argc, char ARG_PTR *argv[]);
**
** Parse command-line arguments.
**
** Arguments:  like arguments to main()
**
** Returns:    TRUE if command-line arguments parsed successfully.  FALSE if
**             not.
**
** Globals:    All globals defined in this module are set in this function,
**             as described above.
*/

BOOL ParseArguments(INT argc, CHAR ARG_PTR *argv[])
{
   INT i;
   CHAR chSwitch;

   // Set up default values for globals.
   bDoRename = FALSE;
   bDisplayHelp = FALSE;
   bTargetIsDir = FALSE;
   nNumFileSpecs = 0;
   iTarget = FAIL;
   bDoListFiles = FALSE;
   pszSelectiveFilesSpec = NULL;

   // Look at each command-line argument.
   for (i = 1; i < argc; i++)
      if (ISSWITCH(*(argv[i])))
      {
         // Get switch character.
         chSwitch = *(argv[i] + 1);

         //for bad DBCS argument
         if( IsDBCSLeadByte(chSwitch) )
         {
            CHAR work[3];
            lstrcpyn(work, argv[i] + 1, 3);
            LoadString(NULL, SID_BAD_SWITCH2, ErrorMsg, 1024);
            printf(ErrorMsg, work);
            return(FALSE);
         }

         // Classify switch.
         if (toupper(chSwitch) == toupper(chRENAME_SWITCH))
            bDoRename = TRUE;
         else if (toupper(chSwitch) == toupper(chHELP_SWITCH))
            bDisplayHelp = TRUE;
         else if (toupper(chSwitch) == toupper(chLIST_SWITCH))
            bDoListFiles = bDoRename = TRUE;
         else if ((toupper(chSwitch) == toupper(chSELECTIVE_SWITCH)) &&
                  (argv[i][2] == ':') &&
                  (argv[i][3] != '\0') &&
                  (pszSelectiveFilesSpec == NULL))
            pszSelectiveFilesSpec = &argv[i][3];
         else
         {
            // Unrecognized switch.
	    LoadString(NULL, SID_BAD_SWITCH, ErrorMsg, 1024);
            // WARNING: Cannot call CharToOemW  with src=dest
            CharToOem(ErrorMsg, ErrorMsg);
            printf(ErrorMsg, chSwitch);
            return(FALSE);
         }
      }
      else
      {
         // Keep track of last non-switch command-line argument as
         // destination argument.
         iTarget = i;

         if (IsDir((LPSTR)argv[i]) == FALSE)
            // Non-switch arguments are assumed to be file specifications.
            nNumFileSpecs++;
      }

   // Set bTargetIsDir.
   if (iTarget != FAIL)
      bTargetIsDir = IsDir((LPSTR)argv[iTarget]);

   // Command-line arguments parsed successsfully.
   return(TRUE);
}



/*
** BOOL CheckArguments(void);
**
** Check command-line arguments for error conditions.
**
** Arguments:  void
**
** Returns:    BOOL - TRUE if no problems found.  FALSE if problem found.
**
** Globals:    none
*/
BOOL CheckArguments(VOID)
{
   if (nNumFileSpecs < 1)
   {
      // No file specifications given.
      LoadString(NULL, SID_NO_FILE_SPECS, ErrorMsg, 1024);
      // WARNING: Cannot call CharToOemW  with src=dest
      CharToOem(ErrorMsg, ErrorMsg);
      printf(ErrorMsg);
      return(FALSE);
   }
   else if (nNumFileSpecs == 1 && bDoRename == FALSE && bTargetIsDir == FALSE && bDoListFiles == FALSE)
   {
      // We don't want to process a source file on to itself.
      LoadString(NULL, SID_NO_OVERWRITE, ErrorMsg, 1024);
      // WARNING: Cannot call CharToOemW  with src=dest
      CharToOem(ErrorMsg, ErrorMsg);
      printf(ErrorMsg, pszTargetName);
      return(FALSE);
   }
   else if (nNumFileSpecs >  2 && bDoRename == FALSE && bTargetIsDir == FALSE && bDoListFiles == FALSE)
   {
      // There are multiple files to process, and the destination
      // specification argument is not a directory.  But we weren't told to
      // rename the output files.  Bail out since we don't want to wipe out
      // the input files.
      LoadString(NULL, SID_NOT_A_DIR, ErrorMsg, 1024);
      // WARNING: Cannot call CharToOemW  with src=dest
      CharToOem(ErrorMsg, ErrorMsg);
      printf(ErrorMsg, pszTargetName);
      return(FALSE);
   }
   else if (bDoListFiles && bTargetIsDir == TRUE)
   {
      // Requested only a listing of the files from the source CAB, but then
      // supplied a destination directory.  There is no destination when we're
      // only displaying names.  Bail out because he must be confused.
      LoadString(NULL, SID_UNEXP_TARGET, ErrorMsg, 1024);
      // WARNING: Cannot call CharToOemW  with src=dest
      CharToOem(ErrorMsg, ErrorMsg);
      printf(ErrorMsg, pszTargetName);
      return(FALSE);
   }
   else
      // No problems encountered.
      return(TRUE);
}


/*
** int GetNextFileArg(char ARG_PTR *argv[]);
**
** Find the next file name argument on the command-line.
**
** Arguments:  like argument to main()
**
** Returns:    int - Index in argv[] of next file name argument.  FAIL if
**                   none found.
**
** Globals:    none
*/
INT GetNextFileArg(CHAR ARG_PTR *argv[])
{
   INT i;
   static INT iLastArg = 0;

   for (i = iLastArg + 1; i <= iTarget; i++)
      if (! ISSWITCH(*(argv[i])) &&
          (i < iTarget || bTargetIsDir == FALSE)
          && (! IsDir((LPSTR)argv[i])))
         return(iLastArg = i);

   return(FAIL);
}

/* WIN32 MODS   */

/* returns 0 if not directory, 1 if so */
INT IsDir(LPSTR lpszTestString)
{

    BOOL bRetVal;

    bRetVal = PathType(lpszTestString);
	 if(bRetVal == 0){		/*assert*/
		bRetVal++;				/* this is because if lpszTestString file doesnt exist*/
									/* API returns 0, so I increment to 1, cause is NOT directory*/  
    }              
	 return(--bRetVal);       /* because returns 2 if dir, 1 if not*/    

}
