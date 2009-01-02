/*
    PortCls FDO Extension

    by Andrew Greenwood
*/

#ifndef PORTCLS_PRIVATE_H
#define PORTCLS_PRIVATE_H

#include <ntddk.h>
#include <portcls.h>
#include <debug.h>

#include <portcls.h>
#include <dmusicks.h>

#include "interfaces.h"

#define TAG(A, B, C, D) (ULONG)(((A)<<0) + ((B)<<8) + ((C)<<16) + ((D)<<24))
#define TAG_PORTCLASS TAG('P', 'C', 'L', 'S')

#ifdef _MSC_VER
  #define STDCALL
  #define DDKAPI
#endif

NTSTATUS
NTAPI
PortClsCreate(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp);

NTSTATUS
NTAPI
PortClsPnp(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp);

NTSTATUS
NTAPI
PortClsPower(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp);

NTSTATUS
NTAPI
PortClsSysControl(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp);

NTSTATUS NewMiniportDMusUART(
    OUT PMINIPORT* OutMiniport,
    IN  REFCLSID ClassId);

NTSTATUS NewMiniportFmSynth(
    OUT PMINIPORT* OutMiniport,
    IN  REFCLSID ClassId);

NTSTATUS NewPortMidi(
    OUT PPORT* OutPort);

NTSTATUS NewPortDMus(
    OUT PPORT* OutPort);

NTSTATUS NewPortTopology(
    OUT PPORT* OutPort);

NTSTATUS NewPortWaveCyclic(
    OUT PPORT* OutPort);

NTSTATUS NewPortWavePci(
    OUT PPORT* OutPort);

NTSTATUS NewDmaChannelSlave(
    IN PDEVICE_DESCRIPTION DeviceDesc,
    IN PDMA_ADAPTER Adapter,
    IN ULONG MapRegisters,
    OUT PDMACHANNELSLAVE* DmaChannel);

NTSTATUS NewIDrmPort(
    OUT PDRMPORT2 *OutPort);

NTSTATUS NewPortClsVersion(
    OUT PPORTCLSVERSION * OutVersion);

NTSTATUS NewPortFilterWaveCyclic(
    OUT IPortFilterWaveCyclic ** OutFilter);

typedef struct
{
    LIST_ENTRY Entry;
    KSOBJECT_HEADER ObjectHeader;
}SUBDEVICE_ENTRY;


typedef struct
{
    PDEVICE_OBJECT PhysicalDeviceObject;
    PCPFNSTARTDEVICE StartDevice;
    KSDEVICE_HEADER KsDeviceHeader;
    IAdapterPowerManagement * AdapterPowerManagement;

    IResourceList* resources;
    LIST_ENTRY SubDeviceList;

} PCExtension;






#endif
