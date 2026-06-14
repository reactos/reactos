/*
 * PROJECT:     ReactOS kernel-mode tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Header for C++ Ob template type definitions
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

typedef NTSTATUS
(NTAPI *OB_OPEN_METHOD_WS03)(
    _In_ OB_OPEN_REASON Reason,
    _In_opt_ PEPROCESS Process,
    _In_ PVOID ObjectBody,
    _In_ ACCESS_MASK GrantedAccess,
    _In_ ULONG HandleCount
);

typedef NTSTATUS
(NTAPI *OB_OPEN_METHOD_VISTA)(
    _In_ OB_OPEN_REASON Reason,
    _In_ KPROCESSOR_MODE AccessMode,
    _In_opt_ PEPROCESS Process,
    _In_ PVOID ObjectBody,
    _In_ PACCESS_MASK GrantedAccess,
    _In_ ULONG HandleCount
);

typedef NTSTATUS
(NTAPI *OB_SECURITY_METHOD_WS03)(
    _In_ PVOID Object,
    _In_ SECURITY_OPERATION_CODE OperationType,
    _In_ PSECURITY_INFORMATION SecurityInformation,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _Inout_ PULONG CapturedLength,
    _Inout_ PSECURITY_DESCRIPTOR *ObjectSecurityDescriptor,
    _In_ POOL_TYPE PoolType,
    _In_ PGENERIC_MAPPING GenericMapping
);

typedef NTSTATUS
(NTAPI *OB_SECURITY_METHOD_VISTA)(
    _In_ PVOID Object,
    _In_ SECURITY_OPERATION_CODE OperationType,
    _In_ PSECURITY_INFORMATION SecurityInformation,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _Inout_ PULONG CapturedLength,
    _Inout_ PSECURITY_DESCRIPTOR *ObjectSecurityDescriptor,
    _In_ POOL_TYPE PoolType,
    _In_ PGENERIC_MAPPING GenericMapping,
    _In_ KPROCESSOR_MODE AccessMode
);

struct OBJECT_TYPE_INITIALIZER_WS03
{
    USHORT Length;                                      // 0x00
    UCHAR UseDefaultObject;                             // 0x02
    UCHAR CaseInsensitive;                              // 0x03
    ULONG InvalidAttributes;                            // 0x04
    GENERIC_MAPPING GenericMapping;                     // 0x08
    ULONG ValidAccessMask;                              // 0x18
    UCHAR SecurityRequired;                             // 0x1c
    UCHAR MaintainHandleCount;                          // 0x1d
    UCHAR MaintainTypeList;                             // 0x1e
    POOL_TYPE PoolType;                                 // 0x20
    ULONG DefaultPagedPoolCharge;                       // 0x24
    ULONG DefaultNonPagedPoolCharge;                    // 0x28
    OB_DUMP_METHOD DumpProcedure;                       // 0x30
    OB_OPEN_METHOD_WS03 OpenProcedure;                  // 0x38
    OB_CLOSE_METHOD CloseProcedure;                     // 0x40
    OB_DELETE_METHOD DeleteProcedure;                   // 0x48
    OB_PARSE_METHOD ParseProcedure;                     // 0x50
    OB_SECURITY_METHOD_WS03 SecurityProcedure;          // 0x58
    OB_QUERYNAME_METHOD QueryNameProcedure;             // 0x60
    OB_OKAYTOCLOSE_METHOD OkayToCloseProcedure;         // 0x68
};
struct OBJECT_TYPE_INITIALIZER_VISTA
{
    USHORT Length;                                      // 0x00
    union
    {
        UCHAR ObjectTypeFlags;                          // 0x2
        struct
        {
            UCHAR CaseInsensitive:1;                    // 0x2
            UCHAR UnnamedObjectsOnly:1;                 // 0x2
            UCHAR UseDefaultObject:1;                   // 0x2
            UCHAR SecurityRequired:1;                   // 0x2
            UCHAR MaintainHandleCount:1;                // 0x2
            UCHAR MaintainTypeList:1;                   // 0x2
        };
    };
    ULONG ObjectTypeCode;                               // 0x4
    ULONG InvalidAttributes;                            // 0x8
    GENERIC_MAPPING GenericMapping;                     // 0xc
    ULONG ValidAccessMask;                              // 0x1c
    POOL_TYPE PoolType;                                 // 0x20
    ULONG DefaultPagedPoolCharge;                       // 0x24
    ULONG DefaultNonPagedPoolCharge;                    // 0x28
    OB_DUMP_METHOD DumpProcedure;                       // 0x30
    OB_OPEN_METHOD_VISTA OpenProcedure;                 // 0x38
    OB_CLOSE_METHOD CloseProcedure;                     // 0x40
    OB_DELETE_METHOD DeleteProcedure;                   // 0x48
    OB_PARSE_METHOD ParseProcedure;                     // 0x50
    OB_SECURITY_METHOD_VISTA SecurityProcedure;         // 0x58
    OB_QUERYNAME_METHOD QueryNameProcedure;             // 0x60
    OB_OKAYTOCLOSE_METHOD OkayToCloseProcedure;         // 0x68
};

struct OBJECT_TYPE_INITIALIZER_VISTASP1
{
    USHORT Length;                                      // 0x00
    union
    {
        UCHAR ObjectTypeFlags;                          // 0x02
        struct
        {
            UCHAR CaseInsensitive:1;                    // 0x02
            UCHAR UnnamedObjectsOnly:1;                 // 0x02
            UCHAR UseDefaultObject:1;                   // 0x02
            UCHAR SecurityRequired:1;                   // 0x02
            UCHAR MaintainHandleCount:1;                // 0x02
            UCHAR MaintainTypeList:1;                   // 0x02
            UCHAR SupportsObjectCallbacks:1;            // 0x02
        };
    };
    ULONG ObjectTypeCode;                               // 0x04
    ULONG InvalidAttributes;                            // 0x08
    GENERIC_MAPPING GenericMapping;                     // 0x0c
    ULONG ValidAccessMask;                              // 0x1c
    ULONG RetainAccess;                                 // 0x20
    POOL_TYPE PoolType;                                 // 0x24
    ULONG DefaultPagedPoolCharge;                       // 0x28
    ULONG DefaultNonPagedPoolCharge;                    // 0x2c
    OB_DUMP_METHOD DumpProcedure;                       // 0x30
    OB_OPEN_METHOD_VISTA OpenProcedure;                 // 0x38
    OB_CLOSE_METHOD CloseProcedure;                     // 0x40
    OB_DELETE_METHOD DeleteProcedure;                   // 0x48
    OB_PARSE_METHOD ParseProcedure;                     // 0x50
    OB_SECURITY_METHOD_VISTA SecurityProcedure;         // 0x58
    OB_QUERYNAME_METHOD QueryNameProcedure;             // 0x60
    OB_OKAYTOCLOSE_METHOD OkayToCloseProcedure;         // 0x68
}; 

template<unsigned NtDdiVersion> struct TOBJECT_TYPE_INITIALIZER : public OBJECT_TYPE_INITIALIZER_WS03 {};
template<> struct TOBJECT_TYPE_INITIALIZER<NTDDI_WS03> : public OBJECT_TYPE_INITIALIZER_WS03 {};
template<> struct TOBJECT_TYPE_INITIALIZER<NTDDI_VISTA> : public OBJECT_TYPE_INITIALIZER_VISTA {};
template<> struct TOBJECT_TYPE_INITIALIZER<NTDDI_VISTASP1> : public OBJECT_TYPE_INITIALIZER_VISTASP1 {};
template<> struct TOBJECT_TYPE_INITIALIZER<NTDDI_VISTASP2> : public OBJECT_TYPE_INITIALIZER_VISTASP1 {};
template<> struct TOBJECT_TYPE_INITIALIZER<NTDDI_WIN7> : public OBJECT_TYPE_INITIALIZER_VISTASP1 {};

struct OBJECT_TYPE_WS03
{
    ERESOURCE Mutex;                                    // 0x00
    LIST_ENTRY TypeList;                                // 0x68
    UNICODE_STRING Name;                                // 0x78
    VOID* DefaultObject;                                // 0x88
    ULONG Index;                                        // 0x90
    ULONG TotalNumberOfObjects;                         // 0x94
    ULONG TotalNumberOfHandles;                         // 0x98
    ULONG HighWaterNumberOfObjects;                     // 0x9c
    ULONG HighWaterNumberOfHandles;                     // 0xa0
    OBJECT_TYPE_INITIALIZER_WS03 TypeInfo;              // 0xa8
    ULONG Key;                                          // 0x118
    ERESOURCE ObjectLocks[4];                           // 0x120
};

struct OBJECT_TYPE_VISTASP1
{
    LIST_ENTRY TypeList;                                // 0x00
    UNICODE_STRING Name;                                // 0x10
    VOID* DefaultObject;                                // 0x20
    ULONG Index;                                        // 0x28
    ULONG TotalNumberOfObjects;                         // 0x2c
    ULONG TotalNumberOfHandles;                         // 0x30
    ULONG HighWaterNumberOfObjects;                     // 0x34
    ULONG HighWaterNumberOfHandles;                     // 0x38
    OBJECT_TYPE_INITIALIZER_VISTASP1 TypeInfo;          // 0x40
    ERESOURCE Mutex;                                    // 0xb0
    EX_PUSH_LOCK TypeLock;                              // 0x118
    ULONG Key;                                          // 0x120
    EX_PUSH_LOCK ObjectLocks[32];                       // 0x128
    LIST_ENTRY CallbackList;                            // 0x228
};

template<unsigned NtDdiVersion> struct TOBJECT_TYPE : public OBJECT_TYPE_WS03 {};
template<> struct TOBJECT_TYPE<NTDDI_WS03> : public OBJECT_TYPE_WS03 {};
template<> struct TOBJECT_TYPE<NTDDI_VISTA> : public OBJECT_TYPE_WS03 {};
template<> struct TOBJECT_TYPE<NTDDI_VISTASP1> : public OBJECT_TYPE_VISTASP1 {};
template<> struct TOBJECT_TYPE<NTDDI_VISTASP2> : public OBJECT_TYPE_VISTASP1 {};
template<> struct TOBJECT_TYPE<NTDDI_WIN7> : public OBJECT_TYPE_VISTASP1 {};
