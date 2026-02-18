/*
 * PROJECT:         ReactOS ComPort Library for NEC PC-98 series
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Provides a serial port library for KDCOM, INIT, and FREELDR
 * COPYRIGHT:       Copyright 2020 Dmitry Borisov (di.sean@protonmail.com)
 */

/* INCLUDES *******************************************************************/

/*
 * Some PC-98 models have a second internal COM port (2ndCCU).
 * It is a 16550-compatible UART controller with I/O port address 238-23F and IRQ5.
 */
#define API_FUNCTION static
#define METHOD(Name) Name##16550
#include "cport.c"

#include <drivers/pc98/serial.h>
#include <drivers/pc98/sysport.h>
#include <drivers/pc98/pit.h>
#include <drivers/pc98/cpu.h>

#define IS_COM1(IoBase)        ((IoBase) == UlongToPtr(0x30))
#define IS_NS16550(IoBase)     ((IoBase) == UlongToPtr(0x238))

/* ioaccess.h header extension */
#define READ_PORT_BUFFER_UCHAR(port, buffer, count)   __inbytestring(H2I(port), buffer, count)
#define WRITE_PORT_BUFFER_UCHAR(port, buffer, count)  __outbytestring(H2I(port), buffer, count)

/* GLOBALS ********************************************************************/

static BOOLEAN IsNp21W;
static BOOLEAN HasFifo;
static BOOLEAN IsFifoEnabled;

/* FUNCTIONS ******************************************************************/

static
BOOLEAN
CpiIsNekoProject(VOID)
{
    UCHAR Input[4] = "NP2";
    UCHAR Output[4] = { 0 };

    if (READ_PORT_UCHAR((PUCHAR)0x7EF) != 0x00)
        return FALSE;

    WRITE_PORT_BUFFER_UCHAR((PUCHAR)0x7EF, Input, 3);
    READ_PORT_BUFFER_UCHAR((PUCHAR)0x7EF, Output, 3);

    return (*(PULONG)Input == *(PULONG)Output);
}

static
ULONG
CpiGetPitTickRate(VOID)
{
    if (READ_PORT_UCHAR((PUCHAR)0x42) & 0x20)
        return TIMER_FREQUENCY_1;
    else
        return TIMER_FREQUENCY_2;
}

static
VOID
CpiWait(VOID)
{
    ULONG i;

    for (i = 0; i < 6; i++)
        WRITE_PORT_UCHAR((PUCHAR)CPU_IO_o_ARTIC_DELAY, 0);
}

static
BOOLEAN
Cpi8251HasFifo(VOID)
{
    UCHAR IntValue1, IntValue2;

    IntValue1 = READ_PORT_UCHAR((PUCHAR)SER_8251F_REG_IIR);
    IntValue2 = READ_PORT_UCHAR((PUCHAR)SER_8251F_REG_IIR);

    return !!((IntValue1 ^ IntValue2) & SR_8251F_IIR_FIFO_DET);
}

static
BOOLEAN
Cpi8251IsPortResponding(
    _In_ PUCHAR IoBase)
{
    UCHAR Data, Status;

    Data = READ_PORT_UCHAR(IoBase);
    Status = READ_PORT_UCHAR(IoBase + 2);

    return ((Data & Status) != 0xFF && (Data | Status) != 0x00);
}

static
BOOLEAN
CpiDoesPortExist8251(
    _In_ PUCHAR IoBase)
{
    if (Cpi8251IsPortResponding(IoBase))
        return TRUE;

    /*
     * Setting FIFO mode effectively disables the data and status ports
     * by wiring them to 0xFF, making the detection attempt fail.
     * The FIFO mode may have been enabled earlier by the kernel bootloader,
     * so try to disable it, and retry.
     */
    if (IS_COM1(IoBase))
    {
        BOOLEAN WasDetected;
        UCHAR FifoControl;

        if (!Cpi8251HasFifo())
            return FALSE;

        /* Disable the FIFO logic */
        FifoControl = READ_PORT_UCHAR((PUCHAR)SER_8251F_REG_FCR);
        WRITE_PORT_UCHAR((PUCHAR)SER_8251F_REG_FCR, 0);

        WasDetected = Cpi8251IsPortResponding(IoBase);

        /* Restore the previous state */
        WRITE_PORT_UCHAR((PUCHAR)SER_8251F_REG_FCR, FifoControl);

        return WasDetected;
    }

    return FALSE;
}

VOID
NTAPI
CpEnableFifo(
    IN PUCHAR Address,
    IN BOOLEAN Enable)
{
    UCHAR Value;

    if (IS_NS16550(Address))
    {
        CpEnableFifo16550(Address, Enable);
        return;
    }

    if (!HasFifo)
        return;

    IsFifoEnabled = Enable;

    if (Enable)
        Value = SR_8251F_FCR_ENABLE | SR_8251F_FCR_RCVR_RESET | SR_8251F_FCR_TXMT_RESET;
    else
        Value = 0;
    WRITE_PORT_UCHAR((PUCHAR)SER_8251F_REG_FCR, Value);
}

VOID
NTAPI
CpSetBaud(
    IN PCPPORT Port,
    IN ULONG BaudRate)
{
    ULONG i;

    if (IS_NS16550(Port->Address))
    {
        CpSetBaud16550(Port, BaudRate);
        return;
    }

    if (IS_COM1(Port->Address))
    {
        TIMER_CONTROL_PORT_REGISTER TimerControl;
        SYSTEM_CONTROL_PORT_C_REGISTER SystemControl;
        USHORT Count;

        /* Disable the serial interrupts */
        SystemControl.Bits = READ_PORT_UCHAR((PUCHAR)PPI_IO_i_PORT_C);
        SystemControl.InterruptEnableRxReady = FALSE;
        SystemControl.InterruptEnableTxEmpty = FALSE;
        SystemControl.InterruptEnableTxReady = FALSE;
        WRITE_PORT_UCHAR((PUCHAR)PPI_IO_o_PORT_C, SystemControl.Bits);

        /* Disable V-FAST mode */
        if (HasFifo)
            WRITE_PORT_UCHAR((PUCHAR)SER_8251F_REG_DLR, SR_8251F_DLR_MODE_LEGACY);

        Count = (CpiGetPitTickRate() / 16) / BaudRate;

        /* Set the baud rate */
        TimerControl.BcdMode = FALSE;
        TimerControl.OperatingMode = PitOperatingMode3;
        TimerControl.AccessMode = PitAccessModeLowHigh;
        TimerControl.Channel = PitChannel2;
        Write8253Timer(TimerControl, Count);

        /* Unlock the legacy registers (0x30 and 0x32) */
        if (HasFifo)
            WRITE_PORT_UCHAR((PUCHAR)SER_8251F_REG_FCR, 0);
    }

    /* Software reset */
    for (i = 0; i < 3; ++i)
    {
        WRITE_PORT_UCHAR(Port->Address + 2, 0);
        CpiWait();
    }
    WRITE_PORT_UCHAR(Port->Address + 2, SR_8251A_COMMMAND_IR);
    CpiWait();

    /* Mode instruction */
    WRITE_PORT_UCHAR(Port->Address + 2,
                     SR_8251A_MODE_LENGTH_8 |SR_8251A_MODE_1_STOP | SR_8251A_MODE_CLOCKx16);
    CpiWait();

    /* Command instruction */
    WRITE_PORT_UCHAR(Port->Address + 2,
                     SR_8251A_COMMMAND_TxEN |
                     SR_8251A_COMMMAND_DTR |
                     SR_8251A_COMMMAND_RxEN |
                     SR_8251A_COMMMAND_ER |
                     SR_8251A_COMMMAND_RTS);
    CpiWait();

    /* Restore the FIFO state */
    if (IsFifoEnabled)
    {
        WRITE_PORT_UCHAR((PUCHAR)SER_8251F_REG_FCR,
                         SR_8251F_FCR_ENABLE | SR_8251F_FCR_RCVR_RESET | SR_8251F_FCR_TXMT_RESET);
    }
}

UCHAR
NTAPI
CpReadLsr(
    IN PCPPORT Port,
    IN UCHAR ExpectedValue)
{
    UCHAR Lsr, Msr;

    if (IS_NS16550(Port->Address))
        return CpReadLsr16550(Port, ExpectedValue);

    if (IsFifoEnabled)
    {
        Lsr = READ_PORT_UCHAR((PUCHAR)SER_8251F_REG_LSR);

        if (!(Lsr & ExpectedValue))
        {
            Msr = READ_PORT_UCHAR((PUCHAR)SER_8251F_REG_MSR);

            RingIndicator |= (Msr & SR_8251F_MSR_RI) ? 1 : 2;
        }
    }
    else
    {
        Lsr = READ_PORT_UCHAR(Port->Address + 2);

        if (!(Lsr & ExpectedValue))
        {
            SYSTEM_CONTROL_PORT_B_REGISTER SystemControl;

            SystemControl.Bits = READ_PORT_UCHAR((PUCHAR)PPI_IO_i_PORT_B);

            RingIndicator |= SystemControl.RingIndicator ? 1 : 2;
        }
    }

    /* If the ring indicator reaches 3, we've seen this on/off twice */
    if (RingIndicator == 3)
        Port->Flags |= CPPORT_FLAG_MODEM_CONTROL;

    return Lsr;
}

USHORT
NTAPI
CpGetByte(
    IN PCPPORT Port,
    OUT PUCHAR Byte,
    IN BOOLEAN Wait,
    IN BOOLEAN Poll)
{
    ULONG RetryCount;
    UCHAR RxReadyFlags, ErrorFlags;

    if (IS_NS16550(Port->Address))
        return CpGetByte16550(Port, Byte, Wait, Poll);

    /* Handle early read-before-init */
    if (!Port->Address)
        return CP_GET_NODATA;

    // FIXME HACK: NP21/W emulation bug, needs to be fixed on the emulator side.
    // Do a read from 0x136 to receive bytes by an emulated serial port.
    if (IsNp21W)
        (VOID)READ_PORT_UCHAR((PUCHAR)SER_8251F_REG_IIR);

    if (IsFifoEnabled)
    {
        RxReadyFlags = SR_8251F_LSR_RxRDY;
        ErrorFlags = SR_8251F_LSR_PE | SR_8251F_LSR_OE;
    }
    else
    {
        RxReadyFlags = SR_8251A_STATUS_RxRDY;
        ErrorFlags = SR_8251A_STATUS_FE | SR_8251A_STATUS_OE | SR_8251A_STATUS_PE;
    }

    /* Poll for data ready */
    for (RetryCount = Wait ? TIMEOUT_COUNT : 1; RetryCount > 0; RetryCount--)
    {
        UCHAR Lsr = CpReadLsr(Port, RxReadyFlags);
        PUCHAR DataReg;

        if (!(Lsr & RxReadyFlags))
            continue;

        /* Handle error condition */
        if (Lsr & ErrorFlags)
        {
            if (IsFifoEnabled)
                WRITE_PORT_UCHAR((PUCHAR)SER_8251F_REG_FCR, 0);

            /* Clear error flag */
            WRITE_PORT_UCHAR(Port->Address + 2,
                             SR_8251A_COMMMAND_TxEN |
                             SR_8251A_COMMMAND_DTR |
                             SR_8251A_COMMMAND_RxEN |
                             SR_8251A_COMMMAND_ER |
                             SR_8251A_COMMMAND_RTS);

            if (IsFifoEnabled)
                WRITE_PORT_UCHAR((PUCHAR)SER_8251F_REG_FCR, SR_8251F_FCR_ENABLE);

            *Byte = 0;
            return CP_GET_ERROR;
        }

        if (Poll)
            return CP_GET_SUCCESS;

        if (IsFifoEnabled)
            DataReg = UlongToPtr(SER_8251F_REG_RBR);
        else
            DataReg = Port->Address;
        *Byte = READ_PORT_UCHAR(DataReg);

        // TODO: Handle CD if port is in modem control mode

        return CP_GET_SUCCESS;
    }

    /* Reset LSR, no data was found */
    CpReadLsr(Port, 0);
    return CP_GET_NODATA;
}

VOID
NTAPI
CpPutByte(
    IN PCPPORT Port,
    IN UCHAR Byte)
{
    PUCHAR DataReg;
    UCHAR TxReadyFlags;

    if (IS_NS16550(Port->Address))
    {
        CpPutByte16550(Port, Byte);
        return;
    }

    if (IsFifoEnabled)
    {
        DataReg = UlongToPtr(SER_8251F_REG_RBR);
        TxReadyFlags = SR_8251F_LSR_TxEMPTY;

        /*
         * Unlike 16550, a call to CpDoesPortExist for the 8251 will succeed even
         * when if the user has not plug the serial port into PC-98 machine.
         * To avoid an infinite loop, we need to check if the other side is ready
         * to receive data.
         */
        if (!(READ_PORT_UCHAR((PUCHAR)SER_8251F_REG_MSR) & SR_8251F_MSR_CTS))
            return;
    }
    else
    {
        DataReg = Port->Address;
        TxReadyFlags = SR_8251A_STATUS_TxEMPTY;
    }

    while (!(CpReadLsr(Port, TxReadyFlags) & TxReadyFlags))
        NOTHING;

    WRITE_PORT_UCHAR(DataReg, Byte);
}

NTSTATUS
NTAPI
CpInitialize(
    IN PCPPORT Port,
    IN PUCHAR Address,
    IN ULONG BaudRate)
{
    PUCHAR DataReg;

    if (IS_NS16550(Address))
        return CpInitialize16550(Port, Address, BaudRate);

    if (Port == NULL || Address == NULL || BaudRate == 0)
        return STATUS_INVALID_PARAMETER;

    if (!CpDoesPortExist(Address))
        return STATUS_NOT_FOUND;

    /* Initialize port data */
    Port->Address  = Address;
    Port->BaudRate = 0;
    Port->Flags    = 0;

    if (IS_COM1(Address))
    {
        IsNp21W = CpiIsNekoProject();
        HasFifo = Cpi8251HasFifo();
    }

    /* Perform port initialization */
    CpSetBaud(Port, BaudRate);
    CpEnableFifo(Address, TRUE);

    /* Read junk out of the data register */
    if (IsFifoEnabled)
        DataReg = UlongToPtr(SER_8251F_REG_RBR);
    else
        DataReg = Port->Address;
    READ_PORT_UCHAR(DataReg);

    return STATUS_SUCCESS;
}

BOOLEAN
NTAPI
CpDoesPortExist(
    IN PUCHAR Address)
{
    if (IS_NS16550(Address))
        return CpDoesPortExist16550(Address);

    return CpiDoesPortExist8251(Address);;
}
