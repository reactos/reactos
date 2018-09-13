//-------------------------------------------------------------------------//
//
//  colprov.cpp
//
//-------------------------------------------------------------------------//

#include "pch.h"
#include "defsrv32.h"

//-------------------------------------------------------------------------//
//
//  Exe, DLL Version Information column provider 
//
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
//  FMTID_ExeDllInformation,
//// {0CEF7D53-FA64-11d1-A203-0000F81FEDEE}
#define PSFMTID_VERSION { 0xcef7d53, 0xfa64, 0x11d1, 0xa2, 0x3, 0x0, 0x0, 0xf8, 0x1f, 0xed, 0xee }

// #define PIDVSI_CompanyName       0x002
#define PIDVSI_FileDescription   0x003
#define PIDVSI_FileVersion       0x004
#define PIDVSI_InternalName      0x005
#define PIDVSI_OriginalFileName  0x006
#define PIDVSI_ProductName       0x007
#define PIDVSI_ProductVersion    0x008

//  Exe, DLL Version Information column identifier defs...
const SHCOLUMNID SCID_CompanyName        = { PSGUID_DOCUMENTSUMMARYINFORMATION, PIDDSI_COMPANY};
const SHCOLUMNID SCID_FileDescription    = { PSFMTID_VERSION, PIDVSI_FileDescription};
const SHCOLUMNID SCID_FileVersion        = { PSFMTID_VERSION, PIDVSI_FileVersion};
const SHCOLUMNID SCID_InternalName       = { PSFMTID_VERSION, PIDVSI_InternalName};
const SHCOLUMNID SCID_OriginalFileName   = { PSFMTID_VERSION, PIDVSI_OriginalFileName};
const SHCOLUMNID SCID_ProductName        = { PSFMTID_VERSION, PIDVSI_ProductName};
const SHCOLUMNID SCID_ProductVersion     = { PSFMTID_VERSION, PIDVSI_ProductVersion};

typedef struct {
    const SHCOLUMNID *pscid;
    DWORD id;
    DWORD dwWid;
    VARTYPE vt;             // Note that the type of a given FMTID/PID pair is a known, fixed value
} VERCOLMAP;

//  Static column data
const VERCOLMAP c_rgExeDllColumns[] =
{
    { &SCID_CompanyName,        IDS_PIDVSI_COMPANYNAME,      20, VT_BSTR },
    { &SCID_FileDescription,    IDS_PIDVSI_FILEDESCRIPTION,  20, VT_BSTR },
    { &SCID_FileVersion,        IDS_PIDVSI_FILEVERSION,      20, VT_BSTR },
//  { &SCID_InternalName,       IDS_PIDVSI_INTERNALNAME,     20, VT_BSTR },
//  { &SCID_OriginalFileName,   IDS_PIDVSI_ORIGINALFILENAME, 20, VT_BSTR },
    { &SCID_ProductName,        IDS_PIDVSI_PRODUCTNAME,      20, VT_BSTR },
    { &SCID_ProductVersion,     IDS_PIDVSI_PRODUCTVERSION,   20, VT_BSTR },
};

STDMETHODIMP CExeVerColumnProvider::GetColumnInfo(DWORD dwIndex, SHCOLUMNINFO *psci)
{
    ZeroMemory(psci, sizeof(*psci));

    if (dwIndex < (UINT)ARRAYSIZE(c_rgExeDllColumns) )
    {
        psci->scid = *c_rgExeDllColumns[dwIndex].pscid;
        psci->cChars = c_rgExeDllColumns[dwIndex].dwWid;
        psci->vt = c_rgExeDllColumns[dwIndex].vt;
        psci->fmt = LVCFMT_LEFT;
        psci->csFlags = SHCOLSTATE_TYPE_STR;

        LoadStringW(_Module.GetModuleInstance(), c_rgExeDllColumns[dwIndex].id, psci->wszTitle, ARRAYSIZE(psci->wszTitle));
        return S_OK;
    }
    return S_FALSE;
}

void CExeVerColumnProvider::_ClearCache()
{
    if (m_pvAllTheInfo)
    {
        delete m_pvAllTheInfo;
        m_pvAllTheInfo = NULL;
    }
    m_szFileCache[0] = 0;
}

HRESULT CExeVerColumnProvider::_CacheFileVerInfo(LPCWSTR pszFile)
{
    if (StrCmpW(m_szFileCache, pszFile))
    {
        HRESULT hr;
        _ClearCache();

        DWORD dwVestigial;
        DWORD versionISize = GetFileVersionInfoSizeW((LPWSTR)pszFile, &dwVestigial); // cast for bad API design
        if (versionISize)
        {
            m_pvAllTheInfo = new BYTE[versionISize];
            if (m_pvAllTheInfo)
            {
                // read the data
                if (GetFileVersionInfoW((LPWSTR)pszFile, dwVestigial, versionISize, m_pvAllTheInfo))
                {
                    hr = S_OK;
                }
                else
                {
                    _ClearCache();
                    hr = E_FAIL;
                }
            }
            else
                hr = E_OUTOFMEMORY; // error, out of memory.
        }
        else
            hr = S_FALSE;

        StrCpyNW(m_szFileCache, pszFile, ARRAYSIZE(m_szFileCache));
        m_hrCache = hr;
    }
    return m_hrCache;
}

STDMETHODIMP CExeVerColumnProvider::GetItemData(LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT *pvarData)
{
    if (pscd->dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_OFFLINE))
        return S_FALSE;

    HRESULT hr = _CacheFileVerInfo(pscd->wszFile);
    if (hr != S_OK)
        return hr;

    TCHAR   szString[128], *pszVersionInfo = NULL; //A pointer to the specific version info I am looking for
    LPCTSTR pszVersionField = NULL ;

    switch (pscid->pid){
        case PIDVSI_FileVersion:
            {
                VS_FIXEDFILEINFO *pffi;
                UINT uInfoSize;
                if (VerQueryValue(m_pvAllTheInfo, TEXT("\\"), (void **)&pffi, &uInfoSize))
                {
                    wnsprintf(szString, ARRAYSIZE(szString), TEXT("%d.%d.%d.%d"), 
                        HIWORD(pffi->dwFileVersionMS),
                        LOWORD(pffi->dwFileVersionMS),
                        HIWORD(pffi->dwFileVersionLS),
                        LOWORD(pffi->dwFileVersionLS));

                    pszVersionInfo = szString;
                }
                else
                    pszVersionField = TEXT("FileVersion");      
            }
            break;

        case PIDDSI_COMPANY:            pszVersionField = TEXT("CompanyName");      break;
        case PIDVSI_FileDescription:    pszVersionField = TEXT("FileDescription");  break;
        case PIDVSI_InternalName:       pszVersionField = TEXT("InternalName");     break;
        case PIDVSI_OriginalFileName:   pszVersionField = TEXT("OriginalFileName"); break;
        case PIDVSI_ProductName:        pszVersionField = TEXT("ProductName");      break;
        case PIDVSI_ProductVersion:     pszVersionField = TEXT("ProductVersion");   break;
        default: 
            return E_FAIL;
        };
    //look for the intended language in the examined object.

    if (pszVersionInfo == NULL)
    {
        struct _VERXLATE
        {
            WORD wLanguage;
            WORD wCodePage;
        } *pxlate;                     /* ptr to translations data */

        //this is a fallthrough set of if statements.
        //on a failure, it just tries the next one, until it runs out of tries.
        UINT uInfoSize;
        if (VerQueryValue(m_pvAllTheInfo, TEXT("\\VarFileInfo\\Translation"), (void **)&pxlate, &uInfoSize))
        {
            TCHAR szVersionKey[60];   //a string to hold all the format string for VerQueryValue
            wnsprintf(szVersionKey, ARRAYSIZE(szVersionKey), TEXT("\\StringFileInfo\\%04X%04X\\%s"),
                                                pxlate[0].wLanguage, pxlate[0].wCodePage, pszVersionField );
            if (!VerQueryValue(m_pvAllTheInfo, szVersionKey, (void **) &pszVersionInfo, &uInfoSize))
            {
    #ifdef UNICODE
                wnsprintf(szVersionKey, ARRAYSIZE(szVersionKey), TEXT("\\StringFileInfo\\040904B0\\%s"), pszVersionField);
                if (!VerQueryValue(m_pvAllTheInfo, szVersionKey, (void **) &pszVersionInfo, &uInfoSize))
    #endif
                {
                    wnsprintf(szVersionKey, ARRAYSIZE(szVersionKey), TEXT("\\StringFileInfo\\040904E4\\%s"), pszVersionField);
                    if (!VerQueryValue(m_pvAllTheInfo, szVersionKey, (void **) &pszVersionInfo, &uInfoSize))
                    {
                        wnsprintf(szVersionKey, ARRAYSIZE(szVersionKey), TEXT("\\StringFileInfo\\04090000\\%s"), pszVersionField);
                        if (!VerQueryValue(m_pvAllTheInfo, szVersionKey, (void **) &pszVersionInfo, &uInfoSize))
                        {
                            pszVersionInfo = NULL;
                        }
                    }
                }
            }
        }
    }
    
    //getting here is success, prepare the output, clean up memory and exit.
    if (pszVersionInfo)
    {
        USES_CONVERSION;

        pvarData->bstrVal = T2BSTR( pszVersionInfo );
        if (pvarData->bstrVal)
        {
            pvarData->vt = VT_BSTR;
            hr = S_OK;
        }
        else
            hr = E_OUTOFMEMORY;
    }
    else
        hr = E_FAIL;

    return hr;
};



//-------------------------------------------------------------------------//
//  late-bound imports.
#if defined(UNICODE) || defined(_UNICODE)
    typedef BOOL  (APIENTRY * PFNVERQUERYVALUE)( const void *, LPWSTR, void * *, PUINT ) ;
    #define szVERQUERYVALUE              "VerQueryValueW"
    typedef DWORD (APIENTRY * PFNGETFILEVERSIONINFOSIZE)( LPWSTR, LPDWORD ) ;
    #define szGETFILEVERSIONINFOSIZE    "GetFileVersionInfoSizeW"
    typedef DWORD (APIENTRY * PFNGETFILEVERSIONINFO)( LPWSTR, DWORD, DWORD, void * ) ;
    #define szGETFILEVERSIONINFO        "GetFileVersionInfoW"
#else
    typedef BOOL  (APIENTRY * PFNVERQUERYVALUE)( const void *, LPSTR, void * *, PUINT ) ;
    #define szVERQUERYVALUE              "VerQueryValueA"
    typedef DWORD (APIENTRY * PFNGETFILEVERSIONINFOSIZE)( LPSTR, LPDWORD ) ;
    #define szGETFILEVERSIONINFOSIZE    "GetFileVersionInfoSizeA"
    typedef DWORD (APIENTRY * PFNGETFILEVERSIONINFO)( LPSTR, DWORD, DWORD, void * ) ;
    #define szGETFILEVERSIONINFO        "GetFileVersionInfoA"
#endif

HMODULE                     g_hImportLib = NULL;
PFNVERQUERYVALUE            g_pfnVQV = NULL;
PFNGETFILEVERSIONINFOSIZE   g_pfnGFVIS = NULL;
PFNGETFILEVERSIONINFO       g_pfnGFVI = NULL;

FARPROC CExeVerColumnProvider::_GetVerProc( LPCSTR pszName )
{
    if( NULL == g_hImportLib && 
        NULL == (g_hImportLib = LoadLibrary( TEXT("version.dll") )) )
        return NULL ;
    return GetProcAddress( g_hImportLib, pszName ) ;
}

BOOL CExeVerColumnProvider::VerQueryValue(const void *pBlock, LPTSTR lpSubBlock, void **lplpBuffer, PUINT puLen )
{
    if( NULL == g_pfnVQV && 
        NULL == (g_pfnVQV = (PFNVERQUERYVALUE)_GetVerProc( szVERQUERYVALUE )) )
        return FALSE ;
    return g_pfnVQV( pBlock, lpSubBlock, lplpBuffer, puLen ) ;
}

BOOL CExeVerColumnProvider::GetFileVersionInfoSize(LPTSTR pszFile, DWORD *lpdwHandle)
{
    if( NULL == g_pfnGFVIS &&
        NULL == (g_pfnGFVIS = (PFNGETFILEVERSIONINFOSIZE)_GetVerProc( szGETFILEVERSIONINFOSIZE )) )
        return FALSE ;
    return g_pfnGFVIS( pszFile, lpdwHandle ) ;
}

BOOL CExeVerColumnProvider::GetFileVersionInfo(LPTSTR pszFile, DWORD dwHandle, DWORD dwLen, void *lpData)
{
    if( NULL == g_pfnGFVI &&
        NULL == (g_pfnGFVI = (PFNGETFILEVERSIONINFO)_GetVerProc( szGETFILEVERSIONINFO )) )
        return FALSE ;
    return g_pfnGFVI( pszFile, dwHandle, dwLen, lpData ) ;
}
