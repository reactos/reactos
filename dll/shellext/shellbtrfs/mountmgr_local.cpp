#include "shellext.h"
#ifndef __REACTOS__
#include "mountmgr.h"
#else
#include "mountmgr_local.h"
#endif
#include <mountmgr.h>

using namespace std;

mountmgr::mountmgr() {
    UNICODE_STRING us;
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK iosb;
    NTSTATUS Status;

    RtlInitUnicodeString(&us, MOUNTMGR_DEVICE_NAME);
    InitializeObjectAttributes(&attr, &us, 0, nullptr, nullptr);

    Status = NtOpenFile(&h, FILE_GENERIC_READ | FILE_GENERIC_WRITE, &attr, &iosb,
                        FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_ALERT);

    if (!NT_SUCCESS(Status))
        throw ntstatus_error(Status);
}

mountmgr::~mountmgr() {
    NtClose(h);
}

void mountmgr::create_point(const wstring_view& symlink, const wstring_view& device) const {
    NTSTATUS Status;
    IO_STATUS_BLOCK iosb;

    vector<uint8_t> buf(sizeof(MOUNTMGR_CREATE_POINT_INPUT) + ((symlink.length() + device.length()) * sizeof(WCHAR)));
    auto mcpi = reinterpret_cast<MOUNTMGR_CREATE_POINT_INPUT*>(buf.data());

    mcpi->SymbolicLinkNameOffset = sizeof(MOUNTMGR_CREATE_POINT_INPUT);
    mcpi->SymbolicLinkNameLength = (USHORT)(symlink.length() * sizeof(WCHAR));
    mcpi->DeviceNameOffset = (USHORT)(mcpi->SymbolicLinkNameOffset + mcpi->SymbolicLinkNameLength);
    mcpi->DeviceNameLength = (USHORT)(device.length() * sizeof(WCHAR));

    memcpy((uint8_t*)mcpi + mcpi->SymbolicLinkNameOffset, symlink.data(), symlink.length() * sizeof(WCHAR));
    memcpy((uint8_t*)mcpi + mcpi->DeviceNameOffset, device.data(), device.length() * sizeof(WCHAR));

    Status = NtDeviceIoControlFile(h, nullptr, nullptr, nullptr, &iosb, IOCTL_MOUNTMGR_CREATE_POINT,
                                   buf.data(), (ULONG)buf.size(), nullptr, 0);

    if (!NT_SUCCESS(Status))
        throw ntstatus_error(Status);
}

void mountmgr::delete_points(const wstring_view& symlink, const wstring_view& unique_id, const wstring_view& device_name) const {
    NTSTATUS Status;
    IO_STATUS_BLOCK iosb;

    vector<uint8_t> buf(sizeof(MOUNTMGR_MOUNT_POINT) + ((symlink.length() + unique_id.length() + device_name.length()) * sizeof(WCHAR)));
    auto mmp = reinterpret_cast<MOUNTMGR_MOUNT_POINT*>(buf.data());

    memset(mmp, 0, sizeof(MOUNTMGR_MOUNT_POINT));

    if (symlink.length() > 0) {
        mmp->SymbolicLinkNameOffset = sizeof(MOUNTMGR_MOUNT_POINT);
        mmp->SymbolicLinkNameLength = (USHORT)(symlink.length() * sizeof(WCHAR));
        memcpy((uint8_t*)mmp + mmp->SymbolicLinkNameOffset, symlink.data(), symlink.length() * sizeof(WCHAR));
    }

    if (unique_id.length() > 0) {
        if (mmp->SymbolicLinkNameLength == 0)
            mmp->UniqueIdOffset = sizeof(MOUNTMGR_MOUNT_POINT);
        else
            mmp->UniqueIdOffset = mmp->SymbolicLinkNameOffset + mmp->SymbolicLinkNameLength;

        mmp->UniqueIdLength = (USHORT)(unique_id.length() * sizeof(WCHAR));
        memcpy((uint8_t*)mmp + mmp->UniqueIdOffset, unique_id.data(), unique_id.length() * sizeof(WCHAR));
    }

    if (device_name.length() > 0) {
        if (mmp->SymbolicLinkNameLength == 0 && mmp->UniqueIdOffset == 0)
            mmp->DeviceNameOffset = sizeof(MOUNTMGR_MOUNT_POINT);
        else if (mmp->SymbolicLinkNameLength != 0)
            mmp->DeviceNameOffset = mmp->SymbolicLinkNameOffset + mmp->SymbolicLinkNameLength;
        else
            mmp->DeviceNameOffset = mmp->UniqueIdOffset + mmp->UniqueIdLength;

        mmp->DeviceNameLength = (USHORT)(device_name.length() * sizeof(WCHAR));
        memcpy((uint8_t*)mmp + mmp->DeviceNameOffset, device_name.data(), device_name.length() * sizeof(WCHAR));
    }

    vector<uint8_t> buf2(sizeof(MOUNTMGR_MOUNT_POINTS));
    auto mmps = reinterpret_cast<MOUNTMGR_MOUNT_POINTS*>(buf2.data());

    Status = NtDeviceIoControlFile(h, nullptr, nullptr, nullptr, &iosb, IOCTL_MOUNTMGR_DELETE_POINTS,
                                   buf.data(), (ULONG)buf.size(), buf2.data(), (ULONG)buf2.size());

    if (Status == STATUS_BUFFER_OVERFLOW) {
        buf2.resize(mmps->Size);

        Status = NtDeviceIoControlFile(h, nullptr, nullptr, nullptr, &iosb, IOCTL_MOUNTMGR_DELETE_POINTS,
                                       buf.data(), (ULONG)buf.size(), buf2.data(), (ULONG)buf2.size());
    }

    if (!NT_SUCCESS(Status))
        throw ntstatus_error(Status);
}

vector<mountmgr_point> mountmgr::query_points(const wstring_view& symlink, const wstring_view& unique_id, const wstring_view& device_name) const {
    NTSTATUS Status;
    IO_STATUS_BLOCK iosb;
    vector<mountmgr_point> v;

    vector<uint8_t> buf(sizeof(MOUNTMGR_MOUNT_POINT) + ((symlink.length() + unique_id.length() + device_name.length()) * sizeof(WCHAR)));
    auto mmp = reinterpret_cast<MOUNTMGR_MOUNT_POINT*>(buf.data());

    memset(mmp, 0, sizeof(MOUNTMGR_MOUNT_POINT));

    if (symlink.length() > 0) {
        mmp->SymbolicLinkNameOffset = sizeof(MOUNTMGR_MOUNT_POINT);
        mmp->SymbolicLinkNameLength = (USHORT)(symlink.length() * sizeof(WCHAR));
        memcpy((uint8_t*)mmp + mmp->SymbolicLinkNameOffset, symlink.data(), symlink.length() * sizeof(WCHAR));
    }

    if (unique_id.length() > 0) {
        if (mmp->SymbolicLinkNameLength == 0)
            mmp->UniqueIdOffset = sizeof(MOUNTMGR_MOUNT_POINT);
        else
            mmp->UniqueIdOffset = mmp->SymbolicLinkNameOffset + mmp->SymbolicLinkNameLength;

        mmp->UniqueIdLength = (USHORT)(unique_id.length() * sizeof(WCHAR));
        memcpy((uint8_t*)mmp + mmp->UniqueIdOffset, unique_id.data(), unique_id.length() * sizeof(WCHAR));
    }

    if (device_name.length() > 0) {
        if (mmp->SymbolicLinkNameLength == 0 && mmp->UniqueIdOffset == 0)
            mmp->DeviceNameOffset = sizeof(MOUNTMGR_MOUNT_POINT);
        else if (mmp->SymbolicLinkNameLength != 0)
            mmp->DeviceNameOffset = mmp->SymbolicLinkNameOffset + mmp->SymbolicLinkNameLength;
        else
            mmp->DeviceNameOffset = mmp->UniqueIdOffset + mmp->UniqueIdLength;

        mmp->DeviceNameLength = (USHORT)(device_name.length() * sizeof(WCHAR));
        memcpy((uint8_t*)mmp + mmp->DeviceNameOffset, device_name.data(), device_name.length() * sizeof(WCHAR));
    }

    vector<uint8_t> buf2(sizeof(MOUNTMGR_MOUNT_POINTS));
    auto mmps = reinterpret_cast<MOUNTMGR_MOUNT_POINTS*>(buf2.data());

    Status = NtDeviceIoControlFile(h, nullptr, nullptr, nullptr, &iosb, IOCTL_MOUNTMGR_QUERY_POINTS,
                                   buf.data(), (ULONG)buf.size(), buf2.data(), (ULONG)buf2.size());

    if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_OVERFLOW)
        throw ntstatus_error(Status);

    buf2.resize(mmps->Size);
    mmps = reinterpret_cast<MOUNTMGR_MOUNT_POINTS*>(buf2.data());

    Status = NtDeviceIoControlFile(h, nullptr, nullptr, nullptr, &iosb, IOCTL_MOUNTMGR_QUERY_POINTS,
                                   buf.data(), (ULONG)buf.size(), buf2.data(), (ULONG)buf2.size());

    if (!NT_SUCCESS(Status))
        throw ntstatus_error(Status);

    for (ULONG i = 0; i < mmps->NumberOfMountPoints; i++) {
        wstring_view mpsl, mpdn;
        string_view mpuid;

        if (mmps->MountPoints[i].SymbolicLinkNameLength)
            mpsl = wstring_view((WCHAR*)((uint8_t*)mmps + mmps->MountPoints[i].SymbolicLinkNameOffset), mmps->MountPoints[i].SymbolicLinkNameLength / sizeof(WCHAR));

        if (mmps->MountPoints[i].UniqueIdLength)
            mpuid = string_view((char*)((uint8_t*)mmps + mmps->MountPoints[i].UniqueIdOffset), mmps->MountPoints[i].UniqueIdLength);

        if (mmps->MountPoints[i].DeviceNameLength)
            mpdn = wstring_view((WCHAR*)((uint8_t*)mmps + mmps->MountPoints[i].DeviceNameOffset), mmps->MountPoints[i].DeviceNameLength / sizeof(WCHAR));

#ifndef __REACTOS__
        v.emplace_back(mpsl, mpuid, mpdn);
#else
        v.push_back(mountmgr_point(mpsl, mpuid, mpdn));
#endif
    }

    return v;
}
