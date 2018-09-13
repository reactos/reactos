/*----------------------------------------------------------------------------
/ Title;
/   dll.cpp
/
/ Authors;
/   Rick Turner (ricktu)
/
/ Notes;
/   Core entry points for the DLL
/----------------------------------------------------------------------------*/
#include "precomp.hxx"
#pragma hdrstop

#include <shguidp.h>

//#define MYDOCS_LOGFILE 1

/*----------------------------------------------------------------------------
/ Globals
/----------------------------------------------------------------------------*/

const GUID CLSID_MyDocumentsExt =   { 0x450d8fbaL, 0xad25, 0x11d0,
                                      { 0x98, 0xa8, 0x08, 0x00,
                                        0x36, 0x1b, 0x11, 0x03
                                      }
                                    };

HINSTANCE g_hInstance = 0;
LONG      g_lWaitCursor = 0;



#ifdef MYDOCS_LOGFILE
extern CRITICAL_SECTION cs;
extern TCHAR szLogFile[MAX_PATH];
#endif

/*-----------------------------------------------------------------------------
/ Private function prototypes
/----------------------------------------------------------------------------*/
HRESULT _CheckPerUserSettings( BOOL fCreatePersonalPath );

/*-----------------------------------------------------------------------------
/ DllMain
/ -------
/   Main entry point.  We are passed reason codes and assored other
/   information when loaded or closed down.
/
/ In:
/   hInstance = our instance handle
/   dwReason = reason code
/   pReserved = depends on the reason code.
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
EXTERN_C BOOL DllMain( HINSTANCE hInstance, DWORD dwReason, LPVOID pReserved )
{
    switch ( dwReason )
    {
        case DLL_PROCESS_ATTACH:
        {
#ifdef MYDOCS_LOGFILE
            InitializeCriticalSection( &cs );
            GetSystemDirectory( szLogFile, ARRAYSIZE(szLogFile) );
            lstrcat( szLogFile, TEXT("\\mydocs.log") );
#endif

#ifdef DEBUG
            DllSetTraceMask();
#endif
            MDTraceEnter( TRACE_SETUP, "DLL_PROCESS_ATTACH" );
            DisableThreadLibraryCalls( hInstance );
            g_hInstance = hInstance;
            _CheckPerUserSettings( TRUE );
            MDTraceLeave( );
            break;
        }

        case DLL_THREAD_ATTACH:
        {
            break;
        }

        case DLL_PROCESS_DETACH:
        {
#ifdef MYDOCS_LOGFILE
            DeleteCriticalSection( &cs );
#endif
            break;
        }
    }

    return TRUE;
}


/*-----------------------------------------------------------------------------
/ DllCanUnloadNow
/ ---------------
/   Called by the outside world to determine if our DLL can be unloaded. If we
/   have any objects in existance then we must not unload.
/
/ In:
/   -
/ Out:
/   BOOL inidicate unload state.
/----------------------------------------------------------------------------*/
STDAPI DllCanUnloadNow( void )
{
    return GLOBAL_REFCOUNT ? S_FALSE : S_OK;
}


/*-----------------------------------------------------------------------------
/ DllGetClassObject
/ -----------------
/   Given a class ID and an interface ID, return the relevant object.  This used
/   by the outside world to access the objects contained here in.
/
/ In:
/   rCLISD = class ID required
/   riid = interface within that class required
/   ppVoid -> receives the newly created object.
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
STDAPI DllGetClassObject( REFCLSID rCLSID, REFIID riid, LPVOID* ppVoid)
{
    HRESULT hr = E_OUTOFMEMORY;

    MDTraceEnter(TRACE_CORE, "DllGetClassObject");
    MDTraceGUID("Object requested", rCLSID);
    MDTraceGUID("Interface requested", riid);

    *ppVoid = NULL;

    if ( IsEqualIID(riid, IID_IClassFactory) )
    {
        CMyDocsClassFactory* pClassFactory = new CMyDocsClassFactory();

        if ( !pClassFactory )
            ExitGracefully(hr, E_OUTOFMEMORY, "Failed to create the class factory");

        hr = pClassFactory->QueryInterface(riid, ppVoid);

        if ( FAILED(hr) )
            delete pClassFactory;
    }
    else
    {
        ExitGracefully(hr, E_NOINTERFACE, "IID_IClassFactory not passed as an interface");
    }

exit_gracefully:

    MDTraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ Debug / Trace support
/----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
/ DllSetTraceMask
/ ---------------
/   Read from the registry and setup the debug level based on the
/   flags word stored there.
/
/ In:
/   -
/ Out:
/   -
/----------------------------------------------------------------------------*/
#ifdef DEBUG
void DllSetTraceMask(void)
{
    DWORD dwTraceMask, cbTraceMask;
    HKEY hkey;

    if ( ERROR_SUCCESS == RegOpenKey(HKEY_CLASSES_ROOT, c_szCLSIDFormat, &hkey) )
    {
        cbTraceMask = SIZEOF(dwTraceMask);

        if ( ERROR_SUCCESS == RegQueryValueEx( hkey,
                                               TEXT("TraceMask"),
                                               NULL,
                                               NULL,
                                               (LPBYTE)&dwTraceMask,
                                               &cbTraceMask
                                              )
           )
        {
            MDTraceSetMask(dwTraceMask);
        }

        RegCloseKey(hkey);
    }
}
#endif


/*-----------------------------------------------------------------------------
/ CallRegInstall
/ --------------
/   Call ADVPACK for the given section of our resource based INF>
/
/ In:
/   hInstance = resource instance to get REGINST section from
/   szSection = section name to invoke
/
/ Out:
/   HRESULT:
/----------------------------------------------------------------------------*/
HRESULT CallRegInstall(HINSTANCE hInstance, LPSTR szSection)
{
    HRESULT hr = E_FAIL;
    HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));

    MDTraceEnter(TRACE_SETUP, "CallRegInstall");
#ifdef DEBUG
#ifdef UNICODE
    WCHAR szSectionW[ 128 ];
    MultiByteToWideChar( CP_ACP, 0, szSection, -1, szSectionW, ARRAYSIZE(szSectionW) );
    MDTrace(TEXT("szSection = %s"),szSectionW);
#else
    MDTrace("szSection = %s", szSection);
#endif
#endif

    if (hinstAdvPack)
    {
        REGINSTALL pfnri = (REGINSTALL)GetProcAddress(hinstAdvPack, "RegInstall");

        if ( pfnri )
        {
#ifdef WINNT
            STRENTRY seReg[] =
            {
                // These two NT-specific entries must be at the end
                { "25", "%SystemRoot%" },
                { "11", "%SystemRoot%\\system32" },
            };
            STRTABLE stReg = { ARRAYSIZE(seReg), seReg };
            hr = pfnri(hInstance, szSection, &stReg);
#else
            hr = pfnri(hInstance, szSection, NULL);
#endif
        }

        FreeLibrary(hinstAdvPack);
    }

    MDTrace(TEXT("hr = 0x%X"), hr );
    MDTraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ CheckPerUserSettings
/ --------------------
/   Make sure the per-user settings are correct
/----------------------------------------------------------------------------*/
HRESULT
_CheckPerUserSettings( BOOL fCreatePersonalPath )
{

    HRESULT hr;
    TCHAR szSpecFolderPath[ MAX_PATH ];
    HKEY hkey1 = NULL, hkey2 = NULL;

    MDTraceEnter( TRACE_SETUP, "_CheckPerUserSettings" );
    MDTrace(
        TEXT("fCreatePersonalPath is %s"),
        fCreatePersonalPath ?  TEXT("TRUE") : TEXT("FALSE")
    );

    //*************************************************************************
    // Try to create empty documents key (for special items)
    //*************************************************************************
    RegOpenKey( HKEY_CURRENT_USER, c_szDocumentSettings, &hkey1 );
    RegOpenKey( HKEY_CURRENT_USER, c_szPerUserCLSID,     &hkey2 );

#ifndef WINNT
    if ((!hkey1) || (!hkey2))
    {
        hr = CallRegInstall( g_hInstance, "RegPerUserWin98" );
#else
    if (!hkey1)
    {
        hr = CallRegInstall( g_hInstance, "RegPerUserNT" );
#endif
        FailGracefully( hr, "CallRegInstall failed for HKCU key" );
    }
    else
    {
        MDTrace(TEXT("No per-user reg init required..."));
    }

#ifdef WINNT
    //
    // Clean up old stuff if it's there
    //
    if (hkey2)
    {
        RegCloseKey( hkey2 );
        hkey2 = NULL;
        RegDelnode( HKEY_CURRENT_USER, (LPTSTR)c_szPerUserCLSID );
    }
#endif

    //*************************************************************************
    // set up CSIDL_PERSONAL correctly...
    //*************************************************************************
    if (!GetPersonalPath( szSpecFolderPath, fCreatePersonalPath ))
    {
        //
        // We need to check whether or not we are running under Winstone 97.
        // They do the sleazy trick of deleting the personal value from
        // HKEY_CURRENT_USER but creating one in HKEY_LOCAL_MACHINE that
        // (usually) points to wstemp\mydocs.  In this case, we want to bail
        // and try and get out of the way as soon as possible...
        //

        if (IsWinstone97(NULL))
        {
            ExitGracefully( hr, E_FAIL, "running under Winstone 97" );
        }

        hr = GetDefaultPersonalPath( szSpecFolderPath, FALSE );
        FailGracefully( hr, "unable to get default personal path" );

        hr = ChangePersonalPath( szSpecFolderPath, NULL );
        FailGracefully( hr, "unable to set default personal path" );

        // At this point we're creating the directory.
        if (!GetPersonalPath( szSpecFolderPath, TRUE ))
        {
            MDTraceAssert(FALSE);
            ExitGracefully( hr, E_FAIL, "BAD BAD BAD, GetPersonalPath failed when we tried to create CSIDL_PERSONAL");
        }
    }

    //*************************************************************************
    // make sure CSIDL_PERSONAL has desktop.ini...
    //*************************************************************************
    if (!IsWinstone97(NULL))
    {
        hr = StampMyDocsFolder( szSpecFolderPath );
        FailGracefully( hr, "unable to stamp MyDocs directory w/desktop.ini" );
    }
    else
    {
        MDTrace(TEXT("Running under Winstone97, didn't stamp MyDocs"));
    }


exit_gracefully:

    if (hkey1)
    {
        RegCloseKey( hkey1 );
    }

    if (hkey2)
    {
        RegCloseKey( hkey2 );
    }

    MDTraceLeaveResult(hr);

}

#define REG_IMAGEPREVIEWTEMPLATE    TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\WebView templates\\Image Preview")
#define REG_TEMPLATEFILE            TEXT("TemplateFile")

void InstallMyPictures()
{
    TCHAR szPath[MAX_PATH];
    // Create the My Pictures folder
    if (SHGetSpecialFolderPath(NULL, szPath, CSIDL_MYPICTURES, TRUE))
    {
        // SHGetSetFolderCustomSettings is exported by shell32 by ordinal. So, we have to make sure
        // that its version >= 5
        UINT uiShell32Version = 0;
        HINSTANCE hinst = GetModuleHandle(TEXT("SHELL32.DLL"));
        if (hinst)
        {
            // NOTE: GetProcAddress always takes ANSI strings!
            DLLGETVERSIONPROC pfnGetVersion = (DLLGETVERSIONPROC)GetProcAddress(hinst, "DllGetVersion");
            DLLVERSIONINFO dllinfo;

            dllinfo.cbSize = sizeof(DLLVERSIONINFO);
            if (pfnGetVersion && pfnGetVersion(&dllinfo) == NOERROR)
                uiShell32Version = dllinfo.dwMajorVersion;
        }

        if (uiShell32Version >= 5)
        {
            // Set the default custom settings for the folder.
            SHFOLDERCUSTOMSETTINGS fcs;
            fcs.dwSize = sizeof(SHFOLDERCUSTOMSETTINGS);
            fcs.dwMask = FCSM_VIEWID | FCSM_PERSISTMONIKER;
            SHELLVIEWID vid = CLSID_ThumbnailViewExt;
            fcs.pvid = &vid;

            // Try to get the template path from the registry.
            TCHAR szPersistMoniker[MAX_PATH];
            DWORD dwType, cbData = (DWORD)SIZEOF(szPersistMoniker);

            if (!SHGetValue(HKEY_LOCAL_MACHINE, REG_IMAGEPREVIEWTEMPLATE, REG_TEMPLATEFILE, &dwType,
                    (LPVOID)szPersistMoniker, &cbData))
            {
                GetWindowsDirectory(szPersistMoniker, ARRAYSIZE(szPersistMoniker));
                PathAppend(szPersistMoniker, TEXT("Web\\ImgView.htt"));
            }

            fcs.pszPersistMoniker = szPersistMoniker;
            fcs.pszInfoTip = NULL;
            fcs.pclsid = NULL;
            fcs.dwFlags = 0;
            SHGetSetFolderCustomSettings(&fcs, szPath, FCS_WRITE);
        }
    }
}

/*-----------------------------------------------------------------------------
/ PerUserInit
/ -----------
/   Do setup stuff for this user
/----------------------------------------------------------------------------*/
EXTERN_C VOID PerUserInit( void )
{
    HRESULT hr;
    TCHAR szPath[ MAX_PATH ];
    HKEY hkey = NULL;

    MDTraceEnter( TRACE_SETUP, "PerUserInit" );

    //*************************************************************************
    // Try to create the HKCR things we need for New Menu if we determine that
    // we're not being shown...
    //*************************************************************************

    // first, delete the key if it exists...
    RemoveMenuStuff();

    // next, check which set of keys to add (either new menu or sendto menu)
    if (IsMyDocsHidden())
    {
        AddNewMenuToRegistry();
    }
    else
    {
        hr = CallRegInstall( g_hInstance, "RegSendTo" );
        MDTraceCheckResult( hr, "CallRegInstall failed for HKCR SendToMenu things" );

        if (SUCCEEDED(hr))
        {
            // Create the dummy file with current display name for My Documents...
            UpdateSendToFile(FALSE);
        }
    }

    InstallMyPictures();

    // Check special case that someone (notable Dell when setting up OSR2
    // w/Office 97) might have made the Personal shell folder a directory under
    // the windows\desktop directory.  In this case, we want to hide our
    // regitm on the desktop
    if (GetPersonalPath( szPath, FALSE ))
    {
        MDTrace(TEXT("Calling IsGoodMyDocsPath on - %s -"), szPath );
        if (IsPathGoodMyDocsPath( NULL, szPath ) == PATH_IS_DESKTOP )
        {
            TCHAR ch = 0;

            MDTrace( TEXT("Hiding My Docs regitm because path is on desktop...") );

            // Need to hide regitm
            if (ERROR_SUCCESS!=RegOpenKey( HKEY_CURRENT_USER, c_szDocumentSettings, &hkey ))
            {
                MDTrace(TEXT("Unable to open Document key"));
                goto exit_gracefully;
            }

            if (ERROR_SUCCESS!=RegSetValueEx( hkey, c_szHidden, NULL, REG_SZ, (LPBYTE)&ch, SIZEOF(TCHAR) ))
            {
                MDTrace(TEXT("error settting hidden registry flag"));
                goto exit_gracefully;
            }

            AddNewMenuToRegistry();

        }
    }

exit_gracefully:

    if (hkey)
    {
        RegCloseKey( hkey );
    }

    MDTraceLeave();
}


/*-----------------------------------------------------------------------------
/ DllRegisterServer
/ -----------------
/   Do setup stuff for this .dll
/----------------------------------------------------------------------------*/
EXTERN_C STDAPI DllRegisterServer( void )
{
    HRESULT hr = S_OK;
    HKEY hkey = NULL,hkeySF;
    TCHAR szBuffer[MAX_PATH];
    TCHAR szDisplayName[MAX_PATH];
    UINT  cchDisplay,cch;
    DWORD dwSortIndex = 16;

    MDTraceEnter(TRACE_SETUP, "DllRegisterServer");

    //*************************************************************************
    // Do our dll reg from .inf first...
    //*************************************************************************
    hr = CallRegInstall( g_hInstance, "RegDll" );
    FailGracefully( hr, "couldn't do DLL Registration via .inf!" );

    //*************************************************************************
    // Now do entries that need to be localized, etc...
    //*************************************************************************
    cchDisplay = LoadString( g_hInstance,
                             IDS_DISPLAY_NAME,
                             szDisplayName,
                             ARRAYSIZE(szDisplayName)
                            );

    if (!cchDisplay)
    {
        MDTrace( TEXT("LoadString failed for display name") );
    }

    //*************************************************************************
    // Try to create the HKCR\CLSID\{our clsid} key...
    //*************************************************************************
    if ( ERROR_SUCCESS !=
         RegOpenKey(HKEY_CLASSES_ROOT, c_szCLSIDFormat, &hkey)
        )
        ExitGracefully( hr, E_FAIL, "Couldn't open our CLSID key" );

    // add our display name...
    if ( ERROR_SUCCESS !=
         RegSetValueEx( hkey, NULL, 0, REG_SZ, (LPBYTE)szDisplayName, (cchDisplay+1)*sizeof(TCHAR))
        )
        MDTraceCheckResult( E_FAIL, "couldn't set display name in CLSID\\{...}" );

    // add our info tip
    *szBuffer = 0;
    cch = LoadString(g_hInstance, IDS_INFOTIP_VALUE, szBuffer, ARRAYSIZE(szBuffer) );
    if (!cch)
        MDTraceCheckResult( E_FAIL, "LoadString failed for info tip" );

    if ( ERROR_SUCCESS !=
         RegSetValueEx( hkey, TEXT("InfoTip"), 0, REG_SZ, (LPBYTE)szBuffer, (cch+1)*sizeof(TCHAR))
        )
        MDTraceCheckResult( E_FAIL, "couldn't set InfoTip in CLSID\\{...}" );

    // remove ShellFolder\Attributes if it exists...
    if ( ERROR_SUCCESS == RegOpenKey( hkey, TEXT("ShellFolder"), &hkeySF ))
    {
        // try to remove Attributes value...
        RegDeleteValue( hkeySF, TEXT("Attributes") );
        RegCloseKey( hkeySF );
    }

    // set SortOrderIndex...
    if ( ERROR_SUCCESS !=
         RegSetValueEx( hkey, TEXT("SortOrderIndex"), 0, REG_DWORD, (LPBYTE)&dwSortIndex, sizeof(dwSortIndex) )
        )
        MDTraceCheckResult( E_FAIL, "couldn't set SortOrderIndex in CLSID\\{...}" );

    if (hkey)
    {
        RegCloseKey( hkey );
    }

    //*************************************************************************
    // Try to create the HKCR things we need for New Menu if we determine that
    // we're not being shown...
    //*************************************************************************
    MDTrace(TEXT("Calling PerUserInit from DllRegisterServer"));
    PerUserInit();

    //*************************************************************************
    // Try to create NameSpace key...
    //*************************************************************************
    lstrcpy( szBuffer, c_szNameSpaceKey );
    lstrcat( szBuffer, c_szMyDocsCLSID );

    if ( ERROR_SUCCESS !=
         RegCreateKey(HKEY_LOCAL_MACHINE, szBuffer, &hkey)
        )
        ExitGracefully( hr, E_FAIL, "couldn't create namespace key" );

    // add our display name...
    if ( ERROR_SUCCESS !=
         RegSetValueEx( hkey, NULL, 0, REG_SZ, (CONST BYTE *)szDisplayName, (cchDisplay+1)*sizeof(TCHAR))
        )
        MDTraceCheckResult( E_FAIL, "couldn't set display name in namespace key" );

    hr = _CheckPerUserSettings( FALSE );
    MDTraceCheckResult( hr, "_CheckPerUserSettings failed" );

exit_gracefully:

    if (hkey)
    {
        RegCloseKey( hkey );
    }

    MDTraceLeaveResult( hr );
}


/*-----------------------------------------------------------------------------
/ DllUnRegisterServer
/ -----------------
/   Do setup stuff for this .dll
/----------------------------------------------------------------------------*/
EXTERN_C STDAPI DllUnregisterServer( void )
{
    HRESULT hr = S_OK;
    HKEY hkey = NULL;
    TCHAR szBuffer[MAX_PATH];

    MDTraceEnter(TRACE_SETUP, "DllUnregisterServer");

    //*************************************************************************
    // Try to delete the NameSpace key...
    //*************************************************************************
    lstrcpy( szBuffer, c_szNameSpaceKey );
    lstrcat( szBuffer, c_szMyDocsCLSID );

    if (!RegDelnode( HKEY_LOCAL_MACHINE, szBuffer ))
        MDTraceCheckResult( E_FAIL, "couldn't RegDelnode namespace key" );

    //*************************************************************************
    // Try to remove the HKCR\CLSID\{our clsid} key...
    //*************************************************************************
    hr = CallRegInstall( g_hInstance, "UnregDll" );

    //*************************************************************************
    // Try to remove the new and/or sendto menu stuff...
    //*************************************************************************
    RemoveMenuStuff();

    MDTraceLeaveResult(hr)

}


/*-----------------------------------------------------------------------------
/ DllInstall
/ -----------------
/   Do specific cmd line based setup
/----------------------------------------------------------------------------*/
EXTERN_C STDAPI DllInstall( BOOL bInstall, LPCWSTR pszCmdLine )
{
    TCHAR szCmdLine[ MAX_PATH ];
    HRESULT hr = S_OK;

    MDTraceEnter( TRACE_SETUP, "DllInstall" );

    if (!pszCmdLine || !(*pszCmdLine))
        ExitGracefully( hr, E_INVALIDARG, "no command line!" );

    OleStrToStrN( szCmdLine, ARRAYSIZE(szCmdLine), pszCmdLine, -1 );
    MDTrace( TEXT("bInstall = 0x%X, pszCmdLine = %s"), bInstall, szCmdLine );

    if (0 == lstrcmpi( szCmdLine, TEXT("UseReadOnly")))
    {
        //
        // Add key for system to use read only bit on shell folders...
        //

        HKEY hkey;
        TCHAR szPath[ MAX_PATH ];

        if (ERROR_SUCCESS == RegCreateKey( HKEY_LOCAL_MACHINE,
                                           c_szExplorerSettings,
                                           &hkey )
            )
        {

            if (bInstall)
            {
                DWORD dwValue = 1;

                RegSetValueEx( hkey, c_szReadOnlyBit, 0, REG_DWORD,
                               (LPBYTE)&dwValue, sizeof(dwValue)
                              );


            }
            else
            {
                RegDeleteValue( hkey, c_szReadOnlyBit );

            }
        }

        RegCloseKey( hkey );

        if (SHGetSpecialFolderPath( NULL, szPath, CSIDL_PERSONAL, FALSE ))
        {
            StampMyDocsFolder( szPath );
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

exit_gracefully:

    MDTraceLeaveResult( hr );
}
