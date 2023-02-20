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
DownloadApplication(CAppInfo *pAppInfo);
BOOL
DownloadListOfApplications(const CAtlList<CAppInfo *> &AppsList, BOOL bIsModal);
