/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            subsystems/mvdm/ntvdm/dos/dos32krnl/dosfiles.h
 * PURPOSE:         DOS32 Files Support
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* DEFINES ********************************************************************/

#define FILE_INFO_STDIN  (1 << 0)
#define FILE_INFO_STDOUT (1 << 1)
#define FILE_INFO_BINARY (1 << 5)
#define FILE_INFO_DEVICE (1 << 7)

#pragma pack(push, 1)

#if 0 // Real DOS-5 SFT entry, for reference only
typedef struct _DOS_FILE_DESCRIPTOR_DOS5
{
    WORD RefCount;                              // 0x00
    WORD OpenMode;                              // 0x02
    BYTE Attributes;                            // 0x04
    WORD DeviceInfo;                            // 0x05
    DWORD DevicePointer;                        // 0x07
    WORD StartCluster;                          // 0x0b
    WORD Time;                                  // 0x0d
    WORD Date;                                  // 0x0f
    DWORD Size;                                 // 0x11
    DWORD Position;                             // 0x15
    BYTE Reserved0[7];                          // 0x19
    CHAR FileName[11];                          // 0x20
    BYTE Reserved1[6];                          // 0x2b
    WORD OwnerPsp;                              // 0x31
    BYTE Reserved2[8];                          // 0x33
} DOS_FILE_DESCRIPTOR_DOS5, *PDOS_FILE_DESCRIPTOR_DOS5;

C_ASSERT(sizeof(DOS_FILE_DESCRIPTOR_DOS5) == 0x3B);
#endif

// Modified DOS SFT entry, compatible for NTVDM only
typedef struct _DOS_FILE_DESCRIPTOR
{
    WORD RefCount;
    WORD OpenMode;
    BYTE Attributes;
    WORD DeviceInfo;
    DWORD DevicePointer;
    WORD Time;
    WORD Date;
    DWORD Size;
    DWORD Position;
    DWORD Reserved;
    WORD OwnerPsp;
    HANDLE Win32Handle;
    CHAR FileName[11];
    BYTE Padding[0x13 - sizeof(HANDLE)];
} DOS_FILE_DESCRIPTOR, *PDOS_FILE_DESCRIPTOR;

C_ASSERT(sizeof(DOS_FILE_DESCRIPTOR) == 0x3B);

typedef struct _DOS_SFT
{
    DWORD Link;
    WORD NumDescriptors;
    DOS_FILE_DESCRIPTOR FileDescriptors[ANYSIZE_ARRAY];
} DOS_SFT, *PDOS_SFT;

/* FUNCTIONS ******************************************************************/

BYTE DosFindFreeDescriptor(VOID);
BYTE DosFindWin32Descriptor(HANDLE Win32Handle);
BYTE DosFindDeviceDescriptor(DWORD DevicePointer);
PDOS_FILE_DESCRIPTOR DosGetFileDescriptor(BYTE Id);
PDOS_FILE_DESCRIPTOR DosGetHandleFileDescriptor(WORD DosHandle);

WORD DosCreateFileEx
(
    LPWORD Handle,
    LPWORD CreationStatus,
    LPCSTR FilePath,
    BYTE AccessShareModes,
    WORD CreateActionFlags,
    WORD Attributes
);

WORD DosCreateFile
(
    LPWORD Handle,
    LPCSTR FilePath,
    DWORD CreationDisposition,
    WORD Attributes
);

WORD DosOpenFile
(
    LPWORD Handle,
    LPCSTR FilePath,
    BYTE AccessShareModes
);

WORD DosReadFile
(
    WORD FileHandle,
    DWORD Buffer,
    WORD Count,
    LPWORD BytesRead
);

WORD DosWriteFile
(
    WORD FileHandle,
    DWORD Buffer,
    WORD Count,
    LPWORD BytesWritten
);

WORD DosSeekFile
(
    WORD FileHandle,
    LONG Offset,
    BYTE Origin,
    LPDWORD NewOffset
);

BOOL DosFlushFileBuffers(WORD FileHandle);
BOOLEAN DosLockFile(WORD DosHandle, DWORD Offset, DWORD Size);
BOOLEAN DosUnlockFile(WORD DosHandle, DWORD Offset, DWORD Size);

BOOLEAN DosDeviceIoControl
(
    WORD FileHandle,
    BYTE ControlCode,
    DWORD Buffer,
    PWORD Length
);

#pragma pack(pop)
