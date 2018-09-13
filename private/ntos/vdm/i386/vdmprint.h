/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    vdmprint.h

Abstract:

    VDM printer defines from softpc tree. These are assumed to
    be constants.

Author:

    Sudeep Bharati (sudeepb) 16-Jan-1993

Revision History:

--*/



#define DATA_PORT_OFFSET	0
#define STATUS_PORT_OFFSET	1
#define CONTROL_PORT_OFFSET	2

#define LPT1_PORT_STATUS        0x3bd
#define LPT2_PORT_STATUS        0x379
#define LPT3_PORT_STATUS        0x279
#define LPT_MASK                0xff0
#define IRQ                     0x10

#define NOTBUSY                 0x80
#define HOST_LPT_BUSY           (1 << 0)
#define STATUS_REG_MASK         0x07

#define get_status(adap)        (*(PUCHAR)((ULONG)(((PVDM_PROCESS_OBJECTS)(PsGetCurrentProcess()->VdmObjects))->PrinterStatus)+(ULONG)adap))
#define get_control(adap)       (*(PUCHAR)((ULONG)(((PVDM_PROCESS_OBJECTS)(PsGetCurrentProcess()->VdmObjects))->PrinterControl)+(ULONG)adap))
#define host_lpt_status(adap)   (*(PUCHAR)((ULONG)(((PVDM_PROCESS_OBJECTS)(PsGetCurrentProcess()->VdmObjects))->PrinterHostState)+(ULONG)adap))
#define set_status(adap,val)    *(PUCHAR)((ULONG)(((PVDM_PROCESS_OBJECTS)(PsGetCurrentProcess()->VdmObjects))->PrinterStatus)+(ULONG)adap) =  val


extern NTSTATUS PspSetProcessIoHandlers(
    IN PEPROCESS Process,
    IN PVOID IoHandlerInformation,
    IN ULONG IoHandlerLength
    );
extern NTSTATUS VdmFlushPrinterWriteData(USHORT Adapter);
