/*++

Copyright (c) 1990-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    fileopen.h

Abstract:

    This module contains the header information for the Win32 fileopen
    dialogs.

Revision History:

--*/



#ifdef __cplusplus
extern "C" {
#endif



//
//  Include Files.
//

#include <help.h>




//
//  Constant Declarations.
//

#define MAX_DISKNAME                   260
#define TOOLONGLIMIT                   MAX_PATH
#define MAX_FULLPATHNAME               520                 // 260 + 260
#define WARNINGMSGLENGTH               MAX_FULLPATHNAME

#define ERROR_NO_DISK_IN_CDROM         92L
#define ERROR_NO_DISK_IN_DRIVE         93L
#define ERROR_DIR_ACCESS_DENIED        94L
#define ERROR_FILE_ACCESS_DENIED       95L
#define ERROR_CREATE_NO_MODIFY         96L
#define ERROR_NO_DRIVE                 97L
#define ERROR_PORTNAME                 98L
#define ERROR_LAZY_READONLY            99L

//
//  Internal Flags.
//
//  Be sure to update OFN_ALL_INTERNAL_FLAGS if more internal flags are
//  added.
//
#define OFN_ALL_INTERNAL_FLAGS         0xf8000000     // Keep this in sync
#define OFN_PREFIXMATCH                0x80000000     // Internal
#define OFN_DIRSELCHANGED              0x40000000     // Internal
#define OFN_DRIVEDOWN                  0x20000000     // Internal
#define OFN_FILTERDOWN                 0x10000000     // Internal
// CD_WX86APP is                       0x08000000     // Internal

//
//  Used with OFN_COMBODOWN.
//
#define MYCBN_DRAW                     0x8000
#define MYCBN_LIST                     0x8001
#define MYCBN_REPAINT                  0x8002
#define MYCBN_CHANGEDIR                0x8003

#define OFN_OFFSETTAG                  0x0001

#define FILEPROP (LPCTSTR)             0xA000L

#define CHANGEDIR_FAILED               -1

#define ADDDISK_NOCHANGE               -1
#define ADDDISK_INVALIDPARMS           -2
#define ADDDISK_MAXNUMDISKS            -3
#define ADDDISK_NETFORMATFAILED        -4
#define ADDDISK_ALLOCFAILED            -5

#define ATTR_READONLY                  0x0000001      // GetFileAttributes flag

#define mskFile                        0x0000         // List files
#define mskDirectory                   0x0010         // List directories
#define mskUNCName                     0x0020         // Note UNC directory

#define mskDrives                      0xC000         // List drives ONLY

#define rgbSolidGreen                  0x0000FF00
#define rgbSolidBlue                   0x00FF0000

#define dxSpace                        4

#define cbCaption                      64

#define SUCCESS                        0x0
#define FAILURE                        0x1

#define DBL_BSLASH(sz) \
   (*(TCHAR *)(sz)       == CHAR_BSLASH) && \
   (*(TCHAR *)((sz) + 1) == CHAR_BSLASH)

#ifdef UNICODE
  #define ISBACKSLASH(szPath, nOffset) (szPath[nOffset] == CHAR_BSLASH)
  #define ISBACKSLASH_P(szPath, pPos)  (*pPos == CHAR_BSLASH)
#else
  #define ISBACKSLASH(szPath, nOffset) (IsBackSlash(szPath, szPath + nOffset))
  #define ISBACKSLASH_P(szPath, pPos)  (IsBackSlash(szPath, pPos))
#endif


//
//  Constant used in FILEOPENINFO to specify the version of
//  the structure passed by the application.//
#define OPENFILEVERSION_NT4                   0x0004
#define OPENFILEVERSION_NT5                   0x0005
#define OPENFILEVERSION                       0x0005  //Current Version if NT5




//
//  Typedef Declarations.
//

typedef struct _OFN_ANSI_STRING {
    ULONG Length;
    ULONG MaximumLength;
    LPSTR Buffer;
} OFN_ANSI_STRING;

typedef OFN_ANSI_STRING *POFN_ANSI_STRING;

typedef struct _OFN_UNICODE_STRING {
    ULONG  Length;
    ULONG  MaximumLength;
    LPWSTR Buffer;
} OFN_UNICODE_STRING;

typedef OFN_UNICODE_STRING *POFN_UNICODE_STRING;

typedef struct {
    UINT                ApiType;
    LPOPENFILENAME      pOFN;
    TCHAR               szCurDir[MAX_FULLPATHNAME + 1];
    TCHAR               szPath[MAX_FULLPATHNAME];
    TCHAR               szSpecCur[MAX_FULLPATHNAME];
    TCHAR               szLastFilter[MAX_FULLPATHNAME + 1];
    DWORD               idirSub;
	//Version of structure.
    DWORD               iVersion;
#ifdef UNICODE
    LPOPENFILENAMEA     pOFNA;
    POFN_UNICODE_STRING pusCustomFilter;
    POFN_ANSI_STRING    pasCustomFilter;
    BOOL                bUseNewDialog;
#endif

} OPENFILEINFO;

typedef OPENFILEINFO * POPENFILEINFO;
typedef OPENFILEINFO * LPOPENFILEINFO;



//
//  Function Prototypes.
//

BOOL NewGetOpenFileName(LPOPENFILEINFO lpOFI);

BOOL NewGetSaveFileName(LPOPENFILEINFO lpOFI);

STDAPI_(void) GetAppOpenDir(LPCTSTR pszDir,LPTSTR pszOut, LPITEMIDLIST *ppidl);
STDAPI_(LPITEMIDLIST) CreateMyDocsIDList(void);
STDAPI_(BOOL) FoundFilterMatch(LPCTSTR pszIn, BOOL bLFN);

#ifdef UNICODE
  VOID
  ThunkOpenFileNameA2WDelayed(
      POPENFILEINFO pOFI);

  BOOL
  ThunkOpenFileNameA2W(
      POPENFILEINFO pOFI);

  BOOL
  ThunkOpenFileNameW2A(
      POPENFILEINFO pOFI);
#else
  VOID
  EliminateString(
      LPSTR lpStr,
      int nLen);

  BOOL
  IsBackSlash(
      LPSTR lpStart,
      LPSTR lpChar);
#endif


#ifdef __cplusplus
};  // extern "C"
#endif
