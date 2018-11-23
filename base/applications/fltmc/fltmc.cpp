/*
* PROJECT:     ReactOS fltmc utility
* LICENSE:     GPL - See COPYING in the top level directory
* FILE:        base/applications/fltmc/fltmc.c
* PURPOSE:     Control utility for file system filter drivers
* PROGRAMMERS: Copyright 2016 Ged Murphy (gedmurphy@gmail.com)
*/

// Please leave this temporary hack in place
// it's used to keep VS2015 happy for development.
#ifdef __REACTOS__
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <wchar.h>
#else
#include <Windows.h>
#endif
#include <fltuser.h>
#include <atlstr.h>
#include <strsafe.h>
#include "resource.h"

EXTERN_C int wmain(int argc, WCHAR *argv[]);

void
LoadAndPrintString(ULONG MessageId, ...)
{
    va_list args;

    CAtlStringW Message;
    if (Message.LoadStringW(MessageId))
    {
        va_start(args, MessageId);
        vwprintf(Message.GetBuffer(), args);
        va_end(args);
    }
}

void
PrintErrorText(_In_ ULONG ErrorCode)
{
    WCHAR Buffer[256];
    if (FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM,
                       0,
                       ErrorCode,
                       0,
                       Buffer,
                       256,
                       0))
    {
        wprintf(L"%s\n", Buffer);
    }
}

DWORD
SetDriverLoadPrivilege()
{
    TOKEN_PRIVILEGES TokenPrivileges;
    HANDLE hToken;
    LUID luid;
    BOOL bSuccess;
    DWORD dwError = ERROR_SUCCESS;
 
    bSuccess = OpenProcessToken(GetCurrentProcess(),
                                TOKEN_ADJUST_PRIVILEGES,
                                &hToken);
    if (bSuccess == FALSE)
        return GetLastError();

    if (!LookupPrivilegeValueW(NULL, SE_LOAD_DRIVER_NAME, &luid))
        return GetLastError();

    TokenPrivileges.PrivilegeCount = 1;
    TokenPrivileges.Privileges[0].Luid = luid;
    TokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    bSuccess = AdjustTokenPrivileges(hToken,
                                     FALSE,
                                     &TokenPrivileges,
                                     sizeof(TOKEN_PRIVILEGES),
                                     NULL,
                                     NULL);
    if (bSuccess == FALSE)
        dwError = GetLastError();

    CloseHandle(hToken);

    return dwError;
}

void
LoadFilter(_In_ LPWSTR FilterName)
{
    DWORD dwError;
    dwError = SetDriverLoadPrivilege();
    if (dwError != ERROR_SUCCESS)
    {
        LoadAndPrintString(IDS_ERROR_PRIV, HRESULT_FROM_WIN32(dwError));
        return;
    }

    HRESULT hr = FilterLoad(FilterName);
    if (hr != S_OK)
    {
        LoadAndPrintString(IDS_ERROR_LOAD, hr);
        PrintErrorText(hr);
    }
}

void
UnloadFilter(_In_ LPWSTR FilterName)
{
    DWORD dwError;
    dwError = SetDriverLoadPrivilege();
    if (dwError != ERROR_SUCCESS)
    {
        LoadAndPrintString(IDS_ERROR_PRIV, HRESULT_FROM_WIN32(dwError));
        return;
    }

    HRESULT hr = FilterUnload(FilterName);
    if (hr != S_OK)
    {
        LoadAndPrintString(IDS_ERROR_UNLOAD, hr);
        PrintErrorText(hr);
    }
}

void
PrintFilterInfo(_In_ PVOID Buffer,
                _In_ BOOL IsNewStyle)
{
    WCHAR FilterName[128] = { 0 };
    WCHAR NumOfInstances[16] = { 0 };
    WCHAR Altitude[64] = { 0 };
    WCHAR Frame[16] = { 0 };

    if (IsNewStyle)
    {
        PFILTER_AGGREGATE_STANDARD_INFORMATION FilterAggInfo;
        FilterAggInfo = (PFILTER_AGGREGATE_STANDARD_INFORMATION)Buffer;

        if (FilterAggInfo->Flags & FLTFL_ASI_IS_MINIFILTER)
        {
            if (FilterAggInfo->Type.MiniFilter.FilterNameLength < 128)
            {
                CopyMemory(FilterName,
                           (PCHAR)FilterAggInfo + FilterAggInfo->Type.MiniFilter.FilterNameBufferOffset,
                           FilterAggInfo->Type.MiniFilter.FilterNameLength);
                FilterName[FilterAggInfo->Type.MiniFilter.FilterNameLength] = UNICODE_NULL;
            }

            StringCchPrintfW(NumOfInstances, 16, L"%lu", FilterAggInfo->Type.MiniFilter.NumberOfInstances);

            if (FilterAggInfo->Type.MiniFilter.FilterAltitudeLength < 64)
            {
                CopyMemory(Altitude,
                           (PCHAR)FilterAggInfo + FilterAggInfo->Type.MiniFilter.FilterAltitudeBufferOffset,
                           FilterAggInfo->Type.MiniFilter.FilterAltitudeLength);
                FilterName[FilterAggInfo->Type.MiniFilter.FilterAltitudeLength] = UNICODE_NULL;
            }

            StringCchPrintfW(Frame, 16, L"%lu", FilterAggInfo->Type.MiniFilter.FrameID);
        }
        else if (FilterAggInfo->Flags & FLTFL_ASI_IS_LEGACYFILTER)
        {
            if (FilterAggInfo->Type.LegacyFilter.FilterNameLength < 128)
            {
                CopyMemory(FilterName,
                           (PCHAR)FilterAggInfo + FilterAggInfo->Type.LegacyFilter.FilterNameBufferOffset,
                           FilterAggInfo->Type.LegacyFilter.FilterNameLength);
                FilterName[FilterAggInfo->Type.LegacyFilter.FilterNameLength] = UNICODE_NULL;
            }

            StringCchCopyW(Frame, 16, L"<Legacy>"); //Fixme: is this localized?
        }

        wprintf(L"%-38s %-10s %-10s %3s\n",
                FilterName,
                NumOfInstances,
                Altitude,
                Frame);
    }
    else
    {
        PFILTER_FULL_INFORMATION FilterInfo;
        FilterInfo = (PFILTER_FULL_INFORMATION)Buffer;

        if (FilterInfo->FilterNameLength < 128)
        {
            CopyMemory(FilterName,
                       FilterInfo->FilterNameBuffer,
                       FilterInfo->FilterNameLength);
            FilterName[FilterInfo->FilterNameLength] = UNICODE_NULL;
        }

        wprintf(L"%-38s %-10lu %-10lu\n",
                FilterName,
                FilterInfo->NumberOfInstances,
                FilterInfo->FrameID);
    }
}

void
PrintVolumeInfo(_In_ PVOID Buffer)
{
    PFILTER_VOLUME_STANDARD_INFORMATION FilterVolInfo;
    WCHAR DosName[16] = { 0 };
    WCHAR VolName[128] = { 0 };
    WCHAR FileSystem[32] = { 0 };

    FilterVolInfo = (PFILTER_VOLUME_STANDARD_INFORMATION)Buffer;

    if (FilterVolInfo->FilterVolumeNameLength < 128)
    {
        CopyMemory(VolName,
                   (PCHAR)FilterVolInfo->FilterVolumeName,
                   FilterVolInfo->FilterVolumeNameLength);
        VolName[FilterVolInfo->FilterVolumeNameLength] = UNICODE_NULL;
    }

    (void)FilterGetDosName(VolName, DosName, 16);

    switch (FilterVolInfo->FileSystemType)
    {
    case FLT_FSTYPE_MUP:
        StringCchCopyW(FileSystem, 32, L"Remote");
        break;

    case FLT_FSTYPE_NTFS:
        StringCchCopyW(FileSystem, 32, L"NTFS");
        break;

    case FLT_FSTYPE_FAT:
        StringCchCopyW(FileSystem, 32, L"FAT");
        break;

    case FLT_FSTYPE_EXFAT:
        StringCchCopyW(FileSystem, 32, L"exFAT");
        break;

    case FLT_FSTYPE_NPFS:
        StringCchCopyW(FileSystem, 32, L"NamedPipe");
        break;

    case FLT_FSTYPE_MSFS:
        StringCchCopyW(FileSystem, 32, L"Mailslot");
        break;

    case FLT_FSTYPE_UNKNOWN:
    default:
        StringCchCopyW(FileSystem, 32, L"<Unknown>");
        break;
    }

    wprintf(L"%-31s %-40s %-10s\n",
            DosName,
            VolName,
            FileSystem);
}

void
ListFilters()
{
    HANDLE FindHandle;
    BYTE Buffer[1024];
    ULONG BytesReturned;
    BOOL IsNewStyle = TRUE;
    HRESULT hr;

    hr = FilterFindFirst(FilterAggregateStandardInformation,
                         Buffer,
                         1024,
                         &BytesReturned,
                         &FindHandle);
    if (!SUCCEEDED(hr))
    {
        IsNewStyle = FALSE;
        hr = FilterFindFirst(FilterFullInformation,
                             Buffer,
                             1024,
                             &BytesReturned,
                             &FindHandle);
    }

    if (SUCCEEDED(hr))
    {
        if (IsNewStyle)
        {
            LoadAndPrintString(IDS_DISPLAY_FILTERS1);
            wprintf(L"------------------------------  -------------  ------------  -----\n");
        }
        else
        {
            LoadAndPrintString(IDS_DISPLAY_FILTERS2);
            wprintf(L"------------------------------  -------------  -----\n");
        }

        PrintFilterInfo(Buffer, IsNewStyle);

        do
        {
            hr = FilterFindNext(FindHandle,
                                IsNewStyle ? FilterAggregateStandardInformation : FilterFullInformation,
                                Buffer,
                                1024,
                                &BytesReturned);
            if (SUCCEEDED(hr))
            {
                PrintFilterInfo(Buffer, IsNewStyle);
            }

        } while (SUCCEEDED(hr));

        FilterFindClose(FindHandle);
    }

    if (!SUCCEEDED(hr) && hr != HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS))
    {
        LoadAndPrintString(IDS_ERROR_FILTERS, hr);
        PrintErrorText(hr);
    }
}

void
ListVolumes()
{
    HANDLE FindHandle;
    BYTE Buffer[1024];
    ULONG BytesReturned;
    HRESULT hr;

    hr = FilterVolumeFindFirst(FilterVolumeStandardInformation,
                               Buffer,
                               1024,
                               &BytesReturned,
                               &FindHandle);
    if (SUCCEEDED(hr))
    {
        LoadAndPrintString(IDS_DISPLAY_VOLUMES);
        wprintf(L"------------------------------  ---------------------------------------  ----------  --------\n");

        PrintVolumeInfo(Buffer);

        do
        {
            hr = FilterVolumeFindNext(FindHandle,
                                      FilterVolumeStandardInformation,
                                      Buffer,
                                      1024,
                                      &BytesReturned);
            if (SUCCEEDED(hr))
            {
                PrintVolumeInfo(Buffer);
            }

        } while (SUCCEEDED(hr));

        FilterVolumeFindClose(FindHandle);
    }

    if (!SUCCEEDED(hr) && hr != HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS))
    {
        LoadAndPrintString(IDS_ERROR_VOLUMES, hr);
        PrintErrorText(hr);
    }
}

int wmain(int argc, WCHAR *argv[])
{
    wprintf(L"\n");

    if ((argc < 2) || (!_wcsicmp(argv[1], L"filters")))
    {
        if (argc < 3)
        {
            ListFilters();
        }
        else
        {
            LoadAndPrintString(IDS_USAGE_FILTERS);
            wprintf(L"fltmc.exe filters\n\n");
        }
    }
    else if (!_wcsicmp(argv[1], L"help"))
    {
        LoadAndPrintString(IDS_USAGE);
    }
    else if (!_wcsicmp(argv[1], L"load"))
    {
        if (argc == 3)
        {
            LoadFilter(argv[2]);
        }
        else
        {
            LoadAndPrintString(IDS_USAGE_LOAD);
            wprintf(L"fltmc.exe load [name]\n\n");
        }
    }
    else if (!_wcsicmp(argv[1], L"unload"))
    {
        if (argc == 3)
        {
            UnloadFilter(argv[2]);
        }
        else
        {
            LoadAndPrintString(IDS_USAGE_UNLOAD);
            wprintf(L"fltmc.exe unload [name]\n\n");
        }
    }
    else if (!_wcsicmp(argv[1], L"volumes"))
    {
        if (argc == 2)
        {
            ListVolumes();
        }
        else
        {
            LoadAndPrintString(IDS_USAGE_VOLUMES);
            wprintf(L"fltmc.exe volumes\n\n");
        }
    }

    return 0;
}
