#include <windows.h>
#include <stdlib.h>
#include <memory.h>

#include "tnmisc.h"

// from the PVAX (http://www.ccas.ru/~posp/popov/spawn.htm)
// Create a process with pipes to stdin/out/err
BOOL CreateHiddenConsoleProcess(LPCTSTR szChildName, PROCESS_INFORMATION* ppi, 
                                LPHANDLE phInWrite, LPHANDLE phOutRead,
                                LPHANDLE phErrRead) {
    BOOL fCreated;
    STARTUPINFO si;
    SECURITY_ATTRIBUTES sa;
    HANDLE hInRead;
    HANDLE hOutWrite;
    HANDLE hErrWrite;

    // Create pipes
    // initialize security attributes for handle inheritance (for WinNT)
    sa.nLength = sizeof( sa );
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor  = NULL;

    // create STDIN pipe
    if( !CreatePipe( &hInRead, phInWrite, &sa, 0 ))
        goto error;

    // create STDOUT pipe
    if( !CreatePipe( phOutRead, &hOutWrite, &sa, 0 ))
        goto error;

    // create STDERR pipe
    if( !CreatePipe( phErrRead, &hErrWrite, &sa, 0 ))
        goto error;

    // process startup information
    memset( &si, 0, sizeof( si ));
    si.cb = sizeof( si ); 
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    // child process' console must be hidden for Win95 compatibility
    si.wShowWindow = SW_HIDE;
    // assign "other" sides of pipes
    si.hStdInput = hInRead;
    si.hStdOutput = hOutWrite;
    si.hStdError = hErrWrite;

    // Create a child process (suspended)
    fCreated = CreateProcess( NULL,
                              (LPTSTR)szChildName,
                              NULL,
                              NULL,
                              TRUE,
                              0,
                              NULL,
                              NULL,
                              &si,
                              ppi );

    if( !fCreated )
        goto error;

    CloseHandle( hInRead );
    CloseHandle( hOutWrite );
    CloseHandle( hErrWrite );

    return TRUE;

error:
    CloseHandle( hInRead );
    CloseHandle( hOutWrite );
    CloseHandle( hErrWrite );
    CloseHandle( ppi->hProcess );
    CloseHandle( ppi->hThread );
    
    hInRead =
    hOutWrite =
    hErrWrite =
    ppi->hProcess =
    ppi->hThread = INVALID_HANDLE_VALUE;

    return FALSE;
}

BOOL SpawnProcess(char *cmd_line, PROCESS_INFORMATION *pi) {
	STARTUPINFO si;

	memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);
		
	return CreateProcess(cmd_line, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS,
	                     CREATE_NEW_CONSOLE, NULL, NULL, &si, pi);
}

// crn@ozemail.com.au
int GetWin32Version(void) {
	// return win32 version; 0 = Win32s, 1 = Win95, 2 = WinNT, 3 = Unknown -crn@ozemail.com.au
	LPOSVERSIONINFO osv;
	DWORD retval;
	
	osv = new OSVERSIONINFO;
	
	osv->dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
	GetVersionEx (osv);
	retval = osv->dwPlatformId;
	delete osv;
	return (retval);
}

// Paul Brannan 8/7/98
// This code is from Michael 'Hacker' Krelin (author of KINSole)
// (slightly modified)
HWND GetConsoleWindow() {
	DWORD pid = GetCurrentProcessId(), wpid;
	char title[512], *t = title;
	HWND hrv = NULL;

#ifndef __BORLANDC__	// Ioannou Dec. 8, 1998
	if(!GetConsoleTitle(title, sizeof(title))) t = NULL;

	for(;;) {
		if((hrv = FindWindowEx(NULL, hrv, "tty", t)) == NULL) break;
		if(!GetWindowThreadProcessId(hrv, &wpid)) continue;
		if(wpid == pid) return hrv;
	}
#endif

	return GetForegroundWindow();
}

// Sets the icon of the console window to hIcon
// If hIcon is 0, then use a default icon
// hConsoleWindow must be set before calling SetIcon
bool SetIcon(HWND hConsoleWindow, HANDLE hIcon, LPARAM *pOldBIcon, LPARAM *pOldSIcon,
			 const char *icondir) {
	if(!hConsoleWindow) return false;
	
// FIX ME!!! The LoadIcon code should work with any compiler!
// (Paul Brannan 12/17/98)
#ifndef __BORLANDC__ // Ioannou Dec. 8, 1998
	if(!hIcon) {
		char filename[128];					// load from telnet.ico
		strncpy(filename, icondir, sizeof(filename));
		strncat(filename, "telnet.ico", sizeof(filename));
		filename[sizeof(filename) - 1] = 0;

		// Note: loading the icon from a file doesn't work on NT
		// There is no LoadImage in Borland headers - only LoadIcon
		hIcon =	LoadImage(NULL, filename, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE +
			LR_LOADFROMFILE);
	}
#else	
	// load the icon from the resource file -crn@ozemail.com.au 16/12/98
	if(!hIcon) {
		hIcon = LoadIcon ((HANDLE)GetWindowLong(hConsoleWindow,
			GWL_HINSTANCE), "TELNETICON");
	}
#endif

	if(hIcon) {
#ifdef ICON_BIG
		*pOldBIcon = SendMessage(hConsoleWindow, WM_SETICON, ICON_BIG,
			(LPARAM)hIcon);
#endif
#ifdef ICON_SMALL
		*pOldSIcon = SendMessage(hConsoleWindow, WM_SETICON, ICON_SMALL,
			(LPARAM)hIcon);
#endif
		return true;
	} else {
		// Otherwise we get a random icon at exit! (Paul Brannan 9/13/98)
		return false;
	}
}

// Allows SetIcon to be called again by resetting the current icon
// Added 12/17/98 by Paul Brannan
void ResetIcon(HWND hConsoleWindow, LPARAM oldBIcon, LPARAM oldSIcon) {
#ifdef ICON_BIG
	SendMessage(hConsoleWindow, WM_SETICON, ICON_BIG, (LPARAM)oldBIcon);
#endif
#ifdef ICON_SMALL
	SendMessage(hConsoleWindow, WM_SETICON, ICON_SMALL, (LPARAM)oldSIcon);
#endif
}
