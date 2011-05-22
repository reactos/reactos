#pragma once

#include <ntddk.h>

//
// OHCI Operational Registers
//

#define OHCI_REVISION_OFFSET               (0x00)
#define OHCI_REVISION_LOW(rev)             ((rev) & 0x0f)
#define OHCI_REVISION_HIGH(rev)            (((rev) >> 4) & 0x03)


//
// OHCI Control Register
//
#define OHCI_CONTROL_OFFSET                     (0x004)
#define OHCI_CONTROL_BULK_SERVICE_RATIO_MASK    (0x003)
#define OHCI_CONTROL_BULK_RATIO_1_1             (0x000)
#define OHCI_CONTROL_BULK_RATIO_1_2             (0x001)
#define OHCI_CONTROL_BULK_RATIO_1_3             (0x002)
#define OHCI_CONTROL_BULK_RATIO_1_4             (0x003)
#define OHCI_PERIODIC_LIST_ENABLE               (0x004)
#define OHCI_ISOCHRONOUS_ENABLE                 (0x008)
#define OHCI_CONTROL_LIST_ENABLE                (0x010)
#define OHCI_BULK_LIST_ENABLE                   (0x020)
#define OHCI_HC_FUNCTIONAL_STATE_MASK           (0x0C0)
#define OHCI_HC_FUNCTIONAL_STATE_RESET          (0x000)
#define OHCI_HC_FUNCTIONAL_STATE_RESUME         (0x040)
#define OHCI_HC_FUNCTIONAL_STATE_OPERATIONAL    (0x080)
#define OHCI_HC_FUNCTIONAL_STATE_SUSPEND        (0x0c0)
#define OHCI_INTERRUPT_ROUTING                  (0x100)
#define OHCI_REMOTE_WAKEUP_CONNECTED            (0x200)
#define OHCI_REMORE_WAKEUP_ENABLED              (0x400)

//
// OHCI Command Status Register
//
#define OHCI_COMMAND_STATUS_OFFSET              (0x08)
#define OHCI_HOST_CONTROLLER_RESET              0x00000001
#define OHCI_CONTROL_LIST_FILLED                0x00000002
#define OHCI_BULK_LIST_FILLED                   0x00000004
#define OHCI_OWNERSHIP_CHANGE_REQUEST           0x00000008
#define OHCI_SCHEDULING_OVERRUN_COUNT_MASK      0x00030000


//
// OHCI Interrupt Status Register
//
#define OHCI_INTERRUPT_STATUS_OFFSET          0x0c
#define OHCI_SCHEDULING_OVERRUN         0x00000001
#define OHCI_WRITEBACK_DONE_HEAD        0x00000002
#define OHCI_START_OF_FRAME             0x00000004
#define OHCI_RESUME_DETECTED            0x00000008
#define OHCI_UNRECOVERABLE_ERROR        0x00000010
#define OHCI_FRAME_NUMBER_OVERFLOW      0x00000020
#define OHCI_ROOT_HUB_STATUS_CHANGE     0x00000040
#define OHCI_OWNERSHIP_CHANGE           0x40000000
#define OHCI_MASTER_INTERRUPT_ENABLE    0x80000000


//
// OHCI Interrupt Enable Register
//
#define OHCI_INTERRUPT_ENABLE_OFFSET       0x10

//
// OHCI Interrupt Enable Register
//
#define OHCI_INTERRUPT_DISABLE_OFFSET      0x14

//
// OHCI HCCA Register
//
#define OHCI_HCCA_OFFSET                          0x18
#define OHCI_PERIOD_CURRENT_ED_OFFSET             0x1c
#define OHCI_CONTROL_HEAD_ED_OFFSET               0x20
#define OHCI_CONTROL_CURRENT_ED_OFFSET            0x24
#define OHCI_BULK_HEAD_ED_OFFSET                  0x28

//
// OHCI Root Hub Descriptor A register
//
#define OHCI_RH_DESCRIPTOR_A_OFFSET                 0x48
#define OHCI_RH_GET_PORT_COUNT(s)                   ((s) & 0xff)
#define OHCI_RH_POWER_SWITCHING_MODE                0x0100
#define OHCI_RH_NO_POWER_SWITCHING                  0x0200
#define OHCI_RH_DEVICE_TYPE                         0x0400
#define OHCI_RH_OVER_CURRENT_PROTECTION_MODE        0x0800
#define OHCI_RH_NO_OVER_CURRENT_PROTECTION          0x1000
#define OHCI_RH_GET_POWER_ON_TO_POWER_GOOD_TIME(s)  ((s) >> 24)


//
//  Root Hub Descriptor B register (section 7.4.2)
//

#define OHCI_RH_DESCRIPTOR_B        0x4c

//
//  Root Hub status register (section 7.4.3)
//
#define OHCI_RH_STATUS                          0x50
#define OHCI_RH_LOCAL_POWER_STATUS              0x00000001
#define OHCI_RH_OVER_CURRENT_INDICATOR          0x00000002
#define OHCI_RH_DEVICE_REMOTE_WAKEUP_ENABLE     0x00008000
#define OHCI_RH_LOCAL_POWER_STATUS_CHANGE       0x00010000
#define OHCI_RH_OVER_CURRENT_INDICATOR_CHANGE   0x00020000
#define OHCI_RH_CLEAR_REMOTE_WAKEUP_ENABLE      0x80000000

//
//  Root Hub port status (n) register (section 7.4.4)
//
#define OHCI_RH_PORT_STATUS(n)      (0x54 + (n) * 4)// 0 based indexing
#define OHCI_RH_PORTSTATUS_CCS      0x00000001
#define OHCI_RH_PORTSTATUS_PES      0x00000002
#define OHCI_RH_PORTSTATUS_PSS      0x00000004
#define OHCI_RH_PORTSTATUS_POCI     0x00000008
#define OHCI_RH_PORTSTATUS_PRS      0x00000010
#define OHCI_RH_PORTSTATUS_PPS      0x00000100
#define OHCI_RH_PORTSTATUS_LSDA     0x00000200
#define OHCI_RH_PORTSTATUS_CSC      0x00010000
#define OHCI_RH_PORTSTATUS_PESC     0x00020000
#define OHCI_RH_PORTSTATUS_PSSC     0x00040000
#define OHCI_RH_PORTSTATUS_OCIC     0x00080000
#define OHCI_RH_PORTSTATUS_PRSC     0x00100000

//
//  Enable List
//

#define OHCI_ENABLE_LIST        (OHCI_PERIODIC_LIST_ENABLE \
                                | OHCI_ISOCHRONOUS_ENABLE \
                                | OHCI_CONTROL_LIST_ENABLE \
                                | OHCI_BULK_LIST_ENABLE)

//
//  All interupts
// 
#define OHCI_ALL_INTERRUPTS     (OHCI_SCHEDULING_OVERRUN \
                                | OHCI_WRITEBACK_DONE_HEAD \
                                | OHCI_START_OF_FRAME \
                                | OHCI_RESUME_DETECTED \
                                | OHCI_UNRECOVERABLE_ERROR \
                                | OHCI_FRAME_NUMBER_OVERFLOW \
                                | OHCI_ROOT_HUB_STATUS_CHANGE \
                                | OHCI_OWNERSHIP_CHANGE)

//
//  All normal interupts
//
#define OHCI_NORMAL_INTERRUPTS      (OHCI_SCHEDULING_OVERRUN \
                                    | OHCI_WRITEBACK_DONE_HEAD \
                                    | OHCI_RESUME_DETECTED \
                                    | OHCI_UNRECOVERABLE_ERROR \
                                    | OHCI_ROOT_HUB_STATUS_CHANGE)

//
// FSMPS
//

#define OHCI_FSMPS(i)               (((i - 210) * 6 / 7) << 16)

//
//  Periodic
//

#define OHCI_PERIODIC(i)            ((i) * 9 / 10)

// --------------------------------
//  HCCA structure (section 4.4)
//  256 bytes aligned
// --------------------------------

#define OHCI_NUMBER_OF_INTERRUPTS   32
#define OHCI_STATIC_ENDPOINT_COUNT  6
#define OHCI_BIGGEST_INTERVAL       32

typedef struct
{
    ULONG      InterruptTable[OHCI_NUMBER_OF_INTERRUPTS];
    ULONG      CurrentFrameNumber;
    ULONG      DoneHead;
    UCHAR      Reserved[120];
}OHCIHCCA, *POHCIHCCA;

#define OHCI_DONE_INTERRUPTS        1
#define OHCI_HCCA_SIZE              256
#define OHCI_HCCA_ALIGN             256
#define OHCI_PAGE_SIZE              0x1000
#define OHCI_PAGE(x)                ((x) &~ 0xfff)
#define OHCI_PAGE_OFFSET(x)         ((x) & 0xfff)


typedef struct 
{
    // Hardware part
    ULONG  Flags;
    ULONG  TailPhysicalDescriptor;
    ULONG  HeadPhysicalDescriptor;
    ULONG  NextPhysicalEndpoint;

    // Software part
    PHYSICAL_ADDRESS  PhysicalAddress;
}OHCI_ENDPOINT_DESCRIPTOR, *POHCI_ENDPOINT_DESCRIPTOR;


#define OHCI_ENDPOINT_SKIP                      0x00004000

//
// Maximum port count set by OHCI
//
#define OHCI_MAX_PORT_COUNT             15