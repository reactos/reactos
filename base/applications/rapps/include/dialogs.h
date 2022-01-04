#pragma once

#include "available.h"

#include <windef.h>
#include <atlsimpcoll.h>

// Settings dialog (settingsdlg.cpp)
VOID CreateSettingsDlg(HWND hwnd);

//Main window
VOID MainWindowLoop(INT nShowCmd);

// Download dialogs
VOID DownloadApplicationsDB(LPCWSTR lpUrl, BOOL IsOfficial);
BOOL DownloadApplication(CAvailableApplicationInfo* pAppInfo);
BOOL DownloadListOfApplications(const ATL::CSimpleArray<CAvailableApplicationInfo>& AppsList, BOOL bIsModal);
