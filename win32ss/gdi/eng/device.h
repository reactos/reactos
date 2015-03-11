
#pragma once

#define TAG_GDEV 'gdev'

extern PGRAPHICS_DEVICE gpPrimaryGraphicsDevice;
extern PGRAPHICS_DEVICE gpVgaGraphicsDevice;

VOID
APIENTRY
EngFileWrite(
    _In_ PFILE_OBJECT pFileObject,
    _In_reads_(nLength) PVOID lpBuffer,
    _In_ SIZE_T nLength,
    _Out_ PSIZE_T lpBytesWritten);

PGRAPHICS_DEVICE
NTAPI
EngpFindGraphicsDevice(
    _In_opt_ PUNICODE_STRING pustrDevice,
    _In_ ULONG iDevNum,
    _In_ DWORD dwFlags);

PGRAPHICS_DEVICE
NTAPI
EngpRegisterGraphicsDevice(
    _In_ PUNICODE_STRING pustrDeviceName,
    _In_ PUNICODE_STRING pustrDiplayDrivers,
    _In_ PUNICODE_STRING pustrDescription,
    _In_ PDEVMODEW pdmDefault);

INIT_FUNCTION
NTSTATUS
NTAPI
InitDeviceImpl(VOID);

