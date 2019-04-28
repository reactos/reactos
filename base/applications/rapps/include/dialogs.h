#pragma once

#include "available.h"

#include <windef.h>
#include <atlsimpcoll.h>

// Download dialog (loaddlg.cpp)
class CDowloadingAppsListView;
struct DownloadInfo;

class CDownloadManager
{
    static ATL::CSimpleArray<DownloadInfo> AppsToInstallList;
    static CDowloadingAppsListView DownloadsListView;

    static VOID Download(const DownloadInfo& DLInfo, BOOL bIsModal = FALSE);
    static VOID SetProgressMarquee(HWND Item, BOOL Enable);

public:
    static INT_PTR CALLBACK DownloadDlgProc(HWND Dlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK DownloadProgressProc(HWND hWnd,
                                                 UINT uMsg,
                                                 WPARAM wParam,
                                                 LPARAM lParam,
                                                 UINT_PTR uIdSubclass,
                                                 DWORD_PTR dwRefData);

    static DWORD WINAPI ThreadFunc(LPVOID Context);
    static BOOL DownloadListOfApplications(const ATL::CSimpleArray<CAvailableApplicationInfo>& AppsList, BOOL bIsModal = FALSE);
    static BOOL DownloadApplication(CAvailableApplicationInfo* pAppInfo, BOOL bIsModal = FALSE);
    static VOID DownloadApplicationsDB(LPCWSTR lpUrl);
    static VOID LaunchDownloadDialog(BOOL);
};

// Settings dialog (settingsdlg.cpp)
VOID CreateSettingsDlg(HWND hwnd);

// About dialog (aboutdlg.cpp)
VOID ShowAboutDialog();

VOID ShowMainWindow(INT nShowCmd);
