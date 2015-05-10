/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            dma.h
 * PURPOSE:         ISA DMA - Direct Memory Access Controller emulation -
 *                  i8237A compatible with 74LS612 Memory Mapper extension
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef _DMA_H_
#define _DMA_H_

/* DEFINES ********************************************************************/

#define DMA_CONTROLLERS         2
#define DMA_CONTROLLER_CHANNELS 4 // Each DMA controller has 4 channels

typedef struct _DMA_CHANNEL
{
    WORD BaseAddress;
    WORD BaseElemCnt;
    WORD CurrAddress;
    WORD CurrElemCnt;
    BYTE Mode;
} DMA_CHANNEL, *PDMA_CHANNEL;

typedef struct _DMA_CONTROLLER
{
    DMA_CHANNEL DmaChannel[DMA_CONTROLLER_CHANNELS];

    WORD TempAddress;
    WORD TempElemCnt;

    BYTE TempReg;

    BYTE Command;
    BYTE Request;
    BYTE Mask;
    BYTE Status;

    BOOLEAN FlipFlop; // 0: LSB ; 1: MSB

} DMA_CONTROLLER, *PDMA_CONTROLLER;

/* 74LS612 Memory Mapper extension */
typedef struct _DMA_PAGE_REGISTER
{
    BYTE Page;
} DMA_PAGE_REGISTER, *PDMA_PAGE_REGISTER;

// The 74LS612 contains 16 bytes, each of them being a page register.
// They are accessible via ports 0x80 through 0x8F.

/* FUNCTIONS ******************************************************************/

DWORD DmaRequest(IN WORD      iChannel,
                 IN OUT PVOID Buffer,
                 IN DWORD     length);

VOID DmaInitialize(VOID);

#endif // _DMA_H_

/* EOF */
