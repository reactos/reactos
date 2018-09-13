#ifndef NO_MMSYSTEMH
#define NO_MMSYSTEMH
#endif

#include <windows.h> /* added -sdj*/
/*---------------------------------------------------------------------------*/
/* Option Declarations - FileOpen()                                          */
/*---------------------------------------------------------------------------*/

#define FO_GETFILE                  0x0000
#define FO_PUTFILE                  0x0001
#define FO_FILEEXIST                0x0002
#define FO_FORCEEXTENT              0x0004   /* mbbx 2.00: no forced extents */
#define FO_BATCHMODE                0x1000
#define FO_NONDOSFILE               0x2000
#define FO_REMOTEFILE               0x4000
#define FO_GETFILENAME2             0x8000


/*---------------------------------------------------------------------------*/
/* Constant Declarations                                                     */
/*---------------------------------------------------------------------------*/

#define FO_MAXPATHLENGTH            513
#define FO_MAXFILELENGTH            16
#define FO_MAXEXTLENGTH             16


/*---------------------------------------------------------------------------*/
/* Type Declarations                                                         */
/*---------------------------------------------------------------------------*/

#define FILEOPENDATA                struct tagFileOpenData

struct tagFileOpenData
{
   BYTE     file[FO_MAXPATHLENGTH];
   BYTE     file1[FO_MAXFILELENGTH];
   BYTE     file2[FO_MAXPATHLENGTH];         /* must be able to hold PATH */
   BYTE     extent[FO_MAXEXTLENGTH];
   BYTE     title[32];                       /* mbbx 1.10: CUA... */
   WORD     wResID;
   FARPROC  lpFilter;                        /* mbbx 2.00: new FO hook */
   WORD     wMode;
   INT      nType;                           /* mbbx 1.10: CUA */
};


/*---------------------------------------------------------------------------*/
/* Variable Declarations                                                     */
/*---------------------------------------------------------------------------*/

FILEOPENDATA   *pFOData;


/*---------------------------------------------------------------------------*/
/* Rescource ID Declarations                                                 */
/*---------------------------------------------------------------------------*/

#define FO_STR_ERRCAPTION           0x0300
#define FO_STR_WARNCAPTION          0x0301
#define FO_STR_BADFILENAME          0x0302
#define FO_STR_FILENOTFOUND         0x0303
#define FO_STR_REPLACEFILE          0x0304

#define FO_LBFILE                   0x0000         /* list FILES only */
#define FO_LBDIR                    0xC010         /* list DIRECTORIES only */

#define FO_DBFILEOPEN               601
#define FO_DBFILETYPE               602
#define FO_DBSNDTEXT                603
#define FO_DBSNDFILE                604
#define FO_DBCOMPILE                605      /* mbbx 2.00: auto save DCP */

#define FO_DBFILESAVE               611
#define FO_DBFILEAPPEND             612
#define FO_DBRCVTEXT                613
#define FO_DBRCVFILE                614

#define FO_IDTITLE                  621
#define FO_IDPROMPT                 631
#define FO_IDFILENAME               632
#define FO_IDPROMPT2                633
#define FO_IDFILENAME2              634
#define FO_IDPATH                   635
#define FO_IDFILELIST               641
#define FO_IDDIRLIST                642

#define FO_IDSETTINGS               651
#define FO_IDSCRIPT                 652
#define FO_IDMEMO                   653

#define FO_IDAPPEND                 691
#define FO_IDCTRL                   692
#define FO_IDTABLE                  693
#define FO_IDSNDLF                  694
#define FO_IDSNDNOLF                695


/*---------------------------------------------------------------------------*/
/* Function Prototypes                                                       */
/*---------------------------------------------------------------------------*/

BOOL FileOpen(BYTE *, BYTE *, BYTE *, BYTE *, BYTE *, WORD, FARPROC, WORD);   /* mbbx 2.00: new FO hook scheme... */
/* BOOL FileOpen(HWND, HANDLE, BYTE *, BYTE *, BYTE *, BYTE *, BYTE *, WORD, WORD); */

BOOL  APIENTRY dbFileOpen(HWND, UINT, WPARAM, LONG);
BOOL NEAR FO_SaveFileName(HWND);             /* mbbx 2.00 */
VOID NEAR FO_SetListItem(HWND, WORD, BOOL);
VOID NEAR FO_NewFilePath(HWND, WORD, BYTE *, BYTE *);
BOOL NEAR FO_AddFileType(BYTE *, BYTE *);
VOID NEAR FO_StripFileType(BYTE *);
BOOL NEAR FO_IsLegalDOSFN(BYTE *);           /* mbbx 2.00: no forced extents... */
BOOL NEAR FO_IsLegalDOSCH(BYTE);
BOOL NEAR FO_IsLegalFN(BYTE *);
INT NEAR FO_ErrProc(WORD, WORD,HWND);
BOOL NEAR FO_SetCtrlFocus(HWND, HWND);

BOOL setPath(BYTE *, BOOL, BYTE *);
BOOL setFilePath(BYTE *);
VOID forceExtension(BYTE *, BYTE *, BOOL);   /* mbbx 2.00: no forced extents... */
BOOL getFileType(BYTE *, BYTE *);


/*---------------------------------------------------------------------------*/

/* mbbx 2.00: new FO hook scheme... */

#ifdef ORGCODE
BOOL  APIENTRY FO_FileOpenType (HWND, WORD, WORD, LONG);   /* mbbx 2.00: CUA */
VOID  APIENTRY FO_SaveSelection(HWND, WORD, WORD, LONG);
VOID  APIENTRY FO_SendTextFile (HWND, WORD, WORD, LONG);
VOID  APIENTRY FO_RcvTextFile  (HWND, WORD, WORD, LONG);
VOID  APIENTRY FO_ScriptCompile(HWND, WORD, WORD, LONG);   /* mbbx 2.00: auto save DCP */
#else
BOOL  APIENTRY FO_FileOpenType (HWND, UINT, WPARAM, LONG);   /* mbbx 2.00: CUA */
VOID  APIENTRY FO_SaveSelection(HWND, WORD, WPARAM, LONG);
VOID  APIENTRY FO_SendTextFile (HWND, WORD, WPARAM, LONG);
VOID  APIENTRY FO_RcvTextFile  (HWND, WORD, WPARAM, LONG);
VOID  APIENTRY FO_ScriptCompile(HWND, WORD, WPARAM, LONG);   /* mbbx 2.00: auto save DCP */
#endif

