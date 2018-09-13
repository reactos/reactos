// pbrush.cpp : Defines the class behaviors for the application.
//
#include "stdafx.h"
#include "global.h"
#include "pbrush.h"
#include "pbrusfrm.h"
#include "ipframe.h"
#include "pbrusdoc.h"
#include "pbrusvw.h"
#include "bmobject.h"
#include "imgsuprt.h"
#include "imgwnd.h"
#include "imgwell.h"
#include "imgtools.h"
#include "ferr.h"
#include "cmpmsg.h"
#include "settings.h"
#include "undo.h"
#include "colorsrc.h"
#include "printres.h"
#include "loadimag.h"
#include "image.h"
#include <dlgs.h>
#include <shlobj.h>
#include "ofn.h"

// turn on visibility of GIF filter

#define GIF_SUPPORT

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


typedef BOOL(WINAPI* SHSPECPATH)(HWND,LPTSTR,int,BOOL);

#include "memtrace.h"

BOOL NEAR g_bShowAllFiles = FALSE;


#ifdef USE_MIRRORING
HINSTANCE ghInstGDI32=NULL;

DWORD WINAPI PBGetLayoutPreNT5(HDC hdc) {
    return 0;   // No mirroring on systems before NT5 or W98-CS
}

DWORD (WINAPI *PBGetLayout) (HDC hdc) = &PBGetLayoutInit;

DWORD WINAPI PBGetLayoutInit(HDC hdc) {

    PBGetLayout = (DWORD (WINAPI *) (HDC hdc)) GetProcAddress(ghInstGDI32, "GetLayout");

    if (!PBGetLayout) {
        PBGetLayout = PBGetLayoutPreNT5;
    }

    return PBGetLayout(hdc);

}


////    RESetLayout - Set layout of DC
//
//      Sets layout flags in an NT5/W98 or later DC.


DWORD WINAPI PBSetLayoutPreNT5(HDC hdc, DWORD dwLayout) {
    return 0;   // No mirroring on systems before NT5 or W98-CS
}

DWORD (WINAPI *PBSetLayout) (HDC hdc, DWORD dwLayout) = &PBSetLayoutInit;

DWORD WINAPI PBSetLayoutInit(HDC hdc, DWORD dwLayout) {

    PBSetLayout = (DWORD (WINAPI *) (HDC hdc, DWORD dwLayout)) GetProcAddress(ghInstGDI32, "SetLayout");

    if (!PBSetLayout) {
        PBSetLayout = PBSetLayoutPreNT5;
    }

    return PBSetLayout(hdc, dwLayout);

}
#endif

/***************************************************************************/
// CPBApp

BEGIN_MESSAGE_MAP(CPBApp, CWinApp)
    //{{AFX_MSG_MAP(CPBApp)
    ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
        // NOTE - the ClassWizard will add and remove mapping macros here.
        //    DO NOT EDIT what you see in these blocks of generated code!
    //}}AFX_MSG_MAP
    // Standard file based document commands
    ON_COMMAND(ID_FILE_NEW, OnFileNew)
    ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
    // Standard print setup command
    ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

/***************************************************************************/
// CPBApp construction

CPBApp::CPBApp()
    {
    // TODO: add construction code here,
    // Place all significant initialization in InitInstance
    #ifdef _DEBUG
    m_bLogUndo = FALSE;
    #endif

    // This is the minimum amount of free memory we like to have
    // (NOTE: These are overwritten by ReadInitFile)
    m_dwLowMemoryBytes = 1024L * 2200;
    m_nLowGdiPercent   = 10;
    m_nLowUserPercent  = 10;

    m_nFileErrorCause  = 0;  // from CFileException::m_cause
    m_wEmergencyFlags  = 0;
    m_tickLastWarning  = 0;
    m_iCurrentUnits    = 0;

    m_bShowStatusbar   = TRUE;

    m_bShowThumbnail   = FALSE;
    m_bShowTextToolbar = TRUE;
    m_bShowIconToolbar = TRUE;


    m_bEmbedded        = FALSE;
    m_bLinked          = FALSE;
    m_bHidden          = FALSE;
    m_bActiveApp       = FALSE;
    m_bPenSystem       = FALSE;
    m_bPaletted        = FALSE;
    m_pPalette         = NULL;
    m_bPrintOnly       = FALSE;
#ifdef PCX_SUPPORT
    m_bPCXfile         = FALSE;
#endif

    m_rectFloatThumbnail.SetRectEmpty();

    m_rectMargins.SetRect(MARGINS_DEFAULT, MARGINS_DEFAULT, MARGINS_DEFAULT,
        MARGINS_DEFAULT);

    m_pwndInPlaceFrame = NULL;
    m_hwndInPlaceApp   = NULL;

    m_pColors = NULL;
    m_iColors = 0;

    for (int index = 0; index < nSysBrushes + nOurBrushes; index++)
    {
       m_pbrSysColors[index] = NULL;
    }

    for (index=0;index<nFilters;index++)
    {
       m_iflFltType[index] = IFLT_UNKNOWN;
    }
    m_nFltTypeUsed = IFLT_UNKNOWN;
    m_nFilterInIdx = m_nFilterOutIdx = 1; // default is *.BMP

#ifdef USE_MIRRORING
    ghInstGDI32 = GetModuleHandle(TEXT("gdi32.dll"));
#endif
}
/***************************************************************************/
// The one and only CPBApp object

CPBApp theApp;

// This identifier was generated to be statistically unique for your app.
// You may change it if you prefer to choose a specific identifier.
const CLSID BASED_CODE CLSID_Paint =
{ 0xd3e34b21, 0x9d75, 0x101a, { 0x8c, 0x3d, 0x0, 0xaa, 0x0, 0x1a, 0x16, 0x52 } };
const CLSID BASED_CODE CLSID_PaintBrush =
{ 0x0003000A, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };

/***************************************************************************/
// Stolen from WordPad
BOOL MatchOption(LPTSTR lpsz, LPTSTR lpszOption)
{
        if (lpsz[0] == TEXT('-') || lpsz[0] == TEXT('/'))
        {
                lpsz++;
                if (lstrcmpi(lpsz, lpszOption) == 0)
                        return TRUE;
        }
        return FALSE;
}

void CPBApp::ParseCommandLine()
{
        BOOL bPrintTo = FALSE;
   #ifdef UNICODE
        int argcw;
        LPWSTR *argvw;
        argvw = CommandLineToArgvW (GetCommandLine (), &argcw);
   #define ARGV argvw
   #define ARGC argcw
   #else
   #define ARGV __argv
   #define ARGC __argc
   #endif
        // start at 1 -- the first is the exe
        for (int i=1; i< ARGC; i++)
        {
                if (MatchOption(ARGV[i], TEXT("pt")))
                        bPrintTo = m_bPrintOnly = TRUE;
                else if (MatchOption(ARGV[i], TEXT("p")))
                        m_bPrintOnly = TRUE;
//              else if (MatchOption(__argv[i], "t"))
//                      m_bForceTextMode = TRUE;
//              else if (MatchOption(__argv[i], "Embedding"))
//                      m_bEmbedded = TRUE;
//              else if (MatchOption(__argv[i], "Automation"))
//                      m_bEmbedded = TRUE;
                else if (m_strDocName.IsEmpty())
                        m_strDocName = ARGV[i];
                else if (bPrintTo && m_strPrinterName.IsEmpty())
                        m_strPrinterName = ARGV[i];
                else if (bPrintTo && m_strDriverName.IsEmpty())
                        m_strDriverName = ARGV[i];
                else if (bPrintTo && m_strPortName.IsEmpty())
                        m_strPortName = ARGV[i];
                else
                {
                        ASSERT(FALSE);
                }
        }
   #ifdef UNICODE
      GlobalFree (argvw);
   #endif
}


void GetShortModuleFileName(HINSTANCE hInst, LPTSTR pszName, UINT uLen)
{
        TCHAR szLongName[_MAX_PATH];

        GetModuleFileName(hInst, szLongName, _MAX_PATH);

#pragma message("GSMFN: Remove this hack!")
        // BUGBUG: GSPN sometimes fails on UNC's.  Try this until that is tracked
        // down
        lstrcpyn(pszName, szLongName, uLen);

        if (!GetShortPathName(szLongName, pszName, uLen))
        {
                GetLastError();
        }
}


void CPBApp::RegisterShell(CSingleDocTemplate *pDocTemplate)
{
        const struct
        {
                LPCTSTR pszActionID;
                LPCTSTR pszCommand;
        } aActions[] =
        {
                { TEXT("Open")   , TEXT("\"%s\" \"%%1\"") },
                { TEXT("Print")  , TEXT("\"%s\" /p \"%%1\"") },
                { TEXT("PrintTo"), TEXT("\"%s\" /pt \"%%1\" \"%%2\" \"%%3\" \"%%4\"") },
        } ;

        // We now need quotes around the file name, and MFC doesn't do this
        CString strTypeID;
        if (!pDocTemplate->GetDocString(strTypeID, CDocTemplate::regFileTypeId))
        {
                return;
        }

        strTypeID += TEXT("\\shell");

        CRegKey rkShellInfo(HKEY_CLASSES_ROOT, strTypeID);
        if (!(HKEY)rkShellInfo)
        {
                return;
        }

        TCHAR szFile[MAX_PATH];
        ::GetShortModuleFileName(AfxGetInstanceHandle(), szFile, ARRAYSIZE(szFile));

        int i;
        for (i=0; i<ARRAYSIZE(aActions); ++i)
        {
                CRegKey rkAction(rkShellInfo, aActions[i].pszActionID);
                if (!(HKEY)rkAction)
                {
                        continue;
                }

                // Note I do not set the name of the action;  I will need to add this
                // if I use anything other than "Open", "Print", or "PrintTo"

                TCHAR szCommand[MAX_PATH + 80];
                wsprintf(szCommand, aActions[i].pszCommand, szFile);

                RegSetValue(rkAction, TEXT("command"), REG_SZ, szCommand, 0);
        }

        // Set the OLE server for PBrush objects
        CRegKey rkPBrushInfo(HKEY_CLASSES_ROOT, TEXT("PBrush\\protocol\\StdFileEditing\\server"));
        if ((HKEY)rkPBrushInfo)
        {
                RegSetValue(rkPBrushInfo, TEXT(""), REG_SZ, szFile, 0);
        }
}


/***************************************************************************/
// CPBApp initialization

BOOL CPBApp::InitInstance()
    {
    SetRegistryKey( IDS_REGISTRY_PATH );
    if (m_pszProfileName)
    {
       free((void*)m_pszProfileName);
    }
    m_pszProfileName = _tcsdup(TEXT("Paint"));

    HDC hdc = ::GetDC( NULL );

    ASSERT( hdc != NULL );

    GetSystemSettings( CDC::FromHandle( hdc ) );

    ::ReleaseDC( NULL, hdc );

    // Because we cannot LoadString when these strings are needed (in
    // WarnUserOfEmergency) load them here in private member variables
    // of CTheApp...
    //
    m_strEmergencyNoMem.LoadString ( IDS_ERROR_NOMEMORY );
    m_strEmergencyLowMem.LoadString( IDS_ERROR_LOWMEMORY );

    // Initialize OLE 2.0 libraries
    if (! AfxOleInit())
        {
        AfxMessageBox( IDP_OLE_INIT_FAILED );
        return FALSE;
        }

    // Standard initialization
    // If you are not using these features and wish to reduce the size
    //  of your final executable, you should remove from the following
    //  the specific initialization routines you do not need.

    // SetDialogBkColor();        // Set dialog background color to gray
    LoadProfileSettings();     // Load standard INI file options (including MRU)
    InitCustomData();

    if (! g_pColors)
            {
            g_pColors = new CColors;

            if (! g_pColors->GetColorCount())
                {
                theApp.SetMemoryEmergency();
                return -1;
                }
            }
    // Register the application's document templates.  Document templates
    //  serve as the connection between documents, frame windows and views.

    CSingleDocTemplate* pDocTemplate;

    pDocTemplate = new CSingleDocTemplate( ID_MAINFRAME,
                                 RUNTIME_CLASS( CPBDoc ),
                                 RUNTIME_CLASS( CPBFrame ), // main SDI frame window
                                 RUNTIME_CLASS( CPBView ) );

    pDocTemplate->SetServerInfo( IDR_SRVR_EMBEDDED, IDR_SRVR_INPLACE,
                                 RUNTIME_CLASS( CInPlaceFrame ),
                                 RUNTIME_CLASS( CPBView ) );

    AddDocTemplate( pDocTemplate );

    // Connect the COleTemplateServer to the document template.
    //  The COleTemplateServer creates new documents on behalf
    //  of requesting OLE containers by using information
    //  specified in the document template.
    m_server.ConnectTemplate( CLSID_Paint, pDocTemplate, TRUE );
        // Note: SDI applications register server objects only if /Embedding
        //   or /Automation is present on the command line.

    RegisterShell(pDocTemplate);

    m_bEmbedded = RunEmbedded();

    // Parse the command line to see if launched as OLE server
    if (m_bEmbedded || RunAutomated())
        {
        // Register all OLE server (factories) as running.  This enables the
        //  OLE 2.0 libraries to create objects from other applications.
        COleTemplateServer::RegisterAll();

        // Application was run with /Embedding or /Automation.  Don't show the
        //  main window in this case.
        return TRUE;
        }

    // When a server application is launched stand-alone, it is a good idea
    //  to update the system registry in case it has been damaged.
    m_server.UpdateRegistry( OAT_INPLACE_SERVER );

    ParseCommandLine();

    // simple command line parsing
    if (m_strDocName.IsEmpty())
        {
        // create a new (empty) document
        OnFileNew();
        }
    else
        {
        CString sExt = GetExtension( m_strDocName );

        if (sExt.IsEmpty())
            {
            if (pDocTemplate->GetDocString( sExt, CDocTemplate::filterExt )
            &&                            ! sExt.IsEmpty())
                m_strDocName += sExt;
            }

        WIN32_FIND_DATA finddata;
        HANDLE hFind = FindFirstFile(m_strDocName, &finddata);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            FindClose(hFind);

            // Find the file name and replace it with the long file name
            int iBS = m_strDocName.ReverseFind(TEXT('\\'));
            if (iBS == -1)
            {
                iBS = m_strDocName.ReverseFind(TEXT(':'));
            }

            // HACK: Notice this is correct even if iBS==-1
            ++iBS;

            // Resize the memory string
            m_strDocName.GetBuffer(iBS);
            m_strDocName.ReleaseBuffer(iBS);

            m_strDocName += finddata.cFileName;
        }

        OpenDocumentFile( m_strDocName );
        }

    if (m_pMainWnd)
       m_pMainWnd->DragAcceptFiles();



    return TRUE;
    }

/***************************************************************************/

int CPBApp::ExitInstance()
    {
    CustomExit();   // clean up in customiz
    CleanupImages();

    if (g_pColors)
        {
        delete g_pColors;
        g_pColors = NULL;
        }

    if (m_fntStatus.m_hObject != NULL)
        m_fntStatus.DeleteObject();

    ResetSysBrushes();

    CTracker::CleanUpTracker();
    //
    // Work-around MFC bug - delete the tool tips and NULL
    // them so we don't crash after inplace activation
    //
    _AFX_THREAD_STATE* pThreadState = AfxGetThreadState();
    CToolTipCtrl* pToolTip = pThreadState->m_pToolTip;

    if (NULL != pToolTip)
    {
      pToolTip->DestroyWindow();
      delete pToolTip;
      pThreadState->m_pToolTip = NULL ;
    }


    return CWinApp::ExitInstance();
    }

/***************************************************************************/

void CPBApp::GetSystemSettings( CDC* pdc )
    {
    NONCLIENTMETRICS ncMetrics;

    ncMetrics.cbSize = sizeof( NONCLIENTMETRICS );

    if (SystemParametersInfo( SPI_GETNONCLIENTMETRICS,
                              sizeof( NONCLIENTMETRICS ),
                              &ncMetrics, 0 ))
        {
        if (m_fntStatus.m_hObject != NULL)
            m_fntStatus.DeleteObject();

        m_fntStatus.CreateFontIndirect( &ncMetrics.lfMenuFont );
        }

    ScreenDeviceInfo.iWidthinMM    = pdc->GetDeviceCaps( HORZSIZE   );
    ScreenDeviceInfo.iHeightinMM   = pdc->GetDeviceCaps( VERTSIZE   );
    ScreenDeviceInfo.iWidthinPels  = pdc->GetDeviceCaps( HORZRES    );
    ScreenDeviceInfo.iHeightinPels = pdc->GetDeviceCaps( VERTRES    );
    ScreenDeviceInfo.ixPelsPerINCH = pdc->GetDeviceCaps( LOGPIXELSX );
    ScreenDeviceInfo.iyPelsPerINCH = pdc->GetDeviceCaps( LOGPIXELSY );

    /* get the pels per decameter '.1' rounded */
    ScreenDeviceInfo.ixPelsPerDM   = (int)(((((long)ScreenDeviceInfo.iWidthinPels  * 1000L) / (long)ScreenDeviceInfo.iWidthinMM ) + 5L) / 10);
    ScreenDeviceInfo.iyPelsPerDM   = (int)(((((long)ScreenDeviceInfo.iHeightinPels * 1000L) / (long)ScreenDeviceInfo.iHeightinMM) + 5L) / 10);
    ScreenDeviceInfo.ixPelsPerMM   = (ScreenDeviceInfo.ixPelsPerDM + 50) / 100;
    ScreenDeviceInfo.iyPelsPerMM   = (ScreenDeviceInfo.iyPelsPerDM + 50) / 100;
    ScreenDeviceInfo.iWidthinINCH  = (int)(((long)ScreenDeviceInfo.iWidthinMM  * 100L / 245L + 5L) / 10L);  //24.5 mm to the inch
    ScreenDeviceInfo.iHeightinINCH = (int)(((long)ScreenDeviceInfo.iHeightinMM * 100L / 245L + 5L) / 10L);

    ScreenDeviceInfo.iBitsPixel    = pdc->GetDeviceCaps( BITSPIXEL );
    ScreenDeviceInfo.iPlanes       = pdc->GetDeviceCaps( PLANES    );

    m_cxFrame    = GetSystemMetrics( SM_CXFRAME );
    m_cyFrame    = GetSystemMetrics( SM_CYFRAME );
    m_cxBorder   = GetSystemMetrics( SM_CXBORDER );
    m_cyBorder   = GetSystemMetrics( SM_CYBORDER );
    m_cyCaption  = GetSystemMetrics( SM_CYSMCAPTION );
    m_bPenSystem = GetSystemMetrics( SM_PENWINDOWS )? TRUE: FALSE;
    m_bPaletted  = (pdc->GetDeviceCaps( RASTERCAPS ) & RC_PALETTE);

    m_bMonoDevice = ((ScreenDeviceInfo.iBitsPixel
                  *   ScreenDeviceInfo.iPlanes) == 1);

    SetErrorMode( SEM_NOOPENFILEERRORBOX );
    }

/***************************************************************************/

CPoint CPBApp::CheckWindowPosition( CPoint ptPosition, CSize& sizeWindow )
    {
    CPoint ptNew = ptPosition;

    sizeWindow.cx = max( sizeWindow.cx, 0 );
    sizeWindow.cy = max( sizeWindow.cy, 0 );

    if (sizeWindow.cx
    &&  sizeWindow.cy)
        {
        sizeWindow.cx = min( sizeWindow.cx, ScreenDeviceInfo.iWidthinPels  );
        sizeWindow.cy = min( sizeWindow.cy, ScreenDeviceInfo.iHeightinPels );
        }

    ptNew.x = max( ptNew.x, 0 );
    ptNew.y = max( ptNew.y, 0 );

    if (ptNew.x
    &&  ptNew.y)
        {
        if (ptNew.x >= ScreenDeviceInfo.iWidthinPels)
            ptNew.x  = ScreenDeviceInfo.iWidthinPels - sizeWindow.cx;

        if (ptNew.y >= ScreenDeviceInfo.iHeightinPels)
            ptNew.y  = ScreenDeviceInfo.iHeightinPels - sizeWindow.cy;
        }

    return ptNew;
    }

/***************************************************************************/

void CPBApp::WinHelp( DWORD dwData, UINT nCmd /* = HELP_CONTEXT */ )
    {
    // This app has been converted to use HtmlHelp.  This is a safety to prevent someone
    // from accidentally adding WinHelp calls for proceedural help
    ASSERT( (nCmd != HELP_FINDER) && (nCmd != HELP_INDEX) && (nCmd != HELP_CONTENTS) );

    CWinApp::WinHelp( dwData, nCmd );
    }

/***************************************************************************/

BOOL CPBApp::OnIdle( LONG lCount )
    {
    if (m_bHidden)
        return CWinApp::OnIdle( lCount );

    if (! lCount)
        {
        if (CheckForEmergency())
            {
            TryToFreeMemory();
            WarnUserOfEmergency();
            }
        if (m_nFileErrorCause != CFileException::none && m_pMainWnd)
            {
            CWnd* pWnd = AfxGetMainWnd();

            pWnd->PostMessage( WM_USER + 1001 );
            }
        }
    extern void IdleImage();

    IdleImage();

    return CWinApp::OnIdle(lCount) || lCount <= 4;
    }

/***************************************************************************/
// Map a file error code to a string id.

struct FERRID
    {
    int ferr;
    int ids;
    } mpidsferr[] =
    {
        { ferrIllformedGroup,    IDS_ERROR_BOGUSFILE    },
        { ferrReadFailed,        IDS_ERROR_BOGUSFILE    }, // error while reading a file or file corupt
        { ferrIllformedFile,     IDS_ERROR_BOGUSFILE    }, // not a valid palette file or zero length pcx file
        { ferrCantProcNewExeHdr, IDS_ERROR_EXE_HDR      },
        { ferrCantProcOldExeHdr, IDS_ERROR_EXE_HDR      },
        { ferrBadMagicNewExe,    IDS_ERROR_EXE_HDRMZ    },
        { ferrBadMagicOldExe,    IDS_ERROR_EXE_HDRMZ    },
        { ferrNotWindowsExe,     IDS_ERROR_EXE_HDRNW    },
        { ferrExeWinVer3,        IDS_ERROR_EXE_HDRWV    },
        { ferrNotValidRc,        IDS_ERROR_NOTVALID_RC  },
        { ferrNotValidExe,       IDS_ERROR_NOTVALID_EXE },
        { ferrNotValidRes,       IDS_ERROR_NOTVALID_RES },
        { ferrNotValidBmp,       IDS_ERROR_NOTVALID_BMP }, // invalid bitmap
        { ferrNotValidIco,       IDS_ERROR_NOTVALID_ICO },
        { ferrNotValidCur,       IDS_ERROR_NOTVALID_CUR },
        { ferrRcInvalidExt,      IDS_ERROR_RCPROB       },
        { ferrFileAlreadyOpen,   IDS_ERROR_COMPEX       },
        { ferrExeTooLarge,       IDS_ERROR_EXE_ALIGN    },
        { ferrCantCopyOldToNew,  IDS_ERROR_EXE_SAVE     },
        { ferrReadLoad,          IDS_ERROR_READLOAD     },
        { ferrExeAlloc,          IDS_ERROR_EXE_ALLOC    },
        { ferrExeInUse,          IDS_ERROR_EXE_INUSE    },
        { ferrExeEmpty,          IDS_ERROR_EXE_EMPTY    },
        { ferrGroup,             IDS_ERROR_GROUP        },
        { ferrResSave,           IDS_ERROR_RES_SAVE     },
        { ferrSaveOverOpen,      IDS_ERROR_SAVEOVEROPEN },
        { ferrSaveOverReadOnly,  IDS_ERROR_SAVERO       },
        { ferrCantDetermineType, IDS_ERROR_WHAAAT       }, // bad pcx file
        { ferrSameName,          IDS_ERROR_SAMENAME     },
        { ferrSaveAborted,       IDS_ERROR_SAVE_ABORTED },
        { ferrLooksLikeNtRes,    IDS_ERROR_NT_RES       },
        { ferrCantSaveReadOnly,  IDS_ERROR_CANT_SAVERO  }, // trying to save over a read only file
    };

int IdsFromFerr(int ferr)
    {
    if (ferr < ferrFirst)
        return IDS_ERROR_FILE + ferr; // was an exception cause

    for (int i = 0; i < sizeof (mpidsferr) / sizeof (FERRID); i++)
        {
        if (mpidsferr[i].ferr == ferr)
            return mpidsferr[i].ids;
        }

    ASSERT(FALSE); // You forgot to stick an entry in the above table!
    return 0;
    }

/***************************************************************************/
// Display a message box informing the user of a file related exception.
// The format of the box is something like:
//
//     <file name>
//     <operation failed>
//     <reason>
//
// <file name> describes what file has the problem, <operation files>
// indicated what kind of thing failed (e.g. "Cannot save file"), and
// <reason> provides more information about why the operation failed
// (e.g. "Disk full").
//
// All the parameters must have been setup previously via a call to
// CWinApp::SetFileError().
//
void CPBApp::FileErrorMessageBox( void )
    {
    static BOOL bInUse = FALSE;

    if (m_nFileErrorCause != CFileException::none && ! bInUse)
        {
        bInUse = TRUE;

        CString strOperation;
        VERIFY( strOperation.LoadString( m_uOperation ) );

        CString strReason;
        VERIFY( strReason.LoadString( IdsFromFerr( m_nFileErrorCause ) ) );


        CString strMsg;

        if (m_sLastFile.IsEmpty())
            strMsg = strOperation + TEXT("\n") + strReason;
        else
            strMsg = m_sLastFile + TEXT("\n") + strOperation + TEXT("\n") + strReason;
        AfxMessageBox( strMsg, MB_TASKMODAL | MB_OK | MB_ICONEXCLAMATION );

        bInUse = FALSE;
        }
    m_nFileErrorCause = CFileException::none;
    }

/***************************************************************************/

void CPBApp::SetFileError( UINT uOperation, int nCause, LPCTSTR lpszFile )
    {
    m_nFileErrorCause = nCause;
    m_uOperation      = uOperation;

    if (lpszFile)
        m_sLastFile = lpszFile;
    }

/***************************************************************************/
//  Memory/resource emergency handling functions

void CPBApp::SetMemoryEmergency(BOOL bFailed)
    {
    TRACE(TEXT("Memory emergency!\n"));

    m_wEmergencyFlags |= memoryEmergency | warnEmergency;

    if (bFailed)
        m_wEmergencyFlags |= failedEmergency;
    }

/***************************************************************************/

void CPBApp::SetGdiEmergency(BOOL bFailed)
    {
    TRACE(TEXT("GDI emergency!\n"));

    m_wEmergencyFlags |= gdiEmergency | warnEmergency;

    if (bFailed)
        m_wEmergencyFlags |= failedEmergency;
    }

/***************************************************************************/

void CPBApp::SetUserEmergency(BOOL bFailed)
    {
    TRACE(TEXT("USER emergency!\n"));

    m_wEmergencyFlags |= userEmergency | warnEmergency;

    if (bFailed)
        m_wEmergencyFlags |= failedEmergency;
    }

/***************************************************************************/

void CPBApp::WarnUserOfEmergency()
    {
    if ((m_wEmergencyFlags & warnEmergency) == 0)
        {
        // We have nothing to warn the user about!
        return;
        }

    if ((m_wEmergencyFlags & failedEmergency) == 0 &&
         GetTickCount() < m_tickLastWarning + ticksBetweenWarnings)
        {
        // We've warned the user recently, so keep quiet for now...
        // The warning flag is cleared so we don't just warn the
        // user after the delay is up unless another emergency
        // occurs AFTER then...

        m_wEmergencyFlags &= ~warnEmergency;
        return;
        }

    // Don't go invoking message boxes when we're not the active app!
    if (! m_bActiveApp)
        return;

    const TCHAR* szMsg = (m_wEmergencyFlags & failedEmergency) != 0 ?
        m_strEmergencyNoMem : m_strEmergencyLowMem;

    if (AfxMessageBox(szMsg, MB_TASKMODAL | MB_OK | MB_ICONSTOP) == IDOK)
        {
        m_wEmergencyFlags &= ~(warnEmergency | failedEmergency);
        m_tickLastWarning = GetTickCount();
        }
    #ifdef _DEBUG
    else
        TRACE(TEXT("Emergency warning message box failed!\n"));
    #endif

    // Update status bar warning message...
    if ( ::IsWindow( ((CPBFrame*)m_pMainWnd)->m_statBar.m_hWnd ) )
        ((CPBFrame*)m_pMainWnd)->m_statBar.Invalidate(FALSE);
    }

/***************************************************************************/

void CPBApp::TryToFreeMemory()
    {
    // We are in a memory/resource emergency state!  Add things to this
    // function to flush caches and do anything else to free up memory
    // we don't really need to be using right now...
    if (m_wEmergencyFlags & memoryEmergency)
        {
        CPBDoc* pDoc = (CPBDoc*)((CFrameWnd*)AfxGetMainWnd())->GetActiveDocument();

        if (pDoc && pDoc->m_pBitmapObj && ! pDoc->m_pBitmapObj->IsDirty()
                                       &&   pDoc->m_pBitmapObj->m_lpvThing)
            pDoc->m_pBitmapObj->Free();
        }

    if (m_wEmergencyFlags & gdiEmergency)
        {
//      theUndo.Flush();
        ResetSysBrushes();
        }
    }

/***************************************************************************/

// App command to run the dialog
void CPBApp::OnAppAbout()
    {
    CString sTitle;
    CString sBrag;
    HICON   hIcon = LoadIcon( ID_MAINFRAME );

    sTitle.LoadString( AFX_IDS_APP_TITLE );
    sBrag.LoadString( IDS_PerContractSoDontChange );

    ShellAbout( AfxGetMainWnd()->GetSafeHwnd(), sTitle, sBrag, hIcon );

    if (hIcon != NULL)
        ::DestroyIcon( hIcon );
    }

/***************************************************************************/

void CPBApp::SetDeviceHandles(HANDLE hDevNames, HANDLE hDevMode)
{
        // The old ones should already be freed
        m_hDevNames = hDevNames;
        m_hDevMode = hDevMode;
}

/***************************************************************************/

#if 0 

class CFileOpenSaveDlg : public CFileDialog
    {
    public:

    BOOL m_bOpenFile;

    CFileOpenSaveDlg( BOOL bOpenFileDialog );

    virtual void OnLBSelChangedNotify( UINT nIDBox, UINT iCurSel, UINT nCode );

    DECLARE_MESSAGE_MAP()
    };

/***************************************************************************/

BEGIN_MESSAGE_MAP(CFileOpenSaveDlg, CFileDialog)
END_MESSAGE_MAP()

/***************************************************************************/

CFileOpenSaveDlg::CFileOpenSaveDlg( BOOL bOpenFileDialog )
                           :CFileDialog( bOpenFileDialog )
    {
    m_bOpenFile = bOpenFileDialog;
    }

/***************************************************************************/

void CFileOpenSaveDlg::OnLBSelChangedNotify( UINT nIDBox, UINT iCurSel, UINT nCode )
    {
    if (! m_bOpenFile && iCurSel <= 5 && nIDBox == cmb1
                                      &&  nCode == CD_LBSELCHANGE)
        {
        // change in the file type
        CWnd* pText = GetDlgItem( edt1 );
        CWnd* pType = GetDlgItem( cmb1 );
        CString sFname;
        CString sDfltExt;

        switch (iCurSel)
            {
#ifdef PCX_SUPPORT
            case 4:
                sDfltExt.LoadString( IDS_EXTENSION_PCX );
                break;
#endif
            case 5:
                sDfltExt.LoadString( IDS_EXTENSION_ICO );
                break;

            default:
                sDfltExt.LoadString( IDS_EXTENSION_BMP );
                break;
            }
        pText->GetWindowText( sFname );

        if (sDfltExt.CompareNoCase( GetExtension( sFname ) ))
            {
            sFname = StripExtension( sFname ) + sDfltExt;
            pText->SetWindowText( sFname );
            }
        }
    }

#endif //0

/***************************************************************************/

extern BOOL AFXAPI AfxFullPath( LPTSTR lpszPathOut, LPCTSTR lpszFileIn );

CDocument*
CPBApp::OpenDocumentFile(
    LPCTSTR lpszFileName
    )
{
    CancelToolMode(FALSE);

    TCHAR szPath[_MAX_PATH];

    AfxFullPath( szPath, lpszFileName );

    return(m_pDocManager->OpenDocumentFile(szPath));

//    CDocTemplate* pTemplate = (CDocTemplate*)m_templateList.GetHead();
//
//    ASSERT( pTemplate->IsKindOf( RUNTIME_CLASS( CDocTemplate ) ) );
//
//    return pTemplate->OpenDocumentFile( szPath );
}

void CancelToolMode(BOOL bSelectionCommand)
{
        if (bSelectionCommand)
        {
                // Check if a selection tool is the current one
                if ((CImgTool::GetCurrentID() == IDMB_PICKTOOL)
                        || (CImgTool::GetCurrentID() == IDMB_PICKRGNTOOL))
                {
                        // Don't try canceling the mode, since the command works on a
                        // selection
                        return;
                }
        }

        // Just select the current tool again to reset everything
        CImgTool *pImgTool = CImgTool::GetCurrent();
        if (pImgTool)
        {
                pImgTool->Select();
        }
}

/***************************************************************************/
// CPBApp commands

void CPBApp::OnFileNew()
    {
    CancelToolMode(FALSE);

    CWinApp::OnFileNew();
    }

void CPBApp::OnFileOpen()
    {
    CancelToolMode(FALSE);

    // prompt the user (with all document templates)
    CString newName;

    int iColor = 0;

    if (! DoPromptFileName( newName, AFX_IDS_OPENFILE,
                                     OFN_HIDEREADONLY | OFN_FILEMUSTEXIST,
                                     TRUE, iColor, FALSE ))
        return; // open cancelled

#ifdef PCX_SUPPORT
    m_bPCXfile = (iColor == 4);
#endif

    if (OpenDocumentFile( newName )==NULL)
    {
       // attempt to open a file failed, so make sure any new doc just
       // created in the process gets destroyed
       POSITION tPos = GetFirstDocTemplatePosition();
       CDocTemplate* pTemplate = GetNextDocTemplate(tPos);
       POSITION dPos = pTemplate->GetFirstDocPosition ();
       CPBDoc *pDoc= (CPBDoc *)(pTemplate->GetNextDoc(dPos));

       if (pDoc->m_pBitmapObjNew)
       {
          delete pDoc->m_pBitmapObjNew;
          pDoc->m_pBitmapObjNew =NULL;
       }
       OnFileNew(); // then start anew...
    }
 }

/****************************************************************************/
// prompt for file name - used for open and save as

BOOL CPBApp::DoPromptFileName( CString& fileName, UINT nIDSTitle, DWORD lFlags,
                               BOOL bOpenFileDialog, int& iColors, BOOL bOnlyBmp )
    {
    COpenFileName dlgFile( bOpenFileDialog );

    ASSERT(dlgFile.m_pofn);

    if (!dlgFile.m_pofn)
        return FALSE;

    CString title;

    VERIFY( title.LoadString( nIDSTitle ) );

    lFlags |= OFN_EXPLORER;

    if (!bOpenFileDialog)
        lFlags |= OFN_OVERWRITEPROMPT;

    dlgFile.m_pofn->Flags |= lFlags;
    dlgFile.m_pofn->Flags &= ~OFN_SHOWHELP;

    CString strFilter;
//    CString strDefault;

    CDocTemplate* pTemplate = NULL;
    POSITION pos = m_pDocManager->GetFirstDocTemplatePosition();

    if (pos != NULL)
        pTemplate = m_pDocManager->GetNextDocTemplate(pos);

    CString strFilterExt;
    CString strFilterName;
    CString strAllPictureFiles;

    ASSERT(pTemplate != NULL);

    pTemplate->GetDocString( strFilterExt , CDocTemplate::filterExt  );
    pTemplate->GetDocString( strFilterName, CDocTemplate::filterName );

    ASSERT( strFilterExt[0] == TEXT('.') );

    // set the default extension
//    strDefault = ((const TCHAR*)strFilterExt) + 1;  // skip the '.'
//    dlgFile.m_pofn->nFilterIndex = iColors + 1; // 1 based number
    dlgFile.m_pofn->lpstrDefExt = ((LPCTSTR)strFilterExt) + 1; // skip the '.'

    if (bOpenFileDialog)
        {
        // add to filter
        strFilter = strFilterName;
        strFilter += (TCHAR)TEXT('\0');       // next string please
        strFilter += TEXT("*") + strFilterExt;
        VERIFY(strFilterExt.LoadString(IDS_EXTENSION_DIB));
        strFilter += TEXT(";*") + strFilterExt;
        strAllPictureFiles += TEXT(";*") + strFilterExt;
        VERIFY(strFilterExt.LoadString(IDS_EXTENSION_BMP));
        strAllPictureFiles += TEXT(";*") + strFilterExt;
        VERIFY(strFilterExt.LoadString(IDS_EXTENSION_RLE));
        strFilter += TEXT(";*") + strFilterExt;
        strFilter += (TCHAR)TEXT('\0');       // next string please

        dlgFile.m_pofn->nMaxCustFilter++;

        }
      else
      {
        for (int i = IDS_BMP_MONO; i <= IDS_BMP_TRUECOLOR; i++)
        {
           strFilterName.LoadString( i );

          // add to filter
          strFilter += strFilterName;

          strFilter += (char)'\0';       // next string please
          strFilter += "*" + strFilterExt;
          strFilter += (char)'\0';       // next string please

          dlgFile.m_pofn->nMaxCustFilter++;
        }

      }

      //
      // get list of all installed filters and add those to the list...
      //
      TCHAR name[128];
      TCHAR ext[sizeof("jpg;*.jpeg") + 1];
      BOOL bImageAPI;

      for (int i=0, j=0; !bOnlyBmp && GetInstalledFilters(bOpenFileDialog,
            i, name, sizeof(name), ext, sizeof(ext), NULL, 0, bImageAPI); i++)
            {
            if (!bImageAPI)
            {
               continue;
            }
            if (ext[0] == 0 || name[0] == 0)
                continue;

            if (lstrlen(ext) > 3)
                continue;

            // dont show these, we already handle these
            if (lstrcmpi(ext,TEXT("bmp")) == 0 ||
                lstrcmpi(ext,TEXT("dib")) == 0 ||
                lstrcmpi(ext,TEXT("rle")) == 0)
                continue;
            #ifndef GIF_SUPPORT
            if (lstrcmpi(ext, TEXT("gif") == 0)
            {
               continue;
            }

            #endif
#if 0 // only use known good filters
            if (!g_bShowAllFiles &&
                (GetKeyState(VK_SHIFT) & 0x8000) == 0 &&
                lstrcmpi(ext,TEXT("pcx")) != 0)
                continue;
#endif
            // save a list of available filter types
            if (lstrcmpi(ext,_T("gif")) == 0)
            {
               m_iflFltType[j++] = IFLT_GIF;
            }
            else if (lstrcmpi(ext,_T("jpg")) == 0)
            {
               m_iflFltType[j++] = IFLT_JPEG;
               _tcscat (ext, _T(";*.jpeg"));
            }
#ifdef SUPPORT_ALL_FILTERS
            else if (lstrcmpi(ext,_T("png")) == 0)
            {
#ifdef PNG_SUPPORT
               m_iflFltType[j++] = IFLT_PNG;
#else
               continue;
#endif // PNG_SUPPORT
            }

            else if (lstrcmpi(ext,_T("pcd")) == 0)
            {
               m_iflFltType[j++] = IFLT_PCD;
            }
            else if (lstrcmpi(ext,_T("pic")) == 0)
            {
               m_iflFltType[j++] = IFLT_PICT;
               _tcscat(ext, _T(";*.pict"));
            }
            else if (lstrcmpi(ext,_T("tga")) == 0)
            {
               m_iflFltType[j++] = IFLT_TGA;
            }
            else if (lstrcmpi(ext,_T("tif")) == 0)
            {
               m_iflFltType[j++] = IFLT_TIFF;
               _tcscat(ext, _T(";*.tiff"));
            }
            else
            {
               m_iflFltType[j++] = IFLT_UNKNOWN;
            }
#else
            else continue;
#endif


            // add to filter
            strFilter += name;
            strFilter += TEXT(" ( *.");
            strFilter += ext;
            strFilter += TEXT(" )");
            strFilter += (TCHAR)TEXT('\0');       // next string please
            strFilter += TEXT("*.");
            strFilter += ext;
            strFilter += (TCHAR)TEXT('\0');       // next string please

            strAllPictureFiles = strAllPictureFiles + _T(";*.")+ext;
            dlgFile.m_pofn->nMaxCustFilter++;
            }

    if (!bOnlyBmp && bOpenFileDialog)
    {
       // append "All Picture Files" only if opening a file
       VERIFY(strFilterName.LoadString(IDS_TYPE_ALLPICTURES));
       strFilter+= strFilterName;
       strFilter += (TCHAR)_T('\0');
       strFilter += strAllPictureFiles;
       strFilter += (TCHAR)_T('\0');
       dlgFile.m_pofn->nMaxCustFilter++;

       // append the  "*.*" filter only if "Open"ing a file
        VERIFY( strFilterName.LoadString( IDS_TYPE_ALLFILES ) );

        strFilter += strFilterName;
        strFilter += (TCHAR)TEXT('\0');        // next string please
        strFilter += TEXT("*.*");
        strFilter += (TCHAR)TEXT('\0');        // last string

        dlgFile.m_pofn->nMaxCustFilter++;

    }

    // prompt the user with the appropriate filter pre-selected
    if (bOpenFileDialog)
    {
       dlgFile.m_pofn->nFilterIndex = m_nFilterInIdx;
    }
    else
    {
       DWORD dwIndex;
       if (m_nFltTypeUsed != IFLT_UNKNOWN &&
                        (dwIndex = GetFilterIndex(m_nFltTypeUsed))) // has an export filter?
            dlgFile.m_pofn->nFilterIndex = dwIndex + 4; // skip the first 4 BMP types
        else if (m_nFilterOutIdx >= 4)
            dlgFile.m_pofn->nFilterIndex = m_nFilterOutIdx;
        else
            dlgFile.m_pofn->nFilterIndex = iColors + 1; // 1 based number

    }
    dlgFile.m_pofn->lpstrFilter = strFilter;
    dlgFile.m_pofn->hwndOwner   = AfxGetMainWnd()->GetSafeHwnd();
    dlgFile.m_pofn->hInstance   = AfxGetResourceHandle();
    dlgFile.m_pofn->lpstrTitle  = title;
    dlgFile.m_pofn->lpstrFile   = fileName.GetBuffer(_MAX_PATH);
    dlgFile.m_pofn->nMaxFile    = _MAX_PATH;
    TCHAR szInitPath[MAX_PATH];
    ::LoadString (GetModuleHandle (NULL), AFX_IDS_UNTITLED, szInitPath, MAX_PATH);
    //
    // Look in "My Documents" on NT 5, Win98, and later.
    //
    if (!theApp.GetLastFile() || !*(theApp.GetLastFile()))
    {
       static SHSPECPATH pfnSpecialPath = NULL;
       if (!pfnSpecialPath)
       {

          #ifdef UNICODE
          pfnSpecialPath = (SHSPECPATH)GetProcAddress (
                                           GetModuleHandle(TEXT("shell32.dll")),
                                           "SHGetSpecialFolderPathW");
          #else
          pfnSpecialPath = (SHSPECPATH)GetProcAddress (
                                           GetModuleHandle(TEXT("shell32.dll")),
                                           "SHGetSpecialFolderPathA");
          #endif //UNICODE

       }
       if (pfnSpecialPath)
       {
          (pfnSpecialPath)(NULL, szInitPath, CSIDL_MYPICTURES, FALSE);
          dlgFile.m_pofn->lpstrInitialDir = szInitPath;
       }

    }

    BOOL bRet = dlgFile.DoModal() == IDOK? TRUE : FALSE;
    fileName.ReleaseBuffer();

    // keep track of the filter selected by the user
    if (bOpenFileDialog)
        m_nFilterInIdx = dlgFile.m_pofn->nFilterIndex;
    else
        m_nFilterOutIdx = dlgFile.m_pofn->nFilterIndex;

    iColors = (int)dlgFile.m_pofn->nFilterIndex - 1;

    CString sExt = dlgFile.m_pofn->lpstrFile + dlgFile.m_pofn->nFileExtension;

#ifdef ICO_SUPPORT
    if (! bOpenFileDialog && dlgFile.m_pofn->nFileExtension)
        // did the user try to sneak a icon extension past us
        if (! sExt.CompareNoCase( ((const TCHAR *)strFilterExt) + 1 ))
            iColors = 5;
#endif


    return bRet;
    }

DWORD CPBApp::GetFilterIndex( int nFltType )
{
    for (int i = 0; i < nFilters; i++)
        if (m_iflFltType[i] == nFltType)
                        return i+1;

        return 0;
}

// fix the file extension based on export filter selected - used for save as

void CPBApp::FixExtension( CString& fileName, int iflFltType )
{
        CString sDfltExt;

        switch (iflFltType)
        {
                case IFLT_GIF:
                        VERIFY(sDfltExt.LoadString( IDS_EXTENSION_GIF ));
                        break;

                case IFLT_JPEG:
                        VERIFY(sDfltExt.LoadString( IDS_EXTENSION_JPEG ));
                        break;

                case IFLT_PCD:
                        VERIFY(sDfltExt.LoadString( IDS_EXTENSION_PCD ));
                        break;


                case IFLT_PCX:
                        VERIFY(sDfltExt.LoadString( IDS_EXTENSION_PCX ));
                        break;


                case IFLT_PICT:
                        VERIFY(sDfltExt.LoadString( IDS_EXTENSION_PICT ));
                        break;
#ifdef PNG_SUPPORT
                case IFLT_PNG:
                        VERIFY(sDfltExt.LoadString( IDS_EXTENSION_PNG ));
                        break;
#endif // PNG_SUPPORT
                case IFLT_TGA:
                        VERIFY(sDfltExt.LoadString( IDS_EXTENSION_TGA ));
                        break;

                case IFLT_TIFF:
                        VERIFY(sDfltExt.LoadString( IDS_EXTENSION_TIFF ));
                        break;

                case IFLT_UNKNOWN:      // unknown or unsupported file type
                default:
                        VERIFY(sDfltExt.LoadString( IDS_EXTENSION_BMP ));
                        break;
        }

        if (sDfltExt.CompareNoCase( GetExtension( (LPCTSTR)fileName ) ))
        {
                fileName = StripExtension( fileName ) + sDfltExt;
        }
}

/***************************************************************************/

// Mostly stolen from MFC
// I made no attempt to strip out stuff I do not actually use
// I just modified this so it used short module file name
//

//////////////////////////////////////////////////////////////////////////////
// data for UpdateRegistry functionality

// %1 - class ID
// %2 - class name
// %3 - executable path
// %4 - short type name
// %5 - long type name
// %6 - long application name
// %7 - icon index

static const TCHAR sz00[] = TEXT("%2\0") TEXT("%5");
static const TCHAR sz01[] = TEXT("%2\\CLSID\0") TEXT("%1");
static const TCHAR sz02[] = TEXT("%2\\Insertable\0") TEXT("");
static const TCHAR sz03[] = TEXT("%2\\protocol\\StdFileEditing\\verb\\0\0") TEXT("&Edit");
static const TCHAR sz04[] = TEXT("%2\\protocol\\StdFileEditing\\server\0") TEXT("%3");
static const TCHAR sz05[] = TEXT("CLSID\\%1\0") TEXT("%5");
static const TCHAR sz06[] = TEXT("CLSID\\%1\\ProgID\0") TEXT("%2");
#ifndef _USRDLL
static const TCHAR sz07[] = TEXT("CLSID\\%1\\InprocHandler32\0") TEXT("ole32.dll");
static const TCHAR sz08[] = TEXT("CLSID\\%1\\LocalServer32\0") TEXT("%3");
#else
static const TCHAR sz07[] = TEXT("\0") TEXT("");
static const TCHAR sz08[] = TEXT("CLSID\\%1\\InProcServer32\0") TEXT("%3");
#endif
static const TCHAR sz09[] = TEXT("CLSID\\%1\\Verb\\0\0") TEXT("&Edit,0,2");
static const TCHAR sz10[] = TEXT("CLSID\\%1\\Verb\\1\0") TEXT("&Open,0,2");
static const TCHAR sz11[] = TEXT("CLSID\\%1\\Insertable\0") TEXT("");
static const TCHAR sz12[] = TEXT("CLSID\\%1\\AuxUserType\\2\0") TEXT("%4");
static const TCHAR sz13[] = TEXT("CLSID\\%1\\AuxUserType\\3\0") TEXT("%6");
static const TCHAR sz14[] = TEXT("CLSID\\%1\\DefaultIcon\0") TEXT("%3,%7");
static const TCHAR sz15[] = TEXT("CLSID\\%1\\MiscStatus\0") TEXT("32");

// registration for OAT_INPLACE_SERVER
static const LPCTSTR rglpszInPlaceRegister[] =
{
        sz00, sz02, sz03, sz05, sz09, sz10, sz11, sz12,
        sz13, sz15, NULL
};

// registration for OAT_SERVER
static const LPCTSTR rglpszServerRegister[] =
{
        sz00, sz02, sz03, sz05, sz09, sz11, sz12,
        sz13, sz15, NULL
};
// overwrite entries for OAT_SERVER & OAT_INPLACE_SERVER
static const LPCTSTR rglpszServerOverwrite[] =
{
        sz01, sz04, sz06, sz07, sz08, sz14, NULL
};

// registration for OAT_CONTAINER
static const LPCTSTR rglpszContainerRegister[] =
{
        sz00, sz05, NULL
};
// overwrite entries for OAT_CONTAINER
static const LPCTSTR rglpszContainerOverwrite[] =
{
        sz01, sz06, sz07, sz08, sz14, NULL
};

// registration for OAT_DISPATCH_OBJECT
static const LPCTSTR rglpszDispatchRegister[] =
{
        sz00, sz05, NULL
};
// overwrite entries for OAT_CONTAINER
static const LPCTSTR rglpszDispatchOverwrite[] =
{
        sz01, sz06, sz08, NULL
};

struct STANDARD_ENTRY
{
        const LPCTSTR* rglpszRegister;
        const LPCTSTR* rglpszOverwrite;
};

static const STANDARD_ENTRY rgStdEntries[] =
{
        { rglpszInPlaceRegister, rglpszServerOverwrite },
        { rglpszServerRegister, rglpszServerOverwrite },
        { rglpszContainerRegister, rglpszContainerOverwrite },
        { rglpszDispatchRegister, rglpszDispatchOverwrite }
};

/////////////////////////////////////////////////////////////////////////////
// Special registration for apps that wish not to use REGLOAD

BOOL AFXAPI PBOleRegisterServerClass(
        REFCLSID clsid, LPCTSTR lpszClassName,
        LPCTSTR lpszShortTypeName, LPCTSTR lpszLongTypeName,
        OLE_APPTYPE nAppType, LPCTSTR* rglpszRegister, LPCTSTR* rglpszOverwrite)
{
        ASSERT(AfxIsValidString(lpszClassName));
        ASSERT(AfxIsValidString(lpszShortTypeName));
        ASSERT(*lpszShortTypeName != 0);
        ASSERT(AfxIsValidString(lpszLongTypeName));
        ASSERT(*lpszLongTypeName != 0);
        ASSERT(nAppType == OAT_INPLACE_SERVER || nAppType == OAT_SERVER ||
                nAppType == OAT_CONTAINER || nAppType == OAT_DISPATCH_OBJECT);

        // use standard registration entries if non given
        if (rglpszRegister == NULL)
                rglpszRegister = (LPCTSTR*)rgStdEntries[nAppType].rglpszRegister;
        if (rglpszOverwrite == NULL)
                rglpszOverwrite = (LPCTSTR*)rgStdEntries[nAppType].rglpszOverwrite;

        LPTSTR rglpszSymbols[7];
                // 0 - class ID
                // 1 - class name
                // 2 - executable path
                // 3 - short type name
                // 4 - long type name
                // 5 - long application name
                // 6 - icon index

        // convert the CLSID to a string
        LPWSTR lpszClassID;
        ::StringFromCLSID(clsid, &lpszClassID);
        if (lpszClassID == NULL)
        {
                TRACE0("Warning: StringFromCLSID failed in AfxOleRegisterServerName --\n");
                TRACE0("\tperhaps AfxOleInit() has not been called.\n");
                return FALSE;
        }
        #ifdef UNICODE
        rglpszSymbols[0] = lpszClassID;
        #else
        int cc = WideCharToMultiByte (CP_ACP, 0, lpszClassID, -1,
                                      (LPSTR)&rglpszSymbols[0], 0,
                                      NULL, NULL);
        rglpszSymbols[0] = (LPSTR)new char[cc];
        WideCharToMultiByte (CP_ACP, 0, lpszClassID, -1,
                             rglpszSymbols[0], cc,
                             NULL, NULL);

        #endif // UNICODE
        rglpszSymbols[1] = (LPTSTR)lpszClassName;

        // get path name to server
        TCHAR szPathName[_MAX_PATH];
        LPTSTR pszTemp = szPathName;
        ::GetShortModuleFileName(AfxGetInstanceHandle(), pszTemp, _MAX_PATH);
        rglpszSymbols[2] = szPathName;

        // fill in rest of symbols
        rglpszSymbols[3] = (LPTSTR)lpszShortTypeName;
        rglpszSymbols[4] = (LPTSTR)lpszLongTypeName;
        rglpszSymbols[5] = (LPTSTR)AfxGetAppName(); // will usually be long, readable name

        LPCTSTR lpszIconIndex;
        HICON hIcon = ExtractIcon(AfxGetInstanceHandle(), szPathName, 1);
        if (hIcon != NULL)
        {
                lpszIconIndex = TEXT("1");
                DestroyIcon(hIcon);
        }
        else
        {
                lpszIconIndex = TEXT("0");
        }
        rglpszSymbols[6] = (LPTSTR)lpszIconIndex;

        // update the registry with helper function
        BOOL bResult;
        bResult = AfxOleRegisterHelper(rglpszRegister, (LPCTSTR*)rglpszSymbols, 7, FALSE);
        if (bResult && rglpszOverwrite != NULL)
                bResult = AfxOleRegisterHelper(rglpszOverwrite, (LPCTSTR*)rglpszSymbols, 7, TRUE);

        // free memory for class ID
        ASSERT(lpszClassID != NULL);
        AfxFreeTaskMem(lpszClassID);
        #ifndef UNICODE
        delete[](LPSTR)rglpszSymbols[0];
        #endif
        return bResult;
}

void CPBTemplateServer::UpdateRegistry(OLE_APPTYPE nAppType,
        LPCTSTR* rglpszRegister, LPCTSTR* rglpszOverwrite)
{
        ASSERT(m_pDocTemplate != NULL);

        // get registration info from doc template string
        CString strServerName;
        CString strLocalServerName;
        CString strLocalShortName;

        if (!m_pDocTemplate->GetDocString(strServerName,
           CDocTemplate::regFileTypeId) || strServerName.IsEmpty())
        {
                TRACE0("Error: not enough information in DocTemplate to register OLE server.\n");
                return;
        }
        if (!m_pDocTemplate->GetDocString(strLocalServerName,
           CDocTemplate::regFileTypeName))
                strLocalServerName = strServerName;     // use non-localized name
        if (!m_pDocTemplate->GetDocString(strLocalShortName,
                CDocTemplate::fileNewName))
                strLocalShortName = strLocalServerName; // use long name

        ASSERT(strServerName.Find(TEXT(' ')) == -1);  // no spaces allowed

        // place entries in system registry
        if (!PBOleRegisterServerClass(m_clsid, strServerName, strLocalShortName,
                strLocalServerName, nAppType, rglpszRegister, rglpszOverwrite))
        {
                // not fatal (don't fail just warn)
                TRACE0("mspaint: Unable to register server class.\n");
        }
}
