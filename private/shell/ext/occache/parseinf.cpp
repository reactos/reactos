///////////////////////////////////////////////////////////////////////////////
// Implementation of class CParseInf
//
// CParseInf is created to deal with parsing of an INF file.

#include <ole2.h>
#include "ParseInf.h"
#include "resource.h"
#include "init.h"
#include "global.h"
#include <shlwapi.h>
#include <initguid.h>
#include <pkgguid.h>
#include <cleanoc.h>        // for STATUS_CTRL values
#include <mluisupp.h>

#define ARRAYSIZE(a)                (sizeof(a)/sizeof(a[0]))

static BOOL FGetCLSIDFile( LPTSTR szFile, LPCTSTR szCLSID )
{
    BOOL fGotIt = FALSE;
    HKEY hkeyClsid;
    TCHAR szT[MAX_PATH];
    TCHAR *szPath = CatPathStrN( szT, HKCR_CLSID, szCLSID, MAX_PATH );

    if ( RegOpenKeyEx( HKEY_CLASSES_ROOT, szPath, 0, KEY_READ, &hkeyClsid ) == ERROR_SUCCESS )
    {
        DWORD dw;
        LRESULT lResult;

        // Look for InprocServer[32] or LocalServer[32] key
        dw = MAX_PATH;
        lResult = RegQueryValue(hkeyClsid, INPROCSERVER32, szT, (PLONG)&dw);
        if (lResult != ERROR_SUCCESS)
        {
            dw = MAX_PATH;
            lResult = RegQueryValue(hkeyClsid, LOCALSERVER32, szT, (PLONG)&dw);
        }

        if (lResult != ERROR_SUCCESS)
        {
            dw = MAX_PATH;
            lResult = RegQueryValue(hkeyClsid, INPROCSERVERX86, szT, (PLONG)&dw);
        }

        if (lResult != ERROR_SUCCESS)
        {
            dw = MAX_PATH;
            lResult = RegQueryValue(hkeyClsid, LOCALSERVERX86, szT, (PLONG)&dw);
        }

        if ( lResult == ERROR_SUCCESS )
        {
            if ( OCCGetLongPathName( szFile, szT, MAX_PATH ) == 0 )
                lstrcpy( szFile, szT );
            fGotIt = TRUE;
        }
        
        RegCloseKey( hkeyClsid );
    }

    return fGotIt;
}

// constructor
CParseInf::CParseInf()
{
    m_pHeadFileList = NULL;
    m_pCurFileNode = NULL;
    m_pFileRetrievalPtr = NULL;
    m_pHeadPackageList = NULL;
    m_pCurPackageNode = NULL;
    m_pPackageRetrievalPtr = NULL;
    m_bIsDistUnit = FALSE;
    m_bHasActiveX = FALSE;
    m_bHasJava = FALSE;
    m_pijpm = NULL;
    m_bCoInit = FALSE;
    m_dwStatus = STATUS_CTRL_UNKNOWN;
    GetDaysBeforeExpireGeneral( &m_cExpireDays );
}

// destructor
CParseInf::~CParseInf()
{
    DestroyFileList();
    DestroyPackageList();

    if ( m_pijpm != NULL )
        m_pijpm->Release();

    if ( m_bCoInit )
        CoUninitialize();
}

// initialization
void CParseInf::Init()
{
    m_dwFileSizeSaved = 0;
    m_dwTotalFileSize = 0;
    m_nTotalFiles = 0;
    m_pHeadFileList = m_pCurFileNode = NULL;
    m_pHeadPackageList = m_pCurPackageNode = NULL;

    lstrcpyn(m_szInf, m_szFileName, ARRAYSIZE(m_szInf));
    TCHAR *pCh = ReverseStrchr(m_szInf, '.');
    if (pCh != NULL)
        *pCh = '\0';
    if ( lstrlen(m_szInf) + lstrlen(INF_EXTENSION) < ARRAYSIZE(m_szInf))
        lstrcat(m_szInf, INF_EXTENSION);
    else
        m_szInf[0] = 0; // if can't hold it, we can't hold it.
 }

// release memory used by a linked list of files
void CParseInf::DestroyFileList()
{
    if (m_pHeadFileList != NULL)
        delete m_pHeadFileList;
    m_pHeadFileList = m_pCurFileNode = NULL;
}

void CParseInf::DestroyPackageList()
{
    if (m_pHeadPackageList != NULL)
        delete m_pHeadPackageList;
    m_pHeadPackageList = m_pCurPackageNode = NULL;
}


// find inf from cache directory if one with the
// same name as the OCX is not found
HRESULT CParseInf::FindInf(LPTSTR szInf)
{
    HRESULT hr = S_OK;
    WIN32_FIND_DATA dataFile;
    HANDLE h = INVALID_HANDLE_VALUE;
    DWORD dwLen = 0;
    TCHAR szValueBuf[MAX_PATH];        
    TCHAR *szOcxFileName = ReverseStrchr(m_szFileName, '\\');
    int nCachePathLength = 0, i = 0;

    Assert(szOcxFileName != NULL);
    szOcxFileName += 1;
    Assert (szInf != NULL);
    if (szInf == NULL)
        goto ExitFindInf;

    // search for inf file in two directories.  First the dir where the
    // OCX is, then the OC cache dir.
    for (i = 0; dwLen == 0 && i < 2; i++)
    {
        if (i == 0)
            hr = GetDirectory(GD_EXTRACTDIR, szInf, ARRAYSIZE(szInf), m_szFileName);
        else
        {
            TCHAR szTemp[MAX_PATH];
            hr = GetDirectory(GD_CACHEDIR, szTemp, ARRAYSIZE(szTemp));
            if (lstrcmpi(szTemp, szInf) == 0)
                continue;
            lstrcpy(szInf, szTemp);
        }

        if (FAILED(hr))
            goto ExitFindInf;

        lstrcat(szInf, TEXT("\\"));
        nCachePathLength = lstrlen(szInf);
        lstrcat(szInf, TEXT("*"));
        lstrcat(szInf, INF_EXTENSION);
        h = FindFirstFile(szInf, &dataFile);
        if (h == INVALID_HANDLE_VALUE)
        {
            goto ExitFindInf;
        }

        // find an inf file with a section in [Add.Code] dedicated
        // to the OCX file in question
        do {
            szInf[nCachePathLength] = '\0';
            lstrcat(szInf, (LPCTSTR)dataFile.cFileName);
            dwLen = GetPrivateProfileString(
                                    KEY_ADDCODE,
                                    szOcxFileName,
                                    DEFAULT_VALUE,
                                    szValueBuf,
                                    MAX_PATH,
                                    szInf);
        } while(dwLen == 0 && FindNextFile(h, &dataFile));
    }
    
    hr = (dwLen != 0 ? hr : S_FALSE);

ExitFindInf:    

    if (h != INVALID_HANDLE_VALUE)
        FindClose(h);

    if (hr != S_OK)
        szInf[0] = '\0';

    return hr;
}

// initiate parsing of INF file
// szCLSID -- address to a buffer storing CLSID of control
// szOCXFileName -- full path and name (ie. long file name) of OCX file
HRESULT CParseInf::DoParse(
                  LPCTSTR szOCXFileName, 
                  LPCTSTR szCLSID)
{
    Assert(szOCXFileName != NULL);
    Assert(szCLSID != NULL);

    HRESULT hr = S_OK;
    const TCHAR *pszPath = NULL;
    TCHAR szFileName[MAX_PATH];
    DWORD dwFileSize = 0;

    if ( FGetCLSIDFile( szFileName, szCLSID ) &&
         lstrcmpi( szFileName, szOCXFileName ) != 0 )
        m_dwStatus = STATUS_CTRL_UNPLUGGED;


    // If DoParse was called, we are assumed to be a legacy control and not
    // a distribution unit (subsequent call to DoParseDU will change the
    // status). This information is required for control removal purposes.

    m_bIsDistUnit = FALSE;
    m_bHasActiveX = TRUE;  // all legacy controls are ActiveX

    // initialization

    if ( OCCGetLongPathName(m_szFileName, szOCXFileName, MAX_PATH) == 0 )
        lstrcpyn( m_szFileName, szOCXFileName, MAX_PATH );

    lstrcpyn(m_szCLSID, szCLSID, MAX_CLSID_LEN);
    DestroyFileList();
    Init();

    BOOL bOCXRemovable = IsModuleRemovable(m_szFileName);

    // test INF file existance, if not, try to find one in OC cache dir.
    if (!FileExist(m_szInf))
    {
        if (!ReadInfFileNameFromRegistry(m_szCLSID, m_szInf, MAX_PATH))
        {
            FindInf(m_szInf);

            // record inf file name into the registry
            WriteInfFileNameToRegistry(
                               m_szCLSID, 
                               (m_szInf[0] == '\0' ? NULL : m_szInf));
        }
    }

    // enumerate files assocated with a particular OCX
    if (FAILED(hr = EnumSections()))
        goto ExitDoParse;

    // S_FALSE is returned when an ocx has no inf file
    if (hr == S_FALSE)
    {
        m_nTotalFiles = 1;
        if (FAILED(GetSizeOfFile(m_szFileName, &m_dwFileSizeSaved)))
        {
            m_dwFileSizeSaved = 0;
            m_dwTotalFileSize = 0;
        }
        else
        {
            m_dwTotalFileSize = m_dwFileSizeSaved;
        }
        hr = S_OK;
        if ( !PathFileExists( m_szFileName ) )
            m_dwStatus = STATUS_CTRL_DAMAGED;
        else
            m_dwStatus = STATUS_CTRL_INSTALLED;
        goto ExitDoParse;
    }

    // OCX has an corresponding INF file.
    // Loop through the list of assocated files to dig out info for each
    // from their corresponding section in the INF file
    for (m_pCurFileNode = m_pHeadFileList;
         m_pCurFileNode != NULL;
         m_pCurFileNode = m_pCurFileNode->GetNextFileNode(), hr = S_OK)
    {
        // if m_pCurFileNode->GetNextFileNode() == NULL => it's the inf file itself,
        // which does not need to be processed.
        if (m_pCurFileNode->GetNextFileNode() != NULL)
        {
            pszPath = m_pCurFileNode->GetPath();
            Assert(pszPath != NULL);
            if (pszPath == NULL)
            {
                hr = E_UNEXPECTED;
                goto ExitDoParse;
            }
            CatPathStrN( szFileName, pszPath, m_pCurFileNode->GetName(), ARRAYSIZE(szFileName));
        }
        else
        {
            lstrcpyn(szFileName, m_szInf, ARRAYSIZE(szFileName));
            pszPath = NULL;
        }

        // hr might either be S_OK or S_FALSE
        // S_OK means file can be removed as it has a SharedDlls count of 1
        // S_FALSE if the count is greater than 1

        // calculate total num of files and their sizes
        if (SUCCEEDED(hr = GetSizeOfFile(szFileName, &dwFileSize)))
        {
            if (pszPath == NULL ||
                IsModuleRemovable(szFileName) ||
                lstrcmpi(szFileName, m_szFileName) == 0)
            {
                m_dwFileSizeSaved += dwFileSize;
            }

            m_dwTotalFileSize += dwFileSize;
        } else
            m_dwStatus = STATUS_CTRL_DAMAGED; // failure to get size indicative of missing file.

        m_nTotalFiles += 1;
    }

    // if we didn't detect a problem, flag the control as installed.
    if ( m_dwStatus == STATUS_CTRL_UNKNOWN )
        m_dwStatus = STATUS_CTRL_INSTALLED;

ExitDoParse:
    return hr;
}

HRESULT CParseInf::BuildDUFileList( HKEY hKeyDU )
{
    HRESULT hr = S_OK;
    LRESULT lResult;
    HKEY    hkeyFiles;
    TCHAR   szDUFileName[MAX_PATH + 1];
    DWORD   dwStrSize = MAX_PATH;
    int     cFilesEnum = 0;

    lResult = RegOpenKeyEx(hKeyDU, REGSTR_DU_CONTAINS_FILES, 0,
                           KEY_READ, &hkeyFiles);

    if ( lResult != ERROR_SUCCESS ) // if no files, maybe there's Java
        return hr;

    while ((lResult = RegEnumValue(hkeyFiles, cFilesEnum++, szDUFileName,
                                   &dwStrSize, NULL, NULL, NULL, NULL)) == ERROR_SUCCESS)
    {
        TCHAR szPath[MAX_PATH + 1];
        CFileNode *pFileNode;

        lstrcpyn(szPath, szDUFileName, MAX_PATH);
        TCHAR *szFName = ReverseStrchr(szPath, '\\');

        Assert(szFName != NULL);
        // long ago and far away, in the IE4, PP1-2 timeframe, there was a horrible
        // bug that corrupted these entries on Memphis and NT5. We suspect that GetLongPathName
        // was doing something wrong for code download, but repro scenarios were not
        // to be found. Anywho, the damaged registries are out there, so we need to
        // cope with them more gracefully than faulting at the *szFName = NULL;
        if ( szFName == NULL )
            continue;

        *szFName = NULL;
        szFName++;

        pFileNode = new CFileNode(szFName, "", szPath);
        if (pFileNode == NULL)
        {
            hr = E_OUTOFMEMORY;
            break; 
        }

        // create and add node to list
        if (m_pHeadFileList == NULL)
        {
            m_pHeadFileList = pFileNode;
            m_pCurFileNode = m_pHeadFileList;
        }
        else
        {
            hr = m_pCurFileNode->Insert(pFileNode);
            m_pCurFileNode = m_pCurFileNode->GetNextFileNode();
        }
        dwStrSize = MAX_PATH;
    }

    RegCloseKey( hkeyFiles );

    return hr;
}

HRESULT CParseInf::BuildDUPackageList( HKEY hKeyDU )
{
    HRESULT hr = S_OK;
    LRESULT lResult;
    HKEY    hkeyJava;
    ICreateJavaPackageMgr *picjpm;


    DestroyPackageList();

    lResult = RegOpenKeyEx(hKeyDU, REGSTR_DU_CONTAINS_JAVA, 0,
                           KEY_READ, &hkeyJava);

    if ( lResult != ERROR_SUCCESS ) // it's OK if there's no Java
        return hr;

    if ( !m_bCoInit )
        m_bCoInit = SUCCEEDED(hr = CoInitialize(NULL));

    if ( m_bCoInit )
    {
        hr=CoCreateInstance(CLSID_JavaPackageManager,NULL,CLSCTX_INPROC_SERVER,
            IID_ICreateJavaPackageMgr,(LPVOID *) &picjpm);
        if (SUCCEEDED(hr))
        {
            hr = picjpm->GetPackageManager(&m_pijpm);
            picjpm->Release();
        }
    }

    if (FAILED(hr))
        return S_OK; // hr; // BUGBUG: quietly fail until we're sure the JavaVM with package manager support is in the build.

    // list the packages under Contains/Java - these are in the gobal namespace
    hr = BuildNamespacePackageList(hkeyJava, "");

    // add packages for each namespace key under Contains\Java
    if ( SUCCEEDED(hr) )
    {
        DWORD   dwIndex;
        TCHAR   szNamespace[MAX_PATH + 1]; // 
        DWORD   dwStrSize;

        for ( dwIndex = 0, dwStrSize = MAX_PATH;
              RegEnumKey( hkeyJava, dwIndex, szNamespace, dwStrSize ) == ERROR_SUCCESS &&
                  SUCCEEDED(hr);
              dwIndex++, dwStrSize = MAX_PATH )
        {
            HKEY  hkeyNamespace;

            lResult = RegOpenKeyEx(hkeyJava, szNamespace, 0, KEY_READ, &hkeyNamespace);
            if ( lResult == ERROR_SUCCESS )
            {
                hr = BuildNamespacePackageList(hkeyNamespace, szNamespace );
                RegCloseKey( hkeyNamespace );
            }
            else
            {
                hr = HRESULT_FROM_WIN32(lResult);
                break;
            }
        } 
        
    }

    RegCloseKey( hkeyJava );

    m_bHasJava = m_pHeadPackageList != NULL;

    return hr;
}

HRESULT CParseInf::BuildNamespacePackageList( HKEY hKeyNS, LPCTSTR szNamespace )
{
    HRESULT hr = S_OK;
    LRESULT lResult;
    int     cPackagesEnum = 0;
    TCHAR   szDUPackageName[MAX_PATH + 1];
    DWORD   dwStrSize = MAX_PATH;
    BOOL    fIsSystemClass = FALSE;

    while ((lResult = RegEnumValue(hKeyNS, cPackagesEnum++, szDUPackageName,
                                   &dwStrSize, NULL, NULL, NULL, NULL)) == ERROR_SUCCESS)
    {
        IJavaPackage *pijp;

#ifndef UNICODE
        MAKE_WIDEPTR_FROMANSI(swzPackage, szDUPackageName );
        MAKE_WIDEPTR_FROMANSI(swzNamespace, szNamespace );
#else
        OLESTR swzPackage = szDUPackageName;
        OLESTR swzNamespace = szNamespace;
#endif
        hr = m_pijpm->GetPackage( swzPackage,
                                  ((*szNamespace == '\0')? NULL : swzNamespace),
                                  &pijp );
        if ( SUCCEEDED(hr) )
        {
            BSTR bstrPath;

            hr = pijp->GetFilePath( &bstrPath );
            if ( SUCCEEDED(hr) ) {
                CPackageNode *pPackageNode;

                pPackageNode = new CPackageNode(szDUPackageName, szNamespace);
                if (pPackageNode == NULL)
                {
                    hr = E_OUTOFMEMORY;
                    pijp->Release();
                    break; 
                }
#ifndef UNICODE
                MAKE_ANSIPTR_FROMWIDE(szPath, bstrPath );
#else
                TCHAR *szPath = bstrPath;
#endif
                pPackageNode->SetPath( szPath );

                pijp->IsSystemClass(&fIsSystemClass);
                pPackageNode->SetIsSystemClass(fIsSystemClass);

                if (m_pHeadPackageList == NULL)
                {
                    m_pHeadPackageList = pPackageNode;
                    m_pCurPackageNode = m_pHeadPackageList;
                }
                else
                {
                    hr = m_pCurPackageNode->Insert(pPackageNode);
                    m_pCurPackageNode = m_pCurPackageNode->GetNextPackageNode();
                }

                SysFreeString( bstrPath );
                pijp->Release(); // we're done with the package
            }
        }
        else
        {
            m_dwStatus = STATUS_CTRL_DAMAGED;
            hr = S_OK; // don't barf if this doesn't work, some villain might have uninstalled it
        }

        dwStrSize = MAX_PATH;
    }

    return hr;
}


HRESULT CParseInf::DoParseDU(LPCTSTR szOCXFileName, LPCTSTR szCLSID)
{
    HRESULT     hr = S_OK;
    TCHAR       szFileName[MAX_PATH];
    TCHAR       szDUSvrName[MAX_PATH];
    const TCHAR *pszSvrFile = NULL; 
    DWORD       dwFileSize = 0;
    HKEY        hKeyFiles = 0;
    HKEY        hKeyDU = 0;
    HKEY        hKeyDLInfo = 0;
    TCHAR       szDistUnit[MAX_REGPATH_LEN];
    HRESULT     lResult;
    CFileNode   *pFileNode = NULL;
    DWORD       dwExpire;
    DWORD       dw;

    Assert(szCLSID != NULL);

    // Since this function was called, we must be a distribution unit.
    // Set a member flag so that all other member functions realize that
    // we are really part of a DU now.

    m_bIsDistUnit = TRUE;

    // initialization

    if ( szOCXFileName != NULL )
        lstrcpyn(m_szFileName, szOCXFileName, ARRAYSIZE(m_szFileName));
    lstrcpyn(m_szCLSID, szCLSID, ARRAYSIZE(m_szCLSID));
    Init();

    // Add files from ...\Distribution Units\{Name}\Contains\Files
    CatPathStrN( szDistUnit, REGSTR_PATH_DIST_UNITS, szCLSID, ARRAYSIZE(szDistUnit));

    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szDistUnit, 0, KEY_READ,
                           &hKeyDU);
    if (FAILED(hr))
    {
        goto ExitDoParseDU;
    }                           

    hr = BuildDUFileList( hKeyDU );
    if (FAILED(hr))
    {
        goto ExitDoParseDU;
    } 

    hr = BuildDUPackageList( hKeyDU );
    if (FAILED(hr))
    {
        goto ExitDoParseDU;
    } 

    // Now add the OSD and INF files

    lResult = RegOpenKeyEx(hKeyDU, REGSTR_DOWNLOAD_INFORMATION, 0,
                           KEY_READ, &hKeyDLInfo);
    if (lResult == ERROR_SUCCESS)
    {
        TCHAR                *pFileName = NULL;
        TCHAR                 szBuffer[MAX_PATH + 1];

        dw = MAX_PATH;
        lResult = RegQueryValueEx(hKeyDLInfo, REGSTR_VALUE_INF, NULL, NULL,
                                  (unsigned char*)szBuffer, &dw);
        if (lResult == ERROR_SUCCESS)
        {
            pFileName = ReverseStrchr(szBuffer, '\\');
            if (pFileName != NULL)
            {
                pFileName++;

                // set INF member variable
                lstrcpyn(m_szInf, szBuffer, ARRAYSIZE(m_szInf));

                pFileNode = new CFileNode(szBuffer, "", NULL);
                if (pFileNode == NULL)
                {
                    hr = E_OUTOFMEMORY;
                    goto ExitDoParseDU; 
                }
        
                // create and add node to list
                if (m_pHeadFileList == NULL)
                {
                    m_pHeadFileList = pFileNode;
                    m_pCurFileNode = m_pHeadFileList;
                }
                else
                {
                    hr = m_pCurFileNode->Insert(pFileNode);
                    m_pCurFileNode = m_pCurFileNode->GetNextFileNode();
                }
            }
        }

        pFileName = NULL;
        dw = MAX_PATH;
        lResult = RegQueryValueEx(hKeyDLInfo, REGSTR_VALUE_OSD, NULL, NULL,
                                  (unsigned char*)szBuffer, &dw);
        if (lResult == ERROR_SUCCESS)
        {
            pFileName = ReverseStrchr(szBuffer, '\\');
            if (pFileName != NULL)
            {
                pFileName++;
                pFileNode = new CFileNode(szBuffer, "", NULL);
                // create and add node to list
                if (m_pHeadFileList == NULL)
                {
                    m_pHeadFileList = pFileNode;
                    m_pCurFileNode = m_pHeadFileList;
                }
                else
                {
                    hr = m_pCurFileNode->Insert(pFileNode);
                    m_pCurFileNode = m_pCurFileNode->GetNextFileNode();
                }
            }
        }
    }

    // See if there's an Expire value, and if so, override the default/general expire.
    dw = sizeof(DWORD);
    dwExpire = 0;
    if ( RegQueryValueEx(hKeyDU, REGSTR_VALUE_EXPIRE, NULL, NULL, (LPBYTE)&dwExpire, &dw) == ERROR_SUCCESS )
    {
        if ( dwExpire )
            m_cExpireDays = dwExpire;
        else
            GetDaysBeforeExpireAuto(&m_cExpireDays);
    }

    // Find out where COM thinks our CLSID is, and what the server name is.
    if ( FGetCLSIDFile( szDUSvrName, szCLSID ) )
    {
        m_bHasActiveX = TRUE;
        pszSvrFile = PathFindFileName(szDUSvrName);
    }
    else
        szDUSvrName[0] = '\0';


    for (m_pCurFileNode = m_pHeadFileList;
         m_pCurFileNode != NULL;
         m_pCurFileNode = m_pCurFileNode->GetNextFileNode(), hr = S_OK)
    {
        const TCHAR *pszPath = m_pCurFileNode->GetPath();

        if (pszPath != NULL)
        {
            CatPathStrN( szFileName, m_pCurFileNode->GetPath(), m_pCurFileNode->GetName(), ARRAYSIZE(szFileName));
        }
        else
        {
            lstrcpyn(szFileName, m_pCurFileNode->GetName(),ARRAYSIZE(szFileName));
        }

        if (SUCCEEDED(hr = GetSizeOfFile(szFileName, &dwFileSize)))
        {
            if (pszPath == NULL ||
                IsModuleRemovable(szFileName) ||
                lstrcmpi(szFileName, m_szFileName) == 0)
            {
                m_dwFileSizeSaved += dwFileSize;
            }

            // only play with the status if we haven't already flagged the installation
            // as damaged and we're looking at the the file that should be the host for
            // our control, if any.
            if ( m_dwStatus != STATUS_CTRL_DAMAGED && pszSvrFile != NULL &&
                 lstrcmpi( pszSvrFile, m_pCurFileNode->GetName() ) == 0 )
            {
                TCHAR    szDUSvrNameSPN[MAX_PATH];
                TCHAR    szFileNameSPN[MAX_PATH];

                GetShortPathName(szDUSvrName, szDUSvrNameSPN, MAX_PATH);
                GetShortPathName(szFileName, szFileNameSPN, MAX_PATH);
                
                if ( lstrcmpi( szDUSvrNameSPN, szFileNameSPN ) == 0 )
                    m_dwStatus = STATUS_CTRL_INSTALLED; // no, we're not unplugged
                else // server and our file are in different directories - unplugged scenario
                    m_dwStatus = STATUS_CTRL_UNPLUGGED;
            }

            m_dwTotalFileSize += dwFileSize;
        } else if ( !PathFileExists( szFileName ) ) // if a DU file is missing, then the installation is damaged.
            m_dwStatus = STATUS_CTRL_DAMAGED;

        m_nTotalFiles += 1;
    }

    // If we're still unsure, and there are packages, then this is a pure Java
    // DU and will say we're installed unless a check of the package files indicates otherwise.
    if ( m_pHeadPackageList != NULL && m_dwStatus == STATUS_CTRL_UNKNOWN )
        m_dwStatus = STATUS_CTRL_INSTALLED;

    // Accumulate package sizes and such into our running total
    for (m_pCurPackageNode = m_pHeadPackageList;
         m_pCurPackageNode != NULL;
         m_pCurPackageNode = m_pCurPackageNode->GetNextPackageNode(), hr = S_OK)
    {
        // the files can hold more than one of our packages, so only add a package
        // path file to the totals if we haven't already counted it.
        // N^2 to be sure, but the numbers will be small.
        CPackageNode *ppn;
        LPCTSTR szPackagePath = m_pCurPackageNode->GetPath();
        BOOL bAlreadySeen = FALSE;

        for ( ppn = m_pHeadPackageList;
              ppn != m_pCurPackageNode && !bAlreadySeen;
              ppn = ppn->GetNextPackageNode() )
            bAlreadySeen = lstrcmp( szPackagePath, ppn->GetPath() ) == 0;
        if ( bAlreadySeen )
            continue;

        // Must be a new file, 
       if ( SUCCEEDED(GetSizeOfFile(szPackagePath, &dwFileSize)) )
       {
           m_dwFileSizeSaved += dwFileSize;
           m_dwTotalFileSize += dwFileSize;
       }
       else
           m_dwStatus = STATUS_CTRL_DAMAGED;

       // m_nTotalFiles += 1; don't count these files, or the dependency file list will have a bunch of blank entries
    }

    // Some DUs, like SportsZone or Shockwave, have no Contains subkeys.
    // If status is still unknown here, but the server is in place, consider it
    // installed.
    if ( m_dwStatus == STATUS_CTRL_UNKNOWN && PathFileExists( szDUSvrName ) )
        m_dwStatus = STATUS_CTRL_INSTALLED;

ExitDoParseDU:

    if (hKeyDU)
    {
        RegCloseKey(hKeyDU);
    }

    if (hKeyDLInfo)
    {
        RegCloseKey(hKeyDLInfo);
    }

    return hr;
}

// ---------------------------------------------------------------------------
// CParseInf::IsSectionInINF
// Checks if a section is in the INF
// returns:
//      S_OK: lpCurCode has the satellite binary name
//      S_FALSE: ignore this code and use default resources in main dll
//      E_XXX: any other error
BOOL
CParseInf::IsSectionInINF(
    LPCSTR lpCurCode)
{
    const char *szDefault = "";
    DWORD len;
#define FAKE_BUF_SIZE   3
    char szBuf[FAKE_BUF_SIZE];

    len = GetPrivateProfileString(lpCurCode, NULL, szDefault,
                                                szBuf, FAKE_BUF_SIZE, m_szInf);

    if (len == (FAKE_BUF_SIZE - 2)) {   // returns Out Of Buffer Space?
        // yes, section found
        return TRUE;
    } else {
        return FALSE;
    }
}
    
// loop through the keys in [Add.Code} section and enumerate the
// files and their corresponding sections.
HRESULT CParseInf::HandleSatellites(LPCTSTR pszFileName)
{

    HRESULT hr = S_OK;

    // BEGIN NOTE: add vars and values in matching order
    // add a var by adding a new define VAR_NEW_VAR = NUM_VARS++
    const char *szVars[] = {

#define VAR_LANG     0       // expands to 3 letter lang code based on lcid
        "%LANG%",

#define NUM_VARS            1

        ""
    };

    const char *szValues[NUM_VARS + 1];
    szValues[VAR_LANG] = "***"; // unint magic
    szValues[NUM_VARS] = NULL;
    // END NOTE: add vars and values in matching order

    // look for and substitute variables like %EXTRACT_DIR%
    // and expand out the command line

    TCHAR szSectionName[MAX_PATH]; 
    TCHAR szSectionNameCopy[MAX_PATH]; 
    hr = ExpandCommandLine(pszFileName, szSectionName, MAX_PATH, szVars, szValues);

    if (hr != S_OK) 
        return hr;      // no vars to expand ignore section

    lstrcpy(szSectionNameCopy, szSectionName); // preserve


    // OK, this is a satellite DLL. Now we need to find the section(s) that
    // got installed.

    // we first enum the registry's Module Usage looking for DLLs that were
    // installed by (or used by) this CLSID. For each of those we need to
    // check if the base filename matches the pattern of the section, 
    // if it does then we process those sections

    DWORD iSubKey = 0;
    TCHAR szModName[MAX_PATH]; 

    while ( SUCCEEDED(hr = FindDLLInModuleUsage( szModName, m_szCLSID, iSubKey))  ) {

        if (PatternMatch(szModName, szSectionName) && 
            IsSectionInINF(szSectionName) ) {

            // create new node

            CFileNode *pFileNode = new CFileNode(szSectionName, szSectionName);
            if (pFileNode == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto Exit;
            }

            // don't insert file into list if it's path cannot be found
            if (FAILED(GetFilePath(pFileNode)))
            {
                delete pFileNode;
                continue;
            }

            // create and add node to list
            if (m_pHeadFileList == NULL)
            {
                m_pHeadFileList = pFileNode;
                m_pCurFileNode = m_pHeadFileList;
            }
            else if (SUCCEEDED(hr = m_pCurFileNode->Insert(pFileNode)))
            {
                m_pCurFileNode = m_pCurFileNode->GetNextFileNode();
            }
            else
            {
                goto Exit;
            }

            lstrcpy(szSectionName, szSectionNameCopy); // restore

        
        }
    }

    if ( hr == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS)) {
        hr = S_OK;
    }

Exit:

    return hr;

}

// loop through the keys in [Add.Code} section and enumerate the
// files and their corresponding sections.
HRESULT CParseInf::EnumSections()
{
    HRESULT hr = S_OK;
    TCHAR szSectionBuffer[MAX_INF_SECTION_SIZE];
    TCHAR szValueBuffer[MAX_PATH];
    TCHAR *pszFileName = NULL;
    CFileNode *pFileNode = NULL;
    DWORD dwLen = GetPrivateProfileString(
                        KEY_ADDCODE,
                        NULL,
                        DEFAULT_VALUE,
                        szSectionBuffer,
                        MAX_INF_SECTION_SIZE,
                        m_szInf);
    if (dwLen == 0)
    {
        // if inf file or [Add.Code] section 
        // does not exist, just delete the OCX

        Assert (m_pHeadFileList == NULL);

        // separate file name from its directory
        Assert( lstrlen(m_szFileName) < ARRAYSIZE(szValueBuffer) );
        lstrcpy(szValueBuffer, m_szFileName);
        TCHAR *szName = ReverseStrchr(szValueBuffer, '\\');
        Assert (szName != NULL); 
        if (szName == NULL)
        {
            hr = E_UNEXPECTED;
            goto ExitEnumSections;
        }

        // create a node of the OCX and put it in a linked list
        m_pHeadFileList = new CFileNode(szName + 1, DEFAULT_VALUE);
        if (m_pHeadFileList == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto ExitEnumSections;
        }
        m_pCurFileNode = m_pHeadFileList;

        *szName = '\0';
        if (FAILED(hr = m_pHeadFileList->SetPath(szValueBuffer)))
        {
            goto ExitEnumSections;
        }
        hr = S_FALSE;
        goto ExitEnumSections;
    }

    // For OCX's that have an INF file and [Add.Code] section, loop
    // through the section to get filenames and section names.  Store
    // each file and its section in a node and add the node to a
    // linked list

    for (pszFileName = szSectionBuffer; 
         pszFileName[0] != '\0';
         pszFileName += lstrlen(pszFileName) + 1)
    {
        dwLen = GetPrivateProfileString(
                            KEY_ADDCODE,
                            pszFileName,
                            DEFAULT_VALUE,
                            szValueBuffer,
                            MAX_PATH,
                            m_szInf);

        // skip the file if no section is specified for it
        if (dwLen == 0) {
            continue;
        }

        if (StrChr(pszFileName, '%')) {
            // if section not found and it contains a %
            // could be a variable like %LANG% that gets
            // substituted to install satellite DLLs

            // check if it has any vars that we know about
            // and expand them and add filenodes if reqd.

            if (HandleSatellites(pszFileName) == S_OK) {

                // if this expanded to a satellite dll name then
                // we would have already added that
                // as a node in HandleSatellites

                continue;

            }
            
        }


        // create new node
        pFileNode = new CFileNode(pszFileName, szValueBuffer);
        if (pFileNode == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto ExitEnumSections;
        }

        // don't insert file into list if it's path cannot be found
        if (FAILED(GetFilePath(pFileNode)))
        {
            delete pFileNode;
            continue;
        }

        // create and add node to list
        if (m_pHeadFileList == NULL)
        {
            m_pHeadFileList = pFileNode;
            m_pCurFileNode = m_pHeadFileList;
        }
        else if (SUCCEEDED(hr = m_pCurFileNode->Insert(pFileNode)))
        {
            m_pCurFileNode = m_pCurFileNode->GetNextFileNode();
        }
        else
        {
            goto ExitEnumSections;
        }
    }

    // include inf file into file list

    if (m_pHeadFileList && m_pCurFileNode)
    {
        hr = m_pCurFileNode->Insert(new CFileNode(m_szInf, DEFAULT_VALUE));
        if (SUCCEEDED(hr))
            m_pCurFileNode = m_pCurFileNode->GetNextFileNode();
    }

ExitEnumSections:

    return hr;
}

// Loop through all the sections in [Setup Hooks].  For each
// section, call ParseUninstallSection to find its UNINSTALL section
// and execute it.
HRESULT CParseInf::ParseSetupHook()
{
    HRESULT hr = S_FALSE; // Return S_FALSE if we don't run into any errors, but also don't do any work.
    TCHAR szSectionBuffer[MAX_INF_SECTION_SIZE];
    TCHAR szSection[MAX_PATH];
    TCHAR *pszKey = NULL;

    DWORD dwLen = GetPrivateProfileString(
                        KEY_SETUPHOOK,
                        NULL,
                        DEFAULT_VALUE,
                        szSectionBuffer,
                        MAX_INF_SECTION_SIZE,
                        m_szInf);

    // no Setup Hook section found
    if (dwLen == 0)
        goto EXITPARSESETUPHOOK;

    for (pszKey = szSectionBuffer; 
         pszKey[0] != '\0';
         pszKey += lstrlen(pszKey) + 1)
    {
        // For each key, get the section and run the section with RunSetupCommand

        dwLen = GetPrivateProfileString(
                       KEY_SETUPHOOK,
                       pszKey,
                       DEFAULT_VALUE,
                       szSection,
                       MAX_PATH,
                       m_szInf);

        if (dwLen == 0)
            continue;

        hr = ParseUninstallSection(szSection);
        if (FAILED(hr))
            goto EXITPARSESETUPHOOK;
     }

EXITPARSESETUPHOOK:
    return hr;
}

// Go to each file's section, find its conditional hook section, then
// call ParseUninstallSection to execute the conditional hook section.
HRESULT CParseInf::ParseConditionalHook()
{
    HRESULT hr = S_FALSE; // Return S_FALSE if we don't run into any errors, but also don't do any work.
    TCHAR szHookSection[MAX_PATH];
    const TCHAR *pszSection = NULL;
    CFileNode *pNode = NULL;

    if (m_pHeadFileList == NULL)
    {
        hr = S_FALSE;
        goto EXITPARSECONDITIONALHOOK;
    }

    pNode = m_pHeadFileList;
    for (pNode = m_pHeadFileList; pNode != NULL; pNode = pNode->GetNextFileNode())
    {
        pszSection = pNode->GetSection();
        if (pszSection == NULL)
            continue;

        if (GetPrivateProfileString(
                            pszSection,
                            KEY_HOOK,
                            DEFAULT_VALUE,
                            szHookSection,
                            MAX_PATH,
                            m_szInf) == 0)
            continue;

        hr = ParseUninstallSection(szHookSection);
        if (FAILED(hr))
            goto EXITPARSECONDITIONALHOOK;
    }

EXITPARSECONDITIONALHOOK:
    return hr;
}

// Given a file section, find its UNINSTALL section, go to the
// section and executes the commands there
HRESULT CParseInf::ParseUninstallSection(LPCTSTR lpszSection)
{
    HRESULT hr = S_OK;
    TCHAR szUninstallSection[MAX_PATH];
    TCHAR szBuf[MAX_PATH];
    TCHAR szInfSection[MAX_PATH];
    TCHAR szCacheDir[MAX_PATH];
    HANDLE hExe = INVALID_HANDLE_VALUE;
    HINSTANCE hInst = NULL;

    // check for "UNINSTALL" key
    DWORD dwLen = GetPrivateProfileString(
                        lpszSection,
                        KEY_UNINSTALL,
                        DEFAULT_VALUE,
                        szUninstallSection,
                        ARRAYSIZE(szUninstallSection),
                        m_szInf);

    // UNINSTALL key not found, quit.
    if (dwLen == 0)
    {
        return S_FALSE;
    }

    // There are 4 possible combinations inside the uninstall section
    // 1) Both inffile and infsection are specified -> simply to go those
    // 2) Only inffile is given -> go to inffile and do DefaultInstall
    // 3) Only infsection is given -> do infsection in this inf file
    // 4) Nothing is specified -> simply do this section

    GetDirectory(GD_EXTRACTDIR, szCacheDir, ARRAYSIZE(szCacheDir), m_szFileName);

    lstrcpyn(szBuf, szCacheDir, MAX_PATH - 1);
    lstrcat(szBuf, TEXT("\\"));

    int cch = lstrlen(szBuf);

    dwLen = GetPrivateProfileString(
                        szUninstallSection,
                        KEY_INFFILE,
                        DEFAULT_VALUE,
                        szBuf + cch,
                        MAX_PATH - cch,
                        m_szInf);

    if (dwLen == 0)
    {
        szBuf[0] = '\0';
    }

    // get inf section
    dwLen = GetPrivateProfileString(
                        szUninstallSection,
                        KEY_INFSECTION,
                        DEFAULT_VALUE,
                        szInfSection,
                        ARRAYSIZE(szInfSection),
                        m_szInf);

    if (dwLen == 0)
    {
        if (szBuf[0] != '\0')
            lstrcpyn(szInfSection, KEY_DEFAULTUNINSTALL,ARRAYSIZE(szInfSection));
        else
        {
            lstrcpyn(szBuf, m_szInf,ARRAYSIZE(szBuf));
            lstrcpyn(szInfSection, szUninstallSection,ARRAYSIZE(szInfSection));
        }
    }

    // load advpack.dll and call RunSetupCommand() to process
    // any special uninstall commands

    hr = STG_E_FILENOTFOUND;

    HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));
    if (hinstAdvPack)
    {
        RUNSETUPCOMMAND pfnRunSetup = (RUNSETUPCOMMAND)GetProcAddress(
            hinstAdvPack, achRUNSETUPCOMMANDFUNCTION);
        if (pfnRunSetup)
        {
            hr = pfnRunSetup(NULL, szBuf, szInfSection, 
                            szCacheDir, NULL, &hExe, 1, NULL);
        }
    }

    return hr;
}

// For each file specified in the INF file, find its
// path in this order
// 1) OCX path
// 2) System dir
// 3) Windows dir
// 4) PATH directories
HRESULT CParseInf::GetFilePath(CFileNode *pFileNode)
{
    Assert (pFileNode != NULL);
    HRESULT hr = S_OK;
    TCHAR szValueBuf[MAX_PATH];
    TCHAR *pszPathPtr = NULL;
    TCHAR *pszPathEnv = NULL;
    TCHAR *pchPathEnd = NULL;
    DWORD dwLenPATH = 0;

    // ocx directory
    hr = GetDirectory(GD_EXTRACTDIR, szValueBuf, ARRAYSIZE(szValueBuf), m_szFileName);
    CatPathStrN( szValueBuf, szValueBuf, pFileNode->GetName(), ARRAYSIZE(szValueBuf));

    // if file being searched for now is the OCX itself, just leave
    if (lstrcmpi(szValueBuf, m_szFileName) == 0)
    {
        goto EXITGETFILEPATH;
    }

    if (SUCCEEDED(hr) && 
        SUCCEEDED(LookUpModuleUsage(szValueBuf, m_szCLSID)))
    {
        goto EXITGETFILEPATH;
    }

    // system directory
    hr = GetDirectory(GD_SYSTEMDIR, szValueBuf, ARRAYSIZE(szValueBuf));
    if (SUCCEEDED(hr) && CatPathStrN( szValueBuf, szValueBuf, pFileNode->GetName(), ARRAYSIZE(szValueBuf)) &&
        SUCCEEDED(LookUpModuleUsage(szValueBuf, m_szCLSID)))
    {
        goto EXITGETFILEPATH;
    }

    // windows directory
    hr = GetDirectory(GD_WINDOWSDIR, szValueBuf, ARRAYSIZE(szValueBuf));
    if (SUCCEEDED(hr) && CatPathStrN( szValueBuf, szValueBuf, pFileNode->GetName(), ARRAYSIZE(szValueBuf)) &&
        SUCCEEDED(LookUpModuleUsage(szValueBuf, m_szCLSID)))
    {
        goto EXITGETFILEPATH;
    }

    // get PATH envirnment variable
    dwLenPATH = GetEnvironmentVariable(ENV_PATH, szValueBuf, 0);
    if (dwLenPATH == 0)
    {
        hr = E_FAIL;
        goto EXITGETFILEPATH;
    }

    pszPathEnv = new TCHAR[dwLenPATH];
    if (pszPathEnv == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto EXITGETFILEPATH;
    }
    GetEnvironmentVariable(ENV_PATH, pszPathEnv, dwLenPATH);
    pchPathEnd = pszPathPtr = pszPathEnv;

    // walk all directories in PATH and see if file is found
    // in any of them
    while (pchPathEnd != NULL)
    {
        pchPathEnd = StrChr(pszPathPtr, ';');
        if (pchPathEnd != NULL)
            *pchPathEnd = '\0';

        CatPathStrN( szValueBuf, pszPathPtr, pFileNode->GetName(), ARRAYSIZE(szValueBuf));

        if (SUCCEEDED(LookUpModuleUsage(szValueBuf, m_szCLSID)))
            goto EXITGETFILEPATH;

        if (pchPathEnd != NULL)
            *(pchPathEnd++) = ';';

        pszPathPtr = pchPathEnd;
    }

    // file not found anywhere
    hr = E_FAIL;

EXITGETFILEPATH:

    if (pszPathEnv != NULL)
        delete pszPathEnv;

    if (SUCCEEDED(hr))
    {
        *(ReverseStrchr(szValueBuf, '\\')) = '\0';
        hr = pFileNode->SetPath(szValueBuf);
    }

    return hr;
}

HRESULT CParseInf::CheckFilesRemovability(void)
{
    HRESULT hr = S_OK;
    TCHAR szFullName[MAX_PATH];
    const TCHAR *pszPath = NULL;
    BOOL bFileExist;

    // Walk through every file and see if it is deletable. If so,
    // then check if for sharing violations on that file.
    for (m_pCurFileNode = m_pHeadFileList;
         m_pCurFileNode != NULL && SUCCEEDED(hr);
         m_pCurFileNode = m_pCurFileNode->GetNextFileNode())
    {
        pszPath = m_pCurFileNode->GetPath();
        if (pszPath == NULL || pszPath[0] == '\0')
            continue;

        CatPathStrN( szFullName, pszPath, m_pCurFileNode->GetName(), ARRAYSIZE(szFullName) );

        if (IsModuleRemovable(szFullName))
        {
            HANDLE h = CreateFile(
                             szFullName,
                             GENERIC_READ|GENERIC_WRITE,
                             0,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL|FILE_FLAG_NO_BUFFERING,
                             NULL);
            if (h == INVALID_HANDLE_VALUE)
            {
                bFileExist = (GetLastError() != ERROR_FILE_NOT_FOUND);
                if (bFileExist)
                {
                    hr = STG_E_SHAREVIOLATION;
                    break;
                }
            }
            else
            {
                CloseHandle(h);
                m_pCurFileNode->SetRemovable( TRUE );
            }
        }
    }

    return hr;
}

HRESULT CParseInf::CheckLegacyRemovability(LONG *cOldSharedCount )
{
    HRESULT hr = S_OK;
    BOOL    bFileExist;

    HANDLE h = CreateFile(
                     m_szFileName,
                     GENERIC_READ|GENERIC_WRITE,
                     0,
                     NULL,
                     OPEN_EXISTING,
                     FILE_ATTRIBUTE_NORMAL|FILE_FLAG_NO_BUFFERING,
                     NULL);
    if (h == INVALID_HANDLE_VALUE)
    {
        bFileExist = (GetLastError() != ERROR_FILE_NOT_FOUND);
        if (bFileExist)
        {
            hr = STG_E_SHAREVIOLATION;
        } else
            hr = S_FALSE;
    }
    else
    {
        CloseHandle(h);
    }

    if ( SUCCEEDED(hr) )
        hr = CheckFilesRemovability();

    return hr;
}

HRESULT CParseInf::CheckDURemovability(HKEY hkeyDUDB, BOOL bSilent)
{
    HRESULT hr = S_OK;
    BOOL    bAskSystemClass = TRUE;

    hr = CheckFilesRemovability();
    if (FAILED(hr)) {
        goto CheckDURemovabilityExit;
    }

    hr = CheckDUDependencies(hkeyDUDB, bSilent);
    if (FAILED(hr)) {
        goto CheckDURemovabilityExit;
    }
    
    // Check for package removability.
    // We shouldn't remove a package if another DU also uses it.
    // TODO: Some sort of package-currently-in-use test. Either test the path file, as above,
    //       or use some groovy new IJavaPackage(Manager) method.
    for (m_pCurPackageNode = m_pHeadPackageList;
         m_pCurPackageNode != NULL;
         m_pCurPackageNode = m_pCurPackageNode->GetNextPackageNode())
    {
        TCHAR   szT[MAX_PATH];
        LRESULT lResult;
        BOOL    bFoundInOtherDU = FALSE;
        int     cDistUnitEnum = 0;

        if (!bAskSystemClass && m_pCurPackageNode->GetIsSystemClass()) {
            char lpszBuf[MAX_MSGBOX_STRING_LEN];
            char lpszBufTitle[MAX_MSGBOX_TITLE_LEN];

            MLLoadString(IDS_OCCACHE_WARNING_JAVA_SYSTEM_CLASS,
                         lpszBuf, MAX_MSGBOX_STRING_LEN);
            MLLoadString(IDS_REMOVAL_WARNING,
                         lpszBufTitle, MAX_MSGBOX_TITLE_LEN);

            // Attempting to remove system class. Warn user.
            if ( bSilent || 
                MessageBox(NULL, lpszBuf, lpszBufTitle,
                           MB_YESNO | MB_ICONWARNING) != IDYES) {

                hr = E_FAIL;
                goto CheckDURemovabilityExit;
            }
            bAskSystemClass = FALSE;
        }

        // Enumerate distribution units
        while ( (lResult = RegEnumKey(hkeyDUDB, cDistUnitEnum++, szT, MAX_PATH)) == ERROR_SUCCESS &&
                !bFoundInOtherDU )
        {
            if ( lstrcmp(szT, m_szCLSID) != 0 ) // skip the current DU
            {
                HKEY    hkeyDUCJ;
                DWORD   dw = MAX_PATH;
                lstrcat(szT, REGSTR_DU_CONTAINS_JAVA );
                lResult = RegOpenKeyEx( hkeyDUDB, szT, 0, KEY_READ, &hkeyDUCJ );
                if ( lResult == ERROR_SUCCESS )
                {
                    lResult = RegQueryValueEx(hkeyDUCJ, REGSTR_VALUE_INF, NULL, NULL,
                                              (unsigned char*)szT, &dw);
                    // To be safe, assume that anything other than value not found means
                    // that the other DU also uses the package.
                    bFoundInOtherDU = lResult != ERROR_FILE_NOT_FOUND;
                    RegCloseKey( hkeyDUCJ );
                } // if we could open other key's Contains\Java subkey
            } // if it's a different DU
        } // while enumerating DUs
        
        // if we found it in another DU, then we shouldn't remove this package with this DU
        m_pCurPackageNode->SetRemovable( !bFoundInOtherDU );
    } // for each package

CheckDURemovabilityExit:

    return hr;
}


HRESULT CParseInf::RemoveLegacyControl( LPCTSTR lpszTypeLibID, BOOL bSilent )
{
    HRESULT     hr = S_FALSE; 
    const TCHAR *pszPath;
    BOOL        bUnplug = m_dwStatus != STATUS_CTRL_UNPLUGGED;
    BOOL        bFileMissing = !PathFileExists( m_szFileName );
    BOOL        bDidRemove = FALSE;
    TCHAR       szFullName[MAX_PATH];

    // loop through the list of assocated files, remove them as
    // well as their registry entries.
    for (m_pCurFileNode = m_pHeadFileList;
         m_pCurFileNode != NULL;
         m_pCurFileNode = m_pCurFileNode->GetNextFileNode())
    {
        int cOwners;

        pszPath = m_pCurFileNode->GetPath();

        // Process INF file, which as no path since it's not described in INF
        if (pszPath == NULL || pszPath[0] == '\0')
        {
            if ( DeleteFile(m_pCurFileNode->GetName()) )
                hr = S_OK; // hey, we did _something_ - averts the "not enough info" message
            continue;
        }
 
        // If we're where, we had some other file besides the INF. 
        // Even if we don't remove it, we still knock down its module
        // usage, which has gotta count for having done something.
        hr = S_OK;

        CatPathStrN( szFullName, pszPath, m_pCurFileNode->GetName(), MAX_PATH);

        cOwners = SubtractModuleOwner( szFullName, m_szCLSID );
        if (m_pCurFileNode->GetRemovable() && cOwners == 0)
        {
            if ( bUnplug )
                UnregisterOCX(szFullName);
            DeleteFile(szFullName);
            bDidRemove = bDidRemove || StrCmpI(szFullName,m_szFileName) == 0;
        }
    }

    if (hr == S_OK && bDidRemove && lpszTypeLibID != NULL)
        CleanInterfaceEntries(lpszTypeLibID);
    
    if ( bUnplug && bFileMissing )
    {
        if ( m_szFileName[0] != '\0' ) // only do this if there is an ocx to clean up after
            CleanOrphanedRegistry(m_szFileName, m_szCLSID, lpszTypeLibID);
    }

    return hr;
}


HRESULT CParseInf::RemoveDU( LPTSTR szFullName, LPCTSTR lpszTypeLibID, HKEY hkeyDUDB, BOOL bSilent )
{
    HRESULT     hr = S_FALSE;   // only say S_OK if we actually do something beyond yanking the INF
    const TCHAR *pszPath = NULL;

    hr = RemoveLegacyControl( lpszTypeLibID, bSilent );
    if (SUCCEEDED(hr))
    {

        // Remove the packages that we have determined are safe to remove.
        for (m_pCurPackageNode = m_pHeadPackageList;
            m_pCurPackageNode != NULL;
            m_pCurPackageNode = m_pCurPackageNode->GetNextPackageNode())
        {
            if ( m_pCurPackageNode->GetRemovable() )
            {
                Assert(m_pijpm != NULL);
    #ifdef UNICODE
                OLECHAR *swzPkg = m_pCurPackageNode->GetName();
                OLECHAR *swzNamespace = m_pCurPackageNode->GetNamespace();
    #else
                MAKE_WIDEPTR_FROMANSI(swzPkg, m_pCurPackageNode->GetName());
                MAKE_WIDEPTR_FROMANSI(swzNamespace, m_pCurPackageNode->GetNamespace());
    #endif
                hr = m_pijpm->UninstallPackage( swzPkg, 
                                                ((*swzNamespace == 0)? NULL : swzNamespace),
                                                0 );
            }
        }
    }

    DeleteKeyAndSubKeys(hkeyDUDB, m_szCLSID);

    return hr;
}

HRESULT CParseInf::CheckDUDependencies(HKEY hKeyDUDB, BOOL bSilent )
{
    long                    lrDist = 0;
    long                    lResult = 0;
    long                    lr = 0;
    int                     iSubKey = 0;
    HKEY                    hkeyCurrent = 0;
    HKEY                    hkeyCurDU = 0;
    char                    szName[MAX_REGPATH_LEN];
    int                     iValue = 0;
    unsigned long           ulSize;
    char                    szDependency[MAX_REGPATH_LEN];
    HKEY                    hkeyCOM = 0;
    DWORD                   dwType = 0;
    char                    szDepName[MAX_CONTROL_NAME_LEN];
    char                    szDepWarnBuf[MAX_MSGBOX_STRING_LEN];
    char                    szCOMControl[MAX_REGPATH_LEN];
    DWORD                   dwSize = 0;
    HRESULT                 hr = S_OK;

    // Iterate through DUs that have a ...\contains\Distribution Units
    // key in the registry and compare the entries inside with the DU
    // being removed.

    while ((lResult = RegEnumKey(hKeyDUDB, iSubKey++, szName,
                                 MAX_REGPATH_LEN)) == ERROR_SUCCESS)
    {

        if (!lstrcmpi(szName, m_szCLSID))
        {
            // Skip ourselves
            continue;
        }

        if (RegOpenKeyEx(hKeyDUDB, szName, 0, KEY_READ, &hkeyCurrent) ==
                         ERROR_SUCCESS)
        {
            lr = RegOpenKeyEx(hkeyCurrent, REGSTR_DU_CONTAINS_DIST_UNITS,
                              0, KEY_READ, &hkeyCurDU);
            if (lr != ERROR_SUCCESS)
            {
                RegCloseKey(hkeyCurrent);
                continue;
            }

            ulSize = MAX_REGPATH_LEN;
            while ((lResult = RegEnumValue(hkeyCurDU, iValue++, szDependency,
                                           &ulSize, NULL, NULL, NULL, NULL)) == ERROR_SUCCESS)
            {
                if (!lstrcmpi(szDependency, m_szCLSID))
                {
                    // dependency found

                    // Try to get a friendly name for the dependency control

                    dwSize = MAX_CONTROL_NAME_LEN;
                    lResult = RegQueryValueEx(hkeyCurrent, NULL, NULL,
                                              &dwType, (unsigned char *)szDepName,
                                              &dwSize);

                    if (lResult != ERROR_SUCCESS || szDepName[0] == '\0') {
                        // Couldn't get a friendly name. Try the COM branch.

                        // BUGBUG: Technically, this could overflow because
                        // szName and szCOMControl are the same size, but
                        // this is already at our defined maximum size for reg
                        // entries.

                        wsprintf(szCOMControl, "%s\\%s", REGSTR_COM_BRANCH, szName);
                        
                        lResult = RegOpenKeyEx(HKEY_CLASSES_ROOT, szCOMControl,
                                               0, KEY_READ, &hkeyCOM);

                        if (lResult != ERROR_SUCCESS) {                                               
                            MLLoadString(IDS_OCCACHE_WARNING_DEP_REMOVAL_NAME_UNKNOWN,
                                         szDepWarnBuf, MAX_MSGBOX_STRING_LEN);
                        }
                        else {
                            dwSize = MAX_CONTROL_NAME_LEN;
                            lResult = RegQueryValueEx(hkeyCOM, NULL, NULL,
                                                      &dwType, (unsigned char *)szDepName,
                                                      &dwSize);

                            if (lResult != ERROR_SUCCESS || szDepName[0] == '\0') {
                                MLLoadString(IDS_OCCACHE_WARNING_DEP_REMOVAL_NAME_UNKNOWN,
                                             szDepWarnBuf, MAX_MSGBOX_STRING_LEN);
                            }
                            else {
                                char                 lpszBuf[MAX_MSGBOX_STRING_LEN];

                                MLLoadString(IDS_OCCACHE_WARNING_DEPENDENCY_REMOVAL,
                                             lpszBuf, MAX_MSGBOX_STRING_LEN);
                                wsprintf(szDepWarnBuf, lpszBuf, szDepName);
                            }

                            if (hkeyCOM) {
                                RegCloseKey(hkeyCOM);
                            }
                        }
                    }
                    else {
                        char                 lpszBuf[MAX_MSGBOX_STRING_LEN];

                        MLLoadString(IDS_OCCACHE_WARNING_DEPENDENCY_REMOVAL,
                                     lpszBuf, MAX_MSGBOX_STRING_LEN);

                        wsprintf(szDepWarnBuf, lpszBuf, szDepName);
                    }

                    
                    // TODO: Consider using better HWND than desktop
                    char lpszBufTitle[MAX_MSGBOX_TITLE_LEN];
    
                    MLLoadString(IDS_REMOVAL_WARNING,
                                 lpszBufTitle, MAX_MSGBOX_TITLE_LEN);
    
                    if (bSilent ||
                        MessageBox(NULL, szDepWarnBuf, lpszBufTitle,
                                   MB_YESNO | MB_ICONWARNING) != IDYES)
                    {
                        hr = E_FAIL;
                        RegCloseKey(hkeyCurDU);
                        RegCloseKey(hkeyCurrent);
                        RegCloseKey(hKeyDUDB);
                        goto ReturnCheckDUDependencies;
                    }
                }
                ulSize = MAX_REGPATH_LEN;
            }
            RegCloseKey(hkeyCurDU);
            RegCloseKey(hkeyCurrent);
        }
    }


ReturnCheckDUDependencies:
    return hr;
}

// uninstall OCX and its associated files
HRESULT CParseInf::RemoveFiles(
                       LPCTSTR lpszTypeLibID /* = NULL */,
                       BOOL bForceRemove, /* = FALSE */
                       DWORD dwIsDistUnit,
                       BOOL bSilent)
{
    HRESULT hr = S_OK;
    HRESULT hrInf1;
    HRESULT hrInf2;
    TCHAR szFullName[MAX_PATH];
    const TCHAR *pszPath = NULL;
    BOOL bRemovable = (dwIsDistUnit) ? (TRUE) : (IsModuleRemovable(m_szFileName));
    BOOL bIsOCX = FALSE;
    LONG cRefOld = 0;
    HKEY  hKeyDUDB = 0;
    BOOL bUnplug = m_dwStatus == STATUS_CTRL_DAMAGED || m_dwStatus == STATUS_CTRL_INSTALLED;

    if ( !g_fAllAccess || (!bForceRemove && !bRemovable))
    {
        hr = E_ACCESSDENIED;
        goto ExitRemoveFiles;
    }

    // Check sharing violation (if it is a legacy control)
    
    if (!dwIsDistUnit)
    {
        hr = CheckLegacyRemovability( &cRefOld );
        // set SharedDlls count to 1 and save up the old
        // count in case the removal fails
        if (hr == S_OK && !bRemovable && 
            FAILED(hr = SetSharedDllsCount(m_szFileName, 1, &cRefOld)))
        {
            hr = (!PathFileExists( m_szFileName ) ? S_OK : hr);
            goto ExitRemoveFiles;
        }

        if ( FAILED(hr) )
            goto ExitRemoveFiles;
    }
    else
    {
        long lResultDist;

        lResultDist = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_DIST_UNITS, 0,
                                   KEY_READ, &hKeyDUDB);
        if (lResultDist == ERROR_SUCCESS)
            hr = CheckDURemovability( hKeyDUDB, bSilent );
        else 
            hr = E_FAIL;

        if ( FAILED(hr) )
            goto ReturnRemoveFiles;
    }


    // ** keyword UNINSTALL -- new feature that hasn't been implemented yet **

    // parse [Setup Hook], look for "UNINSTALL" key  
    if (FAILED(hrInf1 = ParseSetupHook()))
    {
        goto ExitRemoveFiles;
    }

    // parse conditional hooks in each of the file sections
    if (FAILED(hrInf2 = ParseConditionalHook()))
    {
        goto ExitRemoveFiles;
    }

    // Okay, if the both didn't do anything, we'll try the DefaultUninstall
    if ( hrInf2 == S_FALSE && hrInf2 == S_FALSE && PathFileExists( m_szInf ) )
    {
        // see if there's anybody home in the default uninstall section
        DWORD dwSize = GetPrivateProfileString( KEY_DEFAULTUNINSTALL,
                                                NULL,
                                                DEFAULT_VALUE,
                                                szFullName,
                                                MAX_PATH,
                                                m_szInf );

        if ( dwSize > 0 )
        {
            HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));
            HANDLE hExe = INVALID_HANDLE_VALUE;

            GetDirectory(GD_EXTRACTDIR, szFullName, ARRAYSIZE(szFullName), m_szInf);

            if (hinstAdvPack)
            {
                RUNSETUPCOMMAND pfnRunSetup = (RUNSETUPCOMMAND)GetProcAddress(
                    hinstAdvPack, achRUNSETUPCOMMANDFUNCTION);
                if (pfnRunSetup)
                {
                    // reset hrINf2 to reflect the success of running the default
                    // uninstall section. This will prevent us from pointing to the
                    // Add/Remove control panel in some cases, like Shockwave.
                    hrInf2 = pfnRunSetup(NULL, m_szInf, KEY_DEFAULTUNINSTALL, 
                                    szFullName, NULL, &hExe, RSC_FLAG_INF, NULL);
                }

                FreeLibrary( hinstAdvPack );
            }
        }
    }

    if ( !dwIsDistUnit )
        hr = RemoveLegacyControl( lpszTypeLibID, bSilent );
    else
        hr = RemoveDU( szFullName, lpszTypeLibID, hKeyDUDB, bSilent );
    if ( FAILED(hr) )
        goto ExitRemoveFiles;

    // Return S_FALSE iff none of our uninstall efforts succeeded
    if ( hr == S_FALSE && (hrInf1 == S_OK || hrInf2 == S_OK) )
        hr = S_OK;
        
    // remove conflict directory
    if (SUCCEEDED(GetDirectory(GD_CONFLICTDIR, szFullName, ARRAYSIZE(szFullName))) &&
        LStrNICmp(m_szFileName, szFullName, lstrlen(szFullName)) == 0)
    {
        TCHAR *pCh = ReverseStrchr(m_szFileName, '\\');
        Assert (pCh != NULL);
        TCHAR chTemp = *pCh;
        *pCh = '\0';
        RemoveDirectory(m_szFileName);
        *pCh = chTemp;
    }

    DestroyFileList();

ExitRemoveFiles:

    // set shared dlls count back to where it was if OCX cannot be removed
    if (cRefOld > 0 && FileExist(m_szFileName))
    {
        if (SUCCEEDED(hr))
            hr = SetSharedDllsCount(m_szFileName, cRefOld);
        else
            SetSharedDllsCount(m_szFileName, cRefOld);
    }

    if ( hKeyDUDB )
        RegCloseKey( hKeyDUDB );

ReturnRemoveFiles:

    return hr;
}


void CParseInf::SetIsDistUnit(BOOL bDist)
{
    m_bIsDistUnit = bDist;
}

BOOL CParseInf::GetIsDistUnit() const
{
    return m_bIsDistUnit;
}

// return total size of OCX and its associated files
DWORD CParseInf::GetTotalFileSize() const
{
    return m_dwTotalFileSize;
}

DWORD CParseInf::GetTotalSizeSaved() const
{
    return m_dwFileSizeSaved;
}

DWORD CParseInf::GetStatus() const
{
    return m_dwStatus;
}

// return total number of files which will be removed
// together with the OCX
int CParseInf::GetTotalFiles() const
{
    return m_nTotalFiles;
}

// return first file in the list of associated files
CFileNode* CParseInf::GetFirstFile()
{
    m_pFileRetrievalPtr = m_pHeadFileList;
    return m_pFileRetrievalPtr;
}

// get the next file in the list of associated files
CFileNode* CParseInf::GetNextFile()
{
    m_pFileRetrievalPtr = m_pFileRetrievalPtr->GetNextFileNode();
    return m_pFileRetrievalPtr;
}

// return first file in the list of associated files
CPackageNode* CParseInf::GetFirstPackage()
{
    m_pPackageRetrievalPtr = m_pHeadPackageList;
    return m_pPackageRetrievalPtr;
}

// get the next file in the list of associated files
CPackageNode* CParseInf::GetNextPackage()
{
    m_pPackageRetrievalPtr = (m_pPackageRetrievalPtr != NULL)?
                                m_pPackageRetrievalPtr->GetNextPackageNode() :
                                NULL;
    return m_pPackageRetrievalPtr;
}

