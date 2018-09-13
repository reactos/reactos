//*************************************************************
//
//  Global Variables
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//*************************************************************

#include "uenv.h"


HINSTANCE        g_hDllInstance;
DWORD            g_dwBuildNumber;
NTPRODUCTTYPE    g_ProductType;
HANDLE           g_hGUIModeSetup = NULL;
HANDLE           g_hProfileSetup = NULL;
DWORD            g_dwNumShellFolders;
DWORD            g_dwNumCommonShellFolders;


const TCHAR c_szStarDotStar[] = TEXT("*.*");
const TCHAR c_szSlash[] = TEXT("\\");
const TCHAR c_szDot[] = TEXT(".");
const TCHAR c_szDotDot[] = TEXT("..");
const TCHAR c_szMAN[] = TEXT(".man");
const TCHAR c_szUSR[] = TEXT(".usr");
const TCHAR c_szLog[] = TEXT(".log");
const TCHAR c_szPDS[] = TEXT(".pds");
const TCHAR c_szPDM[] = TEXT(".pdm");
const TCHAR c_szLNK[] = TEXT(".lnk");
const TCHAR c_szBAK[] = TEXT(".bak");
const TCHAR c_szNTUserMan[] = TEXT("ntuser.man");
const TCHAR c_szNTUserDat[] = TEXT("ntuser.dat");
const TCHAR c_szNTUserIni[] = TEXT("ntuser.ini");
const TCHAR c_szRegistryPol[] = TEXT("registry.pol");
const TCHAR c_szNTUserStar[] = TEXT("ntuser.*");
const TCHAR c_szUserStar[] = TEXT("user.*");
const TCHAR c_szSpace[] = TEXT(" ");
const TCHAR c_szDotPif[] = TEXT(".pif");
const TCHAR c_szNULL[] = TEXT("");
const TCHAR c_szCommonGroupsLocation[] = TEXT("Software\\Program Groups");
TCHAR c_szRegistryExtName[64];

//
// Registry Extension guid
//

GUID guidRegistryExt = REGISTRY_EXTENSION_GUID;

//
// Special folders
//

FOLDER_INFO c_ShellFolders[] =
{
//Hidden   Local    Add    New      Within           Folder                 Folder             Folder        
// Dir?     Dir    CSIDl?  NT5?     LocalSettings    Resource ID            Name               Location      

  {TRUE,   FALSE,  TRUE,  FALSE,    FALSE,           IDS_SH_APPDATA,       TEXT("AppData"),           0}, // AppData
  {TRUE,   FALSE,  TRUE,  TRUE,     FALSE,           IDS_SH_COOKIES,       TEXT("Cookies"),           0}, // Cookies
  {FALSE,  FALSE,  TRUE,  FALSE,    FALSE,           IDS_SH_DESKTOP,       TEXT("Desktop"),           0}, // Desktop
  {FALSE,  FALSE,  TRUE,  FALSE,    FALSE,           IDS_SH_FAVORITES,     TEXT("Favorites"),         0}, // Favorites
  {TRUE,   FALSE,  TRUE,  FALSE,    FALSE,           IDS_SH_NETHOOD,       TEXT("NetHood"),           0}, // NetHood
  {FALSE,  FALSE,  TRUE,  FALSE,    FALSE,           IDS_SH_PERSONAL,      TEXT("Personal"),          0}, // My Documents
  {FALSE,  FALSE,  TRUE,  TRUE,     FALSE,           IDS_SH_MYPICTURES,    TEXT("My Pictures"),       0}, // My Pictures
  {TRUE,   FALSE,  TRUE,  FALSE,    FALSE,           IDS_SH_PRINTHOOD,     TEXT("PrintHood"),         0}, // PrintHood
  {TRUE,   FALSE,  TRUE,  FALSE,    FALSE,           IDS_SH_RECENT,        TEXT("Recent"),            0}, // Recent
  {TRUE,   FALSE,  TRUE,  FALSE,    FALSE,           IDS_SH_SENDTO,        TEXT("SendTo"),            0}, // SendTo
  {FALSE,  FALSE,  TRUE,  FALSE,    FALSE,           IDS_SH_STARTMENU,     TEXT("Start Menu"),        0}, // Start Menu
  {TRUE,   FALSE,  TRUE,  TRUE,     FALSE,           IDS_SH_TEMPLATES,     TEXT("Templates"),         0}, // Templates
  {FALSE,  FALSE,  TRUE,  FALSE,    FALSE,           IDS_SH_PROGRAMS,      TEXT("Programs"),          0}, // Programs
  {FALSE,  FALSE,  TRUE,  FALSE,    FALSE,           IDS_SH_STARTUP,       TEXT("Startup"),           0}, // Startup

  {TRUE,   TRUE,   TRUE,  TRUE,     FALSE,           IDS_SH_LOCALSETTINGS, TEXT("Local Settings"),    0}, // Local Settings
  {TRUE,   TRUE,   TRUE,  TRUE,     TRUE,            IDS_SH_LOCALAPPDATA,  TEXT("Local AppData"),     0}, // Local AppData
  {TRUE,   TRUE,   TRUE,  TRUE,     TRUE,            IDS_SH_CACHE,         TEXT("Cache"),             0}, // Temporary Internet Files
  {TRUE,   TRUE,   TRUE,  TRUE,     TRUE,            IDS_SH_HISTORY,       TEXT("History"),           0}, // History
  {FALSE,  TRUE,   FALSE, TRUE,     TRUE,            IDS_SH_TEMP,          TEXT("Temp"),              0}, // Temp
};


FOLDER_INFO c_CommonShellFolders[] =
{
  {FALSE,  TRUE,   TRUE,  FALSE,    FALSE,           IDS_SH_DESKTOP,       TEXT("Common Desktop"),    0}, // Common Desktop
  {FALSE,  TRUE,   TRUE,  FALSE,    FALSE,           IDS_SH_STARTMENU,     TEXT("Common Start Menu"), 0}, // Common Start Menu
  {FALSE,  TRUE,   TRUE,  FALSE,    FALSE,           IDS_SH_PROGRAMS,      TEXT("Common Programs"),   0}, // Common Programs
  {FALSE,  TRUE,   TRUE,  FALSE,    FALSE,           IDS_SH_STARTUP,       TEXT("Common Startup"),    0}, // Common Startup
  {TRUE,   TRUE,   TRUE,  TRUE,     FALSE,           IDS_SH_APPDATA,       TEXT("Common AppData"),    0}, // Common Application Data
  {TRUE,   TRUE,   TRUE,  TRUE,     FALSE,           IDS_SH_TEMPLATES,     TEXT("Common Templates"),  0}, // Common Templates
  {FALSE,  TRUE,   TRUE,  TRUE,     FALSE,           IDS_SH_FAVORITES,     TEXT("Common Favorites"),  0}, // Common Favorites
  {FALSE,  TRUE,   TRUE,  TRUE,     FALSE,           IDS_SH_SHAREDDOCS,    TEXT("Common Documents"),  0}, // Common Documents
};


//
// Function proto-types
//

void InitializeProductType (void);
BOOL DetermineLocalSettingsLocation(LPTSTR szLocalSettings);


//*************************************************************
//
//  PatchLocalSettings()
//
//  Purpose:    Initializes the LocalSettingsFolder correctly
//
//  Parameters: hInstance   -   DLL instance handle
//
//  Return:     void
//
//  Comments:
//
//  History:    Date        Author     Comment
//              10/13/95    ushaji     Created
//
// 
// Comments:
//      Should remove this post NT5 and restructure to take care of the
// NT4 Localisation Problems
//
//*************************************************************

void PatchLocalAppData(HANDLE hToken)
{
    TCHAR szLocalSettingsPath[MAX_PATH];
    TCHAR szLocalAppData[MAX_PATH];
    LPTSTR lpEnd = NULL, lpLocalAppDataFolder;
    HANDLE hTokenOld=NULL;
    HKEY hKeyRoot, hKey;
    DWORD dwIndex;


    if (!ImpersonateUser (hToken, &hTokenOld)) 
        return;


    if (RegOpenCurrentUser(KEY_READ, &hKeyRoot) == ERROR_SUCCESS) {

        if (RegOpenKeyEx (hKeyRoot, USER_SHELL_FOLDERS,
                          0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                          
            if (RegQueryValueEx (hKey, TEXT("Local AppData"), NULL, NULL,
                                 NULL, NULL) == ERROR_SUCCESS) {
                
                RegCloseKey(hKey);
                RegCloseKey(hKeyRoot);
                RevertToUser(&hTokenOld);
                return;
            }
            
            RegCloseKey(hKey);
        }
        
        RegCloseKey(hKeyRoot);
    }
    

    //
    // Impersonate and determine the user's localsettings
    //

    DetermineLocalSettingsLocation(szLocalSettingsPath);

    RevertToUser(&hTokenOld);

    lstrcpy(szLocalAppData, TEXT("%userprofile%"));


    //
    // Set the Local AppData Folder after %userprofile% so that we
    // we can update the global variable below.
    //

    lpEnd = lpLocalAppDataFolder = CheckSlash(szLocalAppData);
    
    lstrcat(szLocalAppData, szLocalSettingsPath);

    lpEnd = CheckSlash(szLocalAppData);

    LoadString(g_hDllInstance, IDS_SH_LOCALAPPDATA, lpEnd, MAX_FOLDER_SIZE);


    //
    // Construct the path and let it be set.
    //

    SetFolderPath(CSIDL_LOCAL_APPDATA | CSIDL_FLAG_DONT_UNEXPAND, hToken, szLocalAppData);    


    //
    // the global variable should be reset by the time it gets used.
    // No Need to reset it here, but let us be safer.
    //


    for (dwIndex = 0; dwIndex < g_dwNumShellFolders; dwIndex++) 
        if (c_ShellFolders[dwIndex].iFolderID == IDS_SH_LOCALAPPDATA) 
            lstrcpy(c_ShellFolders[dwIndex].lpFolderLocation, lpLocalAppDataFolder);

}


//*************************************************************
//
//  InitializeGlobals()
//
//  Purpose:    Initializes all the globals variables
//              at DLL load time.
//
//  Parameters: hInstance   -   DLL instance handle
//
//  Return:     void
//
//  Comments:
//
//  History:    Date        Author     Comment
//              10/13/95    ericflo    Created
//
//*************************************************************

void InitializeGlobals (HINSTANCE hInstance)
{
    OSVERSIONINFO ver;
    DWORD dwIndex, dwSize, dwType;
    HKEY hKey, hKeyRoot;
    TCHAR szTemp[MAX_PATH];
    TCHAR szTemp2[MAX_PATH];
    TCHAR szTemp3[MAX_PATH];
    SECURITY_DESCRIPTOR sd;
    SECURITY_ATTRIBUTES sa;
    LPTSTR lpEnd;


    //
    // Save the instance handle
    //

    g_hDllInstance = hInstance;


    //
    // Save the number of shell folders
    //

    g_dwNumShellFolders = ARRAYSIZE(c_ShellFolders);
    g_dwNumCommonShellFolders = ARRAYSIZE(c_CommonShellFolders);


    //
    // Query the build number
    //

    ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&ver);
    g_dwBuildNumber = (DWORD) LOWORD(ver.dwBuildNumber);


    //
    // Initialize the product type
    //

    InitializeProductType ();


    //
    // Determine if GUI mode setup is running by trying to open a
    // known named mutex.
    //

    if (!g_hGUIModeSetup) {

        g_hGUIModeSetup = OpenMutex (MUTEX_ALL_ACCESS, FALSE, GUIMODE_SETUP_MUTEX);

        if (g_hGUIModeSetup) {
            DebugMsg((DM_VERBOSE, TEXT("InitializeGlobals: GUI mode setup is running in another process.")));
        }
    }


    //
    // Open the user profile setup event.  This event is set to non-signalled
    // anytime the default user profile is being updated.  This blocks
    // LoadUserProfile until the update is finished.
    //

    if (!g_hProfileSetup) {

        InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);

        SetSecurityDescriptorDacl (
                        &sd,
                        TRUE,                           // Dacl present
                        NULL,                           // NULL Dacl
                        FALSE                           // Not defaulted
                        );

        sa.lpSecurityDescriptor = &sd;
        sa.bInheritHandle = FALSE;
        sa.nLength = sizeof(sa);

        g_hProfileSetup = CreateEvent (&sa, TRUE, TRUE, USER_PROFILE_SETUP_EVENT);

        if (!g_hProfileSetup) {
            DebugMsg((DM_WARNING, TEXT("InitializeGlobals: Failed to create profile setup event with %d"), GetLastError()));
        }
    }


    //
    // Now load the directory names that match
    // the special folders
    //

    for (dwIndex = 0; dwIndex < g_dwNumShellFolders; dwIndex++) {
        LoadString(hInstance, c_ShellFolders[dwIndex].iFolderID,
                   c_ShellFolders[dwIndex].lpFolderLocation, MAX_FOLDER_SIZE);
    }

    for (dwIndex = 0; dwIndex < g_dwNumCommonShellFolders; dwIndex++) {
        LoadString(hInstance, c_CommonShellFolders[dwIndex].iFolderID,
                   c_CommonShellFolders[dwIndex].lpFolderLocation, MAX_FOLDER_SIZE);
    }


    //
    // Special case for the Personal / My Documents folder.  NT4 used a folder
    // called "Personal" for document storage.  NT5 renamed this folder to
    // My Documents.  In the upgrade case from NT4 to NT5, if the user already
    // had information in "Personal", that name was preserved (for compatibility
    // reasons) and the My Pictures folder is created inside of Personal.
    // We need to make sure and fix up the My Documents and My Pictures entries
    // in the global array so they have the correct directory names.
    //
    

    if (RegOpenCurrentUser(KEY_READ, &hKeyRoot) == ERROR_SUCCESS) {

        if (RegOpenKeyEx (hKeyRoot, USER_SHELL_FOLDERS,
                          0, KEY_READ, &hKey) == ERROR_SUCCESS) {

            dwSize = sizeof(szTemp3);
            szTemp3[0] = TEXT('\0');
            if (RegQueryValueEx (hKey, TEXT("Personal"), NULL, &dwType,
                                 (LPBYTE) szTemp3, &dwSize) == ERROR_SUCCESS) {

                LoadString (g_hDllInstance, IDS_SH_PERSONAL2, szTemp2, ARRAYSIZE(szTemp2));
                lstrcpy (szTemp, TEXT("%USERPROFILE%\\"));
                lstrcat (szTemp, szTemp2);

                if (lstrcmpi(szTemp, szTemp3) == 0) {

                    LoadString(hInstance, IDS_SH_PERSONAL2,
                               c_ShellFolders[5].lpFolderLocation, MAX_FOLDER_SIZE);

                    LoadString(hInstance, IDS_SH_MYPICTURES2,
                               c_ShellFolders[6].lpFolderLocation, MAX_FOLDER_SIZE);
                }
            }


            //
            // Special Case for Local Settings.
            // Due to localisations LocalSettings can be pointing to different places in nt4 and rc might
            // not be in sync with the current value. Read the LocalSettings value first and then
            // update everything else afterwards.
            //

            dwSize = sizeof(szTemp2);
            *szTemp = *szTemp2 = TEXT('\0');

            
            //
            // Read the value from the registry if it is available
            //
            
            if (RegQueryValueEx (hKey, TEXT("Local Settings"), NULL, &dwType,
                                 (LPBYTE) szTemp2, &dwSize) != ERROR_SUCCESS) {

                //
                // if the value is not present load it from the rc file
                //

                LoadString(hInstance, IDS_SH_LOCALSETTINGS, szTemp, MAX_FOLDER_SIZE);                
                DebugMsg((DM_VERBOSE, TEXT("InitializeGlobals: local settings folder from the rc is %s"), szTemp));    
            }
            else {
                
                //
                // The registry value read from the registry is the full unexpanded path.
                //

                
                if (lstrlen(szTemp2) > lstrlen(TEXT("%userprofile%"))) {

                    lstrcpy(szTemp, szTemp2+(lstrlen(TEXT("%userprofile%"))+1));
                
                    DebugMsg((DM_VERBOSE, TEXT("InitializeGlobals: local settings folder from the reigtry is %s"), szTemp));    
                }
                else {
                    LoadString(hInstance, IDS_SH_LOCALSETTINGS, szTemp, MAX_FOLDER_SIZE);                
                    DebugMsg((DM_VERBOSE, TEXT("InitializeGlobals: local settings folder(2) from the rc is %s"), szTemp));    
                }
            }

        
            lpEnd = CheckSlash(szTemp);

            for (dwIndex = 0; dwIndex < g_dwNumShellFolders; dwIndex++) {


                //
                // Fix up all LocalSettings related shfolders
                //


                if (lstrcmpi(c_ShellFolders[dwIndex].lpFolderName, TEXT("Local Settings")) == 0) {
                    *lpEnd = TEXT('\0');
                    
                    //
                    // Don't copy the final slash
                    //

                    lstrcpyn(c_ShellFolders[dwIndex].lpFolderLocation, szTemp, lstrlen(szTemp));                                        
                }                    


                if (c_ShellFolders[dwIndex].bLocalSettings) {                    
                    LoadString(hInstance, c_ShellFolders[dwIndex].iFolderID,
                               szTemp3, MAX_FOLDER_SIZE);

                    //
                    // Append localsetting value read above to the end of %userprofile% 
                    // before putting on the shell folder itself
                    //

                    lstrcpy(lpEnd, szTemp3);
                    lstrcpy(c_ShellFolders[dwIndex].lpFolderLocation, szTemp);

                    DebugMsg((DM_VERBOSE, TEXT("InitializeGlobals: Shell folder %s is  %s"), c_ShellFolders[dwIndex].lpFolderName, 
                                                                                             c_ShellFolders[dwIndex].lpFolderLocation));    
                    
                }
            }


            RegCloseKey (hKey);
        }

        RegCloseKey (hKeyRoot);
    }


    //
    // Get string version of registry extension guid
    //

    GuidToString( &guidRegistryExt, c_szRegistryExtName );
}

//*************************************************************
//
//  InitializeProductType()
//
//  Purpose:    Determines the current product type and
//              sets the g_ProductType global variable.
//
//  Parameters: void
//
//  Return:     void
//
//  Comments:
//
//  History:    Date        Author     Comment
//              4/08/96     ericflo    Created
//
//*************************************************************

void InitializeProductType (void)
{

#ifdef WINNT

    HKEY hkey;
    LONG lResult;
    TCHAR szProductType[50];
    DWORD dwType, dwSize;


    //
    // Default product type is workstation.
    //

    g_ProductType = PT_WORKSTATION;


    //
    // Query the registry for the product type.
    //

    lResult = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                            TEXT("System\\CurrentControlSet\\Control\\ProductOptions"),
                            0,
                            KEY_READ,
                            &hkey);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("InitializeProductType: Failed to open registry (%d)"), lResult));
        goto Exit;
    }


    dwSize = 50;
    szProductType[0] = TEXT('\0');

    lResult = RegQueryValueEx (hkey,
                               TEXT("ProductType"),
                               NULL,
                               &dwType,
                               (LPBYTE) szProductType,
                               &dwSize);

    RegCloseKey (hkey);

    if (lResult != ERROR_SUCCESS) {
        DebugMsg((DM_WARNING, TEXT("InitializeProductType: Failed to query product type (%d)"), lResult));
        goto Exit;
    }


    //
    // Map the product type string to the enumeration value.
    //

    if (!lstrcmpi (szProductType, TEXT("WinNT"))) {
        g_ProductType = PT_WORKSTATION;

    } else if (!lstrcmpi (szProductType, TEXT("ServerNT"))) {
        g_ProductType = PT_SERVER;

    } else if (!lstrcmpi (szProductType, TEXT("LanmanNT"))) {
        g_ProductType = PT_DC;

    } else {
        DebugMsg((DM_WARNING, TEXT("InitializeProductType: Unknown product type! <%s>"), szProductType));
    }



Exit:
    DebugMsg((DM_VERBOSE, TEXT("InitializeProductType: Product Type: %d"), g_ProductType));


#else   // WINNT

    //
    // Windows only has 1 product type
    //

    g_ProductType = PT_WINDOWS;

#endif

}
