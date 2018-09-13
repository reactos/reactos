#ifndef __PCONTROL_H__
#define __PCONTROL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "fvscodes.h"    // FVS_xxxxxx (font validation status) codes & macros.
                         // \nt\private\windows\shell\control\t1instal\fvscodes.h

//
//  Global definitions
//
//
// Note for file macros below.
// The LZxxx functions are no longer used in the macro substitutions.
// The need to use LZxxx implementations is a function of the file type, not
// the target platform.  Wherever LZxxx functions are required, they
// are now used explicitely.
//

#ifdef WINNT

#define FOPEN( sz, lpsz )        MyOpenFile( sz, lpsz, OF_READ )
#define FREAD( fh, buf, len )    MyAnsiReadFile( fh, CP_ACP, buf, len )
#define FWRITE( fh, buf, len )   MyAnsiWriteFile( fh, CP_ACP, buf, len )
#define FREADBYTES(fh,buf,len)   MyByteReadFile( fh, buf, len )
#define FWRITEBYTES(fh,buf,len)  MyByteWriteFile( fh, buf, len )

#define FSEEK( fh, off, i )      MyFileSeek( fh, (DWORD) off, i )
#define FCLOSE( fh )             MyCloseFile( fh )
#define FCREATE( sz )            MyOpenFile( sz, NULL, OF_READWRITE | OF_CREATE )

#else

#define FOPEN( sz )              _lopen( sz, OF_READ )
#define FCLOSE( fh )             _lclose( fh )
#define FREAD( fh, buf, len )    _lread( fh, buf, len )
#define FREADBYTES(fh, buf, len) _lread( fh, buf, len )
#define FSEEK( fh, off, i )      _llseek( fh, (DWORD) off, i )

#define FCREATE( sz )            _lcreat( sz, 0 )
#define FWRITE( fh, buf, len )   _lwrite( fh, buf, len )
#define FWRITEBYTES(fh, buf, len) _lwrite( fh, buf, len )

#endif  //  WINNT



//
//  Global variables
//

extern  FullPathName_t e_szDirOfSrc;
extern  UINT s_wBrowseDoneMsg;


//
//  Flags for ReadLine
//

#define  RL_MORE_MEM       -1
#define  RL_SECTION_END    -2

#define  SEEK_BEG           0
#define  SEEK_CUR           1
#define  SEEK_END           2

#define  PRN               148
#define  MON_OF_YR         276
#define  MOUSE           MON_OF_YR+48
#define  INSTALL         MOUSE+16


//
//  Font file types used in Fonts applet - installation
//

#define NOT_TT_OR_T1        0       //  Neither TrueType or Type 1 font (FALSE)
#define TRUETYPE_FONT       1       //  This is a TrueType font (TRUE)
#define TYPE1_FONT          2       //  This is an Adobe Type1 font
#define TYPE1_FONT_NC       3       //  Type1 font that cannot be converted to TT
#define OPENTYPE_FONT       4       //  Font is OpenType.

//
//  Font file types used in Fonts applet - Main dlg "Installed Fonts" lbox
//

#define IF_OTHER            0       //  TrueType or Bitmap 1 font (FALSE)
#define IF_TYPE1            1       //  Adobe Type1 font
#define IF_TYPE1_TT         2       //  Matching TT font for Adobe Type1 font

#define T1_MAX_DATA     (2 * PATHMAX + 6)

//
//  Return codes from InstallT1Font routine
//

#define TYPE1_INSTALL_IDOK       IDOK        //  User pressed OK from MessageBox error
#define TYPE1_INSTALL_IDYES      IDYES       //  Same as IDOK
#define TYPE1_INSTALL_IDNO       IDNO        //  Font not installed - user pressed NO
#define TYPE1_INSTALL_IDCANCEL   IDCANCEL    //  Entire installation cancelled
#define TYPE1_INSTALL_PS_ONLY     10         //  Only the PS Font installed.
#define TYPE1_INSTALL_PS_AND_MTT  11         //  PostScript Font installed and matching
                                             //   TT font already installed.
#define TYPE1_INSTALL_TT_AND_PS   12         //  PS Font installed and converted to TT.
#define TYPE1_INSTALL_TT_ONLY     13         //  PS Font converted to TT only.
#define TYPE1_INSTALL_TT_AND_MPS  14         //  PS Font converted to TT and matching
                                             //   PS font already installed.
//
//  Global functions
//

//
//  append.cpp
//

BOOL FAR PASCAL fnAppendSplitFiles( LPTSTR FAR *, LPTSTR, int );

//
//  instfls.c
//

typedef int (FAR PASCAL *INSTALL_PROC)(HWND hDlg, WORD wMsg, int i,
             LPTSTR FAR *pszFiles, LPTSTR lpszDir );

#define IFF_CHECKINI  0x0001
#define IFF_SRCANDDST 0x0002

#define IF_ALREADY_INSTALLED    1
#define IF_ALREADY_RUNNING      2
#define IF_JUST_INSTALLED       3

PTSTR  FAR PASCAL CopyString( LPTSTR szStr );
PTSTR  FAR PASCAL MyLoadString( WORD wId );
LPTSTR FAR PASCAL CpyToChr( LPTSTR pDest, LPTSTR pSrc, TCHAR cChr, int iMax );

VOID FAR PASCAL GetDiskAndFile( LPTSTR pszInf,
                                short /* int */ FAR *nDsk,
                                LPTSTR pszDriver,
                                WORD wSize );

DWORD FAR PASCAL InstallFiles( HWND hwnd, LPTSTR FAR *pszFiles, int nCount,
                               INSTALL_PROC lpfnNewFile, WORD wFlags );

//
//  PFONT.CPP
//

extern VOID NEAR PASCAL vConvertExtension( LPTSTR pszFile,  LPTSTR szExt );

class CFontManager;
extern BOOL FAR PASCAL bCPAddFonts( HWND ma );

#define CPDI_CANCEL  -1
#define CPDI_FAIL     0
#define CPDI_SUCCESS  1

extern int FAR PASCAL CPDropInstall( HWND hwndParent,
                                     LPTSTR szFile,
                                     DWORD  dwEffect,
                                     LPTSTR lpszDestName = NULL,
                                     int    iCount = 0 );

extern VOID FAR  PASCAL vCPDeleteFromSharedDir( LPTSTR pszFileOnly );
extern VOID FAR  PASCAL vCPFilesToDescs( );
extern BOOL NEAR PASCAL bUniqueOnSharedDir( LPTSTR lpszDst,  LPTSTR lpszSrc );
extern VOID NEAR PASCAL vHashToNulls( LPTSTR lpStr );
extern BOOL FAR  PASCAL bUniqueFilename (LPTSTR lpszDst, LPTSTR lpszSrc, LPTSTR lpszDir);

//
//  cpsetup.c
//

typedef WORD (*LPSETUPINFPROC)( LPTSTR, LPVOID );
extern DWORD ReadSetupInfSection( LPTSTR pszInfPath,
                                  LPTSTR pszSection,
                                  LPTSTR *ppszSectionItems );


extern WORD ReadSetupInfCB( LPTSTR pszInfPath,
                            LPTSTR pszSection,
                            LPSETUPINFPROC,
                            LPVOID pData);

extern int    FAR PASCAL ReadSetupInfIntoLBs( HWND hLBName,
                                              HWND hLBDBase,
                                              WORD wAddMsg,
                                              LPTSTR pszSection,
                                              WORD (FAR PASCAL *lpfnGetName)(LPTSTR, LPTSTR) );

//
//  PINSTALL.CPP
//

extern BOOL FAR PASCAL bCPInstallFile( HWND hwndParent,
                                       LPTSTR lpDir,
                                       LPTSTR lpFrom,
                                       LPTSTR lpTo );

//
//  PFILES.CPP
//

extern BOOL FAR PASCAL bCPValidFontFile( LPTSTR lpszFile,
                                         LPTSTR lpszDesc = NULL,
                                         WORD FAR *lpwType = NULL,
                                         BOOL bFOTOK = FALSE,
                                         LPDWORD lpdwStatus = NULL);
//
//  PUTIL.CPP
//

extern BOOL FAR PASCAL bCPSetupFromSource( );
extern BOOL FAR PASCAL bCPIsHelp( WORD message );
extern BOOL FAR PASCAL bCPIsBrowseDone( WORD message );
extern RC   FAR PASCAL rcCPLoadFontList( );
extern VOID FAR PASCAL vCPHelp( HWND );
extern VOID FAR PASCAL vCPStripBlanks( LPTSTR lpszString );
extern VOID FAR PASCAL vCPUpdateSourceDir( );
extern VOID FAR PASCAL vCPWinIniFontChange( );
extern VOID FAR PASCAL vCPPanelInit( );
extern LPTSTR FAR PASCAL lpCPBackSlashTerm( LPTSTR lpszPath );
extern BOOL bFileIsInFontsDirectory(LPCTSTR lpszPath);
extern int FAR PASCAL DoDialogBoxParam( int nDlg,
                                        HWND hParent,
                                        DLGPROC lpProc,
                                        DWORD dwHelpContext,
                                        LPARAM dwParam);

#ifdef WINNT
extern HANDLE PASCAL wCPOpenFileWithShare( LPTSTR, LPTSTR, WORD );
#else
extern WORD FAR PASCAL wCPOpenFileWithShare( LPTSTR, LPOFSTRUCT, WORD );
#endif  //  WINNT

extern UINT MyAnsiReadFile( HANDLE  hFile,
                            UINT uCodePage,
                            LPVOID  lpUnicode,
                            DWORD  cchUnicode );

extern UINT MyAnsiWriteFile( HANDLE  hFile,
                             UINT uCodePage,
                             LPVOID lpUnicode,
                             DWORD cchUnicode );

extern UINT   MyByteReadFile( HANDLE  hFile, LPVOID lpBuffer, DWORD nBytes );
extern UINT   MyByteWriteFile( HANDLE hFile, LPVOID lpBuffer, DWORD nBytes );
extern BOOL   MyCloseFile( HANDLE  hFile );
extern LONG   MyFileSeek( HANDLE hFile, LONG lDistanceToMove, DWORD dwMoveMethod );
extern HANDLE MyOpenFile( LPTSTR lpszFile, TCHAR * lpszPath, DWORD fuMode );

VOID  CentreWindow( HWND hwnd );


typedef struct _StringObject {
   HANDLE   h;
   DWORD    dwLen;
} StringObject;

extern BOOL FAR PASCAL AddStringToObject( StringObject&, LPTSTR, WORD );

#define ASO_GLOBAL  0x0001
#define ASO_FIXED   0x0002
#define ASO_EXACT   0x0004
#define ASO_COMPACT 0x0008


#ifdef ROM
extern HANDLE FAR PASCAL IsROMModule( LPTSTR lpName, BOOL fSelector );
#endif


LPVOID AllocMem( DWORD cb );
BOOL   FreeMem( LPVOID pMem, DWORD  cb );
LPVOID ReallocMem( LPVOID lpOldMem, DWORD cbOld, DWORD cbNew );
LPTSTR AllocStr( LPTSTR lpStr );
BOOL   FreeStr( LPTSTR lpStr );
BOOL   ReallocStr( LPTSTR *plpStr, LPTSTR lpStr );


/* t1.cpp */

BOOL CheckT1Install( LPTSTR pszDesc, LPTSTR pszData );
BOOL DeleteT1Install( HWND hwndParent, LPTSTR pszDesc, BOOL bDeleteFiles );
BOOL EnumType1Fonts( HWND hLBox );
BOOL GetT1Install( LPTSTR pszDesc, LPTSTR pszPfmFile, LPTSTR pszPfbFile );
int  InstallT1Font( HWND hwndParent, BOOL bCopyTTFile, BOOL bCopyType1Files,
                    BOOL bInSharedDir, LPTSTR szPfmName, LPTSTR szDesc );

HWND InitProgress( HWND hwnd );
BOOL InstallCancelled(void);
void InitPSInstall( );
BOOL IsPSFont( LPTSTR lpszKey, LPTSTR lpszDesc, LPTSTR lpszPfm, LPTSTR lpszPfb,
               BOOL *pbCreatedPFM, LPDWORD lpdwStatus = NULL );

BOOL OkToConvertType1ToTrueType(LPCTSTR pszFontDesc, LPCTSTR pszPFB, HWND hwndParent);

BOOL ExtractT1Files( LPTSTR pszMulti, LPTSTR pszPfmFile, LPTSTR pszPfbFile );
void Progress2( int PercentDone, LPTSTR szDesc );
void RemoveDecoration( LPTSTR pszDesc, BOOL bDeleteTrailingSpace );
void ResetProgress( );
void TermProgress( );
void TermPSInstall( );
void UpdateProgress( int iTotalCount, int iFontInstalling, int iProgress );

BOOL WriteType1RegistryEntry( HWND hwndParent, LPTSTR szDesc, LPTSTR szPfmName,
                              LPTSTR szPfbName, BOOL bInFontsDir );

HWND GetFirstAncestor( HWND hWnd );

#ifdef WINNT
BOOL BuildType1FontResourceName(LPCTSTR pszPfm, LPCTSTR pszPfb,
                                  LPTSTR pszDest, DWORD cchDest);

#define MAX_TYPE1_FONT_RESOURCE  (MAX_PATH * 2)  // 2 paths + separator.
#endif // WINNT

#ifdef __cplusplus
}
#endif

#endif

/****************************************************************************
 * $lgb$
 * 1.0     7-Mar-94   eric Initial revision.
 * $lge$
 *
 ****************************************************************************/

