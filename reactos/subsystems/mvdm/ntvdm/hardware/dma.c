/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            dma.c
 * PURPOSE:         Direct Memory Access Controller emulation -
 *                  i8237A compatible with 74LS612 Memory Mapper extension
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "ntvdm.h"
#include "emulator.h"
#include "dma.h"

#include "io.h"

/* PRIVATE VARIABLES **********************************************************/

/*
 * DMA Controller 0 (Channels 0..3): Slave controller
 * DMA Controller 1 (Channels 4..7): Master controller
 */
static DMA_CONTROLLER DmaControllers[DMA_CONTROLLERS];

/* External page registers for each channel of the two DMA controllers */
static DMA_PAGE_REGISTER DmaPageRegisters[DMA_CONTROLLERS * DMA_CONTROLLER_CHANNELS];

/* PRIVATE FUNCTIONS **********************************************************/

#define READ_ADDR(CtrlIndex, ChanIndex, Data)   \
do {                                            \
    (Data) =                                    \
    *((PBYTE)&DmaControllers[(CtrlIndex)].DmaChannel[(ChanIndex)].CurrAddress + \
             (DmaControllers[(CtrlIndex)].FlipFlop & 0x01));                    \
    DmaControllers[(CtrlIndex)].FlipFlop ^= 1;                                  \
} while(0)

#define READ_CNT(CtrlIndex, ChanIndex, Data)    \
do {                                            \
    (Data) =                                    \
    *((PBYTE)&DmaControllers[(CtrlIndex)].DmaChannel[(ChanIndex)].CurrWordCnt + \
             (DmaControllers[(CtrlIndex)].FlipFlop & 0x01));                    \
    DmaControllers[(CtrlIndex)].FlipFlop ^= 1;                                  \
} while(0)

static BYTE WINAPI DmaReadPort(USHORT Port)
{
    BYTE ReadValue = 0xFF;

    switch (Port)
    {
        /* Start Address Registers */
        {
        case 0x00:
            READ_ADDR(0, 0, ReadValue);
            return ReadValue;
        case 0x02:
            READ_ADDR(0, 1, ReadValue);
            return ReadValue;
        case 0x04:
            READ_ADDR(0, 2, ReadValue);
            return ReadValue;
        case 0x06:
            READ_ADDR(0, 3, ReadValue);
            return ReadValue;
        case 0xC0:
            READ_ADDR(1, 0, ReadValue);
            return ReadValue;
        case 0xC4:
            READ_ADDR(1, 1, ReadValue);
            return ReadValue;
        case 0xC8:
            READ_ADDR(1, 2, ReadValue);
            return ReadValue;
        case 0xCC:
            READ_ADDR(1, 3, ReadValue);
            return ReadValue;
        }

        /* Count Address Registers */
        {
        case 0x01:
            READ_CNT(0, 0, ReadValue);
            return ReadValue;
        case 0x03:
            READ_CNT(0, 1, ReadValue);
            return ReadValue;
        case 0x05:
            READ_CNT(0, 2, ReadValue);
            return ReadValue;
        case 0x07:
            READ_CNT(0, 3, ReadValue);
            return ReadValue;
        case 0xC2:
            READ_CNT(1, 0, ReadValue);
            return ReadValue;
        case 0xC6:
            READ_CNT(1, 1, ReadValue);
            return ReadValue;
        case 0xCA:
            READ_CNT(1, 2, ReadValue);
            return ReadValue;
        case 0xCE:
            READ_CNT(1, 3, ReadValue);
            return ReadValue;
        }

        /* Status Registers */
        {
        case 0x08:
            return DmaControllers[0].Status;
        case 0xD0:
            return DmaControllers[1].Status;
        }

        /* DMA Intermediate (Temporary) Registers */
        {
        case 0x0D:
            return DmaControllers[0].TempReg;
        case 0xDA:
            return DmaControllers[1].TempReg;
        }
    }

    return 0x00;
}

#define WRITE_ADDR(CtrlIndex, ChanIndex, Data)  \
do {                                            \
    *((PBYTE)&DmaControllers[(CtrlIndex)].DmaChannel[(ChanIndex)].BaseAddress + \
             (DmaControllers[(CtrlIndex)].FlipFlop & 0x01)) = (Data);           \
    *((PBYTE)&DmaControllers[(CtrlIndex)].DmaChannel[(ChanIndex)].CurrAddress + \
             (DmaControllers[(CtrlIndex)].FlipFlop & 0x01)) = (Data);           \
    DmaControllers[(CtrlIndex)].FlipFlop ^= 1;                                  \
} while(0)

#define WRITE_CNT(CtrlIndex, ChanIndex, Data)   \
do {                                            \
    *((PBYTE)&DmaControllers[(CtrlIndex)].DmaChannel[(ChanIndex)].BaseWordCnt + \
             (DmaControllers[(CtrlIndex)].FlipFlop & 0x01)) = (Data);           \
    *((PBYTE)&DmaControllers[(CtrlIndex)].DmaChannel[(ChanIndex)].CurrWordCnt + \
             (DmaControllers[(CtrlIndex)].FlipFlop & 0x01)) = (Data);           \
    DmaControllers[(CtrlIndex)].FlipFlop ^= 1;                                  \
} while(0)

static VOID WINAPI DmaWritePort(USHORT Port, BYTE Data)
{
    switch (Port)
    {
        /* Start Address Registers */
        {
        case 0x00:
            WRITE_ADDR(0, 0, Data);
            break;
        case 0x02:
            WRITE_ADDR(0, 1, Data);
            break;
        case 0x04:
            WRITE_ADDR(0, 2, Data);
            break;
        case 0x06:
            WRITE_ADDR(0, 3, Data);
            break;
        case 0xC0:
            WRITE_ADDR(1, 0, Data);
            break;
        case 0xC4:
            WRITE_ADDR(1, 1, Data);
            break;
        case 0xC8:
            WRITE_ADDR(1, 2, Data);
            break;
        case 0xCC:
            WRITE_ADDR(1, 3, Data);
            break;
        }

        /* Count Address Registers */
        {
        case 0x01:
            WRITE_CNT(0, 0, Data);
            break;
        case 0x03:
            WRITE_CNT(0, 1, Data);
            break;
        case 0x05:
            WRITE_CNT(0, 2, Data);
            break;
        case 0x07:
            WRITE_CNT(0, 3, Data);
            break;
        case 0xC2:
            WRITE_CNT(1, 0, Data);
            break;
        case 0xC6:
            WRITE_CNT(1, 1, Data);
            break;
        case 0xCA:
            WRITE_CNT(1, 2, Data);
            break;
        case 0xCE:
            WRITE_CNT(1, 3, Data);
            break;
        }

        /* Command Registers */
        {
        case 0x08:
            DmaControllers[0].Command = Data;
            break;
        case 0xD0:
            DmaControllers[1].Command = Data;
            break;
        }

        /* Request Registers */
        {
        case 0x09:
            DmaControllers[0].Request = Data;
            break;
        case 0xD2:
            DmaControllers[1].Request = Data;
            break;
        }

        /* Flip-Flop Reset */
        {
        case 0x0C:
            DmaControllers[0].FlipFlop = 0;
            break;
        case 0xD8:
            DmaControllers[1].FlipFlop = 0;
            break;
        }

        /* DMA Master Reset */
        {
        case 0x0D:
            DmaControllers[0].Command  = 0x00;
            DmaControllers[0].Status   = 0x00;
            DmaControllers[0].Request  = 0x00;
            DmaControllers[0].TempReg  = 0x00;
            DmaControllers[0].FlipFlop = 0;
            DmaControllers[0].Mask     = 0x0F;
            break;
        case 0xDA:
            DmaControllers[1].Command  = 0x00;
            DmaControllers[1].Status   = 0x00;
            DmaControllers[1].Request  = 0x00;
            DmaControllers[1].TempReg  = 0x00;
            DmaControllers[1].FlipFlop = 0;
            DmaControllers[1].Mask     = 0x0F;
            break;
        }
    }
}

/* Page Address Registers */

static BYTE WINAPI DmaPageReadPort(USHORT Port)
{
    switch (Port)
    {
        case 0x87:
            return DmaPageRegisters[0].Page;
        case 0x83:
            return DmaPageRegisters[1].Page;
        case 0x81:
            return DmaPageRegisters[2].Page;
        case 0x82:
            return DmaPageRegisters[3].Page;
        case 0x8F:
            return DmaPageRegisters[4].Page;
        case 0x8B:
            return DmaPageRegisters[5].Page;
        case 0x89:
            return DmaPageRegisters[6].Page;
        case 0x8A:
            return DmaPageRegisters[7].Page;
    }

    return 0x00;
}

static VOID WINAPI DmaPageWritePort(USHORT Port, BYTE Data)
{
    switch (Port)
    {
        case 0x87:
            DmaPageRegisters[0].Page = Data;
            break;
        case 0x83:
            DmaPageRegisters[1].Page = Data;
            break;
        case 0x81:
            DmaPageRegisters[2].Page = Data;
            break;
        case 0x82:
            DmaPageRegisters[3].Page = Data;
            break;
        case 0x8F:
            DmaPageRegisters[4].Page = Data;
            break;
        case 0x8B:
            DmaPageRegisters[5].Page = Data;
            break;
        case 0x89:
            DmaPageRegisters[6].Page = Data;
            break;
        case 0x8A:
            DmaPageRegisters[7].Page = Data;
            break;
    }
}

/* PUBLIC FUNCTIONS ***********************************************************/

VOID DmaInitialize(VOID)
{
    /* Register the I/O Ports */

    /* Channels 0(Reserved)..3 */
    RegisterIoPort(0x00, NULL, DmaWritePort);           /* Start Address Register 0 (Reserved) */
    RegisterIoPort(0x01, NULL, DmaWritePort);           /* Count Address Register 0 (Reserved) */
    RegisterIoPort(0x02, NULL, DmaWritePort);           /* Start Address Register 1 */
    RegisterIoPort(0x03, NULL, DmaWritePort);           /* Count Address Register 1 */
    RegisterIoPort(0x04, NULL, DmaWritePort);           /* Start Address Register 2 */
    RegisterIoPort(0x05, NULL, DmaWritePort);           /* Count Address Register 2 */
    RegisterIoPort(0x06, NULL, DmaWritePort);           /* Start Address Register 3 */
    RegisterIoPort(0x07, NULL, DmaWritePort);           /* Count Address Register 3 */

    RegisterIoPort(0x08, DmaReadPort, DmaWritePort);    /* Status (Read) / Command (Write) Registers */
    RegisterIoPort(0x09, NULL, DmaWritePort);           /* Request Register */
    RegisterIoPort(0x0A, NULL, DmaWritePort);           /* Single Channel Mask Register */
    RegisterIoPort(0x0B, NULL, DmaWritePort);           /* Mode Register */
    RegisterIoPort(0x0C, NULL, DmaWritePort);           /* Flip-Flop Reset Register */
    RegisterIoPort(0x0D, DmaReadPort, DmaWritePort);    /* Intermediate (Read) / Master Reset (Write) Registers */
    RegisterIoPort(0x0E, NULL, DmaWritePort);           /* Mask Reset Register */
    RegisterIoPort(0x0F, DmaReadPort, DmaWritePort);    /* Multi-Channel Mask Reset Register */


    /* Channels 4(Reserved)..7 */
    RegisterIoPort(0xC0, NULL, DmaWritePort);           /* Start Address Register 4 (Reserved) */
    RegisterIoPort(0xC2, NULL, DmaWritePort);           /* Count Address Register 4 (Reserved) */
    RegisterIoPort(0xC4, NULL, DmaWritePort);           /* Start Address Register 5 */
    RegisterIoPort(0xC6, NULL, DmaWritePort);           /* Count Address Register 5 */
    RegisterIoPort(0xC8, NULL, DmaWritePort);           /* Start Address Register 6 */
    RegisterIoPort(0xCA, NULL, DmaWritePort);           /* Count Address Register 6 */
    RegisterIoPort(0xCC, NULL, DmaWritePort);           /* Start Address Register 7 */
    RegisterIoPort(0xCE, NULL, DmaWritePort);           /* Count Address Register 7 */

    RegisterIoPort(0xD0, DmaReadPort, DmaWritePort);    /* Status (Read) / Command (Write) Registers */
    RegisterIoPort(0xD2, NULL, DmaWritePort);           /* Request Register */
    RegisterIoPort(0xD4, NULL, DmaWritePort);           /* Single Channel Mask Register */
    RegisterIoPort(0xD6, NULL, DmaWritePort);           /* Mode Register */
    RegisterIoPort(0xD8, NULL, DmaWritePort);           /* Flip-Flop Reset Register */
    RegisterIoPort(0xDA, DmaReadPort, DmaWritePort);    /* Intermediate (Read) / Master Reset (Write) Registers */
    RegisterIoPort(0xDC, NULL, DmaWritePort);           /* Mask Reset Register */
    RegisterIoPort(0xDE, DmaReadPort, DmaWritePort);    /* Multi-Channel Mask Reset Register */


    /* Channels Page Address Registers */
    RegisterIoPort(0x87, DmaPageReadPort, DmaPageWritePort);    /* Channel 0 (Reserved) */
    RegisterIoPort(0x83, DmaPageReadPort, DmaPageWritePort);    /* Channel 1 */
    RegisterIoPort(0x81, DmaPageReadPort, DmaPageWritePort);    /* Channel 2 */
    RegisterIoPort(0x82, DmaPageReadPort, DmaPageWritePort);    /* Channel 3 */
    RegisterIoPort(0x8F, DmaPageReadPort, DmaPageWritePort);    /* Channel 4 (Reserved) */
    RegisterIoPort(0x8B, DmaPageReadPort, DmaPageWritePort);    /* Channel 5 */
    RegisterIoPort(0x89, DmaPageReadPort, DmaPageWritePort);    /* Channel 6 */
    RegisterIoPort(0x8A, DmaPageReadPort, DmaPageWritePort);    /* Channel 7 */
}



DWORD
WINAPI
VDDRequestDMA(IN HANDLE    hVdd,
              IN WORD      iChannel,
              IN OUT PVOID Buffer,
              IN DWORD     length)
{
    UNREFERENCED_PARAMETER(hVdd);

    if (iChannel >= DMA_CONTROLLERS * DMA_CONTROLLER_CHANNELS)
    {
        SetLastError(ERROR_INVALID_ADDRESS);
        return FALSE;
    }

    UNIMPLEMENTED;
    return 0;
}

BOOL
WINAPI
VDDQueryDMA(IN HANDLE        hVdd,
            IN WORD          iChannel,
            IN PVDD_DMA_INFO pDmaInfo)
{
    PDMA_CONTROLLER pDcp;
    WORD Channel;

    UNREFERENCED_PARAMETER(hVdd);

    if (iChannel >= DMA_CONTROLLERS * DMA_CONTROLLER_CHANNELS)
    {
        SetLastError(ERROR_INVALID_ADDRESS);
        return FALSE;
    }

    pDcp    = &DmaControllers[iChannel / DMA_CONTROLLER_CHANNELS];
    Channel = iChannel % DMA_CONTROLLER_CHANNELS;

    pDmaInfo->addr  = pDcp->DmaChannel[Channel].CurrAddress;
    pDmaInfo->count = pDcp->DmaChannel[Channel].CurrWordCnt;

    // pDmaInfo->page   = DmaPageRegisters[iChannel].Page;
    pDmaInfo->status = pDcp->Status;
    pDmaInfo->mode   = pDcp->DmaChannel[Channel].Mode;
    pDmaInfo->mask   = pDcp->Mask;

    return TRUE;
}

BOOL
WINAPI
VDDSetDMA(IN HANDLE        hVdd,
          IN WORD          iChannel,
          IN WORD          fDMA,
          IN PVDD_DMA_INFO pDmaInfo)
{
    PDMA_CONTROLLER pDcp;
    WORD Channel;

    UNREFERENCED_PARAMETER(hVdd);

    if (iChannel >= DMA_CONTROLLERS * DMA_CONTROLLER_CHANNELS)
    {
        SetLastError(ERROR_INVALID_ADDRESS);
        return FALSE;
    }

    pDcp    = &DmaControllers[iChannel / DMA_CONTROLLER_CHANNELS];
    Channel = iChannel % DMA_CONTROLLER_CHANNELS;

    if (fDMA & VDD_DMA_ADDR)
        pDcp->DmaChannel[Channel].CurrAddress = pDmaInfo->addr;

    if (fDMA & VDD_DMA_COUNT)
        pDcp->DmaChannel[Channel].CurrWordCnt = pDmaInfo->count;

    // if (fDMA & VDD_DMA_PAGE)
        // DmaPageRegisters[iChannel].Page = pDmaInfo->page;

    if (fDMA & VDD_DMA_STATUS)
        pDcp->Status = pDmaInfo->status;

    return TRUE;
}

/* EOF */
