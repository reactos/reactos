//-------------------------------------------------------------------------//
//
//  PTsniff.cpp - API for quick determination of file type support.
//
//-------------------------------------------------------------------------//
#include "pch.h"
#include "PTsniff.h"
#include "dictbase.h"

typedef struct tagPTSRV_FILE_TYPES
{
    PTSRV_FILETYPE    fileType ;
    PTSRV_FILECLASS   fileClass ;
    LPCTSTR             pszExt ;
} PTSRV_FILE_TYPES, *PPTSRV_FILE_TYPES, *LPPTSRV_FILE_TYPES;

static const PTSRV_FILE_TYPES ptsrv_file_types[]  =
{
    { PTSFTYPE_DOC,  PTSFCLASS_OFFICEDOC, TEXT("DOC") },
    { PTSFTYPE_XLS,  PTSFCLASS_OFFICEDOC, TEXT("XLS") },
    { PTSFTYPE_PPT,  PTSFCLASS_OFFICEDOC, TEXT("PPT") },

    { PTSFTYPE_BMP,  PTSFCLASS_IMAGE, TEXT("BMP") },
    { PTSFTYPE_EPS,  PTSFCLASS_IMAGE, TEXT("EPS") },
    { PTSFTYPE_GIF,  PTSFCLASS_IMAGE, TEXT("GIF") },
    { PTSFTYPE_JPG,  PTSFCLASS_IMAGE, TEXT("JPG") },
    { PTSFTYPE_FPX,  PTSFCLASS_IMAGE, TEXT("FPX") },
    { PTSFTYPE_JPG,  PTSFCLASS_IMAGE, TEXT("JPEG") },
    { PTSFTYPE_PCD,  PTSFCLASS_IMAGE, TEXT("PCD") },
    { PTSFTYPE_PCX,  PTSFCLASS_IMAGE, TEXT("PCX") },
    { PTSFTYPE_PICT, PTSFCLASS_IMAGE, TEXT("PICT") },
    { PTSFTYPE_PNG,  PTSFCLASS_IMAGE, TEXT("PNG") },
    { PTSFTYPE_TGA,  PTSFCLASS_IMAGE, TEXT("TGA") },
    { PTSFTYPE_TIF,  PTSFCLASS_IMAGE, TEXT("TIF") },
    { PTSFTYPE_TIF,  PTSFCLASS_IMAGE, TEXT("TIFF") },

    { PTSFTYPE_AVI,  PTSFCLASS_VIDEO,  TEXT("avi") },
    { PTSFTYPE_WAV,  PTSFCLASS_AUDIO,  TEXT("wav") },
    { PTSFTYPE_MIDI, PTSFCLASS_AUDIO,  TEXT("mid") },
    { PTSFTYPE_MIDI, PTSFCLASS_AUDIO,  TEXT("midi") },

    { PTSFTYPE_HTML, PTSFCLASS_UNSUPPORTED, TEXT("htm") },
    { PTSFTYPE_HTML, PTSFCLASS_UNSUPPORTED, TEXT("html") },
    { PTSFTYPE_XML,  PTSFCLASS_UNSUPPORTED, TEXT("xml") },
    { PTSFTYPE_LNK,  PTSFCLASS_UNSUPPORTED, TEXT("lnk") },
    { PTSFTYPE_XML,  PTSFCLASS_UNSUPPORTED, TEXT("url") },
} ;

#define cFILETYPES sizeof(ptsrv_file_types)/sizeof(PTSRV_FILE_TYPES)

//-------------------------------------------------------------------------//
LPCTSTR ExtFromPath( LPCTSTR pszPath )
{
    LPCTSTR pszExt = NULL ;
    for( int i = lstrlen( pszPath )-1; i>=0; i-- )
    {
        if( pszPath[i]==TEXT('.') )  {
            pszExt = &pszPath[i+1] ;
            break ;
        }
    }
    return pszExt ;
}

//-------------------------------------------------------------------------//
BOOL WINAPI IsPTsrvKnownFileType(
    LPCTSTR pszPath,
    OUT OPTIONAL PTSRV_FILETYPE* pType,
    OUT OPTIONAL PTSRV_FILECLASS* pClass )
{
    BOOL    bRet = FALSE ;

    if( pType )  *pType  = PTSFTYPE_UNKNOWN ;
    if( pClass ) *pClass = PTSFCLASS_UNKNOWN ;

    LPCTSTR pszExt = ExtFromPath( pszPath ) ;
    if( pszExt && *pszExt )
    {
        for( int j=0, max = cFILETYPES ; j < max; j++ )
        {
            if( lstrcmpi( pszExt, ptsrv_file_types[j].pszExt )==0 )
            {
                if( pType )  *pType  = ptsrv_file_types[j].fileType ;
                if( pClass ) *pClass = ptsrv_file_types[j].fileClass ;
                bRet = TRUE ;
                break ;
            }
        }
    }

    return bRet ;
}

//-------------------------------------------------------------------------//
BOOL WINAPI IsPropSetStgFmt( IN LPCTSTR pszPath, ULONG dwStgFmt, OUT OPTIONAL LPBOOL pbWriteAccess )
{
    LPPROPERTYSETSTORAGE ppss = NULL ;
    USES_CONVERSION ;

    if( pbWriteAccess )
    {
        *pbWriteAccess = FALSE;
        if( SUCCEEDED( StgOpenStorageEx( T2CW( pszPath ), STGM_DIRECT|STGM_READWRITE|STGM_SHARE_EXCLUSIVE,
                       dwStgFmt, 0, 0, NULL, IID_IPropertySetStorage, (PVOID*)&ppss ) ) )
        {
            *pbWriteAccess = FALSE;
            ppss->Release();
            return TRUE;
        }
    }

    if( SUCCEEDED( StgOpenStorageEx( T2W( (LPTSTR)pszPath), STGM_DIRECT|STGM_READ|STGM_SHARE_EXCLUSIVE,
                                     dwStgFmt, 0, 0, NULL, IID_IPropertySetStorage, (PVOID*)&ppss )) )
    {
        ppss->Release() ;
        return TRUE ;
    }
    return FALSE ;
}

//-------------------------------------------------------------------------//
EXTERN_C BOOL WINAPI IsOfficeDocFile( IN LPCTSTR pszPath )
{
    PTSRV_FILECLASS ptfc ;
    return IsPTsrvKnownFileType( pszPath, NULL, &ptfc ) &&
           PTSFCLASS_OFFICEDOC == ptfc ;
}

//-------------------------------------------------------------------------//
//  File extension wrapper
typedef class tagPROPFILEEXT
{
public:
    tagPROPFILEEXT() : _bAdvanced(1) { *_szExt = 0 ; }
    tagPROPFILEEXT( LPCTSTR pszExt ) : _bAdvanced(1)  {
        if( pszExt ) lstrcpyn( _szExt, pszExt, ARRAYSIZE(_szExt) ) ;
        else *_szExt = 0 ;
    }
    BOOL operator == ( const tagPROPFILEEXT& other ) const   {
        return  _bAdvanced == other._bAdvanced && 
                0 == lstrcmpi( _szExt, other._szExt ) ;
    }
    ULONG Hash() const    {
        return HashStringi( (LPTSTR)_szExt ) + (_bAdvanced ? 1 : 0) ;
    }
    TCHAR _szExt[MAX_PATH/2] ;
    BOOL  _bAdvanced ;

} PROPFILEEXT, *PPROPFILEEXT, *LPPROPFILEEXT ;

//-------------------------------------------------------------------------//
//  Global map of file name extension to IAdvancedPropertyServer clsids.
typedef TDictionaryBase< PROPFILEEXT, CLSID >
        CFileAssocMapBase ;

class CFileAssocMap : public CFileAssocMapBase
//-------------------------------------------------------------------------//
{
public:
    CFileAssocMap()
        :   CFileAssocMapBase( 4 /*allocation block size*/ ) {}

    BOOL    Lookup( LPCTSTR pszExt, BOOL bAdvanced, CLSID* pclsidServer ) const ;
    BOOL    Insert( LPCTSTR pszExt, BOOL bAdvanced, REFCLSID clsidServer ) ;

protected:
    virtual ULONG HashKey( const tagPROPFILEEXT& ext ) const {
        return ext.Hash() ;
    }
    virtual BOOLEAN IsEqual( PROPFILEEXT& key1, PROPFILEEXT& key2, BOOLEAN& bHandled ) const {
        bHandled = TRUE ;  return (BOOLEAN)(key1 == key2) ;
    }
} ;

CFileAssocMap _theFileAssocMap ;    // static global instance


//-------------------------------------------------------------------------//
BOOL CFileAssocMap::Lookup( LPCTSTR pszExt, BOOL bAdvanced, CLSID* pclsidServer ) const
{
    PROPFILEEXT ext( pszExt ) ;
    ext._bAdvanced = bAdvanced ;
    return (pszExt && *pszExt) ? CFileAssocMapBase::Lookup( ext, *pclsidServer ) : FALSE ;
}

//-------------------------------------------------------------------------//
BOOL CFileAssocMap::Insert( LPCTSTR pszExt, BOOL bAdvanced, REFCLSID clsidServer )
{
    PROPFILEEXT ext( pszExt ) ;
    CLSID       clsid = clsidServer ;
    int         c = Count() ;
    ext._bAdvanced = bAdvanced ;

    if( pszExt && *pszExt )
    {
        if( Lookup( pszExt, bAdvanced, &clsid ) )
            return TRUE ;

        (*this)[ext] = clsid ;
        return c < Count() ;
    }
    return FALSE ;
}


//-------------------------------------------------------------------------//
//  Retrieves the appropriate server for the indicated source file.
STDMETHODIMP GetPropServerClassForFile(
    IN LPCTSTR pszPath,
    BOOL bAdvanced,
    OUT LPCLSID pclsid )
{
    HRESULT hr = E_FAIL;
    LPCTSTR pszExt = NULL;
    TCHAR   szSubKey[MAX_PATH];
    TCHAR   szProgID[MAX_PATH];
    USES_CONVERSION;

    static LPCTSTR pszAdvancedHandlerKey = TEXT("ShellEx\\AdvancedPropertyHandlers"),
                   pszBasicHandlerKey    = TEXT("ShellEx\\BasicPropertyHandlers");

    LPCTSTR pszHandlerKey = bAdvanced ? pszAdvancedHandlerKey : pszBasicHandlerKey ;

    ASSERT( pclsid ) ;
    ASSERT( pszPath && *pszPath ) ;

    *pclsid = CLSID_NULL ;
    if( (pszExt = PathFindExtension( pszPath )) == NULL )
        return hr;
    lstrcpy( szSubKey, pszExt );

    //  check our cache for an association...
    if( _theFileAssocMap.Lookup( pszExt, bAdvanced, pclsid ) )
        return S_OK ;

    HKEY  hkeyExt = NULL,
          hkeyClsid = NULL  ;
    DWORD dwType, cbSize, dwRet ;
    BOOL  bContinue = TRUE ;

    while( bContinue )
    {
        //  Open subkey
        dwRet = RegOpenKeyEx( HKEY_CLASSES_ROOT, szSubKey, 0L, KEY_READ, &hkeyExt ) ;
        if( ERROR_SUCCESS == dwRet )
        {
            //  Enumerate ShellEx\\{Advanced/Basic}PropertyHandlers
            dwRet = RegOpenKeyEx( hkeyExt, pszHandlerKey,
                                  0L, KEY_READ, &hkeyClsid ) ;

            if( ERROR_SUCCESS == dwRet )
            {
                for( DWORD i = 0; bContinue && ERROR_SUCCESS == dwRet; i++ )
                {
                    TCHAR szClsid[128],
                          szClass[128] ;
                    DWORD cchClsid = ARRAYSIZE(szClsid),
                          cchClass = ARRAYSIZE(szClass) ;
                    FILETIME ft ;

                    dwRet = RegEnumKeyEx( hkeyClsid, i, szClsid, &cchClsid, NULL,
                                          szClass, &cchClass, &ft ) ;

                    if( ERROR_SUCCESS == dwRet )
                    {
                        if( SUCCEEDED( CLSIDFromString( T2W(szClsid), pclsid ) ) )
                        {
                            bContinue = FALSE ;
                            //  cache the extension, server association.
                            _theFileAssocMap.Insert( pszExt, bAdvanced, *pclsid ) ;
                            hr = S_OK ;
                        }
                    }
                }
                RegCloseKey( hkeyClsid ) ;
                hkeyClsid = NULL ;
            }

            //  If we don't have a server at this point, check for progID...
            if( bContinue )
            {
                cbSize = sizeof(szProgID) ;
                dwRet = RegQueryValueEx( hkeyExt, NULL, 0L, &dwType,
                                        (LPBYTE)szProgID, &cbSize ) ;
                if( ERROR_SUCCESS == dwRet )
                {
                    if( 0 == lstrcmpi( szSubKey, szProgID ) )
                        bContinue = FALSE ; // progid == description; weird case.
                    else
                        lstrcpy( szSubKey, szProgID ); // got progID; loop and try again
                }
                else
                    bContinue = FALSE ;
            }

            RegCloseKey( hkeyExt ) ;
            hkeyExt = NULL ;
        }
        else
            bContinue = FALSE ;
    }

    return hr ;
}

//-------------------------------------------------------------------------//
//  Retrieves the appropriate server for the indicated source file.
BOOL OpenRegKeyForExtension( LPCTSTR pszExt, REGSAM samDesired, OUT HKEY* phKey )
{
    ASSERT( pszExt && *pszExt );
    ASSERT( phKey );

    HKEY  hKey;
    TCHAR szSubKey[MAX_PATH];
    BOOL  bContinueNext = TRUE;

    *phKey = NULL;
    lstrcpyn( szSubKey, pszExt, ARRAYSIZE(szSubKey) );

    while( bContinueNext )
    {
        bContinueNext = FALSE;

        if( RegOpenKeyEx( HKEY_CLASSES_ROOT, szSubKey,
                          0L, samDesired|KEY_READ, &hKey ) == ERROR_SUCCESS )
        {
            if( *phKey )
                RegCloseKey( *phKey );

            *phKey = hKey;

            TCHAR szProgID[MAX_PATH];
            DWORD dwType,
                  cbRet = sizeof(szProgID)+sizeof(TCHAR);
            HKEY  hkeyShellEx = NULL;

            //  If we have a shellex subkey here, no neec to go further.
            if( RegOpenKeyEx( hKey, TEXT("ShellEx"), 0L, KEY_READ, &hkeyShellEx ) == ERROR_SUCCESS )
                RegCloseKey( hkeyShellEx );
            else  // no shellex key, so pine down further.
            {
                if( RegQueryValueEx( hKey, NULL, NULL,
                        &dwType, (LPBYTE)szProgID, &cbRet ) == ERROR_SUCCESS && *szProgID )
                {
                    lstrcpyn( szSubKey, szProgID, ARRAYSIZE(szSubKey) );
                    bContinueNext = TRUE;
                }
                else
                {
                    //  If our intention is to write out a shellex subkey, and we ran to the 
                    //  last progid in the chain without finding one, we slam one in.
                    if( samDesired & KEY_WRITE )
                    {
                        DWORD dwDisp = 0L;
                        if( RegCreateKeyEx( hKey, TEXT("ShellEx"), 0L, NULL, 0, KEY_WRITE, NULL,
                                            &hkeyShellEx, &dwDisp ) == ERROR_SUCCESS )
                        {
                            RegCloseKey( hkeyShellEx );
                        }
                    }
                }
            }
        }
    }

    return *phKey != NULL;
}

HRESULT RegisterPropServerClassForExtension( IN LPCTSTR pszExt, REFCLSID clsid )
{
    OLECHAR szClsid[48] ;
    HRESULT hr = E_FAIL ;

    if( SUCCEEDED( (hr = StringFromGUID2( clsid, szClsid, ARRAYSIZE(szClsid) )) ) )
    {
        HKEY hKey, hSubKey ;
        DWORD dwErr, dwDisp ;

        if( !OpenRegKeyForExtension( pszExt, KEY_WRITE, &hKey ) )
            return E_FAIL ;

        dwErr = RegCreateKeyEx( hKey, TEXT("ShellEx\\AdvancedPropertyHandlers"),
                                0L, NULL, 0L, KEY_WRITE, NULL, &hSubKey, &dwDisp ) ;
        hr = HRESULT_FROM_WIN32( dwErr ) ;

        if( ERROR_SUCCESS == dwErr )
        {
            HKEY hkeyClsid ;
            dwErr = RegCreateKeyEx( hSubKey, szClsid, 0L, NULL, 0L, KEY_WRITE, NULL,
                                    &hkeyClsid, &dwDisp ) ;
            hr = HRESULT_FROM_WIN32( dwErr ) ;

            if( ERROR_SUCCESS == dwErr )
                RegCloseKey( hkeyClsid ) ;

            RegCloseKey( hSubKey ) ;
        }

        RegCloseKey( hKey ) ;
    }

    return hr ;
}
