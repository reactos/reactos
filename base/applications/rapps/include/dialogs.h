#pragma once

#include "available.h"

#include <windef.h>
#include <atlsimpcoll.h>

// Settings dialog (settingsdlg.cpp)
VOID CreateSettingsDlg(HWND hwnd);

// About dialog (aboutdlg.cpp)
VOID ShowAboutDialog();

//Main window
VOID ShowMainWindow(INT nShowCmd);

// Download dialogs
VOID DownloadApplicationsDB(LPCWSTR lpUrl);
BOOL DownloadApplication(CAvailableApplicationInfo* pAppInfo, BOOL bIsModal);
BOOL DownloadListOfApplications(const ATL::CSimpleArray<CAvailableApplicationInfo>& AppsList, BOOL bIsModal);
