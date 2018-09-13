


#include    <windows.h>
#include    <windowsx.h>
#include    <stdarg.h>
#include    "common.h"
#include    "clipsrv.h"
#include    "clipfile.h"
#include    "callback.h"
#include    "debugout.h"







static HANDLE   hmutexClp;      // for syncing open and close of clipboard

static BOOL     fAnythingToRender = FALSE;
static BOOL     fService = TRUE;
static BOOL     fServiceStopped = FALSE;
static TCHAR    szClass[] = TEXT("ClipSrvWClass");
static TCHAR    szServiceName[] = TEXT("Clipbook Service");

static TCHAR    wsbuf[128];




DWORD           idInst = 0;
HINSTANCE       hInst;
HWND            hwndApp;
HSZ             hszAppName = 0L;
TCHAR           szTopic[MAX_TOPIC] = TEXT("ClipData");
TCHAR           szServer[MAX_TOPIC] = TEXT("ClipSrv");
TCHAR           szExec[MAX_EXEC] = TEXT("");

TCHAR           szUpdateName[MAX_CLPSHRNAME+1] = TEXT("");


UINT            cf_preview;
ShrInfo         *SIHead = NULL;






// Service status stuff
static SERVICE_STATUS_HANDLE hService;
static SERVICE_STATUS srvstatus =
   {
   SERVICE_WIN32_OWN_PROCESS,
   SERVICE_START_PENDING,
   SERVICE_ACCEPT_STOP,
   NO_ERROR,
   0L,
   1,
   200
   };



#if DEBUG
HKEY hkeyRoot;
HKEY hkeyClp;
#endif



void ClipSrvHandler (DWORD);







/////////////////////////////////////////////////////////////////////////
//
// "main" function... just calls StartServiceCtrlDispatcher.
//
//
/////////////////////////////////////////////////////////////////////////



/*
 *      main
 */

void _cdecl main(
    int     argc,
    char    **argv)
{
SERVICE_TABLE_ENTRY srvtabl[] = {{szServiceName, ClipSrvMain},
                                 {NULL,          NULL}};




#if DEBUG
    DeleteFile("C:\\CLIPSRV.OUT");
#endif


    if (argv[1] && !lstrcmpi(argv[1], "-debug"))
        {
        fService = FALSE;
        ClipSrvMain(argc, argv);
        }
    else
        {
        StartServiceCtrlDispatcher(srvtabl);
        }
}





/*
 *      ClipSrvMain
 */

void ClipSrvMain(
    DWORD   argc,
    LPSTR   *argv)
{
MSG msg;

    if (fService)
        {
        hService = RegisterServiceCtrlHandler(szServiceName, ClipSrvHandler);
        }


    if (0L != hService || FALSE == fService)
        {
        if (fService)
            {
            // Tell SCM that we're starting
            SetServiceStatus(hService, &srvstatus);
            }

        hInst = GetModuleHandle(TEXT("CLIPSRV.EXE"));

        // Perform initializations
        if (InitApplication(hInst, &srvstatus))
            {
            if (fService)
               {
               // Tell SCM we've started OK
               srvstatus.dwCurrentState = SERVICE_RUNNING;
               SetServiceStatus(hService, &srvstatus);

               PINFO(TEXT("Told system we're running\r\n"));
               }

            // Process messages
            while (GetMessage(&msg, NULL, 0, 0))
               {
               TranslateMessage(&msg);
               DispatchMessage(&msg);
               }

            UnregisterClass(szClass, hInst);

            if (fService && !fServiceStopped)
               {
               fServiceStopped = TRUE;
               srvstatus.dwCurrentState = SERVICE_STOPPED;
               SetServiceStatus(hService, &srvstatus);
               }

            if (NULL != hmutexClp)
               {
               CloseHandle(hmutexClp);
               }
            }
        else
            {
            PERROR(TEXT("ClSrv: InitApplication failed!\r\n"));
            if (fService && !fServiceStopped)
                {
                fServiceStopped = TRUE;
                srvstatus.dwCurrentState = SERVICE_STOPPED;
                SetServiceStatus(hService, &srvstatus);
                }
            }
        }
}








/*
 *      ReportStatusToSDMgr
 *
 *  This function is called by the ServMainFunc() and
 *  by the ServCtrlHandler() to update the service's status
 *  to the Service Control Manager.
 */

BOOL ReportStatusToSCMgr (
    DWORD   dwCurrentState,
    DWORD   dwWin32ExitCode,
    DWORD   dwCheckPoint,
    DWORD   dwWaitHint)
{
BOOL fResult;


    /* disable control requests until service is started */
    if (dwCurrentState == SERVICE_START_PENDING)
        {
        srvstatus.dwControlsAccepted = 0;
        }
    else
        {
        srvstatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
        }

    /* These SERVICE_STATUS members set from parameters */

    srvstatus.dwCurrentState = dwCurrentState;
    srvstatus.dwWin32ExitCode = dwWin32ExitCode;
    srvstatus.dwCheckPoint = dwCheckPoint;

    srvstatus.dwWaitHint = dwWaitHint;

    /* Report status of service to Service Control Manager */

    if (!(fResult = SetServiceStatus(hService,&srvstatus)))
        {
        /* if error occurs, stop service */
        }

    return fResult;
}









/*
 *      ClipSrvHandler
 *
 *
 *  Purpose: Acts as the HANDLER_FUNCTION for the Clipbook service.
 *
 *  Parameters:
 *     fdwControl = Flags saying what action to take
 *
 *  Returns: Void (SetServiceStatus is used to set status)
 */

void ClipSrvHandler(
    DWORD   fdwControl)
{

    if (SERVICE_CONTROL_STOP == fdwControl)
       {
       PINFO(TEXT("Handler: stopping service\r\n"));
       srvstatus.dwCheckPoint = 0;
       srvstatus.dwCurrentState = SERVICE_STOP_PENDING;
       SetServiceStatus(hService, &srvstatus);

       SendMessage(hwndApp, WM_CLOSE, 0, 0);

       if (!fServiceStopped) {
           fServiceStopped = TRUE;
           srvstatus.dwCurrentState = SERVICE_STOPPED;
           SetServiceStatus(hService, &srvstatus);
       }
       PINFO(TEXT("Handler: Service stopped\r\n"));
       }
    else
       {
       // Unhandled control request.. just keep running.
       srvstatus.dwCurrentState = SERVICE_RUNNING;
       srvstatus.dwWin32ExitCode = NO_ERROR;
       SetServiceStatus(hService, &srvstatus);
       }
    return;

}








/*
 *      InitApplication
 *
 *
 *  Purpose: Application initialization, including creation of a window
 *     to do DDE with, getting settings out of the registry, and starting
 *     up DDE.
 *
 *  Parameters:
 *     hInstance - Application instance.
 *     psrvstatus - Pointer to a SERVICE_STATUS struct. We update the
 *           dwCheckPoint member of this struct and call SetServiceStatus,
 *           so the system knows that we didn't die.
 *
 *  Returns: True on OK, false on fail.
 */

BOOL InitApplication(
    HINSTANCE       hInstance,
    SERVICE_STATUS  *psrvstatus)
{
WNDCLASS        wc;
#if DEBUG
DWORD           dwKeyStatus;
#endif
HWINSTA hwinsta;

    // lpDdeI = CreateDDEShare(szServer, TEXT(SZDDESYS_TOPIC), NULL);
    //
    // if (NULL != lpDdeI)
    //    {
    //    GlobalFreePtr(lpDdeI);
    //    }
    // else
    //    {
    //    PERROR(TEXT("Couldn't create DDE share for ClipSrv!\r\n"));
    //    }


    wc.style = 0L;
    wc.lpfnWndProc = (WNDPROC)MainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CLIPSRV));
    wc.hCursor = NULL;
    wc.hbrBackground = NULL;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = TEXT("ClipSrvWClass");

    if (!RegisterClass(&wc))
       {
       PERROR(TEXT("Couldn't register wclass\r\n"));
       return FALSE;
       }

  /*
   * We are now connected to the appropriate service windowstation
   * and desktop. In order to get stuff from clipbook, we need to
   * switch our process over to use the interactive user's
   * clipboard.  Verify that we have access to do this.
   */
  hwinsta = OpenWindowStation("WinSta0", FALSE,
          WINSTA_ACCESSCLIPBOARD | WINSTA_ACCESSGLOBALATOMS);
  if (hwinsta == NULL) {
      PERROR(TEXT("Couldn't open windowstation WinSta0\r\n"));
      return FALSE;
  }

  SetProcessWindowStation(hwinsta);



    psrvstatus->dwCheckPoint++;

    hmutexClp = CreateMutex(NULL, FALSE, SZMUTEXCLP);

    hwndApp = CreateWindow(TEXT("ClipSrvWClass"),
                              TEXT("Hidden Data Server"),
                              WS_POPUP,
                              CW_USEDEFAULT,
                              CW_USEDEFAULT,
                              400,
                              200,
                              NULL,
                              NULL,
                              hInstance,
                              NULL
                              );



    if (!hwndApp)
       {
       PERROR(TEXT("No window created\r\n"));
       return FALSE;
       }


    psrvstatus->dwCheckPoint++;


    #if DEBUG
    if (ERROR_SUCCESS != RegCreateKeyEx (HKEY_CURRENT_USER,
                                         szClipviewRoot,
                                         0L,
                                         szRegClass,
                                         REG_OPTION_NON_VOLATILE,
                                         KEY_QUERY_VALUE, NULL,
                                         &hkeyClp,
                                         &dwKeyStatus)
         &&

        ERROR_SUCCESS != RegOpenKeyEx   (HKEY_CURRENT_USER,
                                         szClipviewRoot,
                                         0,
                                         KEY_QUERY_VALUE,
                                         &hkeyClp))
        {
        DebugLevel = 2;
        PINFO(TEXT("Clipsrv: Could not get root key\r\n"));
        }
    else
        {
        DWORD iSize = sizeof(DebugLevel);

        RegQueryValueEx (hkeyClp,
                         szDebug,
                         NULL,
                         NULL,
                         (LPBYTE)&DebugLevel,
                         &iSize);

        RegCloseKey (hkeyClp);
        }

    if (DebugLevel > 0)
        {
        ShowWindow(hwndApp, SW_SHOWMINNOACTIVE );
        }
    #endif


    if (DdeInitialize ((LPDWORD)&idInst,
                       (PFNCALLBACK)DdeCallback,
                       APPCMD_FILTERINITS,
                       0L))
        {
        PERROR(TEXT("Couldn't initialize DDE\r\n"));
        return FALSE;
        }



    PINFO(TEXT("DdeInit OK...\r\n"));
    psrvstatus->dwCheckPoint++;
    if (fService)
        {
        SetServiceStatus(hService, psrvstatus);
        }

    Hszize();
    DdeNameService(idInst, hszAppName, 0L, DNS_REGISTER);

    InitShares();

    return TRUE;

}










/*
 *      MainWndProc
 */

LRESULT CALLBACK MainWndProc(
    HWND    hwnd,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam)
{

    switch (message)
        {
        case WM_CREATE:
            fAnythingToRender = FALSE;
            cf_preview = RegisterClipboardFormat (SZPREVNAME);

            if (fService)
                {
                // Let the SCP that started us know that we're making progress
                srvstatus.dwCheckPoint++;
                SetServiceStatus(hService, &srvstatus);
                }

            PINFO(TEXT("Creating ClSrv window\r\n"));
            break;

        case WM_DESTROYCLIPBOARD:
            /* Prevent unnecessary file I/O when getting a WM_RENDERALLFORMATS */
            fAnythingToRender = FALSE;
            break;

        case WM_RENDERALLFORMATS:
            PINFO(TEXT("ClSrv\\WM_RNDRALL rcvd\r\n"));
            return (LRESULT)RenderAllFromFile(szSaveFileName);
            break;

        case WM_RENDERFORMAT:
            SetClipboardData((UINT)wParam, RenderFormatFromFile(szSaveFileName, (WORD)wParam));
            break;

        case WM_QUERYOPEN:
            return FALSE;

        case WM_DESTROY:
            PINFO(TEXT("sTOPPING...\r\n"));
            CleanUpShares();
            DdeNameService(idInst, 0L, 0L, DNS_UNREGISTER);
            UnHszize();
            DdeUninitialize(idInst);

            if (fService)
                {
                // Tell SCP we're stopping
                srvstatus.dwCheckPoint++;
                SetServiceStatus(hService, &srvstatus);
                }
            PostQuitMessage(0);
            break;

        default:
            return (DefWindowProc(hwnd, message, wParam, lParam));
        }

    return 0L;

}







/*
 *      RenderRawFormatToDDE
 *
 *  CLAUSGI - don't setClipboardData - just return the handle...
 */

HDDEDATA RenderRawFormatToDDE(
    FORMATHEADER    *pfmthdr,
    HANDLE          fh )
{
HDDEDATA   hDDE;
LPBYTE     lpDDE;
DWORD      cbData;
DWORD      dwBytesRead;
BOOL       fPrivate = FALSE, fMetafile = FALSE, fBitmap = FALSE;


    PINFO(TEXT("ClSrv\\RndrRawFmtToDDE:"));

    // Note that complex-data formats are sent under a private
    // format instead.

    if (PRIVATE_FORMAT(pfmthdr->FormatID )
       || pfmthdr->FormatID == CF_BITMAP
       || pfmthdr->FormatID == CF_METAFILEPICT
       || pfmthdr->FormatID == CF_PALETTE
       || pfmthdr->FormatID == CF_DIB
       || pfmthdr->FormatID == CF_ENHMETAFILE
       )
       {
       fPrivate = TRUE;
       if (pfmthdr->FormatID == CF_BITMAP)
          {
          fBitmap = TRUE;
          }
       else if (pfmthdr->FormatID == CF_METAFILEPICT)
          {
          fMetafile = TRUE;
          }
       pfmthdr->FormatID = RegisterClipboardFormatW(pfmthdr->Name);
       }


    PINFO(TEXT("rendering format %ws as %x\n\r"), (LPTSTR)pfmthdr->Name, pfmthdr->FormatID );

    #ifdef CACHEPREVIEWS
    if ( pfmthdr->FormatID == cf_preview )
       PINFO(TEXT("making APPOWNED data\n\r"));
    #endif




    if (!(hDDE = DdeCreateDataHandle (idInst,
                                      NULL,
                                      pfmthdr->DataLen,
                                      0L,
                                      hszAppName,
                                      pfmthdr->FormatID,
                                      #ifdef CACHEPREVIEWS
                                       pfmthdr->FormatID == cf_preview ? HDATA_APPOWNED : 0
                                      #else
                                       0
                                      #endif
                                      )))
        {
        PERROR(TEXT("Couldn't createdata handle\r\n"));
        goto done;
        }




    if ( !(lpDDE = DdeAccessData ( hDDE, &cbData )) )
       {
        PERROR(TEXT("Couldn't access handle\r\n"));
        DdeFreeDataHandle(hDDE);
        hDDE = 0L;
        goto done;
        }




    if (~0 == SetFilePointer(fh, pfmthdr->DataOffset, NULL, FILE_BEGIN))
        {
        PERROR(TEXT("Couldn't set file pointer\r\n"));
        DdeUnaccessData(hDDE);
        DdeFreeDataHandle(hDDE);
        hDDE = 0L;
        goto done;
        }


    ReadFile(fh, lpDDE, pfmthdr->DataLen, &dwBytesRead, NULL);

    if (dwBytesRead != pfmthdr->DataLen)
       {
       // Error in reading the file
       DdeUnaccessData(hDDE);
       DdeFreeDataHandle(hDDE);
       PERROR(TEXT("Error reading file: %ld from lread\n\r"),
         dwBytesRead);
       hDDE =  0L;
       goto done;
       }


    // BUGBUG This code packs CF_METAFILEPICT and CF_BITMAP
    // structs to WFW-type structs. It may lose extents for
    // very large bitmaps and metafiles when going from NT to NT.
    // Main symptom would be "Moved a metafile across clipbook and
    // it suddenly grew way outside its bounds".
    if (fMetafile)
       {
       WIN31METAFILEPICT w31mfp;
       unsigned uNewSize;
       HDDEDATA hDDETmp;

       uNewSize = pfmthdr->DataLen + sizeof(WIN31METAFILEPICT) -
                   sizeof(METAFILEPICT);

       // Have to make a smaller data handle now
       hDDETmp = hDDE;
       hDDE = DdeCreateDataHandle(idInst, NULL,  uNewSize, 0L,
             hszAppName, pfmthdr->FormatID, 0);

       w31mfp.mm   = (WORD)((METAFILEPICT *)lpDDE)->mm;
       w31mfp.xExt = (WORD)((METAFILEPICT *)lpDDE)->xExt;
       w31mfp.yExt = (WORD)((METAFILEPICT *)lpDDE)->yExt;

       // Place oldmetafilepict and data in new DDE block
       DdeAddData(hDDE, (LPTSTR)&w31mfp, sizeof(WIN31METAFILEPICT), 0L);
       DdeAddData(hDDE, lpDDE + sizeof(METAFILEPICT),
             uNewSize - sizeof(WIN31METAFILEPICT),
             sizeof(WIN31METAFILEPICT));

       // Drop old handle
       DdeUnaccessData(hDDETmp);
       DdeFreeDataHandle(hDDETmp);

       // We came in with hDDE accessed
       lpDDE = DdeAccessData(hDDE, &cbData);
       }
    else if (fBitmap)
       {
       WIN31BITMAP w31bm;
       unsigned uNewSize;
       HDDEDATA hDDETmp;

       uNewSize = pfmthdr->DataLen + sizeof(WIN31BITMAP) -
                 sizeof(BITMAP);

       // Have to make a smaller data handle now
       hDDETmp = hDDE;
       hDDE = DdeCreateDataHandle(idInst, NULL,  uNewSize, 0L,
             hszAppName, pfmthdr->FormatID, 0);

       w31bm.bmType       = (WORD)((BITMAP *)lpDDE)->bmType;
       w31bm.bmWidth      = (WORD)((BITMAP *)lpDDE)->bmWidth;
       w31bm.bmHeight     = (WORD)((BITMAP *)lpDDE)->bmHeight;
       w31bm.bmWidthBytes = (WORD)((BITMAP *)lpDDE)->bmWidthBytes;
       w31bm.bmPlanes     = (BYTE)((BITMAP *)lpDDE)->bmPlanes;
       w31bm.bmBitsPixel  = (BYTE)((BITMAP *)lpDDE)->bmBitsPixel;

       // Place old-style bitmap header and data in DDE block
       DdeAddData(hDDE, (LPTSTR)&w31bm, sizeof(WIN31BITMAP), 0L);
       DdeAddData(hDDE, lpDDE + sizeof(BITMAP),
             uNewSize - sizeof(WIN31BITMAP),
             sizeof(WIN31BITMAP));

       // Drop old handle
       DdeUnaccessData(hDDETmp);
       DdeFreeDataHandle(hDDETmp);

       // We came in with hDDE accessed
       lpDDE = DdeAccessData(hDDE, &cbData);
       }

    DdeUnaccessData(hDDE);



done:


    PINFO("Ret %lx\r\n", hDDE);
    return(hDDE);

}








/*
 *      SyncOpenClipboard
 */

BOOL SyncOpenClipboard(
    HWND    hwnd)
{
BOOL fOK;

    PINFO(TEXT("\r\nClipSrv: Opening Clipboard\r\n"));

    WaitForSingleObject (hmutexClp, INFINITE);
    fOK = OpenClipboard (hwnd);
    if (!fOK)
        {
        ReleaseMutex (hmutexClp);
        }

    return fOK;
}






/*
 *      SyncCloseClipboard
 */

BOOL SyncCloseClipboard(void)
{
BOOL fOK;

    PINFO(TEXT("\r\nClipSrv: Closing Clipboard\r\n"));

    fOK = CloseClipboard ();
    ReleaseMutex (hmutexClp);

    return fOK;
}
