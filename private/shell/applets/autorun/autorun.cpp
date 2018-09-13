// autorun.cpp: implementation of the CDataSource class for the welcome applet.
//
//////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <ntverp.h>
#include <winbase.h>    // for GetCommandLine
#include "autorun.h"
#include "resource.h"

#define ARRAYSIZE(x)    (sizeof(x)/sizeof(x[0]))
#define MAJOR           (5)                             // hardcoded unfortunately
#define MINOR           (0)                             // hardcoded unfortunately
#define BUILD           (VER_PRODUCTBUILD)              // defined in ntverp.h


// I'm doing my own version of these functions because they weren't in win95.
// These come from shell\shlwapi\strings.c.

#ifdef UNIX

#ifdef BIG_ENDIAN
#define READNATIVEWORD(x) MAKEWORD(*(char*)(x), *(char*)((char*)(x) + 1))
#else 
#define READNATIVEWORD(x) MAKEWORD(*(char*)((char*)(x) + 1), *(char*)(x))
#endif

#else

#define READNATIVEWORD(x) (*(UNALIGNED WORD *)x)

#endif

/*
 * ChrCmp -  Case sensitive character comparison for DBCS
 * Assumes   w1, wMatch are characters to be compared
 * Return    FALSE if they match, TRUE if no match
 */
__inline BOOL ChrCmpA_inline(WORD w1, WORD wMatch)
{
    /* Most of the time this won't match, so test it first for speed.
    */
    if (LOBYTE(w1) == LOBYTE(wMatch))
    {
        if (IsDBCSLeadByte(LOBYTE(w1)))
        {
            return(w1 != wMatch);
        }
        return FALSE;
    }
    return TRUE;
}

/*
 * StrRChr - Find last occurrence of character in string
 * Assumes   lpStart points to start of string
 *           lpEnd   points to end of string (NOT included in search)
 *           wMatch  is the character to match
 * returns ptr to the last occurrence of ch in str, NULL if not found.
 */
LPSTR StrRChr(LPCSTR lpStart, LPCSTR lpEnd, WORD wMatch)
{
    LPCSTR lpFound = NULL;

    ASSERT(lpStart);
    ASSERT(!lpEnd || lpEnd <= lpStart + lstrlenA(lpStart));

    if (!lpEnd)
        lpEnd = lpStart + lstrlenA(lpStart);

    for ( ; lpStart < lpEnd; lpStart = AnsiNext(lpStart))
    {
        // (ChrCmp returns FALSE when characters match)

        if (!ChrCmpA_inline(READNATIVEWORD(lpStart), wMatch))
            lpFound = lpStart;
    }
    return ((LPSTR)lpFound);
}

/*
 * StrChr - Find first occurrence of character in string
 * Assumes   lpStart points to start of null terminated string
 *           wMatch  is the character to match
 * returns ptr to the first occurrence of ch in str, NULL if not found.
 */
LPSTR _StrChrA(LPCSTR lpStart, WORD wMatch, BOOL fMBCS)
{
    if (fMBCS) {
        for ( ; *lpStart; lpStart = AnsiNext(lpStart))
        {
            if (!ChrCmpA_inline(READNATIVEWORD(lpStart), wMatch))
                return((LPSTR)lpStart);
        }
    } else {
        for ( ; *lpStart; lpStart++)
        {
            if ((BYTE)*lpStart == LOBYTE(wMatch)) {
                return((LPSTR)lpStart);
            }
        }
    }
    return (NULL);
}

LPSTR StrChr(LPCSTR lpStart, WORD wMatch)
{
    CPINFO cpinfo;
    return _StrChrA(lpStart, wMatch, GetCPInfo(CP_ACP, &cpinfo) && cpinfo.LeadByte[0]);
}


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDataSource::CDataSource()
{
    m_iItems = 0;
}

CDataSource::~CDataSource()
{
}

CDataItem & CDataSource::operator[](int i)
{
    return m_data[i];
}

/*
    10.05.96    Shunichi Kajisa (shunk)     Support NEC PC-98

    1. Determine if autorun is running on PC-98 or regular PC/AT by:

	    bNEC98 = (HIBYTE(LOWORD(GetKeyboardType(1))) == 0x0D)? TRUE : FALSE;

        Following description is from KB Q130054, and this can be applied on NT and Win95:

	    If an application uses the GetKeyboardType API, it can get OEM ID by
	    specifying "1" (keyboard subtype) as argument of the function. Each OEM ID
	    is listed here:
	     
	       OEM Windows       OEM ID
	       ------------------------------
	       Microsoft         00H (DOS/V)
	       ....
	       NEC               0DH

 
    2. If autorun is running on PC-98, replace every "I386" resource with "PC98" at runtime,
       regardless that autorun is running on NT or Win95.


    Notes:
    - NEC PC-98 is available only in Japan.
    - NEC PC-98 uses x86 processor, but the underlaying hardware architecture is different.
      The PC98 files is stored under CD:\pc98 directory instead of CD:\i386.
    - There was an idea that we should detect PC-98 in SHELL32.DLL, and treat PC98 as a different
      platform, like having [AutoRun.Pc98] section in NT CD's autorun.inf. We don't do this, since
      Win95 doesn't support this, and we don't want to introduce the apps incompatibility.
      In any case, if app has any dependency on the hardware and needs to do any special things,
      the app should detect the hardware and OS. This is separate issue from Autorun.exe.
    
*/
BOOL CDataSource::IsNec98()
{
    return ((GetKeyboardType(0) == 7) && ((GetKeyboardType(1) & 0xff00) == 0x0d00));
}

void PathRemoveFilespec( LPTSTR psz )
{
    TCHAR * pszT = StrRChr( psz, psz+lstrlen(psz)-1, TEXT('\\') );

    if (pszT)
        *(pszT+1) = NULL;
}

void PathAppend(LPTSTR pszPath, LPTSTR pMore)
{
    lstrcpy(pszPath+lstrlen(pszPath), pMore);
}

BOOL PathFileExists( LPTSTR pszPath )
{
    BOOL fResult = FALSE;
    DWORD dwErrMode;

    dwErrMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    fResult = ((UINT)GetFileAttributes(pszPath) != (UINT)-1);

    SetErrorMode(dwErrMode);

    return fResult;
}

// These defines lay out which menu items are in which slots
#define INSTALL_WINNT   0
#define LUANCH_ARP      1
#define BROWSE_CD       2
#define EXIT_AUTORUN    3

// Init
//
// For autorun we read all the items out of the resources.
bool CDataSource::Init()
{
    // read the text for the items from the resources
    HINSTANCE hinst = GetModuleHandle(NULL);
    TCHAR szModuleName[MAX_PATH];

    TCHAR szTitle[256];
    TCHAR szDesc[1024];
    TCHAR szMenu[256];
    TCHAR szConfig[MAX_PATH];
    TCHAR szArgs[MAX_PATH];

    GetModuleFileName(hinst, szModuleName, ARRAYSIZE(szModuleName));    // ex: "e:\i386\autorun.exe" or "e:\setup.exe"
    PathRemoveFilespec(szModuleName);                                   // ex: "e:\i386\" or "e:\"
    PathAppend(szModuleName, TEXT("winnt32.exe"));                      //

    if ( PathFileExists(szModuleName) )
    {
        // we were launched from the platform directory, use szModuleName as the winnt32 path
    }
    else
    {
        // we were launched from the root.  Append either "alpha", "i386", or "NEC98" to the path.
        SYSTEM_INFO si;

        PathRemoveFilespec(szModuleName);
        GetSystemInfo(&si);
        if ( si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ALPHA )
        {
            PathAppend(szModuleName, TEXT("alpha\\winnt32.exe"));
        }
        else if ( IsNec98() )
        {
            PathAppend(szModuleName, TEXT("nec98\\winnt32.exe"));
        }
        else
        {
            PathAppend(szModuleName, TEXT("i386\\winnt32.exe"));
        }
    }

    m_iItems = 4;
    for (int i=0; i<m_iItems;i++)
    {
        LoadString(hinst, IDS_TITLE0+i, szTitle, ARRAYSIZE(szTitle));
        LoadString(hinst, IDS_MENU0+i, szMenu, ARRAYSIZE(szMenu));
        LoadString(hinst, IDS_DESC0+i, szDesc, ARRAYSIZE(szDesc));

        // for INSTALL_WINNT we prepend the correct path to winnt32 in front of the string
        if ( INSTALL_WINNT == i )
        {
            lstrcpy( szConfig, szModuleName );
            if ( !PathFileExists(szModuleName) )
            {
                // we can't run the item if it's not there.  This will prevent an
                // alpha CD from trying to install on an x86 and vice versa.
                m_data[INSTALL_WINNT].m_dwFlags |= WF_DISABLED|WF_ALTERNATECOLOR;
            }
        }
        else
        {
            LoadString(hinst, IDS_CONFIG0+i, szConfig, ARRAYSIZE(szConfig));
        }

        // for BROWSE_CD we pass the directory as an argument to explorer.exe
        if ( BROWSE_CD == i )
        {
            lstrcpy( szArgs, szModuleName );
            PathRemoveFilespec( szArgs );
            PathRemoveFilespec( szArgs );
        }
        else
        {
            LoadString(hinst, IDS_ARGS0+i, szArgs, ARRAYSIZE(szArgs));
        }

        m_data[i].SetData( szTitle, szMenu, szDesc, szConfig, *szArgs?szArgs:NULL, 0, (i+1)%4 );
    }

    // Should we display the "This CD contains a newer version" dialog?
	OSVERSIONINFO ovi;
	ovi.dwOSVersionInfoSize = sizeof ( OSVERSIONINFO );
    if ( !GetVersionEx(&ovi) || ovi.dwPlatformId==VER_PLATFORM_WIN32s )
    {
        // We cannot upgrade win32s systems.
        m_Version = VER_INCOMPATIBLE;
    }
    else if ( ovi.dwPlatformId==VER_PLATFORM_WIN32_WINDOWS )
    {
        // we can always upgrade win9x systems to NT
        m_Version = VER_OLDER;
        
        // Disable ARP.  ARP is only enabled if the CD and the OS are the same version
        m_data[LUANCH_ARP].m_dwFlags    |= WF_DISABLED|WF_ALTERNATECOLOR;
    }
    else if ( MAJOR>ovi.dwMajorVersion || (MAJOR==ovi.dwMajorVersion && (MINOR>ovi.dwMinorVersion || (MINOR==ovi.dwMinorVersion && (BUILD > ovi.dwBuildNumber )))) )
    {
        // For NT to NT upgrades, we only upgrade if the version is lower

        // For NT 3.51 we have some special case code
        if ( ovi.dwMajorVersion == 3 )
        {
            // must be at least NT 3.51
            if ( ovi.dwMinorVersion >= 51 )
            {
                // Explorer doesn't exist on NT 3.51 so don't try to launch it.  I don't
                // want to both with trying to launch "winfile.exe" since it doesn't
                // accept a path as an argument.
                m_data[BROWSE_CD].m_dwFlags    |= WF_DISABLED|WF_ALTERNATECOLOR;
            }
            else
            {
                // On NT 3.1 we might be able to launch winnt32.exe
                STARTUPINFO sinfo =
                {
                    sizeof(STARTUPINFO),
                };
                PROCESS_INFORMATION pinfo;
                CreateProcess(NULL,szModuleName,NULL,NULL,FALSE,0,NULL,NULL,&sinfo,&pinfo);

                return FALSE;
            }
        }

        m_Version = VER_OLDER;
        
        // Disable ARP.  ARP is only enabled if the CD and the OS are the same version
        m_data[LUANCH_ARP].m_dwFlags    |= WF_DISABLED|WF_ALTERNATECOLOR;
    }
    else if ( MAJOR<ovi.dwMajorVersion || MINOR<ovi.dwMinorVersion || BUILD<ovi.dwBuildNumber )
    {
        m_Version = VER_NEWER;

        // disable upgrade and ARP buttons
        m_data[INSTALL_WINNT].m_dwFlags |= WF_DISABLED|WF_ALTERNATECOLOR;
        m_data[LUANCH_ARP].m_dwFlags    |= WF_DISABLED|WF_ALTERNATECOLOR;
    }
    else
    {
        m_Version = VER_SAME;
    }

    return true;
}

void CDataSource::Invoke( int i, HWND hwnd )
{
    // if this item is disalbled then do nothing
    if ( m_data[i].m_dwFlags & WF_DISABLED )
    {
        MessageBeep(0);
        return;
    }

    // otherwise we have already built the correct command and arg strings so just invoke them
    switch ( i )
    {
    case INSTALL_WINNT:
    case LUANCH_ARP:
    case BROWSE_CD:
        m_data[i].Invoke(hwnd);
        break;
    case EXIT_AUTORUN:
        DestroyWindow( m_hwndDlg );
        PostQuitMessage( 0 );
        break;
    default:
        // Assert?  Debug trace message?
        break;
    }
}

// Uninit
//
// This is a chance to do any required shutdown stuff, such as persisting state information.
void CDataSource::Uninit(DWORD dwData)
{
}

// ShowSplashScreen
//
// This hook is provided to allow the display of additional UI right after the main window is diaplyed.
// In our case we want to show a dialog asking if the user wants to upgrade.
void CDataSource::ShowSplashScreen(HWND hwnd)
{
    // Should we display the "This CD contains a newer/older version" dialog?

    TCHAR szTitle[MAX_PATH];
    TCHAR szMessage[1024];
    HINSTANCE hinst = GetModuleHandle(NULL);
    
    m_hwndDlg = hwnd;

    if ( VER_NEWER == m_Version)
    {
        // the os is a newer version than the CD
        // display a dialog saying that the CD contains an older version so you can't upgrade.
        LoadString(hinst,IDS_OLDCDROM, szMessage, ARRAYSIZE(szMessage));
        LoadString(hinst,IDS_TITLE, szTitle, ARRAYSIZE(szTitle));
        MessageBox(hwnd, szMessage, szTitle, MB_OK | MB_ICONEXCLAMATION);
    }
    else if ( VER_OLDER == m_Version )
    {
        // the os is older than the CD
        int cmd;
        LoadString(hinst,IDS_NEWCDROM, szMessage, ARRAYSIZE(szMessage));
        LoadString(hinst,IDS_TITLE, szTitle, ARRAYSIZE(szTitle));
        cmd = MessageBox(hwnd, szMessage, szTitle, MB_YESNO | MB_ICONQUESTION);
        if ( IDYES == cmd )
        {
            // upgrade by invoking item 0.  Pass a NULL hwnd so we don't wait in the inner
            // loop, instead we will go ahead and exit after launching winnt32.
            m_data[0].Invoke(NULL);

            // Now we're done so we exit
            DestroyWindow(hwnd);
        }
    }
}
