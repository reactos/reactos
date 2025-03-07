/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test for SHSimpleIDListFromPath
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 *              Copyright 2024 Whindmar Saksit <whindsaks@proton.me>
 */

#include "shelltest.h"
#include <shellutils.h>

enum { 
    DIRBIT = 1, FILEBIT = 2,
    PT_COMPUTER_REGITEM = 0x2E,
};

static BYTE GetPIDLType(LPCITEMIDLIST pidl)
{
    // Return the type without the 0x80 flag
    return pidl && pidl->mkid.cb >= 3 ? (pidl->mkid.abID[0] & 0x7F) : 0;
}

struct FS95 // FileSystem item header
{
    WORD cb;
    BYTE type;
    BYTE unknown;
    UINT size;
    WORD date, time;
    WORD att;
    CHAR name[ANYSIZE_ARRAY];

    static BOOL IsFS(LPCITEMIDLIST p)
    {
        return (p && p->mkid.cb > 2) ? (p->mkid.abID[0] & 0x70) == 0x30 : FALSE;
    }
    static FS95* Validate(LPCITEMIDLIST p)
    {
        C_ASSERT(FIELD_OFFSET(FS95, name) == 14);
        return p && p->mkid.cb > FIELD_OFFSET(FS95, name) && IsFS(p) ? (FS95*)p : NULL;
    }
};

static int FileStruct_Att(LPCITEMIDLIST pidl)
{
    C_ASSERT(FIELD_OFFSET(FS95, att) == 12);
    FS95 *p = FS95::Validate(pidl);
    return p ? p->att : (UINT(1) << 31);
}

#define TEST_CLSID(pidl, type, offset, clsid) \
    do { \
        ok_long(GetPIDLType(pidl), (type)); \
        ok_int(*(CLSID*)((&pidl->mkid.abID[(offset) - sizeof(WORD)])) == clsid, TRUE); \
    } while (0)

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

    LPITEMIDLIST pidl;
    ok_int((pidl = SHSimpleIDListFromPath(L"c:")) != NULL, TRUE);
    ILFree(pidl);
    ok_int((pidl = SHSimpleIDListFromPath(L"c:\\")) != NULL, TRUE);
    ILFree(pidl);
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

START_TEST(PIDL)
{
    LPITEMIDLIST pidl;

    pidl = SHCloneSpecialIDList(NULL, CSIDL_DRIVES, FALSE);
    if (pidl)
        TEST_CLSID(ILFindLastID(pidl), 0x1f, 4, CLSID_MyComputer);
    else
        skip("?\n");
    ILFree(pidl);

    pidl = SHCloneSpecialIDList(NULL, CSIDL_PRINTERS, FALSE);
    if (pidl)
        TEST_CLSID(ILFindLastID(pidl), 0x71, 14, CLSID_Printers);
    else
        skip("?\n");
    ILFree(pidl);
}

START_TEST(ILIsEqual)
{
    LPITEMIDLIST p1, p2, pidl;

    p1 = p2 = NULL;
    ok_int(ILIsEqual(p1, p2), TRUE);

    ITEMIDLIST emptyitem = {}, emptyitem2 = {};
    ok_int(ILIsEqual(&emptyitem, &emptyitem2), TRUE);

    ok_int(ILIsEqual(NULL, &emptyitem), FALSE); // These two are not equal for some reason

    p1 = SHCloneSpecialIDList(NULL, CSIDL_DRIVES, FALSE);
    p2 = SHCloneSpecialIDList(NULL, CSIDL_DRIVES, FALSE);
    if (p1 && p2)
    {
        ok_int(ILIsEqual(p1, p2), TRUE);
        p1->mkid.abID[0] = PT_COMPUTER_REGITEM; // RegItem in wrong parent
        ok_int(ILIsEqual(p1, p2), FALSE);
    }
    else
    {
        skip("Unable to initialize test\n");
    }
    ILFree(p1);
    ILFree(p2);

    // ILIsParent must compare like ILIsEqual
    p1 = SHSimpleIDListFromPath(L"c:\\");
    p2 = SHSimpleIDListFromPath(L"c:\\dir\\file");
    if (p1 && p2)
    {
        ok_int(ILIsParent(NULL, p1, FALSE), FALSE); // NULL is always false
        ok_int(ILIsParent(p1, NULL, FALSE), FALSE); // NULL is always false
        ok_int(ILIsParent(NULL, NULL, FALSE), FALSE); // NULL is always false
        ok_int(ILIsParent(p1, p1, FALSE), TRUE); // I'm my own parent
        ok_int(ILIsParent(p1, p1, TRUE), FALSE); // Self is not immediate
        ok_int(ILIsParent(p1, p2, FALSE), TRUE); // Grandchild
        ok_int(ILIsParent(p1, p2, TRUE), FALSE); // Grandchild is not immediate
        ok_ptr(ILFindChild(p1, p2), ILGetNext(ILGetNext(p2))); // Child is "dir\\file", skip MyComputer and C:
        ok_int(ILIsEmpty(pidl = ILFindChild(p1, p1)) && pidl, TRUE); // Self
        ILRemoveLastID(p2);
        ok_int(ILIsParent(p1, p2, TRUE), TRUE); // Immediate child

        p1->mkid.abID[0] = PT_COMPUTER_REGITEM; // RegItem in wrong parent
        ok_int(ILIsParent(p1, p2, FALSE), FALSE);
    }
    else
    {
        skip("Unable to initialize test\n");
    }
    ILFree(p1);
    ILFree(p2);
}
