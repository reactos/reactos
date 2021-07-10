/*
 * PROJECT:         ReactOS system libraries
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         main functions of xHCI
 * PROGRAMMER:      Rama Teja Gampa <ramateja.g@gmail.com>
*/
#include "usbxhci.h"
#define NDEBUG
#include <debug.h>
#define NDEBUG_XHCI_TRACE

#include "dbg_xhci.h"

USBPORT_REGISTRATION_PACKET RegPacket;

VOID
NTAPI
XHCI_Write64bitReg(IN PULONG BaseAddr,
                   IN ULONGLONG Data)
{
    WRITE_REGISTER_ULONG(BaseAddr, Data);
    WRITE_REGISTER_ULONG(BaseAddr + 1, Data >> 32);
}

MPSTATUS
NTAPI
XHCI_OpenEndpoint(IN PVOID xhciExtension,
                  IN PUSBPORT_ENDPOINT_PROPERTIES  endpointParameters,
                  IN PVOID xhciEndpoint)
{
    DPRINT1("XHCI_OpenEndpoint: function initiated\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
XHCI_ReopenEndpoint(IN PVOID xhciExtension,
                    IN PUSBPORT_ENDPOINT_PROPERTIES  endpointParameters,
                    IN PVOID  xhciEndpoint)
{
    DPRINT1("XHCI_ReopenEndpoint: function initiated\n");
    return MP_STATUS_SUCCESS;
}

VOID
NTAPI
XHCI_QueryEndpointRequirements(IN PVOID xhciExtension,
                               IN PUSBPORT_ENDPOINT_PROPERTIES  endpointParameters,
                               IN PUSBPORT_ENDPOINT_REQUIREMENTS EndpointRequirements)
{
    PUSBPORT_ENDPOINT_PROPERTIES EndpointProperties = endpointParameters;
    ULONG TransferType;
    
    DPRINT1("XHCI_QueryEndpointRequirements: function initiated\n");
    TransferType = EndpointProperties->TransferType;

    switch (TransferType)
    {
        case USBPORT_TRANSFER_TYPE_ISOCHRONOUS:
            DPRINT1("XHCI_QueryEndpointRequirements: IsoTransfer\n");
            break;

        case USBPORT_TRANSFER_TYPE_CONTROL:
            DPRINT1("XHCI_QueryEndpointRequirements: ControlTransfer\n");
            break;

        case USBPORT_TRANSFER_TYPE_BULK:
            DPRINT1("XHCI_QueryEndpointRequirements: BulkTransfer\n");
            break;

        case USBPORT_TRANSFER_TYPE_INTERRUPT:
            DPRINT1("XHCI_QueryEndpointRequirements: InterruptTransfer\n");
            break;

        default:
            DPRINT1("XHCI_QueryEndpointRequirements: Unknown TransferType - %x\n",
                    TransferType);
            DbgBreakPoint();
            break;
    }
}

VOID
NTAPI
XHCI_CloseEndpoint(IN PVOID xhciExtension,
                   IN PVOID xhciEndpoint,
                   IN BOOLEAN IsDoDisablePeriodic)
{
    DPRINT1("XHCI_CloseEndpoint: UNIMPLEMENTED. FIXME\n");
}

MPSTATUS
NTAPI
XHCI_ProcessEvent (IN PXHCI_EXTENSION XhciExtension)
{
    PXHCI_HC_RESOURCES HcResourcesVA;
    PHYSICAL_ADDRESS HcResourcesPA;
    XHCI_EVENT_RING_DEQUEUE_POINTER erstdp;
    PULONG  RunTimeRegisterBase;
    PXHCI_TRB dequeue_pointer;
    ULONG TRBType;
    XHCI_EVENT_TRB eventTRB;
    
    HcResourcesVA = XhciExtension -> HcResourcesVA;
    HcResourcesPA = XhciExtension -> HcResourcesPA;
    
    RunTimeRegisterBase = XhciExtension-> RunTimeRegisterBase;
    dequeue_pointer = HcResourcesVA-> EventRing.dequeue_pointer;
    
    while (TRUE) 
    {
        eventTRB = (*dequeue_pointer).EventTRB;
        if (eventTRB.EventGenericTRB.CycleBit != HcResourcesVA->EventRing.ConsumerCycleState)
        {
            DPRINT1("XHCI_ProcessEvent: cycle bit mismatch. end of processing\n");
            break;
        }
        TRBType = eventTRB.EventGenericTRB.TRBType;
        switch (TRBType)
        {
            case TRANSFER_EVENT:
                DPRINT1("XHCI_ProcessEvent: TRANSFER_EVENT \n");
                break;
            case COMMAND_COMPLETION_EVENT: 
                DPRINT1("XHCI_ProcessEvent: COMMAND_COMPLETION_EVENT\n");
                if (eventTRB.CommandCompletionTRB.CompletionCode == SUCCESS)
                {
                    DPRINT1("XHCI_ProcessEvent: COMMAND_COMPLETION_EVENT,  successful command completion\n");
                }
                else DPRINT1("XHCI_ProcessEvent: COMMAND_COMPLETION_EVENT,  unsuccessful command completion %i \n",
                             eventTRB.CommandCompletionTRB.CompletionCode);
                break;
            case PORT_STATUS_CHANGE_EVENT: 
                DPRINT1("XHCI_ProcessEvent: Port Status change event\n");
                break;
            case BANDWIDTH_RESET_REQUEST_EVENT:
                DPRINT1("XHCI_ProcessEvent: BANDWIDTH_RESET_REQUEST_EVENT\n");
                break;
            case DOORBELL_EVENT:
                DPRINT1("XHCI_ProcessEvent: DOORBELL_EVENT\n");
                break;
            case HOST_CONTROLLER_EVENT:
                DPRINT1("XHCI_ProcessEvent: HOST_CONTROLLER_EVENT\n");
                break;
            case DEVICE_NOTIFICATION_EVENT:
                DPRINT1("XHCI_ProcessEvent: DEVICE_NOTIFICATION_EVENT\n");
                break;
            case MF_INDEX_WARP_EVENT:
                DPRINT1("XHCI_ProcessEvent: MF_INDEX_WARP_EVENT\n");
                break;
            default:
                DPRINT1("XHCI_ProcessEvent: Unknown TRBType - %x\n",
                        TRBType);
                DbgBreakPoint(); 
                break;
        }
        if (dequeue_pointer == &(HcResourcesVA->EventRing.firstSeg.XhciTrb[255]))
        {
            HcResourcesVA->EventRing.ConsumerCycleState = ~(HcResourcesVA->EventRing.ConsumerCycleState);
            HcResourcesVA->EventRing.ProducerCycleState = ~(HcResourcesVA->EventRing.ProducerCycleState);
            dequeue_pointer = &(HcResourcesVA->EventRing.firstSeg.XhciTrb[0]);
        }
        dequeue_pointer = dequeue_pointer + 1;
    }
    
    HcResourcesVA-> EventRing.dequeue_pointer = dequeue_pointer;
    HcResourcesVA-> EventRing.enqueue_pointer = dequeue_pointer;

    erstdp.AsULONGLONG = HcResourcesPA.QuadPart + ((ULONG_PTR)dequeue_pointer - (ULONG_PTR)HcResourcesVA);
    ASSERT(erstdp.AsULONGLONG >= HcResourcesPA.QuadPart && erstdp.AsULONGLONG < HcResourcesPA.QuadPart + sizeof(XHCI_HC_RESOURCES)) ;
    erstdp.DequeueERSTIndex = 0;
    XHCI_Write64bitReg(RunTimeRegisterBase + XHCI_ERSTDP, erstdp.AsULONGLONG);
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
XHCI_SendCommand (IN XHCI_TRB CommandTRB,
                  IN PXHCI_EXTENSION XhciExtension)
{
    PXHCI_HC_RESOURCES HcResourcesVA;
    PHYSICAL_ADDRESS HcResourcesPA;
    PULONG DoorBellRegisterBase;
    XHCI_DOORBELL Doorbell_0;
    PXHCI_TRB enqueue_pointer;
    PXHCI_TRB enqueue_pointer_prev;
    PXHCI_TRB dequeue_pointer;
    XHCI_TRB CheckLink;
    PHYSICAL_ADDRESS LinkPointer;
    
    HcResourcesVA = XhciExtension->HcResourcesVA;
    HcResourcesPA = XhciExtension->HcResourcesPA;
    enqueue_pointer = HcResourcesVA->CommandRing.enqueue_pointer;
    dequeue_pointer = HcResourcesVA->CommandRing.dequeue_pointer;
    // check if ring is full
    if ((enqueue_pointer + 1) == dequeue_pointer) 
    {
        DPRINT1 ("XHCI_SendCommand : Command ring is full \n");
        return MP_STATUS_FAILURE;
    }
    // check if the trb is link trb.
    CheckLink = *enqueue_pointer;
    if (CheckLink.LinkTRB.TRBType == LINK)
    {
        LinkPointer.QuadPart = CheckLink.GenericTRB.Word1;
        LinkPointer.QuadPart = LinkPointer.QuadPart << 32;
        LinkPointer.QuadPart = LinkPointer.QuadPart + CheckLink.GenericTRB.Word0;
        ASSERT(LinkPointer.QuadPart >= HcResourcesPA.QuadPart && LinkPointer.QuadPart < HcResourcesPA.QuadPart + sizeof(XHCI_HC_RESOURCES));
        enqueue_pointer_prev = enqueue_pointer;
        enqueue_pointer = (PXHCI_TRB)(HcResourcesVA + LinkPointer.QuadPart - HcResourcesPA.QuadPart);
        if ((enqueue_pointer == dequeue_pointer) || (enqueue_pointer == dequeue_pointer + 1))
        { // it can't move ahead break out of function
                DPRINT1 ("XHCI_SendCommand : Command ring is full \n");
                return MP_STATUS_FAILURE;
        }
        // now the link trb is valid. set its cycle state to Producer cycle state for the command ring to read
        CheckLink.LinkTRB.CycleBit = HcResourcesVA -> CommandRing.ProducerCycleState;
        // write the link trb back. 
        *enqueue_pointer_prev = CheckLink;
        // now we can go ahead to the next pointer where we want to write the new trb. before that check and toggle if necessaary.
        if (CheckLink.LinkTRB.ToggleCycle == 1)
        {
            HcResourcesVA -> CommandRing.ProducerCycleState = ~ (HcResourcesVA -> CommandRing.ProducerCycleState); //update this when the xHC reaches link trb 
        }
    }
    // place trb on the command ring
    *enqueue_pointer = CommandTRB;
    enqueue_pointer = enqueue_pointer + 1;
    HcResourcesVA->CommandRing.enqueue_pointer = enqueue_pointer;
    // ring doorbell 
    DoorBellRegisterBase = XhciExtension->DoorBellRegisterBase;
    Doorbell_0.DoorBellTarget = 0;
    Doorbell_0.RsvdZ = 0;
    Doorbell_0.AsULONG = 0;
    WRITE_REGISTER_ULONG(DoorBellRegisterBase, Doorbell_0.AsULONG);
    return MP_STATUS_SUCCESS;
}
 
MPSTATUS
NTAPI
XHCI_ControllerWorkTest(IN PXHCI_EXTENSION XhciExtension,
                        IN PXHCI_HC_RESOURCES HcResourcesVA,
                        IN PVOID resourcesStartPA)
{
    
    /*PULONG DoorBellRegisterBase;
    //XHCI_DOORBELL Doorbell_0;
    LARGE_INTEGER CurrentTime = {{0, 0}};
    LARGE_INTEGER LastTime = {{0, 0}};
    XHCI_USB_STATUS Status;
    PHYSICAL_ADDRESS HcResourcesPA;
    //PULONG erstba,erstdp;
    //XHCI_EVENT_RING_SEGMENT_TABLE EventRingSegTable;
   
    XHCI_COMMAND_RING_CONTROL CommandRingControlRegister;
    ULONGLONG CommandRingAddr;
    ULONGLONG EventRingAddr;
    XHCI_EVENT_RING_TABLE_SIZE erstz;
    XHCI_EVENT_RING_TABLE_BASE_ADDR erstba;
    XHCI_EVENT_RING_DEQUEUE_POINTER erstdp;
    XHCI_EVENT_RING_SEGMENT_TABLE EventRingSegTable; */
    // place a no op command trb on the command ring
    XHCI_TRB trb;
    int i = 0;
    //XHCI_TRB eventtrb;
    DPRINT1("XHCI_ControllerWorkTest: Initiated.\n");
    trb.CommandTRB.NoOperation.RsvdZ1 = 0;
    trb.CommandTRB.NoOperation.RsvdZ2 = 0;
    trb.CommandTRB.NoOperation.RsvdZ3 = 0;
    trb.CommandTRB.NoOperation.CycleBit = 1;
    trb.CommandTRB.NoOperation.RsvdZ4 = 0;
    trb.CommandTRB.NoOperation.TRBType = NO_OP_COMMAND;
    trb.CommandTRB.NoOperation.RsvdZ5 = 0;
    for(i=0; i<256; i++){
        XHCI_SendCommand(trb,XhciExtension);
    
        //XHCI_ProcessEvent(XhciExtension);
    }
    //XHCI_SendCommand(trb,XhciExtension);
    
    XHCI_ProcessEvent(XhciExtension);
    /*HcResourcesVA -> CommandRing.firstSeg.XhciTrb[0] = trb;
    // ring the commmand ring door bell register
    DoorBellRegisterBase = XhciExtension->DoorBellRegisterBase;
    Doorbell_0.DoorBellTarget = 0;
    Doorbell_0.RsvdZ = 0;
    Doorbell_0.AsULONG = 0;
    WRITE_REGISTER_ULONG(DoorBellRegisterBase, Doorbell_0.AsULONG);
    // wait for some time.
    KeQuerySystemTime(&CurrentTime);
    CurrentTime.QuadPart += 100 * 100; // 100 msec
    while(TRUE)
    {
        KeQuerySystemTime(&LastTime);
        if (LastTime.QuadPart >= CurrentTime.QuadPart)
        {
            break;
        }
    }
    
    // check for event completion trb
    eventtrb =  HcResourcesVA -> EventRing.firstSeg.XhciTrb[0];
    DPRINT("XHCI_ControllerWorkTest: eventtrb word0    - %p\n", eventtrb.GenericTRB.Word0);
    DPRINT("XHCI_ControllerWorkTest: eventtrb word1    - %p\n", eventtrb.GenericTRB.Word1);
    DPRINT("XHCI_ControllerWorkTest: eventtrb word2    - %p\n", eventtrb.GenericTRB.Word2);
    DPRINT("XHCI_ControllerWorkTest: eventtrb word3    - %p\n", eventtrb.GenericTRB.Word3);
    // status check code
    Status.AsULONG = READ_REGISTER_ULONG(XhciExtension->OperationalRegs + XHCI_USBSTS);
    DPRINT("XHCI_ControllerWorkTest: Status HCHalted    - %p\n", Status.HCHalted);
    DPRINT("XHCI_ControllerWorkTest: Status HostSystemError    - %p\n", Status.HostSystemError);
    DPRINT("XHCI_ControllerWorkTest: Status EventInterrupt    - %p\n", Status.EventInterrupt);
    DPRINT("XHCI_ControllerWorkTest: Status PortChangeDetect    - %p\n", Status.PortChangeDetect);
    DPRINT("XHCI_ControllerWorkTest: Status ControllerNotReady    - %p\n", Status.ControllerNotReady);
    DPRINT("XHCI_ControllerWorkTest: Status HCError    - %p\n", Status.HCError);
    DPRINT("XHCI_ControllerWorkTest: Status     - %p\n", Status.AsULONG);
    // command ring check
    HcResourcesPA.QuadPart = (ULONG_PTR)resourcesStartPA;
    CommandRingAddr = HcResourcesPA.QuadPart + FIELD_OFFSET(XHCI_HC_RESOURCES, CommandRing.firstSeg.XhciTrb[0]);
    DPRINT("XHCI_ControllerWorkTest: CommandRingAddr     - %x\n", CommandRingAddr);
    CommandRingControlRegister.AsULONGLONG = READ_REGISTER_ULONG(XhciExtension->OperationalRegs + XHCI_CRCR+1) | READ_REGISTER_ULONG(XhciExtension->OperationalRegs + XHCI_CRCR );
    DPRINT("XHCI_ControllerWorkTest: CommandRingControlRegister     - %x\n", CommandRingControlRegister.AsULONGLONG);
        DPRINT("XHCI_ControllerWorkTest: CommandRingControlRegister1     - %p\n", READ_REGISTER_ULONG(XhciExtension->OperationalRegs + XHCI_CRCR ));
        DPRINT("XHCI_ControllerWorkTest: CommandRingControlRegister2     - %p\n", READ_REGISTER_ULONG(XhciExtension->OperationalRegs + XHCI_CRCR + 1 ));
    // event ring dprints
    EventRingAddr = HcResourcesPA.QuadPart + FIELD_OFFSET(XHCI_HC_RESOURCES, EventRing.firstSeg.XhciTrb[0]);
    DPRINT("XHCI_ControllerWorkTest: EventRingSegTable.RingSegmentBaseAddr     - %x\n", HcResourcesVA -> EventRingSegTable.RingSegmentBaseAddr);
    DPRINT("XHCI_ControllerWorkTest: EventRingSegTable.RingSegmentSize     - %i\n", HcResourcesVA -> EventRingSegTable.RingSegmentSize);
    DPRINT("XHCI_ControllerWorkTest: event ring addr     - %x\n", EventRingAddr);
    //RunTimeRegisterBase + XHCI_ERSTSZ
    erstz.AsULONG = READ_REGISTER_ULONG(XhciExtension->RunTimeRegisterBase + XHCI_ERSTSZ) ;
    DPRINT("XHCI_ControllerWorkTest: erstz     - %p\n", erstz.AsULONG);
    
    erstba.AsULONGLONG = HcResourcesPA.QuadPart + FIELD_OFFSET(XHCI_HC_RESOURCES, EventRingSegTable);
    DPRINT("XHCI_ControllerWorkTest: erstba addr     - %x\n", erstba.AsULONGLONG);
    erstba.AsULONGLONG = READ_REGISTER_ULONG(XhciExtension->RunTimeRegisterBase + XHCI_ERSTBA+1) | READ_REGISTER_ULONG(XhciExtension->RunTimeRegisterBase + XHCI_ERSTBA );
    DPRINT("XHCI_ControllerWorkTest: erstba reg read     - %x\n", erstba.AsULONGLONG);
    
    DPRINT("XHCI_ControllerWorkTest: pointer crcr     - %p %p\n", XhciExtension->OperationalRegs + XHCI_CRCR+1 , XhciExtension->OperationalRegs + XHCI_CRCR);
    DPRINT("XHCI_ControllerWorkTest: pointer erstz     - %p\n", XhciExtension->RunTimeRegisterBase + XHCI_ERSTSZ);
    DPRINT("XHCI_ControllerWorkTest: pointer erstba     - %p %p\n", XhciExtension->RunTimeRegisterBase + XHCI_ERSTBA+1 , XhciExtension->RunTimeRegisterBase + XHCI_ERSTBA);
    
    */
    //DbgBreakPoint();
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
XHCI_InitializeResources(IN PXHCI_EXTENSION XhciExtension,
                         IN ULONG_PTR resourcesStartVA,
                         IN ULONG  resourcesStartPA)
{
    
    
    PXHCI_HC_RESOURCES HcResourcesVA;
    PHYSICAL_ADDRESS HcResourcesPA;
    PULONG OperationalRegs;
    USHORT PageSize;
    USHORT MaxScratchPadBuffers;
    
    PULONG  RunTimeRegisterBase;
    XHCI_EVENT_RING_TABLE_SIZE erstz;
    XHCI_EVENT_RING_TABLE_BASE_ADDR erstba;
    XHCI_EVENT_RING_DEQUEUE_POINTER erstdp;
    XHCI_EVENT_RING_SEGMENT_TABLE EventRingSegTable;
    
    XHCI_COMMAND_RING_CONTROL CommandRingControlRegister, CommandRingControlRegister_temp;
    XHCI_DEVICE_CONTEXT_BASE_ADD_ARRAY_POINTER DCBAAPointer;
    XHCI_TRB CommandLinkTRB;
    XHCI_LINK_ADDR RingStartAddr;
    
    PHYSICAL_ADDRESS Zero, Max;
    PMDL ScratchPadArrayMDL;
    PXHCI_SCRATCHPAD_BUFFER_ARRAY BufferArrayPointer;
    PMDL ScratchPadBufferMDL;
    int i = 0;
    
    DPRINT1("XHCI_InitializeResources: function initiated\n");
    DPRINT_XHCI("XHCI_InitializeResources: BaseVA - %p, BasePA - %p\n",
                resourcesStartVA,
                resourcesStartPA);
                
    HcResourcesVA = (PXHCI_HC_RESOURCES)resourcesStartVA;
    ASSERT((ULONG_PTR)HcResourcesVA % PAGE_SIZE == 0);
    XhciExtension->HcResourcesVA = HcResourcesVA;
    
    HcResourcesPA.QuadPart = (ULONG_PTR)resourcesStartPA;
    XhciExtension->HcResourcesPA = HcResourcesPA;
    OperationalRegs = XhciExtension->OperationalRegs;
    
    //DCBAA init
    DCBAAPointer.AsULONGLONG =  HcResourcesPA.QuadPart + FIELD_OFFSET(XHCI_HC_RESOURCES, DCBAA);
    DPRINT1("XHCI_InitializeResources  : DCBAAPointer   %p\n", DCBAAPointer.AsULONGLONG );

    XHCI_Write64bitReg(OperationalRegs + XHCI_DCBAAP, DCBAAPointer.AsULONGLONG);

    // command ring intialisation.
    HcResourcesVA->CommandRing.enqueue_pointer = &(HcResourcesVA->CommandRing.firstSeg.XhciTrb[0]);
    HcResourcesVA->CommandRing.dequeue_pointer = &(HcResourcesVA->CommandRing.firstSeg.XhciTrb[0]);
    for (i=0; i<256; i++)
    {
        HcResourcesVA->CommandRing.firstSeg.XhciTrb[i].GenericTRB.Word0 = 0;
        HcResourcesVA->CommandRing.firstSeg.XhciTrb[i].GenericTRB.Word1 = 0;
        HcResourcesVA->CommandRing.firstSeg.XhciTrb[i].GenericTRB.Word2 = 0;
        HcResourcesVA->CommandRing.firstSeg.XhciTrb[i].GenericTRB.Word3 = 0;
    }
    CommandRingControlRegister.AsULONGLONG = HcResourcesPA.QuadPart + FIELD_OFFSET(XHCI_HC_RESOURCES, CommandRing.firstSeg);
    CommandRingControlRegister_temp.AsULONGLONG = READ_REGISTER_ULONG(OperationalRegs + XHCI_CRCR + 1) |  READ_REGISTER_ULONG(OperationalRegs + XHCI_CRCR);
    CommandRingControlRegister.RingCycleState = 1;
    HcResourcesVA->CommandRing.ProducerCycleState = 1;
    HcResourcesVA->CommandRing.ConsumerCycleState = 1;
    CommandRingControlRegister.RsvdP = CommandRingControlRegister_temp.RsvdP;  
    DPRINT1("XHCI_InitializeResources  : CommandRingControlRegister   %p\n", CommandRingControlRegister.AsULONGLONG );
    XHCI_Write64bitReg(OperationalRegs + XHCI_CRCR, CommandRingControlRegister.AsULONGLONG);
    
    // Place link trb with toggle cycle state in the last link trb.

    CommandLinkTRB.GenericTRB.Word0 = 0;
    CommandLinkTRB.GenericTRB.Word1 = 0;
    CommandLinkTRB.GenericTRB.Word2 = 0;
    CommandLinkTRB.GenericTRB.Word3 = 0;
    
    RingStartAddr.AsULONGLONG = HcResourcesPA.QuadPart + FIELD_OFFSET(XHCI_HC_RESOURCES, CommandRing.firstSeg);
    CommandLinkTRB.LinkTRB.RsvdZ1 = 0; 
    CommandLinkTRB.LinkTRB.RingSegmentPointerLo = RingStartAddr.RingSegmentPointerLo;
    CommandLinkTRB.LinkTRB.RingSegmentPointerHi = RingStartAddr.RingSegmentPointerHi;
    CommandLinkTRB.LinkTRB.InterrupterTarget = 0;
    CommandLinkTRB.LinkTRB.CycleBit = ~(HcResourcesVA->CommandRing.ProducerCycleState);
    CommandLinkTRB.LinkTRB.ToggleCycle = 1; //impt
    CommandLinkTRB.LinkTRB.ChainBit = 0;
    CommandLinkTRB.LinkTRB.InterruptOnCompletion = 1; //  NOT NECESSARY
    CommandLinkTRB.LinkTRB.TRBType = LINK;
    
    HcResourcesVA->CommandRing.firstSeg.XhciTrb[255] = CommandLinkTRB;
    // end of command ring init
    
    //Primary Interrupter init
    RunTimeRegisterBase =  XhciExtension -> RunTimeRegisterBase;
    
    // dont change imod now
    erstz.AsULONG = READ_REGISTER_ULONG(RunTimeRegisterBase + XHCI_ERSTSZ) ;
    erstz.EventRingSegTableSize = 1;
    DPRINT1("XHCI_InitializeResources  : erstz.AsULONG   %p\n", erstz.AsULONG );
    WRITE_REGISTER_ULONG(RunTimeRegisterBase + XHCI_ERSTSZ, erstz.AsULONG);
    // event ring dequeue pointer.
    erstdp.AsULONGLONG = HcResourcesPA.QuadPart + FIELD_OFFSET(XHCI_HC_RESOURCES, EventRing.firstSeg.XhciTrb[0]);
    
    HcResourcesVA->EventRing.enqueue_pointer = &(HcResourcesVA->EventRing.firstSeg.XhciTrb[0]);
    HcResourcesVA->EventRing.dequeue_pointer = &(HcResourcesVA->EventRing.firstSeg.XhciTrb[0]);
    
    HcResourcesVA->EventRing.ProducerCycleState = 1;
    HcResourcesVA->EventRing.ConsumerCycleState = 1;
    
    erstdp.DequeueERSTIndex =0;
    DPRINT1("XHCI_InitializeResources  : erstdp.AsULONGLONG %p\n", erstdp.AsULONGLONG );
    XHCI_Write64bitReg(RunTimeRegisterBase + XHCI_ERSTDP, erstdp.AsULONGLONG);
    // event ring segment table base address array
    
    erstba.AsULONGLONG = HcResourcesPA.QuadPart + FIELD_OFFSET(XHCI_HC_RESOURCES, EventRingSegTable);
    EventRingSegTable.RingSegmentBaseAddr = (ULONGLONG)HcResourcesPA.QuadPart + FIELD_OFFSET(XHCI_HC_RESOURCES, EventRing.firstSeg.XhciTrb[0]);
    EventRingSegTable.RingSegmentSize = 256;
    EventRingSegTable.RsvdZ = 0;
    HcResourcesVA->EventRingSegTable = EventRingSegTable;
    DPRINT1("XHCI_InitializeResources  : erstba.AsULONGLONG   %p\n", erstba.AsULONGLONG );
    XHCI_Write64bitReg(RunTimeRegisterBase + XHCI_ERSTBA, erstba.AsULONGLONG);
    // intially enque and deque are equal. 
    
    
    for (i=0; i<256; i++)
    {
        HcResourcesVA->EventRing.firstSeg.XhciTrb[i].GenericTRB.Word0 = 0;
        HcResourcesVA->EventRing.firstSeg.XhciTrb[i].GenericTRB.Word1 = 0;
        HcResourcesVA->EventRing.firstSeg.XhciTrb[i].GenericTRB.Word2 = 0;
        HcResourcesVA->EventRing.firstSeg.XhciTrb[i].GenericTRB.Word3 = 0;
    }
    
    // check if the controller supports 4k page size or quit.
    PageSize = XhciExtension-> PageSize;
    MaxScratchPadBuffers = XhciExtension->MaxScratchPadBuffers;
    
    if (MaxScratchPadBuffers == 0)
    { // xHCI may declare 0 scratchpad arrays. if so there is no need for memory allocation.
        return MP_STATUS_SUCCESS;
    }
    if ((PageSize & (1 << 0)) == 0) 
    {
        DPRINT1("XHCI_InitializeResources  : fail. does not support 4k page size   %p\n",PageSize);
        return MP_STATUS_FAILURE;
    }
    // allocate scratchpad buffer array
     
    Zero.QuadPart = 0; 
    Max.QuadPart = -1;   
    
    BufferArrayPointer = MmAllocateContiguousMemory(MaxScratchPadBuffers * sizeof(XHCI_SCRATCHPAD_BUFFER_ARRAY), Max);
    if (BufferArrayPointer == NULL)
    {
        DPRINT1("XHCI_InitializeResources  : Scratch pad array ContiguousMemory allcoation fail NULL\n");
        return MP_STATUS_FAILURE;
    }
    ScratchPadArrayMDL = IoAllocateMdl(BufferArrayPointer, MaxScratchPadBuffers * sizeof(XHCI_SCRATCHPAD_BUFFER_ARRAY), FALSE, FALSE, NULL);
    if (ScratchPadArrayMDL == NULL) 
    {
        DPRINT1("XHCI_InitializeResources  : Scratch pad array could not be allocated. it is NULL\n");
        MmFreeContiguousMemory(BufferArrayPointer);
        return MP_STATUS_FAILURE;
    }
    MmBuildMdlForNonPagedPool(ScratchPadArrayMDL);

    HcResourcesVA->DCBAA.ContextBaseAddr[0].QuadPart = MmGetMdlPfnArray(ScratchPadArrayMDL)[0] << PAGE_SHIFT; 
    //allocate scratchpad buffers
    
    ScratchPadBufferMDL = MmAllocatePagesForMdlEx(Zero, Max, Zero, MaxScratchPadBuffers*PAGE_SIZE, MmNonCached, 0);
    if (ScratchPadBufferMDL == NULL) 
    { 
        IoFreeMdl(ScratchPadArrayMDL);
        MmFreeContiguousMemory(BufferArrayPointer);
        return MP_STATUS_FAILURE;
    }
    if (MmGetMdlByteCount(ScratchPadBufferMDL) < MaxScratchPadBuffers*PAGE_SIZE) 
    { 
        MmFreePagesFromMdl(ScratchPadBufferMDL); 
        ExFreePool(ScratchPadBufferMDL);
        IoFreeMdl(ScratchPadArrayMDL);
        MmFreeContiguousMemory(BufferArrayPointer);
        return MP_STATUS_FAILURE;
    }
    for (i = 0; i < MaxScratchPadBuffers ; i++)
    {
        BufferArrayPointer[i].AsULONGLONG = MmGetMdlPfnArray(ScratchPadBufferMDL)[i] << PAGE_SHIFT;
    }
    XhciExtension-> ScratchPadArrayMDL = ScratchPadArrayMDL;
    XhciExtension-> ScratchPadBufferMDL = ScratchPadBufferMDL;
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
XHCI_InitializeHardware(IN PXHCI_EXTENSION XhciExtension)
{
    PULONG BaseIoAdress;
    PULONG OperationalRegs;
    XHCI_USB_COMMAND Command;
    XHCI_USB_STATUS Status;
    LARGE_INTEGER CurrentTime = {{0, 0}};
    LARGE_INTEGER LastTime = {{0, 0}};
    XHCI_HC_STRUCTURAL_PARAMS_1 StructuralParams_1;
    XHCI_CONFIGURE Config;
    
    DPRINT1("XHCI_InitializeHardware: function initiated\n");
    DPRINT1("XHCI_InitializeHardware: ... \n");

    OperationalRegs = XhciExtension->OperationalRegs;
    BaseIoAdress = XhciExtension->BaseIoAdress;
   
    KeQuerySystemTime(&CurrentTime);
    CurrentTime.QuadPart += 100 * 10000;  
    
    Status.AsULONG = READ_REGISTER_ULONG(OperationalRegs + XHCI_USBSTS);
    ASSERT(Status.ControllerNotReady != 1); // this is needed before writing anything to the operaational or doorbell registers

    Command.AsULONG = READ_REGISTER_ULONG(OperationalRegs + XHCI_USBCMD);
    Command.HCReset = 1;
    WRITE_REGISTER_ULONG(OperationalRegs + XHCI_USBCMD, Command.AsULONG);
    while (TRUE)
    {
        KeQuerySystemTime(&LastTime);
        
        Command.AsULONG = READ_REGISTER_ULONG(OperationalRegs + XHCI_USBCMD);
       
        if (Command.HCReset != 1)
        {
            break;
        }

        if (LastTime.QuadPart >= CurrentTime.QuadPart)
        {
            DPRINT1("XHCI_InitializeHardware: Software Reset failed!\n");
            return MP_STATUS_FAILURE;
        }
    }
    DPRINT1("XHCI_InitializeHardware: Reset - OK\n");
    
    StructuralParams_1.AsULONG = READ_REGISTER_ULONG(BaseIoAdress + XHCI_HCSP1); // HCSPARAMS1 register

    XhciExtension->NumberOfPorts = StructuralParams_1.NumberOfPorts;
    
    Command.AsULONG = READ_REGISTER_ULONG(OperationalRegs + XHCI_USBCMD);
    Config.AsULONG = READ_REGISTER_ULONG(OperationalRegs + XHCI_CONFIG);
    ASSERT(Command.RunStop == 0); //required before setting max device slots enabled.
    Config.MaxDeviceSlotsEnabled = 1; // max possible value is number of slots HCSPARAMS1
    Config.U3EntryEnable = 0;
    Config.ConfigurationInfoEnable = 0;
    WRITE_REGISTER_ULONG(OperationalRegs + XHCI_CONFIG, Config.AsULONG);
    
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
XHCI_StartController(IN PVOID xhciExtension,
                     IN PUSBPORT_RESOURCES Resources)
{
    
    PXHCI_EXTENSION XhciExtension;
    PULONG BaseIoAdress;
    PULONG OperationalRegs;
    PULONG RunTimeRegisterBase;
    PULONG DoorBellRegisterBase;
    XHCI_CAPLENGHT_INTERFACE_VERSION CapLenReg;
    XHCI_DOORBELL_OFFSET DoorBellOffsetRegister;
    MPSTATUS MPStatus;
    XHCI_USB_COMMAND Command;
    XHCI_RT_REGISTER_SPACE_OFFSET RTSOffsetRegister;
    UCHAR CapabilityRegLength;
    UCHAR Fladj;
    XHCI_PAGE_SIZE PageSizeReg;
    USHORT MaxScratchPadBuffers;
    XHCI_HC_STRUCTURAL_PARAMS_2 HCSPARAMS2;
    
    DPRINT1("XHCI_StartController: function initiated\n");
    if ((Resources->ResourcesTypes & (USBPORT_RESOURCES_MEMORY | USBPORT_RESOURCES_INTERRUPT)) !=
                                     (USBPORT_RESOURCES_MEMORY | USBPORT_RESOURCES_INTERRUPT))
    {
        DPRINT1("XHCI_StartController: Resources->ResourcesTypes - %x\n",
                Resources->ResourcesTypes);

        return MP_STATUS_ERROR;
    }
    
    XhciExtension = (PXHCI_EXTENSION)xhciExtension;

    BaseIoAdress = (PULONG)Resources->ResourceBase;
    XhciExtension->BaseIoAdress = BaseIoAdress;

    CapLenReg.AsULONG = READ_REGISTER_ULONG(BaseIoAdress);
    CapLenReg.Rsvd = 0;
    CapLenReg.HostControllerInterfaceVersion = 0;
    CapabilityRegLength = (UCHAR)CapLenReg.CapabilityRegistersLength;
    OperationalRegs = (PULONG)((ULONG_PTR)BaseIoAdress + CapabilityRegLength);
    XhciExtension->OperationalRegs = OperationalRegs;
    
    DoorBellOffsetRegister.AsULONG = READ_REGISTER_ULONG(BaseIoAdress + XHCI_DBOFF);
    DoorBellRegisterBase = (PULONG)((PBYTE)BaseIoAdress + DoorBellOffsetRegister.AsULONG);
    XhciExtension->DoorBellRegisterBase = DoorBellRegisterBase;
    
    RTSOffsetRegister.AsULONG = READ_REGISTER_ULONG(BaseIoAdress + XHCI_RTSOFF);
    RunTimeRegisterBase = (PULONG)((PBYTE)BaseIoAdress + RTSOffsetRegister.AsULONG);
    XhciExtension->RunTimeRegisterBase = RunTimeRegisterBase;
    
    PageSizeReg.AsULONG =  READ_REGISTER_ULONG(OperationalRegs + XHCI_PGSZ);
    XhciExtension->PageSize = PageSizeReg.PageSize;
    HCSPARAMS2.AsULONG = READ_REGISTER_ULONG(BaseIoAdress + XHCI_HCSP2);
    MaxScratchPadBuffers = 0;
    MaxScratchPadBuffers = HCSPARAMS2.MaxSPBuffersHi;
    MaxScratchPadBuffers = MaxScratchPadBuffers << 5;
    MaxScratchPadBuffers = MaxScratchPadBuffers + HCSPARAMS2.MaxSPBuffersLo;
    XhciExtension->MaxScratchPadBuffers = MaxScratchPadBuffers;
    
    DPRINT1("XHCI_StartController: BaseIoAdress    - %p\n", BaseIoAdress);
    DPRINT1("XHCI_StartController: OperationalRegs - %p\n", OperationalRegs);
    DPRINT1("XHCI_StartController: DoorBellRegisterBase - %p\n", DoorBellRegisterBase);
    DPRINT1("XHCI_StartController: RunTimeRegisterBase - %p\n", RunTimeRegisterBase);
    DPRINT1("XHCI_StartController: PageSize - %p\n", XhciExtension->PageSize);
    DPRINT1("XHCI_StartController: MaxScratchPadBuffers - %p\n", MaxScratchPadBuffers);
    
    RegPacket.UsbPortReadWriteConfigSpace(XhciExtension,
                                          1,
                                          &Fladj,
                                          0x61,
                                          1);

    XhciExtension->FrameLengthAdjustment = Fladj;
    
    MPStatus = XHCI_InitializeHardware(XhciExtension);

    if (MPStatus)
    {
        DPRINT1("XHCI_StartController: Unsuccessful InitializeHardware()\n");
        return MPStatus;
    }

    MPStatus = XHCI_InitializeResources(XhciExtension,
                                       Resources->StartVA,
                                       Resources->StartPA);

    if (MPStatus)
    {
        DPRINT1("XHCI_StartController: Unsuccessful InitializeSchedule()\n");
        return MPStatus;
    }
    
    // starting the controller
    Command.AsULONG = READ_REGISTER_ULONG(OperationalRegs + XHCI_USBCMD);
    Command.RunStop = 1;
    WRITE_REGISTER_ULONG(OperationalRegs + XHCI_USBCMD, Command.AsULONG);
    
    //below line should be uncommented if you want to use the controller work test function
    //MPStatus = XHCI_ControllerWorkTest(XhciExtension,Resources->StartVA, Resources->StartPA );

    return MP_STATUS_SUCCESS;
}


VOID
NTAPI
XHCI_StopController(IN PVOID xhciExtension,
                    IN BOOLEAN IsDoDisableInterrupts)
{
    
    PXHCI_EXTENSION XhciExtension;
    USHORT MaxScratchPadBuffers;
    PMDL ScratchPadArrayMDL;
    PMDL ScratchPadBufferMDL;
    PXHCI_SCRATCHPAD_BUFFER_ARRAY BufferArrayPointer;
    XHCI_USB_COMMAND Command, Command_temp;
    PULONG OperationalRegs;
    XHCI_USB_STATUS Status;
    LARGE_INTEGER CurrentTime = {{0, 0}};
    LARGE_INTEGER LastTime = {{0, 0}};
    
    DPRINT1("XHCI_StopController: Function initiated. \n");
    XhciExtension = (PXHCI_EXTENSION) xhciExtension;
    MaxScratchPadBuffers = XhciExtension->MaxScratchPadBuffers;
    // free memory allocated to scratchpad buffers.
    ScratchPadArrayMDL = XhciExtension-> ScratchPadArrayMDL; 
    ScratchPadBufferMDL = XhciExtension-> ScratchPadBufferMDL;
    if (MaxScratchPadBuffers != 0)
    {
        // free the scratchpad buffers
        MmFreePagesFromMdl(ScratchPadBufferMDL);
        ExFreePool(ScratchPadBufferMDL);
        // free scratchpad array
        BufferArrayPointer = MmGetMdlVirtualAddress(ScratchPadArrayMDL);
        IoFreeMdl(ScratchPadArrayMDL);
        MmFreeContiguousMemory(BufferArrayPointer);
    }
    
    OperationalRegs = XhciExtension->OperationalRegs;
    Command_temp.AsULONG = READ_REGISTER_ULONG(OperationalRegs + XHCI_USBCMD);
    Command.AsULONG = 0;
    Command.RsvdP1 = Command_temp.RsvdP1;
    Command.RsvdP2 = Command_temp.RsvdP2;
    Command.RsvdP3 = Command_temp.RsvdP3;
    WRITE_REGISTER_ULONG(OperationalRegs + XHCI_USBCMD, Command.AsULONG);
    
    KeQuerySystemTime(&CurrentTime);
    CurrentTime.QuadPart += 100 * 10000;
    while (TRUE)
    {
        KeQuerySystemTime(&LastTime);
        
        Status.AsULONG = READ_REGISTER_ULONG(OperationalRegs + XHCI_USBSTS);
       
        if (Status.HCHalted == 1)
        {
            break;
        }

        if (LastTime.QuadPart >= CurrentTime.QuadPart)
        {
            DPRINT1("XHCI_StopController: controller stop  failed!\n");
        }
    }

}

VOID
NTAPI
XHCI_SuspendController(IN PVOID xhciExtension)
{
    
    PXHCI_EXTENSION XhciExtension;
    DPRINT1("XHCI_SuspendController: function initiated\n");
    XhciExtension = (PXHCI_EXTENSION)xhciExtension;
    
    XhciExtension->Flags |= XHCI_FLAGS_CONTROLLER_SUSPEND;
    
}

MPSTATUS
NTAPI
XHCI_ResumeController(IN PVOID xhciExtension)
{
    
    PXHCI_EXTENSION XhciExtension;
    DPRINT1("XHCI_ResumeController: function initiated\n");
    XhciExtension = (PXHCI_EXTENSION)xhciExtension;
    
    XhciExtension->Flags &= ~XHCI_FLAGS_CONTROLLER_SUSPEND;
    return MP_STATUS_SUCCESS;
}

BOOLEAN
NTAPI
XHCI_HardwarePresent(IN PXHCI_EXTENSION xhciExtension,
                     IN BOOLEAN IsInvalidateController)
{
    DPRINT1("XHCI_HardwarePresent: function initiated\n");
    return TRUE;
}

BOOLEAN
NTAPI
XHCI_InterruptService(IN PVOID xhciExtension)
{
    
    PULONG  RunTimeRegisterBase;
    XHCI_INTERRUPTER_MANAGEMENT Iman;
    PXHCI_EXTENSION XhciExtension;
    
    DPRINT1("XHCI_InterruptService: function initiated\n");
    XhciExtension = (PXHCI_EXTENSION)xhciExtension;
    
    RunTimeRegisterBase = XhciExtension->RunTimeRegisterBase; 
    Iman.AsULONG = READ_REGISTER_ULONG(RunTimeRegisterBase + XHCI_IMAN);
    if (Iman.InterruptPending == 0)
    {
        return FALSE;
    }
    Iman.InterruptPending = 1; 
    WRITE_REGISTER_ULONG(RunTimeRegisterBase + XHCI_IMAN, Iman.AsULONG);
    DPRINT1("XHCI_InterruptService: Succesful Interupt\n");
    // changing the enque pointer

    return TRUE;
}

VOID
NTAPI
XHCI_InterruptDpc(IN PVOID xhciExtension,
                  IN BOOLEAN IsDoEnableInterrupts)
{
    DPRINT1("XHCI_InterruptDpc: function initiated\n");
    XHCI_ProcessEvent(xhciExtension);
}

MPSTATUS
NTAPI
XHCI_SubmitTransfer(IN PVOID xhciExtension,
                    IN PVOID xhciEndpoint,
                    IN PUSBPORT_TRANSFER_PARAMETERS  transferParameters,
                    IN PVOID xhciTransfer,
                    IN PUSBPORT_SCATTER_GATHER_LIST  sgList)
{
    DPRINT1("XHCI_SubmitTransfer: function initiated\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
XHCI_SubmitIsoTransfer(IN PVOID xhciExtension,
                       IN PVOID xhciEndpoint,
                       IN PUSBPORT_TRANSFER_PARAMETERS transferParameters,
                       IN PVOID xhciTransfer,
                       IN PVOID isoParameters)
{
    DPRINT1("XHCI_SubmitIsoTransfer: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

VOID
NTAPI
XHCI_AbortIsoTransfer(IN PXHCI_EXTENSION xhciExtension,
                      IN PXHCI_ENDPOINT xhciEndpoint,
                      IN PXHCI_TRANSFER xhciTransfer)
{
    DPRINT1("XHCI_AbortIsoTransfer: UNIMPLEMENTED. FIXME\n");
}

VOID
NTAPI
XHCI_AbortAsyncTransfer(IN PXHCI_EXTENSION xhciExtension,
                        IN PXHCI_ENDPOINT xhciEndpoint,
                        IN PXHCI_TRANSFER xhciTransfer)
{
    DPRINT1("XHCI_AbortAsyncTransfer: function initiated\n");
}

VOID
NTAPI
XHCI_AbortTransfer(IN PVOID xhciExtension,
                   IN PVOID xhciEndpoint,
                   IN PVOID xhciTransfer,
                   IN PULONG CompletedLength)
{
    DPRINT1("XHCI_AbortTransfer: function initiated\n");
}

ULONG
NTAPI
XHCI_GetEndpointState(IN PVOID xhciExtension,
                      IN PVOID xhciEndpoint)
{
    DPRINT1("XHCI_GetEndpointState: UNIMPLEMENTED. FIXME\n");
    return 0;
}

VOID
NTAPI
XHCI_SetEndpointState(IN PVOID xhciExtension,
                      IN PVOID xhciEndpoint,
                      IN ULONG EndpointState)
{
    DPRINT1("XHCI_SetEndpointState: function initiated\n");
}

VOID
NTAPI
XHCI_PollEndpoint(IN PVOID xhciExtension,
                  IN PVOID xhciEndpoint)
{
    DPRINT1("XHCI_PollEndpoint: function initiated\n");
}

VOID
NTAPI
XHCI_CheckController(IN PVOID xhciExtension)
{
    DPRINT("XHCI_CheckController: function initiated\n");
}

ULONG
NTAPI
XHCI_Get32BitFrameNumber(IN PVOID xhciExtension)
{
    DPRINT("XHCI_Get32BitFrameNumber: function initiated\n"); 
    return 0;
}

VOID
NTAPI
XHCI_InterruptNextSOF(IN PVOID xhciExtension)
{
    DPRINT1("XHCI_InterruptNextSOF: function initiated\n");
}

VOID
NTAPI
XHCI_EnableInterrupts(IN PVOID xhciExtension)
{
    
    PXHCI_EXTENSION XhciExtension;
    PULONG  RunTimeRegisterBase;
    XHCI_INTERRUPTER_MANAGEMENT Iman;

    DPRINT1("XHCI_EnableInterrupts: function initiated\n");
    XhciExtension = (PXHCI_EXTENSION)xhciExtension;
    
    RunTimeRegisterBase =  XhciExtension->RunTimeRegisterBase;
    Iman.AsULONG = READ_REGISTER_ULONG(RunTimeRegisterBase + XHCI_IMAN);
    Iman.InterruptEnable = 1;
    WRITE_REGISTER_ULONG(RunTimeRegisterBase + XHCI_IMAN, Iman.AsULONG);
    DPRINT1("XHCI_EnableInterrupts: Interrupts enabled\n");
}

VOID
NTAPI
XHCI_DisableInterrupts(IN PVOID xhciExtension)
{
    
    PXHCI_EXTENSION XhciExtension;
    PULONG  RunTimeRegisterBase;
    XHCI_INTERRUPTER_MANAGEMENT Iman;
    DPRINT1("XHCI_DisableInterrupts: function initiated\n");
    XhciExtension = (PXHCI_EXTENSION)xhciExtension;
    
    RunTimeRegisterBase =  XhciExtension -> RunTimeRegisterBase;
    Iman.AsULONG = READ_REGISTER_ULONG(RunTimeRegisterBase + XHCI_IMAN);
    Iman.InterruptEnable = 0;
    WRITE_REGISTER_ULONG(RunTimeRegisterBase + XHCI_IMAN,Iman.AsULONG);

    DPRINT1("XHCI_EnableInterrupts: Interrupts enabled\n");
}

VOID
NTAPI
XHCI_PollController(IN PVOID xhciExtension)
{
    
    PXHCI_EXTENSION XhciExtension;
    DPRINT("XHCI_PollController: function initiated\n");
    XhciExtension = (PXHCI_EXTENSION)xhciExtension;

    if (!(XhciExtension->Flags & XHCI_FLAGS_CONTROLLER_SUSPEND))
    {
        RegPacket.UsbPortInvalidateRootHub(XhciExtension);
        return;
    }
    
}

VOID
NTAPI
XHCI_SetEndpointDataToggle(IN PVOID xhciExtension,
                           IN PVOID xhciEndpoint,
                           IN ULONG DataToggle)
{
    DPRINT1("XHCI_SetEndpointDataToggle: function initiated\n");
}

ULONG
NTAPI
XHCI_GetEndpointStatus(IN PVOID xhciExtension,
                       IN PVOID xhciEndpoint)
{
    DPRINT1("XHCI_GetEndpointStatus: function initiated\n");
    return 0;
}

VOID
NTAPI
XHCI_SetEndpointStatus(IN PVOID xhciExtension,
                       IN PVOID xhciEndpoint,
                       IN ULONG EndpointStatus)
{
    DPRINT1("XHCI_SetEndpointStatus: function initiated\n");
}

MPSTATUS
NTAPI
XHCI_StartSendOnePacket(IN PVOID xhciExtension,
                        IN PVOID PacketParameters,
                        IN PVOID Data,
                        IN PULONG pDataLength,
                        IN PVOID BufferVA,
                        IN PVOID BufferPA,
                        IN ULONG BufferLength,
                        IN USBD_STATUS * pUSBDStatus)
{
    DPRINT1("XHCI_StartSendOnePacket: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
XHCI_EndSendOnePacket(IN PVOID xhciExtension,
                      IN PVOID PacketParameters,
                      IN PVOID Data,
                      IN PULONG pDataLength,
                      IN PVOID BufferVA,
                      IN PVOID BufferPA,
                      IN ULONG BufferLength,
                      IN USBD_STATUS * pUSBDStatus)
{
    DPRINT1("XHCI_EndSendOnePacket: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
XHCI_PassThru(IN PVOID xhciExtension,
              IN PVOID passThruParameters,
              IN ULONG ParameterLength,
              IN PVOID pParameters)
{
    DPRINT1("XHCI_PassThru: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

VOID
NTAPI
XHCI_RebalanceEndpoint(IN PVOID ohciExtension,
                       IN PUSBPORT_ENDPOINT_PROPERTIES endpointParameters,
                       IN PVOID ohciEndpoint)
{
    DPRINT1("XHCI_RebalanceEndpoint: UNIMPLEMENTED. FIXME\n");
}

VOID
NTAPI
XHCI_FlushInterrupts(IN PVOID xhciExtension)
{
    DPRINT1("XHCI_FlushInterrupts: function initiated\n");
}

MPSTATUS
NTAPI
XHCI_RH_ChirpRootPort(IN PVOID xhciExtension,
                      IN USHORT Port)
{
    DPRINT1("XHCI_RH_ChirpRootPort: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

VOID
NTAPI
XHCI_TakePortControl(IN PVOID ohciExtension)
{
    DPRINT1("XHCI_TakePortControl: UNIMPLEMENTED. FIXME\n");
}

VOID
NTAPI
XHCI_Unload(PDRIVER_OBJECT DriverObject)
{
    DPRINT1("XHCI_Unload: UNIMPLEMENTED. FIXME\n");
}

NTSTATUS
NTAPI
DriverEntry(IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegistryPath)
{
    DPRINT1("DriverEntry: DriverObject - %p, RegistryPath - %wZ\n",
           DriverObject,
           RegistryPath);
    if (USBPORT_GetHciMn() != USBPORT_HCI_MN) 
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlZeroMemory(&RegPacket, sizeof(USBPORT_REGISTRATION_PACKET));
    
    RegPacket.MiniPortVersion = USB_MINIPORT_VERSION_XHCI;
    /* FIXE: USB_MINIPORT_FLAGS_USB2 on a USB3 driver? */
    RegPacket.MiniPortFlags = USB_MINIPORT_FLAGS_INTERRUPT |
                              USB_MINIPORT_FLAGS_MEMORY_IO |
                              USB_MINIPORT_FLAGS_USB2 |
                              USB_MINIPORT_FLAGS_POLLING |
                              USB_MINIPORT_FLAGS_WAKE_SUPPORT;

    RegPacket.MiniPortBusBandwidth = TOTAL_USB30_BUS_BANDWIDTH;

    RegPacket.MiniPortExtensionSize = sizeof(XHCI_EXTENSION);
    RegPacket.MiniPortEndpointSize = sizeof(XHCI_ENDPOINT);
    RegPacket.MiniPortTransferSize = sizeof(XHCI_TRANSFER);
    RegPacket.MiniPortResourcesSize = sizeof(XHCI_HC_RESOURCES);
    
    RegPacket.OpenEndpoint = XHCI_OpenEndpoint;
    RegPacket.ReopenEndpoint = XHCI_ReopenEndpoint;
    RegPacket.QueryEndpointRequirements = XHCI_QueryEndpointRequirements;
    RegPacket.CloseEndpoint = XHCI_CloseEndpoint;
    RegPacket.StartController = XHCI_StartController;
    RegPacket.StopController = XHCI_StopController;
    RegPacket.SuspendController = XHCI_SuspendController;
    RegPacket.ResumeController = XHCI_ResumeController;
    RegPacket.InterruptService = XHCI_InterruptService;
    RegPacket.InterruptDpc = XHCI_InterruptDpc;
    RegPacket.SubmitTransfer = XHCI_SubmitTransfer;
    RegPacket.SubmitIsoTransfer = XHCI_SubmitIsoTransfer;
    RegPacket.AbortTransfer = XHCI_AbortTransfer;
    RegPacket.GetEndpointState = XHCI_GetEndpointState;
    RegPacket.SetEndpointState = XHCI_SetEndpointState;
    RegPacket.PollEndpoint = XHCI_PollEndpoint;
    RegPacket.CheckController = XHCI_CheckController;
    RegPacket.Get32BitFrameNumber = XHCI_Get32BitFrameNumber;
    RegPacket.InterruptNextSOF = XHCI_InterruptNextSOF;
    RegPacket.EnableInterrupts = XHCI_EnableInterrupts;
    RegPacket.DisableInterrupts = XHCI_DisableInterrupts;
    RegPacket.PollController = XHCI_PollController;
    RegPacket.SetEndpointDataToggle = XHCI_SetEndpointDataToggle;
    RegPacket.GetEndpointStatus = XHCI_GetEndpointStatus;
    RegPacket.SetEndpointStatus = XHCI_SetEndpointStatus;
    RegPacket.RH_GetRootHubData = XHCI_RH_GetRootHubData;
    RegPacket.RH_GetStatus = XHCI_RH_GetStatus;
    RegPacket.RH_GetPortStatus = XHCI_RH_GetPortStatus;
    RegPacket.RH_GetHubStatus = XHCI_RH_GetHubStatus;
    RegPacket.RH_SetFeaturePortReset = XHCI_RH_SetFeaturePortReset;
    RegPacket.RH_SetFeaturePortPower = XHCI_RH_SetFeaturePortPower;
    RegPacket.RH_SetFeaturePortEnable = XHCI_RH_SetFeaturePortEnable;
    RegPacket.RH_SetFeaturePortSuspend = XHCI_RH_SetFeaturePortSuspend;
    RegPacket.RH_ClearFeaturePortEnable = XHCI_RH_ClearFeaturePortEnable;
    RegPacket.RH_ClearFeaturePortPower = XHCI_RH_ClearFeaturePortPower;
    RegPacket.RH_ClearFeaturePortSuspend = XHCI_RH_ClearFeaturePortSuspend;
    RegPacket.RH_ClearFeaturePortEnableChange = XHCI_RH_ClearFeaturePortEnableChange;
    RegPacket.RH_ClearFeaturePortConnectChange = XHCI_RH_ClearFeaturePortConnectChange;
    RegPacket.RH_ClearFeaturePortResetChange = XHCI_RH_ClearFeaturePortResetChange;
    RegPacket.RH_ClearFeaturePortSuspendChange = XHCI_RH_ClearFeaturePortSuspendChange;
    RegPacket.RH_ClearFeaturePortOvercurrentChange = XHCI_RH_ClearFeaturePortOvercurrentChange;
    RegPacket.RH_DisableIrq = XHCI_RH_DisableIrq;
    RegPacket.RH_EnableIrq = XHCI_RH_EnableIrq;
    RegPacket.StartSendOnePacket = XHCI_StartSendOnePacket;
    RegPacket.EndSendOnePacket = XHCI_EndSendOnePacket;
    RegPacket.PassThru = XHCI_PassThru;
    RegPacket.RebalanceEndpoint = XHCI_RebalanceEndpoint;
    RegPacket.FlushInterrupts = XHCI_FlushInterrupts;
    RegPacket.RH_ChirpRootPort = XHCI_RH_ChirpRootPort;
    RegPacket.TakePortControl = XHCI_TakePortControl;
    
    DPRINT1("XHCI_DriverEntry: before driver unload. FIXME\n");
    DriverObject->DriverUnload = XHCI_Unload;
    
    DPRINT1("XHCI_DriverEntry: after driver unload, before usbport_reg call. FIXME\n");

    return USBPORT_RegisterUSBPortDriver(DriverObject, USB30_MINIPORT_INTERFACE_VERSION, &RegPacket);

}