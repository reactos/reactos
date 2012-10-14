
#define TAG_GDEV 'gdev'

VOID
APIENTRY
EngFileWrite(
    IN PFILE_OBJECT pFileObject,
    IN PVOID lpBuffer,
    IN SIZE_T nLength,
    IN PSIZE_T lpBytesWritten);

PGRAPHICS_DEVICE
NTAPI
EngpFindGraphicsDevice(
    PUNICODE_STRING pustrDevice,
    DWORD iDevNum,
    DWORD dwFlags);

PGRAPHICS_DEVICE
NTAPI
EngpRegisterGraphicsDevice(
    PUNICODE_STRING pustrDeviceName,
    PUNICODE_STRING pustrDiplayDrivers,
    PUNICODE_STRING pustrDescription,
    PDEVMODEW pdmDefault);

INIT_FUNCTION
NTSTATUS
NTAPI
InitDeviceImpl(VOID);

//#define KeRosDumpStackFrames(Frames, Count) KdSystemDebugControl(TAG('R', 'o', 's', 'D'), (PVOID)Frames, Count, NULL, 0, NULL, KernelMode)
NTSYSAPI ULONG APIENTRY RtlWalkFrameChain(OUT PVOID *Callers, IN ULONG Count, IN ULONG Flags);

BOOL
NTAPI
PDEVOBJ_bSwitchMode(
    PPDEVOBJ ppdev,
    PDEVMODEW pdm);

PDEVMODEW
NTAPI
PDEVOBJ_pdmMatchDevMode(
    PPDEVOBJ ppdev,
    PDEVMODEW pdm);

extern PGRAPHICS_DEVICE gpPrimaryGraphicsDevice;
extern PGRAPHICS_DEVICE gpVgaGraphicsDevice;
