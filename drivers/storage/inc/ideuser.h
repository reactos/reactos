/*++

Copyright (C) Microsoft Corporation, 1999 - 1999

Module Name:

    ideuser.h

Abstract:

    These are the structures and defines that are used in the
    PCI IDE mini drivers.

Revision History:

--*/

#if !defined (___ideuser_h___)
#define ___ideuser_h___

  
#define PIO_MODE0           (1 << 0)
#define PIO_MODE1           (1 << 1)
#define PIO_MODE2           (1 << 2)
#define PIO_MODE3           (1 << 3)
#define PIO_MODE4           (1 << 4)

#define SWDMA_MODE0         (1 << 5)
#define SWDMA_MODE1         (1 << 6)
#define SWDMA_MODE2         (1 << 7)

#define MWDMA_MODE0         (1 << 8)
#define MWDMA_MODE1         (1 << 9)
#define MWDMA_MODE2         (1 << 10)

#define UDMA_MODE0          (1 << 11)
#define UDMA_MODE1          (1 << 12)
#define UDMA_MODE2          (1 << 13)
#define UDMA_MODE3          (1 << 14)
#define UDMA_MODE4          (1 << 15)
#define UDMA_MODE5          (1 << 16)

#define PIO_SUPPORT         (PIO_MODE0      | PIO_MODE1     | PIO_MODE2    | PIO_MODE3     | PIO_MODE4)
#define SWDMA_SUPPORT       (SWDMA_MODE0    | SWDMA_MODE1   | SWDMA_MODE2)
#define MWDMA_SUPPORT       (MWDMA_MODE0    | MWDMA_MODE1   | MWDMA_MODE2)
#define UDMA33_SUPPORT      (UDMA_MODE0     | UDMA_MODE1    | UDMA_MODE2)
#define UDMA66_SUPPORT      (UDMA_MODE3     | UDMA_MODE4)
#define UDMA100_SUPPORT     (UDMA_MODE5 )
#define UDMA_SUPPORT        (UNINITIALIZED_TRANSFER_MODE & (~(PIO_SUPPORT | SWDMA_SUPPORT | MWDMA_SUPPORT)))

#define DMA_SUPPORT         (SWDMA_SUPPORT  | MWDMA_SUPPORT | UDMA_SUPPORT)
#define ALL_MODE_SUPPORT    (PIO_SUPPORT | DMA_SUPPORT)

#define PIO0                        0
#define PIO1                        1
#define PIO2                        2
#define PIO3                        3
#define PIO4                        4
#define SWDMA0                      5
#define SWDMA1                      6
#define SWDMA2                      7
#define MWDMA0                      8
#define MWDMA1                      9
#define MWDMA2                      10
#define UDMA0                       11

#define MAX_XFER_MODE               17
#define UNINITIALIZED_CYCLE_TIME    0xffffffff
#define UNINITIALIZED_TRANSFER_MODE 0x7fffffff
#define IS_DEFAULT(mode)    (!(mode & 0x80000000))

#define GenTransferModeMask(i, mode) {\
    ULONG temp=0xffffffff; \
    mode |= (temp >> (31-(i)));\
}

//
// mode should not be 0
//
#define GetHighestTransferMode(mode, i) {\
    ULONG temp=(mode); \
    ASSERT(temp); \
    i=0; \
    while ( temp) { \
        temp = (temp >> 1);\
        i++;\
    } \
    i--; \
}

#define GetHighestDMATransferMode(mode, i) {\
    ULONG temp=mode >> 5;\
    i=5; \
    while ( temp) { \
        temp = (temp >> 1); \
        i++; \
    } \
    i--; \
}
#define GetHighestPIOTransferMode(mode, i) { \
    ULONG temp = (mode & PIO_SUPPORT); \
    i=0; \
    temp = temp >> 1; \
    while (temp) { \
        temp = temp >> 1; \
        i++; \
    } \
}

#define SetDefaultTiming(timingTable, length) {\
    timingTable[0]=PIO_MODE0_CYCLE_TIME; \
    timingTable[1]=PIO_MODE1_CYCLE_TIME; \
    timingTable[2]=PIO_MODE2_CYCLE_TIME; \
    timingTable[3]=PIO_MODE3_CYCLE_TIME; \
    timingTable[4]=PIO_MODE4_CYCLE_TIME; \
    timingTable[5]=SWDMA_MODE0_CYCLE_TIME; \
    timingTable[6]=SWDMA_MODE1_CYCLE_TIME; \
    timingTable[7]=SWDMA_MODE2_CYCLE_TIME; \
    timingTable[8]=MWDMA_MODE0_CYCLE_TIME; \
    timingTable[9]=MWDMA_MODE1_CYCLE_TIME; \
    timingTable[10]=MWDMA_MODE2_CYCLE_TIME; \
    timingTable[11]=UDMA_MODE0_CYCLE_TIME; \
    timingTable[12]=UDMA_MODE1_CYCLE_TIME; \
    timingTable[13]=UDMA_MODE2_CYCLE_TIME; \
    timingTable[14]=UDMA_MODE3_CYCLE_TIME; \
    timingTable[15]=UDMA_MODE4_CYCLE_TIME; \
    timingTable[16]=UDMA_MODE5_CYCLE_TIME; \
    length = MAX_XFER_MODE; \
}

#endif // ___ideuser_h___

