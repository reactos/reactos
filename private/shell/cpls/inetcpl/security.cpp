//*********************************************************************
//*          Microsoft Windows                                       **
//*        Copyright(c) Microsoft Corp., 1995                        **
//*********************************************************************

//
// SECURITY.cpp - "Security" Property Sheet
//

// HISTORY:
//
// 6/22/96  t-gpease    moved to this file
// 5/14/97  t-ashlm     new dialog

#include "inetcplp.h"
#include "inetcpl.h"   // for LSDFLAGS
#include "intshcut.h"
#include "permdlg.h"   // java permissions
#include "pdlgguid.h"  // guids for Java VM permissions dlg
#include <cryptui.h>

#include <mluisupp.h>

void LaunchSecurityDialogEx(HWND hDlg, DWORD dwZone, BOOL bForceUI, BOOL bDisableAddSites);

//
// Private Functions and Structures
//
INT_PTR CALLBACK SecurityAddSitesDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK SecurityCustomSettingsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,LPARAM lParam);
INT_PTR CALLBACK SecurityAddSitesIntranetDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,LPARAM lParam);
void SecurityChanged();

TCHAR *MyIntToStr(TCHAR *pBuf, BYTE iVal);
BOOL SecurityDlgInit(HWND hDlg);


#define WIDETEXT(x) L ## x
#define REGSTR_PATH_SECURITY_LOCKOUT  TEXT("Software\\Policies\\Microsoft\\Windows\\CurrentVersion\\Internet Settings")
#define REGSTR_VAL_OPTIONS_EDIT       TEXT("Security_options_edit")
#define REGSTR_VAL_ZONES_MAP_EDIT     TEXT("Security_zones_map_edit")
#define REGSTR_VAL_HKLM_ONLY          TEXT("Security_HKLM_only")
#define REGSTR_PATH_SO                TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\SO")
#define REGSTR_PATH_SOIEAK            TEXT("Sofwtare\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\SOIEAK")

///////////////////////////////////////////////////////////////////////////////////////
//
// Structures
//
///////////////////////////////////////////////////////////////////////////////////////

typedef struct tagSECURITYZONESETTINGS
{
    BOOL    dwFlags;            // from the ZONEATTRIBUTES struct
    DWORD   dwZoneIndex;        // as defined by ZoneManager
    DWORD   dwSecLevel;         // current level (High, Medium, Low, Custom)
    DWORD   dwPrevSecLevel;
    DWORD   dwMinSecLevel;      // current min level (High, Medium, Low, Custom)
    DWORD   dwRecSecLevel;      // current recommended level (High, Medium, Low, Custom)
    TCHAR   szDescription[MAX_ZONE_DESCRIPTION];
    TCHAR   szDisplayName[MAX_ZONE_PATH];
    HICON   hicon;
} SECURITYZONESETTINGS, *LPSECURITYZONESETTINGS;

// structure for main security page
typedef struct tagSECURITYPAGE
{
    HWND                    hDlg;                   // handle to window
    LPURLZONEMANAGER        pInternetZoneManager;   // pointer to InternetZoneManager
    IInternetSecurityManager *pInternetSecurityManager; // pointer to InternetSecurityManager
    HIMAGELIST              himl;                   // imagelist for Zones combobox
    HWND                    hwndZones;              // zones combo box hwnd
    LPSECURITYZONESETTINGS  pszs;                   // current settings for displayed zone
    INT                     iZoneSel;               // selected zone (as defined by ComboBox)
    DWORD                   dwZoneCount;            // number of zones
    BOOL                    fChanged;
    BOOL                    fPendingChange;         // to prevent the controls sending multiple sets (for cancel, mostly)
    HINSTANCE               hinstUrlmon;
    BOOL                    fNoEdit;                // hklm lockout of level edit
    BOOL                    fNoAddSites;            // hklm lockout of addsites
    BOOL                    fNoZoneMapEdit;         // hklm lockout of zone map edits
    HFONT                   hfontBolded;            // special bolded font created for the zone title
    BOOL                    fForceUI;               // Force every zone to show ui?
    BOOL                    fDisableAddSites;       // Automatically diable add sites button?
} SECURITYPAGE, *LPSECURITYPAGE;

// structure for Intranet Add Sites
typedef struct tagADDSITESINTRANETINFO {
    HWND hDlg;                                      // handle to window
    BOOL fUseIntranet;                              // Use local defined intranet addresses (in reg)
    BOOL fUseProxyExclusion;                        // Use proxy exclusion list
    BOOL fUseUNC;                                   // Include UNC in intranet
    LPSECURITYPAGE pSec;            
} ADDSITESINTRANETINFO, *LPADDSITESINTRANETINFO;

// structure for Add Sites
typedef struct tagADDSITESINFO {
    HWND hDlg;                                      // handle to window
    BOOL fRequireServerVerification;                // Require Server Verification on sites in zone
    HWND hwndWebSites;                              // handle to list
    HWND hwndAdd;                                   // handle to edit
    TCHAR szWebSite[MAX_ZONE_PATH];                 // text in edit control
    BOOL fRSVOld;
    LPSECURITYPAGE pSec;            
} ADDSITESINFO, *LPADDSITESINFO;

// structure for Custom Settings 
typedef struct tagCUSTOMSETTINGSINFO {
    HWND  hDlg;                                     // handle to window
    HWND hwndTree;

    LPSECURITYPAGE pSec;
    HWND hwndCombo;
    INT iLevelSel;
    IRegTreeOptions *pTO;
    BOOL fUseHKLM;          // get/set settings from HKLM
    DWORD dwJavaPolicy;     // Java policy selected
    BOOL fChanged;
} CUSTOMSETTINGSINFO, *LPCUSTOMSETTINGSINFO;


BOOL SecurityEnableControls(LPSECURITYPAGE pSec, BOOL fSetFocus);
BOOL SecurityDlgApplyNow(LPSECURITYPAGE pSec, BOOL bPrompt);


// global variables
extern DWORD g_dwtlsSecInitFlags;

extern BOOL g_fSecurityChanged; // flag indicating that Active Security has changed.

//////////////////////////////////////////////////////////////////////////////
//
// Main Security Page Helper Functions
//
//////////////////////////////////////////////////////////////////////////////

#define NUM_TEMPLATE_LEVELS      4
TCHAR g_szLevel[3][64];
TCHAR LEVEL_DESCRIPTION0[300];
TCHAR LEVEL_DESCRIPTION1[300];
TCHAR LEVEL_DESCRIPTION2[300];
TCHAR LEVEL_DESCRIPTION3[300];
LPTSTR LEVEL_DESCRIPTION[NUM_TEMPLATE_LEVELS] = {
    LEVEL_DESCRIPTION0,
    LEVEL_DESCRIPTION1,
    LEVEL_DESCRIPTION2,
    LEVEL_DESCRIPTION3
};
TCHAR CUSTOM_DESCRIPTION[300];

TCHAR LEVEL_NAME0[30];
TCHAR LEVEL_NAME1[30];
TCHAR LEVEL_NAME2[30];
TCHAR LEVEL_NAME3[30];
LPTSTR LEVEL_NAME[NUM_TEMPLATE_LEVELS] = {
    LEVEL_NAME0,
    LEVEL_NAME1,
    LEVEL_NAME2,
    LEVEL_NAME3
};
TCHAR CUSTOM_NAME[30];

// Some accessibility related prototypes.

// Our override of the slider window proc. 
LRESULT CALLBACK SliderSubWndProc (HWND hwndSlider, UINT uMsg, WPARAM wParam, LPARAM lParam, WPARAM uID, ULONG_PTR dwRefData );

extern BOOL g_fAttemptedOleAccLoad ;
extern HMODULE g_hOleAcc;


// Can't find value for WM_GETOBJECT in the headers. Need to figure out the right header to include
// here. 
#ifndef WM_GETOBJECT
#define WM_GETOBJECT        0x03d
#endif

// Prototype for CreateStdAccessibleProxy.
// A and W versions are available - pClassName can be ANSI or UNICODE
// string. This is a TCHAR-style prototype, but you can do a A or W
// specific one if desired.
typedef HRESULT (WINAPI *PFNCREATESTDACCESSIBLEPROXY) (
    HWND     hWnd,
    LPTSTR   pClassName,
    LONG     idObject,
    REFIID   riid,
    void **  ppvObject 
    );
/*
 * Arguments:
 *
 * HWND hWnd
 *   Handle of window to return IAccessible for.
 *
 * LPTSTR pClassName
 *   Class name indicating underlying class of the window. For
 *   example, if "LISTBOX" is used here, the returned object will
 *   behave appropriately for a listbox, and will expect the given
 *   hWnd to support listbox messages and styles. This argument
 *   nearly always reflects the window class from which the control
 *   is derived.
 *
 * LONG idObject
 *   Always OBJID_CLIENT
 *
 * REFIID riid
 *   Always IID_IAccessible
 *
 * void ** ppvObject
 *   Out pointer used to return an IAccessible to a newly-created
 *   object which represents the control hWnd as though it were of
 *   window class pClassName.
 *
 * If successful,
 * returns S_OK, *ppvObject != NULL;
 * otherwise returns error HRESULT.
 *
 *
 */



// Same for LresultFromObject...
typedef LRESULT (WINAPI *PFNLRESULTFROMOBJECT)(
    REFIID riid,
    WPARAM wParam,
    LPUNKNOWN punk 
    );


PRIVATE PFNCREATESTDACCESSIBLEPROXY s_pfnCreateStdAccessibleProxy = NULL;
PRIVATE PFNLRESULTFROMOBJECT s_pfnLresultFromObject = NULL;

// Simple accessibility wrapper class which returns the right string values

class CSecurityAccessibleWrapper: public CAccessibleWrapper
{
                // Want to remember the hwnd of the trackbar...
                HWND m_hWnd;
public:
                CSecurityAccessibleWrapper( HWND hWnd, IAccessible * pAcc );
               ~CSecurityAccessibleWrapper();

                STDMETHODIMP get_accValue(VARIANT varChild, BSTR* pszValue);
};

// Ctor - pass through the IAccessible we're wrapping to the
// CAccessibleWrapper base class; also remember the trackbar hwnd.
CSecurityAccessibleWrapper::CSecurityAccessibleWrapper( HWND hWnd, IAccessible * pAcc )
    : CAccessibleWrapper( pAcc ),
      m_hWnd( hWnd )

{
    // Do nothing
}

// Nothing to do here - but if we do need to do cleanup, this is the
// place for it.
CSecurityAccessibleWrapper::~CSecurityAccessibleWrapper()
{
    // Do nothing
}


// Overridden get_accValue method...
STDMETHODIMP   CSecurityAccessibleWrapper::get_accValue(VARIANT varChild, BSTR* pszValue)
{
    // varChild.lVal specifies which sub-part of the component
    // is being queried.
    // CHILDID_SELF (0) specifies the overall component - other
    // non-0 values specify a child.

    // In a trackbar, CHILDID_SELF refers to the overall trackbar
    // (which is what we want), whereas other values refer to the
    // sub-components - the actual slider 'thumb', and the 'page
    // up/page down' areas to the left/right of it.
    if( varChild.vt == VT_I4 && varChild.lVal == CHILDID_SELF )
    {
        // Get the scrollbar value...
        int iPos = (int)SendMessage( m_hWnd, TBM_GETPOS , 0, 0 );

        // Check that it's in range...
        // (It's possible that we may get this request after the
        // trackbar has been created, bu before we've set it to
        // a meaningful value.)
        if( iPos < 0 || iPos >= NUM_TEMPLATE_LEVELS )
        {
            TCHAR rgchUndefined[40];
            int cch = MLLoadString(IDS_TEMPLATE_NAME_UNDEFINED, rgchUndefined, ARRAYSIZE(rgchUndefined));
            if (cch != 0)
            {
                *pszValue = SysAllocString(rgchUndefined);
            }
            else
            {
                // Load String failed, for some reason.
                return HRESULT_FROM_WIN32(GetLastError());
            }
    
        }
        else
        {
            *pszValue = SysAllocString( LEVEL_NAME[iPos]);
        }
        
        // All done!
        return S_OK;

    }
    else
    {
        // Pass requests about the sub-components to the
        // base class (which will forward to the 'original'
        // IAccessible for us).
        return CAccessibleWrapper::get_accValue(varChild, pszValue);
    }
}


// Converting the Security Level DWORD identitifiers to slider levels, and vice versa
int SecLevelToSliderPos(DWORD dwLevel)
{
    switch(dwLevel)
    {
        case URLTEMPLATE_LOW:
            return 3;
        case URLTEMPLATE_MEDLOW:
            return 2;
        case URLTEMPLATE_MEDIUM:
            return 1;
        case URLTEMPLATE_HIGH:
            return 0;        
        case URLTEMPLATE_CUSTOM:
            return -1;            
        default:
            return -2;
    }
}

DWORD SliderPosToSecLevel(int iPos)
{
    switch(iPos)
    {
        case 3:
            return URLTEMPLATE_LOW;
        case 2:
            return URLTEMPLATE_MEDLOW;
        case 1:
            return URLTEMPLATE_MEDIUM;
        case 0:
            return URLTEMPLATE_HIGH;
        default:
            return URLTEMPLATE_CUSTOM;
    }
}

int ZoneIndexToGuiIndex(DWORD dwZoneIndex)
// Product testing asked for the zones in a specific order in the list box;
// This function returns the desired gui position for a given zone
// Unrecognized zones are added to the front
{
    int iGuiIndex = -1;
    switch(dwZoneIndex)
    {
        // Intranet: 2nd spot
        case 1:
            iGuiIndex = 1;
            break;

        // Internet: 1st spot
        case 3:
            iGuiIndex = 0;
            break;

        // Trusted Sites: 3rd Spot
        case 2:
            iGuiIndex = 2;
            break;

        // Restricted Sites: 4th Spot
        case 4:
            iGuiIndex = 3;
            break;

        // unknown zone
        default:
            iGuiIndex = -1;   
            break;
    }


    return iGuiIndex;
}



// Initialize the global variables (to be destroyed at WM_DESTROY)
// pSec, Urlmon, pSec->pInternetZoneManager, pSec->hIml
// and set up the proper relationships among them
BOOL SecurityInitGlobals(LPSECURITYPAGE * ppSec, HWND hDlg, SECURITYINITFLAGS * psif)
{
    DWORD cxIcon;
    DWORD cyIcon;

    LPSECURITYPAGE pSec = NULL;

    *ppSec = (LPSECURITYPAGE)LocalAlloc(LPTR, sizeof(SECURITYPAGE));
    pSec = *ppSec;
    if (!pSec)
    {
        return FALSE;   // no memory?
    }

    // make sure Urlmon stays around until we're done with it.
    pSec->hinstUrlmon = LoadLibrary(TEXT("URLMON.DLL"));
    if(pSec->hinstUrlmon == NULL)
    {
        return FALSE;  // no urlmon?
    }

    // Get the zone manager
    if (FAILED(CoInternetCreateZoneManager(NULL, &(pSec->pInternetZoneManager),0)))
    {
        return FALSE;  // no zone manager?
    }

    // get our zones hwnd
    pSec->hwndZones = GetDlgItem(hDlg, IDC_LIST_ZONE);
    if(! pSec->hwndZones)
    {
        ASSERT(FALSE);
        return FALSE;  // no list box?
    }

    // Get the internet secrity manager (for telling if a zone is empty, 
    // and deciphering the current URL
    if(FAILED(CoInternetCreateSecurityManager(NULL, &(pSec->pInternetSecurityManager), 0)))
        pSec->pInternetSecurityManager = NULL;

    // tell dialog where to get info
    SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pSec);

    // save the handle to the page
    pSec->hDlg = hDlg;
    pSec->fPendingChange = FALSE;

    // set dialog options: force ui and disable add sites
    if(psif)
    {
        pSec->fForceUI = psif->fForceUI;
        pSec->fDisableAddSites = psif->fDisableAddSites;
    }
    
    // create an imagelist for the ListBox            
    cxIcon = GetSystemMetrics(SM_CXICON);
    cyIcon = GetSystemMetrics(SM_CYICON);
#ifndef UNIX
    UINT flags = ILC_COLOR32|ILC_MASK;
    
    if(IS_WINDOW_RTL_MIRRORED(hDlg))
    {
        flags |= ILC_MIRROR;
    }
    pSec->himl = ImageList_Create(cxIcon, cyIcon, flags, pSec->dwZoneCount, 0);
#else
    pSec->himl = ImageList_Create(cxIcon, cyIcon, ILC_COLOR|ILC_MASK, pSec->dwZoneCount, 0);
#endif
    if(! pSec->himl)
    {
        return FALSE;  // Image list not created
    }
    SendMessage(pSec->hwndZones, LVM_SETIMAGELIST, (WPARAM)LVSIL_NORMAL, (LPARAM)pSec->himl);

    return TRUE;    
}

// Set up the variables in pSec about whether the zone settings can be editted
void SecuritySetEdit(LPSECURITYPAGE pSec)
{
    // if these calls fail then we'll use the default of zero which means no lockout
    DWORD cb;
    

    cb = SIZEOF(pSec->fNoEdit);  
    SHGetValue(HKEY_LOCAL_MACHINE, REGSTR_PATH_SECURITY_LOCKOUT, REGSTR_VAL_OPTIONS_EDIT, 
                NULL, &(pSec->fNoEdit), &cb);

    // also allow g_restrict to restrict changing settings
    pSec->fNoEdit += g_restrict.fSecChangeSettings;
    
    SHGetValue(HKEY_LOCAL_MACHINE, REGSTR_PATH_SECURITY_LOCKOUT, REGSTR_VAL_OPTIONS_EDIT, 
                NULL, &(pSec->fNoAddSites), &cb);

    cb = SIZEOF(pSec->fNoZoneMapEdit);
    SHGetValue(HKEY_LOCAL_MACHINE, REGSTR_PATH_SECURITY_LOCKOUT, REGSTR_VAL_ZONES_MAP_EDIT, 
                NULL, &(pSec->fNoZoneMapEdit), &cb);

    // also allow the g_restrict to restrict edit
    pSec->fNoAddSites += g_restrict.fSecAddSites;
}


// Fill a zone with information from the zone manager and add it to the
// ordered list going to the listbox
// REturn values:
//  S_OK indicates success
//  S_FALSE indicates a good state, but the zone was not added (example: flag ZAFLAGS_NO_UI)
//  E_OUTOFMEMORY
//  E_FAIL - other failure
HRESULT SecurityInitZone(DWORD dwIndex, LPSECURITYPAGE pSec, DWORD dwZoneEnumerator, 
                         LV_ITEM * plviZones, BOOL * pfSpotTaken)
{
        DWORD                   dwZone;
        ZONEATTRIBUTES          za = {0};
        HICON                   hiconSmall = NULL;
        HICON                   hiconLarge = NULL;
        LPSECURITYZONESETTINGS  pszs;
        WORD                    iIcon=0;
        LPWSTR                  psz;
        TCHAR                   szIconPath[MAX_PATH];
        int                     iSpot;
        LV_ITEM *               plvItem;
        HRESULT                 hr = 0;



        // get the zone attributes for this zone
        za.cbSize = sizeof(ZONEATTRIBUTES);
        pSec->pInternetZoneManager->GetZoneAt(dwZoneEnumerator, dwIndex, &dwZone);
        hr = pSec->pInternetZoneManager->GetZoneAttributes(dwZone, &za);
        if(FAILED(hr))
        {
            return S_FALSE;
        }



        // if no ui, then ignore
        if ((za.dwFlags & ZAFLAGS_NO_UI) && !pSec->fForceUI)
        {
            return S_FALSE;
        }



        // create a structure for zone settings
        pszs = (LPSECURITYZONESETTINGS)LocalAlloc(LPTR, sizeof(*pszs));
        if (!pszs)
        {
            return E_OUTOFMEMORY;
        }



        // store settings for later use
        pszs->dwFlags       = za.dwFlags;
        pszs->dwZoneIndex   = dwZone;
        pszs->dwSecLevel    = za.dwTemplateCurrentLevel;    
        pszs->dwMinSecLevel = za.dwTemplateMinLevel;
        pszs->dwRecSecLevel = za.dwTemplateRecommended;                 
#ifdef UNICODE
        StrCpyN(pszs->szDescription, za.szDescription, ARRAYSIZE(pszs->szDescription));
        StrCpyN(pszs->szDisplayName, za.szDisplayName, ARRAYSIZE(pszs->szDisplayName));
#else
        WideCharToMultiByte(CP_ACP, 0, za.szDescription, -1, pszs->szDescription, ARRAYSIZE(pszs->szDescription), NULL, NULL);
        WideCharToMultiByte(CP_ACP, 0, za.szDisplayName, -1, pszs->szDisplayName, ARRAYSIZE(pszs->szDisplayName), NULL, NULL);
#endif
        // load the icon                
        psz = za.szIconPath;
        if (*psz)
        {
            // search for the '#'
            while ((psz[0] != WIDETEXT('#')) && (psz[0] != WIDETEXT('\0')))
                psz++;
            
            // if we found it, then we have the foo.dll#00001200 format
            if (psz[0] == WIDETEXT('#'))
            {
                psz[0] = WIDETEXT('\0');
#ifdef UNICODE
                StrCpyN(szIconPath, za.szIconPath, ARRAYSIZE(szIconPath));
#else               
                WideCharToMultiByte(CP_ACP, 0, za.szIconPath, -1, szIconPath, ARRAYSIZE(szIconPath), NULL, NULL);
#endif // UNICODE
                iIcon = (WORD)StrToIntW(psz+1);
#ifdef UNICODE
                CHAR szPath[MAX_PATH];
                SHUnicodeToAnsi(szIconPath, szPath, ARRAYSIZE(szPath));
                ExtractIconExA(szPath,(UINT)(-1*iIcon), &hiconLarge, &hiconSmall, 1);
#else
                ExtractIconEx(szIconPath,(UINT)(-1*iIcon), &hiconLarge, &hiconSmall, 1);
#endif
            }
            else
            {
                hiconLarge = (HICON)ExtractAssociatedIcon(ghInstance, szIconPath, (LPWORD)&iIcon);
            }
        }
        // no icons?!  well, just use the generic icon
        if (!hiconSmall && !hiconLarge)
        {
            hiconLarge = LoadIcon(ghInstance, MAKEINTRESOURCE(IDI_ZONE));
            if(! hiconLarge)
            {
                LocalFree((HLOCAL)pszs);
                return S_FALSE;  // no icon found for this zone, not even the generic one
            }
        }
        // we want to save the Large icon if possible for use in the subdialogs
        pszs->hicon = hiconLarge ? hiconLarge : hiconSmall;

        


        // Find the proper index for the zone in the listbox (there is a user-preferred order)
        iSpot = ZoneIndexToGuiIndex(dwIndex);
        if(iSpot == -1)
        {
            // if not a recognized zone, add it to the end of the list
            iSpot = pSec->dwZoneCount - 1;
        }
        // Make sure there are no collisisons
        while(iSpot >= 0 && pfSpotTaken[iSpot] == TRUE)
        {
            iSpot--;
        }
        // Don't go past beginning of array
        if(iSpot < 0)
        {
            // It can be proven that it is impossible to get here, unless there is
            // something wrong with the function ZoneIndexToGuiIndex
            ASSERT(FALSE);
            LocalFree((HLOCAL)pszs);
            if(hiconSmall)
                DestroyIcon(hiconSmall);
            if(hiconLarge)
                DestroyIcon(hiconLarge);
            return E_FAIL;
        }
        plvItem = &(plviZones[iSpot]);
        pfSpotTaken[iSpot] = TRUE;


        // init the List Box item and save it for later addition
        plvItem->mask           = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
        plvItem->iItem          = iSpot;
        plvItem->iSubItem       = 0;
        // large icons prefered for the icon view (if switch back to report view, prefer small icons)
        plvItem->iImage         = ImageList_AddIcon(pSec->himl, hiconLarge ? hiconLarge : hiconSmall);

        plvItem->pszText        = new TCHAR[ARRAYSIZE(za.szDisplayName)+1];
        if(! plvItem->pszText)
        {
            LocalFree((HLOCAL)pszs);
            if(hiconSmall)
                DestroyIcon(hiconSmall);   
            if(hiconLarge)
                DestroyIcon(hiconLarge);
            return E_OUTOFMEMORY;
        }

#ifdef UNICODE 
        StrCpyW(plvItem->pszText,za.szDisplayName);
#else    
        CHAR szDisplayName[MAX_ZONE_PATH];
        WideCharToMultiByte(CP_ACP, 0, za.szDisplayName, -1, szDisplayName, ARRAYSIZE(szDisplayName), NULL, NULL);
        lstrcpy(plvItem->pszText,szDisplayName);
#endif // UNICODE

        plvItem->lParam         = (LPARAM)pszs;       // save the zone settings here

        

        // if we created a small icon, destroy it, since the system does not save the handle
        // when it is added to the imagelist (see ImageList_AddIcon in VC help)
        // Keep it around if we had to use it in place of the large icon
        if (hiconSmall && hiconLarge)
            DestroyIcon(hiconSmall);   

        return S_OK;
}

// Find the current zone from, in order of preference,
// Current URL
// Parameter passed in through dwZone
// Default of internet
void SecurityFindCurrentZone(LPSECURITYPAGE pSec, SECURITYINITFLAGS * psif)
{
    INT_PTR iItem;
    DWORD dwZone=0;
    HRESULT hr = E_FAIL;

    // Check for zone selection in psif
    if(psif)
    {
        dwZone = psif->dwZone;
        hr = S_OK;
    }

    // check for current url, and if found, make it's zone the current (overwriting any request from
    // psif)
    if (g_szCurrentURL[0] && (pSec->pInternetSecurityManager != NULL))
    {
        LPWSTR pwsz;

#ifndef UNICODE
        WCHAR wszCurrentURL[MAX_URL_STRING];
        MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, g_szCurrentURL, -1, wszCurrentURL, ARRAYSIZE(wszCurrentURL));
        pwsz = wszCurrentURL;
#else
        pwsz = g_szCurrentURL;
#endif
        hr = pSec->pInternetSecurityManager->MapUrlToZone(pwsz, (LPDWORD)&dwZone, 0);        
    }
    

    // If there is an active zone, then dwZone now holds the zone's identifier
    // if there is no active zone, check to see if there was a zone requested in dwZone
    iItem = -1;
    if (SUCCEEDED(hr)) // then we have a zone to display
    {
        ZONEATTRIBUTES za = {0};
        LPTSTR pszText;
        LV_FINDINFO lvfiName;
       
        za.cbSize = (ULONG) sizeof(ZONEATTRIBUTES);
        if(pSec->pInternetZoneManager->GetZoneAttributes(dwZone, &za) != E_FAIL)
        {
#ifdef UNICODE    
            pszText        = za.szDisplayName;
#else    
            CHAR szDisplayName[MAX_ZONE_PATH];
            WideCharToMultiByte(CP_ACP, 0, za.szDisplayName, -1, szDisplayName, ARRAYSIZE(szDisplayName), NULL, NULL);
            pszText        = szDisplayName;
#endif // UNICODE

            // Create a find info structure to find the index of the Zone
            lvfiName.flags = LVFI_STRING;
            lvfiName.psz = pszText;
            iItem = SendMessage(pSec->hwndZones, LVM_FINDITEM, (WPARAM)-1, (LPARAM)&lvfiName);
        }
    }

    if (iItem < 0)
    {
        iItem = 0;
        // 0 is the the index (in the listbox) of the "Internet" zone, which we want to come up by default
    }
    // Sundown: typecast OK since zone values restricted
    pSec->iZoneSel = (int) iItem;
}

// To make the slider control accessbile we have to subclass it and over-ride 
// the accessiblity object 

void SecurityInitSlider(LPSECURITYPAGE pSec)
{
    HWND hwndSlider = GetDlgItem(pSec->hDlg, IDC_SLIDER);
    ASSERT(hwndSlider != NULL);

    // Sub-class the control
    BOOL fSucceeded = SetWindowSubclass(hwndSlider, SliderSubWndProc, 0, NULL);

    // Shouldn't fail normally. If we fail we will just fall through and use the
    // base slider control.
    ASSERT(fSucceeded);

    // Initialize the slider control (set number of levels, and frequency one tick per level)
    SendDlgItemMessage(pSec->hDlg, IDC_SLIDER, TBM_SETRANGE, (WPARAM) (BOOL) FALSE, (LPARAM) MAKELONG(0, NUM_TEMPLATE_LEVELS - 1));
    SendDlgItemMessage(pSec->hDlg, IDC_SLIDER, TBM_SETTICFREQ, (WPARAM) 1, (LPARAM) 0);
}
                    
void SecurityInitControls(LPSECURITYPAGE pSec)
{
    LV_COLUMN lvCasey;
    LV_ITEM lvItem;

    // select the item in the listbox
    lvItem.mask = LVIF_STATE;
    lvItem.stateMask = LVIS_SELECTED;
    lvItem.state = LVIS_SELECTED;
    SendMessage(pSec->hwndZones, LVM_SETITEMSTATE, (WPARAM)pSec->iZoneSel, (LPARAM)&lvItem);
    


    // get the zone settings for the selected item
    lvItem.mask  = LVIF_PARAM;
    lvItem.iItem = pSec->iZoneSel;
    lvItem.iSubItem = 0;
    SendMessage(pSec->hwndZones, LVM_GETITEM, (WPARAM)0, (LPARAM)&lvItem);
    pSec->pszs = (LPSECURITYZONESETTINGS)lvItem.lParam;


    // Initialize the local strings to carry the Level Descriptions
    MLLoadString(IDS_TEMPLATE_DESC_HI, LEVEL_DESCRIPTION0, ARRAYSIZE(LEVEL_DESCRIPTION0));
    MLLoadString(IDS_TEMPLATE_DESC_MED, LEVEL_DESCRIPTION1, ARRAYSIZE(LEVEL_DESCRIPTION1));
    MLLoadString(IDS_TEMPLATE_DESC_MEDLOW, LEVEL_DESCRIPTION2, ARRAYSIZE(LEVEL_DESCRIPTION2));
    MLLoadString(IDS_TEMPLATE_DESC_LOW, LEVEL_DESCRIPTION3, ARRAYSIZE(LEVEL_DESCRIPTION3));
    MLLoadString(IDS_TEMPLATE_DESC_CUSTOM, CUSTOM_DESCRIPTION, ARRAYSIZE(CUSTOM_DESCRIPTION));

    MLLoadString(IDS_TEMPLATE_NAME_HI, LEVEL_NAME0, ARRAYSIZE(LEVEL_NAME0));
    MLLoadString(IDS_TEMPLATE_NAME_MED, LEVEL_NAME1, ARRAYSIZE(LEVEL_NAME1));
    MLLoadString(IDS_TEMPLATE_NAME_MEDLOW, LEVEL_NAME2, ARRAYSIZE(LEVEL_NAME2));
    MLLoadString(IDS_TEMPLATE_NAME_LOW, LEVEL_NAME3, ARRAYSIZE(LEVEL_NAME3));
    MLLoadString(IDS_TEMPLATE_NAME_CUSTOM, CUSTOM_NAME, ARRAYSIZE(CUSTOM_NAME));

    // Initialize text boxes and icons for the current zone
    SetDlgItemText(pSec->hDlg, IDC_ZONE_DESCRIPTION, pSec->pszs->szDescription);
    SetDlgItemText(pSec->hDlg, IDC_ZONELABEL, pSec->pszs->szDisplayName);
    SendDlgItemMessage(pSec->hDlg, IDC_ZONE_ICON, STM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)pSec->pszs->hicon);

    // Initialize the slider control
    SecurityInitSlider(pSec);

    // Initialize the list view (add column 0 for icon and text, and autosize it)
    lvCasey.mask = 0;
    SendDlgItemMessage(pSec->hDlg, IDC_LIST_ZONE, LVM_INSERTCOLUMN, (WPARAM) 0, (LPARAM) &lvCasey);
    SendDlgItemMessage(pSec->hDlg, IDC_LIST_ZONE, LVM_SETCOLUMNWIDTH, (WPARAM) 0, (LPARAM) MAKELPARAM(LVSCW_AUTOSIZE, 0));

    // Set the font of the name to the bold font
    pSec->hfontBolded = NULL;
    HFONT hfontOrig = (HFONT) SendDlgItemMessage(pSec->hDlg, IDC_STATIC_EMPTY, WM_GETFONT, (WPARAM) 0, (LPARAM) 0);
    if(hfontOrig == NULL)
        hfontOrig = (HFONT) GetStockObject(SYSTEM_FONT);

    // set the zone name and level font to bolded
    if(hfontOrig)
    {
        LOGFONT lfData;
        if(GetObject(hfontOrig, SIZEOF(lfData), &lfData) != 0)
        {
            // The distance from 400 (normal) to 700 (bold)
            lfData.lfWeight += 300;
            if(lfData.lfWeight > 1000)
                lfData.lfWeight = 1000;
            pSec->hfontBolded = CreateFontIndirect(&lfData);
            if(pSec->hfontBolded)
            {
                // the zone level and zone name text boxes should have the same font, so this is okat
                SendDlgItemMessage(pSec->hDlg, IDC_ZONELABEL, WM_SETFONT, (WPARAM) pSec->hfontBolded, (LPARAM) MAKELPARAM(FALSE, 0));
                SendDlgItemMessage(pSec->hDlg, IDC_LEVEL_NAME, WM_SETFONT, (WPARAM) pSec->hfontBolded, (LPARAM) MAKELPARAM(FALSE, 0));

            }
        }
    }

/*
    {
        // calculate the postions of the static text boxes for the "The current level is:" "<bold>(Level)</bold>" message
        TCHAR * pszText = NULL;
        LONG lLength = 30;
        HDC hdc = NULL;
        SIZE size;
        RECT rect;
        LONG lNameLeftPos = 0;

        // Get the text from the "The current level is" box.
        lLength = SendDlgItemMessage(pSec->hDlg, IDC_SEC_STATIC_CURRENT_LEVEL, WM_GETTEXTLENGTH, 
                                     (WPARAM) 0, (LPARAM) 0);
        pszText = new TCHAR[lLength + 1];
        if(!pszText)
            goto Exit; // E_OUTOFMEMORY
        SendDlgItemMessage(pSec->hDlg, IDC_SEC_STATIC_CURRENT_LEVEL, WM_GETTEXT, (WPARAM) lLength, 
                           (LPARAM) pszText);

        // get the device context
        hdc = GetDC(GetDlgItem(pSec->hDlg, IDC_SEC_STATIC_CURRENT_LEVEL));
        if(! hdc)
            goto Exit;
        // get the length of the text from the device context; assumes the proper font is already in
        if(GetTextExtentPoint32(hdc, pszText, lLength, &size) == 0)
            goto Exit;

        // set the width of the "The current level is" box
        GetClientRect(GetDlgItem(pSec->hDlg, IDC_SEC_STATIC_CURRENT_LEVEL), &rect);
        rect.right = rect.left + size.cx;
        lNameLeftPos = rect.right;
        if(MoveWindow(GetDlgItem(pSec->hDlg, IDC_SEC_STATIC_CURRENT_LEVEL), rect.left, rect.top, 
                      rect.right - rect.left, rect.top - rect.bottom, FALSE) == 0)
            goto Exit;

        // set the x position of the level name box
        GetClientRect(GetDlgItem(pSec->hDlg, IDC_LEVEL_NAME), &rect);
        rect.left = lNameLeftPos;
        if(MoveWindow(GetDlgItem(pSec->hDlg, IDC_LEVEL_NAME), rect.left, 
                      rect.top, rect.right - rect.left, rect.top - rect.bottom, FALSE) == 0)
            goto Exit;

Exit:
        if(hdc)
            ReleaseDC(GetDlgItem(pSec->hDlg, IDC_SEC_STATIC_CURRENT_LEVEL), hdc);
        if(pszText)
            delete pszText;
    }
    */
}


//
// SecurityDlgInit()
//
// Does initalization for Security Dlg.
//
// History:
//
// 6/17/96  t-gpease   remove 'gPrefs', cleaned up code
// 6/20/96  t-gpease   UI changes
// 5/14/97  t-ashlm    UI changes 
//
// 7/02/97  t-mattp    UI changes (slider, listbox)
//
// hDlg is the handle to the SecurityDialog window
// psif holds initialization parameters.  In the case of our entry point
//      from shdocvw (ie, double click browser zone icon, view-internetoptions-security, or right click
//      on desktop icon), it can be NULL

BOOL SecurityDlgInit(HWND hDlg, SECURITYINITFLAGS * psif)
{
    LPSECURITYPAGE  pSec = NULL;
    UINT iIndex = 0;
    HRESULT hr = 0;
    DWORD dwZoneEnumerator;
    
    // Initialize globals variables (to be destroyed at WM_DESTROY)
    if(SecurityInitGlobals(&pSec, hDlg, psif) == FALSE)
    {
        EndDialog(hDlg, 0);
        return FALSE;  // Initialization failed
    }

    // Get a (local) enumerator for the zones
    if (FAILED(pSec->pInternetZoneManager->
                     CreateZoneEnumerator(&dwZoneEnumerator, &(pSec->dwZoneCount), 0)))
    {
        EndDialog(hDlg, 0);
        return FALSE;  // no zone enumerator?
    }


    // Set up the variables in pSec about whether the zone settings can be editted
    SecuritySetEdit(pSec);

         
    // Add the Listbox items for the zones


    // The zones have to be added in a particular order
    // Array used to order zones for adding
    LV_ITEM * plviZones = new LV_ITEM[pSec->dwZoneCount];
    BOOL * pfSpotTaken = new BOOL[pSec->dwZoneCount];
    for(iIndex =0; iIndex < pSec->dwZoneCount; iIndex++)
        pfSpotTaken[iIndex] = FALSE;

    // propogate zone dropdown
    for (DWORD dwIndex=0; dwIndex < pSec->dwZoneCount; dwIndex++)
    {
        if(FAILED(SecurityInitZone(dwIndex, pSec, dwZoneEnumerator, plviZones, pfSpotTaken)))
        {
            // Delete all memory allocated for any previous zones (which have not yet been added to
            // the listbox)
            for(iIndex = 0; iIndex < pSec->dwZoneCount; iIndex++)
            {
                if(pfSpotTaken[iIndex] && (LPSECURITYZONESETTINGS) (plviZones[iIndex].lParam) != NULL)
                {
                    LocalFree((LPSECURITYZONESETTINGS) (plviZones[iIndex].lParam));
                    plviZones[iIndex].lParam = NULL;
                    if(plviZones[iIndex].pszText)
                        delete [] plviZones[iIndex].pszText;
                }
            }
            delete [] plviZones;
            delete [] pfSpotTaken;
            pSec->pInternetZoneManager->DestroyZoneEnumerator(dwZoneEnumerator);
            EndDialog(hDlg, 0);
            return FALSE;
        }
    }
    pSec->pInternetZoneManager->DestroyZoneEnumerator(dwZoneEnumerator);


    // Add all of the arrayed listitems to the listbox
    for(iIndex = 0; iIndex < pSec->dwZoneCount; iIndex++)
    {
        if(pfSpotTaken[iIndex])
        {
            SendMessage(pSec->hwndZones, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&(plviZones[iIndex]));
            delete [] plviZones[iIndex].pszText;
        }
    }
    delete [] plviZones;
    delete [] pfSpotTaken;



    SecurityFindCurrentZone(pSec, psif);
    SecurityInitControls(pSec);
    SecurityEnableControls(pSec, FALSE);
    return TRUE;
}

void SecurityChanged()
{
    TCHAR szClassName[32];
    HWND hwnd = GetTopWindow(GetDesktopWindow());

    //
    // BUGBUG: These should be gotten from some place that is public
    //         to both MSHTML and INETCPL.
    //
    while (hwnd) {
        GetClassName(hwnd, szClassName, ARRAYSIZE(szClassName));

        // notify all "browser" windows that security has changed            
        if (!StrCmpI(szClassName, TEXT("ExploreWClass"))            ||
            !StrCmpI(szClassName, TEXT("IEFrame"))                  ||
            !StrCmpI(szClassName, TEXT("CabinetWClass")))
        {
            // yes...  post it a message..
            PostMessage(hwnd, CWM_GLOBALSTATECHANGE, CWMF_SECURITY, 0L );
        }

        hwnd = GetNextWindow(hwnd, GW_HWNDNEXT);
    }
}

int SecurityWarning(LPSECURITYPAGE pSec)
{
    TCHAR szWarning[64];

    TCHAR szBuf[512];
    TCHAR szMessage[512];
    TCHAR szLevel[64];

    // Load "Warning!"
    MLLoadShellLangString(IDS_WARNING, szWarning, ARRAYSIZE(szWarning));

    // Load "It is not recommended...."
    MLLoadShellLangString(IDS_SECURITY_WARNING, szBuf, ARRAYSIZE(szBuf));

    // Load level: "High, Medium, Medium Low, Low"
    if (pSec->pszs->dwMinSecLevel == URLTEMPLATE_HIGH)
        MLLoadShellLangString(IDS_TEMPLATE_NAME_HI, szLevel, ARRAYSIZE(szLevel));
    else if (pSec->pszs->dwMinSecLevel == URLTEMPLATE_MEDIUM)
        MLLoadShellLangString(IDS_TEMPLATE_NAME_MED, szLevel, ARRAYSIZE(szLevel));
    else if (pSec->pszs->dwMinSecLevel == URLTEMPLATE_MEDLOW)
        MLLoadShellLangString(IDS_TEMPLATE_NAME_MEDLOW, szLevel, ARRAYSIZE(szLevel));
    else
        MLLoadShellLangString(IDS_TEMPLATE_NAME_LOW, szLevel, ARRAYSIZE(szLevel));

    wnsprintf(szMessage, ARRAYSIZE(szMessage), szBuf, szLevel);

    return MessageBox(pSec->hDlg,szMessage,szWarning, MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2);
}

int RegWriteWarning(HWND hParent)
{
    TCHAR szWarning[64];
    TCHAR szWriteWarning[128];

    // load "Warning!"
    MLLoadShellLangString(IDS_WARNING, szWarning, ARRAYSIZE(szWarning));
    // Load "You are about to write..."
    MLLoadShellLangString(IDS_WRITE_WARNING, szWriteWarning, ARRAYSIZE(szWriteWarning));

    return MessageBox(hParent,szWriteWarning, szWarning, MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2);
}


BOOL SecurityEnableControls(LPSECURITYPAGE pSec, BOOL fSetFocus)
// Duties:
// Make the controls (slider, en/disabled buttons) match the data for the current zone
// Make the views (Level description text) match the data for the current zone
// Set focus (to slider, if enabled, else custom settings button, if enabled, else 
//     listbox) if fSetFocus is TRUE
// Note: the zone descriptions are not set here; those are handled by the code responsible
//       for changing zones
{
    int iLevel = -1;


    if (pSec && pSec->pszs)
    {
        HWND hwndSlider = GetDlgItem(pSec->hDlg, IDC_SLIDER);
        
        iLevel = SecLevelToSliderPos(pSec->pszs->dwSecLevel);
        ASSERT(iLevel > -2);

        // Set the level of the slider to the setting for the current zone
        // Show or hide the slider for preset levels/custom
        // Set the level description text
        if(iLevel >= 0)
        {
            SendMessage(hwndSlider, TBM_SETPOS, (WPARAM) (BOOL) TRUE, (LPARAM) (LONG) iLevel);
            // Make sure the slider is visible
            ShowWindow(hwndSlider, SW_SHOW);
            ShowWindow(GetDlgItem(pSec->hDlg, IDC_STATIC_SLIDERMOVETEXT), SW_SHOW);
            SetDlgItemText(pSec->hDlg, IDC_LEVEL_DESCRIPTION, LEVEL_DESCRIPTION[iLevel]);         
            SetDlgItemText(pSec->hDlg, IDC_LEVEL_NAME, LEVEL_NAME[iLevel]);
        }
        else
        {
            // Hide the slider for custom
            ShowWindow(hwndSlider, SW_HIDE);
            ShowWindow(GetDlgItem(pSec->hDlg, IDC_STATIC_SLIDERMOVETEXT), SW_HIDE);
            SetDlgItemText(pSec->hDlg, IDC_LEVEL_DESCRIPTION, CUSTOM_DESCRIPTION);
            SetDlgItemText(pSec->hDlg, IDC_LEVEL_NAME, CUSTOM_NAME);
        }

        // If the zone is empty, show the "zone is empty" string
        // Default is to not show the sting (if something goes wrong)
        // Empty zone not possible for internet, intranet, or local zones
        if((pSec->pszs->dwZoneIndex != URLZONE_INTRANET && 
            pSec->pszs->dwZoneIndex != URLZONE_INTERNET) &&
            pSec->pszs->dwZoneIndex != URLZONE_LOCAL_MACHINE &&
            (pSec->pInternetSecurityManager != NULL))
        {
            IEnumString * piesZones = NULL;
            LPOLESTR ppszDummy[1];
            pSec->pInternetSecurityManager->GetZoneMappings(pSec->pszs->dwZoneIndex, &piesZones, 0);

            // If enumerator can not get 1 item, zone is empty (not valid for internet and intranet)
            if(piesZones && (piesZones->Next(1, ppszDummy, NULL) == S_FALSE))
            {
                ShowWindow(GetDlgItem(pSec->hDlg, IDC_STATIC_EMPTY), SW_SHOW);
            }
            else
            {
                ShowWindow(GetDlgItem(pSec->hDlg, IDC_STATIC_EMPTY), SW_HIDE);
            }
            if(piesZones)
                piesZones->Release();
        }
        else
        {
            ShowWindow(GetDlgItem(pSec->hDlg, IDC_STATIC_EMPTY), SW_HIDE);
        }

        // If we were told to set focus then move focus to the slider.
        if (fSetFocus)
        {
            if(!pSec->fNoEdit)
            {
               if(iLevel >= 0)
                    SetFocus(hwndSlider);
               else if(pSec->pszs->dwFlags & ZAFLAGS_CUSTOM_EDIT)
                    SetFocus(GetDlgItem(pSec->hDlg, IDC_BUTTON_SETTINGS));
               else
                 SetFocus(GetDlgItem(pSec->hDlg, IDC_LIST_ZONE));
            }
            else // No focus is allowed, set focus to the list box
            {
                SetFocus(GetDlgItem(pSec->hDlg, IDC_LIST_ZONE));
            }

        }
        EnableWindow(hwndSlider, (iLevel >= 0) && !pSec->fNoEdit);
        EnableWindow(GetDlgItem(pSec->hDlg, IDC_ZONE_RESET), 
                     !pSec->fNoEdit && (pSec->pszs->dwSecLevel != pSec->pszs->dwRecSecLevel));
        EnableWindow(GetDlgItem(pSec->hDlg, IDC_BUTTON_SETTINGS), 
                     (pSec->pszs->dwFlags & ZAFLAGS_CUSTOM_EDIT) && !pSec->fNoEdit);
        EnableWindow(GetDlgItem(pSec->hDlg, IDC_BUTTON_ADD_SITES), 
                     (pSec->pszs->dwFlags & ZAFLAGS_ADD_SITES) && !pSec->fDisableAddSites);

        return TRUE;
    }

    return FALSE;
}

void SecuritySetLevel(DWORD dwLevel, LPSECURITYPAGE pSec)
{
    // All calls to this function are requests to change the security
    // level for the current zone
    // dwLevel = requested level template (URLTEMPLATE_???)
    int iPos = SecLevelToSliderPos(dwLevel);
    ASSERT(iPos != -2);
    BOOL bCanceled = FALSE;

    // Do nothing if the requested level is equal to the current level 
    if(dwLevel != pSec->pszs->dwSecLevel)
    {
        // Pop up warning box if under recommended min level and lowering security (custom N/A)
        if((pSec->pszs->dwMinSecLevel > dwLevel) && (pSec->pszs->dwSecLevel > dwLevel)
            && (dwLevel != URLTEMPLATE_CUSTOM))
        {
            if(SecurityWarning(pSec) == IDNO)
            {
                bCanceled = TRUE;
            }
        }                
        if(! bCanceled)
        {
            // Set the level
            pSec->pszs->dwPrevSecLevel = pSec->pszs->dwSecLevel;
            pSec->pszs->dwSecLevel = dwLevel;
            ENABLEAPPLY(pSec->hDlg);

            //Tell apply and ok that settings have been changed
            pSec->fChanged = TRUE;
        }
        // Sync the controls to the new level (or back to the old if cancelled)
        SecurityEnableControls(pSec, TRUE);
    }
    // Record that the change request has been handled
    pSec->fPendingChange = FALSE;
}


//
// SecurityDlgApplyNow()
//
// Retrieves the user's choices in dlg ctls,
//    and saves them through SecurityManager interfaces
// If bSaveAll is true, the data for all zones is saved,
// if false, only the current
// Return value is whether the changes were okayed 
//
BOOL SecurityDlgApplyNow(LPSECURITYPAGE pSec, BOOL bSaveAll)
{
    if (pSec->fChanged)
    {
        for (int iIndex = (int)SendMessage(pSec->hwndZones, LVM_GETITEMCOUNT, 0, 0) - 1;
             iIndex >= 0; iIndex--)
        {
            if(!((bSaveAll) || (iIndex == pSec->iZoneSel)))
                continue;
            LV_ITEM lvItem = {0};
            ZONEATTRIBUTES za = {0};
            LPSECURITYZONESETTINGS pszs;
            
            // get the item settings
            lvItem.mask  = LVIF_PARAM;
            lvItem.iItem = iIndex;
            lvItem.iSubItem = 0;
            if(SendMessage(pSec->hwndZones, LVM_GETITEM, (WPARAM)0, (LPARAM)&lvItem))
            {
                pszs = (LPSECURITYZONESETTINGS)lvItem.lParam;

                za.cbSize = sizeof(ZONEATTRIBUTES);
                pSec->pInternetZoneManager->GetZoneAttributes(pszs->dwZoneIndex, &za);
                za.dwTemplateCurrentLevel = pszs->dwSecLevel;
                pSec->pInternetZoneManager->SetZoneAttributes(pszs->dwZoneIndex, &za);
                // Custom settings are saved on exit from the Custom Settings window
            }
        }
        UpdateAllWindows();
        SecurityChanged();
        pSec->fChanged = FALSE;
    }
    return TRUE;
}


//
// SecurityOnCommand()
//
// Handles Security Dialog's window messages
//
// History:
//
// 6/17/96  t-gpease   created
// 5/14/97  t-ashlm    ui changes
//
void SecurityOnCommand(LPSECURITYPAGE pSec, UINT id, UINT nCmd)
{

    switch (id)
    {
        case IDC_BUTTON_ADD_SITES:
        {
            if (pSec->pszs->dwZoneIndex == URLZONE_INTRANET)
                DialogBoxParam(MLGetHinst(), MAKEINTRESOURCE(IDD_SECURITY_INTRANET), pSec->hDlg,
                               SecurityAddSitesIntranetDlgProc, (LPARAM)pSec);
            else
                DialogBoxParam(MLGetHinst(), MAKEINTRESOURCE(IDD_SECURITY_ADD_SITES), pSec->hDlg,
                               SecurityAddSitesDlgProc, (LPARAM)pSec);
                               
            // Resynch controls (in case the "zone is empty" message needs to be updated)
            SecurityEnableControls(pSec, FALSE);
        }   
        break;

        case IDC_BUTTON_SETTINGS:
        {
            // Note: messages to change the level from preset to custom as a result of this call
            //       are sent by the CustomSettings dialog
            DialogBoxParam(MLGetHinst(), MAKEINTRESOURCE(IDD_SECURITY_CUSTOM_SETTINGS), pSec->hDlg,
                           SecurityCustomSettingsDlgProc, (LPARAM)pSec);
            break;
        }
        case IDC_ZONE_RESET:
            if(!pSec->fPendingChange && pSec->pszs->dwSecLevel != pSec->pszs->dwRecSecLevel)
            {
                pSec->fPendingChange = TRUE;
                PostMessage(pSec->hDlg, WM_APP, (WPARAM) 0, (LPARAM) pSec->pszs->dwRecSecLevel);
            }
            break;
            
        case IDOK:
            SecurityDlgApplyNow(pSec, TRUE);
            EndDialog(pSec->hDlg, IDOK);
            break;
            
        case IDCANCEL:
            EndDialog(pSec->hDlg, IDCANCEL);
            break;
            
        case IDC_SLIDER:
            {
                // Get the current slider position
                // Sundown: forced typecast to int, slider positions are restricted
                int iPos = (int) SendDlgItemMessage(pSec->hDlg, IDC_SLIDER, TBM_GETPOS, (WPARAM) 0, (LPARAM) 0);
                if(nCmd == TB_THUMBTRACK)
                {
                    // on Mouse Move, change the level description only
                    SetDlgItemText(pSec->hDlg, IDC_LEVEL_DESCRIPTION, LEVEL_DESCRIPTION[iPos]);
                    SetDlgItemText(pSec->hDlg, IDC_LEVEL_NAME, LEVEL_NAME[iPos]);
                }
                else
                {
                    // Request that the current zone's security level be set to the corresponding level
                    DWORD_PTR dwLevel = SliderPosToSecLevel(iPos);
                    if(! pSec->fPendingChange)
                    {
                        pSec->fPendingChange = TRUE;
                        PostMessage(pSec->hDlg, WM_APP, (WPARAM) 0, (LPARAM) dwLevel);
                    }
                }
            }
            break;
            
        case IDC_LIST_ZONE:
        {
            // Sundown: coercion to int-- selection is range-restricted
            int iNewSelection = (int) SendMessage(pSec->hwndZones, LVM_GETNEXTITEM, (WPARAM)-1, 
                                                  MAKELPARAM(LVNI_SELECTED, 0));

            if ((iNewSelection != pSec->iZoneSel) && (iNewSelection != -1))
            {
                LV_ITEM lvItem;

                lvItem.iItem = iNewSelection;
                lvItem.iSubItem = 0;
                lvItem.mask  = LVIF_PARAM;                                            
                SendMessage(pSec->hwndZones, LVM_GETITEM, (WPARAM)0, (LPARAM)&lvItem);
                pSec->pszs = (LPSECURITYZONESETTINGS)lvItem.lParam;
                pSec->iZoneSel = iNewSelection;

                SetDlgItemText(pSec->hDlg, IDC_ZONE_DESCRIPTION, pSec->pszs->szDescription);
                SetDlgItemText(pSec->hDlg, IDC_ZONELABEL, pSec->pszs->szDisplayName);
                SendDlgItemMessage(pSec->hDlg, IDC_ZONE_ICON, STM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)pSec->pszs->hicon);
                SecurityEnableControls(pSec, FALSE);
            }    
            break;
        }
    }   

} // SecurityOnCommand()


//
// SecurityDlgProc()
//
// Handles Security Dialog's window messages
//
// History:
//
// 6/17/96  t-gpease   created
// 5/14/97  t-ashlm    ui changes
//
INT_PTR CALLBACK SecurityDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LPSECURITYPAGE pSec;

    if (uMsg == WM_INITDIALOG)
    {
        // A hack forced by PropertyPage:
        // PropertyPage creates this dialog in mainwnd.cpp when the dialog is entered from
        // the desktop e's properties, the browser's menu view-internetoptions-security, or
        // right clicking on the browser's zone icon.
        // In the property page case, lParam (our only route to get initialization information
        // in) is a pointer to a PROPERTYSHEETHEADER, more or less, and of entirely no use to us.
        // However, when called from our exported function LaunchSecurityDialogEx, using
        // CreateDialogParamWrapW, we want to pass useful information in.  The only way to make sure 
        // we our dealing with useful information is to make the passed in pointer be to a 
        // structure we know and love, and hence could not possibly be pointed to by PropertyPage.  
        // We use a ThreadLocalStorage object, as our information reference
        SECURITYINITFLAGS * psif = NULL;
        if(g_dwtlsSecInitFlags != (DWORD) -1)
            psif = (SECURITYINITFLAGS *) TlsGetValue(g_dwtlsSecInitFlags);
        if((SECURITYINITFLAGS *) lParam != psif)
            psif = NULL; 
        return SecurityDlgInit(hDlg, psif);
    }

    pSec = (LPSECURITYPAGE)GetWindowLongPtr(hDlg, DWLP_USER);
    if (!pSec)
        return FALSE;
    
    switch (uMsg)
    {
        case WM_COMMAND:
            SecurityOnCommand(pSec, LOWORD(wParam), HIWORD(wParam));
            return TRUE;

        case WM_NOTIFY:
        {
            NMHDR *lpnm = (NMHDR *) lParam;

            ASSERT(lpnm);

            // List Box Messages
            if(lpnm->idFrom == IDC_LIST_ZONE)
            {
                NM_LISTVIEW * lplvnm = (NM_LISTVIEW *) lParam;
                if(lplvnm->hdr.code == LVN_ITEMCHANGED)
                {
                    // If an item's state has changed, and it is now selected
                    if(((lplvnm->uChanged & LVIF_STATE) != 0) && ((lplvnm->uNewState & LVIS_SELECTED) != 0))
                    {
                        SecurityOnCommand(pSec, IDC_LIST_ZONE, LVN_ITEMCHANGED);
                    }                   
                }
            }
            else
            {
                switch (lpnm->code)
                {
                    case PSN_QUERYCANCEL:
                    case PSN_KILLACTIVE:
                    case PSN_RESET:
                        SetWindowLongPtr(pSec->hDlg, DWLP_MSGRESULT, FALSE);
                        return TRUE;

                    case PSN_APPLY:
                        // Hitting the apply button runs this code
                        SecurityDlgApplyNow(pSec, TRUE);
                        break;
                }
            }
        }
        break;

        case WM_HELP:           // F1
            ResWinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle, IDS_HELPFILE,
                        HELP_WM_HELP, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;

        case WM_APP:
            // A message needs to be posted, because the set tools sometimes send two messages
            // hence we need delayed action and a pending change boolean
            // lParam is the level to set for this message
            // wParam is not used
            SecuritySetLevel((DWORD) lParam, pSec);
            break;
        case WM_VSCROLL:
            // Slider Messages
            SecurityOnCommand(pSec, IDC_SLIDER, LOWORD(wParam));
            return TRUE;

        case WM_CONTEXTMENU:        // right mouse click
            ResWinHelp( (HWND) wParam, IDS_HELPFILE,
                        HELP_CONTEXTMENU, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;

        case WM_DESTROY:
            if(! pSec)
                break;

            if(pSec->hwndZones)
            {
                for (int iIndex = (int)SendMessage(pSec->hwndZones, LVM_GETITEMCOUNT, 0, 0) - 1;
                     iIndex >= 0; iIndex--)
                {
                    LV_ITEM lvItem;

                    // get security zone settings object for this item and release it
                    lvItem.mask = LVIF_PARAM;
                    lvItem.iItem = iIndex;
                    lvItem.iSubItem = 0;
                    if (SendMessage(pSec->hwndZones, LVM_GETITEM, (WPARAM)0, (LPARAM)&lvItem) == TRUE)
                    {
                        LPSECURITYZONESETTINGS pszs = (LPSECURITYZONESETTINGS)lvItem.lParam;
                        if (pszs)
                        {
                            if (pszs->hicon)
                                DestroyIcon(pszs->hicon);
                            LocalFree((HLOCAL)pszs);
                            pszs = NULL;
                        }
                    }                 
                }   
            }

            if(pSec->pInternetZoneManager)
                pSec->pInternetZoneManager->Release();

            if(pSec->pInternetSecurityManager)
                pSec->pInternetSecurityManager->Release();

            if(pSec->himl)
                ImageList_Destroy(pSec->himl);

            if(pSec->hfontBolded)
                DeleteObject(pSec->hfontBolded);

            // ok, we're done with URLMON
            if(pSec->hinstUrlmon)
                FreeLibrary(pSec->hinstUrlmon);
            
            LocalFree(pSec);
            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)NULL);
            break;
    }
    return FALSE;
}

// Subclassed window proc for the slider. This is used to take over the
// accessibility wrapper for the class so we can return the right zone
// string ( i.e. High, Medium, Low, etc). Just trap WM_GETOBJECT and pass
// in our override of the accessibility wrapper. 

LRESULT CALLBACK SliderSubWndProc (HWND hwndSlider, UINT uMsg, WPARAM wParam, LPARAM lParam, WPARAM uID, ULONG_PTR dwRefData)
{
    ASSERT(uID == 0);
    ASSERT(dwRefData == 0);

    switch (uMsg)
    {
        case WM_GETOBJECT:
            if ( lParam == OBJID_CLIENT )
            {       
                // At this point we will try to load oleacc and get the functions
                // we need. 
                if (!g_fAttemptedOleAccLoad)
                {
                    g_fAttemptedOleAccLoad = TRUE;

                    ASSERT(s_pfnCreateStdAccessibleProxy == NULL);
                    ASSERT(s_pfnLresultFromObject == NULL);

                    g_hOleAcc = LoadLibrary(TEXT("OLEACC"));
                    if (g_hOleAcc != NULL)
                    {
        #ifdef UNICODE
                        s_pfnCreateStdAccessibleProxy = (PFNCREATESTDACCESSIBLEPROXY)
                                                    GetProcAddress(g_hOleAcc, "CreateStdAccessibleProxyW");
        #else
                        s_pfnCreateStdAccessibleProxy = (PFNCREATESTDACCESSIBLEPROXY)
                                                    GetProcAddress(g_hOleAcc, "CreateStdAccessibleProxyA");
        #endif
                        s_pfnLresultFromObject = (PFNLRESULTFROMOBJECT)
                                                    GetProcAddress(g_hOleAcc, "LresultFromObject");
                    }
                    if (s_pfnLresultFromObject == NULL || s_pfnCreateStdAccessibleProxy == NULL)
                    {
                        // No point holding on to Oleacc since we can't use it.
                        FreeLibrary(g_hOleAcc);
                        g_hOleAcc = NULL;
                        s_pfnLresultFromObject = NULL;
                        s_pfnCreateStdAccessibleProxy = NULL;
                    }
                }

                
                if (g_hOleAcc && s_pfnCreateStdAccessibleProxy && s_pfnLresultFromObject)
                {
                    IAccessible *pAcc = NULL;
                    HRESULT hr;
                
                    // Create default slider proxy.
                    hr = s_pfnCreateStdAccessibleProxy(
                            hwndSlider,
                            TEXT("msctls_trackbar32"),
                            OBJID_CLIENT,
                            IID_IAccessible,
                            (void **)&pAcc
                            );


                    if (SUCCEEDED(hr) && pAcc)
                    {
                        // now wrap it up in our customized wrapper...
                        IAccessible * pWrapAcc = new CSecurityAccessibleWrapper( hwndSlider, pAcc );
                        // Release our ref to proxy (wrapper has its own addref'd ptr)...
                        pAcc->Release();
                    
                        if (pWrapAcc != NULL)
                        {

                            // ...and return the wrapper via LresultFromObject...
                            LRESULT lr = s_pfnLresultFromObject( IID_IAccessible, wParam, pWrapAcc );
                            // Release our interface pointer - OLEACC has its own addref to the object
                            pWrapAcc->Release();

                            // Return the lresult, which 'contains' a reference to our wrapper object.
                            return lr;
                            // All done!
                        }
                    // If it didn't work, fall through to default behavior instead. 
                    }
                }
            }
            break;

        case WM_DESTROY:
            RemoveWindowSubclass(hwndSlider, SliderSubWndProc, uID);
            break;    

    } /* end switch */

    return DefSubclassProc(hwndSlider, uMsg, wParam, lParam);
}

                        
                          




HRESULT _AddSite(LPADDSITESINFO pasi)
{
    HRESULT hr = S_OK;
    LPWSTR psz;

    SendMessage(pasi->hwndAdd, WM_GETTEXT, MAX_ZONE_PATH, (LPARAM)pasi->szWebSite);
#ifndef UNICODE
    WCHAR wszMapping[MAX_ZONE_PATH];
    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pasi->szWebSite, sizeof(pasi->szWebSite),  wszMapping, ARRAYSIZE(wszMapping));
    psz = wszMapping;
#else
    psz = pasi->szWebSite;
#endif

    if (*psz)
    {


        pasi->fRSVOld = pasi->fRequireServerVerification;
        pasi->fRequireServerVerification = IsDlgButtonChecked(pasi->hDlg, IDC_CHECK_REQUIRE_SERVER_VERIFICATION);                 

        // if the state of RequireServerVer has changed, then do a SetZoneAttr so we'll get the correct error codes
        if (pasi->fRSVOld != pasi->fRequireServerVerification)
        {
            ZONEATTRIBUTES za;
            za.cbSize = sizeof(ZONEATTRIBUTES);
            pasi->pSec->pInternetZoneManager->GetZoneAttributes(pasi->pSec->pszs->dwZoneIndex, &za);
            if (pasi->fRequireServerVerification)
                za.dwFlags |= ZAFLAGS_REQUIRE_VERIFICATION;
            else
                za.dwFlags &= ~ZAFLAGS_REQUIRE_VERIFICATION;
            
            pasi->pSec->pInternetZoneManager->SetZoneAttributes(pasi->pSec->pszs->dwZoneIndex, &za);

        }
        
        hr = pasi->pSec->pInternetSecurityManager->SetZoneMapping(pasi->pSec->pszs->dwZoneIndex,
            psz, SZM_CREATE);

        if (FAILED(hr))
        {
            UINT id = IDS_MAPPINGFAIL;
        
            if (hr == URL_E_INVALID_SYNTAX)
            {
                id = IDS_INVALIDURL;
            }
            else if (hr == E_INVALIDARG)
            {
                id = IDS_INVALIDWILDCARD;
            }
            else if (hr == E_ACCESSDENIED)
            {
                id = IDS_HTTPSREQ;
            }
            else if (hr == HRESULT_FROM_WIN32(ERROR_FILE_EXISTS))
            {
                id = IDS_SITEEXISTS;
            }

            MLShellMessageBox(pasi->hDlg, MAKEINTRESOURCEW(id), NULL, MB_ICONSTOP|MB_OK);
            Edit_SetSel(pasi->hwndAdd, 0, -1);
        }
        else
        {
            SendMessage(pasi->hwndWebSites, LB_ADDSTRING, (WPARAM)0, (LPARAM)pasi->szWebSite);
            SendMessage(pasi->hwndAdd, WM_SETTEXT, (WPARAM)0, (LPARAM)NULL);
            SetFocus(pasi->hwndAdd);
        }
    }
    return hr;
}

INT_PTR CALLBACK SecurityAddSitesDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
    LPADDSITESINFO pasi;

    if (uMsg == WM_INITDIALOG)
    {
        pasi = (LPADDSITESINFO)LocalAlloc(LPTR, sizeof(*pasi));
        if (!pasi)
        {
            EndDialog(hDlg, IDCANCEL);
            return FALSE;
        }

        // tell dialog where to get info
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pasi);

        // save the handle to the page
        pasi->hDlg         = hDlg;
        pasi->pSec         = (LPSECURITYPAGE)lParam;
        pasi->hwndWebSites = GetDlgItem(hDlg, IDC_LIST_WEBSITES);
        pasi->hwndAdd      = GetDlgItem(hDlg, IDC_EDIT_ADD_SITE);

        // cross-lang platform support
        SHSetDefaultDialogFont(hDlg, IDC_EDIT_ADD_SITE);

        // limit the text so it will fit
        SendMessage(pasi->hwndAdd, EM_SETLIMITTEXT, (WPARAM)sizeof(pasi->szWebSite), (LPARAM)0);

        pasi->fRequireServerVerification = pasi->pSec->pszs->dwFlags & ZAFLAGS_REQUIRE_VERIFICATION;
        CheckDlgButton(hDlg, IDC_CHECK_REQUIRE_SERVER_VERIFICATION, pasi->fRequireServerVerification);
        
        // hide the checkbox if it doesn't support server verification
        if (!(pasi->pSec->pszs->dwFlags & ZAFLAGS_SUPPORTS_VERIFICATION))
            ShowWindow(GetDlgItem(hDlg, IDC_CHECK_REQUIRE_SERVER_VERIFICATION), SW_HIDE);

        SendMessage(hDlg, WM_SETTEXT, (WPARAM)0, (LPARAM)pasi->pSec->pszs->szDisplayName);
        SetDlgItemText(hDlg, IDC_ADDSITES_GROUPBOX,(LPTSTR)pasi->pSec->pszs->szDisplayName);
        SendDlgItemMessage(hDlg, IDC_ZONE_ICON, STM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)pasi->pSec->pszs->hicon);
        EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_REMOVE), FALSE);
        
        if (pasi->pSec->pInternetSecurityManager || SUCCEEDED(CoInternetCreateSecurityManager(NULL, &(pasi->pSec->pInternetSecurityManager), 0)))
        {
            IEnumString *pEnum;

            if (SUCCEEDED(pasi->pSec->pInternetSecurityManager->GetZoneMappings(pasi->pSec->pszs->dwZoneIndex, &pEnum, 0)))
            {
                LPOLESTR pszMapping;
#ifndef UNICODE
                CHAR szMapping[MAX_URL_STRING];
#endif
                LPTSTR psz;

                while (pEnum->Next(1, &pszMapping, NULL) == S_OK)
                {
#ifndef UNICODE
                    WideCharToMultiByte(CP_ACP, 0, pszMapping, -1, szMapping, ARRAYSIZE(szMapping), NULL, NULL);
                    psz = szMapping;
#else
                    psz = pszMapping;
#endif // UNICODE                       
                    SendMessage(pasi->hwndWebSites, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)psz);
                    CoTaskMemFree(pszMapping);
                }
                pEnum->Release();
            }
        }
        
        if (pasi->pSec->fNoAddSites || pasi->pSec->fNoZoneMapEdit)
        {
            EnableDlgItem(hDlg, IDC_EDIT_ADD_SITE, FALSE);
            EnableDlgItem(hDlg, IDC_BUTTON_REMOVE, FALSE);            
        }

        if (pasi->pSec->fNoZoneMapEdit)
        {
            EnableDlgItem(hDlg, IDC_CHECK_REQUIRE_SERVER_VERIFICATION, FALSE);
            EnableDlgItem(hDlg, IDS_STATIC_ADDSITE, FALSE);            
        }

        SHAutoComplete(GetDlgItem(hDlg, IDC_EDIT_ADD_SITE), SHACF_DEFAULT);
    }
    
    else
        pasi = (LPADDSITESINFO)GetWindowLongPtr(hDlg, DWLP_USER);

    if (!pasi)
        return FALSE;
    
    switch (uMsg)
    {
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                {
                    ZONEATTRIBUTES za;

                    pasi->fRequireServerVerification = IsDlgButtonChecked(hDlg, IDC_CHECK_REQUIRE_SERVER_VERIFICATION);                 

                    if (pasi->fRequireServerVerification)
                        pasi->pSec->pszs->dwFlags |= ZAFLAGS_REQUIRE_VERIFICATION;
                    else
                        pasi->pSec->pszs->dwFlags &= ~ZAFLAGS_REQUIRE_VERIFICATION;

                    za.cbSize = sizeof(ZONEATTRIBUTES);
                    pasi->pSec->pInternetZoneManager->GetZoneAttributes(pasi->pSec->pszs->dwZoneIndex, &za);
                    za.dwFlags = pasi->pSec->pszs->dwFlags;
                    pasi->pSec->pInternetZoneManager->SetZoneAttributes(pasi->pSec->pszs->dwZoneIndex, &za);
                    if (SUCCEEDED(_AddSite(pasi)))
                    {
                        SecurityChanged();
                        EndDialog(hDlg, IDOK);
                    }
                    break;
                }

                case IDCANCEL:
                {
                    ZONEATTRIBUTES za;

                    // reset to original state of RequireServerVerification on cancel
                    za.cbSize = sizeof(ZONEATTRIBUTES);
                    pasi->pSec->pInternetZoneManager->GetZoneAttributes(pasi->pSec->pszs->dwZoneIndex, &za);
                    za.dwFlags = pasi->pSec->pszs->dwFlags;
                    pasi->pSec->pInternetZoneManager->SetZoneAttributes(pasi->pSec->pszs->dwZoneIndex, &za);

                    EndDialog(hDlg, IDCANCEL);
                    break;
                }
                
                case IDC_LIST_WEBSITES:
                    switch (HIWORD(wParam))
                    {
                        case LBN_SELCHANGE:
                        case LBN_SELCANCEL:
                            if (!pasi->pSec->fNoAddSites && !pasi->pSec->fNoZoneMapEdit)
                                EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_REMOVE), SendDlgItemMessage(hDlg, IDC_LIST_WEBSITES, LB_GETCURSEL, 0, 0) != -1);
                            break;
                    }
                    break;
                            
                case IDC_EDIT_ADD_SITE:
                    switch(HIWORD(wParam))
                    {
                        case EN_CHANGE:
                            BOOL fEnable = GetWindowTextLength(GetDlgItem(hDlg, IDC_EDIT_ADD_SITE)) ? TRUE:FALSE;
                            EnableWindow(GetDlgItem(hDlg,IDC_BUTTON_ADD), fEnable);
                            SendMessage(hDlg, DM_SETDEFID, fEnable ? IDC_BUTTON_ADD : IDOK, 0);
                            break;
                    }   
                    break;

                case IDC_BUTTON_ADD:
                    _AddSite(pasi);
                    break;

                case IDC_BUTTON_REMOVE:
                {
                    TCHAR szMapping[MAX_ZONE_PATH];
                    LPWSTR psz;
                    
                            
                    INT_PTR iSel = SendMessage(pasi->hwndWebSites, LB_GETCURSEL, 0, 0);
                    if (iSel != -1)
                    {
                        SendMessage(pasi->hwndWebSites, LB_GETTEXT, (WPARAM)iSel, (LPARAM)szMapping);
#ifndef UNICODE
                        WCHAR wszMapping[MAX_ZONE_PATH];
                        MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szMapping, sizeof(szMapping),  wszMapping, ARRAYSIZE(wszMapping));
                        psz = wszMapping;
#else
                        psz = szMapping;
#endif
                        SendMessage(pasi->hwndWebSites, LB_DELETESTRING, iSel , 0);
                        SendMessage(pasi->hwndWebSites, LB_SETCURSEL, iSel-1, 0);
                        if (!pasi->pSec->fNoAddSites && !pasi->pSec->fNoZoneMapEdit)
                            EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_REMOVE), SendDlgItemMessage(hDlg, IDC_LIST_WEBSITES, LB_GETCURSEL, 0, 0) != -1);

                        pasi->pSec->pInternetSecurityManager->SetZoneMapping(pasi->pSec->pszs->dwZoneIndex,
                            psz, SZM_DELETE);
                    }

                    break;
                }
                default:
                    return FALSE;
            }
            return TRUE;                
            break;

        case WM_HELP:           // F1
            ResWinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle, IDS_HELPFILE,
                        HELP_WM_HELP, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;

        case WM_CONTEXTMENU:        // right mouse click
            ResWinHelp( (HWND) wParam, IDS_HELPFILE,
                        HELP_CONTEXTMENU, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;

        case WM_DESTROY:
            SHRemoveDefaultDialogFont(hDlg);
            if (pasi)
            {
                LocalFree(pasi);
                SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)NULL);
            }
            break;
    }
    return FALSE;
}

INT_PTR CALLBACK SecurityAddSitesIntranetDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
    LPADDSITESINTRANETINFO pasii;

    if (uMsg == WM_INITDIALOG)
    {
        pasii = (LPADDSITESINTRANETINFO)LocalAlloc(LPTR, sizeof(*pasii));
        if (!pasii)
        {
            EndDialog(hDlg, IDCANCEL);
            return FALSE;
        }

        // tell dialog where to get info
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pasii);

        // save the handle to the page
        pasii->hDlg = hDlg;
        pasii->pSec = (LPSECURITYPAGE)lParam;

        SendMessage(hDlg, WM_SETTEXT, (WPARAM)0, (LPARAM)pasii->pSec->pszs->szDisplayName);
        CheckDlgButton(hDlg, IDC_CHECK_USEINTRANET, pasii->pSec->pszs->dwFlags & ZAFLAGS_INCLUDE_INTRANET_SITES);
        CheckDlgButton(hDlg, IDC_CHECK_PROXY, pasii->pSec->pszs->dwFlags & ZAFLAGS_INCLUDE_PROXY_OVERRIDE);
        CheckDlgButton(hDlg, IDC_CHECK_UNC, pasii->pSec->pszs->dwFlags & ZAFLAGS_UNC_AS_INTRANET);
        SendDlgItemMessage(hDlg, IDC_ZONE_ICON, STM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)pasii->pSec->pszs->hicon);

        if (pasii->pSec->fNoAddSites || pasii->pSec->fNoZoneMapEdit)
        {
            EnableDlgItem(hDlg, IDC_CHECK_USEINTRANET, FALSE);
            EnableDlgItem(hDlg, IDC_CHECK_PROXY, FALSE);
        }

        if (pasii->pSec->fNoZoneMapEdit)
        {
            EnableDlgItem(hDlg, IDC_CHECK_UNC, FALSE);
        }
        return TRUE;
    }

    else
         pasii = (LPADDSITESINTRANETINFO)GetWindowLongPtr(hDlg, DWLP_USER);

    if (!pasii)
        return FALSE;
    
    switch (uMsg) {
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                {
                    ZONEATTRIBUTES za;

                    pasii->fUseIntranet       = IsDlgButtonChecked(hDlg, IDC_CHECK_USEINTRANET);
                    pasii->fUseProxyExclusion = IsDlgButtonChecked(hDlg, IDC_CHECK_PROXY);
                    pasii->fUseUNC            = IsDlgButtonChecked(hDlg, IDC_CHECK_UNC);
                    
                    if (pasii->fUseIntranet)
                        pasii->pSec->pszs->dwFlags |= ZAFLAGS_INCLUDE_INTRANET_SITES;
                    else
                        pasii->pSec->pszs->dwFlags &= ~ZAFLAGS_INCLUDE_INTRANET_SITES;

                    if (pasii->fUseProxyExclusion)
                        pasii->pSec->pszs->dwFlags |= ZAFLAGS_INCLUDE_PROXY_OVERRIDE;
                    else
                        pasii->pSec->pszs->dwFlags &= ~ZAFLAGS_INCLUDE_PROXY_OVERRIDE;

                    if (pasii->fUseUNC)
                        pasii->pSec->pszs->dwFlags |= ZAFLAGS_UNC_AS_INTRANET;
                    else
                        pasii->pSec->pszs->dwFlags &= ~ZAFLAGS_UNC_AS_INTRANET;
                    
                    za.cbSize = sizeof(ZONEATTRIBUTES);
                    pasii->pSec->pInternetZoneManager->GetZoneAttributes(pasii->pSec->pszs->dwZoneIndex, &za);
                    za.dwFlags = pasii->pSec->pszs->dwFlags;
                    pasii->pSec->pInternetZoneManager->SetZoneAttributes(pasii->pSec->pszs->dwZoneIndex, &za);
                    SecurityChanged();
                    EndDialog(hDlg, IDOK);
                    break;
                }
                case IDCANCEL:
                    EndDialog(hDlg, IDCANCEL);
                    break;

                case IDC_INTRANET_ADVANCED:
                    DialogBoxParam(MLGetHinst(), MAKEINTRESOURCE(IDD_SECURITY_ADD_SITES), hDlg,
                                   SecurityAddSitesDlgProc, (LPARAM)pasii->pSec);
                    break;

                default:
                    return FALSE;
            }
            return TRUE;                
            break;

        case WM_HELP:           // F1
            ResWinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle, IDS_HELPFILE,
                        HELP_WM_HELP, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;

        case WM_CONTEXTMENU:        // right mouse click
            ResWinHelp( (HWND) wParam, IDS_HELPFILE,
                        HELP_CONTEXTMENU, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            break;

        case WM_DESTROY:
            if (pasii)
            {
                LocalFree(pasii);
                SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)NULL);
            }
            break;
    }
    return FALSE;
}

VOID ShowJavaZonePermissionsDialog (HWND hdlg, LPCUSTOMSETTINGSINFO pcsi)
{
    HRESULT           hr;
    IJavaZonePermissionEditor *zoneeditor;

    hr = CoCreateInstance(
            CLSID_JavaRuntimeConfiguration,
            NULL,
            CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER | CLSCTX_LOCAL_SERVER,
            IID_IJavaZonePermissionEditor,
            (PVOID*)&zoneeditor
            );

    if (SUCCEEDED(hr))
    {
        hr = zoneeditor->ShowUI(
                hdlg,
                0,
                0,
                pcsi->fUseHKLM ? URLZONEREG_HKLM : URLZONEREG_DEFAULT,
                pcsi->pSec->pszs->dwZoneIndex,
                pcsi->dwJavaPolicy | URLACTION_JAVA_PERMISSIONS,
                pcsi->pSec->pInternetZoneManager
                );

        zoneeditor->Release();
    }
}



void ShowCustom(LPCUSTOMSETTINGSINFO pcsi, HTREEITEM hti)
{
    TV_ITEM        tvi;
    tvi.hItem = hti;
    tvi.mask = TVIF_HANDLE | TVIF_PARAM | TVIF_IMAGE;

    TreeView_GetItem( pcsi->hwndTree, &tvi );

        // If it's not selected don't bother.
    if (tvi.iImage != IDRADIOON)
        return;

    TCHAR szValName[64];
    DWORD cb = SIZEOF(szValName);
    DWORD dwChecked;

    if (RegQueryValueEx((HKEY)tvi.lParam,
                        TEXT("ValueName"),
                        NULL,
                        NULL,
                        (LPBYTE)szValName,
                        &cb) == ERROR_SUCCESS)
    {
        if (!(StrCmp(szValName, TEXT("1C00"))))
        {
            cb = SIZEOF(dwChecked);
            if (RegQueryValueEx((HKEY)tvi.lParam,
                                TEXT("CheckedValue"),
                                NULL,
                                NULL,
                                (LPBYTE)&dwChecked,
                                &cb) == ERROR_SUCCESS)
            {
#ifndef UNIX
                HWND hCtl = GetDlgItem(pcsi->hDlg, IDC_JAVACUSTOM);
                ShowWindow(hCtl,
                           (dwChecked == URLPOLICY_JAVA_CUSTOM) && (tvi.iImage == IDRADIOON) ? SW_SHOWNA : SW_HIDE);
                EnableWindow(hCtl, dwChecked==URLPOLICY_JAVA_CUSTOM ? TRUE : FALSE);
                pcsi->dwJavaPolicy = dwChecked;
#endif
            }
        }
    }
}

void _FindCustomRecursive(
    LPCUSTOMSETTINGSINFO pcsi,
    HTREEITEM htvi
)
{
    HTREEITEM hctvi;    // child
    
    // step through the children
    hctvi = TreeView_GetChild( pcsi->hwndTree, htvi );
    while ( hctvi )
    {
        _FindCustomRecursive(pcsi,hctvi);
        hctvi = TreeView_GetNextSibling( pcsi->hwndTree, hctvi );
    }

    ShowCustom(pcsi, htvi);
}

void _FindCustom(
    LPCUSTOMSETTINGSINFO pcsi
    )
{
    HTREEITEM hti = TreeView_GetRoot( pcsi->hwndTree );
    
    // and walk the list of other roots
    while (hti)
    {
        // recurse through its children
        _FindCustomRecursive(pcsi, hti);

        // get the next root
        hti = TreeView_GetNextSibling(pcsi->hwndTree, hti );
    }
}

BOOL SecurityCustomSettingsInitDialog(HWND hDlg, LPARAM lParam)
{
    LPCUSTOMSETTINGSINFO pcsi = (LPCUSTOMSETTINGSINFO)LocalAlloc(LPTR, sizeof(*pcsi));
    HRESULT hr;
    
    if (!pcsi)
    {
        EndDialog(hDlg, IDCANCEL);
        return FALSE;
    }

    // tell dialog where to get info
    SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pcsi);

    // save the handle to the page
    pcsi->hDlg = hDlg;
    pcsi->pSec = (LPSECURITYPAGE)lParam;


    // save dialog handle
    pcsi->hwndTree = GetDlgItem(pcsi->hDlg, IDC_TREE_SECURITY_SETTINGS);

    CoInitialize(0);
    hr = CoCreateInstance(CLSID_CRegTreeOptions, NULL, CLSCTX_INPROC_SERVER,
                          IID_IRegTreeOptions, (LPVOID *)&(pcsi->pTO));


    DWORD cb = SIZEOF(pcsi->fUseHKLM);
    
    SHGetValue(HKEY_LOCAL_MACHINE,
               REGSTR_PATH_SECURITY_LOCKOUT,
               REGSTR_VAL_HKLM_ONLY,
               NULL,
               &(pcsi->fUseHKLM),
               &cb);

    // if this fails, we'll just use the default of fUseHKLM == 0
               
    if (SUCCEEDED(hr))
    {
        CHAR szZone[32];

        wnsprintfA(szZone, ARRAYSIZE(szZone), "%ld", pcsi->pSec->pszs->dwZoneIndex);

        // use the SOHKLM tree when fUseHKLM==TRUE for IEAK
        hr = pcsi->pTO->InitTree(pcsi->hwndTree, HKEY_LOCAL_MACHINE,
                                 pcsi->fUseHKLM ?
                                 "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\SOIEAK" :
                                 "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\SO",
                                 szZone);
    }
    
    // find the first root and make sure that it is visible
    TreeView_EnsureVisible( pcsi->hwndTree, TreeView_GetRoot( pcsi->hwndTree ) );

    pcsi->hwndCombo = GetDlgItem(hDlg, IDC_COMBO_RESETLEVEL);
    
    SendMessage(pcsi->hwndCombo, CB_INSERTSTRING, (WPARAM)0, (LPARAM)LEVEL_NAME[3]);
    SendMessage(pcsi->hwndCombo, CB_INSERTSTRING, (WPARAM)0, (LPARAM)LEVEL_NAME[2]);
    SendMessage(pcsi->hwndCombo, CB_INSERTSTRING, (WPARAM)0, (LPARAM)LEVEL_NAME[1]);
    SendMessage(pcsi->hwndCombo, CB_INSERTSTRING, (WPARAM)0, (LPARAM)LEVEL_NAME[0]);
    
    switch (pcsi->pSec->pszs->dwRecSecLevel)
    {
        case URLTEMPLATE_LOW:
            pcsi->iLevelSel = 3;
            break;
        case URLTEMPLATE_MEDLOW:
            pcsi->iLevelSel = 2;
            break;
        case URLTEMPLATE_MEDIUM:
            pcsi->iLevelSel = 1;
            break;
        case URLTEMPLATE_HIGH:
            pcsi->iLevelSel = 0;
            break;
        default:
            pcsi->iLevelSel = 0;
            break;
    }

    _FindCustom(pcsi);

    SendMessage(pcsi->hwndCombo, CB_SETCURSEL, (WPARAM)pcsi->iLevelSel, (LPARAM)0);

    if (pcsi->pSec->fNoEdit)
    {
        EnableDlgItem(hDlg, IDC_COMBO_RESETLEVEL, FALSE);
        EnableDlgItem(hDlg, IDC_BUTTON_APPLY, FALSE);
    }
    pcsi->fChanged = FALSE;
    return TRUE;
}

INT_PTR CALLBACK SecurityCustomSettingsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
    LPCUSTOMSETTINGSINFO pcsi;

    if (uMsg == WM_INITDIALOG)
        return SecurityCustomSettingsInitDialog(hDlg, lParam);
    else
        pcsi = (LPCUSTOMSETTINGSINFO)GetWindowLongPtr(hDlg, DWLP_USER);
    
    if (!pcsi)
        return FALSE;
                
    switch (uMsg) {

        case WM_NOTIFY:
        {
            LPNMHDR psn = (LPNMHDR)lParam;
            switch( psn->code )
            {
                case TVN_KEYDOWN:
                {
                    TV_KEYDOWN *pnm = (TV_KEYDOWN*)psn;
                    if (pnm->wVKey == VK_SPACE) {
                        if (!pcsi->pSec->fNoEdit)
                        {
                            HTREEITEM hti = (HTREEITEM)SendMessage(pcsi->hwndTree, TVM_GETNEXTITEM, TVGN_CARET, NULL);
                            pcsi->pTO->ToggleItem(hti);
                            ShowCustom(pcsi, hti);
                            pcsi->fChanged = TRUE;
                            EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_APPLY),TRUE);
                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE); // eat the key
                            return TRUE;
                         }
                    }
                    break;
                }
            
                case NM_CLICK:
                case NM_DBLCLK:
                {   // is this click in our tree?
                    if ( psn->idFrom == IDC_TREE_SECURITY_SETTINGS )
                    {   // yes...
                        TV_HITTESTINFO ht;
                        HTREEITEM hti;

                        if (!pcsi->pSec->fNoEdit)
                        {
                            GetCursorPos( &ht.pt );                         // get where we were hit
                            ScreenToClient( pcsi->hwndTree, &ht.pt );       // translate it to our window

                            // retrieve the item hit
                            hti = TreeView_HitTest( pcsi->hwndTree, &ht);

                            pcsi->pTO->ToggleItem(hti);
                            pcsi->fChanged = TRUE;
                            ShowCustom(pcsi, hti);
                            EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_APPLY),TRUE);
                         }
                    }   
                }
                break;
            }
        }
        
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    if(pcsi->pSec->fPendingChange)
                        break;
                    if(pcsi->fChanged && RegWriteWarning(pcsi->pSec->hDlg) == IDNO)
                        break;
                    // we use send message instead of post because there is no chance of this button
                    // receiving multiple signals at one click, and we need the change level message to be
                    // processed before the apply message below
                    pcsi->pSec->fPendingChange = TRUE;
                    SendMessage(pcsi->pSec->hDlg, WM_APP, (WPARAM) 0, (LPARAM) URLTEMPLATE_CUSTOM);
                    if(pcsi->fChanged)
                    {
                        pcsi->pTO->WalkTree( WALK_TREE_SAVE );
                    }
                    // Saves custom to registry and Handles updateallwindows 
                    // and securitychanged calls

                    // BUGBUG: Force a call to SetZoneAttributes when anything in custom changes.
                    // This forces the security manager to flush any caches it has for that zone. 
                    pcsi->pSec->fChanged = TRUE;

                    SecurityDlgApplyNow(pcsi->pSec, FALSE);
                    EndDialog(hDlg, IDOK);
                    break;

                case IDCANCEL:
                    EndDialog(hDlg, IDCANCEL);
                    break;

                case IDC_COMBO_RESETLEVEL:
                    switch (HIWORD(wParam))
                    {
                        case CBN_SELCHANGE:
                        {
                            // Sundown: coercion to integer since cursor selection is 32b
                            int iNewSelection = (int) SendMessage(pcsi->hwndCombo, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);

                            if (iNewSelection != pcsi->iLevelSel)
                            {
                                pcsi->iLevelSel = iNewSelection;
                                EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_APPLY),TRUE);
                            }
                            break;
                        }
                    }
                    break;

                case IDC_JAVACUSTOM:
                    ShowJavaZonePermissionsDialog(hDlg, pcsi);
                    break;
                    
                case IDC_BUTTON_APPLY:
                {
                    TCHAR szLevel[64];
                    ZONEATTRIBUTES za;
                    
                    if(pcsi->pSec->fPendingChange == TRUE)
                        break;
                    if(RegWriteWarning(hDlg) == IDNO)
                    {
                        break;
                    }
                    pcsi->pSec->fPendingChange = TRUE;

                    SendMessage(pcsi->hwndCombo, WM_GETTEXT, (WPARAM)ARRAYSIZE(szLevel), (LPARAM)szLevel);

                    za.cbSize = sizeof(ZONEATTRIBUTES);
                        
                    pcsi->pSec->pInternetZoneManager->GetZoneAttributes(pcsi->pSec->pszs->dwZoneIndex, &za);
                                
                    if (!StrCmp(szLevel, LEVEL_NAME[3])) 
                        za.dwTemplateCurrentLevel = URLTEMPLATE_LOW;
                    else if (!StrCmp(szLevel, LEVEL_NAME[2]))
                        za.dwTemplateCurrentLevel = URLTEMPLATE_MEDLOW;
                    else if (!StrCmp(szLevel, LEVEL_NAME[1]))
                        za.dwTemplateCurrentLevel = URLTEMPLATE_MEDIUM;
                    else if (!StrCmp(szLevel, LEVEL_NAME[0]))
                        za.dwTemplateCurrentLevel = URLTEMPLATE_HIGH;
                    else
                        za.dwTemplateCurrentLevel = URLTEMPLATE_CUSTOM;

                    pcsi->pSec->pInternetZoneManager->SetZoneAttributes(pcsi->pSec->pszs->dwZoneIndex, &za);

                    pcsi->pTO->WalkTree(WALK_TREE_REFRESH);

                    // find the first root and make sure that it is visible
                    TreeView_EnsureVisible( pcsi->hwndTree, TreeView_GetRoot( pcsi->hwndTree ) );
                    EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_APPLY), FALSE);
                    SendMessage(hDlg, DM_SETDEFID, IDOK, 0);
                    SetFocus(GetDlgItem(hDlg, IDOK));   // since we grayout the reset button, might have keyboard
                                                        // focus, so we should set focus somewhere else
                    _FindCustom(pcsi);

                    // BUG #57358. We tell the Zone Manager to change to [High/Med/Low] level because we want
                    //             the policy values for those, but we don't want it to change the level from
                    //             custom.  So, after it changes the setting from Custom, we change it back.
                    // Save the level as custom

                    // we use send message instead of post because there is no chance of this button
                    // receiving multiple signals at one click, and we need the change level message to be
                    // processed before the apply message below
                    SendMessage(pcsi->pSec->hDlg, WM_APP, (WPARAM) 0, (LPARAM) URLTEMPLATE_CUSTOM);

                    // Saves custom to registry and Handles updateallwindows 
                    // and securitychanged calls

                    // BUGBUG: Force a call to SetZoneAttributes when anything in custom changes.
                    // This forces the security manager to flush any caches it has for that zone. 
                    pcsi->pSec->fChanged = TRUE;

                    SecurityDlgApplyNow(pcsi->pSec, TRUE);

                    pcsi->fChanged = FALSE;
                    break;
                }
                    

                default:
                    return FALSE;
            }
            return TRUE;                
            break;

        case WM_HELP:           // F1
        {
            LPHELPINFO lphelpinfo;
            lphelpinfo = (LPHELPINFO)lParam;

            TV_HITTESTINFO ht;
            HTREEITEM hItem;

            // If this help is invoked through the F1 key.
            if (GetAsyncKeyState(VK_F1) < 0)
            {
                // Yes we need to give help for the currently selected item.
                hItem = TreeView_GetSelection(pcsi->hwndTree);
            }
            else
            {
                // Else we need to give help for the item at current cursor position
                ht.pt =((LPHELPINFO)lParam)->MousePos;
                ScreenToClient(pcsi->hwndTree, &ht.pt); // Translate it to our window
                hItem = TreeView_HitTest(pcsi->hwndTree, &ht);
            }

                        
            if (FAILED(pcsi->pTO->ShowHelp(hItem , HELP_WM_HELP)))
            {
                ResWinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle, IDS_HELPFILE,
                            HELP_WM_HELP, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            }
            break; 

        }
        case WM_CONTEXTMENU:        // right mouse click
        {
            TV_HITTESTINFO ht;

            GetCursorPos( &ht.pt );                         // get where we were hit
            ScreenToClient( pcsi->hwndTree, &ht.pt );       // translate it to our window

            // retrieve the item hit
            if (FAILED(pcsi->pTO->ShowHelp(TreeView_HitTest( pcsi->hwndTree, &ht),HELP_CONTEXTMENU)))
            {           
                ResWinHelp( (HWND) wParam, IDS_HELPFILE,
                            HELP_CONTEXTMENU, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
            }
            break; 
        }
        case WM_DESTROY:
            if (pcsi)
            {
                if (pcsi->pTO)
                {
                    pcsi->pTO->WalkTree( WALK_TREE_DELETE );
                    pcsi->pTO->Release();
                    pcsi->pTO=NULL;
                }
                LocalFree(pcsi);
                SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)NULL);
                CoUninitialize();
            }
            break;
    }
    return FALSE;
}

#ifdef UNIX
extern "C" 
#endif
BOOL LaunchSecurityDialogEx(HWND hDlg, DWORD dwZone, DWORD dwFlags)
{
    INITCOMMONCONTROLSEX icex;
    SECURITYINITFLAGS * psif = NULL;

    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC  = ICC_USEREX_CLASSES|ICC_NATIVEFNTCTL_CLASS;
    InitCommonControlsEx(&icex);

    if(g_dwtlsSecInitFlags != (DWORD) -1)
        psif = (SECURITYINITFLAGS *) TlsGetValue(g_dwtlsSecInitFlags);
    if(psif)
    {
        psif->fForceUI = dwFlags & LSDFLAG_FORCEUI;
        psif->fDisableAddSites = dwFlags & LSDFLAG_NOADDSITES;
        psif->dwZone = dwZone;
    }

    // passing in a NULL psif is okay
    DialogBoxParam(MLGetHinst(), MAKEINTRESOURCE(IDD_SECSTANDALONE), hDlg,
                           SecurityDlgProc, (LPARAM) psif);
    
    return TRUE;
}

// backwards compatability
#ifdef UNIX
extern "C"
#endif
void LaunchSecurityDialog(HWND hDlg, DWORD dwZone)
{
    LaunchSecurityDialogEx(hDlg, dwZone, LSDFLAG_DEFAULT);
}

#ifdef UNIX
extern "C" 
#endif
void LaunchSiteCertDialog(HWND hDlg)
{
    CRYPTUI_CERT_MGR_STRUCT ccm = {0};
    ccm.dwSize = sizeof(ccm);
    ccm.hwndParent = hDlg;
    CryptUIDlgCertMgr(&ccm);
}
