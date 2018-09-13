/*
 * SULIB.H - Windows/DOS Setup common code
 *
 *  Modification History:
 *
 *
 *  3/23/89  Toddla   combined common.h and prototypes into this file
 *  1/28/91  MichaelE Added AUDIO_CARDS_SECT for different audio card choices.
 *  4/17/91  Removed some DOS.ASM routines not used anywhere.
 *  5/29/91  JKLin added prototype for IsCDROMDrive function
 *
 */

#define FALLOC(n)                ((VOID *)GlobalAlloc(GPTR, n))
#define FFREE(n)                 GlobalFree(n)

#define ALLOC(n)                 (VOID *)LocalAlloc(LPTR,n)
#define FREE(p)                  LocalFree(p)
#define REALLOC(p,n)             LocalRealloc(p,n,LMEM_MOVEABLE)


/* flags for _llseek */

#define  SEEK_CUR 1
#define  SEEK_END 2
#define  SEEK_SET 0

#define MAXFILESPECLEN       MAX_PATH /* drive: + path length max + Null Byte */
#define MAX_INF_LINE_LEN     256      /* Maximum length of any .inf line */
#define MAX_SYS_INF_LEN      256      /* ##: + 8.3 + NULL */
#define MAX_SECT_NAME_LEN    40       /* Max length of a section Name. */
#define MAX_FILE_SPEC        MAX_PATH // 8.3 + X: + NULL.

#define DISK_SECT              TEXT("disks")
#define OEMDISK_SECT           TEXT("oemdisks")



/* Return codes from 'file exists' dialog */

enum {
    CopyNeither,            // User wants to cancel if file exists
    CopyCurrent,            // User wants to use current file
    CopyNew                 // User wants to copy new file
};

#define SLASH(c)     ((c) == TEXT('/') || (c) == TEXT('\\'))
#define CHSEPSTR                TEXT("\\")
#define COMMA   TEXT(',')
#define SPACE   TEXT(' ')

/* Globals and routines for .inf file parsing */

typedef LPTSTR    PINF;

extern PINF infOpen(LPTSTR szInf);
extern void infClose(PINF pinf);
extern PINF infSetDefault(PINF pinf);
extern PINF infFindSection(PINF pinf, LPTSTR szSection);
extern BOOL infGetProfileString(PINF pinf, LPTSTR szSection, LPTSTR szItem,LPTSTR szBuf);
extern BOOL infParseField(PINF szData, int n, LPTSTR szBuf);
extern PINF infNextLine(PINF pinf);
extern int  infLineCount(PINF pinf);
extern BOOL infLookup(LPTSTR szInf, LPTSTR szBuf);
extern PINF FindInstallableDriversSection(PINF pinf);

/* Message types for FileCopy callback function */

typedef BOOL (*FPFNCOPY) (int,DWORD_PTR,LPTSTR);
#define COPY_ERROR          0x0001
#define COPY_INSERTDISK     0x0003
#define COPY_QUERYCOPY      0x0004
#define COPY_START          0x0005
#define COPY_END            0x0006
#define COPY_EXISTS         0x0007

extern UINT FileCopy (LPTSTR szSource, LPTSTR szDir, FPFNCOPY fpfnCopy, UINT fCopy);

/* Option Flag values for FileCopy */

#define FC_FILE              0x0000
#define FC_LIST              0x0001
#define FC_SECTION           0x0002
#define FC_QUALIFIED         0x0008
#define FC_DEST_QUALIFIED    0x0010
#define FC_LISTTYPE          0x0020
#define FC_CALLBACK_WITH_VER 0x0040

#define FC_ABORT    0
#define FC_IGNORE   1
#define FC_RETRY    2
#define FC_ERROR_LOADED_DRIVER  0x80

/* External functions from copy.c */

extern BOOL ExpandFileName(LPTSTR szFile, LPTSTR szPath);
extern void catpath(LPTSTR path, LPTSTR sz);
extern BOOL fnFindFile(TCHAR *);
extern LPTSTR FileName(LPTSTR szPath);
extern LPTSTR RemoveDiskId(LPTSTR szPath);
extern LPTSTR StripPathName(LPTSTR szPath);
extern BOOL IsFileKernelDriver(LPTSTR szPath);


/*******************************************************************
 *
 * Global Variables
 *
 *******************************************************************/

 // Path to the directory where we found the .inf file

 extern TCHAR szSetupPath[MAX_PATH];

 // Path to the user's disk(s)

 extern TCHAR szDiskPath[MAX_PATH];   // Path to the default drive -
                                     //
 extern BOOL bRetry;

 // Name of the driver being installed

 extern TCHAR szDrv[120];

 //

 extern TCHAR szFileError[50];

 // Parent window for file copy dialogues

 HWND hMesgBoxParent;

 // TRUE on copying first file to prompt user if file already exists
 // FALSE for subsequent copies

 extern BOOL bQueryExist;
