/*
 * PROJECT:     ReactOS kernel-mode tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Regression test for MountMgr recovery of persisted symbolic links
 * COPYRIGHT:   Copyright 2026 Alejandro Sánchez <alesangreat@gmail.com>
 */

#include <kmt_test.h>
#include <mountmgr.h>

#include "MountMgrVolume.h"

#define MAX_MATCHING_VALUES 32
#define MAX_VALUE_NAME_CCH  128
#define CLEANUP_BUFFER_SIZE 4096

static const WCHAR MountedDevicesKey[] = L"SYSTEM\\MountedDevices";
static const WCHAR VolumePrefix[] = L"\\??\\Volume{";

typedef struct _MATCHING_VALUES
{
    DWORD Count;
    DWORD StoredCount;
    DWORD VolumeCount;
    DWORD SentinelCount;
    WCHAR Names[MAX_MATCHING_VALUES][MAX_VALUE_NAME_CCH];
} MATCHING_VALUES, *PMATCHING_VALUES;

static
BOOL
IsVolumeValueName(
    _In_ PCWSTR ValueName)
{
    return wcsncmp(ValueName,
                   VolumePrefix,
                   RTL_NUMBER_OF(VolumePrefix) - 1) == 0;
}

static
BOOL
ContainsValueName(
    _In_ const MATCHING_VALUES *Values,
    _In_ PCWSTR ValueName)
{
    DWORD Index;

    for (Index = 0; Index < Values->StoredCount; ++Index)
    {
        if (wcscmp(Values->Names[Index], ValueName) == 0)
            return TRUE;
    }

    return FALSE;
}

static
LONG
EnumerateMatchingValues(
    _In_ HKEY Key,
    _In_ const GUID *UniqueId,
    _Out_ PMATCHING_VALUES Values)
{
    DWORD Index;

    ZeroMemory(Values, sizeof(*Values));

    for (Index = 0; ; ++Index)
    {
        LONG Error;
        DWORD Type;
        DWORD NameLength = MAX_VALUE_NAME_CCH;
        DWORD DataLength = 512;
        WCHAR Name[MAX_VALUE_NAME_CCH];
        BYTE Data[512];

        Error = RegEnumValueW(Key,
                              Index,
                              Name,
                              &NameLength,
                              NULL,
                              &Type,
                              Data,
                              &DataLength);
        if (Error == ERROR_NO_MORE_ITEMS)
            return ERROR_SUCCESS;

        if (Error == ERROR_MORE_DATA)
            continue;

        if (Error != ERROR_SUCCESS)
            return Error;

        if (Type != REG_BINARY || DataLength != sizeof(*UniqueId) ||
            memcmp(Data, UniqueId, sizeof(*UniqueId)) != 0)
        {
            continue;
        }

        ++Values->Count;
        if (Name[0] == L'#')
            ++Values->SentinelCount;
        if (IsVolumeValueName(Name))
            ++Values->VolumeCount;

        if (Values->StoredCount < MAX_MATCHING_VALUES)
        {
            wcscpy(Values->Names[Values->StoredCount], Name);
            ++Values->StoredCount;
        }
    }
}

static
BOOL
DeleteMountPointByName(
    _In_ HANDLE MountMgrHandle,
    _In_ PCWSTR SymbolicName,
    _In_ const GUID *UniqueId)
{
    BOOL Ret;
    BYTE Output[CLEANUP_BUFFER_SIZE];
    BYTE *Input;
    DWORD BytesReturned;
    DWORD InputLength;
    ULONG NameBytes;
    PMOUNTMGR_MOUNT_POINT Point;

    NameBytes = (ULONG)(wcslen(SymbolicName) * sizeof(WCHAR));
    InputLength = sizeof(MOUNTMGR_MOUNT_POINT) + NameBytes + sizeof(*UniqueId);
    Input = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, InputLength);
    if (!Input)
        return FALSE;

    Point = (PMOUNTMGR_MOUNT_POINT)Input;
    Point->SymbolicLinkNameOffset = sizeof(MOUNTMGR_MOUNT_POINT);
    Point->SymbolicLinkNameLength = (USHORT)NameBytes;
    Point->UniqueIdOffset = Point->SymbolicLinkNameOffset + NameBytes;
    Point->UniqueIdLength = sizeof(*UniqueId);

    CopyMemory(Input + Point->SymbolicLinkNameOffset, SymbolicName, NameBytes);
    CopyMemory(Input + Point->UniqueIdOffset, UniqueId, sizeof(*UniqueId));

    Ret = DeviceIoControl(MountMgrHandle,
                          IOCTL_MOUNTMGR_DELETE_POINTS,
                          Input,
                          InputLength,
                          Output,
                          sizeof(Output),
                          &BytesReturned,
                          NULL);

    HeapFree(GetProcessHeap(), 0, Input);
    return Ret;
}

static
BOOL
SendArrivalNotification(
    _In_ HANDLE MountMgrHandle,
    _In_ PCWSTR DeviceName)
{
    BOOL Ret;
    DWORD BytesReturned;
    DWORD InputLength;
    ULONG DeviceNameBytes;
    PMOUNTMGR_TARGET_NAME Target;

    DeviceNameBytes = (ULONG)(wcslen(DeviceName) * sizeof(WCHAR));
    InputLength = FIELD_OFFSET(MOUNTMGR_TARGET_NAME, DeviceName) + DeviceNameBytes;
    Target = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, InputLength);
    if (!Target)
        return FALSE;

    Target->DeviceNameLength = (USHORT)DeviceNameBytes;
    CopyMemory(Target->DeviceName, DeviceName, DeviceNameBytes);

    Ret = DeviceIoControl(MountMgrHandle,
                          IOCTL_MOUNTMGR_VOLUME_ARRIVAL_NOTIFICATION,
                          Target,
                          InputLength,
                          NULL,
                          0,
                          &BytesReturned,
                          NULL);

    HeapFree(GetProcessHeap(), 0, Target);
    return Ret;
}

static
VOID
CleanupFixture(
    _In_ HKEY Key,
    _In_ HANDLE MountMgrHandle,
    _In_ const MOUNTMGR_VOLUME_TEST_INFO *Info)
{
    DWORD Index;
    LONG Error;
    MATCHING_VALUES Values;

    Error = EnumerateMatchingValues(Key, &Info->UniqueId, &Values);
    if (Error == ERROR_SUCCESS)
    {
        for (Index = 0; Index < Values.StoredCount; ++Index)
        {
            if (MountMgrHandle != INVALID_HANDLE_VALUE)
                DeleteMountPointByName(MountMgrHandle, Values.Names[Index], &Info->UniqueId);

            RegDeleteValueW(Key, Values.Names[Index]);
        }
    }

    /* These names may have been registry-only and therefore absent from MountMgr memory. */
    RegDeleteValueW(Key, Info->SentinelName);
    RegDeleteValueW(Key, Info->VolumeName);
}

START_TEST(MountMgrVolume)
{
    BOOL Ret;
    DWORD Error;
    DWORD InfoLength;
    HKEY Key = NULL;
    HANDLE MountMgrHandle = INVALID_HANDLE_VALUE;
    MATCHING_VALUES Before, After, Remaining;
    MOUNTMGR_VOLUME_TEST_INFO Info;
    BOOLEAN HaveInfo = FALSE;
    BOOLEAN FixtureWritten = FALSE;

    ZeroMemory(&Info, sizeof(Info));

    Error = KmtLoadAndOpenDriver(L"MountMgrVolume", FALSE);
    ok_eq_int(Error, ERROR_SUCCESS);
    if (Error != ERROR_SUCCESS)
        return;

    InfoLength = sizeof(Info);
    Error = KmtSendBufferToDriver(MOUNTMGR_VOLUME_QUERY_INFO,
                                  &Info,
                                  0,
                                  &InfoLength);
    ok_eq_int(Error, ERROR_SUCCESS);
    ok_eq_ulong(InfoLength, sizeof(Info));
    if (Error != ERROR_SUCCESS || InfoLength != sizeof(Info))
        goto Cleanup;

    HaveInfo = TRUE;
    ok(Info.DeviceName[0] != UNICODE_NULL, "Synthetic device name is empty\n");
    ok(Info.VolumeName[0] != UNICODE_NULL, "Synthetic volume name is empty\n");
    ok(Info.SentinelName[0] == L'#', "Sentinel does not begin with '#': %ls\n", Info.SentinelName);

    MountMgrHandle = CreateFileW(MOUNTMGR_DOS_DEVICE_NAME,
                                 GENERIC_READ | GENERIC_WRITE,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 NULL,
                                 OPEN_EXISTING,
                                 0,
                                 NULL);
    ok(MountMgrHandle != INVALID_HANDLE_VALUE,
       "Unable to open MountMgr: %lu\n", GetLastError());
    if (MountMgrHandle == INVALID_HANDLE_VALUE)
        goto Cleanup;

    Error = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                          MountedDevicesKey,
                          0,
                          KEY_QUERY_VALUE | KEY_SET_VALUE,
                          &Key);
    ok_eq_long(Error, ERROR_SUCCESS);
    if (Error != ERROR_SUCCESS)
        goto Cleanup;

    /* A fresh GUID must not already exist in MountedDevices. */
    Error = EnumerateMatchingValues(Key, &Info.UniqueId, &Before);
    ok_eq_long(Error, ERROR_SUCCESS);
    ok_eq_ulong(Before.Count, 0);
    if (Error != ERROR_SUCCESS || Before.Count != 0)
        goto Cleanup;

    Error = RegSetValueExW(Key,
                           Info.SentinelName,
                           0,
                           REG_BINARY,
                           (const BYTE *)&Info.UniqueId,
                           sizeof(Info.UniqueId));
    ok_eq_long(Error, ERROR_SUCCESS);
    if (Error != ERROR_SUCCESS)
        goto Cleanup;
    FixtureWritten = TRUE;

    Error = RegSetValueExW(Key,
                           Info.VolumeName,
                           0,
                           REG_BINARY,
                           (const BYTE *)&Info.UniqueId,
                           sizeof(Info.UniqueId));
    ok_eq_long(Error, ERROR_SUCCESS);
    if (Error != ERROR_SUCCESS)
        goto Cleanup;

    Error = EnumerateMatchingValues(Key, &Info.UniqueId, &Before);
    ok_eq_long(Error, ERROR_SUCCESS);
    ok_eq_ulong(Before.Count, 2);
    ok_eq_ulong(Before.VolumeCount, 1);
    ok_eq_ulong(Before.SentinelCount, 1);
    ok(ContainsValueName(&Before, Info.VolumeName),
       "Persisted volume link was not found before arrival\n");
    ok(ContainsValueName(&Before, Info.SentinelName),
       "No-drive-letter sentinel was not found before arrival\n");
    if (Error != ERROR_SUCCESS || Before.Count != 2)
        goto Cleanup;

    Ret = SendArrivalNotification(MountMgrHandle, Info.DeviceName);
    ok(Ret == TRUE, "Volume arrival notification failed: %lu\n", GetLastError());
    if (!Ret)
        goto Cleanup;

    Error = EnumerateMatchingValues(Key, &Info.UniqueId, &After);
    ok_eq_long(Error, ERROR_SUCCESS);
    if (Error != ERROR_SUCCESS)
        goto Cleanup;

    /*
     * The persisted Volume GUID is a real symbolic link and the '#' value is
     * only a no-drive-letter sentinel. MountMgr must reuse the former and
     * must not manufacture another Volume GUID for the same UniqueId.
     */
    ok_eq_ulong(After.Count, 2);
    ok_eq_ulong(After.VolumeCount, 1);
    ok_eq_ulong(After.SentinelCount, 1);
    ok(ContainsValueName(&After, Info.VolumeName),
       "Persisted volume link disappeared during arrival\n");
    ok(ContainsValueName(&After, Info.SentinelName),
       "No-drive-letter sentinel disappeared during arrival\n");

Cleanup:
    if (Key && HaveInfo && FixtureWritten)
    {
        CleanupFixture(Key, MountMgrHandle, &Info);

        Error = EnumerateMatchingValues(Key, &Info.UniqueId, &Remaining);
        ok_eq_long(Error, ERROR_SUCCESS);
        if (Error == ERROR_SUCCESS)
            ok_eq_ulong(Remaining.Count, 0);
    }

    if (Key)
        RegCloseKey(Key);

    if (MountMgrHandle != INVALID_HANDLE_VALUE)
        CloseHandle(MountMgrHandle);

    KmtCloseDriver();
    KmtUnloadDriver();
}
