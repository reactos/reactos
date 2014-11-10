#pragma once

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
//  Frame interval register (section 7.3.1)
//
#define OHCI_FRAME_INTERVAL_OFFSET                 0x34
#define OHCI_GET_INTERVAL_VALUE(s)          ((s) & 0x3fff)
#define OHCI_GET_FS_LARGEST_DATA_PACKET(s)  (((s) >> 16) & 0x7fff)
#define OHCI_FRAME_INTERVAL_TOGGLE          0x80000000

//
// frame interval
//
#define OHCI_FRAME_INTERVAL_NUMBER_OFFSET          0x3C

//
// periodic start register
//
#define OHCI_PERIODIC_START_OFFSET             0x40
#define OHCI_PERIODIC(i)            ((i) * 9 / 10)

//
//  Root Hub Descriptor B register (section 7.4.2)
//

#define OHCI_RH_DESCRIPTOR_B_OFFSET        0x4c

//
//  Root Hub status register (section 7.4.3)
//
#define OHCI_RH_STATUS_OFFSET                          0x50
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
                                    | OHCI_ROOT_HUB_STATUS_CHANGE \
									| OHCI_OWNERSHIP_CHANGE)

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


typedef struct _OHCI_ENDPOINT_DESCRIPTOR
{
    // Hardware part
    ULONG  Flags;
    ULONG  TailPhysicalDescriptor;
    ULONG  HeadPhysicalDescriptor;
    ULONG  NextPhysicalEndpoint;

    // Software part
    PHYSICAL_ADDRESS  PhysicalAddress;
    PVOID HeadLogicalDescriptor;
    PVOID NextDescriptor;
    PVOID Request;
    LIST_ENTRY DescriptorListEntry;
}OHCI_ENDPOINT_DESCRIPTOR, *POHCI_ENDPOINT_DESCRIPTOR;


#define OHCI_ENDPOINT_SKIP                      0x00004000
#define OHCI_ENDPOINT_SET_DEVICE_ADDRESS(s)     (s)
#define OHCI_ENDPOINT_GET_DEVICE_ADDRESS(s)     ((s) & 0xFF)
#define OHCI_ENDPOINT_GET_ENDPOINT_NUMBER(s)    (((s) >> 7) & 0xf)
#define OHCI_ENDPOINT_SET_ENDPOINT_NUMBER(s)    ((s) << 7)
#define OHCI_ENDPOINT_GET_MAX_PACKET_SIZE(s)    (((s) >> 16) & 0x07ff)
#define OHCI_ENDPOINT_SET_MAX_PACKET_SIZE(s)    ((s) << 16)
#define OHCI_ENDPOINT_LOW_SPEED                 0x00002000
#define OHCI_ENDPOINT_FULL_SPEED                0x00000000
#define OHCI_ENDPOINT_DIRECTION_OUT             0x00000800
#define OHCI_ENDPOINT_DIRECTION_IN              0x00001000
#define OHCI_ENDPOINT_GENERAL_FORMAT            0x00000000
#define OHCI_ENDPOINT_ISOCHRONOUS_FORMAT        0x00008000
#define	OHCI_ENDPOINT_HEAD_MASK                 0xfffffffc
#define	OHCI_ENDPOINT_HALTED                    0x00000001
#define	OHCI_ENDPOINT_TOGGLE_CARRY              0x00000002
#define	OHCI_ENDPOINT_DIRECTION_DESCRIPTOR      0x00000000

//
// Maximum port count set by OHCI
//
#define OHCI_MAX_PORT_COUNT             15


typedef struct
{
    ULONG PortStatus;
    ULONG PortChange;
}OHCI_PORT_STATUS;


typedef struct
{
    // Hardware part 16 bytes
    ULONG Flags;                      // Flags field
    ULONG  BufferPhysical;            // Physical buffer pointer
    ULONG  NextPhysicalDescriptor;   // Physical pointer next descriptor
    ULONG LastPhysicalByteAddress; // Physical pointer to buffer end
    // Software part
    PHYSICAL_ADDRESS  PhysicalAddress;           // Physical address of this descriptor
    PVOID NextLogicalDescriptor;
    ULONG  BufferSize;                // Size of the buffer
    PVOID    BufferLogical;            // Logical pointer to the buffer
}OHCI_GENERAL_TD, *POHCI_GENERAL_TD;


#define OHCI_TD_BUFFER_ROUNDING         0x00040000
#define OHCI_TD_DIRECTION_PID_MASK      0x00180000
#define OHCI_TD_DIRECTION_PID_SETUP     0x00000000
#define OHCI_TD_DIRECTION_PID_OUT       0x00080000
#define OHCI_TD_DIRECTION_PID_IN        0x00100000
#define OHCI_TD_GET_DELAY_INTERRUPT(x)  (((x) >> 21) & 7)
#define OHCI_TD_SET_DELAY_INTERRUPT(x)  ((x) << 21)
#define OHCI_TD_INTERRUPT_MASK          0x00e00000
#define OHCI_TD_TOGGLE_CARRY            0x00000000
#define OHCI_TD_TOGGLE_0                0x02000000
#define OHCI_TD_TOGGLE_1                0x03000000
#define OHCI_TD_TOGGLE_MASK             0x03000000
#define OHCI_TD_GET_ERROR_COUNT(x)      (((x) >> 26) & 3)
#define OHCI_TD_GET_CONDITION_CODE(x)   ((x) >> 28)
#define OHCI_TD_SET_CONDITION_CODE(x)   ((x) << 28)
#define OHCI_TD_CONDITION_CODE_MASK     0xf0000000

#define OHCI_TD_INTERRUPT_IMMEDIATE         0x00
#define OHCI_TD_INTERRUPT_NONE              0x07

#define OHCI_TD_CONDITION_NO_ERROR          0x00
#define OHCI_TD_CONDITION_CRC_ERROR         0x01
#define OHCI_TD_CONDITION_BIT_STUFFING      0x02
#define OHCI_TD_CONDITION_TOGGLE_MISMATCH   0x03
#define OHCI_TD_CONDITION_STALL             0x04
#define OHCI_TD_CONDITION_NO_RESPONSE       0x05
#define OHCI_TD_CONDITION_PID_CHECK_FAILURE 0x06
#define OHCI_TD_CONDITION_UNEXPECTED_PID    0x07
#define OHCI_TD_CONDITION_DATA_OVERRUN      0x08
#define OHCI_TD_CONDITION_DATA_UNDERRUN     0x09
#define OHCI_TD_CONDITION_BUFFER_OVERRUN    0x0c
#define OHCI_TD_CONDITION_BUFFER_UNDERRUN   0x0d
#define OHCI_TD_CONDITION_NOT_ACCESSED      0x0f

// --------------------------------
//  Isochronous transfer descriptor structure (section 4.3.2)
// --------------------------------

#define OHCI_ITD_NOFFSET 8

typedef struct _OHCI_ISO_TD_
{

    // Hardware part 32 byte
    ULONG Flags;
    ULONG BufferPhysical;                       // Physical page number of byte 0
    ULONG NextPhysicalDescriptor;               // Next isochronous transfer descriptor
    ULONG LastPhysicalByteAddress;              // Physical buffer end
    USHORT Offset[OHCI_ITD_NOFFSET];             // Buffer offsets

    // Software part
    PHYSICAL_ADDRESS PhysicalAddress;             // Physical address of this descriptor
    struct _OHCI_ISO_TD_ * NextLogicalDescriptor; // Logical pointer next descriptor
}OHCI_ISO_TD, *POHCI_ISO_TD;

C_ASSERT(FIELD_OFFSET(OHCI_ISO_TD, Flags) == 0);
C_ASSERT(FIELD_OFFSET(OHCI_ISO_TD, BufferPhysical) == 4);
C_ASSERT(FIELD_OFFSET(OHCI_ISO_TD, NextPhysicalDescriptor) == 8);
C_ASSERT(FIELD_OFFSET(OHCI_ISO_TD, LastPhysicalByteAddress) == 12);
C_ASSERT(FIELD_OFFSET(OHCI_ISO_TD, Offset) == 16);
C_ASSERT(FIELD_OFFSET(OHCI_ISO_TD, PhysicalAddress) == 32);
C_ASSERT(FIELD_OFFSET(OHCI_ISO_TD, NextLogicalDescriptor) == 40);
C_ASSERT(sizeof(OHCI_ISO_TD) == 48);

#define OHCI_ITD_GET_STARTING_FRAME(x)          ((x) & 0x0000ffff)
#define OHCI_ITD_SET_STARTING_FRAME(x)          ((x) & 0xffff)
#define OHCI_ITD_GET_DELAY_INTERRUPT(x)         (((x) >> 21) & 7)
#define OHCI_ITD_SET_DELAY_INTERRUPT(x)         ((x) << 21)
#define OHCI_ITD_NO_INTERRUPT                   0x00e00000
#define OHCI_ITD_GET_FRAME_COUNT(x)             ((((x) >> 24) & 7) + 1)
#define OHCI_ITD_SET_FRAME_COUNT(x)             (((x) - 1) << 24)
#define OHCI_ITD_GET_CONDITION_CODE(x)          ((x) >> 28)
#define OHCI_ITD_NO_CONDITION_CODE              0xf0000000
