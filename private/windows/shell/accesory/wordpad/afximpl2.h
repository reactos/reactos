// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1995 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

/////////////////////////////////////////////////////////////////////////////
// Auxiliary System/Screen metrics

struct AUX_DATA
{
        // system metrics
        int cxVScroll, cyHScroll;
        int cxIcon, cyIcon;

        int cxBorder2, cyBorder2;

        // device metrics for screen
        int cxPixelsPerInch, cyPixelsPerInch;

        // convenient system color
        HBRUSH hbrWindowFrame;
        HBRUSH hbrBtnFace;
#ifdef _MAC
        HBRUSH hbr3DLight;
#endif

        // color values of system colors used for CToolBar
        COLORREF clrBtnFace, clrBtnShadow, clrBtnHilite;
        COLORREF clrBtnText, clrWindowFrame;
#ifdef _MAC
        COLORREF clr3DLight;
#endif

        // standard cursors
        HCURSOR hcurWait;
        HCURSOR hcurArrow;
        HCURSOR hcurHelp;       // cursor used in Shift+F1 help

        // special GDI objects allocated on demand
        HFONT   hStatusFont;
        HFONT   hToolTipsFont;
        HBITMAP hbmMenuDot;

        // other system information
        UINT    nWinVer;        // Major.Minor version numbers
        BOOL    bWin32s;        // TRUE if Win32s (or Windows 95)
        BOOL    bWin4;          // TRUE if Windows 4.0
        BOOL    bNotWin4;       // TRUE if not Windows 4.0
        BOOL    bSmCaption;     // TRUE if WS_EX_SMCAPTION is supported
        BOOL    bMarked4;       // TRUE if marked as 4.0

#ifdef _MAC
        BOOL    bOleIgnoreSuspend;
#endif

// Implementation
        AUX_DATA();
        ~AUX_DATA();
        void UpdateSysColors();
        void UpdateSysMetrics();
};

extern AFX_DATA_IMPORT AUX_DATA afxData;

/////////////////////////////////////////////////////////////////////////////
// _AFX_EDIT_STATE

class _AFX_EDIT_STATE : public CNoTrackObject
{
public:
        _AFX_EDIT_STATE();
        virtual ~_AFX_EDIT_STATE();

        CFindReplaceDialog* pFindReplaceDlg; // find or replace dialog
        BOOL bFindOnly; // Is pFindReplace the find or replace?
        CString strFind;    // last find string
        CString strReplace; // last replace string
        BOOL bCase; // TRUE==case sensitive, FALSE==not
        int bNext;  // TRUE==search down, FALSE== search up
        BOOL bWord; // TRUE==match whole word, FALSE==not
};

#undef AFX_DATA
#define AFX_DATA

class _AFX_RICHEDIT2_STATE : public _AFX_EDIT_STATE
{
public:
	HINSTANCE m_hInstRichEdit;      // handle to richedit dll
	virtual ~_AFX_RICHEDIT2_STATE();
};

EXTERN_PROCESS_LOCAL(_AFX_RICHEDIT2_STATE, _afxRichEdit2State)

_AFX_RICHEDIT2_STATE* AFX_CDECL AfxGetRichEdit2State();


// dialog/commdlg hook procs
INT_PTR CALLBACK AfxDlgProc(HWND, UINT, WPARAM, LPARAM);

// support for standard dialogs
extern const UINT _afxNMsgSETRGB;
typedef UINT (CALLBACK* COMMDLGPROC)(HWND, UINT, WPARAM, LPARAM);

/////////////////////////////////////////////////////////////////////////////
// Special helpers

BOOL AFXAPI AfxHelpEnabled();  // determine if ID_HELP handler exists
