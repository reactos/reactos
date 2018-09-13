#include "viruspch.h"
#include "vrsscan.h"
#include "util.h"
#include "viruschk.h"
#include "resource.h"
#include "virusmn.h"
#include "richedit.h"


// List of virus scanner providers
#define SCANNER_KEY         "Software\\Microsoft\\Virus Check\\Scanners"
#define SCANNER_VIRUSCHECK  "Software\\Microsoft\\Virus Check"
#define SCANNER_COOKIE      "Cookie"
#define SCANNER_STARTDATE   "StartDate"

// Keys for accessing virus scanner engine
#define SCANNER_CLSID_REG   "CLSID\\%s\\InprocServer32"
#define VENDOR_REG          "CLSID\\%s\\VirusScanner"

// Keys stored under VENDOR_REG
#define VENDOR_DESC         "VendorDescription"
#define VENDOR_CONTACT      "VendorContactInfo"
#define VENDOR_FLAGS        "VendorFlags"
#define VENDOR_ICON         "VendorIcon"

#define STR_UNKNOWN     L"<Not available>"
#define STR_NONE        L"None"

extern TCHAR g_szTitle[];

//REVIEW: We support Apartment model only so only 1 thread at a time can enter us.
static CVirusScanProvider *g_pvp;               // Used for DLGPROC callback procedure

CVirusCheck::CVirusCheck(IUnknown *punkOuter, IUnknown **punkRet)
{
   // Initilize members vars
   m_uNumProviders = 0;
   m_provList = NULL;
   m_cObjRef = 0;
   g_pvp = NULL;

   *punkRet = NULL;

   if(punkOuter != NULL)
       *punkRet = NULL;          // Do not support aggregation
   else
       *punkRet = (IVirusScanner *)this;

   AddRef();
   LoadProviders();
}

void CVirusCheck::LoadProviders()
{
   DWORD    dwSubKeys;
   DWORD    dwSize;
   LPWSTR   pwsz;
   char     szCLSID[GUID_STR_LEN];
   char     szBuf[MAX_STRING];
   DWORD    dwFlags;
   CLSID    clsid;
   IVirusScanEngine *pvs;
   HKEY     hVendorKey = NULL;
   HKEY     hKey= NULL;
   HKEY     hSubKey = NULL;
   ULONG    ulCRC = 0;
   DWORD    dwCookie;

   CVirusScanProvider *curProv = NULL;

   // Open the branch that contains providers
   if(RegOpenKeyExA(HKEY_LOCAL_MACHINE, SCANNER_KEY, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
      return;

   // Find out how many providers we have
   if(RegQueryInfoKey(hKey, NULL, NULL, NULL, &dwSubKeys, NULL, NULL, NULL, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
      dwSubKeys = 0;

   // We will loop thru each key
   for( DWORD i = 0; i < dwSubKeys; i++ )
   {

      dwSize = sizeof(szCLSID);
      if(RegEnumKeyExA(hKey, i, szCLSID, &dwSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
      {
       
         // We probably do any validatition right about here...
         if ( !DoCheckSum( szCLSID, &ulCRC ) )
         {
            // skip the un-signed scanner
            continue;
         }

         // verify the scanner checksum
         //REVIEW: How should error be reported if virus scanner engine fails check?
         if ( RegOpenKeyExA( hKey, szCLSID, 0, KEY_READ, &hSubKey ) == ERROR_SUCCESS )
         {
             dwSize = sizeof(DWORD);
             dwCookie = -1;
             if (RegQueryValueExA( hSubKey, SCANNER_COOKIE, NULL, 
                        NULL, (BYTE *)&dwCookie, &dwSize) == ERROR_SUCCESS) {
                 
                 RegCloseKey( hSubKey );    

                 if ( ulCRC != (ULONG)dwCookie )
                 {
                     continue;
                 }
             } else
                 continue;

         } else
             continue;

         // Open the \HKCR\CLSID\xxx\VirusScanner key
         wsprintf(szBuf, VENDOR_REG, szCLSID);
         
         if(RegOpenKeyExA(HKEY_CLASSES_ROOT, szBuf, 0, KEY_READ, &hVendorKey) == ERROR_SUCCESS)
         {
            // MakeWideStrFromAnsi allocs memory using CoMemTaskAlloc (so we better free it later!)
            pwsz = MakeWideStrFromAnsi(szCLSID);

            if ( pwsz != NULL && SUCCEEDED(CLSIDFromString(pwsz, &clsid)) )
            {
               if ( SUCCEEDED(CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_IVirusScanEngine,(void **) &pvs)) )
               {
                  // Only know do we know we a) a key existed b) it was a clsid c) it was creatable and has
                  //  a IVirusScanner interface. Yea! Add it to our list of providers
                  
                   if (curProv == NULL) {
                      curProv = m_provList = new CVirusScanProvider();
                   } else {
                       curProv->nextProv = new CVirusScanProvider();
                       curProv = curProv->nextProv;
                   }

                   curProv->clsid = clsid;
                   curProv->pvs = pvs;

                  //Get the description
                  szBuf[0] = '\0';
                  dwSize = MAX_DESCRIPTION;
                  RegQueryValueExA(hVendorKey, VENDOR_DESC, NULL, NULL, (BYTE *) szBuf, &dwSize);
                  curProv->pwszDescription = MakeWideStrFromAnsi(szBuf);

                  //Get the Flags
                  dwFlags = 0;
                  dwSize = sizeof(dwFlags);
                  RegQueryValueExA(hVendorKey, VENDOR_FLAGS, NULL, NULL, (BYTE *) &dwFlags, &dwSize);
                  curProv->dwFlags = (DWORD)dwFlags;

                  // Everything is in place, bump our provider count
                  m_uNumProviders++;
       
               }
            }

            CoTaskMemFree(pwsz);
            RegCloseKey(hVendorKey);
         }

      } // get enumerated subkey
      
   } // loop

   if (curProv != NULL)
       curProv->nextProv = NULL;

   if(hKey)
      RegCloseKey(hKey);
}

CVirusCheck::~CVirusCheck()
{
CVirusScanProvider *curProv = m_provList, *temp;

    if (m_provList != NULL)
        delete m_provList;

}

//IUnknown things
STDMETHODIMP CVirusCheck::QueryInterface(REFIID riid, void **ppv)
{
   *ppv = NULL;

   //IUnknown
   if((riid == IID_IUnknown) || (riid == IID_IVirusScanner))
      *ppv = (IVirusScanner *)this;

   if(riid == IID_IRegisterVirusScanEngine)
      *ppv = (IRegisterVirusScanEngine *)this;

   // Sorry...
   if(*ppv == NULL)
      return ResultFromScode(E_NOINTERFACE);

   ((IUnknown *) *ppv)->AddRef();
   return NOERROR;
}


STDMETHODIMP_(ULONG) CVirusCheck::AddRef(void)
{
   m_cObjRef++;
   return m_cObjRef;
}

STDMETHODIMP_(ULONG) CVirusCheck::Release(void)
{
   m_cObjRef--;
   if(m_cObjRef != 0)
      return m_cObjRef;

   delete this;
   return 0;
}

STDMETHODIMP CVirusCheck::ScanForVirus(HWND hwnd, STGMEDIUM *pstgmed, LPWSTR pwszItemDesc,
                                       DWORD dwFlags, LPVIRUSINFO pVrsInfo)
{
   HRESULT hr;
   DWORD   dwMedium;
   BOOL    bOneCheckFailed = FALSE;
   BOOL    bOneCheckWorked = FALSE;
   LPWSTR  pwsz = NULL;
   LPTSTR   psz;
   LPVIRUSINFO pvrsinfo;
   LPSTREAM pIStream = NULL;
   LARGE_INTEGER li;
   g_pvp = m_provList;
 
   // Temporarily disable all Virus Scan UI
   dwFlags |= SFV_DONTDOUI;
   
   // No providers, not much to do
   if(m_provList == NULL || m_uNumProviders == 0)
      return VSCAN_E_NOPROVIDERS;

   if(pstgmed == NULL)
      return E_POINTER;

   if(pstgmed->tymed == TYMED_FILE)
      dwMedium = VSC_LIKESFILE;
   else if(pstgmed->tymed == TYMED_ISTREAM)
      dwMedium = VSC_LIKESISTREAM;
   else if(pstgmed->tymed == TYMED_HGLOBAL)
      dwMedium = VSC_LIKESHGLOBAL;
   else
      return E_INVALIDARG;

   if( !pVrsInfo || (pVrsInfo->cbSize != sizeof(VIRUSINFO)) )
         return E_INVALIDARG;

   pvrsinfo = pVrsInfo;

   // Need to be careful about Couldn't check vs virus found!
   for(UINT i = 0; i < m_uNumProviders; i++)
   {

       // Clear all fields
       // This is put here as a precaution to ensure one provider can't interfere another.
       pvrsinfo->wszVendorDescription[0] = '\0';
       pvrsinfo->wszVendorContactInfo[0] = '\0';
       pvrsinfo->hVendorIcon = NULL;
       pvrsinfo->wszVirusName[0] = '\0';
       pvrsinfo->wszVirusDescription[0] = '\0';
       
       if( g_pvp->dwFlags & dwMedium )
       {
         // make sure the starting date is set
         // RegisterStartDate( pvp[i].clsid );

         if (pstgmed->tymed == TYMED_ISTREAM)
         {
            LISet32(li, 0);
            // Set the seek pointer to the beginning, just in case
            pstgmed->pstm->Seek(li, STREAM_SEEK_SET, NULL);
         }

         if (FAILED(hr = g_pvp->pvs->ScanForVirus(hwnd, pstgmed, pwszItemDesc, GetScannerEngineFlags(dwFlags), 0, pvrsinfo))) {

            // We should do different things depending on the hr.
             // If the virus provider returns (VSE_E_UNSUPPORTEDINPUTTYPE)
             // We might want to return this to the caller. Specialy if
             // this is the only one.
            if ((hr == VSE_E_UNSUPPORTEDINPUTTYPE) &&
                (pstgmed->tymed == TYMED_ISTREAM))
            {
                // Convert to file and try again.
                pwsz = ConvertIStreamToFile(pstgmed->pstm);
                if (pwsz != NULL)
                {
                   STGMEDIUM stgMed;
                   stgMed.tymed = TYMED_FILE;
                   stgMed.lpszFileName = pwsz;
                   hr = g_pvp->pvs->ScanForVirus(hwnd, &stgMed, pwszItemDesc, GetScannerEngineFlags(dwFlags), 0, pvrsinfo);
                   // Delete temp file created by ConvertIStreamToFile
                   psz = MakeAnsiStrFromWide(pwsz);
                   DeleteFile(psz);
                   CoTaskMemFree(pwsz);
                   CoTaskMemFree(psz);
                }
            }
         }
         if(SUCCEEDED(hr))
         {
            if(hr == S_FALSE)
            {
               // Found a virus!!!
               FillDefaultVirusInfo( pvrsinfo, g_pvp, dwFlags );
               if(!(dwFlags & SFV_DONTDOUI)) {
                   hr = DoVirusFoundDefaultUI(hwnd, pvrsinfo, pwszItemDesc, dwFlags);
               }

               // try to delete the file
               if((hr == S_FALSE) && (dwFlags & SFV_DELETE) && (pstgmed->tymed == TYMED_FILE))
               {
#ifdef UNICODE
                  psz = pstgmed->lpszFileName;
#else
                  psz = MakeAnsiStrFromWide(pstgmed->lpszFileName);
#endif
                  if(!DeleteFile(psz))
                     hr = VSCAN_E_DELETEFAIL;
#ifndef UNICODE
                  CoTaskMemFree(psz);
#endif
               }

               if (!(dwFlags & SFV_WANTVENDORICON) && pvrsinfo->hVendorIcon != NULL)
               {
                   DestroyIcon(pvrsinfo->hVendorIcon);
               }

               return hr;
            }
            bOneCheckWorked = TRUE;
         }
         else
         {
            bOneCheckFailed = TRUE;
         }
      }
   }

   if(bOneCheckFailed)
      if(bOneCheckWorked)
         return VSCAN_E_CHECKPARTIAL;
      else
         return VSCAN_E_CHECKFAIL;

   return S_OK;
}

HRESULT CVirusCheck::DoVirusFoundDefaultUI(HWND hwnd, LPVIRUSINFO pvrsinfo, LPWSTR pwszDesc, DWORD dwFlags)
{
   VIRUSDLGPARAM vrsDlgParam;
   HRESULT hr;
   INT_PTR ret;

   vrsDlgParam.pwszDesc = pwszDesc;
   vrsDlgParam.pvrsinfo = pvrsinfo;

   ret = DialogBoxParam(g_hinst, MAKEINTRESOURCE(IDD_FOUNDVIRUS), hwnd, VirusFoundDlg, (LPARAM) &vrsDlgParam);
   if(ret == IDNO)
      hr = S_OK;
   else
      hr = S_FALSE;

   return hr;
}

void CVirusCheck::FillDefaultVirusInfo(LPVIRUSINFO pvrsinfo, CVirusScanProvider *vsp, DWORD dwFlags)
{
   // description
   if(pvrsinfo->wszVendorDescription[0] == '\0')
      CopyWideStr(pvrsinfo->wszVendorDescription, vsp->pwszDescription);

   // Contact info
   if( (pvrsinfo->wszVendorContactInfo[0] == '\0') || (pvrsinfo->hVendorIcon == NULL) )
   {
       LPSTR    pszCLSID;
       char     szVendorRegPath[MAX_STRING];
       LPWSTR   pwszTmp = NULL;
       HKEY     hVendorKey;
       DWORD    dwSize;

       // Open the \HKCR\CLSID\xxx\VirusScanner key
       StringFromCLSID(vsp->clsid, &pwszTmp);
       pszCLSID = MakeAnsiStrFromWide(pwszTmp);
       CoTaskMemFree(pwszTmp);  // Free the CLSID

       wsprintf(szVendorRegPath, VENDOR_REG, pszCLSID);
       CoTaskMemFree( pszCLSID );
       if( RegOpenKeyExA(HKEY_CLASSES_ROOT, szVendorRegPath, 0, KEY_READ, &hVendorKey) == ERROR_SUCCESS )
       {
            if ( pvrsinfo->wszVendorContactInfo[0] == '\0' )
            {
                char szBuf[MAX_URL_LENGTH];
                dwSize = MAX_URL_LENGTH;
                if (RegQueryValueExA( hVendorKey, VENDOR_CONTACT, NULL, NULL, 
                        (BYTE *)szBuf, &dwSize ) == ERROR_SUCCESS) {
                    
                    MultiByteToWideChar(CP_ACP, 0, szBuf, -1, pvrsinfo->wszVendorContactInfo, ARRAYSIZE(pvrsinfo->wszVendorContactInfo));
            
                }
            }

            // very dangouse thing !
            if ( pvrsinfo->hVendorIcon == NULL )
            {
                char szBuf[MAX_STRING];
                LPSTR pArg;
                HINSTANCE hDll;

                dwSize = MAX_STRING;
                RegQueryValueExA(hVendorKey, VENDOR_ICON, NULL, NULL, (BYTE *) szBuf, &dwSize);

                if ( szBuf[0] )
                {
                    pArg = strtok( szBuf, "," );
                    if ( pArg )
                    {
                        if ( hDll = LoadLibraryA( pArg ) )
                        {
                            if ( pArg = strtok( NULL, "," ) )
                            {
                                pvrsinfo->hVendorIcon = LoadIcon( hDll, MAKEINTRESOURCE(atoi(pArg)) );
                            }
                            FreeLibrary( hDll );
                        }
                    }
                }

            }
            RegCloseKey( hVendorKey );
       }
   }

   if(pvrsinfo->wszVirusName[0] == 0)
      CopyWideStr(pvrsinfo->wszVirusName, STR_UNKNOWN);

   if( (pvrsinfo->wszVirusDescription[0] == 0) || (lstrcmpiW(pvrsinfo->wszVirusDescription, L"NONE" ) == 0) )
      CopyWideStr(pvrsinfo->wszVirusDescription, STR_UNKNOWN);

   pvrsinfo->cbSize = sizeof(VIRUSINFO);

}


STDMETHODIMP CVirusCheck::RegisterScanEngine(REFCLSID rclsid, LPWSTR pwszDescription, DWORD dwFlags,
                                             DWORD dwReserved, DWORD *pdwCookie)
{
    char    szSubKey[MAX_STRING];
    char    szValue[MAX_STRING];
    HKEY    hKeyScanner;
    ULONG   ulSize;
    ULONG   ulCRC = 0;
    LPOLESTR    pwszClsid = NULL;
    LPSTR   pszCLSID = NULL;
    LPSTR   psz;
    UINT    idErr = 0;
    HRESULT hr = E_FAIL;

    // Get confirmation from user before register the scanner
    //
    if ( (dwFlags & SFV_DONTDOUI) ||
         DialogBoxParam(g_hinst, MAKEINTRESOURCE(IDD_REGISTER_VSCANNER), NULL, (DLGPROC)RegisterVirusScannerDlg, (LPARAM) pwszDescription) == IDYES )
    {
       StringFromCLSID(rclsid, &pwszClsid);
       pszCLSID = MakeAnsiStrFromWide(pwszClsid);
       CoTaskMemFree(pwszClsid);  // Free the CLSID

       // checking code sign
       if ( (dwFlags & SFV_DONTDOUI) || (IsScannerSigned(pszCLSID) != E_FAIL) )
       {
           // want to make sure the vendor's reg entries are set properly
           // such as VendorContactInfo and VendorDescription.
           //
           wsprintf( szSubKey, VENDOR_REG, pszCLSID );
           if ( RegOpenKeyExA(HKEY_CLASSES_ROOT, szSubKey, 0, KEY_READ, &hKeyScanner) == ERROR_SUCCESS )
           {
               // Compare Vendor Description
               ulSize = sizeof(szValue);
               if ( (RegQueryValueExA(hKeyScanner, VENDOR_DESC, NULL, NULL, (LPBYTE)szValue, &ulSize) == ERROR_SUCCESS) &&
                    (RegQueryValueExA(hKeyScanner, VENDOR_CONTACT, NULL, NULL, NULL, &ulSize) == ERROR_SUCCESS) &&
                    (ulSize>0) )
               {
                   psz = MakeAnsiStrFromWide(pwszDescription);
                   if ( lstrcmpi(psz, szValue) == 0 )
                   {
                       // Generate/Store and set cookie
                       if ( DoCheckSum( pszCLSID,  &ulCRC ) )
                       {
                           // Add Registry key to Virus Check\Scanners
                           if ( SUCCEEDED(AddScanner(pszCLSID, psz, &ulCRC, TRUE)) )
                           {
                               if (pdwCookie)
                                  *pdwCookie = ulCRC;
                               hr = S_OK;
                           }
                       }

                    }
                    else
                    {
                        // Description not the same.
                        idErr = IDS_ERR_MISMHVENDORDESC;
                    }
                    CoTaskMemFree(psz);
               }
               else
                   idErr = IDS_ERR_NOVENDORINFO;
               RegCloseKey(hKeyScanner);
           }
           else
               idErr = IDS_ERR_NOVENDOR;
       }
       else
           idErr = IDS_ERR_CODESIGN;

       CoTaskMemFree(pszCLSID); // Free the ANSI version of the CLSID

       if ( idErr )
       {
          ErrMsgBox( idErr, g_szTitle, MB_OK );
       }
    }
    return hr;
}


STDMETHODIMP CVirusCheck::UnRegisterScanEngine(REFCLSID rclsid, LPWSTR pwszDescription,
                                               DWORD dwFlags, DWORD dwReserved, DWORD dwCookie )
{

   return( RemoveScanner( rclsid, pwszDescription, dwCookie ) );
}

inline DWORD CVirusCheck::GetScannerEngineFlags(DWORD dwFlags) 
{ 
    return (dwFlags & 0xFFFF0000); 
};

CVirusScanProvider::CVirusScanProvider()
{
   pwszDescription = NULL;
   dwFlags = 0;
   pvs = NULL;
   nextProv = NULL;
}

CVirusScanProvider::~CVirusScanProvider()
{
   if(pwszDescription)
      CoTaskMemFree(pwszDescription);

   if(pvs)
      pvs->Release();

   if (nextProv != NULL)
       delete nextProv;
}


INT_PTR CALLBACK VirusFoundDlg(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   static RECT rctDlg;

   switch(uMsg)
   {
       case WM_INITDIALOG:
           {
               VIRUSDLGPARAM *pvs;

               pvs = (VIRUSDLGPARAM *)lParam;
               GetWindowRect(hwnd, &rctDlg);
               if ( pvs->pvrsinfo->hVendorIcon )
               {
                  SendDlgItemMessage( hwnd, IDC_VENDORICON, STM_SETICON, (WPARAM)(HICON)pvs->pvrsinfo->hVendorIcon, 0L );
               }
               SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)pvs);
               InitVirusFoundDlg(hwnd, pvs, rctDlg);

           }
         break;

      case WM_COMMAND:
         switch(wParam)
         {
            case IDYES:
                EndDialog(hwnd, wParam);
                break;

             case IDNO:
               {
                   TCHAR szBuf[MAX_STRING];
                   TCHAR szTitle[MAX_STRING];
                   szBuf[0] = szTitle[0] = '\0';

                   LoadString(g_hinst, IDS_IGNOREWARNCONFIRM, szBuf, ARRAYSIZE(szBuf) );
                   LoadString(g_hinst, IDS_WARNING, szTitle, ARRAYSIZE(szTitle) );

                   if ( szBuf[0] && szTitle[0])
                   {
                       if ( MessageBox( hwnd, szBuf, szTitle, MB_ICONWARNING|MB_YESNO ) == IDNO )
                            break;
                   }
                   EndDialog(hwnd, wParam);
               }
               break;

             case IDC_VENDORINFO:
                 if (g_pvp != NULL)
                     g_pvp->pvs->DisplayCustomInfo();
                 break;

            case IDC_DETAILS:
                 {
                     LONG  lBaseUnit;
                     VIRUSDLGPARAM *pvrs;
                     LPTSTR lpBuf, lpTmpName, lpTmpDesc;
                     TCHAR szBuf[MAX_STRING], szTmp[MAX_STRING], szTmp2[MAX_STRING];
                     szBuf[0] = szTmp[0] = szTmp2[0] = '\0';

                     lpBuf = (LPTSTR) CoTaskMemAlloc( MAX_URL_LENGTH+1024 );
                     if ( !lpBuf )
                     {
                         ErrMsgBox( IDS_ERR_OUTMEM, g_szTitle, MB_OK );
                         break;
                     }
                     pvrs = (VIRUSDLGPARAM *)GetWindowLongPtr(hwnd, DWLP_USER);

                     SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, (rctDlg.right - rctDlg.left),
                                  (rctDlg.bottom - rctDlg.top), SWP_NOMOVE | SWP_NOZORDER);

                     SendDlgItemMessage( hwnd, IDC_DETAIL_GROUP, EM_SETMARGINS, EC_LEFTMARGIN, MAKELONG(10, 10) );
                     // SendDlgItemMessage( hwnd, IDC_DETAIL_GROUP, EM_SETBKGNDCOLOR, 0, RGB(256,256,256) );
                     lBaseUnit = (GetDialogBaseUnits()&0x0000FFFF)*4 ;
                     SendDlgItemMessage( hwnd, IDC_DETAIL_GROUP, EM_SETTABSTOPS, 1, (LPARAM)(LPDWORD)&lBaseUnit);


                     if ( *pvrs->pvrsinfo->wszVirusDescription &&
                          lstrcmpiW(pvrs->pvrsinfo->wszVirusDescription, STR_UNKNOWN) )
                     {
                        TCHAR szTmp3[MAX_STRING];
						szTmp3[0] = '\0';

#ifdef UNICODE
                        lpTmpDesc = pvrs->pvrsinfo->wszVirusDescription;
#else
                        lpTmpDesc = MakeAnsiStrFromWide( pvrs->pvrsinfo->wszVirusDescription );
#endif
                        LoadString(g_hinst, IDS_VIRUSDESC, szTmp3, ARRAYSIZE(szTmp));
                        if (szTmp3[0])
                        {
                            wsprintf(szTmp2,szTmp3,lpTmpDesc);
                        }
#ifndef UNICODE
                        CoTaskMemFree( lpTmpDesc );
#endif
                     }


#ifdef UNICODE
                     lpTmpName = pvrs->pvrsinfo->wszVirusName;
                     lpTmpDesc = pvrs->pvrsinfo->wszVendorDescription;
#else
                     lpTmpName = MakeAnsiStrFromWide( pvrs->pvrsinfo->wszVirusName );
                     lpTmpDesc = MakeAnsiStrFromWide( pvrs->pvrsinfo->wszVendorDescription );
#endif
                     LoadString(g_hinst, IDS_VIRUSNAME, szTmp, ARRAYSIZE(szBuf));
                     
                     if (szTmp[0])
                     {
                         wsprintf(szBuf, szTmp, lpTmpName, lpTmpDesc, szTmp2);
                     }

#ifndef UNICODE
                     CoTaskMemFree( lpTmpName );
                     CoTaskMemFree( lpTmpDesc );
#endif
                     SetDlgItemText( hwnd, IDC_DETAIL_GROUP, szBuf);

                     CoTaskMemFree( lpBuf );

                 }
                 break;

            default:
               return FALSE;
         }
         break;

      default:
         return FALSE;
   }
   return TRUE;
}



void InitVirusFoundDlg(HWND hwnd, VIRUSDLGPARAM *pvrs, RECT rctDlg )
{
   RECT  rctDetails;
   INT   cx, cy;
   TCHAR szBuf[MAX_STRING];
   TCHAR szMessBuf[MAX_STRING];
   TCHAR *psz;

   // Fill in the header
   szBuf[0] = 0;
   szMessBuf[0] = 0;
   if(pvrs->pwszDesc != NULL)
   {
      LoadString(g_hinst, IDS_VIRUSFOUND, szBuf, ARRAYSIZE(szBuf) );
      if(szBuf[0] != 0)
      {
#ifdef UNICODE
         psz = pvrs->pwszDesc;
#else
         psz = MakeAnsiStrFromWide(pvrs->pwszDesc);
#endif
         wsprintf(szMessBuf, szBuf, psz);
#ifndef UNICODE
         CoTaskMemFree(psz);
#endif
      }
   }
   else
      LoadString(g_hinst, IDS_VIRUSFOUND_NODESC, szMessBuf, ARRAYSIZE(szMessBuf) );

   SetDlgItemText(hwnd, IDC_TEXT_FOUNDVIRUS, szMessBuf);


   GetWindowRect(GetDlgItem(hwnd, IDC_DETAIL_GROUP), &rctDetails);

   cx = rctDlg.right - rctDlg.left;
   cy = rctDetails.top - rctDlg.top;

   SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, cx, cy, SWP_NOMOVE | SWP_NOZORDER);

}



#define BUFFERSIZE   1024
LPWSTR ConvertIStreamToFile(LPSTREAM pIStream )
{
    HANDLE  fh;
    LPWSTR  pwsz = NULL;
    char szTempFile[MAX_PATH];
    LPVOID lpv = NULL;
    LARGE_INTEGER li;
    DWORD   dwl;
    ULONG   ul;
    HRESULT hr;

    if (GetTempPath(sizeof(szTempFile), szTempFile) > 0)
    {
        if (GetTempFileName(szTempFile, "~VS", 0, szTempFile) != 0)
        {
            fh = CreateFile(szTempFile, GENERIC_READ|GENERIC_WRITE,
                       0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if (fh != INVALID_HANDLE_VALUE)
            {
                lpv = (LPSTR)LocalAlloc(LPTR, BUFFERSIZE);
                if (lpv)
                {
                    LISet32(li, 0);
                    pIStream->Seek(li, STREAM_SEEK_SET, NULL); // Set the seek pointer to the beginning
                    do
                    {
                        hr = pIStream->Read(lpv, BUFFERSIZE, &ul);
                        if(SUCCEEDED(hr))
                        {
                            if (!WriteFile(fh, lpv, ul, &dwl, NULL))
                                hr = E_FAIL;
                        }
                    }
                    while ((SUCCEEDED(hr)) && (ul == BUFFERSIZE));

                }
                CloseHandle(fh);
                if (SUCCEEDED(hr))
                {
                    // Convert from ANSI to UNICODE
                    pwsz = MakeWideStrFromAnsi(szTempFile);
                }
                else
                {
                    // Failed during read or write, delete the file.
                    DeleteFile(szTempFile);
                }
            }

        }
    }
    return pwsz;
}

void InitRegisterVirusScannerDlg(HWND hwnd, LPWSTR pwszDesc)
{
   TCHAR *psz;

   if(pwszDesc != NULL)
   {
#ifdef UNICODE
       psz = pvrs->pwszDesc;
#else
       psz = MakeAnsiStrFromWide(pwszDesc);
#endif
       SetDlgItemText(hwnd, IDC_SCANNER_DESC, psz);
   }

#ifndef UNICODE
   CoTaskMemFree(psz);
#endif

}


BOOL CALLBACK RegisterVirusScannerDlg(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   switch(uMsg)
   {
      case WM_INITDIALOG:
         InitRegisterVirusScannerDlg(hwnd, (LPWSTR)lParam);
         break;

      case WM_COMMAND:
         switch(wParam)
         {
            case IDYES:
            case IDNO:
               EndDialog(hwnd, wParam);
               break;

            default:
               return FALSE;
         }
         break;

      default:
         return FALSE;
   }
   return TRUE;
}

HRESULT IsScannerSigned(LPSTR pszCLSID)
{
   HKEY hKey;
   char szSubKey[MAX_STRING];
   char szValue[MAX_STRING];
   ULONG    ulSize;
   HRESULT hr = E_FAIL;

   // Need to get the path for the scanner from the registry "InprocServer32"
   wsprintf(szSubKey, SCANNER_CLSID_REG, pszCLSID);
   if (RegOpenKeyExA(HKEY_CLASSES_ROOT, szSubKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
   {
      ulSize = sizeof(szValue);
      // Get the default value, this should contain the full qualified path of the DLL
      if (RegQueryValueExA(hKey, "", NULL, NULL, (LPBYTE)szValue, &ulSize) == ERROR_SUCCESS)
      {
         hr = CheckTrust((LPSTR) szValue);
      }
      RegCloseKey(hKey);
   }
   return hr;
}


HRESULT AddScanner(LPSTR pszCLSID, LPSTR psz, LPDWORD pdwCookie, BOOL bNewEntry)
{
   HRESULT hr = E_FAIL;
   HKEY hKey;
   HKEY hKeyScanner;
   DWORD dwDisposition;

   // make sure the parent keys exist
   if ( FAILED( MakeSureKeyExist( HKEY_LOCAL_MACHINE, SCANNER_VIRUSCHECK ) ) )
   {
        return hr;
   }

   if ( FAILED( MakeSureKeyExist( HKEY_LOCAL_MACHINE, SCANNER_KEY ) ) )
   {
        return hr;
   }

   if ( RegOpenKeyExA(HKEY_LOCAL_MACHINE, SCANNER_KEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS )
   {
      if ( RegCreateKeyExA( hKey, pszCLSID, 0, "",
                            REG_OPTION_NON_VOLATILE, KEY_READ|KEY_WRITE,
                            NULL, &hKeyScanner, &dwDisposition) == ERROR_SUCCESS )
      {
         if ( bNewEntry && (dwDisposition == REG_OPENED_EXISTING_KEY) )
         {
            // want to create new key but key already exists.  Clear it up and recreate it.
            //
            RegCloseKey(hKeyScanner);
            RegDeleteKeyA(hKey, pszCLSID);

            if ( RegCreateKeyExA( hKey, pszCLSID, 0, "",
                                  REG_OPTION_NON_VOLATILE, KEY_READ|KEY_WRITE,
                                  NULL, &hKeyScanner, &dwDisposition) != ERROR_SUCCESS)
            {
                // this just try to skip the next if() { }
                goto done;
            }
         }

         RegSetValueExA( hKeyScanner, "", 0, REG_SZ, (CONST BYTE *)psz, lstrlen(psz)+1 );
         RegSetValueExA( hKeyScanner, SCANNER_COOKIE, 0, REG_DWORD, (CONST BYTE *)pdwCookie, sizeof(DWORD) );
         RegCloseKey( hKeyScanner );
         hr = S_OK;
      }

done:
      RegCloseKey(hKey);
   }
   return hr;
}


HRESULT RemoveScanner( REFCLSID rclsid, LPWSTR pDesc, DWORD dwCookie )
{
   HRESULT  hr = E_FAIL;
   HKEY     hKey, hSubKey;
   LPOLESTR pwszCLSID;
   LPSTR    pszCLSID;
   LPSTR    pszInDesc;
   DWORD    dwTmp;
   DWORD    dwRegCookie;
   char     szDesc[MAX_PATH];

   StringFromCLSID(rclsid, &pwszCLSID);
   pszCLSID = MakeAnsiStrFromWide(pwszCLSID);
   pszInDesc = MakeAnsiStrFromWide(pDesc);
   CoTaskMemFree(pwszCLSID);  // Free the CLSID

   if ( (RegOpenKeyExA(HKEY_LOCAL_MACHINE, SCANNER_KEY, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS) &&
        (RegOpenKeyExA(hKey, pszCLSID, 0, KEY_ALL_ACCESS, &hSubKey) == ERROR_SUCCESS) )
   {
      // check the cookie and description before remove the scanner
      //
      dwTmp = sizeof(szDesc);
      if ( (RegQueryValueExA(hSubKey, "", NULL, NULL, (LPBYTE)szDesc, &dwTmp) == ERROR_SUCCESS) &&
           (lstrcmpi(szDesc, pszInDesc) == 0 ) )
      {
          dwTmp = sizeof(DWORD);
          if ( (RegQueryValueExA(hSubKey, SCANNER_COOKIE, NULL, NULL, (LPBYTE)&dwRegCookie, &dwTmp) == ERROR_SUCCESS) &&
               (dwCookie == dwRegCookie ) )
          {
               if ( DeleteKeyAndSubKeys(hKey, pszCLSID) )
                    hr = S_OK;
               else
                    hr = S_FALSE;
          }
      }
      RegCloseKey(hSubKey);
      RegCloseKey(hKey);
   }
   CoTaskMemFree(pszCLSID);
   CoTaskMemFree(pszInDesc);
   return hr;
}

#if 0
void RegisterStartDate( CLSID clsid )
{
    HKEY        hKey;
    LPOLESTR    pwszCLSID;
    LPSTR       pszCLSID;
    DWORD       dwSize;
    char        szSubKey[MAX_STRING];
    char        szTime[MAX_DESCRIPTION];

    StringFromCLSID( clsid, &pwszCLSID );
    pszCLSID = MakeAnsiStrFromWide(pwszCLSID);
    CoTaskMemFree(pwszCLSID);

    lstrcpy( szSubKey, SCANNER_KEY );
    lstrcat( szSubKey, "\\" );
    lstrcat( szSubKey, pszCLSID );
    if ( RegOpenKeyExA(HKEY_LOCAL_MACHINE, szSubKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS )
    {
        dwSize = sizeof(szTime);
        if ( RegQueryValueExA(hKey, SCANNER_STARTDATE, NULL, NULL, (LPBYTE)szTime, &dwSize) != ERROR_SUCCESS )
        {
            SYSTEMTIME systemTime;

            GetSystemTime( &systemTime );
            dwSize = sizeof(SYSTEMTIME);
            RegSetValueExA(hKey, SCANNER_STARTDATE, NULL, REG_BINARY, (LPBYTE)&systemTime, dwSize);
        }
        RegCloseKey( hKey );
    }
    CoTaskMemFree( pszCLSID );
}
#endif

BOOL DoCheckSum( LPSTR pszCLSID, ULONG *pulCRC )
{
    ULONG   ulCRC = 0;
    HKEY    hKey;
    char    szSubKey[MAX_STRING];
    char    szValue[MAX_PATH];
    HANDLE  hFile;
    DWORD   dwSize;

    // Need to get the path for the scanner from the registry "InprocServer32"
    wsprintf(szSubKey, SCANNER_CLSID_REG, pszCLSID);
    if (RegOpenKeyExA(HKEY_CLASSES_ROOT, szSubKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
       dwSize = sizeof(szValue);
       // Get the default value, this should contain the full qualified path of the DLL
       if (RegQueryValueExA(hKey, "", NULL, NULL, (LPBYTE)szValue, &dwSize) == ERROR_SUCCESS)
       {
          hFile = CreateFile( szValue, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
          if ( hFile != INVALID_HANDLE_VALUE )
          {
             dwSize = GetFileSize( hFile, NULL );
          }
       }
       RegCloseKey(hKey);
    }
    else
       return FALSE;

    ulCRC = CRC32Compute( (BYTE *)&dwSize, sizeof(DWORD), ulCRC );
    ulCRC = CRC32Compute( (BYTE *)pszCLSID, lstrlen(pszCLSID), ulCRC );

    *pulCRC = ulCRC;

    return TRUE;
}
