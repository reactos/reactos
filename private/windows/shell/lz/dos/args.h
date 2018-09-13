/*
** args.h - Globals and prototypes for args.c.
**
** Author:  DavidDi
*/


// Globals
///////////

extern BOOL bDoRename,     // flag for performing compressed file renaming
            bDisplayHelp,  // flag for displaying help information
            bTargetIsDir,  // flag telling whether or not files are being
                           // compressed to a directory
            bUpdateOnly;   // flag for conditional compression based on
                           // existing target file's date/time stamp relative
                           // to source file.

extern INT nNumFileSpecs,  // number of non-switch command-line arguments
           iTarget;        // argv[] index of target directory argument

extern BOOL bDoListFiles;  // flag for displaying list of files from a CAB
                           // (instead of actually expanding them)
extern CHAR ARG_PTR *pszSelectiveFilesSpec; // name of file(s) to expand from a CAB

#ifdef COMPRESS
extern BYTE byteAlgorithm; // compression / expansion algorithm to use
#endif


// Prototypes
//////////////

extern BOOL ParseArguments(INT argc, CHAR ARG_PTR *argv[]);
extern BOOL CheckArguments(VOID);
extern INT GetNextFileArg(CHAR ARG_PTR *argv[]);
