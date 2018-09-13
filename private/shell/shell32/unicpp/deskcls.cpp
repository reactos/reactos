#include "stdafx.h"
#include <LMCONS.H>     // 97/07/22 vtan: for UNLEN

#pragma hdrstop

#ifdef POSTSPLIT

void CreateMyCurHomeComponent(BOOL fChecked)
{
    //Add the base components!
    TCHAR szBuf[MAX_PATH];
    ISubscriptionMgr * psm;

    // Add a component that points to "about:home"
    LoadString(HINST_THISDLL, IDS_MY_CURRENT_HOMEPAGE, szBuf, ARRAYSIZE(szBuf));

//  98/07/14 vtan #176721: Changed the following to pass default component
//  positions to AddDesktopComponentNoUI so that the restored position may
//  be set to the default component position.

//  int   iTop, iLeft;
//  DWORD dwWidth, dwHeight;
//  GetMyCurHomePageStartPos(&iLeft, &iTop, &dwWidth, &dwHeight);
//  AddDesktopComponentNoUI(AD_APPLY_SAVE, MY_HOMEPAGE_SOURCE, szBuf, COMP_TYPE_WEBSITE, iLeft, iTop, dwWidth, dwHeight, TRUE, IS_SPLIT);
    AddDesktopComponentNoUI(AD_APPLY_SAVE, MY_HOMEPAGE_SOURCE, szBuf, COMP_TYPE_WEBSITE, COMPONENT_DEFAULT_LEFT, COMPONENT_DEFAULT_TOP, COMPONENT_DEFAULT_WIDTH, COMPONENT_DEFAULT_HEIGHT, fChecked, IS_SPLIT);
    if (SUCCEEDED(CoCreateInstance(CLSID_SubscriptionMgr, NULL,
                CLSCTX_INPROC_SERVER, IID_ISubscriptionMgr, (void**)&psm)))
    {
        WCHAR wszName[MAX_PATH];
        //We need to zero init this structure except the cbSize field.
        SUBSCRIPTIONINFO siDefault = {sizeof(SUBSCRIPTIONINFO)};

        SHTCharToUnicode(szBuf, wszName, ARRAYSIZE(wszName));

        //This field is already initialized above.
        //siDefault.cbSize = sizeof(siDefault);
        psm->CreateSubscription(NULL, MY_HOMEPAGE_SOURCEW, wszName, CREATESUBS_NOUI, SUBSTYPE_DESKTOPURL, &siDefault);
        psm->Release();
    }
}

STDAPI CDeskHtmlProp_RegUnReg(BOOL bReg)
{
    HKEY    hKey;
    DWORD   userNameSize;
    TCHAR   lpszDeskcomp[MAX_PATH];
    TCHAR   userName[UNLEN];

//  98/07/22 vtan #202707: Problem: This code gets called for the first time when NT
//  runs. This sets up the default user profile. Anything that is added to this profile
//  is propagated to any current user when upgrading from NT 4.0 to NT 5.0. This
//  causes the DeskHtmlVersion, DeskHtmlMinorVersion and Component\0 to be replaced
//  with the default component. By replacing the version registry entries any old
//  components will not get correctly upgraded by the component reading code. It also
//  destroys the first component.

//  Solution: Prevent a default component being added at setup time by checking the
//  logged on user is "SYSTEM". If the user is anybody other than system then perform
//  the update or addition of the default component.

    userNameSize = sizeof(userName);
    if ((GetUserName(userName, &userNameSize) != 0) && (lstrcmp(userName, TEXT("SYSTEM")) == 0))
        return(S_OK);           // an ungracious exit right here and now!

    if(bReg)
    {
        DWORD dwDisposition;
        DWORD dwDeskHtmlVersion = 0;
        DWORD dwDeskHtmlMinorVersion = 0;
        DWORD dwType;

        GetRegLocation(lpszDeskcomp, SIZECHARS(lpszDeskcomp), REG_DESKCOMP_COMPONENTS, NULL);

        if(RegCreateKeyEx(HKEY_CURRENT_USER, (LPCTSTR)lpszDeskcomp, 
               0, NULL, 0, KEY_CREATE_SUB_KEY|KEY_QUERY_VALUE, NULL, &hKey, &dwDisposition) == ERROR_SUCCESS)
        {
            //Get the version stamp from the registry
            if(dwDisposition == REG_OPENED_EXISTING_KEY)
            {
                DWORD dwDataLength = SIZEOF(DWORD);
                SHQueryValueEx(hKey, REG_VAL_COMP_VERSION, NULL, &dwType, (LPBYTE)(&dwDeskHtmlVersion), &dwDataLength);
                SHQueryValueEx(hKey, REG_VAL_COMP_MINOR_VERSION, NULL, &dwType, (LPBYTE)(&dwDeskHtmlMinorVersion), &dwDataLength);
            }

            //We need to close this key before we delete it
            RegCloseKey(hKey);

            // If this branch is already there, don't set default comp.
            // Note: The differences between IE4_DESKHTML_VERSION and CUR_DESKHTML_VERSION are 
            // automatically taken care of when we read the components. So, we need to check only
            // for very old versions here.
            if (dwDeskHtmlVersion < IE4_DESKHTML_VERSION)
            {
                //Delete the existing components.
                SHDeleteKey(HKEY_CURRENT_USER, (LPCTSTR)lpszDeskcomp);

                // Create the default active desktop configuration
                if(RegCreateKeyEx(HKEY_CURRENT_USER, (LPCTSTR)lpszDeskcomp, 0, NULL, 0, 
                            (KEY_CREATE_SUB_KEY | KEY_SET_VALUE), NULL, &hKey, &dwDisposition) == ERROR_SUCCESS)
                {
                    //We need an initial state
                    DWORD dw;

                    dw = CUR_DESKHTML_VERSION;
                    RegSetValueEx(hKey, REG_VAL_COMP_VERSION, 0, REG_DWORD, (LPBYTE)&dw, SIZEOF(dw));

                    dw = CUR_DESKHTML_MINOR_VERSION;
                    RegSetValueEx(hKey, REG_VAL_COMP_MINOR_VERSION, 0, REG_DWORD, (LPBYTE)&dw, SIZEOF(dw));
                    
                    dw = COMPSETTING_ENABLE;
                    RegSetValueEx(hKey, REG_VAL_COMP_SETTINGS, 0, REG_DWORD, (LPBYTE)&dw, SIZEOF(dw));

                    //Add the home page component
                    CreateMyCurHomeComponent(TRUE);
                    
                    //Set the default wallpaper. This should be the last statement, i.e., an
                    // active desktop COM object should not be created during the installation process after this.
#ifdef WE_WANT_DEFAULT_WALLPAPER
                    //SetDefaultWallpaper();  //As of Feb'99, we do not install a default wallpaper. So, commented out!
#endif // WE_WANT_DEFAULT_WALLPAPER

                    RegCloseKey(hKey);
                }

                // Create the default active desktop safemode configuration
                if(RegCreateKeyEx(HKEY_CURRENT_USER, REG_DESKCOMP_SAFEMODE,
                        0, NULL, 0, KEY_CREATE_SUB_KEY, NULL, &hKey, &dwDisposition) == ERROR_SUCCESS)
                {
                    DWORD dwDisposition;
                    HKEY hKey2;

                    if(RegCreateKeyEx(hKey, REG_DESKCOMP_GENERAL_SUFFIX, 0, NULL, 0, 
                                (KEY_CREATE_SUB_KEY | KEY_SET_VALUE), NULL, &hKey2, &dwDisposition) == ERROR_SUCCESS)
                    {
                        TCHAR szSafeMode[MAX_PATH];
                        GetWindowsDirectory(szSafeMode, ARRAYSIZE(szSafeMode));
                        lstrcat(szSafeMode, DESKTOPHTML_DEFAULT_SAFEMODE);
                        // Show safemode.htx
                        SHRegSetPath(hKey2, NULL, REG_VAL_GENERAL_WALLPAPER, szSafeMode, 0);
                        // Don't bring up the gallery dialog box
                        dwDisposition = 0;
                        RegSetValueEx(hKey2, REG_VAL_GENERAL_VISITGALLERY, 0, REG_DWORD, (LPBYTE)&dwDisposition, sizeof(dwDisposition));
                        RegCloseKey(hKey2);
                    }
                    
                    RegCloseKey(hKey);
                }

                // Create the default scheme key
                if(RegCreateKeyEx(HKEY_CURRENT_USER, REG_DESKCOMP_SCHEME, 0, NULL, 0, 
                        (KEY_CREATE_SUB_KEY | KEY_SET_VALUE), NULL, &hKey, &dwDisposition) == ERROR_SUCCESS)
                {
                    RegSetValueEx(hKey, REG_VAL_SCHEME_EDIT, 0, REG_SZ, (LPBYTE)TEXT(""), SIZEOF(TCHAR));
                    RegSetValueEx(hKey, REG_VAL_SCHEME_DISPLAY, 0, REG_SZ, (LPBYTE)TEXT(""), SIZEOF(TCHAR));
                    RegCloseKey(hKey);
                }

                // Set the components to be dirty sothat we re-generate desktop.htm
                // the first boot after installing IE4.0.
                SetDesktopFlags(COMPONENTS_DIRTY, COMPONENTS_DIRTY);

                GetRegLocation(lpszDeskcomp, SIZECHARS(lpszDeskcomp), REG_DESKCOMP_GENERAL, NULL);
                RegCreateKeyEx(HKEY_CURRENT_USER, lpszDeskcomp, 
                    0, NULL, 0, KEY_CREATE_SUB_KEY, NULL, &hKey, &dwDisposition);
                RegCloseKey(hKey);
            }
            else
            {
                //See if we are upgrading from an older version like IE4.
                if (dwDeskHtmlVersion < CUR_DESKHTML_VERSION)
                {
                    // If so, save the DESKHTML_VERSION we are upgrading from.
                    // We use this later in SHGetSetSettings to decide if active desktop is ON/OFF.
                    // NOTE: The "UpgradedFrom" value is at "...\Desktop" and NOT at "..\Desktop\Components"
                    // This is because the "Components" key gets destroyed very often.
                    SHSetValue(HKEY_CURRENT_USER, REG_DESKCOMP, REG_VAL_COMP_UPGRADED_FROM,
                                REG_DWORD, (LPBYTE)&dwDeskHtmlVersion, SIZEOF(dwDeskHtmlVersion));
                }
                 // The major version numbers match. So check if the minor version numbers 
                // match too!
                if(dwDeskHtmlMinorVersion < CUR_DESKHTML_MINOR_VERSION)
                {
                    //Update the new Minor version number!
                    if(RegCreateKeyEx(HKEY_CURRENT_USER, (LPCTSTR)lpszDeskcomp, 0, NULL, 0, 
                            (KEY_CREATE_SUB_KEY | KEY_SET_VALUE), NULL, &hKey, &dwDisposition) == ERROR_SUCCESS)
                    {
                        DWORD   dw;
                        
                        dw = CUR_DESKHTML_MINOR_VERSION;
                        RegSetValueEx(hKey, REG_VAL_COMP_MINOR_VERSION, 0, REG_DWORD, (LPBYTE)&dw, SIZEOF(dw));
                        
                        RegCloseKey(hKey);
                    }
                    
                    // Add the new home page component
                    if((dwDeskHtmlVersion <= 0x10f) && (dwDeskHtmlMinorVersion <= 0x0001))
                        CreateMyCurHomeComponent(FALSE);

                    // Minor version numbers do not match. So, set the dirty bit to force
                    // the regeneration of desktop.htt later when needed.
                    SetDesktopFlags(COMPONENTS_DIRTY, COMPONENTS_DIRTY);

//  98/07/16 vtan #176721/#202707: Added the following code to delete HKCU\Software\
//  Microsoft\Internet Explorer\Desktop\General\ComponentsPositioned because in NT 4.0
//  with IE 4.0 SP1 this registry entry is incorrectly incremented when the components
//  are iterated rather than when they are positioned. This resets the counter
//  in NT 5.0 where the bug has been fixed.

                    GetRegLocation(lpszDeskcomp, SIZECHARS(lpszDeskcomp), REG_DESKCOMP_GENERAL, NULL);

                    DWORD   dw;

                    if (RegCreateKeyEx(HKEY_CURRENT_USER, (LPCTSTR)lpszDeskcomp, 0, NULL, 0, KEY_READ | KEY_WRITE, NULL, &hKey, &dw) == ERROR_SUCCESS)
                    {
                        (LONG)RegDeleteValue(hKey, REG_VAL_GENERAL_CCOMPPOS);
                        (LONG)RegCloseKey(hKey);
                    }
                }
            }
        }
    }
    else
    {
        SHDeleteKey(HKEY_LOCAL_MACHINE, c_szRegDeskHtmlProp);
        SHDeleteKey(HKEY_CURRENT_USER, REG_DESKCOMP_COMPONENTS_ROOT);
        SHDeleteKey(HKEY_CURRENT_USER, REG_DESKCOMP_GENERAL_ROOT);
        SHDeleteKey(HKEY_CURRENT_USER, REG_DESKCOMP_SAFEMODE);
        SHDeleteKey(HKEY_CURRENT_USER, REG_DESKCOMP_SCHEME);
    }

    return S_OK;
}

#endif

