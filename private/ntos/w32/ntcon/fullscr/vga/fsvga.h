/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    fsvga.h

Abstract:

    This is the console fullscreen driver for the VGA card.

Environment:

    kernel mode only

Notes:

Revision History:

--*/


#ifndef _FSVGA_
#define _FSVGA_

#include "ntddvdeo.h"
#include "fscrnctl.h"
#include "vga.h"

//
// Define the i8042 controller input/output ports.
//

typedef enum _VGA_IO_PORT_TYPE {
    CRTCAddressPortColor = 0,
    CRTCDataPortColor,
    GRAPHAddressPort,
    SEQUAddressPort,
    MaximumPortCount
} VGA_IO_PORT_TYPE;


//
// FSVGA configuration information.
//

typedef struct _FSVGA_CONFIGURATION_INFORMATION {
    USHORT    EmulationMode;
        #define ENABLE_WORD_WRITE_VRAM   0x01

    USHORT    HardwareCursor;
        #define NO_HARDWARE_CURSOR          0
        #define HARDWARE_CURSOR          0x01

    USHORT    HardwareScroll;
        #define NO_HARDWARE_SCROLL          0
        #define HARDWARE_SCROLL          0x01
        #define USE_LINE_COMPARE         0x02
        #define OFFSET_128_TO_NEXT_SLICE 0x04

    //
    // Bus interface type.
    //

    INTERFACE_TYPE InterfaceType;

    //
    // Bus Number.
    //

    ULONG BusNumber;

    //
    // The port/register resources used by this device.
    //

    CM_PARTIAL_RESOURCE_DESCRIPTOR PortList[MaximumPortCount];
    ULONG PortListCount;

} FSVGA_CONFIGURATION_INFORMATION, *PFSVGA_CONFIGURATION_INFORMATION;

//
// EMULATE_BUFFER_INFORMATION structure
//
typedef struct _EMULATE_BUFFER_INFORMATION {
    //
    // Hardware scroll
    //
    WORD StartAddress;
    WORD LineCompare;
    WORD PrevLineCompare;
    WORD BytePerLine;
    WORD MaxScanLine;
    DWORD LimitGRAM;
        #define LIMIT_64K 0x10000L
    WORD DeltaNextFontRow;
    //
    // Color Attributes for last access.
    //
    BYTE ColorFg;
    BYTE ColorBg;
    //
    // Cursor position and attributes for last access.
    VIDEO_CURSOR_ATTRIBUTES CursorAttributes;
    FSVIDEO_CURSOR_POSITION CursorPosition;
    BOOL ShowCursor;
} EMULATE_BUFFER_INFORMATION, *PEMULATE_BUFFER_INFORMATION;

//
// Port device extension.
//

typedef struct _DEVICE_EXTENSION {

    //
    // Indicate which hardware is actually present (display).
    //

    ULONG HardwarePresent;

    //
    // Pointer to the device object.
    //

    PDEVICE_OBJECT DeviceObject;

    //
    // The mapped addresses for this device's registers.
    //

    PVOID DeviceRegisters[MaximumPortCount];

    //
    // Port configuration information.
    //

    FSVGA_CONFIGURATION_INFORMATION Configuration;

    //
    // FSVIDEO_MODE_INFORMATION structure for the current mode
    //
    FSVIDEO_MODE_INFORMATION CurrentMode;

    //
    // FSVIDEO_SCREEN_INFORMATION structure
    //
    FSVIDEO_SCREEN_INFORMATION ScreenAndFont;

    //
    // EMULATE_BUFFER_INFORMATION structure
    //
    EMULATE_BUFFER_INFORMATION EmulateInfo;

    //
    // Set at intialization to indicate that the register
    // addresses must be unmapped when the driver is unloaded.
    //

    BOOLEAN UnmapRegistersRequired;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

//
// Define the base values for the error log packet's UniqueErrorValue field.
//

#define FSVGA_ERROR_VALUE_BASE        1000

//
// Defines for DeviceExtension->HardwarePresent.
//

#define FSVGA_HARDWARE_PRESENT  1


//
// Function prototypes.
//


//
// fsvga.c
//
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
FsVgaConfiguration(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PUNICODE_STRING RegistryPath,
    IN PUNICODE_STRING KeyboardDeviceName
    );

NTSTATUS
FsVgaPeripheralCallout(
    IN PVOID Context,
    IN PUNICODE_STRING PathName,
    IN INTERFACE_TYPE BusType,
    IN ULONG BusNumber,
    IN PKEY_VALUE_FULL_INFORMATION *BusInformation,
    IN CONFIGURATION_TYPE ControllerType,
    IN ULONG ControllerNumber,
    IN PKEY_VALUE_FULL_INFORMATION *ControllerInformation,
    IN CONFIGURATION_TYPE PeripheralType,
    IN ULONG PeripheralNumber,
    IN PKEY_VALUE_FULL_INFORMATION *PeripheralInformation
    );

VOID
FsVgaServiceParameters(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PUNICODE_STRING RegistryPath,
    IN PUNICODE_STRING FsVgaDeviceName
    );

VOID
FsVgaBuildResourceList(
    IN PDEVICE_EXTENSION DeviceExtension,
    OUT PCM_RESOURCE_LIST *ResourceList,
    OUT PULONG ResourceListSize
    );

NTSTATUS
FsVgaOpenCloseDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
FsVgaDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
FsVgaCopyFrameBuffer(
    PDEVICE_EXTENSION DeviceExtension,
    PFSVIDEO_COPY_FRAME_BUFFER CopyFrameBuffer,
    ULONG inputBufferLength
    );

NTSTATUS
FsVgaWriteToFrameBuffer(
    PDEVICE_EXTENSION DeviceExtension,
    PFSVIDEO_WRITE_TO_FRAME_BUFFER WriteFrameBuffer,
    ULONG inputBufferLength
    );

NTSTATUS
FsVgaReverseMousePointer(
    PDEVICE_EXTENSION DeviceExtension,
    PFSVIDEO_REVERSE_MOUSE_POINTER MouseBuffer,
    ULONG inputBufferLength
    );

NTSTATUS
FsVgaSetMode(
    PDEVICE_EXTENSION DeviceExtension,
    PFSVIDEO_MODE_INFORMATION ModeInformation,
    ULONG inputBufferLength
    );

NTSTATUS
FsVgaSetScreenInformation(
    PDEVICE_EXTENSION DeviceExtension,
    PFSVIDEO_SCREEN_INFORMATION ScreenInformation,
    ULONG inputBufferLength
    );

NTSTATUS
FsVgaSetCursorPosition(
    PDEVICE_EXTENSION DeviceExtension,
    PFSVIDEO_CURSOR_POSITION CursorPosition,
    ULONG inputBufferLength
    );

NTSTATUS
FsVgaSetCursorAttribute(
    PDEVICE_EXTENSION DeviceExtension,
    PVIDEO_CURSOR_ATTRIBUTES CursorAttributes,
    ULONG inputBufferLength
    );

VOID
FsVgaLogError(
    IN PDEVICE_OBJECT DeviceObject,
    IN NTSTATUS ErrorCode,
    IN ULONG UniqueErrorValue,
    IN NTSTATUS FinalStatus,
    IN PULONG DumpData,
    IN ULONG DumpCount
    );

#if DBG

VOID
FsVgaDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    );

extern ULONG FsVgaDebug;
#define FsVgaPrint(x) FsVgaDebugPrint x
#else
#define FsVgaPrint(x)
#endif

//
// drawscrn.c
//
ULONG
CalcGRAMSize(
    IN COORD WindowSize,
    IN PDEVICE_EXTENSION DeviceExtension
    );

PUCHAR
CalcGRAMAddress(
    IN COORD WindowSize,
    IN PDEVICE_EXTENSION DeviceExtension
    );

BOOL
IsGRAMRowOver(
    PUCHAR BufPtr,
    BOOL fDbcs,
    PDEVICE_EXTENSION DeviceExtension
    );

PUCHAR
NextGRAMRow(
    PUCHAR BufPtr,
    PDEVICE_EXTENSION DeviceExtension
    );

VOID
memcpyGRAM(
    IN PCHAR TargetPtr,
    IN PCHAR SourcePtr,
    IN ULONG Length
    );

VOID
memcpyGRAMOver(
    IN PCHAR TargetPtr,
    IN PCHAR SourcePtr,
    IN ULONG Length,
    IN PUCHAR FrameBufPtr,
    IN PDEVICE_EXTENSION DeviceExtension
    );

VOID
MoveGRAM(
    IN PCHAR TargetPtr,
    IN PCHAR SourcePtr,
    IN ULONG Length,
    IN PUCHAR FrameBufPtr,
    IN PDEVICE_EXTENSION DeviceExtension
    );

NTSTATUS
FsgVgaInitializeHWFlags(
    PDEVICE_EXTENSION DeviceExtension
    );

NTSTATUS
FsgCopyFrameBuffer(
    PDEVICE_EXTENSION DeviceExtension,
    PFSVIDEO_COPY_FRAME_BUFFER CopyFrameBuffer,
    ULONG inputBufferLength
    );

NTSTATUS
FsgWriteToFrameBuffer(
    PDEVICE_EXTENSION DeviceExtension,
    PFSVIDEO_WRITE_TO_FRAME_BUFFER WriteFrameBuffer,
    ULONG inputBufferLength
    );

NTSTATUS
FsgReverseMousePointer(
    PDEVICE_EXTENSION DeviceExtension,
    PFSVIDEO_REVERSE_MOUSE_POINTER MouseBuffer,
    ULONG inputBufferLength
    );

NTSTATUS
FsgInvertCursor(
    PDEVICE_EXTENSION DeviceExtension,
    BOOL Invert
    );

NTSTATUS
FsgWriteToScreen(
    PUCHAR FrameBuffer,
    PUCHAR BitmapBuffer,
    DWORD cjBytes,
    BOOL fDbcs,
    WORD Attributes1,
    WORD Attributes2,
    PDEVICE_EXTENSION DeviceExtension
    );

NTSTATUS
FsgWriteToScreenCommonLVB(
    PUCHAR FrameBuffer,
    WORD Attributes,
    PDEVICE_EXTENSION DeviceExtension
    );

UCHAR
AccessGRAM_WR(
    PUCHAR FrameBuffer,
    UCHAR  write
    );

UCHAR
AccessGRAM_RW(
    PUCHAR FrameBuffer,
    UCHAR  write
    );

UCHAR
AccessGRAM_AND(
    PUCHAR FrameBuffer,
    UCHAR  write
    );

//
// foncache.c
//
DWORD
CalcBitmapBufferSize(
    IN COORD FontSize,
    IN DWORD dwAlign
    );

VOID
AlignCopyMemory(
    OUT PBYTE pDestBits,
    IN DWORD dwDestAlign,
    IN PBYTE pSrcBits,
    IN DWORD dwSrcAlign,
    IN COORD FontSize
    );

//
// misc.c
//
int
ConvertOutputToOem(
    IN LPWSTR Source,
    IN int SourceLength,    // in chars
    OUT LPSTR Target,
    IN int TargetLength     // in chars
    );

NTSTATUS
TranslateOutputToOem(
    OUT PCHAR_IMAGE_INFO OutputBuffer,
    IN  PCHAR_IMAGE_INFO InputBuffer,
    IN  DWORD Length
    );

//
// port.c
//
VOID
GetHardwareScrollReg(
    PDEVICE_EXTENSION deviceExtension
    );

VOID
SetGRAMWriteMode(
    PDEVICE_EXTENSION deviceExtension
    );

VOID
SetGRAMCopyMode(
    PDEVICE_EXTENSION deviceExtension
    );

VOID
SetGRAMInvertMode(
    PDEVICE_EXTENSION deviceExtension
    );

VOID
set_opaque_bkgnd_proc(
    PDEVICE_EXTENSION deviceExtension,
    PUCHAR FrameBuffer,
    WORD Attributes
    );

VOID
ColorSetGridMask(
    PDEVICE_EXTENSION deviceExtension,
    BYTE BitMask
    );

VOID
ColorSetDirect(
    PDEVICE_EXTENSION deviceExtension,
    PUCHAR FrameBuffer,
    BYTE ColorFg,
    BYTE ColorBg
    );

#endif // _FSVGA_
