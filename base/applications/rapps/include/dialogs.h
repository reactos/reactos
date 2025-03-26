#pragma once

#include "appinfo.h"

#include <atlsimpcoll.h>

// Settings dialog (settingsdlg.cpp)
VOID
CreateSettingsDlg(HWND hwnd);

// Download dialogs
VOID
DownloadApplicationsDB(LPCWSTR lpUrl, BOOL IsOfficial);
BOOL
DownloadApplication(CAppInfo *pAppInfo);
BOOL
DownloadListOfApplications(const CAtlList<CAppInfo *> &AppsList, BOOL bIsModal);
