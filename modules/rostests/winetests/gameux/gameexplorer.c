/*
 * IGameExplorer and IGameExplorer2 tests
 *
 * Copyright (C) 2010 Mariusz Pluciński
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

#define COBJMACROS

#include "windows.h"
#include "ole2.h"
#include "objsafe.h"
#include "objbase.h"
#include "shlwapi.h"
#include "sddl.h"
#include "shobjidl.h"

#include "initguid.h"
#include "gameux.h"

#include "wine/test.h"

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);

static void test_create(BOOL* gameExplorerAvailable, BOOL* gameExplorer2Available)
{
    HRESULT hr;

    IGameExplorer* ge = NULL;
    IGameExplorer2* ge2 = NULL;

    /* interface available up from Vista */
    hr = CoCreateInstance( &CLSID_GameExplorer, NULL, CLSCTX_INPROC_SERVER, &IID_IGameExplorer, (LPVOID*)&ge);
    if(ge)
    {
        ok(hr == S_OK, "IGameExplorer creating failed (result false)\n");
        *gameExplorerAvailable = TRUE;
        IGameExplorer_Release(ge);
    }
    else
        win_skip("IGameExplorer cannot be created\n");

    /* interface available up from Win7 */
    hr = CoCreateInstance( &CLSID_GameExplorer, NULL, CLSCTX_INPROC_SERVER, &IID_IGameExplorer2, (LPVOID*)&ge2);
    if(ge2)
    {
        ok( hr == S_OK, "IGameExplorer2 creating failed (result false)\n");
        *gameExplorer2Available = TRUE;
        IGameExplorer2_Release(ge2);
    }
    else
        win_skip("IGameExplorer2 cannot be created\n");
}

static void test_add_remove_game(void)
{
    static const GUID defaultGUID = {0x01234567, 0x89AB, 0xCDEF,
        { 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF}};

    HRESULT hr;

    IGameExplorer* ge = NULL;
    WCHAR sExeName[MAX_PATH];
    WCHAR sExePath[MAX_PATH];
    BSTR bstrExeName = NULL, bstrExePath = NULL;
    DWORD dwExeNameLen;
    GUID guid;

    hr = CoCreateInstance(&CLSID_GameExplorer, NULL, CLSCTX_INPROC_SERVER, &IID_IGameExplorer, (LPVOID*) & ge);
    ok(ge != NULL, "cannot create coclass IGameExplorer\n");
    ok(hr == S_OK, "cannot create coclass IGameExplorer\n");

    if(ge)
    {
        /* prepare path to binary */
        dwExeNameLen = GetModuleFileNameW(NULL, sExeName, ARRAY_SIZE(sExeName));
        ok(dwExeNameLen != 0, "GetModuleFileNameW returned invalid value\n");
        lstrcpynW(sExePath, sExeName, StrRChrW(sExeName, NULL, '\\') - sExeName + 1);
        bstrExeName = SysAllocString(sExeName);
        ok(bstrExeName != NULL, "cannot allocate string for exe name\n");
        bstrExePath = SysAllocString(sExePath);
        ok(bstrExePath != NULL, "cannot allocate string for exe path\n");

        if(bstrExeName && bstrExePath)
        {
            trace("prepared EXE name: %s\n", wine_dbgstr_w(bstrExeName));
            trace("prepared EXE path: %s\n", wine_dbgstr_w(bstrExePath));


            /* try to register game with provided guid */
            memcpy(&guid, &defaultGUID, sizeof (guid));

            hr = IGameExplorer_AddGame(ge, bstrExeName, bstrExePath, GIS_CURRENT_USER, &guid);
            ok(SUCCEEDED(hr), "IGameExplorer::AddGame failed (error 0x%08Ix)\n", hr);
            ok(memcmp(&guid, &defaultGUID, sizeof (guid)) == 0, "AddGame unexpectedly modified GUID\n");

            if(SUCCEEDED(hr))
            {
                hr = IGameExplorer_RemoveGame(ge, guid);
                ok(SUCCEEDED(hr), "IGameExplorer::RemoveGame failed (error 0x%08Ix)\n", hr);
            }

            /* try to register game with empty guid */
            memcpy(&guid, &GUID_NULL, sizeof (guid));

            hr = IGameExplorer_AddGame(ge, bstrExeName, bstrExePath, GIS_CURRENT_USER, &guid);
            ok(SUCCEEDED(hr), "IGameExplorer::AddGame failed (error 0x%08Ix)\n", hr);

            if(SUCCEEDED(hr))
            {
                hr = IGameExplorer_RemoveGame(ge, guid);
                ok(SUCCEEDED(hr), "IGameExplorer::RemoveGame failed (error 0x%08Ix)\n", hr);
            }
        }

        /* free allocated resources */
        SysFreeString(bstrExePath);
        SysFreeString(bstrExeName);

        IGameExplorer_Release(ge);
    }
}

static void test_install_uninstall_game(void)
{
    HRESULT hr;

    IGameExplorer2* ge2 = NULL;
    WCHAR sExeName[MAX_PATH];
    WCHAR sExePath[MAX_PATH];
    DWORD dwExeNameLen;

    hr = CoCreateInstance(&CLSID_GameExplorer, NULL, CLSCTX_INPROC_SERVER, &IID_IGameExplorer2, (LPVOID*)&ge2);
    ok(ge2 != NULL, "cannot create coclass IGameExplorer2\n");
    ok(hr == S_OK, "cannot create coclass IGameExplorer2\n");

    if(ge2)
    {
        /* prepare path to binary */
        dwExeNameLen = GetModuleFileNameW(NULL, sExeName, ARRAY_SIZE(sExeName));
        ok(dwExeNameLen != 0, "GetModuleFileNameW returned invalid value\n");
        lstrcpynW(sExePath, sExeName, StrRChrW(sExeName, NULL, '\\') - sExeName + 1);

        trace("prepared EXE name: %s\n", wine_dbgstr_w(sExeName));
        trace("prepared EXE path: %s\n", wine_dbgstr_w(sExePath));


        hr = IGameExplorer2_InstallGame(ge2, sExeName, sExePath, GIS_CURRENT_USER);
        ok(SUCCEEDED(hr), "IGameExplorer2::InstallGame failed (error 0x%08Ix)\n", hr);

        if(SUCCEEDED(hr))
        {
            hr = IGameExplorer2_UninstallGame(ge2, sExeName);
            ok(SUCCEEDED(hr), "IGameExplorer2::UninstallGame failed (error 0x%08Ix)\n", hr);
        }

        IGameExplorer2_Release(ge2);
    }
}

static void run_tests(void)
{
    BOOL gameExplorerAvailable = FALSE;
    BOOL gameExplorer2Available = FALSE;

    test_create(&gameExplorerAvailable, &gameExplorer2Available);

    if(gameExplorerAvailable)
        test_add_remove_game();

    if(gameExplorer2Available)
        test_install_uninstall_game();
}

START_TEST(gameexplorer)
{
    HRESULT hr;

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    ok(hr == S_OK, "Failed to initialize COM, hr %#Ix.\n", hr);
    trace("Running multithreaded tests.\n");
    run_tests();
    CoUninitialize();

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    ok(hr == S_OK, "Failed to initialize COM, hr %#Ix.\n", hr);
    trace("Running apartment threaded tests.\n");
    run_tests();
    CoUninitialize();
}
