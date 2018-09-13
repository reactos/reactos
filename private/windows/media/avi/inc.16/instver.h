/**************************************************************************
 *
 * InstVer.h - SDK Version-checking installer include file
 *
 * Copyright (C) Microsoft, 1991-1994
 * February 25, 1991
 *
 **************************************************************************/

/** Error returns from DLL/.EXE */
#define ERR_BADSRCDIR	       -1    /* invalid library (source) pathname */
#define ERR_BADWINDIR	       -2    /* invalid windows directory */
#define ERR_BADARGS	       -3    /* incorrect arguments (DOS .EXE only) */
#define ERR_CREATINGFILE       -4    /* error creating destination file */
#define ERR_CANNOTREADSRC      -5    /* error opening/reading source file  */
#define ERR_OUTOFSPACE	       -6    /* out of disk space copying file */
#define ERR_BADDATFILE	       -7    /* invalid DAT file */
#define ERR_CANTOPENDATFILE    -8    /* can't open DAT file for reading */
#define ERR_NOMEM	       -9    /* out of memory for local buffers */
#define ERR_READINGDATFILE     -10   /* error in reading DAT file*/
#define ERR_RENAME	       -11   /* error renaming destination file */


/* library installer function */

BOOL FAR PASCAL InstallVersionFiles (LPSTR lpszLibPath,
				     LPSTR lpszWindowsPath,
				     LPSTR lpszDatFile);
