/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndis/cm.c
 * PURPOSE:     Call Manager services
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */
#include <ndissys.h>


/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
NdisCmActivateVc(
    IN      NDIS_HANDLE         NdisVcHandle,
    IN OUT  PCO_CALL_PARAMETERS CallParameters)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return NDIS_STATUS_FAILURE;
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisCmAddPartyComplete(
    IN  NDIS_STATUS         Status,
    IN  NDIS_HANDLE         NdisPartyHandle,
    IN  NDIS_HANDLE         CallMgrPartyContext OPTIONAL,
    IN  PCO_CALL_PARAMETERS CallParameters)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisCmCloseAddressFamilyComplete(
    IN  NDIS_STATUS Status,
    IN  NDIS_HANDLE NdisAfHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisCmCloseCallComplete(
    IN  NDIS_STATUS Status,
    IN  NDIS_HANDLE NdisVcHandle,
    IN  NDIS_HANDLE NdisPartyHandle OPTIONAL)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
NdisCmDeactivateVc(
    IN  NDIS_HANDLE NdisVcHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return NDIS_STATUS_FAILURE;
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisCmDeregisterSapComplete(
    IN  NDIS_STATUS Status,
    IN  NDIS_HANDLE NdisSapHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisCmDispatchCallConnected(
    IN  NDIS_HANDLE NdisVcHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
NdisCmDispatchIncomingCall(
    IN  NDIS_HANDLE         NdisSapHandle,
    IN  NDIS_HANDLE         NdisVcHandle,
    IN  PCO_CALL_PARAMETERS CallParameters)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return NDIS_STATUS_FAILURE;
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisCmDispatchIncomingCallQoSChange(
    IN  NDIS_HANDLE         NdisVcHandle,
    IN  PCO_CALL_PARAMETERS CallParameters)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisCmDispatchIncomingCloseCall(
    IN  NDIS_STATUS CloseStatus,
    IN  NDIS_HANDLE NdisVcHandle,
    IN  PVOID       Buffer  OPTIONAL,
    IN  UINT        Size)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisCmDispatchIncomingDropParty(
    IN  NDIS_STATUS DropStatus,
    IN  NDIS_HANDLE NdisPartyHandle,
    IN  PVOID       Buffer  OPTIONAL,
    IN  UINT        Size)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisCmDropPartyComplete(
    IN  NDIS_STATUS Status,
    IN  NDIS_HANDLE NdisPartyHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisCmMakeCallComplete(
    IN  NDIS_STATUS         Status,
    IN  NDIS_HANDLE         NdisVcHandle,
    IN  NDIS_HANDLE         NdisPartyHandle     OPTIONAL,
    IN  NDIS_HANDLE         CallMgrPartyContext OPTIONAL,
    IN  PCO_CALL_PARAMETERS CallParameters)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisCmModifyCallQoSComplete(
    IN  NDIS_STATUS         Status,
    IN  NDIS_HANDLE         NdisVcHandle,
    IN  PCO_CALL_PARAMETERS CallParameters)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisCmOpenAddressFamilyComplete(
    IN  NDIS_STATUS Status,
    IN  NDIS_HANDLE NdisAfHandle,
    IN  NDIS_HANDLE CallMgrAfContext)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
NdisCmRegisterAddressFamily(
    IN  NDIS_HANDLE                         NdisBindingHandle,
    IN  PCO_ADDRESS_FAMILY                  AddressFamily,
    IN  PNDIS_CALL_MANAGER_CHARACTERISTICS  CmCharacteristics,
    IN  UINT                                SizeOfCmCharacteristics)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return NDIS_STATUS_FAILURE;
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisCmRegisterSapComplete(
    IN  NDIS_STATUS Status,
    IN  NDIS_HANDLE NdisSapHandle,
    IN  NDIS_HANDLE CallMgrSapContext)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
NdisMCmActivateVc(
    IN  NDIS_HANDLE         NdisVcHandle,
    IN  PCO_CALL_PARAMETERS CallParameters)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return NDIS_STATUS_FAILURE;
}


/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
NdisMCmCreateVc(
    IN  NDIS_HANDLE     MiniportAdapterHandle,
    IN  NDIS_HANDLE     NdisAfHandle,
    IN  NDIS_HANDLE     MiniportVcContext,
    OUT PNDIS_HANDLE    NdisVcHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return NDIS_STATUS_FAILURE;
}


/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
NdisMCmDeactivateVc(
    IN  NDIS_HANDLE NdisVcHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return NDIS_STATUS_FAILURE;
}


/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
NdisMCmDeleteVc(
    IN  NDIS_HANDLE NdisVcHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return NDIS_STATUS_FAILURE;
}


/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
NdisMCmRegisterAddressFamily(
    IN  NDIS_HANDLE                         MiniportAdapterHandle,
    IN  PCO_ADDRESS_FAMILY                  AddressFamily,
    IN  PNDIS_CALL_MANAGER_CHARACTERISTICS  CmCharacteristics,
    IN  UINT                                SizeOfCmCharacteristics)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return NDIS_STATUS_FAILURE;
}


/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
NdisMCmRequest(
    IN      NDIS_HANDLE     NdisAfHandle,
    IN      NDIS_HANDLE     NdisVcHandle    OPTIONAL,
    IN      NDIS_HANDLE     NdisPartyHandle OPTIONAL,
    IN OUT  PNDIS_REQUEST   NdisRequest)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return NDIS_STATUS_FAILURE;
}

/* EOF */
