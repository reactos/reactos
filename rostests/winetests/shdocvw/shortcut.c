/*
 * Unit tests to document shdocvw's 'Shell Instance Objects' features
 *
 * Copyright 2005 Michael Jung
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/* At least since Windows 2000 it's possible to add FolderShortcut objects
 * by creating some registry entries. Those objects, which refer to some
 * point in the filesystem, can be registered in the shell namespace like other 
 * shell namespace extensions. Icons, names and filesystem location can be
 * configured. This is documented at http://www.virtualplastic.net/html/ui_shell.html
 * You can also google for a tool called "ShellObjectEditor" by "Tropical 
 * Technologies". This mechanism would be cool for wine, since we could 
 * map Gnome's virtual devices to FolderShortcuts and have them appear in the
 * file dialogs. These unit tests are meant to document how this mechanism
 * works on windows.
 *
 * Search MSDN for "Creating Shell Extensions with Shell Instance Objects" for
 * more documentation.*/

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winreg.h"

#include "shlobj.h"
#include "shobjidl.h"
#include "shlguid.h"
#include "ole2.h"

#include "wine/test.h"

/* The following definitions and helper functions are meant to make the de-/registration
 * of the various necessary registry keys easier. */

struct registry_value {
    const char  *szName;
    const DWORD dwType;
    const char  *szValue;
    const DWORD dwValue;
};

#define REG_VALUE_ADDR(x) ((x->dwType==REG_SZ)?(const BYTE *)x->szValue:(const BYTE *)&x->dwValue)
#define REG_VALUE_SIZE(x) ((x->dwType==REG_SZ)?strlen(x->szValue)+1:sizeof(DWORD))

struct registry_key {
    const char                  *szName;
    const struct registry_value *pValues;
    const unsigned int          cValues;
    const struct registry_key   *pSubKeys;
    const unsigned int          cSubKeys;
};

static const struct registry_value ShellFolder_values[] = {
    { "WantsFORPARSING", REG_SZ,    "",   0          },
    { "Attributes",      REG_DWORD, NULL, 0xF8000100 }
};

static const struct registry_value Instance_values[] = {
    { "CLSID", REG_SZ, "{0AFACED1-E828-11D1-9187-B532F1E9575D}", 0 }
};

static const struct registry_value InitPropertyBag_values[] = {
    { "Attributes", REG_DWORD, NULL,   0x00000015 },
    { "Target",     REG_SZ,    "C:\\", 0          }
};

static const struct registry_key Instance_keys[] = {
    { "InitPropertyBag", InitPropertyBag_values, 2, NULL, 0 }
};

static const struct registry_value InProcServer32_values[] = {
    { NULL,             REG_SZ, "shdocvw.dll", 0 },
    { "ThreadingModel", REG_SZ, "Apartment",   0 }
};

static const struct registry_value DefaultIcon_values[] = {
    { NULL, REG_SZ,"shell32.dll,8", 0 }
};

static const struct registry_key ShortcutCLSID_keys[] = {
    { "DefaultIcon",    DefaultIcon_values,    1, NULL,          0 },
    { "InProcServer32", InProcServer32_values, 2, NULL,          0 },
    { "Instance",       Instance_values,       1, Instance_keys, 1 },
    { "ShellFolder",    ShellFolder_values,    2, NULL,          0 }
};

static const struct registry_value ShortcutCLSID_values[] = {
    { NULL, REG_SZ, "WineTest", 0 }
};

static const struct registry_key HKEY_CLASSES_ROOT_keys[] = {
    { "CLSID\\{9B352EBF-2765-45C1-B4C6-85CC7F7ABC64}", ShortcutCLSID_values, 1, ShortcutCLSID_keys, 4}
};

/* register_keys - helper function, which recursively creates the registry keys and values in 
 * parameter 'keys' in the registry under hRootKey. */
static BOOL register_keys(HKEY hRootKey, const struct registry_key *keys, unsigned int numKeys) {
    HKEY hKey;
    unsigned int iKey, iValue;

    for (iKey = 0; iKey < numKeys; iKey++) {
        if (ERROR_SUCCESS == RegCreateKeyExA(hRootKey, keys[iKey].szName, 0, NULL, 0, 
                                             KEY_WRITE, NULL, &hKey, NULL))
        {
            for (iValue = 0; iValue < keys[iKey].cValues; iValue++) {
                const struct registry_value * value = &keys[iKey].pValues[iValue];
                if (ERROR_SUCCESS != RegSetValueExA(hKey, value->szName, 0, value->dwType,
                                                    REG_VALUE_ADDR(value), REG_VALUE_SIZE(value)))
                {
                    RegCloseKey(hKey);
                    return FALSE;
                }
            }
            
            if (!register_keys(hKey, keys[iKey].pSubKeys, keys[iKey].cSubKeys)) {
                RegCloseKey(hKey);
                return FALSE;
            }
            
            RegCloseKey(hKey);
        }
    }
        
    return TRUE;
}

/* unregister_keys - clean up after register_keys */
static void unregister_keys(HKEY hRootKey, const struct registry_key *keys, unsigned int numKeys) {
    HKEY hKey;
    unsigned int iKey;

    for (iKey = 0; iKey < numKeys; iKey++) {
        if (ERROR_SUCCESS == RegOpenKeyExA(hRootKey, keys[iKey].szName, 0, DELETE, &hKey)) {
            unregister_keys(hKey, keys[iKey].pSubKeys, keys[iKey].cSubKeys);
            RegCloseKey(hKey);
        }
        RegDeleteKeyA(hRootKey, keys[iKey].szName);
    }
}
    
static void test_ShortcutFolder(void) {
    LPSHELLFOLDER pDesktopFolder, pWineTestFolder;
    IPersistFolder3 *pWineTestPersistFolder;
    LPITEMIDLIST pidlWineTestFolder, pidlCurFolder;
    HRESULT hr;
    CLSID clsid;
    const CLSID CLSID_WineTest = 
        { 0x9b352ebf, 0x2765, 0x45c1, { 0xb4, 0xc6, 0x85, 0xcc, 0x7f, 0x7a, 0xbc, 0x64 } };
    WCHAR wszWineTestFolder[] = {
        ':',':','{','9','B','3','5','2','E','B','F','-','2','7','6','5','-','4','5','C','1','-',
        'B','4','C','6','-','8','5','C','C','7','F','7','A','B','C','6','4','}',0 };

    /* First, we register all the necessary registry keys/values for our 'WineTest'
     * shell object. */
    register_keys(HKEY_CLASSES_ROOT, HKEY_CLASSES_ROOT_keys, 1);

    hr = SHGetDesktopFolder(&pDesktopFolder);
    ok (SUCCEEDED(hr), "SHGetDesktopFolder failed! hr = %08x\n", hr);
    if (FAILED(hr)) goto cleanup;

    /* Convert the wszWineTestFolder string to an ITEMIDLIST. */
    hr = IShellFolder_ParseDisplayName(pDesktopFolder, NULL, NULL, wszWineTestFolder, NULL, 
                                       &pidlWineTestFolder, NULL);
    todo_wine
    {
        ok (hr == HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER),
            "Expected %08x, got %08x\n", HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER), hr);
    }
    if (FAILED(hr)) {
        IShellFolder_Release(pDesktopFolder);
        goto cleanup;
    }

    /* FIXME: these tests are never run */

    /* Bind to a WineTest folder object. There has to be some support for this in shdocvw.dll.
     * This isn't implemented in wine yet.*/
    hr = IShellFolder_BindToObject(pDesktopFolder, pidlWineTestFolder, NULL, &IID_IShellFolder, 
                                   (LPVOID*)&pWineTestFolder);
    IShellFolder_Release(pDesktopFolder);
    ILFree(pidlWineTestFolder);
    ok (SUCCEEDED(hr), "IShellFolder::BindToObject(WineTestFolder) failed! hr = %08x\n", hr);
    if (FAILED(hr)) goto cleanup;

    hr = IShellFolder_QueryInterface(pWineTestFolder, &IID_IPersistFolder3, (LPVOID*)&pWineTestPersistFolder);
    ok (SUCCEEDED(hr), "IShellFolder::QueryInterface(IPersistFolder3) failed! hr = %08x\n", hr);
    IShellFolder_Release(pWineTestFolder);
    if (FAILED(hr)) goto cleanup;

    /* The resulting folder object has the FolderShortcut CLSID, instead of it's own. */
    hr = IPersistFolder3_GetClassID(pWineTestPersistFolder, &clsid);
    ok (SUCCEEDED(hr), "IPersist::GetClassID failed! hr = %08x\n", hr);
    ok (IsEqualCLSID(&CLSID_FolderShortcut, &clsid), "GetClassId returned wrong CLSID!\n"); 
  
    pidlCurFolder = (LPITEMIDLIST)0xdeadbeef;
    hr = IPersistFolder3_GetCurFolder(pWineTestPersistFolder, &pidlCurFolder);
    ok (SUCCEEDED(hr), "IPersistFolder3::GetCurFolder failed! hr = %08x\n", hr);
    ok (pidlCurFolder->mkid.cb == 20 && ((LPSHITEMID)((BYTE*)pidlCurFolder+20))->cb == 0 && 
        IsEqualCLSID(&CLSID_WineTest, (REFCLSID)((LPBYTE)pidlCurFolder+4)), 
        "GetCurFolder returned unexpected pidl!\n");
    
    IPersistFolder3_Release(pWineTestPersistFolder);
    
cleanup:
    unregister_keys(HKEY_CLASSES_ROOT, HKEY_CLASSES_ROOT_keys, 1);
}    
    
START_TEST(shortcut)
{
    OleInitialize(NULL);
    test_ShortcutFolder();
    OleUninitialize();
}
