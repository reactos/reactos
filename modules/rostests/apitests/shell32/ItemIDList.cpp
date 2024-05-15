/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test for SHSimpleIDListFromPath
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "shelltest.h"
#include <shellutils.h>

enum { DIRBIT = 1, FILEBIT = 2 };

static int FileStruct_Att(LPCITEMIDLIST pidl)
{
    return pidl && pidl->mkid.cb > 14 ? *((WORD*)((BYTE*)(pidl) + 12)) : (1 << 31); // See FileStruct in pidl.h
}

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

    // There is the difference in attributes because SHSimpleIDListFromPath
    // cannot create PIDLs to folders, only files and drives:
    ok_long((attrs1 & SFGAO_FOLDER), 0);
    ok_long((attrs2 & SFGAO_FOLDER), SFGAO_FOLDER);


    // Make sure the internal details match Windows NT5+
    LPITEMIDLIST item;
    GetSystemDirectoryW(szPath, _countof(szPath));
    CComHeapPtr<ITEMIDLIST> pidlSys32(SHSimpleIDListFromPath(szPath));
    if (szPath[1] != ':' || PathFindFileNameW(szPath) <= szPath)
    {
        skip("Not a local directory %ls\n", szPath);
    }
    else if (!(LPITEMIDLIST)pidlSys32)
    {
        skip("?\n");
    }
    else
    {
        item = ILFindLastID(pidlSys32);
        ok_long(item->mkid.abID[0] & 0x73, 0x30 | FILEBIT); // This is actually a file PIDL
        ok_long(FileStruct_Att(item), 0); // Simple PIDL without attributes
        ok_int(*(UINT*)(&item->mkid.abID[2]), 0); // No size

        ILRemoveLastID(pidlSys32); // Now we should have "c:\Windows"
        item = ILFindLastID(pidlSys32);
        ok_long(item->mkid.abID[0] & 0x73, 0x30 | DIRBIT);
        ok_int(*(UINT*)(&item->mkid.abID[2]), 0); // No size
    }

    WCHAR drive[4] = { szPath[0], szPath[1], L'\\', L'\0' };
    CComHeapPtr<ITEMIDLIST> pidlDrive(SHSimpleIDListFromPath(drive));
    if (drive[1] != ':')
    {
        skip("Not a local drive %ls\n", drive);
    }
    else if (!(LPITEMIDLIST)pidlDrive)
    {
        skip("?\n");
    }
    else
    {
        item = ILFindLastID(pidlDrive);
        ok_long(item->mkid.abID[0] & 0x70, 0x20); // Something in My Computer
        ok_char(item->mkid.abID[1] | 32, drive[0] | 32);
    }

    CComHeapPtr<ITEMIDLIST> pidlVirt(SHSimpleIDListFromPath(L"x:\\IDontExist"));
    if (!(LPITEMIDLIST)pidlVirt)
    {
        skip("?\n");
    }
    else
    {
        item = ILFindLastID(pidlVirt);
        ok_long(item->mkid.abID[0] & 0x73, 0x30 | FILEBIT); // Yes, a file
        ok_long(FileStruct_Att(item), 0); // Simple PIDL, no attributes
        ok_int(*(UINT*)(&item->mkid.abID[2]), 0); // No size

        ILRemoveLastID(pidlVirt); // "x:\"
        item = ILFindLastID(pidlVirt);
        ok_long(item->mkid.abID[0] & 0x70, 0x20); // Something in My Computer
        ok_char(item->mkid.abID[1] | 32, 'x' | 32); // x:
    }
}

START_TEST(ILCreateFromPath)
{
    WCHAR szPath[MAX_PATH];
    LPITEMIDLIST item;

    ok_ptr(ILCreateFromPathW(L"c:\\IDontExist"), NULL);

    GetSystemDirectoryW(szPath, _countof(szPath));
    CComHeapPtr<ITEMIDLIST> pidlDir(ILCreateFromPathW(szPath));
    if (szPath[1] != ':' || PathFindFileNameW(szPath) <= szPath)
    {
        skip("Not a local directory %ls\n", szPath);
    }
    else if (!(LPITEMIDLIST)pidlDir)
    {
        skip("?\n");
    }
    else
    {
        item = ILFindLastID(pidlDir);
        ok_long(item->mkid.abID[0] & 0x73, 0x30 | DIRBIT);
        ok_int(*(UINT*)(&item->mkid.abID[2]), 0); // No size
    }
    PathAppendW(szPath, L"kernel32.dll");
    CComHeapPtr<ITEMIDLIST> pidlFile(ILCreateFromPathW(szPath));
    if (!(LPITEMIDLIST)pidlFile)
    {
        skip("?\n");
    }
    else
    {
        item = ILFindLastID(pidlFile);
        ok_long(item->mkid.abID[0] & 0x73, 0x30 | FILEBIT);
        ok_int(*(UINT*)(&item->mkid.abID[2]) > 1024 * 42, TRUE); // At least this large
    }
}
