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
#include <stdlib.h>

#include "..\..\libs\common.h"
#include "..\..\libs\header.h"

#include "args.h"
#include "main.h"
#include "messages.h"
#include <diamondc.h>
#include "mydiam.h"

extern BOOL PathType(LPSTR lpszFileString);   /* WIN32 MOD*/

// Globals
///////////

// All the globals defined in this module are set by ParseArguments().

BOOL bDoRename,      // flag for performing compressed file renaming
     bDisplayHelp,   // flag for displaying help information
     bTargetIsDir,   // flag telling whether or not files are being
                     // compressed to a directory
     bUpdateOnly;    // flag for conditional compression based on
                     // existing target file's date/time stamp relative
                     // to source file.

INT nNumFileSpecs,   // number of non-switch, non-directory command-line
                     // arguments, assumed to be file specifications
    iTarget;         // argv[] index of target directory argument, or FAIL if
                     // none present

BYTE byteAlgorithm;  // compression / expansion algorithm to use
TCOMP DiamondCompressionType;  // 0 if not diamond (ie, LZ)


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
   TCOMP Level;
   TCOMP Mem;
   CHAR *p;

   // Set up default values for globals.
   bDoRename = FALSE;
   bDisplayHelp = FALSE;
   bTargetIsDir = FALSE;
   nNumFileSpecs = 0;
   iTarget = FAIL;
   byteAlgorithm = DEFAULT_ALG;
   DiamondCompressionType = 0;

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
         else if (toupper(chSwitch) == toupper(chUPDATE_SWITCH))
            bUpdateOnly = TRUE;
         else if (toupper(chSwitch) == toupper(chALG_SWITCH)) {

            switch(*(argv[i] + 2)) {

            case 'x':
            case 'X':
                //
                // LZX. Also set memory.
                //
                Mem = (TCOMP)atoi(argv[i] + 3);

                if((Mem < (tcompLZX_WINDOW_LO >> tcompSHIFT_LZX_WINDOW))
                || (Mem > (tcompLZX_WINDOW_HI >> tcompSHIFT_LZX_WINDOW))) {

                    Mem = (tcompLZX_WINDOW_LO >> tcompSHIFT_LZX_WINDOW);
                }

                byteAlgorithm = LZX_ALG;
                DiamondCompressionType = TCOMPfromLZXWindow( Mem );
                break;

            case 'q':
            case 'Q':
                //
                // Quantum. Also set level.
                //
                Level = (TCOMP)atoi(argv[i] + 3);
                Mem = (p = strchr(argv[i]+3,',')) ? (TCOMP)atoi(p+1) : 0;

                if((Level < (tcompQUANTUM_LEVEL_LO >> tcompSHIFT_QUANTUM_LEVEL))
                || (Level > (tcompQUANTUM_LEVEL_HI >> tcompSHIFT_QUANTUM_LEVEL))) {

                    Level = ((tcompQUANTUM_LEVEL_HI - tcompQUANTUM_LEVEL_LO) / 2)
                          + tcompQUANTUM_LEVEL_LO;

                    Level >>= tcompSHIFT_QUANTUM_LEVEL;
                }

                if((Mem < (tcompQUANTUM_MEM_LO >> tcompSHIFT_QUANTUM_MEM))
                || (Mem > (tcompQUANTUM_MEM_HI >> tcompSHIFT_QUANTUM_MEM))) {

                    Mem = ((tcompQUANTUM_MEM_HI - tcompQUANTUM_MEM_LO) / 2)
                        + tcompQUANTUM_MEM_LO;

                    Mem >>= tcompSHIFT_QUANTUM_MEM;
                }

                byteAlgorithm = QUANTUM_ALG;
                DiamondCompressionType = TCOMPfromTypeLevelMemory(
                                            tcompTYPE_QUANTUM,
                                            Level,
                                            Mem
                                            );
                break;

            case 'l':
            case 'L':
                DiamondCompressionType = 0;
                byteAlgorithm = DEFAULT_ALG;
                break;

            default:
                DiamondCompressionType = tcompTYPE_MSZIP;
                byteAlgorithm = MSZIP_ALG;
                break;
            }
         } else
         {
            // Unrecognized switch.
            LoadString(NULL, SID_BAD_SWITCH, ErrorMsg, 1024);
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
      printf(ErrorMsg);
      return(FALSE);
   }
   else if (nNumFileSpecs == 1 && bDoRename == FALSE && bTargetIsDir == FALSE)
   {
      // We don't want to process a source file on to itself.
      LoadString(NULL, SID_NO_OVERWRITE, ErrorMsg, 1024);
      printf(ErrorMsg, pszTargetName);
      return(FALSE);
   }
   else if (nNumFileSpecs >  2 && bDoRename == FALSE && bTargetIsDir == FALSE)
   {
      // There are multiple files to process, and the destination
      // specification argument is not a directory.  But we weren't told to
      // rename the output files.  Bail out since we don't want to wipe out
      // the input files.
      LoadString(NULL, SID_NOT_A_DIR, ErrorMsg, 1024);
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
