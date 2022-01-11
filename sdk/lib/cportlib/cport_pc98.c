/*
 * PROJECT:         ReactOS ComPort Library for NEC PC-98 series
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Provides a serial port library for KDCOM, INIT, and FREELDR
 * COPYRIGHT:       Copyright 2020 Dmitry Borisov (di.sean@protonmail.com)
 */

/* Note: ns16550 code from cportlib.c */

/* INCLUDES *******************************************************************/

#include <intrin.h>
#include <ioaccess.h>
#include <ntstatus.h>
#include <cportlib/cportlib.h>
#include <drivers/pc98/serial.h>
#include <drivers/pc98/sysport.h>
#include <drivers/pc98/pit.h>
#include <drivers/pc98/cpu.h>

/* GLOBALS ********************************************************************/

#define TIMEOUT_COUNT 1024 * 200

static struct
{
    PUCHAR Address;
    BOOLEAN HasFifo;
    BOOLEAN FifoEnabled;
    UCHAR RingIndicator;
} Rs232ComPort[] =
{
    { (PUCHAR)0x030, FALSE, FALSE, 0 },
    { (PUCHAR)0x238, FALSE, FALSE, 0 }
};

static BOOLEAN IsNekoProject = FALSE;

/* FUNCTIONS ******************************************************************/

static BOOLEAN
CpIsNekoProject(VOID)
{
    UCHAR Input[4] = "NP2";
    UCHAR Output[4] = {0};
    UCHAR i;

    for (i = 0; i < 3; i++)
        WRITE_PORT_UCHAR((PUCHAR)0x7EF, Input[i]);

    for (i = 0; i < 3; i++)
        Output[i] = READ_PORT_UCHAR((PUCHAR)0x7EF);

    return (*(PULONG)Input == *(PULONG)Output);
}

static VOID
CpWait(VOID)
{
    UCHAR i;

    for (i = 0; i < 6; i++)
        WRITE_PORT_UCHAR((PUCHAR)CPU_IO_o_ARTIC_DELAY, 0);
}

VOID
NTAPI
CpEnableFifo(
    IN PUCHAR Address,
    IN BOOLEAN Enable)
{
    /* Set FIFO and clear the receive/transmit buffers */
    if (Address == Rs232ComPort[0].Address && Rs232ComPort[0].HasFifo)
    {
        if (Enable)
            WRITE_PORT_UCHAR((PUCHAR)SER1_IO_o_FIFO_CONTROL,
                             SER_FCR_ENABLE | SER_FCR_RCVR_RESET | SER_FCR_TXMT_RESET);
        else
            WRITE_PORT_UCHAR((PUCHAR)SER1_IO_o_FIFO_CONTROL, SER_FCR_DISABLE);
        Rs232ComPort[0].FifoEnabled = Enable;
    }
    else if (Address == Rs232ComPort[1].Address && Rs232ComPort[1].HasFifo)
    {
        if (Enable)
            WRITE_PORT_UCHAR((PUCHAR)SER2_IO_o_FIFO_CONTROL,
                             SER_FCR_ENABLE | SER_FCR_RCVR_RESET | SER_FCR_TXMT_RESET);
        else
            WRITE_PORT_UCHAR((PUCHAR)SER2_IO_o_FIFO_CONTROL, SER_FCR_DISABLE);
        Rs232ComPort[1].FifoEnabled = Enable;
    }
    CpWait();
}

VOID
NTAPI
CpSetBaud(
    IN PCPPORT Port,
    IN ULONG BaudRate)
{
    UCHAR Lcr;
    USHORT Count;
    TIMER_CONTROL_PORT_REGISTER TimerControl;

    if (Port->Address == Rs232ComPort[0].Address)
    {
        if (Rs232ComPort[0].HasFifo)
            WRITE_PORT_UCHAR((PUCHAR)SER1_IO_o_DIVISOR_LATCH, SER1_DLR_MODE_LEGACY);

        TimerControl.BcdMode = FALSE;
        TimerControl.OperatingMode = PitOperatingMode3;
        TimerControl.AccessMode = PitAccessModeLowHigh;
        TimerControl.Channel = PitChannel2;
        if (IsNekoProject)
        {
            /* The horrible text input lag happens by about 6 seconds on my PC */
            Count = 3;
        }
        else
        {
            Count = (READ_PORT_UCHAR((PUCHAR)0x42) & 0x20) ?
                    (TIMER_FREQUENCY_1 / (BaudRate * 16)) : (TIMER_FREQUENCY_2 / (BaudRate * 16));
        }
        Write8253Timer(TimerControl, Count);

        /* Save baud rate in port */
        Port->BaudRate = BaudRate;
    }
    else if (Port->Address == Rs232ComPort[1].Address)
    {
        /* Set the DLAB on */
        Lcr = READ_PORT_UCHAR((PUCHAR)SER2_IO_i_LINE_CONTROL);
        WRITE_PORT_UCHAR((PUCHAR)SER2_IO_o_LINE_CONTROL, Lcr | SER2_LCR_DLAB);

        /* Set the baud rate */
        Count = SER2_CLOCK_RATE / BaudRate;
        WRITE_PORT_UCHAR((PUCHAR)SER2_IO_o_DIVISOR_LATCH_LSB, Count & 0xFF);
        WRITE_PORT_UCHAR((PUCHAR)SER2_IO_o_DIVISOR_LATCH_MSB, (Count >> 8) & 0xFF);

        /* Reset DLAB */
        WRITE_PORT_UCHAR((PUCHAR)SER2_IO_o_LINE_CONTROL, Lcr & ~SER2_LCR_DLAB);

        /* Save baud rate in port */
        Port->BaudRate = BaudRate;
    }
}

NTSTATUS
NTAPI
CpInitialize(
    IN PCPPORT Port,
    IN PUCHAR Address,
    IN ULONG BaudRate)
{
    SYSTEM_CONTROL_PORT_C_REGISTER SystemControl;
    UCHAR FifoStatus;

    if (Port == NULL || Address == NULL || BaudRate == 0)
        return STATUS_INVALID_PARAMETER;

    if (!CpDoesPortExist(Address))
        return STATUS_NOT_FOUND;

    /* Initialize port data */
    Port->Address = Address;
    Port->BaudRate = 0;
    Port->Flags = 0;

    IsNekoProject = CpIsNekoProject();

    if (Port->Address == Rs232ComPort[0].Address)
    {
        /* FIFO test */
        FifoStatus = READ_PORT_UCHAR((PUCHAR)SER1_IO_i_INTERRUPT_ID) & SER1_IIR_FIFOS_ENABLED;
        CpWait();
        Rs232ComPort[0].HasFifo = ((READ_PORT_UCHAR((PUCHAR)SER1_IO_i_INTERRUPT_ID) & SER1_IIR_FIFOS_ENABLED) != FifoStatus);

        /* Disable the interrupts */
        SystemControl.Bits = READ_PORT_UCHAR((PUCHAR)PPI_IO_i_PORT_C);
        SystemControl.InterruptEnableRxReady = FALSE;
        SystemControl.InterruptEnableTxEmpty = FALSE;
        SystemControl.InterruptEnableTxReady = FALSE;
        WRITE_PORT_UCHAR((PUCHAR)PPI_IO_o_PORT_C, SystemControl.Bits);

        /* Turn off FIFO */
        if (Rs232ComPort[0].HasFifo)
            CpEnableFifo(Address, FALSE);

        /* Set the baud rate */
        CpSetBaud(Port, BaudRate);

        /* Software reset */
        WRITE_PORT_UCHAR((PUCHAR)SER1_IO_o_MODE_COMMAND, 0);
        CpWait();
        WRITE_PORT_UCHAR((PUCHAR)SER1_IO_o_MODE_COMMAND, 0);
        CpWait();
        WRITE_PORT_UCHAR((PUCHAR)SER1_IO_o_MODE_COMMAND, 0);
        CpWait();
        WRITE_PORT_UCHAR((PUCHAR)SER1_IO_o_MODE_COMMAND, SER1_COMMMAND_IR);
        CpWait();

        /* Mode instruction - asynchronous mode, 8 data bits, 1 stop bit, no parity, 16x clock divisor */
        WRITE_PORT_UCHAR((PUCHAR)SER1_IO_o_MODE_COMMAND,
                         SER1_MODE_LENGTH_8 | SER1_MODE_1_STOP | SER1_MODE_CLOCKx16);
        CpWait();

        /* Command instruction - transmit enable, turn on DTR and RTS, receive enable, clear error flag */
        WRITE_PORT_UCHAR((PUCHAR)SER1_IO_o_MODE_COMMAND,
                         SER1_COMMMAND_TxEN | SER1_COMMMAND_DTR |
                         SER1_COMMMAND_RxEN | SER1_COMMMAND_ER | SER1_COMMMAND_RTS);
        CpWait();

        /* Disable the interrupts again */
        WRITE_PORT_UCHAR((PUCHAR)PPI_IO_o_PORT_C, SystemControl.Bits);

        /* Turn on FIFO */
        if (Rs232ComPort[0].HasFifo)
            CpEnableFifo(Address, TRUE);

        /* Read junk out of the data register */
        if (Rs232ComPort[0].HasFifo)
            (VOID)READ_PORT_UCHAR((PUCHAR)SER1_IO_i_RECEIVER_BUFFER);
        else
            (VOID)READ_PORT_UCHAR((PUCHAR)SER1_IO_i_DATA);

        return STATUS_SUCCESS;
    }
    else if (Port->Address == Rs232ComPort[1].Address)
    {
        /* Disable the interrupts */
        WRITE_PORT_UCHAR((PUCHAR)SER2_IO_o_LINE_CONTROL, 0);
        WRITE_PORT_UCHAR((PUCHAR)SER2_IO_o_INTERRUPT_EN, 0);

        /* Turn on DTR, RTS and OUT2 */
        WRITE_PORT_UCHAR((PUCHAR)SER2_IO_o_MODEM_CONTROL,
                         SER2_MCR_DTR_STATE | SER2_MCR_RTS_STATE | SER2_MCR_OUT_2);

        /* Set the baud rate */
        CpSetBaud(Port, BaudRate);

        /* Set 8 data bits, 1 stop bit, no parity, no break */
        WRITE_PORT_UCHAR((PUCHAR)SER2_IO_o_LINE_CONTROL,
                         SER2_LCR_LENGTH_8 | SER2_LCR_ST1 | SER2_LCR_NO_PARITY);

        /* FIFO test */
        Rs232ComPort[1].HasFifo = READ_PORT_UCHAR((PUCHAR)SER2_IO_i_INTERRUPT_ID) & SER2_IIR_HAS_FIFO;

        /* Turn on FIFO */
        if (Rs232ComPort[1].HasFifo)
            CpEnableFifo(Address, TRUE);

        /* Read junk out of the RBR */
        (VOID)READ_PORT_UCHAR((PUCHAR)SER2_IO_i_RECEIVER_BUFFER);

        return STATUS_SUCCESS;
    }

    return STATUS_NOT_FOUND;
}

static BOOLEAN
ComPortTest1(IN PUCHAR Address)
{
    /*
     * See "Building Hardware and Firmware to Complement Microsoft Windows Headless Operation"
     * Out-of-Band Management Port Device Requirements:
     * The device must act as a 16550 or 16450 UART.
     * Windows Server 2003 will test this device using the following process:
     *     1. Save off the current modem status register.
     *     2. Place the UART into diagnostic mode (The UART is placed into loopback mode
     *        by writing SERIAL_MCR_LOOP to the modem control register).
     *     3. The modem status register is read and the high bits are checked. This means
     *        SERIAL_MSR_CTS, SERIAL_MSR_DSR, SERIAL_MSR_RI and SERIAL_MSR_DCD should
     *        all be clear.
     *     4. Place the UART in diagnostic mode and turn on OUTPUT (Loopback Mode and
     *         OUTPUT are both turned on by writing (SERIAL_MCR_LOOP | SERIAL_MCR_OUT1)
     *         to the modem control register).
     *     5. The modem status register is read and the ring indicator is checked.
     *        This means SERIAL_MSR_RI should be set.
     *     6. Restore original modem status register.
     *
     * REMARK: Strangely enough, the Virtual PC 2007 virtual machine
     *         doesn't pass this test.
     */

    BOOLEAN RetVal = FALSE;
    UCHAR Mcr, Msr;

    /* Save the Modem Control Register */
    Mcr = READ_PORT_UCHAR((PUCHAR)SER2_IO_i_MODEM_CONTROL);

    /* Enable loop (diagnostic) mode (set Bit 4 of the MCR) */
    WRITE_PORT_UCHAR((PUCHAR)SER2_IO_o_MODEM_CONTROL, SER2_MCR_LOOPBACK);

    /* Clear all modem output bits */
    WRITE_PORT_UCHAR((PUCHAR)SER2_IO_o_MODEM_CONTROL, SER2_MCR_LOOPBACK);

    /* Read the Modem Status Register */
    Msr = READ_PORT_UCHAR((PUCHAR)SER2_IO_i_MODEM_STATUS);

    /*
     * The upper nibble of the MSR (modem output bits) must be
     * equal to the lower nibble of the MCR (modem input bits).
     */
    if ((Msr & (SER_MSR_CTS | SER_MSR_DSR | SER_MSR_RI | SER_MSR_DCD)) == 0x00)
    {
        /* Set all modem output bits */
        WRITE_PORT_UCHAR((PUCHAR)SER2_IO_o_MODEM_CONTROL,
                         SER2_MCR_OUT_1 | SER2_MCR_LOOPBACK); // Windows
/* ReactOS
        WRITE_PORT_UCHAR((PUCHAR)SER2_IO_o_MODEM_CONTROL,
                         SER2_MCR_DTR_STATE | SER2_MCR_RTS_STATE |
                         SER2_MCR_OUT_1 | SER2_MCR_OUT_2 | SER2_MCR_LOOPBACK);
*/

        /* Read the Modem Status Register */
        Msr = READ_PORT_UCHAR((PUCHAR)SER2_IO_i_MODEM_STATUS);

        /*
         * The upper nibble of the MSR (modem output bits) must be
         * equal to the lower nibble of the MCR (modem input bits).
         */
        if (Msr & SER_MSR_RI) // Windows
        // if (Msr & (SER_MSR_CTS | SER_MSR_DSR | SER_MSR_RI | SER_MSR_DCD) == 0xF0) // ReactOS
        {
            RetVal = TRUE;
        }
    }

    /* Restore the MCR */
    WRITE_PORT_UCHAR((PUCHAR)SER2_IO_o_MODEM_CONTROL, Mcr);

    return RetVal;
}

static BOOLEAN
ComPortTest2(IN PUCHAR Address)
{
    /*
     * This test checks whether the 16450/16550 scratch register is available.
     * If not, the serial port is considered as unexisting.
     */

    UCHAR Byte = 0;

    do
    {
        WRITE_PORT_UCHAR((PUCHAR)SER2_IO_o_SCRATCH, Byte);

        if (READ_PORT_UCHAR((PUCHAR)SER2_IO_i_SCRATCH) != Byte)
            return FALSE;

    }
    while (++Byte != 0);

    return TRUE;
}

BOOLEAN
NTAPI
CpDoesPortExist(IN PUCHAR Address)
{
    UCHAR Data, Status;

    if (Address == Rs232ComPort[0].Address || Address == (PUCHAR)0x41)
    {
        Data = READ_PORT_UCHAR(Address);
        Status = READ_PORT_UCHAR(Address + 2);
        if ((Data & Status) == 0xFF || (Data | Status) == 0x00)
            return FALSE;
        else
            return TRUE;
    }
    else if (Address == Rs232ComPort[1].Address)
    {
        return (ComPortTest1(Address) || ComPortTest2(Address));
    }

    return FALSE;
}

UCHAR
NTAPI
CpReadLsr(
    IN PCPPORT Port,
    IN UCHAR ExpectedValue)
{
    UCHAR Lsr, Msr;
    SYSTEM_CONTROL_PORT_B_REGISTER SystemControl;

    if (Port->Address == Rs232ComPort[0].Address)
    {
        /* Read the LSR and check if the expected value is present */
        if (Rs232ComPort[0].HasFifo)
        {
            Lsr = READ_PORT_UCHAR((PUCHAR)SER1_IO_i_LINE_STATUS);
            if (!(Lsr & ExpectedValue))
            {
                Msr = READ_PORT_UCHAR((PUCHAR)SER1_IO_i_MODEM_STATUS);

                /* If the ring indicator reaches 3, we've seen this on/off twice */
                Rs232ComPort[0].RingIndicator |= (Msr & SER_MSR_RI) ? 1 : 2;
                if (Rs232ComPort[0].RingIndicator == 3)
                    Port->Flags |= CPPORT_FLAG_MODEM_CONTROL;
            }
        }
        else
        {
            Lsr = READ_PORT_UCHAR((PUCHAR)SER1_IO_i_STATUS);
            if (!(Lsr & ExpectedValue))
            {
                SystemControl.Bits = READ_PORT_UCHAR((PUCHAR)PPI_IO_i_PORT_B);

                /* If the ring indicator reaches 3, we've seen this on/off twice */
                Rs232ComPort[0].RingIndicator |= SystemControl.RingIndicator ? 1 : 2;
                if (Rs232ComPort[0].RingIndicator == 3)
                    Port->Flags |= CPPORT_FLAG_MODEM_CONTROL;
            }
        }

        return Lsr;
    }
    else if (Port->Address == Rs232ComPort[1].Address)
    {
        /* Read the LSR and check if the expected value is present */
        Lsr = READ_PORT_UCHAR((PUCHAR)SER2_IO_i_LINE_STATUS);
        if (!(Lsr & ExpectedValue))
        {
            Msr = READ_PORT_UCHAR((PUCHAR)SER2_IO_i_MODEM_STATUS);

            /* If the indicator reaches 3, we've seen this on/off twice */
            Rs232ComPort[1].RingIndicator |= (Msr & SER_MSR_RI) ? 1 : 2;
            if (Rs232ComPort[1].RingIndicator == 3)
                Port->Flags |= CPPORT_FLAG_MODEM_CONTROL;
        }

        return Lsr;
    }

    return 0;
}

USHORT
NTAPI
CpGetByte(
    IN PCPPORT Port,
    OUT PUCHAR Byte,
    IN BOOLEAN Wait,
    IN BOOLEAN Poll)
{
    UCHAR Lsr;
    ULONG LimitCount = Wait ? TIMEOUT_COUNT : 1;
    UCHAR SuccessFlags, ErrorFlags;
    BOOLEAN FifoEnabled;

    /* Handle early read-before-init */
    if (!Port->Address)
        return CP_GET_NODATA;

    if (Port->Address == Rs232ComPort[0].Address)
    {
        SuccessFlags = Rs232ComPort[0].HasFifo ? SER1_LSR_RxRDY : SER1_STATUS_RxRDY;
        ErrorFlags = Rs232ComPort[0].HasFifo ? (SER1_LSR_PE | SER1_LSR_OE) :
                                               (SER1_STATUS_FE | SER1_STATUS_PE | SER1_STATUS_OE);

        /* If "wait" mode enabled, spin many times, otherwise attempt just once */
        while (LimitCount--)
        {
            /* Read LSR for data ready */
            Lsr = CpReadLsr(Port, SuccessFlags);
            if (Lsr & SuccessFlags)
            {
                /* If an error happened, clear the byte and fail */
                if (Lsr & ErrorFlags)
                {
                    /* Save the last FIFO state */
                    FifoEnabled = Rs232ComPort[0].FifoEnabled;

                    /* Turn off FIFO */
                    if (FifoEnabled)
                        CpEnableFifo(Port->Address, FALSE);

                    /* Clear error flag */
                    WRITE_PORT_UCHAR((PUCHAR)SER1_IO_o_MODE_COMMAND,
                                     SER1_COMMMAND_TxEN | SER1_COMMMAND_DTR |
                                     SER1_COMMMAND_RxEN | SER1_COMMMAND_ER | SER1_COMMMAND_RTS);

                    /* Turn on FIFO */
                    if (FifoEnabled)
                        CpEnableFifo(Port->Address, TRUE);

                    *Byte = 0;
                    return CP_GET_ERROR;
                }

                /* If only polling was requested by caller, return now */
                if (Poll)
                    return CP_GET_SUCCESS;

                /* Otherwise read the byte and return it */
                if (Rs232ComPort[0].HasFifo)
                    *Byte = READ_PORT_UCHAR((PUCHAR)SER1_IO_i_RECEIVER_BUFFER);
                else
                    *Byte = READ_PORT_UCHAR((PUCHAR)SER1_IO_i_DATA);

                /* TODO: Handle CD if port is in modem control mode */

                /* Byte was read */
                return CP_GET_SUCCESS;
            }
            else if (IsNekoProject && Rs232ComPort[0].HasFifo)
            {
                /*
                 * Neko Project 21/W doesn't set RxRDY without reading any data from 0x136.
                 * TODO: Check real hardware behavior.
                 */
                (VOID)READ_PORT_UCHAR((PUCHAR)SER1_IO_i_INTERRUPT_ID);
            }
        }

        /* Reset LSR, no data was found */
        CpReadLsr(Port, 0);
    }
    else if (Port->Address == Rs232ComPort[1].Address)
    {
        /* If "wait" mode enabled, spin many times, otherwise attempt just once */
        while (LimitCount--)
        {
            /* Read LSR for data ready */
            Lsr = CpReadLsr(Port, SER2_LSR_DR);
            if ((Lsr & SER2_LSR_DR) == SER2_LSR_DR)
            {
                /* If an error happened, clear the byte and fail */
                if (Lsr & (SER2_LSR_FE | SER2_LSR_PE | SER2_LSR_OE))
                {
                    *Byte = 0;
                    return CP_GET_ERROR;
                }

                /* If only polling was requested by caller, return now */
                if (Poll)
                    return CP_GET_SUCCESS;

                /* Otherwise read the byte and return it */
                *Byte = READ_PORT_UCHAR((UCHAR)SER2_IO_i_RECEIVER_BUFFER);

                /* TODO: Handle CD if port is in modem control mode */

                /* Byte was read */
                return CP_GET_SUCCESS;
            }
        }

        /* Reset LSR, no data was found */
        CpReadLsr(Port, 0);
    }

    return CP_GET_NODATA;
}

VOID
NTAPI
CpPutByte(
    IN PCPPORT Port,
    IN UCHAR Byte)
{
    if (Port->Address == Rs232ComPort[0].Address)
    {
        /* TODO: Check if port is in modem control to handle CD */

        if (Rs232ComPort[0].HasFifo)
        {
            while ((CpReadLsr(Port, SER1_LSR_TxRDY) & SER1_LSR_TxRDY) == 0)
                NOTHING;

            WRITE_PORT_UCHAR((PUCHAR)SER1_IO_o_TRANSMITTER_BUFFER, Byte);
        }
        else
        {
            while ((CpReadLsr(Port, SER1_STATUS_TxRDY) & SER1_STATUS_TxRDY) == 0)
                NOTHING;

            WRITE_PORT_UCHAR((PUCHAR)SER1_IO_o_DATA, Byte);
        }
    }
    else if (Port->Address == Rs232ComPort[1].Address)
    {
        /* TODO: Check if port is in modem control to handle CD */

        while ((CpReadLsr(Port, SER2_LSR_THR_EMPTY) & SER2_LSR_THR_EMPTY) == 0)
            NOTHING;

        WRITE_PORT_UCHAR((PUCHAR)SER2_IO_o_TRANSMITTER_BUFFER, Byte);
    }
}
