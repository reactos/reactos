
#pragma once

#define TAG_GDEV 'gdev'

VOID
NTAPI
PDEVOBJ_vRefreshModeList(
    PPDEVOBJ ppdev);

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
    _In_ ULONG iDevNum);

PGRAPHICS_DEVICE
NTAPI
EngpRegisterGraphicsDevice(
    _In_ PUNICODE_STRING pustrDeviceName,
    _In_ PUNICODE_STRING pustrDiplayDrivers,
    _In_ PUNICODE_STRING pustrDescription);

NTSTATUS
EngpUpdateGraphicsDeviceList(VOID);

CODE_SEG("INIT")
NTSTATUS
NTAPI
InitDeviceImpl(VOID);

