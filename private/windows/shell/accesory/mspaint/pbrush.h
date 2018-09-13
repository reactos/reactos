// pbrush.h : main header file for the PBRUSH application
//

#ifndef __AFXWIN_H__
#error include TEXT('stdafx.h') before including this file for PCH
#endif

#include "resource.h"       // main symbols
#ifdef PNG_SUPPORT
#undef PNG_SUPPORT
#endif
// Bits for CTheApp::m_wEmergencyFlags
//
#define memoryEmergency 0x0001 // in a low free memory state
#define gdiEmergency    0x0002 // some GDI create failed
#define userEmergency   0x0004 // a CreateWindow failed
#define warnEmergency   0x0008 // still need to warn the user
#define failedEmergency 0x0010 // last operation actually failed

// This is the minimum delay between warning messages so the user doesn't
// get bombed by repetitious message boxes.  The value is in milli-seconds.

#define ticksBetweenWarnings (1000L * 60)

#define nSysBrushes 25
#define nOurBrushes 4
#define nFilters    16
class CPBTemplateServer : public COleTemplateServer
{
        public:

        void UpdateRegistry(OLE_APPTYPE nAppType,
                LPCTSTR* rglpszRegister = NULL, LPCTSTR* rglpszOverwrite = NULL);
} ;

/***************************************************************************/
// CPBApp:
// See pbrush.cpp for the implementation of this class
//

class CPBApp : public CWinApp
    {
    public:


    CPBApp();
    //
    // OnFileNew made public for scanning support
    //
    afx_msg void OnFileNew();
    // Overrides
    virtual BOOL InitInstance();
    virtual int  ExitInstance();

    virtual void WinHelp( DWORD dwData, UINT nCmd = HELP_CONTEXT ); // general
    virtual BOOL OnIdle(LONG);

    // error handling routines
    inline  BOOL InEmergencyState() const { return m_wEmergencyFlags != 0; }

    void    SetMemoryEmergency(BOOL bFailed = TRUE);
    void    SetGdiEmergency   (BOOL bFailed = TRUE);
    void    SetUserEmergency  (BOOL bFailed = TRUE);

    BOOL    CheckForEmergency() { return (m_wEmergencyFlags? TRUE: FALSE); }
    void    WarnUserOfEmergency();

    void    SetFileError( UINT uOperationint, int nCause, LPCTSTR lpszFile = NULL );
    void    FileErrorMessageBox( void );

    CString GetLastFile() {return m_sLastFile;}

    void    TryToFreeMemory();

    void    ParseCommandLine();

    // Patch to set the devmode and devname after pagesetup
    void    SetDeviceHandles(HANDLE hDevNames, HANDLE hDevMode);

    // setup routines
    void    LoadProfileSettings();
    void    SaveProfileSettings();
    void    GetSystemSettings( CDC* pdc );

    // Methods
    CPoint  CheckWindowPosition( CPoint ptPosition, CSize& sizeWindow );

    CDocument* OpenDocumentFile( LPCTSTR lpszFileName );

    BOOL    DoPromptFileName( CString& fileName, UINT nIDSTitle, DWORD lFlags,
                              BOOL bOpenFileDialog, int& iColors, BOOL bOnlyBmp );

    void    RegisterShell(CSingleDocTemplate *pDocTemplate);

    // Implementation
    CPBTemplateServer m_server; // Server object for document creation

    // This is the minimum amount of free memory we like to have
    DWORD   m_dwLowMemoryBytes;
    UINT    m_nLowGdiPercent;
    UINT    m_nLowUserPercent;

    WORD    m_wEmergencyFlags;

    // General user settings
    BOOL    m_bShowStatusbar;

#ifdef CUSTOMFLOAT
    BOOL    m_bShowToolbar;
    BOOL    m_bShowColorbar;
#endif

    BOOL    m_bShowThumbnail;
    BOOL    m_bShowTextToolbar;
    BOOL    m_bShowIconToolbar;
    BOOL    m_bShowGrid;

#ifdef CUSTOMFLOAT
    BOOL    m_bToolsDocked;
    BOOL    m_bColorsDocked;
#endif //CUSTOMFLOAT


    BOOL    m_bEmbedded;
    BOOL    m_bLinked;
    BOOL    m_bHidden;
    BOOL    m_bActiveApp;
    BOOL    m_bPenSystem;
    BOOL    m_bMonoDevice;
    BOOL    m_bPaletted;

    BOOL    m_bPrintOnly;
    CString m_strDocName;
    CString m_strPrinterName;
    CString m_strDriverName;
    CString m_strPortName;

#ifdef PCX_SUPPORT
    BOOL    m_bPCXfile;
#endif

    int     m_iCurrentUnits;

    // custom colors defined by the user
    COLORREF* m_pColors;
    int       m_iColors;

    // copy of the system wide palette
    CPalette* m_pPalette;

    CFont   m_fntStatus;

    int     m_nEmbeddedType;

    HWND    m_hwndInPlaceApp;

    class   CInPlaceFrame* m_pwndInPlaceFrame;

#ifdef CUSTOMFLOAT
    CRect   m_rectFloatTools;
    CRect   m_rectFloatColors;
#endif

    CRect   m_rectFloatThumbnail;

    CRect   m_rectMargins;

    WINDOWPLACEMENT m_wpPlacement;

    CSize   m_sizeBitmap;

    int     m_iPointSize;
    int     m_iPosTextX;
    int     m_iPosTextY;
    int     m_iBoldText;
    int     m_iUnderlineText;
    int     m_iItalicText;

    int     m_iVertEditText;

    int     m_iPenText;
    CString m_strTypeFaceName;
    int     m_iCharSet;

    int     m_iSnapToGrid;
    int     m_iGridExtent;

    // general system metrics. updated on system notification
    struct
        {
        int iWidthinPels;
        int iHeightinPels;
        int iWidthinMM;
        int iHeightinMM;
        int iWidthinINCH;
        int iHeightinINCH;
        int ixPelsPerDM;
        int iyPelsPerDM;
        int ixPelsPerMM;
        int iyPelsPerMM;
        int ixPelsPerINCH;
        int iyPelsPerINCH;
        int iBitsPixel;
        int iPlanes;
        } ScreenDeviceInfo;

    int     m_cxFrame;
    int     m_cyFrame;
    int     m_cxBorder;
    int     m_cyBorder;
    int     m_cyCaption;

    CBrush* m_pbrSysColors[nSysBrushes + nOurBrushes];
    CString m_sCurFile;
    int m_iflFltType[nFilters]; // export filter types available
    int m_nFltTypeUsed;
    int m_nFilterInIdx;
    int m_nFilterOutIdx;
    DWORD GetFilterIndex (int nFltType);
    void FixExtension (CString& fileName, int iflFltType);

    #ifdef _DEBUG
    BOOL    m_bLogUndo;
    #endif

    private:

    int     m_nFileErrorCause;  // from CFileException::m_cause
    WORD    m_wEmergencyFlagss;
    DWORD   m_tickLastWarning;
    CString m_strEmergencyNoMem;
    CString m_strEmergencyLowMem;
    CString m_sLastFile;
    UINT    m_uOperation;

    afx_msg void OnFileOpen();


    //{{AFX_MSG(CPBApp)
    afx_msg void OnAppAbout();
        // NOTE - the ClassWizard will add and remove member functions here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()
    };

extern CPBApp theApp;

#define IsInPlace()     (theApp.m_pwndInPlaceFrame != NULL)

//#define SZ_MAPISENDDOC TEXT("MAPISendDocuments")
//#define MAPIDLL TEXT("MAPI32.DLL")

typedef ULONG (FAR PASCAL *LPFNMAPISENDDOCUMENTS)(ULONG, LPTSTR, LPTSTR, LPTSTR, ULONG);

void CancelToolMode(BOOL bSelectionCommand);

class CRegKey
{
public:
        CRegKey(HKEY hkParent, LPCTSTR pszSubKey) { if (RegCreateKey(hkParent, pszSubKey, &m_hk)!=ERROR_SUCCESS) m_hk=NULL; }
        ~CRegKey() { if (m_hk) RegCloseKey(m_hk); }
        operator HKEY() const { return(m_hk); }

private:
        HKEY m_hk;
};

extern const CLSID BASED_CODE CLSID_Paint;
extern const CLSID BASED_CODE CLSID_PaintBrush;

#define ARRAYSIZE(_x) sizeof(_x)/sizeof(_x[0])

// make atoi work if building unicode
//
#ifdef UNICODE
#define Atoi _wtoi
#define _Itoa _itow
#define Itoa _itow
#else
#define Atoi atoi
#define _Itoa _itoa
#define Itoa itoa
#endif


// macro-ize ansi/unicode conversions
#define AtoW(x, y) MultiByteToWideChar (CP_ACP, 0, (x), -1, (y), (lstrlenA ((x))+1))
#define WtoA(x,y) WideCharToMultiByte(CP_ACP, 0, (x), -1, (y), (lstrlenW((x))+1), NULL,NULL)


#ifdef USE_MIRRORING

////    REGetLayout - Get layout of DC
//
//      Returns layout flags from an NT5/W98 or later DC, or zero
//      on legacy platforms.

#ifndef WS_EX_LAYOUTRTL
#define WS_EX_LAYOUTRTL         0x00400000L
#endif
#ifndef WS_EX_NOINHERITLAYOUT
#define WS_EX_NOINHERITLAYOUT   0x00100000L
#endif
#ifndef LAYOUT_RTL
#define LAYOUT_RTL              0x01
#endif

DWORD WINAPI PBGetLayoutInit(HDC hdc);
DWORD WINAPI PBSetLayoutInit(HDC hdc, DWORD dwLayout);
extern DWORD (WINAPI *PBSetLayout) (HDC hdc, DWORD dwLayout);
extern DWORD (WINAPI *PBGetLayout) (HDC hdc);

#endif


/***************************************************************************/
