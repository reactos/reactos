#include "stdafx.h"
#pragma hdrstop
//#include "deskstat.h"
//#include "dutil.h"
//#include "resource.h"

#include <mluisupp.h>

#ifdef POSTSPLIT

STDAPI CDeskHtmlProp_RegUnReg(BOOL bReg)
{
    HKEY    hKey;
    TCHAR lpszDeskcomp[MAX_PATH];

    if(bReg)
    {
        DWORD dwDisposition;
        DWORD dwDeskHtmlVersion = 0;
        DWORD dwDeskHtmlMinorVersion = 0;
        DWORD dwType;

        GetRegLocation(lpszDeskcomp, REG_DESKCOMP_COMPONENTS, NULL);

        if(RegCreateKeyEx(HKEY_CURRENT_USER, (LPCTSTR)lpszDeskcomp, 
               0, NULL, 0, KEY_CREATE_SUB_KEY|KEY_QUERY_VALUE, NULL, &hKey, &dwDisposition) == ERROR_SUCCESS)
        {
            //Get the version stamp from the registry
            if(dwDisposition == REG_OPENED_EXISTING_KEY)
            {
                DWORD dwDataLength = SIZEOF(DWORD);
                RegQueryValueEx(hKey, REG_VAL_COMP_VERSION, NULL, &dwType, (LPBYTE)(&dwDeskHtmlVersion), &dwDataLength);
                RegQueryValueEx(hKey, REG_VAL_COMP_MINOR_VERSION, NULL, &dwType, (LPBYTE)(&dwDeskHtmlMinorVersion), &dwDataLength);
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

                    //Add the base components!
#ifdef ENABLE_CHANNELS
                    TCHAR szBuf[MAX_PATH];
#endif

#ifdef WANT_ORANGE_EGG
                    // The "Hey!"
                    TCHAR szFileName[MAX_PATH];

                    GetWindowsDirectory(szFileName, ARRAYSIZE(szFileName));
                    lstrcat(szFileName, COMPON_FILENAME);
                    MLLoadString(IDS_SAMPLE_COMPONENT, szBuf, ARRAYSIZE(szBuf));
                    AddDesktopComponentNoUI(AD_APPLY_SAVE, szFileName, szBuf, COMP_TYPE_HTMLDOC, EGG_LEFT, EGG_TOP, EGG_WIDTH, EGG_HEIGHT, NULL, TRUE);
#endif

#ifdef ENABLE_CHANNELS
                    // Channels ISFBandOC

                    //
                    // Don't display the channel bar on W98 post W98 gold.
                    //

                    if (!IsOS(OS_MEMPHIS) || IsOS(OS_MEMPHIS_GOLD))
                    {
                        MLLoadString(IDS_CHANNEL_BAR, szBuf, ARRAYSIZE(szBuf));
                        int iLeft, iTop;
                        DWORD dwWidth, dwHeight;
                        GetCBarStartPos(&iLeft, &iTop, &dwWidth, &dwHeight);
                        AddDesktopComponentNoUI(AD_APPLY_SAVE, CBAR_SOURCE, szBuf, COMP_TYPE_CONTROL, iLeft, iTop, dwWidth, dwHeight, NULL, !g_fRunningOnNT);
                    }
#endif  // ENABLE_CHANNELS

                    //Set the default wallpaper. This should be the last statement, i.e., an
                    // active desktop COM object should not be created during the installation process after this.
                    SetDefaultWallpaper();

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
                        RegSetValueEx(hKey2, REG_VAL_GENERAL_WALLPAPER, 0, REG_SZ, (LPBYTE)szSafeMode, SIZEOF(TCHAR)*(lstrlen(szSafeMode)+1));
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

                GetRegLocation(lpszDeskcomp, REG_DESKCOMP_GENERAL, NULL);
                RegCreateKeyEx(HKEY_CURRENT_USER, lpszDeskcomp, 
                    0, NULL, 0, KEY_CREATE_SUB_KEY, NULL, &hKey, &dwDisposition);
                RegCloseKey(hKey);
            }
            else
            {
                // The major version numbers match. So check if the minor version numbers 
                // match too!
                if(dwDeskHtmlMinorVersion < CUR_DESKHTML_MINOR_VERSION)
                {
                    // Minor version numbers do not match. So, set the dirty bit to force
                    // the regeneration of desktop.htt later when needed.
                    SetDesktopFlags(COMPONENTS_DIRTY, COMPONENTS_DIRTY);

                    //Update the new Minor version number!
                    if(RegCreateKeyEx(HKEY_CURRENT_USER, (LPCTSTR)lpszDeskcomp, 0, NULL, 0, 
                            (KEY_CREATE_SUB_KEY | KEY_SET_VALUE), NULL, &hKey, &dwDisposition) == ERROR_SUCCESS)
                    {
                        DWORD   dw;
                        
                        dw = CUR_DESKHTML_MINOR_VERSION;
                        RegSetValueEx(hKey, REG_VAL_COMP_MINOR_VERSION, 0, REG_DWORD, (LPBYTE)&dw, SIZEOF(dw));
                        
                        RegCloseKey(hKey);
                    }
                }
            }
        }
    }
    else
    {
        // When shdoc401 is un-installed, we should not nuke the components because the 
        // end-user may be going back to IE4.0X. So, the following lines are commented out.
        // Fix for IE5 Bug #22796
#ifdef NEVER
        SHDeleteKey(HKEY_LOCAL_MACHINE, c_szRegDeskHtmlProp);
        SHDeleteKey(HKEY_CURRENT_USER, REG_DESKCOMP_COMPONENTS_ROOT);
        SHDeleteKey(HKEY_CURRENT_USER, REG_DESKCOMP_GENERAL_ROOT);
        SHDeleteKey(HKEY_CURRENT_USER, REG_DESKCOMP_SAFEMODE);
        SHDeleteKey(HKEY_CURRENT_USER, REG_DESKCOMP_SCHEME);
#endif //NEVER
    }

    return S_OK;
}

#endif
