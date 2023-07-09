/*---------------------------------------------------------------------------*\
| GLOBALS HEADER FILE
|   This module contains the external references for the global-variables
|   in globals.c
|
|
| Copyright (c) Microsoft Corp., 1990-1993
|
| created: 29-Dec-92
| history: 29-Dec-92 <chriswil> created with port to NT.
|          19-Oct-93 <chriswil> unicode enhancements from a-dianeo.
|
\*---------------------------------------------------------------------------*/

#ifndef WINCHAT_GLOBALS_H
#define WINCHAT_GLOBALS_H

extern HFONT    hEditSndFont;
extern HWND     hwndSnd;
extern HBRUSH   hEditSndBrush;
extern COLORREF SndColorref;
extern COLORREF SndBrushColor;
extern LOGFONT  lfSnd;
extern RECT     SndRc;

extern HFONT    hEditRcvFont;
extern HWND     hwndRcv;
extern HBRUSH   hEditRcvBrush;
extern COLORREF RcvColorref;
extern COLORREF RcvBrushColor;
extern COLORREF PartBrushColor;
extern LOGFONT  lfRcv;
extern RECT     RcvRc;

extern DWORD    idInst;
extern HSZ      hszServiceName;
extern HSZ      hszConnect;
extern HSZ      hszChatTopic;
extern HSZ      hszChatShare;
extern HSZ      hszTextItem;
extern HSZ      hszConvPartner;
extern HSZ      hszConnectTest;
extern HSZ      hszLocalName;
extern HCONV    ghConv;
extern UINT_PTR idTimer;
extern int      dyStatus;
extern int      dyButtonBar;
extern int      dyBorder;
extern int      cxIcon;
extern int      cyIcon;
extern int      cbTextLen;

extern DWORD    StrXactID;
extern DWORD    XactID;
extern HANDLE   hInst;
extern HACCEL   hAccel;
extern HDC      hMemDC;
extern HBITMAP  hOldBitmap;
extern HBITMAP  hPhnBitmap;
extern HBITMAP  hOldMemObj;
extern HICON    hPhones[3];
extern HFONT    hFontStatus;
extern HFONT    hOldFont;
extern HBRUSH   hBtnFaceBrush;
extern HPEN     hShadowPen;
extern HPEN     hHilitePen;
extern HPEN     hFramePen;
extern UINT     cf_chatdata;
extern HWND     hwndActiveEdit;
extern HWND     hwndApp;
extern HWND     hwndToolbar;
extern HWND     hwndStatus;

extern WNETCALL WNetServerBrowseDialog;


extern int     ASeq[];
extern WORD    cAnimate;
extern HANDLE  hMemTextBuffer;
extern int     nConnectAttempt;


extern WNDPROC  lpfnOldEditProc;
extern WNDPROC  lpfnOldRcvEditProc;

extern LPBYTE   lpbTextBuffer;



extern CHOOSEFONT  chf;
extern CHOOSECOLOR chc;
extern DWORD       CustColors[16];


extern CHATSTATE       ChatState;
extern CHATDATA        ChatData;
extern CHATDATA        ChatDataRcv;
extern WINDOWPLACEMENT Wpl;


extern TCHAR szHelv          [];
extern TCHAR szAppName       [];
extern TCHAR szServiceName   [];
extern TCHAR szAlreadyConnect[];
extern TCHAR szAbandonFirst  [];
extern TCHAR szDialing       [];
extern TCHAR szYouCaller     [];
extern TCHAR szNotCalled     [];
extern TCHAR szNotConnected  [];
extern TCHAR szConnectAbandon[];
extern TCHAR szHangingUp     [];
extern TCHAR szHasTerminated [];
extern TCHAR szConnectedTo   [];
extern TCHAR szConnecting    [];
extern TCHAR szIsCalling     [];
extern TCHAR szDialHelp      [];
extern TCHAR szAnswerHelp    [];
extern TCHAR szHangUpHelp    [];
extern TCHAR szNoConnect     [];
extern TCHAR szNoConnectionTo[];
extern TCHAR szSysErr        [];
extern TCHAR szAlwaysOnTop   [];
extern TCHAR szNoNet         [];
extern TCHAR szBuf           [];
extern TCHAR szHelp          [];
extern TCHAR szConvPartner   [];
extern TCHAR szLocalName     [];
extern CONST TCHAR szChatTopic     [];
extern TCHAR szChatShare     [];
extern CONST TCHAR szWcRingIn      [];
extern CONST TCHAR szWcRingOut     [];
extern CONST TCHAR szSysIni        [];
extern CONST TCHAR szVredir        [];
extern CONST TCHAR szComputerName  [];
extern CONST TCHAR szChatText      [];
extern CONST TCHAR szConnectTest   [];
extern CONST TCHAR szWinChatClass  [];
extern CONST TCHAR szWinChatMenu   [];
extern CONST TCHAR szHelpFile      [];
extern CONST TCHAR szIni           [];
extern CONST TCHAR szFnt           [];
extern CONST TCHAR szPref          [];
extern CONST TCHAR szSnd           [];
extern CONST TCHAR szTool          [];
extern CONST TCHAR szStat          [];
extern CONST TCHAR szTop           [];
extern CONST TCHAR szUseOF         [];
extern CONST TCHAR szSbS           [];
extern CONST TCHAR szAutoAns       [];
extern CONST TCHAR szBkgnd         [];
extern CONST TCHAR szNull          [];

extern TCHAR szIniSection    [];
extern TCHAR szIniKey1       [];
extern TCHAR szIniKey2       [];
extern TCHAR szIniRingIn     [];
extern TCHAR szIniRingOut    [];

extern CONST TCHAR szHeight        [];
extern CONST TCHAR szWeight        [];
extern CONST TCHAR szPitchFam      [];
extern CONST TCHAR szItalic        [];
extern CONST TCHAR szUnderline     [];
extern CONST TCHAR szStrikeOut     [];
extern CONST TCHAR szFontName      [];
extern CONST TCHAR szWidth         [];
extern CONST TCHAR szCharSet       [];
extern CONST TCHAR szOutPrecision  [];
extern CONST TCHAR szClipPrec      [];
extern CONST TCHAR szQuality       [];
extern CONST TCHAR szColor         [];
extern CONST TCHAR szPlacement     [];
extern CONST TCHAR szPlcFmt        [];


extern UINT CONST nIDs[];

extern BOOL gfDbcsEnabled;

extern HIMC (WINAPI* pfnImmGetContext)(HWND);
extern BOOL (WINAPI* pfnImmReleaseContext)(HWND, HIMC);
extern LONG (WINAPI* pfnImmGetCompositionStringW)(HIMC, DWORD, LPVOID, DWORD);

#endif  // WINCHAT_GLOBALS_H

