#pragma once

#include "available.h"

#include <windef.h>
#include <atlsimpcoll.h>

// Download dialog (loaddlg.cpp)
class CDowloadingAppsListView;

class CDownloadManager
{
    static PAPPLICATION_INFO AppInfo;
    static ATL::CSimpleArray<PAPPLICATION_INFO> AppsToInstallList;
    static CDowloadingAppsListView DownloadsListView;
    static INT iCurrentApp;

public:
    static INT_PTR CALLBACK DownloadDlgProc(HWND Dlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK DownloadProgressProc(HWND hWnd,
                                                 UINT uMsg,
                                                 WPARAM wParam,
                                                 LPARAM lParam,
                                                 UINT_PTR uIdSubclass,
                                                 DWORD_PTR dwRefData);

    static DWORD WINAPI ThreadFunc(LPVOID Context);
    static BOOL DownloadListOfApplications(const ATL::CSimpleArray<PAPPLICATION_INFO>& AppsList, BOOL bIsModal = FALSE);
    static BOOL DownloadApplication(PAPPLICATION_INFO pAppInfo, BOOL modal = FALSE);
    static VOID DownloadApplicationsDB(LPCWSTR lpUrl);
    static VOID LaunchDownloadDialog(BOOL);
};

// Settings dialog (settingsdlg.cpp)
VOID CreateSettingsDlg(HWND hwnd);

// About dialog (aboutdlg.cpp)
VOID ShowAboutDialog();

// Installation dialog (installdlg.cpp)
//BOOL InstallApplication(INT Index);
