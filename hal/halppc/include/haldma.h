#ifndef HALDMA_H
#define HALDMA_H

/*
 * DMA Page Register Structure
 * 080     DMA        RESERVED
 * 081     DMA        Page Register (channel 2)
 * 082     DMA        Page Register (channel 3)
 * 083     DMA        Page Register (channel 1)
 * 084     DMA        RESERVED
 * 085     DMA        RESERVED
 * 086     DMA        RESERVED
 * 087     DMA        Page Register (channel 0)
 * 088     DMA        RESERVED
 * 089     PS/2-DMA   Page Register (channel 6)
 * 08A     PS/2-DMA   Page Register (channel 7)
 * 08B     PS/2-DMA   Page Register (channel 5)
 * 08C     PS/2-DMA   RESERVED
 * 08D     PS/2-DMA   RESERVED
 * 08E     PS/2-DMA   RESERVED
 * 08F     PS/2-DMA   Page Register (channel 4)
 */

typedef struct _DMA_PAGE
{
   UCHAR Reserved1;
   UCHAR Channel2;
   UCHAR Channel3;
   UCHAR Channel1;
   UCHAR Reserved2[3];
   UCHAR Channel0;
   UCHAR Reserved3;
   UCHAR Channel6;
   UCHAR Channel7;
   UCHAR Channel5;
   UCHAR Reserved4[3];
   UCHAR Channel4;
} DMA_PAGE, *PDMA_PAGE;

/*
 * DMA Channel Mask Register Structure
 *
 * MSB                             LSB
 *       x   x   x   x     x   x   x   x
 *       -------------------   -   -----
 *                |            |     |     00 - Select channel 0 mask bit
 *                |            |     \---- 01 - Select channel 1 mask bit
 *                |            |           10 - Select channel 2 mask bit
 *                |            |           11 - Select channel 3 mask bit
 *                |            |
 *                |            \----------  0 - Clear mask bit
 *                |                         1 - Set mask bit
 *                |
 *                \----------------------- xx - Reserved
 */

typedef struct _DMA_CHANNEL_MASK
{
   UCHAR Channel: 2;
   UCHAR SetMask: 1;
   UCHAR Reserved: 5;
} DMA_CHANNEL_MASK, *PDMA_CHANNEL_MASK;

/*
 * DMA Mask Register Structure
 *
 * MSB                             LSB
 *    x   x   x   x     x   x   x   x
 *    \---/   -   -     -----   -----
 *      |     |   |       |       |     00 - Channel 0 select
 *      |     |   |       |       \---- 01 - Channel 1 select
 *      |     |   |       |             10 - Channel 2 select
 *      |     |   |       |             11 - Channel 3 select
 *      |     |   |       |
 *      |     |   |       |             00 - Verify transfer
 *      |     |   |       \------------ 01 - Write transfer
 *      |     |   |                     10 - Read transfer
 *      |     |   |
 *      |     |   \--------------------  0 - Autoinitialized
 *      |     |                          1 - Non-autoinitialized
 *      |     |
 *      |     \------------------------  0 - Address increment select
 *      |
 *      |                               00 - Demand mode
 *      \------------------------------ 01 - Single mode
 *                                      10 - Block mode
 *                                      11 - Cascade mode
 */

typedef union _DMA_MODE
{
   struct
   {
      UCHAR Channel: 2;
      UCHAR TransferType: 2;
      UCHAR AutoInitialize: 1;
      UCHAR AddressDecrement: 1;
      UCHAR RequestMode: 2;
   };
   UCHAR Byte;
} DMA_MODE, *PDMA_MODE;

/*
 * DMA Extended Mode Register Structure
 *
 * MSB                             LSB
 *    x   x   x   x     x   x   x   x
 *    -   -   -----     -----   -----
 *    |   |     |         |       |     00 - Channel 0 select
 *    |   |     |         |       \---- 01 - Channel 1 select
 *    |   |     |         |             10 - Channel 2 select
 *    |   |     |         |             11 - Channel 3 select
 *    |   |     |         |
 *    |   |     |         |             00 - 8-bit I/O, by bytes
 *    |   |     |         \------------ 01 - 16-bit I/O, by words, address shifted
 *    |   |     |                       10 - 32-bit I/O, by bytes
 *    |   |     |                       11 - 16-bit I/O, by bytes
 *    |   |     |
 *    |   |     \---------------------- 00 - Compatible
 *    |   |                             01 - Type A
 *    |   |                             10 - Type B
 *    |   |                             11 - Burst
 *    |   |
 *    |   \---------------------------- 0 - Terminal Count is Output
 *    |
 *    \---------------------------------0 - Disable Stop Register
 *                                      1 - Enable Stop Register
 */

typedef union _DMA_EXTENDED_MODE
{
   struct
   {
      UCHAR ChannelNumber: 2;
      UCHAR TransferSize: 2;
      UCHAR TimingMode: 2;
      UCHAR TerminalCountIsOutput: 1;
      UCHAR EnableStopRegister: 1;
   };
   UCHAR Byte;
} DMA_EXTENDED_MODE, *PDMA_EXTENDED_MODE;

/* DMA Extended Mode Register Transfer Sizes */
#define B_8BITS 0
#define W_16BITS 1
#define B_32BITS 2
#define B_16BITS 3

/* DMA Extended Mode Register Timing */
#define COMPATIBLE_TIMING 0
#define TYPE_A_TIMING 1
#define TYPE_B_TIMING 2
#define BURST_TIMING 3

/* Channel Stop Registers for each Channel */
typedef struct _DMA_CHANNEL_STOP
{
   UCHAR ChannelLow;
   UCHAR ChannelMid;
   UCHAR ChannelHigh;
   UCHAR Reserved;
} DMA_CHANNEL_STOP, *PDMA_CHANNEL_STOP;

/* Transfer Types */
#define VERIFY_TRANSFER 0x00
#define READ_TRANSFER 0x01
#define WRITE_TRANSFER 0x02

/* Request Modes */
#define DEMAND_REQUEST_MODE 0x00
#define SINGLE_REQUEST_MODE 0x01
#define BLOCK_REQUEST_MODE 0x02
#define CASCADE_REQUEST_MODE 0x03

#define DMA_SETMASK 4
#define DMA_CLEARMASK 0
#define DMA_READ 4
#define DMA_WRITE 8
#define DMA_SINGLE_TRANSFER 0x40
#define DMA_AUTO_INIT 0x10

typedef struct _DMA1_ADDRESS_COUNT
{
   UCHAR DmaBaseAddress;
   UCHAR DmaBaseCount;
} DMA1_ADDRESS_COUNT, *PDMA1_ADDRESS_COUNT;

typedef struct _DMA2_ADDRESS_COUNT
{
   UCHAR DmaBaseAddress;
   UCHAR Reserved1;
   UCHAR DmaBaseCount;
   UCHAR Reserved2;
} DMA2_ADDRESS_COUNT, *PDMA2_ADDRESS_COUNT;

typedef struct _DMA1_CONTROL
{
   DMA1_ADDRESS_COUNT DmaAddressCount[4];
   UCHAR DmaStatus;
   UCHAR DmaRequest;
   UCHAR SingleMask;
   UCHAR Mode;
   UCHAR ClearBytePointer;
   UCHAR MasterClear;
   UCHAR ClearMask;
   UCHAR AllMask;
} DMA1_CONTROL, *PDMA1_CONTROL;

typedef struct _DMA2_CONTROL
{
   DMA2_ADDRESS_COUNT DmaAddressCount[4];
   UCHAR DmaStatus;
   UCHAR Reserved1;
   UCHAR DmaRequest;
   UCHAR Reserved2;
   UCHAR SingleMask;
   UCHAR Reserved3;
   UCHAR Mode;
   UCHAR Reserved4;
   UCHAR ClearBytePointer;
   UCHAR Reserved5;
   UCHAR MasterClear;
   UCHAR Reserved6;
   UCHAR ClearMask;
   UCHAR Reserved7;
   UCHAR AllMask;
   UCHAR Reserved8;
} DMA2_CONTROL, *PDMA2_CONTROL;

/* This structure defines the I/O Map of the 82537 controller. */
typedef struct _EISA_CONTROL
{
   /* DMA Controller 1 */
   DMA1_CONTROL DmaController1;         /* 00h-0Fh */
   UCHAR Reserved1[16];                 /* 0Fh-1Fh */

   /* Interrupt Controller 1 (PIC) */
   UCHAR Pic1Operation;                 /* 20h     */
   UCHAR Pic1Interrupt;                 /* 21h     */
   UCHAR Reserved2[30];                 /* 22h-3Fh */

   /* Timer */
   UCHAR TimerCounter;                  /* 40h     */
   UCHAR TimerMemoryRefresh;            /* 41h     */
   UCHAR Speaker;                       /* 42h     */
   UCHAR TimerOperation;                /* 43h     */
   UCHAR TimerMisc;                     /* 44h     */
   UCHAR Reserved3[2];                  /* 45-46h  */
   UCHAR TimerCounterControl;           /* 47h     */
   UCHAR TimerFailSafeCounter;          /* 48h     */
   UCHAR Reserved4;                     /* 49h     */
   UCHAR TimerCounter2;                 /* 4Ah     */
   UCHAR TimerOperation2;               /* 4Bh     */
   UCHAR Reserved5[20];                 /* 4Ch-5Fh */

   /* NMI / Keyboard / RTC */
   UCHAR Keyboard;                      /* 60h     */
   UCHAR NmiStatus;                     /* 61h     */
   UCHAR Reserved6[14];                 /* 62h-6Fh */
   UCHAR NmiEnable;                     /* 70h     */
   UCHAR Reserved7[15];                 /* 71h-7Fh */

   /* DMA Page Registers Controller 1 */
   DMA_PAGE DmaController1Pages;        /* 80h-8Fh */
   UCHAR Reserved8[16];                 /* 90h-9Fh */

    /* Interrupt Controller 2 (PIC) */
   UCHAR Pic2Operation;                 /* 0A0h      */
   UCHAR Pic2Interrupt;                 /* 0A1h      */
   UCHAR Reserved9[30];                 /* 0A2h-0BFh */

   /* DMA Controller 2 */
   DMA1_CONTROL DmaController2;         /* 0C0h-0CFh */

   /* System Reserved Ports */
   UCHAR SystemReserved[816];           /* 0D0h-3FFh */

   /* Extended DMA Registers, Controller 1 */
   UCHAR DmaHighByteCount1[8];          /* 400h-407h */
   UCHAR Reserved10[2];                 /* 408h-409h */
   UCHAR DmaChainMode1;                 /* 40Ah      */
   UCHAR DmaExtendedMode1;              /* 40Bh      */
   UCHAR DmaBufferControl;              /* 40Ch      */
   UCHAR Reserved11[84];                /* 40Dh-460h */
   UCHAR ExtendedNmiControl;            /* 461h      */
   UCHAR NmiCommand;                    /* 462h      */
   UCHAR Reserved12;                    /* 463h      */
   UCHAR BusMaster;                     /* 464h      */
   UCHAR Reserved13[27];                /* 465h-47Fh */

   /* DMA Page Registers Controller 2 */
   DMA_PAGE DmaController2Pages;        /* 480h-48Fh */
   UCHAR Reserved14[48];                /* 490h-4BFh */

   /* Extended DMA Registers, Controller 2 */
   UCHAR DmaHighByteCount2[16];         /* 4C0h-4CFh */

   /* Edge/Level Control Registers */
   UCHAR Pic1EdgeLevel;                 /* 4D0h      */
   UCHAR Pic2EdgeLevel;                 /* 4D1h      */
   UCHAR Reserved15[2];                 /* 4D2h-4D3h */

   /* Extended DMA Registers, Controller 2 */
   UCHAR DmaChainMode2;                 /* 4D4h      */
   UCHAR Reserved16;                    /* 4D5h      */
   UCHAR DmaExtendedMode2;              /* 4D6h      */
   UCHAR Reserved17[9];                 /* 4D7h-4DFh */

   /* DMA Stop Registers */
   DMA_CHANNEL_STOP DmaChannelStop[8];  /* 4E0h-4FFh */
} EISA_CONTROL, *PEISA_CONTROL;

typedef struct _ROS_MAP_REGISTER_ENTRY
{
   PVOID VirtualAddress;
   PHYSICAL_ADDRESS PhysicalAddress;
   ULONG Counter;
} ROS_MAP_REGISTER_ENTRY, *PROS_MAP_REGISTER_ENTRY;

struct _ADAPTER_OBJECT {
   /*
    * New style DMA object definition. The fact that it is at the beginning
    * of the ADAPTER_OBJECT structure allows us to easily implement the
    * fallback implementation of IoGetDmaAdapter.
    */
   DMA_ADAPTER DmaHeader;

   /*
    * For normal adapter objects pointer to master adapter that takes care
    * of channel allocation. For master adapter set to NULL.
    */
   struct _ADAPTER_OBJECT *MasterAdapter;

   ULONG MapRegistersPerChannel;
   PVOID AdapterBaseVa;
   PROS_MAP_REGISTER_ENTRY MapRegisterBase;

   ULONG NumberOfMapRegisters;
   ULONG CommittedMapRegisters;

   PWAIT_CONTEXT_BLOCK CurrentWcb;
   KDEVICE_QUEUE ChannelWaitQueue;
   PKDEVICE_QUEUE RegisterWaitQueue;
   LIST_ENTRY AdapterQueue;
   KSPIN_LOCK SpinLock;
   PRTL_BITMAP MapRegisters;
   PUCHAR PagePort;
   UCHAR ChannelNumber;
   UCHAR AdapterNumber;
   USHORT DmaPortAddress;
   DMA_MODE AdapterMode;
   BOOLEAN NeedsMapRegisters;
   BOOLEAN MasterDevice;
   BOOLEAN Width16Bits;
   BOOLEAN ScatterGather;
   BOOLEAN IgnoreCount;
   BOOLEAN Dma32BitAddresses;
   BOOLEAN Dma64BitAddresses;
   LIST_ENTRY AdapterList;
} ADAPTER_OBJECT;

typedef struct _GROW_WORK_ITEM {
   WORK_QUEUE_ITEM WorkQueueItem;
   PADAPTER_OBJECT AdapterObject;
   ULONG NumberOfMapRegisters;
} GROW_WORK_ITEM, *PGROW_WORK_ITEM;

#define MAP_BASE_SW_SG 1

PADAPTER_OBJECT NTAPI
HalpDmaAllocateMasterAdapter(VOID);

PDMA_ADAPTER NTAPI
HalpGetDmaAdapter(
   IN PVOID Context,
   IN PDEVICE_DESCRIPTION DeviceDescription,
   OUT PULONG NumberOfMapRegisters);

ULONG NTAPI
HalpDmaGetDmaAlignment(
   PADAPTER_OBJECT AdapterObject);

#endif /* HALDMA_H */
