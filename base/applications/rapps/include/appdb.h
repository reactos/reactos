#pragma once

#include <atlcoll.h>
#include <atlpath.h>

#include "applicationinfo.h"

class CApplicationDB
{
  private:
    CPathW m_BasePath;
    CAtlList<CApplicationInfo *> m_Available;
    CAtlList<CApplicationInfo *> m_Installed;

    BOOL
    EnumerateFiles();

  public:
    CApplicationDB(const CStringW &path);

    VOID
    GetApps(CAtlList<CApplicationInfo *> &List, AppsCategories Type) const;
    CApplicationInfo *
    FindByPackageName(const CStringW &name);

    VOID
    UpdateAvailable();
    VOID
    UpdateInstalled();
    VOID
    RemoveCached();

    BOOL
    RemoveInstalledAppFromRegistry(const CApplicationInfo *Info);
};
