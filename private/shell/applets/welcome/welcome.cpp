// welcome.cpp: implementation of the CDataSource class for the welcome applet.
//
//////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <shlwapi.h>
#include <shlobj.h>     // needed for shellp.h
#include <shlobjp.h>
#include <shellp.h>
#include "welcome.h"
#include "resource.h"

#define ARRAYSIZE(x)    (sizeof(x)/sizeof(x[0]))

// Reg Keys:
#ifdef WINNT
const TCHAR WELCOMESETUP[]    = TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Setup\\Welcome");
const TCHAR RUNKEY[]          = TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Tips");
const TCHAR RUNVALUENAME[]    = TEXT("Show");

#define HKEY_WELCOME          HKEY_CURRENT_USER
#else
const TCHAR WELCOMESETUP[]    = TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Applets\\Welcome");
const TCHAR RUNKEY[]          = TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
const TCHAR RUNVALUENAME[]    = TEXT("Welcome");

const TCHAR CURRENTVERSION[]  = TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion");
const TCHAR PRODUCTTYPE[]     = TEXT("ProductType");

#define HKEY_WELCOME          HKEY_LOCAL_MACHINE
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDataSource::CDataSource()
{
    m_iItems = 0;
    m_iCurItem = -1;
    m_bCheckState = false;   // By default we assume the welcome registry key doesn't exist and we won't run at startup.
}

CDataSource::~CDataSource()
{
}

CDataItem & CDataSource::operator[](int i)
{
    return m_data[i];
}

bool CDataSource::Init()
{
    // STEP 1: Read items from the resources
    ReadItemsFromResources();

    // STEP 2: Read any OEM items from the INI file
    ReadItemsFromINIFile();

    // STEP 3: Check each item against the registry to see if they have been done, remove completed items
    RemoveCompletedItems();

    // STEP 4: If welcome is no longer set to automatically run then hide the check box (since its already been disabled)
    SetCheckboxState();

    return true;
}

void CDataSource::ReadItemsFromResources()
{
    HINSTANCE hinst = GetModuleHandle(NULL);
    TCHAR szTitle[256];
    TCHAR szDesc[MAX_PATH+1];
    TCHAR szMenu[256];
    TCHAR szConfig[MAX_PATH];
    TCHAR szArgs[MAX_PATH];

    DWORD dwDeletedItems = GetPrivateProfileInt( TEXT("Deleted"), TEXT("ItemMask"), 0, WELCOME_INI );

#ifndef WINNT
    // on win98 OEM we only show 2 items.  We do this by deleteing the other two using a
    // mask on dwDeletedItems.  We or in this mask if we have an OEM value:
    HKEY hkey;
    if ( ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, CURRENTVERSION, &hkey) )
    {
        DWORD dwType;
        DWORD dwSize = sizeof(szTitle);
        if ( ERROR_SUCCESS == RegQueryValueEx(hkey, PRODUCTTYPE, 0, &dwType, (LPBYTE)szTitle, &dwSize) )
        {
            if ( REG_SZ == dwType )
            {
                DWORD dwVal = StrToInt( szTitle );
                switch (dwVal)
                {
                case 115:
                case 116:
                case 120:
                    // This is an OEM install:
                    dwDeletedItems |= 0x0003;
                    break;

                default:
                    break;
                }
            }
        }
    }
#endif

    m_iItems = 0;
    for (int i=0;i<7;i++)   // 7 is the max allowed number of items
    {
        if ( (0x1<<i) & dwDeletedItems )
            continue;

        if ( !LoadString(hinst, IDS_TITLE0+i, szTitle, ARRAYSIZE(szTitle)) )
            break;

        LoadString(hinst, IDS_MENU0+i, szMenu, ARRAYSIZE(szMenu));
        LoadString(hinst, IDS_DESC0+i, szDesc, ARRAYSIZE(szDesc));
        LoadString(hinst, IDS_CONFIG0+i, szConfig, ARRAYSIZE(szConfig));
        LoadString(hinst, IDS_ARGS0+i, szArgs, ARRAYSIZE(szArgs));

        m_data[m_iItems].SetData( szTitle, szMenu, szDesc, szConfig, *szArgs?szArgs:NULL, WF_PERUSER, i );
        m_iItems++;
    }
}

// ReadItemsFromINIFile
//
// The standard items, including any items added by an OEM, are read in from an INI file.
// This file contains one section for each menu item.  The section name is the name that
// will be displayed in the list of menu items.  Other fields include the description,
// the config command to run, and the flags for the item (should the header name be seperate?)
void CDataSource::ReadItemsFromINIFile()
{
    const int iMaxSize = 1024;  // size used for temporary buffers

    TCHAR sectionNames[iMaxSize];
    TCHAR * thisSection = sectionNames;
    int cInitItems = m_iItems;

    if ( 0==GetPrivateProfileSectionNames( sectionNames, iMaxSize, WELCOME_INI ) )
        return;

    while ( *thisSection && m_iItems < 7 )
    {
        // We skip the special "Deleted" section which is used for control
        if ( 0 != StrCmpI(TEXT("Deleted"), thisSection) )
        {
            TCHAR szTitle[iMaxSize];
            TCHAR szMenu[iMaxSize];
            TCHAR szDesc[iMaxSize];
            TCHAR szConfig[MAX_PATH];
            TCHAR szArgs[MAX_PATH];
            DWORD dwFlags;
            DWORD dwIndex;
            int iReplace;

            GetPrivateProfileString( thisSection, TEXT("Title"), thisSection, szTitle, iMaxSize, WELCOME_INI );
            GetPrivateProfileString( thisSection, TEXT("MenuText"), TEXT(""), szMenu, iMaxSize, WELCOME_INI );
            GetPrivateProfileString( thisSection, TEXT("Description"), TEXT(""), szDesc, iMaxSize, WELCOME_INI );
            GetPrivateProfileString( thisSection, TEXT("ConfigCommand"), TEXT(""), szConfig, MAX_PATH, WELCOME_INI );
            GetPrivateProfileString( thisSection, TEXT("ConfigArgs"), TEXT(""), szArgs, MAX_PATH, WELCOME_INI );
            dwFlags = GetPrivateProfileInt( thisSection, TEXT("Flags"), WF_PERUSER, WELCOME_INI );
            dwIndex = GetPrivateProfileInt( thisSection, TEXT("ImageIndex"), 2, WELCOME_INI );

            iReplace = GetPrivateProfileInt( thisSection, TEXT("Replace"), -1, WELCOME_INI );
            if ( (iReplace < 0) || (iReplace >= cInitItems) ) 
            {
                // the replace was either invalid or missing, use m_iItems for the insert location
                iReplace = m_iItems;
                m_iItems++;
            }

            m_data[iReplace].SetData( szTitle, *szMenu?szMenu:NULL, szDesc, szConfig, *szArgs?szArgs:NULL, dwFlags, dwIndex );
        }
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
void CDataSource::RemoveCompletedItems()
{
    HKEY hkey = 0;
    int i;

    // if we fail to open one of these keys then we assume that all items under that
    // key still need to be completed.  This is based on the assumption that its better
    // to ask the user to do something twice then to never ask the user at all.
    if ( ERROR_SUCCESS == RegCreateKey(HKEY_CURRENT_USER, WELCOMESETUP, &hkey) )
    {
        for ( i=0; i < m_iItems; i++ )
        {
            DWORD dwType;
            DWORD dwData;
            DWORD dwSize = sizeof(dwData);
            if ( ERROR_SUCCESS==RegQueryValueEx( hkey, m_data[i].GetTitle(), NULL, &dwType, (LPBYTE)&dwData, &dwSize ))
            {
                // this menu item has an entry in the registry
                if ( dwType == REG_DWORD )
                {
                    // A non-zero value means show it in an alternate color, 0 means show it normal.
                    if ( dwData )
                    {
                        m_data[i].m_dwFlags |= WF_ALTERNATECOLOR;
                    }
                }
            }
        }
        RegCloseKey( hkey );
    }
}

// Invoke
//
// Launches item i from the array of available items.  Marked the item as executed so it
// will be grayed the next time you run welcome.
void CDataSource::Invoke( int i, HWND hwnd )
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
            if ( (ERROR_SUCCESS!=RegQueryValueEx( hkey, m_data[i].GetTitle(), NULL, &dwType, (LPBYTE)&dwData, &dwSize )) ||
                 (dwType != REG_DWORD) ||
                 ( 1 != dwData ) )
            {
                // if we get here then it means that:
                // 1.) The reg value doesn't exist yet -OR-
                // 2.) The reg value exists but doesn't contain a DWORD data type -OR-
                // 3.) The reg value exists and contains a DWORD but that DWORD is not 1 so we want to change it
                dwData = 2;
                RegSetValueEx(hkey, m_data[i].GetTitle(), NULL, REG_DWORD, (LPBYTE)&dwData, sizeof(dwData) );
            }
            RegCloseKey(hkey);
        }

        SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
#ifndef WINNT
        PlaySound( NULL, NULL, 0 );
#endif
        m_data[i].Invoke(hwnd);
        SetForegroundWindow(hwnd);

        m_data[i].m_dwFlags |= WF_ALTERNATECOLOR;
    }
}

#ifdef WINNT
// SetCheckboxState (WinNT version)
//
// This function checks the registry to see if welcome is still listed under the "explorer\tips\show"
// key.  It sets the checkbox state accordingly:
//  true  = checkbox is checked, welcome will run at startup
//  false = checkbox is unchecked, welcome won't run at startup
void CDataSource::SetCheckboxState()
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
            // the dword value of this key determine if we should run
            m_bCheckState = (dwData != 0);
        }
        RegCloseKey(hkey);
    }
}
#else
// SetCheckboxState (Win98 version)
//
// This function checks the registry to see if welcome is still listed under the run key.  It sets the
// checkbox state accordingly:
//  true  = checkbox is checked, welcome will run at startup
//  false = checkbox is unchecked, welcome won't run at startup
void CDataSource::SetCheckboxState()
{
    HKEY hkey;

    // We initialize m_bCheckState to false.  As a result, if any of the below commands should fail
    // then we know m_bCheckState already has the correct value.
    m_bCheckState = false;
    if ( ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, RUNKEY, 0, KEY_READ, &hkey) )
    {
        DWORD dwType;
        TCHAR szData[MAX_PATH];
        DWORD cbSize = sizeof(szData);
        if ( ERROR_SUCCESS == RegQueryValueEx( hkey, RUNVALUENAME, NULL, &dwType, (LPBYTE)szData, &cbSize ) )
        {
            // we check only for the existance of the key, not it's value
            m_bCheckState = TRUE;
        }
        RegCloseKey(hkey);
    }
}
#endif

#ifdef WINNT
// Uninit (WinNT Version)
//
// Simply update the registry with the on/off value (dwData)
void CDataSource::Uninit(DWORD dwData)
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
        RegCloseKey(hkey);
    }
}
#else
// Uninit (Win98 Version)
//
// Either create or delete a key under the run key based on dwData
void CDataSource::Uninit(DWORD dwData)
{
    HKEY hkey;
    LONG result;

    result = RegCreateKeyEx(
                    HKEY_LOCAL_MACHINE,
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
        if ( dwData )
        {
            // create the link
            TCHAR szData[] = TEXT("welcome.exe");

            RegSetValueEx( hkey, RUNVALUENAME, NULL, REG_SZ, (LPBYTE)szData, sizeof(szData) );
        }
        else
        {
            // delete the key
            RegDeleteValue(hkey, RUNVALUENAME);
        }
        RegCloseKey(hkey);
    }
}
#endif