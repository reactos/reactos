#ifndef UTILITY_H
#define UTILITY_H

#define ResultFromDWORD(dw) ResultFromScode(MAKE_SCODE(SEVERITY_SUCCESS, 0, dw))

//
// pidl utility functions
//

LPCITEMIDLIST
SearchPidlByType(
    LPCITEMIDLIST pidl,
    DWORD dwPidlType
    );

DWORD
GetLastPidlType(
    LPCITEMIDLIST pidl
    );

PST_KEY
GetLastPidlKeyType(
    LPCITEMIDLIST pidl
    );

GUID *
GetLastPidlGuid(
    LPCITEMIDLIST pidl
    );

LPCWSTR
GetLastPidlText(
    LPCITEMIDLIST pidl
    );

LPCWSTR
GetPidlText(
    LPCITEMIDLIST pidl
    );

GUID *
GetPidlGuid(
    LPCITEMIDLIST pidl
    );

DWORD
GetPidlType(
    LPCITEMIDLIST pidl
    );

PST_KEY
GetPidlKeyType(
    LPCITEMIDLIST pidl
    );

LPCITEMIDLIST
GetPidlNextItem(
    LPCITEMIDLIST
    );

UINT
GetPidlSize(
    LPCITEMIDLIST
    );

LPITEMIDLIST
CopyPidl(
    LPMALLOC,
    LPCITEMIDLIST
    );

LPITEMIDLIST
CopyCatPidl(
    LPCITEMIDLIST pidl1,
    LPCITEMIDLIST pidl2
    );

VOID
FreePidl(
    LPITEMIDLIST pidl
    );

#endif   // UTILITY_H
