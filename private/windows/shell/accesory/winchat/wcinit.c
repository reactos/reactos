/*---------------------------------------------------------------------------*\
| INITIALIZATION MODULE
|   This module contains the one-time initialization routines.
|
|   FUNCTIONS
|   ---------
|   InitFontFromIni
|   SaveFontToIni
|   SaveBkGndToIni
|   LoadIntlStrings
|   SaveWindowPlacement
|   ReadWindowPlacement
|   CreateTools
|   DeleteTools
|   CreateChildWindows
|
|
| Copyright (c) Microsoft Corp., 1990-1993
|
| created: 01-Nov-91
| history: 01-Nov-91 <clausgi>  created.
|          29-Dec-92 <chriswil> port to NT, cleanup.
|          19-Oct-93 <chriswil> unicode enhancements from a-dianeo.
|
\*---------------------------------------------------------------------------*/

#include <windows.h>
#include <ddeml.h>
#include <commdlg.h>
#include <stdio.h>
#include <stdlib.h>
#include <commctrl.h>
#include <tchar.h>
#include "winchat.h"
#include "globals.h"
//#include "uniconv.h"


static TBBUTTON tbButtons[] =
{
    {0,0,         TBSTATE_ENABLED, TBSTYLE_SEP,  0},
    {0,IDM_DIAL  ,TBSTATE_ENABLED,TBSTYLE_BUTTON,0},
    {1,IDM_ANSWER,TBSTATE_ENABLED,TBSTYLE_BUTTON,0},
    {2,IDM_HANGUP,TBSTATE_ENABLED,TBSTYLE_BUTTON,0},
};
#define cTbButtons sizeof(tbButtons)/sizeof(TBBUTTON)

#ifdef WIN16
#pragma alloc_text (_INIT, InitFontFromIni)
#endif
/*---------------------------------------------------------------------------*\
| INITIALIZE FONT FROM INI FILE
|   This routine initializes the font information from the winchat.ini file.
|
| created: 11-Nov-91
| history: 29-Dec-92 <chriswil> ported to NT.
|
\*---------------------------------------------------------------------------*/
VOID FAR InitFontFromIni(VOID)
{
    CHARSETINFO csi;
    DWORD dw = GetACP();

    if (!TranslateCharsetInfo((DWORD*)&dw, &csi, TCI_SRCCODEPAGE)) {
        csi.ciCharset = ANSI_CHARSET;
    }

    // font related stuff
    // CODEWORK - the following code defines some somewhat arbitrary
    // constants for a first shot font - we should default to the
    // system font in an easier more portable manner.
    //
    lfSnd.lfHeight         = (int) GetPrivateProfileInt(szFnt,szHeight      ,(UINT)-13          ,szIni);
    lfSnd.lfWeight         = (int) GetPrivateProfileInt(szFnt,szWeight      ,700                ,szIni);
    lfSnd.lfWidth          = (int) GetPrivateProfileInt(szFnt,szWidth       ,  0                ,szIni);
    lfSnd.lfPitchAndFamily = (BYTE)GetPrivateProfileInt(szFnt,szPitchFam    , 22                ,szIni);
    lfSnd.lfItalic         = (BYTE)GetPrivateProfileInt(szFnt,szItalic      ,  0                ,szIni);
    lfSnd.lfUnderline      = (BYTE)GetPrivateProfileInt(szFnt,szUnderline   ,  0                ,szIni);
    lfSnd.lfStrikeOut      = (BYTE)GetPrivateProfileInt(szFnt,szStrikeOut   ,  0                ,szIni);

    lfSnd.lfCharSet        = (BYTE)GetPrivateProfileInt(szFnt, szCharSet    ,csi.ciCharset      ,szIni);

    lfSnd.lfOutPrecision   = (BYTE)GetPrivateProfileInt(szFnt,szOutPrecision,OUT_DEFAULT_PRECIS ,szIni);
    lfSnd.lfClipPrecision  = (BYTE)GetPrivateProfileInt(szFnt,szClipPrec    ,CLIP_DEFAULT_PRECIS,szIni);
    lfSnd.lfQuality        = (BYTE)GetPrivateProfileInt(szFnt,szQuality     ,DEFAULT_QUALITY    ,szIni);
    lfSnd.lfEscapement     = 0;
    lfSnd.lfOrientation    = 0;

#ifdef UNICODE
    if (gfDbcsEnabled) {
        GetPrivateProfileString(szFnt,szFontName,TEXT("MS Shell Dlg"),lfSnd.lfFaceName,LF_XPACKFACESIZE,szIni);
    }
    else {
        GetPrivateProfileString(szFnt,szFontName,TEXT("MS Shell Dlg"),lfSnd.lfFaceName,LF_XPACKFACESIZE,szIni);
    }
#else
    GetPrivateProfileString(szFnt,szFontName,TEXT("MS Shell Dlg"),lfSnd.lfFaceName,LF_XPACKFACESIZE,szIni);
#endif


    if(GetPrivateProfileString(szFnt,szColor,szNull,szBuf,SZBUFSIZ,szIni))
        SndColorref = myatol(szBuf);
    else
        SndColorref = GetSysColor(COLOR_WINDOWTEXT);

    return;
}


#ifdef WIN16
#pragma alloc_text (_INIT, SaveFontToIni)
#endif
/*---------------------------------------------------------------------------*\
| SAVE FONT TO INI FILE
|   This routine saves the font to the ini-file.
|
| created: 11-Nov-91
| history: 29-Dec-92 <chriswil> ported to NT.
|
\*---------------------------------------------------------------------------*/
VOID FAR SaveFontToIni(VOID)
{
    wsprintf(szBuf, TEXT("%d"), lfSnd.lfHeight);
    WritePrivateProfileString(szFnt, szHeight, szBuf, szIni);

    wsprintf(szBuf, TEXT("%d"), lfSnd.lfWidth);
    WritePrivateProfileString(szFnt, szWidth, szBuf, szIni);

    wsprintf(szBuf, TEXT("%d"), (WORD)lfSnd.lfCharSet);
    WritePrivateProfileString(szFnt, szCharSet, szBuf, szIni);

    wsprintf(szBuf, TEXT("%d"), (WORD)lfSnd.lfOutPrecision);
    WritePrivateProfileString(szFnt, szOutPrecision, szBuf, szIni);

    wsprintf(szBuf, TEXT("%d"), (WORD)lfSnd.lfClipPrecision);
    WritePrivateProfileString(szFnt, szClipPrec, szBuf, szIni);

    wsprintf(szBuf, TEXT("%d"), (WORD)lfSnd.lfQuality);
    WritePrivateProfileString(szFnt, szQuality, szBuf, szIni);

    wsprintf(szBuf, TEXT("%d"), lfSnd.lfWeight);
    WritePrivateProfileString(szFnt, szWeight, szBuf, szIni);

    wsprintf(szBuf, TEXT("%d"), (WORD)lfSnd.lfPitchAndFamily);
    WritePrivateProfileString(szFnt, szPitchFam, szBuf, szIni);

    wsprintf(szBuf, TEXT("%d"), (WORD)lfSnd.lfItalic);
    WritePrivateProfileString(szFnt, szItalic, szBuf, szIni);

    wsprintf(szBuf, TEXT("%d"), (WORD)lfSnd.lfUnderline);
    WritePrivateProfileString(szFnt, szUnderline, szBuf, szIni);

    wsprintf(szBuf, TEXT("%d"), (WORD)lfSnd.lfStrikeOut);
    WritePrivateProfileString(szFnt, szStrikeOut, szBuf, szIni);

    WritePrivateProfileString(szFnt, szFontName, lfSnd.lfFaceName, szIni);
    wsprintf(szBuf, TEXT("%ld"), (DWORD)SndColorref);

    WritePrivateProfileString(szFnt, szColor, szBuf, szIni);

    return;
}


#ifdef WIN16
#pragma alloc_text (_INIT, SaveBkGndToIni)
#endif
/*---------------------------------------------------------------------------*\
| SAVE BACKGROUND TO INI FILE
|   This routine saves the background-color to file.
|
| created: 27-Mar-95
| history: 27-Mar-95 <chriswil> created.
|
\*---------------------------------------------------------------------------*/
VOID FAR SaveBkGndToIni(VOID)
{
    wsprintf(szBuf, TEXT("%ld"), (DWORD)SndBrushColor);
    WritePrivateProfileString(szPref, szBkgnd, szBuf, szIni);

    return;
}


#ifdef WIN16
#pragma alloc_text (_INIT, LoadIntlStrings)
#endif
/*---------------------------------------------------------------------------*\
| LOAD INTERNAL STRINGS
|   This routine loads the resources strings.
|
| created: 11-Nov-91
| history: 29-Dec-92 <chriswil> ported to NT.
|
\*---------------------------------------------------------------------------*/
VOID FAR LoadIntlStrings(VOID)
{
    LoadString(hInst,IDS_HELV          , szHelv          , SMLRCBUF);
    LoadString(hInst,IDS_APPNAME       , szAppName       , SMLRCBUF);
    LoadString(hInst,IDS_SERVICENAME   , szServiceName   , SMLRCBUF);
    LoadString(hInst,IDS_SYSERR        , szSysErr        , BIGRCBUF);
    LoadString(hInst,IDS_DIALING       , szDialing       , BIGRCBUF);
    LoadString(hInst,IDS_CONNECTABANDON, szConnectAbandon, BIGRCBUF);
    LoadString(hInst,IDS_HANGINGUP     , szHangingUp     , BIGRCBUF);
    LoadString(hInst,IDS_HASTERMINATED , szHasTerminated , BIGRCBUF);
    LoadString(hInst,IDS_CONNECTEDTO   , szConnectedTo   , BIGRCBUF);
    LoadString(hInst,IDS_CONNECTING    , szConnecting    , BIGRCBUF);
    LoadString(hInst,IDS_ISCALLING     , szIsCalling     , BIGRCBUF);
    LoadString(hInst,IDS_DIALHELP      , szDialHelp      , BIGRCBUF);
    LoadString(hInst,IDS_ANSWERHELP    , szAnswerHelp    , BIGRCBUF);
    LoadString(hInst,IDS_HANGUPHELP    , szHangUpHelp    , BIGRCBUF);
    LoadString(hInst,IDS_NOCONNECT     , szNoConnect     , BIGRCBUF);
    LoadString(hInst,IDS_ALWAYSONTOP   , szAlwaysOnTop   , BIGRCBUF);
    LoadString(hInst,IDS_NOCONNECTTO   , szNoConnectionTo, BIGRCBUF);
    LoadString(hInst,IDS_NONETINSTALLED, szNoNet         , SZBUFSIZ);

    LoadString(hInst,IDS_INISECTION, szIniSection    , SZBUFSIZ);
    LoadString(hInst,IDS_INIPREFKEY, szIniKey1       , BIGRCBUF);
    LoadString(hInst,IDS_INIFONTKEY, szIniKey2       , BIGRCBUF);
    LoadString(hInst,IDS_INIRINGIN , szIniRingIn     , BIGRCBUF);
    LoadString(hInst,IDS_INIRINGOUT, szIniRingOut    , BIGRCBUF);

    return;
}


#ifdef WIN16
#pragma alloc_text (_INIT, SaveWindowPlacement)
#endif
/*---------------------------------------------------------------------------*\
| SAVE WINDOW PLACEMENT
|   This routine saves the window position to the inifile.
|
| created: 11-Nov-91
| history: 29-Dec-92 <chriswil> ported to NT.
|
\*---------------------------------------------------------------------------*/
VOID FAR SaveWindowPlacement(PWINDOWPLACEMENT w)
{
    wsprintf(szBuf,szPlcFmt,w->showCmd,
                            w->ptMaxPosition.x,
                            w->ptMaxPosition.y,
                            w->rcNormalPosition.left,
                            w->rcNormalPosition.top,
                            w->rcNormalPosition.right,
                            w->rcNormalPosition.bottom);

    WritePrivateProfileString(szPref,szPlacement,szBuf,szIni);

    return;
}


/*---------------------------------------------------------------------------*\
| GET WINDOW PLACEMENT
|   This routine loads the window placement from the inifile.
|
| created: 11-Nov-91
| history: 29-Dec-92 <chriswil> ported to NT.
|
\*---------------------------------------------------------------------------*/
BOOL FAR ReadWindowPlacement(PWINDOWPLACEMENT w)
{
    BOOL bRet;


    bRet = FALSE;
    if(GetPrivateProfileString(szPref,szPlacement,szNull,szBuf,SZBUFSIZ,szIni))
    {
        w->length = sizeof(WINDOWPLACEMENT);

        if(_stscanf(szBuf,szPlcFmt,&(w->showCmd),
                                 &(w->ptMaxPosition.x),
                                 &(w->ptMaxPosition.y),
                                 &(w->rcNormalPosition.left),
                                 &(w->rcNormalPosition.top),
                                 &(w->rcNormalPosition.right),
                                 &(w->rcNormalPosition.bottom)) == 7)
        {

            bRet = TRUE;
        }
    }

    return(bRet);
}


#ifdef WIN16
#pragma alloc_text (_INIT, CreateTools)
#endif
/*---------------------------------------------------------------------------*\
| CREATE TOOLS
|   This routine creates the visual tools for the interface.
|
| created: 11-Nov-91
| history: 29-Dec-92 <chriswil> ported to NT.
|
\*---------------------------------------------------------------------------*/
VOID FAR CreateTools(HWND hwnd)
{
    HDC hdc;


    hdc        = GetDC(hwnd);
    hMemDC     = CreateCompatibleDC(hdc);
    hPhnBitmap = CreateCompatibleBitmap(hdc,cxIcon * 3,cyIcon);
    hOldMemObj = SelectObject(hMemDC,hPhnBitmap);
    ReleaseDC(hwnd,hdc);


    hHilitePen    = CreatePen(PS_SOLID,1,GetSysColor(COLOR_BTNHIGHLIGHT));
    hShadowPen    = CreatePen(PS_SOLID,1,GetSysColor(COLOR_BTNSHADOW));
    hFramePen     = CreatePen(PS_SOLID,1,GetSysColor(COLOR_WINDOWFRAME));

    hBtnFaceBrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
    hEditSndBrush = CreateSolidBrush(SndBrushColor);
    hEditRcvBrush = CreateSolidBrush(RcvBrushColor);

    // Create the statusbar/toolbar for the interface.
    //
    hwndToolbar = CreateToolbarEx(hwnd,(ChatState.fToolBar ? WS_VISIBLE : 0) | WS_BORDER | TBSTYLE_TOOLTIPS,IDC_TOOLBAR,6,hInst,IDBITMAP,tbButtons,cTbButtons,0,0,0,0,sizeof(TBBUTTON));
    hwndStatus  = CreateStatusWindow((ChatState.fStatusBar ? WS_VISIBLE : 0) | WS_BORDER | WS_CHILD,szNull,hwnd,IDSTATUS);

    // Load the application icons.
    //
    hPhones[0] = LoadIcon(hInst,TEXT("phone1"));
    hPhones[1] = LoadIcon(hInst,TEXT("phone2"));
    hPhones[2] = LoadIcon(hInst,TEXT("phone3"));


    // now build the parameters based on the font we will be using
    //
    dyBorder = GetSystemMetrics(SM_CYBORDER);

    return;
}


#ifdef WIN16
#pragma alloc_text (_INIT, DeleteTools)
#endif
/*---------------------------------------------------------------------------*\
| DELETES TOOLS
|   This routine deletes the visual tools for the interface.
|
| created: 11-Nov-91
| history: 29-Dec-92 <chriswil> ported to NT.
|
\*---------------------------------------------------------------------------*/
VOID FAR DeleteTools(HWND hwnd)
{
    DestroyWindow(hwndStatus);
    DestroyWindow(hwndToolbar);


    if(hEditSndFont)
        DeleteObject(hEditSndFont);

    if(hEditRcvFont)
        DeleteObject(hEditRcvFont);

    DeleteObject(hHilitePen);
    DeleteObject(hShadowPen);
    DeleteObject(hFramePen);
    DeleteObject(hBtnFaceBrush);
    DeleteObject(hEditSndBrush);
    DeleteObject(hEditRcvBrush);

    SelectObject(hMemDC,hOldMemObj);
    DeleteObject(hPhnBitmap);
    DeleteDC(hMemDC);

    return;
}


#ifdef WIN16
#pragma alloc_text (_INIT, CreateChildWindows)
#endif
/*---------------------------------------------------------------------------*\
| CREATE CHILD WINDOWS
|   This routine creates the child-windows for the application.
|
| created: 11-Nov-91
| history: 29-Dec-92 <chriswil> ported to NT.
|
\*---------------------------------------------------------------------------*/
VOID FAR CreateChildWindows(HWND hwnd)
{
    hwndSnd = CreateWindow (TEXT("edit"),
                            NULL,
                            WS_CHILD | WS_BORDER | WS_MAXIMIZE | WS_VISIBLE |
                            WS_VSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_READONLY,
                            0, 0, 0, 0,
                            hwnd,
                            (HMENU)ID_EDITSND,
                            hInst,
                            NULL);


    hwndRcv = CreateWindow (TEXT("edit"),
                            NULL,
                            WS_CHILD | WS_BORDER | WS_MAXIMIZE | WS_VISIBLE |
                            WS_VSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_READONLY,
                            0, 0, 0, 0,
                            hwnd,
                            (HMENU)ID_EDITRCV,
                            hInst,
                            NULL);

    // hook the send window...
    //
    lpfnOldEditProc = (WNDPROC)GetWindowLongPtr(hwndSnd,GWLP_WNDPROC);
    SetWindowLongPtr(hwndSnd,GWLP_WNDPROC,(LONG_PTR)EditProc);

    ShowWindow(hwndSnd,SW_SHOW);
    ShowWindow(hwndRcv,SW_SHOW);

    return;
}
