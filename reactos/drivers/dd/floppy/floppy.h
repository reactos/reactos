

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



