
#define FLOPPY_MAX_STAT_RETRIES 10000

#define FLOPPY_NEEDS_OUTPUT  -2
//
//  Floppy register definitions
//

#define  FLOPPY_REG_DOR        0x0002
#define  FLOPPY_REG_MSTAT      0x0004
#define    FLOPPY_MS_DRV0BUSY    0x01
#define    FLOPPY_MS_DRV1BUSY    0x02
#define    FLOPPY_MS_DRV2BUSY    0x04
#define    FLOPPY_MS_DRV3BUSY    0x08
#define    FLOPPY_MS_FDCBUSY     0x10
#define    FLOPPY_MS_DMAMODE     0x20
#define    FLOPPY_MS_DATADIR     0x40
#define    FLOPPY_MS_DATARDY     0x80
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
#define    FLOPPY_CSI_IC_MASK      0xe0
#define    FLOPPY_CSI_IC_RDYCH     0x60
#define    FLOPPY_CSI_IC_SEEKGD    0x80
#define    FLOPPY_CSI_IC_SEEKBD    0xc0
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
#define FloppyWriteDOR(A, V) (WRITE_BYTE((A) + FLOPPY_REG_DOR, (V)))
#define FloppyReadMSTAT(A) (READ_BYTE((A) + FLOPPY_REG_MSTAT))
#define FloppyReadDATA(A) (READ_BYTE((A) + FLOPPY_REG_DATA))
#define FloppyWriteDATA(A, V) (WRITE_BYTE((A) + FLOPPY_REG_DATA, (V)))
#define FloppyReadDIR(A) (READ_BYTE((A) + FLOPPY_REG_DIR))
#define FloppyWriteCCNTL(A, V) (WRITE_BYTE((A) + FLOPPY_REG_CCNTL, (V)))

//
//  Known Floppy controller types
//
typedef enum _FLOPPY_CONTROLLER_TYPE
{
  FDC_NONE, 
  FDC_UNKNOWN,
  FDC_8272A,       /* Intel 8272a, NEC 765 */
  FDC_765ED,       /* Non-Intel 1MB-compatible FDC, can't detect */
  FDC_82072,       /* Intel 82072; 8272a + FIFO + DUMPREGS */
  FDC_82072A,      /* 82072A (on Sparcs) */
  FDC_82077_ORIG,  /* Original version of 82077AA, sans LOCK */
  FDC_82077,       /* 82077AA-1 */
  FDC_82078_UNKN,  /* Unknown 82078 variant */
  FDC_82078,       /* 44pin 82078 or 64pin 82078SL */
  FDC_82078_1,     /* 82078-1 (2Mbps fdc) */
  FDC_S82078B,     /* S82078B (first seen on Adaptec AVA-2825 VLB SCSI/EIDE/Floppy controller) */
  FDC_87306        /* National Semiconductor PC 87306 */
} FLOPPY_CONTROLLER_TYPE, *PFLOPPY_CONTROLLER_TYPE;

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

#define FDP_DEBUG            0x02
#define FDP_SILENT_DCL_CLEAR 0x04
#define FDP_MSG              0x10
#define FDP_BROKEN_DCL       0x20
#define FDP_INVERTED_DCL     0x80

typedef struct _FLOPPY_DEVICE_PARAMETERS
{
  char CMOSType;
  unsigned long MaxDTR;          /* Step rate, usec */
  unsigned long HLT;             /* Head load/settle time, msec */
  unsigned long HUT;             /* Head unload time (remnant of 8" drives) */
  unsigned long SRT;             /* Step rate, usec */
  unsigned long Spinup;          /* time needed for spinup  */
  unsigned long Spindown;        /* timeout needed for spindown */
  unsigned char SpindownOffset;  /* decides in which position the disk will stop */
  unsigned char SelectDelay;     /* delay to wait after select */
  unsigned char RPS;             /* rotations per second */
  unsigned char Tracks;          /* maximum number of tracks */
  unsigned long Timeout;         /* timeout for interrupt requests */
  unsigned char InterleaveSect;  /* if there are more sectors, use interleave */
  FLOPPY_ERROR_THRESHOLDS MaxErrors;
  char Flags;                    /* various flags, including ftd_msg */
  BOOLEAN ReadTrack;             /* use readtrack during probing? */
  short Autodetect[8];           /* autodetected formats */
  int CheckFreq;                 /* how often should the drive be checked for disk changes */
  int NativeFormat;              /* native format of this drive */
  char *DriveName;               /* name of the drive for reporting */
} FLOPPY_DEVICE_PARAMETERS, *PFLOPPY_DEVICE_PARAMETERS;


