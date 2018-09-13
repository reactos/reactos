/****************************Module*Header******************************\
* Module Name: ICMUPG.C
*
* Module Descripton: This file has code that upgrades Win9x ICM to
*                    Memphis and NT 5.0
*
* Warnings:
*
* Issues:
*
* Public Routines:
*
* Created:  14 November 1996
* Author:   Srinivasan Chandrasekar    [srinivac]
*
* Copyright (c) 1996, 1997  Microsoft Corporation
\***********************************************************************/

#include "icmupg.h"
#include "msg.h"
#include <setupapi.h>
#include <stdio.h>


//#define ICM_MIG_DEBUG

#ifdef UNICODE
error.
This dll needs to be built with ANSI, not UNICODE because it must run on
Win95, Win98 and on Windows 2000
#endif


//
// Local typedefs
//

typedef struct tagMANUMODELIDS {
    DWORD dwManuID;
    DWORD dwModelID;
} MANUMODELIDS, *PMANUMODELIDS;

typedef struct tagREGDATA {
    DWORD dwRefCount;
    DWORD dwManuID;
    DWORD dwModelID;
} REGDATA, *PREGDATA;

typedef BOOL (WINAPI *PFNINSTALLCOLORPROFILEA)(PSTR, PSTR);
typedef BOOL (WINAPI *PFNINSTALLCOLORPROFILE)(LPCTSTR, LPCTSTR);
typedef BOOL (WINAPI *PFNENUMCOLORPROFILES)(PCTSTR, PENUMTYPE, PBYTE, PDWORD, PDWORD);

typedef struct {
    CHAR CompanyName[256];
    CHAR SupportNumber[256];
    CHAR SupportUrl[256];
    CHAR InstructionsToUser[1024];
} VENDORINFO, *PVENDORINFO;


//
// Global variables
//

TCHAR  const gszICMRegPath[]     = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ICM";
TCHAR  const gszProfile[]        = "profile";
TCHAR  const gszMSCMSdll[]       = "mscms.dll";

char   const gszProductID[]      = "Microsoft Color Management System";

char   const gszInstallColorProfile[] = "InstallColorProfileA";
char   const gszGetColorDirectory[]   = "GetColorDirectoryA";
char   const gszEnumColorProfiles[]   = "EnumColorProfilesA";
VENDORINFO   gVendorInfo;
char         gszMigInf[MAX_PATH];
char   const gszFullICMRegPath[]      = "\"HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ICM\"";

char   const gszInstallColorProfileA[] = "InstallColorProfileA";


//BOOL gbWin98 = FALSE;

#ifdef DBG
DWORD  gdwDebugControl;
#endif
TCHAR  szValue[MAX_PATH];
TCHAR  szName[MAX_PATH];

PFNINSTALLCOLORPROFILEA pInstallColorProfileA = NULL;
PFNINSTALLCOLORPROFILE pInstallColorProfile = NULL;
PFNENUMCOLORPROFILES   pEnumColorProfiles = NULL;

//
// Local functions
//

VOID  InternalUpgradeICM();
VOID  UpgradeClass(HKEY);
BOOL  AssociateMonitorProfile();
BOOL  AssociatePrinterProfiles(HKEY);
VOID  InstallProfiles();
VOID  DeleteOldICMKey();
void  GetManuAndModelIDs(PTSTR, DWORD*, DWORD*);
int   lstrcmpn(PTSTR, PTSTR, DWORD);

HINSTANCE hinstMigDll;


BOOL WINAPI 
DllEntryPoint(HINSTANCE hinstDll, DWORD dwReason, LPVOID lpReserved) {
  if(dwReason==DLL_PROCESS_ATTACH) {
    hinstMigDll = hinstDll;
  }
  return TRUE;
}



/******************************************************************************
 *
 *                            QueryVersion
 *
 *  Function:
 *       This function is called to get the DLL version information.
 *
 *  Arguments:
 *       pszProductID - Fill in a unique string identifying us.
 *       puDllVersion - Our DLL version
 *
 *       None of the other arguments are used
 *
 *  Returns:
 *       ERROR_SUCCESS to indicate success
 *
 ******************************************************************************/

LONG
CALLBACK
QueryVersion(
	OUT LPCSTR  *pszProductID,
	OUT LPUINT  puDllVersion,
	OUT LPINT   *pCodePageArray,	OPTIONAL
	OUT LPCSTR  *ppszExeNamesBuf,	OPTIONAL
	OUT PVENDORINFO  *ppVendorInfo
	)
{
    *pszProductID = gszProductID;
    *puDllVersion = 1;
    *ppszExeNamesBuf    = NULL;
    *pCodePageArray = NULL;
    *ppVendorInfo = &gVendorInfo;
    memset(&gVendorInfo, 0, sizeof(VENDORINFO));
    FormatMessageA(
                 FORMAT_MESSAGE_FROM_HMODULE,
                 hinstMigDll,
                 MSG_VI_COMPANY_NAME,
                 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                 gVendorInfo.CompanyName,
                 sizeof(gVendorInfo.CompanyName),
                 NULL
                 );

    FormatMessageA(
                 FORMAT_MESSAGE_FROM_HMODULE,
                 hinstMigDll,
                 MSG_VI_SUPPORT_NUMBER,
                 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                 gVendorInfo.SupportNumber,
                 sizeof(gVendorInfo.SupportNumber),
                 NULL
                 );

    FormatMessageA(
                 FORMAT_MESSAGE_FROM_HMODULE,
                 hinstMigDll,
                 MSG_VI_SUPPORT_URL,
                 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                 gVendorInfo.SupportUrl,
                 sizeof(gVendorInfo.SupportUrl),
                 NULL
                 );

    FormatMessageA(
                 FORMAT_MESSAGE_FROM_HMODULE,
                 hinstMigDll,
                 MSG_VI_INSTRUCTIONS,
                 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                 gVendorInfo.InstructionsToUser,
                 sizeof(gVendorInfo.InstructionsToUser),
                 NULL
                 );
    WARNING((__TEXT("QueryVersion called\n")));
    return ERROR_SUCCESS;
}


/******************************************************************************
 *
 *                            Initialize9x
 *
 *  Function:
 *       This function is called when upgrading to NT 5.0 from Win9x on the
 *       Win9x side.
 *
 *  Arguments:
 *       pszWorkingDir - Directory where migrate.inf will be found
 *
 *  Returns:
 *       ERROR_SUCCESS to indicate success
 *
 ******************************************************************************/

LONG
CALLBACK
Initialize9x(
    IN  LPCSTR   pszWorkingDir,
    IN  LPCSTR   pszSourceDir,
    IN  LPVOID   pvReserved
    )
{
  //
  // Lets figure out if we're on a Win98 or Win95 system
  // We don't migrate Win95 because Win95 doesn't have a 
  // profile database to migrate.
  //

/*  OSVERSIONINFO osVer;

  osVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
  GetVersionEx(&osVer);
  gbWin98 = 
    (osVer.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) &&
    ( (osVer.dwMajorVersion > 4) ||
    ( (osVer.dwMajorVersion == 4) && (osVer.dwMinorVersion > 0) ) );
 */
  WARNING((__TEXT("Initialize9x called\n")));
   
  lstrcpyA(gszMigInf, pszWorkingDir);
  lstrcatA(gszMigInf, "\\migrate.inf");

  return ERROR_SUCCESS;
}


/******************************************************************************
 *
 *                            MigrateUser9x
 *
 *  Function:
 *       This function is called on Win9x to upgrade per user settings.
 *
 *  Arguments:
 *       None of the arguments are used
 *
 *  Returns:
 *       ERROR_SUCCES to indicate success
 *
 ******************************************************************************/

LONG
CALLBACK
MigrateUser9x(
    IN  HWND     hwndParent,
    IN  LPCSTR   pszUnattendFile,
    IN  HKEY     hUserRegKey,
    IN  LPCSTR   pszUserName,
    LPVOID       pvReserved
    )
{
    //
    // Nothing to do
    //

    WARNING((__TEXT("MigrateUser9x called\n")));
    return  ERROR_SUCCESS;
}


/******************************************************************************
 *
 *                            MigrateSystem9x
 *
 *  Function:
 *       This function is called on the Win9x to upgrade system settings.
 *
 *  Arguments:
 *       None of the arguments are used
 *
 *  Returns:
 *       ERROR_SUCCES to indicate success
 *
 ******************************************************************************/

LONG
CALLBACK
MigrateSystem9x(
    IN  HWND    hwndParent,
    IN  LPCSTR  pszUnattendFile,
    LPVOID      pvReserved
    )
{
    DWORD            nProfiles;
    DWORD            dwSize;
    char             szColorDir[MAX_PATH];
    char             szNewColorDir[MAX_PATH];
    char             szDrive[2];
    HMODULE          hModule;
    ENUMTYPE         et = {sizeof (ENUMTYPE), ENUM_TYPE_VERSION, 0, NULL};    
    PBYTE            pBuffer;
    PSTR             pstrBuffer;
    PSTR             pstrTraversal;

    WARNING((__TEXT("MigrateSystem9x called\n")));
    
    //
    // Produce the Win9x Color Directory.
    //

    GetWindowsDirectoryA(szColorDir, MAX_PATH);
    if (szColorDir[lstrlenA(szColorDir)-1] != '\\') 
    {
        lstrcatA(szColorDir,"\\");
    }
    lstrcatA(szColorDir, "system\\color\\");
    
    GetWindowsDirectoryA(szNewColorDir, MAX_PATH);
    if (szNewColorDir[lstrlenA(szNewColorDir)-1] != '\\') 
    {
        lstrcatA(szNewColorDir,"\\");
    }
    lstrcatA(szNewColorDir, "system32\\spool\\drivers\\color\\");


    //
    // If this is a Win95 system we have nothing to do because
    // Win95 doesn't have a color profile database.
    //

    
    //
    // We can't have mscms as an implib because when they try to load us in
    // Win95, they won't find mscms.dll and reject us.
    //
    
    hModule = LoadLibrary(gszMSCMSdll);
    if (hModule) {
      #ifdef ICM_MIG_DEBUG
      WritePrivateProfileStringA("ICM Debug", "hModule", "not NULL", gszMigInf);
      WritePrivateProfileStringA("ICM Debug", "gbWin98", "TRUE", gszMigInf);
      #endif

      pEnumColorProfiles = (PFNENUMCOLORPROFILES)GetProcAddress(hModule, gszEnumColorProfiles);
      if (pEnumColorProfiles) {
        
        #ifdef ICM_MIG_DEBUG
        WritePrivateProfileStringA("ICM Debug", "pEnumColorProfiles", "not NULL", gszMigInf);
        #endif

        //
        // Compute the size of the EnumColorProfiles buffer.
        //
    
        dwSize = 0;
        pEnumColorProfiles(NULL, &et, NULL, &dwSize, &nProfiles);
        
        if(dwSize==0) 
        {
          #ifdef ICM_MIG_DEBUG
          WritePrivateProfileStringA("ICM Debug", "dwSize", "0", gszMigInf);
          #endif 
          //
          // Need to exit - nothing to do if there are no profiles installed,
          // except to move the directory and registry settings.
          //
          WARNING((__TEXT("No profiles installed\n")));
          goto EndMigrateSystem9x;
        }
    
    
        //
        // Enumerate all the currently installed color profiles.
        //

        #ifdef ICM_MIG_DEBUG
        WritePrivateProfileStringA("ICM Debug", "Enumerate", "Start", gszMigInf);
        #endif 

        pBuffer = (BYTE *)malloc(dwSize);
        pstrBuffer = (PSTR)pBuffer;
        
        #ifdef ICM_MIG_DEBUG
        WritePrivateProfileStringA("ICM Debug", "Enumerate", "TRUE", gszMigInf);
        #endif         
        
        if(pEnumColorProfiles(NULL, &et, pBuffer, &dwSize, &nProfiles))
        {            
            #ifdef ICM_MIG_DEBUG
            WritePrivateProfileStringA("ICM Debug", "Enumerate", "for", gszMigInf);
            #endif 

            for(pstrTraversal = pstrBuffer;
                nProfiles--;
                pstrTraversal += 1 + lstrlenA(pstrTraversal)) {

                //
                // Write the fact into the Migration Information file.
                //
                
                WritePrivateProfileStringA("Installed ICM Profiles", pstrTraversal, "1", gszMigInf);
            }
        }
        free(pBuffer);
      } 
      #ifdef ICM_MIG_DEBUG
        else {
        WritePrivateProfileStringA("ICM Debug", "pEnumColorProfiles", "NULL", gszMigInf);
      }
      #endif

  
      EndMigrateSystem9x:
      if (hModule)
      {
          FreeLibrary(hModule);
      }
  }
  #ifdef ICM_MIG_DEBUG
    else {
    WritePrivateProfileStringA("ICM Debug", "hModule", "NULL", gszMigInf);    
    WritePrivateProfileStringA("ICM Debug", "gbWin98", "FALSE", gszMigInf);
  }
  #endif

  //
  // We'll handle the ICM branch of the registry
  //

  WritePrivateProfileStringA("Handled", gszFullICMRegPath, "Registry", gszMigInf);

      
  //
  // We'll be moving the entire subdirectory.
  //

  WritePrivateProfileStringA("Moved", szColorDir, szNewColorDir, gszMigInf);


  return  ERROR_SUCCESS;
}


/******************************************************************************
 *
 *                            InitializeNT
 *
 *  Function:
 *       This function is called when upgrading to NT 5.0 from Win9x on the NT
 *       side. Its main purpose is to initialize us.
 *
 *  Arguments:
 *       None of the arguments are used
 *
 *  Returns:
 *       ERROR_SUCCESS to indicate success
 *
 ******************************************************************************/

LONG
CALLBACK
InitializeNT(
    IN  LPCWSTR pszWorkingDir,
    IN  LPCWSTR pszSourceDir,
    LPVOID      pvReserved
    )
{
    SetupOpenLog(FALSE);
    SetupLogError("ICM Migration: InitializeNT called\r\n", LogSevInformation);
    return ERROR_SUCCESS;
}


/******************************************************************************
 *
 *                            MigrateUserNT
 *
 *  Function:
 *       This function is called on the NT to upgrade per user settings.
 *
 *  Arguments:
 *       None of the arguments are used
 *
 *  Returns:
 *       ERROR_SUCCES to indicate success
 *
 ******************************************************************************/

LONG
CALLBACK
MigrateUserNT(
    IN  HANDLE    hUnattendInf,
    IN  HKEY      hUserRegKey,
    IN  LPCWSTR   pszUserName,
    LPVOID        pvReserved
    )
{
    SetupLogError("ICM Migration: MigrateUserNT called\r\n", LogSevInformation);

    //
    // Nothing to do
    //

    return  ERROR_SUCCESS;
}


/******************************************************************************
 *
 *                            MigrateSystemNT
 *
 *  Function:
 *       This function is called on the Win9x to upgrade system settings. This
 *       is where we upgrade ICM 2.0
 *
 *  Arguments:
 *       None of the other arguments are used
 *
 *  Returns:
 *       ERROR_SUCCES to indicate success
 *
 ******************************************************************************/

LONG
CALLBACK
MigrateSystemNT(
    IN  HANDLE  hUnattendInf,
    LPVOID      pvReserved
    )
{
    HINSTANCE hModule;
    LONG      rc = ERROR_FILE_NOT_FOUND;
    CHAR      szMessage[MAX_PATH];

    SetupLogError("ICM Migration: MigrateSystemNT called\r\n", LogSevInformation);
    
    //
    // We can't have mscms as an implib because when they try to load us in
    // Win95, they won't find mscms.dll and reject us.
    //

    hModule = LoadLibrary(gszMSCMSdll);
    if (!hModule)
    {
        sprintf(szMessage, "ICM Migration: Fatal Error, cannot load mscms.dll. Error %d\r\n", GetLastError());
        SetupLogError(szMessage, LogSevFatalError);
        return rc;
    }

    pInstallColorProfileA = (PFNINSTALLCOLORPROFILEA)GetProcAddress(hModule, gszInstallColorProfileA);
    pInstallColorProfile = (PFNINSTALLCOLORPROFILE)GetProcAddress(hModule, gszInstallColorProfile);

    if (!pInstallColorProfile || !pInstallColorProfileA)
    {
        SetupLogError("ICM Migration: Fatal Error, cannot find mscms functions. \r\n", LogSevFatalError);
        goto EndMigrateSystemNT;
    }

    InternalUpgradeICM();   // Upgrade over Win9x
    InstallProfiles();      // Install all profiles in the old color directory
    DeleteOldICMKey();

    rc = ERROR_SUCCESS;

EndMigrateSystemNT:

    if (hModule)
    {
        FreeLibrary(hModule);
    }

    return rc;
}


/******************************************************************************
 *
 *                           DeleteOldICMKey
 *
 *  Function:
 *       This function deletes the ICM key and subkeys from the Windows branch.
 *
 *  Arguments:
 *       None
 *
 *  Returns:
 *       Nothing
 *
 ******************************************************************************/

VOID
DeleteOldICMKey()
{
    HKEY      hkICM = NULL;         // key to ICM branch in registry
    DWORD nSubkeys, i;
    TCHAR szKeyName[32];

    //
    // Open the registry path where profiles used to be kept
    //

    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, gszICMRegPath, 0, NULL, 
                       REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
                       NULL, &hkICM, NULL) != ERROR_SUCCESS)
    {
        
        SetupLogError("ICM Migration: Cannot open ICM branch of registry\r\n", LogSevError);
        return;
    }

    if (RegQueryInfoKey(hkICM, NULL, NULL, 0, &nSubkeys, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
    {
        SetupLogError("ICM Migration: Cannot enumerate ICM branch of registry\r\n", LogSevError);
        goto EndDeleteOldICMKey;
    }

    //
    // Go through all the device classes and delete all subkeys - this should
    // only be one level deep
    //

    for (i=nSubkeys; i>0; i--)
    {
        RegEnumKey(hkICM, i-1, szKeyName, sizeof(szKeyName));
        RegDeleteKey(hkICM, szKeyName);
    }

EndDeleteOldICMKey:
    if (hkICM)
    {
        RegCloseKey(hkICM);
    }
    RegDeleteKey(HKEY_LOCAL_MACHINE, gszICMRegPath);

    return;
}


//
// Move directory with contents.
// Note this is not recursive.
// The purpose of this routine is to move the old color directory to the 
// new color directory. During setup the new color directory may have already
// been created and populated with files. Vendor apps may have populated the 
// old color directory with private subdirectories and files which will not 
// appear in the new directory created by setup. This routine is designed 
// to move those files.
//
// Note it'll fail to move a subdirectory of the old color directory if 
// a similar subdirectory exists in the new color directory - this should not
// be the case.
//
// s and d should have the trailing slash.
//

void MyMoveDir(char *s, char *d) {
  WIN32_FIND_DATA rf;
  HANDLE hf;
  char s2[MAX_PATH];
  char s_[MAX_PATH];
  char d_[MAX_PATH];
  char err[MAX_PATH];

  //
  // If MoveFileEx succeeds, we're done.
  //

  if(!MoveFileEx(s, d, MOVEFILE_REPLACE_EXISTING)) {
    sprintf(s2, "%s*", s);
    hf = FindFirstFile(s2, &rf);
    do {
      // don't move . and ..
      if(!(strcmp(".", rf.cFileName)==0 ||
           strcmp("..", rf.cFileName)==0) ) {
        sprintf(s_, "%s%s", s, rf.cFileName);
        sprintf(d_, "%s%s", d, rf.cFileName);
        if(!MoveFileEx(s_, d_, MOVEFILE_REPLACE_EXISTING)) {
          int e = GetLastError();  
          sprintf(err, "ICM Migration: Failed the move of %s with %d\r\n", s_, e);
          SetupLogError(err, LogSevError);
        } else {
          sprintf(err, "ICM Migration: Moved %s to %s\n", s_, d_);
          SetupLogError(err, LogSevInformation);
        }
      }

    } while(FindNextFile(hf, &rf));
    FindClose(hf);
  }

  //
  // source directory should theoretically be empty at this point
  // If there are errors, we'll leave files behind and report this in 
  // the setup log as a LogSevError.
  //
}



/******************************************************************************
 *
 *                           InstallProfiles
 *
 *  Function:
 *       This function installs all profiles in %windir%\system\color.
 *       This is used when upgrading from Win9x to NT 5.0.
 *
 *  Arguments:
 *       None
 *
 *  Returns:
 *       Nothing
 *
 ******************************************************************************/

VOID
InstallProfiles()
{
    WIN32_FIND_DATAA wfd;
    PSTR             pNewColorDirEnd;
    HANDLE           hFindFile;
    CHAR             szOldColorDir[MAX_PATH];
    CHAR             szNewColorDir[MAX_PATH];
    CHAR             szReturnString[2];
    CHAR             szDefaultString[2];
    CHAR             szMessage[2*MAX_PATH+100];

    GetWindowsDirectoryA(szOldColorDir, MAX_PATH);
    if (szOldColorDir[lstrlenA(szOldColorDir)-1] != '\\')
        lstrcatA(szOldColorDir, "\\");
    lstrcatA(szOldColorDir, "system\\color\\");


    GetWindowsDirectoryA(szNewColorDir, MAX_PATH);
    if (szNewColorDir[lstrlenA(szNewColorDir)-1] != '\\')
    {
        lstrcatA(szNewColorDir, "\\");
    }
    lstrcatA(szNewColorDir, "system32\\spool\\drivers\\color\\");

    ASSERT(pInstallColorProfileA != NULL);


    //
    // Eat any errors on the MoveFile. This is just in case the migration 
    // was stopped after a previous move and now the source doesn't exist.
    //

    MyMoveDir(szOldColorDir, szNewColorDir);

    //
    // Now we have presumably moved everything so run through the list of 
    // previously installed profiles and install those in the new directory
    // (if we find them).
    //

    pNewColorDirEnd = szNewColorDir + lstrlenA(szNewColorDir);
    lstrcatA(szNewColorDir, "*.*");



    szDefaultString[0]='0';
    szDefaultString[1]=0;
    hFindFile = FindFirstFileA(szNewColorDir, &wfd);


    if (hFindFile != INVALID_HANDLE_VALUE)
    {
        do
        {
            lstrcpyA(pNewColorDirEnd, wfd.cFileName);

            //
            // Check to see if the profile was installed on Win9x
            //

            GetPrivateProfileStringA("Installed ICM Profiles", wfd.cFileName, szDefaultString, szReturnString, 2, gszMigInf);

            //
            // If it was installed, attempt to install it on NT
            //

            if(szReturnString[0]=='1') { 
                if (!(*pInstallColorProfileA)(NULL, szNewColorDir))
                {
                    sprintf(szMessage, "ICM Migration: Error %d installing profile %s\r\n", GetLastError(), szNewColorDir);
                    SetupLogError(szMessage, LogSevError);
                }
                else
                {
                    sprintf(szMessage, "ICM Migration: Installed profile %s\r\n", szNewColorDir);
                    SetupLogError(szMessage, LogSevInformation);
                }
            }

        } while (FindNextFileA(hFindFile, &wfd));

        FindClose(hFindFile);
    }
    else
    {
        SetupLogError("ICM Migration: FindFirstFile returned an invalid handle\r\n", LogSevFatalError);
    }
}


/******************************************************************************
 *
 *                           InternalUpgradeICM
 *
 *  Function:
 *       This function forms the core of the upgrade code. It installs all
 *       profiles in the regsitry, and associates with the right devices
 *
 *  Arguments:
 *       None
 *
 *  Returns:
 *       Nothing
 *
 ******************************************************************************/

VOID
InternalUpgradeICM()
{
    HKEY      hkICM = NULL;         // key to ICM branch in registry
    HKEY      hkDevice = NULL;      // key to ICM device branch in registry
    int       i;                    // counter variable
    TCHAR    *pszClasses[] = {      // different profile classes
        __TEXT("mntr"),
        __TEXT("prtr"),
        __TEXT("scnr"),
        __TEXT("link"),
        __TEXT("abst"),
        __TEXT("spac"),
        __TEXT("nmcl")
    };
    CHAR szMessage[MAX_PATH];
    LONG errcode;

    //
    // Open the registry path where profiles are kept
    //
    
    if (errcode = RegCreateKeyEx(HKEY_LOCAL_MACHINE, gszICMRegPath, 0, NULL, 
                                 REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, 
                                 &hkICM, NULL) != ERROR_SUCCESS)
    {
        sprintf(szMessage, "ICM Migration: Fatal Error, cannot open registry entry (%s) code:%d\r\n", 
                gszICMRegPath, errcode);
        SetupLogError(szMessage, LogSevFatalError);
        return;
    }

    //
    // Go through all the device classes and install the profiles
    //

    for (i=0; i<sizeof(pszClasses)/sizeof(PTSTR); i++)
    {
        if (RegOpenKeyEx(hkICM, pszClasses[i], 0, KEY_ALL_ACCESS, &hkDevice) != ERROR_SUCCESS)
        {
            continue;           // go to next key
        }
       
        sprintf(szMessage, "ICM Migration: Upgrading %s\r\n", pszClasses[i]);
        SetupLogError(szMessage, LogSevInformation);
        UpgradeClass(hkDevice);

        RegCloseKey(hkDevice);
    }

    //
    // Set default monitor profile
    //

    // AssociateMonitorProfile(); - Not needed for Memphis
    // If Pnp moves everything from Win9x PnP S/W section to NT 5.0 PnP S/W
    // section, then we don't need this for NT either

    if (hkICM)
    {
        RegCloseKey(hkICM);
    }

    return;
}


/******************************************************************************
 *
 *                           UpgradeClass
 *
 *  Function:
 *       This function recursively calls itself to go down a registry path
 *       till it reaches the leaf, and installs all profiles it finds there
 *
 *  Arguments:
 *       hKey            - registry key for root node
 *
 *  Returns:
 *       Nothing
 *
 ******************************************************************************/

VOID
UpgradeClass(
    HKEY  hKey
    )
{
    HKEY  hSubkey;
    DWORD nSubkeys, nValues, i, cbName, cbValue;
    TCHAR szKeyName[32];
    CHAR  szMessage[MAX_PATH];

    //
    // If there is an error, return
    //

    if (RegQueryInfoKey(hKey, NULL, NULL, 0, &nSubkeys, NULL, NULL,
        &nValues, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
    {
        return;
    }

    if (nSubkeys > 0)
    {
        //
        // This is not the leaf node, recurse
        //

        for (i=nSubkeys; i>0; i--)
        {
            RegEnumKey(hKey, i-1, szKeyName, sizeof(szKeyName));
            if (RegOpenKeyEx(hKey, szKeyName, 0, KEY_ALL_ACCESS, &hSubkey) == ERROR_SUCCESS)
            {
                UpgradeClass(hSubkey);
                RegCloseKey(hSubkey);
                RegDeleteKey(hKey, szKeyName);
            }
        }
    }
    else
    {
        //
        // This is the leaf node - install all the profiles registered
        //

        ASSERT(pInstallColorProfile != NULL);

        for (i=nValues; i>0; i--)
        {
            cbName = MAX_PATH;
            cbValue = MAX_PATH;
            if (RegEnumValue(hKey, i-1, szName, &cbName, 0, NULL, (LPBYTE)szValue,
                &cbValue) == ERROR_SUCCESS)
            {
                if (! lstrcmpn(szName, (PTSTR)gszProfile, lstrlen(gszProfile)))
                {
                    if (! (*pInstallColorProfile)(NULL, szValue))
                    {
                        sprintf(szMessage, "ICM Migration: Error installing profile %s\r\n", szValue);
                        SetupLogError(szMessage, LogSevError);
                    }
                    else
                    {
                        sprintf(szMessage, "ICM Migration: Installed profile %s\r\n", szValue);
                        SetupLogError(szMessage, LogSevInformation);
                    }            
                }
                else
                {
                    PTSTR pProfile;
                
                    //
                    // We might be upgrading over Memphis or later
                    // In Memphis it is "file name" "value" instead of
                    // "profilexx" "value" in Win95 & OSR2
                    //
                
                    if (szName[1] == ':')
                    {
                        //
                        // Assume full path name
                        //
                
                        pProfile = szName;
                
                    }
                    else
                    {
                        GetWindowsDirectory(szValue, MAX_PATH);
                        if (szValue[lstrlen(szValue)-1] != '\\')
                            lstrcat(szValue, __TEXT("\\"));
                        lstrcat(szValue, __TEXT("system\\color\\"));
                        lstrcat(szValue, szName);
                        pProfile = szValue;
                    }
                
                    if (! (*pInstallColorProfile)(NULL, pProfile))
                    {
                        sprintf(szMessage, "ICM Migration: Error installing profile %s\r\n", pProfile);
                        SetupLogError(szMessage, LogSevError);
                    }
                    else
                    {
                        sprintf(szMessage, "ICM Migration: Installed Profile %s\r\n", pProfile);
                        SetupLogError(szMessage, LogSevInformation);
                    }
                }                
                RegDeleteValue(hKey, szName);
            }
        }
    }

    return;
}


/******************************************************************************
 *
 *                            lstrcmpn
 *
 *  Function:
 *       This function compares dwLen characters of two strings and decides if
 *       they are equal
 *
 *  Arguments:
 *       pStr1           - pointer to string 1
 *       pStr2           - pointer to string 2
 *       dwLen           - number of characters to compare
 *
 *  Returns:
 *       Zero if the strings are equal, non zero otherwise
 *
 ******************************************************************************/

int
lstrcmpn(
    PTSTR pStr1,
    PTSTR pStr2,
    DWORD dwLen
    )
{
    //
    // Assume no NULL strings
    //

    while (*pStr1 && *pStr2 && --dwLen)
    {
        if (*pStr1 != *pStr2)
            break;

        pStr1++;
        pStr2++;
    }

    return (int)(*pStr1 - *pStr2);
}
#if DBG

/******************************************************************************
 *
 *                              MyDebugPrint
 *
 *  Function:
 *       This function takes a format string and paramters, composes a string
 *       and sends it out to the debug port. Available only in debug build.
 *
 *  Arguments:
 *       pFormat  - pointer to format string
 *       .......  - parameters based on the format string like printf()
 *
 *  Returns:
 *       No return value
 *
 ******************************************************************************/

VOID
MyDebugPrintA(
    PSTR pFormat,
    ...
    )
{
    char     szBuffer[256];
    va_list  arglist;

    va_start(arglist, pFormat);
    wvsprintfA(szBuffer, pFormat, arglist);
    va_end(arglist);

    OutputDebugStringA(szBuffer);

    return;
}


VOID
MyDebugPrintW(
    PWSTR pFormat,
    ...
    )
{
    WCHAR    szBuffer[256];
    va_list  arglist;

    va_start(arglist, pFormat);
    wvsprintfW(szBuffer, pFormat, arglist);
    va_end(arglist);

    OutputDebugStringW(szBuffer);

    return;
}

/******************************************************************************
 *
 *                              StripDirPrefixA
 *
 *  Function:
 *       This function takes a path name and returns a pointer to the filename
 *       part. This is availabel only for the debug build.
 *
 *  Arguments:
 *       pszPathName - path name of file (can be file name alone)
 *
 *  Returns:
 *       A pointer to the file name
 *
 ******************************************************************************/

PSTR
StripDirPrefixA(
    PSTR pszPathName
    )
{
    DWORD dwLen = lstrlenA(pszPathName);

    pszPathName += dwLen - 1;       // go to the end

    while (*pszPathName != '\\' && dwLen--)
    {
        pszPathName--;
    }

    return pszPathName + 1;
}

#endif

#ifdef STANDALONE

//
// For testing
//

main()
{
    UpgradeICM(NULL, NULL, NULL, 0);
    
    TCHAR buffer[MAX_PATH];
    MigrateInit(buffer, NULL, 0, NULL, NULL);
    MigrateInit(NULL, NULL, 0, NULL, NULL);
    MigrateLocalMachine(NULL, NULL);
    return 0;
}
#endif

