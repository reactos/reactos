//
//  Floppy register definitions
//

#define  FLOPPY_REG_DOR        0x0002
#define   FLOPPY_DOR_ENABLE      0x04
#define   FLOPPY_DOR_DMA         0x08
#define   FLOPPY_DOR_MOTOR0      0x10
#define   FLOPPY_DOR_MOTOR1      0x20
#define   FLOPPY_DRIVE0_ON    ( FLOPPY_DOR_ENABLE | FLOPPY_DOR_DMA | FLOPPY_DOR_MOTOR0 )
#define   FLOPPY_DRIVE1_ON    ( FLOPPY_DOR_ENABLE | FLOPPY_DOR_DMA | FLOPPY_DOR_MOTOR1 | 1 )
#define  FLOPPY_REG_MSTAT      0x0004
#define    FLOPPY_MS_DRV0BUSY    0x01
#define    FLOPPY_MS_DRV1BUSY    0x02
#define    FLOPPY_MS_DRV2BUSY    0x04
#define    FLOPPY_MS_DRV3BUSY    0x08
#define    FLOPPY_MS_FDCBUSY     0x10
#define    FLOPPY_MS_DMAMODE     0x20
#define    FLOPPY_MS_DATADIR     0x40
#define    FLOPPY_MS_RDYMASK     0xF0
#define    FLOPPY_MS_DATARDYW    0x80
#define    FLOPPY_MS_DATARDYR    0xC0
#define  FLOPPY_REG_DATA       0x0005
#define  FLOPPY_REG_DIR        0x0007  /* READ ONLY */
#define    FLOPPY_DI_DSKCHNG     0x80
#define  FLOPPY_REG_CCNTL      0x0007  /* WRITE ONLY */

#define  FLOPPY_CMD_RD_TRK       0x02
#define  FLOPPY_CMD_SPEC_CHARS   0x03
#define    FLOPPY_CSC_SRT_SHIFT       4
#define    FLOPPY_CSC_HUT_MASK     0x0f
#define    FLOPPY_CSC_HLT_SHIFT       1
#define    FLOPPY_CSC_NON_DMA      0x01
#define  FLOPPY_CMD_SNS_DRV      0x04
#define  FLOPPY_CMD_WRT_DATA     0x05
#define  FLOPPY_CMD_RD_DATA      0x06
#define  FLOPPY_CMD_RECAL        0x07
#define  FLOPPY_CMD_SNS_INTR     0x08
#define    FLOPPY_ST0_SEEKGD     0x20
#define    FLOPPY_ST0_GDMASK     0xd8
#define  FLOPPY_CMD_WRT_DEL      0x09
#define  FLOPPY_CMD_RD_ID        0x0a
#define  FLOPPY_CMD_RD_DEL       0x0c
#define  FLOPPY_CMD_FMT_TRK      0x0d
#define  FLOPPY_CMD_DUMP_FDC     0x0e
#define  FLOPPY_CMD_SEEK         0x0f
#define  FLOPPY_CMD_VERSION      0x10
#define  FLOPPY_CMD_SCN_EQ       0x11
#define  FLOPPY_CMD_PPND_RW      0x12
#define  FLOPPY_CMD_CFG_FIFO     0x13
#define  FLOPPY_CMD_LCK_FIFO     0x14
#define  FLOPPY_CMD_PARTID       0x18
#define  FLOPPY_CMD_SCN_LE       0x19
#define  FLOPPY_CMD_SCN_GE       0x1d
#define  FLOPPY_CMD_CFG_PWR      0x27
#define  FLOPPY_CMD_SAVE_FDC     0x2e
#define  FLOPPY_CMD_FMT_ISO      0x33
#define  FLOPPY_CMD_DMA_READ     0x46
#define  FLOPPY_CMD_DMA_WRT      0x4a
#define  FLOPPY_CMD_REST_FDC     0x4e
#define  FLOPPY_CMD_DRV_SPEC     0x8e
#define  FLOPPY_CMD_RSEEK_OUT    0x8f
#define  FLOPPY_CMD_ULK_FIFO     0x94
#define  FLOPPY_CMD_RSEEK_IN     0xcf
#define  FLOPPY_CMD_FMT_WRT      0xef

//  Command Code modifiers
#define    FLOPPY_C0M_SK         0x20
#define    FLOPPY_C0M_MFM        0x40
#define    FLOPPY_C0M_MT         0x80
#define    FLOPPY_C1M_DRVMASK    0x03
#define    FLOPPY_C1M_HEAD1      0x04

//  Status code values and masks
#define    FLOPPY_ST0_INVALID    0x80

//  useful command defines
#define  FLOPPY_CMD_READ    (FLOPPY_CMD_RD_DATA | FLOPPY_C0M_SK | FLOPPY_C0M_MFM | FLOPPY_C0M_MT)
#define  FLOPPY_CMD_WRITE   (FLOPPY_CMD_WRT_DATA | FLOPPY_C0M_MFM | FLOPPY_C0M_MT)
#define  FLOPPY_CMD_FORMAT  (FLOPPY_CMD_FMT_TRK | FLOPPY_C0M_MFM)

//
//  HAL floppy register access commands
//
#define FloppyWriteDOR(A, V) (WRITE_PORT_UCHAR((PVOID)(A) + FLOPPY_REG_DOR, (V)))
#define FloppyReadMSTAT(A) (READ_PORT_UCHAR((PVOID)(A) + FLOPPY_REG_MSTAT))
#define FloppyReadDATA(A) (READ_PORT_UCHAR((PVOID)(A) + FLOPPY_REG_DATA))
#define FloppyWriteDATA(A, V) (WRITE_PORT_UCHAR((PVOID)(A) + FLOPPY_REG_DATA, (V)))
#define FloppyReadDIR(A) (READ_PORT_UCHAR((PVOID)(A) + FLOPPY_REG_DIR))
#define FloppyWriteCCNTL(A, V) (WRITE_PORT_UCHAR((PVOID)(A) + FLOPPY_REG_CCNTL, (V)))

typedef struct _FLOPPY_ERROR_THRESHOLDS
{
    /* number of errors to be reached before aborting */
  unsigned int Abort;
    /* maximal number of errors permitted to read an entire track at once */
  unsigned int ReadTrack;  
    /* maximal number of errors before a reset is tried */
  unsigned int Reset;
    /* maximal number of errors before a recalibrate is tried */
  unsigned int Recal;
    /*
     * Threshold for reporting FDC errors to the console.
     * Setting this to zero may flood your screen when using
     * ultra cheap floppies ;-)
     */
  unsigned int Reporting;
} FLOPPY_ERROR_THRESHOLDS;

typedef struct _FLOPPY_MEDIA_TYPE
{
  BYTE SectorSizeCode;
  BYTE MaximumTrack;
  BYTE Heads;
  DWORD SectorsPerTrack;
  ULONG BytesPerSector;
} FLOPPY_MEDIA_TYPE;

extern const FLOPPY_MEDIA_TYPE MediaTypes[];

#define FDP_DEBUG            0x02
#define FDP_SILENT_DCL_CLEAR 0x04
#define FDP_MSG              0x10
#define FDP_BROKEN_DCL       0x20
#define FDP_INVERTED_DCL     0x80

// time to hold reset line low
#define FLOPPY_RESET_TIME          50000
#define FLOPPY_MOTOR_SPINUP_TIME   -15000000
#define FLOPPY_MOTOR_SPINDOWN_TIME -50000000
#define FLOPPY_RECAL_TIMEOUT       -30000000

typedef BOOLEAN (*FloppyIsrStateRoutine)( PCONTROLLER_OBJECT Controller );
typedef PIO_DPC_ROUTINE FloppyDpcStateRoutine;

typedef struct _FLOPPY_DEVICE_EXTENSION
{
  PCONTROLLER_OBJECT Controller;
  CHAR DriveSelect;
  CHAR Cyl;                       // current cylinder
  ULONG MediaType;                // Media type index
} FLOPPY_DEVICE_EXTENSION, *PFLOPPY_DEVICE_EXTENSION;

typedef struct _FLOPPY_CONTROLLER_EXTENSION
{
  PKINTERRUPT Interrupt;
  KSPIN_LOCK SpinLock;
  ULONG Number;
  ULONG PortBase;
  ULONG Vector;
  KEVENT Event;                   // Event set by ISR/DPC to wake DeviceEntry
  PDEVICE_OBJECT Device;          // Pointer to the primary device on this controller 
  PIRP Irp;                       // Current IRP
  CHAR St0;                       // Status registers
  CHAR St1;
  CHAR St2;
  CHAR SectorSizeCode;
  FloppyIsrStateRoutine IsrState; // pointer to state routine handler for ISR
  FloppyDpcStateRoutine DpcState; // pointer to state routine handler for DPC
  CHAR MotorOn;                   // drive select for drive with motor on
  KDPC MotorSpinupDpc;            // DPC for motor spin up time
  KTIMER SpinupTimer;             // Timer for motor spin up time
  KDPC MotorSpindownDpc;          // DPC for motor spin down
  PADAPTER_OBJECT AdapterObject;  // Adapter object for dma
  PVOID MapRegisterBase;
} FLOPPY_CONTROLLER_EXTENSION, *PFLOPPY_CONTROLLER_EXTENSION;

typedef struct _FLOPPY_CONTROLLER_PARAMETERS
{
   ULONG            PortBase;
   ULONG            Vector;
   ULONG            IrqL;
   ULONG            DmaChannel;
   ULONG            SynchronizeIrqL;
   KINTERRUPT_MODE  InterruptMode;
   KAFFINITY        Affinity;
} FLOPPY_CONTROLLER_PARAMETERS, *PFLOPPY_CONTROLLER_PARAMETERS;

#define  FLOPPY_MAX_CONTROLLERS  1

VOID FloppyDpcDetectMedia( PKDPC Dpc,
			   PDEVICE_OBJECT DeviceObject,
			   PIRP Irp,
			   PVOID Context );
VOID FloppyDpcFailIrp( PKDPC Dpc,
		       PDEVICE_OBJECT DeviceObject,
		       PIRP Irp,
		       PVOID Context );

IO_ALLOCATION_ACTION FloppyExecuteReadWrite( PDEVICE_OBJECT DeviceObject,
					     PIRP Irp,
					     PVOID MapRegisterbase,
					     PVOID Context );

IO_ALLOCATION_ACTION FloppyExecuteSpindown( PDEVICE_OBJECT DeviceObject,
					    PIRP Irp,
					    PVOID MapRegisterbase,
					    PVOID Context );

VOID FloppyMotorSpinupDpc( PKDPC Dpc,
			   PVOID Context,
			   PVOID Arg1,
			   PVOID Arg2 );

VOID FloppySeekDpc( PKDPC Dpc,
		    PDEVICE_OBJECT DeviceObject,
		    PIRP Irp,
		    PVOID Context );

VOID FloppyMotorSpindownDpc( PKDPC Dpc,
			     PVOID Context,
			     PVOID Arg1,
			     PVOID Arg2 );

VOID FloppyDpcDetect( PKDPC Dpc,
		      PDEVICE_OBJECT DeviceObject,
		      PIRP Irp,
		      PVOID Context );

VOID FloppyDpcReadWrite( PKDPC Dpc,
			 PDEVICE_OBJECT DeviceObject,
			 PIRP Irp,
			 PVOID Context );

VOID FloppyDpc( PKDPC Dpc,
		PDEVICE_OBJECT DeviceObject,
		PIRP Irp,
		PVOID Context );

BOOLEAN FloppyIsrDetect( PCONTROLLER_OBJECT Controller );

BOOLEAN FloppyIsrReadWrite( PCONTROLLER_OBJECT Controller );

BOOLEAN FloppyIsrUnexpected( PCONTROLLER_OBJECT Controller );

BOOLEAN FloppyIsrDetectMedia( PCONTROLLER_OBJECT Controller );

BOOLEAN FloppyIsrRecal( PCONTROLLER_OBJECT Controller );

BOOLEAN FloppyIsr(PKINTERRUPT Interrupt, PVOID ServiceContext);

IO_ALLOCATION_ACTION FloppyAdapterControl( PDEVICE_OBJECT DeviceObject,
					   PIRP Irp,
					   PVOID MapRegisterBase,
					   PVOID Context );
