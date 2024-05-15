/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test for SHSimpleIDListFromPath
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "shelltest.h"
#include <shellutils.h>

START_TEST(SHSimpleIDListFromPath)
{
    HRESULT hr;
    WCHAR szPath[MAX_PATH];
    GetWindowsDirectoryW(szPath, _countof(szPath));

    // We compare pidl1 and pidl2
    CComHeapPtr<ITEMIDLIST> pidl1(SHSimpleIDListFromPath(szPath));
    CComHeapPtr<ITEMIDLIST> pidl2(ILCreateFromPathW(szPath));

    // Yes, they are equal logically
    LPITEMIDLIST pidl1Last = ILFindLastID(pidl1), pidl2Last = ILFindLastID(pidl2);
    ok_int(ILIsEqual(pidl1, pidl2), TRUE);
    ok_int(ILIsEqual(pidl1Last, pidl2Last), TRUE);

    // Bind to parent
    CComPtr<IShellFolder> psf1, psf2;
    hr = SHBindToParent(pidl1, IID_PPV_ARG(IShellFolder, &psf1), NULL);
    ok_long(hr, S_OK);
    hr = SHBindToParent(pidl2, IID_PPV_ARG(IShellFolder, &psf2), NULL);
    ok_long(hr, S_OK);

    // Get attributes
    DWORD attrs1 = SFGAO_FOLDER, attrs2 = SFGAO_FOLDER;
    hr = (psf1 ? psf1->GetAttributesOf(1, &pidl1Last, &attrs1) : E_UNEXPECTED);
    ok_long(hr, S_OK);
    hr = (psf2 ? psf2->GetAttributesOf(1, &pidl2Last, &attrs2) : E_UNEXPECTED);
    ok_long(hr, S_OK);

    // There is the difference in attributes:
    ok_long((attrs1 & SFGAO_FOLDER), 0);
    ok_long((attrs2 & SFGAO_FOLDER), SFGAO_FOLDER);
}
