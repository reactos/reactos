/*****************************************************************************\
*                                                                             *
* commdlg.h -   Common dialog functions, types, and definitions               *
*                                                                             *
* Version 1.0								      *
*                                                                             *
* NOTE: windows.h must be #included first				      *
*                                                                             *
* Copyright (c) 1992-1994, Microsoft Corp.	All rights reserved.	      *
*                                                                             *
\*****************************************************************************/

#ifndef _INC_COMMDLG
#define _INC_COMMDLG

#ifndef RC_INVOKED
#pragma pack(1)         /* Assume byte packing throughout */
#endif /* !RC_INVOKED */

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

//---------------------------------------------------------------------------
typedef struct tagOFN
{
    DWORD   lStructSize;
    HWND    hwndOwner;
    HINSTANCE hInstance;
    LPCSTR  lpstrFilter;
    LPSTR   lpstrCustomFilter;
    DWORD   nMaxCustFilter;
    DWORD   nFilterIndex;
    LPSTR   lpstrFile;
    DWORD   nMaxFile;
    LPSTR   lpstrFileTitle;
    DWORD   nMaxFileTitle;
    LPCSTR  lpstrInitialDir;
    LPCSTR  lpstrTitle;
    DWORD   Flags;
    UINT    nFileOffset;
    UINT    nFileExtension;
    LPCSTR  lpstrDefExt;
    LPARAM  lCustData;
    UINT    (CALLBACK *lpfnHook)(HWND, UINT, WPARAM, LPARAM);
    LPCSTR  lpTemplateName;
}   OPENFILENAME;
typedef OPENFILENAME FAR* LPOPENFILENAME;

BOOL    WINAPI GetOpenFileName(OPENFILENAME FAR*);
BOOL    WINAPI GetSaveFileName(OPENFILENAME FAR*);
int     WINAPI GetFileTitle(LPCSTR, LPSTR, UINT);

#define OFN_READONLY                 0x00000001
#define OFN_OVERWRITEPROMPT          0x00000002
#define OFN_HIDEREADONLY             0x00000004
#define OFN_NOCHANGEDIR              0x00000008
#define OFN_SHOWHELP                 0x00000010
#define OFN_ENABLEHOOK               0x00000020
#define OFN_ENABLETEMPLATE           0x00000040
#define OFN_ENABLETEMPLATEHANDLE     0x00000080
#define OFN_NOVALIDATE               0x00000100
#define OFN_ALLOWMULTISELECT         0x00000200
#define OFN_EXTENSIONDIFFERENT       0x00000400
#define OFN_PATHMUSTEXIST            0x00000800
#define OFN_FILEMUSTEXIST            0x00001000
#define OFN_CREATEPROMPT             0x00002000
#define OFN_SHAREAWARE               0x00004000
#define OFN_NOREADONLYRETURN         0x00008000
#define OFN_NOTESTFILECREATE         0x00010000
#define OFN_NONETWORKBUTTON          0x00020000
#define OFN_NOLONGNAMES              0x00040000		// force no long names for 4.x modules
#if (WINVER >= 0x400)
#define OFN_EXPLORER                 0x00080000
#define OFN_NODEREFERENCELINKS       0x00100000
#define OFN_LONGNAMES		     0x00200000		// force long names for 3.x modules
#endif  /* WINVER >= 0x400 */

/* Return values for the registered message sent to the hook function
 * when a sharing violation occurs.  OFN_SHAREFALLTHROUGH allows the
 * filename to be accepted, OFN_SHARENOWARN rejects the name but puts
 * up no warning (returned when the app has already put up a warning
 * message), and OFN_SHAREWARN puts up the default warning message
 * for sharing violations.
 *
 * Note:  Undefined return values map to OFN_SHAREWARN, but are
 *        reserved for future use.
 */

#define OFN_SHAREFALLTHROUGH     2
#define OFN_SHARENOWARN          1
#define OFN_SHAREWARN            0

//---------------------------------------------------------------------------
typedef struct tagCHOOSECOLOR
{
    DWORD   lStructSize;
    HWND    hwndOwner;
    HINSTANCE hInstance;
    COLORREF rgbResult;
    COLORREF FAR* lpCustColors;
    DWORD   Flags;
    LPARAM  lCustData;
    UINT    (CALLBACK* lpfnHook)(HWND, UINT, WPARAM, LPARAM);
    LPCSTR  lpTemplateName;
} CHOOSECOLOR;
typedef CHOOSECOLOR FAR *LPCHOOSECOLOR;

BOOL    WINAPI ChooseColor(CHOOSECOLOR FAR*);

#define CC_RGBINIT               0x00000001
#define CC_FULLOPEN              0x00000002
#define CC_PREVENTFULLOPEN       0x00000004
#define CC_SHOWHELP              0x00000008
#define CC_ENABLEHOOK            0x00000010
#define CC_ENABLETEMPLATE        0x00000020
#define CC_ENABLETEMPLATEHANDLE  0x00000040
//// reserved                    0x?0000000

//---------------------------------------------------------------------------
typedef struct tagFINDREPLACE
{
    DWORD    lStructSize;            /* size of this struct 0x20 */
    HWND     hwndOwner;              /* handle to owner's window */
    HINSTANCE hInstance;             /* instance handle of.EXE that
                                      * contains cust. dlg. template
                                      */
    DWORD    Flags;                  /* one or more of the FR_?? */
    LPSTR    lpstrFindWhat;          /* ptr. to search string    */
    LPSTR    lpstrReplaceWith;       /* ptr. to replace string   */
    UINT     wFindWhatLen;           /* size of find buffer      */
    UINT     wReplaceWithLen;        /* size of replace buffer   */
    LPARAM   lCustData;              /* data passed to hook fn.  */
    UINT    (CALLBACK* lpfnHook)(HWND, UINT, WPARAM, LPARAM);
                                     /* ptr. to hook fn. or NULL */
    LPCSTR   lpTemplateName;         /* custom template name     */
} FINDREPLACE;
typedef FINDREPLACE FAR *LPFINDREPLACE;

#define FR_DOWN                         0x00000001
#define FR_WHOLEWORD                    0x00000002
#define FR_MATCHCASE                    0x00000004
#define FR_FINDNEXT                     0x00000008
#define FR_REPLACE                      0x00000010
#define FR_REPLACEALL                   0x00000020
#define FR_DIALOGTERM                   0x00000040
#define FR_SHOWHELP                     0x00000080
#define FR_ENABLEHOOK                   0x00000100
#define FR_ENABLETEMPLATE               0x00000200
#define FR_NOUPDOWN                     0x00000400
#define FR_NOMATCHCASE                  0x00000800
#define FR_NOWHOLEWORD                  0x00001000
#define FR_ENABLETEMPLATEHANDLE         0x00002000
#define FR_HIDEUPDOWN                   0x00004000
#define FR_HIDEMATCHCASE                0x00008000
#define FR_HIDEWHOLEWORD                0x00010000
/// 0x?0000000 is reserved for internal use


HWND    WINAPI FindText(FINDREPLACE FAR*);
HWND    WINAPI ReplaceText(FINDREPLACE FAR*);

//---------------------------------------------------------------------------
typedef struct tagCHOOSEFONT
{
    DWORD           lStructSize;        /* */
    HWND            hwndOwner;          /* caller's window handle   */
    HDC             hDC;                /* printer DC/IC or NULL    */
    LOGFONT FAR*    lpLogFont;          /* ptr. to a LOGFONT struct */
    int             iPointSize;         /* 10 * size in points of selected font */
    DWORD           Flags;              /* enum. type flags         */
    COLORREF        rgbColors;          /* returned text color      */
    LPARAM          lCustData;          /* data passed to hook fn.  */
    UINT (CALLBACK* lpfnHook)(HWND, UINT, WPARAM, LPARAM);
                                        /* ptr. to hook function    */
    LPCSTR          lpTemplateName;     /* custom template name     */
    HINSTANCE       hInstance;          /* instance handle of.EXE that
                                         * contains cust. dlg. template
                                         */
    LPSTR           lpszStyle;          /* return the style field here
                                         * must be LF_FACESIZE or bigger */
    UINT            nFontType;          /* same value reported to the EnumFonts
                                         * call back with the extra FONTTYPE_
                                         * bits added */
    int             nSizeMin;           /* minimum pt size allowed & */
    int             nSizeMax;           /* max pt size allowed if    */
                                        /* CF_LIMITSIZE is used      */
} CHOOSEFONT;
typedef CHOOSEFONT FAR *LPCHOOSEFONT;

BOOL WINAPI ChooseFont(CHOOSEFONT FAR*);

#define CF_SCREENFONTS               0x00000001
#define CF_PRINTERFONTS              0x00000002
#define CF_BOTH                      (CF_SCREENFONTS | CF_PRINTERFONTS)
#define CF_SHOWHELP                  0x00000004L
#define CF_ENABLEHOOK                0x00000008L
#define CF_ENABLETEMPLATE            0x00000010L
#define CF_ENABLETEMPLATEHANDLE      0x00000020L
#define CF_INITTOLOGFONTSTRUCT       0x00000040L
#define CF_USESTYLE                  0x00000080L
#define CF_EFFECTS                   0x00000100L
#define CF_APPLY                     0x00000200L
#define CF_ANSIONLY                  0x00000400L
#define CF_NOVECTORFONTS             0x00000800L
#define CF_NOOEMFONTS                CF_NOVECTORFONTS
#define CF_NOSIMULATIONS             0x00001000L
#define CF_LIMITSIZE                 0x00002000L
#define CF_FIXEDPITCHONLY            0x00004000L
#define CF_WYSIWYG                   0x00008000L /* must also have CF_SCREENFONTS & CF_PRINTERFONTS */
#define CF_FORCEFONTEXIST            0x00010000L
#define CF_SCALABLEONLY              0x00020000L
#define CF_TTONLY                    0x00040000L
#define CF_NOFACESEL                 0x00080000L
#define CF_NOSTYLESEL                0x00100000L
#define CF_NOSIZESEL                 0x00200000L
/////  reserved for internal use     0x?0000000L

/* these are extra nFontType bits that are added to what is returned to the
 * EnumFonts callback routine */

#define SIMULATED_FONTTYPE      0x8000
#define PRINTER_FONTTYPE        0x4000
#define SCREEN_FONTTYPE         0x2000
#define BOLD_FONTTYPE           0x0100
#define ITALIC_FONTTYPE         0x0200
#define REGULAR_FONTTYPE        0x0400

#define WM_CHOOSEFONT_GETLOGFONT        (WM_USER + 1)


/* strings used to obtain unique window message for communication
 * between dialog and caller
 */
#define LBSELCHSTRING  "commdlg_LBSelChangedNotify"
#define SHAREVISTRING  "commdlg_ShareViolation"
#define FILEOKSTRING   "commdlg_FileNameOK"
#define COLOROKSTRING  "commdlg_ColorOK"
#define SETRGBSTRING   "commdlg_SetRGBColor"
#define FINDMSGSTRING  "commdlg_FindReplace"
#define HELPMSGSTRING  "commdlg_help"

/* HIWORD values for lParam of commdlg_LBSelChangeNotify message */
#define CD_LBSELNOITEMS -1
#define CD_LBSELCHANGE   0
#define CD_LBSELSUB      1
#define CD_LBSELADD      2

//---------------------------------------------------------------------------
typedef struct tagPD
{
    DWORD   lStructSize;
    HWND    hwndOwner;
    HGLOBAL hDevMode;
    HGLOBAL hDevNames;
    HDC     hDC;
    DWORD   Flags;
    UINT    nFromPage;
    UINT    nToPage;
    UINT    nMinPage;
    UINT    nMaxPage;
    UINT    nCopies;
    HINSTANCE hInstance;
    LPARAM  lCustData;
    UINT    (CALLBACK* lpfnPrintHook)(HWND, UINT, WPARAM, LPARAM);
    UINT    (CALLBACK* lpfnSetupHook)(HWND, UINT, WPARAM, LPARAM);
    LPCSTR  lpPrintTemplateName;
    LPCSTR  lpSetupTemplateName;
    HGLOBAL hPrintTemplate;
    HGLOBAL hSetupTemplate;
} PRINTDLG;
typedef PRINTDLG  FAR* LPPRINTDLG;

BOOL    WINAPI PrintDlg(PRINTDLG FAR*);

#define PD_ALLPAGES                  0x00000000
#define PD_SELECTION                 0x00000001
#define PD_PAGENUMS                  0x00000002
#define PD_NOSELECTION               0x00000004
#define PD_NOPAGENUMS                0x00000008
#define PD_COLLATE                   0x00000010
#define PD_PRINTTOFILE               0x00000020
#define PD_PRINTSETUP                0x00000040
#define PD_NOWARNING                 0x00000080
#define PD_RETURNDC                  0x00000100
#define PD_RETURNIC                  0x00000200
#define PD_RETURNDEFAULT             0x00000400
#define PD_SHOWHELP                  0x00000800
#define PD_ENABLEPRINTHOOK           0x00001000
#define PD_ENABLESETUPHOOK           0x00002000
#define PD_ENABLEPRINTTEMPLATE       0x00004000
#define PD_ENABLESETUPTEMPLATE       0x00008000
#define PD_ENABLEPRINTTEMPLATEHANDLE 0x00010000
#define PD_ENABLESETUPTEMPLATEHANDLE 0x00020000
#define PD_USEDEVMODECOPIES          0x00040000
#define PD_DISABLEPRINTTOFILE        0x00080000
#define PD_HIDEPRINTTOFILE           0x00100000
#define PD_NONETWORKBUTTON           0x00200000
////  reserved                       0x?0000000

typedef struct tagDEVNAMES
{
    UINT wDriverOffset;
    UINT wDeviceOffset;
    UINT wOutputOffset;
    UINT wDefault;
} DEVNAMES;
typedef DEVNAMES FAR* LPDEVNAMES;

#define DN_DEFAULTPRN      0x0001

DWORD   WINAPI CommDlgExtendedError(void);

//---------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif  /* __cplusplus */

#ifndef RC_INVOKED
#pragma pack()
#endif  /* !RC_INVOKED */

#endif  /* !_INC_COMMDLG */
