#pragma once

#include "applicationinfo.h"

#include <windef.h>
#include <atlsimpcoll.h>

// Settings dialog (settingsdlg.cpp)
VOID
CreateSettingsDlg(HWND hwnd);

// Main window
VOID
MainWindowLoop(class CApplicationDB *db, INT nShowCmd);

// Download dialogs
VOID
DownloadApplicationsDB(LPCWSTR lpUrl, BOOL IsOfficial);
BOOL
DownloadApplication(CApplicationInfo *pAppInfo);
BOOL
DownloadListOfApplications(const CAtlList<CApplicationInfo *> &AppsList, BOOL bIsModal);
