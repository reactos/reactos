/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Object types test
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <kmt_test.h>
#include "ObTypes.hpp"

#define NDEBUG
#include <debug.h>

const UCHAR TypeIndex_Type[] = { 1, 1 };
const UCHAR TypeIndex_Directory[] = { 2, 2 };
const UCHAR TypeIndex_SymbolicLink[] = { 3, 3 };
const UCHAR TypeIndex_Token[] = { 4, 4 };
const UCHAR TypeIndex_Process[] = { 5, 6 };
const UCHAR TypeIndex_Thread[] = { 6, 7 };
const UCHAR TypeIndex_Job[] = { 7, 5 };
const UCHAR TypeIndex_DebugObject[] = { 8, 8 };
const UCHAR TypeIndex_Event[] = { 9, 9 };
const UCHAR TypeIndex_EventPair[] = { 10, 10 };
const UCHAR TypeIndex_Mutant[] = { 11, 11 };
const UCHAR TypeIndex_Callback[] = { 12, 12 };
const UCHAR TypeIndex_Semaphore[] = { 13, 13 };
const UCHAR TypeIndex_Timer[] = { 14, 14 };
const UCHAR TypeIndex_Profile[] = { 15, 15 };
const UCHAR TypeIndex_KeyedEvent[] = { 16, 16 };
const UCHAR TypeIndex_WindowStation[] = { 17, 17 };
const UCHAR TypeIndex_Desktop[] = { 18, 18 };
const UCHAR TypeIndex_Section[] = { 19, 30 };
const UCHAR TypeIndex_Key[] = { 20, 32 };
const UCHAR TypeIndex_Port[] = { 21, 21 };
const UCHAR TypeIndex_WaitablePort[] = { 22, 22 };
const UCHAR TypeIndex_Adapter[] = { 23, 20 };
const UCHAR TypeIndex_Controller[] = { 24, 21 };
const UCHAR TypeIndex_Device[] = { 25, 22 };
const UCHAR TypeIndex_Driver[] = { 26, 23 };
const UCHAR TypeIndex_IoCompletion[] = { 27, 24 };
const UCHAR TypeIndex_File[] = { 28, 25 };
const UCHAR TypeIndex_WmiGuid[] = { 29, 34 };
const UCHAR TypeIndex_FilterConnectionPort[] = { 30, 36 };
const UCHAR TypeIndex_FilterCommunicationPort[] = { 31, 37 };

static
ULONG
GetNtDdiIndex(ULONG NtDdiVersion)
{
    switch (NtDdiVersion)
    {
        case NTDDI_WS03: return 0;
        case NTDDI_VISTA: return 1;
        case NTDDI_VISTASP1: return 1;
        case NTDDI_VISTASP2: return 1;
        default:
            trace("Unsupported NTDDI version 0x%lx\n", NtDdiVersion);
            return 0;
    }
}

#define GetTypeIndex(TypeName, NtDdiVersion) \
    (TypeIndex_ ## TypeName[GetNtDdiIndex(NtDdiVersion)])

static
POBJECT_TYPE
GetObjectType(
    IN PCWSTR TypeName)
{
    NTSTATUS Status;
    UNICODE_STRING Name;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Handle;
    PVOID ObjectType = NULL;

    RtlInitUnicodeString(&Name, TypeName);
    InitializeObjectAttributes(&ObjectAttributes, &Name, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
    Status = ObOpenObjectByName(&ObjectAttributes, NULL, KernelMode, NULL, 0, NULL, &Handle);
    ok(Status == STATUS_SUCCESS, "ObOpenObjectByName failed for '%wZ': %08x\n", &Name, Status);
    ok(Handle != NULL, "ObjectTypeHandle = NULL\n");
    if (!skip(Status == STATUS_SUCCESS && Handle, "No handle\n"))
    {
        Status = ObReferenceObjectByHandle(Handle, 0, NULL, KernelMode, &ObjectType, NULL);
        ok(Status == STATUS_SUCCESS, "ObReferenceObjectByHandle failed for '%wZ': %08x\n", &Name, Status);
        ok(ObjectType != NULL, "ObjectType = NULL\n");
        Status = ZwClose(Handle);
        ok(Status == STATUS_SUCCESS, "ZwClose failed for '%wZ': %08x\n", &Name, Status);
    }
    return (POBJECT_TYPE)ObjectType;
}

#define ok_eq_ustr(value, expected) ok(RtlEqualUnicodeString(value, expected, FALSE), #value " = \"%wZ\", expected \"%%wZ\"\n", value, expected)

#define CheckObjectType(TypeName, Variable, Flags, InvalidAttr,                     \
                        ReadMapping, WriteMapping, ExecMapping, AllMapping,         \
                        ValidMask) do                                               \
{                                                                                   \
    OBJECT_TYPE* ObjectType;                                                        \
    UNICODE_STRING Name;                                                            \
    ULONG Key;                                                                      \
    BOOLEAN UseDefault = ((Flags) & OBT_NO_DEFAULT) == 0;                           \
    BOOLEAN CustomSecurityProc = ((Flags) & OBT_CUSTOM_SECURITY_PROC) != 0;         \
    BOOLEAN SecurityRequired = ((Flags) & OBT_SECURITY_REQUIRED) != 0;              \
    BOOLEAN CaseInsensitive = ((Flags) & OBT_CASE_INSENSITIVE) != 0;                \
    BOOLEAN MaintainTypeList = ((Flags) & OBT_MAINTAIN_TYPE_LIST) != 0;             \
    BOOLEAN MaintainHandleCount = ((Flags) & OBT_MAINTAIN_HANDLE_COUNT) != 0;       \
    POOL_TYPE PoolType = ((Flags) & OBT_PAGED_POOL) ? PagedPool : NonPagedPool;     \
    BOOLEAN CustomKey = ((Flags) & OBT_CUSTOM_KEY) != 0;                            \
    ULONG Index = GetTypeIndex(TypeName, NtDdiVersion);                             \
                                                                                    \
    trace(#TypeName "\n");                                                          \
    ObjectType = (OBJECT_TYPE*)GetObjectType(L"\\ObjectTypes\\" L ## #TypeName);    \
    ok(ObjectType != NULL, "ObjectType = NULL\n");                                  \
    if (!skip(ObjectType != NULL, "No ObjectType\n"))                               \
    {                                                                               \
        ok(!Variable || (OBJECT_TYPE*)Variable == (OBJECT_TYPE*)ObjectType,         \
           #Variable "is %p, expected %p\n", Variable, ObjectType);                 \
        RtlInitUnicodeString(&Name, L ## #TypeName);                                \
        ok_eq_ustr(&ObjectType->Name, &Name);                                       \
        ok_eq_ulong(ObjectType->Index, Index);                                      \
        /* apparently, File and WaitablePort are evil and have other stuff          \
         * in DefaultObject. All others are NULL */                                 \
        if (UseDefault)                                                             \
            ok_eq_pointer(ObjectType->DefaultObject, ObpDefaultObject);             \
        /*ok(ObjectType->TotalNumberOfObjects >= 1,                                 \
           "Number of objects = %lu\n", ObjectType->TotalNumberOfObjects);          \
        ok(ObjectType->TotalNumberOfHandles >= ObjectType->TotalNumberOfObjects,    \
           "%lu objects, but %lu handles\n",                                        \
           ObjectType->TotalNumberOfObjects, ObjectType->TotalNumberOfHandles);*/   \
        ok(ObjectType->HighWaterNumberOfObjects >= ObjectType->TotalNumberOfObjects,\
           "%lu objects, but high water %lu\n",                                     \
           ObjectType->TotalNumberOfObjects, ObjectType->HighWaterNumberOfObjects); \
        ok(ObjectType->HighWaterNumberOfHandles >= ObjectType->TotalNumberOfHandles,\
           "%lu handles, but high water %lu\n",                                     \
           ObjectType->TotalNumberOfHandles, ObjectType->HighWaterNumberOfHandles); \
        ok_eq_ulong(ObjectType->TypeInfo.Length, sizeof(OBJECT_TYPE_INITIALIZER));  \
        ok_eq_bool(ObjectType->TypeInfo.UseDefaultObject, UseDefault);              \
        ok_eq_bool(ObjectType->TypeInfo.CaseInsensitive, CaseInsensitive);          \
        ok_eq_hex(ObjectType->TypeInfo.InvalidAttributes, InvalidAttr);             \
        ok_eq_hex(ObjectType->TypeInfo.GenericMapping.GenericRead, ReadMapping);    \
        ok_eq_hex(ObjectType->TypeInfo.GenericMapping.GenericWrite, WriteMapping);  \
        ok_eq_hex(ObjectType->TypeInfo.GenericMapping.GenericExecute, ExecMapping); \
        ok_eq_hex(ObjectType->TypeInfo.GenericMapping.GenericAll, AllMapping);      \
        ok_eq_hex(ObjectType->TypeInfo.ValidAccessMask, ValidMask);                 \
        ok_eq_bool(ObjectType->TypeInfo.SecurityRequired, SecurityRequired);        \
        ok_eq_bool(ObjectType->TypeInfo.MaintainHandleCount, MaintainHandleCount);  \
        ok_eq_bool(ObjectType->TypeInfo.MaintainTypeList, MaintainTypeList);        \
        ok_eq_ulong(ObjectType->TypeInfo.PoolType, PoolType);                       \
        /* DefaultPagedPoolCharge */                                                \
        /* DefaultNonPagedPoolCharge */                                             \
        /* DumpProcedure */                                                         \
        /* OpenProcedure */                                                         \
        /* CloseProcedure */                                                        \
        /* DeleteProcedure */                                                       \
        /* ParseProcedure */                                                        \
        if (CustomSecurityProc)                                                     \
            ok(ObjectType->TypeInfo.SecurityProcedure != NULL,                      \
               "No Security Proc\n");                                               \
        else                                                                        \
            ok_eq_pointer(ObjectType->TypeInfo.SecurityProcedure,                   \
                          SeDefaultObjectMethod);                                   \
        /* QueryNameProcedure */                                                    \
        /* OkayToCloseProcedure */                                                  \
        Key = *(PULONG)#TypeName;                                                   \
        if (sizeof(#TypeName) <= 4) Key |= ' ' << 24;                               \
        if (sizeof(#TypeName) <= 3) Key |= ' ' << 16;                               \
        if (sizeof(#TypeName) <= 2) Key |= ' ' <<  8;                               \
        if (!CustomKey)                                                             \
            ok_eq_hex(ObjectType->Key, Key);                                        \
        ObDereferenceObject(ObjectType);                                            \
    }                                                                               \
} while (0)

#define ObpDirectoryObjectType      NULL
#define ObpSymbolicLinkObjectType   NULL
#define DbgkDebugObjectType         NULL
#define ExEventPairObjectType       NULL
#define ExMutantObjectType          NULL
#define ExCallbackObjectType        NULL
#define ExTimerObjectType           NULL
#define ExProfileObjectType         NULL
#define ExpKeyedEventObjectType     NULL
#define CmpKeyObjectType            NULL
#define LpcWaitablePortObjectType   NULL
#define IoControllerObjectType      NULL
#define IoCompletionObjectType      NULL
#define WmipGuidObjectType          NULL

#define OBT_NO_DEFAULT              0x01
#define OBT_CUSTOM_SECURITY_PROC    0x02
#define OBT_SECURITY_REQUIRED       0x04
#define OBT_CASE_INSENSITIVE        0x08
#define OBT_MAINTAIN_TYPE_LIST      0x10
#define OBT_MAINTAIN_HANDLE_COUNT   0x20
#define OBT_PAGED_POOL              0x40
#define OBT_CUSTOM_KEY              0x80

#define TAG(x) RtlUlongByteSwap(x)

template<unsigned NtDdiVersion>
static
VOID
TestObjectTypes(VOID)
{
    using OBJECT_TYPE = TOBJECT_TYPE<NtDdiVersion>;
    static OBJECT_TYPE* ObpTypeObjectType;
    static OBJECT_TYPE* ObpDefaultObject;
    using OB_SECURITY_METHOD = decltype(ObpTypeObjectType->TypeInfo.SecurityProcedure);
    static OB_SECURITY_METHOD SeDefaultObjectMethod;

    ObpTypeObjectType = (OBJECT_TYPE*)GetObjectType(L"\\ObjectTypes\\Type");
    if (skip(ObpTypeObjectType != NULL, "No Type object type\n"))
        return;

    ObpDefaultObject = (OBJECT_TYPE*)ObpTypeObjectType->DefaultObject;
    ok(ObpDefaultObject != NULL, "No ObpDefaultObject\n");
    SeDefaultObjectMethod = ObpTypeObjectType->TypeInfo.SecurityProcedure;
    ok(SeDefaultObjectMethod != NULL, "No SeDefaultObjectMethod\n");

#ifdef _PROPER_NT_NDK_EXPORTS
#define ObpTypeObjectType *ObpTypeObjectType
#define ObpDirectoryObjectType *ObpDirectoryObjectType
#define ObpSymbolicLinkObjectType *ObpSymbolicLinkObjectType
#define PsJobType *PsJobType
#define DbgkDebugObjectType *DbgkDebugObjectType
#define ExEventPairObjectType *ExEventPairObjectType
#define ExMutantObjectType *ExMutantObjectType
#define ExCallbackObjectType *ExCallbackObjectType
#define ExTimerObjectType *ExTimerObjectType
#define ExProfileObjectType *ExProfileObjectType
#define ExpKeyedEventObjectType *ExpKeyedEventObjectType
#define ExWindowStationObjectType *ExWindowStationObjectType
#define ExDesktopObjectType *ExDesktopObjectType
#define MmSectionObjectType *MmSectionObjectType
#define CmpKeyObjectType *CmpKeyObjectType
#define LpcPortObjectType *LpcPortObjectType
#define LpcWaitablePortObjectType *LpcWaitablePortObjectType
#define IoAdapterObjectType *IoAdapterObjectType
#define IoControllerObjectType *IoControllerObjectType
#define IoDeviceObjectType *IoDeviceObjectType
#define IoDriverObjectType *IoDriverObjectType
#define IoCompletionObjectType *IoCompletionObjectType
#define WmipGuidObjectType *WmipGuidObjectType
#endif

#define DIR_SEC_REQUIRED ((NtDdiVersion >= NTDDI_VISTA) ? OBT_SECURITY_REQUIRED : 0)
#define TOKEN_GENERIC_READ ((NtDdiVersion >= NTDDI_VISTA) ? 0x02001a : 0x020008)
#define TOKEN_GENERIC_WRITE ((NtDdiVersion >= NTDDI_VISTA) ? 0x0201e0 : 0x0200e0)
#define TOKEN_GENERIC_EXECUTE ((NtDdiVersion >= NTDDI_VISTA) ? 0x020005 : 0x020000)
#define PROCESS_GENERIC_WRITE ((NtDdiVersion >= NTDDI_VISTA) ? 0x020bea : 0x020beb)
#define PROCESS_GENERIC_EXECUTE ((NtDdiVersion >= NTDDI_VISTA) ? 0x121001 : 0x120000)
#define PROCESS_GENERIC_ALL ((NtDdiVersion >= NTDDI_VISTA) ? 0x001fffff : 0x1f0fff)
#define THREAD_GENERIC_WRITE ((NtDdiVersion >= NTDDI_VISTA) ? 0x020437 : 0x020037)
#define THREAD_GENERIC_EXECUTE ((NtDdiVersion >= NTDDI_VISTA) ? 0x120800 : 0x120000)
#define THREAD_GENERIC_ALL ((NtDdiVersion >= NTDDI_VISTA) ? 0x1fffff : 0x1f03ff)
#define JOB_GENERIC_ALL ((NtDdiVersion >= NTDDI_VISTA) ? 0x1f001f : 0x1f03ff)
#define KEY_GENERIC_EXECUTE ((NtDdiVersion >= NTDDI_VISTA) ? 0x020039 : 0x020019)
#define PROCESS_INDEX ((NtDdiVersion >= NTDDI_VISTA) ? 6 : 5)

    CheckObjectType(Type, ObpTypeObjectType,                    OBT_MAINTAIN_TYPE_LIST | OBT_CUSTOM_KEY,    0x100,  0x020000, 0x020000, 0x020000, 0x0f0001, 0x1f0001);
        ok_eq_hex(ObpTypeObjectType->Key, TAG('ObjT'));
    CheckObjectType(Directory, ObpDirectoryObjectType,          DIR_SEC_REQUIRED | OBT_CASE_INSENSITIVE | OBT_PAGED_POOL,      0x100,  0x020003, 0x02000c, 0x020003, 0x0f000f, 0x0f000f);
    CheckObjectType(SymbolicLink, ObpSymbolicLinkObjectType,    OBT_CASE_INSENSITIVE | OBT_PAGED_POOL,      0x100,  0x020001, 0x020000, 0x020001, 0x0f0001, 0x0f0001);
    CheckObjectType(Token, *SeTokenObjectType,                   OBT_SECURITY_REQUIRED | OBT_PAGED_POOL,     0x100,  TOKEN_GENERIC_READ, TOKEN_GENERIC_WRITE, TOKEN_GENERIC_EXECUTE, 0x0f01ff, 0x1f01ff);
    CheckObjectType(Process, *PsProcessType,                     OBT_NO_DEFAULT | OBT_SECURITY_REQUIRED,     0x0b0,  0x020410, PROCESS_GENERIC_WRITE, PROCESS_GENERIC_EXECUTE, PROCESS_GENERIC_ALL, PROCESS_GENERIC_ALL);
    CheckObjectType(Thread, *PsThreadType,                       OBT_NO_DEFAULT | OBT_SECURITY_REQUIRED,     0x0b0,  0x020048, THREAD_GENERIC_WRITE, THREAD_GENERIC_EXECUTE, THREAD_GENERIC_ALL, THREAD_GENERIC_ALL);
    CheckObjectType(Job, PsJobType,                             OBT_NO_DEFAULT | OBT_SECURITY_REQUIRED,     0x000,  0x020004, 0x02000b, 0x120000, JOB_GENERIC_ALL, 0x1f001f);
    CheckObjectType(DebugObject, DbgkDebugObjectType,           OBT_NO_DEFAULT | OBT_SECURITY_REQUIRED,     0x000,  0x020001, 0x020002, 0x120000, 0x1f000f, 0x1f000f);
    CheckObjectType(Event, *ExEventObjectType,                   OBT_NO_DEFAULT,                             0x100,  0x020001, 0x020002, 0x120000, 0x1f0003, 0x1f0003);
    CheckObjectType(EventPair, ExEventPairObjectType,           0,                                          0x100,  0x120000, 0x120000, 0x120000, 0x1f0000, 0x1f0000);
    CheckObjectType(Mutant, ExMutantObjectType,                 OBT_NO_DEFAULT,                             0x100,  0x020001, 0x020000, 0x120000, 0x1f0001, 0x1f0001);
    CheckObjectType(Callback, ExCallbackObjectType,             OBT_NO_DEFAULT,                             0x100,  0x020000, 0x020001, 0x120000, 0x1f0001, 0x1f0001);
    CheckObjectType(Semaphore, *ExSemaphoreObjectType,           OBT_NO_DEFAULT,                             0x100,  0x020001, 0x020002, 0x120000, 0x1f0003, 0x1f0003);
    CheckObjectType(Timer, ExTimerObjectType,                   OBT_NO_DEFAULT,                             0x100,  0x020001, 0x020002, 0x120000, 0x1f0003, 0x1f0003);
    CheckObjectType(Profile, ExProfileObjectType,               OBT_NO_DEFAULT,                             0x100,  0x020001, 0x020001, 0x020001, 0x0f0001, 0x0f0001);
    CheckObjectType(KeyedEvent, ExpKeyedEventObjectType,        OBT_PAGED_POOL,                             0x000,  0x020001, 0x020002, 0x020000, 0x0f0003, 0x1f0003);
    CheckObjectType(WindowStation, ExWindowStationObjectType,   OBT_NO_DEFAULT | OBT_SECURITY_REQUIRED | OBT_MAINTAIN_HANDLE_COUNT,
                                                                                                            0x130,  0x020303, 0x02001c, 0x020060, 0x0f037f, 0x0f037f);
    CheckObjectType(Desktop, ExDesktopObjectType,               OBT_NO_DEFAULT | OBT_SECURITY_REQUIRED | OBT_MAINTAIN_HANDLE_COUNT,
                                                                                                            0x130,  0x020041, 0x0200be, 0x020100, 0x0f01ff, 0x0f01ff);
    CheckObjectType(Section, MmSectionObjectType,               OBT_PAGED_POOL,                             0x100,  0x020005, 0x020002, 0x020008, 0x0f001f, 0x1f001f);
    CheckObjectType(Key, CmpKeyObjectType,                      OBT_CUSTOM_SECURITY_PROC | OBT_SECURITY_REQUIRED | OBT_PAGED_POOL,
                                                                                                            0x030,  0x020019, 0x020006, KEY_GENERIC_EXECUTE, 0x0f003f, 0x1f003f);
    if (NtDdiVersion <= NTDDI_WS03)
    {
        // 0x7b2 is used for Server 2003 SP2 RTM, it seems it was changed to 0xfb2 in some patch level.
        CheckObjectType(Port, LpcPortObjectType,                    OBT_PAGED_POOL,                             0xfb2,  0x020001, 0x010001, 0x000000, 0x1f0001, 0x1f0001);
        // 0x7b2 is used for Server 2003 SP2 RTM, it seems it was changed to 0xfb2 in some patch level.
        CheckObjectType(WaitablePort, LpcWaitablePortObjectType,    OBT_NO_DEFAULT,                             0xfb2,  0x020001, 0x010001, 0x000000, 0x1f0001, 0x1f0001);
    }
    CheckObjectType(Adapter, IoAdapterObjectType,               0,                                          0x100,  0x120089, 0x120116, 0x1200a0, 0x1f01ff, 0x1f01ff);
    CheckObjectType(Controller, IoControllerObjectType,         0,                                          0x100,  0x120089, 0x120116, 0x1200a0, 0x1f01ff, 0x1f01ff);
    CheckObjectType(Device, IoDeviceObjectType,                 OBT_CUSTOM_SECURITY_PROC | OBT_CASE_INSENSITIVE,
                                                                                                            0x100,  0x120089, 0x120116, 0x1200a0, 0x1f01ff, 0x1f01ff);
    CheckObjectType(Driver, IoDriverObjectType,                 OBT_CASE_INSENSITIVE,                       0x100,  0x120089, 0x120116, 0x1200a0, 0x1f01ff, 0x1f01ff);
    CheckObjectType(IoCompletion, IoCompletionObjectType,       OBT_CASE_INSENSITIVE,                       0x110,  0x020001, 0x020002, 0x120000, 0x1f0003, 0x1f0003);
    CheckObjectType(File, *IoFileObjectType,                     OBT_NO_DEFAULT | OBT_CUSTOM_SECURITY_PROC | OBT_CASE_INSENSITIVE | OBT_MAINTAIN_HANDLE_COUNT,
                                                                                                            0x130,  0x120089, 0x120116, 0x1200a0, 0x1f01ff, 0x1f01ff);
    CheckObjectType(WmiGuid, WmipGuidObjectType,                OBT_NO_DEFAULT | OBT_CUSTOM_SECURITY_PROC | OBT_SECURITY_REQUIRED,
                                                                                                            0x100,  0x000001, 0x000002, 0x000010, 0x120fff, 0x1f0fff);
    CheckObjectType(FilterConnectionPort, NULL,                 OBT_NO_DEFAULT | OBT_SECURITY_REQUIRED,     0x100,  0x020001, 0x010001, 0x000000, 0x1f0001, 0x1f0001);
    CheckObjectType(FilterCommunicationPort, NULL,              OBT_NO_DEFAULT,                             0x100,  0x020001, 0x010001, 0x000000, 0x1f0001, 0x1f0001);

    // exported but not created
    ok_eq_pointer(IoDeviceHandlerObjectType, NULL);

    // my Win7/x64 additionally has:
    // ALPC Port
    // EtwConsumer
    // EtwRegistration
    // IoCompletionReserve
    // PcwObject
    // PowerRequest
    // Session
    // TmEn
    // TmRm
    // TmTm
    // TmTx
    // TpWorkerFactory
    // UserApcReserve
    // ... and does not have:
    // Port
    // WaitablePort

    ObDereferenceObject(ObpTypeObjectType);
}

START_TEST(ObTypes)
{
    ULONG NtDdiVersion = GetNTDDIVersion();
    switch (NtDdiVersion)
    {
        case NTDDI_WS03:
            TestObjectTypes<NTDDI_WS03>();
            return;
        case NTDDI_VISTA:
            TestObjectTypes<NTDDI_VISTA>();
            return;
        case NTDDI_VISTASP1:
            TestObjectTypes<NTDDI_VISTASP1>();
            return;
        case NTDDI_VISTASP2:
            TestObjectTypes<NTDDI_VISTASP2>();
            return;
        case NTDDI_VISTASP3:
            TestObjectTypes<NTDDI_VISTASP3>();
            return;
        default:
            skip(FALSE, "Unsupported NTDDI version: 0x%lx\n", NtDdiVersion);
            return;
    }
}
