#include "common/fxdeviceinterface.h"
#include "common/fxdevice.h"
#include "common/mxgeneral.h"
#include "common/stringutil.h"


FxDeviceInterface::FxDeviceInterface(
    )
/*++

Routine Description:
    Constructor for the object.  Initializes all fields

Arguments:
    None

Return Value:
    None

  --*/
{
    RtlZeroMemory(&m_InterfaceClassGUID, sizeof(m_InterfaceClassGUID));

    RtlZeroMemory(&m_SymbolicLinkName, sizeof(m_SymbolicLinkName));
    RtlZeroMemory(&m_ReferenceString, sizeof(m_ReferenceString));

    m_Entry.Next = NULL;

    m_State = FALSE;
}

FxDeviceInterface::~FxDeviceInterface()
/*++

Routine Description:
    Destructor for FxDeviceInterface.  Cleans up any allocations previously
    allocated.

Arguments:
    None

Return Value:
    None

  --*/
{
    // the device interface should be off now
    ASSERT(m_State == FALSE);

    // should no longer be in any list
    ASSERT(m_Entry.Next == NULL);

    if (m_ReferenceString.Buffer != NULL)
    {
        FxPoolFree(m_ReferenceString.Buffer);
        RtlZeroMemory(&m_ReferenceString, sizeof(m_ReferenceString));
    }

    if (m_SymbolicLinkName.Buffer != NULL)
    {
        RtlFreeUnicodeString(&m_SymbolicLinkName);
    }
}

_Must_inspect_result_
NTSTATUS
FxDeviceInterface::Initialize(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in CONST GUID* InterfaceGUID,
    __in_opt PCUNICODE_STRING ReferenceString
    )
/*++

Routine Description:
    Initializes the object with the interface GUID and optional reference string

Arguments:
    InterfaceGUID - GUID describing the interface

    ReferenceString - string used to differentiate between 2 interfaces on the
        same PDO

Return Value:
    STATUS_SUCCESS or STATUS_INSUFFICIENT_RESOURCES

  --*/
{
    RtlCopyMemory(&m_InterfaceClassGUID, InterfaceGUID, sizeof(GUID));

    if (ReferenceString != NULL)
    {
        return FxDuplicateUnicodeString(FxDriverGlobals,
                                        ReferenceString,
                                        &m_ReferenceString);
    }
    else
    {
        return STATUS_SUCCESS;
    }
}

_Must_inspect_result_
NTSTATUS
FxDeviceInterface::Register(
    __in PDEVICE_OBJECT Pdo
    )
/*++

Routine Description:
    Registers the device interface for a given PDO

Arguments:
    Pdo - PDO for the device stack


Return Value:
    returned by IoRegisterDeviceInterface

  --*/
{
    PUNICODE_STRING pString;

    if (m_ReferenceString.Length > 0)
    {
        pString = &m_ReferenceString;
    }
    else
    {
        pString = NULL;
    }

    return Mx::MxRegisterDeviceInterface(
        Pdo, &m_InterfaceClassGUID, pString, &m_SymbolicLinkName);
}

_Must_inspect_result_
NTSTATUS
FxDeviceInterface::Register(
    _In_ FxDevice* Device
    )
{
    NTSTATUS status;
    MdDeviceObject pdo;

    pdo = Device->GetSafePhysicalDevice();

    if (pdo != NULL)
    {
        status = Register(pdo);
    }
    else
    {
        //
        // Leave the device interface unregistered.  When we are in hardware
        // available, we will register there once we know for sure we have a
        // real live PDO that the system has acknowledged.
        //
        DO_NOTHING();

        status = STATUS_SUCCESS;
    }

    return status;
}
