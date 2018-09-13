
/******************************************************************************

                        C L I P B R D   H E A D E R

    Name:       clipbrd.h
    Date:       1/20/94
    Creator:    John Fu

    Description:
        This is the header for clipbrd.c

    History:
        19-Apr-1994 John Fu     Add DDE_DIB2BITMAP.

******************************************************************************/




// used for winnet server browse call

#define WNETBROWSEENTRYPOINT    146
#define BIGRCBUF                64
#define SMLRCBUF                32

#define DDE_DIB2BITMAP          0xFFFFFFFF




extern  HANDLE  hmutexClp;
extern  HANDLE  hXacting;
extern  HANDLE  hmodNetDriver;


extern  HICON   hicClipbrd;
extern  HICON   hicClipbook;
extern  HICON   hicRemote;




extern  HICON   hicLock;                    // Icon for Lock on thumbnail bitmaps
extern  HFONT   hfontUni;                   // Handle for Unicode font, if it exists




// Application-wide flags

extern  BOOL    fStatus;                    // status bar shown?
extern  BOOL    fToolBar;                   // tool bar shown?
extern  BOOL    fShareEnabled;              // sharing allowed in system.ini?
extern  BOOL    fNetDDEActive;              // NetDDE detected?
extern  BOOL    fAppLockedState;            // app UI locked (see LockApp())
extern  BOOL    fClipboardNeedsPainting;    // indicates deferred clp paint
extern  BOOL    fSharePreference;           // shared checked on paste?
extern  BOOL    fNeedToTileWindows;         // need to tile windows on size
extern  BOOL    fAppShuttingDown;           // in process of closing
extern  BOOL    fFillingClpFromDde;         // in process of adding clp formats
extern  BOOL    fAuditEnabled;

extern  HWND    hwndNextViewer;             // for clpbrd viewer chain
extern  HWND    hwndDummy;                  // used as dummy SetCapture target



// special case clipboard formats

extern  UINT    cf_bitmap;                      // we send/receive these in private 'packed' format
extern  UINT    cf_metafilepict;
extern  UINT    cf_palette;

extern  UINT    cf_preview;                     // PREVBMPSIZxPREVBMPSIZ preview bitmap private format



// these are formats that contain untranslated copies of link and objlink data

extern  UINT    cf_objectlinkcopy;
extern  UINT    cf_objectlink;
extern  UINT    cf_linkcopy;
extern  UINT    cf_link;




// DDEML

// These are effective constants created once and destroyed when we die

extern  HSZ     hszSystem;
extern  HSZ     hszTopics;
extern  HSZ     hszDataSrv;
extern  HSZ     hszFormatList;
extern  HSZ     hszClpBookShare;


extern  DWORD   dwCurrentHelpId ;



// instance proc from MSGF_DDEMGR filter

extern  WINDOWPLACEMENT Wpl;
extern  HOOKPROC        lpMsgFilterProc;
extern  HINSTANCE       hInst;
extern  HACCEL          hAccel;

extern  HFONT           hOldFont;
extern  HFONT           hFontStatus;
extern  HFONT           hFontPreview;



extern  HWND        hwndActiveChild;    // this handle identifies the currently active MDI window

extern  PMDIINFO    pActiveMDI;         // this pointer points to the MDI info struct of the
                                        // active MDI window IT SHOULD ALWAYS ==
                                        // GETMDIINFO(hwndActiveChild)


extern  HWND        hwndClpbrd;         // this handle identifies the clipboard window
extern  HWND        hwndLocal;          // this handle identifies the local clipbook window
extern  HWND        hwndClpOwner;       // this handle identifies the clipboard owning MDI child (if any)
extern  HWND        hwndMDIClient;      // handle to MDI Client window
extern  HWND        hwndApp;            // global app window
extern  HDC         hBtnDC;             // memory DC used for owner draw stuff
extern  HBITMAP     hOldBitmap;
extern  HBITMAP     hPreviewBmp;
extern  HBITMAP     hPgUpBmp;
extern  HBITMAP     hPgDnBmp;
extern  HBITMAP     hPgUpDBmp;
extern  HBITMAP     hPgDnDBmp;

extern  int         dyStatus;           // height of status bar
extern  int         dyButtonBar;        // height of button bar
extern  int         dyPrevFont;         // height of listbox font - height+external



extern  TCHAR       szHelpFile[];
extern  TCHAR       szChmHelpFile[];

extern  TCHAR       szClipBookClass[];  // frame window class
extern  TCHAR       szChild[];          // Class name for MDI window
extern  TCHAR       szDummy[];          // class name of hidden dummy window

extern  TCHAR       szNDDEcode[];
extern  TCHAR       szNDDEcode1[];
extern  TCHAR       szClpBookShare[];


// localized strings
extern  TCHAR       szHelv[SMLRCBUF];   // status line font
extern  TCHAR       szAppName[SMLRCBUF];
extern  TCHAR       szLocalClpBk[SMLRCBUF];
extern  TCHAR       szSysClpBrd[SMLRCBUF];
extern  TCHAR       szDataUnavail[BIGRCBUF];
extern  TCHAR       szReadingItem[BIGRCBUF];
extern  TCHAR       szViewHelpFmt[BIGRCBUF];
extern  TCHAR       szActivateFmt[BIGRCBUF];
extern  TCHAR       szRendering[BIGRCBUF];
extern  TCHAR       szDefaultFormat[BIGRCBUF];
extern  TCHAR       szGettingData[BIGRCBUF];
extern  TCHAR       szEstablishingConn[BIGRCBUF];
extern  TCHAR       szClipBookOnFmt[BIGRCBUF];
extern  TCHAR       szPageFmt[SMLRCBUF];
extern  TCHAR       szPageFmtPl[SMLRCBUF];
extern  TCHAR       szPageOfPageFmt[SMLRCBUF];
extern  TCHAR       szDelete[SMLRCBUF];
extern  TCHAR       szDeleteConfirmFmt[SMLRCBUF];
extern  TCHAR       szFileFilter[BIGRCBUF];
extern  TCHAR       *szFilter;




// Registry key strings
extern  TCHAR       szPref[];
extern  TCHAR       szConn[];
extern  TCHAR       szStatusbar[];
extern  TCHAR       szToolbar[];
extern  TCHAR       szShPref[];
extern  TCHAR       szEnableShr[];
extern  TCHAR       szDefView[];


#if DEBUG
extern  TCHAR       szDebug[];
#endif
extern  TCHAR       szNull[];



HKEY hkeyRoot;


// buffers
extern  TCHAR       szBuf[SZBUFSIZ];
extern  TCHAR       szBuf2[SZBUFSIZ];

extern  TCHAR       szConvPartner[128];                 // bigger than max server name
extern  TCHAR       szKeepAs[MAX_NDDESHARENAME + 2];


// DDEML stuff

extern  DWORD      idInst;                              // DDEML handle







//
// function prototypes
//

void OnDrawClipboard(
    HWND    hwnd);


LRESULT OnEraseBkgnd(
    HWND    hwnd,
    HDC     hdc);


LRESULT OnPaint(
    HWND    hwnd);


LRESULT CALLBACK FrameWndProc(
    HWND    hwnd,
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam);


LRESULT CALLBACK ChildWndProc(
    HWND    hwnd,
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam);


VOID SendMessageToKids (
    WORD    msg,
    WPARAM  wParam,
    LPARAM  lParam);


BOOL SyncOpenClipboard(
    HWND    hwnd);


BOOL SyncCloseClipboard(void);
