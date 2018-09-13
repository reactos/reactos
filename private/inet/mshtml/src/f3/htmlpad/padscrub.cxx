//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       scrub.cxx
//
//  Contents:   Function to delete dangling references from the system registry.
//
//----------------------------------------------------------------------------

#include "padhead.hxx"

#ifndef X_STDIO_H_
#define X_STDIO_H_
#include "stdio.h"
#endif

char g_achScrub[2048];

static BOOL
OkToDelete(char *pch)
{
    static BOOL g_fAlwaysDelete;
    int iRet;

    if (g_fAlwaysDelete)
        return TRUE;

    wsprintfA(&g_achScrub[lstrlenA(g_achScrub)], "\nDelete %s from the registry?", pch);

    iRet = MessageBoxA(g_hwndMain, g_achScrub, "Scrub Registry", MB_APPLMODAL | MB_YESNO);

    if (iRet != IDYES)
        return FALSE;

    if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
        g_fAlwaysDelete = TRUE;

    return TRUE;
}

static void
DeleteKey(HKEY hkeyParent, char *pstrDelete)
{
    char    achSubKey[MAX_PATH];
    HKEY    hkeyDelete;

    if (RegOpenKeyA(hkeyParent, pstrDelete, &hkeyDelete) == ERROR_SUCCESS)
    {
        while (RegEnumKeyA(hkeyDelete, 0, achSubKey, sizeof(achSubKey))
                == ERROR_SUCCESS)
        {
            DeleteKey(hkeyDelete, achSubKey);
        }

        RegCloseKey(hkeyDelete);
    }
    RegDeleteKeyA(hkeyParent, pstrDelete);
}

static void
CheckCLSIDs(HKEY hkeyCLSID)
{
    char    achServer[MAX_PATH];
    char    achClass[MAX_PATH];
    char    achProgID[MAX_PATH];
    char    achPath[MAX_PATH];
    char    achUserType[MAX_PATH];
    HKEY    hkeyClass = NULL;
    DWORD   i;
    long    cb;

    // For each class....

    i = 0;
    while (RegEnumKeyA(hkeyCLSID,
            i,
            achClass,
            sizeof(achClass)) == ERROR_SUCCESS)
    {
        if (hkeyClass)
        {
            Verify(RegCloseKey(hkeyClass)  == ERROR_SUCCESS);
            hkeyClass = NULL;
        }

        if (RegOpenKeyA(hkeyCLSID, achClass, &hkeyClass) == ERROR_SUCCESS)
        {
            // If we can't find the server for a class....

            cb = sizeof(achServer);
            if (RegQueryValueA(hkeyClass, INPROCSERVER, achServer, &cb)
                    == ERROR_SUCCESS &&
                achServer[0] &&         // Ignore empty strings.
                achServer[0] != '"' &&  // Ole2View registers with quotes. Ignore.
                SearchPathA(NULL, achServer, NULL, sizeof(achPath), achPath, NULL)
                    == 0)
            {
                // Find ProgID and user type name.

                cb = sizeof(achProgID);
                if (RegQueryValueA(hkeyClass, "ProgID", achProgID, &cb)
                        != ERROR_SUCCESS)
                {
                    achProgID[0] = 0;
                    achUserType[0] = 0;
                }
                else
                {
                    cb = sizeof(achUserType);
                    if (RegQueryValueA(HKEY_CLASSES_ROOT, achProgID, achUserType, &cb)
                            != ERROR_SUCCESS)
                    {
                        achUserType[0] = 0;
                    }
                }

                // If we couldn't get a user type name from the ProgID, then
                // try to fetch one from the class id key.

                if (achUserType[0] == 0)
                {
                    cb = sizeof(achUserType);
                    if (RegQueryValueA(hkeyCLSID, achClass, achUserType, &cb)
                            != ERROR_SUCCESS)
                    {
                        achUserType[0] = 0;
                    }
                }

                // Ask the user about deleting the class.

                wsprintfA(g_achScrub, "Server not found for CLSID\n\n"
                    "  Name   = %s\n"
                    "  Server = %s\n"
                    "  CLSID  = %s\n"
                    "  ProgID = %s\n",
                    achUserType,
                    achServer,
                    achClass,
                    achProgID);
                if (OkToDelete("class"))
                {
                    Verify(RegCloseKey(hkeyClass)  == ERROR_SUCCESS);
                    hkeyClass = NULL;
                    DeleteKey(hkeyCLSID, achClass);
                    if (achProgID[0])
                        DeleteKey(HKEY_CLASSES_ROOT, achProgID);
                    continue;
                }
            }
        }
        i += 1;
    }

    if (hkeyClass)
    {
        Verify(RegCloseKey(hkeyClass)  == ERROR_SUCCESS);
    }
}

static void
CheckProgIDs(HKEY hkeyCLSID)
{
    char    achClass[MAX_PATH];
    char    achProgID[MAX_PATH];
    char    achUserType[MAX_PATH];
    HKEY    hkeyProgID = NULL;
    HKEY    hkeyClass;
    DWORD   i;
    long    cb;

    // For each ProgID....

    i = 0;
    while (RegEnumKeyA(HKEY_CLASSES_ROOT,
            i,
            achProgID,
            sizeof(achProgID)) == ERROR_SUCCESS)
    {
        if (hkeyProgID)
        {
            Verify(RegCloseKey(hkeyProgID)  == ERROR_SUCCESS);
            hkeyProgID = NULL;
        }

        if (RegOpenKeyA(HKEY_CLASSES_ROOT, achProgID, &hkeyProgID) == ERROR_SUCCESS)
        {
            // If there's a class id...

            cb = sizeof(achClass);
            if (RegQueryValueA(hkeyProgID, "CLSID", achClass, &cb)
                    == ERROR_SUCCESS)
            {
                if (achClass[0] && RegOpenKeyA(hkeyCLSID, achClass, &hkeyClass) == ERROR_SUCCESS)
                {
                    RegCloseKey(hkeyClass);
                    hkeyClass = NULL;
                }
                else
                {
                    // We can't find the class id in the registry, so ask about deleting it.

                    cb = sizeof(achUserType);
                    if (RegQueryValueA(HKEY_CLASSES_ROOT, achProgID, achUserType, &cb)
                            != ERROR_SUCCESS)
                    {
                        achUserType[0] = 0;
                    }

                    wsprintfA(g_achScrub, "CLSID not found for ProgID:\n\n"
                        "  Name   = %s\n"
                        "  CLSID  = %s\n"
                        "  ProgID = %s\n",
                        achUserType,
                        achClass,
                        achProgID);
                    if (OkToDelete("ProgID"))
                    {
                        RegCloseKey(hkeyProgID);
                        hkeyProgID = NULL;
                        DeleteKey(HKEY_CLASSES_ROOT, achProgID);
                        continue;
                    }
                }
            }

        }
        i += 1;
    }

    if (hkeyProgID)
    {
        Verify(RegCloseKey(hkeyProgID)  == ERROR_SUCCESS);
    }
}

static BOOL
CheckTypeLibraryPlatforms(
    HKEY hkeyVersion,
    char *pchTypeLib,
    char *pchVersion,
    char *pchName,
    char *pchLanguage)
{
    char    achFile[MAX_PATH];
    char    achPath[MAX_PATH];
    char    achPlatform[MAX_PATH];
    HKEY    hkeyLanguage;
    BOOL    fDelete = TRUE;
    long    cb;
    int     i;

    if (RegOpenKeyA(hkeyVersion, pchLanguage, &hkeyLanguage) != ERROR_SUCCESS)
        return FALSE;

    i = 0;
    while (RegEnumKeyA(hkeyLanguage,
            i,
            achPlatform,
            sizeof(achPlatform)) == ERROR_SUCCESS)
    {
        cb = sizeof(achFile);
        if (RegQueryValueA(hkeyLanguage, achPlatform, achFile, &cb)
                == ERROR_SUCCESS &&
            SearchPathA(NULL, achFile, NULL, sizeof(achPath), achPath, NULL)
                == 0)
        {
            wsprintfA(g_achScrub, "Type library file not found:\n\n"
                "  Name     = %s\n"
                "  TLID     = %s\n"
                "  Version  = %s\n"
                "  Language = %s\n"
                "  Platform = %s\n"
                "  File     = %s\n",
                pchName,
                pchTypeLib,
                pchVersion,
                pchLanguage,
                achPlatform,
                achFile);
            if (OkToDelete("platform entry"))
            {
                DeleteKey(hkeyLanguage, achPlatform);
                continue;
            }
            else
            {
                fDelete = FALSE;
            }
        }
        else
        {
            fDelete = FALSE;
        }
        i += 1;
    }

    RegCloseKey(hkeyLanguage);

    return fDelete;
}

static BOOL
CheckTypeLibraryLanguages(
    HKEY hkeyTypeLib,
    char *pchTypeLib,
    char *pchName,
    char *pchVersion)
{
    char    achLanguage[MAX_PATH];
    HKEY    hkeyVersion;
    BOOL    fDelete = TRUE;
    int     i;

    if (RegOpenKeyA(hkeyTypeLib, pchVersion, &hkeyVersion) != ERROR_SUCCESS)
        return FALSE;

    i = 0;
    while (RegEnumKeyA(hkeyVersion,
            i,
            achLanguage,
            sizeof(achLanguage)) == ERROR_SUCCESS)
    {
        if ('0' <= achLanguage[0] && achLanguage[0] <= '9')
        {
            if (CheckTypeLibraryPlatforms(
                        hkeyVersion,
                        pchTypeLib,
                        pchVersion,
                        pchName,
                        achLanguage))
            {
                wsprintfA(g_achScrub, "No entries for type library language found:\n\n"
                    "  Name     = %s\n"
                    "  TLID     = %s\n"
                    "  Version  = %s\n"
                    "  Language = %s\n",
                    pchName,
                    pchTypeLib,
                    pchVersion,
                    achLanguage);
                if (OkToDelete("language entry"))
                {
                    DeleteKey(hkeyVersion, achLanguage);
                    continue;
                }
                else
                {
                    fDelete = FALSE;
                }
            }
            else
            {
                fDelete = FALSE;
            }
        }
        i += 1;
    }

    RegCloseKey(hkeyVersion);

    return fDelete;
}

static BOOL
CheckTypeLibraryVersions(
    HKEY hkeyTypeLibRoot,
    char *pchTypeLib,
    char *pchName)
{
    char    achVersion[MAX_PATH];
    HKEY    hkeyTypeLib;
    BOOL    fDelete = TRUE;
    int     i;
    long    cb;

    if (RegOpenKeyA(hkeyTypeLibRoot, pchTypeLib, &hkeyTypeLib) != ERROR_SUCCESS)
        return FALSE;

    i = 0;
    while (RegEnumKeyA(hkeyTypeLib,
            i,
            achVersion,
            sizeof(achVersion)) == ERROR_SUCCESS)
    {
        cb = MAX_PATH;
        if (RegQueryValueA(hkeyTypeLib, achVersion, pchName, &cb)
                != ERROR_SUCCESS)
        {
            pchName[0] = 0;
        }

        if (CheckTypeLibraryLanguages(hkeyTypeLib, pchTypeLib, pchName, achVersion))
        {
            wsprintfA(g_achScrub, "No entries for type library version found:\n\n"
                "  Name     = %s\n"
                "  TLID     = %s\n"
                "  Version  = %s\n",
                pchName,
                pchTypeLib,
                achVersion);
            if (OkToDelete("version entry"))
            {
                DeleteKey(hkeyTypeLib, achVersion);
                continue;
            }
            else
            {
                fDelete = FALSE;
            }
        }
        else
        {
            fDelete = FALSE;
        }
        i += 1;
    }

    RegCloseKey( hkeyTypeLib );

    return fDelete;
}

static void
CheckTypeLibraries()
{
    char    achTypeLib[MAX_PATH];
    char    achName[MAX_PATH];
    HKEY    hkeyTypeLibRoot;
    int i;

    RegOpenKeyA(HKEY_CLASSES_ROOT, "TypeLib", &hkeyTypeLibRoot);

    i = 0;
    while (RegEnumKeyA(hkeyTypeLibRoot,
            i,
            achTypeLib,
            sizeof(achTypeLib)) == ERROR_SUCCESS)
    {
        achName[0] = 0;

        if (CheckTypeLibraryVersions(hkeyTypeLibRoot, achTypeLib, achName))
        {
            wsprintfA(g_achScrub, "No entries for type library found:\n\n"
                "  Name     = %s\n"
                "  TLID     = %s\n",
                achName,
                achTypeLib);
            if (OkToDelete("type library entry"))
            {
                DeleteKey(hkeyTypeLibRoot, achTypeLib);
                continue;
            }
        }
        i += 1;
    }

    RegCloseKey(hkeyTypeLibRoot);
}

void
ScrubRegistry()
{
    HKEY  hkeyCLSID;

    RegOpenKeyA(HKEY_CLASSES_ROOT, "CLSID", &hkeyCLSID);

    // Find dangling CLSIDs (server is missing)
    CheckCLSIDs(hkeyCLSID);

    // Find dangling ProgIDs (CLSID is missing)
    CheckProgIDs(hkeyCLSID);

    RegCloseKey(hkeyCLSID);

    // Find dangling Type libraries

    CheckTypeLibraries();

    MessageBoxA(g_hwndMain, "Done scanning registry for dangling references.", "Scrub Registry", MB_APPLMODAL | MB_OK);
}

