/*
 * 
 */

#ifndef __INTERNAL_HAL_HAL_H
#define __INTERNAL_HAL_HAL_H

#define HAL_APC_REQUEST	    0
#define HAL_DPC_REQUEST	    1

/* display.c */
VOID FASTCALL HalInitializeDisplay (PLOADER_PARAMETER_BLOCK LoaderBlock);
VOID FASTCALL HalClearDisplay (UCHAR CharAttribute);

/* adapter.c */
PADAPTER_OBJECT STDCALL HalpAllocateAdapterEx(ULONG NumberOfMapRegisters,BOOLEAN IsMaster, BOOLEAN Dma32BitAddresses);
  
/* bus.c */
VOID HalpInitBusHandlers (VOID);

/* irql.c */
VOID HalpInitPICs(VOID);

/* udelay.c */
VOID HalpCalibrateStallExecution(VOID);

/* pci.c */
VOID HalpInitPciBus (VOID);

/* enum.c */
VOID HalpStartEnumerator (VOID);

/* dma.c */
VOID HalpInitDma (VOID);

/* mem.c */
PVOID HalpMapPhysMemory(ULONG PhysAddr, ULONG Size);

/* Non-generic initialization */
VOID HalpInitPhase0 (VOID);

/* DMA Page Register Structure  
 080     DMA        RESERVED
 081     DMA        Page Register (channel 2)
 082     DMA        Page Register (channel 3)
 083     DMA        Page Register (channel 1)
 084     DMA        RESERVED
 085     DMA        RESERVED
 086     DMA        RESERVED
 087     DMA        Page Register (channel 0)
 088     DMA        RESERVED
 089     PS/2-DMA   Page Register (channel 6)
 08A     PS/2-DMA   Page Register (channel 7)
 08B     PS/2-DMA   Page Register (channel 5)
 08C     PS/2-DMA   RESERVED
 08D     PS/2-DMA   RESERVED
 08E     PS/2-DMA   RESERVED
 08F     PS/2-DMA   Page Register (channel 4)
*/
 typedef struct _DMA_PAGE{
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

/* DMA Channel Mask Register Structure

MSB                             LSB
      x   x   x   x     x   x   x   x
      -------------------   -   -----
               |            |     |     00 - Select channel 0 mask bit
               |            |     \---- 01 - Select channel 1 mask bit
               |            |           10 - Select channel 2 mask bit
               |            |           11 - Select channel 3 mask bit
               |            |
               |            \----------  0 - Clear mask bit
               |                         1 - Set mask bit
               |
               \----------------------- xx - Reserved
*/
typedef struct _DMA_CHANNEL_MASK {
  UCHAR Channel : 2;
  UCHAR SetMask : 1;
  UCHAR Reserved : 5;
} DMA_CHANNEL_MASK, *PDMA_CHANNEL_MASK;

/* DMA Mask Register Structure 

   MSB                             LSB
      x   x   x   x     x   x   x   x
      \---/   -   -     -----   -----
        |     |   |       |       |     00 - Channel 0 select
        |     |   |       |       \---- 01 - Channel 1 select
        |     |   |       |             10 - Channel 2 select
        |     |   |       |             11 - Channel 3 select
        |     |   |       |
        |     |   |       |             00 - Verify transfer
        |     |   |       \------------ 01 - Write transfer
        |     |   |                     10 - Read transfer
        |     |   |
        |     |   \--------------------  0 - Autoinitialized
        |     |                          1 - Non-autoinitialized
        |     |
        |     \------------------------  0 - Address increment select
        |
        |                               00 - Demand mode
        \------------------------------ 01 - Single mode
                                        10 - Block mode
                                        11 - Cascade mode
*/
typedef struct _DMA_MODE {
  UCHAR Channel : 2;
  UCHAR TransferType : 2;
  UCHAR AutoInitialize : 1;
  UCHAR AddressDecrement : 1;
  UCHAR RequestMode : 2;
} DMA_MODE, *PDMA_MODE;


/* DMA Extended Mode Register Structure 

   MSB                             LSB
      x   x   x   x     x   x   x   x
      -   -   -----     -----   -----
      |   |     |         |       |     00 - Channel 0 select
      |   |     |         |       \---- 01 - Channel 1 select
      |   |     |         |             10 - Channel 2 select
      |   |     |         |             11 - Channel 3 select
      |   |     |         | 
      |   |     |         |             00 - 8-bit I/O, by bytes
      |   |     |         \------------ 01 - 16-bit I/O, by words, address shifted
      |   |     |                       10 - 32-bit I/O, by bytes
      |   |     |                       11 - 16-bit I/O, by bytes
      |   |     |
      |   |     \---------------------- 00 - Compatible
      |   |                             01 - Type A
      |   |                             10 - Type B
      |   |                             11 - Burst
      |   |
      |   \---------------------------- 0 - Terminal Count is Output
      |                                 
      \---------------------------------0 - Disable Stop Register
                                        1 - Enable Stop Register
*/
typedef struct _DMA_EXTENDED_MODE {
    UCHAR ChannelNumber : 2;
    UCHAR TransferSize : 2;
    UCHAR TimingMode : 2;
    UCHAR TerminalCountIsOutput : 1;
    UCHAR EnableStopRegister : 1;
}DMA_EXTENDED_MODE, *PDMA_EXTENDED_MODE;

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
typedef struct _DMA_CHANNEL_STOP {
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

typedef struct _DMA1_ADDRESS_COUNT {
   UCHAR DmaBaseAddress;
   UCHAR DmaBaseCount; 
} DMA1_ADDRESS_COUNT, *PDMA1_ADDRESS_COUNT;

typedef struct _DMA2_ADDRESS_COUNT {
   UCHAR DmaBaseAddress;
   UCHAR Reserved1;
   UCHAR DmaBaseCount; 
   UCHAR Reserved2;
} DMA2_ADDRESS_COUNT, *PDMA2_ADDRESS_COUNT;

typedef struct _DMA1_CONTROL {
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

typedef struct _DMA2_CONTROL {
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

/* This Structure Defines the I/O Map of the 82537 Controller
   I've only defined the registers which are likely to be useful to us */
typedef struct _EISA_CONTROL {
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
   DMA1_CONTROL DmaController2;         /* 0C0h-0DFh */
   
   /* System Reserved Ports */
   UCHAR SystemReserved[800];           /* 0E0h-3FFh */
   
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

extern ULONG HalpEisaDma;
extern PADAPTER_OBJECT MasterAdapter;

EXPORTED ULONG HalpEisaDma;
EXPORTED PADAPTER_OBJECT MasterAdapter;

/*
 * ADAPTER_OBJECT - Track a busmaster DMA adapter and its associated resources
 *
 * NOTES:
 *     - I've updated this to the Windows Object Defintion. 
 */
struct _ADAPTER_OBJECT {
  DMA_ADAPTER DmaHeader;
  struct _ADAPTER_OBJECT *MasterAdapter;
  ULONG MapRegistersPerChannel;
  PVOID AdapterBaseVa;
  PVOID MapRegisterBase;
  ULONG NumberOfMapRegisters;
  ULONG CommittedMapRegisters; 
  PWAIT_CONTEXT_BLOCK CurrentWcb;
  KDEVICE_QUEUE ChannelWaitQueue;
  PKDEVICE_QUEUE RegisterWaitQueue;
  LIST_ENTRY AdapterQueue;
  ULONG SpinLock;
  PRTL_BITMAP MapRegisters;
  PUCHAR PagePort;
  UCHAR ChannelNumber;
  UCHAR AdapterNumber;
  USHORT DmaPortAddress;
  UCHAR AdapterMode;
  BOOLEAN  NeedsMapRegisters;
  BOOLEAN MasterDevice;
  UCHAR Width16Bits;
  UCHAR ScatterGather;
  UCHAR IgnoreCount;
  UCHAR Dma32BitAddresses;
  UCHAR Dma64BitAddresses;
  BOOLEAN LegacyAdapter;
  LIST_ENTRY AdapterList;
};
 
/*
struct _ADAPTER_OBJECT {
  INTERFACE_TYPE InterfaceType;
  BOOLEAN Master;
  int Channel;
  PVOID PagePort;
  PVOID CountPort;
  PVOID OffsetPort;
  KSPIN_LOCK SpinLock;
  PVOID Buffer;
  BOOLEAN Inuse;
  ULONG AvailableMapRegisters;
  PVOID MapRegisterBase;
  ULONG AllocatedMapRegisters;
  PWAIT_CONTEXT_BLOCK WaitContextBlock;
  KDEVICE_QUEUE DeviceQueue;
  BOOLEAN ScatterGather;
  BOOLEAN DemandMode;
  BOOLEAN AutoInitialize;
};
*/

/* sysinfo.c */
NTSTATUS STDCALL
HalpQuerySystemInformation(IN HAL_QUERY_INFORMATION_CLASS InformationClass,
			   IN ULONG BufferSize,
			   IN OUT PVOID Buffer,
			   OUT PULONG ReturnedLength);


/* Non-standard functions */
VOID STDCALL
HalReleaseDisplayOwnership();

BOOLEAN STDCALL
HalQueryDisplayOwnership();

#if defined(__GNUC__)
#define Ki386SaveFlags(x)	    __asm__ __volatile__("pushfl ; popl %0":"=g" (x): /* no input */)
#define Ki386RestoreFlags(x)	    __asm__ __volatile__("pushl %0 ; popfl": /* no output */ :"g" (x):"memory")
#define Ki386DisableInterrupts()    __asm__ __volatile__("cli\n\t")
#define Ki386EnableInterrupts()	    __asm__ __volatile__("sti\n\t")
#define Ki386HaltProcessor()	    __asm__ __volatile__("hlt\n\t")
#define Ki386RdTSC(x)		    __asm__ __volatile__("rdtsc\n\t" : "=A" (x.u.LowPart), "=d" (x.u.HighPart));

static inline BYTE Ki386ReadFsByte(ULONG offset)
{
   BYTE b;
   __asm__ __volatile__("movb %%fs:(%1),%0":"=g" (b):"0" (offset));
   return b;
}

static inline VOID Ki386WriteFsByte(ULONG offset, BYTE value)
{
    __asm__ __volatile__("movb %0,%%fs:(%1)"::"r" (value), "r" (offset));
}

#elif defined(_MSC_VER)
#define Ki386SaveFlags(x)	    __asm pushfd  __asm pop x;
#define Ki386RestoreFlags(x)	    __asm push x  __asm popfd;
#define Ki386DisableInterrupts()    __asm cli
#define Ki386EnableInterrupts()	    __asm sti
#define Ki386HaltProcessor()	    __asm hlt
#else
#error Unknown compiler for inline assembler
#endif

typedef struct tagHALP_HOOKS
{
  void (*InitPciBus)(ULONG BusNumber, PBUS_HANDLER BusHandler);
} HALP_HOOKS, *PHALP_HOOKS;

extern HALP_HOOKS HalpHooks;

#endif /* __INTERNAL_HAL_HAL_H */
