/*
 * Tests for special shell folders
 *
 * Copyright 2008 Robert Shearman for CodeWeavers
 * Copyright 2008 Owen Rudge
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

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "shellapi.h"
#include "shlobj.h"

#include "wine/test.h"

static inline BOOL SHELL_OsIsUnicode(void)
{
    return !(GetVersion() & 0x80000000);
}

/* Tests for My Network Places */
static void test_parse_for_entire_network(void)
{
    static WCHAR my_network_places_path[] = {
        ':',':','{','2','0','8','D','2','C','6','0','-','3','A','E','A','-',
                    '1','0','6','9','-','A','2','D','7','-','0','8','0','0','2','B','3','0','3','0','9','D','}', 0 };
    static WCHAR entire_network_path[] = {
        ':',':','{','2','0','8','D','2','C','6','0','-','3','A','E','A','-',
                    '1','0','6','9','-','A','2','D','7','-','0','8','0','0','2','B','3','0','3','0','9','D',
                '}','\\','E','n','t','i','r','e','N','e','t','w','o','r','k',0 };
    IShellFolder *psfDesktop;
    HRESULT hr;
    DWORD eaten = 0xdeadbeef;
    LPITEMIDLIST pidl;
    DWORD attr = ~0;
    DWORD expected_attr;

    hr = SHGetDesktopFolder(&psfDesktop);
    ok(hr == S_OK, "SHGetDesktopFolder failed with error 0x%x\n", hr);

    hr = IShellFolder_ParseDisplayName(psfDesktop, NULL, NULL, my_network_places_path, &eaten, &pidl, &attr);
    ok(hr == S_OK, "IShellFolder_ParseDisplayName failed with error 0x%x\n", hr);
    todo_wine
    ok(eaten == 0xdeadbeef, "eaten should not have been set to %u\n", eaten);
    expected_attr = SFGAO_HASSUBFOLDER|SFGAO_FOLDER|SFGAO_FILESYSANCESTOR|SFGAO_DROPTARGET|SFGAO_HASPROPSHEET|SFGAO_CANRENAME|SFGAO_CANLINK;
    todo_wine
    ok((attr == expected_attr) || /* Win9x, NT4 */
       (attr == (expected_attr | SFGAO_STREAM)) || /* W2K */
       (attr == (expected_attr | SFGAO_CANDELETE)) || /* XP, W2K3 */
       (attr == (expected_attr | SFGAO_CANDELETE | SFGAO_NONENUMERATED)), /* Vista */
       "Unexpected attributes : %08x\n", attr);

    ILFree(pidl);

    /* Start clean again */
    eaten = 0xdeadbeef;
    attr = ~0;

    hr = IShellFolder_ParseDisplayName(psfDesktop, NULL, NULL, entire_network_path, &eaten, &pidl, &attr);
    IShellFolder_Release(psfDesktop);
    if (hr == HRESULT_FROM_WIN32(ERROR_BAD_NET_NAME) ||
        hr == HRESULT_FROM_WIN32(ERROR_NO_NET_OR_BAD_PATH) ||
        hr == HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER))
    {
        win_skip("'EntireNetwork' is not available on Win9x, NT4 and Vista\n");
        return;
    }
    ok(hr == S_OK, "IShellFolder_ParseDisplayName failed with error 0x%x\n", hr);
    todo_wine
    ok(eaten == 0xdeadbeef, "eaten should not have been set to %u\n", eaten);
    expected_attr = SFGAO_HASSUBFOLDER|SFGAO_FOLDER|SFGAO_FILESYSANCESTOR|SFGAO_HASPROPSHEET|SFGAO_CANLINK;
    todo_wine
    ok(attr == expected_attr || /* winme, nt4 */
       attr == (expected_attr | SFGAO_STREAM) || /* win2k */
       attr == (expected_attr | SFGAO_STORAGEANCESTOR),  /* others */
       "attr should be 0x%x, not 0x%x\n", expected_attr, attr);

    ILFree(pidl);
}

/* Tests for Control Panel */
static void test_parse_for_control_panel(void)
{
    /* path of My Computer\Control Panel */
    static WCHAR control_panel_path[] = {
        ':',':','{','2','0','D','0','4','F','E','0','-','3','A','E','A','-','1','0','6','9','-','A','2','D','8','-','0','8','0','0','2','B','3','0','3','0','9','D','}','\\',
        ':',':','{','2','1','E','C','2','0','2','0','-','3','A','E','A','-','1','0','6','9','-','A','2','D','D','-','0','8','0','0','2','B','3','0','3','0','9','D','}', 0 };
    IShellFolder *psfDesktop;
    HRESULT hr;
    DWORD eaten = 0xdeadbeef;
    LPITEMIDLIST pidl;
    DWORD attr = ~0;

    hr = SHGetDesktopFolder(&psfDesktop);
    ok(hr == S_OK, "SHGetDesktopFolder failed with error 0x%x\n", hr);

    hr = IShellFolder_ParseDisplayName(psfDesktop, NULL, NULL, control_panel_path, &eaten, &pidl, &attr);
    ok(hr == S_OK, "IShellFolder_ParseDisplayName failed with error 0x%x\n", hr);
    todo_wine ok(eaten == 0xdeadbeef, "eaten should not have been set to %u\n", eaten);
    todo_wine
    ok((attr == (SFGAO_CANLINK | SFGAO_FOLDER)) || /* Win9x, NT4 */
       (attr == (SFGAO_CANLINK | SFGAO_FOLDER | SFGAO_HASSUBFOLDER | SFGAO_STREAM)) || /* W2K */
       (attr == (SFGAO_CANLINK | SFGAO_FOLDER | SFGAO_HASSUBFOLDER)) || /* W2K, XP, W2K3 */
       (attr == (SFGAO_CANLINK | SFGAO_NONENUMERATED)) || /* Vista */
       (attr == SFGAO_CANLINK), /* Vista, W2K8 */
       "Unexpected attributes : %08x\n", attr);

    ILFree(pidl);
    IShellFolder_Release(psfDesktop);
}

static void test_printers_folder(void)
{
    IShellFolder2 *folder;
    IPersistFolder2 *pf;
    SHELLDETAILS details;
    SHCOLSTATEF state;
    LPITEMIDLIST pidl1, pidl2;
    HRESULT hr;
    INT i;

    CoInitialize( NULL );

    hr = CoCreateInstance(&CLSID_Printers, NULL, CLSCTX_INPROC_SERVER, &IID_IShellFolder2, (void**)&folder);
    if (hr != S_OK)
    {
        win_skip("Failed to created IShellFolder2 for Printers folder\n");
        CoUninitialize();
        return;
    }

if (0)
{
    /* crashes on XP */
    IShellFolder2_GetDetailsOf(folder, NULL, 0, NULL);
    IShellFolder2_GetDefaultColumnState(folder, 0, NULL);
    IPersistFolder2_GetCurFolder(pf, NULL);
}

    /* 5 columns defined */
    hr = IShellFolder2_GetDetailsOf(folder, NULL, 6, &details);
    ok(hr == E_NOTIMPL, "got 0x%08x\n", hr);

    hr = IShellFolder2_GetDefaultColumnState(folder, 6, &state);
    ok(broken(hr == E_NOTIMPL) || hr == E_INVALIDARG /* Win7 */, "got 0x%08x\n", hr);

    details.str.u.pOleStr = NULL;
    hr = IShellFolder2_GetDetailsOf(folder, NULL, 0, &details);
    ok(hr == S_OK || broken(E_NOTIMPL) /* W2K */, "got 0x%08x\n", hr);
    if (SHELL_OsIsUnicode()) SHFree(details.str.u.pOleStr);

    /* test every column if method is implemented */
    if (hr == S_OK)
    {
        ok(details.str.uType == STRRET_WSTR, "got %d\n", details.str.uType);

        for(i = 0; i < 6; i++)
        {
            hr = IShellFolder2_GetDetailsOf(folder, NULL, i, &details);
            ok(hr == S_OK, "got 0x%08x\n", hr);

            /* all columns are left-aligned */
            ok(details.fmt == LVCFMT_LEFT, "got 0x%x\n", details.fmt);
            /* can't be on w9x at this point, IShellFolder2 unsupported there,
               check present for running Wine with w9x setup */
            if (SHELL_OsIsUnicode()) SHFree(details.str.u.pOleStr);

            hr = IShellFolder2_GetDefaultColumnState(folder, i, &state);
            ok(hr == S_OK, "got 0x%08x\n", hr);
            /* all columns are string except document count */
            if (i == 1)
                ok(state == (SHCOLSTATE_TYPE_INT | SHCOLSTATE_ONBYDEFAULT), "got 0x%x\n", state);
            else
                ok(state == (SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT), "got 0x%x\n", state);
        }
    }

    /* default pidl */
    hr = IShellFolder2_QueryInterface(folder, &IID_IPersistFolder2, (void**)&pf);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /* not initialized */
    pidl1 = (void*)0xdeadbeef;
    hr = IPersistFolder2_GetCurFolder(pf, &pidl1);
    ok(hr == S_FALSE, "got 0x%08x\n", hr);
    ok(pidl1 == NULL, "got %p\n", pidl1);

    hr = SHGetSpecialFolderLocation(NULL, CSIDL_PRINTERS, &pidl2);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IPersistFolder2_Initialize(pf, pidl2);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IPersistFolder2_GetCurFolder(pf, &pidl1);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    ok(ILIsEqual(pidl1, pidl2), "expected same PIDL\n");
    IPersistFolder2_Release(pf);

    ILFree(pidl1);
    ILFree(pidl2);
    IShellFolder2_Release(folder);

    CoUninitialize();
}

static void test_desktop_folder(void)
{
    IShellFolder *psf;
    HRESULT hr;

    hr = SHGetDesktopFolder(&psf);
    ok(hr == S_OK, "Got %x\n", hr);

    hr = IShellFolder_QueryInterface(psf, &IID_IShellFolder, NULL);
    ok(hr == E_POINTER, "Got %x\n", hr);

    IShellFolder_Release(psf);
}

START_TEST(shfldr_special)
{
    test_parse_for_entire_network();
    test_parse_for_control_panel();
    test_printers_folder();
    test_desktop_folder();
}
