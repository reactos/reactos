/*
 * PROJECT:         ReactOS system libraries
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         hardware register definitions
 * PROGRAMMER:      Rama Teja Gampa <ramateja.g@gmail.com>
*/
    
// base io addr register offsets
#define XHCI_HCSP1            1
#define XHCI_HCSP2            2
#define XHCI_HCSP3            3
#define XHCI_HCCP1            4
#define XHCI_DBOFF            5
#define XHCI_RTSOFF           6
#define XHCI_HCCP2            7
// operational register offsets
#define XHCI_USBCMD           0
#define XHCI_USBSTS           1
#define XHCI_PGSZ             2
#define XHCI_DNCTRL           5
#define XHCI_CRCR             6
#define XHCI_DCBAAP           12
#define XHCI_CONFIG           14
#define XHCI_PORTSC           256
// runtime register offsets
#define XHCI_IMAN             8
#define XHCI_IMOD             9
#define XHCI_ERSTSZ           10
#define XHCI_ERSTBA           12
#define XHCI_ERSTDP           14


typedef volatile union _XHCI_CAPLENGHT_INTERFACE_VERSION 
{
    struct 
    {
        ULONG CapabilityRegistersLength                         : 8; 
        ULONG Rsvd                                              : 8; 
        ULONG HostControllerInterfaceVersion                    : 16;
    };
    ULONG AsULONG;
} XHCI_CAPLENGHT_INTERFACE_VERSION;

typedef volatile union _XHCI_HC_STRUCTURAL_PARAMS_1 
{
    struct 
    {
        ULONG NumberOfDeviceSlots     : 8; // MAXSLOTS
        ULONG NumberOfInterrupters    : 11; // MAXINTRS
        ULONG Rsvd                    : 5;
        ULONG NumberOfPorts           : 8; //MAXPORTS
    };
    ULONG AsULONG;
} XHCI_HC_STRUCTURAL_PARAMS_1;

typedef volatile union _XHCI_HC_STRUCTURAL_PARAMS_2 
{
    struct 
    {
        ULONG Ist               : 4; // Isochronous Scheduling Treshold 
        ULONG ERSTMax           : 4; //Even ring segment table max
        ULONG Rsvd              : 13;
        ULONG MaxSPBuffersHi    : 5; //Max Scratchpad buffers high
        ULONG SPR               : 1; //Scratchpad Restore
        ULONG MaxSPBuffersLo    : 5; //Max Scratchpad buffers Low
    };
    ULONG AsULONG;
} XHCI_HC_STRUCTURAL_PARAMS_2;

typedef volatile union _XHCI_HC_STRUCTURAL_PARAMS_3 
{
    struct 
    {
        ULONG U1DeviceExitLatecy : 8;
        ULONG Rsvd               : 8;
        ULONG U2DeviceExitLatecy : 16;
    };
    ULONG AsULONG;
} XHCI_HC_STRUCTURAL_PARAMS_3;

typedef volatile union _XHCI_HC_CAPABILITY_PARAMS_1 
{  // need to comment full forms, pg 291 in xHCI documentation 
    struct
    {
        ULONG AC64               : 1;
        ULONG BNC                : 1;
        ULONG CSZ                : 1;
        ULONG PPC                : 1;
        ULONG PIND               : 1;
        ULONG LHRC               : 1;
        ULONG LTC                : 1;
        ULONG NSS                : 1;
        ULONG PAE                : 1;
        ULONG SPC                : 1;
        ULONG SEC                : 1;
        ULONG CFC                : 1;
        ULONG MaxPSASize         : 4;
        ULONG xECP               : 16;
    };
    ULONG AsULONG;
} XHCI_HC_CAPABILITY_PARAMS_1;

typedef volatile union _XHCI_DOORBELL_OFFSET 
{
    struct 
    {
        ULONG Rsvd               : 2;
        ULONG DBArrayOffset      : 30;
    };
    ULONG AsULONG;
} XHCI_DOORBELL_OFFSET;

typedef volatile union _XHCI_RT_REGISTER_SPACE_OFFSET 
{ //RUNTIME REGISTER SPACE OFFSET
    struct {
        ULONG Rsvd               : 5;
        ULONG RTSOffset          : 27;
    };
    ULONG AsULONG;
} XHCI_RT_REGISTER_SPACE_OFFSET;

typedef volatile union _XHCI_HC_CAPABILITY_PARAMS_2 
{
    struct 
    {
        ULONG U3C                : 1;
        ULONG CMC                : 1;
        ULONG FSC                : 1;
        ULONG CTC                : 1;
        ULONG LEC                : 1;
        ULONG CIC                : 1;
        ULONG Rsvd               : 26;
    };
    ULONG AsULONG;
} XHCI_HC_CAPABILITY_PARAMS_2;

typedef volatile union _XHCI_USB_COMMAND 
{
    struct 
        {
        ULONG RunStop                    : 1;
        ULONG HCReset                    : 1;
        ULONG InterrupterEnable          : 1;
        ULONG HostSystemErrorEnable      : 1;
        ULONG RsvdP1                     : 3;
        ULONG LightHCReset               : 1;
        ULONG ControllerSaveState        : 1; 
        ULONG ControllerRestoreState     : 1; 
        ULONG EnableWrapEvent            : 1;
        ULONG EnableU3Stop               : 1; 
        ULONG RsvdP2                     : 1;
        ULONG CEMEnable                  : 1;
        ULONG RsvdP3                     : 18;
    };
    ULONG AsULONG;
} XHCI_USB_COMMAND;
C_ASSERT(sizeof(XHCI_USB_COMMAND) == sizeof(ULONG));

typedef volatile union _XHCI_USB_STATUS 
{
    struct 
    {
        ULONG HCHalted               : 1;
        ULONG RsvdZ1                 : 1;
        ULONG HostSystemError        : 1;
        ULONG EventInterrupt         : 1;
        ULONG PortChangeDetect       : 1;
        ULONG RsvdZ2                 : 3;
        ULONG SaveStateStatus        : 1; 
        ULONG RestoreStateStatus     : 1; 
        ULONG SaveRestoreError       : 1;
        ULONG ControllerNotReady     : 1; 
        ULONG HCError                : 1;
        ULONG RsvdZ3                 : 19;
    };
    ULONG AsULONG;
} XHCI_USB_STATUS;
C_ASSERT(sizeof(XHCI_USB_STATUS) == sizeof(ULONG));

typedef volatile union _XHCI_PAGE_SIZE 
{ 
    struct 
    {
        ULONG PageSize           : 16;
        ULONG Rsvd               : 16;
    };
    ULONG AsULONG;
} XHCI_PAGE_SIZE;

typedef volatile union _XHCI_DEVICE_NOTIFICATION_CONTROL 
{ 
    struct 
    {
        ULONG NotificationEnable : 16;
        ULONG Rsvd               : 16;
    };
    ULONG AsULONG;
} XHCI_DEVICE_NOTIFICATION_CONTROL;

typedef volatile union _XHCI_COMMAND_RING_CONTROL 
{ 
    struct 
    {
        ULONGLONG RingCycleState           : 1;
        ULONGLONG CommandStop              : 1;
        ULONGLONG CommandAbort             : 1;
        ULONGLONG CommandRingRunning       : 1;
        ULONGLONG RsvdP                    : 2;
        ULONGLONG CommandRingPointerLo     : 26;
        ULONGLONG CommandRingPointerHi     : 32;
    };
    ULONGLONG AsULONGLONG;
} XHCI_COMMAND_RING_CONTROL;

typedef volatile union _XHCI_DEVICE_CONTEXT_BASE_ADD_ARRAY_POINTER 
{ 
    struct 
    {
        ULONGLONG RsvdZ                       : 6;
        ULONGLONG DCBAAPointerLo              : 26;
        ULONGLONG DCBAAPointerHi              : 32;
    };
    ULONGLONG AsULONGLONG;
} XHCI_DEVICE_CONTEXT_BASE_ADD_ARRAY_POINTER;

typedef volatile union _XHCI_CONFIGURE 
{ 
    struct 
    {
        ULONG MaxDeviceSlotsEnabled        : 8;
        ULONG U3EntryEnable                : 1;
        ULONG ConfigurationInfoEnable      : 1;
        ULONG Rsvd                         : 22;
    };
    ULONG AsULONG;
} XHCI_CONFIGURE;
C_ASSERT(sizeof(XHCI_CONFIGURE) == sizeof(ULONG));

#define PORT_STATUS_MASK    0x4F01FFE9  // 0100 1111 0000 0001 1111 1111 1110 1001 // RW 1, RW1C/RW1S 0, RO 1
typedef volatile union _XHCI_PORT_STATUS_CONTROL 
{
    struct 
    {
        ULONG CurrentConnectStatus                  : 1;
        ULONG PortEnableDisable                     : 1;
        ULONG RsvdZ1                                : 1;
        ULONG OverCurrentActive                     : 1;
        ULONG PortReset                             : 1;
        ULONG PortLinkState                         : 4;
        ULONG PortPower                             : 1;
        ULONG PortSpeed                             : 4;
        ULONG PortIndicatorControl                  : 2;
        ULONG LinkWriteStrobe                       : 1;
        ULONG ConnectStatusChange                   : 1;
        ULONG PortEnableDisableChange               : 1;
        ULONG WarmResetChange                       : 1;
        ULONG OverCurrentChange                     : 1;
        ULONG PortResetChange                       : 1;
        ULONG PortLinkStateChange                   : 1;
        ULONG ConfigErrorChange                     : 1;
        ULONG ColdAttachStatus                      : 1;
        ULONG WakeONConnectEnable                   : 1;
        ULONG WakeONDisconnectEnable                : 1;
        ULONG WakeONOverCurrentEnable               : 1;
        ULONG RsvdZ2                                : 2;
        ULONG DeviceRemovable                       : 1;
        ULONG WarmPortReset                         : 1;
    };
    ULONG AsULONG;
} XHCI_PORT_STATUS_CONTROL;

// Interrupt Register Set
typedef volatile union _XHCI_INTERRUPTER_MANAGEMENT 
{
    struct 
    {
        ULONG InterruptPending  : 1;
        ULONG InterruptEnable   : 1;
        ULONG RsvdP             : 30;
    };
    ULONG AsULONG;
} XHCI_INTERRUPTER_MANAGEMENT;

typedef volatile union _XHCI_INTERRUPTER_MODERATION 
{
    struct 
    {
        ULONG InterruptModIterval  : 16;
        ULONG InterruptModCounter  : 16;
    };
    ULONG AsULONG;
} XHCI_INTERRUPTER_MODERATION;

typedef volatile union _XHCI_EVENT_RING_TABLE_SIZE 
{
    struct 
    {
        ULONG EventRingSegTableSize  : 16;
        ULONG RsvdP                  : 16;
    };
    ULONG AsULONG;
} XHCI_EVENT_RING_TABLE_SIZE;

typedef volatile union _XHCI_EVENT_RING_TABLE_BASE_ADDR 
{ 
    struct
    {
        ULONGLONG RsvdP                       : 6;
        ULONGLONG EventRingSegTableBaseAddr   : 58;
    };
    ULONGLONG AsULONGLONG;
} XHCI_EVENT_RING_TABLE_BASE_ADDR;

typedef volatile union _XHCI_EVENT_RING_DEQUEUE_POINTER 
{ 
    struct 
    {
        ULONGLONG DequeueERSTIndex            : 3;
        ULONGLONG EventHandlerBusy            : 1;
        ULONGLONG EventRingSegDequeuePointer  : 60;
    };
    ULONGLONG AsULONGLONG;
} XHCI_EVENT_RING_DEQUEUE_POINTER;

// Doorbell register
typedef volatile union _XHCI_DOORBELL 
{
    struct 
    {
        ULONG DoorBellTarget        : 8;
        ULONG RsvdZ                 : 8;
        ULONG DoorbellStreamID      : 16;
    };
    ULONG AsULONG;
} XHCI_DOORBELL;
