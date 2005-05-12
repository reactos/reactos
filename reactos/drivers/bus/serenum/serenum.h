#if defined(__GNUC__)
  #include <ddk/ntddk.h>
  #include <ddk/ntddser.h>
  #include <ddk/wdmguid.h>
  #include <stdio.h>

  #include <debug.h>

  #define SR_MSR_DSR 0x20

  /* FIXME: these prototypes MUST NOT be here! */
  NTSTATUS STDCALL
  IoAttachDeviceToDeviceStackSafe(
    IN PDEVICE_OBJECT SourceDevice,
    IN PDEVICE_OBJECT TargetDevice,
    OUT PDEVICE_OBJECT *AttachedToDeviceObject);

#elif defined(_MSC_VER)
  #include <ntddk.h>
  #include <ntddser.h>
  #include <c:/progra~1/winddk/inc/ddk/wdm/wxp/wdmguid.h>
  #include <stdio.h>

  #define STDCALL

  #define DPRINT1 DbgPrint("(%s:%d) ", __FILE__, __LINE__), DbgPrint
  #define CHECKPOINT1 DbgPrint("(%s:%d)\n")

  #define TAG(A, B, C, D) (ULONG)(((A)<<0) + ((B)<<8) + ((C)<<16) + ((D)<<24))

  NTSTATUS STDCALL
  IoAttachDeviceToDeviceStackSafe(
    IN PDEVICE_OBJECT SourceDevice,
    IN PDEVICE_OBJECT TargetDevice,
    OUT PDEVICE_OBJECT *AttachedToDeviceObject);

  #define DPRINT DPRINT1
  #define CHECKPOINT CHECKPOINT1

  #define SR_MSR_DSR 0x20
#else
  #error Unknown compiler!
#endif

typedef enum
{
  dsStopped,
  dsStarted,
  dsPaused,
  dsRemoved,
  dsSurpriseRemoved
} SERENUM_DEVICE_STATE;

typedef struct _COMMON_DEVICE_EXTENSION
{
	BOOLEAN IsFDO;
	SERENUM_DEVICE_STATE PnpState;
} COMMON_DEVICE_EXTENSION, *PCOMMON_DEVICE_EXTENSION;

typedef struct _FDO_DEVICE_EXTENSION
{
	COMMON_DEVICE_EXTENSION Common;

	PDEVICE_OBJECT LowerDevice;
	PDEVICE_OBJECT Pdo;
	IO_REMOVE_LOCK RemoveLock;

	UNICODE_STRING SerenumInterfaceName;

	PDEVICE_OBJECT AttachedPdo;
	ULONG Flags;
} FDO_DEVICE_EXTENSION, *PFDO_DEVICE_EXTENSION;

typedef struct _PDO_DEVICE_EXTENSION
{
	COMMON_DEVICE_EXTENSION Common;

	PDEVICE_OBJECT AttachedFdo;

	UNICODE_STRING DeviceDescription; // REG_SZ
	UNICODE_STRING DeviceId;          // REG_SZ
	UNICODE_STRING HardwareIds;       // REG_MULTI_SZ
	UNICODE_STRING CompatibleIds;     // REG_MULTI_SZ
} PDO_DEVICE_EXTENSION, *PPDO_DEVICE_EXTENSION;

#define SERENUM_TAG TAG('S', 'e', 'r', 'e')

/* Flags */
#define FLAG_ENUMERATION_DONE    0x01

/************************************ detect.c */

NTSTATUS
SerenumDetectPnpDevice(
	IN PDEVICE_OBJECT DeviceObject,
	IN PDEVICE_OBJECT LowerDevice);

NTSTATUS
SerenumDetectLegacyDevice(
	IN PDEVICE_OBJECT DeviceObject,
	IN PDEVICE_OBJECT LowerDevice);

/************************************ fdo.c */

NTSTATUS STDCALL
SerenumAddDevice(
	IN PDRIVER_OBJECT DriverObject,
	IN PDEVICE_OBJECT Pdo);

NTSTATUS
SerenumFdoPnp(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

/************************************ misc.c */

NTSTATUS
SerenumDuplicateUnicodeString(
	OUT PUNICODE_STRING Destination,
	IN PUNICODE_STRING Source,
	IN POOL_TYPE PoolType);

NTSTATUS
SerenumInitMultiSzString(
	OUT PUNICODE_STRING Destination,
	... /* list of PCSZ */);

NTSTATUS
ForwardIrpAndWait(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

NTSTATUS STDCALL
ForwardIrpToLowerDeviceAndForget(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

NTSTATUS STDCALL
ForwardIrpToAttachedFdoAndForget(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

NTSTATUS STDCALL
ForwardIrpAndForget(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

/************************************ pdo.c */

NTSTATUS
SerenumPdoPnp(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);
