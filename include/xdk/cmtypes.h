/******************************************************************************
 *                            Configuration Manager Types                     *
 ******************************************************************************/

$if (_WDMDDK_)
/* Resource list definitions */
typedef int CM_RESOURCE_TYPE;

#define CmResourceTypeNull              0
#define CmResourceTypePort              1
#define CmResourceTypeInterrupt         2
#define CmResourceTypeMemory            3
#define CmResourceTypeDma               4
#define CmResourceTypeDeviceSpecific    5
#define CmResourceTypeBusNumber         6
#define CmResourceTypeNonArbitrated     128
#define CmResourceTypeConfigData        128
#define CmResourceTypeDevicePrivate     129
#define CmResourceTypePcCardConfig      130
#define CmResourceTypeMfCardConfig      131

/* KEY_VALUE_Xxx.Type */
#define REG_NONE                           0
#define REG_SZ                             1
#define REG_EXPAND_SZ                      2
#define REG_BINARY                         3
#define REG_DWORD                          4
#define REG_DWORD_LITTLE_ENDIAN            4
#define REG_DWORD_BIG_ENDIAN               5
#define REG_LINK                           6
#define REG_MULTI_SZ                       7
#define REG_RESOURCE_LIST                  8
#define REG_FULL_RESOURCE_DESCRIPTOR       9
#define REG_RESOURCE_REQUIREMENTS_LIST     10
#define REG_QWORD                          11
#define REG_QWORD_LITTLE_ENDIAN            11

/* Registry Access Rights */
#define KEY_QUERY_VALUE         (0x0001)
#define KEY_SET_VALUE           (0x0002)
#define KEY_CREATE_SUB_KEY      (0x0004)
#define KEY_ENUMERATE_SUB_KEYS  (0x0008)
#define KEY_NOTIFY              (0x0010)
#define KEY_CREATE_LINK         (0x0020)
#define KEY_WOW64_32KEY         (0x0200)
#define KEY_WOW64_64KEY         (0x0100)
#define KEY_WOW64_RES           (0x0300)

#define KEY_READ                ((STANDARD_RIGHTS_READ       |\
                                  KEY_QUERY_VALUE            |\
                                  KEY_ENUMERATE_SUB_KEYS     |\
                                  KEY_NOTIFY)                 \
                                  &                           \
                                 (~SYNCHRONIZE))

#define KEY_WRITE               ((STANDARD_RIGHTS_WRITE      |\
                                  KEY_SET_VALUE              |\
                                  KEY_CREATE_SUB_KEY)         \
                                  &                           \
                                 (~SYNCHRONIZE))

#define KEY_EXECUTE             ((KEY_READ)                   \
                                  &                           \
                                 (~SYNCHRONIZE))

#define KEY_ALL_ACCESS          ((STANDARD_RIGHTS_ALL        |\
                                  KEY_QUERY_VALUE            |\
                                  KEY_SET_VALUE              |\
                                  KEY_CREATE_SUB_KEY         |\
                                  KEY_ENUMERATE_SUB_KEYS     |\
                                  KEY_NOTIFY                 |\
                                  KEY_CREATE_LINK)            \
                                  &                           \
                                 (~SYNCHRONIZE))

/* Registry Open/Create Options */
#define REG_OPTION_RESERVED         (0x00000000L)
#define REG_OPTION_NON_VOLATILE     (0x00000000L)
#define REG_OPTION_VOLATILE         (0x00000001L)
#define REG_OPTION_CREATE_LINK      (0x00000002L)
#define REG_OPTION_BACKUP_RESTORE   (0x00000004L)
#define REG_OPTION_OPEN_LINK        (0x00000008L)

#define REG_LEGAL_OPTION            \
                (REG_OPTION_RESERVED            |\
                 REG_OPTION_NON_VOLATILE        |\
                 REG_OPTION_VOLATILE            |\
                 REG_OPTION_CREATE_LINK         |\
                 REG_OPTION_BACKUP_RESTORE      |\
                 REG_OPTION_OPEN_LINK)

#define REG_OPEN_LEGAL_OPTION       \
                (REG_OPTION_RESERVED            |\
                 REG_OPTION_BACKUP_RESTORE      |\
                 REG_OPTION_OPEN_LINK)

#define REG_STANDARD_FORMAT            1
#define REG_LATEST_FORMAT              2
#define REG_NO_COMPRESSION             4

/* Key creation/open disposition */
#define REG_CREATED_NEW_KEY         (0x00000001L)
#define REG_OPENED_EXISTING_KEY     (0x00000002L)

/* Key restore & hive load flags */
#define REG_WHOLE_HIVE_VOLATILE         (0x00000001L)
#define REG_REFRESH_HIVE                (0x00000002L)
#define REG_NO_LAZY_FLUSH               (0x00000004L)
#define REG_FORCE_RESTORE               (0x00000008L)
#define REG_APP_HIVE                    (0x00000010L)
#define REG_PROCESS_PRIVATE             (0x00000020L)
#define REG_START_JOURNAL               (0x00000040L)
#define REG_HIVE_EXACT_FILE_GROWTH      (0x00000080L)
#define REG_HIVE_NO_RM                  (0x00000100L)
#define REG_HIVE_SINGLE_LOG             (0x00000200L)
#define REG_BOOT_HIVE                   (0x00000400L)

/* Unload Flags */
#define REG_FORCE_UNLOAD            1

/* Notify Filter Values */
#define REG_NOTIFY_CHANGE_NAME          (0x00000001L)
#define REG_NOTIFY_CHANGE_ATTRIBUTES    (0x00000002L)
#define REG_NOTIFY_CHANGE_LAST_SET      (0x00000004L)
#define REG_NOTIFY_CHANGE_SECURITY      (0x00000008L)

#define REG_LEGAL_CHANGE_FILTER                 \
                (REG_NOTIFY_CHANGE_NAME          |\
                 REG_NOTIFY_CHANGE_ATTRIBUTES    |\
                 REG_NOTIFY_CHANGE_LAST_SET      |\
                 REG_NOTIFY_CHANGE_SECURITY)

typedef struct _CM_FLOPPY_DEVICE_DATA {
  USHORT Version;
  USHORT Revision;
  CHAR Size[8];
  ULONG MaxDensity;
  ULONG MountDensity;
  UCHAR StepRateHeadUnloadTime;
  UCHAR HeadLoadTime;
  UCHAR MotorOffTime;
  UCHAR SectorLengthCode;
  UCHAR SectorPerTrack;
  UCHAR ReadWriteGapLength;
  UCHAR DataTransferLength;
  UCHAR FormatGapLength;
  UCHAR FormatFillCharacter;
  UCHAR HeadSettleTime;
  UCHAR MotorSettleTime;
  UCHAR MaximumTrackValue;
  UCHAR DataTransferRate;
} CM_FLOPPY_DEVICE_DATA, *PCM_FLOPPY_DEVICE_DATA;

#include <pshpack4.h>
typedef struct _CM_PARTIAL_RESOURCE_DESCRIPTOR {
  UCHAR Type;
  UCHAR ShareDisposition;
  USHORT Flags;
  union {
    struct {
      PHYSICAL_ADDRESS Start;
      ULONG Length;
    } Generic;
    struct {
      PHYSICAL_ADDRESS Start;
      ULONG Length;
    } Port;
    struct {
#if defined(NT_PROCESSOR_GROUPS)
      USHORT Level;
      USHORT Group;
#else
      ULONG Level;
#endif
      ULONG Vector;
      KAFFINITY Affinity;
    } Interrupt;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    struct {
      __GNU_EXTENSION union {
        struct {
#if defined(NT_PROCESSOR_GROUPS)
          USHORT Group;
#else
          USHORT Reserved;
#endif
          USHORT MessageCount;
          ULONG Vector;
          KAFFINITY Affinity;
        } Raw;
        struct {
#if defined(NT_PROCESSOR_GROUPS)
          USHORT Level;
          USHORT Group;
#else
          ULONG Level;
#endif
          ULONG Vector;
          KAFFINITY Affinity;
        } Translated;
      } DUMMYUNIONNAME;
    } MessageInterrupt;
#endif
    struct {
      PHYSICAL_ADDRESS Start;
      ULONG Length;
    } Memory;
    struct {
      ULONG Channel;
      ULONG Port;
      ULONG Reserved1;
    } Dma;
    struct {
      ULONG Data[3];
    } DevicePrivate;
    struct {
      ULONG Start;
      ULONG Length;
      ULONG Reserved;
    } BusNumber;
    struct {
      ULONG DataSize;
      ULONG Reserved1;
      ULONG Reserved2;
    } DeviceSpecificData;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    struct {
      PHYSICAL_ADDRESS Start;
      ULONG Length40;
    } Memory40;
    struct {
      PHYSICAL_ADDRESS Start;
      ULONG Length48;
    } Memory48;
    struct {
      PHYSICAL_ADDRESS Start;
      ULONG Length64;
    } Memory64;
#endif
  } u;
} CM_PARTIAL_RESOURCE_DESCRIPTOR, *PCM_PARTIAL_RESOURCE_DESCRIPTOR;
#include <poppack.h>

/* CM_PARTIAL_RESOURCE_DESCRIPTOR.Type */
#define CmResourceTypeNull                0
#define CmResourceTypePort                1
#define CmResourceTypeInterrupt           2
#define CmResourceTypeMemory              3
#define CmResourceTypeDma                 4
#define CmResourceTypeDeviceSpecific      5
#define CmResourceTypeBusNumber           6
#define CmResourceTypeMemoryLarge         7
#define CmResourceTypeNonArbitrated       128
#define CmResourceTypeConfigData          128
#define CmResourceTypeDevicePrivate       129
#define CmResourceTypePcCardConfig        130
#define CmResourceTypeMfCardConfig        131

/* CM_PARTIAL_RESOURCE_DESCRIPTOR.ShareDisposition */
typedef enum _CM_SHARE_DISPOSITION {
  CmResourceShareUndetermined,
  CmResourceShareDeviceExclusive,
  CmResourceShareDriverExclusive,
  CmResourceShareShared
} CM_SHARE_DISPOSITION;

/* CM_PARTIAL_RESOURCE_DESCRIPTOR.Flags if Type = CmResourceTypePort */
#define CM_RESOURCE_PORT_MEMORY           0x0000
#define CM_RESOURCE_PORT_IO               0x0001
#define CM_RESOURCE_PORT_10_BIT_DECODE    0x0004
#define CM_RESOURCE_PORT_12_BIT_DECODE    0x0008
#define CM_RESOURCE_PORT_16_BIT_DECODE    0x0010
#define CM_RESOURCE_PORT_POSITIVE_DECODE  0x0020
#define CM_RESOURCE_PORT_PASSIVE_DECODE   0x0040
#define CM_RESOURCE_PORT_WINDOW_DECODE    0x0080
#define CM_RESOURCE_PORT_BAR              0x0100

/* CM_PARTIAL_RESOURCE_DESCRIPTOR.Flags if Type = CmResourceTypeInterrupt */
#define CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE 0x0000
#define CM_RESOURCE_INTERRUPT_LATCHED         0x0001
#define CM_RESOURCE_INTERRUPT_MESSAGE         0x0002
#define CM_RESOURCE_INTERRUPT_POLICY_INCLUDED 0x0004

/* CM_PARTIAL_RESOURCE_DESCRIPTOR.Flags if Type = CmResourceTypeMemory */
#define CM_RESOURCE_MEMORY_READ_WRITE                    0x0000
#define CM_RESOURCE_MEMORY_READ_ONLY                     0x0001
#define CM_RESOURCE_MEMORY_WRITE_ONLY                    0x0002
#define CM_RESOURCE_MEMORY_WRITEABILITY_MASK             0x0003
#define CM_RESOURCE_MEMORY_PREFETCHABLE                  0x0004
#define CM_RESOURCE_MEMORY_COMBINEDWRITE                 0x0008
#define CM_RESOURCE_MEMORY_24                            0x0010
#define CM_RESOURCE_MEMORY_CACHEABLE                     0x0020
#define CM_RESOURCE_MEMORY_WINDOW_DECODE                 0x0040
#define CM_RESOURCE_MEMORY_BAR                           0x0080
#define CM_RESOURCE_MEMORY_COMPAT_FOR_INACCESSIBLE_RANGE 0x0100

/* CM_PARTIAL_RESOURCE_DESCRIPTOR.Flags if Type = CmResourceTypeDma */
#define CM_RESOURCE_DMA_8                 0x0000
#define CM_RESOURCE_DMA_16                0x0001
#define CM_RESOURCE_DMA_32                0x0002
#define CM_RESOURCE_DMA_8_AND_16          0x0004
#define CM_RESOURCE_DMA_BUS_MASTER        0x0008
#define CM_RESOURCE_DMA_TYPE_A            0x0010
#define CM_RESOURCE_DMA_TYPE_B            0x0020
#define CM_RESOURCE_DMA_TYPE_F            0x0040

typedef struct _CM_PARTIAL_RESOURCE_LIST {
  USHORT Version;
  USHORT Revision;
  ULONG Count;
  CM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptors[1];
} CM_PARTIAL_RESOURCE_LIST, *PCM_PARTIAL_RESOURCE_LIST;

typedef struct _CM_FULL_RESOURCE_DESCRIPTOR {
  INTERFACE_TYPE InterfaceType;
  ULONG BusNumber;
  CM_PARTIAL_RESOURCE_LIST PartialResourceList;
} CM_FULL_RESOURCE_DESCRIPTOR, *PCM_FULL_RESOURCE_DESCRIPTOR;

typedef struct _CM_RESOURCE_LIST {
  ULONG Count;
  CM_FULL_RESOURCE_DESCRIPTOR List[1];
} CM_RESOURCE_LIST, *PCM_RESOURCE_LIST;

#include <pshpack1.h>
typedef struct _CM_INT13_DRIVE_PARAMETER {
  USHORT DriveSelect;
  ULONG MaxCylinders;
  USHORT SectorsPerTrack;
  USHORT MaxHeads;
  USHORT NumberDrives;
} CM_INT13_DRIVE_PARAMETER, *PCM_INT13_DRIVE_PARAMETER;

typedef struct _CM_PNP_BIOS_DEVICE_NODE {
  USHORT Size;
  UCHAR Node;
  ULONG ProductId;
  UCHAR DeviceType[3];
  USHORT DeviceAttributes;
} CM_PNP_BIOS_DEVICE_NODE,*PCM_PNP_BIOS_DEVICE_NODE;

typedef struct _CM_PNP_BIOS_INSTALLATION_CHECK {
  UCHAR Signature[4];
  UCHAR Revision;
  UCHAR Length;
  USHORT ControlField;
  UCHAR Checksum;
  ULONG EventFlagAddress;
  USHORT RealModeEntryOffset;
  USHORT RealModeEntrySegment;
  USHORT ProtectedModeEntryOffset;
  ULONG ProtectedModeCodeBaseAddress;
  ULONG OemDeviceId;
  USHORT RealModeDataBaseAddress;
  ULONG ProtectedModeDataBaseAddress;
} CM_PNP_BIOS_INSTALLATION_CHECK, *PCM_PNP_BIOS_INSTALLATION_CHECK;
#include <poppack.h>

typedef struct _CM_DISK_GEOMETRY_DEVICE_DATA {
  ULONG BytesPerSector;
  ULONG NumberOfCylinders;
  ULONG SectorsPerTrack;
  ULONG NumberOfHeads;
} CM_DISK_GEOMETRY_DEVICE_DATA, *PCM_DISK_GEOMETRY_DEVICE_DATA;

typedef struct _CM_KEYBOARD_DEVICE_DATA {
  USHORT Version;
  USHORT Revision;
  UCHAR Type;
  UCHAR Subtype;
  USHORT KeyboardFlags;
} CM_KEYBOARD_DEVICE_DATA, *PCM_KEYBOARD_DEVICE_DATA;

typedef struct _CM_MCA_POS_DATA {
  USHORT AdapterId;
  UCHAR PosData1;
  UCHAR PosData2;
  UCHAR PosData3;
  UCHAR PosData4;
} CM_MCA_POS_DATA, *PCM_MCA_POS_DATA;

typedef struct _CM_SCSI_DEVICE_DATA {
  USHORT Version;
  USHORT Revision;
  UCHAR HostIdentifier;
} CM_SCSI_DEVICE_DATA, *PCM_SCSI_DEVICE_DATA;

typedef struct _CM_SERIAL_DEVICE_DATA {
  USHORT Version;
  USHORT Revision;
  ULONG BaudClock;
} CM_SERIAL_DEVICE_DATA, *PCM_SERIAL_DEVICE_DATA;

typedef enum _KEY_INFORMATION_CLASS {
  KeyBasicInformation,
  KeyNodeInformation,
  KeyFullInformation,
  KeyNameInformation,
  KeyCachedInformation,
  KeyFlagsInformation,
  KeyVirtualizationInformation,
  KeyHandleTagsInformation,
  MaxKeyInfoClass
} KEY_INFORMATION_CLASS;

typedef struct _KEY_BASIC_INFORMATION {
  LARGE_INTEGER LastWriteTime;
  ULONG TitleIndex;
  ULONG NameLength;
  WCHAR Name[1];
} KEY_BASIC_INFORMATION, *PKEY_BASIC_INFORMATION;

typedef struct _KEY_CONTROL_FLAGS_INFORMATION {
  ULONG ControlFlags;
} KEY_CONTROL_FLAGS_INFORMATION, *PKEY_CONTROL_FLAGS_INFORMATION;

typedef struct _KEY_FULL_INFORMATION {
  LARGE_INTEGER LastWriteTime;
  ULONG TitleIndex;
  ULONG ClassOffset;
  ULONG ClassLength;
  ULONG SubKeys;
  ULONG MaxNameLen;
  ULONG MaxClassLen;
  ULONG Values;
  ULONG MaxValueNameLen;
  ULONG MaxValueDataLen;
  WCHAR Class[1];
} KEY_FULL_INFORMATION, *PKEY_FULL_INFORMATION;

typedef struct _KEY_HANDLE_TAGS_INFORMATION {
  ULONG HandleTags;
} KEY_HANDLE_TAGS_INFORMATION, *PKEY_HANDLE_TAGS_INFORMATION;

typedef struct _KEY_NODE_INFORMATION {
  LARGE_INTEGER LastWriteTime;
  ULONG TitleIndex;
  ULONG ClassOffset;
  ULONG ClassLength;
  ULONG NameLength;
  WCHAR Name[1];
} KEY_NODE_INFORMATION, *PKEY_NODE_INFORMATION;

typedef enum _KEY_SET_INFORMATION_CLASS {
  KeyWriteTimeInformation,
  KeyWow64FlagsInformation,
  KeyControlFlagsInformation,
  KeySetVirtualizationInformation,
  KeySetDebugInformation,
  KeySetHandleTagsInformation,
  MaxKeySetInfoClass
} KEY_SET_INFORMATION_CLASS;

typedef struct _KEY_SET_VIRTUALIZATION_INFORMATION {
  ULONG VirtualTarget:1;
  ULONG VirtualStore:1;
  ULONG VirtualSource:1;
  ULONG Reserved:29;
} KEY_SET_VIRTUALIZATION_INFORMATION, *PKEY_SET_VIRTUALIZATION_INFORMATION;

typedef struct _KEY_VALUE_BASIC_INFORMATION {
  ULONG TitleIndex;
  ULONG Type;
  ULONG NameLength;
  WCHAR Name[1];
} KEY_VALUE_BASIC_INFORMATION, *PKEY_VALUE_BASIC_INFORMATION;

typedef struct _KEY_VALUE_FULL_INFORMATION {
  ULONG TitleIndex;
  ULONG Type;
  ULONG DataOffset;
  ULONG DataLength;
  ULONG NameLength;
  WCHAR Name[1];
} KEY_VALUE_FULL_INFORMATION, *PKEY_VALUE_FULL_INFORMATION;

typedef struct _KEY_VALUE_PARTIAL_INFORMATION {
  ULONG TitleIndex;
  ULONG Type;
  ULONG DataLength;
  UCHAR Data[1];
} KEY_VALUE_PARTIAL_INFORMATION, *PKEY_VALUE_PARTIAL_INFORMATION;

typedef struct _KEY_VALUE_PARTIAL_INFORMATION_ALIGN64 {
  ULONG Type;
  ULONG DataLength;
  UCHAR Data[1];
} KEY_VALUE_PARTIAL_INFORMATION_ALIGN64, *PKEY_VALUE_PARTIAL_INFORMATION_ALIGN64;

typedef struct _KEY_VALUE_ENTRY {
  PUNICODE_STRING ValueName;
  ULONG DataLength;
  ULONG DataOffset;
  ULONG Type;
} KEY_VALUE_ENTRY, *PKEY_VALUE_ENTRY;

typedef enum _KEY_VALUE_INFORMATION_CLASS {
  KeyValueBasicInformation,
  KeyValueFullInformation,
  KeyValuePartialInformation,
  KeyValueFullInformationAlign64,
  KeyValuePartialInformationAlign64
} KEY_VALUE_INFORMATION_CLASS;

typedef struct _KEY_WOW64_FLAGS_INFORMATION {
  ULONG UserFlags;
} KEY_WOW64_FLAGS_INFORMATION, *PKEY_WOW64_FLAGS_INFORMATION;

typedef struct _KEY_WRITE_TIME_INFORMATION {
  LARGE_INTEGER LastWriteTime;
} KEY_WRITE_TIME_INFORMATION, *PKEY_WRITE_TIME_INFORMATION;

typedef enum _REG_NOTIFY_CLASS {
  RegNtDeleteKey,
  RegNtPreDeleteKey = RegNtDeleteKey,
  RegNtSetValueKey,
  RegNtPreSetValueKey = RegNtSetValueKey,
  RegNtDeleteValueKey,
  RegNtPreDeleteValueKey = RegNtDeleteValueKey,
  RegNtSetInformationKey,
  RegNtPreSetInformationKey = RegNtSetInformationKey,
  RegNtRenameKey,
  RegNtPreRenameKey = RegNtRenameKey,
  RegNtEnumerateKey,
  RegNtPreEnumerateKey = RegNtEnumerateKey,
  RegNtEnumerateValueKey,
  RegNtPreEnumerateValueKey = RegNtEnumerateValueKey,
  RegNtQueryKey,
  RegNtPreQueryKey = RegNtQueryKey,
  RegNtQueryValueKey,
  RegNtPreQueryValueKey = RegNtQueryValueKey,
  RegNtQueryMultipleValueKey,
  RegNtPreQueryMultipleValueKey = RegNtQueryMultipleValueKey,
  RegNtPreCreateKey,
  RegNtPostCreateKey,
  RegNtPreOpenKey,
  RegNtPostOpenKey,
  RegNtKeyHandleClose,
  RegNtPreKeyHandleClose = RegNtKeyHandleClose,
  RegNtPostDeleteKey,
  RegNtPostSetValueKey,
  RegNtPostDeleteValueKey,
  RegNtPostSetInformationKey,
  RegNtPostRenameKey,
  RegNtPostEnumerateKey,
  RegNtPostEnumerateValueKey,
  RegNtPostQueryKey,
  RegNtPostQueryValueKey,
  RegNtPostQueryMultipleValueKey,
  RegNtPostKeyHandleClose,
  RegNtPreCreateKeyEx,
  RegNtPostCreateKeyEx,
  RegNtPreOpenKeyEx,
  RegNtPostOpenKeyEx,
  RegNtPreFlushKey,
  RegNtPostFlushKey,
  RegNtPreLoadKey,
  RegNtPostLoadKey,
  RegNtPreUnLoadKey,
  RegNtPostUnLoadKey,
  RegNtPreQueryKeySecurity,
  RegNtPostQueryKeySecurity,
  RegNtPreSetKeySecurity,
  RegNtPostSetKeySecurity,
  RegNtCallbackObjectContextCleanup,
  RegNtPreRestoreKey,
  RegNtPostRestoreKey,
  RegNtPreSaveKey,
  RegNtPostSaveKey,
  RegNtPreReplaceKey,
  RegNtPostReplaceKey,
  MaxRegNtNotifyClass
} REG_NOTIFY_CLASS, *PREG_NOTIFY_CLASS;

typedef NTSTATUS
(NTAPI *PEX_CALLBACK_FUNCTION)(
  IN PVOID CallbackContext,
  IN PVOID Argument1,
  IN PVOID Argument2);

typedef struct _REG_DELETE_KEY_INFORMATION {
  PVOID Object;
  PVOID CallContext;
  PVOID ObjectContext;
  PVOID Reserved;
} REG_DELETE_KEY_INFORMATION, *PREG_DELETE_KEY_INFORMATION
#if (NTDDI_VERSION >= NTDDI_VISTA)
, REG_FLUSH_KEY_INFORMATION, *PREG_FLUSH_KEY_INFORMATION
#endif
;

typedef struct _REG_SET_VALUE_KEY_INFORMATION {
  PVOID Object;
  PUNICODE_STRING ValueName;
  ULONG TitleIndex;
  ULONG Type;
  PVOID Data;
  ULONG DataSize;
  PVOID CallContext;
  PVOID ObjectContext;
  PVOID Reserved;
} REG_SET_VALUE_KEY_INFORMATION, *PREG_SET_VALUE_KEY_INFORMATION;

typedef struct _REG_DELETE_VALUE_KEY_INFORMATION {
  PVOID Object;
  PUNICODE_STRING ValueName;
  PVOID CallContext;
  PVOID ObjectContext;
  PVOID Reserved;
} REG_DELETE_VALUE_KEY_INFORMATION, *PREG_DELETE_VALUE_KEY_INFORMATION;

typedef struct _REG_SET_INFORMATION_KEY_INFORMATION {
  PVOID Object;
  KEY_SET_INFORMATION_CLASS KeySetInformationClass;
  PVOID KeySetInformation;
  ULONG KeySetInformationLength;
  PVOID CallContext;
  PVOID ObjectContext;
  PVOID Reserved;
} REG_SET_INFORMATION_KEY_INFORMATION, *PREG_SET_INFORMATION_KEY_INFORMATION;

typedef struct _REG_ENUMERATE_KEY_INFORMATION {
  PVOID Object;
  ULONG Index;
  KEY_INFORMATION_CLASS KeyInformationClass;
  PVOID KeyInformation;
  ULONG Length;
  PULONG ResultLength;
  PVOID CallContext;
  PVOID ObjectContext;
  PVOID Reserved;
} REG_ENUMERATE_KEY_INFORMATION, *PREG_ENUMERATE_KEY_INFORMATION;

typedef struct _REG_ENUMERATE_VALUE_KEY_INFORMATION {
  PVOID Object;
  ULONG Index;
  KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass;
  PVOID KeyValueInformation;
  ULONG Length;
  PULONG ResultLength;
  PVOID CallContext;
  PVOID ObjectContext;
  PVOID Reserved;
} REG_ENUMERATE_VALUE_KEY_INFORMATION, *PREG_ENUMERATE_VALUE_KEY_INFORMATION;

typedef struct _REG_QUERY_KEY_INFORMATION {
  PVOID Object;
  KEY_INFORMATION_CLASS KeyInformationClass;
  PVOID KeyInformation;
  ULONG Length;
  PULONG ResultLength;
  PVOID CallContext;
  PVOID ObjectContext;
  PVOID Reserved;
} REG_QUERY_KEY_INFORMATION, *PREG_QUERY_KEY_INFORMATION;

typedef struct _REG_QUERY_VALUE_KEY_INFORMATION {
  PVOID Object;
  PUNICODE_STRING ValueName;
  KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass;
  PVOID KeyValueInformation;
  ULONG Length;
  PULONG ResultLength;
  PVOID CallContext;
  PVOID ObjectContext;
  PVOID Reserved;
} REG_QUERY_VALUE_KEY_INFORMATION, *PREG_QUERY_VALUE_KEY_INFORMATION;

typedef struct _REG_QUERY_MULTIPLE_VALUE_KEY_INFORMATION {
  PVOID Object;
  PKEY_VALUE_ENTRY ValueEntries;
  ULONG EntryCount;
  PVOID ValueBuffer;
  PULONG BufferLength;
  PULONG RequiredBufferLength;
  PVOID CallContext;
  PVOID ObjectContext;
  PVOID Reserved;
} REG_QUERY_MULTIPLE_VALUE_KEY_INFORMATION, *PREG_QUERY_MULTIPLE_VALUE_KEY_INFORMATION;

typedef struct _REG_PRE_CREATE_KEY_INFORMATION {
  PUNICODE_STRING CompleteName;
} REG_PRE_CREATE_KEY_INFORMATION, REG_PRE_OPEN_KEY_INFORMATION,*PREG_PRE_CREATE_KEY_INFORMATION, *PREG_PRE_OPEN_KEY_INFORMATION;;

typedef struct _REG_POST_CREATE_KEY_INFORMATION {
  PUNICODE_STRING CompleteName;
  PVOID Object;
  NTSTATUS Status;
} REG_POST_CREATE_KEY_INFORMATION,REG_POST_OPEN_KEY_INFORMATION, *PREG_POST_CREATE_KEY_INFORMATION, *PREG_POST_OPEN_KEY_INFORMATION;

typedef struct _REG_POST_OPERATION_INFORMATION {
  PVOID Object;
  NTSTATUS Status;
  PVOID PreInformation;
  NTSTATUS ReturnStatus;
  PVOID CallContext;
  PVOID ObjectContext;
  PVOID Reserved;
} REG_POST_OPERATION_INFORMATION,*PREG_POST_OPERATION_INFORMATION;

typedef struct _REG_KEY_HANDLE_CLOSE_INFORMATION {
  PVOID Object;
  PVOID CallContext;
  PVOID ObjectContext;
  PVOID Reserved;
} REG_KEY_HANDLE_CLOSE_INFORMATION, *PREG_KEY_HANDLE_CLOSE_INFORMATION;

$endif


