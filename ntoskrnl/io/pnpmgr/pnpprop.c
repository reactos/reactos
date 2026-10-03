/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     DEVPROPKEY property store for devices and device interfaces
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#include <devpkey.h>

#define NDEBUG
#include <debug.h>

/* DEFINES *******************************************************************/

#define TAG_PNP_PROPERTY            'rPpP'

/* Store values: <ObjectKey>\Properties\{FMTID}\<PID>, type PIP_PROP_REG_TYPE_MARK | DEVPROPTYPE */
#define PIP_PROP_REG_TYPE_MARK      0xFFFF0000UL

#define PIP_PROP_FMTID_CHARS        (38 + 1)
#define PIP_PROP_PID_CHARS          (8 + 1)
#define PIP_PROP_PATH_CHARS         (10 + 1 + 38 + 1 + 8 + 1)
#define PIP_PROP_MAX_QUERY_SIZE     0x10000

#define PIP_PROP_READ_ONLY          0x00000001
#define PIP_PROP_ROOT_ENUM_WRITE    0x00000002

#define PIP_PROP_OBJECT_PROTECTED   0x00000001

/* TYPES *********************************************************************/

typedef enum _PIP_PROP_OBJECT_KIND
{
    PipPropObjectDevice,
    PipPropObjectInterface
} PIP_PROP_OBJECT_KIND;

typedef struct _PIP_PROP_OBJECT
{
    PIP_PROP_OBJECT_KIND Kind;
    ULONG Flags;
    PDEVICE_OBJECT Pdo;
    PDEVICE_NODE DeviceNode;
    PCUNICODE_STRING Name;
    GUID ClassGuid;
} PIP_PROP_OBJECT, *PPIP_PROP_OBJECT;

typedef struct _PIP_PROP_NAME
{
    const DEVPROPKEY *Key;
    UNICODE_STRING Locale;
    UNICODE_STRING Fmtid;
    UNICODE_STRING Pid;
    WCHAR LocaleBuffer[LOCALE_NAME_MAX_LENGTH];
    WCHAR FmtidBuffer[PIP_PROP_FMTID_CHARS];
    WCHAR PidBuffer[PIP_PROP_PID_CHARS];
} PIP_PROP_NAME, *PPIP_PROP_NAME;

typedef enum _PIP_PROP_SOURCE
{
    PipSourceKeyValue,
    PipSourceRegistryProperty,
    PipSourceDeviceNode,
    PipSourceInterface
} PIP_PROP_SOURCE;

typedef enum _PIP_PROP_FIELD
{
    PipFieldName,
    PipFieldInstanceId,
    PipFieldParent,
    PipFieldStatus,
    PipFieldProblemCode,
    PipFieldHasProblem,
    PipFieldIsPresent,
    PipFieldInterfaceClass,
    PipFieldInterfaceEnabled,
    PipFieldInterfaceDevice
} PIP_PROP_FIELD;

typedef struct _PIP_BUILTIN_PROPERTY
{
    const DEVPROPKEY *Key;
    DEVPROPTYPE Type;
    PIP_PROP_SOURCE Source;
    ULONG Flags;
    PCWSTR ValueName;
    ULONG Selector;                     /* REG_xxx, DEVICE_REGISTRY_PROPERTY or PIP_PROP_FIELD */
} PIP_BUILTIN_PROPERTY, *PPIP_BUILTIN_PROPERTY;

/* GLOBALS *******************************************************************/

static const PIP_BUILTIN_PROPERTY PipDeviceBuiltins[] =
{
    { &DEVPKEY_Device_DeviceDesc,     DEVPROP_TYPE_STRING,      PipSourceKeyValue,         0,                        REGSTR_VAL_DEVDESC,              REG_SZ },
    { &DEVPKEY_Device_HardwareIds,    DEVPROP_TYPE_STRING_LIST, PipSourceKeyValue,         PIP_PROP_ROOT_ENUM_WRITE, REGSTR_VAL_HARDWAREID,           REG_MULTI_SZ },
    { &DEVPKEY_Device_CompatibleIds,  DEVPROP_TYPE_STRING_LIST, PipSourceKeyValue,         PIP_PROP_ROOT_ENUM_WRITE, REGSTR_VAL_COMPATIBLEIDS,        REG_MULTI_SZ },
    { &DEVPKEY_Device_Service,        DEVPROP_TYPE_STRING,      PipSourceKeyValue,         0,                        REGSTR_VAL_SERVICE,              REG_SZ },
    { &DEVPKEY_Device_Class,          DEVPROP_TYPE_STRING,      PipSourceKeyValue,         0,                        REGSTR_VAL_CLASS,                REG_SZ },
    { &DEVPKEY_Device_ClassGuid,      DEVPROP_TYPE_GUID,        PipSourceKeyValue,         0,                        REGSTR_VAL_CLASSGUID,            REG_SZ },
    { &DEVPKEY_Device_Driver,         DEVPROP_TYPE_STRING,      PipSourceKeyValue,         0,                        REGSTR_VAL_DRIVER,               REG_SZ },
    { &DEVPKEY_Device_ConfigFlags,    DEVPROP_TYPE_UINT32,      PipSourceKeyValue,         0,                        REGSTR_VAL_CONFIGFLAGS,          REG_DWORD },
    { &DEVPKEY_Device_Manufacturer,   DEVPROP_TYPE_STRING,      PipSourceKeyValue,         0,                        REGSTR_VAL_MFG,                  REG_SZ },
    { &DEVPKEY_Device_FriendlyName,   DEVPROP_TYPE_STRING,      PipSourceKeyValue,         0,                        REGSTR_VAL_FRIENDLYNAME,         REG_SZ },
    { &DEVPKEY_Device_LocationInfo,   DEVPROP_TYPE_STRING,      PipSourceKeyValue,         0,                        REGSTR_VAL_LOCATION_INFORMATION, REG_SZ },
    { &DEVPKEY_Device_Capabilities,   DEVPROP_TYPE_UINT32,      PipSourceKeyValue,         PIP_PROP_READ_ONLY,       REGSTR_VAL_CAPABILITIES,         REG_DWORD },
    { &DEVPKEY_Device_UINumber,       DEVPROP_TYPE_UINT32,      PipSourceKeyValue,         PIP_PROP_READ_ONLY,       REGSTR_VAL_UI_NUMBER,            REG_DWORD },
    { &DEVPKEY_Device_UpperFilters,   DEVPROP_TYPE_STRING_LIST, PipSourceKeyValue,         0,                        REGSTR_VAL_UPPERFILTERS,         REG_MULTI_SZ },
    { &DEVPKEY_Device_LowerFilters,   DEVPROP_TYPE_STRING_LIST, PipSourceKeyValue,         0,                        REGSTR_VAL_LOWERFILTERS,         REG_MULTI_SZ },

    { &DEVPKEY_Device_PDOName,        DEVPROP_TYPE_STRING,      PipSourceRegistryProperty, PIP_PROP_READ_ONLY,       NULL, DevicePropertyPhysicalDeviceObjectName },
    { &DEVPKEY_Device_BusTypeGuid,    DEVPROP_TYPE_GUID,        PipSourceRegistryProperty, PIP_PROP_READ_ONLY,       NULL, DevicePropertyBusTypeGuid },
    { &DEVPKEY_Device_LegacyBusType,  DEVPROP_TYPE_UINT32,      PipSourceRegistryProperty, PIP_PROP_READ_ONLY,       NULL, DevicePropertyLegacyBusType },
    { &DEVPKEY_Device_BusNumber,      DEVPROP_TYPE_UINT32,      PipSourceRegistryProperty, PIP_PROP_READ_ONLY,       NULL, DevicePropertyBusNumber },
    { &DEVPKEY_Device_EnumeratorName, DEVPROP_TYPE_STRING,      PipSourceRegistryProperty, PIP_PROP_READ_ONLY,       NULL, DevicePropertyEnumeratorName },
    { &DEVPKEY_Device_Address,        DEVPROP_TYPE_UINT32,      PipSourceRegistryProperty, PIP_PROP_READ_ONLY,       NULL, DevicePropertyAddress },
    { &DEVPKEY_Device_RemovalPolicy,  DEVPROP_TYPE_UINT32,      PipSourceRegistryProperty, PIP_PROP_READ_ONLY,       NULL, DevicePropertyRemovalPolicy },

    { &DEVPKEY_NAME,                  DEVPROP_TYPE_STRING,      PipSourceDeviceNode,       PIP_PROP_READ_ONLY,       NULL, PipFieldName },
    { &DEVPKEY_Device_InstanceId,     DEVPROP_TYPE_STRING,      PipSourceDeviceNode,       PIP_PROP_READ_ONLY,       NULL, PipFieldInstanceId },
    { &DEVPKEY_Device_Parent,         DEVPROP_TYPE_STRING,      PipSourceDeviceNode,       PIP_PROP_READ_ONLY,       NULL, PipFieldParent },
    { &DEVPKEY_Device_DevNodeStatus,  DEVPROP_TYPE_UINT32,      PipSourceDeviceNode,       PIP_PROP_READ_ONLY,       NULL, PipFieldStatus },
    { &DEVPKEY_Device_ProblemCode,    DEVPROP_TYPE_UINT32,      PipSourceDeviceNode,       PIP_PROP_READ_ONLY,       NULL, PipFieldProblemCode },
    { &DEVPKEY_Device_HasProblem,     DEVPROP_TYPE_BOOLEAN,     PipSourceDeviceNode,       PIP_PROP_READ_ONLY,       NULL, PipFieldHasProblem },
    { &DEVPKEY_Device_IsPresent,      DEVPROP_TYPE_BOOLEAN,     PipSourceDeviceNode,       PIP_PROP_READ_ONLY,       NULL, PipFieldIsPresent },
};

static const PIP_BUILTIN_PROPERTY PipInterfaceBuiltins[] =
{
    { &DEVPKEY_DeviceInterface_ClassGuid, DEVPROP_TYPE_GUID,    PipSourceInterface,        PIP_PROP_READ_ONLY,       NULL, PipFieldInterfaceClass },
    { &DEVPKEY_DeviceInterface_Enabled,   DEVPROP_TYPE_BOOLEAN, PipSourceInterface,        PIP_PROP_READ_ONLY,       NULL, PipFieldInterfaceEnabled },
    { &DEVPKEY_Device_InstanceId,         DEVPROP_TYPE_STRING,  PipSourceInterface,        PIP_PROP_READ_ONLY,       NULL, PipFieldInterfaceDevice },
};

static const DEVPROPKEY PiPropInterruptKey =
{
    { 0xF0E20F09, 0xD97A, 0x49A9, { 0x80, 0x46, 0xBB, 0x6E, 0x22, 0xE6, 0xBB, 0x2E } },
    2
};

/* PRIVATE FUNCTIONS *********************************************************/

static
BOOLEAN
PiPropIsSameKey(
    _In_ const DEVPROPKEY *Key1,
    _In_ const DEVPROPKEY *Key2)
{
    return (Key1->pid == Key2->pid) &&
           RtlEqualMemory(&Key1->fmtid, &Key2->fmtid, sizeof(GUID));
}

typedef enum _PIP_PROP_CHECK
{
    PipCheckNone,
    PipCheckNoData,
    PipCheckBoolean,
    PipCheckFileTime,
    PipCheckString,
    PipCheckSecurity
} PIP_PROP_CHECK;

typedef struct _PIP_PROP_TYPE_RULE
{
    UCHAR Width;
    BOOLEAN ListAllowed;
    UCHAR Check;
} PIP_PROP_TYPE_RULE;

/* Indexed by base DEVPROPTYPE, a zero width means variable length */
static const PIP_PROP_TYPE_RULE PiPropTypeRules[MAX_DEVPROP_TYPE + 1] =
{
    { 0,                       FALSE, PipCheckNoData },     /* EMPTY */
    { 0,                       FALSE, PipCheckNoData },     /* NULL */
    { sizeof(CHAR),            FALSE, PipCheckNone },       /* SBYTE */
    { sizeof(UCHAR),           FALSE, PipCheckNone },       /* BYTE */
    { sizeof(SHORT),           FALSE, PipCheckNone },       /* INT16 */
    { sizeof(USHORT),          FALSE, PipCheckNone },       /* UINT16 */
    { sizeof(LONG),            FALSE, PipCheckNone },       /* INT32 */
    { sizeof(ULONG),           FALSE, PipCheckNone },       /* UINT32 */
    { sizeof(LONGLONG),        FALSE, PipCheckNone },       /* INT64 */
    { sizeof(ULONGLONG),       FALSE, PipCheckNone },       /* UINT64 */
    { sizeof(ULONG),           FALSE, PipCheckNone },       /* FLOAT */
    { sizeof(ULONGLONG),       FALSE, PipCheckNone },       /* DOUBLE */
    { 16,                      FALSE, PipCheckNone },       /* DECIMAL */
    { sizeof(GUID),            FALSE, PipCheckNone },       /* GUID */
    { sizeof(LONGLONG),        FALSE, PipCheckNone },       /* CURRENCY */
    { sizeof(ULONGLONG),       FALSE, PipCheckNone },       /* DATE */
    { sizeof(LARGE_INTEGER),   FALSE, PipCheckFileTime },   /* FILETIME */
    { sizeof(DEVPROP_BOOLEAN), FALSE, PipCheckBoolean },    /* BOOLEAN */
    { 0,                       TRUE,  PipCheckString },     /* STRING */
    { 0,                       FALSE, PipCheckSecurity },   /* SECURITY_DESCRIPTOR */
    { 0,                       TRUE,  PipCheckString },     /* SECURITY_DESCRIPTOR_STRING */
    { sizeof(DEVPROPKEY),      FALSE, PipCheckNone },       /* DEVPROPKEY */
    { sizeof(DEVPROPTYPE),     FALSE, PipCheckNone },       /* DEVPROPTYPE */
    { sizeof(ULONG),           FALSE, PipCheckNone },       /* ERROR */
    { sizeof(NTSTATUS),        FALSE, PipCheckNone },       /* NTSTATUS */
    { 0,                       FALSE, PipCheckString },     /* STRING_INDIRECT */
};

static
BOOLEAN
PiPropCheckElements(
    _In_ UCHAR Check,
    _In_reads_bytes_(Width * Count) PUCHAR Data,
    _In_ ULONG Width,
    _In_ ULONG Count)
{
    LONGLONG Time;

    for (; Count; Count--, Data += Width)
    {
        if (Check == PipCheckBoolean)
        {
            if ((*Data != (UCHAR)DEVPROP_FALSE) && (*Data != (UCHAR)DEVPROP_TRUE))
                return FALSE;
        }
        else if (Check == PipCheckFileTime)
        {
            RtlCopyMemory(&Time, Data, sizeof(Time));
            if (Time < 0)
                return FALSE;
        }
    }

    return TRUE;
}

static
BOOLEAN
PiPropIsValidString(
    _In_reads_bytes_(Size) PCWSTR String,
    _In_ ULONG Size)
{
    ULONG Chars, Index;

    if ((Size < sizeof(WCHAR)) || (Size % sizeof(WCHAR)) ||
        (Size > UNICODE_STRING_MAX_BYTES))
    {
        return FALSE;
    }

    Chars = Size / sizeof(WCHAR);
    for (Index = 0; Index < Chars - 1; Index++)
    {
        if (String[Index] == UNICODE_NULL)
            return FALSE;
    }

    return (String[Chars - 1] == UNICODE_NULL);
}

static
BOOLEAN
PiPropIsValidStringList(
    _In_reads_bytes_(Size) PCWSTR List,
    _In_ ULONG Size)
{
    ULONG Chars, Index, EntryStart;

    if ((Size < sizeof(WCHAR)) || (Size % sizeof(WCHAR)))
        return FALSE;

    Chars = Size / sizeof(WCHAR);
    EntryStart = 0;

    for (Index = 0; Index < Chars; Index++)
    {
        if (List[Index] != UNICODE_NULL)
            continue;

        if (Index == EntryStart)
            return (Index == Chars - 1);

        if ((Index - EntryStart + 1) * sizeof(WCHAR) > UNICODE_STRING_MAX_BYTES)
            return FALSE;

        EntryStart = Index + 1;
    }

    return FALSE;
}

static
NTSTATUS
PiPropCheckPayload(
    _In_ DEVPROPTYPE Type,
    _In_reads_bytes_opt_(Size) PVOID Data,
    _In_ ULONG Size)
{
    DEVPROPTYPE BaseType = Type & DEVPROP_MASK_TYPE;
    const PIP_PROP_TYPE_RULE *Rule;
    BOOLEAN Valid;

    if ((BaseType > MAX_DEVPROP_TYPE) || (Size && !Data))
        return STATUS_INVALID_PARAMETER;

    Rule = &PiPropTypeRules[BaseType];

    /* Everything above the base type must be a single known modifier */
    switch (Type - BaseType)
    {
        case 0:
            if (Rule->Width)
                Valid = (Size == Rule->Width);
            else if (Rule->Check == PipCheckNoData)
                Valid = (Size == 0);
            else if (Rule->Check == PipCheckString)
                Valid = PiPropIsValidString(Data, Size);
            else
                Valid = (Size != 0) &&
                        RtlValidRelativeSecurityDescriptor(Data, Size, 0) &&
                        (RtlLengthSecurityDescriptor(Data) == Size);
            break;

        case DEVPROP_TYPEMOD_ARRAY:
            Valid = (Rule->Width != 0) && (Size != 0) && !(Size % Rule->Width);
            break;

        case DEVPROP_TYPEMOD_LIST:
            Valid = Rule->ListAllowed && PiPropIsValidStringList(Data, Size);
            break;

        default:
            Valid = FALSE;
            break;
    }

    if (Valid && Rule->Width && (Rule->Check != PipCheckNone))
        Valid = PiPropCheckElements(Rule->Check, Data, Rule->Width, Size / Rule->Width);

    return Valid ? STATUS_SUCCESS : STATUS_INVALID_PARAMETER;
}

static
NTSTATUS
PiPropInitLocale(
    _Out_ PPIP_PROP_NAME Name,
    _In_ LCID Lcid)
{
    RtlInitEmptyUnicodeString(&Name->Locale,
                              Name->LocaleBuffer,
                              sizeof(Name->LocaleBuffer));

    if ((Lcid != LOCALE_NEUTRAL) && !RtlLCIDToCultureName(Lcid, &Name->Locale))
    {
        DPRINT1("Cannot resolve LCID 0x%lx\n", Lcid);
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

static
NTSTATUS
PiPropInitName(
    _Out_ PPIP_PROP_NAME Name,
    _In_ const DEVPROPKEY *Key,
    _In_ LCID Lcid)
{
    const GUID *Fmtid = &Key->fmtid;
    NTSTATUS Status;

    Name->Key = Key;

    Status = PiPropInitLocale(Name, Lcid);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = RtlStringCchPrintfW(Name->FmtidBuffer,
                                 RTL_NUMBER_OF(Name->FmtidBuffer),
                                 L"{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
                                 Fmtid->Data1, Fmtid->Data2, Fmtid->Data3,
                                 Fmtid->Data4[0], Fmtid->Data4[1],
                                 Fmtid->Data4[2], Fmtid->Data4[3],
                                 Fmtid->Data4[4], Fmtid->Data4[5],
                                 Fmtid->Data4[6], Fmtid->Data4[7]);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = RtlStringCchPrintfW(Name->PidBuffer,
                                 RTL_NUMBER_OF(Name->PidBuffer),
                                 L"%04lX",
                                 Key->pid);
    if (!NT_SUCCESS(Status))
        return Status;

    RtlInitUnicodeString(&Name->Fmtid, Name->FmtidBuffer);
    RtlInitUnicodeString(&Name->Pid, Name->PidBuffer);
    return STATUS_SUCCESS;
}

static
BOOLEAN
PiPropIsInterruptKey(
    _In_ PPIP_PROP_OBJECT Object,
    _In_ PPIP_PROP_NAME Name)
{
    return (Object->Pdo != NULL) &&
           (Name->Locale.Length == 0) &&
           PiPropIsSameKey(Name->Key, &PiPropInterruptKey);
}

static
NTSTATUS
PiPropNormalizeStatus(
    _In_ NTSTATUS Status)
{
    if ((Status == STATUS_NOT_FOUND) ||
        (Status == STATUS_OBJECT_PATH_NOT_FOUND) ||
        (Status == STATUS_KEY_DELETED))
    {
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    return Status;
}

static
NTSTATUS
PiPropCopyOut(
    _In_ DEVPROPTYPE SourceType,
    _In_reads_bytes_opt_(SourceSize) PVOID Source,
    _In_ ULONG SourceSize,
    _Out_ PDEVPROPTYPE Type,
    _Out_writes_bytes_to_opt_(Size, *RequiredSize) PVOID Data,
    _In_ ULONG Size,
    _Out_ PULONG RequiredSize)
{
    *Type = SourceType;
    *RequiredSize = SourceSize;

    if (Size < SourceSize)
        return STATUS_BUFFER_TOO_SMALL;

    if (SourceSize)
        RtlCopyMemory(Data, Source, SourceSize);

    return STATUS_SUCCESS;
}

/* \??\<Device>#{ClassGuid}[\RefString] */
static
BOOLEAN
PiPropParseInterfaceName(
    _In_ PCUNICODE_STRING LinkName,
    _Out_ LPGUID ClassGuid)
{
    static const UNICODE_STRING KernelPrefix = RTL_CONSTANT_STRING(L"\\??\\");
    static const UNICODE_STRING UserPrefix = RTL_CONSTANT_STRING(L"\\\\?\\");
    UNICODE_STRING GuidString;
    USHORT Index, Chars;

    if (LinkName->Length % sizeof(WCHAR))
        return FALSE;

    if (!RtlPrefixUnicodeString(&KernelPrefix, LinkName, FALSE) &&
        !RtlPrefixUnicodeString(&UserPrefix, LinkName, FALSE))
    {
        return FALSE;
    }

    Chars = LinkName->Length / sizeof(WCHAR);
    Index = 4;
    while ((Index < Chars) && (LinkName->Buffer[Index] != L'\\'))
        Index++;

    if (Index < 4 + 1 + 38)
        return FALSE;

    GuidString.Buffer = &LinkName->Buffer[Index - 38];
    GuidString.Length = GuidString.MaximumLength = 38 * sizeof(WCHAR);

    return NT_SUCCESS(RtlGUIDFromString(&GuidString, ClassGuid));
}

/* Property store ************************************************************/

static
NTSTATUS
PiPropOpenObjectKey(
    _In_ PPIP_PROP_OBJECT Object,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE ObjectKey)
{
    UNICODE_STRING EnumKeyName =
        RTL_CONSTANT_STRING(L"\\Registry\\Machine\\System\\CurrentControlSet\\Enum");
    HANDLE EnumKey;
    NTSTATUS Status;

    *ObjectKey = NULL;

    switch (Object->Kind)
    {
        case PipPropObjectInterface:
            return IopOpenDeviceInterfaceKeys(Object->Name, DesiredAccess, NULL, ObjectKey);

        default:
            break;
    }

    Status = IopOpenRegistryKeyEx(&EnumKey, NULL, &EnumKeyName, KEY_READ);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = IopOpenRegistryKeyEx(ObjectKey,
                                  EnumKey,
                                  (PUNICODE_STRING)Object->Name,
                                  DesiredAccess);
    ZwClose(EnumKey);
    return Status;
}

static
VOID
PiPropBuildValuePath(
    _In_ PPIP_PROP_NAME Name,
    _Out_ PUNICODE_STRING Path,
    _Out_writes_(PIP_PROP_PATH_CHARS) PWCHAR Buffer)
{
    RtlInitEmptyUnicodeString(Path, Buffer, PIP_PROP_PATH_CHARS * sizeof(WCHAR));
    RtlAppendUnicodeToString(Path, REGSTR_KEY_DEVICE_PROPERTIES L"\\");
    RtlAppendUnicodeStringToString(Path, &Name->Fmtid);
    RtlAppendUnicodeToString(Path, L"\\");
    RtlAppendUnicodeStringToString(Path, &Name->Pid);
}

static
NTSTATUS
PiPropStoreQuery(
    _In_ HANDLE ObjectKey,
    _In_ PPIP_PROP_NAME Name,
    _Out_ PDEVPROPTYPE Type,
    _Out_writes_bytes_to_opt_(Size, *RequiredSize) PVOID Data,
    _In_ ULONG Size,
    _Out_ PULONG RequiredSize)
{
    struct
    {
        KEY_VALUE_PARTIAL_INFORMATION Info;
        UCHAR Inline[64];
    } Small;
    PKEY_VALUE_PARTIAL_INFORMATION Info = &Small.Info;
    WCHAR PathBuffer[PIP_PROP_PATH_CHARS];
    UNICODE_STRING Path;
    HANDLE PropertyKey;
    ULONG InfoSize;
    NTSTATUS Status;

    PiPropBuildValuePath(Name, &Path, PathBuffer);

    Status = IopOpenRegistryKeyEx(&PropertyKey, ObjectKey, &Path, KEY_QUERY_VALUE);
    if (!NT_SUCCESS(Status))
        return Status;

    InfoSize = sizeof(Small);
    for (;;)
    {
        Status = ZwQueryValueKey(PropertyKey,
                                 &Name->Locale,
                                 KeyValuePartialInformation,
                                 Info,
                                 InfoSize,
                                 &InfoSize);
        if ((Status != STATUS_BUFFER_OVERFLOW) && (Status != STATUS_BUFFER_TOO_SMALL))
            break;

        if (Info != &Small.Info)
            ExFreePoolWithTag(Info, TAG_PNP_PROPERTY);

        Info = ExAllocatePoolWithTag(PagedPool, InfoSize, TAG_PNP_PROPERTY);
        if (!Info)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }
    }
    ZwClose(PropertyKey);

    if (NT_SUCCESS(Status))
    {
        if ((Info->Type & PIP_PROP_REG_TYPE_MARK) == PIP_PROP_REG_TYPE_MARK)
        {
            Status = PiPropCopyOut(Info->Type & ~PIP_PROP_REG_TYPE_MARK,
                                   Info->Data,
                                   Info->DataLength,
                                   Type,
                                   Data,
                                   Size,
                                   RequiredSize);
        }
        else
        {
            DPRINT1("Unexpected value type 0x%lx for %wZ\\%wZ\n",
                    Info->Type, &Name->Fmtid, &Name->Pid);
            Status = STATUS_OBJECT_NAME_NOT_FOUND;
        }
    }

    if (Info && (Info != &Small.Info))
        ExFreePoolWithTag(Info, TAG_PNP_PROPERTY);

    return Status;
}

static
NTSTATUS
PiPropPutValue(
    _In_ HANDLE ValueKey,
    _In_ PPIP_PROP_NAME Name,
    _In_ DEVPROPTYPE Type,
    _In_reads_bytes_opt_(Size) PVOID Data,
    _In_ ULONG Size)
{
    return ZwSetValueKey(ValueKey,
                         &Name->Locale,
                         0,
                         PIP_PROP_REG_TYPE_MARK | Type,
                         Data,
                         Size);
}

static
NTSTATUS
PiPropCreateBranch(
    _In_ HANDLE Parent,
    _In_reads_(Depth) PCUNICODE_STRING *Parts,
    _In_ ULONG Depth,
    _In_ PPIP_PROP_NAME Name,
    _In_ DEVPROPTYPE Type,
    _In_reads_bytes_opt_(Size) PVOID Data,
    _In_ ULONG Size)
{
    ACCESS_MASK Access = KEY_CREATE_SUB_KEY | KEY_SET_VALUE | DELETE;
    ULONG Disposition;
    HANDLE Child;
    NTSTATUS Status;

    Status = IopCreateRegistryKeyEx(&Child, Parent, (PUNICODE_STRING)Parts[0], Access,
                                    REG_OPTION_NON_VOLATILE, &Disposition);
    if (Status == STATUS_CHILD_MUST_BE_VOLATILE)
    {
        Status = IopCreateRegistryKeyEx(&Child, Parent, (PUNICODE_STRING)Parts[0], Access,
                                        REG_OPTION_VOLATILE, &Disposition);
    }
    if (!NT_SUCCESS(Status))
        return Status;

    if (Depth > 1)
        Status = PiPropCreateBranch(Child, Parts + 1, Depth - 1, Name, Type, Data, Size);
    else
        Status = PiPropPutValue(Child, Name, Type, Data, Size);

    if (!NT_SUCCESS(Status) && (Disposition == REG_CREATED_NEW_KEY))
        ZwDeleteKey(Child);

    ZwClose(Child);
    return Status;
}

static
NTSTATUS
PiPropStoreWrite(
    _In_ HANDLE ObjectKey,
    _In_ PPIP_PROP_NAME Name,
    _In_ DEVPROPTYPE Type,
    _In_reads_bytes_opt_(Size) PVOID Data,
    _In_ ULONG Size)
{
    UNICODE_STRING PropertiesName = RTL_CONSTANT_STRING(REGSTR_KEY_DEVICE_PROPERTIES);
    PCUNICODE_STRING Parts[] = { &PropertiesName, &Name->Fmtid, &Name->Pid };
    WCHAR PathBuffer[PIP_PROP_PATH_CHARS];
    UNICODE_STRING Path;
    HANDLE ValueKey;
    NTSTATUS Status;

    PiPropBuildValuePath(Name, &Path, PathBuffer);

    Status = IopOpenRegistryKeyEx(&ValueKey, ObjectKey, &Path, KEY_SET_VALUE);
    if (!NT_SUCCESS(Status))
        return PiPropCreateBranch(ObjectKey, Parts, RTL_NUMBER_OF(Parts), Name, Type, Data, Size);

    Status = PiPropPutValue(ValueKey, Name, Type, Data, Size);
    ZwClose(ValueKey);
    return Status;
}

static
BOOLEAN
PiPropIsKeyEmpty(
    _In_ HANDLE KeyHandle)
{
    struct
    {
        KEY_FULL_INFORMATION Info;
        WCHAR ClassName[32];
    } Buffer;
    ULONG ResultLength;
    NTSTATUS Status;

    Status = ZwQueryKey(KeyHandle,
                        KeyFullInformation,
                        &Buffer,
                        sizeof(Buffer),
                        &ResultLength);

    return NT_SUCCESS(Status) &&
           (Buffer.Info.SubKeys == 0) &&
           (Buffer.Info.Values == 0);
}

static
NTSTATUS
PiPropStoreDelete(
    _In_ HANDLE ObjectKey,
    _In_ PPIP_PROP_NAME Name)
{
    UNICODE_STRING PropertiesName = RTL_CONSTANT_STRING(REGSTR_KEY_DEVICE_PROPERTIES);
    PUNICODE_STRING Chain[] = { &PropertiesName, &Name->Fmtid, &Name->Pid };
    HANDLE Keys[RTL_NUMBER_OF(Chain) + 1];
    ULONG Depth;
    NTSTATUS Status = STATUS_SUCCESS;

    Keys[0] = ObjectKey;
    for (Depth = 0; Depth < RTL_NUMBER_OF(Chain); Depth++)
    {
        Status = IopOpenRegistryKeyEx(&Keys[Depth + 1],
                                      Keys[Depth],
                                      Chain[Depth],
                                      KEY_QUERY_VALUE | KEY_SET_VALUE | DELETE);
        if (!NT_SUCCESS(Status))
            break;
    }

    if (NT_SUCCESS(Status))
        Status = ZwDeleteValueKey(Keys[Depth], &Name->Locale);

    for (; Depth > 0; Depth--)
    {
        if (NT_SUCCESS(Status) && PiPropIsKeyEmpty(Keys[Depth]))
            ZwDeleteKey(Keys[Depth]);

        ZwClose(Keys[Depth]);
    }

    return Status;
}

/* Saved interrupt data ******************************************************/

typedef struct _PIP_PROP_SAVED_VALUE
{
    LIST_ENTRY Link;
    PDEVICE_OBJECT Pdo;
    DEVPROPTYPE Type;
    ULONG Size;
    UCHAR Data[ANYSIZE_ARRAY];
} PIP_PROP_SAVED_VALUE, *PPIP_PROP_SAVED_VALUE;

static LIST_ENTRY PiPropSavedValues = { &PiPropSavedValues, &PiPropSavedValues };
static EX_PUSH_LOCK PiPropSavedLock;

/* Caller holds PiPropSavedLock */
static
PPIP_PROP_SAVED_VALUE
PiPropFindSaved(
    _In_ PDEVICE_OBJECT Pdo)
{
    PPIP_PROP_SAVED_VALUE Saved;
    PLIST_ENTRY Entry;

    for (Entry = PiPropSavedValues.Flink; Entry != &PiPropSavedValues; Entry = Entry->Flink)
    {
        Saved = CONTAINING_RECORD(Entry, PIP_PROP_SAVED_VALUE, Link);
        if (Saved->Pdo == Pdo)
            return Saved;
    }

    return NULL;
}

/* DEVPROP_TYPE_EMPTY removes the value */
static
NTSTATUS
PiPropReplaceSaved(
    _In_ PDEVICE_OBJECT Pdo,
    _In_ DEVPROPTYPE Type,
    _In_reads_bytes_opt_(Size) PVOID Data,
    _In_ ULONG Size,
    _Out_opt_ PBOOLEAN Replaced)
{
    PPIP_PROP_SAVED_VALUE Fresh = NULL, Stale;

    if (Type != DEVPROP_TYPE_EMPTY)
    {
        Fresh = ExAllocatePoolWithTag(PagedPool,
                                      FIELD_OFFSET(PIP_PROP_SAVED_VALUE, Data) + Size,
                                      TAG_PNP_PROPERTY);
        if (!Fresh)
            return STATUS_INSUFFICIENT_RESOURCES;

        Fresh->Pdo = Pdo;
        Fresh->Type = Type;
        Fresh->Size = Size;
        if (Size)
            RtlCopyMemory(Fresh->Data, Data, Size);
    }

    KeEnterCriticalRegion();
    ExAcquirePushLockExclusive(&PiPropSavedLock);

    Stale = PiPropFindSaved(Pdo);
    if (Stale)
        RemoveEntryList(&Stale->Link);
    if (Fresh)
        InsertTailList(&PiPropSavedValues, &Fresh->Link);

    ExReleasePushLockExclusive(&PiPropSavedLock);
    KeLeaveCriticalRegion();

    if (Replaced)
        *Replaced = (Stale != NULL);

    if (Stale)
        ExFreePoolWithTag(Stale, TAG_PNP_PROPERTY);

    return STATUS_SUCCESS;
}

/* Builtin properties ********************************************************/

static
const PIP_BUILTIN_PROPERTY *
PiPropBuiltinTable(
    _In_ PIP_PROP_OBJECT_KIND Kind,
    _Out_ PULONG Count)
{
    switch (Kind)
    {
        case PipPropObjectDevice:
            *Count = RTL_NUMBER_OF(PipDeviceBuiltins);
            return PipDeviceBuiltins;

        case PipPropObjectInterface:
            *Count = RTL_NUMBER_OF(PipInterfaceBuiltins);
            return PipInterfaceBuiltins;

        default:
            *Count = 0;
            return NULL;
    }
}

static
const PIP_BUILTIN_PROPERTY *
PiPropFindBuiltin(
    _In_ PPIP_PROP_OBJECT Object,
    _In_ const DEVPROPKEY *Key)
{
    const PIP_BUILTIN_PROPERTY *Table;
    ULONG Count;

    Table = PiPropBuiltinTable(Object->Kind, &Count);
    for (; Count; Count--, Table++)
    {
        if (PiPropIsSameKey(Table->Key, Key))
            return Table;
    }

    return NULL;
}

static
NTSTATUS
PiPropAllocateBlob(
    _In_reads_bytes_opt_(Size) PVOID Source,
    _In_ ULONG Size,
    _Out_ PVOID *Blob)
{
    *Blob = ExAllocatePoolWithTag(PagedPool, max(Size, 1), TAG_PNP_PROPERTY);
    if (!*Blob)
        return STATUS_INSUFFICIENT_RESOURCES;

    if (Source && Size)
        RtlCopyMemory(*Blob, Source, Size);

    return STATUS_SUCCESS;
}

static
NTSTATUS
PiPropCaptureString(
    _In_reads_bytes_(RawSize) PCWSTR Raw,
    _In_ ULONG RawSize,
    _In_ BOOLEAN IsList,
    _Out_ PVOID *Blob,
    _Out_ PULONG BlobSize)
{
    ULONG RawChars = RawSize / sizeof(WCHAR);
    ULONG Index, Out = 0;
    BOOLEAN InEntry = FALSE;
    PWCHAR String;
    NTSTATUS Status;

    Status = PiPropAllocateBlob(NULL, (RawChars + 2) * sizeof(WCHAR), Blob);
    if (!NT_SUCCESS(Status))
        return Status;

    String = *Blob;

    for (Index = 0; Index < RawChars; Index++)
    {
        if (Raw[Index] != UNICODE_NULL)
        {
            String[Out++] = Raw[Index];
            InEntry = TRUE;
            continue;
        }

        if (!IsList || !InEntry)
            break;

        String[Out++] = UNICODE_NULL;
        InEntry = FALSE;
    }

    if (!IsList || InEntry)
        String[Out++] = UNICODE_NULL;
    if (IsList)
        String[Out++] = UNICODE_NULL;

    *BlobSize = Out * sizeof(WCHAR);
    return STATUS_SUCCESS;
}

static
NTSTATUS
PiPropReadRegistryValue(
    _In_ HANDLE KeyHandle,
    _In_ PCWSTR ValueName,
    _In_ ULONG RegType,
    _In_ DEVPROPTYPE Type,
    _Out_ PVOID *Blob,
    _Out_ PULONG BlobSize)
{
    PKEY_VALUE_FULL_INFORMATION Info;
    PVOID RawData;
    ULONG RawSize;
    NTSTATUS Status;

    Status = IopGetRegistryValue(KeyHandle, (PWSTR)ValueName, &Info);
    if (!NT_SUCCESS(Status))
        return Status;

    RawData = (PUCHAR)Info + Info->DataOffset;
    RawSize = Info->DataLength;

    if ((Info->Type != RegType) &&
        !((RegType == REG_SZ) && (Info->Type == REG_EXPAND_SZ)))
    {
        DPRINT1("Value '%S' has type %lu, expected %lu\n", ValueName, Info->Type, RegType);
        ExFreePool(Info);
        return STATUS_OBJECT_TYPE_MISMATCH;
    }

    switch (Type)
    {
        case DEVPROP_TYPE_STRING:
        case DEVPROP_TYPE_STRING_LIST:
            Status = PiPropCaptureString(RawData,
                                         RawSize,
                                         (Type == DEVPROP_TYPE_STRING_LIST),
                                         Blob,
                                         BlobSize);
            break;

        case DEVPROP_TYPE_UINT32:
            if (RawSize != sizeof(ULONG))
            {
                Status = STATUS_OBJECT_TYPE_MISMATCH;
                break;
            }
            Status = PiPropAllocateBlob(RawData, RawSize, Blob);
            *BlobSize = RawSize;
            break;

        case DEVPROP_TYPE_GUID:
        {
            UNICODE_STRING GuidString;
            GUID Guid;

            Status = PiPropCaptureString(RawData, RawSize, FALSE, Blob, BlobSize);
            if (!NT_SUCCESS(Status))
                break;

            RtlInitUnicodeString(&GuidString, *Blob);
            Status = RtlGUIDFromString(&GuidString, &Guid);
            if (NT_SUCCESS(Status))
            {
                RtlCopyMemory(*Blob, &Guid, sizeof(Guid));
                *BlobSize = sizeof(Guid);
            }
            else
            {
                ExFreePoolWithTag(*Blob, TAG_PNP_PROPERTY);
                Status = STATUS_OBJECT_TYPE_MISMATCH;
            }
            break;
        }

        default:
            ASSERT(FALSE);
            Status = STATUS_INTERNAL_ERROR;
            break;
    }

    ExFreePool(Info);
    return Status;
}

static
NTSTATUS
PiPropReadKeyValue(
    _In_ PPIP_PROP_OBJECT Object,
    _In_ PCWSTR ValueName,
    _In_ ULONG RegType,
    _In_ DEVPROPTYPE Type,
    _Out_ PVOID *Blob,
    _Out_ PULONG BlobSize)
{
    HANDLE ObjectKey;
    NTSTATUS Status;

    Status = PiPropOpenObjectKey(Object, KEY_QUERY_VALUE, &ObjectKey);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = PiPropReadRegistryValue(ObjectKey, ValueName, RegType, Type, Blob, BlobSize);
    ZwClose(ObjectKey);
    return Status;
}

static
NTSTATUS
PiPropReadDeviceRegistryProperty(
    _In_ PPIP_PROP_OBJECT Object,
    _In_ const PIP_BUILTIN_PROPERTY *Builtin,
    _Out_ PVOID *Blob,
    _Out_ PULONG BlobSize)
{
    ULONG Length = 128, Needed;
    PVOID Buffer;
    NTSTATUS Status;

    if (!Object->Pdo)
        return STATUS_OBJECT_NAME_NOT_FOUND;

    for (;;)
    {
        Buffer = ExAllocatePoolWithTag(PagedPool, Length, TAG_PNP_PROPERTY);
        if (!Buffer)
            return STATUS_INSUFFICIENT_RESOURCES;

        Status = IoGetDeviceProperty(Object->Pdo,
                                     (DEVICE_REGISTRY_PROPERTY)Builtin->Selector,
                                     Length,
                                     Buffer,
                                     &Needed);
        if (Status != STATUS_BUFFER_TOO_SMALL)
            break;

        ExFreePoolWithTag(Buffer, TAG_PNP_PROPERTY);

        Length = ALIGN_UP_BY(max(Needed, Length * 2), sizeof(WCHAR));
        if (Length > PIP_PROP_MAX_QUERY_SIZE)
            return STATUS_UNSUCCESSFUL;
    }

    if (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(Buffer, TAG_PNP_PROPERTY);
        return Status;
    }

    if (Builtin->Type == DEVPROP_TYPE_STRING)
    {
        Status = PiPropCaptureString(Buffer, Needed, FALSE, Blob, BlobSize);
        ExFreePoolWithTag(Buffer, TAG_PNP_PROPERTY);
        return Status;
    }

    if (Needed != PiPropTypeRules[Builtin->Type].Width)
    {
        ExFreePoolWithTag(Buffer, TAG_PNP_PROPERTY);
        return STATUS_OBJECT_TYPE_MISMATCH;
    }

    *Blob = Buffer;
    *BlobSize = Needed;
    return STATUS_SUCCESS;
}

static
NTSTATUS
PiPropReadDeviceNodeField(
    _In_ PPIP_PROP_OBJECT Object,
    _In_ PIP_PROP_FIELD Field,
    _Out_ PVOID *Blob,
    _Out_ PULONG BlobSize)
{
    PDEVICE_NODE DeviceNode = Object->DeviceNode;
    DEVPROP_BOOLEAN Boolean;
    ULONG Value;
    NTSTATUS Status;

    switch (Field)
    {
        case PipFieldName:
            Status = PiPropReadKeyValue(Object, REGSTR_VAL_FRIENDLYNAME, REG_SZ,
                                        DEVPROP_TYPE_STRING, Blob, BlobSize);
            if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
            {
                Status = PiPropReadKeyValue(Object, REGSTR_VAL_DEVDESC, REG_SZ,
                                            DEVPROP_TYPE_STRING, Blob, BlobSize);
            }
            return Status;

        case PipFieldInstanceId:
            return PiPropCaptureString(Object->Name->Buffer,
                                       Object->Name->Length,
                                       FALSE,
                                       Blob,
                                       BlobSize);

        case PipFieldIsPresent:
            Boolean = (DeviceNode && !(DeviceNode->Flags & DNF_DEVICE_GONE)) ? DEVPROP_TRUE : DEVPROP_FALSE;
            *BlobSize = sizeof(Boolean);
            return PiPropAllocateBlob(&Boolean, sizeof(Boolean), Blob);

        default:
            break;
    }

    if (!DeviceNode)
        return STATUS_OBJECT_NAME_NOT_FOUND;

    switch (Field)
    {
        case PipFieldParent:
            if (!DeviceNode->Parent)
                return STATUS_OBJECT_NAME_NOT_FOUND;

            return PiPropCaptureString(DeviceNode->Parent->InstancePath.Buffer,
                                       DeviceNode->Parent->InstancePath.Length,
                                       FALSE,
                                       Blob,
                                       BlobSize);

        case PipFieldStatus:
            Value = IopGetDeviceNodeStatus(DeviceNode);
            break;

        case PipFieldProblemCode:
            Value = DeviceNode->Problem;
            break;

        case PipFieldHasProblem:
            Boolean = (DeviceNode->Flags & DNF_HAS_PROBLEM) ? DEVPROP_TRUE : DEVPROP_FALSE;
            *BlobSize = sizeof(Boolean);
            return PiPropAllocateBlob(&Boolean, sizeof(Boolean), Blob);

        default:
            ASSERT(FALSE);
            return STATUS_INTERNAL_ERROR;
    }

    *BlobSize = sizeof(Value);
    return PiPropAllocateBlob(&Value, sizeof(Value), Blob);
}

static
NTSTATUS
PiPropReadInterfaceField(
    _In_ PPIP_PROP_OBJECT Object,
    _In_ PIP_PROP_FIELD Field,
    _Out_ PVOID *Blob,
    _Out_ PULONG BlobSize)
{
    UNICODE_STRING ControlName = RTL_CONSTANT_STRING(L"Control");
    HANDLE DeviceKey, InstanceKey, ControlKey;
    PKEY_VALUE_FULL_INFORMATION Info;
    DEVPROP_BOOLEAN Enabled;
    NTSTATUS Status;

    Status = IopOpenDeviceInterfaceKeys(Object->Name, KEY_READ, &DeviceKey, &InstanceKey);
    if (!NT_SUCCESS(Status))
        return Status;

    switch (Field)
    {
        case PipFieldInterfaceClass:
            *BlobSize = sizeof(GUID);
            Status = PiPropAllocateBlob(&Object->ClassGuid, sizeof(GUID), Blob);
            break;

        case PipFieldInterfaceEnabled:
            Enabled = DEVPROP_FALSE;
            if (NT_SUCCESS(IopOpenRegistryKeyEx(&ControlKey, InstanceKey, &ControlName, KEY_QUERY_VALUE)))
            {
                if (NT_SUCCESS(IopGetRegistryValue(ControlKey, L"Linked", &Info)))
                {
                    if ((Info->Type == REG_DWORD) &&
                        (Info->DataLength == sizeof(ULONG)) &&
                        (*(PULONG)((PUCHAR)Info + Info->DataOffset) != 0))
                    {
                        Enabled = DEVPROP_TRUE;
                    }
                    ExFreePool(Info);
                }
                ZwClose(ControlKey);
            }

            *BlobSize = sizeof(Enabled);
            Status = PiPropAllocateBlob(&Enabled, sizeof(Enabled), Blob);
            break;

        case PipFieldInterfaceDevice:
            Status = PiPropReadRegistryValue(DeviceKey, L"DeviceInstance", REG_SZ,
                                             DEVPROP_TYPE_STRING, Blob, BlobSize);
            break;

        default:
            ASSERT(FALSE);
            Status = STATUS_INTERNAL_ERROR;
            break;
    }

    ZwClose(InstanceKey);
    ZwClose(DeviceKey);
    return Status;
}

static
NTSTATUS
PiPropReadBuiltin(
    _In_ PPIP_PROP_OBJECT Object,
    _In_ const PIP_BUILTIN_PROPERTY *Builtin,
    _Out_ PVOID *Blob,
    _Out_ PULONG BlobSize)
{
    switch (Builtin->Source)
    {
        case PipSourceKeyValue:
            return PiPropReadKeyValue(Object,
                                      Builtin->ValueName,
                                      Builtin->Selector,
                                      Builtin->Type,
                                      Blob,
                                      BlobSize);

        case PipSourceRegistryProperty:
            return PiPropReadDeviceRegistryProperty(Object, Builtin, Blob, BlobSize);

        case PipSourceDeviceNode:
            return PiPropReadDeviceNodeField(Object, Builtin->Selector, Blob, BlobSize);

        case PipSourceInterface:
            return PiPropReadInterfaceField(Object, Builtin->Selector, Blob, BlobSize);

        default:
            ASSERT(FALSE);
            return STATUS_INTERNAL_ERROR;
    }
}

static
NTSTATUS
PiPropWriteBuiltin(
    _In_ PPIP_PROP_OBJECT Object,
    _In_ const PIP_BUILTIN_PROPERTY *Builtin,
    _In_ DEVPROPTYPE Type,
    _In_reads_bytes_opt_(Size) PVOID Data,
    _In_ ULONG Size)
{
    static const UNICODE_STRING RootEnumerator = RTL_CONSTANT_STRING(L"ROOT\\");
    UNICODE_STRING ValueName;
    HANDLE ObjectKey;
    NTSTATUS Status;

    if ((Builtin->Flags & PIP_PROP_READ_ONLY) ||
        (Builtin->Source != PipSourceKeyValue))
    {
        return STATUS_ACCESS_DENIED;
    }

    if ((Builtin->Flags & PIP_PROP_ROOT_ENUM_WRITE) &&
        !RtlPrefixUnicodeString(&RootEnumerator, Object->Name, TRUE))
    {
        return STATUS_ACCESS_DENIED;
    }

    if ((Type != DEVPROP_TYPE_EMPTY) && (Type != Builtin->Type))
        return STATUS_INVALID_PARAMETER;

    Status = PiPropOpenObjectKey(Object, KEY_SET_VALUE, &ObjectKey);
    if (!NT_SUCCESS(Status))
        return Status;

    RtlInitUnicodeString(&ValueName, Builtin->ValueName);

    if (Type == DEVPROP_TYPE_EMPTY)
    {
        Status = ZwDeleteValueKey(ObjectKey, &ValueName);
    }
    else if (Type == DEVPROP_TYPE_GUID)
    {
        UNICODE_STRING GuidString;
        GUID Guid;

        RtlCopyMemory(&Guid, Data, sizeof(Guid));
        Status = RtlStringFromGUID(&Guid, &GuidString);
        if (NT_SUCCESS(Status))
        {
            Status = ZwSetValueKey(ObjectKey,
                                   &ValueName,
                                   0,
                                   REG_SZ,
                                   GuidString.Buffer,
                                   GuidString.Length + sizeof(UNICODE_NULL));
            RtlFreeUnicodeString(&GuidString);
        }
    }
    else
    {
        Status = ZwSetValueKey(ObjectKey, &ValueName, 0, Builtin->Selector, Data, Size);
    }

    ZwClose(ObjectKey);
    return Status;
}

/* Reading *******************************************************************/

typedef struct _PIP_PROP_OUTPUT
{
    PVOID Data;
    ULONG Size;
    ULONG Required;
    DEVPROPTYPE Type;
} PIP_PROP_OUTPUT, *PPIP_PROP_OUTPUT;

/* Returns TRUE when the source has the final answer in *Status */
typedef BOOLEAN
(*PIP_PROP_READER)(
    _In_ PPIP_PROP_OBJECT Object,
    _In_ PPIP_PROP_NAME Name,
    _Inout_ PPIP_PROP_OUTPUT Out,
    _Out_ PNTSTATUS Status);

static
BOOLEAN
PiPropReadFromBuiltin(
    _In_ PPIP_PROP_OBJECT Object,
    _In_ PPIP_PROP_NAME Name,
    _Inout_ PPIP_PROP_OUTPUT Out,
    _Out_ PNTSTATUS Status)
{
    const PIP_BUILTIN_PROPERTY *Builtin;
    ULONG BlobSize;
    PVOID Blob;

    if (Name->Locale.Length)
        return FALSE;

    Builtin = PiPropFindBuiltin(Object, Name->Key);
    if (!Builtin)
        return FALSE;

    *Status = PiPropReadBuiltin(Object, Builtin, &Blob, &BlobSize);
    if (NT_SUCCESS(*Status))
    {
        *Status = PiPropCopyOut(Builtin->Type, Blob, BlobSize,
                                &Out->Type, Out->Data, Out->Size, &Out->Required);
        ExFreePoolWithTag(Blob, TAG_PNP_PROPERTY);
    }

    return TRUE;
}

static
BOOLEAN
PiPropReadFromStore(
    _In_ PPIP_PROP_OBJECT Object,
    _In_ PPIP_PROP_NAME Name,
    _Inout_ PPIP_PROP_OUTPUT Out,
    _Out_ PNTSTATUS Status)
{
    HANDLE ObjectKey;

    KeEnterCriticalRegion();
    ExAcquireResourceSharedLite(&PpRegistryDeviceResource, TRUE);

    *Status = PiPropOpenObjectKey(Object, KEY_READ, &ObjectKey);
    if (NT_SUCCESS(*Status))
    {
        *Status = PiPropStoreQuery(ObjectKey, Name, &Out->Type,
                                   Out->Data, Out->Size, &Out->Required);
        ZwClose(ObjectKey);
    }

    ExReleaseResourceLite(&PpRegistryDeviceResource);
    KeLeaveCriticalRegion();

    return (PiPropNormalizeStatus(*Status) != STATUS_OBJECT_NAME_NOT_FOUND);
}

static
BOOLEAN
PiPropReadFromSaved(
    _In_ PPIP_PROP_OBJECT Object,
    _In_ PPIP_PROP_NAME Name,
    _Inout_ PPIP_PROP_OUTPUT Out,
    _Out_ PNTSTATUS Status)
{
    PPIP_PROP_SAVED_VALUE Saved;

    if (!PiPropIsInterruptKey(Object, Name))
        return FALSE;

    KeEnterCriticalRegion();
    ExAcquirePushLockShared(&PiPropSavedLock);

    Saved = PiPropFindSaved(Object->Pdo);
    if (Saved)
    {
        *Status = PiPropCopyOut(Saved->Type, Saved->Data, Saved->Size,
                                &Out->Type, Out->Data, Out->Size, &Out->Required);
    }

    ExReleasePushLockShared(&PiPropSavedLock);
    KeLeaveCriticalRegion();

    return (Saved != NULL);
}

static const PIP_PROP_READER PiPropReaders[] =
{
    PiPropReadFromBuiltin,
    PiPropReadFromStore,
    PiPropReadFromSaved
};

static
NTSTATUS
PiPropGet(
    _In_ PPIP_PROP_OBJECT Object,
    _In_ const DEVPROPKEY *PropertyKey,
    _In_ LCID Lcid,
    _In_ ULONG Size,
    _Out_writes_bytes_to_opt_(Size, *RequiredSize) PVOID Data,
    _Out_ PULONG RequiredSize,
    _Out_ PDEVPROPTYPE Type)
{
    PIP_PROP_OUTPUT Out = { Data, Data ? Size : 0, 0, DEVPROP_TYPE_EMPTY };
    PIP_PROP_NAME Name;
    NTSTATUS Status;
    ULONG Index;

    PAGED_CODE();

    if (!PropertyKey || !RequiredSize || !Type)
        return STATUS_INVALID_PARAMETER;

    Status = PiPropInitName(&Name, PropertyKey, Lcid);
    if (NT_SUCCESS(Status))
    {
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
        for (Index = 0; Index < RTL_NUMBER_OF(PiPropReaders); Index++)
        {
            if (PiPropReaders[Index](Object, &Name, &Out, &Status))
                break;
        }
    }

    *RequiredSize = Out.Required;
    *Type = Out.Type;
    return PiPropNormalizeStatus(Status);
}

/* Writing *******************************************************************/

typedef enum _PIP_PROP_TARGET
{
    PipTargetDenied,
    PipTargetBuiltin,
    PipTargetStore
} PIP_PROP_TARGET;

static
PIP_PROP_TARGET
PiPropPickTarget(
    _In_ PPIP_PROP_OBJECT Object,
    _In_ PPIP_PROP_NAME Name,
    _Out_ const PIP_BUILTIN_PROPERTY **Builtin)
{
    *Builtin = NULL;

    /* Localized values are still allowed on a protected object */
    if ((Object->Flags & PIP_PROP_OBJECT_PROTECTED) && !Name->Locale.Length)
        return PipTargetDenied;

    if (!Name->Locale.Length)
        *Builtin = PiPropFindBuiltin(Object, Name->Key);

    return *Builtin ? PipTargetBuiltin : PipTargetStore;
}

static
NTSTATUS
PiPropStoreApply(
    _In_ PPIP_PROP_OBJECT Object,
    _In_ PPIP_PROP_NAME Name,
    _In_ DEVPROPTYPE Type,
    _In_reads_bytes_opt_(Size) PVOID Data,
    _In_ ULONG Size)
{
    BOOLEAN Replaced;
    HANDLE ObjectKey;
    NTSTATUS Status;

    Status = PiPropOpenObjectKey(Object, KEY_READ | KEY_CREATE_SUB_KEY, &ObjectKey);
    if (NT_SUCCESS(Status))
    {
        if (Type == DEVPROP_TYPE_EMPTY)
            Status = PiPropStoreDelete(ObjectKey, Name);
        else
            Status = PiPropStoreWrite(ObjectKey, Name, Type, Data, Size);

        ZwClose(ObjectKey);
    }

    if (!PiPropIsInterruptKey(Object, Name))
        return Status;

    if (NT_SUCCESS(Status))
        return PiPropReplaceSaved(Object->Pdo, Type, Data, Size, NULL);

    /* Deleting a value that only exists in the saved copy still succeeds */
    if ((Type == DEVPROP_TYPE_EMPTY) &&
        (PiPropNormalizeStatus(Status) == STATUS_OBJECT_NAME_NOT_FOUND))
    {
        PiPropReplaceSaved(Object->Pdo, Type, NULL, 0, &Replaced);
        if (Replaced)
            Status = STATUS_SUCCESS;
    }

    return Status;
}

static
BOOLEAN
PiPropShouldAnnounce(
    _In_ PDEVICE_NODE DeviceNode)
{
    switch (DeviceNode->State)
    {
        case DeviceNodeUnspecified:
        case DeviceNodeUninitialized:
        case DeviceNodeInitialized:
            return FALSE;

        default:
            return TRUE;
    }
}

static
NTSTATUS
PiPropSet(
    _In_ PPIP_PROP_OBJECT Object,
    _In_ const DEVPROPKEY *PropertyKey,
    _In_ LCID Lcid,
    _In_ DEVPROPTYPE Type,
    _In_ ULONG Size,
    _In_reads_bytes_opt_(Size) PVOID Data)
{
    const PIP_BUILTIN_PROPERTY *Builtin;
    PIP_PROP_TARGET Target;
    PIP_PROP_NAME Name;
    NTSTATUS Status;

    PAGED_CODE();

    if (!PropertyKey)
        return STATUS_INVALID_PARAMETER;

    Status = PiPropCheckPayload(Type, Data, Size);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = PiPropInitName(&Name, PropertyKey, Lcid);
    if (!NT_SUCCESS(Status))
        return Status;

    if ((Type == DEVPROP_TYPE_STRING_INDIRECT) && Name.Locale.Length)
        return STATUS_INVALID_PARAMETER;

    Target = PiPropPickTarget(Object, &Name, &Builtin);
    if (Target == PipTargetDenied)
        return STATUS_ACCESS_DENIED;

    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&PpRegistryDeviceResource, TRUE);

    if (Target == PipTargetBuiltin)
        Status = PiPropWriteBuiltin(Object, Builtin, Type, Data, Size);
    else
        Status = PiPropStoreApply(Object, &Name, Type, Data, Size);

    ExReleaseResourceLite(&PpRegistryDeviceResource);
    KeLeaveCriticalRegion();

    if (NT_SUCCESS(Status) && Object->DeviceNode && PiPropShouldAnnounce(Object->DeviceNode))
        IopQueueDevicePropertyChangeEvent(&Object->DeviceNode->InstancePath);

    return PiPropNormalizeStatus(Status);
}

/* Objects *******************************************************************/

static
NTSTATUS
PiPropInitDeviceObject(
    _In_ PDEVICE_OBJECT Pdo,
    _Out_ PPIP_PROP_OBJECT Object)
{
    PDEVICE_NODE DeviceNode = Pdo ? IopGetDeviceNode(Pdo) : NULL;

    if (!DeviceNode || (DeviceNode->Flags & DNF_LEGACY_RESOURCE_DEVICENODE))
    {
        DPRINT1("%p is not a PDO\n", Pdo);
        KeBugCheckEx(PNP_DETECTED_FATAL_ERROR, 0x2, (ULONG_PTR)Pdo, 0, 0);
    }

    if (!DeviceNode->InstancePath.Length)
        return STATUS_INVALID_DEVICE_REQUEST;

    RtlZeroMemory(Object, sizeof(*Object));
    Object->Kind = PipPropObjectDevice;
    Object->Pdo = Pdo;
    Object->DeviceNode = DeviceNode;
    Object->Name = &DeviceNode->InstancePath;

    if (DeviceNode == IopRootDeviceNode)
        Object->Flags |= PIP_PROP_OBJECT_PROTECTED;

    return STATUS_SUCCESS;
}

static
NTSTATUS
PiPropInitInterfaceObject(
    _In_ PCUNICODE_STRING SymbolicLinkName,
    _Out_ PPIP_PROP_OBJECT Object)
{
    RtlZeroMemory(Object, sizeof(*Object));

    if (!SymbolicLinkName || !SymbolicLinkName->Buffer || !SymbolicLinkName->Length ||
        !PiPropParseInterfaceName(SymbolicLinkName, &Object->ClassGuid))
    {
        return STATUS_INVALID_PARAMETER;
    }

    Object->Kind = PipPropObjectInterface;
    Object->Name = SymbolicLinkName;
    return STATUS_SUCCESS;
}

CODE_SEG("PAGE")
VOID
PiPropReleaseDevice(
    _In_ PDEVICE_OBJECT DeviceObject)
{
    PAGED_CODE();

    PiPropReplaceSaved(DeviceObject, DEVPROP_TYPE_EMPTY, NULL, 0, NULL);
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
CODE_SEG("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS
NTAPI
IoSetDevicePropertyData(
    _In_ PDEVICE_OBJECT Pdo,
    _In_ CONST DEVPROPKEY *PropertyKey,
    _In_ LCID Lcid,
    _In_ ULONG Flags,
    _In_ DEVPROPTYPE Type,
    _In_ ULONG Size,
    _In_opt_ PVOID Data)
{
    PIP_PROP_OBJECT Object;
    NTSTATUS Status;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(Flags);

    Status = PiPropInitDeviceObject(Pdo, &Object);
    if (!NT_SUCCESS(Status))
        return Status;

    /* A NULL buffer deletes the property */
    if (!Data)
    {
        Type = DEVPROP_TYPE_EMPTY;
        Size = 0;
    }

    return PiPropSet(&Object, PropertyKey, Lcid, Type, Size, Data);
}

/*
 * @implemented
 */
CODE_SEG("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS
NTAPI
IoGetDevicePropertyData(
    _In_ PDEVICE_OBJECT Pdo,
    _In_ CONST DEVPROPKEY *PropertyKey,
    _In_ LCID Lcid,
    _Reserved_ ULONG Flags,
    _In_ ULONG Size,
    _Out_ PVOID Data,
    _Out_ PULONG RequiredSize,
    _Out_ PDEVPROPTYPE Type)
{
    PIP_PROP_OBJECT Object;
    NTSTATUS Status;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(Flags);

    Status = PiPropInitDeviceObject(Pdo, &Object);
    if (!NT_SUCCESS(Status))
        return Status;

    return PiPropGet(&Object, PropertyKey, Lcid, Size, Data, RequiredSize, Type);
}

/*
 * @implemented
 */
CODE_SEG("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS
NTAPI
IoSetDeviceInterfacePropertyData(
    _In_ PUNICODE_STRING SymbolicLinkName,
    _In_ CONST DEVPROPKEY *PropertyKey,
    _In_ LCID Lcid,
    _In_ ULONG Flags,
    _In_ DEVPROPTYPE Type,
    _In_ ULONG Size,
    _In_reads_bytes_opt_(Size) PVOID Data)
{
    PIP_PROP_OBJECT Object;
    NTSTATUS Status;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(Flags);

    Status = PiPropInitInterfaceObject(SymbolicLinkName, &Object);
    if (!NT_SUCCESS(Status))
        return Status;

    if (!Data)
    {
        Type = DEVPROP_TYPE_EMPTY;
        Size = 0;
    }

    return PiPropSet(&Object, PropertyKey, Lcid, Type, Size, Data);
}

/*
 * @implemented
 */
CODE_SEG("PAGE")
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS
NTAPI
IoGetDeviceInterfacePropertyData(
    _In_ PUNICODE_STRING SymbolicLinkName,
    _In_ CONST DEVPROPKEY *PropertyKey,
    _In_ LCID Lcid,
    _Reserved_ ULONG Flags,
    _In_ ULONG Size,
    _Out_writes_bytes_to_(Size, *RequiredSize) PVOID Data,
    _Out_ PULONG RequiredSize,
    _Out_ PDEVPROPTYPE Type)
{
    PIP_PROP_OBJECT Object;
    NTSTATUS Status;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(Flags);

    Status = PiPropInitInterfaceObject(SymbolicLinkName, &Object);
    if (!NT_SUCCESS(Status))
        return Status;

    return PiPropGet(&Object, PropertyKey, Lcid, Size, Data, RequiredSize, Type);
}

/* EOF */
