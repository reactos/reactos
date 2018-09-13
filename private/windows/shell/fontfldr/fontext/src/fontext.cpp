///////////////////////////////////////////////////////////////////////////////
//
// fontext.cpp
//      Explorer Font Folder extension routines
//     Fonts Folder Shell Extension
//
//
// History:
//      31 May 95 SteveCat
//          Ported to Windows NT and Unicode, cleaned up
//
//
// NOTE/BUGS
//
//  Copyright (C) 1992-1995 Microsoft Corporation
//
///////////////////////////////////////////////////////////////////////////////

//==========================================================================
//                              Include files
//==========================================================================

#include "priv.h"

// ********************************************************
// Initialize GUIDs
//

#pragma data_seg(".text")
#define INITGUID
#include <initguid.h>
#include <cguid.h>
#include <shlguid.h>
#include "fontext.h"
#include "panmap.h"     // the IID for the Panose Mapper.

//#undef INITGUID
#pragma data_seg()

#include "globals.h"
#include "extinit.h"
#include "fontman.h"
#include "fontview.h"
#include "cpanel.h"
#include "ui.h"
#include "dbutl.h"
#include "extricon.h"

#ifndef ARRAYSIZE
#   define ARRAYSIZE(a)  (sizeof(a) / sizeof((a)[0]))
#endif

#define GUIDSIZE  (GUIDSTR_MAX + 1)

HINSTANCE g_hInst = NULL;
LONG      g_cRefThisDll = 0; // Number of references to objects in this dll
LONG      g_cLock = 0;       // Used by the LockContainer member of CFontFolder
BOOL      g_bDBCS;           // Running in a DBCS locale ?
CRITICAL_SECTION g_csFontManager; // For acquiring font manager ptr.

class CImpIClassFactory;

// UINT g_DebugMask; //  = DM_ERROR | DM_TRACE1 | DM_MESSAGE_TRACE1 | DM_TRACE2;
UINT g_DebugMask = DM_ERROR | DM_TRACE1 | DM_MESSAGE_TRACE1 | DM_TRACE2;


#ifdef _DEBUG

#ifdef WINNT
//
// The Alpha compiler doesn't like the typecast used in the call to wvsprintf().
// Using standard variable argument mechanism.
//
#include <stdarg.h>
#endif

void DebugMessage( UINT mask, LPCTSTR pszMsg, ... )
{

    TCHAR ach[ 256 ];

#ifdef WINNT
    va_list args;
    va_start(args, pszMsg);
#endif

    if( !( mask & g_DebugMask ) ) return;

    wvsprintf( ach, pszMsg, ( (char *)(TCHAR *) &pszMsg + sizeof( TCHAR * ) ) );

#ifdef WINNT
    wvsprintf( ach, pszMsg, args);
    va_end(args);
#endif

    if( !( mask & DM_NOEOL ) ) lstrcat( ach, TEXT( "\r\n" ) );


#ifndef USE_FILE
    OutputDebugString( ach );
#else
    HANDLE hFile;
    long x;
    
    hFile = CreateFile( g_szLogFile, GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
                        OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );

    if( INVALID_HANDLE_VALUE == hFile )
    {
       OutputDebugString( TEXT( "FontExt: Unable to open log file\r\n" ) );
       return;
    }
    
    if( 0xFFFFFFFF == SetFilePointer( hFile, 0, NULL, FILE_END ) )
    {
       OutputDebugString( TEXT( "FontExt: Unable to seek to end of log file\r\n" ) );
       return;
    }
    
    if( !WriteFile( hFile, ach, strlen( ach ), &x, NULL ) )
    {
       OutputDebugString( TEXT( "FontExt: Unable to write to log file\r\n" ) );
       return;
    }
    
    if( !CloseHandle( hFile ) )
    {
       OutputDebugString( TEXT( "FontExt: Unable to close log file\r\n" ) );
       return;
    }
#endif
}


// ******************************************************************
// Send an HRESULT to the debug output
//

void DebugHRESULT( int flags, HRESULT hResult )
{
    switch( GetScode( hResult ) )
    {
        case S_OK:          DEBUGMSG( (flags, TEXT( "S_OK" ) ) );          return;
        case S_FALSE:       DEBUGMSG( (flags, TEXT( "S_FALSE" ) ) );       return;
        case E_NOINTERFACE: DEBUGMSG( (flags, TEXT( "E_NOINTERFACE" ) ) ); return;
        case E_NOTIMPL:     DEBUGMSG( (flags, TEXT( "E_NOTIMPL" ) ) );     return;
        case E_FAIL:        DEBUGMSG( (flags, TEXT( "E_FAIL" ) ) );        return;
        case E_OUTOFMEMORY: DEBUGMSG( (flags, TEXT( "E_OUTOFMEMORY" ) ) ); return;
    } // switch

    if( SUCCEEDED( hResult ) ) 
        DEBUGMSG( (flags, TEXT( "S_unknown" ) ) );
    else if( FAILED( hResult ) ) 
        DEBUGMSG( (flags, TEXT( "E_unknown" ) ) );
    else 
        DEBUGMSG( (flags, TEXT( "No Clue" ) ) );
}


// ******************************************************************
// Print a REFIID to the debugger

void DebugREFIID( int flags, REFIID riid )
{
   if( riid == IID_IUnknown ) DEBUGMSG( (flags, TEXT( "IID_IUnknown" ) ) );
   else if( riid == IID_IShellFolder )  DEBUGMSG( (flags, TEXT( "IID_IShellFolder" ) ) );
   else if( riid == IID_IClassFactory ) DEBUGMSG( (flags, TEXT( "IID_IClassFactory" ) ) );
   else if( riid == IID_IShellView )    DEBUGMSG( (flags, TEXT( "IID_IShellView" ) ) );
   else if( riid == IID_IShellBrowser ) DEBUGMSG( (flags, TEXT( "IID_IShellBrowser" ) ) );
   else if( riid == IID_IContextMenu )  DEBUGMSG( (flags, TEXT( "IID_IContextMenu" ) ) );
   else if( riid == IID_IShellExtInit ) DEBUGMSG( (flags, TEXT( "IID_IShellExtInit" ) ) );
   else if( riid == IID_IShellPropSheetExt ) DEBUGMSG( (flags, TEXT( "IID_IShellPropSheetExt" ) ) );
   else if( riid == IID_IPersistFolder ) DEBUGMSG( (flags, TEXT( "IID_IPersistFolder" ) ) );
   else if( riid == IID_IExtractIconW )  DEBUGMSG( (flags, TEXT( "IID_IExtractIconW" ) ) );
   else if( riid == IID_IExtractIconA )  DEBUGMSG( (flags, TEXT( "IID_IExtractIconA" ) ) );
   else if( riid == IID_IDropTarget )   DEBUGMSG( (flags, TEXT( "IID_IDropTarget" ) ) );
   else if( riid == IID_IPersistFile )   DEBUGMSG( (flags, TEXT( "IID_IPersistFile" ) ) );
   //else if( riid == IID_I ) DEBUGMSG( (flags, TEXT( "IID_I" ) ) );
   else DEBUGMSG( (flags, TEXT( "No clue what interface this is" ) ) );
}
#endif   // _DEBUG


// ******************************************************************
// ******************************************************************
// DllMain

STDAPI_(BOOL) APIENTRY DllMain( HINSTANCE hDll, 
                                DWORD dwReason, 
                                LPVOID lpReserved )
{
    switch( dwReason )
    {
        case DLL_PROCESS_ATTACH:
        {
            g_DebugMask = DM_ERROR | DM_TRACE1 | DM_TRACE2
                          | DM_MESSAGE_TRACE1; //  | DM_MESSAGE_TRACE2;
            DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: LibMain - DLL_PROCESS_ATTACH" ) ) );
            g_hInst = hDll;

            DisableThreadLibraryCalls(hDll);
            InitializeCriticalSection(&g_csFontManager);
            
            //
            // Initialize the global g_bDBCS flag.
            //
            USHORT wLanguageId = LANGIDFROMLCID(GetThreadLocale());

            g_bDBCS = (LANG_JAPANESE == PRIMARYLANGID(wLanguageId)) ||
                      (LANG_KOREAN   == PRIMARYLANGID(wLanguageId)) ||
                      (LANG_CHINESE  == PRIMARYLANGID(wLanguageId));

            //
            // Initialize the various modules.
            //
            
            vCPPanelInit( );
            vUIMsgInit( );
            
            DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: LibMain - DLL_PROCESS_ATTACH" ) ) );
            break;
        }
        
        case DLL_PROCESS_DETACH:
            DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: LibMain - DLL_PROCESS_DETACH" ) ) );
            DeleteCriticalSection(&g_csFontManager);
            break;
        
        case DLL_THREAD_ATTACH:
            DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: LibMain - DLL_THREAD_ATTACH" ) ) );
            break;
        
        case DLL_THREAD_DETACH:
            DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: LibMain - DLL_THREAD_DETACH" ) ) );
            break;
        
        default:
            DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: LibMain - DLL_something else" ) ) );
            break;
      
    } // switch
    
    return( TRUE );
}

 
// ******************************************************************
// DllCanUnloadNow

STDAPI DllCanUnloadNow( )
{
    HRESULT retval;
    
    DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: DllCanUnloadNow called - %d references" ),
               g_cRefThisDll ) );

    retval = ResultFromScode( (g_cRefThisDll == 0 ) && (g_cLock == 0 ) 
                               ? S_OK : S_FALSE );

    DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: DllCanUnloadNow returning: %s" ),
               g_cRefThisDll ? TEXT( "S_FALSE" ) : TEXT( "S_OK" ) ) );

    return( retval );
}


// ********************************************************************

class CImpIClassFactory : public IClassFactory
{

public:
   CImpIClassFactory( ) : m_cRef( 0 )

      { g_cRefThisDll++;}
   ~CImpIClassFactory( ) { 
      DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: ~CImpIClassFactory" ) ) );
      g_cRefThisDll--; }

   //
   // *** IUnknown methods ***
   //

   STDMETHODIMP QueryInterface( REFIID riid, LPVOID FAR* ppvObj );
   STDMETHODIMP_(ULONG) AddRef( void );
   STDMETHODIMP_(ULONG) Release( void );
 
   //
   // *** IClassFactory methods ***
   //

   STDMETHODIMP CreateInstance( LPUNKNOWN pUnkOuter,
                                REFIID riid,
                                LPVOID FAR* ppvObject );

   STDMETHODIMP LockServer( BOOL fLock );

private:
  int m_cRef;

};

// ******************************************************************
// ******************************************************************
// DllGetClassObject

STDAPI DllGetClassObject( REFCLSID rclsid, 
                          REFIID riid, 
                          LPVOID FAR* ppvObj )
{

    // DEBUGBREAK;
    
    DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: DllGetClassObject called" ) ) );
    
    if( !(rclsid == CLSID_FontExt ) )
    {
       DEBUGMSG( (DM_ERROR, TEXT( "FONTEXT: Dll-GCO: Tried to create a ClassFactory for an unknown class" ) ) );
    
       return( ResultFromScode( E_FAIL ) );
    }
    
    if( !(riid == IID_IUnknown ) && !(riid == IID_IClassFactory ) )
    {
       DEBUGMSG( (DM_ERROR, TEXT( "FONTEXT: Dll-GCO: Unknown Interface requested" ) ) );
       return( ResultFromScode( E_NOINTERFACE ) );
    }
    
    DEBUGMSG( (DM_TRACE2, TEXT( "FONTEXT: Dll-GCO Creating a class factory for CLSID_FontExt" ) ) );
    
    *ppvObj = (LPVOID) new CImpIClassFactory;
    
    if( !*ppvObj )
    {
        DEBUGMSG( (DM_ERROR, TEXT( "FONTEXT: Dll-GCO: Out of memory" ) ) );

        return( ResultFromScode( E_OUTOFMEMORY ) );
    }
    
    ((LPUNKNOWN)*ppvObj)->AddRef( );
    
    return NOERROR;
}


HRESULT CreateViewObject( LPVOID FAR * ppvObj )
{
    HRESULT    retval;
    CFontView* prv;
    
    retval = ResultFromScode( E_OUTOFMEMORY );
    
    prv = new CFontView();

    if( !prv )
        return( retval );
    
    //
    //  AddRef the view and then Release after the QI. If QI fails,
    //  then prv with delete itself gracefully.
    //

    prv->AddRef( );

    retval = prv->QueryInterface( IID_IShellView, ppvObj );

    prv->Release( );
    
    return( retval );

}

// ***********************************************************************
// ***********************************************************************
//  CImpIClassFactory member functions
//
//  *** IUnknown methods ***
//

STDMETHODIMP CImpIClassFactory::QueryInterface( REFIID riid, 
                                                LPVOID FAR* ppvObj )
{
    *ppvObj = NULL;
    
    DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: CImpIClassFactory::QueryInterface called" ) ) );
    
    //
    //  Any interface on this object is the object pointer
    //

    if( (riid == IID_IUnknown) || (riid == IID_IClassFactory) )
       *ppvObj = (LPVOID) this;
    
    if( *ppvObj )
    {
       ((LPUNKNOWN)*ppvObj)->AddRef( );
       return NOERROR;
    }
    
    return( ResultFromScode( E_NOINTERFACE ) );
}


STDMETHODIMP_(ULONG) CImpIClassFactory::AddRef( void )
{
    DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: CImpIClassFactory::AddRef called: %d->%d references" ),
              m_cRef, m_cRef + 1) );

    return( ++m_cRef );
}


STDMETHODIMP_(ULONG) CImpIClassFactory::Release( void )
{
    DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: CImpIClassFactory::Release called: %d->%d references" ),
              m_cRef, m_cRef - 1) );
    
    ULONG retval;
    
    retval = --m_cRef;
    
    if( !retval ) 
       delete this;

    DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: CImpIClassFactory Leaving. " ) ) );

    return( retval );
}
 

//
//  *** IClassFactory methods ***
//

STDMETHODIMP CImpIClassFactory::CreateInstance( LPUNKNOWN pUnkOuter,
                                                REFIID riid,
                                                LPVOID FAR* ppvObj )
{
    HRESULT   retval;
    LPUNKNOWN poUnk = NULL;
    
    
    retval = ResultFromScode( E_OUTOFMEMORY );
    
    DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: CImpIClassFactory::CreateInstance called" ) ) );
    DEBUGREFIID( (DM_TRACE1, riid) );
    
    //
    //  we do not support aggregation
    //
    
    if( pUnkOuter )
       return( ResultFromScode( CLASS_E_NOAGGREGATION ) );
    
    if( riid == IID_IShellView || riid == IID_IPersistFolder )
    {
    
#if 0
        if( !g_poFontFolder )
        {
            g_poFontFolder = new CFontFolder;

            if( !g_poFontFolder )
            {
                DEBUGMSG( (DM_ERROR, TEXT( "FONTEXT: factory - no memory" ) ) );
                return( retval );
            }

            if( !g_poFontFolder->Init( ) )
            {
                DEBUGMSG( (DM_ERROR, TEXT( "FONTEXT: factory - no memory" ) ) );

                g_poFontFolder = 0;

                return( retval );
            }
        }

        //
        //  The ref count is initialized to 0 during creation, so we AddRef
        //  here and Release after CreateViewObject so it gets destroyed
        //  properly (if necessary)
        //

        g_poFontFolder->AddRef( );

        retval = g_poFontFolder->CreateViewObject( NULL, IID_IShellView, (void **)&poUnk );

        g_poFontFolder->Release( );

#else
        retval = CreateViewObject( (void **)&poUnk );
#endif
    }
    else if( riid == IID_IShellExtInit )
    {
        CShellExtInit * poExt = new CShellExtInit;

        if( !poExt || !poExt->bInit( ) )
        {
            DEBUGMSG( (DM_ERROR, TEXT( "FONTEXT: factory - no memory") ) );
            return( retval );
        }

        retval = poExt->QueryInterface( IID_IUnknown, (void **)&poUnk );
    }
    else if (riid == IID_IExtractIconW || 
             riid == IID_IExtractIconA ||
             riid == IID_IPersistFile)
    {
        CFontIconHandler *pfih = new CFontIconHandler;

        if (NULL == pfih)
        {
            DEBUGMSG( (DM_ERROR, TEXT( "FONTEXT: factory - no memory") ) );
            return (retval);
        }

        retval = pfih->QueryInterface(IID_IUnknown, (LPVOID *)&poUnk);
    }
   
    //
    //  If we got an IUnknown, then AddRef (above) before QI and then Release. 
    //  This will force the object to be deleted if QI fails.
    //
    //  This method of first querying for IUnknown then again for the
    //  actual interface of interest is really stupid and unnecessary.
    //  I've left it this way just because it works and I don't want to 
    //  risk breaking something that has been coded around this weirdness.
    //  [brianau - 07/23/97]
    //

    if( poUnk )
    {
        retval = poUnk->QueryInterface( riid, ppvObj );

        // DEBUGHRESULT( (retval) );

        poUnk->Release( );
    }

    return( retval );
}


STDMETHODIMP CImpIClassFactory::LockServer( BOOL fLock )
{
    DEBUGMSG( (DM_TRACE1, TEXT( "FONTEXT: CImpIClassFactory::LockServer called" ) ) );

    if( fLock ) 
        g_cLock++;
    else 
        g_cLock--;

    return( NOERROR );
}


HKEY OpenRegistryKey( TCHAR* szRegPath, TCHAR** ppValueName )
{
    TCHAR szCopy[ 300 ];
    int  lKey = lstrlen( szRegPath );
    int  i;
    
    lstrcpy( szCopy, szRegPath );

    for( i = 0; szCopy[ i ]; i++)
    {
        if( szCopy[ i ] == TEXT( '\\' ) )
        {
            szCopy[ i ] = TEXT( '\0' );
            *ppValueName = &szRegPath[ i+1 ];

            if( !szCopy[ i+1 ] )
            {
                DEBUGMSG( (DM_ERROR, TEXT( "REGEXT:  Value path name ended in a \\:%s" ),
                          szRegPath ) );

                return( NULL );
            }
        }
    }

    HKEY hKey;

    if( !lstrcmp( szCopy, TEXT( "HKEY_USERS" ) ) )              hKey = HKEY_USERS;
    else if( !lstrcmp( szCopy, TEXT( "HKEY_CURRENT_USER" ) ) )  hKey = HKEY_CURRENT_USER;
    else if( !lstrcmp( szCopy, TEXT( "HKEY_CLASSES_ROOT" ) ) )  hKey = HKEY_CLASSES_ROOT;
    else if( !lstrcmp( szCopy, TEXT( "HKEY_LOCAL_MACHINE" ) ) ) hKey = HKEY_LOCAL_MACHINE;
    else
    {
        DEBUGMSG( (DM_ERROR, TEXT( "FONTEXT:  Bad root node name:%s" ), szCopy ) );
        return( NULL );
    }
    
    for( i = 0; &szRegPath[ i+1 ] != *ppValueName; i++ )
    {
        if( !szCopy[ i ] )
        {
            HKEY hKeyPrev = hKey;
            DWORD dwDisp;

            if( ERROR_SUCCESS != RegCreateKeyEx( hKeyPrev,
                                                 &szCopy[ i+1 ],
                                                 NULL,
                                                 TEXT( "" ),
                                                 NULL,
                                                 KEY_ALL_ACCESS,
                                                 NULL,
                                                 &hKey,
                                                 &dwDisp ) )
            {
                DEBUGMSG( (DM_ERROR, TEXT( "FONTEXT:  Can't open key %s" ), &szCopy[ i+1 ]) );
                return( NULL );
            }

            if( dwDisp == REG_CREATED_NEW_KEY )
            {
                DEBUGMSG( (DM_ERROR, TEXT( "FONTEXT: Created new key:%s" ), &szCopy[ i+1 ]) );
            }

            // CRegID::CloseKey( hKeyPrev );

            RegCloseKey( hKey );
        }
    }
    
    return( hKey );
}

//
// We need a CLSID->string converter but I don't want to link to 
// ole32 to get it.  This isn't a terribly efficient implementation but
// we only call it once during DllRegServer so it doesn't need to be.
// [brianau - 2/23/99]
//
bool
GetClsidStringA(
    REFGUID clsid,
    LPSTR pszDest,
    UINT cchDest
    )
{
    bool bResult = false;
    if (cchDest >= GUIDSIZE)
    {
        wsprintfA(pszDest, 
                  "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
                  clsid.Data1,
                  clsid.Data2,
                  clsid.Data3,
                  clsid.Data4[0],
                  clsid.Data4[1],
                  clsid.Data4[2],
                  clsid.Data4[3],
                  clsid.Data4[4],
                  clsid.Data4[5],
                  clsid.Data4[6],
                  clsid.Data4[7]);

        bResult = true;
    }
    return bResult;
}



HRESULT
CreateDesktopIniFile(
    void
    )
{
    //
    // Get the path for the file (%windir%\fonts\desktop.ini)
    //
    TCHAR szPath[MAX_PATH * 2];
    HRESULT hr = SHGetSpecialFolderPath(NULL, szPath, CSIDL_FONTS, FALSE);
    if (SUCCEEDED(hr))
    {
        PathAppend(szPath, TEXT("desktop.ini"));
        //
        // Build the file's content.  Note that it's ANSI text.
        //
        char szClsid[GUIDSIZE];

        if (GetClsidStringA(CLSID_FontExt, szClsid, ARRAYSIZE(szClsid)))
        {
            const char szFmt[] = "[.ShellClassInfo]\r\nUICLSID=%s\r\n";
            char szText[ARRAYSIZE(szClsid) + ARRAYSIZE(szFmt)];
            DWORD dwBytesWritten;

            wsprintfA(szText, szFmt, szClsid);

            //
            // Always create the file.  Attr are SYSTEM+HIDDEN.
            //
            HANDLE hFile = CreateFile(szPath,
                                      GENERIC_WRITE,
                                      FILE_SHARE_READ,
                                      NULL,
                                      CREATE_ALWAYS,
                                      FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN,
                                      NULL);

            if (INVALID_HANDLE_VALUE != hFile)
            {
                //
                // Write out the contents.
                //
                if (!WriteFile(hFile, szText, lstrlenA(szText), &dwBytesWritten, NULL))
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                }
                CloseHandle(hFile);
            }
            else
                hr = HRESULT_FROM_WIN32(GetLastError());
        }
        else
            hr = E_UNEXPECTED;
    }
    return hr;
}


STDAPI DllRegisterServer(void)
{
    //
    // Currently, all we do is create the desktop.ini file.
    //
    return CreateDesktopIniFile();
}

STDAPI DllUnregisterServer(void)
{
    //
    // Do nothing.
    //
    return S_OK;
}
