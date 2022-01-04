/******************************************************************************
 *
 * Name: acefiex.h - Extra OS specific defines, etc. for EFI
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2021, Intel Corp.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce at minimum a disclaimer
 *    substantially similar to the "NO WARRANTY" disclaimer below
 *    ("Disclaimer") and any redistribution must be conditioned upon
 *    including a substantially similar Disclaimer requirement for further
 *    binary redistribution.
 * 3. Neither the names of the above-listed copyright holders nor the names
 *    of any contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * NO WARRANTY
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES.
 */

#ifndef __ACEFIEX_H__
#define __ACEFIEX_H__


#ifndef ACPI_USE_SYSTEM_CLIBRARY

typedef signed char                     int8_t;
typedef short int                       int16_t;
typedef int                             int32_t;
typedef unsigned char                   uint8_t;
typedef unsigned short int              uint16_t;
typedef unsigned int                    uint32_t;
typedef COMPILER_DEPENDENT_INT64        int64_t;
typedef COMPILER_DEPENDENT_UINT64       uint64_t;

#endif /* ACPI_USE_SYSTEM_CLIBRARY */

#define ACPI_EFI_ERROR(a)               (((INTN) a) < 0)
#define ACPI_EFI_SUCCESS                0
#define ACPI_EFI_LOAD_ERROR             ACPI_EFI_ERR(1)
#define ACPI_EFI_INVALID_PARAMETER      ACPI_EFI_ERR(2)
#define ACPI_EFI_UNSUPPORTED            ACPI_EFI_ERR(3)
#define ACPI_EFI_BAD_BUFFER_SIZE        ACPI_EFI_ERR(4)
#define ACPI_EFI_BUFFER_TOO_SMALL       ACPI_EFI_ERR(5)
#define ACPI_EFI_NOT_READY              ACPI_EFI_ERR(6)
#define ACPI_EFI_DEVICE_ERROR           ACPI_EFI_ERR(7)
#define ACPI_EFI_WRITE_PROTECTED        ACPI_EFI_ERR(8)
#define ACPI_EFI_OUT_OF_RESOURCES       ACPI_EFI_ERR(9)
#define ACPI_EFI_VOLUME_CORRUPTED       ACPI_EFI_ERR(10)
#define ACPI_EFI_VOLUME_FULL            ACPI_EFI_ERR(11)
#define ACPI_EFI_NO_MEDIA               ACPI_EFI_ERR(12)
#define ACPI_EFI_MEDIA_CHANGED          ACPI_EFI_ERR(13)
#define ACPI_EFI_NOT_FOUND              ACPI_EFI_ERR(14)
#define ACPI_EFI_ACCESS_DENIED          ACPI_EFI_ERR(15)
#define ACPI_EFI_NO_RESPONSE            ACPI_EFI_ERR(16)
#define ACPI_EFI_NO_MAPPING             ACPI_EFI_ERR(17)
#define ACPI_EFI_TIMEOUT                ACPI_EFI_ERR(18)
#define ACPI_EFI_NOT_STARTED            ACPI_EFI_ERR(19)
#define ACPI_EFI_ALREADY_STARTED        ACPI_EFI_ERR(20)
#define ACPI_EFI_ABORTED                ACPI_EFI_ERR(21)
#define ACPI_EFI_PROTOCOL_ERROR         ACPI_EFI_ERR(24)


typedef UINTN ACPI_EFI_STATUS;
typedef VOID *ACPI_EFI_HANDLE;
typedef VOID *ACPI_EFI_EVENT;

typedef struct {
    UINT32  Data1;
    UINT16  Data2;
    UINT16  Data3;
    UINT8   Data4[8];
} ACPI_EFI_GUID;

typedef struct {
    UINT16 Year;       /* 1998 - 20XX */
    UINT8  Month;      /* 1 - 12 */
    UINT8  Day;        /* 1 - 31 */
    UINT8  Hour;       /* 0 - 23 */
    UINT8  Minute;     /* 0 - 59 */
    UINT8  Second;     /* 0 - 59 */
    UINT8  Pad1;
    UINT32 Nanosecond; /* 0 - 999,999,999 */
    INT16  TimeZone;   /* -1440 to 1440 or 2047 */
    UINT8  Daylight;
    UINT8  Pad2;
} ACPI_EFI_TIME;

typedef struct _ACPI_EFI_DEVICE_PATH {
        UINT8                           Type;
        UINT8                           SubType;
        UINT8                           Length[2];
} ACPI_EFI_DEVICE_PATH;

typedef UINT64          ACPI_EFI_PHYSICAL_ADDRESS;
typedef UINT64          ACPI_EFI_VIRTUAL_ADDRESS;

typedef enum {
    AcpiEfiAllocateAnyPages,
    AcpiEfiAllocateMaxAddress,
    AcpiEfiAllocateAddress,
    AcpiEfiMaxAllocateType
} ACPI_EFI_ALLOCATE_TYPE;

typedef enum {
    AcpiEfiReservedMemoryType,
    AcpiEfiLoaderCode,
    AcpiEfiLoaderData,
    AcpiEfiBootServicesCode,
    AcpiEfiBootServicesData,
    AcpiEfiRuntimeServicesCode,
    AcpiEfiRuntimeServicesData,
    AcpiEfiConventionalMemory,
    AcpiEfiUnusableMemory,
    AcpiEfiACPIReclaimMemory,
    AcpiEfiACPIMemoryNVS,
    AcpiEfiMemoryMappedIO,
    AcpiEfiMemoryMappedIOPortSpace,
    AcpiEfiPalCode,
    AcpiEfiMaxMemoryType
} ACPI_EFI_MEMORY_TYPE;

/* possible caching types for the memory range */
#define ACPI_EFI_MEMORY_UC      0x0000000000000001
#define ACPI_EFI_MEMORY_WC      0x0000000000000002
#define ACPI_EFI_MEMORY_WT      0x0000000000000004
#define ACPI_EFI_MEMORY_WB      0x0000000000000008
#define ACPI_EFI_MEMORY_UCE     0x0000000000000010

/* physical memory protection on range */
#define ACPI_EFI_MEMORY_WP      0x0000000000001000
#define ACPI_EFI_MEMORY_RP      0x0000000000002000
#define ACPI_EFI_MEMORY_XP      0x0000000000004000

/* range requires a runtime mapping */
#define ACPI_EFI_MEMORY_RUNTIME 0x8000000000000000

#define ACPI_EFI_MEMORY_DESCRIPTOR_VERSION  1
typedef struct {
    UINT32                          Type;
    UINT32                          Pad;
    ACPI_EFI_PHYSICAL_ADDRESS       PhysicalStart;
    ACPI_EFI_VIRTUAL_ADDRESS        VirtualStart;
    UINT64                          NumberOfPages;
    UINT64                          Attribute;
} ACPI_EFI_MEMORY_DESCRIPTOR;

typedef struct _ACPI_EFI_TABLE_HEARDER {
    UINT64                      Signature;
    UINT32                      Revision;
    UINT32                      HeaderSize;
    UINT32                      CRC32;
    UINT32                      Reserved;
} ACPI_EFI_TABLE_HEADER;

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_UNKNOWN_INTERFACE) (
    void);


/*
 * Text output protocol
 */
#define ACPI_SIMPLE_TEXT_OUTPUT_PROTOCOL \
    { 0x387477c2, 0x69c7, 0x11d2, {0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b} }

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_TEXT_RESET) (
    struct _ACPI_SIMPLE_TEXT_OUTPUT_INTERFACE   *This,
    BOOLEAN                                     ExtendedVerification);

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_TEXT_OUTPUT_STRING) (
    struct _ACPI_SIMPLE_TEXT_OUTPUT_INTERFACE   *This,
    CHAR16                                      *String);

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_TEXT_TEST_STRING) (
    struct _ACPI_SIMPLE_TEXT_OUTPUT_INTERFACE   *This,
    CHAR16                                      *String);

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_TEXT_QUERY_MODE) (
    struct _ACPI_SIMPLE_TEXT_OUTPUT_INTERFACE   *This,
    UINTN                                       ModeNumber,
    UINTN                                       *Columns,
    UINTN                                       *Rows);

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_TEXT_SET_MODE) (
    struct _ACPI_SIMPLE_TEXT_OUTPUT_INTERFACE   *This,
    UINTN                                       ModeNumber);

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_TEXT_SET_ATTRIBUTE) (
    struct _ACPI_SIMPLE_TEXT_OUTPUT_INTERFACE   *This,
    UINTN                                       Attribute);

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_TEXT_CLEAR_SCREEN) (
    struct _ACPI_SIMPLE_TEXT_OUTPUT_INTERFACE   *This);

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_TEXT_SET_CURSOR_POSITION) (
    struct _ACPI_SIMPLE_TEXT_OUTPUT_INTERFACE   *This,
    UINTN                                       Column,
    UINTN                                       Row);

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_TEXT_ENABLE_CURSOR) (
    struct _ACPI_SIMPLE_TEXT_OUTPUT_INTERFACE   *This,
    BOOLEAN                                     Enable);

typedef struct {
    INT32                           MaxMode;
    INT32                           Mode;
    INT32                           Attribute;
    INT32                           CursorColumn;
    INT32                           CursorRow;
    BOOLEAN                         CursorVisible;
} ACPI_SIMPLE_TEXT_OUTPUT_MODE;

typedef struct _ACPI_SIMPLE_TEXT_OUTPUT_INTERFACE {
    ACPI_EFI_TEXT_RESET                 Reset;

    ACPI_EFI_TEXT_OUTPUT_STRING         OutputString;
    ACPI_EFI_TEXT_TEST_STRING           TestString;

    ACPI_EFI_TEXT_QUERY_MODE            QueryMode;
    ACPI_EFI_TEXT_SET_MODE              SetMode;
    ACPI_EFI_TEXT_SET_ATTRIBUTE         SetAttribute;

    ACPI_EFI_TEXT_CLEAR_SCREEN          ClearScreen;
    ACPI_EFI_TEXT_SET_CURSOR_POSITION   SetCursorPosition;
    ACPI_EFI_TEXT_ENABLE_CURSOR         EnableCursor;

    ACPI_SIMPLE_TEXT_OUTPUT_MODE        *Mode;
} ACPI_SIMPLE_TEXT_OUTPUT_INTERFACE;

/*
 * Text input protocol
 */
#define ACPI_SIMPLE_TEXT_INPUT_PROTOCOL  \
    { 0x387477c1, 0x69c7, 0x11d2, {0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b} }

typedef struct {
    UINT16                              ScanCode;
    CHAR16                              UnicodeChar;
} ACPI_EFI_INPUT_KEY;

/*
 * Baseline unicode control chars
 */
#define CHAR_NULL                       0x0000
#define CHAR_BACKSPACE                  0x0008
#define CHAR_TAB                        0x0009
#define CHAR_LINEFEED                   0x000A
#define CHAR_CARRIAGE_RETURN            0x000D

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_INPUT_RESET) (
    struct _ACPI_SIMPLE_INPUT_INTERFACE         *This,
    BOOLEAN                                     ExtendedVerification);

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_INPUT_READ_KEY) (
    struct _ACPI_SIMPLE_INPUT_INTERFACE         *This,
    ACPI_EFI_INPUT_KEY                          *Key);

typedef struct _ACPI_SIMPLE_INPUT_INTERFACE {
    ACPI_EFI_INPUT_RESET                    Reset;
    ACPI_EFI_INPUT_READ_KEY                 ReadKeyStroke;
    ACPI_EFI_EVENT                          WaitForKey;
} ACPI_SIMPLE_INPUT_INTERFACE;


/*
 * Simple file system protocol
 */
#define ACPI_SIMPLE_FILE_SYSTEM_PROTOCOL \
    { 0x964e5b22, 0x6459, 0x11d2, {0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b} }

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_VOLUME_OPEN) (
    struct _ACPI_EFI_FILE_IO_INTERFACE          *This,
    struct _ACPI_EFI_FILE_HANDLE                **Root);

#define ACPI_EFI_FILE_IO_INTERFACE_REVISION     0x00010000

typedef struct _ACPI_EFI_FILE_IO_INTERFACE {
    UINT64                  Revision;
    ACPI_EFI_VOLUME_OPEN    OpenVolume;
} ACPI_EFI_FILE_IO_INTERFACE;

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_FILE_OPEN) (
    struct _ACPI_EFI_FILE_HANDLE                *File,
    struct _ACPI_EFI_FILE_HANDLE                **NewHandle,
    CHAR16                                      *FileName,
    UINT64                                      OpenMode,
    UINT64                                      Attributes);

/* Values for OpenMode used above */

#define ACPI_EFI_FILE_MODE_READ     0x0000000000000001
#define ACPI_EFI_FILE_MODE_WRITE    0x0000000000000002
#define ACPI_EFI_FILE_MODE_CREATE   0x8000000000000000

/* Values for Attribute used above */

#define ACPI_EFI_FILE_READ_ONLY     0x0000000000000001
#define ACPI_EFI_FILE_HIDDEN        0x0000000000000002
#define ACPI_EFI_FILE_SYSTEM        0x0000000000000004
#define ACPI_EFI_FILE_RESERVIED     0x0000000000000008
#define ACPI_EFI_FILE_DIRECTORY     0x0000000000000010
#define ACPI_EFI_FILE_ARCHIVE       0x0000000000000020
#define ACPI_EFI_FILE_VALID_ATTR    0x0000000000000037

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_FILE_CLOSE) (
    struct _ACPI_EFI_FILE_HANDLE                *File);

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_FILE_DELETE) (
    struct _ACPI_EFI_FILE_HANDLE                *File);

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_FILE_READ) (
    struct _ACPI_EFI_FILE_HANDLE                *File,
    UINTN                                       *BufferSize,
    VOID                                        *Buffer);

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_FILE_WRITE) (
    struct _ACPI_EFI_FILE_HANDLE                *File,
    UINTN                                       *BufferSize,
    VOID                                        *Buffer);

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_FILE_SET_POSITION) (
    struct _ACPI_EFI_FILE_HANDLE                *File,
    UINT64                                      Position);

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_FILE_GET_POSITION) (
    struct _ACPI_EFI_FILE_HANDLE                *File,
    UINT64                                      *Position);

#define ACPI_EFI_FILE_INFO_ID \
    { 0x9576e92, 0x6d3f, 0x11d2, {0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b} }

typedef struct {
    UINT64 Size;
    UINT64 FileSize;
    UINT64 PhysicalSize;
    ACPI_EFI_TIME CreateTime;
    ACPI_EFI_TIME LastAccessTime;
    ACPI_EFI_TIME ModificationTime;
    UINT64 Attribute;
    CHAR16 FileName[1];
} ACPI_EFI_FILE_INFO;

#define SIZE_OF_ACPI_EFI_FILE_INFO  ACPI_OFFSET(ACPI_EFI_FILE_INFO, FileName)

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_FILE_GET_INFO) (
    struct _ACPI_EFI_FILE_HANDLE                *File,
    ACPI_EFI_GUID                               *InformationType,
    UINTN                                       *BufferSize,
    VOID                                        *Buffer);

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_FILE_SET_INFO) (
    struct _ACPI_EFI_FILE_HANDLE                *File,
    ACPI_EFI_GUID                               *InformationType,
    UINTN                                       BufferSize,
    VOID                                        *Buffer);

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_FILE_FLUSH) (
    struct _ACPI_EFI_FILE_HANDLE                *File);


#define ACPI_EFI_FILE_HANDLE_REVISION           0x00010000

typedef struct _ACPI_EFI_FILE_HANDLE {
    UINT64                      Revision;
    ACPI_EFI_FILE_OPEN          Open;
    ACPI_EFI_FILE_CLOSE         Close;
    ACPI_EFI_FILE_DELETE        Delete;
    ACPI_EFI_FILE_READ          Read;
    ACPI_EFI_FILE_WRITE         Write;
    ACPI_EFI_FILE_GET_POSITION  GetPosition;
    ACPI_EFI_FILE_SET_POSITION  SetPosition;
    ACPI_EFI_FILE_GET_INFO      GetInfo;
    ACPI_EFI_FILE_SET_INFO      SetInfo;
    ACPI_EFI_FILE_FLUSH         Flush;
} ACPI_EFI_FILE_STRUCT, *ACPI_EFI_FILE_HANDLE;


/*
 * Loaded image protocol
 */
#define ACPI_EFI_LOADED_IMAGE_PROTOCOL \
    { 0x5B1B31A1, 0x9562, 0x11d2, {0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B} }

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_IMAGE_ENTRY_POINT) (
    ACPI_EFI_HANDLE                         ImageHandle,
    struct _ACPI_EFI_SYSTEM_TABLE           *SystemTable);

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_IMAGE_LOAD) (
    BOOLEAN                         BootPolicy,
    ACPI_EFI_HANDLE                 ParentImageHandle,
    ACPI_EFI_DEVICE_PATH            *FilePath,
    VOID                            *SourceBuffer,
    UINTN                           SourceSize,
    ACPI_EFI_HANDLE                 *ImageHandle);

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_IMAGE_START) (
    ACPI_EFI_HANDLE                 ImageHandle,
    UINTN                           *ExitDataSize,
    CHAR16                          **ExitData);

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_EXIT) (
    ACPI_EFI_HANDLE                 ImageHandle,
    ACPI_EFI_STATUS                 ExitStatus,
    UINTN                           ExitDataSize,
    CHAR16                          *ExitData);

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_IMAGE_UNLOAD) (
    ACPI_EFI_HANDLE                 ImageHandle);


typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_STALL) (
    UINTN                           Microseconds);

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_SET_WATCHDOG_TIMER) (
    UINTN                           Timeout,
    UINT64                          WatchdogCode,
    UINTN                           DataSize,
    CHAR16                          *WatchdogData);


#define EFI_IMAGE_INFORMATION_REVISION      0x1000
typedef struct {
    UINT32                          Revision;
    ACPI_EFI_HANDLE                 ParentHandle;
    struct _ACPI_EFI_SYSTEM_TABLE   *SystemTable;
    ACPI_EFI_HANDLE                 DeviceHandle;
    ACPI_EFI_DEVICE_PATH            *FilePath;
    VOID                            *Reserved;
    UINT32                          LoadOptionsSize;
    VOID                            *LoadOptions;
    VOID                            *ImageBase;
    UINT64                          ImageSize;
    ACPI_EFI_MEMORY_TYPE            ImageCodeType;
    ACPI_EFI_MEMORY_TYPE            ImageDataType;
    ACPI_EFI_IMAGE_UNLOAD           Unload;

} ACPI_EFI_LOADED_IMAGE;


/*
 * EFI Memory
 */
typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_ALLOCATE_PAGES) (
    ACPI_EFI_ALLOCATE_TYPE          Type,
    ACPI_EFI_MEMORY_TYPE            MemoryType,
    UINTN                           NoPages,
    ACPI_EFI_PHYSICAL_ADDRESS       *Memory);

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_FREE_PAGES) (
    ACPI_EFI_PHYSICAL_ADDRESS       Memory,
    UINTN                           NoPages);

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_GET_MEMORY_MAP) (
    UINTN                           *MemoryMapSize,
    ACPI_EFI_MEMORY_DESCRIPTOR      *MemoryMap,
    UINTN                           *MapKey,
    UINTN                           *DescriptorSize,
    UINT32                          *DescriptorVersion);

#define NextMemoryDescriptor(Ptr,Size)  ((ACPI_EFI_MEMORY_DESCRIPTOR *) (((UINT8 *) Ptr) + Size))

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_ALLOCATE_POOL) (
    ACPI_EFI_MEMORY_TYPE            PoolType,
    UINTN                           Size,
    VOID                            **Buffer);

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_FREE_POOL) (
    VOID                            *Buffer);


/*
 * EFI Time
 */
typedef struct {
    UINT32 Resolution;
    UINT32 Accuracy;
    BOOLEAN SetsToZero;
} ACPI_EFI_TIME_CAPABILITIES;

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_GET_TIME) (
    ACPI_EFI_TIME                   *Time,
    ACPI_EFI_TIME_CAPABILITIES      *Capabilities);

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_SET_TIME) (
    ACPI_EFI_TIME                   *Time);


/*
 * Protocol handler functions
 */
typedef enum {
    ACPI_EFI_NATIVE_INTERFACE,
    ACPI_EFI_PCODE_INTERFACE
} ACPI_EFI_INTERFACE_TYPE;

typedef enum {
    AcpiEfiAllHandles,
    AcpiEfiByRegisterNotify,
    AcpiEfiByProtocol
} ACPI_EFI_LOCATE_SEARCH_TYPE;

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_INSTALL_PROTOCOL_INTERFACE) (
    ACPI_EFI_HANDLE                 *Handle,
    ACPI_EFI_GUID                   *Protocol,
    ACPI_EFI_INTERFACE_TYPE         InterfaceType,
    VOID                            *Interface);

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_REINSTALL_PROTOCOL_INTERFACE) (
    ACPI_EFI_HANDLE                 Handle,
    ACPI_EFI_GUID                   *Protocol,
    VOID                            *OldInterface,
    VOID                            *NewInterface);

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_UNINSTALL_PROTOCOL_INTERFACE) (
    ACPI_EFI_HANDLE                 Handle,
    ACPI_EFI_GUID                   *Protocol,
    VOID                            *Interface);

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_HANDLE_PROTOCOL) (
    ACPI_EFI_HANDLE                 Handle,
    ACPI_EFI_GUID                   *Protocol,
    VOID                            **Interface);

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_REGISTER_PROTOCOL_NOTIFY) (
    ACPI_EFI_GUID                   *Protocol,
    ACPI_EFI_EVENT                  Event,
    VOID                            **Registration);

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_LOCATE_HANDLE) (
    ACPI_EFI_LOCATE_SEARCH_TYPE     SearchType,
    ACPI_EFI_GUID                   *Protocol,
    VOID                            *SearchKey,
    UINTN                           *BufferSize,
    ACPI_EFI_HANDLE                 *Buffer);

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_LOCATE_DEVICE_PATH) (
    ACPI_EFI_GUID                   *Protocol,
    ACPI_EFI_DEVICE_PATH            **DevicePath,
    ACPI_EFI_HANDLE                 *Device);

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_INSTALL_CONFIGURATION_TABLE) (
    ACPI_EFI_GUID                   *Guid,
    VOID                            *Table);

#define ACPI_EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL  0x00000001
#define ACPI_EFI_OPEN_PROTOCOL_GET_PROTOCOL        0x00000002
#define ACPI_EFI_OPEN_PROTOCOL_TEST_PROTOCOL       0x00000004
#define ACPI_EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER 0x00000008
#define ACPI_EFI_OPEN_PROTOCOL_BY_DRIVER           0x00000010
#define ACPI_EFI_OPEN_PROTOCOL_EXCLUSIVE           0x00000020

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_OPEN_PROTOCOL) (
    ACPI_EFI_HANDLE                 Handle,
    ACPI_EFI_GUID                   *Protocol,
    VOID                            **Interface,
    ACPI_EFI_HANDLE                 AgentHandle,
    ACPI_EFI_HANDLE                 ControllerHandle,
    UINT32                          Attributes);

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_CLOSE_PROTOCOL) (
    ACPI_EFI_HANDLE                 Handle,
    ACPI_EFI_GUID                   *Protocol,
    ACPI_EFI_HANDLE                 AgentHandle,
    ACPI_EFI_HANDLE                 ControllerHandle);

typedef struct {
    ACPI_EFI_HANDLE                 AgentHandle;
    ACPI_EFI_HANDLE                 ControllerHandle;
    UINT32                          Attributes;
    UINT32                          OpenCount;
} ACPI_EFI_OPEN_PROTOCOL_INFORMATION_ENTRY;

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_OPEN_PROTOCOL_INFORMATION) (
    ACPI_EFI_HANDLE                 Handle,
    ACPI_EFI_GUID                   *Protocol,
    ACPI_EFI_OPEN_PROTOCOL_INFORMATION_ENTRY **EntryBuffer,
    UINTN                           *EntryCount);

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_PROTOCOLS_PER_HANDLE) (
    ACPI_EFI_HANDLE                 Handle,
    ACPI_EFI_GUID                   ***ProtocolBuffer,
    UINTN                           *ProtocolBufferCount);

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_LOCATE_HANDLE_BUFFER) (
    ACPI_EFI_LOCATE_SEARCH_TYPE     SearchType,
    ACPI_EFI_GUID                   *Protocol,
    VOID                            *SearchKey,
    UINTN                           *NoHandles,
    ACPI_EFI_HANDLE                 **Buffer);

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_LOCATE_PROTOCOL) (
    ACPI_EFI_GUID                   *Protocol,
    VOID                            *Registration,
    VOID                            **Interface);

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_INSTALL_MULTIPLE_PROTOCOL_INTERFACES) (
    ACPI_EFI_HANDLE                 *Handle,
    ...);

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_UNINSTALL_MULTIPLE_PROTOCOL_INTERFACES) (
    ACPI_EFI_HANDLE                 Handle,
    ...);

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_CALCULATE_CRC32) (
    VOID                            *Data,
    UINTN                           DataSize,
    UINT32                          *Crc32);

typedef
VOID
(ACPI_EFI_API *ACPI_EFI_COPY_MEM) (
    VOID                            *Destination,
    VOID                            *Source,
    UINTN                           Length);

typedef
VOID
(ACPI_EFI_API *ACPI_EFI_SET_MEM) (
    VOID                            *Buffer,
    UINTN                           Size,
    UINT8                           Value);

/*
 * EFI Boot Services Table
 */
#define ACPI_EFI_BOOT_SERVICES_SIGNATURE 0x56524553544f4f42
#define ACPI_EFI_BOOT_SERVICES_REVISION  (ACPI_EFI_SPECIFICATION_MAJOR_REVISION<<16) | (ACPI_EFI_SPECIFICATION_MINOR_REVISION)

typedef struct _ACPI_EFI_BOOT_SERVICES {
    ACPI_EFI_TABLE_HEADER               Hdr;

#if 0
    ACPI_EFI_RAISE_TPL                  RaiseTPL;
    ACPI_EFI_RESTORE_TPL                RestoreTPL;
#else
    ACPI_EFI_UNKNOWN_INTERFACE          RaiseTPL;
    ACPI_EFI_UNKNOWN_INTERFACE          RestoreTPL;
#endif

    ACPI_EFI_ALLOCATE_PAGES             AllocatePages;
    ACPI_EFI_FREE_PAGES                 FreePages;
    ACPI_EFI_GET_MEMORY_MAP             GetMemoryMap;
    ACPI_EFI_ALLOCATE_POOL              AllocatePool;
    ACPI_EFI_FREE_POOL                  FreePool;

#if 0
    ACPI_EFI_CREATE_EVENT               CreateEvent;
    ACPI_EFI_SET_TIMER                  SetTimer;
    ACPI_EFI_WAIT_FOR_EVENT             WaitForEvent;
    ACPI_EFI_SIGNAL_EVENT               SignalEvent;
    ACPI_EFI_CLOSE_EVENT                CloseEvent;
    ACPI_EFI_CHECK_EVENT                CheckEvent;
#else
    ACPI_EFI_UNKNOWN_INTERFACE          CreateEvent;
    ACPI_EFI_UNKNOWN_INTERFACE          SetTimer;
    ACPI_EFI_UNKNOWN_INTERFACE          WaitForEvent;
    ACPI_EFI_UNKNOWN_INTERFACE          SignalEvent;
    ACPI_EFI_UNKNOWN_INTERFACE          CloseEvent;
    ACPI_EFI_UNKNOWN_INTERFACE          CheckEvent;
#endif

    ACPI_EFI_INSTALL_PROTOCOL_INTERFACE InstallProtocolInterface;
    ACPI_EFI_REINSTALL_PROTOCOL_INTERFACE ReinstallProtocolInterface;
    ACPI_EFI_UNINSTALL_PROTOCOL_INTERFACE UninstallProtocolInterface;
    ACPI_EFI_HANDLE_PROTOCOL            HandleProtocol;
    ACPI_EFI_HANDLE_PROTOCOL            PCHandleProtocol;
    ACPI_EFI_REGISTER_PROTOCOL_NOTIFY   RegisterProtocolNotify;
    ACPI_EFI_LOCATE_HANDLE              LocateHandle;
    ACPI_EFI_LOCATE_DEVICE_PATH         LocateDevicePath;
    ACPI_EFI_INSTALL_CONFIGURATION_TABLE InstallConfigurationTable;

    ACPI_EFI_IMAGE_LOAD                 LoadImage;
    ACPI_EFI_IMAGE_START                StartImage;
    ACPI_EFI_EXIT                       Exit;
    ACPI_EFI_IMAGE_UNLOAD               UnloadImage;

#if 0
    ACPI_EFI_EXIT_BOOT_SERVICES         ExitBootServices;
    ACPI_EFI_GET_NEXT_MONOTONIC_COUNT   GetNextMonotonicCount;
#else
    ACPI_EFI_UNKNOWN_INTERFACE          ExitBootServices;
    ACPI_EFI_UNKNOWN_INTERFACE          GetNextMonotonicCount;
#endif
    ACPI_EFI_STALL                      Stall;
    ACPI_EFI_SET_WATCHDOG_TIMER         SetWatchdogTimer;

#if 0
    ACPI_EFI_CONNECT_CONTROLLER         ConnectController;
    ACPI_EFI_DISCONNECT_CONTROLLER      DisconnectController;
#else
    ACPI_EFI_UNKNOWN_INTERFACE          ConnectController;
    ACPI_EFI_UNKNOWN_INTERFACE          DisconnectController;
#endif

    ACPI_EFI_OPEN_PROTOCOL              OpenProtocol;
    ACPI_EFI_CLOSE_PROTOCOL             CloseProtocol;
    ACPI_EFI_OPEN_PROTOCOL_INFORMATION  OpenProtocolInformation;
    ACPI_EFI_PROTOCOLS_PER_HANDLE       ProtocolsPerHandle;
    ACPI_EFI_LOCATE_HANDLE_BUFFER       LocateHandleBuffer;
    ACPI_EFI_LOCATE_PROTOCOL            LocateProtocol;
    ACPI_EFI_INSTALL_MULTIPLE_PROTOCOL_INTERFACES InstallMultipleProtocolInterfaces;
    ACPI_EFI_UNINSTALL_MULTIPLE_PROTOCOL_INTERFACES UninstallMultipleProtocolInterfaces;

    ACPI_EFI_CALCULATE_CRC32        CalculateCrc32;

    ACPI_EFI_COPY_MEM               CopyMem;
    ACPI_EFI_SET_MEM                SetMem;

#if 0
    ACPI_EFI_CREATE_EVENT_EX        CreateEventEx;
#else
    ACPI_EFI_UNKNOWN_INTERFACE      CreateEventEx;
#endif
} ACPI_EFI_BOOT_SERVICES;


/*
 * EFI Runtime Services Table
 */
#define ACPI_EFI_RUNTIME_SERVICES_SIGNATURE 0x56524553544e5552
#define ACPI_EFI_RUNTIME_SERVICES_REVISION  (EFI_SPECIFICATION_MAJOR_REVISION<<16) | (EFI_SPECIFICATION_MINOR_REVISION)

typedef struct _ACPI_EFI_RUNTIME_SERVICES {
    ACPI_EFI_TABLE_HEADER               Hdr;

    ACPI_EFI_GET_TIME                   GetTime;
    ACPI_EFI_SET_TIME                   SetTime;
#if 0
    ACPI_EFI_GET_WAKEUP_TIME            GetWakeupTime;
    ACPI_EFI_SET_WAKEUP_TIME            SetWakeupTime;
#else
    ACPI_EFI_UNKNOWN_INTERFACE          GetWakeupTime;
    ACPI_EFI_UNKNOWN_INTERFACE          SetWakeupTime;
#endif

#if 0
    ACPI_EFI_SET_VIRTUAL_ADDRESS_MAP    SetVirtualAddressMap;
    ACPI_EFI_CONVERT_POINTER            ConvertPointer;
#else
    ACPI_EFI_UNKNOWN_INTERFACE          SetVirtualAddressMap;
    ACPI_EFI_UNKNOWN_INTERFACE          ConvertPointer;
#endif

#if 0
    ACPI_EFI_GET_VARIABLE               GetVariable;
    ACPI_EFI_GET_NEXT_VARIABLE_NAME     GetNextVariableName;
    ACPI_EFI_SET_VARIABLE               SetVariable;
#else
    ACPI_EFI_UNKNOWN_INTERFACE          GetVariable;
    ACPI_EFI_UNKNOWN_INTERFACE          GetNextVariableName;
    ACPI_EFI_UNKNOWN_INTERFACE          SetVariable;
#endif

#if 0
    ACPI_EFI_GET_NEXT_HIGH_MONO_COUNT   GetNextHighMonotonicCount;
    ACPI_EFI_RESET_SYSTEM               ResetSystem;
#else
    ACPI_EFI_UNKNOWN_INTERFACE          GetNextHighMonotonicCount;
    ACPI_EFI_UNKNOWN_INTERFACE          ResetSystem;
#endif

} ACPI_EFI_RUNTIME_SERVICES;


/*
 * EFI System Table
 */

/*
 * EFI Configuration Table and GUID definitions
 */
#define ACPI_TABLE_GUID \
    { 0xeb9d2d30, 0x2d88, 0x11d3, {0x9a, 0x16, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d} }
#define ACPI_20_TABLE_GUID \
    { 0x8868e871, 0xe4f1, 0x11d3, {0xbc, 0x22, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81} }

typedef struct _ACPI_EFI_CONFIGURATION_TABLE {
    ACPI_EFI_GUID           VendorGuid;
    VOID                    *VendorTable;
} ACPI_EFI_CONFIGURATION_TABLE;


#define ACPI_EFI_SYSTEM_TABLE_SIGNATURE 0x5453595320494249
#define ACPI_EFI_SYSTEM_TABLE_REVISION  (ACPI_EFI_SPECIFICATION_MAJOR_REVISION<<16) | (ACPI_EFI_SPECIFICATION_MINOR_REVISION)

typedef struct _ACPI_EFI_SYSTEM_TABLE {
    ACPI_EFI_TABLE_HEADER               Hdr;

    CHAR16                              *FirmwareVendor;
    UINT32                              FirmwareRevision;

    ACPI_EFI_HANDLE                     ConsoleInHandle;
    ACPI_SIMPLE_INPUT_INTERFACE         *ConIn;

    ACPI_EFI_HANDLE                     ConsoleOutHandle;
    ACPI_SIMPLE_TEXT_OUTPUT_INTERFACE   *ConOut;

    ACPI_EFI_HANDLE                     StandardErrorHandle;
    ACPI_SIMPLE_TEXT_OUTPUT_INTERFACE   *StdErr;

    ACPI_EFI_RUNTIME_SERVICES           *RuntimeServices;
    ACPI_EFI_BOOT_SERVICES              *BootServices;

    UINTN                               NumberOfTableEntries;
    ACPI_EFI_CONFIGURATION_TABLE        *ConfigurationTable;

} ACPI_EFI_SYSTEM_TABLE;


/*
 * EFI PCI I/O Protocol
 */
#define ACPI_EFI_PCI_IO_PROTOCOL \
    { 0x4cf5b200, 0x68b8, 0x4ca5, {0x9e, 0xec, 0xb2, 0x3e, 0x3f, 0x50, 0x2, 0x9a} }

typedef enum {
    AcpiEfiPciIoWidthUint8 = 0,
    AcpiEfiPciIoWidthUint16,
    AcpiEfiPciIoWidthUint32,
    AcpiEfiPciIoWidthUint64,
    AcpiEfiPciIoWidthFifoUint8,
    AcpiEfiPciIoWidthFifoUint16,
    AcpiEfiPciIoWidthFifoUint32,
    AcpiEfiPciIoWidthFifoUint64,
    AcpiEfiPciIoWidthFillUint8,
    AcpiEfiPciIoWidthFillUint16,
    AcpiEfiPciIoWidthFillUint32,
    AcpiEfiPciIoWidthFillUint64,
    AcpiEfiPciIoWidthMaximum
} ACPI_EFI_PCI_IO_PROTOCOL_WIDTH;

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_PCI_IO_PROTOCOL_CONFIG)(
    struct _ACPI_EFI_PCI_IO             *This,
    ACPI_EFI_PCI_IO_PROTOCOL_WIDTH      Width,
    UINT32                              Offset,
    UINTN                               Count,
    VOID                                *Buffer);

typedef struct {
    ACPI_EFI_PCI_IO_PROTOCOL_CONFIG     Read;
    ACPI_EFI_PCI_IO_PROTOCOL_CONFIG     Write;
} ACPI_EFI_PCI_IO_PROTOCOL_CONFIG_ACCESS;

typedef
ACPI_EFI_STATUS
(ACPI_EFI_API *ACPI_EFI_PCI_IO_PROTOCOL_GET_LOCATION)(
    struct _ACPI_EFI_PCI_IO             *This,
    UINTN                               *SegmentNumber,
    UINTN                               *BusNumber,
    UINTN                               *DeviceNumber,
    UINTN                               *FunctionNumber);

typedef struct _ACPI_EFI_PCI_IO {
    ACPI_EFI_UNKNOWN_INTERFACE          PollMem;
    ACPI_EFI_UNKNOWN_INTERFACE          PollIo;
    ACPI_EFI_UNKNOWN_INTERFACE          Mem;
    ACPI_EFI_UNKNOWN_INTERFACE          Io;
    ACPI_EFI_PCI_IO_PROTOCOL_CONFIG_ACCESS Pci;
    ACPI_EFI_UNKNOWN_INTERFACE          CopyMem;
    ACPI_EFI_UNKNOWN_INTERFACE          Map;
    ACPI_EFI_UNKNOWN_INTERFACE          Unmap;
    ACPI_EFI_UNKNOWN_INTERFACE          AllocateBuffer;
    ACPI_EFI_UNKNOWN_INTERFACE          FreeBuffer;
    ACPI_EFI_UNKNOWN_INTERFACE          Flush;
    ACPI_EFI_PCI_IO_PROTOCOL_GET_LOCATION GetLocation;
    ACPI_EFI_UNKNOWN_INTERFACE          Attributes;
    ACPI_EFI_UNKNOWN_INTERFACE          GetBarAttributes;
    ACPI_EFI_UNKNOWN_INTERFACE          SetBarAttributes;
    UINT64                              RomSize;
    VOID                                *RomImage;
} ACPI_EFI_PCI_IO;

/* FILE abstraction */

union acpi_efi_file {
    struct _ACPI_EFI_FILE_HANDLE File;
    struct _ACPI_SIMPLE_TEXT_OUTPUT_INTERFACE ConOut;
    struct _ACPI_SIMPLE_INPUT_INTERFACE ConIn;
};


/* EFI definitions */

#if defined(_GNU_EFI) || defined(_EDK2_EFI)

/*
 * This is needed to hide platform specific code from ACPICA
 */
UINT64 ACPI_EFI_API
DivU64x32 (
    UINT64                  Dividend,
    UINTN                   Divisor,
    UINTN                   *Remainder);

UINT64 ACPI_EFI_API
MultU64x32 (
    UINT64                  Multiplicand,
    UINTN                   Multiplier);

UINT64 ACPI_EFI_API
LShiftU64 (
    UINT64                  Operand,
    UINTN                   Count);

UINT64 ACPI_EFI_API
RShiftU64 (
    UINT64                  Operand,
    UINTN                   Count);

/*
 * EFI specific prototypes
 */
ACPI_EFI_STATUS
efi_main (
    ACPI_EFI_HANDLE         Image,
    ACPI_EFI_SYSTEM_TABLE   *SystemTab);

int
acpi_main (
    int                     argc,
    char                    *argv[]);

#endif

extern ACPI_EFI_GUID AcpiGbl_LoadedImageProtocol;
extern ACPI_EFI_GUID AcpiGbl_TextInProtocol;
extern ACPI_EFI_GUID AcpiGbl_TextOutProtocol;
extern ACPI_EFI_GUID AcpiGbl_FileSystemProtocol;
extern ACPI_EFI_GUID AcpiGbl_GenericFileInfo;

#endif /* __ACEFIEX_H__ */
