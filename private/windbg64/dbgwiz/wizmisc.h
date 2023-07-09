#ifndef __WIZMISC_H__
#define __WIZMISC_H__

void
BrowseForDir(
    HWND hDlg,
    int nPath_EditCtrlId
    );

void
BrowseForFile(
    HWND hDlg,
    PSTR pszInitialDir,
    PSTR pszFileFilter,
    int nPath_EditCtrlId
    );

BOOL
SimpleBrowseForFile(
    HWND hDlg,
    PSTR pszPath,
    DWORD dwPathSize,
    PSTR pszInitialDir,
    PSTR pszFileFilter
    );

void
SaveFileDlg(
    HWND hDlg,
    PSTR pszPath,
    DWORD dwPathSize,
    PSTR pszInitialDir,
    PSTR pszFileFilter,
    int nPath_EditCtrlId
    );

BOOL
SimpleSaveFileDlg(
    HWND hDlg,
    PSTR pszPath,
    DWORD dwPathSize,
    PSTR pszInitialDir,
    PSTR pszFileFilter
    );


void
GetSummaryInfo(
    char * const pszBuffer,
    UINT uSize
    );


PSTR
CreateLinkOnDesktop(HWND hwnd, PSTR pszShortcutName, BOOL bCreateLink);

void
DeleteLinkOnDesktop(HWND hwnd, PSTR pszDeskTopShortcut_FullPath);

void
SpawnMunger();


void
EnumComPorts(PCOMPORT_INFO pComInfo, DWORD dwSizeOfArray, DWORD & dwNumComPortsFound);



BOOL
CreateDirTree(PCSTR pszPath);

BOOL
CreateDirIfNotExist(PCSTR pszDir);

void
CreateUniqueDestPath();


void
StripWhiteSpace(char * const pszStart);


void
GuessAtSymSrc();


void
CleanupAfterWizard(HWND);


BOOL
GetDiskInfo(PSTR pszRoot, PSTR & pszVolumeLabel, DWORD & dwSerialNum, UINT & uDriveType);


BOOL
AreAnyOfTheFilesCompressed(BOOL bWarnUser, PSTR pszPath);

PTSTR
NT4SafeStrDup(PCTSTR);

#endif

