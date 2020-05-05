/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for zipfldr
 * COPYRIGHT:   Copyright 2020 Mark Jansen (mark.jansen@reactos.org)
 */

#include "precomp.h"

#ifndef FILE_ATTRIBUTE_VIRTUAL
#define FILE_ATTRIBUTE_VIRTUAL 0x10000
#endif

DEFINE_GUID(CLSID_ZipFolderContextMenu,    0xb8cdcb65, 0xb1bf, 0x4b42, 0x94, 0x28, 0x1d, 0xfd, 0xb7, 0xee, 0x92, 0xaf);


static bool g_bOldZipfldr;
const char test_file_1_contents[] = "Some generic text in the root file.\r\nMore text on a new line.";
const char test_file_2_contents[] = "Some generic text in the file in folder_1.\r\nMore text on a new line.";

static BOOL write_raw_file(const WCHAR* FileName, const void* Data, DWORD Size)
{
    BOOL Success;
    DWORD dwWritten;
    HANDLE Handle = CreateFileW(FileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (Handle == INVALID_HANDLE_VALUE)
    {
        skip("Failed to create temp file %ls, error %lu\n", FileName, GetLastError());
        return FALSE;
    }
    Success = WriteFile(Handle, Data, Size, &dwWritten, NULL);
    ok(Success == TRUE, "WriteFile failed with %lu\n", GetLastError());
    ok(dwWritten == Size, "WriteFile wrote %lu bytes instead of %lu\n", dwWritten, Size);
    CloseHandle(Handle);
    return Success && (dwWritten == Size);
}

BOOL extract_resource(WCHAR* Filename, LPCWSTR ResourceName, WCHAR* ParentFolder)
{
    WCHAR workdir[MAX_PATH];
    UINT TickMask = 0xffff;
    if (!ParentFolder)
    {
        GetTempPathW(_countof(workdir), workdir);
        ParentFolder = workdir;
    }
    else
    {
        // Fixed filename
        TickMask = 0;
    }
    StringCchPrintfW(Filename, MAX_PATH, L"%sTMP%u.zip", ParentFolder, GetTickCount() & TickMask);

    HMODULE hMod = GetModuleHandleW(NULL);
    HRSRC hRsrc = FindResourceW(hMod, ResourceName, MAKEINTRESOURCEW(RT_RCDATA));
    ok(!!hRsrc, "Unable to find %s\n", wine_dbgstr_w(ResourceName));
    if (!hRsrc)
        return FALSE;

    HGLOBAL hGlobal = LoadResource(hMod, hRsrc);
    DWORD Size = SizeofResource(hMod, hRsrc);
    LPVOID pData = LockResource(hGlobal);

    ok(Size && !!pData, "Unable to load %s\n", wine_dbgstr_w(ResourceName));
    if (!Size || !pData)
        return FALSE;

    BOOL Written = write_raw_file(Filename, pData, Size);
    UnlockResource(pData);
    return Written;
}

bool InitializeShellFolder_(const char* file, int line, const WCHAR* Filename, CComPtr<IShellFolder>& spFolder)
{
    CComHeapPtr<ITEMIDLIST_ABSOLUTE> pidl;
    HRESULT hr;
    ok_hr_(file, line, (hr = SHParseDisplayName(Filename, NULL, &pidl, 0, NULL)), S_OK);
    if (!SUCCEEDED(hr))
        return false;

    CComPtr<IShellFolder> spParent;
    PCUIDLIST_RELATIVE pidlLast;
    ok_hr_(file, line, (hr = SHBindToParent(pidl, IID_PPV_ARG(IShellFolder, &spParent), &pidlLast)), S_OK);
    if (!SUCCEEDED(hr))
        return false;

    ok_hr_(file, line, (hr = spParent->BindToObject(pidlLast, 0, IID_PPV_ARG(IShellFolder, &spFolder))), S_OK);

    return SUCCEEDED(hr);
}

#define GetFirstDataObject(spFolder, grfFlags, spData)      GetFirstDataObject_(__FILE__, __LINE__, spFolder, grfFlags, spData)
static bool GetFirstDataObject_(const char* file, int line, IShellFolder* spFolder, SHCONTF grfFlags, CComPtr<IDataObject>& spData)
{
    CComPtr<IEnumIDList> spEnum;
    HRESULT hr;
    ok_hr_(file, line, (hr = spFolder->EnumObjects(NULL, grfFlags, &spEnum)), S_OK);
    if (!SUCCEEDED(hr))
        return false;

    CComHeapPtr<ITEMID_CHILD> child;
    ULONG celtFetched = 0;
    ok_hr_(file, line, (hr = spEnum->Next(1, &child, &celtFetched)), S_OK);
    ok_int_(file, line, celtFetched, 1);
    if (!SUCCEEDED(hr))
        return false;

    // This call fails without the extension being '.zip'
    ok_hr_(file, line, (hr = spFolder->GetUIObjectOf(NULL, 1, &child, IID_IDataObject, NULL, reinterpret_cast<LPVOID*>(&spData))), S_OK);
    return SUCCEEDED(hr);
}

bool IsFormatAdvertised_(const char* file, int line, IDataObject* pDataObj, CLIPFORMAT cfFormat, TYMED tymed)
{
    CComPtr<IEnumFORMATETC> pEnumFmt;
    HRESULT hr = pDataObj->EnumFormatEtc(DATADIR_GET, &pEnumFmt);

    ok_hex_(file, line, hr, S_OK);
    if (!SUCCEEDED(hr))
        return false;

    FORMATETC fmt;
    while (S_OK == (hr = pEnumFmt->Next(1, &fmt, NULL)))
    {
        if (fmt.cfFormat == cfFormat)
        {
            ok_hex_(file, line, fmt.lindex, -1);
            if (tymed)
                ok_hex_(file, line, fmt.tymed, tymed);
            return true;
        }
    }
    ok_hex_(file, line, hr, S_FALSE);
    return false;
}

#if 0
#define DumpDataObjectFormats(pDataObj) DumpDataObjectFormats_(__FILE__, __LINE__, pDataObj)
static inline void DumpDataObjectFormats_(const char* file, int line, IDataObject* pDataObj)
{
    CComPtr<IEnumFORMATETC> pEnumFmt;
    HRESULT hr = pDataObj->EnumFormatEtc(DATADIR_GET, &pEnumFmt);

    if (FAILED_UNEXPECTEDLY(hr))
        return;

    FORMATETC fmt;
    while (S_OK == pEnumFmt->Next(1, &fmt, NULL))
    {
        char szBuf[512];
        GetClipboardFormatNameA(fmt.cfFormat, szBuf, sizeof(szBuf));
        trace_(file, line)("Format: %s\n", szBuf);
        trace_(file, line)(" Tymed: %u\n", fmt.tymed);
        if (fmt.tymed & TYMED_HGLOBAL)
        {
            trace_(file, line)(" TYMED_HGLOBAL supported\n");
        }
    }
}
#endif

static void test_FileDescriptor(FILEGROUPDESCRIPTORW* Descriptor)
{
    ok_int(Descriptor->cItems, 1u);
    if (Descriptor->cItems > 0)
    {
        FILETIME LocalFileTime, FileTime;
        WORD Mask = g_bOldZipfldr ? 0xffe0 : (0xffff);   // bits 0-4 are the seconds
        DosDateTimeToFileTime(0x5024, (0xa5f2 & Mask), &LocalFileTime);
        LocalFileTimeToFileTime(&LocalFileTime, &FileTime);

        FILEDESCRIPTORW* FileDescriptor = Descriptor->fgd;
        ok_hex(FileDescriptor->dwFlags, FD_ATTRIBUTES | FD_WRITESTIME | FD_FILESIZE | FD_PROGRESSUI);
        ok_hex(FileDescriptor->dwFileAttributes, FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_VIRTUAL);
        ok_hex(FileDescriptor->ftLastWriteTime.dwHighDateTime, FileTime.dwHighDateTime);
        ok_hex(FileDescriptor->ftLastWriteTime.dwLowDateTime, FileTime.dwLowDateTime);
        ok_hex(FileDescriptor->nFileSizeHigh, 0);
        ok_hex(FileDescriptor->nFileSizeLow, strlen(test_file_1_contents));
        ok_wstr(FileDescriptor->cFileName, L"test_file_for_zip.txt");
    }
}

static void test_FileDescriptor_Folder(FILEGROUPDESCRIPTORW* Descriptor)
{
    ok_int(Descriptor->cItems, 2u);
    if (Descriptor->cItems > 0)
    {
        FILETIME LocalFileTime, FileTime;
        WORD Mask = g_bOldZipfldr ? 0xffe0 : (0xffff);   // bits 0-4 are the seconds
        DosDateTimeToFileTime(0x5024, (0xa5fc & Mask), &LocalFileTime);
        LocalFileTimeToFileTime(&LocalFileTime, &FileTime);

        FILEDESCRIPTORW* FileDescriptor = Descriptor->fgd;
        ok_hex(FileDescriptor->dwFlags, FD_ATTRIBUTES | FD_WRITESTIME | FD_FILESIZE | FD_PROGRESSUI);
        ok(FileDescriptor->dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY ||
           FileDescriptor->dwFileAttributes == (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VIRTUAL), "Got attr: 0x%lx\n", FileDescriptor->dwFileAttributes);
        //ok_hex(FileDescriptor->dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY);
        ok_hex(FileDescriptor->ftLastWriteTime.dwHighDateTime, FileTime.dwHighDateTime);
        ok_hex(FileDescriptor->ftLastWriteTime.dwLowDateTime, FileTime.dwLowDateTime);
        ok_hex(FileDescriptor->nFileSizeHigh, 0);
        ok_hex(FileDescriptor->nFileSizeLow, 0);
        ok_wstr(FileDescriptor->cFileName, L"folder_1");

        if (Descriptor->cItems > 1)
        {
            DosDateTimeToFileTime(0x5024, (0xa60d & Mask), &LocalFileTime);
            LocalFileTimeToFileTime(&LocalFileTime, &FileTime);

            FileDescriptor = Descriptor->fgd + 1;
            ok_hex(FileDescriptor->dwFlags, FD_ATTRIBUTES | FD_WRITESTIME | FD_FILESIZE | FD_PROGRESSUI);
            ok_hex(FileDescriptor->dwFileAttributes, FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_VIRTUAL);
            ok_hex(FileDescriptor->ftLastWriteTime.dwHighDateTime, FileTime.dwHighDateTime);
            ok_hex(FileDescriptor->ftLastWriteTime.dwLowDateTime, FileTime.dwLowDateTime);
            ok_hex(FileDescriptor->nFileSizeHigh, 0);
            ok_hex(FileDescriptor->nFileSizeLow, strlen(test_file_2_contents));
            ok_wstr(FileDescriptor->cFileName, L"folder_1\\test_file_for_zip.txt");
        }
    }
}

static GUID GUID_NULL_;
static void test_FileContents1(IStream* Stream)
{
    STATSTG statstg = {0};
    HRESULT hr = Stream->Stat(&statstg, STATFLAG_DEFAULT);
    ok_hex(hr, g_bOldZipfldr ? E_NOTIMPL : S_OK);
    if (SUCCEEDED(hr))
    {
        FILETIME LocalFileTime, FileTime;
        WORD Mask = g_bOldZipfldr ? 0xffe0 : (0xffff);   // bits 0-4 are the seconds
        DosDateTimeToFileTime(0x5024, (0xa5f2 & Mask), &LocalFileTime);
        LocalFileTimeToFileTime(&LocalFileTime, &FileTime);

        ok_wstr(statstg.pwcsName, L"test_file_for_zip.txt");
        ok_hex(statstg.type, STGTY_STREAM);
        ok_int(statstg.cbSize.LowPart, strlen(test_file_1_contents));
        ok_hex(statstg.cbSize.HighPart, 0);
        ok_hex(statstg.mtime.dwHighDateTime, FileTime.dwHighDateTime);
        ok_hex(statstg.mtime.dwLowDateTime, FileTime.dwLowDateTime);
        ok_hex(statstg.ctime.dwHighDateTime, FileTime.dwHighDateTime);
        ok_hex(statstg.ctime.dwLowDateTime, FileTime.dwLowDateTime);
        ok_hex(statstg.atime.dwHighDateTime, FileTime.dwHighDateTime);
        ok_hex(statstg.atime.dwLowDateTime, FileTime.dwLowDateTime);
        ok_hex(statstg.grfMode, STGM_SHARE_DENY_WRITE);
        ok_hex(statstg.grfLocksSupported, 0);
        ok(!memcmp(&statstg.clsid, &GUID_NULL_, sizeof(GUID_NULL_)), "Expected GUID_NULL, got %s\n", wine_dbgstr_guid(&statstg.clsid));
        ok_hex(statstg.grfStateBits, 0);
        CoTaskMemFree(statstg.pwcsName);
    }

    LARGE_INTEGER Offset = { {0} };
    ULARGE_INTEGER NewPosition = { {0} };
    hr = Stream->Seek(Offset, STREAM_SEEK_CUR, &NewPosition);
    ok_hex(hr, g_bOldZipfldr ? E_NOTIMPL : S_OK);
    ok_int(NewPosition.HighPart, 0);
    ok_int(NewPosition.LowPart, 0);

    char buf[100] = { 0 };
    ULONG cbRead;
    hr = Stream->Read(buf, sizeof(buf)-1, &cbRead);
    ok_hex(hr, S_FALSE);
    ok_int(cbRead, strlen(test_file_1_contents));
    ok_str(buf, test_file_1_contents);

    hr = Stream->Seek(Offset, STREAM_SEEK_CUR, &NewPosition);
    ok_hex(hr, g_bOldZipfldr ? E_NOTIMPL : S_OK);
    ok_int(NewPosition.HighPart, 0);
    if (SUCCEEDED(hr))
        ok_int(NewPosition.LowPart, strlen(test_file_1_contents));

    ULONG cbWritten;
    hr = Stream->Write("DUMMY", 5, &cbWritten);
    if (!g_bOldZipfldr)
    {
        ok_hex(hr, STG_E_ACCESSDENIED);
    }
    else
    {
        // Write succeeds, but is not reflected in the file on disk
        ok_hex(hr, S_OK);
    }

    // Can increase the size...
    NewPosition.LowPart = statstg.cbSize.LowPart + 1;
    hr = Stream->SetSize(NewPosition);
    ok_hex(hr, g_bOldZipfldr ? E_NOTIMPL : S_OK);

    // But is not reflected in the Stat result
    hr = Stream->Stat(&statstg, STATFLAG_DEFAULT);
    ok_hex(hr, g_bOldZipfldr ? E_NOTIMPL : S_OK);
    if (SUCCEEDED(hr))
    {
        ok_int(statstg.cbSize.LowPart, strlen(test_file_1_contents));
        CoTaskMemFree(statstg.pwcsName);
    }

    // Old zipfldr does not support seek, so we can not read it again
    if (!g_bOldZipfldr)
    {
        Offset.QuadPart = 0;
        hr = Stream->Seek(Offset, STREAM_SEEK_SET, &NewPosition);
        ok_hex(hr, S_OK);

        memset(buf, 0, sizeof(buf));
        hr = Stream->Read(buf, sizeof(buf)-1, &cbRead);
        ok_hex(hr, S_FALSE);
        ok_int(cbRead, strlen(test_file_1_contents));
    }
}


static void test_FileContents2(IStream* Stream)
{
    STATSTG statstg = {0};
    HRESULT hr = Stream->Stat(&statstg, STATFLAG_DEFAULT);
    ok_hex(hr, g_bOldZipfldr ? E_NOTIMPL : S_OK);
    if (SUCCEEDED(hr))
    {
        FILETIME LocalFileTime, FileTime;
        WORD Mask = g_bOldZipfldr ? 0xffe0 : (0xffff);   // bits 0-4 are the seconds
        DosDateTimeToFileTime(0x5024, (0xa60d & Mask), &LocalFileTime);
        LocalFileTimeToFileTime(&LocalFileTime, &FileTime);

        ok_wstr(statstg.pwcsName, L"test_file_for_zip.txt");
        ok_hex(statstg.type, STGTY_STREAM);
        ok_int(statstg.cbSize.LowPart, strlen(test_file_2_contents));
        ok_hex(statstg.cbSize.HighPart, 0);
        ok_hex(statstg.mtime.dwHighDateTime, FileTime.dwHighDateTime);
        ok_hex(statstg.mtime.dwLowDateTime, FileTime.dwLowDateTime);
        ok_hex(statstg.ctime.dwHighDateTime, FileTime.dwHighDateTime);
        ok_hex(statstg.ctime.dwLowDateTime, FileTime.dwLowDateTime);
        ok_hex(statstg.atime.dwHighDateTime, FileTime.dwHighDateTime);
        ok_hex(statstg.atime.dwLowDateTime, FileTime.dwLowDateTime);
        ok_hex(statstg.grfMode, STGM_SHARE_DENY_WRITE);
        ok_hex(statstg.grfLocksSupported, 0);
        ok(!memcmp(&statstg.clsid, &GUID_NULL_, sizeof(GUID_NULL_)), "Expected GUID_NULL, got %s\n", wine_dbgstr_guid(&statstg.clsid));
        ok_hex(statstg.grfStateBits, 0);
        CoTaskMemFree(statstg.pwcsName);
    }

    LARGE_INTEGER Offset = { {0} };
    ULARGE_INTEGER NewPosition = { {0} };
    hr = Stream->Seek(Offset, STREAM_SEEK_CUR, &NewPosition);
    ok_hex(hr, g_bOldZipfldr ? E_NOTIMPL : S_OK);
    ok_int(NewPosition.HighPart, 0);
    ok_int(NewPosition.LowPart, 0);

    char buf[100] = { 0 };
    ULONG cbRead;
    hr = Stream->Read(buf, sizeof(buf)-1, &cbRead);
    ok_hex(hr, S_FALSE);
    ok_int(cbRead, strlen(test_file_2_contents));
    ok_str(buf, test_file_2_contents);

    hr = Stream->Seek(Offset, STREAM_SEEK_CUR, &NewPosition);
    ok_hex(hr, g_bOldZipfldr ? E_NOTIMPL : S_OK);
    ok_int(NewPosition.HighPart, 0);
    if (SUCCEEDED(hr))
        ok_int(NewPosition.LowPart, strlen(test_file_2_contents));

    ULONG cbWritten;
    hr = Stream->Write("DUMMY", 5, &cbWritten);
    if (!g_bOldZipfldr)
    {
        ok_hex(hr, STG_E_ACCESSDENIED);
    }
    else
    {
        // Write succeeds, but is not reflected in the file on disk
        ok_hex(hr, S_OK);
    }

    // Can increase the size...
    NewPosition.LowPart = statstg.cbSize.LowPart + 1;
    hr = Stream->SetSize(NewPosition);
    ok_hex(hr, g_bOldZipfldr ? E_NOTIMPL : S_OK);

    // But is not reflected in the Stat result
    hr = Stream->Stat(&statstg, STATFLAG_DEFAULT);
    ok_hex(hr, g_bOldZipfldr ? E_NOTIMPL : S_OK);
    if (SUCCEEDED(hr))
    {
        ok_int(statstg.cbSize.LowPart, strlen(test_file_2_contents));
        CoTaskMemFree(statstg.pwcsName);
    }

    // Old zipfldr does not support seek, so we can not read it again
    if (!g_bOldZipfldr)
    {
        Offset.QuadPart = 0;
        hr = Stream->Seek(Offset, STREAM_SEEK_SET, &NewPosition);
        ok_hex(hr, S_OK);

        memset(buf, 0, sizeof(buf));
        hr = Stream->Read(buf, sizeof(buf)-1, &cbRead);
        ok_hex(hr, S_FALSE);
        ok_int(cbRead, strlen(test_file_2_contents));
        ok_str(buf, test_file_2_contents);
    }
}



static CLIPFORMAT cfHIDA = RegisterClipboardFormatA(CFSTR_SHELLIDLISTA);
static CLIPFORMAT cfFileDescriptor = RegisterClipboardFormatW(CFSTR_FILEDESCRIPTORW);
static CLIPFORMAT cfFileContents = RegisterClipboardFormatW(CFSTR_FILECONTENTSW);

static void test_DataObject_FirstFile(IShellFolder* pFolder)
{
    CComPtr<IDataObject> spDataObj;
    if (!GetFirstDataObject(pFolder, SHCONTF_NONFOLDERS, spDataObj))
        return;

    if (!IsFormatAdvertised(spDataObj, cfHIDA, TYMED_HGLOBAL))
    {
        trace("Pre-Vista zipfldr\n");
        // No seconds in filetimes, less functional IStream* implementation
        g_bOldZipfldr = true;
    }

    ok(!IsFormatAdvertised(spDataObj, CF_HDROP, TYMED_NULL), "Expected CF_HDROP to be absent\n");
    ok(IsFormatAdvertised(spDataObj, cfFileDescriptor, TYMED_HGLOBAL), "Expected FileDescriptorW to be supported\n");
    ok(IsFormatAdvertised(spDataObj, cfFileContents, TYMED_ISTREAM), "Expected FileContents to be supported\n");

    STGMEDIUM medium = {0};
    FORMATETC etc = { cfFileDescriptor, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    HRESULT hr = spDataObj->GetData(&etc, &medium);
    ok_hex(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;

    ok_hex(medium.tymed, TYMED_HGLOBAL);
    PVOID pData = GlobalLock(medium.hGlobal);
    test_FileDescriptor(static_cast<FILEGROUPDESCRIPTORW*>(pData));
    GlobalUnlock(medium.hGlobal);
    ReleaseStgMedium(&medium);

    // Invalid index
    etc.cfFormat = cfFileContents;
    etc.ptd = NULL;
    etc.dwAspect = DVASPECT_CONTENT;
    etc.lindex = -1;
    etc.tymed = TYMED_ISTREAM;
    memset(&medium, 0xcc, sizeof(medium));
    hr = spDataObj->GetData(&etc, &medium);
    ok_hex(hr, E_INVALIDARG);
    ok_hex(medium.tymed, TYMED_NULL);
    ok_ptr(medium.hGlobal, NULL);
    ok_ptr(medium.pUnkForRelease, NULL);
    if (SUCCEEDED(hr))
        ReleaseStgMedium(&medium);

    // Correct index
    etc.cfFormat = cfFileContents;
    etc.ptd = NULL;
    etc.dwAspect = DVASPECT_CONTENT;
    etc.lindex = 0;
    etc.tymed = TYMED_ISTREAM;
    memset(&medium, 0xcc, sizeof(medium));
    // During this call a temp file is created: %TMP%\Temp%u_%s\test_file_for_zip.txt
    // Or for the 2k3 version:                  %TMP%\Temporary Directory %u for %s\test_file_for_zip.txt
    hr = spDataObj->GetData(&etc, &medium);
    ok_hex(hr, S_OK);
    ok_hex(medium.tymed, TYMED_ISTREAM);
    if (SUCCEEDED(hr))
    {
        test_FileContents1(medium.pstm);
        ReleaseStgMedium(&medium);
    }

    //if (winetest_get_failures())
    //    DumpDataObjectFormats(spDataObj);
}

static void test_DataObject_FirstFolder(IShellFolder* pFolder)
{
    CComPtr<IDataObject> spDataObj;
    if (!GetFirstDataObject(pFolder, SHCONTF_FOLDERS, spDataObj))
        return;

    ok(!IsFormatAdvertised(spDataObj, CF_HDROP, TYMED_NULL), "Expected CF_HDROP to be absent\n");
    ok(IsFormatAdvertised(spDataObj, cfFileDescriptor, TYMED_HGLOBAL), "Expected FileDescriptorW to be supported\n");
    ok(IsFormatAdvertised(spDataObj, cfFileContents, TYMED_ISTREAM), "Expected FileContents to be supported\n");
    // 7+
    ok(!!IsFormatAdvertised(spDataObj, cfHIDA, TYMED_HGLOBAL) != g_bOldZipfldr, "Expected HIDA to be %s\n", g_bOldZipfldr ? "absent" : "supported");

    STGMEDIUM medium = {0};
    FORMATETC etc = { cfFileDescriptor, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    HRESULT hr = spDataObj->GetData(&etc, &medium);
    ok_hex(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;

    ok_hex(medium.tymed, TYMED_HGLOBAL);
    PVOID pData = GlobalLock(medium.hGlobal);
    test_FileDescriptor_Folder(static_cast<FILEGROUPDESCRIPTORW*>(pData));
    GlobalUnlock(medium.hGlobal);
    ReleaseStgMedium(&medium);

    // Invalid index
    etc.cfFormat = cfFileContents;
    etc.ptd = NULL;
    etc.dwAspect = DVASPECT_CONTENT;
    etc.lindex = -1;
    etc.tymed = TYMED_ISTREAM;
    memset(&medium, 0xcc, sizeof(medium));
    hr = spDataObj->GetData(&etc, &medium);
    ok_hex(hr, E_INVALIDARG);
    ok_hex(medium.tymed, TYMED_NULL);
    ok_ptr(medium.hGlobal, NULL);
    ok_ptr(medium.pUnkForRelease, NULL);
    if (SUCCEEDED(hr))
        ReleaseStgMedium(&medium);

    // Not a file (first index is the folder)
    etc.cfFormat = cfFileContents;
    etc.ptd = NULL;
    etc.dwAspect = DVASPECT_CONTENT;
    etc.lindex = 0;
    etc.tymed = TYMED_ISTREAM;
    memset(&medium, 0xcc, sizeof(medium));
    hr = spDataObj->GetData(&etc, &medium);
    ok_hex(hr, E_FAIL);
    ok_hex(medium.tymed, TYMED_NULL);
    ok_ptr(medium.hGlobal, NULL);
    ok_ptr(medium.pUnkForRelease, NULL);
    if (SUCCEEDED(hr))
        ReleaseStgMedium(&medium);

    // The file (content of the folder)
    etc.cfFormat = cfFileContents;
    etc.ptd = NULL;
    etc.dwAspect = DVASPECT_CONTENT;
    etc.lindex = 1;
    etc.tymed = TYMED_ISTREAM;
    memset(&medium, 0xcc, sizeof(medium));
    // During this call a temp file is created: %TMP%\Temp%u_%s\folder1\test_file_for_zip.txt
    // Or for the 2k3 version:                  %TMP%\Temporary Directory %u for %s\folder1\test_file_for_zip.txt
    hr = spDataObj->GetData(&etc, &medium);
    ok_hex(hr, S_OK);
    ok_hex(medium.tymed, TYMED_ISTREAM);
    if (SUCCEEDED(hr))
    {
        test_FileContents2(medium.pstm);
        ReleaseStgMedium(&medium);
    }

    //if (winetest_get_failures())
    //    DumpDataObjectFormats(spDataObj);
}


static void test_DataObject(const WCHAR* Filename)
{
    CComPtr<IShellFolder> spFolder;
    if (!InitializeShellFolder(Filename, spFolder))
        return;

    test_DataObject_FirstFile(spFolder);
    test_DataObject_FirstFolder(spFolder);
}


START_TEST(IDataObject)
{
    skip("Code in zipfldr not implemented yet\n");
    return;

    HRESULT hr = CoInitialize(NULL);

    ok_hr(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;

    WCHAR ZipTestFile[MAX_PATH];
    if (!extract_resource(ZipTestFile, MAKEINTRESOURCEW(IDR_ZIP_TEST_FILE), NULL))
        return;
    test_DataObject(ZipTestFile);
    DeleteFileW(ZipTestFile);
    CoUninitialize();
}
