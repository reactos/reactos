#ifndef __TNMISC_H
#define __TNMISC_H

// Process-related functions
BOOL CreateHiddenConsoleProcess(LPCTSTR szChildName, PROCESS_INFORMATION* ppi, 
                                LPHANDLE phInWrite, LPHANDLE phOutRead,
                                LPHANDLE phErrRead);
BOOL SpawnProcess(char *cmd_line, PROCESS_INFORMATION *pi);

int GetWin32Version(void);

HWND GetConsoleWindow();

bool SetIcon(HWND hConsoleWindow, HANDLE hIcon, LPARAM *pOldBIcon, LPARAM *pOldSIcon,
			 const char *icondir);
void ResetIcon(HWND hConsoleWindow, LPARAM oldBIcon, LPARAM oldSIcon);

#endif
