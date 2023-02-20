#pragma once

#include "appinfo.h"

#include <windef.h>
#include <atlsimpcoll.h>

// Settings dialog (settingsdlg.cpp)
VOID
CreateSettingsDlg(HWND hwnd);

// Main window
VOID
MainWindowLoop(class CAppDB *db, INT nShowCmd);

// Download dialogs
VOID
DownloadApplicationsDB(LPCWSTR lpUrl, BOOL IsOfficial);
BOOL
DownloadApplication(CApplicationInfo *pAppInfo);
BOOL
DownloadListOfApplications(const CAtlList<CApplicationInfo *> &AppsList, BOOL bIsModal);
