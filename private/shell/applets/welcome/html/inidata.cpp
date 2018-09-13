// IniData.cpp: implementation of the CIniData class.
//
//////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <shlwapi.h>
#include <shlobj.h>     // needed for shellp.h
#include <shellp.h>     // IsUserAnAdmin
#include "IniData.h"
#include "resource.h"

// Reg Keys:
const TCHAR POLICY[]          = TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Policy");
const TCHAR POSTSETUPCONFIG[] = TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Setup\\OCManager\\ToDoList");
const TCHAR WELCOMESETUP[]    = TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Setup\\Welcome");
const TCHAR RUNKEY[]          = TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Tips");
const TCHAR RUNVALUENAME[]    = TEXT("Show");

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CIniData::CIniData()
{
    m_iItems = 0;
    m_iCurItem = -1;
    m_bCheckState = false;   // By default we assume the welcome registry key doesn't exist and we won't run at startup.
}

CIniData::~CIniData()
{
}

CIniData::CDataItem & CIniData::operator[](int i)
{
    return m_data[i];
}

// BUGBUG: This is a hack, remove when setup changes are made
void CIniData::HackHackPreventRunningTwice()
{
    HKEY hkey;
    if (ERROR_SUCCESS == RegOpenKeyEx( HKEY_CURRENT_USER, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\run"), 0, KEY_ALL_ACCESS, &hkey ) )
    {
        static const TCHAR c_szWelcome[] = TEXT("Welcome");
        TCHAR szBuf[MAX_PATH];
        DWORD dwType;
        DWORD dwSize = sizeof(szBuf);
        if ( ERROR_SUCCESS == RegQueryValueEx( hkey, c_szWelcome, NULL, &dwType, (LPBYTE)&szBuf, &dwSize ) )
        {
            // make sure the explorer show key is turned on
            SetRunKey( TRUE );
        }
        RegDeleteValue( hkey, c_szWelcome );
        RegCloseKey( hkey );
    }
}

void CIniData::Init()
{
    // BUGBUG: Temporary hack, remove after setup changes are made.
    HackHackPreventRunningTwice();

    // STEP 1: Check for any Post Setup Config items in the registry
    //  step 1b:  if found, check if the user is an admin (these are admin only)
    InitalizePostSetupConfig();

    // STEP 2: Read the standard items from the INI file
    ReadItemsFromINIFile();

    // STEP 3: Check each item against the registry to see if they have been done, remove completed items
    RemoveCompletedItems();

    // STEP 4: If welcome is no longer set to automatically run then hide the check box (since its already been disabled)
    SetCheckboxState();
}

// InitalizePostSetupConfig
//
// This handles the special case menu item for displaying a link to the PostSetupConfig stuff.
// This menu item is a special case in that it isn't read from the welcome.ini file, instead
// it is automatically added if there are any PSC items in the regestry.  Since these items
// can only be setup by an admin we don't display this item unless the current user is a member
// of the administrators group.
void CIniData::InitalizePostSetupConfig()
{
    HKEY hkeyRoot;
    if ( ERROR_SUCCESS==RegOpenKeyEx(HKEY_LOCAL_MACHINE, POSTSETUPCONFIG, 0, KEY_ENUMERATE_SUB_KEYS, &hkeyRoot))
    {
        TCHAR szDescription[MAX_PATH+1];
        if (ERROR_SUCCESS==RegEnumKey( hkeyRoot, 0, szDescription, MAX_PATH+1 ))
        {
            // there are sub keys under this key, check if the user is an admin
            if ( IsUserAnAdmin() )
            {
                // Since the user has the authority to complete setup, display the special
                // link to the new Add/Remove Programs in slappmgr.dll
                TCHAR szTitle[256];
                TCHAR szMenu[256];
                TCHAR szConfig[MAX_PATH];
                TCHAR szArgs[MAX_PATH];
                HINSTANCE hinst = GetModuleHandle(NULL);
                LoadString(hinst, IDS_PSCTITLE,  szTitle,       256 );
                LoadString(hinst, IDS_PSCMENU,   szMenu,        256 );
                LoadString(hinst, IDS_PSCDESC,   szDescription, MAX_PATH+1 );
                LoadString(hinst, IDS_PSCCONFIG, szConfig,      MAX_PATH );
                LoadString(hinst, IDS_PSCARGS,   szArgs,        MAX_PATH );
                Add(szTitle,szMenu,szDescription,szConfig,szArgs,WF_ADMINONLY,0);
            }
        }
        RegCloseKey( hkeyRoot );
    }
    // if the key doesn't exist or has no values under it then we don't have to do anything
}

// ReadItemsFromINIFile
//
// The standard items, including any items added by an OEM, are read in from an INI file.
// This file contains one section for each menu item.  The section name is the name that
// will be displayed in the list of menu items.  Other fields include the description,
// the config command to run, and the flags for the item (should the header name be seperate?)
void CIniData::ReadItemsFromINIFile()
{
    const int iMaxSize = 1024;  // size used for temporary buffers

    TCHAR sectionNames[iMaxSize];
    TCHAR * thisSection = sectionNames;
    if ( 0==GetPrivateProfileSectionNames( sectionNames, iMaxSize, WELCOME_INI ) )
        return;

    while ( *thisSection && m_iItems < 7 )
    {
        TCHAR szMenu[iMaxSize];
        TCHAR szDesc[iMaxSize];
        TCHAR szConfig[MAX_PATH];
        TCHAR szArgs[MAX_PATH];
        DWORD dwFlags;
        DWORD dwIndex;

        GetPrivateProfileString( thisSection, TEXT("MenuText"), TEXT(""), szMenu, iMaxSize, WELCOME_INI );
        GetPrivateProfileString( thisSection, TEXT("Description"), TEXT(""), szDesc, iMaxSize, WELCOME_INI );
        GetPrivateProfileString( thisSection, TEXT("ConfigCommand"), TEXT(""), szConfig, MAX_PATH, WELCOME_INI );
        GetPrivateProfileString( thisSection, TEXT("ConfigArgs"), TEXT(""), szArgs, MAX_PATH, WELCOME_INI );
        dwFlags = GetPrivateProfileInt( thisSection, TEXT("Flags"), WF_PERUSER, WELCOME_INI );
        dwIndex = GetPrivateProfileInt( thisSection, TEXT("ImageIndex"), 0, WELCOME_INI );
        Add( thisSection, *szMenu?szMenu:NULL, szDesc, szConfig, *szArgs?szArgs:NULL, dwFlags, dwIndex );
        thisSection += lstrlen( thisSection ) + 1;
    }
}

// RemoveCompletedItems
//
// This function checks the registry to see which items have been completed.  When a program
// launched from welcome has finished it's task that program is responsible for adding an entry
// to the registry indicating that it is done.  These entries follow a standard convention (what is that convention?)
// which allows programs to specify wheter the should be removed from the list of menu items
// or shown in an alternate color.
void CIniData::RemoveCompletedItems()
{
    HKEY hkey = 0;
    int i;

    // if we fail to open one of these keys then we assume that all items under that
    // key still need to be completed.  This is based on the assumption that its better
    // to ask the user to do something twice then to never ask the user at all.
    if ( ERROR_SUCCESS == RegCreateKey(HKEY_CURRENT_USER, WELCOMESETUP, &hkey) )
    {
        // NOTE:  If item 0 is the Post Setup Config item then we don't really need to check
        // for it below since its a special case.  However, since it won't be in this part of
        // the registry it doesn't hurt to check.
        for ( i=0; i < m_iItems; i++ )
        {
            DWORD dwType;
            DWORD dwData;
            DWORD dwSize = sizeof(dwData);
            if ( ERROR_SUCCESS==RegQueryValueEx( hkey, m_data[i].title, NULL, &dwType, (LPBYTE)&dwData, &dwSize ))
            {
                // this menu item has an entry in the registry
                if ( dwType == REG_DWORD )
                {
                    // data can have several vaules depending on if the item should
                    // be excluded of just shown in an alternate color.  A value of
                    // 1 means don't show the item, any other non-zero value means
                    // show it in an alternate color, and 0 means show it normal
                    if ( 1 == dwData )
                    {
                        // setting completed to true will prevent this menu item from being injected
                        m_data[i].completed = true;
                    }
                    else if ( dwData )
                    {
                        m_data[i].flags |= WF_ALTERNATECOLOR;
                    }
                }
            }
        }
        RegCloseKey( hkey );
    }
}

// SetCheckboxState
//
// This function checks the registry to see if welcome is still listed under the run key.  It sets the
// checkbox state accordingly:
//  true  = checkbox is checked, welcome will run at startup
//  false = checkbox is unchecked, welcome won't run at startup
void CIniData::SetCheckboxState()
{
    HKEY hkey;

    // We initialize m_bCheckState to false.  As a result, if any of the below commands should fail
    // then we know m_bCheckState already has the correct value.
    m_bCheckState = false;
    if ( ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, RUNKEY, 0, KEY_READ, &hkey) )
    {
        DWORD dwType;
        DWORD dwData;
        DWORD cbSize = sizeof(dwData);
        if ( ERROR_SUCCESS == RegQueryValueEx( hkey, RUNVALUENAME, NULL, &dwType, (LPBYTE)&dwData, &cbSize ) )
        {
            m_bCheckState = (dwData != 0);
        }
        RegCloseKey(hkey);
    }
}

void CIniData::Add( LPTSTR szTitle, LPTSTR szMenu, LPTSTR szDesc, LPTSTR szCmd, LPTSTR szArgs, DWORD dwFlags, int iImgIndex )
{
    // we can only have a max of seven items (according to the spec) so we need to
    // inforce that limit
    if ( m_iItems >= 7 )
        return;

    m_data[m_iItems].title = new TCHAR[lstrlen(szTitle)+1];
    StrCpy( m_data[m_iItems].title, szTitle );
    if ( szMenu )
    {
        // menuname is allowed to remain NULL.  This is only used if you want the
        // text on the menu item to be different than the description. This could
        // be useful for localization where a shortened name might be required.
        m_data[m_iItems].menuname = new TCHAR[lstrlen(szMenu)+1];
        StrCpy( m_data[m_iItems].menuname, szMenu );
    }
    m_data[m_iItems].description = new TCHAR[lstrlen(szDesc)+1];
    StrCpy( m_data[m_iItems].description, szDesc );
    m_data[m_iItems].cmdline = new TCHAR[lstrlen(szCmd)+1];
    StrCpy( m_data[m_iItems].cmdline, szCmd );
    if ( szArgs )
    {
        // Some commands don't have any args so this can remain NULL.  This is only used
        // if the executable requires arguments.
        m_data[m_iItems].args = new TCHAR[lstrlen(szArgs)+1];
        StrCpy( m_data[m_iItems].args, szArgs );
    }
    m_data[m_iItems].flags = dwFlags;
    m_data[m_iItems].imgindex = iImgIndex;
    m_iItems++;
}

void CIniData::UpdateAndRun( int i )
{
    if ( (i>=0) && (i < m_iItems))
    {
        // Add entry to the registry to indicate that this item has been launched already.
        // This info will be persisted so welcome will look correct the next time it's run.
        HKEY hkey;
        if (ERROR_SUCCESS==RegOpenKeyEx(HKEY_CURRENT_USER, WELCOMESETUP, 0, KEY_READ|KEY_WRITE, &hkey))
        {
            DWORD dwType;
            DWORD dwData;
            DWORD dwSize = sizeof(dwData);
            if ( (ERROR_SUCCESS!=RegQueryValueEx( hkey, m_data[i].title, NULL, &dwType, (LPBYTE)&dwData, &dwSize )) ||
                 (dwType != REG_DWORD) ||
                 ( 1 != dwData ) )
            {
                // if we get here then it means that:
                // 1.) The reg value doesn't exist yet -OR-
                // 2.) The reg value exists but doesn't contain a DWORD data type -OR-
                // 3.) The reg value exists and contains a DWORD but that DWORD is not 1 so we want to change it
                dwData = 2;
                RegSetValueEx(hkey, m_data[i].title, NULL, REG_DWORD, (LPBYTE)&dwData, sizeof(dwData) );
            }
            RegCloseKey(hkey);
        }

        SHELLEXECUTEINFO ei;
        ei.cbSize          = sizeof(SHELLEXECUTEINFO);
        ei.fMask           = SEE_MASK_DOENVSUBST | SEE_MASK_NOCLOSEPROCESS;
        ei.hwnd            = NULL;
        ei.lpVerb          = TEXT("open");
        ei.lpFile          = m_data[i].cmdline;
        ei.lpParameters    = m_data[i].args;
        ei.lpDirectory     = NULL;
        ei.nShow           = SW_SHOWNORMAL;

        if (ShellExecuteEx(&ei))
        {
            if (NULL != ei.hProcess)
            {
                DWORD dwObject;

                while (1)
                {
                    dwObject = MsgWaitForMultipleObjects(1, &ei.hProcess, FALSE, INFINITE, QS_ALLEVENTS);
                    
                    if (WAIT_OBJECT_0 == dwObject)
                        break;
                    else if (WAIT_OBJECT_0+1 == dwObject)
                    {
                        MSG msg;

                        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                        {
                            if (WM_PAINT == msg.message)
                            {
                                TranslateMessage(&msg);
                                DispatchMessage(&msg);
                            }
                        }
                    }
                }

                CloseHandle(ei.hProcess);
            }
        }
    }
}

void CIniData::SetRunKey(DWORD dwData)
{
    HKEY hkey;
    LONG result;

    result = RegCreateKeyEx(
                    HKEY_CURRENT_USER,
                    RUNKEY, 
                    0,                          // dwReserved
                    NULL,                       // pointer to class name string
                    REG_OPTION_NON_VOLATILE,    // options for key creation
                    KEY_READ|KEY_WRITE,         // requested access for resulting key
                    NULL,                       // Sercurity Attribs, NULL=use default
                    &hkey,                      // the result
                    NULL);                      // LPDWORD, Dispostion (opened vs created)

    if (ERROR_SUCCESS==result)
    {
        RegSetValueEx( hkey, RUNVALUENAME, NULL, REG_DWORD, (LPBYTE)&dwData, sizeof(dwData) );
    }
}
