#ifndef INSTALL_H
#define INSTALL_H

///////////////////////////////////////////////////////////////////////////////


#define SECTION         512                   // Maximum size of section
#define MAXSTR          256
#define UNLIST_LINE     1
#define NO_UNLIST_LINE  0
#define WEC_RESTART     0x42
#define DESC_SYS        3
#define DESC_INF        2
#define DESC_EXE        1
#define DESC_NOFILE     0

#define FALLOC(n)                ((VOID *)GlobalAlloc(GPTR, n))
#define FFREE(n)                 GlobalFree(n)
#define ALLOC(n)                 (VOID *)LocalAlloc(LPTR,n)
#define FREE(p)                  LocalFree(p)
#define REALLOC(p,n)             LocalRealloc(p,n,LMEM_MOVEABLE)

#define SEEK_CUR 1
#define SEEK_END 2
#define SEEK_SET 0

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

#define SLASH(c)     ((c) == TEXT('/')|| (c) == TEXT('\\'))
#define CHSEPSTR                TEXT("\\")
#define COMMA   TEXT(',')
#define SPACE   TEXT(' ')

/* Globals and routines for .inf file parsing */

typedef LPTSTR    PINF;

/* Message types for FileCopy callback function */

typedef BOOL (*FPFNCOPY) (int,DWORD_PTR,LPTSTR);
#define COPY_ERROR          0x0001
#define COPY_INSERTDISK     0x0003
#define COPY_QUERYCOPY      0x0004
#define COPY_START          0x0005
#define COPY_END            0x0006
#define COPY_EXISTS         0x0007

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


/*******************************************************************
 *
 * Global Variables
 *
 *******************************************************************/

 // Path to the directory where we found the .inf file

 extern char szSetupPath[MAX_PATH];

 // Path to the user's disk(s)

 extern char szDiskPath[MAX_PATH];   // Path to the default drive -
                                     //
 extern BOOL bRetry;

 // Name of the driver being installed

 extern char szDrv[120];

 //

 extern char szFileError[50];

 // Parent window for file copy dialogues

 extern HWND hMesgBoxParent;

 // TRUE on copying first file to prompt user if file already exists
 // FALSE for subsequent copies

 extern BOOL bQueryExist;

///////////////////////////////////////////////////////////////////////////////

BOOL DefCopyCallback(int msg, DWORD_PTR n, LPTSTR szFile);
UINT FileCopy (LPTSTR szSource, LPTSTR szDir, FPFNCOPY fpfnCopy, UINT fCopy, HWND hPar, BOOL fQuery);
LONG TryCopy(LPTSTR, LPTSTR, LPTSTR, FPFNCOPY);
BOOL GetDiskPath(LPTSTR Disk, LPTSTR szPath);
BOOL ExpandFileName(LPTSTR szFile, LPTSTR szPath);
void catpath(LPTSTR path, LPTSTR sz);
LPTSTR FileName(LPTSTR szPath);
LPTSTR RemoveDiskId(LPTSTR szPath);
LPTSTR StripPathName(LPTSTR szPath);
BOOL IsFileKernelDriver(LPTSTR szPath);
UINT ConvertFlagToValue(DWORD dwFlags);
BOOL IsValidDiskette(int iDrive);
BOOL IsDiskInDrive(int iDisk);
BOOL GetInstallPath(LPTSTR szDirOfSrc);
BOOL wsInfParseInit(void);
void wsStartWait();
void wsEndWait();
int fDialog(int id, HWND hwnd, DLGPROC fpfn);
UINT wsCopyError(int n, LPTSTR szFile);
UINT wsInsertDisk(LPTSTR Disk, LPTSTR szSrcPath);
INT_PTR wsDiskDlg(HWND hDlg, UINT uiMessage, UINT wParam, LPARAM lParam);
UINT wsCopySingleStatus(int msg, DWORD_PTR n, LPTSTR szFile);
INT_PTR wsExistDlg(HWND hDlg, UINT uiMessage, UINT wParam, LPARAM lParam);
VOID RemoveSpaces(LPTSTR szPath, LPTSTR szEdit);
PINF infLoadFile(int fh);
PINF infOpen(LPTSTR szInf);
void infClose(PINF pinf);
UINT_PTR FindSection(PINF pInf, LPTSTR pszSect);
BOOL fnGetDataString(PINF npszData, LPTSTR szDataStr, LPTSTR szBuf);
PINF infSetDefault(PINF pinf);
PINF infFindSection(PINF pinf, LPTSTR szSection);
BOOL infGetProfileString(PINF pinf, LPTSTR szSection,LPTSTR szItem,LPTSTR szBuf);
BOOL infParseField(PINF szData, int n, LPTSTR szBuf);
int infLineCount(PINF pinf);
PINF infNextLine(PINF pinf);
int infLineCount(PINF pinf);
PINF infFindInstallableDriversSection(PINF pinf);

#endif
