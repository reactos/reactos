/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndis/cl.c
 * PURPOSE:     Services for connectionless NDIS drivers
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */
#include <ndissys.h>

NDIS_STATUS
STDCALL
NdisClAddParty(
    IN      NDIS_HANDLE         NdisVcHandle,
    IN      NDIS_HANDLE         ProtocolPartyContext,
    IN OUT  PCO_CALL_PARAMETERS CallParameters,
    OUT     PNDIS_HANDLE        NdisPartyHandle)
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


NDIS_STATUS
STDCALL
NdisClCloseAddressFamily(
    IN  NDIS_HANDLE NdisAfHandle)
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


NDIS_STATUS
STDCALL
NdisClCloseCall(
    IN  NDIS_HANDLE NdisVcHandle,
    IN  NDIS_HANDLE NdisPartyHandle OPTIONAL,
    IN  PVOID       Buffer          OPTIONAL,
    IN  UINT        Size)
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


NDIS_STATUS
STDCALL
NdisClDeregisterSap(
    IN  NDIS_HANDLE NdisSapHandle)
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


NDIS_STATUS
STDCALL
NdisClDropParty(
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

    return NDIS_STATUS_FAILURE;
}


VOID
STDCALL
NdisClIncomingCallComplete(
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


NDIS_STATUS
STDCALL
NdisClMakeCall(
    IN      NDIS_HANDLE         NdisVcHandle,
    IN OUT  PCO_CALL_PARAMETERS CallParameters,
    IN      NDIS_HANDLE         ProtocolPartyContext    OPTIONAL,
    OUT     PNDIS_HANDLE        NdisPartyHandle         OPTIONAL)
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


NDIS_STATUS 
STDCALL
NdisClModifyCallQoS(
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


NDIS_STATUS
STDCALL
NdisClOpenAddressFamily(
    IN  NDIS_HANDLE                     NdisBindingHandle,
    IN  PCO_ADDRESS_FAMILY              AddressFamily,
    IN  NDIS_HANDLE                     ProtocolAfContext,
    IN  PNDIS_CLIENT_CHARACTERISTICS    ClCharacteristics,
    IN  UINT                            SizeOfClCharacteristics,
    OUT PNDIS_HANDLE                    NdisAfHandle)
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


NDIS_STATUS
STDCALL
NdisClRegisterSap(
    IN  NDIS_HANDLE     NdisAfHandle,
    IN  NDIS_HANDLE     ProtocolSapContext,
    IN  PCO_SAP         Sap,
    OUT PNDIS_HANDLE    NdisSapHandle)
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
