#pragma once

#include <atlcoll.h>
#include <atlpath.h>

#include "appinfo.h"

class CAppDB
{
  private:
    CPathW m_BasePath;
    CAtlList<CAppInfo *> m_Available;
    CAtlList<CAppInfo *> m_Installed;

    BOOL
    EnumerateFiles();

  public:
    CAppDB(const CStringW &path);

    VOID
    GetApps(CAtlList<CAppInfo *> &List, AppsCategories Type) const;
    CAppInfo *
    FindByPackageName(const CStringW &name);

    VOID
    UpdateAvailable();
    VOID
    UpdateInstalled();
    VOID
    RemoveCached();

    BOOL
    RemoveInstalledAppFromRegistry(const CAppInfo *Info);
};
