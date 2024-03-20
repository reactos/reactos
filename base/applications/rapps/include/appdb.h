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

    static CInstalledApplicationInfo *
    EnumerateRegistry(CAtlList<CAppInfo *> *List, LPCWSTR Name);
    static CInstalledApplicationInfo *
    CreateInstalledAppByRegistryKey(LPCWSTR KeyName, HKEY hKeyParent, UINT KeyIndex);

  public:
    CAppDB(const CStringW &path);

    VOID
    GetApps(CAtlList<CAppInfo *> &List, AppsCategories Type) const;
    CAvailableApplicationInfo *
    FindAvailableByPackageName(const CStringW &name);
    CAppInfo *
    FindByPackageName(const CStringW &name) { return FindAvailableByPackageName(name); }

    VOID
    UpdateAvailable();
    VOID
    UpdateInstalled();
    VOID
    RemoveCached();

    static DWORD
    RemoveInstalledAppFromRegistry(const CAppInfo *Info);

    static CInstalledApplicationInfo *
    CreateInstalledAppByRegistryKey(LPCWSTR Name);
    static CInstalledApplicationInfo *
    CreateInstalledAppInstance(LPCWSTR KeyName, BOOL User, REGSAM WowSam);
    static HKEY
    EnumInstalledRootKey(UINT Index, REGSAM &RegSam);

    size_t GetAvailableCount() const
    {
        return m_Available.GetCount();
    }
};
