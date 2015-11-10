/******************************************************************************
 *
 * Name: acefiex.h - Extra OS specific defines, etc. for EFI
 *
 *****************************************************************************/

/******************************************************************************
 *
 * 1. Copyright Notice
 *
 * Some or all of this work - Copyright (c) 1999 - 2015, Intel Corp.
 * All rights reserved.
 *
 * 2. License
 *
 * 2.1. This is your license from Intel Corp. under its intellectual property
 * rights. You may have additional license terms from the party that provided
 * you this software, covering your right to use that party's intellectual
 * property rights.
 *
 * 2.2. Intel grants, free of charge, to any person ("Licensee") obtaining a
 * copy of the source code appearing in this file ("Covered Code") an
 * irrevocable, perpetual, worldwide license under Intel's copyrights in the
 * base code distributed originally by Intel ("Original Intel Code") to copy,
 * make derivatives, distribute, use and display any portion of the Covered
 * Code in any form, with the right to sublicense such rights; and
 *
 * 2.3. Intel grants Licensee a non-exclusive and non-transferable patent
 * license (with the right to sublicense), under only those claims of Intel
 * patents that are infringed by the Original Intel Code, to make, use, sell,
 * offer to sell, and import the Covered Code and derivative works thereof
 * solely to the minimum extent necessary to exercise the above copyright
 * license, and in no event shall the patent license extend to any additions
 * to or modifications of the Original Intel Code. No other license or right
 * is granted directly or by implication, estoppel or otherwise;
 *
 * The above copyright and patent license is granted only if the following
 * conditions are met:
 *
 * 3. Conditions
 *
 * 3.1. Redistribution of Source with Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification with rights to further distribute source must include
 * the above Copyright Notice, the above License, this list of Conditions,
 * and the following Disclaimer and Export Compliance provision. In addition,
 * Licensee must cause all Covered Code to which Licensee contributes to
 * contain a file documenting the changes Licensee made to create that Covered
 * Code and the date of any change. Licensee must include in that file the
 * documentation of any changes made by any predecessor Licensee. Licensee
 * must include a prominent statement that the modification is derived,
 * directly or indirectly, from Original Intel Code.
 *
 * 3.2. Redistribution of Source with no Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification without rights to further distribute source must
 * include the following Disclaimer and Export Compliance provision in the
 * documentation and/or other materials provided with distribution. In
 * addition, Licensee may not authorize further sublicense of source of any
 * portion of the Covered Code, and must include terms to the effect that the
 * license from Licensee to its licensee is limited to the intellectual
 * property embodied in the software Licensee provides to its licensee, and
 * not to intellectual property embodied in modifications its licensee may
 * make.
 *
 * 3.3. Redistribution of Executable. Redistribution in executable form of any
 * substantial portion of the Covered Code or modification must reproduce the
 * above Copyright Notice, and the following Disclaimer and Export Compliance
 * provision in the documentation and/or other materials provided with the
 * distribution.
 *
 * 3.4. Intel retains all right, title, and interest in and to the Original
 * Intel Code.
 *
 * 3.5. Neither the name Intel nor any other trademark owned or controlled by
 * Intel shall be used in advertising or otherwise to promote the sale, use or
 * other dealings in products derived from or relating to the Covered Code
 * without prior written authorization from Intel.
 *
 * 4. Disclaimer and Export Compliance
 *
 * 4.1. INTEL MAKES NO WARRANTY OF ANY KIND REGARDING ANY SOFTWARE PROVIDED
 * HERE. ANY SOFTWARE ORIGINATING FROM INTEL OR DERIVED FROM INTEL SOFTWARE
 * IS PROVIDED "AS IS," AND INTEL WILL NOT PROVIDE ANY SUPPORT, ASSISTANCE,
 * INSTALLATION, TRAINING OR OTHER SERVICES. INTEL WILL NOT PROVIDE ANY
 * UPDATES, ENHANCEMENTS OR EXTENSIONS. INTEL SPECIFICALLY DISCLAIMS ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT AND FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 * 4.2. IN NO EVENT SHALL INTEL HAVE ANY LIABILITY TO LICENSEE, ITS LICENSEES
 * OR ANY OTHER THIRD PARTY, FOR ANY LOST PROFITS, LOST DATA, LOSS OF USE OR
 * COSTS OF PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES, OR FOR ANY INDIRECT,
 * SPECIAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THIS AGREEMENT, UNDER ANY
 * CAUSE OF ACTION OR THEORY OF LIABILITY, AND IRRESPECTIVE OF WHETHER INTEL
 * HAS ADVANCE NOTICE OF THE POSSIBILITY OF SUCH DAMAGES. THESE LIMITATIONS
 * SHALL APPLY NOTWITHSTANDING THE FAILURE OF THE ESSENTIAL PURPOSE OF ANY
 * LIMITED REMEDY.
 *
 * 4.3. Licensee shall not export, either directly or indirectly, any of this
 * software or system incorporating such software without first obtaining any
 * required license or other approval from the U. S. Department of Commerce or
 * any other agency or department of the United States Government. In the
 * event Licensee exports any such software from the United States or
 * re-exports any such software from a foreign destination, Licensee shall
 * ensure that the distribution and export/re-export of the software is in
 * compliance with all laws, regulations, orders, or other restrictions of the
 * U.S. Export Administration Regulations. Licensee agrees that neither it nor
 * any of its subsidiaries will export/re-export any technical data, process,
 * software, or service, directly or indirectly, to any country for which the
 * United States government or any agency thereof requires an export license,
 * other governmental approval, or letter of assurance, without first obtaining
 * such license, approval or letter.
 *
 *****************************************************************************/

#ifndef __ACEFIEX_H__
#define __ACEFIEX_H__


#define EFI_ERROR(a)              (((INTN) a) < 0)
#define EFI_SUCCESS                             0
#define EFI_LOAD_ERROR                  EFIERR(1)
#define EFI_INVALID_PARAMETER           EFIERR(2)
#define EFI_UNSUPPORTED                 EFIERR(3)
#define EFI_BAD_BUFFER_SIZE             EFIERR(4)
#define EFI_BUFFER_TOO_SMALL            EFIERR(5)
#define EFI_NOT_READY                   EFIERR(6)
#define EFI_DEVICE_ERROR                EFIERR(7)
#define EFI_WRITE_PROTECTED             EFIERR(8)
#define EFI_OUT_OF_RESOURCES            EFIERR(9)
#define EFI_VOLUME_CORRUPTED            EFIERR(10)
#define EFI_VOLUME_FULL                 EFIERR(11)
#define EFI_NO_MEDIA                    EFIERR(12)
#define EFI_MEDIA_CHANGED               EFIERR(13)
#define EFI_NOT_FOUND                   EFIERR(14)
#define EFI_ACCESS_DENIED               EFIERR(15)
#define EFI_NO_RESPONSE                 EFIERR(16)
#define EFI_NO_MAPPING                  EFIERR(17)
#define EFI_TIMEOUT                     EFIERR(18)
#define EFI_NOT_STARTED                 EFIERR(19)
#define EFI_ALREADY_STARTED             EFIERR(20)
#define EFI_ABORTED                     EFIERR(21)
#define EFI_PROTOCOL_ERROR              EFIERR(24)


typedef UINTN EFI_STATUS;
typedef VOID *EFI_HANDLE;
typedef VOID *EFI_EVENT;

typedef struct {
    UINT32  Data1;
    UINT16  Data2;
    UINT16  Data3;
    UINT8   Data4[8];
} EFI_GUID;

typedef struct _EFI_DEVICE_PATH {
        UINT8                           Type;
        UINT8                           SubType;
        UINT8                           Length[2];
} EFI_DEVICE_PATH;

typedef UINT64          EFI_PHYSICAL_ADDRESS;
typedef UINT64          EFI_VIRTUAL_ADDRESS;

typedef enum {
    AllocateAnyPages,
    AllocateMaxAddress,
    AllocateAddress,
    MaxAllocateType
} EFI_ALLOCATE_TYPE;

typedef enum {
    EfiReservedMemoryType,
    EfiLoaderCode,
    EfiLoaderData,
    EfiBootServicesCode,
    EfiBootServicesData,
    EfiRuntimeServicesCode,
    EfiRuntimeServicesData,
    EfiConventionalMemory,
    EfiUnusableMemory,
    EfiACPIReclaimMemory,
    EfiACPIMemoryNVS,
    EfiMemoryMappedIO,
    EfiMemoryMappedIOPortSpace,
    EfiPalCode,
    EfiMaxMemoryType
} EFI_MEMORY_TYPE;

/* possible caching types for the memory range */
#define EFI_MEMORY_UC           0x0000000000000001
#define EFI_MEMORY_WC           0x0000000000000002
#define EFI_MEMORY_WT           0x0000000000000004
#define EFI_MEMORY_WB           0x0000000000000008
#define EFI_MEMORY_UCE          0x0000000000000010

/* physical memory protection on range */
#define EFI_MEMORY_WP           0x0000000000001000
#define EFI_MEMORY_RP           0x0000000000002000
#define EFI_MEMORY_XP           0x0000000000004000

/* range requires a runtime mapping */
#define EFI_MEMORY_RUNTIME      0x8000000000000000

#define EFI_MEMORY_DESCRIPTOR_VERSION  1
typedef struct {
    UINT32                          Type;
    UINT32                          Pad;
    EFI_PHYSICAL_ADDRESS            PhysicalStart;
    EFI_VIRTUAL_ADDRESS             VirtualStart;
    UINT64                          NumberOfPages;
    UINT64                          Attribute;
} EFI_MEMORY_DESCRIPTOR;

typedef struct _EFI_TABLE_HEARDER {
    UINT64                      Signature;
    UINT32                      Revision;
    UINT32                      HeaderSize;
    UINT32                      CRC32;
    UINT32                      Reserved;
} EFI_TABLE_HEADER;

typedef
EFI_STATUS
(EFIAPI *EFI_UNKNOWN_INTERFACE) (
    void);


/*
 * Text output protocol
 */
#define SIMPLE_TEXT_OUTPUT_PROTOCOL \
    { 0x387477c2, 0x69c7, 0x11d2, {0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b} }

typedef
EFI_STATUS
(EFIAPI *EFI_TEXT_RESET) (
    struct _SIMPLE_TEXT_OUTPUT_INTERFACE    *This,
    BOOLEAN                                 ExtendedVerification);

typedef
EFI_STATUS
(EFIAPI *EFI_TEXT_OUTPUT_STRING) (
    struct _SIMPLE_TEXT_OUTPUT_INTERFACE    *This,
    CHAR16                                  *String);

typedef
EFI_STATUS
(EFIAPI *EFI_TEXT_TEST_STRING) (
    struct _SIMPLE_TEXT_OUTPUT_INTERFACE    *This,
    CHAR16                                  *String);

typedef
EFI_STATUS
(EFIAPI *EFI_TEXT_QUERY_MODE) (
    struct _SIMPLE_TEXT_OUTPUT_INTERFACE    *This,
    UINTN                                   ModeNumber,
    UINTN                                   *Columns,
    UINTN                                   *Rows);

typedef
EFI_STATUS
(EFIAPI *EFI_TEXT_SET_MODE) (
    struct _SIMPLE_TEXT_OUTPUT_INTERFACE    *This,
    UINTN                                   ModeNumber);

typedef
EFI_STATUS
(EFIAPI *EFI_TEXT_SET_ATTRIBUTE) (
    struct _SIMPLE_TEXT_OUTPUT_INTERFACE    *This,
    UINTN                                   Attribute);

typedef
EFI_STATUS
(EFIAPI *EFI_TEXT_CLEAR_SCREEN) (
    struct _SIMPLE_TEXT_OUTPUT_INTERFACE    *This);

typedef
EFI_STATUS
(EFIAPI *EFI_TEXT_SET_CURSOR_POSITION) (
    struct _SIMPLE_TEXT_OUTPUT_INTERFACE    *This,
    UINTN                                   Column,
    UINTN                                   Row);

typedef
EFI_STATUS
(EFIAPI *EFI_TEXT_ENABLE_CURSOR) (
    struct _SIMPLE_TEXT_OUTPUT_INTERFACE    *This,
    BOOLEAN                                 Enable);

typedef struct {
    INT32                           MaxMode;
    INT32                           Mode;
    INT32                           Attribute;
    INT32                           CursorColumn;
    INT32                           CursorRow;
    BOOLEAN                         CursorVisible;
} SIMPLE_TEXT_OUTPUT_MODE;

typedef struct _SIMPLE_TEXT_OUTPUT_INTERFACE {
    EFI_TEXT_RESET                  Reset;

    EFI_TEXT_OUTPUT_STRING          OutputString;
    EFI_TEXT_TEST_STRING            TestString;

    EFI_TEXT_QUERY_MODE             QueryMode;
    EFI_TEXT_SET_MODE               SetMode;
    EFI_TEXT_SET_ATTRIBUTE          SetAttribute;

    EFI_TEXT_CLEAR_SCREEN           ClearScreen;
    EFI_TEXT_SET_CURSOR_POSITION    SetCursorPosition;
    EFI_TEXT_ENABLE_CURSOR          EnableCursor;

    SIMPLE_TEXT_OUTPUT_MODE         *Mode;
} SIMPLE_TEXT_OUTPUT_INTERFACE;

/*
 * Text input protocol
 */
#define SIMPLE_TEXT_INPUT_PROTOCOL  \
    { 0x387477c1, 0x69c7, 0x11d2, {0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b} }

typedef struct {
    UINT16                              ScanCode;
    CHAR16                              UnicodeChar;
} EFI_INPUT_KEY;

/*
 * Baseline unicode control chars
 */
#define CHAR_NULL                       0x0000
#define CHAR_BACKSPACE                  0x0008
#define CHAR_TAB                        0x0009
#define CHAR_LINEFEED                   0x000A
#define CHAR_CARRIAGE_RETURN            0x000D

typedef
EFI_STATUS
(EFIAPI *EFI_INPUT_RESET) (
    struct _SIMPLE_INPUT_INTERFACE              *This,
    BOOLEAN                                     ExtendedVerification);

typedef
EFI_STATUS
(EFIAPI *EFI_INPUT_READ_KEY) (
    struct _SIMPLE_INPUT_INTERFACE              *This,
    EFI_INPUT_KEY                               *Key);

typedef struct _SIMPLE_INPUT_INTERFACE {
    EFI_INPUT_RESET                     Reset;
    EFI_INPUT_READ_KEY                  ReadKeyStroke;
    EFI_EVENT                           WaitForKey;
} SIMPLE_INPUT_INTERFACE;


/*
 * Simple file system protocol
 */
#define SIMPLE_FILE_SYSTEM_PROTOCOL \
    { 0x964e5b22, 0x6459, 0x11d2, {0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b} }

typedef
EFI_STATUS
(EFIAPI *EFI_VOLUME_OPEN) (
    struct _EFI_FILE_IO_INTERFACE               *This,
    struct _EFI_FILE_HANDLE                     **Root);

#define EFI_FILE_IO_INTERFACE_REVISION   0x00010000

typedef struct _EFI_FILE_IO_INTERFACE {
    UINT64                  Revision;
    EFI_VOLUME_OPEN         OpenVolume;
} EFI_FILE_IO_INTERFACE;

typedef
EFI_STATUS
(EFIAPI *EFI_FILE_OPEN) (
    struct _EFI_FILE_HANDLE                     *File,
    struct _EFI_FILE_HANDLE                     **NewHandle,
    CHAR16                                      *FileName,
    UINT64                                      OpenMode,
    UINT64                                      Attributes);

/* Values for OpenMode used above */

#define EFI_FILE_MODE_READ      0x0000000000000001
#define EFI_FILE_MODE_WRITE     0x0000000000000002
#define EFI_FILE_MODE_CREATE    0x8000000000000000

/* Values for Attribute used above */

#define EFI_FILE_READ_ONLY      0x0000000000000001
#define EFI_FILE_HIDDEN         0x0000000000000002
#define EFI_FILE_SYSTEM         0x0000000000000004
#define EFI_FILE_RESERVIED      0x0000000000000008
#define EFI_FILE_DIRECTORY      0x0000000000000010
#define EFI_FILE_ARCHIVE        0x0000000000000020
#define EFI_FILE_VALID_ATTR     0x0000000000000037

typedef
EFI_STATUS
(EFIAPI *EFI_FILE_CLOSE) (
    struct _EFI_FILE_HANDLE                     *File);

typedef
EFI_STATUS
(EFIAPI *EFI_FILE_DELETE) (
    struct _EFI_FILE_HANDLE                     *File);

typedef
EFI_STATUS
(EFIAPI *EFI_FILE_READ) (
    struct _EFI_FILE_HANDLE                     *File,
    UINTN                                       *BufferSize,
    VOID                                        *Buffer);

typedef
EFI_STATUS
(EFIAPI *EFI_FILE_WRITE) (
    struct _EFI_FILE_HANDLE                     *File,
    UINTN                                       *BufferSize,
    VOID                                        *Buffer);

typedef
EFI_STATUS
(EFIAPI *EFI_FILE_SET_POSITION) (
    struct _EFI_FILE_HANDLE                     *File,
    UINT64                                      Position);

typedef
EFI_STATUS
(EFIAPI *EFI_FILE_GET_POSITION) (
    struct _EFI_FILE_HANDLE                     *File,
    UINT64                                      *Position);

typedef
EFI_STATUS
(EFIAPI *EFI_FILE_GET_INFO) (
    struct _EFI_FILE_HANDLE                     *File,
    EFI_GUID                                    *InformationType,
    UINTN                                       *BufferSize,
    VOID                                        *Buffer);

typedef
EFI_STATUS
(EFIAPI *EFI_FILE_SET_INFO) (
    struct _EFI_FILE_HANDLE                     *File,
    EFI_GUID                                    *InformationType,
    UINTN                                       BufferSize,
    VOID                                        *Buffer);

typedef
EFI_STATUS
(EFIAPI *EFI_FILE_FLUSH) (
    struct _EFI_FILE_HANDLE                     *File);


#define EFI_FILE_HANDLE_REVISION         0x00010000

typedef struct _EFI_FILE_HANDLE {
    UINT64                  Revision;
    EFI_FILE_OPEN           Open;
    EFI_FILE_CLOSE          Close;
    EFI_FILE_DELETE         Delete;
    EFI_FILE_READ           Read;
    EFI_FILE_WRITE          Write;
    EFI_FILE_GET_POSITION   GetPosition;
    EFI_FILE_SET_POSITION   SetPosition;
    EFI_FILE_GET_INFO       GetInfo;
    EFI_FILE_SET_INFO       SetInfo;
    EFI_FILE_FLUSH          Flush;
} EFI_FILE, *EFI_FILE_HANDLE;


/*
 * Loaded image protocol
 */
#define LOADED_IMAGE_PROTOCOL      \
    { 0x5B1B31A1, 0x9562, 0x11d2, {0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B} }

typedef
EFI_STATUS
(EFIAPI *EFI_IMAGE_ENTRY_POINT) (
    EFI_HANDLE                              ImageHandle,
    struct _EFI_SYSTEM_TABLE                *SystemTable);

typedef
EFI_STATUS
(EFIAPI *EFI_IMAGE_LOAD) (
    BOOLEAN                         BootPolicy,
    EFI_HANDLE                      ParentImageHandle,
    EFI_DEVICE_PATH                 *FilePath,
    VOID                            *SourceBuffer,
    UINTN                           SourceSize,
    EFI_HANDLE                      *ImageHandle);

typedef
EFI_STATUS
(EFIAPI *EFI_IMAGE_START) (
    EFI_HANDLE                      ImageHandle,
    UINTN                           *ExitDataSize,
    CHAR16                          **ExitData);

typedef
EFI_STATUS
(EFIAPI *EFI_EXIT) (
    EFI_HANDLE                      ImageHandle,
    EFI_STATUS                      ExitStatus,
    UINTN                           ExitDataSize,
    CHAR16                          *ExitData);

typedef
EFI_STATUS
(EFIAPI *EFI_IMAGE_UNLOAD) (
    EFI_HANDLE                      ImageHandle);


#define EFI_IMAGE_INFORMATION_REVISION      0x1000
typedef struct {
    UINT32                          Revision;
    EFI_HANDLE                      ParentHandle;
    struct _EFI_SYSTEM_TABLE        *SystemTable;
    EFI_HANDLE                      DeviceHandle;
    EFI_DEVICE_PATH                 *FilePath;
    VOID                            *Reserved;
    UINT32                          LoadOptionsSize;
    VOID                            *LoadOptions;
    VOID                            *ImageBase;
    UINT64                          ImageSize;
    EFI_MEMORY_TYPE                 ImageCodeType;
    EFI_MEMORY_TYPE                 ImageDataType;
    EFI_IMAGE_UNLOAD                Unload;

} EFI_LOADED_IMAGE;


/*
 * EFI Memory
 */
typedef
EFI_STATUS
(EFIAPI *EFI_ALLOCATE_PAGES) (
    EFI_ALLOCATE_TYPE               Type,
    EFI_MEMORY_TYPE                 MemoryType,
    UINTN                           NoPages,
    EFI_PHYSICAL_ADDRESS            *Memory);

typedef
EFI_STATUS
(EFIAPI *EFI_FREE_PAGES) (
    EFI_PHYSICAL_ADDRESS            Memory,
    UINTN                           NoPages);

typedef
EFI_STATUS
(EFIAPI *EFI_GET_MEMORY_MAP) (
    UINTN                           *MemoryMapSize,
    EFI_MEMORY_DESCRIPTOR           *MemoryMap,
    UINTN                           *MapKey,
    UINTN                           *DescriptorSize,
    UINT32                          *DescriptorVersion);

#define NextMemoryDescriptor(Ptr,Size)  ((EFI_MEMORY_DESCRIPTOR *) (((UINT8 *) Ptr) + Size))

typedef
EFI_STATUS
(EFIAPI *EFI_ALLOCATE_POOL) (
    EFI_MEMORY_TYPE                 PoolType,
    UINTN                           Size,
    VOID                            **Buffer);

typedef
EFI_STATUS
(EFIAPI *EFI_FREE_POOL) (
    VOID                            *Buffer);


/*
 * Protocol handler functions
 */
typedef enum {
    EFI_NATIVE_INTERFACE,
    EFI_PCODE_INTERFACE
} EFI_INTERFACE_TYPE;

typedef enum {
    AllHandles,
    ByRegisterNotify,
    ByProtocol
} EFI_LOCATE_SEARCH_TYPE;

typedef
EFI_STATUS
(EFIAPI *EFI_INSTALL_PROTOCOL_INTERFACE) (
    EFI_HANDLE                      *Handle,
    EFI_GUID                        *Protocol,
    EFI_INTERFACE_TYPE              InterfaceType,
    VOID                            *Interface);

typedef
EFI_STATUS
(EFIAPI *EFI_REINSTALL_PROTOCOL_INTERFACE) (
    EFI_HANDLE                      Handle,
    EFI_GUID                        *Protocol,
    VOID                            *OldInterface,
    VOID                            *NewInterface);

typedef
EFI_STATUS
(EFIAPI *EFI_UNINSTALL_PROTOCOL_INTERFACE) (
    EFI_HANDLE                      Handle,
    EFI_GUID                        *Protocol,
    VOID                            *Interface);

typedef
EFI_STATUS
(EFIAPI *EFI_HANDLE_PROTOCOL) (
    EFI_HANDLE                      Handle,
    EFI_GUID                        *Protocol,
    VOID                            **Interface);

typedef
EFI_STATUS
(EFIAPI *EFI_REGISTER_PROTOCOL_NOTIFY) (
    EFI_GUID                        *Protocol,
    EFI_EVENT                       Event,
    VOID                            **Registration);

typedef
EFI_STATUS
(EFIAPI *EFI_LOCATE_HANDLE) (
    EFI_LOCATE_SEARCH_TYPE          SearchType,
    EFI_GUID                        *Protocol,
    VOID                            *SearchKey,
    UINTN                           *BufferSize,
    EFI_HANDLE                      *Buffer);

typedef
EFI_STATUS
(EFIAPI *EFI_LOCATE_DEVICE_PATH) (
    EFI_GUID                        *Protocol,
    EFI_DEVICE_PATH                 **DevicePath,
    EFI_HANDLE                      *Device);

typedef
EFI_STATUS
(EFIAPI *EFI_INSTALL_CONFIGURATION_TABLE) (
    EFI_GUID                        *Guid,
    VOID                            *Table);

#define EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL  0x00000001
#define EFI_OPEN_PROTOCOL_GET_PROTOCOL        0x00000002
#define EFI_OPEN_PROTOCOL_TEST_PROTOCOL       0x00000004
#define EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER 0x00000008
#define EFI_OPEN_PROTOCOL_BY_DRIVER           0x00000010
#define EFI_OPEN_PROTOCOL_EXCLUSIVE           0x00000020

typedef
EFI_STATUS
(EFIAPI *EFI_OPEN_PROTOCOL) (
    EFI_HANDLE                      Handle,
    EFI_GUID                        *Protocol,
    VOID                            **Interface,
    EFI_HANDLE                      AgentHandle,
    EFI_HANDLE                      ControllerHandle,
    UINT32                          Attributes);

typedef
EFI_STATUS
(EFIAPI *EFI_CLOSE_PROTOCOL) (
    EFI_HANDLE                      Handle,
    EFI_GUID                        *Protocol,
    EFI_HANDLE                      AgentHandle,
    EFI_HANDLE                      ControllerHandle);

typedef struct {
    EFI_HANDLE                  AgentHandle;
    EFI_HANDLE                  ControllerHandle;
    UINT32                      Attributes;
    UINT32                      OpenCount;
} EFI_OPEN_PROTOCOL_INFORMATION_ENTRY;

typedef
EFI_STATUS
(EFIAPI *EFI_OPEN_PROTOCOL_INFORMATION) (
    EFI_HANDLE                      Handle,
    EFI_GUID                        *Protocol,
    EFI_OPEN_PROTOCOL_INFORMATION_ENTRY **EntryBuffer,
    UINTN                           *EntryCount);

typedef
EFI_STATUS
(EFIAPI *EFI_PROTOCOLS_PER_HANDLE) (
    EFI_HANDLE                      Handle,
    EFI_GUID                        ***ProtocolBuffer,
    UINTN                           *ProtocolBufferCount);

typedef
EFI_STATUS
(EFIAPI *EFI_LOCATE_HANDLE_BUFFER) (
    EFI_LOCATE_SEARCH_TYPE          SearchType,
    EFI_GUID                        *Protocol,
    VOID                            *SearchKey,
    UINTN                           *NoHandles,
    EFI_HANDLE                      **Buffer);

typedef
EFI_STATUS
(EFIAPI *EFI_LOCATE_PROTOCOL) (
    EFI_GUID                        *Protocol,
    VOID                            *Registration,
    VOID                            **Interface);

typedef
EFI_STATUS
(EFIAPI *EFI_INSTALL_MULTIPLE_PROTOCOL_INTERFACES) (
    EFI_HANDLE                      *Handle,
    ...);

typedef
EFI_STATUS
(EFIAPI *EFI_UNINSTALL_MULTIPLE_PROTOCOL_INTERFACES) (
    EFI_HANDLE                      Handle,
    ...);

typedef
EFI_STATUS
(EFIAPI *EFI_CALCULATE_CRC32) (
    VOID                            *Data,
    UINTN                           DataSize,
    UINT32                          *Crc32);

typedef
VOID
(EFIAPI *EFI_COPY_MEM) (
    VOID                            *Destination,
    VOID                            *Source,
    UINTN                           Length);

typedef
VOID
(EFIAPI *EFI_SET_MEM) (
    VOID                            *Buffer,
    UINTN                           Size,
    UINT8                           Value);

/*
 * EFI Boot Services Table
 */
#define EFI_BOOT_SERVICES_SIGNATURE     0x56524553544f4f42
#define EFI_BOOT_SERVICES_REVISION      (EFI_SPECIFICATION_MAJOR_REVISION<<16) | (EFI_SPECIFICATION_MINOR_REVISION)

typedef struct _EFI_BOOT_SERVICES {
    EFI_TABLE_HEADER                Hdr;

#if 0
    EFI_RAISE_TPL                   RaiseTPL;
    EFI_RESTORE_TPL                 RestoreTPL;
#else
    EFI_UNKNOWN_INTERFACE           RaiseTPL;
    EFI_UNKNOWN_INTERFACE           RestoreTPL;
#endif

    EFI_ALLOCATE_PAGES              AllocatePages;
    EFI_FREE_PAGES                  FreePages;
    EFI_GET_MEMORY_MAP              GetMemoryMap;
    EFI_ALLOCATE_POOL               AllocatePool;
    EFI_FREE_POOL                   FreePool;

#if 0
    EFI_CREATE_EVENT                CreateEvent;
    EFI_SET_TIMER                   SetTimer;
    EFI_WAIT_FOR_EVENT              WaitForEvent;
    EFI_SIGNAL_EVENT                SignalEvent;
    EFI_CLOSE_EVENT                 CloseEvent;
    EFI_CHECK_EVENT                 CheckEvent;
#else
    EFI_UNKNOWN_INTERFACE           CreateEvent;
    EFI_UNKNOWN_INTERFACE           SetTimer;
    EFI_UNKNOWN_INTERFACE           WaitForEvent;
    EFI_UNKNOWN_INTERFACE           SignalEvent;
    EFI_UNKNOWN_INTERFACE           CloseEvent;
    EFI_UNKNOWN_INTERFACE           CheckEvent;
#endif

    EFI_INSTALL_PROTOCOL_INTERFACE  InstallProtocolInterface;
    EFI_REINSTALL_PROTOCOL_INTERFACE ReinstallProtocolInterface;
    EFI_UNINSTALL_PROTOCOL_INTERFACE UninstallProtocolInterface;
    EFI_HANDLE_PROTOCOL             HandleProtocol;
    EFI_HANDLE_PROTOCOL             PCHandleProtocol;
    EFI_REGISTER_PROTOCOL_NOTIFY    RegisterProtocolNotify;
    EFI_LOCATE_HANDLE               LocateHandle;
    EFI_LOCATE_DEVICE_PATH          LocateDevicePath;
    EFI_INSTALL_CONFIGURATION_TABLE InstallConfigurationTable;

    EFI_IMAGE_LOAD                  LoadImage;
    EFI_IMAGE_START                 StartImage;
    EFI_EXIT                        Exit;
    EFI_IMAGE_UNLOAD                UnloadImage;

#if 0
    EFI_EXIT_BOOT_SERVICES          ExitBootServices;
    EFI_GET_NEXT_MONOTONIC_COUNT    GetNextMonotonicCount;
    EFI_STALL                       Stall;
    EFI_SET_WATCHDOG_TIMER          SetWatchdogTimer;
#else
    EFI_UNKNOWN_INTERFACE           ExitBootServices;
    EFI_UNKNOWN_INTERFACE           GetNextMonotonicCount;
    EFI_UNKNOWN_INTERFACE           Stall;
    EFI_UNKNOWN_INTERFACE           SetWatchdogTimer;
#endif

#if 0
    EFI_CONNECT_CONTROLLER          ConnectController;
    EFI_DISCONNECT_CONTROLLER       DisconnectController;
#else
    EFI_UNKNOWN_INTERFACE           ConnectController;
    EFI_UNKNOWN_INTERFACE           DisconnectController;
#endif

    EFI_OPEN_PROTOCOL               OpenProtocol;
    EFI_CLOSE_PROTOCOL              CloseProtocol;
    EFI_OPEN_PROTOCOL_INFORMATION   OpenProtocolInformation;
    EFI_PROTOCOLS_PER_HANDLE        ProtocolsPerHandle;
    EFI_LOCATE_HANDLE_BUFFER        LocateHandleBuffer;
    EFI_LOCATE_PROTOCOL             LocateProtocol;
    EFI_INSTALL_MULTIPLE_PROTOCOL_INTERFACES InstallMultipleProtocolInterfaces;
    EFI_UNINSTALL_MULTIPLE_PROTOCOL_INTERFACES UninstallMultipleProtocolInterfaces;

    EFI_CALCULATE_CRC32             CalculateCrc32;

    EFI_COPY_MEM                    CopyMem;
    EFI_SET_MEM                     SetMem;

#if 0
    EFI_CREATE_EVENT_EX             CreateEventEx;
#else
    EFI_UNKNOWN_INTERFACE           CreateEventEx;
#endif
} EFI_BOOT_SERVICES;


/*
 * EFI System Table
 */

/*
 * EFI Configuration Table and GUID definitions
 */
#define ACPI_TABLE_GUID    \
    { 0xeb9d2d30, 0x2d88, 0x11d3, {0x9a, 0x16, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d} }
#define ACPI_20_TABLE_GUID  \
    { 0x8868e871, 0xe4f1, 0x11d3, {0xbc, 0x22, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81} }

typedef struct _EFI_CONFIGURATION_TABLE {
    EFI_GUID                VendorGuid;
    VOID                    *VendorTable;
} EFI_CONFIGURATION_TABLE;


#define EFI_SYSTEM_TABLE_SIGNATURE      0x5453595320494249
#define EFI_SYSTEM_TABLE_REVISION      (EFI_SPECIFICATION_MAJOR_REVISION<<16) | (EFI_SPECIFICATION_MINOR_REVISION)

typedef struct _EFI_SYSTEM_TABLE {
    EFI_TABLE_HEADER                Hdr;

    CHAR16                          *FirmwareVendor;
    UINT32                          FirmwareRevision;

    EFI_HANDLE                      ConsoleInHandle;
    SIMPLE_INPUT_INTERFACE          *ConIn;

    EFI_HANDLE                      ConsoleOutHandle;
    SIMPLE_TEXT_OUTPUT_INTERFACE    *ConOut;

    EFI_HANDLE                      StandardErrorHandle;
    SIMPLE_TEXT_OUTPUT_INTERFACE    *StdErr;

#if 0
    EFI_RUNTIME_SERVICES            *RuntimeServices;
#else
    EFI_HANDLE                      *RuntimeServices;
#endif
    EFI_BOOT_SERVICES               *BootServices;

    UINTN                           NumberOfTableEntries;
    EFI_CONFIGURATION_TABLE         *ConfigurationTable;

} EFI_SYSTEM_TABLE;


/* GNU EFI definitions */

#if defined(_GNU_EFI)

/*
 * This is needed to hide platform specific code from ACPICA
 */
UINT64
DivU64x32 (
    UINT64                  Dividend,
    UINTN                   Divisor,
    UINTN                   *Remainder);

/*
 * EFI specific prototypes
 */
EFI_STATUS
efi_main (
    EFI_HANDLE              Image,
    EFI_SYSTEM_TABLE        *SystemTab);

int
acpi_main (
    int                     argc,
    char                    *argv[]);


#endif

extern EFI_GUID AcpiGbl_LoadedImageProtocol;
extern EFI_GUID AcpiGbl_TextInProtocol;
extern EFI_GUID AcpiGbl_TextOutProtocol;
extern EFI_GUID AcpiGbl_FileSystemProtocol;

#endif /* __ACEFIEX_H__ */
