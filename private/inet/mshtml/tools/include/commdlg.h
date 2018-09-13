/************************************************************************
*                                                                       *
*   commdlg.h -- This module defines the 32-Bit Common Dialog APIs      *
*                                                                       *
*   Copyright (c) 1992-1996, Microsoft Corp. All rights reserved.       *
*                                                                       *
************************************************************************/
#ifndef _INC_COMMDLG
#define _INC_COMMDLG


#include <pshpack1.h>         /* Assume byte packing throughout */

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

#if(WINVER >= 0x0400)
#ifdef __cplusplus
#define SNDMSG ::SendMessage
#else   /* __cplusplus */
#define SNDMSG SendMessage
#endif  /* __cplusplus */
#endif /* WINVER >= 0x0400 */

typedef UINT (APIENTRY *LPOFNHOOKPROC) (HWND, UINT, WPARAM, LPARAM);

typedef struct tagOFNA {
   DWORD        lStructSize;
   HWND         hwndOwner;
   HINSTANCE    hInstance;
   LPCSTR       lpstrFilter;
   LPSTR        lpstrCustomFilter;
   DWORD        nMaxCustFilter;
   DWORD        nFilterIndex;
   LPSTR        lpstrFile;
   DWORD        nMaxFile;
   LPSTR        lpstrFileTitle;
   DWORD        nMaxFileTitle;
   LPCSTR       lpstrInitialDir;
   LPCSTR       lpstrTitle;
   DWORD        Flags;
   WORD         nFileOffset;
   WORD         nFileExtension;
   LPCSTR       lpstrDefExt;
   LPARAM       lCustData;
   LPOFNHOOKPROC lpfnHook;
   LPCSTR       lpTemplateName;
} OPENFILENAMEA, *LPOPENFILENAMEA;
typedef struct tagOFNW {
   DWORD        lStructSize;
   HWND         hwndOwner;
   HINSTANCE    hInstance;
   LPCWSTR      lpstrFilter;
   LPWSTR       lpstrCustomFilter;
   DWORD        nMaxCustFilter;
   DWORD        nFilterIndex;
   LPWSTR       lpstrFile;
   DWORD        nMaxFile;
   LPWSTR       lpstrFileTitle;
   DWORD        nMaxFileTitle;
   LPCWSTR      lpstrInitialDir;
   LPCWSTR      lpstrTitle;
   DWORD        Flags;
   WORD         nFileOffset;
   WORD         nFileExtension;
   LPCWSTR      lpstrDefExt;
   LPARAM       lCustData;
   LPOFNHOOKPROC lpfnHook;
   LPCWSTR      lpTemplateName;
} OPENFILENAMEW, *LPOPENFILENAMEW;
#ifdef UNICODE
typedef OPENFILENAMEW OPENFILENAME;
typedef LPOPENFILENAMEW LPOPENFILENAME;
#else
typedef OPENFILENAMEA OPENFILENAME;
typedef LPOPENFILENAMEA LPOPENFILENAME;
#endif // UNICODE

BOOL  APIENTRY     GetOpenFileNameA(LPOPENFILENAMEA);
BOOL  APIENTRY     GetOpenFileNameW(LPOPENFILENAMEW);
#ifdef UNICODE
#define GetOpenFileName  GetOpenFileNameW
#else
#define GetOpenFileName  GetOpenFileNameA
#endif // !UNICODE
BOOL  APIENTRY     GetSaveFileNameA(LPOPENFILENAMEA);
BOOL  APIENTRY     GetSaveFileNameW(LPOPENFILENAMEW);
#ifdef UNICODE
#define GetSaveFileName  GetSaveFileNameW
#else
#define GetSaveFileName  GetSaveFileNameA
#endif // !UNICODE
short APIENTRY     GetFileTitleA(LPCSTR, LPSTR, WORD);
short APIENTRY     GetFileTitleW(LPCWSTR, LPWSTR, WORD);
#ifdef UNICODE
#define GetFileTitle  GetFileTitleW
#else
#define GetFileTitle  GetFileTitleA
#endif // !UNICODE

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
#define OFN_NOLONGNAMES              0x00040000     // force no long names for 4.x modules
#if(WINVER >= 0x0400)
#define OFN_EXPLORER                 0x00080000     // new look commdlg
#define OFN_NODEREFERENCELINKS       0x00100000
#define OFN_LONGNAMES                0x00200000     // force long names for 3.x modules
#endif /* WINVER >= 0x0400 */

// Return values for the registered message sent to the hook function
// when a sharing violation occurs.  OFN_SHAREFALLTHROUGH allows the
// filename to be accepted, OFN_SHARENOWARN rejects the name but puts
// up no warning (returned when the app has already put up a warning
// message), and OFN_SHAREWARN puts up the default warning message
// for sharing violations.
//
// Note:  Undefined return values map to OFN_SHAREWARN, but are
//        reserved for future use.

#define OFN_SHAREFALLTHROUGH     2
#define OFN_SHARENOWARN          1
#define OFN_SHAREWARN            0

typedef UINT (APIENTRY *LPCCHOOKPROC) (HWND, UINT, WPARAM, LPARAM);

#if(WINVER >= 0x0400)
// Structure used for all OpenFileName notifications
typedef struct _OFNOTIFYA
{
        NMHDR           hdr;
        LPOPENFILENAMEA lpOFN;
        LPSTR           pszFile;        // May be NULL
} OFNOTIFYA, FAR *LPOFNOTIFYA;
// Structure used for all OpenFileName notifications
typedef struct _OFNOTIFYW
{
        NMHDR           hdr;
        LPOPENFILENAMEW lpOFN;
        LPWSTR          pszFile;        // May be NULL
} OFNOTIFYW, FAR *LPOFNOTIFYW;
#ifdef UNICODE
typedef OFNOTIFYW OFNOTIFY;
typedef LPOFNOTIFYW LPOFNOTIFY;
#else
typedef OFNOTIFYA OFNOTIFY;
typedef LPOFNOTIFYA LPOFNOTIFY;
#endif // UNICODE

#define CDN_FIRST   (0U-601U)
#define CDN_LAST    (0U-699U)

// Notifications when Open or Save dialog status changes
#define CDN_INITDONE            (CDN_FIRST - 0x0000)
#define CDN_SELCHANGE           (CDN_FIRST - 0x0001)
#define CDN_FOLDERCHANGE        (CDN_FIRST - 0x0002)
#define CDN_SHAREVIOLATION      (CDN_FIRST - 0x0003)
#define CDN_HELP                (CDN_FIRST - 0x0004)
#define CDN_FILEOK              (CDN_FIRST - 0x0005)
#define CDN_TYPECHANGE          (CDN_FIRST - 0x0006)

#define CDM_FIRST       (WM_USER + 100)
#define CDM_LAST        (WM_USER + 200)

// Messages to query information from the Open or Save dialogs

// lParam = pointer to text buffer that gets filled in
// wParam = max number of characters of the text buffer (including NULL)
// return = < 0 if error; number of characters needed (including NULL)
#define CDM_GETSPEC             (CDM_FIRST + 0x0000)
#define CommDlg_OpenSave_GetSpecA(_hdlg, _psz, _cbmax) \
        (int)SNDMSG(_hdlg, CDM_GETSPEC, (WPARAM)_cbmax, (LPARAM)(LPSTR)_psz)
#define CommDlg_OpenSave_GetSpecW(_hdlg, _psz, _cbmax) \
        (int)SNDMSG(_hdlg, CDM_GETSPEC, (WPARAM)_cbmax, (LPARAM)(LPWSTR)_psz)
#ifdef UNICODE
#define CommDlg_OpenSave_GetSpec  CommDlg_OpenSave_GetSpecW
#else
#define CommDlg_OpenSave_GetSpec  CommDlg_OpenSave_GetSpecA
#endif // !UNICODE

// lParam = pointer to text buffer that gets filled in
// wParam = max number of characters of the text buffer (including NULL)
// return = < 0 if error; number of characters needed (including NULL)
#define CDM_GETFILEPATH         (CDM_FIRST + 0x0001)
#define CommDlg_OpenSave_GetFilePathA(_hdlg, _psz, _cbmax) \
        (int)SNDMSG(_hdlg, CDM_GETFILEPATH, (WPARAM)_cbmax, (LPARAM)(LPSTR)_psz)
#define CommDlg_OpenSave_GetFilePathW(_hdlg, _psz, _cbmax) \
        (int)SNDMSG(_hdlg, CDM_GETFILEPATH, (WPARAM)_cbmax, (LPARAM)(LPWSTR)_psz)
#ifdef UNICODE
#define CommDlg_OpenSave_GetFilePath  CommDlg_OpenSave_GetFilePathW
#else
#define CommDlg_OpenSave_GetFilePath  CommDlg_OpenSave_GetFilePathA
#endif // !UNICODE

// lParam = pointer to text buffer that gets filled in
// wParam = max number of characters of the text buffer (including NULL)
// return = < 0 if error; number of characters needed (including NULL)
#define CDM_GETFOLDERPATH       (CDM_FIRST + 0x0002)
#define CommDlg_OpenSave_GetFolderPathA(_hdlg, _psz, _cbmax) \
        (int)SNDMSG(_hdlg, CDM_GETFOLDERPATH, (WPARAM)_cbmax, (LPARAM)(LPSTR)_psz)
#define CommDlg_OpenSave_GetFolderPathW(_hdlg, _psz, _cbmax) \
        (int)SNDMSG(_hdlg, CDM_GETFOLDERPATH, (WPARAM)_cbmax, (LPARAM)(LPWSTR)_psz)
#ifdef UNICODE
#define CommDlg_OpenSave_GetFolderPath  CommDlg_OpenSave_GetFolderPathW
#else
#define CommDlg_OpenSave_GetFolderPath  CommDlg_OpenSave_GetFolderPathA
#endif // !UNICODE

// lParam = pointer to ITEMIDLIST buffer that gets filled in
// wParam = size of the ITEMIDLIST buffer
// return = < 0 if error; length of buffer needed
#define CDM_GETFOLDERIDLIST     (CDM_FIRST + 0x0003)
#define CommDlg_OpenSave_GetFolderIDList(_hdlg, _pidl, _cbmax) \
        (int)SNDMSG(_hdlg, CDM_GETFOLDERIDLIST, (WPARAM)_cbmax, (LPARAM)(LPVOID)_pidl)

// lParam = pointer to a string
// wParam = ID of control to change
// return = not used
#define CDM_SETCONTROLTEXT      (CDM_FIRST + 0x0004)
#define CommDlg_OpenSave_SetControlText(_hdlg, _id, _text) \
        (void)SNDMSG(_hdlg, CDM_SETCONTROLTEXT, (WPARAM)_id, (LPARAM)(LPSTR)_text)

// lParam = not used
// wParam = ID of control to change
// return = not used
#define CDM_HIDECONTROL         (CDM_FIRST + 0x0005)
#define CommDlg_OpenSave_HideControl(_hdlg, _id) \
        (void)SNDMSG(_hdlg, CDM_HIDECONTROL, (WPARAM)_id, 0)

// lParam = pointer to default extension (no dot)
// wParam = not used
// return = not used
#define CDM_SETDEFEXT           (CDM_FIRST + 0x0006)
#define CommDlg_OpenSave_SetDefExt(_hdlg, _pszext) \
        (void)SNDMSG(_hdlg, CDM_SETDEFEXT, 0, (LPARAM)(LPSTR)_pszext)
#endif /* WINVER >= 0x0400 */

typedef struct tagCHOOSECOLORA {
   DWORD        lStructSize;
   HWND         hwndOwner;
   HWND         hInstance;
   COLORREF     rgbResult;
   COLORREF*    lpCustColors;
   DWORD        Flags;
   LPARAM       lCustData;
   LPCCHOOKPROC lpfnHook;
   LPCSTR       lpTemplateName;
} CHOOSECOLORA, *LPCHOOSECOLORA;
typedef struct tagCHOOSECOLORW {
   DWORD        lStructSize;
   HWND         hwndOwner;
   HWND         hInstance;
   COLORREF     rgbResult;
   COLORREF*    lpCustColors;
   DWORD        Flags;
   LPARAM       lCustData;
   LPCCHOOKPROC lpfnHook;
   LPCWSTR      lpTemplateName;
} CHOOSECOLORW, *LPCHOOSECOLORW;
#ifdef UNICODE
typedef CHOOSECOLORW CHOOSECOLOR;
typedef LPCHOOSECOLORW LPCHOOSECOLOR;
#else
typedef CHOOSECOLORA CHOOSECOLOR;
typedef LPCHOOSECOLORA LPCHOOSECOLOR;
#endif // UNICODE

BOOL  APIENTRY ChooseColorA(LPCHOOSECOLORA);
BOOL  APIENTRY ChooseColorW(LPCHOOSECOLORW);
#ifdef UNICODE
#define ChooseColor  ChooseColorW
#else
#define ChooseColor  ChooseColorA
#endif // !UNICODE

#define CC_RGBINIT               0x00000001
#define CC_FULLOPEN              0x00000002
#define CC_PREVENTFULLOPEN       0x00000004
#define CC_SHOWHELP              0x00000008
#define CC_ENABLEHOOK            0x00000010
#define CC_ENABLETEMPLATE        0x00000020
#define CC_ENABLETEMPLATEHANDLE  0x00000040
#if(WINVER >= 0x0400)
#define CC_SOLIDCOLOR            0x00000080
#define CC_ANYCOLOR              0x00000100
#endif /* WINVER >= 0x0400 */

typedef UINT (APIENTRY *LPFRHOOKPROC) (HWND, UINT, WPARAM, LPARAM);

typedef struct tagFINDREPLACEA {
   DWORD        lStructSize;        // size of this struct 0x20
   HWND         hwndOwner;          // handle to owner's window
   HINSTANCE    hInstance;          // instance handle of.EXE that
                                    //   contains cust. dlg. template
   DWORD        Flags;              // one or more of the FR_??
   LPSTR        lpstrFindWhat;      // ptr. to search string
   LPSTR        lpstrReplaceWith;   // ptr. to replace string
   WORD         wFindWhatLen;       // size of find buffer
   WORD         wReplaceWithLen;    // size of replace buffer
   LPARAM       lCustData;          // data passed to hook fn.
   LPFRHOOKPROC lpfnHook;           // ptr. to hook fn. or NULL
   LPCSTR       lpTemplateName;     // custom template name
} FINDREPLACEA, *LPFINDREPLACEA;
typedef struct tagFINDREPLACEW {
   DWORD        lStructSize;        // size of this struct 0x20
   HWND         hwndOwner;          // handle to owner's window
   HINSTANCE    hInstance;          // instance handle of.EXE that
                                    //   contains cust. dlg. template
   DWORD        Flags;              // one or more of the FR_??
   LPWSTR       lpstrFindWhat;      // ptr. to search string
   LPWSTR       lpstrReplaceWith;   // ptr. to replace string
   WORD         wFindWhatLen;       // size of find buffer
   WORD         wReplaceWithLen;    // size of replace buffer
   LPARAM       lCustData;          // data passed to hook fn.
   LPFRHOOKPROC lpfnHook;           // ptr. to hook fn. or NULL
   LPCWSTR      lpTemplateName;     // custom template name
} FINDREPLACEW, *LPFINDREPLACEW;
#ifdef UNICODE
typedef FINDREPLACEW FINDREPLACE;
typedef LPFINDREPLACEW LPFINDREPLACE;
#else
typedef FINDREPLACEA FINDREPLACE;
typedef LPFINDREPLACEA LPFINDREPLACE;
#endif // UNICODE

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

HWND  APIENTRY    FindTextA(LPFINDREPLACEA);
HWND  APIENTRY    FindTextW(LPFINDREPLACEW);
#ifdef UNICODE
#define FindText  FindTextW
#else
#define FindText  FindTextA
#endif // !UNICODE
HWND  APIENTRY    ReplaceTextA(LPFINDREPLACEA);
HWND  APIENTRY    ReplaceTextW(LPFINDREPLACEW);
#ifdef UNICODE
#define ReplaceText  ReplaceTextW
#else
#define ReplaceText  ReplaceTextA
#endif // !UNICODE

typedef UINT (APIENTRY *LPCFHOOKPROC) (HWND, UINT, WPARAM, LPARAM);

typedef struct tagCHOOSEFONTA {
   DWORD           lStructSize;
   HWND            hwndOwner;          // caller's window handle
   HDC             hDC;                // printer DC/IC or NULL
   LPLOGFONTA      lpLogFont;          // ptr. to a LOGFONT struct
   INT             iPointSize;         // 10 * size in points of selected font
   DWORD           Flags;              // enum. type flags
   COLORREF        rgbColors;          // returned text color
   LPARAM          lCustData;          // data passed to hook fn.
   LPCFHOOKPROC    lpfnHook;           // ptr. to hook function
   LPCSTR          lpTemplateName;     // custom template name
   HINSTANCE       hInstance;          // instance handle of.EXE that
                                       //   contains cust. dlg. template
   LPSTR           lpszStyle;          // return the style field here
                                       // must be LF_FACESIZE or bigger
   WORD            nFontType;          // same value reported to the EnumFonts
                                       //   call back with the extra FONTTYPE_
                                       //   bits added
   WORD            ___MISSING_ALIGNMENT__;
   INT             nSizeMin;           // minimum pt size allowed &
   INT             nSizeMax;           // max pt size allowed if
                                       //   CF_LIMITSIZE is used
} CHOOSEFONTA, *LPCHOOSEFONTA;
typedef struct tagCHOOSEFONTW {
   DWORD           lStructSize;
   HWND            hwndOwner;          // caller's window handle
   HDC             hDC;                // printer DC/IC or NULL
   LPLOGFONTW      lpLogFont;          // ptr. to a LOGFONT struct
   INT             iPointSize;         // 10 * size in points of selected font
   DWORD           Flags;              // enum. type flags
   COLORREF        rgbColors;          // returned text color
   LPARAM          lCustData;          // data passed to hook fn.
   LPCFHOOKPROC    lpfnHook;           // ptr. to hook function
   LPCWSTR         lpTemplateName;     // custom template name
   HINSTANCE       hInstance;          // instance handle of.EXE that
                                       //   contains cust. dlg. template
   LPWSTR          lpszStyle;          // return the style field here
                                       // must be LF_FACESIZE or bigger
   WORD            nFontType;          // same value reported to the EnumFonts
                                       //   call back with the extra FONTTYPE_
                                       //   bits added
   WORD            ___MISSING_ALIGNMENT__;
   INT             nSizeMin;           // minimum pt size allowed &
   INT             nSizeMax;           // max pt size allowed if
                                       //   CF_LIMITSIZE is used
} CHOOSEFONTW, *LPCHOOSEFONTW;
#ifdef UNICODE
typedef CHOOSEFONTW CHOOSEFONT;
typedef LPCHOOSEFONTW LPCHOOSEFONT;
#else
typedef CHOOSEFONTA CHOOSEFONT;
typedef LPCHOOSEFONTA LPCHOOSEFONT;
#endif // UNICODE

BOOL APIENTRY ChooseFontA(LPCHOOSEFONTA);
BOOL APIENTRY ChooseFontW(LPCHOOSEFONTW);
#ifdef UNICODE
#define ChooseFont  ChooseFontW
#else
#define ChooseFont  ChooseFontA
#endif // !UNICODE

#define CF_SCREENFONTS             0x00000001
#define CF_PRINTERFONTS            0x00000002
#define CF_BOTH                    (CF_SCREENFONTS | CF_PRINTERFONTS)
#define CF_SHOWHELP                0x00000004L
#define CF_ENABLEHOOK              0x00000008L
#define CF_ENABLETEMPLATE          0x00000010L
#define CF_ENABLETEMPLATEHANDLE    0x00000020L
#define CF_INITTOLOGFONTSTRUCT     0x00000040L
#define CF_USESTYLE                0x00000080L
#define CF_EFFECTS                 0x00000100L
#define CF_APPLY                   0x00000200L
#define CF_ANSIONLY                0x00000400L
#if(WINVER >= 0x0400)
#define CF_SCRIPTSONLY             CF_ANSIONLY
#endif /* WINVER >= 0x0400 */
#define CF_NOVECTORFONTS           0x00000800L
#define CF_NOOEMFONTS              CF_NOVECTORFONTS
#define CF_NOSIMULATIONS           0x00001000L
#define CF_LIMITSIZE               0x00002000L
#define CF_FIXEDPITCHONLY          0x00004000L
#define CF_WYSIWYG                 0x00008000L // must also have CF_SCREENFONTS & CF_PRINTERFONTS
#define CF_FORCEFONTEXIST          0x00010000L
#define CF_SCALABLEONLY            0x00020000L
#define CF_TTONLY                  0x00040000L
#define CF_NOFACESEL               0x00080000L
#define CF_NOSTYLESEL              0x00100000L
#define CF_NOSIZESEL               0x00200000L
#if(WINVER >= 0x0400)
#define CF_SELECTSCRIPT            0x00400000L
#define CF_NOSCRIPTSEL             0x00800000L
#define CF_NOVERTFONTS             0x01000000L
#endif /* WINVER >= 0x0400 */

// these are extra nFontType bits that are added to what is returned to the
// EnumFonts callback routine

#define SIMULATED_FONTTYPE    0x8000
#define PRINTER_FONTTYPE      0x4000
#define SCREEN_FONTTYPE       0x2000
#define BOLD_FONTTYPE         0x0100
#define ITALIC_FONTTYPE       0x0200
#define REGULAR_FONTTYPE      0x0400

#define WM_CHOOSEFONT_GETLOGFONT      (WM_USER + 1)

// strings used to obtain unique window message for communication
// between dialog and caller

#define LBSELCHSTRINGA  "commdlg_LBSelChangedNotify"
#define SHAREVISTRINGA  "commdlg_ShareViolation"
#define FILEOKSTRINGA   "commdlg_FileNameOK"
#define COLOROKSTRINGA  "commdlg_ColorOK"
#define SETRGBSTRINGA   "commdlg_SetRGBColor"
#define HELPMSGSTRINGA  "commdlg_help"
#define FINDMSGSTRINGA  "commdlg_FindReplace"

#define LBSELCHSTRINGW  L"commdlg_LBSelChangedNotify"
#define SHAREVISTRINGW  L"commdlg_ShareViolation"
#define FILEOKSTRINGW   L"commdlg_FileNameOK"
#define COLOROKSTRINGW  L"commdlg_ColorOK"
#define SETRGBSTRINGW   L"commdlg_SetRGBColor"
#define HELPMSGSTRINGW  L"commdlg_help"
#define FINDMSGSTRINGW  L"commdlg_FindReplace"

#ifdef UNICODE
#define LBSELCHSTRING  LBSELCHSTRINGW
#define SHAREVISTRING  SHAREVISTRINGW
#define FILEOKSTRING   FILEOKSTRINGW
#define COLOROKSTRING  COLOROKSTRINGW
#define SETRGBSTRING   SETRGBSTRINGW
#define HELPMSGSTRING  HELPMSGSTRINGW
#define FINDMSGSTRING  FINDMSGSTRINGW
#else
#define LBSELCHSTRING  LBSELCHSTRINGA
#define SHAREVISTRING  SHAREVISTRINGA
#define FILEOKSTRING   FILEOKSTRINGA
#define COLOROKSTRING  COLOROKSTRINGA
#define SETRGBSTRING   SETRGBSTRINGA
#define HELPMSGSTRING  HELPMSGSTRINGA
#define FINDMSGSTRING  FINDMSGSTRINGA
#endif

// HIWORD values for lParam of commdlg_LBSelChangeNotify message
#define CD_LBSELNOITEMS -1
#define CD_LBSELCHANGE   0
#define CD_LBSELSUB      1
#define CD_LBSELADD      2

typedef UINT (APIENTRY *LPPRINTHOOKPROC) (HWND, UINT, WPARAM, LPARAM);
typedef UINT (APIENTRY *LPSETUPHOOKPROC) (HWND, UINT, WPARAM, LPARAM);

typedef struct tagPDA {
   DWORD            lStructSize;
   HWND             hwndOwner;
   HGLOBAL          hDevMode;
   HGLOBAL          hDevNames;
   HDC              hDC;
   DWORD            Flags;
   WORD             nFromPage;
   WORD             nToPage;
   WORD             nMinPage;
   WORD             nMaxPage;
   WORD             nCopies;
   HINSTANCE        hInstance;
   LPARAM           lCustData;
   LPPRINTHOOKPROC  lpfnPrintHook;
   LPSETUPHOOKPROC  lpfnSetupHook;
   LPCSTR           lpPrintTemplateName;
   LPCSTR           lpSetupTemplateName;
   HGLOBAL          hPrintTemplate;
   HGLOBAL          hSetupTemplate;
} PRINTDLGA, *LPPRINTDLGA;
typedef struct tagPDW {
   DWORD            lStructSize;
   HWND             hwndOwner;
   HGLOBAL          hDevMode;
   HGLOBAL          hDevNames;
   HDC              hDC;
   DWORD            Flags;
   WORD             nFromPage;
   WORD             nToPage;
   WORD             nMinPage;
   WORD             nMaxPage;
   WORD             nCopies;
   HINSTANCE        hInstance;
   LPARAM           lCustData;
   LPPRINTHOOKPROC  lpfnPrintHook;
   LPSETUPHOOKPROC  lpfnSetupHook;
   LPCWSTR          lpPrintTemplateName;
   LPCWSTR          lpSetupTemplateName;
   HGLOBAL          hPrintTemplate;
   HGLOBAL          hSetupTemplate;
} PRINTDLGW, *LPPRINTDLGW;
#ifdef UNICODE
typedef PRINTDLGW PRINTDLG;
typedef LPPRINTDLGW LPPRINTDLG;
#else
typedef PRINTDLGA PRINTDLG;
typedef LPPRINTDLGA LPPRINTDLG;
#endif // UNICODE

BOOL  APIENTRY     PrintDlgA(LPPRINTDLGA);
BOOL  APIENTRY     PrintDlgW(LPPRINTDLGW);
#ifdef UNICODE
#define PrintDlg  PrintDlgW
#else
#define PrintDlg  PrintDlgA
#endif // !UNICODE

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
#define PD_USEDEVMODECOPIESANDCOLLATE 0x00040000
#define PD_DISABLEPRINTTOFILE        0x00080000
#define PD_HIDEPRINTTOFILE           0x00100000
#define PD_NONETWORKBUTTON           0x00200000

typedef struct tagDEVNAMES {
   WORD wDriverOffset;
   WORD wDeviceOffset;
   WORD wOutputOffset;
   WORD wDefault;
} DEVNAMES;

typedef DEVNAMES * LPDEVNAMES;

#define DN_DEFAULTPRN      0x0001


DWORD APIENTRY     CommDlgExtendedError(VOID);

#if(WINVER >= 0x0400)
#define WM_PSD_PAGESETUPDLG     (WM_USER  )
#define WM_PSD_FULLPAGERECT     (WM_USER+1)
#define WM_PSD_MINMARGINRECT    (WM_USER+2)
#define WM_PSD_MARGINRECT       (WM_USER+3)
#define WM_PSD_GREEKTEXTRECT    (WM_USER+4)
#define WM_PSD_ENVSTAMPRECT     (WM_USER+5)
#define WM_PSD_YAFULLPAGERECT   (WM_USER+6)

typedef UINT (APIENTRY* LPPAGEPAINTHOOK)( HWND, UINT, WPARAM, LPARAM );
typedef UINT (APIENTRY* LPPAGESETUPHOOK)( HWND, UINT, WPARAM, LPARAM );

typedef struct tagPSDA
{
    DWORD           lStructSize;
    HWND            hwndOwner;
    HGLOBAL         hDevMode;
    HGLOBAL         hDevNames;
    DWORD           Flags;
    POINT           ptPaperSize;
    RECT            rtMinMargin;
    RECT            rtMargin;
    HINSTANCE       hInstance;
    LPARAM          lCustData;
    LPPAGESETUPHOOK lpfnPageSetupHook;
    LPPAGEPAINTHOOK lpfnPagePaintHook;
    LPCSTR          lpPageSetupTemplateName;
    HGLOBAL         hPageSetupTemplate;
} PAGESETUPDLGA, * LPPAGESETUPDLGA;
typedef struct tagPSDW
{
    DWORD           lStructSize;
    HWND            hwndOwner;
    HGLOBAL         hDevMode;
    HGLOBAL         hDevNames;
    DWORD           Flags;
    POINT           ptPaperSize;
    RECT            rtMinMargin;
    RECT            rtMargin;
    HINSTANCE       hInstance;
    LPARAM          lCustData;
    LPPAGESETUPHOOK lpfnPageSetupHook;
    LPPAGEPAINTHOOK lpfnPagePaintHook;
    LPCWSTR         lpPageSetupTemplateName;
    HGLOBAL         hPageSetupTemplate;
} PAGESETUPDLGW, * LPPAGESETUPDLGW;
#ifdef UNICODE
typedef PAGESETUPDLGW PAGESETUPDLG;
typedef LPPAGESETUPDLGW LPPAGESETUPDLG;
#else
typedef PAGESETUPDLGA PAGESETUPDLG;
typedef LPPAGESETUPDLGA LPPAGESETUPDLG;
#endif // UNICODE

BOOL APIENTRY PageSetupDlgA( LPPAGESETUPDLGA );
BOOL APIENTRY PageSetupDlgW( LPPAGESETUPDLGW );
#ifdef UNICODE
#define PageSetupDlg  PageSetupDlgW
#else
#define PageSetupDlg  PageSetupDlgA
#endif // !UNICODE

#define PSD_DEFAULTMINMARGINS             0x00000000 // default (printer's)
#define PSD_INWININIINTLMEASURE           0x00000000 // 1st of 4 possible

#define PSD_MINMARGINS                    0x00000001 // use caller's
#define PSD_MARGINS                       0x00000002 // use caller's
#define PSD_INTHOUSANDTHSOFINCHES         0x00000004 // 2nd of 4 possible
#define PSD_INHUNDREDTHSOFMILLIMETERS     0x00000008 // 3rd of 4 possible
#define PSD_DISABLEMARGINS                0x00000010
#define PSD_DISABLEPRINTER                0x00000020
#define PSD_NOWARNING                     0x00000080 // must be same as PD_*
#define PSD_DISABLEORIENTATION            0x00000100
#define PSD_RETURNDEFAULT                 0x00000400 // must be same as PD_*
#define PSD_DISABLEPAPER                  0x00000200
#define PSD_SHOWHELP                      0x00000800 // must be same as PD_*
#define PSD_ENABLEPAGESETUPHOOK           0x00002000 // must be same as PD_*
#define PSD_ENABLEPAGESETUPTEMPLATE       0x00008000 // must be same as PD_*
#define PSD_ENABLEPAGESETUPTEMPLATEHANDLE 0x00020000 // must be same as PD_*
#define PSD_ENABLEPAGEPAINTHOOK           0x00040000
#define PSD_DISABLEPAGEPAINTING           0x00080000
#define PSD_NONETWORKBUTTON               0x00200000 // must be same as PD_*
#endif /* WINVER >= 0x0400 */


#ifdef __cplusplus
}
#endif  /* __cplusplus */

#include <poppack.h>
#endif  /* !_INC_COMMDLG */
