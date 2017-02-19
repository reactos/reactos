/*
 * PROJECT: ReactOS ACPI bus driver
 * FILE:    acpi/ospm/include/acpisys.h
 * PURPOSE: ACPI bus driver definitions
 */

extern UNICODE_STRING ProcessorHardwareIds;
extern LPWSTR ProcessorIdString;
extern LPWSTR ProcessorNameString;

typedef enum _DEVICE_PNP_STATE {

    NotStarted = 0,         // Not started yet
    Started,                // Device has received the START_DEVICE IRP
    StopPending,            // Device has received the QUERY_STOP IRP
    Stopped,                // Device has received the STOP_DEVICE IRP
    RemovalPending,         // Device has received the QUERY_REMOVE IRP
    UnKnown                 // Unknown state

} DEVICE_PNP_STATE;

//
// A common header for the device extensions of the PDOs and FDO
//

typedef struct _COMMON_DEVICE_DATA
{
    PDEVICE_OBJECT  Self;
    BOOLEAN         IsFDO;
    DEVICE_PNP_STATE DevicePnPState;
    DEVICE_PNP_STATE PreviousPnPState;
    SYSTEM_POWER_STATE  SystemPowerState;
    DEVICE_POWER_STATE  DevicePowerState;
} COMMON_DEVICE_DATA, *PCOMMON_DEVICE_DATA;

typedef struct _PDO_DEVICE_DATA
{
    COMMON_DEVICE_DATA Common;
    ACPI_HANDLE AcpiHandle;
    // A back pointer to the bus
    PDEVICE_OBJECT  ParentFdo;
    // An array of (zero terminated wide character strings).
    // The array itself also null terminated
    PWCHAR      HardwareIDs;
    // Link point to hold all the PDOs for a single bus together
    LIST_ENTRY  Link;
    ULONG       InterfaceRefCount;
    UNICODE_STRING InterfaceName;

} PDO_DEVICE_DATA, *PPDO_DEVICE_DATA;

//
// The device extension of the bus itself.  From whence the PDO's are born.
//

typedef struct _FDO_DEVICE_DATA
{
    COMMON_DEVICE_DATA Common;
    PDEVICE_OBJECT  UnderlyingPDO;

    // The underlying bus PDO and the actual device object to which our
    // FDO is attached
    PDEVICE_OBJECT  NextLowerDriver;

    // List of PDOs created so far
    LIST_ENTRY      ListOfPDOs;

    // The PDOs currently enumerated.
    ULONG           NumPDOs;

    // A synchronization for access to the device extension.
    FAST_MUTEX      Mutex;

} FDO_DEVICE_DATA, *PFDO_DEVICE_DATA;

#define FDO_FROM_PDO(pdoData) \
          ((PFDO_DEVICE_DATA) (pdoData)->ParentFdo->DeviceExtension)

#define INITIALIZE_PNP_STATE(_Data_)    \
        (_Data_).DevicePnPState =  NotStarted;\
        (_Data_).PreviousPnPState = NotStarted;

#define SET_NEW_PNP_STATE(_Data_, _state_) \
        (_Data_).PreviousPnPState =  (_Data_).DevicePnPState;\
        (_Data_).DevicePnPState = (_state_);

#define RESTORE_PREVIOUS_PNP_STATE(_Data_)   \
        (_Data_).DevicePnPState =   (_Data_).PreviousPnPState;\

/* acpienum.c */

NTSTATUS
ACPIEnumerateDevices(
  PFDO_DEVICE_DATA DeviceExtension);

NTSTATUS
NTAPI
Bus_PDO_EvalMethod(PPDO_DEVICE_DATA DeviceData,
                   PIRP Irp);

NTSTATUS
NTAPI
Bus_CreateClose (
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    );

VOID
Bus_DriverUnload (
    PDRIVER_OBJECT DriverObject
    );

PCHAR
PnPMinorFunctionString (
    UCHAR MinorFunction
);

NTSTATUS
NTAPI
Bus_AddDevice(
    PDRIVER_OBJECT DriverObject,
    PDEVICE_OBJECT PhysicalDeviceObject
    );

NTSTATUS
Bus_SendIrpSynchronously (
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

NTSTATUS
NTAPI
Bus_PnP (
    PDEVICE_OBJECT   DeviceObject,
    PIRP             Irp
    );

NTSTATUS
NTAPI
Bus_CompletionRoutine(
     PDEVICE_OBJECT   DeviceObject,
     PIRP             Irp,
     PVOID            Context
    );

VOID
Bus_InitializePdo (
    PDEVICE_OBJECT      Pdo,
    PFDO_DEVICE_DATA    FdoData
    );


void
Bus_RemoveFdo (
     PFDO_DEVICE_DATA    FdoData
    );

NTSTATUS
Bus_DestroyPdo (
    PDEVICE_OBJECT      Device,
    PPDO_DEVICE_DATA    PdoData
    );


NTSTATUS
Bus_FDO_PnP (
     PDEVICE_OBJECT       DeviceObject,
     PIRP                 Irp,
     PIO_STACK_LOCATION   IrpStack,
     PFDO_DEVICE_DATA     DeviceData
    );


NTSTATUS
Bus_StartFdo (
      PFDO_DEVICE_DATA            FdoData,
      PIRP   Irp );

PCHAR
DbgDeviceIDString(
    BUS_QUERY_ID_TYPE Type
    );

PCHAR
DbgDeviceRelationString(
     DEVICE_RELATION_TYPE Type
    );

NTSTATUS
Bus_FDO_Power (
    PFDO_DEVICE_DATA    FdoData,
    PIRP                Irp
    );

NTSTATUS
Bus_PDO_Power (
    PPDO_DEVICE_DATA    PdoData,
    PIRP                Irp
    );

NTSTATUS
NTAPI
Bus_Power (
     PDEVICE_OBJECT DeviceObject,
     PIRP Irp
    );

PCHAR
PowerMinorFunctionString (
    UCHAR MinorFunction
);

PCHAR
DbgSystemPowerString(
     SYSTEM_POWER_STATE Type
    );

PCHAR
DbgDevicePowerString(
     DEVICE_POWER_STATE Type
    );

NTSTATUS
Bus_PDO_PnP (
     PDEVICE_OBJECT       DeviceObject,
     PIRP                 Irp,
     PIO_STACK_LOCATION   IrpStack,
     PPDO_DEVICE_DATA     DeviceData
    );

NTSTATUS
Bus_PDO_QueryDeviceCaps(
     PPDO_DEVICE_DATA     DeviceData,
      PIRP   Irp );

NTSTATUS
Bus_PDO_QueryDeviceId(
     PPDO_DEVICE_DATA     DeviceData,
      PIRP   Irp );


NTSTATUS
Bus_PDO_QueryDeviceText(
     PPDO_DEVICE_DATA     DeviceData,
      PIRP   Irp );

NTSTATUS
Bus_PDO_QueryResources(
     PPDO_DEVICE_DATA     DeviceData,
      PIRP   Irp );

NTSTATUS
Bus_PDO_QueryResourceRequirements(
     PPDO_DEVICE_DATA     DeviceData,
      PIRP   Irp );

NTSTATUS
Bus_PDO_QueryDeviceRelations(
     PPDO_DEVICE_DATA     DeviceData,
      PIRP   Irp );

NTSTATUS
Bus_PDO_QueryBusInformation(
     PPDO_DEVICE_DATA     DeviceData,
      PIRP   Irp );

NTSTATUS
Bus_GetDeviceCapabilities(
      PDEVICE_OBJECT          DeviceObject,
      PDEVICE_CAPABILITIES    DeviceCapabilities
    );

NTSTATUS
Bus_PDO_QueryInterface(
     PPDO_DEVICE_DATA     DeviceData,
      PIRP   Irp );

BOOLEAN
Bus_GetCrispinessLevel(
       PVOID Context,
      PUCHAR Level
    );
BOOLEAN
Bus_SetCrispinessLevel(
       PVOID Context,
      UCHAR Level
    );
BOOLEAN
Bus_IsSafetyLockEnabled(
     PVOID Context
    );
VOID
Bus_InterfaceReference (
    PVOID Context
   );
VOID
Bus_InterfaceDereference (
    PVOID Context
   );

/* EOF */
