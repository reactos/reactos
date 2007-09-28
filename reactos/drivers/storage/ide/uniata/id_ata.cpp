/*++

Copyright (c) 2002-2007 Alexandr A. Telyatnikov (Alter)

Module Name:
    id_ata.cpp

Abstract:
    This is the miniport driver for ATA/ATAPI IDE controllers
    with Busmaster DMA and Serial ATA support

Author:
    Alexander A. Telyatnikov (Alter)

Environment:
    kernel mode only

Notes:

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Revision History:

    The skeleton was taken from standard ATAPI.SYS from NT4 DDK by
         Mike Glass (MGlass)
         Chuck Park (ChuckP)

    Some parts of code were taken from FreeBSD 4.3-6.1 ATA driver by
         Søren Schmidt, Copyright (c) 1998-2007

    All parts of code are greatly changed/updated by
         Alter, Copyright (c) 2002-2007:

    1. Internal command queueing/reordering
    2. Drive identification
    3. Support for 2 _independent_ channels in a single PCI device
    4. Smart host<->drive transfer rate slowdown (for bad cable)
    5. W2k support (binary compatibility)
    6. HDD hot swap under NT4
    7. XP support (binary compatibility)
    8. Serial ATA (SATA/SATA2) support
    9. NT 3.51 support (binary compatibility)
    
    etc. (See todo.txt)


--*/

#include "stdafx.h"

#ifndef UNIATA_CORE

static const CHAR ver_string[] = "\n\nATAPI IDE MiniPort Driver (UniATA) v 0." UNIATA_VER_STR "\n";

UNICODE_STRING SavedRegPath;
WCHAR SavedRegPathBuffer[256];

#endif //UNIATA_CORE

UCHAR AtaCommands48[256];
UCHAR AtaCommandFlags[256];

ULONG  SkipRaids = 1;
ULONG  ForceSimplex = 0;

LONGLONG g_Perf = 0;
ULONG    g_PerfDt = 0;

#ifdef _DEBUG
ULONG  g_LogToDisplay = 0;
#endif //_DEBUG

ULONG  g_WaitBusyInISR = 1;

BOOLEAN InDriverEntry = TRUE;

BOOLEAN g_opt_Verbose = 0;

//UCHAR EnableDma = FALSE;
//UCHAR EnableReorder = FALSE;

UCHAR g_foo = 0;

BOOLEAN
DDKAPI
AtapiResetController__(
    IN PVOID HwDeviceExtension,
    IN ULONG PathId,
    IN UCHAR CompleteType
    );

VOID
AtapiHwInitialize__(
    IN PHW_DEVICE_EXTENSION deviceExtension,
    IN ULONG lChannel
    );

#define RESET_COMPLETE_CURRENT  0x00
#define RESET_COMPLETE_ALL      0x01
#define RESET_COMPLETE_NONE     0x02

#ifndef UNIATA_CORE

VOID
DDKAPI
AtapiCallBack_X(
    IN PVOID HwDeviceExtension
    );

#ifdef UNIATA_USE_XXableInterrupts
  #define RETTYPE_XXableInterrupts   BOOLEAN
  #define RETVAL_XXableInterrupts    TRUE
#else
  #define RETTYPE_XXableInterrupts   VOID
  #define RETVAL_XXableInterrupts
#endif

RETTYPE_XXableInterrupts
DDKAPI
AtapiInterruptDpc(
    IN PVOID HwDeviceExtension
    );

RETTYPE_XXableInterrupts
DDKAPI
AtapiEnableInterrupts__(
    IN PVOID HwDeviceExtension
    );

VOID
AtapiQueueTimerDpc(
    IN PVOID HwDeviceExtension,
    IN ULONG lChannel,
    IN PHW_TIMER HwScsiTimer,
    IN ULONG MiniportTimerValue
    );

SCSI_ADAPTER_CONTROL_STATUS
DDKAPI
AtapiAdapterControl(
    IN PVOID HwDeviceExtension,
    IN SCSI_ADAPTER_CONTROL_TYPE ControlType,
    IN PVOID Parameters
    );

#endif //UNIATA_CORE

BOOLEAN
AtapiCheckInterrupt__(
    IN PVOID HwDeviceExtension,
    IN UCHAR c
    );


#ifndef UNIATA_CORE

BOOLEAN
AtapiRegGetStringParameterValue(
    IN PWSTR RegistryPath,
    IN PWSTR Name,
    IN PWCHAR Str,
    IN ULONG MaxLen
    )
{
#define ITEMS_TO_QUERY 2 // always 1 greater than what is searched 
    NTSTATUS          status;
    RTL_QUERY_REGISTRY_TABLE parameters[ITEMS_TO_QUERY];
    UNICODE_STRING ustr;

    ustr.Buffer = Str;
    ustr.Length =
    ustr.MaximumLength = (USHORT)MaxLen;
    RtlZeroMemory(parameters, (sizeof(RTL_QUERY_REGISTRY_TABLE)*ITEMS_TO_QUERY));

    parameters[0].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    parameters[0].Name          = Name;
    parameters[0].EntryContext  = &ustr;
    parameters[0].DefaultType   = REG_SZ;
    parameters[0].DefaultData   = Str;
    parameters[0].DefaultLength = MaxLen;

    status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE /*| RTL_REGISTRY_OPTIONAL*/,
                                    RegistryPath, parameters, NULL, NULL);

    if(!NT_SUCCESS(status))
        return FALSE;

    return TRUE;

#undef ITEMS_TO_QUERY
} // end AtapiRegGetStringParameterValue()


#endif //UNIATA_CORE

VOID
DDKFASTAPI
UniataNanoSleep(
    ULONG nano
    )
{
    LONGLONG t;
    LARGE_INTEGER t0;

    if(!nano || !g_Perf || !g_PerfDt)
        return;
    t = (g_Perf * nano) / g_PerfDt / 1000;
    if(!t) {
        t = 1;
    }
    do {
        KeQuerySystemTime(&t0);
        t--;
    } while(t);
} // end UniataNanoSleep()


#define AtapiWritePortN_template(_type, _Type, sz) \
VOID \
DDKFASTAPI \
AtapiWritePort##sz( \
    IN PHW_CHANNEL chan, \
    IN ULONG _port, \
    IN _type  data \
    ) \
{ \
    PIORES res; \
    if(_port >= IDX_MAX_REG) { \
        res = (PIORES)(_port);  \
    } else \
    if(chan) { \
        res = &chan->RegTranslation[_port];  \
    } else {                                     \
        KdPrint(("invalid io write request @ ch %x, res* %x\n", chan, _port)); \
        return; \
    } \
    if(!res->MemIo) {             \
        ScsiPortWritePort##_Type((_type*)(res->Addr), data); \
    } else {                                      \
        /*KdPrint(("r_mem @ (%x) %x\n", _port, port));*/ \
        ScsiPortWriteRegister##_Type((_type*)(res->Addr), data); \
    }                                                        \
    return;                                                  \
}

AtapiWritePortN_template(ULONG,  Ulong,  4);
AtapiWritePortN_template(USHORT, Ushort, 2);
AtapiWritePortN_template(UCHAR,  Uchar,  1);

#define AtapiWritePortExN_template(_type, _Type, sz) \
VOID \
DDKFASTAPI \
AtapiWritePortEx##sz( \
    IN PHW_CHANNEL chan, \
    IN ULONG _port, \
    IN ULONG offs, \
    IN _type  data \
    ) \
{ \
    PIORES res; \
    if(_port >= IDX_MAX_REG) { \
        res = (PIORES)(_port);  \
    } else \
    if(chan) { \
        res = &chan->RegTranslation[_port];  \
    } else {                                     \
        KdPrint(("invalid io write request @ ch %x, res* %x, offs %x\n", chan, _port, offs)); \
        return; \
    } \
    if(!res->MemIo) {             \
        ScsiPortWritePort##_Type((_type*)(res->Addr+offs), data); \
    } else {                                      \
        /*KdPrint(("r_mem @ (%x) %x\n", _port, port));*/ \
        ScsiPortWriteRegister##_Type((_type*)(res->Addr+offs), data); \
    }                                                        \
    return;                                                  \
}

AtapiWritePortExN_template(ULONG,  Ulong,  4);
//AtapiWritePortExN_template(USHORT, Ushort, 2);
AtapiWritePortExN_template(UCHAR,  Uchar,  1);

#define AtapiReadPortN_template(_type, _Type, sz) \
_type \
DDKFASTAPI \
AtapiReadPort##sz( \
    IN PHW_CHANNEL chan, \
    IN ULONG _port \
    ) \
{ \
    PIORES res; \
    if(_port >= IDX_MAX_REG) { \
        res = (PIORES)(_port);  \
    } else \
    if(chan) { \
        res = &chan->RegTranslation[_port];  \
    } else {                                     \
        KdPrint(("invalid io read request @ ch %x, res* %x\n", chan, _port)); \
        return (_type)(-1); \
    } \
    if(!res->MemIo) {             \
        /*KdPrint(("r_io @ (%x) %x\n", _port, res->Addr));*/ \
        return ScsiPortReadPort##_Type((_type*)(res->Addr)); \
    } else {                                      \
        /*KdPrint(("r_mem @ (%x) %x\n", _port, res->Addr));*/ \
        return ScsiPortReadRegister##_Type((_type*)(res->Addr)); \
    }                                                        \
}

AtapiReadPortN_template(ULONG,  Ulong,  4);
AtapiReadPortN_template(USHORT, Ushort, 2);
AtapiReadPortN_template(UCHAR,  Uchar,  1);

#define AtapiReadPortExN_template(_type, _Type, sz) \
_type \
DDKFASTAPI \
AtapiReadPortEx##sz( \
    IN PHW_CHANNEL chan, \
    IN ULONG _port, \
    IN ULONG offs \
    ) \
{ \
    PIORES res; \
    if(_port >= IDX_MAX_REG) { \
        res = (PIORES)(_port);  \
    } else \
    if(chan) { \
        res = &chan->RegTranslation[_port];  \
    } else {                                     \
        KdPrint(("invalid io read request @ ch %x, res* %x, offs %x\n", chan, _port, offs)); \
        return (_type)(-1); \
    } \
    if(!res->MemIo) {             \
        return ScsiPortReadPort##_Type((_type*)(res->Addr+offs)); \
    } else {                                      \
        /*KdPrint(("r_mem @ (%x) %x\n", _port, port));*/ \
        return ScsiPortReadRegister##_Type((_type*)(res->Addr+offs)); \
    }                                                        \
}

AtapiReadPortExN_template(ULONG,  Ulong,  4);
//AtapiReadPortExN_template(USHORT, Ushort, 2);
AtapiReadPortExN_template(UCHAR,  Uchar,  1);

#define AtapiReadPortBufferN_template(type, Type, sz) \
VOID \
DDKFASTAPI \
AtapiReadBuffer##sz( \
    IN PHW_CHANNEL chan, \
    IN ULONG _port, \
    IN PVOID Buffer, \
    IN ULONG Count,   \
    IN ULONG Timing   \
    ) \
{ \
    ULONG i=0; \
    while(Count) { \
        ((type*)Buffer)[i] = AtapiReadPort##sz(chan, _port); \
        i++; \
        Count--; \
        UniataNanoSleep(Timing); \
    } \
    return;                                                  \
}

#define AtapiWritePortBufferN_template(type, Type, sz) \
VOID \
DDKFASTAPI \
AtapiWriteBuffer##sz( \
    IN PHW_CHANNEL chan, \
    IN ULONG _port, \
    IN PVOID Buffer, \
    IN ULONG Count,   \
    IN ULONG Timing   \
    ) \
{ \
    ULONG i=0; \
    while(Count) { \
        AtapiWritePort##sz(chan, _port, ((type*)Buffer)[i]); \
        i++; \
        Count--; \
        UniataNanoSleep(Timing); \
    } \
    return;                                                  \
}

AtapiWritePortBufferN_template(ULONG,  Ulong,  4);
AtapiWritePortBufferN_template(USHORT, Ushort, 2);

AtapiReadPortBufferN_template(ULONG,  Ulong,  4);
AtapiReadPortBufferN_template(USHORT, Ushort, 2);


UCHAR
DDKFASTAPI
AtapiSuckPort2(
    IN PHW_CHANNEL chan
    )
{
    UCHAR statusByte;
    ULONG i;

    WaitOnBusyLong(chan);
    for (i = 0; i < 0x10000; i++) {

        GetStatus(chan, statusByte);
        if (statusByte & IDE_STATUS_DRQ) {
            // Suck out any remaining bytes and throw away.
            AtapiReadPort2(chan, IDX_IO1_i_Data);
        } else {
            break;
        }
    }
    if(i) {
        KdPrint2((PRINT_PREFIX "AtapiSuckPort2: overrun detected (%#x words)\n", i ));
    }
    return statusByte;
} // AtapiSuckPort2()

UCHAR
DDKFASTAPI
WaitOnBusy(
    IN PHW_CHANNEL   chan
    )
{
    ULONG i;
    UCHAR Status;
    for (i=0; i<200; i++) {
        GetStatus(chan, Status);
        if (Status & IDE_STATUS_BUSY) {
            AtapiStallExecution(10);
            continue;
        } else {
            break;
        }
    }
    return Status;
} // end WaitOnBusy()

UCHAR
DDKFASTAPI
WaitOnBusyLong(
    IN PHW_CHANNEL   chan
    )
{
    ULONG i;
    UCHAR Status;

    Status = WaitOnBusy(chan);
    if(!(Status & IDE_STATUS_BUSY))
        return Status;
    for (i=0; i<2000; i++) {
        GetStatus(chan, Status);
        if (Status & IDE_STATUS_BUSY) {
            AtapiStallExecution(250);
            continue;
        } else {
            break;
        }
    }
    return Status;
} // end WaitOnBusyLong()

UCHAR
DDKFASTAPI
WaitOnBaseBusy(
    IN PHW_CHANNEL   chan
    )
{
    ULONG i;
    UCHAR Status;
    for (i=0; i<200; i++) {
        GetBaseStatus(chan, Status);
        if (Status & IDE_STATUS_BUSY) {
            AtapiStallExecution(10);
            continue;
        } else {
            break;
        }
    }
    return Status;
} // end WaitOnBaseBusy()

UCHAR
DDKFASTAPI
WaitOnBaseBusyLong(
    IN PHW_CHANNEL   chan
    )
{
    ULONG i;
    UCHAR Status;

    Status = WaitOnBaseBusy(chan);
    if(!(Status & IDE_STATUS_BUSY))
        return Status;
    for (i=0; i<2000; i++) {
        GetBaseStatus(chan, Status);
        if (Status & IDE_STATUS_BUSY) {
            AtapiStallExecution(250);
            continue;
        } else {
            break;
        }
    }
    return Status;
} // end WaitOnBaseBusyLong()

UCHAR
DDKFASTAPI
UniataIsIdle(
    IN struct _HW_DEVICE_EXTENSION* deviceExtension,
    IN UCHAR Status
    )
{
    UCHAR Status2;

    if(Status == 0xff) {
        return 0xff;
    }
    if(Status & IDE_STATUS_BUSY) {
        return Status;
    }
//    if(deviceExtension->HwFlags & UNIATA_SATA) {
    if(deviceExtension->BaseIoAddressSATA_0.Addr) {
        if(Status & (IDE_STATUS_BUSY | IDE_STATUS_ERROR)) {
            return Status;
        }
    } else {
        Status2 = Status & ~(IDE_STATUS_ERROR | IDE_STATUS_INDEX);
        if ((Status & IDE_STATUS_BUSY) ||
            (Status2 != IDE_STATUS_IDLE && Status2 != IDE_STATUS_DRDY)) {
            return Status;
        }
    }
    return IDE_STATUS_IDLE;
} // end UniataIsIdle()

UCHAR
DDKFASTAPI
WaitForIdleLong(
    IN PHW_CHANNEL   chan
    )
{
    ULONG i;
    UCHAR Status;
    UCHAR Status2;
    for (i=0; i<20000; i++) {
        GetStatus(chan, Status);
        Status2 = UniataIsIdle(chan->DeviceExtension, Status);
        if(Status2 == 0xff) {
            // no drive ?
            break;
        } else
        if(Status2 & IDE_STATUS_BUSY) {
            AtapiStallExecution(10);
            continue;
        } else {
            break;
        }
    }
    return Status;
} // end WaitForIdleLong()

UCHAR
DDKFASTAPI
WaitForDrq(
    IN PHW_CHANNEL   chan
    )
{
    ULONG i;
    UCHAR Status;
    for (i=0; i<1000; i++) {
        GetStatus(chan, Status);
        if (Status & IDE_STATUS_BUSY) {
            AtapiStallExecution(10);
        } else if (Status & IDE_STATUS_DRQ) {
            break;
        } else {
            AtapiStallExecution(10);
        }
    }
    return Status;
} // end WaitForDrq()

UCHAR
DDKFASTAPI
WaitShortForDrq(
    IN PHW_CHANNEL   chan
    )
{
    ULONG i;
    UCHAR Status;
    for (i=0; i<2; i++) {
        GetStatus(chan, Status);
        if (Status & IDE_STATUS_BUSY) {
            AtapiStallExecution(10);
        } else if (Status & IDE_STATUS_DRQ) {
            break;
        } else {
            AtapiStallExecution(10);
        }
    }
    return Status;
} // end WaitShortForDrq()

VOID
DDKFASTAPI
AtapiSoftReset(
    IN PHW_CHANNEL   chan,
    ULONG            DeviceNumber
    )
{
    //ULONG c = chan->lChannel;
    ULONG i;
    UCHAR dma_status = 0;
    KdPrint2((PRINT_PREFIX "AtapiSoftReset:\n"));
    UCHAR statusByte2;

    GetBaseStatus(chan, statusByte2);
    KdPrint2((PRINT_PREFIX "  statusByte2 %x:\n", statusByte2));
    SelectDrive(chan, DeviceNumber);
    AtapiStallExecution(10000);
    AtapiWritePort1(chan, IDX_IO1_o_Command, IDE_COMMAND_ATAPI_RESET);
    for (i = 0; i < 1000; i++) {
        AtapiStallExecution(999);
    }
    SelectDrive(chan, DeviceNumber);
    WaitOnBusy(chan);
    GetBaseStatus(chan, statusByte2);
    AtapiStallExecution(500);

    GetBaseStatus(chan, statusByte2);
    if(chan && chan->DeviceExtension) {
        dma_status = GetDmaStatus(chan->DeviceExtension, chan->lChannel);
        KdPrint2((PRINT_PREFIX "  DMA status %#x\n", dma_status));
    } else {
        KdPrint2((PRINT_PREFIX "  can't get DMA status\n"));
    }
    if(dma_status & BM_STATUS_INTR) {
        // bullshit, we have DMA interrupt, but had never initiate DMA operation
        KdPrint2((PRINT_PREFIX "  clear unexpected DMA intr on ATAPI reset\n"));
        AtapiDmaDone(chan->DeviceExtension, DeviceNumber, chan->lChannel, NULL);
        GetBaseStatus(chan, statusByte2);
    }
    if(chan->DeviceExtension->HwFlags & UNIATA_SATA) {
        UniataSataClearErr(chan->DeviceExtension, chan->lChannel, UNIATA_SATA_IGNORE_CONNECT);
    }
    return;

} // end AtapiSoftReset()

/*
    Send command to device.
    Translate to 48-Lba form if required
*/
UCHAR
AtaCommand48(
    IN PHW_DEVICE_EXTENSION deviceExtension,
    IN ULONG DeviceNumber,
    IN ULONG lChannel,
    IN UCHAR command,
    IN ULONGLONG lba,
    IN USHORT count,
    IN USHORT feature,
    IN ULONG flags
    )
{
    PHW_CHANNEL          chan = &deviceExtension->chan[lChannel];
    UCHAR                statusByte;
    ULONG ldev = lChannel*2 + DeviceNumber;
    ULONG i;
    PUCHAR plba;

    KdPrint2((PRINT_PREFIX "AtaCommand48: cntrlr %#x:%#x ldev %#x, cmd %#x, lba %#I64x count %#x feature %#x\n",
                 deviceExtension->DevIndex, deviceExtension->Channel, ldev, command, lba, count, feature ));

    //if(!(deviceExtension->HwFlags & UNIATA_SATA)) {
        SelectDrive(chan, DeviceNumber);

        statusByte = WaitOnBusy(chan);

        /* ready to issue command ? */
        if (statusByte & IDE_STATUS_BUSY) {
            KdPrint2((PRINT_PREFIX "  Returning BUSY status\n"));
            return statusByte;
        }
    //}
    // !!! We should not check ERROR condition here
    // ERROR bit may be asserted durring previous operation
    // and not cleared after SELECT

    //>>>>>> NV: 2006/08/03
    if((AtaCommandFlags[command] & ATA_CMD_FLAG_LBAIOsupp) &&
       CheckIfBadBlock(&(deviceExtension->lun[ldev]), lba, count)) {
        KdPrint2((PRINT_PREFIX ": artificial bad block, lba %#I64x count %#x\n", lba, count));
        return IDE_STATUS_ERROR;
        //return SRB_STATUS_ERROR;
    }
    //<<<<<< NV:  2006/08/03

    /* only use 48bit addressing if needed because of the overhead */
    if ((lba >= ATA_MAX_LBA28 || count > 256) &&
        deviceExtension->lun[ldev].IdentifyData.FeaturesSupport.Address48) {

        KdPrint2((PRINT_PREFIX "  ldev %#x USE_LBA_48\n", ldev ));
        /* translate command into 48bit version */
        if(AtaCommandFlags[command] & ATA_CMD_FLAG_48supp) {
            command = AtaCommands48[command];
        } else {
            KdPrint2((PRINT_PREFIX "  unhandled LBA48 command\n"));
			return (UCHAR)-1;
        }

        chan->ChannelCtrlFlags |= CTRFLAGS_LBA48;
        plba = (PUCHAR)&lba;

        AtapiWritePort1(chan, IDX_IO1_o_Feature,      (UCHAR)(feature>>8) & 0xff);
        AtapiWritePort1(chan, IDX_IO1_o_Feature,      (UCHAR)feature);
        AtapiWritePort1(chan, IDX_IO1_o_BlockCount,   (UCHAR)(count>>8) & 0xff);
        AtapiWritePort1(chan, IDX_IO1_o_BlockCount,   (UCHAR)count & 0xff);
        AtapiWritePort1(chan, IDX_IO1_o_BlockNumber,  (UCHAR)(plba[3]));
        AtapiWritePort1(chan, IDX_IO1_o_BlockNumber,  (UCHAR)(plba[0]));
        AtapiWritePort1(chan, IDX_IO1_o_CylinderLow,  (UCHAR)(plba[4]));
        AtapiWritePort1(chan, IDX_IO1_o_CylinderLow,  (UCHAR)(plba[1]));
        AtapiWritePort1(chan, IDX_IO1_o_CylinderHigh, (UCHAR)(plba[5]));
        AtapiWritePort1(chan, IDX_IO1_o_CylinderHigh, (UCHAR)(plba[2]));

        //KdPrint2((PRINT_PREFIX "AtaCommand48: ldev %#x USE_LBA48 (2)\n", ldev ));
        AtapiWritePort1(chan, IDX_IO1_o_DriveSelect, IDE_USE_LBA | (DeviceNumber ? IDE_DRIVE_2 : IDE_DRIVE_1) );
    } else {

        chan->ChannelCtrlFlags &= ~CTRFLAGS_LBA48;
        
        //if(feature ||
        //   (deviceExtension->lun[ldev].DeviceFlags & (DFLAGS_ATAPI_DEVICE | DFLAGS_TAPE_DEVICE | DFLAGS_LBA_ENABLED))) {
            AtapiWritePort1(chan, IDX_IO1_o_Feature,      (UCHAR)feature);
        //}
        AtapiWritePort1(chan, IDX_IO1_o_BlockCount,   (UCHAR)count);
        AtapiWritePort1(chan, IDX_IO1_o_BlockNumber,  (UCHAR)lba & 0xff);
        AtapiWritePort1(chan, IDX_IO1_o_CylinderLow,  (UCHAR)(lba>>8) & 0xff);
        AtapiWritePort1(chan, IDX_IO1_o_CylinderHigh, (UCHAR)(lba>>16) & 0xff);
        if(deviceExtension->lun[ldev].DeviceFlags & DFLAGS_LBA_ENABLED) {
            //KdPrint2((PRINT_PREFIX "AtaCommand28: ldev %#x USE_LBA\n", ldev ));
            AtapiWritePort1(chan, IDX_IO1_o_DriveSelect,  (UCHAR)((lba>>24) & 0xf) | IDE_USE_LBA | (DeviceNumber ? IDE_DRIVE_SELECT_2 : IDE_DRIVE_SELECT_1) );
        } else {
            //KdPrint2((PRINT_PREFIX "AtaCommand28: ldev %#x USE_CHS\n", ldev ));
            AtapiWritePort1(chan, IDX_IO1_o_DriveSelect,  (UCHAR)((lba>>24) & 0xf) | (DeviceNumber ? IDE_DRIVE_SELECT_2 : IDE_DRIVE_SELECT_1) );
        }
    }

    // write command code to device
    AtapiWritePort1(chan, IDX_IO1_o_Command, command);

    switch (flags) {
    case ATA_WAIT_INTR:

        // caller requested wait for interrupt
        for(i=0;i<4;i++) {
            WaitOnBusy(chan);
            statusByte = WaitForDrq(chan);
            if (statusByte & IDE_STATUS_DRQ)
                break;
            AtapiStallExecution(500);
            KdPrint2((PRINT_PREFIX "  retry waiting DRQ, status %#x\n", statusByte));
        }

        return statusByte;

    case ATA_WAIT_IDLE:

        // caller requested wait for entering Wait state
        for (i=0; i<30 * 1000; i++) {

            GetStatus(chan, statusByte);
            statusByte = UniataIsIdle(deviceExtension, statusByte);
            if(statusByte == 0xff) {
                // no drive ?
                break;
            } else
            if(statusByte & IDE_STATUS_ERROR) {
                break;
            } else
            if(statusByte & IDE_STATUS_BUSY) {
                AtapiStallExecution(100);
                continue;
            } else
            if(statusByte == IDE_STATUS_IDLE) {
                break;
            } else {
                //if(deviceExtension->HwFlags & UNIATA_SATA) {
                if(deviceExtension->BaseIoAddressSATA_0.Addr) {
                    break;
                }
                AtapiStallExecution(100);
            }
        }
        //statusByte |= IDE_STATUS_BUSY;
        break;

    case ATA_WAIT_READY:
        statusByte = WaitOnBusyLong(chan);
        break;
    case ATA_WAIT_BASE_READY:
        statusByte = WaitOnBaseBusyLong(chan);
        break;
    case ATA_IMMEDIATE:
        GetStatus(chan, statusByte);
        if (statusByte & IDE_STATUS_ERROR) {
            KdPrint2((PRINT_PREFIX "  Warning: Immed Status %#x :(\n", statusByte));
            if(statusByte == (IDE_STATUS_IDLE | IDE_STATUS_ERROR)) {
                break;
            }
            KdPrint2((PRINT_PREFIX "  try to continue\n"));
            statusByte &= ~IDE_STATUS_ERROR;
        }
        chan->ExpectingInterrupt = TRUE;
        // !!!!!
        InterlockedExchange(&(chan->CheckIntr),
                                      CHECK_INTR_IDLE);
        statusByte = 0;
        break;
    }

    KdPrint2((PRINT_PREFIX "  Status %#x\n", statusByte));

    return statusByte;
} // end AtaCommand48()

/*
    Send command to device.
    This is simply wrapper for AtaCommand48()
*/
UCHAR
AtaCommand(
    IN PHW_DEVICE_EXTENSION deviceExtension,
    IN ULONG DeviceNumber,
    IN ULONG lChannel,
    IN UCHAR command,
    IN USHORT cylinder,
    IN UCHAR head,
    IN UCHAR sector, 
    IN UCHAR count,
    IN UCHAR feature,
    IN ULONG flags
    )
{
    return AtaCommand48(deviceExtension, DeviceNumber, lChannel,
                        command, 
                        (ULONG)sector | ((ULONG)cylinder << 8) | ((ULONG)(head & 0x0f) << 24),
                        count, feature, flags);
} // end AtaCommand()

LONG
AtaPio2Mode(LONG pio)
{
    switch (pio) {
    default: return ATA_PIO;
    case 0: return ATA_PIO0;
    case 1: return ATA_PIO1;
    case 2: return ATA_PIO2;
    case 3: return ATA_PIO3;
    case 4: return ATA_PIO4;
    case 5: return ATA_PIO5;
    }
} // end AtaPio2Mode()

LONG
AtaPioMode(PIDENTIFY_DATA2 ident)
{
    if (ident->PioTimingsValid) {
        if (ident->AdvancedPIOModes & AdvancedPIOModes_5)
            return 5;
        if (ident->AdvancedPIOModes & AdvancedPIOModes_4)
            return 4;
        if (ident->AdvancedPIOModes & AdvancedPIOModes_3) 
            return 3;
    }        
    if (ident->PioCycleTimingMode == 2)
        return 2;
    if (ident->PioCycleTimingMode == 1)
        return 1;
    if (ident->PioCycleTimingMode == 0)
        return 0;
    return -1; 
} // end AtaPioMode()

LONG
AtaWmode(PIDENTIFY_DATA2 ident)
{
    if (ident->MultiWordDMASupport & 0x04)
        return 2;
    if (ident->MultiWordDMASupport & 0x02)
        return 1;
    if (ident->MultiWordDMASupport & 0x01)
        return 0;
    return -1;
} // end AtaWmode()

LONG
AtaUmode(PIDENTIFY_DATA2 ident)
{
    if (!ident->UdmaModesValid)
        return -1;
    if (ident->UltraDMASupport & 0x40)
        return 6;
    if (ident->UltraDMASupport & 0x20)
        return 5;
    if (ident->UltraDMASupport & 0x10)
        return 4;
    if (ident->UltraDMASupport & 0x08)
        return 3;
    if (ident->UltraDMASupport & 0x04)
        return 2;
    if (ident->UltraDMASupport & 0x02)
        return 1;
    if (ident->UltraDMASupport & 0x01)
        return 0;
    return -1;
} // end AtaUmode()


#ifndef UNIATA_CORE

VOID
DDKAPI
AtapiTimerDpc(
    IN PVOID HwDeviceExtension
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    PHW_TIMER HwScsiTimer;
    LARGE_INTEGER time;
    ULONG MiniportTimerValue;
    BOOLEAN recall = FALSE;
    ULONG lChannel;
    PHW_CHANNEL chan;

    KdPrint2((PRINT_PREFIX "AtapiTimerDpc:\n"));

    lChannel = deviceExtension->ActiveDpcChan = deviceExtension->FirstDpcChan;
    if(lChannel == (ULONG)-1) {
        KdPrint2((PRINT_PREFIX "AtapiTimerDpc: no items\n"));
        return;
    }
    chan = &deviceExtension->chan[lChannel];

    while(TRUE) {

        HwScsiTimer = chan->HwScsiTimer;
        chan->HwScsiTimer = NULL;

        deviceExtension->FirstDpcChan = chan->NextDpcChan;
        if(deviceExtension->FirstDpcChan != (ULONG)-1) {
            recall = TRUE;
        }

        HwScsiTimer(HwDeviceExtension);

        chan->NextDpcChan = -1;

        lChannel = deviceExtension->ActiveDpcChan = deviceExtension->FirstDpcChan;
        if(lChannel == (ULONG)-1) {
            KdPrint2((PRINT_PREFIX "AtapiTimerDpc: no more items\n"));
            deviceExtension->FirstDpcChan =
            deviceExtension->ActiveDpcChan = -1;
            return;
        }

        KeQuerySystemTime(&time);
        KdPrint2((PRINT_PREFIX "AtapiTimerDpc: KeQuerySystemTime=%#x%#x\n", time.HighPart, time.LowPart));

        chan = &deviceExtension->chan[lChannel];
        if(time.QuadPart >= chan->DpcTime - 10) {
            // call now
            KdPrint2((PRINT_PREFIX "AtapiTimerDpc: get next DPC, DpcTime1=%#x%#x\n",
                         (ULONG)(chan->DpcTime >> 32), (ULONG)(chan->DpcTime)));
            continue;
        }
        break;
    }

    if(recall) {
        deviceExtension->ActiveDpcChan = -1;
        MiniportTimerValue = (ULONG)(time.QuadPart - chan->DpcTime)/10;
        if(!MiniportTimerValue)
            MiniportTimerValue = 1;

        KdPrint2((PRINT_PREFIX "AtapiTimerDpc: recall AtapiTimerDpc\n"));
        ScsiPortNotification(RequestTimerCall, HwDeviceExtension,
                             AtapiTimerDpc,
                             MiniportTimerValue
                             );
    }
    return;

} // end AtapiTimerDpc()

/*
    Wrapper for ScsiPort, that implements smart Dpc
    queueing. We need it to allow parallel functioning
    of IDE channles with shared interrupt. Standard Dpc mechanism
    cancels previous Dpc request (if any), but we need Dpc queue.
*/
VOID
AtapiQueueTimerDpc(
    IN PVOID HwDeviceExtension,
    IN ULONG lChannel,
    IN PHW_TIMER HwScsiTimer,
    IN ULONG MiniportTimerValue
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    LARGE_INTEGER time;
    LARGE_INTEGER time2;
    ULONG i;
    PHW_CHANNEL prev_chan;
    PHW_CHANNEL chan;
//    BOOLEAN UseRequestTimerCall = TRUE;

    KdPrint2((PRINT_PREFIX "AtapiQueueTimerDpc: dt=%d for lChn %#x\n", MiniportTimerValue, lChannel));
    KeQuerySystemTime(&time);
    time2 = time;
    KdPrint2((PRINT_PREFIX "AtapiQueueTimerDpc: KeQuerySystemTime=%#x%#x\n", time.HighPart, time.LowPart));
    time.QuadPart += MiniportTimerValue*10;
    KdPrint2((PRINT_PREFIX "AtapiQueueTimerDpc: KeQuerySystemTime2=%#x%#x\n", time.HighPart, time.LowPart));

    KdPrint2((PRINT_PREFIX "  ActiveDpcChan=%d, FirstDpcChan=%d\n", deviceExtension->ActiveDpcChan, deviceExtension->FirstDpcChan));

    i = deviceExtension->FirstDpcChan;
    chan = prev_chan = NULL;
    while(i != (ULONG)-1) {
        prev_chan = chan;
        chan = &deviceExtension->chan[i];
        if(chan->DpcTime > time.QuadPart) {
            break;
        }
        i = chan->NextDpcChan;
    }
    chan = &deviceExtension->chan[lChannel];
    if(!prev_chan) {
        deviceExtension->FirstDpcChan = lChannel;
    } else {
        prev_chan->NextDpcChan = lChannel;
    }
    chan->NextDpcChan = i;
    chan->HwScsiTimer = HwScsiTimer;
    chan->DpcTime     = time.QuadPart;

    KdPrint2((PRINT_PREFIX "AtapiQueueTimerDpc: KeQuerySystemTime3=%#x%#x\n", time2.HighPart, time2.LowPart));
    if(time.QuadPart <= time2.QuadPart) {
        MiniportTimerValue = 1;
    } else {
        MiniportTimerValue = (ULONG)((time.QuadPart - time2.QuadPart) / 10);
    }

    KdPrint2((PRINT_PREFIX "AtapiQueueTimerDpc: dt=%d for lChn %#x\n", MiniportTimerValue, lChannel));
    ScsiPortNotification(RequestTimerCall, HwDeviceExtension,
                         AtapiTimerDpc,
                         MiniportTimerValue);

} // end AtapiQueueTimerDpc()

#endif //UNIATA_CORE

VOID
UniataDumpATARegs(
    IN PHW_CHANNEL chan
    )
{
    ULONG                j;
    UCHAR                statusByteAlt;

    GetStatus(chan, statusByteAlt);
    KdPrint2((PRINT_PREFIX "  AltStatus (%#x)\n", statusByteAlt));

    for(j=1; j<IDX_IO1_SZ; j++) {
        statusByteAlt = AtapiReadPort1(chan, IDX_IO1+j);
        KdPrint2((PRINT_PREFIX 
                   "  Reg_%#x (%#x) = %#x\n",
                   j,
                   chan->RegTranslation[IDX_IO1+j].Addr,
                   statusByteAlt));
    }
    for(j=0; j<IDX_BM_IO_SZ-1; j++) {
        statusByteAlt = AtapiReadPort1(chan, IDX_BM_IO+j);
        KdPrint2((PRINT_PREFIX 
                   "  BM_%#x (%#x) = %#x\n",
                   j,
                   chan->RegTranslation[IDX_BM_IO+j].Addr,
                   statusByteAlt));
    }
    return;
} // end UniataDumpATARegs()

/*++

Routine Description:

    Issue IDENTIFY command to a device.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    DeviceNumber - Indicates which device.
    Command - Either the standard (EC) or the ATAPI packet (A1) IDENTIFY.

Return Value:

    TRUE if all goes well.

--*/
BOOLEAN
IssueIdentify(
    IN PVOID HwDeviceExtension,
    IN ULONG DeviceNumber,
    IN ULONG lChannel,
    IN UCHAR Command,
    IN BOOLEAN NoSetup
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    PHW_CHANNEL          chan = &deviceExtension->chan[lChannel];
    ULONG                waitCount = 50000;
    ULONG                j;
    UCHAR                statusByte;
    UCHAR                statusByte2;
    UCHAR                signatureLow,
                         signatureHigh;
    BOOLEAN              atapiDev = FALSE;
    ULONG                ldev = (lChannel * 2) + DeviceNumber;
    PHW_LU_EXTENSION     LunExt = &(deviceExtension->lun[ldev]);

    if(DeviceNumber && (chan->ChannelCtrlFlags & CTRFLAGS_NO_SLAVE)) {
        KdPrint2((PRINT_PREFIX "IssueIdentify: NO SLAVE\n"));
        return FALSE;
    }

    SelectDrive(chan, DeviceNumber);
    AtapiStallExecution(10);
    statusByte = WaitOnBusyLong(chan);
    // Check that the status register makes sense.
    GetBaseStatus(chan, statusByte2);

    UniataDumpATARegs(chan);

    if (Command == IDE_COMMAND_IDENTIFY) {
        // Mask status byte ERROR bits.
        statusByte = UniataIsIdle(deviceExtension, statusByte & ~(IDE_STATUS_ERROR | IDE_STATUS_INDEX));
        KdPrint2((PRINT_PREFIX "IssueIdentify: Checking for IDE. Status (%#x)\n", statusByte));
        // Check if register value is reasonable.

        if(statusByte != IDE_STATUS_IDLE) {

            // No reset here !!!
            KdPrint2((PRINT_PREFIX "IssueIdentify: statusByte != IDE_STATUS_IDLE\n"));

            //if(!(deviceExtension->HwFlags & UNIATA_SATA)) {
            if(!deviceExtension->BaseIoAddressSATA_0.Addr) {
                SelectDrive(chan, DeviceNumber);
                WaitOnBusyLong(chan);

                signatureLow = AtapiReadPort1(chan, IDX_IO1_i_CylinderLow);
                signatureHigh = AtapiReadPort1(chan, IDX_IO1_i_CylinderHigh);

                if (signatureLow == ATAPI_MAGIC_LSB &&
                    signatureHigh == ATAPI_MAGIC_MSB) {
                    // Device is Atapi.
                    KdPrint2((PRINT_PREFIX "IssueIdentify: this is ATAPI (ldev %d)\n", ldev));
                    return FALSE;
                }

                // We really should wait up to 31 seconds
                // The ATA spec. allows device 0 to come back from BUSY in 31 seconds!
                // (30 seconds for device 1)
                do {
                    // Wait for Busy to drop.
                    AtapiStallExecution(100);
                    GetStatus(chan, statusByte);

                } while ((statusByte & IDE_STATUS_BUSY) && waitCount--);
                GetBaseStatus(chan, statusByte2);

                SelectDrive(chan, DeviceNumber);
            } else {
                GetBaseStatus(chan, statusByte2);
            }
            // Another check for signature, to deal with one model Atapi that doesn't assert signature after
            // a soft reset.
            signatureLow = AtapiReadPort1(chan, IDX_IO1_i_CylinderLow);
            signatureHigh = AtapiReadPort1(chan, IDX_IO1_i_CylinderHigh);

            if (signatureLow == ATAPI_MAGIC_LSB &&
                signatureHigh == ATAPI_MAGIC_MSB) {
                KdPrint2((PRINT_PREFIX "IssueIdentify: this is ATAPI (2) (ldev %d)\n", ldev));
                // Device is Atapi.
                return FALSE;
            }

            statusByte = UniataIsIdle(deviceExtension, statusByte) & ~IDE_STATUS_INDEX;
            if (statusByte != IDE_STATUS_IDLE) {
                // Give up on this.
                KdPrint2((PRINT_PREFIX "IssueIdentify: no dev (ldev %d)\n", ldev));
                return FALSE;
            }
        }
    } else {
        KdPrint2((PRINT_PREFIX "IssueIdentify: Checking for ATAPI. Status (%#x)\n", statusByte));
        //if(!(deviceExtension->HwFlags & UNIATA_SATA)) {
        if(!deviceExtension->BaseIoAddressSATA_0.Addr) {
            statusByte = WaitForIdleLong(chan);
            KdPrint2((PRINT_PREFIX "IssueIdentify: Checking for ATAPI (2). Status (%#x)\n", statusByte));
        }
        atapiDev = TRUE;
    }

//    if(deviceExtension->HwFlags & UNIATA_SATA) {
    if(deviceExtension->BaseIoAddressSATA_0.Addr) {
        j = 4;
    } else {
        j = 0;
    }
    for (; j < 4*2; j++) {
        // Send IDENTIFY command.
        statusByte = AtaCommand(deviceExtension, DeviceNumber, lChannel, Command, 0, 0, 0, (j >= 4) ? 0x200 : 0, 0, ATA_WAIT_INTR);
        // Clear interrupt

        if (statusByte & IDE_STATUS_DRQ) {
            // Read status to acknowledge any interrupts generated.
            KdPrint2((PRINT_PREFIX "IssueIdentify: IDE_STATUS_DRQ (%#x)\n", statusByte));
            GetBaseStatus(chan, statusByte);
            // One last check for Atapi.
            signatureLow = AtapiReadPort1(chan, IDX_IO1_i_CylinderLow);
            signatureHigh = AtapiReadPort1(chan, IDX_IO1_i_CylinderHigh);

            if (signatureLow == ATAPI_MAGIC_LSB &&
                signatureHigh == ATAPI_MAGIC_MSB) {
                KdPrint2((PRINT_PREFIX "IssueIdentify: this is ATAPI (3) (ldev %d)\n", ldev));
                // Device is Atapi.
                return FALSE;
            }
            break;
        } else {
            KdPrint2((PRINT_PREFIX "IssueIdentify: !IDE_STATUS_DRQ (%#x)\n", statusByte));
            if (Command == IDE_COMMAND_IDENTIFY) {
                // Check the signature. If DRQ didn't come up it's likely Atapi.
                signatureLow = AtapiReadPort1(chan, IDX_IO1_i_CylinderLow);
                signatureHigh = AtapiReadPort1(chan, IDX_IO1_i_CylinderHigh);

                if (signatureLow == ATAPI_MAGIC_LSB &&
                    signatureHigh == ATAPI_MAGIC_MSB) {
                    // Device is Atapi.
                    KdPrint2((PRINT_PREFIX "IssueIdentify: this is ATAPI (4) (ldev %d)\n", ldev));
                    return FALSE;
                }
            }
            // Device didn't respond correctly. It will be given one more chances.
            KdPrint2((PRINT_PREFIX "IssueIdentify: DRQ never asserted (%#x). Error reg (%#x)\n",
                        statusByte, AtapiReadPort1(chan, IDX_IO1_i_Error)));
            GetBaseStatus(chan, statusByte);
            AtapiSoftReset(chan,DeviceNumber);

            AtapiDisableInterrupts(deviceExtension, lChannel);
            AtapiEnableInterrupts(deviceExtension, lChannel);

            GetBaseStatus(chan, statusByte);
            //GetStatus(chan, statusByte);
            KdPrint2((PRINT_PREFIX "IssueIdentify: Status after soft reset (%#x)\n", statusByte));
        }
    }
    // Check for error on really stupid master devices that assert random
    // patterns of bits in the status register at the slave address.
    if ((Command == IDE_COMMAND_IDENTIFY) && (statusByte & IDE_STATUS_ERROR)) {
        KdPrint2((PRINT_PREFIX "IssueIdentify: Exit on error (%#x)\n", statusByte));
        return FALSE;
    }

    KdPrint2((PRINT_PREFIX "IssueIdentify: Status before read words %#x\n", statusByte));
    // Suck out 256 words. After waiting for one model that asserts busy
    // after receiving the Packet Identify command.
    statusByte = WaitForDrq(chan);
    statusByte = WaitOnBusyLong(chan);
        KdPrint2((PRINT_PREFIX "IssueIdentify: statusByte %#x\n", statusByte));

    if (!(statusByte & IDE_STATUS_DRQ)) {
        KdPrint2((PRINT_PREFIX "IssueIdentify: !IDE_STATUS_DRQ (2) (%#x)\n", statusByte));
        GetBaseStatus(chan, statusByte);
        return FALSE;
    }
    GetBaseStatus(chan, statusByte);
    KdPrint2((PRINT_PREFIX "IssueIdentify: BASE statusByte %#x\n", statusByte));

    if (atapiDev || !(LunExt->DeviceFlags & DFLAGS_DWORDIO_ENABLED) /*!deviceExtension->DWordIO*/) {
        
        KdPrint2((PRINT_PREFIX "  use 16bit IO\n"));
#if 0
        USHORT w;
        ULONG i;
        // ATI/SII chipsets with memory-mapped IO hangs when
        // I call ReadBuffer(), probably due to PCI burst/prefetch enabled
        // Unfortunately, I don't know yet how to workaround it except the way you see below.
        KdPrint2((PRINT_PREFIX 
                   "  IO_%#x (%#x), %s:\n",
                   IDX_IO1_i_Data,
                   chan->RegTranslation[IDX_IO1_i_Data].Addr,
                   chan->RegTranslation[IDX_IO1_i_Data].MemIo ? "Mem" : "IO"));
        for(i=0; i<256; i++) {
/*
            KdPrint2((PRINT_PREFIX 
                       "  IO_%#x (%#x):\n",
                       IDX_IO1_i_Data,
                       chan->RegTranslation[IDX_IO1_i_Data].Addr));
*/
            w = AtapiReadPort2(chan, IDX_IO1_i_Data);
            KdPrint2((PRINT_PREFIX 
                       "    %x\n", w));
            AtapiStallExecution(1);
            ((PUSHORT)&deviceExtension->FullIdentifyData)[i] = w;
        }
#else
        ReadBuffer(chan, (PUSHORT)&deviceExtension->FullIdentifyData, 256, PIO0_TIMING);
#endif
        // Work around for some IDE and one model Atapi that will present more than
        // 256 bytes for the Identify data.
        KdPrint2((PRINT_PREFIX "IssueIdentify: suck data port\n", statusByte));
        statusByte = AtapiSuckPort2(chan);
    } else {
        KdPrint2((PRINT_PREFIX "  use 32bit IO\n"));
        ReadBuffer2(chan, (PUSHORT)&deviceExtension->FullIdentifyData, 256/2, PIO0_TIMING);
    }

    KdPrint2((PRINT_PREFIX "IssueIdentify: statusByte %#x\n", statusByte));
    statusByte = WaitForDrq(chan);
    KdPrint2((PRINT_PREFIX "IssueIdentify: statusByte %#x\n", statusByte));
    GetBaseStatus(chan, statusByte);

    KdPrint2((PRINT_PREFIX "IssueIdentify: Status after read words %#x\n", statusByte));

    if(NoSetup) {
        KdPrint2((PRINT_PREFIX "IssueIdentify: no setup, exiting\n"));
        return TRUE;
    }

    KdPrint2((PRINT_PREFIX "Model: %20.20s\n", deviceExtension->FullIdentifyData.ModelNumber));
    KdPrint2((PRINT_PREFIX "FW:    %4.4s\n", deviceExtension->FullIdentifyData.FirmwareRevision));
    KdPrint2((PRINT_PREFIX "S/N:   %20.20s\n", deviceExtension->FullIdentifyData.SerialNumber));
    KdPrint2((PRINT_PREFIX "Pio:   %x\n", deviceExtension->FullIdentifyData.PioCycleTimingMode));
    if(deviceExtension->FullIdentifyData.PioTimingsValid) {
        KdPrint2((PRINT_PREFIX "APio:  %x\n", deviceExtension->FullIdentifyData.AdvancedPIOModes));
    }
    KdPrint2((PRINT_PREFIX "SWDMA: %x\n", deviceExtension->FullIdentifyData.SingleWordDMAActive));
    KdPrint2((PRINT_PREFIX "MWDMA: %x\n", deviceExtension->FullIdentifyData.MultiWordDMAActive));
    if(deviceExtension->FullIdentifyData.UdmaModesValid) {
        KdPrint2((PRINT_PREFIX "UDMA:  %x\n", deviceExtension->FullIdentifyData.UltraDMAActive));
    }
    KdPrint2((PRINT_PREFIX "SATA:  %x\n", deviceExtension->FullIdentifyData.SataEnable));

    // Check out a few capabilities / limitations of the device.
    if (deviceExtension->FullIdentifyData.RemovableStatus & 1) {
        // Determine if this drive supports the MSN functions.
        KdPrint2((PRINT_PREFIX "IssueIdentify: Marking drive %d as removable. SFE = %d\n",
                    ldev,
                    deviceExtension->FullIdentifyData.RemovableStatus));
        LunExt->DeviceFlags |= DFLAGS_REMOVABLE_DRIVE;
    }
    if (deviceExtension->FullIdentifyData.MaximumBlockTransfer) {
        // Determine max. block transfer for this device.
        LunExt->MaximumBlockXfer =
            (UCHAR)(deviceExtension->FullIdentifyData.MaximumBlockTransfer & 0xFF);
    }
    LunExt->NumOfSectors = 0;
    if (Command == IDE_COMMAND_IDENTIFY) {
        ULONGLONG NumOfSectors=0;
        ULONGLONG NativeNumOfSectors=0;
        ULONGLONG cylinders=0;
        ULONGLONG tmp_cylinders=0;
        // Read very-old-style drive geometry
        KdPrint2((PRINT_PREFIX "CHS %#x:%#x:%#x\n", 
                deviceExtension->FullIdentifyData.NumberOfCylinders,
                deviceExtension->FullIdentifyData.NumberOfHeads,
                deviceExtension->FullIdentifyData.SectorsPerTrack
                ));
        NumOfSectors = deviceExtension->FullIdentifyData.NumberOfCylinders *
                       deviceExtension->FullIdentifyData.NumberOfHeads *
                       deviceExtension->FullIdentifyData.SectorsPerTrack;
        KdPrint2((PRINT_PREFIX "NumOfSectors %#I64x\n", NumOfSectors));
        // Check for HDDs > 8Gb
        if ((deviceExtension->FullIdentifyData.NumberOfCylinders == 0x3fff) &&
/*            (deviceExtension->FullIdentifyData.TranslationFieldsValid) &&*/
            (NumOfSectors < deviceExtension->FullIdentifyData.UserAddressableSectors)) {
            KdPrint2((PRINT_PREFIX "NumberOfCylinders == 0x3fff\n"));
            cylinders = 
                (deviceExtension->FullIdentifyData.UserAddressableSectors /
                    (deviceExtension->FullIdentifyData.NumberOfHeads *
                       deviceExtension->FullIdentifyData.SectorsPerTrack));

            KdPrint2((PRINT_PREFIX "cylinders %#I64x\n", cylinders));

            NumOfSectors = cylinders *
                           deviceExtension->FullIdentifyData.NumberOfHeads *
                           deviceExtension->FullIdentifyData.SectorsPerTrack;

            KdPrint2((PRINT_PREFIX "NumOfSectors %#I64x\n", NumOfSectors));
        } else {

        }
        // Check for LBA mode
        KdPrint2((PRINT_PREFIX "SupportLba flag %#x\n", deviceExtension->FullIdentifyData.SupportLba));
        KdPrint2((PRINT_PREFIX "MajorRevision %#x\n", deviceExtension->FullIdentifyData.MajorRevision));
        KdPrint2((PRINT_PREFIX "UserAddressableSectors %#x\n", deviceExtension->FullIdentifyData.UserAddressableSectors));
        if ( deviceExtension->FullIdentifyData.SupportLba
                            ||
            (deviceExtension->FullIdentifyData.MajorRevision &&
/*             deviceExtension->FullIdentifyData.TranslationFieldsValid &&*/
             deviceExtension->FullIdentifyData.UserAddressableSectors)) {
            KdPrint2((PRINT_PREFIX "LBA mode\n"));
            LunExt->DeviceFlags |= DFLAGS_LBA_ENABLED;
        } else {
            KdPrint2((PRINT_PREFIX "Keep orig geometry\n"));
            LunExt->DeviceFlags |= DFLAGS_ORIG_GEOMETRY;
            goto skip_lba_staff;
        }
        // Check for LBA48 support
        if(LunExt->DeviceFlags & DFLAGS_LBA_ENABLED) {
            if(deviceExtension->FullIdentifyData.FeaturesSupport.Address48 &&
               deviceExtension->FullIdentifyData.FeaturesEnabled.Address48 &&
               (deviceExtension->FullIdentifyData.UserAddressableSectors48 > NumOfSectors)
               ) {
                KdPrint2((PRINT_PREFIX "LBA48\n"));
                cylinders = 
                    (deviceExtension->FullIdentifyData.UserAddressableSectors48 /
                        (deviceExtension->FullIdentifyData.NumberOfHeads *
                           deviceExtension->FullIdentifyData.SectorsPerTrack));

                KdPrint2((PRINT_PREFIX "cylinders %#I64x\n", cylinders));
                
                NativeNumOfSectors = cylinders *
                               deviceExtension->FullIdentifyData.NumberOfHeads *
                               deviceExtension->FullIdentifyData.SectorsPerTrack;

                KdPrint2((PRINT_PREFIX "NativeNumOfSectors %#I64x\n", NativeNumOfSectors));

                if(NativeNumOfSectors > NumOfSectors) {
                    KdPrint2((PRINT_PREFIX "Update NumOfSectors to %#I64x\n", NativeNumOfSectors));
                    NumOfSectors = NativeNumOfSectors;
                }
            }

            // Check drive capacity report for LBA48-capable drives.
            if(deviceExtension->FullIdentifyData.FeaturesSupport.Address48) {
                ULONG hNativeNumOfSectors;
                KdPrint2((PRINT_PREFIX "Use IDE_COMMAND_READ_NATIVE_SIZE48\n"));

                statusByte = AtaCommand48(deviceExtension, DeviceNumber, lChannel,
                             IDE_COMMAND_READ_NATIVE_SIZE48, 0, 0, 0, ATA_WAIT_READY);

                if(!(statusByte & IDE_STATUS_ERROR)) {
                    NativeNumOfSectors = (ULONG)AtapiReadPort1(chan, IDX_IO1_i_BlockNumber) |
                                        ((ULONG)AtapiReadPort1(chan, IDX_IO1_i_CylinderLow)  << 8) |
                                        ((ULONG)AtapiReadPort1(chan, IDX_IO1_i_CylinderHigh) << 16) ;

                    AtapiWritePort1(chan, IDX_IO2_o_Control,
                                           IDE_DC_USE_HOB );

                    KdPrint2((PRINT_PREFIX "Read high order bytes\n"));
                    NativeNumOfSectors |=
                                        ((ULONG)AtapiReadPort1(chan, IDX_IO1_i_BlockNumber)  << 24 );
                    hNativeNumOfSectors= 
                                         (ULONG)AtapiReadPort1(chan, IDX_IO1_i_CylinderLow) |
                                        ((ULONG)AtapiReadPort1(chan, IDX_IO1_i_CylinderHigh) << 8) ;
                    ((PULONG)&NativeNumOfSectors)[1] = hNativeNumOfSectors;

                    KdPrint2((PRINT_PREFIX "NativeNumOfSectors %#I64x\n", NativeNumOfSectors));

                    // Some drives report LBA48 capability while has capacity below 128Gb
                    // Probably they support large block-counters.
                    // But the problem is that some of them reports higher part of Max LBA equal to lower part.
                    // Here we check this
                    if((NativeNumOfSectors & 0xffffff) == ((NativeNumOfSectors >> 24) & 0xffffff)) {
                        KdPrint2((PRINT_PREFIX "High-order bytes == Low-order bytes !!!\n"));

                        statusByte = AtaCommand48(deviceExtension, DeviceNumber, lChannel,
                                     IDE_COMMAND_READ_NATIVE_SIZE48, 0, 0, 0, ATA_WAIT_READY);

                        if(!(statusByte & IDE_STATUS_ERROR)) {
                            NativeNumOfSectors = (ULONGLONG)AtapiReadPort1(chan, IDX_IO1_i_BlockNumber) |
                                                ((ULONGLONG)AtapiReadPort1(chan, IDX_IO1_i_BlockNumber)  << 24) |
                                                ((ULONGLONG)AtapiReadPort1(chan, IDX_IO1_i_CylinderLow)  << 8 ) |
                                                ((ULONGLONG)AtapiReadPort1(chan, IDX_IO1_i_CylinderLow)  << 32) |
                                                ((ULONGLONG)AtapiReadPort1(chan, IDX_IO1_i_CylinderHigh) << 16) |
                                                ((ULONGLONG)AtapiReadPort1(chan, IDX_IO1_i_CylinderHigh) << 40) 
                                                ;
                        }

                        if((NativeNumOfSectors & 0xffffff) == ((NativeNumOfSectors >> 24) & 0xffffff)) {
                            KdPrint2((PRINT_PREFIX "High-order bytes == Low-order bytes !!! (2)\n"));
                            NativeNumOfSectors = 0;
                        }
                    }

                    if(NumOfSectors <= ATA_MAX_LBA28 &&
                       NativeNumOfSectors > NumOfSectors) {

                        KdPrint2((PRINT_PREFIX "Use IDE_COMMAND_SET_NATIVE_SIZE48\n"));
                        KdPrint2((PRINT_PREFIX "Update NumOfSectors to %#I64x\n", NativeNumOfSectors));

                        statusByte = AtaCommand48(deviceExtension, DeviceNumber, lChannel,
                                     IDE_COMMAND_SET_NATIVE_SIZE, NativeNumOfSectors, 0, 0, ATA_WAIT_READY);
                        if(!(statusByte & IDE_STATUS_ERROR)) {
                            NumOfSectors = NativeNumOfSectors;
                        }
                    }
                }
            }
    
            if(NumOfSectors < 0x2100000 /*&& NumOfSectors > 31*1000*1000*/) {
                // check for native LBA size
                // some drives report ~32Gb in Identify Block
                KdPrint2((PRINT_PREFIX "Use IDE_COMMAND_READ_NATIVE_SIZE\n"));

                statusByte = AtaCommand(deviceExtension, DeviceNumber, lChannel, IDE_COMMAND_READ_NATIVE_SIZE,
                             0, IDE_USE_LBA, 0, 0, 0, ATA_WAIT_READY);

                if(!(statusByte & IDE_STATUS_ERROR)) {
                    NativeNumOfSectors = (ULONG)AtapiReadPort1(chan, IDX_IO1_i_BlockNumber) |
                                        ((ULONG)AtapiReadPort1(chan, IDX_IO1_i_CylinderLow) << 8) |
                                        ((ULONG)AtapiReadPort1(chan, IDX_IO1_i_CylinderHigh) << 16) |
                                       (((ULONG)AtapiReadPort1(chan, IDX_IO1_i_DriveSelect) & 0xf) << 24);

                    KdPrint2((PRINT_PREFIX "NativeNumOfSectors %#I64x\n", NativeNumOfSectors));

                    if(NativeNumOfSectors > NumOfSectors) {

                        KdPrint2((PRINT_PREFIX "Use IDE_COMMAND_SET_NATIVE_SIZE\n"));
                        KdPrint2((PRINT_PREFIX "Update NumOfSectors to %#I64x\n", NativeNumOfSectors));

                        statusByte = AtaCommand48(deviceExtension, DeviceNumber, lChannel,
                                     IDE_COMMAND_SET_NATIVE_SIZE, NativeNumOfSectors, 0, 0, ATA_WAIT_READY);
                        if(!(statusByte & IDE_STATUS_ERROR)) {
                            NumOfSectors = NativeNumOfSectors;
                        }
                    }
                }
            }

        } // if(LunExt->DeviceFlags & DFLAGS_LBA_ENABLED)

        // fill IdentifyData with bogus geometry
        KdPrint2((PRINT_PREFIX "requested LunExt->GeomType=%x\n", LunExt->opt_GeomType));
        tmp_cylinders = NumOfSectors / (deviceExtension->FullIdentifyData.CurrentSectorsPerTrack *
                                        deviceExtension->FullIdentifyData.NumberOfCurrentHeads);
        KdPrint2((PRINT_PREFIX "tmp_cylinders = %#I64x\n", tmp_cylinders));
        if((tmp_cylinders < 0xffff) || (LunExt->opt_GeomType == GEOM_ORIG)) {
            // ok, we can keep original values
            if(LunExt->opt_GeomType == GEOM_AUTO) {
                LunExt->opt_GeomType = GEOM_ORIG;
            }
        } else {
            tmp_cylinders = NumOfSectors / (255*63);
            if(tmp_cylinders < 0xffff) {
                // we can use generic values for H/S for generic geometry approach
                if(LunExt->opt_GeomType == GEOM_AUTO) {
                    LunExt->opt_GeomType = GEOM_STD;
                }
            } else {
                // we should use UNIATA geometry approach
                if(LunExt->opt_GeomType == GEOM_AUTO) {
                    LunExt->opt_GeomType = GEOM_UNIATA;
                }
            }
        }
        KdPrint2((PRINT_PREFIX "final LunExt->opt_GeomType=%x\n", LunExt->opt_GeomType));

        if(LunExt->opt_GeomType == GEOM_STD) {
            deviceExtension->FullIdentifyData.CurrentSectorsPerTrack =
            deviceExtension->FullIdentifyData.SectorsPerTrack = 63;

            deviceExtension->FullIdentifyData.NumberOfCurrentHeads =
            deviceExtension->FullIdentifyData.NumberOfHeads   = 255;

            cylinders = NumOfSectors / (255*63);
            KdPrint2((PRINT_PREFIX "Use GEOM_STD, CHS=%I64x/%x/%x\n", cylinders, 255, 63));
        } else
        if(LunExt->opt_GeomType == GEOM_UNIATA) {
            while ((cylinders > 0xffff) && (deviceExtension->FullIdentifyData.SectorsPerTrack < 0x80)) {
                cylinders /= 2;
                KdPrint2((PRINT_PREFIX "cylinders /= 2\n"));
                deviceExtension->FullIdentifyData.SectorsPerTrack *= 2;
                deviceExtension->FullIdentifyData.CurrentSectorsPerTrack *= 2;
            }
            while ((cylinders > 0xffff) && (deviceExtension->FullIdentifyData.NumberOfHeads < 0x80)) {
                cylinders /= 2;
                KdPrint2((PRINT_PREFIX "cylinders /= 2 (2)\n"));
                deviceExtension->FullIdentifyData.NumberOfHeads *= 2;
                deviceExtension->FullIdentifyData.NumberOfCurrentHeads *= 2;
            }
            while ((cylinders > 0xffff) && (deviceExtension->FullIdentifyData.SectorsPerTrack < 0x8000)) {
                cylinders /= 2;
                KdPrint2((PRINT_PREFIX "cylinders /= 2 (3)\n"));
                deviceExtension->FullIdentifyData.SectorsPerTrack *= 2;
                deviceExtension->FullIdentifyData.CurrentSectorsPerTrack *= 2;
            }
            while ((cylinders > 0xffff) && (deviceExtension->FullIdentifyData.NumberOfHeads < 0x8000)) {
                cylinders /= 2;
                KdPrint2((PRINT_PREFIX "cylinders /= 2 (4)\n"));
                deviceExtension->FullIdentifyData.NumberOfHeads *= 2;
                deviceExtension->FullIdentifyData.NumberOfCurrentHeads *= 2;
            }
            KdPrint2((PRINT_PREFIX "Use GEOM_UNIATA, CHS=%I64x/%x/%x\n", cylinders,
                deviceExtension->FullIdentifyData.NumberOfCurrentHeads,
                deviceExtension->FullIdentifyData.CurrentSectorsPerTrack));
        }
        if(!cylinders) {
            KdPrint2((PRINT_PREFIX "cylinders = tmp_cylinders (%x = %x)\n", cylinders, tmp_cylinders));
            cylinders = tmp_cylinders;
        }
        deviceExtension->FullIdentifyData.NumberOfCurrentCylinders =
        deviceExtension->FullIdentifyData.NumberOfCylinders = (USHORT)cylinders;

skip_lba_staff:

        KdPrint2((PRINT_PREFIX "Geometry: C %#x (%#x)\n",
                  deviceExtension->FullIdentifyData.NumberOfCylinders,
                  deviceExtension->FullIdentifyData.NumberOfCurrentCylinders
                  ));
        KdPrint2((PRINT_PREFIX "Geometry: H %#x (%#x)\n",
                  deviceExtension->FullIdentifyData.NumberOfHeads,
                  deviceExtension->FullIdentifyData.NumberOfCurrentHeads
                  ));
        KdPrint2((PRINT_PREFIX "Geometry: S %#x (%#x)\n",
                  deviceExtension->FullIdentifyData.SectorsPerTrack,
                  deviceExtension->FullIdentifyData.CurrentSectorsPerTrack
                  ));

        if(NumOfSectors)
            LunExt->NumOfSectors = NumOfSectors;
/*        if(deviceExtension->FullIdentifyData.MajorRevision &&
           deviceExtension->FullIdentifyData.DoubleWordIo) {
            LunExt->DeviceFlags |= DFLAGS_DWORDIO_ENABLED;
        }*/
    }

    ScsiPortMoveMemory(&LunExt->IdentifyData,
                       &deviceExtension->FullIdentifyData,sizeof(IDENTIFY_DATA2));

    InitBadBlocks(LunExt);

    if ((LunExt->IdentifyData.DrqType & ATAPI_DRQT_INTR) &&
        (Command != IDE_COMMAND_IDENTIFY)) {

        // This device interrupts with the assertion of DRQ after receiving
        // Atapi Packet Command
        LunExt->DeviceFlags |= DFLAGS_INT_DRQ;
        KdPrint2((PRINT_PREFIX "IssueIdentify: Device interrupts on assertion of DRQ.\n"));

    } else {
        KdPrint2((PRINT_PREFIX "IssueIdentify: Device does not interrupt on assertion of DRQ.\n"));
    }

    if(Command != IDE_COMMAND_IDENTIFY) {
        // ATAPI branch
        if(LunExt->IdentifyData.DeviceType == ATAPI_TYPE_TAPE) {
            // This is a tape.
            LunExt->DeviceFlags |= DFLAGS_TAPE_DEVICE;
            KdPrint2((PRINT_PREFIX "IssueIdentify: Device is a tape drive.\n"));
        } else
        if(LunExt->IdentifyData.DeviceType == ATAPI_TYPE_CDROM ||
            LunExt->IdentifyData.DeviceType == ATAPI_TYPE_OPTICAL) {
            KdPrint2((PRINT_PREFIX "IssueIdentify: Device is CD/Optical drive.\n"));
            // set CD default costs
            LunExt->RwSwitchCost  = REORDER_COST_SWITCH_RW_CD;
            LunExt->RwSwitchMCost = REORDER_MCOST_SWITCH_RW_CD;
            LunExt->SeekBackMCost = REORDER_MCOST_SEEK_BACK_CD;
            statusByte = WaitForDrq(chan);
        } else {
            KdPrint2((PRINT_PREFIX "IssueIdentify: ATAPI drive type %#x.\n",
                LunExt->IdentifyData.DeviceType));
        }
    } else {
        KdPrint2((PRINT_PREFIX "IssueIdentify: hard drive.\n"));
    }

    GetBaseStatus(chan, statusByte);
    KdPrint2((PRINT_PREFIX "IssueIdentify: final Status on exit (%#x)\n", statusByte));
    return TRUE;

} // end IssueIdentify()


/*++

Routine Description:
    Set drive parameters using the IDENTIFY data.

Arguments:
    HwDeviceExtension - HBA miniport driver's adapter data storage
    DeviceNumber - Indicates which device.

Return Value:
    TRUE if all goes well.

--*/
BOOLEAN
SetDriveParameters(
    IN PVOID HwDeviceExtension,
    IN ULONG DeviceNumber,
    IN ULONG lChannel
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    PIDENTIFY_DATA2      identifyData   = &deviceExtension->lun[(lChannel * 2) + DeviceNumber].IdentifyData;
//    ULONG i;
    UCHAR statusByte;
    UCHAR errorByte;

    if(deviceExtension->lun[(lChannel * 2) + DeviceNumber].DeviceFlags &
          (DFLAGS_LBA_ENABLED | DFLAGS_ORIG_GEOMETRY))
       return TRUE;

    KdPrint2((PRINT_PREFIX "SetDriveParameters: Number of heads %#x\n", identifyData->NumberOfHeads));
    KdPrint2((PRINT_PREFIX "SetDriveParameters: Sectors per track %#x\n", identifyData->SectorsPerTrack));

    // Send SET PARAMETER command.
    statusByte = AtaCommand(deviceExtension, DeviceNumber, lChannel,
                            IDE_COMMAND_SET_DRIVE_PARAMETERS, 0,
                            (identifyData->NumberOfHeads - 1), 0,
                            (UCHAR)identifyData->SectorsPerTrack, 0, ATA_WAIT_IDLE);

    statusByte = UniataIsIdle(deviceExtension, statusByte);
    if(statusByte & IDE_STATUS_ERROR) {
        errorByte = AtapiReadPort1(&deviceExtension->chan[lChannel], IDX_IO1_i_Error);
        KdPrint2((PRINT_PREFIX "SetDriveParameters: Error bit set. Status %#x, error %#x\n",
                    errorByte, statusByte));
        return FALSE;
    }

    if(statusByte == IDE_STATUS_IDLE) {
        return TRUE;
    }

    return FALSE;

} // end SetDriveParameters()


/*++

Routine Description:
    Reset IDE controller and/or Atapi device.

Arguments:
    HwDeviceExtension - HBA miniport driver's adapter data storage

Return Value:
    Nothing.


--*/
BOOLEAN
DDKAPI
AtapiResetController(
    IN PVOID HwDeviceExtension,
    IN ULONG PathId
    )
{
    KdPrint2((PRINT_PREFIX "AtapiResetController()\n"));
    return AtapiResetController__(HwDeviceExtension, PathId, RESET_COMPLETE_ALL);
} // end AtapiResetController()


BOOLEAN
AtapiResetController__(
    IN PVOID HwDeviceExtension,
    IN ULONG PathId,
    IN BOOLEAN CompleteType
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    ULONG                numberChannels  = deviceExtension->NumberChannels;
    PHW_CHANNEL          chan = NULL;
    ULONG i,j;
    ULONG max_ldev;
    UCHAR statusByte;
    PSCSI_REQUEST_BLOCK CurSrb;
    ULONG ChannelCtrlFlags;
    UCHAR dma_status = 0;

    ULONG slotNumber = deviceExtension->slotNumber;
    ULONG SystemIoBusNumber = deviceExtension->SystemIoBusNumber;
    ULONG VendorID =  deviceExtension->DevID        & 0xffff;
    ULONG DeviceID = (deviceExtension->DevID >> 16) & 0xffff;
    //ULONG RevID    =  deviceExtension->RevID;
    ULONG ChipFlags = deviceExtension->HwFlags & CHIPFLAG_MASK;
    UCHAR tmp8;
    UCHAR tmp16;

    KdPrint2((PRINT_PREFIX "AtapiResetController: Reset IDE %#x/%#x @ %#x\n", VendorID, DeviceID, slotNumber));

    if(!deviceExtension->simplexOnly) {
        // we shall reset both channels on SimplexOnly devices,
        // It's not worth doing so on normal controllers
        j = PathId;
        numberChannels = j+1;
    } else {
        j=0;
        numberChannels = deviceExtension->NumberChannels;
    }

    for (; j < numberChannels; j++) {

        KdPrint2((PRINT_PREFIX "AtapiResetController: Reset channel %d\n", j));
        chan = &deviceExtension->chan[j];
        KdPrint2((PRINT_PREFIX "  CompleteType %#x\n", CompleteType));
        max_ldev = (chan->ChannelCtrlFlags & CTRFLAGS_NO_SLAVE) ? 1 : 2;
        if(CompleteType != RESET_COMPLETE_NONE) {
#ifndef UNIATA_CORE
            while((CurSrb = UniataGetCurRequest(chan))) {

                PATA_REQ AtaReq = (PATA_REQ)(CurSrb->SrbExtension);

                KdPrint2((PRINT_PREFIX "AtapiResetController: pending SRB %#x\n", CurSrb));
                // Check and see if we are processing an internal srb
                if (AtaReq->OriginalSrb) {
                    KdPrint2((PRINT_PREFIX "  restore original SRB %#x\n", AtaReq->OriginalSrb));
                    AtaReq->Srb = AtaReq->OriginalSrb;
                    AtaReq->OriginalSrb = NULL;
                    // NOTE: internal SRB doesn't get to SRB queue !!!
                    CurSrb = AtaReq->Srb;
                }

                // Remove current request from queue
                UniataRemoveRequest(chan, CurSrb);

                // Check if request is in progress.
                ASSERT(AtaReq->Srb == CurSrb);
                if (CurSrb) {
                    // Complete outstanding request with SRB_STATUS_BUS_RESET.
                    UCHAR PathId   = CurSrb->PathId;
                    UCHAR TargetId = CurSrb->TargetId;
                    UCHAR Lun = CurSrb->Lun;

                    CurSrb->SrbStatus = ((CompleteType == RESET_COMPLETE_ALL) ? SRB_STATUS_BUS_RESET : SRB_STATUS_ABORTED) | SRB_STATUS_AUTOSENSE_VALID;
                    CurSrb->ScsiStatus = SCSISTAT_CHECK_CONDITION;

                    if (CurSrb->SenseInfoBuffer) {

                        PSENSE_DATA  senseBuffer = (PSENSE_DATA)CurSrb->SenseInfoBuffer;

                        senseBuffer->ErrorCode = 0x70;
                        senseBuffer->Valid     = 1;
                        senseBuffer->AdditionalSenseLength = 0xb;
                        if(CompleteType == RESET_COMPLETE_ALL) {
                            KdPrint2((PRINT_PREFIX "AtapiResetController: report SCSI_SENSE_UNIT_ATTENTION + SCSI_ADSENSE_BUS_RESET\n"));
                            senseBuffer->SenseKey = SCSI_SENSE_UNIT_ATTENTION;
                            senseBuffer->AdditionalSenseCode = SCSI_ADSENSE_BUS_RESET;
                            senseBuffer->AdditionalSenseCodeQualifier = SCSI_SENSEQ_SCSI_BUS;
                        } else {
                            KdPrint2((PRINT_PREFIX "AtapiResetController: report SCSI_SENSE_ABORTED_COMMAND\n"));
                            senseBuffer->SenseKey = SCSI_SENSE_ABORTED_COMMAND;
                            senseBuffer->AdditionalSenseCode = 0;
                            senseBuffer->AdditionalSenseCodeQualifier = 0;
                        }
                    }

                    // Clear request tracking fields.
                    AtaReq->WordsLeft = 0;
                    AtaReq->DataBuffer = NULL;

                    ScsiPortNotification(RequestComplete,
                                         deviceExtension,
                                         CurSrb);

                    // Indicate ready for next request.
                    ScsiPortNotification(NextLuRequest,
                                         deviceExtension, 
                                         PathId,
                                         TargetId,
                                         Lun);
                }
                if(CompleteType != RESET_COMPLETE_ALL)
                    break;
            } // end while()
#endif //UNIATA_CORE
        } // end if (!CompleteType != RESET_COMPLETE_NONE)

        // Save control flags
        ChannelCtrlFlags = chan->ChannelCtrlFlags;
        // Clear expecting interrupt flag.
        chan->ExpectingInterrupt = FALSE;
        chan->RDP = FALSE;
        chan->ChannelCtrlFlags = 0;
        InterlockedExchange(&(chan->CheckIntr),
                                      CHECK_INTR_IDLE);
        
        // Reset controller
        KdPrint2((PRINT_PREFIX "  disable intr (0)\n"));
        AtapiDisableInterrupts(deviceExtension, j);
        KdPrint2((PRINT_PREFIX "  done\n"));
        switch(VendorID) {
        case ATA_INTEL_ID: {
            ULONG mask;
            ULONG timeout;
            if(!(ChipFlags & UNIATA_SATA))
                goto default_reset;
            if(!deviceExtension->BaseIoAddressSATA_0.Addr) {
                goto default_reset;
            }

            /* ICH6 & ICH7 in compat mode has 4 SATA ports as master/slave on 2 ch's */
            if(ChipFlags & UNIATA_AHCI) {
                mask = 0x0005 << j;
            } else {
                /* ICH5 in compat mode has SATA ports as master/slave on 1 channel */
                GetPciConfig1(0x90, tmp8);
                if(tmp8 & 0x04) {
                    mask = 0x0003;
                } else {
                    mask = 0x0001 << j;
                }
            }
            ChangePciConfig2(0x92, a & ~mask);
            AtapiStallExecution(10);
            ChangePciConfig2(0x92, a | mask);
            timeout = 100;
            while (timeout--) {
                AtapiStallExecution(10000);
                GetPciConfig2(0x92, tmp16);
                if ((tmp16 & (mask << 4)) == (mask << 4)) {
                    AtapiStallExecution(10000);
                    break;
                }
            }
            break; }
        case ATA_SIS_ID:
        case ATA_NVIDIA_ID: {
            KdPrint2((PRINT_PREFIX "  SIS/nVidia\n"));
            if(!(ChipFlags & UNIATA_SATA))
                goto default_reset;
            break; }
        case ATA_SILICON_IMAGE_ID: {
            ULONG offset;
            ULONG Channel = deviceExtension->Channel + j;
            if(!(ChipFlags & UNIATA_SATA))
                goto default_reset;
            offset = ((Channel & 1) << 7) + ((Channel & 2) << 8);
            /* disable PHY state change interrupt */
            AtapiWritePortEx4(NULL, (ULONG)(&deviceExtension->BaseIoAddressSATA_0), 0x148 + offset, 0);

            UniataSataClearErr(HwDeviceExtension, j, UNIATA_SATA_IGNORE_CONNECT);

            /* reset controller part for this channel */
            AtapiWritePortEx4(NULL, (ULONG)(&deviceExtension->BaseIoAddressSATA_0), 0x48,
                 AtapiReadPortEx4(NULL, (ULONG)(&deviceExtension->BaseIoAddressSATA_0), 0x48) | (0xc0 >> Channel));
            AtapiStallExecution(1000);
            AtapiWritePortEx4(NULL, (ULONG)(&deviceExtension->BaseIoAddressSATA_0), 0x48,
                 AtapiReadPortEx4(NULL, (ULONG)(&deviceExtension->BaseIoAddressSATA_0), 0x48) & ~(0xc0 >> Channel));


            break; }
        case ATA_PROMISE_ID: {
            break; }
        default:
            if(ChipFlags & UNIATA_SATA) {
                KdPrint2((PRINT_PREFIX "  SATA generic reset\n"));
                UniataSataClearErr(HwDeviceExtension, j, UNIATA_SATA_IGNORE_CONNECT);
            }
default_reset:
            KdPrint2((PRINT_PREFIX "  send reset\n"));
            AtapiWritePort1(chan, IDX_IO2_o_Control, IDE_DC_DISABLE_INTERRUPTS |
                                                                    IDE_DC_RESET_CONTROLLER );
            KdPrint2((PRINT_PREFIX "  wait a little\n"));
            AtapiStallExecution(10000);
            // Disable interrupts
            KdPrint2((PRINT_PREFIX "  disable intr\n"));
            AtapiDisableInterrupts(deviceExtension, j);
            KdPrint2((PRINT_PREFIX "  re-enable intr\n"));
            AtapiEnableInterrupts(deviceExtension, j);
            KdPrint2((PRINT_PREFIX "  wait a little (2)\n"));
            AtapiStallExecution(100000);
            KdPrint2((PRINT_PREFIX "  done\n"));

            break;
        }

        //if(!(ChipFlags & UNIATA_SATA)) {
        if(!deviceExtension->BaseIoAddressSATA_0.Addr) {
            // Reset DMA engine if active
            KdPrint2((PRINT_PREFIX "  check DMA engine\n"));
            dma_status = GetDmaStatus(chan->DeviceExtension, chan->lChannel);
            KdPrint2((PRINT_PREFIX "  DMA status %#x\n", dma_status));
            if((ChannelCtrlFlags & CTRFLAGS_DMA_ACTIVE) ||
               (dma_status & BM_STATUS_INTR)) {
                AtapiDmaDone(HwDeviceExtension, 0, j, NULL);
            }
        }

        // all these shall be performed inside AtapiHwInitialize__() ?
#if 1
        KdPrint2((PRINT_PREFIX "  process connected devices\n"));
        // Do special processing for ATAPI and IDE disk devices.
        for (i = 0; i < max_ldev; i++) {

            // Check if device present.
            if (!(deviceExtension->lun[i + (j * 2)].DeviceFlags & DFLAGS_DEVICE_PRESENT)) {
                //if(!CheckDevice(HwDeviceExtension, i, j, FALSE))
                if(!UniataAnybodyHome(HwDeviceExtension, j, i)) {
                    continue;
                }
                if(!CheckDevice(HwDeviceExtension, j, i, TRUE)) {
                    continue;
                }
            } else {
                if(!UniataAnybodyHome(HwDeviceExtension, j, i)) {
                    KdPrint2((PRINT_PREFIX "  device have gone\n"));
                    deviceExtension->lun[i + (j * 2)].DeviceFlags &= ~DFLAGS_DEVICE_PRESENT;
                }
            }

            SelectDrive(chan, i);
            AtapiStallExecution(10);
            statusByte = WaitOnBusyLong(chan);
            statusByte = UniataIsIdle(deviceExtension, statusByte);
            if(statusByte == 0xff) {
                KdPrint2((PRINT_PREFIX 
                           "no drive, status %#x\n",
                           statusByte));
                deviceExtension->lun[i + (j * 2)].DeviceFlags = 0;
            } else
            // Check for ATAPI disk.
            if (deviceExtension->lun[i + (j * 2)].DeviceFlags & DFLAGS_ATAPI_DEVICE) {
                // Issue soft reset and issue identify.
                GetStatus(chan, statusByte);
                KdPrint2((PRINT_PREFIX "AtapiResetController: Status before Atapi reset (%#x).\n",
                            statusByte));

                AtapiDisableInterrupts(deviceExtension, j);
                AtapiSoftReset(chan, i);
                AtapiEnableInterrupts(deviceExtension, j);

                GetStatus(chan, statusByte);

                if(statusByte == IDE_STATUS_SUCCESS) {

                    IssueIdentify(HwDeviceExtension,
                                  i, j,
                                  IDE_COMMAND_ATAPI_IDENTIFY, FALSE);
                } else {

                    KdPrint2((PRINT_PREFIX 
                               "AtapiResetController: Status after soft reset %#x\n",
                               statusByte));
                }
                GetBaseStatus(chan, statusByte);

            } else {
                // Issue identify and reinit after channel reset.

                if (statusByte != IDE_STATUS_IDLE &&
                    statusByte != IDE_STATUS_SUCCESS &&
                    statusByte != IDE_STATUS_DRDY) {
//                    result2 = FALSE;
                    KdPrint2((PRINT_PREFIX "AtapiResetController: IdeHardReset failed\n"));
                } else
                if(!IssueIdentify(HwDeviceExtension,
                                  i, j,
                                  IDE_COMMAND_IDENTIFY, FALSE)) {
//                    result2 = FALSE;
                    KdPrint2((PRINT_PREFIX "AtapiResetController: IDE IssueIdentify failed\n"));
                } else
                // Set disk geometry parameters.
                if (!SetDriveParameters(HwDeviceExtension, i, j)) {
                    KdPrint2((PRINT_PREFIX "AtapiResetController: SetDriveParameters failed\n"));
                }
                GetBaseStatus(chan, statusByte);
            }
            // force DMA mode reinit
            deviceExtension->lun[i + (j * 2)].DeviceFlags |= DFLAGS_REINIT_DMA;
        }
#endif //0

        // Enable interrupts, note, the we can have here recursive disable
        AtapiStallExecution(10);
        KdPrint2((PRINT_PREFIX "AtapiResetController: deviceExtension->chan[%d].DisableIntr %d -> 1\n",
            j,
            chan->DisableIntr));
        AtapiEnableInterrupts(deviceExtension, j);

        // Call the HwInitialize routine to setup multi-block.
        AtapiHwInitialize__(deviceExtension, j);
    }
    ScsiPortNotification(NextRequest, deviceExtension, NULL);

    return TRUE;

} // end AtapiResetController__()


/*++

Routine Description:
    This routine maps ATAPI and IDE errors to specific SRB statuses.

Arguments:
    HwDeviceExtension - HBA miniport driver's adapter data storage
    Srb - IO request packet

Return Value:
    SRB status

--*/
ULONG
MapError(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    ULONG lChannel = GET_CHANNEL(Srb);
    PHW_CHANNEL chan = &(deviceExtension->chan[lChannel]);
//    ULONG i;
    UCHAR errorByte;
    UCHAR srbStatus = SRB_STATUS_SUCCESS;
    UCHAR scsiStatus;
    ULONG ldev = GET_LDEV(Srb);

    // Read the error register.

    errorByte = AtapiReadPort1(chan, IDX_IO1_i_Error);
    KdPrint2((PRINT_PREFIX 
               "MapError: Error register is %#x\n",
               errorByte));

    if (deviceExtension->lun[ldev].DeviceFlags & DFLAGS_ATAPI_DEVICE) {

        switch (errorByte >> 4) {
        case SCSI_SENSE_NO_SENSE:

            KdPrint2((PRINT_PREFIX 
                       "ATAPI: No sense information\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;
            break;

        case SCSI_SENSE_RECOVERED_ERROR:

            KdPrint2((PRINT_PREFIX 
                       "ATAPI: Recovered error\n"));
            scsiStatus = 0;
            srbStatus = SRB_STATUS_SUCCESS;
            break;

        case SCSI_SENSE_NOT_READY:

            KdPrint2((PRINT_PREFIX 
                       "ATAPI: Device not ready\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;
            break;

        case SCSI_SENSE_MEDIUM_ERROR:

            KdPrint2((PRINT_PREFIX 
                       "ATAPI: Media error\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;
            break;

        case SCSI_SENSE_HARDWARE_ERROR:

            KdPrint2((PRINT_PREFIX 
                       "ATAPI: Hardware error\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;
            break;

        case SCSI_SENSE_ILLEGAL_REQUEST:

            KdPrint2((PRINT_PREFIX 
                       "ATAPI: Illegal request\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;
            break;

        case SCSI_SENSE_UNIT_ATTENTION:

            KdPrint2((PRINT_PREFIX 
                       "ATAPI: Unit attention\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;
            break;

        case SCSI_SENSE_DATA_PROTECT:

            KdPrint2((PRINT_PREFIX 
                       "ATAPI: Data protect\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;
            break;

        case SCSI_SENSE_BLANK_CHECK:

            KdPrint2((PRINT_PREFIX 
                       "ATAPI: Blank check\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;
            break;

        case SCSI_SENSE_ABORTED_COMMAND:
            KdPrint2((PRINT_PREFIX 
                        "Atapi: Command Aborted\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;
            break;

        default:

            KdPrint2((PRINT_PREFIX 
                       "ATAPI: Invalid sense information\n"));
            scsiStatus = 0;
            srbStatus = SRB_STATUS_ERROR;
            break;
        }

    } else {

        scsiStatus = 0;

        // Save errorByte,to be used by SCSIOP_REQUEST_SENSE.
        chan->ReturningMediaStatus = errorByte;

        if (errorByte & IDE_ERROR_MEDIA_CHANGE_REQ) {
            KdPrint2((PRINT_PREFIX 
                       "IDE: Media change\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;

            if (Srb->SenseInfoBuffer) {

                PSENSE_DATA  senseBuffer = (PSENSE_DATA)Srb->SenseInfoBuffer;

                senseBuffer->ErrorCode = 0x70;
                senseBuffer->Valid     = 1;
                senseBuffer->AdditionalSenseLength = 0xb;
                senseBuffer->SenseKey =  SCSI_SENSE_UNIT_ATTENTION;
                senseBuffer->AdditionalSenseCode = SCSI_ADSENSE_MEDIUM_CHANGED;
                senseBuffer->AdditionalSenseCodeQualifier = 0;

                srbStatus |= SRB_STATUS_AUTOSENSE_VALID;
            }

        } else if (errorByte & IDE_ERROR_COMMAND_ABORTED) {
            KdPrint2((PRINT_PREFIX 
                       "IDE: Command abort\n"));
            srbStatus = SRB_STATUS_ABORTED;
            scsiStatus = SCSISTAT_CHECK_CONDITION;

            if (Srb->SenseInfoBuffer) {

                PSENSE_DATA  senseBuffer = (PSENSE_DATA)Srb->SenseInfoBuffer;

                senseBuffer->ErrorCode = 0x70;
                senseBuffer->Valid     = 1;
                senseBuffer->AdditionalSenseLength = 0xb;
                senseBuffer->SenseKey =  SCSI_SENSE_ABORTED_COMMAND;
                senseBuffer->AdditionalSenseCode = 0;
                senseBuffer->AdditionalSenseCodeQualifier = 0;

                srbStatus |= SRB_STATUS_AUTOSENSE_VALID;
            }

            deviceExtension->lun[ldev].ErrorCount++;

        } else if (errorByte & IDE_ERROR_END_OF_MEDIA) {

            KdPrint2((PRINT_PREFIX 
                       "IDE: End of media\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;

            if (Srb->SenseInfoBuffer) {

                PSENSE_DATA  senseBuffer = (PSENSE_DATA)Srb->SenseInfoBuffer;

                senseBuffer->ErrorCode = 0x70;
                senseBuffer->Valid     = 1;
                senseBuffer->AdditionalSenseLength = 0xb;
                senseBuffer->SenseKey =  SCSI_SENSE_UNIT_ATTENTION;
                senseBuffer->AdditionalSenseCode = SCSI_ADSENSE_MEDIA_STATE;
                senseBuffer->AdditionalSenseCodeQualifier = SCSI_SENSEQ_END_OF_MEDIUM;
                senseBuffer->EndOfMedia = 1;

                srbStatus |= SRB_STATUS_AUTOSENSE_VALID;
            }

            if (!(deviceExtension->lun[ldev].DeviceFlags & DFLAGS_MEDIA_STATUS_ENABLED)){
                deviceExtension->lun[ldev].ErrorCount++;
            }

        } else if (errorByte & IDE_ERROR_ILLEGAL_LENGTH) {

            KdPrint2((PRINT_PREFIX 
                       "IDE: Illegal length\n"));
            srbStatus = SRB_STATUS_INVALID_REQUEST;

            if (Srb->SenseInfoBuffer) {

                PSENSE_DATA  senseBuffer = (PSENSE_DATA)Srb->SenseInfoBuffer;

                senseBuffer->ErrorCode = 0x70;
                senseBuffer->Valid     = 1;
                senseBuffer->AdditionalSenseLength = 0xb;
                senseBuffer->SenseKey =  SCSI_SENSE_ILLEGAL_REQUEST;
                senseBuffer->AdditionalSenseCode = SCSI_ADSENSE_INVALID_VALUE;
                senseBuffer->AdditionalSenseCodeQualifier = SCSI_SENSEQ_PARAM_INVALID_VALUE;
                senseBuffer->IncorrectLength = 1;

                srbStatus |= SRB_STATUS_AUTOSENSE_VALID;
            }

        } else if (errorByte & IDE_ERROR_BAD_BLOCK) {

            KdPrint2((PRINT_PREFIX 
                       "IDE: Bad block\n"));
            srbStatus = SRB_STATUS_ERROR;
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            if (Srb->SenseInfoBuffer) {

                PSENSE_DATA  senseBuffer = (PSENSE_DATA)Srb->SenseInfoBuffer;

                senseBuffer->ErrorCode = 0x70;
                senseBuffer->Valid     = 1;
                senseBuffer->AdditionalSenseLength = 0xb;
                senseBuffer->SenseKey =  SCSI_SENSE_MEDIUM_ERROR;
                senseBuffer->AdditionalSenseCode = 0;
                senseBuffer->AdditionalSenseCodeQualifier = 0;

                srbStatus |= SRB_STATUS_AUTOSENSE_VALID;
            }

        } else if (errorByte & IDE_ERROR_ID_NOT_FOUND) {

            KdPrint2((PRINT_PREFIX 
                       "IDE: Id not found\n"));
            srbStatus = SRB_STATUS_ERROR;
            scsiStatus = SCSISTAT_CHECK_CONDITION;

            if (Srb->SenseInfoBuffer) {

                PSENSE_DATA  senseBuffer = (PSENSE_DATA)Srb->SenseInfoBuffer;

                senseBuffer->ErrorCode = 0x70;
                senseBuffer->Valid     = 1;
                senseBuffer->AdditionalSenseLength = 0xb;
                senseBuffer->SenseKey =  SCSI_SENSE_MEDIUM_ERROR;
                senseBuffer->AdditionalSenseCode = 0;
                senseBuffer->AdditionalSenseCodeQualifier = 0;

                srbStatus |= SRB_STATUS_AUTOSENSE_VALID;
            }

            deviceExtension->lun[ldev].ErrorCount++;

        } else if (errorByte & IDE_ERROR_MEDIA_CHANGE) {

            KdPrint2((PRINT_PREFIX 
                       "IDE: Media change\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;

            if (Srb->SenseInfoBuffer) {

                PSENSE_DATA  senseBuffer = (PSENSE_DATA)Srb->SenseInfoBuffer;

                senseBuffer->ErrorCode = 0x70;
                senseBuffer->Valid     = 1;
                senseBuffer->AdditionalSenseLength = 0xb;
                senseBuffer->SenseKey =  SCSI_SENSE_UNIT_ATTENTION;
                senseBuffer->AdditionalSenseCode = SCSI_ADSENSE_MEDIUM_CHANGED;
                senseBuffer->AdditionalSenseCodeQualifier = 0;

                srbStatus |= SRB_STATUS_AUTOSENSE_VALID;
            }

        } else if (errorByte & IDE_ERROR_DATA_ERROR) {

            KdPrint2((PRINT_PREFIX 
                   "IDE: Data error\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;

            if (!(deviceExtension->lun[ldev].DeviceFlags & DFLAGS_MEDIA_STATUS_ENABLED)){
                deviceExtension->lun[ldev].ErrorCount++;
            }

            // Build sense buffer
            if (Srb->SenseInfoBuffer) {

                PSENSE_DATA  senseBuffer = (PSENSE_DATA)Srb->SenseInfoBuffer;

                senseBuffer->ErrorCode = 0x70;
                senseBuffer->Valid     = 1;
                senseBuffer->AdditionalSenseLength = 0xb;
                senseBuffer->SenseKey =  SCSI_SENSE_MEDIUM_ERROR;
                senseBuffer->AdditionalSenseCode = 0;
                senseBuffer->AdditionalSenseCodeQualifier = 0;

                srbStatus |= SRB_STATUS_AUTOSENSE_VALID;
            }
        }

        if (deviceExtension->lun[ldev].ErrorCount >= MAX_ERRORS) {
//            deviceExtension->DWordIO = FALSE;

            KdPrint2((PRINT_PREFIX 
                        "MapError: ErrorCount >= MAX_ERRORS\n"));

            deviceExtension->lun[ldev].DeviceFlags &= ~DFLAGS_DWORDIO_ENABLED;
            deviceExtension->lun[ldev].MaximumBlockXfer = 0;
            BrutePoint();

            KdPrint2((PRINT_PREFIX 
                        "MapError: Disabling 32-bit PIO and Multi-sector IOs\n"));

            // Log the error.
            KdPrint2((PRINT_PREFIX 
                        "ScsiPortLogError: devExt %#x, Srb %#x, P:T:D=%d:%d:%d, MsgId %#x (%d)\n",
                              HwDeviceExtension,
                              Srb,
                              Srb->PathId,
                              Srb->TargetId,
                              Srb->Lun,
                              SP_BAD_FW_WARNING,
                              4
                        ));
            ScsiPortLogError( HwDeviceExtension,
                              Srb,
                              Srb->PathId,
                              Srb->TargetId,
                              Srb->Lun,
                              SP_BAD_FW_WARNING,
                              4);

            // Reprogram to not use Multi-sector.
            UCHAR statusByte;

            if (deviceExtension->lun[ldev].DeviceFlags & DFLAGS_DEVICE_PRESENT &&
                 !(deviceExtension->lun[ldev].DeviceFlags & DFLAGS_ATAPI_DEVICE)) {

                statusByte = AtaCommand(deviceExtension, ldev & 0x1, lChannel, IDE_COMMAND_SET_MULTIPLE, 0, 0, 0, 0, 0, ATA_WAIT_BASE_READY);

                // Check for errors. Reset the value to 0 (disable MultiBlock) if the
                // command was aborted.
                if (statusByte & IDE_STATUS_ERROR) {

                    // Read the error register.
                    errorByte = AtapiReadPort1(chan, IDX_IO1_i_Error);

                    KdPrint2((PRINT_PREFIX "MapError: Error setting multiple mode. Status %#x, error byte %#x\n",
                                statusByte,
                                errorByte));

                    // Adjust the devExt. value, if necessary.
                    deviceExtension->lun[ldev].MaximumBlockXfer = 0;
                    BrutePoint();

                }
            }
        }
    }

    // Set SCSI status to indicate a check condition.
    Srb->ScsiStatus = scsiStatus;

    return srbStatus;

} // end MapError()


/*++

Routine Description:

Arguments:
    HwDeviceExtension - HBA miniport driver's adapter data storage

Return Value:
    TRUE - if initialization successful.
    FALSE - if initialization unsuccessful.

--*/
BOOLEAN
DDKAPI
AtapiHwInitialize(
    IN PVOID HwDeviceExtension
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    ULONG                numberChannels  = deviceExtension->NumberChannels;
    ULONG c;

    KdPrint2((PRINT_PREFIX "AtapiHwInitialize: (base)\n"));

    /* do extra chipset specific setups */
    AtapiChipInit(HwDeviceExtension, DEVNUM_NOT_SPECIFIED, CHAN_NOT_SPECIFIED);
/*
    if(deviceExtension->Isr2DevObj && (deviceExtension->HwFlags & UNIATA_SATA)) {
        KdPrint2((PRINT_PREFIX " enable ISR2 to catch unexpected interrupts\n"));
        BMList[deviceExtension->DevIndex].Isr2Enable = TRUE;
    }
*/
    for (c = 0; c < numberChannels; c++) {
        AtapiHwInitialize__(deviceExtension, c);
    }
    KdPrint2((PRINT_PREFIX "AtapiHwInitialize: (base) done\n"));
    return TRUE;
} // end AtapiHwInitialize()

VOID
AtapiHwInitialize__(
    IN PHW_DEVICE_EXTENSION deviceExtension,
    IN ULONG lChannel
    )
{
    ULONG i;
    UCHAR statusByte, errorByte;
    PHW_CHANNEL chan = &(deviceExtension->chan[lChannel]);
    PHW_LU_EXTENSION     LunExt;
//    ULONG tmp32;
    ULONG PreferedMode = 0xffffffff;

    AtapiChipInit(deviceExtension, DEVNUM_NOT_SPECIFIED, lChannel);
    FindDevices(deviceExtension, FALSE, lChannel);

    for (i = lChannel*2; i < (lChannel+1)*2; i++) {

        KdPrint2((PRINT_PREFIX "AtapiHwInitialize: lChannel %#x\n", lChannel));

        LunExt = &(deviceExtension->lun[i]);
        // skip empty slots
        if (!(LunExt->DeviceFlags & DFLAGS_DEVICE_PRESENT)) {
            continue;
        }

        AtapiDisableInterrupts(deviceExtension, lChannel);
        AtapiStallExecution(1);

        if (!(LunExt->DeviceFlags & DFLAGS_ATAPI_DEVICE)) {

            KdPrint2((PRINT_PREFIX "AtapiHwInitialize: IDE branch\n"));
            // Enable media status notification
            IdeMediaStatus(TRUE,deviceExtension,(UCHAR)i);

            // If supported, setup Multi-block transfers.
            if (LunExt->MaximumBlockXfer) {

                statusByte = AtaCommand(deviceExtension, i & 1, lChannel,
                                    IDE_COMMAND_SET_MULTIPLE, 0, 0, 0,
                                    LunExt->MaximumBlockXfer, 0, ATA_WAIT_BASE_READY);

                // Check for errors. Reset the value to 0 (disable MultiBlock) if the
                // command was aborted.
                if (statusByte & IDE_STATUS_ERROR) {

                    // Read the error register.
                    errorByte = AtapiReadPort1(chan, IDX_IO1_i_Error);

                    KdPrint2((PRINT_PREFIX "AtapiHwInitialize: Error setting multiple mode. Status %#x, error byte %#x\n",
                                statusByte,
                                errorByte));

                    // Adjust the devExt. value, if necessary.
                    LunExt->MaximumBlockXfer = 0;

                } else {
                    KdPrint2((PRINT_PREFIX 
                                "AtapiHwInitialize: Using Multiblock on Device %d. Blocks / int - %d\n",
                                i,
                                LunExt->MaximumBlockXfer));
                }
            }

            if(LunExt->IdentifyData.MajorRevision) {
            
                if(LunExt->opt_ReadCacheEnable) {
                    KdPrint2((PRINT_PREFIX "  Try Enable Read Cache\n"));
                    // If supported, setup read/write cacheing
                    statusByte = AtaCommand(deviceExtension, i & 1, lChannel,
                                        IDE_COMMAND_SET_FEATURES, 0, 0, 0,
                                        0, ATA_C_F_ENAB_RCACHE, ATA_WAIT_BASE_READY);

                    // Check for errors.
                    if (statusByte & IDE_STATUS_ERROR) {
                        KdPrint2((PRINT_PREFIX 
                                    "AtapiHwInitialize: Enable read/write cacheing on Device %d failed\n",
                                    i));
                        LunExt->DeviceFlags &= ~DFLAGS_RCACHE_ENABLED;
                    } else {
                        LunExt->DeviceFlags |= DFLAGS_RCACHE_ENABLED;
                    }
                } else {
                    KdPrint2((PRINT_PREFIX "  Disable Read Cache\n"));
                    statusByte = AtaCommand(deviceExtension, i & 1, lChannel,
                                        IDE_COMMAND_SET_FEATURES, 0, 0, 0,
                                        0, ATA_C_F_DIS_RCACHE, ATA_WAIT_BASE_READY);
                    LunExt->DeviceFlags &= ~DFLAGS_RCACHE_ENABLED;
                }
                if(LunExt->opt_WriteCacheEnable) {
                    KdPrint2((PRINT_PREFIX "  Try Enable Write Cache\n"));
                    // If supported & allowed, setup write cacheing
                    statusByte = AtaCommand(deviceExtension, i & 1, lChannel,
                                        IDE_COMMAND_SET_FEATURES, 0, 0, 0,
                                        0, ATA_C_F_ENAB_WCACHE, ATA_WAIT_BASE_READY);
                    // Check for errors.
                    if (statusByte & IDE_STATUS_ERROR) {
                        KdPrint2((PRINT_PREFIX 
                                    "AtapiHwInitialize: Enable write cacheing on Device %d failed\n",
                                    i));
                        LunExt->DeviceFlags &= ~DFLAGS_WCACHE_ENABLED;
                    } else {
                        LunExt->DeviceFlags |= DFLAGS_WCACHE_ENABLED;
                    }
                } else {
                    KdPrint2((PRINT_PREFIX "  Disable Write Cache\n"));
                    statusByte = AtaCommand(deviceExtension, i & 1, lChannel,
                                        IDE_COMMAND_SET_FEATURES, 0, 0, 0,
                                        0, ATA_C_F_ENAB_WCACHE, ATA_WAIT_BASE_READY);
                    LunExt->DeviceFlags &= ~DFLAGS_WCACHE_ENABLED;
                }
            }

        } else if (!(LunExt->DeviceFlags & DFLAGS_CHANGER_INITED)){

            ULONG j;
            BOOLEAN isSanyo = FALSE;
            CCHAR vendorId[26];

            KdPrint2((PRINT_PREFIX "AtapiHwInitialize: ATAPI/Changer branch\n"));

            // Attempt to identify any special-case devices - psuedo-atapi changers, atapi changers, etc.
            for (j = 0; j < 26; j += 2) {

                // Build a buffer based on the identify data.
                vendorId[j] = RtlUshortByteSwap(((PUCHAR)LunExt->IdentifyData.ModelNumber)[j]);
            }

            if (!AtapiStringCmp (vendorId, "CD-ROM  CDR", 11)) {

                // Inquiry string for older model had a '-', newer is '_'
                if (vendorId[12] == 'C') {

                    // Torisan changer. Set the bit. This will be used in several places
                    // acting like 1) a multi-lun device and 2) building the 'special' TUR's.
                    LunExt->DeviceFlags |= (DFLAGS_CHANGER_INITED | DFLAGS_SANYO_ATAPI_CHANGER);
                    LunExt->DiscsPresent = 3;
                    isSanyo = TRUE;
                }
            }
        }

        PreferedMode = LunExt->opt_MaxTransferMode;
        if(PreferedMode == 0xffffffff) {
            KdPrint2((PRINT_PREFIX "MaxTransferMode (overriden): %#x\n", chan->MaxTransferMode));
            PreferedMode = chan->MaxTransferMode;
        }

        if(LunExt->opt_PreferedTransferMode != 0xffffffff) {
            KdPrint2((PRINT_PREFIX "PreferedTransferMode: %#x\n", PreferedMode));
            PreferedMode = min(LunExt->opt_PreferedTransferMode, PreferedMode);
        }

        KdPrint2((PRINT_PREFIX "  try mode %#x\n", PreferedMode));
        LunExt->OrigTransferMode =
        LunExt->LimitedTransferMode =
        LunExt->TransferMode =
            (CHAR)PreferedMode;

        AtapiDmaInit__(deviceExtension, i);

        LunExt->OrigTransferMode =
        LunExt->LimitedTransferMode =
            LunExt->TransferMode;
        KdPrint2((PRINT_PREFIX "Using %#x mode\n", LunExt->TransferMode));

        // We need to get our device ready for action before
        // returning from this function

        // According to the atapi spec 2.5 or 2.6, an atapi device
        // clears its status BSY bit when it is ready for atapi commands.
        // However, some devices (Panasonic SQ-TC500N) are still
        // not ready even when the status BSY is clear.  They don't react
        // to atapi commands.
        //
        // Since there is really no other indication that tells us
        // the drive is really ready for action.  We are going to check BSY
        // is clear and then just wait for an arbitrary amount of time!
        //
        if (LunExt->DeviceFlags & DFLAGS_ATAPI_DEVICE) {
            ULONG waitCount;

            // have to get out of the loop sometime!
            // 10000 * 100us = 1000,000us = 1000ms = 1s
            waitCount = 10000;
            GetStatus(chan, statusByte);
            while ((statusByte & IDE_STATUS_BUSY) && waitCount) {

                KdPrint2((PRINT_PREFIX "Wait for ATAPI (status %x\n)", statusByte));
                // Wait for Busy to drop.
                AtapiStallExecution(100);
                GetStatus(chan, statusByte);
                waitCount--;
            }

            // 5000 * 100us = 500,000us = 500ms = 0.5s
            waitCount = 5000;
            do {
                AtapiStallExecution(100);
            } while (waitCount--);
        }
        GetBaseStatus(chan, statusByte);
        AtapiEnableInterrupts(deviceExtension, lChannel);
        AtapiStallExecution(10);
    }

    return;

} // end AtapiHwInitialize()


#ifndef UNIATA_CORE

VOID
AtapiHwInitializeChanger(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PMECHANICAL_STATUS_INFORMATION_HEADER MechanismStatus)
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    ULONG                ldev = GET_LDEV(Srb);

    if (MechanismStatus) {
        deviceExtension->lun[ldev].DiscsPresent = MechanismStatus->NumberAvailableSlots;
        if (deviceExtension->lun[ldev].DiscsPresent > 1) {
            deviceExtension->lun[ldev].DeviceFlags |= DFLAGS_ATAPI_CHANGER;
        }
    }
    return;
} // end AtapiHwInitializeChanger()


/*++

Routine Description:
    This routine will parse the string for a match on the keyword, then
    calculate the value for the keyword and return it to the caller.

Arguments:
    String - The ASCII string to parse.
    KeyWord - The keyword for the value desired.

Return Values:
    Zero if value not found
    Value converted from ASCII to binary.

--*/
ULONG
AtapiParseArgumentString(
    IN PCHAR String,
    IN PCHAR KeyWord
    )
{
    PCHAR cptr;
    PCHAR kptr;
    ULONG value;
    ULONG stringLength = 0;
    ULONG keyWordLength = 0;
    ULONG index;

    if (!String) {
        return 0;
    }
    if (!KeyWord) {
        return 0;
    }

    // Calculate the string length and lower case all characters.
    cptr = String;
    while (*cptr) {
        if (*cptr >= 'A' && *cptr <= 'Z') {
            *cptr = *cptr + ('a' - 'A');
        }
        cptr++;
        stringLength++;
    }

    // Calculate the keyword length and lower case all characters.
    cptr = KeyWord;
    while (*cptr) {

        if (*cptr >= 'A' && *cptr <= 'Z') {
            *cptr = *cptr + ('a' - 'A');
        }
        cptr++;
        keyWordLength++;
    }

    if (keyWordLength > stringLength) {

        // Can't possibly have a match.
        return 0;
    }

    // Now setup and start the compare.
    cptr = String;

ContinueSearch:

    // The input string may start with white space.  Skip it.
    while (*cptr == ' ' || *cptr == '\t') {
        cptr++;
    }

    if (*cptr == '\0') {
        // end of string.
        return 0;
    }

    kptr = KeyWord;
    while (*cptr++ == *kptr++) {

        if (*(cptr - 1) == '\0') {
            // end of string
            return 0;
        }
    }

    if (*(kptr - 1) == '\0') {

        // May have a match backup and check for blank or equals.
        cptr--;
        while (*cptr == ' ' || *cptr == '\t') {
            cptr++;
        }

        // Found a match.  Make sure there is an equals.
        if (*cptr != '=') {

            // Not a match so move to the next semicolon.
            while (*cptr) {
                if (*cptr++ == ';') {
                    goto ContinueSearch;
                }
            }
            return 0;
        }
        // Skip the equals sign.
        cptr++;

        // Skip white space.
        while ((*cptr == ' ') || (*cptr == '\t')) {
            cptr++;
        }

        if (*cptr == '\0') {
            // Early end of string, return not found
            return 0;
        }

        if (*cptr == ';') {
            // This isn't it either.
            cptr++;
            goto ContinueSearch;
        }

        value = 0;
        if ((*cptr == '0') && (*(cptr + 1) == 'x')) {
            // Value is in Hex.  Skip the "0x"
            cptr += 2;
            for (index = 0; *(cptr + index); index++) {

                if (*(cptr + index) == ' ' ||
                    *(cptr + index) == '\t' ||
                    *(cptr + index) == ';') {
                     break;
                }

                if ((*(cptr + index) >= '0') && (*(cptr + index) <= '9')) {
                    value = (16 * value) + (*(cptr + index) - '0');
                } else {
                    if ((*(cptr + index) >= 'a') && (*(cptr + index) <= 'f')) {
                        value = (16 * value) + (*(cptr + index) - 'a' + 10);
                    } else {
                        // Syntax error, return not found.
                        return 0;
                    }
                }
            }
        } else {

            // Value is in Decimal.
            for (index = 0; *(cptr + index); index++) {

                if (*(cptr + index) == ' ' ||
                    *(cptr + index) == '\t' ||
                    *(cptr + index) == ';') {
                     break;
                }

                if ((*(cptr + index) >= '0') && (*(cptr + index) <= '9')) {
                    value = (10 * value) + (*(cptr + index) - '0');
                } else {

                    // Syntax error return not found.
                    return 0;
                }
            }
        }

        return value;
    } else {

        // Not a match check for ';' to continue search.
        while (*cptr) {
            if (*cptr++ == ';') {
                goto ContinueSearch;
            }
        }

        return 0;
    }
} // end AtapiParseArgumentString()_

/*
    Timer callback
*/
VOID
AtapiCallBack__(
    IN PVOID HwDeviceExtension,
    IN UCHAR lChannel
    )
{

    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    PHW_CHANNEL chan = &(deviceExtension->chan[lChannel]);
    ULONG c, _c;

    PSCSI_REQUEST_BLOCK  srb = UniataGetCurRequest(chan);
    UCHAR statusByte;

    KdPrint2((PRINT_PREFIX "AtapiCallBack:\n"));
    // If the last command was DSC restrictive, see if it's set. If so, the device is
    // ready for a new request. Otherwise, reset the timer and come back to here later.

    // If ISR decided to wait for BUSY or DRQ in DPC, we shall also get here.
    // In this case chan->ExpectingInterrupt == TRUE, but interrupts are disabled, thus,
    // we shall have no problem with interrupt handler.
    if (!srb || chan->ExpectingInterrupt) {
        KdPrint2((PRINT_PREFIX "AtapiCallBack: Calling ISR directly due to BUSY\n"));
        chan->DpcState = DPC_STATE_TIMER;
        if(!AtapiInterrupt__(HwDeviceExtension, lChannel)) {
            InterlockedExchange(&(chan->CheckIntr), CHECK_INTR_IDLE);
            KdPrint2((PRINT_PREFIX "AtapiCallBack: What's fucking this ???\n"));
        }
        goto ReturnCallback;
    }

#if DBG
    if (!IS_RDP((srb->Cdb[0]))) {
        KdPrint2((PRINT_PREFIX "AtapiCallBack: Invalid CDB marked as RDP - %#x\n", srb->Cdb[0]));
    }
#endif
    if(!(chan->RDP)) {
        goto ReturnEnableIntr;
    }
    GetStatus(chan, statusByte);
    if (statusByte & IDE_STATUS_DSC) {

        UCHAR PathId   = srb->PathId;
        UCHAR TargetId = srb->TargetId;
        UCHAR Lun      = srb->Lun;

        KdPrint2((PRINT_PREFIX "AtapiCallBack: Found DSC for RDP - %#x\n", srb->Cdb[0]));
        AtapiDmaDBSync(chan, srb);
        UniataRemoveRequest(chan, srb);
        ScsiPortNotification(RequestComplete, deviceExtension, srb);
        // Clear current SRB.
        if(!deviceExtension->simplexOnly) {
            srb = UniataGetCurRequest(chan);
        } else {
            srb = NULL;
        }
        chan->RDP = FALSE;

        // Ask for next request.
        ScsiPortNotification(NextLuRequest,
                             deviceExtension, 
                             PathId,
                             TargetId,
                             Lun);
        ScsiPortNotification(NextRequest, deviceExtension, NULL);

        if(srb) {
            AtapiStartIo__(HwDeviceExtension, srb, FALSE);
        }

    } else {
        KdPrint2((PRINT_PREFIX "AtapiCallBack: Requesting another timer for Op %#x\n",
                    srb->Cdb[0]));

        AtapiQueueTimerDpc(HwDeviceExtension, lChannel,
                             AtapiCallBack_X,
                             1000);

        goto ReturnCallback;
    }

ReturnEnableIntr:

    if(CrNtInterlockedExchangeAdd(&(chan->DisableIntr), 0)) {
        KdPrint2((PRINT_PREFIX "AtapiCallBack: CallDisableInterrupts\n"));
        //ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
#ifdef UNIATA_USE_XXableInterrupts
        chan->ChannelCtrlFlags |= CTRFLAGS_ENABLE_INTR_REQ;
        // must be called on DISPATCH_LEVEL
        ScsiPortNotification(CallDisableInterrupts, HwDeviceExtension,
                             AtapiEnableInterrupts__);
#else
        AtapiEnableInterrupts(HwDeviceExtension, lChannel);
        InterlockedExchange(&(chan->CheckIntr),
                                      CHECK_INTR_IDLE);
        // Will raise IRQL to DIRQL
        AtapiQueueTimerDpc(HwDeviceExtension, lChannel,
                             AtapiEnableInterrupts__,
                             1);
        KdPrint2((PRINT_PREFIX "AtapiInterrupt: Timer DPC inited\n"));
#endif // UNIATA_USE_XXableInterrupts
    } else {
        //ASSERT(!deviceExtension->simplexOnly);
    }

ReturnCallback:

    // Check other channel
    // In simplex mode no interrupts must appear on other channels
    for(_c=0; _c<deviceExtension->NumberChannels-1; _c++) {
        c = (_c+deviceExtension->FirstChannelToCheck) % deviceExtension->NumberChannels;

        chan = &(deviceExtension->chan[c]);

        if((ULONG)InterlockedCompareExchange(&chan->CheckIntr,
                                      CHECK_INTR_ACTIVE,
                                      CHECK_INTR_DETECTED) == CHECK_INTR_DETECTED) {
            //ASSERT(!deviceExtension->simplexOnly);
            chan->DpcState = DPC_STATE_ISR;
            if(!AtapiInterrupt__(HwDeviceExtension, (UCHAR)c)) {
                InterlockedExchange(&(chan->CheckIntr), CHECK_INTR_IDLE);
            }
        }
    }
    KdPrint2((PRINT_PREFIX "AtapiCallBack: return\n"));
    return;

} // end AtapiCallBack__()

VOID
AtapiCallBack_X(
    IN PVOID HwDeviceExtension
    )
{
    AtapiCallBack__(HwDeviceExtension, (UCHAR)((PHW_DEVICE_EXTENSION)HwDeviceExtension)->ActiveDpcChan);
}

#endif //UNIATA_CORE

/*++

Routine Description:

    This is the interrupt service routine for ATAPI IDE miniport driver.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage

Return Value:

    TRUE if expecting an interrupt.

--*/
BOOLEAN
DDKAPI
AtapiInterrupt(
    IN PVOID HwDeviceExtension
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    ULONG c, _c;
    BOOLEAN status = FALSE;
    ULONG c_state;
    ULONG i_res = 0;
    ULONG pass;
    BOOLEAN checked[AHCI_MAX_PORT];

    KdPrint2((PRINT_PREFIX "Intr: VendorID+DeviceID/Rev %#x/%#x\n", deviceExtension->DevID, deviceExtension->RevID));

    for(_c=0; _c<deviceExtension->NumberChannels; _c++) {
        checked[_c] = FALSE;
    }
//    fc = 
//    atapiDev = (deviceExtension->lun[ldev].DeviceFlags & DFLAGS_ATAPI_DEVICE) ? TRUE : FALSE;
    for(pass=0; pass<2; pass++) {
        for(_c=0; _c<deviceExtension->NumberChannels; _c++) {

            c = (_c+deviceExtension->FirstChannelToCheck) % deviceExtension->NumberChannels;
            //non_empty_chan = (deviceExtension->lun[c*2].DeviceFlags | deviceExtension->lun[c*2+1].DeviceFlags)
            //        & DFLAGS_DEVICE_PRESENT;

            if(checked[c])
                continue;

            // check non-empty and execting interrupt channels first
            if(!pass && !deviceExtension->chan[c].ExpectingInterrupt)
                continue;

            checked[c] = TRUE;

            KdPrint2((PRINT_PREFIX "AtapiInterrupt(base): cntrlr %#x chan %#x\n",deviceExtension->DevIndex, c));

            if(CrNtInterlockedExchangeAdd(&(deviceExtension->chan[c].DisableIntr), 0)) {
                KdPrint2((PRINT_PREFIX "AtapiInterrupt(base): disabled INTR on ch %d\n", c));
                continue;
            }
            // lock channel. Wait, while 2nd ISR checks interrupt on this channel
            do {
                KdPrint2((PRINT_PREFIX "AtapiInterrupt(base): try lock\n"));
                // c_state = deviceExtension->chan[c].CheckIntr;
                // if (deviceExtension->chan[c].CheckIntr == CHECK_INTR_DETECTED) {
                //     deviceExtension->chan[c].CheckIntr = CHECK_INTR_ACTIVE;
                // }
                c_state = (ULONG)InterlockedCompareExchange(&(deviceExtension->chan[c].CheckIntr),
                                              CHECK_INTR_ACTIVE,
                                              CHECK_INTR_DETECTED);
                if(c_state == CHECK_INTR_IDLE) {
                    // c_state = deviceExtension->chan[c].CheckIntr;
                    // if (deviceExtension->chan[c].CheckIntr == CHECK_INTR_IDLE) {
                    //     deviceExtension->chan[c].CheckIntr = CHECK_INTR_ACTIVE
                    // }
                    c_state = (ULONG)InterlockedCompareExchange(&(deviceExtension->chan[c].CheckIntr),
                                                  CHECK_INTR_ACTIVE,
                                                  CHECK_INTR_IDLE);
                }
            } while(c_state == CHECK_INTR_CHECK);
            KdPrint2((PRINT_PREFIX "AtapiInterrupt(base): locked\n"));
            // check if already serviced
            if(c_state == CHECK_INTR_ACTIVE) {
                KdPrint2((PRINT_PREFIX "AtapiInterrupt(base): CHECK_INTR_ACTIVE\n"));
                continue;
            }

            if((c_state == CHECK_INTR_DETECTED) ||
               (i_res = AtapiCheckInterrupt__(deviceExtension, (UCHAR)c))) {

                if(i_res == 2) {
                    KdPrint2((PRINT_PREFIX "AtapiInterrupt(base): Catch unexpected\n"));
                    InterlockedExchange(&(deviceExtension->chan[c].CheckIntr), CHECK_INTR_IDLE);
                    return TRUE;
                }
                // disable interrupts on other channel of legacy mode
                // ISA-bridged onboard controller
                if(deviceExtension->simplexOnly /*||
                   ((WinVer_Id() > WinVer_NT) && BMList[deviceExtension->DevIndex].MasterDev)*/) {
                    AtapiDisableInterrupts(deviceExtension, !c);
                }

                deviceExtension->chan[c].DpcState = DPC_STATE_ISR;
                if(AtapiInterrupt__(HwDeviceExtension, (UCHAR)c)) {
                    deviceExtension->LastInterruptedChannel = (UCHAR)c;
                    KdPrint2((PRINT_PREFIX "AtapiInterrupt(base): return status TRUE\n"));
                    status = TRUE;
                } else {
                    KdPrint2((PRINT_PREFIX "AtapiInterrupt(base): set CHECK_INTR_IDLE\n"));
                    InterlockedExchange(&(deviceExtension->chan[c].CheckIntr), CHECK_INTR_IDLE);
                }

                // re-enable interrupts on other channel
                if(deviceExtension->simplexOnly /*||
                   ((WinVer_Id() > WinVer_NT) && BMList[deviceExtension->DevIndex].MasterDev)*/) {
                    AtapiEnableInterrupts(deviceExtension, !c);
                }

            } else {
                KdPrint2((PRINT_PREFIX "AtapiInterrupt(base): set CHECK_INTR_IDLE (2)\n"));
                InterlockedExchange(&(deviceExtension->chan[c].CheckIntr), CHECK_INTR_IDLE);
            }

        }
    }
    KdPrint2((PRINT_PREFIX "AtapiInterrupt(base): exit with status %#x\n", status));
    if(status) {
        deviceExtension->FirstChannelToCheck++;
        if(deviceExtension->FirstChannelToCheck >= deviceExtension->NumberChannels)
            deviceExtension->FirstChannelToCheck = 0;
    }
    return status;
} // end AtapiInterrupt()

//ULONG i2c = 0;
#ifndef UNIATA_CORE

BOOLEAN
AtapiInterrupt2(
    IN PKINTERRUPT Interrupt,
    IN PVOID Isr2HwDeviceExtension
    )
{

    PISR2_DEVICE_EXTENSION Isr2DeviceExtension = (PISR2_DEVICE_EXTENSION)Isr2HwDeviceExtension;
    PHW_DEVICE_EXTENSION deviceExtension = Isr2DeviceExtension->HwDeviceExtension;
    ULONG c;
    BOOLEAN status = FALSE;
    ULONG c_count = 0;
    ULONG i_res;

    if(!BMList[deviceExtension->DevIndex].Isr2Enable) {
        KdPrint2((PRINT_PREFIX "AtapiInterrupt2: NOT ACTIVE cntrlr %#x chan %#x\n",deviceExtension->DevIndex, deviceExtension->Channel));
        return FALSE;
    }

    for(c=0; c<deviceExtension->NumberChannels; c++) {
        KdPrint2((PRINT_PREFIX "AtapiInterrupt2: cntrlr %#x chan %#x\n",deviceExtension->DevIndex, c));

        if(CrNtInterlockedExchangeAdd(&(deviceExtension->chan[c].DisableIntr), 0)) {
            KdPrint2((PRINT_PREFIX "AtapiInterrupt2: disabled INTR\n"));
            continue;
        }

        if((ULONG)CrNtInterlockedCompareExchange(&(deviceExtension->chan[c].CheckIntr),
                                      CHECK_INTR_CHECK,
                                      CHECK_INTR_IDLE) != CHECK_INTR_IDLE) {
            KdPrint2((PRINT_PREFIX "AtapiInterrupt2: !CHECK_INTR_IDLE\n"));
            // hunt on unexpected intr (Some devices generate double interrupts,
            // some controllers (at least CMD649) interrupt twice with small delay.
            // If interrupts are disabled, they queue interrupt and re-issue it later,
            // when we do not expect it.
            continue;
        }

        c_count++;
        if((i_res = AtapiCheckInterrupt__(deviceExtension, (UCHAR)c))) {

            KdPrint2((PRINT_PREFIX "AtapiInterrupt2: intr\n"));
            if(i_res == 2) {
                KdPrint2((PRINT_PREFIX "AtapiInterrupt2: Catch unexpected\n"));
                InterlockedExchange(&(deviceExtension->chan[c].CheckIntr), CHECK_INTR_IDLE);
                return TRUE;
            }

            status = TRUE;
            InterlockedExchange(&(deviceExtension->chan[c].CheckIntr), CHECK_INTR_DETECTED);
        } else {
            InterlockedExchange(&(deviceExtension->chan[c].CheckIntr), CHECK_INTR_IDLE);
        }
    }
    KdPrint2((PRINT_PREFIX "AtapiInterrupt2: status %d, c_count %d\n", status, c_count));
    if(status && (c_count != deviceExtension->NumberChannels)) {
        // there is an active ISR/DPC for one channel, but
        // we have an interrupt from another one
        // Lets inform current ISR/DPC about new interrupt
        InterlockedExchange(&(deviceExtension->ReCheckIntr), CHECK_INTR_DETECTED);
    } else {
        status = FALSE;
    }
    KdPrint2((PRINT_PREFIX "AtapiInterrupt2: return %d\n", status));
    return status;
    
} // end AtapiInterrupt2()

RETTYPE_XXableInterrupts
DDKAPI
AtapiInterruptDpc(
    IN PVOID HwDeviceExtension
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    ULONG c;

    for(c=0; c<deviceExtension->NumberChannels; c++) {
        KdPrint2((PRINT_PREFIX "AtapiInterruptDpc: %#x\n",c));

        if(!(deviceExtension->chan[c].ChannelCtrlFlags & CTRFLAGS_DPC_REQ)) {

            if((ULONG)InterlockedCompareExchange(&(deviceExtension->chan[c].CheckIntr),
                                          CHECK_INTR_ACTIVE,
                                          CHECK_INTR_DETECTED) != CHECK_INTR_DETECTED) {
                continue;
            }
                        
        } else {
            deviceExtension->chan[c].ChannelCtrlFlags &= ~CTRFLAGS_DPC_REQ;
        }
/*
        if(OldReqState != REQ_STATE_DPC_INTR_REQ) {
            AtapiDisableInterrupts(deviceExtension, lChannel);
        }
*/
        deviceExtension->chan[c].DpcState = DPC_STATE_DPC;
        if(!AtapiInterrupt__(HwDeviceExtension, (UCHAR)c)) {
            InterlockedExchange(&(deviceExtension->chan[c].CheckIntr), CHECK_INTR_IDLE);
        }
    }
    return RETVAL_XXableInterrupts;
} // end AtapiInterruptDpc()


RETTYPE_XXableInterrupts
DDKAPI
AtapiEnableInterrupts__(
    IN PVOID HwDeviceExtension
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    KdPrint2((PRINT_PREFIX "AtapiEnableInterrupts__():\n"));
    ULONG c;
    PHW_CHANNEL chan = NULL;

    for(c=0; c<deviceExtension->NumberChannels; c++) {
        KdPrint2((PRINT_PREFIX "AtapiEnableInterrupts__(2): %#x\n",c));
        chan = &(deviceExtension->chan[c]);

        if(chan->ChannelCtrlFlags & CTRFLAGS_ENABLE_INTR_REQ) {
            // enable intrs on requested channel
            chan->ChannelCtrlFlags &= ~CTRFLAGS_ENABLE_INTR_REQ;
            AtapiEnableInterrupts(HwDeviceExtension, c);
            InterlockedExchange(&(chan->CheckIntr),
                                          CHECK_INTR_IDLE);

            // check if current or other channel(s) interrupted
            //AtapiInterrupt(HwDeviceExtension);

            if(deviceExtension->simplexOnly) {
                break;
            }
        } else {
            // check if other channel(s) interrupted
            // must do nothing in simplex mode
            if((ULONG)CrNtInterlockedCompareExchange(&(chan->CheckIntr),
                                          CHECK_INTR_ACTIVE,
                                          CHECK_INTR_DETECTED) != CHECK_INTR_DETECTED) {
                continue;
            }
            //ASSERT(!deviceExtension->simplexOnly);
            chan->DpcState = DPC_STATE_ISR;
            if(!AtapiInterrupt__(HwDeviceExtension, (UCHAR)c)) {
                InterlockedExchange(&(chan->CheckIntr), CHECK_INTR_IDLE);
            }
        }
    }
    // In simplex mode next command must be sent to device here
    if(deviceExtension->simplexOnly && chan) {
        PSCSI_REQUEST_BLOCK srb;
        chan = UniataGetNextChannel(chan);
        if(chan) {
            srb = UniataGetCurRequest(chan);
        } else {
            srb = NULL;
        }
        if(srb) {
            AtapiStartIo__(HwDeviceExtension, srb, FALSE);
        }
    }

    return RETVAL_XXableInterrupts;

} // end AtapiEnableInterrupts__()

#endif //UNIATA_CORE


VOID
AtapiEnableInterrupts(
    IN PVOID HwDeviceExtension,
    IN ULONG c
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    KdPrint2((PRINT_PREFIX "AtapiEnableInterrupts_%d: %d\n",c, deviceExtension->chan[c].DisableIntr));
    if(c >= deviceExtension->NumberChannels) {
        return;
    }
    if(!InterlockedDecrement(&deviceExtension->chan[c].DisableIntr)) {
        AtapiWritePort1(&deviceExtension->chan[c], IDX_IO2_o_Control,
                               IDE_DC_A_4BIT );
        deviceExtension->chan[c].ChannelCtrlFlags &= ~CTRFLAGS_INTR_DISABLED;
    } else {
        AtapiWritePort1(&deviceExtension->chan[c], IDX_IO2_o_Control,
                               IDE_DC_DISABLE_INTERRUPTS /*| IDE_DC_A_4BIT*/ );
    }
    return;
} // end AtapiEnableInterrupts()

VOID
AtapiDisableInterrupts(
    IN PVOID HwDeviceExtension,
    IN ULONG c
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    KdPrint2((PRINT_PREFIX "AtapiDisableInterrupts_%d: %d\n",c, deviceExtension->chan[c].DisableIntr));
    // mark channel as busy
    if(c >= deviceExtension->NumberChannels) {
        return;
    }
    if(InterlockedIncrement(&deviceExtension->chan[c].DisableIntr)) {
        AtapiWritePort1(&deviceExtension->chan[c], IDX_IO2_o_Control,
                               IDE_DC_DISABLE_INTERRUPTS /*| IDE_DC_A_4BIT*/ );
        deviceExtension->chan[c].ChannelCtrlFlags |= CTRFLAGS_INTR_DISABLED;
    }

    return;
} // end AtapiDisableInterrupts()


/*
    Check hardware for interrupt state
 */
BOOLEAN
AtapiCheckInterrupt__(
    IN PVOID HwDeviceExtension,
    IN UCHAR c // logical channel
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    PHW_CHANNEL chan = &(deviceExtension->chan[c]);

    ULONG VendorID  = deviceExtension->DevID & 0xffff;
    ULONG ChipType  = deviceExtension->HwFlags & CHIPTYPE_MASK;

    ULONG status;
    ULONG pr_status = 0;
    UCHAR dma_status = 0;
    UCHAR reg8 = 0;
    UCHAR reg32 = 0;
    UCHAR statusByte;
    ULONG slotNumber = deviceExtension->slotNumber;
    ULONG SystemIoBusNumber = deviceExtension->SystemIoBusNumber;
    ULONG ChipFlags = deviceExtension->HwFlags & CHIPFLAG_MASK;
    UCHAR Channel;
    UCHAR lChannel;
    BOOLEAN DmaTransfer = FALSE;
    BOOLEAN OurInterrupt = FALSE;
    ULONG k;
    UCHAR interruptReason;
    BOOLEAN EarlyIntr = FALSE;

    KdPrint2((PRINT_PREFIX "AtapiCheckInterrupt__:\n"));

    lChannel = c;
    Channel = (UCHAR)(deviceExtension->Channel + lChannel);

    if(chan->ChannelCtrlFlags & CTRFLAGS_DMA_ACTIVE) {
        DmaTransfer = TRUE;
        KdPrint2((PRINT_PREFIX "  cntrlr %#x:%#x, lch %#x DmaTransfer = TRUE\n", deviceExtension->DevIndex,
            deviceExtension->Channel + c, c));
    } else {
        KdPrint2((PRINT_PREFIX "  cntrlr %#x:%#x, lch %#x DmaTransfer = FALSE\n", deviceExtension->DevIndex,
            deviceExtension->Channel + c, c));
        dma_status = GetDmaStatus(deviceExtension, lChannel);
        KdPrint2((PRINT_PREFIX "  DMA status %#x\n", dma_status));
    }

    // do controller-specific interrupt servicing staff
    if(deviceExtension->UnknownDev) {
        KdPrint2((PRINT_PREFIX "  UnknownDev\n"));
        goto check_unknown;
    }

    // Attention !
    // We can catch (BM_STATUS_ACTIVE + BM_STATUS_INTR) when operation is actually completed
    // Such behavior was observed with Intel ICH-xxx chips
    // This condition shall also be treated as 'our interrupt' because of BM_STATUS_INTR flag

    switch(VendorID) {

    case ATA_PROMISE_ID: {
        switch(ChipType) {
        case PROLD:
        case PRNEW:
            status = AtapiReadPortEx4(chan, (ULONG)(&deviceExtension->BaseIoAddressBM_0),0x1c);
            if (!DmaTransfer)
                break;
            if (!(status & 
                  ((Channel) ? 0x00004000 : 0x00000400))) {
                KdPrint2((PRINT_PREFIX "  Promise old/new unexpected\n"));
                return FALSE;
            }
            break;
        case PRTX:
            AtapiWritePort1(chan, IDX_BM_DeviceSpecific0, 0x0b);
            status = AtapiReadPort1(chan, IDX_BM_DeviceSpecific1);
            if (!DmaTransfer)
                break;
            if(!(status & 0x20)) {
                KdPrint2((PRINT_PREFIX "  Promise tx unexpected\n"));
                return FALSE;
            }
            break;
        case PRMIO:
            status = AtapiReadPortEx4(chan, (ULONG)(&deviceExtension->BaseIoAddressBM_0),0x0040);
            if(ChipFlags & PRSATA) {
                pr_status = AtapiReadPortEx4(chan, (ULONG)(&deviceExtension->BaseIoAddressBM_0),0x006c);
                AtapiWritePortEx4(chan, (ULONG)(&deviceExtension->BaseIoAddressBM_0),0x006c, pr_status & 0x000000ff);
            }
            if(pr_status & (0x11 << Channel)) {
                // TODO: reset channel
                KdPrint2((PRINT_PREFIX "  Promise mio unexpected + reset req\n"));
                return FALSE;
            }
            if(!(status & (0x01 << Channel))) {
                KdPrint2((PRINT_PREFIX "  Promise mio unexpected\n"));
                return FALSE;
            }
            AtapiWritePort4(chan, IDX_BM_DeviceSpecific0, 0x00000001);
            break;
        }
        break; }
    case ATA_NVIDIA_ID: {
        if(!(ChipFlags & UNIATA_SATA))
            break;

        KdPrint2((PRINT_PREFIX "NVIDIA\n"));

        ULONG offs = (ChipFlags & NV4OFF) ? 0x0440 : 0x0010;
        ULONG shift = Channel << ((ChipFlags & NVQ) ? 4 : 2);

        /* get and clear interrupt status */
        if(ChipFlags & NVQ) {
            pr_status = AtapiReadPortEx4(chan, (ULONG)(&deviceExtension->BaseIoAddressSATA_0),offs);
            AtapiWritePortEx4(chan, (ULONG)(&deviceExtension->BaseIoAddressSATA_0),offs, (0x0fUL << shift) | 0x00f000f0);
        } else {
            pr_status = AtapiReadPortEx1(chan, (ULONG)(&deviceExtension->BaseIoAddressSATA_0),offs);
            AtapiWritePortEx1(chan, (ULONG)(&deviceExtension->BaseIoAddressSATA_0),offs, (0x0f << shift));
        }
        KdPrint2((PRINT_PREFIX "  pr_status %x\n", pr_status));

        /* check for and handle connect events */
        if(((pr_status & (0x0cUL << shift)) == (0x04UL << shift)) ) {
            UniataSataEvent(deviceExtension, lChannel, UNIATA_SATA_EVENT_ATTACH);
        }
        /* check for and handle disconnect events */
        if((pr_status & (0x08UL << shift)) &&
            !((pr_status & (0x04UL << shift) &&
            AtapiReadPort4(chan, IDX_SATA_SStatus))) ) {
            UniataSataEvent(deviceExtension, lChannel, UNIATA_SATA_EVENT_DETACH);
        }
        /* do we have any device action ? */
        if(!(pr_status & (0x01UL << shift))) {
            KdPrint2((PRINT_PREFIX "  nVidia unexpected\n"));
            if(UniataSataClearErr(HwDeviceExtension, c, UNIATA_SATA_DO_CONNECT)) {
                OurInterrupt = 2;
            } else {
                return FALSE;
            }
        }

        break; }
    case ATA_ATI_ID:
        KdPrint2((PRINT_PREFIX "ATI\n"));
        if(ChipType == SIIMIO) {
            // fall to SiI
        } else {
            break;
        }
    case ATA_SILICON_IMAGE_ID:

        if(ChipType == SIIMIO) {

            reg32 = AtapiReadPort1(chan, IDX_BM_DeviceSpecific0);
            KdPrint2((PRINT_PREFIX "  Sii DS0 %x\n", reg32));
            if(reg32 == (UCHAR)-1) {
                KdPrint2((PRINT_PREFIX "  Sii mio unexpected\n"));
                return FALSE;
            }
            if(!(reg32 & (BM_DS0_SII_DMA_SATA_IRQ | BM_DS0_SII_DMA_COMPLETE | BM_DS0_SII_IRQ | BM_DS0_SII_DMA_ENABLE | BM_DS0_SII_DMA_ERROR))) {
                KdPrint2((PRINT_PREFIX "  Sii mio unexpected (2)\n"));
                return FALSE;
            }

            if(ChipFlags & UNIATA_SATA) {
                if(reg32 & (BM_DS0_SII_DMA_SATA_IRQ | BM_DS0_SII_IRQ)) {

                    /* SIEN doesn't mask SATA IRQs on some 3112s.  Those
                    * controllers continue to assert IRQ as long as
                    * SError bits are pending.  Clear SError immediately.
                    */
                    if(UniataSataClearErr(HwDeviceExtension, c, UNIATA_SATA_DO_CONNECT)) {
                        OurInterrupt = 2;
                    }
                }
            }

            if (!DmaTransfer)
                break;
            if (!((dma_status = GetDmaStatus(deviceExtension, lChannel)) & BM_STATUS_INTR)) {
                KdPrint2((PRINT_PREFIX "  Sii mio unexpected (3)\n"));
                return OurInterrupt;
            }
            AtapiWritePort1(chan, IDX_BM_Status, dma_status & ~BM_STATUS_ERR);
            goto skip_dma_stat_check;

        } else {
            if(!(deviceExtension->HwFlags & SIIINTR))
                break;
            GetPciConfig1(0x71, reg8);
            KdPrint2((PRINT_PREFIX "  0x71 = %#x\n", reg8));
            if (!(reg8 &
                  (Channel ? 0x08 : 0x04))) {
                return FALSE;
            }
            if (!DmaTransfer) {
                KdPrint2((PRINT_PREFIX "  cmd our\n"));
                OurInterrupt = 2;
            }
            SetPciConfig1(0x71, (Channel ? 0x08 : 0x04));
        }
        break;

    case ATA_ACARD_ID:
        if (!DmaTransfer)
            break;
        //dma_status = GetDmaStatus(deviceExtension, lChannel);
        if (!((dma_status = GetDmaStatus(deviceExtension, lChannel)) & BM_STATUS_INTR)) {
            KdPrint2((PRINT_PREFIX "  Acard unexpected\n"));
            return FALSE;
        }
        AtapiWritePort1(chan, IDX_BM_Status, dma_status | BM_STATUS_INTR);
        AtapiStallExecution(1);
        AtapiWritePort1(chan, IDX_BM_Command,
            AtapiReadPort1(chan, IDX_BM_Command) & ~BM_COMMAND_START_STOP);
        goto skip_dma_stat_check;
    default:
        if(deviceExtension->BaseIoAddressSATA_0.Addr &&
           (ChipFlags & UNIATA_SATA)) {
            if(UniataSataClearErr(HwDeviceExtension, c, UNIATA_SATA_DO_CONNECT)) {
                OurInterrupt = 2;
            }
        }
    }
check_unknown:
    KdPrint2((PRINT_PREFIX "  perform generic check\n"));
    if (DmaTransfer) {
        if (!((dma_status = GetDmaStatus(deviceExtension, lChannel)) & BM_STATUS_INTR)) {
            KdPrint2((PRINT_PREFIX "  DmaTransfer + !BM_STATUS_INTR (%x)\n", dma_status));
            if(dma_status & BM_STATUS_ERR) {
                KdPrint2((PRINT_PREFIX "  DmaTransfer + BM_STATUS_ERR -> our\n"));
                OurInterrupt = 2;
            } else {
                KdPrint2((PRINT_PREFIX "  getting status...\n"));
                GetStatus(chan, statusByte);
                KdPrint2((PRINT_PREFIX "  status %#x\n", statusByte));
                if(statusByte & IDE_STATUS_ERROR) {
                    KdPrint2((PRINT_PREFIX "  IDE_STATUS_ERROR -> our\n", statusByte));
                    OurInterrupt = 2;
                } else {
                    return FALSE;
                }
            }
        }
    } else {
        if(dma_status & BM_STATUS_INTR) {
            // bullshit, we have DMA interrupt, but had never initiate DMA operation
            KdPrint2((PRINT_PREFIX "  clear unexpected DMA intr\n"));
            AtapiDmaDone(deviceExtension, DEVNUM_NOT_SPECIFIED ,lChannel, NULL);
            // catch it !
            OurInterrupt = 2;
        }
    }
skip_dma_stat_check:
    if(!(ChipFlags & UNIATA_SATA)) {
        AtapiStallExecution(1);
    }

    /* if drive is busy it didn't interrupt */
    /* the exception is DCS + BSY state of ATAPI dvices */
    KdPrint2((PRINT_PREFIX "  getting status...\n"));
    GetStatus(chan, statusByte);
    KdPrint2((PRINT_PREFIX "  status %#x\n", statusByte));
    if (statusByte == 0xff) {
        // interrupt from empty controller ?
    } else 
    if (statusByte & IDE_STATUS_BUSY) {
        if(!chan->ExpectingInterrupt) {
            KdPrint2((PRINT_PREFIX "  unexpected intr + BUSY\n"));
            return OurInterrupt;
        }

        if(deviceExtension->lun[c*2 + chan->cur_cdev].DeviceFlags & DFLAGS_ATAPI_DEVICE) {
            KdPrint2((PRINT_PREFIX "  ATAPI additional check\n"));
        } else {
            KdPrint2((PRINT_PREFIX "  expecting intr + BUSY (3), non ATAPI\n"));
            return FALSE;
        }
        if(statusByte != (IDE_STATUS_BUSY | IDE_STATUS_DRDY | IDE_STATUS_DSC)) {
            KdPrint2((PRINT_PREFIX "  unexpected status, seems it is not our\n"));
            return FALSE;
        }

        EarlyIntr = TRUE;

        if(dma_status & BM_STATUS_INTR) {
            KdPrint2((PRINT_PREFIX "  our interrupt with BSY set, try wait in ISR or post to DPC\n"));
            /* clear interrupt and get status */
            GetBaseStatus(chan, statusByte);
            KdPrint2((PRINT_PREFIX "  base status %#x\n", statusByte));
            return TRUE;
        }

        if(g_WaitBusyInISR) {
            for(k=20; k; k--) {
                GetStatus(chan, statusByte);
                KdPrint2((PRINT_PREFIX "  status re-check %#x\n", statusByte));
                KdPrint2((PRINT_PREFIX "  Error reg (%#x)\n",
                            AtapiReadPort1(chan, IDX_IO1_i_Error)));
                if (!(statusByte & IDE_STATUS_BUSY)) {
                    KdPrint2((PRINT_PREFIX "  expecting intr + cleared BUSY\n"));
                    break;
                }
                break;
                //AtapiStallExecution(25);
            }
            if (statusByte & IDE_STATUS_BUSY) {
                KdPrint2((PRINT_PREFIX "  still BUSY, seems it is not our\n"));
                return FALSE;
            }
        }

    }

    /* clear interrupt and get status */
    GetBaseStatus(chan, statusByte);
    KdPrint2((PRINT_PREFIX "  base status %#x\n", statusByte));
    if (statusByte == 0xff) {
        // interrupt from empty controller ?
    } else 
    if(!(statusByte & (IDE_STATUS_DRQ | IDE_STATUS_DRDY))) {
        KdPrint2((PRINT_PREFIX "  no DRQ/DRDY set\n"));
        return OurInterrupt;
    }

#ifndef UNIATA_PIO_ONLY
    if(DmaTransfer) {
        if(!EarlyIntr || g_WaitBusyInISR) {
            dma_status = AtapiDmaDone(HwDeviceExtension, DEVNUM_NOT_SPECIFIED, lChannel, NULL/*srb*/);
        } else {
            PSCSI_REQUEST_BLOCK srb = UniataGetCurRequest(chan);
            PATA_REQ AtaReq = srb ? (PATA_REQ)(srb->SrbExtension) : NULL;

            //ASSERT(AtaReq);

            KdPrint2((PRINT_PREFIX "  set REQ_STATE_EARLY_INTR.\n"));
            if(AtaReq) {
                AtaReq->ReqState = REQ_STATE_EARLY_INTR;
            }
        }
    }
#endif //

    if (!(chan->ExpectingInterrupt)) {

        KdPrint2((PRINT_PREFIX "  Unexpected interrupt.\n"));

        if(deviceExtension->lun[c*2 + chan->cur_cdev * 2].DeviceFlags & DFLAGS_ATAPI_DEVICE) {
            KdPrint2((PRINT_PREFIX "  ATAPI additional check\n"));
        } else {
            KdPrint2((PRINT_PREFIX "  OurInterrupt = %d\n", OurInterrupt));
            return OurInterrupt;
        }
        interruptReason = (AtapiReadPort1(chan, IDX_ATAPI_IO1_i_InterruptReason) & 0x3);
        KdPrint2((PRINT_PREFIX "AtapiCheckInterrupt__: ATAPI int reason %x\n", interruptReason));
        return OurInterrupt;
    }
    //ASSERT(!chan->queue_depth || chan->cur_req);

    KdPrint2((PRINT_PREFIX "AtapiCheckInterrupt__: exit with TRUE\n"));
    return TRUE;

} // end AtapiCheckInterrupt__()


BOOLEAN
AtapiInterrupt__(
    IN PVOID HwDeviceExtension,
    IN UCHAR c
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    PHW_CHANNEL chan = &(deviceExtension->chan[c]);
    // Get current Srb
    PSCSI_REQUEST_BLOCK srb = UniataGetCurRequest(chan);
    PATA_REQ AtaReq = srb ? (PATA_REQ)(srb->SrbExtension) : NULL;

    ULONG wordCount = 0, wordsThisInterrupt = DEV_BSIZE/2;
    ULONG status = SRB_STATUS_SUCCESS;
    UCHAR dma_status = 0;
    ULONG i;
    ULONG k;
    UCHAR statusByte = 0,interruptReason;

    BOOLEAN atapiDev = FALSE;

    UCHAR Channel;
    UCHAR lChannel;
    UCHAR DeviceNumber;
    BOOLEAN DmaTransfer = FALSE;
    UCHAR error = 0;
    ULONG TimerValue = 1000;
#ifdef UNIATA_USE_XXableInterrupts
    BOOLEAN InDpc = (KeGetCurrentIrql() == DISPATCH_LEVEL);
#else
    BOOLEAN InDpc = (chan->DpcState != DPC_STATE_ISR);
#endif // UNIATA_USE_XXableInterrupts
    BOOLEAN UseDpc = deviceExtension->UseDpc;
//    BOOLEAN RestoreUseDpc = FALSE;
    BOOLEAN DataOverrun = FALSE;
    BOOLEAN NoStartIo = TRUE;

    KdPrint2((PRINT_PREFIX "AtapiInterrupt:\n"));
    if(InDpc) {
        KdPrint2((PRINT_PREFIX "  InDpc = TRUE\n"));
        //ASSERT((chan->ChannelCtrlFlags & CTRFLAGS_INTR_DISABLED));
    }

    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
    UCHAR OldReqState = REQ_STATE_NONE;
    ULONG ldev;
    PHW_LU_EXTENSION LunExt;

    lChannel = c;
    Channel = (UCHAR)(deviceExtension->Channel + lChannel);

    KdPrint2((PRINT_PREFIX "  cntrlr %#x:%d, irql %#x, c %d\n", deviceExtension->DevIndex, Channel, KeGetCurrentIrql(), c));

    if((chan->ChannelCtrlFlags & CTRFLAGS_DMA_ACTIVE) ||
       (AtaReq && (AtaReq->Flags & REQ_FLAG_DMA_OPERATION)) ) {
        DmaTransfer = TRUE;
        KdPrint2((PRINT_PREFIX "  DmaTransfer = TRUE\n"));
    }

    if (srb) {
        PathId   = srb->PathId;  
        TargetId = srb->TargetId;
        Lun      = srb->Lun;     
    } else {
        PathId = (UCHAR)c;
        TargetId =
        Lun      = 0;
        goto enqueue_next_req;
    }

    ldev = GET_LDEV2(PathId, TargetId, Lun);
    DeviceNumber = (UCHAR)(ldev & 1);
    LunExt = &(deviceExtension->lun[ldev]);
    atapiDev = (LunExt->DeviceFlags & DFLAGS_ATAPI_DEVICE) ? TRUE : FALSE;
    KdPrint2((PRINT_PREFIX "  dev_type %s\n", atapiDev ? "ATAPI" : "IDE"));

    // check if we are in ISR DPC
    if(InDpc) {
        KdPrint2((PRINT_PREFIX "  InDpc -> CTRFLAGS_INTR_DISABLED\n"));
        goto ServiceInterrupt;
    }

    if (DmaTransfer) {
        dma_status = GetDmaStatus(deviceExtension, lChannel);
    }

    if (!(chan->ExpectingInterrupt)) {

        KdPrint2((PRINT_PREFIX "  Unexpected interrupt for this channel.\n"));
        return FALSE;
    }

    // change request state
    if(AtaReq) {
        OldReqState = AtaReq->ReqState;
        AtaReq->ReqState = REQ_STATE_PROCESSING_INTR;
        KdPrint2((PRINT_PREFIX "  OldReqState = %x\n", OldReqState));
    }

    // We don't want using DPC for fast operations, like
    // DMA completion, sending CDB, short ATAPI transfers, etc.
    // !!!! BUT !!!!
    // We MUST use DPC, because of interprocessor synchronization
    // on multiprocessor platforms

    if(DmaTransfer)
        goto ServiceInterrupt;

    switch(OldReqState) {
    case REQ_STATE_ATAPI_EXPECTING_CMD_INTR:
    case REQ_STATE_ATAPI_EXPECTING_DATA_INTR:
    case REQ_STATE_DPC_WAIT_BUSY0:
    case REQ_STATE_DPC_WAIT_BUSY1:
        KdPrint2((PRINT_PREFIX "  continue service interrupt\n"));
        goto ServiceInterrupt;
    case REQ_STATE_ATAPI_DO_NOTHING_INTR:
        KdPrint2((PRINT_PREFIX "  do nothing on interrupt\n"));
        return TRUE;
    }

    if(!DmaTransfer && !atapiDev) {
        KdPrint2((PRINT_PREFIX "  service PIO HDD\n"));
        UseDpc = FALSE;
    }

#ifndef UNIATA_CORE

    if(!UseDpc)
        goto ServiceInterrupt;

#ifdef UNIATA_USE_XXableInterrupts
    if(InDpc) {
        KdPrint2((PRINT_PREFIX "  Unexpected InDpc\n"));
        ASSERT(FALSE);
        // shall never get here
        TimerValue = 1;
        goto CallTimerDpc;
    }

    KdPrint2((PRINT_PREFIX "  this is direct DPC call on DRQL\n"));
    if(AtaReq) {
        AtaReq->ReqState = REQ_STATE_DPC_INTR_REQ;
        KdPrint2((PRINT_PREFIX "  ReqState -> REQ_STATE_DPC_INTR_REQ\n"));
    } else {
        KdPrint2((PRINT_PREFIX "  DPC without AtaReq!!!\n"));
    }
#else
    KdPrint2((PRINT_PREFIX "call service interrupt\n"));
    goto ServiceInterrupt;
#endif // UNIATA_USE_XXableInterrupts

PostToDpc:

    // Attention !!!
    // AtapiInterruptDpc() is called on DISPATCH_LEVEL
    // We always get here when are called from timer callback, which is invoked on DRQL.
    // It is intended to lower IRQL and let other interrupts to be serviced while we are waiting for BUSY release

    KdPrint2((PRINT_PREFIX "AtapiInterrupt: start DPC init...\n"));
    // disable interrupts for this channel,
    // but avoid recursion and double-disable
    if(OldReqState != REQ_STATE_DPC_WAIT_BUSY1) {
        AtapiDisableInterrupts(deviceExtension, lChannel);
    }
    // go to ISR DPC
    chan->ChannelCtrlFlags |= CTRFLAGS_DPC_REQ;

#ifdef UNIATA_USE_XXableInterrupts
    // Will lower IRQL to DISPATCH_LEVEL
    ScsiPortNotification(CallEnableInterrupts, HwDeviceExtension,
                         /*c ?*/ AtapiInterruptDpc/*_1 : AtapiInterruptDpc_0*/);
    KdPrint2((PRINT_PREFIX "AtapiInterrupt: DPC inited\n"));
#else
    // Will raise IRQL to DIRQL
    AtapiQueueTimerDpc(HwDeviceExtension, c,
                         AtapiInterruptDpc,
                         TimerValue);
    KdPrint2((PRINT_PREFIX "AtapiInterrupt: Timer DPC inited\n"));
#endif // UNIATA_USE_XXableInterrupts
    return TRUE;

#ifndef UNIATA_CORE
CallTimerDpc:
    AtaReq->ReqState = REQ_STATE_PROCESSING_INTR;
CallTimerDpc2:
    // Will raise IRQL to DIRQL
    AtapiQueueTimerDpc(HwDeviceExtension, c,
                         AtapiCallBack_X,
                         TimerValue);
    return TRUE;
#endif //UNIATA_CORE

ServiceInterrupt:

    if(AtaReq && InDpc) {
        switch(AtaReq->ReqState) {
        case REQ_STATE_DPC_WAIT_DRQ0:
            goto PIO_wait_DRQ0;
        case REQ_STATE_DPC_WAIT_BUSY:
            goto PIO_wait_busy;
        case REQ_STATE_DPC_WAIT_DRQ:
            goto PIO_wait_DRQ;
        case REQ_STATE_DPC_WAIT_DRQ_ERR:
            goto continue_err;
        case REQ_STATE_DPC_WAIT_BUSY0:
        case REQ_STATE_DPC_WAIT_BUSY1:
            // continue normal execution
            break;
        }
    }
#else
ServiceInterrupt:
#endif //UNIATA_CORE

    // make additional delay for old devices (if we are not in DPC)
    if((!LunExt->IdentifyData.MajorRevision || (deviceExtension->lun[DeviceNumber].TransferMode < ATA_PIO4))
             &&
       !InDpc &&
       !atapiDev &&
       !(deviceExtension->HwFlags & UNIATA_SATA)
       ) {
        KdPrint2((PRINT_PREFIX "  additional delay 10us for old devices\n"));
        AtapiStallExecution(10);
    }

    /* clear interrupt and get status */
    GetBaseStatus(chan, statusByte);
    KdPrint2((PRINT_PREFIX "AtapiInterrupt: Entered with status (%#x)\n", statusByte));

    if(!UseDpc) {
        KdPrint2((PRINT_PREFIX "  operate like in DPC\n"));
        InDpc = TRUE;
    }
                                                                 	
    if (!atapiDev) {
        // IDE
        if (statusByte & IDE_STATUS_BUSY) {
            if (deviceExtension->DriverMustPoll) {
                // Crashdump is polling and we got caught with busy asserted.
                // Just go away, and we will be polled again shortly.
                KdPrint2((PRINT_PREFIX "  Hit BUSY while polling during crashdump.\n"));
                goto ReturnEnableIntr;
            }
try_dpc_wait:
            // Ensure BUSY is non-asserted.
            // make a very small idle before falling to DPC
            k = (InDpc && UseDpc) ? 1000 : 2;

            for (i = 0; i < k; i++) {

                GetBaseStatus(chan, statusByte);
                if (!(statusByte & IDE_STATUS_BUSY)) {
                    break;
                }
                AtapiStallExecution(10);
            }

            if (!InDpc && UseDpc && i == 2) {

                KdPrint2((PRINT_PREFIX "  BUSY on entry. Status %#x, Base IO %#x\n", statusByte));

                TimerValue = 50;
                AtaReq->ReqState = REQ_STATE_DPC_WAIT_BUSY0;

#ifndef UNIATA_CORE
                goto PostToDpc;
#else //UNIATA_CORE
                AtapiStallExecution(TimerValue);
                goto ServiceInterrupt;
#endif //UNIATA_CORE
            } else 
            if (InDpc && i == k) {
                // reset the controller.
                KdPrint2((PRINT_PREFIX 
                            "  Resetting due to BUSY on entry - %#x.\n",
                            statusByte));
                goto IntrPrepareResetController;
            }
        }
    } else {
        // ATAPI
        if(!LunExt->IdentifyData.MajorRevision &&
            InDpc &&
            !atapiDev &&
            !(deviceExtension->HwFlags & UNIATA_SATA)
            ) {
            KdPrint2((PRINT_PREFIX "  additional delay 10us for old devices (2)\n"));
            AtapiStallExecution(10);
        }
        if (statusByte & IDE_STATUS_BUSY) {
        //if(chan->ChannelCtrlFlags & CTRFLAGS_DSC_BSY) {
            KdPrint2((PRINT_PREFIX "  BUSY on ATAPI device, waiting\n"));
            for(k=20; k; k--) {
                GetStatus(chan, statusByte);
                KdPrint2((PRINT_PREFIX "  status re-check %#x\n", statusByte));
                KdPrint2((PRINT_PREFIX "  Error reg (%#x)\n",
                            AtapiReadPort1(chan, IDX_IO1_i_Error)));
                if (!(statusByte & IDE_STATUS_BUSY)) {
                    KdPrint2((PRINT_PREFIX "  expecting intr + cleared BUSY\n"));
                    break;
                }
                if(k <= 18) {
                    KdPrint2((PRINT_PREFIX "  too long wait -> DPC\n"));
                    if(!InDpc) {
                        KdPrint2((PRINT_PREFIX "  too long wait: ISR -> DPC\n"));
                        TimerValue = 100;
                        AtaReq->ReqState = REQ_STATE_DPC_WAIT_BUSY0;
                    } else {
                        KdPrint2((PRINT_PREFIX "  too long wait: DPC -> DPC\n"));
                        TimerValue = 1000;
                        AtaReq->ReqState = REQ_STATE_DPC_WAIT_BUSY1;
                    }
#ifndef UNIATA_CORE
                    goto CallTimerDpc2;
#else //UNIATA_CORE
                    AtapiStallExecution(TimerValue);
#endif //UNIATA_CORE
                }

                AtapiStallExecution(10);
            }
            if (statusByte & IDE_STATUS_BUSY) {
                KdPrint2((PRINT_PREFIX "  expecting intr + BUSY (2), try DPC wait\n"));
                goto try_dpc_wait;
            }
        }
    }

    if(AtaReq && DmaTransfer) {
        switch(OldReqState) {
        case REQ_STATE_EARLY_INTR:
        case REQ_STATE_DPC_WAIT_BUSY0:

            if(chan->ChannelCtrlFlags & CTRFLAGS_DMA_ACTIVE) {
                KdPrint2((PRINT_PREFIX "AtapiInterrupt: DMA still active\n"));
                dma_status = AtapiDmaDone(HwDeviceExtension, DEVNUM_NOT_SPECIFIED, lChannel, NULL/*srb*/);
            }
            break;
        }
    }

//retry_check:
    // Check for error conditions.
    if ((statusByte & IDE_STATUS_ERROR) ||
        (dma_status & BM_STATUS_ERR)) {

        error = AtapiReadPort1(chan, IDX_IO1_i_Error);
        KdPrint2((PRINT_PREFIX "AtapiInterrupt: Error %#x\n", error));
/*
        if(error & IDE_STATUS_CORRECTED_ERROR) {
            KdPrint2((PRINT_PREFIX "AtapiInterrupt: (corrected)\n"));
            statusByte &= ~IDE_STATUS_ERROR;
            goto retry_check;
        }
*/
        if(AtaReq) {
            KdPrint2((PRINT_PREFIX "  Bad Lba %#I64x\n", AtaReq->lba));
        } else {
            KdPrint2((PRINT_PREFIX "  Bad Lba unknown\n"));
        }

        KdPrint2((PRINT_PREFIX "  wait ready after error\n"));

        if(!atapiDev) {
            AtapiStallExecution(100);
        } else {
            AtapiStallExecution(10);
        }
continue_err:

        KdPrint2((PRINT_PREFIX "  Intr on DRQ %x\n",
            LunExt->DeviceFlags & DFLAGS_INT_DRQ));

        for (k = atapiDev ? 0 : 200; k; k--) {
            GetStatus(chan, statusByte);
            if (!(statusByte & IDE_STATUS_DRQ)) {
                AtapiStallExecution(50);
            } else {
                break;
            }
        }

        if (!atapiDev) {
            /* if this is a UDMA CRC error, reinject request */

            AtaReq->retry++;
            if(AtaReq->retry < MAX_RETRIES) {
#ifdef IO_STATISTICS
                chan->lun[DeviceNumber]->ModeErrorCount[AtaReq->retry]++;
#endif //IO_STATISTICS
                if(DmaTransfer /*&&
                   (error & IDE_ERROR_ICRC)*/) {
                    if(AtaReq->retry < MAX_RETRIES) {
//fallback_pio:
                        AtaReq->Flags &= ~REQ_FLAG_DMA_OPERATION;
                        AtaReq->Flags |= REQ_FLAG_FORCE_DOWNRATE;
//                        LunExt->DeviceFlags |= DFLAGS_FORCE_DOWNRATE;
                        AtaReq->ReqState = REQ_STATE_QUEUED;
                        goto reenqueue_req;
                    }
                } else {
                    if(!(AtaReq->Flags & REQ_FLAG_FORCE_DOWNRATE)) {
                        AtaReq->retry++;
                    }
                    KdPrint2((PRINT_PREFIX "Errors in PIO mode\n"));
                }
            }
        } else {
            interruptReason = (AtapiReadPort1(chan, IDX_ATAPI_IO1_i_InterruptReason) & 0x3);
            KdPrint2((PRINT_PREFIX "AtapiInterrupt: ATAPI Error, int reason %x\n", interruptReason));
        }

        KdPrint2((PRINT_PREFIX "AtapiInterrupt: Error\n"));
        if (srb->Cdb[0] != SCSIOP_REQUEST_SENSE) {
            // Fail this request.
            status = SRB_STATUS_ERROR;
            goto CompleteRequest;
        } else {
            KdPrint2((PRINT_PREFIX "  continue with SCSIOP_REQUEST_SENSE\n"));
        }
#ifdef IO_STATISTICS
    } else
    if(AtaReq->Flags & REQ_FLAG_FORCE_DOWNRATE_LBA48) {
        KdPrint2((PRINT_PREFIX "DMA doesn't work right with LBA48\n"));
        deviceExtension->HbaCtrlFlags |= HBAFLAGS_DMA_DISABLED_LBA48;
    } else
    if(AtaReq->Flags & REQ_FLAG_FORCE_DOWNRATE) {
        KdPrint2((PRINT_PREFIX "Some higher mode doesn't work right :((\n"));
        chan->lun[DeviceNumber]->RecoverCount[AtaReq->retry]++;
        if(chan->lun[DeviceNumber]->RecoverCount[AtaReq->retry] >= chan->lun[DeviceNumber]->IoCount/3) {
            deviceExtension->lun[DeviceNumber].LimitedTransferMode =
                deviceExtension->lun[DeviceNumber].TransferMode;
        }
#endif //IO_STATISTICS
    }
#ifdef IO_STATISTICS
    chan->lun[DeviceNumber]->IoCount++;
#endif //IO_STATISTICS

continue_PIO:

    // check reason for this interrupt.
    if (atapiDev) {

        KdPrint2((PRINT_PREFIX "AtapiInterrupt: ATAPI branch\n"));
        // ATAPI branch

        interruptReason = (AtapiReadPort1(chan, IDX_ATAPI_IO1_i_InterruptReason) & 0x3);
        KdPrint2((PRINT_PREFIX "AtapiInterrupt: iReason %x\n", interruptReason));
        if(DmaTransfer) {
            wordsThisInterrupt = DEV_BSIZE/2*512;
        } else {
            wordsThisInterrupt = DEV_BSIZE;
        }

    } else {

        // ATA branch

        if(DmaTransfer) {
            // simulate DRQ for DMA transfers
            statusByte |= IDE_STATUS_DRQ;
        }
        if (statusByte & IDE_STATUS_DRQ) {

            if(DmaTransfer) {
                wordsThisInterrupt = DEV_BSIZE/2*512;
            } else
            if (LunExt->MaximumBlockXfer) {
                wordsThisInterrupt = DEV_BSIZE/2 * LunExt->MaximumBlockXfer;
            }

            if (srb->SrbFlags & SRB_FLAGS_DATA_IN) {

                interruptReason =  0x2;

            } else if (srb->SrbFlags & SRB_FLAGS_DATA_OUT) {
                interruptReason = 0x0;

            } else {
                status = SRB_STATUS_ERROR;
                goto CompleteRequest;
            }

        } else if (statusByte & IDE_STATUS_BUSY) {

            //AtapiEnableInterrupts(deviceExtension, lChannel);
            KdPrint2((PRINT_PREFIX "AtapiInterrupt: return FALSE on ATA IDE_STATUS_BUSY\n"));
            return FALSE;

        } else {

            if (AtaReq->WordsLeft) {

                // Funky behaviour seen with PCI IDE (not all, just one).
PIO_wait_DRQ0:
                // The ISR hits with DRQ low, but comes up later.
                for (k = 0; k < 5000; k++) {
                    GetStatus(chan, statusByte);
                    if (statusByte & IDE_STATUS_DRQ) {
                        break;
                    }
                    if(!InDpc) {
                        // goto DPC
                        AtaReq->ReqState = REQ_STATE_DPC_WAIT_DRQ0;
                        TimerValue = 100;
                        KdPrint2((PRINT_PREFIX "AtapiInterrupt: go to DPC (drq0)\n"));
#ifndef UNIATA_CORE
                        goto PostToDpc;
#else //UNIATA_CORE
                        AtapiStallExecution(TimerValue);
                        goto ServiceInterrupt;
#endif //UNIATA_CORE
                    }
                    AtapiStallExecution(100);
                }
                if (k == 5000) {
                    // reset the controller.
                    KdPrint2((PRINT_PREFIX "AtapiInterrupt: Resetting due to DRQ not up. Status %#x\n",
                                statusByte));
IntrPrepareResetController:
                    AtapiResetController__(HwDeviceExtension, lChannel, RESET_COMPLETE_CURRENT);
                    goto ReturnEnableIntr;

                } else {
                    interruptReason = (srb->SrbFlags & SRB_FLAGS_DATA_IN) ? 0x2 : 0x0;
                }

            } else {
                // Command complete - verify, write, or the SMART enable/disable.
                // Also get_media_status
                interruptReason = 0x3;
            }
        }
    }

    KdPrint2((PRINT_PREFIX "AtapiInterrupt: i-reason=%d, status=%#x\n", interruptReason, statusByte));
    if (interruptReason == 0x1 && (statusByte & IDE_STATUS_DRQ)) {
        // Write the packet.
        KdPrint2((PRINT_PREFIX "AtapiInterrupt: Writing Atapi packet.\n"));
        // Send CDB to device.
        WriteBuffer(chan, (PUSHORT)srb->Cdb, 6, 0);
        AtaReq->ReqState = REQ_STATE_ATAPI_EXPECTING_DATA_INTR;

        if(chan->ChannelCtrlFlags & CTRFLAGS_DMA_OPERATION) {
            KdPrint2((PRINT_PREFIX "AtapiInterrupt: AtapiDmaStart().\n"));
            AtapiDmaStart(HwDeviceExtension, ldev & 1, lChannel, srb);
        }

        goto ReturnEnableIntr;

    } else if (interruptReason == 0x0 && (statusByte & IDE_STATUS_DRQ)) {

        // Write the data.
        if (atapiDev) {

            // Pick up bytes to transfer and convert to words.
            wordCount =
                AtapiReadPort1(chan, IDX_ATAPI_IO1_i_ByteCountLow);

            wordCount |=
                AtapiReadPort1(chan, IDX_ATAPI_IO1_i_ByteCountHigh) << 8;

            // Covert bytes to words.
            wordCount >>= 1;
            KdPrint2((PRINT_PREFIX "AtapiInterrupt: get W wordCount %#x\n", wordCount));

            if (wordCount != AtaReq->WordsLeft) {
                KdPrint2((PRINT_PREFIX 
                           "AtapiInterrupt: %d words requested; %d words xferred\n",
                           AtaReq->WordsLeft,
                           wordCount));
            }

            // Verify this makes sense.
            if (wordCount > AtaReq->WordsLeft) {
                wordCount = AtaReq->WordsLeft;
                KdPrint2((PRINT_PREFIX 
                           "AtapiInterrupt: Write underrun\n"));
                DataOverrun = TRUE;
            }

        } else {

            // IDE path. Check if words left is at least DEV_BSIZE/2 = 256.
            if (AtaReq->WordsLeft < wordsThisInterrupt) {

               // Transfer only words requested.
               wordCount = AtaReq->WordsLeft;

            } else {

               // Transfer next block.
               wordCount = wordsThisInterrupt;
            }
        }

        if (DmaTransfer && (chan->ChannelCtrlFlags & CTRFLAGS_DMA_OPERATION)) {
            //ASSERT(AtaReq->WordsLeft == wordCount);
            AtaReq->WordsLeft = 0;
            status = SRB_STATUS_SUCCESS;
            chan->ChannelCtrlFlags &= ~CTRFLAGS_DMA_OPERATION;
            goto CompleteRequest;
        }
        // Ensure that this is a write command.
        if (srb->SrbFlags & SRB_FLAGS_DATA_OUT) {

           KdPrint2((PRINT_PREFIX 
                      "AtapiInterrupt: Write interrupt\n"));

           statusByte = WaitOnBusy(chan);

           if (atapiDev || !(LunExt->DeviceFlags & DFLAGS_DWORDIO_ENABLED) /*!deviceExtension->DWordIO*/) {

               WriteBuffer(chan,
                           AtaReq->DataBuffer,
                           wordCount,
                           UniataGetPioTiming(LunExt));
           } else {

               WriteBuffer2(chan,
                           (PULONG)(AtaReq->DataBuffer),
                           wordCount / 2,
                           UniataGetPioTiming(LunExt));
           }
        } else {

            KdPrint2((PRINT_PREFIX 
                        "AtapiInterrupt: Int reason %#x, but srb is for a write %#x.\n",
                        interruptReason,
                        srb));

            // Fail this request.
            status = SRB_STATUS_ERROR;
            goto CompleteRequest;
        }

        // Advance data buffer pointer and bytes left.
        AtaReq->DataBuffer += wordCount;
        AtaReq->WordsLeft -= wordCount;

        if (atapiDev) {
            AtaReq->ReqState = REQ_STATE_ATAPI_EXPECTING_DATA_INTR;
        }

        goto ReturnEnableIntr;

    } else if (interruptReason == 0x2 && (statusByte & IDE_STATUS_DRQ)) {


        if (atapiDev) {

            // Pick up bytes to transfer and convert to words.
            wordCount =
                AtapiReadPort1(chan, IDX_ATAPI_IO1_i_ByteCountLow) |
                (AtapiReadPort1(chan, IDX_ATAPI_IO1_i_ByteCountHigh) << 8);

            // Covert bytes to words.
            wordCount >>= 1;
            KdPrint2((PRINT_PREFIX "AtapiInterrupt: get R wordCount %#x\n", wordCount));

            if (wordCount != AtaReq->WordsLeft) {
                KdPrint2((PRINT_PREFIX 
                           "AtapiInterrupt: %d words requested; %d words xferred\n",
                           AtaReq->WordsLeft,
                           wordCount));
            }

            // Verify this makes sense.
            if (wordCount > AtaReq->WordsLeft) {
                wordCount = AtaReq->WordsLeft;
                DataOverrun = TRUE;
            }

        } else {

            // Check if words left is at least 256.
            if (AtaReq->WordsLeft < wordsThisInterrupt) {

               // Transfer only words requested.
               wordCount = AtaReq->WordsLeft;

            } else {

               // Transfer next block.
               wordCount = wordsThisInterrupt;
            }
        }

        if (DmaTransfer && (chan->ChannelCtrlFlags & CTRFLAGS_DMA_OPERATION)) {
            //ASSERT(AtaReq->WordsLeft == wordCount);
            AtaReq->WordsLeft = 0;
            status = SRB_STATUS_SUCCESS;
            chan->ChannelCtrlFlags &= ~CTRFLAGS_DMA_OPERATION;
            goto CompleteRequest;
        }
        // Ensure that this is a read command.
        if (srb->SrbFlags & SRB_FLAGS_DATA_IN) {

/*           KdPrint2((
                      "AtapiInterrupt: Read interrupt\n"));*/

            statusByte = WaitOnBusy(chan);

            if (atapiDev || !(LunExt->DeviceFlags & DFLAGS_DWORDIO_ENABLED) /*!deviceExtension->DWordIO*/) {
                KdPrint2((PRINT_PREFIX 
                           "IdeIntr: Read %#x words\n", wordCount));

                ReadBuffer(chan,
                          AtaReq->DataBuffer,
                          wordCount,
                          UniataGetPioTiming(LunExt));
                KdPrint2(("IdeIntr: PIO Read AtaReq->DataBuffer %#x, srb->DataBuffer %#x\n", AtaReq->DataBuffer, (srb ? srb->DataBuffer : (void*)-1) ));
                //KdDump(AtaReq->DataBuffer, wordCount*2);

                GetStatus(chan, statusByte);
                KdPrint2((PRINT_PREFIX "  status re-check %#x\n", statusByte));

                if(DataOverrun) {
                    KdPrint2((PRINT_PREFIX "  DataOverrun\n"));
                    AtapiSuckPort2(chan);
                }

            } else {
                KdPrint2((PRINT_PREFIX 
                          "IdeIntr: Read %#x Dwords\n", wordCount/2));

                ReadBuffer2(chan,
                           (PULONG)(AtaReq->DataBuffer),
                           wordCount / 2,
                           UniataGetPioTiming(LunExt));
            }
        } else {

            KdPrint2((PRINT_PREFIX 
                        "AtapiInterrupt: Int reason %#x, but srb is for a read %#x.\n",
                        interruptReason,
                        srb));

            // Fail this request.
            status = SRB_STATUS_ERROR;
            goto CompleteRequest;
        }

        // Advance data buffer pointer and bytes left.
        AtaReq->DataBuffer += wordCount;
        AtaReq->WordsLeft -= wordCount;

        // Check for read command complete.
        if (AtaReq->WordsLeft == 0) {

            KdPrint2((PRINT_PREFIX "AtapiInterrupt: all transferred, AtaReq->WordsLeft == 0\n"));
            if (atapiDev) {

                // Work around to make many atapi devices return correct sector size
                // of 2048. Also certain devices will have sector count == 0x00, check
                // for that also.
                if ((srb->Cdb[0] == SCSIOP_READ_CAPACITY) &&
                    (LunExt->IdentifyData.DeviceType == ATAPI_TYPE_CDROM)) {

                    AtaReq->DataBuffer -= wordCount;
                    if (AtaReq->DataBuffer[0] == 0x00) {

                        *((ULONG *) &(AtaReq->DataBuffer[0])) = 0xFFFFFF7F;

                    }

                    *((ULONG *) &(AtaReq->DataBuffer[2])) = 0x00080000;
                    AtaReq->DataBuffer += wordCount;
                }
            } else {

            /*
                // Completion for IDE drives.
                if (AtaReq->WordsLeft) {
                    status = SRB_STATUS_DATA_OVERRUN;
                } else {
                    status = SRB_STATUS_SUCCESS;
                }

                goto CompleteRequest;
            */
                status = SRB_STATUS_SUCCESS;
                goto CompleteRequest;

            }
        } else {
            if (atapiDev) {
                AtaReq->ReqState = REQ_STATE_ATAPI_EXPECTING_DATA_INTR;
            }
        }

        goto ReturnEnableIntr;

    } else if (interruptReason == 0x3  && !(statusByte & IDE_STATUS_DRQ)) {

        KdPrint2((PRINT_PREFIX "AtapiInterrupt: interruptReason = CompleteRequest\n"));
        // Command complete.
        if(DmaTransfer) {
            KdPrint2((PRINT_PREFIX "AtapiInterrupt: CompleteRequest, was DmaTransfer\n"));
            AtaReq->WordsLeft = 0;
        }
        if (AtaReq->WordsLeft) {
            status = SRB_STATUS_DATA_OVERRUN;
        } else {
            status = SRB_STATUS_SUCCESS;
        }

#ifdef UNIATA_DUMP_ATAPI
        if(srb &&
           srb->SrbFlags & SRB_FLAGS_DATA_IN) {
            UCHAR                   ScsiCommand;
            PCDB                    Cdb;
            PCHAR                   CdbData;
            PCHAR                   ModeSelectData;
            ULONG                   CdbDataLen;
            PSCSI_REQUEST_BLOCK     Srb = srb;

            Cdb = (PCDB)(Srb->Cdb);
            ScsiCommand = Cdb->CDB6.OperationCode;
            CdbData = (PCHAR)(Srb->DataBuffer);
            CdbDataLen = Srb->DataTransferLength;

            if(CdbDataLen > 0x1000) {
                CdbDataLen = 0x1000;
            }

            KdPrint(("--\n"));
            KdPrint2(("VendorID+DeviceID/Rev %#x/%#x\n", deviceExtension->DevID, deviceExtension->RevID));
            KdPrint2(("P:T:D=%d:%d:%d\n",
                                      Srb->PathId,
                                      Srb->TargetId,
                                      Srb->Lun));
            KdPrint(("Complete SCSI Command %2.2x\n", ScsiCommand));
            KdDump(Cdb, 16);

            if(ScsiCommand == SCSIOP_MODE_SENSE) {
                KdPrint(("ModeSense 6\n"));
                PMODE_PARAMETER_HEADER ParamHdr = (PMODE_PARAMETER_HEADER)CdbData;
                ModeSelectData = CdbData+4;
                KdDump(CdbData, CdbDataLen);
            } else
            if(ScsiCommand == SCSIOP_MODE_SENSE10) {
                KdPrint(("ModeSense 10\n"));
                PMODE_PARAMETER_HEADER ParamHdr = (PMODE_PARAMETER_HEADER)CdbData;
                ModeSelectData = CdbData+8;
                KdDump(CdbData, CdbDataLen);
            } else {
                if(srb->SrbFlags & SRB_FLAGS_DATA_IN) {
                    KdPrint(("Read buffer from device:\n"));
                    KdDump(CdbData, CdbDataLen);
                }
            }
            KdPrint(("--\n"));
        }
#endif //UNIATA_DUMP_ATAPI

CompleteRequest:

        KdPrint2((PRINT_PREFIX "AtapiInterrupt: CompleteRequest\n"));
        // Check and see if we are processing our secret (mechanism status/request sense) srb
        if (AtaReq->OriginalSrb) {

            ULONG srbStatus;

            KdPrint2((PRINT_PREFIX "AtapiInterrupt: OriginalSrb != NULL\n"));
            if (srb->Cdb[0] == SCSIOP_MECHANISM_STATUS) {

                KdPrint2((PRINT_PREFIX "AtapiInterrupt: SCSIOP_MECHANISM_STATUS status %#x\n", status));
                if (status == SRB_STATUS_SUCCESS) {
                    // Bingo!!
                    AtapiHwInitializeChanger (HwDeviceExtension,
                                              srb,
                                              (PMECHANICAL_STATUS_INFORMATION_HEADER) srb->DataBuffer);

                    // Get ready to issue the original srb
                    srb = AtaReq->Srb = AtaReq->OriginalSrb;
                    AtaReq->OriginalSrb = NULL;

                } else {
                    // failed!  Get the sense key and maybe try again
                    srb = AtaReq->Srb = BuildRequestSenseSrb (
                                                          HwDeviceExtension,
                                                          AtaReq->OriginalSrb);
                }
/*
                // do not enable interrupts in DPC, do not waste time, do it now!
                if(UseDpc && chan->DisableIntr) {
                    AtapiEnableInterrupts(HwDeviceExtension, c);
                    UseDpc = FALSE;
                    RestoreUseDpc = TRUE;
                }
*/
                srbStatus = AtapiSendCommand(HwDeviceExtension, srb, CMD_ACTION_ALL);

                KdPrint2((PRINT_PREFIX "AtapiInterrupt: chan->ExpectingInterrupt %d (1)\n", chan->ExpectingInterrupt));

                if (srbStatus == SRB_STATUS_PENDING) {
                    KdPrint2((PRINT_PREFIX "AtapiInterrupt: send orig SRB_STATUS_PENDING (1)\n"));
                    goto ReturnEnableIntr;
                }
/*
                if(RestoreUseDpc) {
                    // restore state on error
                    UseDpc = TRUE;
                    AtapiDisableInterrupts(HwDeviceExtension, c);
                }
*/

            } else { // srb->Cdb[0] == SCSIOP_REQUEST_SENSE)

                PSENSE_DATA senseData = (PSENSE_DATA) srb->DataBuffer;

                KdPrint2((PRINT_PREFIX "AtapiInterrupt: ATAPI command status %#x\n", status));
                if (status == SRB_STATUS_DATA_OVERRUN) {
                    // Check to see if we at least get mininum number of bytes
                    if ((srb->DataTransferLength - AtaReq->WordsLeft) >
                        (offsetof (SENSE_DATA, AdditionalSenseLength) + sizeof(senseData->AdditionalSenseLength))) {
                        status = SRB_STATUS_SUCCESS;
                    }
                }

                if (status == SRB_STATUS_SUCCESS) {
#ifndef UNIATA_CORE
                    if ((senseData->SenseKey != SCSI_SENSE_ILLEGAL_REQUEST) &&
                        chan->MechStatusRetryCount) {

                        // The sense key doesn't say the last request is illegal, so try again
                        chan->MechStatusRetryCount--;
                        srb = AtaReq->Srb = BuildMechanismStatusSrb (
                                                              HwDeviceExtension,
                                                              AtaReq->OriginalSrb);
                    } else {

                        // last request was illegal.  No point trying again
                        AtapiHwInitializeChanger (HwDeviceExtension,
                                                  srb,
                                                  (PMECHANICAL_STATUS_INFORMATION_HEADER) NULL);

                        // Get ready to issue the original srb
                        srb = AtaReq->Srb = AtaReq->OriginalSrb;
                        AtaReq->OriginalSrb = NULL;
                    }
#endif //UNIATA_CORE
/*
                    // do not enable interrupts in DPC, do not waste time, do it now!
                    if(UseDpc && chan->DisableIntr) {
                        AtapiEnableInterrupts(HwDeviceExtension, c);
                        UseDpc = FALSE;
                        RestoreUseDpc = TRUE;
                    }
*/
                    srbStatus = AtapiSendCommand(HwDeviceExtension, srb, CMD_ACTION_ALL);

                    KdPrint2((PRINT_PREFIX "AtapiInterrupt: chan->ExpectingInterrupt %d (2)\n", chan->ExpectingInterrupt));

                    if (srbStatus == SRB_STATUS_PENDING) {
                        KdPrint2((PRINT_PREFIX "AtapiInterrupt: send orig SRB_STATUS_PENDING (2)\n"));
                        goto ReturnEnableIntr;
                    }
/*
                    if(RestoreUseDpc) {
                        // restore state on error
                        UseDpc = TRUE;
                        AtapiDisableInterrupts(HwDeviceExtension, c);
                    }
*/
                }
            }

            // If we get here, it means AtapiSendCommand() has failed
            // Can't recover.  Pretend the original srb has failed and complete it.

            KdPrint2((PRINT_PREFIX "AtapiInterrupt: Error. complete OriginalSrb\n"));

            if (AtaReq->OriginalSrb) {
                KdPrint2((PRINT_PREFIX "AtapiInterrupt: call AtapiHwInitializeChanger()\n"));
                AtapiHwInitializeChanger (HwDeviceExtension,
                                          srb,
                                          (PMECHANICAL_STATUS_INFORMATION_HEADER) NULL);
                srb = AtaReq->Srb = AtaReq->OriginalSrb;
                AtaReq->OriginalSrb = NULL;
            }

            KdPrint2((PRINT_PREFIX "AtapiInterrupt: chan->ExpectingInterrupt %d (3)\n", chan->ExpectingInterrupt));

            // fake an error and read no data
            status = SRB_STATUS_ERROR;
            srb->ScsiStatus = 0;
            AtaReq->DataBuffer = (PUSHORT)(srb->DataBuffer);
            AtaReq->WordsLeft = srb->DataTransferLength;
            chan->RDP = FALSE;

        } else if (status == SRB_STATUS_ERROR) {

            // Map error to specific SRB status and handle request sense.
            KdPrint2((PRINT_PREFIX "AtapiInterrupt: Error. Begin mapping...\n"));
            status = MapError(deviceExtension,
                              srb);

            chan->RDP = FALSE;

        } else if(!DmaTransfer) {

            KdPrint2((PRINT_PREFIX "AtapiInterrupt: PIO completion\n"));
PIO_wait_busy:
            KdPrint2((PRINT_PREFIX "AtapiInterrupt: PIO completion, wait BUSY\n"));
            // Wait for busy to drop.
            for (i = 0; i < 5*30; i++) {
                GetStatus(chan, statusByte);
                if (!(statusByte & IDE_STATUS_BUSY)) {
                    break;
                }
                if(!InDpc) {
                    // goto DPC
                    AtaReq->ReqState = REQ_STATE_DPC_WAIT_BUSY;
                    TimerValue = 200;
                    KdPrint2((PRINT_PREFIX "AtapiInterrupt: go to DPC (busy)\n"));
#ifndef UNIATA_CORE
                    goto PostToDpc;
#else //UNIATA_CORE
                    AtapiStallExecution(TimerValue);
                    goto ServiceInterrupt;
#endif //UNIATA_CORE
                }
                AtapiStallExecution(100);
            }

            if (i == 5*30) {

                // reset the controller.
                KdPrint2((PRINT_PREFIX 
                            "AtapiInterrupt: Resetting due to BSY still up - %#x.\n",
                            statusByte));
                goto IntrPrepareResetController;
            }
            // Check to see if DRQ is still up.
            if(statusByte & IDE_STATUS_DRQ) {
                KdPrint2((PRINT_PREFIX "AtapiInterrupt: DRQ...\n"));
                if(srb) {
                    if(srb->SrbFlags & SRB_FLAGS_DATA_IN) {
                        KdPrint2((PRINT_PREFIX "srb %x data in\n", srb));
                    } else {
                        KdPrint2((PRINT_PREFIX "srb %x data out\n", srb));
                    }
                } else {
                    KdPrint2((PRINT_PREFIX "srb NULL\n"));
                }
                if(AtaReq) {
                    KdPrint2((PRINT_PREFIX "AtaReq %x AtaReq->WordsLeft=%x\n", AtaReq, AtaReq->WordsLeft));
                } else {
                    KdPrint2((PRINT_PREFIX "AtaReq NULL\n"));
                }
                if(AtaReq && AtaReq->WordsLeft /*&&
                   !(LunExt->DeviceFlags & (DFLAGS_ATAPI_DEVICE | DFLAGS_TAPE_DEVICE | DFLAGS_LBA_ENABLED))*/) {
                    KdPrint2((PRINT_PREFIX "DRQ+AtaReq->WordsLeft -> next portion\n"));
                    goto continue_PIO;
                }
            }
            //if (atapiDev && (statusByte & IDE_STATUS_DRQ)) {}
            //if ((statusByte & IDE_STATUS_DRQ)) {}
            if((statusByte & IDE_STATUS_DRQ) &&
               (LunExt->DeviceFlags & (DFLAGS_ATAPI_DEVICE | DFLAGS_TAPE_DEVICE | DFLAGS_LBA_ENABLED)) ) {

PIO_wait_DRQ:
                KdPrint2((PRINT_PREFIX "AtapiInterrupt: PIO_wait_DRQ\n"));
                for (i = 0; i < 200; i++) {
                    GetStatus(chan, statusByte);
                    if (!(statusByte & IDE_STATUS_DRQ)) {
                        break;
                    }
                    if(!InDpc) {
                        // goto DPC
                        KdPrint2((PRINT_PREFIX "AtapiInterrupt: go to DPC (drq)\n"));
                        AtaReq->ReqState = REQ_STATE_DPC_WAIT_DRQ;
                        TimerValue = 100;
#ifndef UNIATA_CORE
                        goto PostToDpc;
#else //UNIATA_CORE
                        AtapiStallExecution(TimerValue);
                        goto ServiceInterrupt;
#endif //UNIATA_CORE
                    }
                    AtapiStallExecution(100);
                }

                if (i == 200) {
                    // reset the controller.
                    KdPrint2((PRINT_PREFIX   "AtapiInterrupt: Resetting due to DRQ still up - %#x\n",
                                statusByte));
                    goto IntrPrepareResetController;
                }
            }
            if(atapiDev) {
                KdPrint2(("IdeIntr: ATAPI Read AtaReq->DataBuffer %#x, srb->DataBuffer %#x, len %#x\n",
                     AtaReq->DataBuffer, (srb ? srb->DataBuffer : (void*)(-1)), srb->DataTransferLength ));
                //KdDump(srb->DataBuffer, srb->DataTransferLength);
            }
            if(!AtapiDmaPioSync(HwDeviceExtension, srb, (PUCHAR)(srb->DataBuffer), srb->DataTransferLength)) {
                KdPrint2(("IdeIntr: Can't sync DMA and PIO buffers\n"));
            }
        }

        // Clear interrupt expecting flag.
        chan->ExpectingInterrupt = FALSE;
        InterlockedExchange(&(chan->CheckIntr),
                                      CHECK_INTR_IDLE);

        // Sanity check that there is a current request.
        if (srb != NULL) {
            // Set status in SRB.
            srb->SrbStatus = (UCHAR)status;

            // Check for underflow.
            if (AtaReq->WordsLeft) {

                KdPrint2((PRINT_PREFIX "AtapiInterrupt: Check for underflow, AtaReq->WordsLeft %x\n", AtaReq->WordsLeft));
                // Subtract out residual words and update if filemark hit,
                // setmark hit , end of data, end of media...
                if (!(LunExt->DeviceFlags & DFLAGS_TAPE_DEVICE)) {
                    if (status == SRB_STATUS_DATA_OVERRUN) {
                        srb->DataTransferLength -= AtaReq->WordsLeft;
                    } else {
                        srb->DataTransferLength = 0;
                    }
                } else {
                    srb->DataTransferLength -= AtaReq->WordsLeft;
                }
            }

            if (srb->Function != SRB_FUNCTION_IO_CONTROL) {

CompleteRDP:
                // Indicate command complete.
                if (!(chan->RDP)) {
                    KdPrint2((PRINT_PREFIX "AtapiInterrupt: RequestComplete\n"));
IntrCompleteReq:

                    if (status == SRB_STATUS_SUCCESS &&
                        srb->SenseInfoBuffer &&
                        srb->SenseInfoBufferLength >= sizeof(SENSE_DATA)) {

                        PSENSE_DATA  senseBuffer = (PSENSE_DATA)srb->SenseInfoBuffer;

                        KdPrint2((PRINT_PREFIX "AtapiInterrupt: set AutoSense\n"));
                        senseBuffer->ErrorCode = 0;
                        senseBuffer->Valid     = 1;
                        senseBuffer->AdditionalSenseLength = 0xb;
                        senseBuffer->SenseKey =  0;
                        senseBuffer->AdditionalSenseCode = 0;
                        senseBuffer->AdditionalSenseCodeQualifier = 0;

                        srb->SrbStatus |= SRB_STATUS_AUTOSENSE_VALID;
                    }
                    AtapiDmaDBSync(chan, srb);
                    KdPrint2((PRINT_PREFIX "AtapiInterrupt: remove srb %#x, status %x\n", srb, status));
                    UniataRemoveRequest(chan, srb);
                    KdPrint2((PRINT_PREFIX "AtapiInterrupt: RequestComplete, srb %#x\n", srb));
                    ScsiPortNotification(RequestComplete,
                                         deviceExtension,
                                         srb);
                }
            } else {

                KdPrint2((PRINT_PREFIX "AtapiInterrupt: IOCTL completion\n"));
                PSENDCMDOUTPARAMS cmdOutParameters = (PSENDCMDOUTPARAMS)(((PUCHAR)srb->DataBuffer) + sizeof(SRB_IO_CONTROL));

                if (status != SRB_STATUS_SUCCESS) {
                    error = AtapiReadPort1(chan, IDX_IO1_i_Error);
                    KdPrint2((PRINT_PREFIX "AtapiInterrupt: error %#x\n", error));
                }

                // Build the SMART status block depending upon the completion status.
                cmdOutParameters->cBufferSize = wordCount;
                cmdOutParameters->DriverStatus.bDriverError = (error) ? SMART_IDE_ERROR : 0;
                cmdOutParameters->DriverStatus.bIDEError = error;

                // If the sub-command is return smart status, jam the value from cylinder low and high, into the
                // data buffer.
                if (chan->SmartCommand == RETURN_SMART_STATUS) {
                    cmdOutParameters->bBuffer[0] = RETURN_SMART_STATUS;
                    cmdOutParameters->bBuffer[1] = AtapiReadPort1(chan, IDX_ATAPI_IO1_i_InterruptReason);
                    cmdOutParameters->bBuffer[2] = AtapiReadPort1(chan, IDX_ATAPI_IO1_i_Unused1);
                    cmdOutParameters->bBuffer[3] = AtapiReadPort1(chan, IDX_ATAPI_IO1_i_ByteCountLow);
                    cmdOutParameters->bBuffer[4] = AtapiReadPort1(chan, IDX_ATAPI_IO1_i_ByteCountHigh);
                    cmdOutParameters->bBuffer[5] = AtapiReadPort1(chan, IDX_ATAPI_IO1_i_DriveSelect);
                    cmdOutParameters->bBuffer[6] = SMART_CMD;
                    cmdOutParameters->cBufferSize = 8;
                }

                // Indicate command complete.
                goto IntrCompleteReq;
            }

        } else {

            KdPrint2((PRINT_PREFIX "AtapiInterrupt: No SRB!\n"));
        }

        if (chan->RDP) {
            // Check DSC
            for (i = 0; i < 5; i++) {
                GetStatus(chan, statusByte);
                if(!(statusByte & IDE_STATUS_BUSY)) {
                    KdPrint2((PRINT_PREFIX "AtapiInterrupt: RDP + cleared BUSY\n"));
                    chan->RDP = FALSE;
                    goto CompleteRDP;
                } else
                if (statusByte & IDE_STATUS_DSC) {
                    KdPrint2((PRINT_PREFIX "AtapiInterrupt: Clear RDP\n"));
                    chan->RDP = FALSE;
                    goto CompleteRDP;
                } 
                AtapiStallExecution(50);
            }
        }
        // RDP can be cleared since previous check
        if (chan->RDP) {
            KdPrint2((PRINT_PREFIX "AtapiInterrupt: RequestTimerCall 2000\n"));

            TimerValue = 2000;
#ifndef UNIATA_CORE
            goto CallTimerDpc;
#else //UNIATA_CORE
            AtapiStallExecution(TimerValue);
            goto ServiceInterrupt;
#endif //UNIATA_CORE
        }

//            ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
enqueue_next_req:
        // Get next request
        srb = UniataGetCurRequest(chan);

reenqueue_req:

#ifndef UNIATA_CORE
        KdPrint2((PRINT_PREFIX "AtapiInterrupt: NextRequest, srb=%#x\n",srb));
        if(!srb) {
            ScsiPortNotification(NextRequest,
                                 deviceExtension,
                                 NULL);
        } else {
            ScsiPortNotification(NextLuRequest,
                                 deviceExtension, 
                                 PathId,
                                 TargetId,
                                 Lun);
            // in simplex mode next command must NOT be sent here
            if(!deviceExtension->simplexOnly) {
                AtapiStartIo__(HwDeviceExtension, srb, FALSE);
            }
        }
        // Try to get SRB fron any non-empty queue (later)
        if(deviceExtension->simplexOnly) {
            NoStartIo = FALSE;
        }
#endif //UNIATA_CORE

        goto ReturnEnableIntr;

    } else {

        // Unexpected int. Catch it
        KdPrint2((PRINT_PREFIX "AtapiInterrupt: Unexpected ATAPI interrupt. InterruptReason %#x. Status %#x.\n",
                    interruptReason,
                    statusByte));

    }

ReturnEnableIntr:

    KdPrint2((PRINT_PREFIX "AtapiInterrupt: ReturnEnableIntr\n",srb));
    if(UseDpc) {
        if(CrNtInterlockedExchangeAdd(&(chan->DisableIntr), 0)) {
            KdPrint2((PRINT_PREFIX "AtapiInterrupt: call AtapiEnableInterrupts__()\n"));
#ifdef UNIATA_USE_XXableInterrupts
            //ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
            chan->ChannelCtrlFlags |= CTRFLAGS_ENABLE_INTR_REQ;
            // must be called on DISPATCH_LEVEL
            ScsiPortNotification(CallDisableInterrupts, HwDeviceExtension,
                                 AtapiEnableInterrupts__);
#else
            AtapiEnableInterrupts(HwDeviceExtension, c);
            InterlockedExchange(&(chan->CheckIntr),
                                          CHECK_INTR_IDLE);
            // Will raise IRQL to DIRQL
#ifndef UNIATA_CORE
            AtapiQueueTimerDpc(HwDeviceExtension, lChannel,
                                 AtapiEnableInterrupts__,
                                 1);
#endif // UNIATA_CORE
            KdPrint2((PRINT_PREFIX "AtapiInterrupt: Timer DPC inited\n"));
#endif // UNIATA_USE_XXableInterrupts
        }
    }

    InterlockedExchange(&(chan->CheckIntr), CHECK_INTR_IDLE);
    // in simplex mode next command must be sent here if
    // DPC is not used
    KdPrint2((PRINT_PREFIX "AtapiInterrupt: exiting, UseDpc=%d, NoStartIo=%d\n", UseDpc, NoStartIo));

#ifndef UNIATA_CORE
    if(!UseDpc && /*deviceExtension->simplexOnly &&*/ !NoStartIo) {
        chan = UniataGetNextChannel(chan);
        if(chan) {
            srb = UniataGetCurRequest(chan);
        } else {
            srb = NULL;
        }
        KdPrint2((PRINT_PREFIX "AtapiInterrupt: run srb %x\n", srb));
        if(srb) {
            AtapiStartIo__(HwDeviceExtension, srb, FALSE);
        }
    }
#endif //UNIATA_CORE
    return TRUE;

} // end AtapiInterrupt__()

#ifndef UNIATA_CORE

/*++

Routine Description:

    This routine handles SMART enable, disable, read attributes and threshold commands.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    Srb - IO request packet

Return Value:

    SRB status

--*/
ULONG
IdeSendSmartCommand(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    ULONG                c               = GET_CHANNEL(Srb);
    PHW_CHANNEL          chan            = &(deviceExtension->chan[c]);
    PATA_REQ             AtaReq          = (PATA_REQ)(Srb->SrbExtension);
    PSENDCMDOUTPARAMS    cmdOutParameters = (PSENDCMDOUTPARAMS)(((PUCHAR)Srb->DataBuffer) + sizeof(SRB_IO_CONTROL));
    SENDCMDINPARAMS      cmdInParameters = *(PSENDCMDINPARAMS)(((PUCHAR)Srb->DataBuffer) + sizeof(SRB_IO_CONTROL));
    PIDEREGS             regs            = &cmdInParameters.irDriveRegs;
//    ULONG                i;
    UCHAR                statusByte,targetId;


    if (regs->bCommandReg != SMART_CMD) {
        KdPrint2((PRINT_PREFIX 
                    "IdeSendSmartCommand: bCommandReg != SMART_CMD\n"));
        return SRB_STATUS_INVALID_REQUEST;
    }

    targetId = cmdInParameters.bDriveNumber;

    //TODO optimize this check
    if ((!(deviceExtension->lun[targetId].DeviceFlags & DFLAGS_DEVICE_PRESENT)) ||
         (deviceExtension->lun[targetId].DeviceFlags & DFLAGS_ATAPI_DEVICE)) {

        return SRB_STATUS_SELECTION_TIMEOUT;
    }

    chan->SmartCommand = regs->bFeaturesReg;

    // Determine which of the commands to carry out.
    switch(regs->bFeaturesReg) {
    case READ_ATTRIBUTES:
    case READ_THRESHOLDS:

        statusByte = WaitOnBusy(chan);

        if (statusByte & IDE_STATUS_BUSY) {
            KdPrint2((PRINT_PREFIX 
                        "IdeSendSmartCommand: Returning BUSY status\n"));
            return SRB_STATUS_BUSY;
        }

        // Zero the ouput buffer as the input buffer info. has been saved off locally (the buffers are the same).
        RtlZeroMemory(cmdOutParameters, sizeof(SENDCMDOUTPARAMS) + READ_ATTRIBUTE_BUFFER_SIZE - 1);

        // Set data buffer pointer and words left.
        AtaReq->DataBuffer = (PUSHORT)cmdOutParameters->bBuffer;
        AtaReq->WordsLeft = READ_ATTRIBUTE_BUFFER_SIZE / 2;

        statusByte = AtaCommand(deviceExtension, targetId & 0x1, c,
                   regs->bCommandReg,
                   (USHORT)(regs->bCylLowReg) | (((USHORT)(regs->bCylHighReg)) << 8),
                   0,
                   regs->bSectorNumberReg,
                   regs->bSectorCountReg,
                   regs->bFeaturesReg,
                   ATA_IMMEDIATE);

        if(!(statusByte & IDE_STATUS_ERROR)) {
            // Wait for interrupt.
            return SRB_STATUS_PENDING;
        }
        return SRB_STATUS_ERROR;

    case ENABLE_SMART:
    case DISABLE_SMART:
    case RETURN_SMART_STATUS:
    case ENABLE_DISABLE_AUTOSAVE:
    case EXECUTE_OFFLINE_DIAGS:
    case SAVE_ATTRIBUTE_VALUES:

        statusByte = WaitOnBusy(chan);

        if (statusByte & IDE_STATUS_BUSY) {
            KdPrint2((PRINT_PREFIX 
                        "IdeSendSmartCommand: Returning BUSY status\n"));
            return SRB_STATUS_BUSY;
        }

        // Zero the ouput buffer as the input buffer info. has been saved off locally (the buffers are the same).
        RtlZeroMemory(cmdOutParameters, sizeof(SENDCMDOUTPARAMS) - 1);

        // Set data buffer pointer and indicate no data transfer.
        AtaReq->DataBuffer = (PUSHORT)cmdOutParameters->bBuffer;
        AtaReq->WordsLeft = 0;

        statusByte = AtaCommand(deviceExtension, targetId & 0x1, c,
                   regs->bCommandReg,
                   (USHORT)(regs->bCylLowReg) | (((USHORT)(regs->bCylHighReg)) << 8),
                   0,
                   regs->bSectorNumberReg,
                   regs->bSectorCountReg,
                   regs->bFeaturesReg,
                   ATA_IMMEDIATE);

        if(!(statusByte & IDE_STATUS_ERROR)) {
            // Wait for interrupt.
            return SRB_STATUS_PENDING;
        }
        return SRB_STATUS_ERROR;
    } // end switch(regs->bFeaturesReg)

    return SRB_STATUS_INVALID_REQUEST;

} // end IdeSendSmartCommand()

#endif //UNIATA_CORE

ULONGLONG
UniAtaCalculateLBARegs(
    PHW_LU_EXTENSION     LunExt,
    ULONG                startingSector
    )
{
    UCHAR                drvSelect,sectorNumber;
    USHORT               cylinder;
    ULONG                tmp;

    if(LunExt->DeviceFlags & DFLAGS_LBA_ENABLED) {
        return startingSector;
    }
    tmp = LunExt->IdentifyData.SectorsPerTrack *
                       LunExt->IdentifyData.NumberOfHeads;
    if(!tmp) {
        KdPrint2((PRINT_PREFIX "UniAtaCalculateLBARegs: 0-sized\n"));
        cylinder     = 0;
        drvSelect    = 0;
        sectorNumber = 1;
    } else {
        cylinder =    (USHORT)(startingSector / tmp);
        drvSelect =   (UCHAR)((startingSector % tmp) / LunExt->IdentifyData.SectorsPerTrack); 
        sectorNumber = (UCHAR)(startingSector % LunExt->IdentifyData.SectorsPerTrack) + 1;
    }

    return (ULONG)(sectorNumber&0xff) | (((ULONG)cylinder&0xffff)<<8) | (((ULONG)drvSelect&0xf)<<24);
} // end UniAtaCalculateLBARegs()

ULONGLONG
UniAtaCalculateLBARegsBack(
    PHW_LU_EXTENSION     LunExt,
    ULONGLONG            lba
    )
{
    ULONG                drvSelect,sectorNumber;
    ULONG                cylinder;
    ULONG                tmp;

    if(LunExt->DeviceFlags & DFLAGS_LBA_ENABLED) {
        return lba;
    }
    tmp = LunExt->IdentifyData.SectorsPerTrack *
                       LunExt->IdentifyData.NumberOfHeads;

    cylinder     = (USHORT)((lba >> 8) & 0xffff);
    drvSelect    = (UCHAR)((lba >> 24) & 0xf);
    sectorNumber = (UCHAR)(lba & 0xff);

    lba = sectorNumber-1 +
          (drvSelect*LunExt->IdentifyData.SectorsPerTrack) +
          (cylinder*tmp);

    return lba;
} // end UniAtaCalculateLBARegsBack()


/*++

Routine Description:

    This routine handles IDE read and writes.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    Srb - IO request packet

Return Value:

    SRB status

--*/
ULONG
IdeReadWrite(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN ULONG CmdAction
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    UCHAR                lChannel = GET_CHANNEL(Srb);
    PHW_CHANNEL          chan = &(deviceExtension->chan[lChannel]);
    PHW_LU_EXTENSION     LunExt;
    PATA_REQ             AtaReq = (PATA_REQ)(Srb->SrbExtension);
    ULONG                ldev = GET_LDEV(Srb);
    UCHAR                DeviceNumber = (UCHAR)(ldev & 1);
    ULONG                startingSector;
    ULONG                wordCount = 0;
    UCHAR                statusByte,statusByte2;
    UCHAR                cmd;
    ULONGLONG            lba;
    BOOLEAN              use_dma = FALSE;

    AtaReq->Flags |= REQ_FLAG_REORDERABLE_CMD;
    LunExt = &deviceExtension->lun[ldev];

    if((CmdAction & CMD_ACTION_PREPARE) &&
       (AtaReq->ReqState != REQ_STATE_READY_TO_TRANSFER)) {

        // Set data buffer pointer and words left.
        AtaReq->DataBuffer = (PUSHORT)Srb->DataBuffer;

        MOV_SWP_DW2DD(AtaReq->bcount, ((PCDB)Srb->Cdb)->CDB10.TransferBlocks);
        //ASSERT(Srb->DataTransferLength == AtaReq->bcount * DEV_BSIZE);
        AtaReq->WordsLeft = min(Srb->DataTransferLength,
                                AtaReq->bcount * DEV_BSIZE) / 2;

        // Set up 1st block.
        MOV_DD_SWP(startingSector, ((PCDB)Srb->Cdb)->CDB10.LBA);

        KdPrint2((PRINT_PREFIX "IdeReadWrite (REQ): Starting sector is %#x, Number of WORDS %#x, DevSize %#x\n",
                   startingSector,
                   AtaReq->WordsLeft,
                   AtaReq->bcount));

        lba = UniAtaCalculateLBARegs(LunExt, startingSector);
        AtaReq->lba = lba;

        // assume best case here
        // we cannot reinit Dma until previous request is completed
        if ((LunExt->LimitedTransferMode >= ATA_DMA)) {
            use_dma = TRUE;
            // this will set REQ_FLAG_DMA_OPERATION in AtaReq->Flags on success
            if(!AtapiDmaSetup(HwDeviceExtension, DeviceNumber, lChannel, Srb,
                          (PUCHAR)(AtaReq->DataBuffer),
                          ((Srb->DataTransferLength + DEV_BSIZE-1) & ~(DEV_BSIZE-1)))) {
                use_dma = FALSE;
            }
        }
        AtaReq->ReqState = REQ_STATE_READY_TO_TRANSFER;
    } else { // exec_only
        KdPrint2((PRINT_PREFIX "IdeReadWrite (ExecOnly): \n"));
        lba = AtaReq->lba;

        if(AtaReq->Flags & REQ_FLAG_DMA_OPERATION) {
            use_dma = TRUE;
        }
    }
    if(!(CmdAction & CMD_ACTION_EXEC)) {
        return SRB_STATUS_PENDING;
    }

    // if this is queued request, reinit DMA and check
    // if DMA mode is still available
    AtapiDmaReinit(deviceExtension, ldev, AtaReq);
    if (/*EnableDma &&*/
        (LunExt->TransferMode >= ATA_DMA)) {
        use_dma = TRUE;
    } else {
        AtaReq->Flags &= ~REQ_FLAG_DMA_OPERATION;
        use_dma = FALSE;
    }

    // Check if write request.
    if (Srb->SrbFlags & SRB_FLAGS_DATA_IN) {

        // Prepare read command.
        if(use_dma) {
            cmd = IDE_COMMAND_READ_DMA;
        } else
        if(LunExt->MaximumBlockXfer) {
            cmd = IDE_COMMAND_READ_MULTIPLE;
        } else {
            cmd = IDE_COMMAND_READ;
        }
    } else {

        // Prepare write command.
        if (use_dma) {
            wordCount = Srb->DataTransferLength/2;
            cmd = IDE_COMMAND_WRITE_DMA;
        } else
        if (LunExt->MaximumBlockXfer) {
            wordCount = DEV_BSIZE/2 * LunExt->MaximumBlockXfer;

            if (AtaReq->WordsLeft < wordCount) {
               // Transfer only words requested.
               wordCount = AtaReq->WordsLeft;
            }
            cmd = IDE_COMMAND_WRITE_MULTIPLE;

        } else {
            wordCount = DEV_BSIZE/2;
            cmd = IDE_COMMAND_WRITE;
        }
    }

    if(use_dma) {
        chan->ChannelCtrlFlags |= CTRFLAGS_DMA_OPERATION;
    } else {
        chan->ChannelCtrlFlags &= ~CTRFLAGS_DMA_OPERATION;
    }

    // Send IO command.
    KdPrint2((PRINT_PREFIX "IdeReadWrite: Lba %#I64x, Count %#x(%#x)\n", lba, ((Srb->DataTransferLength + 0x1FF) / 0x200),
                                                           ((wordCount*2 + DEV_BSIZE-1) / DEV_BSIZE)));

    if ((Srb->SrbFlags & SRB_FLAGS_DATA_IN) ||
        use_dma) {
        statusByte2 = AtaCommand48(deviceExtension, DeviceNumber, lChannel,
                     cmd, lba,
                     (UCHAR)((Srb->DataTransferLength + DEV_BSIZE-1) / DEV_BSIZE),
//                     (UCHAR)((wordCount*2 + DEV_BSIZE-1) / DEV_BSIZE),
                     0, ATA_IMMEDIATE);
        GetStatus(chan, statusByte2);
        if(statusByte2 & IDE_STATUS_ERROR) {
            statusByte = AtapiReadPort1(chan, IDX_IO1_i_Error);
            KdPrint2((PRINT_PREFIX "IdeReadWrite: status %#x, error %#x\n", statusByte2, statusByte));
            return SRB_STATUS_ERROR;
        }
        if(use_dma) {
            AtapiDmaStart(HwDeviceExtension, DeviceNumber, lChannel, Srb);
        }
        return SRB_STATUS_PENDING;
    }

    statusByte = AtaCommand48(deviceExtension, DeviceNumber, lChannel,
                 cmd, lba,
                 (UCHAR)((Srb->DataTransferLength + DEV_BSIZE-1) / DEV_BSIZE),
//                 (UCHAR)((wordCount*2 + DEV_BSIZE-1) / DEV_BSIZE),
                 0, ATA_WAIT_INTR);

    if (!(statusByte & IDE_STATUS_DRQ)) {

        KdPrint2((PRINT_PREFIX 
                   "IdeReadWrite: DRQ never asserted (%#x)\n",
                   statusByte));

        AtaReq->WordsLeft = 0;

        // Clear interrupt expecting flag.
        chan->ExpectingInterrupt = FALSE;
        InterlockedExchange(&(chan->CheckIntr),
                                      CHECK_INTR_IDLE);

        // Clear current SRB.
        UniataRemoveRequest(chan, Srb);

        return SRB_STATUS_TIMEOUT;
    }

    chan->ExpectingInterrupt = TRUE;
    InterlockedExchange(&(chan->CheckIntr),
                                  CHECK_INTR_IDLE);

    // Write next DEV_BSIZE/2*N words.
    if (!(LunExt->DeviceFlags & DFLAGS_DWORDIO_ENABLED)) {
        KdPrint2((PRINT_PREFIX 
                   "IdeReadWrite: Write %#x words\n", wordCount));

        WriteBuffer(chan,
                  AtaReq->DataBuffer,
                  wordCount,
                  UniataGetPioTiming(LunExt));

    } else {

        KdPrint2((PRINT_PREFIX 
                   "IdeReadWrite: Write %#x Dwords\n", wordCount/2));

        WriteBuffer2(chan,
                   (PULONG)(AtaReq->DataBuffer),
                   wordCount / 2,
                   UniataGetPioTiming(LunExt));
    }

    // Adjust buffer address and words left count.
    AtaReq->WordsLeft -= wordCount;
    AtaReq->DataBuffer += wordCount;

    // Wait for interrupt.
    return SRB_STATUS_PENDING;

} // end IdeReadWrite()

#ifndef UNIATA_CORE

/*++

Routine Description:
    This routine handles IDE Verify.

Arguments:
    HwDeviceExtension - HBA miniport driver's adapter data storage
    Srb - IO request packet
    `
Return Value:
    SRB status

--*/
ULONG
IdeVerify(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    UCHAR                lChannel = GET_CHANNEL(Srb);
    PHW_CHANNEL          chan = &(deviceExtension->chan[lChannel]);
    PATA_REQ             AtaReq = (PATA_REQ)(Srb->SrbExtension);
    PHW_LU_EXTENSION     LunExt;
    ULONG                ldev = GET_LDEV(Srb);
    UCHAR                statusByte;
    ULONG                startingSector;
    ULONG                sectors;
    ULONG                endSector;
    USHORT               sectorCount;
    ULONGLONG            lba;

    LunExt = &deviceExtension->lun[ldev];
    // Drive has these number sectors.
    if(!(sectors = (ULONG)(LunExt->NumOfSectors))) {
        sectors = LunExt->IdentifyData.SectorsPerTrack *
                  LunExt->IdentifyData.NumberOfHeads *
                  LunExt->IdentifyData.NumberOfCylinders;
    }

    KdPrint2((PRINT_PREFIX 
                "IdeVerify: Total sectors %#x\n",
                sectors));

    // Get starting sector number from CDB.
    startingSector = RtlUlongByteSwap((ULONG)((PCDB)Srb->Cdb)->CDB10.LBA);
    MOV_DW_SWP(sectorCount, ((PCDB)Srb->Cdb)->CDB10.TransferBlocks);

    KdPrint2((PRINT_PREFIX 
                "IdeVerify: Starting sector %#x. Number of blocks %#x\n",
                startingSector,
                sectorCount));

    endSector = startingSector + sectorCount;

    KdPrint2((PRINT_PREFIX 
                "IdeVerify: Ending sector %#x\n",
                endSector));

    if (endSector > sectors) {

        // Too big, round down.
        KdPrint2((PRINT_PREFIX 
                    "IdeVerify: Truncating request to %#x blocks\n",
                    sectors - startingSector - 1));

        sectorCount = (USHORT)(sectors - startingSector - 1);

    } else {

        // Set up sector count register. Round up to next block.
        if (sectorCount > 0xFF) {
            sectorCount = (USHORT)0xFF;
        }
    }

    // Set data buffer pointer and words left.
    AtaReq->DataBuffer = (PUSHORT)Srb->DataBuffer;
    AtaReq->WordsLeft = Srb->DataTransferLength / 2;

    // Indicate expecting an interrupt.
    InterlockedExchange(&(chan->CheckIntr),
                                  CHECK_INTR_IDLE);

    lba = UniAtaCalculateLBARegs(LunExt, startingSector);

    statusByte = AtaCommand48(deviceExtension, ldev & 0x01, GET_CHANNEL(Srb),
                 IDE_COMMAND_VERIFY, lba,
                 sectorCount,
                 0, ATA_IMMEDIATE);

    if(!(statusByte & IDE_STATUS_ERROR)) {
        // Wait for interrupt.
        return SRB_STATUS_PENDING;
    }
    return SRB_STATUS_ERROR;

} // end IdeVerify()

#endif //UNIATA_CORE

/*++

Routine Description:
    Send ATAPI packet command to device.

Arguments:
    HwDeviceExtension - HBA miniport driver's adapter data storage
    Srb - IO request packet

Return Value:

--*/
ULONG
AtapiSendCommand(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN ULONG CmdAction
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    UCHAR                lChannel = GET_CHANNEL(Srb);
    PHW_CHANNEL          chan = &(deviceExtension->chan[lChannel]);
    PATA_REQ             AtaReq = (PATA_REQ)(Srb->SrbExtension);
    ULONG                ldev = GET_LDEV(Srb);
    ULONG i;
    ULONG flags;
    UCHAR statusByte,byteCountLow,byteCountHigh;
    BOOLEAN use_dma = FALSE;
    BOOLEAN dma_reinited = FALSE;

    KdPrint2((PRINT_PREFIX "AtapiSendCommand: req state %#x\n", AtaReq->ReqState));
    if(AtaReq->ReqState < REQ_STATE_PREPARE_TO_TRANSFER)
        AtaReq->ReqState = REQ_STATE_PREPARE_TO_TRANSFER;

#ifdef UNIATA_DUMP_ATAPI
    if(CmdAction & CMD_ACTION_PREPARE) {
        UCHAR                   ScsiCommand;
        PCDB                    Cdb;
        PCHAR                   CdbData;
        PCHAR                   ModeSelectData;
        ULONG                   CdbDataLen;

        Cdb = (PCDB)(Srb->Cdb);
        ScsiCommand = Cdb->CDB6.OperationCode;
        CdbData = (PCHAR)(Srb->DataBuffer);
        CdbDataLen = Srb->DataTransferLength;

        if(CdbDataLen > 0x1000) {
            CdbDataLen = 0x1000;
        }

        KdPrint(("--\n"));
        KdPrint2(("VendorID+DeviceID/Rev %#x/%#x\n", deviceExtension->DevID, deviceExtension->RevID));
        KdPrint2(("P:T:D=%d:%d:%d\n",
                                  Srb->PathId,
                                  Srb->TargetId,
                                  Srb->Lun));
        KdPrint(("SCSI Command %2.2x\n", ScsiCommand));
        KdDump(Cdb, 16);

        if(ScsiCommand == SCSIOP_WRITE_CD) {
            KdPrint(("Write10, LBA %2.2x%2.2x%2.2x%2.2x\n",
                     Cdb->WRITE_CD.LBA[0],
                     Cdb->WRITE_CD.LBA[1],
                     Cdb->WRITE_CD.LBA[2],
                     Cdb->WRITE_CD.LBA[3]
                     ));
        } else
        if(ScsiCommand == SCSIOP_WRITE12) {
            KdPrint(("Write12, LBA %2.2x%2.2x%2.2x%2.2x\n",
                     Cdb->CDB12READWRITE.LBA[0],
                     Cdb->CDB12READWRITE.LBA[1],
                     Cdb->CDB12READWRITE.LBA[2],
                     Cdb->CDB12READWRITE.LBA[3]
                     ));
        } else
        if(ScsiCommand == SCSIOP_MODE_SELECT) {
            KdPrint(("ModeSelect 6\n"));
            PMODE_PARAMETER_HEADER ParamHdr = (PMODE_PARAMETER_HEADER)CdbData;
            ModeSelectData = CdbData+4;
            KdDump(CdbData, CdbDataLen);
        } else
        if(ScsiCommand == SCSIOP_MODE_SELECT10) {
            KdPrint(("ModeSelect 10\n"));
            PMODE_PARAMETER_HEADER ParamHdr = (PMODE_PARAMETER_HEADER)CdbData;
            ModeSelectData = CdbData+8;
            KdDump(CdbData, CdbDataLen);
        } else {
            if(Srb->SrbFlags & SRB_FLAGS_DATA_OUT) {
                KdPrint(("Send buffer to device:\n"));
                KdDump(CdbData, CdbDataLen);
            }
        }
        KdPrint(("--\n"));
    }
#endif //UNIATA_DUMP_ATAPI


    if(CmdAction == CMD_ACTION_PREPARE) {
        KdPrint2((PRINT_PREFIX "AtapiSendCommand: CMD_ACTION_PREPARE\n"));
        switch (Srb->Cdb[0]) {
        case SCSIOP_READ:
        case SCSIOP_WRITE:
        case SCSIOP_READ12:
        case SCSIOP_WRITE12:
            // all right
            break;
        default:
            KdPrint2((PRINT_PREFIX "AtapiSendCommand: SRB_STATUS_BUSY\n"));
            return SRB_STATUS_BUSY;
        }
        //
        if (!(deviceExtension->lun[ldev].DeviceFlags & DFLAGS_CHANGER_INITED) &&
            !AtaReq->OriginalSrb) {
            KdPrint2((PRINT_PREFIX "AtapiSendCommand: SRB_STATUS_BUSY (2)\n"));
            return SRB_STATUS_BUSY;
        }
    }

    if((CmdAction & CMD_ACTION_PREPARE) &&
       (AtaReq->ReqState != REQ_STATE_READY_TO_TRANSFER)) {

        KdPrint2((PRINT_PREFIX "AtapiSendCommand: prepare..., ATAPI CMD %x\n", Srb->Cdb[0]));
        // Set data buffer pointer and words left.
        AtaReq->DataBuffer = (PUSHORT)Srb->DataBuffer;
        AtaReq->WordsLeft = Srb->DataTransferLength / 2;
        AtaReq->TransferLength = Srb->DataTransferLength;
        AtaReq->Flags &= ~REQ_FLAG_DMA_OPERATION;

        // check if reorderable
        switch(Srb->Cdb[0]) {
        case SCSIOP_READ12:
        case SCSIOP_WRITE12:

            MOV_DD_SWP(AtaReq->bcount, ((PCDB)Srb->Cdb)->CDB12READWRITE.NumOfBlocks);
            goto GetLba;

        case SCSIOP_READ:
        case SCSIOP_WRITE:

            MOV_SWP_DW2DD(AtaReq->bcount, ((PCDB)Srb->Cdb)->CDB10.TransferBlocks);
GetLba:
            MOV_DD_SWP(AtaReq->lba, ((PCDB)Srb->Cdb)->CDB10.LBA);

            AtaReq->Flags |= REQ_FLAG_REORDERABLE_CMD;
            AtaReq->Flags &= ~REQ_FLAG_RW_MASK;
            AtaReq->Flags |= (Srb->Cdb[0] == SCSIOP_WRITE || Srb->Cdb[0] == SCSIOP_WRITE12) ?
                              REQ_FLAG_WRITE : REQ_FLAG_READ;
            break;
        }

        // check if DMA read/write
        if(Srb->Cdb[0] == SCSIOP_REQUEST_SENSE) {
            KdPrint2((PRINT_PREFIX "AtapiSendCommand: SCSIOP_REQUEST_SENSE, no DMA setup\n"));
        } else
        if(AtaReq->TransferLength) {
            // try use DMA
            switch(Srb->Cdb[0]) {
            case SCSIOP_WRITE:
            case SCSIOP_WRITE12:
                if(chan->ChannelCtrlFlags & CTRFLAGS_DMA_RO)
                    break;
                /* FALLTHROUGH */
            case SCSIOP_READ:
            case SCSIOP_READ12:

                if(deviceExtension->opt_AtapiDmaReadWrite) {
call_dma_setup:
                    if(AtapiDmaSetup(HwDeviceExtension, ldev & 1, lChannel, Srb,
                                  (PUCHAR)(AtaReq->DataBuffer),
                                  Srb->DataTransferLength
                                  /*((Srb->DataTransferLength + DEV_BSIZE-1) & ~(DEV_BSIZE-1))*/
                                  )) {
                        KdPrint2((PRINT_PREFIX "AtapiSendCommand: use dma\n"));
                        use_dma = TRUE;
                    }
                }
                break;
            case SCSIOP_READ_CD:
                if(deviceExtension->opt_AtapiDmaRawRead)
                    goto call_dma_setup;
                break;
            default:

                if(deviceExtension->opt_AtapiDmaControlCmd) {
                    if(Srb->SrbFlags & SRB_FLAGS_DATA_IN) {
                        // read operation
                        use_dma = TRUE;
                    } else {
                        // write operation
                        if(chan->ChannelCtrlFlags & CTRFLAGS_DMA_RO) {
                            KdPrint2((PRINT_PREFIX "dma RO\n"));
                            use_dma = FALSE;
                        } else {
                            use_dma = TRUE;
                        }
                    }
                }
                break;
            }
            // try setup DMA
            if(use_dma) {
                if(!AtapiDmaSetup(HwDeviceExtension, ldev & 1, lChannel, Srb,
                              (PUCHAR)(AtaReq->DataBuffer),
                              Srb->DataTransferLength)) {
                    KdPrint2((PRINT_PREFIX "AtapiSendCommand: no dma\n"));
                    use_dma = FALSE;
                } else {
                    KdPrint2((PRINT_PREFIX "AtapiSendCommand: use dma\n"));
                }
            }
        } else {
            KdPrint2((PRINT_PREFIX "AtapiSendCommand: zero transfer, no DMA setup\n"));
        }
    
    } else {
        if(AtaReq->Flags & REQ_FLAG_DMA_OPERATION) {
            // if this is queued request, reinit DMA and check
            // if DMA mode is still available
            KdPrint2((PRINT_PREFIX "AtapiSendCommand: AtapiDmaReinit()  (1)\n"));
            AtapiDmaReinit(deviceExtension, ldev, AtaReq);
            if (/*EnableDma &&*/
                (deviceExtension->lun[ldev].TransferMode >= ATA_DMA)) {
                KdPrint2((PRINT_PREFIX "AtapiSendCommand: use dma (2)\n"));
                use_dma = TRUE;
            } else {
                AtaReq->Flags &= ~REQ_FLAG_DMA_OPERATION;
                KdPrint2((PRINT_PREFIX "AtapiSendCommand: no dma (2)\n"));
                use_dma = FALSE;
            }
            dma_reinited = TRUE;
        }
    }

    if(!(CmdAction & CMD_ACTION_EXEC)) {
        KdPrint2((PRINT_PREFIX "AtapiSendCommand: !CMD_ACTION_EXEC => SRB_STATUS_PENDING\n"));
        return SRB_STATUS_PENDING;
    }
    KdPrint2((PRINT_PREFIX "AtapiSendCommand: use_dma=%d\n", use_dma));
    if(AtaReq->Flags & REQ_FLAG_DMA_OPERATION) {
        KdPrint2((PRINT_PREFIX "  REQ_FLAG_DMA_OPERATION\n"));
    }

    if(Srb->Cdb[0] == SCSIOP_REQUEST_SENSE) {
        KdPrint2((PRINT_PREFIX "AtapiSendCommand: SCSIOP_REQUEST_SENSE -> no dma setup (2)\n"));
        use_dma = FALSE;
        AtaReq->Flags &= ~REQ_FLAG_DMA_OPERATION;
        //AtapiDmaReinit(deviceExtension, ldev, AtaReq);
    } if(AtaReq->TransferLength) {
        if(!dma_reinited) {
            KdPrint2((PRINT_PREFIX "AtapiSendCommand: AtapiDmaReinit()\n"));
            AtapiDmaReinit(deviceExtension, ldev, AtaReq);
            if (/*EnableDma &&*/
                (deviceExtension->lun[ldev].TransferMode >= ATA_DMA)) {
                use_dma = TRUE;
            } else {
                AtaReq->Flags &= ~REQ_FLAG_DMA_OPERATION;
                use_dma = FALSE;
            }
        }
    } else {
        KdPrint2((PRINT_PREFIX "AtapiSendCommand: zero transfer\n"));
        use_dma = FALSE;
        AtaReq->Flags &= ~REQ_FLAG_DMA_OPERATION;
        if(!deviceExtension->opt_AtapiDmaZeroTransfer) {
            KdPrint2((PRINT_PREFIX "AtapiSendCommand: AtapiDmaReinit() to PIO\n"));
            AtapiDmaReinit(deviceExtension, ldev, AtaReq);
        }
    }
    KdPrint2((PRINT_PREFIX "AtapiSendCommand: use_dma=%d\n", use_dma));
    if(AtaReq->Flags & REQ_FLAG_DMA_OPERATION) {
        KdPrint2((PRINT_PREFIX "  REQ_FLAG_DMA_OPERATION\n"));
    }
    
    KdPrint2((PRINT_PREFIX "AtapiSendCommand: CMD_ACTION_EXEC\n"));

#ifndef UNIATA_CORE
    // We need to know how many platters our atapi cd-rom device might have.
    // Before anyone tries to send a srb to our target for the first time,
    // we must "secretly" send down a separate mechanism status srb in order to
    // initialize our device extension changer data.  That's how we know how
    // many platters our target has.

    if (!(deviceExtension->lun[ldev].DeviceFlags & DFLAGS_CHANGER_INITED) &&
        !AtaReq->OriginalSrb) {

        ULONG srbStatus;

        KdPrint2((PRINT_PREFIX "AtapiSendCommand: BuildMechanismStatusSrb()\n"));
        // Set this flag now. If the device hangs on the mech. status
        // command, we will not have the chance to set it.
        deviceExtension->lun[ldev].DeviceFlags |= DFLAGS_CHANGER_INITED;

        chan->MechStatusRetryCount = 3;
        AtaReq->OriginalSrb = Srb;
        AtaReq->Srb = BuildMechanismStatusSrb (
                                        HwDeviceExtension,
                                        Srb);

        KdPrint2((PRINT_PREFIX "AtapiSendCommand: AtapiSendCommand recursive\n"));
        srbStatus = AtapiSendCommand(HwDeviceExtension, AtaReq->Srb, CMD_ACTION_ALL);
        if (srbStatus == SRB_STATUS_PENDING) {
            KdPrint2((PRINT_PREFIX "AtapiSendCommand: SRB_STATUS_PENDING (2)\n"));
            return srbStatus;
        } else {
            AtaReq->Srb = AtaReq->OriginalSrb;
            AtaReq->OriginalSrb = NULL;
            KdPrint2((PRINT_PREFIX "AtapiSendCommand: AtapiHwInitializeChanger()\n"));
            AtapiHwInitializeChanger (HwDeviceExtension, Srb,
                                      (PMECHANICAL_STATUS_INFORMATION_HEADER) NULL);
            // fall out
        }
    }
#endif //UNIATA_CORE

    KdPrint2((PRINT_PREFIX "AtapiSendCommand: Command %#x to TargetId %d lun %d\n",
               Srb->Cdb[0], Srb->TargetId, Srb->Lun));
    
    // Make sure command is to ATAPI device.
    flags = deviceExtension->lun[ldev].DeviceFlags;
    if (flags & (DFLAGS_SANYO_ATAPI_CHANGER | DFLAGS_ATAPI_CHANGER)) {
        if ((Srb->Lun) > (deviceExtension->lun[ldev].DiscsPresent - 1)) {

            // Indicate no device found at this address.
            AtaReq->ReqState = REQ_STATE_TRANSFER_COMPLETE;
            return SRB_STATUS_SELECTION_TIMEOUT;
        }
    } else if (Srb->Lun > 0) {
        AtaReq->ReqState = REQ_STATE_TRANSFER_COMPLETE;
        return SRB_STATUS_SELECTION_TIMEOUT;
    }

    if (!(flags & DFLAGS_ATAPI_DEVICE)) {
        AtaReq->ReqState = REQ_STATE_TRANSFER_COMPLETE;
        return SRB_STATUS_SELECTION_TIMEOUT;
    }

    // Select device 0 or 1.
    SelectDrive(chan, ldev & 0x1);

    // Verify that controller is ready for next command.
    GetStatus(chan, statusByte);
    KdPrint2((PRINT_PREFIX "AtapiSendCommand: Entered with status %#x\n", statusByte));

    if (statusByte == 0xff) {
        KdPrint2((PRINT_PREFIX "AtapiSendCommand: bad status 0xff on entry\n"));
        goto make_reset;
    }
    if (statusByte & IDE_STATUS_BUSY) {
        if(statusByte & IDE_STATUS_DSC) {
            KdPrint2((PRINT_PREFIX "AtapiSendCommand: DSC on entry (%#x), try exec\n", statusByte));
        }
        KdPrint2((PRINT_PREFIX "AtapiSendCommand: Device busy (%#x)\n", statusByte));
        AtaReq->ReqState = REQ_STATE_TRANSFER_COMPLETE;
        return SRB_STATUS_BUSY;
    }
    if (statusByte & IDE_STATUS_ERROR) {
        if (Srb->Cdb[0] != SCSIOP_REQUEST_SENSE) {

            KdPrint2((PRINT_PREFIX "AtapiSendCommand: Error on entry: (%#x)\n", statusByte));
            // Read the error reg. to clear it and fail this request.
            AtaReq->ReqState = REQ_STATE_TRANSFER_COMPLETE;
            return MapError(deviceExtension, Srb);
        } else {
            KdPrint2((PRINT_PREFIX "  continue with SCSIOP_REQUEST_SENSE\n", statusByte));
        }
    }
    // If a tape drive doesn't have DSC set and the last command is restrictive, don't send
    // the next command. See discussion of Restrictive Delayed Process commands in QIC-157.
    if ((!(statusByte & IDE_STATUS_DSC)) &&
          (flags & (DFLAGS_TAPE_DEVICE | DFLAGS_ATAPI_DEVICE)) && chan->RDP) {

        AtapiStallExecution(200);
        KdPrint2((PRINT_PREFIX "AtapiSendCommand: DSC not set. %#x => SRB_STATUS_PENDING\n",statusByte));
        AtaReq->ReqState = REQ_STATE_QUEUED;
        return SRB_STATUS_PENDING;
    }

    if (IS_RDP(Srb->Cdb[0])) {
        chan->RDP = TRUE;
        KdPrint2((PRINT_PREFIX "AtapiSendCommand: %#x mapped as DSC restrictive\n", Srb->Cdb[0]));
    } else {
        chan->RDP = FALSE;
    }
    if (statusByte & IDE_STATUS_DRQ) {

        KdPrint2((PRINT_PREFIX "AtapiSendCommand: Entered with status (%#x). Attempting to recover.\n",
                    statusByte));
        // Try to drain the data that one preliminary device thinks that it has
        // to transfer. Hopefully this random assertion of DRQ will not be present
        // in production devices.
        for (i = 0; i < 0x10000; i++) {
            GetStatus(chan, statusByte);
            if (statusByte & IDE_STATUS_DRQ) {
                AtapiReadPort2(chan, IDX_IO1_i_Data);
            } else {
                break;
            }
        }

        if (i == 0x10000) {
make_reset:
            KdPrint2((PRINT_PREFIX "AtapiSendCommand: DRQ still asserted.Status (%#x)\n", statusByte));

            AtapiDisableInterrupts(deviceExtension, lChannel);

            AtapiSoftReset(chan, ldev & 1);

            KdPrint2((PRINT_PREFIX "AtapiSendCommand: Issued soft reset to Atapi device. \n"));
            // Re-initialize Atapi device.
            IssueIdentify(HwDeviceExtension, ldev & 1, GET_CHANNEL(Srb),
                          IDE_COMMAND_ATAPI_IDENTIFY, FALSE);
            // Inform the port driver that the bus has been reset.
            ScsiPortNotification(ResetDetected, HwDeviceExtension, 0);
            // Clean up device extension fields that AtapiStartIo won't.
            chan->ExpectingInterrupt = FALSE;
            chan->RDP = FALSE;
            InterlockedExchange(&(deviceExtension->chan[GET_CHANNEL(Srb)].CheckIntr),
                                          CHECK_INTR_IDLE);

            AtapiEnableInterrupts(deviceExtension, lChannel);

            AtaReq->ReqState = REQ_STATE_TRANSFER_COMPLETE;
            return SRB_STATUS_BUS_RESET;

        }
    }

    if (flags & (DFLAGS_SANYO_ATAPI_CHANGER | DFLAGS_ATAPI_CHANGER)) {
        // As the cdrom driver sets the LUN field in the cdb, it must be removed.
        Srb->Cdb[1] &= ~0xE0;
        if ((Srb->Cdb[0] == SCSIOP_TEST_UNIT_READY) && (flags & DFLAGS_SANYO_ATAPI_CHANGER)) {
            // Torisan changer. TUR's are overloaded to be platter switches.
            Srb->Cdb[7] = Srb->Lun;
        }
    }

    // SETUP DMA !!!!!

    if(use_dma) {
        chan->ChannelCtrlFlags |= CTRFLAGS_DMA_OPERATION;
    } else {
        chan->ChannelCtrlFlags &= ~CTRFLAGS_DMA_OPERATION;
    }

    statusByte = WaitOnBusy(chan);
    KdPrint2((PRINT_PREFIX "AtapiSendCommand: Entry Status (%#x)\n",
               statusByte));

    AtapiWritePort1(chan, IDX_IO1_o_Feature,
                            use_dma ? ATA_F_DMA : 0);

    // Write transfer byte count to registers.
    byteCountLow = (UCHAR)(Srb->DataTransferLength & 0xFF);
    byteCountHigh = (UCHAR)(Srb->DataTransferLength >> 8);

    if (Srb->DataTransferLength >= 0x10000) {
        byteCountLow = byteCountHigh = 0xFF;
    }

    AtapiWritePort1(chan, IDX_ATAPI_IO1_o_ByteCountLow, byteCountLow);
    AtapiWritePort1(chan, IDX_ATAPI_IO1_o_ByteCountHigh, byteCountHigh);

    if (flags & DFLAGS_INT_DRQ) {

        // This device interrupts when ready to receive the packet.

        KdPrint2((PRINT_PREFIX "AtapiSendCommand: Wait for int. to send packet. Status (%#x)\n",
                   statusByte));

        chan->ExpectingInterrupt = TRUE;
        AtaReq->ReqState = REQ_STATE_ATAPI_EXPECTING_CMD_INTR;
        InterlockedExchange(&(chan->CheckIntr),
                                      CHECK_INTR_IDLE);

        // Write ATAPI packet command.
        AtapiWritePort1(chan, IDX_IO1_o_Command, IDE_COMMAND_ATAPI_PACKET);

        KdPrint2((PRINT_PREFIX "AtapiSendCommand: return SRB_STATUS_PENDING\n"));
        return SRB_STATUS_PENDING;

    } else {

        // This device quickly sets DRQ when ready to receive the packet.

        KdPrint2((PRINT_PREFIX "AtapiSendCommand: Poll for int. to send packet. Status (%#x)\n",
                   statusByte));

        chan->ExpectingInterrupt = TRUE;
        AtaReq->ReqState = REQ_STATE_ATAPI_DO_NOTHING_INTR;
        InterlockedExchange(&(chan->CheckIntr),
                                      CHECK_INTR_IDLE);

        AtapiDisableInterrupts(deviceExtension, lChannel);

        // Write ATAPI packet command.
        AtapiWritePort1(chan, IDX_IO1_o_Command, IDE_COMMAND_ATAPI_PACKET);

        // Wait for DRQ.
        WaitOnBusy(chan);
        statusByte = WaitForDrq(chan);

        // Need to read status register and clear interrupt (if any)
        GetBaseStatus(chan, statusByte);

        if (!(statusByte & IDE_STATUS_DRQ)) {

            AtapiEnableInterrupts(deviceExtension, lChannel);
            KdPrint2((PRINT_PREFIX "AtapiSendCommand: DRQ never asserted (%#x)\n", statusByte));
            AtaReq->ReqState = REQ_STATE_TRANSFER_COMPLETE;
            return SRB_STATUS_ERROR;
        }
    }

    GetStatus(chan, statusByte);
    KdPrint2((PRINT_PREFIX "AtapiSendCommand: status (%#x)\n", statusByte));

    // Send CDB to device.
    statusByte = WaitOnBaseBusy(chan);

    // Indicate expecting an interrupt and wait for it.
    chan->ExpectingInterrupt = TRUE;
    InterlockedExchange(&(chan->CheckIntr),
                                  CHECK_INTR_IDLE);
    AtaReq->ReqState = REQ_STATE_ATAPI_EXPECTING_DATA_INTR;

    GetBaseStatus(chan, statusByte);

    AtapiEnableInterrupts(deviceExtension, lChannel);

    WriteBuffer(chan,
                (PUSHORT)Srb->Cdb,
                6,
                0);

    if(chan->ChannelCtrlFlags & CTRFLAGS_DMA_OPERATION) {
        AtapiDmaStart(HwDeviceExtension, ldev & 1, lChannel, Srb);
    }

    KdPrint2((PRINT_PREFIX "AtapiSendCommand: ExpectingInterrupt (%#x)\n", chan->ExpectingInterrupt));

    KdPrint2((PRINT_PREFIX "AtapiSendCommand: return SRB_STATUS_PENDING (3)\n"));
    return SRB_STATUS_PENDING;

} // end AtapiSendCommand()


#ifndef UNIATA_CORE

/*++

Routine Description:
    Program ATA registers for IDE disk transfer.

Arguments:
    HwDeviceExtension - ATAPI driver storage.
    Srb - System request block.

Return Value:
    SRB status (pending if all goes well).

--*/

#ifdef _DEBUG
ULONG check_point = 0;
#define SetCheckPoint(cp)  { check_point = (cp) ; }
#else
#define SetCheckPoint(cp)
#endif

ULONG
IdeSendCommand(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN ULONG CmdAction
    )
{
    SetCheckPoint(1);
    KdPrint2((PRINT_PREFIX "** Ide: Command: entryway\n"));
    SetCheckPoint(2);

    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    SetCheckPoint(3);
    UCHAR                lChannel;
    PHW_CHANNEL          chan;
    PCDB cdb;

    SetCheckPoint(4);

    UCHAR statusByte,errorByte;
    ULONG status;
    ULONG i;
    PMODE_PARAMETER_HEADER   modeData;
    ULONG ldev;
    PATA_REQ AtaReq;
    SetCheckPoint(5);
    //ULONG __ebp__ = 0;

    SetCheckPoint(0x20);
    KdPrint2((PRINT_PREFIX "** Ide: Command:\n\n"));
/*    __asm {
        mov eax,ebp
        mov __ebp__, eax
    }*/
    /*KdPrint2((PRINT_PREFIX "** Ide: Command EBP %#x, pCdb %#x, cmd %#x\n",
        __ebp__, &(Srb->Cdb[0]), Srb->Cdb[0]));
    KdPrint2((PRINT_PREFIX "** Ide: Command %s\n",
        (CmdAction == CMD_ACTION_PREPARE) ? "Prep " : ""));
    KdPrint2((PRINT_PREFIX "** Ide: Command Srb %#x\n",
        Srb));
    KdPrint2((PRINT_PREFIX "** Ide: Command SrbExt %#x\n",
        Srb->SrbExtension));
    KdPrint2((PRINT_PREFIX "** Ide: Command to device %d\n",
        Srb->TargetId));*/

    SetCheckPoint(0x30);
    AtaReq = (PATA_REQ)(Srb->SrbExtension);

    KdPrint2((PRINT_PREFIX "** Ide: Command &AtaReq %#x\n",
        &AtaReq));
    KdPrint2((PRINT_PREFIX "** Ide: Command AtaReq %#x\n",
        AtaReq));
    KdPrint2((PRINT_PREFIX "** --- **\n"));

    lChannel = GET_CHANNEL(Srb);
    chan = &(deviceExtension->chan[lChannel]);
    ldev = GET_LDEV(Srb);

    SetCheckPoint(0x40);
    if(AtaReq->ReqState < REQ_STATE_PREPARE_TO_TRANSFER)
        AtaReq->ReqState = REQ_STATE_PREPARE_TO_TRANSFER;

    if(CmdAction == CMD_ACTION_PREPARE) {
        switch (Srb->Cdb[0]) {
        //case SCSIOP_INQUIRY: // now it requires device access
        case SCSIOP_READ_CAPACITY:
        case SCSIOP_READ:
        case SCSIOP_WRITE:
        case SCSIOP_REQUEST_SENSE:
            // all right
            KdPrint2((PRINT_PREFIX "** Ide: Command continue prep\n"));
            SetCheckPoint(50);
            break;
        default:
            SetCheckPoint(0);
            KdPrint2((PRINT_PREFIX "** Ide: Command break prep\n"));
            return SRB_STATUS_BUSY;
        }
    }

    SetCheckPoint(0x100 | Srb->Cdb[0]);
    switch (Srb->Cdb[0]) {
    case SCSIOP_INQUIRY:

        KdPrint2((PRINT_PREFIX 
                   "IdeSendCommand: SCSIOP_INQUIRY PATH:LUN:TID = %#x:%#x:%#x\n",
                   Srb->PathId, Srb->Lun, Srb->TargetId));
        // Filter out all TIDs but 0 and 1 since this is an IDE interface
        // which support up to two devices.
        if ((Srb->Lun != 0) ||
            (Srb->PathId >= deviceExtension->NumberChannels) ||
            (Srb->TargetId > 2) /*||
            (!deviceExtension->lun[ldev].DeviceFlags & DFLAGS_DEVICE_PRESENT)*/) {

            KdPrint2((PRINT_PREFIX 
                       "IdeSendCommand: SCSIOP_INQUIRY rejected\n"));
            // Indicate no device found at this address.
            status = SRB_STATUS_SELECTION_TIMEOUT;
            break;

        } else {

            KdPrint2((PRINT_PREFIX 
                       "IdeSendCommand: SCSIOP_INQUIRY ok\n"));
            PINQUIRYDATA    inquiryData  = (PINQUIRYDATA)(Srb->DataBuffer);
            PIDENTIFY_DATA2 identifyData = &(deviceExtension->lun[ldev].IdentifyData);

            if (!(deviceExtension->lun[ldev].DeviceFlags & DFLAGS_DEVICE_PRESENT)) {
                
                if(!CheckDevice(HwDeviceExtension, lChannel, ldev & 1, FALSE)) {
                    KdPrint2((PRINT_PREFIX 
                               "IdeSendCommand: SCSIOP_INQUIRY rejected (2)\n"));
                    // Indicate no device found at this address.
                    status = SRB_STATUS_SELECTION_TIMEOUT;
                    break;
                }
            } else {
                if(!UniataAnybodyHome(HwDeviceExtension, lChannel, ldev & 1)) {
                    KdPrint2((PRINT_PREFIX 
                               "IdeSendCommand: SCSIOP_INQUIRY device have gone\n"));
                    // Indicate no device found at this address.
                    deviceExtension->lun[ldev].DeviceFlags &= ~DFLAGS_DEVICE_PRESENT;
                    status = SRB_STATUS_SELECTION_TIMEOUT;
                    break;
                }
            }

            // Zero INQUIRY data structure.
            RtlZeroMemory((PCHAR)(Srb->DataBuffer), Srb->DataTransferLength);

            // Standard IDE interface only supports disks.
            inquiryData->DeviceType = DIRECT_ACCESS_DEVICE;

            // Set the removable bit, if applicable.
            if (deviceExtension->lun[ldev].DeviceFlags & DFLAGS_REMOVABLE_DRIVE) {
                KdPrint2((PRINT_PREFIX 
                           "RemovableMedia\n"));
                inquiryData->RemovableMedia = 1;
            }
            // Set the Relative Addressing (LBA) bit, if applicable.
            if (deviceExtension->lun[ldev].DeviceFlags & DFLAGS_LBA_ENABLED) {
                inquiryData->RelativeAddressing = 1;
                KdPrint2((PRINT_PREFIX 
                           "RelativeAddressing\n"));
            }
            // Set the CommandQueue bit
            inquiryData->CommandQueue = 1;

            // Fill in vendor identification fields.
            for (i = 0; i < 24; i += 2) {
                inquiryData->VendorId[i] = RtlUshortByteSwap(((PUCHAR)identifyData->ModelNumber)[i]);
            }
/*
            // Initialize unused portion of product id.
            for (i = 0; i < 4; i++) {
                inquiryData->ProductId[12+i] = ' ';
            }
*/
            // Move firmware revision from IDENTIFY data to
            // product revision in INQUIRY data.
            for (i = 0; i < 4; i += 2) {
                inquiryData->ProductRevisionLevel[i] = RtlUshortByteSwap(((PUCHAR)identifyData->FirmwareRevision)[i]);
            }

            status = SRB_STATUS_SUCCESS;
        }

        break;

    case SCSIOP_MODE_SENSE:

        KdPrint2((PRINT_PREFIX 
                   "IdeSendCommand: SCSIOP_MODE_SENSE PATH:LUN:TID = %#x:%#x:%#x\n",
                   Srb->PathId, Srb->Lun, Srb->TargetId));
        // This is used to determine if the media is write-protected.
        // Since IDE does not support mode sense then we will modify just the portion we need
        // so the higher level driver can determine if media is protected.
        if (deviceExtension->lun[ldev].DeviceFlags & DFLAGS_MEDIA_STATUS_ENABLED) {

            SelectDrive(chan, ldev & 0x1);
            AtapiWritePort1(chan, IDX_IO1_o_Command,IDE_COMMAND_GET_MEDIA_STATUS);
            statusByte = WaitOnBusy(chan);

            if (!(statusByte & IDE_STATUS_ERROR)){

                // no error occured return success, media is not protected
                chan->ExpectingInterrupt = FALSE;
                InterlockedExchange(&(chan->CheckIntr),
                                              CHECK_INTR_IDLE);
                status = SRB_STATUS_SUCCESS;

            } else {

                // error occured, handle it locally, clear interrupt
                errorByte = AtapiReadPort1(chan, IDX_IO1_i_Error);

                GetBaseStatus(chan, statusByte);
                chan->ExpectingInterrupt = FALSE;
                InterlockedExchange(&(chan->CheckIntr),
                                              CHECK_INTR_IDLE);
                status = SRB_STATUS_SUCCESS;

                if (errorByte & IDE_ERROR_DATA_ERROR) {

                    //media is write-protected, set bit in mode sense buffer
                    modeData = (PMODE_PARAMETER_HEADER)Srb->DataBuffer;

                    Srb->DataTransferLength = sizeof(MODE_PARAMETER_HEADER);
                    modeData->DeviceSpecificParameter |= MODE_DSP_WRITE_PROTECT;
                }
            }
            status = SRB_STATUS_SUCCESS;
        } else {
            status = SRB_STATUS_INVALID_REQUEST;
        }
        break;

    case SCSIOP_TEST_UNIT_READY:

        KdPrint2((PRINT_PREFIX 
                   "IdeSendCommand: SCSIOP_TEST_UNIT_READY PATH:LUN:TID = %#x:%#x:%#x\n",
                   Srb->PathId, Srb->Lun, Srb->TargetId));
        if (deviceExtension->lun[ldev].DeviceFlags & DFLAGS_MEDIA_STATUS_ENABLED) {

            // Select device 0 or 1.
            SelectDrive(chan, ldev & 0x1);
            AtapiWritePort1(chan, IDX_IO1_o_Command,IDE_COMMAND_GET_MEDIA_STATUS);

            // Wait for busy. If media has not changed, return success
            statusByte = WaitOnBusy(chan);

            if (!(statusByte & IDE_STATUS_ERROR)){
                chan->ExpectingInterrupt = FALSE;
                InterlockedExchange(&(chan->CheckIntr),
                                              CHECK_INTR_IDLE);
                status = SRB_STATUS_SUCCESS;
            } else {
                errorByte = AtapiReadPort1(chan, IDX_IO1_i_Error);
                if (errorByte == IDE_ERROR_DATA_ERROR){

                    // Special case: If current media is write-protected,
                    // the 0xDA command will always fail since the write-protect bit
                    // is sticky,so we can ignore this error
                    GetBaseStatus(chan, statusByte);
                    chan->ExpectingInterrupt = FALSE;
                    InterlockedExchange(&(chan->CheckIntr),
                                                  CHECK_INTR_IDLE);
                    status = SRB_STATUS_SUCCESS;

                } else {

                    // Request sense buffer to be build
                    chan->ExpectingInterrupt = TRUE;
                    InterlockedExchange(&(chan->CheckIntr),
                                                  CHECK_INTR_IDLE);
                    status = SRB_STATUS_PENDING;
               }
            }
        } else {
            status = SRB_STATUS_SUCCESS;
        }

        break;

    case SCSIOP_READ_CAPACITY:

        KdPrint2((PRINT_PREFIX 
                   "** IdeSendCommand: SCSIOP_READ_CAPACITY PATH:LUN:TID = %#x:%#x:%#x\n",
                   Srb->PathId, Srb->Lun, Srb->TargetId));
        // Claim 512 byte blocks (big-endian).
        //((PREAD_CAPACITY_DATA)Srb->DataBuffer)->BytesPerBlock = 0x20000;
        i = DEV_BSIZE;
        MOV_DD_SWP( ((PREAD_CAPACITY_DATA)Srb->DataBuffer)->BytesPerBlock, i );

        // Calculate last sector.
        if(!(i = (ULONG)deviceExtension->lun[ldev].NumOfSectors)) {
            i = deviceExtension->lun[ldev].IdentifyData.SectorsPerTrack *
                deviceExtension->lun[ldev].IdentifyData.NumberOfHeads *
                deviceExtension->lun[ldev].IdentifyData.NumberOfCylinders;
        }
        i--;

        //((PREAD_CAPACITY_DATA)Srb->DataBuffer)->LogicalBlockAddress =
        //    (((PUCHAR)&i)[0] << 24) |  (((PUCHAR)&i)[1] << 16) |
        //    (((PUCHAR)&i)[2] << 8) | ((PUCHAR)&i)[3];

        MOV_DD_SWP( ((PREAD_CAPACITY_DATA)Srb->DataBuffer)->LogicalBlockAddress, i );

        KdPrint2((PRINT_PREFIX 
                   "** IDE disk %#x - #sectors %#x, #heads %#x, #cylinders %#x\n",
                   Srb->TargetId,
                   deviceExtension->lun[ldev].IdentifyData.SectorsPerTrack,
                   deviceExtension->lun[ldev].IdentifyData.NumberOfHeads,
                   deviceExtension->lun[ldev].IdentifyData.NumberOfCylinders));


        status = SRB_STATUS_SUCCESS;
        break;

    case SCSIOP_VERIFY:

        KdPrint2((PRINT_PREFIX 
                   "IdeSendCommand: SCSIOP_VERIFY PATH:LUN:TID = %#x:%#x:%#x\n",
                   Srb->PathId, Srb->Lun, Srb->TargetId));
        status = IdeVerify(HwDeviceExtension,Srb);

        break;

    case SCSIOP_READ:
    case SCSIOP_WRITE:

        KdPrint2((PRINT_PREFIX 
                   "IdeSendCommand: SCSIOP_READ/SCSIOP_WRITE PATH:LUN:TID = %#x:%#x:%#x\n",
                   Srb->PathId, Srb->Lun, Srb->TargetId));
        AtaReq->Flags &= ~REQ_FLAG_RW_MASK;
        AtaReq->Flags |= (Srb->Cdb[0] == SCSIOP_WRITE) ? REQ_FLAG_WRITE : REQ_FLAG_READ;
        status = IdeReadWrite(HwDeviceExtension,
                              Srb, CmdAction);
        break;

    case SCSIOP_START_STOP_UNIT:

        KdPrint2((PRINT_PREFIX 
                   "IdeSendCommand: SCSIOP_START_STOP_UNIT PATH:LUN:TID = %#x:%#x:%#x\n",
                   Srb->PathId, Srb->Lun, Srb->TargetId));
        //Determine what type of operation we should perform
        cdb = (PCDB)Srb->Cdb;

        if (cdb->START_STOP.LoadEject == 1){

            statusByte = WaitOnBaseBusy(chan);
            // Eject media,
            // first select device 0 or 1.
            SelectDrive(chan, ldev & 0x1);
            AtapiWritePort1(chan, IDX_IO1_o_Command,IDE_COMMAND_MEDIA_EJECT);
        }
        status = SRB_STATUS_SUCCESS;
        break;

    case SCSIOP_MEDIUM_REMOVAL:

       cdb = (PCDB)Srb->Cdb;

       statusByte = WaitOnBaseBusy(chan);

       SelectDrive(chan, ldev & 0x1);
       if (cdb->MEDIA_REMOVAL.Prevent == TRUE) {
           AtapiWritePort1(chan, IDX_IO1_o_Command,IDE_COMMAND_DOOR_LOCK);
       } else {
           AtapiWritePort1(chan, IDX_IO1_o_Command,IDE_COMMAND_DOOR_UNLOCK);
       }
       status = SRB_STATUS_SUCCESS;
       break;

    // Note: I don't implement this, because NTFS driver too often issues this command
    // It causes awful performance degrade. However, if somebody wants, I will implement
    // SCSIOP_FLUSH_BUFFER/SCSIOP_SYNCHRONIZE_CACHE optionally.

#if 0
    case SCSIOP_FLUSH_BUFFER:
    case SCSIOP_SYNCHRONIZE_CACHE:

        SelectDrive(chan, ldev & 0x1);
        AtapiWritePort1(chan, IDX_IO1_o_Command,IDE_COMMAND_FLUSH_CACHE);
        status = SRB_STATUS_SUCCESS;
//        status = SRB_STATUS_PENDING;
        statusByte = WaitOnBusy(chan);
        break;
#endif

    case SCSIOP_REQUEST_SENSE:
        // this function makes sense buffers to report the results
        // of the original GET_MEDIA_STATUS command

        KdPrint2((PRINT_PREFIX 
                   "IdeSendCommand: SCSIOP_REQUEST_SENSE PATH:LUN:TID = %#x:%#x:%#x\n",
                   Srb->PathId, Srb->Lun, Srb->TargetId));
        if (deviceExtension->lun[ldev].DeviceFlags & DFLAGS_MEDIA_STATUS_ENABLED) {
            status = IdeBuildSenseBuffer(HwDeviceExtension,Srb);
            break;
        }
        status = SRB_STATUS_INVALID_REQUEST;
        break;

    // ATA_PASSTHORUGH
    case SCSIOP_ATA_PASSTHROUGH:
    {
        PIDEREGS_EX regs;
        BOOLEAN use_dma = FALSE;
        ULONG to_lim;
        
        regs = (PIDEREGS_EX) &(Srb->Cdb[2]);

        lChannel = Srb->TargetId >> 1;

        regs->bDriveHeadReg &= 0x0f;
        regs->bDriveHeadReg |= (UCHAR) (((Srb->TargetId & 0x1) << 4) | 0xA0);

        if((regs->bReserved & 1) == 0) {      // execute ATA command

            KdPrint2((PRINT_PREFIX 
                       "IdeSendCommand: SCSIOP_START_STOP_UNIT PATH:LUN:TID = %#x:%#x:%#x\n",
                       Srb->PathId, Srb->Lun, Srb->TargetId));


            AtapiDisableInterrupts(deviceExtension, lChannel);

            if(AtaCommandFlags[regs->bCommandReg] & ATA_CMD_FLAG_DMA) {
                if((chan->lun[Srb->TargetId & 0x1]->LimitedTransferMode >= ATA_DMA)) {
                    use_dma = TRUE;
                    // this will set REQ_FLAG_DMA_OPERATION in AtaReq->Flags on success
                    if(!AtapiDmaSetup(HwDeviceExtension, Srb->TargetId & 0x1, lChannel, Srb,
                                  (PUCHAR)(Srb->DataBuffer),
                                  ((Srb->DataTransferLength + DEV_BSIZE-1) & ~(DEV_BSIZE-1)))) {
                        use_dma = FALSE;
                    }
                }
            }

            AtapiWritePort1(chan, IDX_IO1_o_DriveSelect,  regs->bDriveHeadReg);
            AtapiStallExecution(10);

            if((regs->bReserved & ATA_FLAGS_48BIT_COMMAND) == 0) {      // execute ATA command
                AtapiWritePort1(chan, IDX_IO1_o_Feature,      regs->bFeaturesReg);
                AtapiWritePort1(chan, IDX_IO1_o_BlockCount,   regs->bSectorCountReg);
                AtapiWritePort1(chan, IDX_IO1_o_BlockNumber,  regs->bSectorNumberReg);
                AtapiWritePort1(chan, IDX_IO1_o_CylinderLow,  regs->bCylLowReg);
                AtapiWritePort1(chan, IDX_IO1_o_CylinderHigh, regs->bCylHighReg);
            } else {
                AtapiWritePort1(chan, IDX_IO1_o_Feature,      regs->bFeaturesRegH);
                AtapiWritePort1(chan, IDX_IO1_o_Feature,      regs->bFeaturesReg);
                AtapiWritePort1(chan, IDX_IO1_o_BlockCount,   regs->bSectorCountRegH);
                AtapiWritePort1(chan, IDX_IO1_o_BlockCount,   regs->bSectorCountReg);
                AtapiWritePort1(chan, IDX_IO1_o_BlockNumber,  regs->bSectorNumberRegH);
                AtapiWritePort1(chan, IDX_IO1_o_BlockNumber,  regs->bSectorNumberReg);
                AtapiWritePort1(chan, IDX_IO1_o_CylinderLow,  regs->bCylLowRegH);
                AtapiWritePort1(chan, IDX_IO1_o_CylinderLow,  regs->bCylLowReg);
                AtapiWritePort1(chan, IDX_IO1_o_CylinderHigh, regs->bCylHighRegH);
                AtapiWritePort1(chan, IDX_IO1_o_CylinderHigh, regs->bCylHighReg);
            }
            AtapiWritePort1(chan, IDX_IO1_o_Command,      regs->bCommandReg);
            
            if(use_dma) {
                GetBaseStatus(chan, statusByte);
                if(statusByte & IDE_STATUS_ERROR) {
                    goto passthrough_err;
                }
                AtapiDmaStart(HwDeviceExtension, (Srb->TargetId & 0x1), lChannel, Srb);
            }

            ScsiPortStallExecution(1);                  // wait for busy to be set

            if(regs->bReserved & UNIATA_SPTI_EX_SPEC_TO) {
                to_lim = Srb->TimeOutValue;
            } else {
                if(Srb->TimeOutValue <= 2) {
                    to_lim = Srb->TimeOutValue*900;
                } else {
                    to_lim = (Srb->TimeOutValue*999) - 500;
                }
            }
            for(i=0; i<to_lim;i+=2) {      // 2 msec from WaitOnBaseBusy()
                statusByte = WaitOnBaseBusy(chan);      // wait for busy to be clear, up to 2 msec
                GetBaseStatus(chan, statusByte);
                if(statusByte & IDE_STATUS_ERROR) {
                    break;
                }
                if(!(statusByte & IDE_STATUS_BUSY)) {
                    break;
                }
            }
            if(i >= to_lim) {
                //if(regs->bReserved & UNIATA_SPTI_EX_FREEZE_TO) {
                //}
                AtapiResetController__(HwDeviceExtension, lChannel, RESET_COMPLETE_NONE);
                goto passthrough_err;
            }

            if(use_dma) {
                AtapiCheckInterrupt__(deviceExtension, (UCHAR)lChannel);
            }
            AtapiDmaDone(deviceExtension, (Srb->TargetId & 0x1), lChannel, NULL);
            GetBaseStatus(chan, statusByte);

            if(statusByte & (IDE_STATUS_BUSY | IDE_STATUS_ERROR)) {
                AtapiSuckPort2(chan);
passthrough_err:
                if (Srb->SenseInfoBuffer) {

                    PSENSE_DATA  senseBuffer = (PSENSE_DATA)Srb->SenseInfoBuffer;

                    senseBuffer->ErrorCode = 0x70;
                    senseBuffer->Valid     = 1;
                    senseBuffer->AdditionalSenseLength = 0xb;
                    senseBuffer->SenseKey =  SCSI_SENSE_ABORTED_COMMAND;
                    senseBuffer->AdditionalSenseCode = 0;
                    senseBuffer->AdditionalSenseCodeQualifier = 0;

                    Srb->SrbStatus = SRB_STATUS_AUTOSENSE_VALID;
                    Srb->ScsiStatus = SCSISTAT_CHECK_CONDITION;
                }
                status = SRB_STATUS_ERROR;
            } else {

                if(!use_dma) {
                    if (statusByte & IDE_STATUS_DRQ) {
                        if (Srb->SrbFlags & SRB_FLAGS_DATA_IN) {
                            ReadBuffer(chan,
                                       (PUSHORT) Srb->DataBuffer,
                                       Srb->DataTransferLength / 2,
                                       0);
                        } else if (Srb->SrbFlags & SRB_FLAGS_DATA_OUT) {
                            WriteBuffer(chan,
                                        (PUSHORT) Srb->DataBuffer,
                                        Srb->DataTransferLength / 2,
                                        0);
                        }
                    }
                }
                status = SRB_STATUS_SUCCESS;
            }

            AtapiEnableInterrupts(deviceExtension, lChannel);

        } else { // read task register

            regs = (PIDEREGS_EX) Srb->DataBuffer;

            regs->bDriveHeadReg    = AtapiReadPort1(chan, IDX_IO1_i_DriveSelect);

            if((regs->bReserved & ATA_FLAGS_48BIT_COMMAND) == 0) {      // execute ATA command
                regs->bFeaturesReg     = AtapiReadPort1(chan, IDX_IO1_i_Error);
                regs->bSectorCountReg  = AtapiReadPort1(chan, IDX_IO1_i_BlockCount);
                regs->bSectorNumberReg = AtapiReadPort1(chan, IDX_IO1_i_BlockNumber);
                regs->bCylLowReg       = AtapiReadPort1(chan, IDX_IO1_i_CylinderLow);
                regs->bCylHighReg      = AtapiReadPort1(chan, IDX_IO1_i_CylinderHigh);
            } else {
                regs->bFeaturesReg     = AtapiReadPort1(chan, IDX_IO1_i_Error);
                regs->bFeaturesRegH    = AtapiReadPort1(chan, IDX_IO1_i_Error);
                regs->bSectorCountReg  = AtapiReadPort1(chan, IDX_IO1_i_BlockCount);
                regs->bSectorCountRegH = AtapiReadPort1(chan, IDX_IO1_i_BlockCount);
                regs->bSectorNumberReg = AtapiReadPort1(chan, IDX_IO1_i_BlockNumber);
                regs->bSectorNumberRegH= AtapiReadPort1(chan, IDX_IO1_i_BlockNumber);
                regs->bCylLowReg       = AtapiReadPort1(chan, IDX_IO1_i_CylinderLow);
                regs->bCylLowRegH      = AtapiReadPort1(chan, IDX_IO1_i_CylinderLow);
                regs->bCylHighReg      = AtapiReadPort1(chan, IDX_IO1_i_CylinderHigh);
                regs->bCylHighRegH     = AtapiReadPort1(chan, IDX_IO1_i_CylinderHigh);
            }
            regs->bCommandReg      = AtapiReadPort1(chan, IDX_IO1_i_Status);
            status = SRB_STATUS_SUCCESS;
        }
        break;
    }

    default:

        KdPrint2((PRINT_PREFIX 
                   "IdeSendCommand: Unsupported command %#x\n",
                   Srb->Cdb[0]));

        status = SRB_STATUS_INVALID_REQUEST;

    } // end switch

    if(status == SRB_STATUS_PENDING) {
        KdPrint2((PRINT_PREFIX "IdeSendCommand: SRB_STATUS_PENDING\n"));
        if(CmdAction & CMD_ACTION_EXEC) {
            KdPrint2((PRINT_PREFIX "IdeSendCommand: REQ_STATE_EXPECTING_INTR\n"));
            AtaReq->ReqState = REQ_STATE_EXPECTING_INTR;
        }
    } else {
        KdPrint2((PRINT_PREFIX "IdeSendCommand: REQ_STATE_TRANSFER_COMPLETE\n"));
        AtaReq->ReqState = REQ_STATE_TRANSFER_COMPLETE;
    }

    return status;

} // end IdeSendCommand()


/*++

Routine Description:
    Enables disables media status notification

Arguments:
    HwDeviceExtension - ATAPI driver storage.

--*/
VOID
IdeMediaStatus(
    BOOLEAN EnableMSN,
    IN PVOID HwDeviceExtension,
    UCHAR ldev
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    PHW_CHANNEL          chan;
    UCHAR lChannel = ldev >> 1;
    UCHAR statusByte,errorByte;

    chan = &(deviceExtension->chan[lChannel]);

    if (EnableMSN == TRUE){

        // If supported enable Media Status Notification support
        if ((deviceExtension->lun[ldev].DeviceFlags & DFLAGS_REMOVABLE_DRIVE)) {

            // enable
            statusByte = AtaCommand(deviceExtension, ldev & 1, lChannel,
                                IDE_COMMAND_SET_FEATURES, 0, 0, 0,
                                0, ATA_C_F_ENAB_MEDIASTAT, ATA_WAIT_BASE_READY);

            if (statusByte & IDE_STATUS_ERROR) {
                // Read the error register.
                errorByte = AtapiReadPort1(chan, IDX_IO1_i_Error);

                KdPrint2((PRINT_PREFIX 
                            "IdeMediaStatus: Error enabling media status. Status %#x, error byte %#x\n",
                             statusByte,
                             errorByte));
            } else {
                deviceExtension->lun[ldev].DeviceFlags |= DFLAGS_MEDIA_STATUS_ENABLED;
                KdPrint2((PRINT_PREFIX "IdeMediaStatus: Media Status Notification Supported\n"));
                chan->ReturningMediaStatus = 0;

            }

        }
    } else { // end if EnableMSN == TRUE

        // disable if previously enabled
        if ((deviceExtension->lun[ldev].DeviceFlags & DFLAGS_MEDIA_STATUS_ENABLED)) {

            statusByte = AtaCommand(deviceExtension, ldev & 1, lChannel,
                                IDE_COMMAND_SET_FEATURES, 0, 0, 0,
                                0, ATA_C_F_DIS_MEDIASTAT, ATA_WAIT_BASE_READY);
            deviceExtension->lun[ldev].DeviceFlags &= ~DFLAGS_MEDIA_STATUS_ENABLED;
        }


    }


} // end IdeMediaStatus()


/*++

Routine Description:

    Builts an artificial sense buffer to report the results of a GET_MEDIA_STATUS
    command. This function is invoked to satisfy the SCSIOP_REQUEST_SENSE.
Arguments:

    HwDeviceExtension - ATAPI driver storage.
    Srb - System request block.

Return Value:

    SRB status (ALWAYS SUCCESS).

--*/
ULONG
IdeBuildSenseBuffer(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
//    ULONG status;
    PSENSE_DATA  senseBuffer = (PSENSE_DATA)Srb->DataBuffer;
    UCHAR ReturningMediaStatus = deviceExtension->chan[GET_CHANNEL(Srb)].ReturningMediaStatus;

    if (senseBuffer){

        if(ReturningMediaStatus & IDE_ERROR_MEDIA_CHANGE) {

            senseBuffer->ErrorCode = 0x70;
            senseBuffer->Valid     = 1;
            senseBuffer->AdditionalSenseLength = 0xb;
            senseBuffer->SenseKey =  SCSI_SENSE_UNIT_ATTENTION;
            senseBuffer->AdditionalSenseCode = SCSI_ADSENSE_MEDIUM_CHANGED;
            senseBuffer->AdditionalSenseCodeQualifier = 0;
        } else if(ReturningMediaStatus & IDE_ERROR_MEDIA_CHANGE_REQ) {

            senseBuffer->ErrorCode = 0x70;
            senseBuffer->Valid     = 1;
            senseBuffer->AdditionalSenseLength = 0xb;
            senseBuffer->SenseKey =  SCSI_SENSE_UNIT_ATTENTION;
            senseBuffer->AdditionalSenseCode = SCSI_ADSENSE_MEDIUM_CHANGED;
            senseBuffer->AdditionalSenseCodeQualifier = 0;
        } else if(ReturningMediaStatus & IDE_ERROR_END_OF_MEDIA) {

            senseBuffer->ErrorCode = 0x70;
            senseBuffer->Valid     = 1;
            senseBuffer->AdditionalSenseLength = 0xb;
            senseBuffer->SenseKey =  SCSI_SENSE_NOT_READY;
            senseBuffer->AdditionalSenseCode = SCSI_ADSENSE_NO_MEDIA_IN_DEVICE;
            senseBuffer->AdditionalSenseCodeQualifier = 0;
        } else if(ReturningMediaStatus & IDE_ERROR_DATA_ERROR) {

            senseBuffer->ErrorCode = 0x70;
            senseBuffer->Valid     = 1;
            senseBuffer->AdditionalSenseLength = 0xb;
            senseBuffer->SenseKey =  SCSI_SENSE_DATA_PROTECT;
            senseBuffer->AdditionalSenseCode = 0;
            senseBuffer->AdditionalSenseCodeQualifier = 0;
        }
        return SRB_STATUS_SUCCESS;
    }
    return SRB_STATUS_ERROR;

}// End of IdeBuildSenseBuffer

VOID
UniataUserDeviceReset(
    PHW_DEVICE_EXTENSION deviceExtension,
    PHW_LU_EXTENSION LunExt,
    ULONG PathId,
    ULONG ldev
    )
{
    AtapiDisableInterrupts(deviceExtension, PathId);
    if (LunExt->DeviceFlags & DFLAGS_ATAPI_DEVICE) {
        KdPrint2((PRINT_PREFIX "UniataUserDeviceReset: Reset ATAPI\n"));
        AtapiSoftReset(&(deviceExtension->chan[PathId]), ldev & 1);
    } else {
        KdPrint2((PRINT_PREFIX "UniataUserDeviceReset: Reset IDE -> reset entire channel\n"));
        AtapiResetController__(deviceExtension, PathId, RESET_COMPLETE_NONE);
    }
    LunExt->DeviceFlags |= DFLAGS_REINIT_DMA;  // force PIO/DMA reinit
    AtapiEnableInterrupts(deviceExtension, PathId);
    return;
} // end UniataUserDeviceReset()

BOOLEAN
UniataNeedQueueing(
    PHW_DEVICE_EXTENSION deviceExtension,
    PHW_CHANNEL          chan,
    BOOLEAN              TopLevel
    )
{
    BOOLEAN PostReq = FALSE;
    if(TopLevel) {
        if(chan->queue_depth > 0) {
#if 0
            if(atapiDev &&
               ((Srb->Cdb[0] == SCSIOP_TEST_UNIT_READY)/* ||
                (Srb->Cdb[0] == SCSIOP_REQUEST_SENSE)*/) ) {
                KdPrint2((PRINT_PREFIX "spec: SCSIOP_TEST_UNIT_READY\n"));
                //PostReq = FALSE;
                status = SRB_STATUS_BUSY;
                goto skip_exec;
            } else {
                PostReq = TRUE;
            }
#else
            PostReq = TRUE;
#endif
        } else
        if(deviceExtension->simplexOnly && deviceExtension->queue_depth > 0) {
            PostReq = TRUE;
        }
    }
    return PostReq;
} // end UniataNeedQueueing()

/*++

Routine Description:

    This routine is called from the SCSI port driver synchronized
    with the kernel to start an IO request.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    Srb - IO request packet

Return Value:

    TRUE

--*/
BOOLEAN
DDKAPI
AtapiStartIo(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    )
{
    return AtapiStartIo__(HwDeviceExtension, Srb, TRUE);
} // end AtapiStartIo()

BOOLEAN
AtapiStartIo__(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN BOOLEAN TopLevel
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    UCHAR                lChannel;
    PHW_CHANNEL          chan;
    ULONG status;
    ULONG ldev;
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
    PATA_REQ AtaReq;
    PSCSI_REQUEST_BLOCK tmpSrb;
    BOOLEAN PostReq = FALSE;
    BOOLEAN atapiDev;

    if(deviceExtension->Isr2DevObj && !BMList[deviceExtension->DevIndex].Isr2Enable) {
        KdPrint2((PRINT_PREFIX "Isr2Enable -> 1\n"));
        BMList[deviceExtension->DevIndex].Isr2Enable = TRUE;
    }
//    deviceExtension->QueueNewIrql = max(deviceExtension->QueueNewIrql, KeGetCurrentIrql());

/*                KeBugCheckEx(0xc000000e,
                             (Srb->PathId<<16) | (Srb->TargetId<<8) | (Srb->Lun),
                             Srb->Function, 
                             TopLevel, 0x80000001);
*/
    if(TopLevel && Srb && Srb->SrbExtension) {
        KdPrint2((PRINT_PREFIX "TopLevel\n"));
        RtlZeroMemory(Srb->SrbExtension, sizeof(ATA_REQ));
    }

    do {

        lChannel = GET_CHANNEL(Srb);
        chan = &(deviceExtension->chan[lChannel]);
        ldev = GET_LDEV(Srb);

        //ASSERT(deviceExtension);
        //ASSERT(chan);

        KdPrint2((PRINT_PREFIX 
                   "** AtapiStartIo: Function %#x, PATH:LUN:TID = %#x:%#x:%#x\n",
                   Srb->Function, Srb->PathId, Srb->Lun, Srb->TargetId));
        KdPrint2((PRINT_PREFIX "   VendorID+DeviceID/Rev %#x/%#x\n", deviceExtension->DevID, deviceExtension->RevID));

        if(GET_CDEV(Srb) >= 2 ||
           ldev >= deviceExtension->NumberChannels*2 ||
           lChannel >= deviceExtension->NumberChannels ||
           Srb->Lun) {

           if(lChannel >= deviceExtension->NumberChannels) {
               chan = NULL;
           }

reject_srb:
            //if(!CheckDevice(HwDeviceExtension, lChannel, ldev & 1, FALSE)) {
                KdPrint2((PRINT_PREFIX 
                           "AtapiStartIo: SRB rejected\n"));
                // Indicate no device found at this address.
                KdPrint2((PRINT_PREFIX "SRB_STATUS_SELECTION_TIMEOUT\n"));
                status = SRB_STATUS_SELECTION_TIMEOUT;
                goto complete_req;
            //}
        }

        atapiDev = (deviceExtension->lun[ldev].DeviceFlags & DFLAGS_ATAPI_DEVICE) ? TRUE : FALSE;

#ifdef _DEBUG
        if(!(chan->lun[ldev & 1])) {
            PrintNtConsole("de = %#x, chan = %#x , dev %#x, nchan %#x\n",
                deviceExtension,
                chan, ldev & 1,
                deviceExtension->NumberChannels);
            PrintNtConsole("lchan = %#x, ldev %#x, cdev %#x, lun0 %#x\n",
                lChannel, ldev, GET_CDEV(Srb), deviceExtension->chan[0].lun[0]);
            PrintNtConsole("Function %#x, PATH:LUN:TID = %#x:%#x:%#x\n",
                       Srb->Function, Srb->PathId, Srb->Lun, Srb->TargetId);
/*
            int i;
            for(i=0; i<1000; i++) {
                AtapiStallExecution(3*1000);
            }
*/
            goto reject_srb;
        }
#endif //_DEBUG
        //ASSERT(chan->lun[ldev & 1]);

        // Determine which function.
        switch (Srb->Function) {

        case SRB_FUNCTION_EXECUTE_SCSI:

            if(!(deviceExtension->lun[ldev].DeviceFlags & DFLAGS_DEVICE_PRESENT)) {
                if(Srb->Cdb[0] == SCSIOP_ATA_PASSTHROUGH) {
                    // let passthrough go
                } else
                if(Srb->Cdb[0] == SCSIOP_INQUIRY) {
                    // let INQUIRY go
                } else {

                //if(!CheckDevice(HwDeviceExtension, lChannel, ldev & 1, FALSE)) {
                    KdPrint2((PRINT_PREFIX 
                               "AtapiStartIo: EXECUTE_SCSI rejected (2)\n"));
                    // Indicate no device found at this address.
                    KdPrint2((PRINT_PREFIX "SRB_STATUS_SELECTION_TIMEOUT\n"));
                    status = SRB_STATUS_SELECTION_TIMEOUT;
                    break;
                //}
                }
            }
/*
            __try {
                if(Srb->DataTransferLength) {
                    UCHAR a;
                    a = ((PUCHAR)(Srb->DataBuffer))[0];
                    g_foo += a;
                }
            } __except(EXCEPTION_EXECUTE_HANDLER) {
                KdPrint2((PRINT_PREFIX 
                           "AtapiStartIo: Bad data buffer -> EXECUTE_SCSI rejected\n"));
                // Indicate no device found at this address.
                KdPrint2((PRINT_PREFIX "SRB_STATUS_ERROR\n"));
                status = SRB_STATUS_ERROR;
                KdPrint2((PRINT_PREFIX "  *** Exception...\n"));
                ASSERT(FALSE);
                break;
            }
*/
            PostReq = UniataNeedQueueing(deviceExtension, chan, TopLevel);

            if(PostReq) {

                KdPrint2((PRINT_PREFIX "Non-empty queue\n"));
                if (atapiDev &&
                    (Srb->Cdb[0] != SCSIOP_ATA_PASSTHROUGH)) {
                    KdPrint2((PRINT_PREFIX "Try ATAPI prepare\n"));

                    status = AtapiSendCommand(HwDeviceExtension, Srb, CMD_ACTION_PREPARE);
                } else {
                    KdPrint2((PRINT_PREFIX "Try IDE prepare\n"));
                    status = IdeSendCommand(HwDeviceExtension, Srb, CMD_ACTION_PREPARE);
                }
                /*KeBugCheckEx(0xc000000e,
                             (Srb->PathId<<16) | (Srb->TargetId<<8) | (Srb->Lun),
                             Srb->Function, 
                             status, 0x80000001);*/
                if(status == SRB_STATUS_BUSY)
                    status = SRB_STATUS_PENDING;
                // Insert requests AFTER they have been initialized on
                // CMD_ACTION_PREPARE stage
                // we should not check TopLevel here (it is always TRUE)
                //ASSERT(chan->lun[GET_LDEV(Srb) & 1]);
                UniataQueueRequest(chan, Srb);

                KdPrint2((PRINT_PREFIX "AtapiStartIo: Already have %d request(s)!\n", chan->queue_depth));

            } else {
                // Send command to device.
                KdPrint2((PRINT_PREFIX "Send to device\n"));
                if(TopLevel) {
                    KdPrint2((PRINT_PREFIX "TopLevel (2), srb %#x\n", Srb));
                    AtaReq = (PATA_REQ)(Srb->SrbExtension);
                    KdPrint2((PRINT_PREFIX "TopLevel (3), AtaReq %#x\n", AtaReq));
                    //ASSERT(!AtaReq->Flags);
                    //ASSERT(chan->lun[GET_LDEV(Srb) & 1]);
                    UniataQueueRequest(chan, Srb);
//                    AtaReq = (PATA_REQ)(Srb->SrbExtension);
                    //ASSERT(!AtaReq->Flags);
                    AtaReq->ReqState = REQ_STATE_QUEUED;
                    //ASSERT(!AtaReq->Flags);
                }

                if(!(deviceExtension->lun[ldev].DeviceFlags & DFLAGS_DEVICE_PRESENT)) {
                    if(Srb->Cdb[0] == SCSIOP_INQUIRY) {
                        if(UniataAnybodyHome(deviceExtension, chan->lChannel, ldev & 1)) {
                            if(!CheckDevice(HwDeviceExtension, chan->lChannel, ldev & 1, TRUE)) {
                                goto reject_srb;
                            }
                        }
                        if(!(deviceExtension->lun[ldev].DeviceFlags & DFLAGS_DEVICE_PRESENT)) {
                            goto reject_srb;
                        }
                    } else
                    if(Srb->Cdb[0] == SCSIOP_ATA_PASSTHROUGH) {
                        // allow
                    } else {
                        goto reject_srb;
                    }
                }

                if(atapiDev &&
                   (Srb->Cdb[0] != SCSIOP_ATA_PASSTHROUGH)) {
                    KdPrint2((PRINT_PREFIX "Try ATAPI send\n"));
                    status = AtapiSendCommand(HwDeviceExtension, Srb, CMD_ACTION_ALL);
                } else {
                    KdPrint2((PRINT_PREFIX "Try IDE send\n"));
/*                    {
                        ULONG __ebp__ = 0;
                        ULONG __esp__ = 0;

                        KdPrint2((PRINT_PREFIX "** before IdeSendCommand:\n"));
                        __asm {
                            mov eax,ebp
                            mov __ebp__, eax
                            mov eax,esp
                            mov __esp__, eax
                        }
                        KdPrint2((PRINT_PREFIX "** before Ide: EBP:%#x ESP:%#x\n", __ebp__, __esp__));
                    }*/
                    status = IdeSendCommand(HwDeviceExtension, Srb, CMD_ACTION_ALL);
                }
/*                KeBugCheckEx(0xc000000e,
                             (Srb->PathId<<16) | (Srb->TargetId<<8) | (Srb->Lun),
                             Srb->Function, 
                             status, 0x80000002);*/

            }
//skip_exec:
            TopLevel = FALSE;

            break;

        case SRB_FUNCTION_ABORT_COMMAND:

            tmpSrb = ScsiPortGetSrb(HwDeviceExtension, Srb->PathId, Srb->TargetId, Srb->Lun,
                               Srb->QueueTag);
            // Verify that SRB to abort is still outstanding.
            if((tmpSrb != Srb->NextSrb) ||
               !chan->queue_depth) {

                KdPrint2((PRINT_PREFIX "AtapiStartIo: SRB to abort already completed\n"));

                // Complete abort SRB.
                status = SRB_STATUS_ABORT_FAILED;
                break;
            }

            AtaReq = (PATA_REQ)(tmpSrb->SrbExtension);
            if(AtaReq->ReqState > REQ_STATE_READY_TO_TRANSFER) {
                if (!AtapiResetController__(deviceExtension, lChannel, RESET_COMPLETE_CURRENT)) {
                      KdPrint2((PRINT_PREFIX "AtapiStartIo: Abort command failed\n"));
                    // Log reset failure.
                    KdPrint2((PRINT_PREFIX 
                                "ScsiPortLogError: devExt %#x, Srb %#x, P:T:D=%d:%d:%d, MsgId %#x (%d)\n",
                                      HwDeviceExtension, NULL, 0, 0, 0, SP_INTERNAL_ADAPTER_ERROR, 5 << 8
                                ));
                    ScsiPortLogError(HwDeviceExtension, NULL, 0, 0, 0, SP_INTERNAL_ADAPTER_ERROR, 5 << 8);
                    status = SRB_STATUS_ERROR;

                } else {
                    status = SRB_STATUS_SUCCESS;
                }
            } else {
                KdPrint2((PRINT_PREFIX "AtapiInterrupt: remove aborted srb %#x\n", tmpSrb));
                if (tmpSrb->SenseInfoBuffer &&
                    tmpSrb->SenseInfoBufferLength >= sizeof(SENSE_DATA)) {

                    PSENSE_DATA  senseBuffer = (PSENSE_DATA)tmpSrb->SenseInfoBuffer;

                    senseBuffer->ErrorCode = 0;
                    senseBuffer->Valid     = 1;
                    senseBuffer->AdditionalSenseLength = 0xb;
                    senseBuffer->SenseKey =  SCSI_SENSE_ABORTED_COMMAND;
                    senseBuffer->AdditionalSenseCode = 0;
                    senseBuffer->AdditionalSenseCodeQualifier = 0;

                    tmpSrb->SrbStatus |= SRB_STATUS_AUTOSENSE_VALID;
                }
                AtapiDmaDBSync(chan, tmpSrb);
                UniataRemoveRequest(chan, tmpSrb);
                // Indicate command complete.
                ScsiPortNotification(RequestComplete,
                                     deviceExtension,
                                     tmpSrb);
                status = SRB_STATUS_SUCCESS;
            }
            break;

            // Abort function indicates that a request timed out.
            // Call reset routine. Card will only be reset if
            // status indicates something is wrong.
            // Fall through to reset code.

        case SRB_FUNCTION_RESET_DEVICE:
        case SRB_FUNCTION_RESET_LOGICAL_UNIT:

            // Reset single device.
            // For now we support only Lun=0

            // Note: reset is immediate command, it cannot be queued since it is usually used to
            // revert not- responding device to operational state
            KdPrint2((PRINT_PREFIX "AtapiStartIo: Reset device request received\n"));
            UniataUserDeviceReset(deviceExtension, &(deviceExtension->lun[ldev]), lChannel, ldev);
            status = SRB_STATUS_SUCCESS;
            break;

        case SRB_FUNCTION_RESET_BUS:

            // Reset Atapi and SCSI bus.

            // Note: reset is immediate command, it cannot be queued since it is usually used to
            // revert not- responding device to operational state
            KdPrint2((PRINT_PREFIX "AtapiStartIo: Reset bus request received\n"));
            if (!AtapiResetController__(deviceExtension, lChannel, RESET_COMPLETE_ALL)) {
                  KdPrint2((PRINT_PREFIX "AtapiStartIo: Reset bus failed\n"));
                // Log reset failure.
                KdPrint2((PRINT_PREFIX 
                            "ScsiPortLogError: devExt %#x, Srb %#x, P:T:D=%d:%d:%d, MsgId %#x (%d) - (2)\n",
                                  HwDeviceExtension, NULL, 0, 0, 0, SP_INTERNAL_ADAPTER_ERROR, 5 << 8
                            ));
                ScsiPortLogError(HwDeviceExtension, NULL, 0, 0, 0, SP_INTERNAL_ADAPTER_ERROR, 5 << 8);
                status = SRB_STATUS_ERROR;

            } else {
                status = SRB_STATUS_SUCCESS;
            }

            break;

        case SRB_FUNCTION_SHUTDOWN:

            KdPrint2((PRINT_PREFIX "AtapiStartIo: Shutdown\n"));
            if(!(deviceExtension->lun[ldev].DeviceFlags & DFLAGS_DEVICE_PRESENT)) {
                KdPrint2((PRINT_PREFIX "AtapiStartIo: Shutdown - no such device\n"));
            }
            if(atapiDev) {
                // FLUSH ATAPI device - do nothing
                KdPrint2((PRINT_PREFIX "AtapiStartIo: Shutdown - ATAPI device\n"));
            } else {
                // FLUSH IDE/ATA device
                KdPrint2((PRINT_PREFIX "AtapiStartIo: Shutdown - IDE device\n"));
                AtapiDisableInterrupts(deviceExtension, lChannel);
                status = AtaCommand(deviceExtension, ldev & 1, GET_CHANNEL(Srb),
                           IDE_COMMAND_FLUSH_CACHE, 0, 0, 0, 0, 0, ATA_WAIT_IDLE);
                // If supported & allowed, reset write cacheing
                if(deviceExtension->lun[ldev].DeviceFlags & DFLAGS_WCACHE_ENABLED) {

                    // Disable write cache
                    status = AtaCommand(deviceExtension, ldev & 1, lChannel,
                                        IDE_COMMAND_SET_FEATURES, 0, 0, 0,
                                        0, ATA_C_F_DIS_WCACHE, ATA_WAIT_BASE_READY);
                    // Check for errors.
                    if (status & IDE_STATUS_ERROR) {
                        KdPrint2((PRINT_PREFIX 
                                    "AtapiHwInitialize: Disable write cacheing on Device %d failed\n",
                                    ldev));
                    }
                    deviceExtension->lun[ldev].DeviceFlags &= ~DFLAGS_WCACHE_ENABLED;

                    // Re-enable write cache
                    status = AtaCommand(deviceExtension, ldev & 1, lChannel,
                                        IDE_COMMAND_SET_FEATURES, 0, 0, 0,
                                        0, ATA_C_F_ENAB_WCACHE, ATA_WAIT_BASE_READY);
                    // Check for errors.
                    if (status & IDE_STATUS_ERROR) {
                        KdPrint2((PRINT_PREFIX 
                                    "AtapiHwInitialize: Enable write cacheing on Device %d failed\n",
                                    ldev));
                        deviceExtension->lun[ldev].DeviceFlags &= ~DFLAGS_WCACHE_ENABLED;
                    } else {
                        deviceExtension->lun[ldev].DeviceFlags |= DFLAGS_WCACHE_ENABLED;
                    }
                }

                AtapiEnableInterrupts(deviceExtension, lChannel);
            }
            status = SRB_STATUS_SUCCESS;

            break;

        case SRB_FUNCTION_FLUSH:

            KdPrint2((PRINT_PREFIX "AtapiStartIo: Flush (do nothing)\n"));
            status = SRB_STATUS_SUCCESS;
            break;

    /*    case SRB_FUNCTION_SHUTDOWN:
        case SRB_FUNCTION_FLUSH:

            // Flush device's cache.
            KdPrint2((PRINT_PREFIX "AtapiStartIo: Device flush received\n"));

            if (chan->CurrentSrb) {

                KdPrint2((PRINT_PREFIX "AtapiStartIo (SRB_FUNCTION_FLUSH): Already have a request!\n"));
                Srb->SrbStatus = SRB_STATUS_BUSY;
                ScsiPortNotification(RequestComplete,
                                     deviceExtension,
                                     Srb);
                return FALSE;
            }

            if (deviceExtension->lun[ldev].DeviceFlags & DFLAGS_ATAPI_DEVICE) {
                status = SRB_STATUS_SUCCESS;
            } else {
                status = AtaCommand(deviceExtension, ldev & 1, GET_CHANNEL(Srb),
                           IDE_COMMAND_FLUSH_CACHE, 0, 0, 0, 0, 0, ATA_WAIT_INTR);
                if (status & IDE_STATUS_DRQ) {
                    status = SRB_STATUS_SUCCESS;
                } else {
                    status = SRB_STATUS_SELECTION_TIMEOUT;
                }
            }
            break;*/

        case SRB_FUNCTION_IO_CONTROL: {

            ULONG len;

            KdPrint2((PRINT_PREFIX "AtapiStartIo: SRB_FUNCTION_IO_CONTROL\n"));

            len = Srb->DataTransferLength;

            if(!AtapiStringCmp( (PCHAR)(((PSRB_IO_CONTROL)(Srb->DataBuffer))->Signature),"SCSIDISK",sizeof("SCSIDISK")-1)) {

                switch (((PSRB_IO_CONTROL)(Srb->DataBuffer))->ControlCode) {
                case IOCTL_SCSI_MINIPORT_SMART_VERSION: {

                    PGETVERSIONINPARAMS versionParameters = (PGETVERSIONINPARAMS)(((PUCHAR)Srb->DataBuffer) + sizeof(SRB_IO_CONTROL));
                    UCHAR deviceNumber;

                    KdPrint2((PRINT_PREFIX "AtapiStartIo: IOCTL_SCSI_MINIPORT_SMART_VERSION\n"));

                    // Version and revision per SMART 1.03

                    versionParameters->bVersion = 1;
                    versionParameters->bRevision = 1;
                    versionParameters->bReserved = 0;

                    // Indicate that support for IDE IDENTIFY, ATAPI IDENTIFY and SMART commands.
                    versionParameters->fCapabilities = (CAP_ATA_ID_CMD | CAP_ATAPI_ID_CMD | CAP_SMART_CMD);

                    // This is done because of how the IOCTL_SCSI_MINIPORT
                    // determines 'targetid's'. Disk.sys places the real target id value
                    // in the DeviceMap field. Once we do some parameter checking, the value passed
                    // back to the application will be determined.

                    deviceNumber = versionParameters->bIDEDeviceMap;

                    if (!(deviceExtension->lun[ldev].DeviceFlags & DFLAGS_DEVICE_PRESENT) ||
                        atapiDev) {

                        status = SRB_STATUS_SELECTION_TIMEOUT;
                        break;
                    }

                    // NOTE: This will only set the bit
                    // corresponding to this drive's target id.
                    // The bit mask is as follows:
                    //
                    //     -Sec Pri
                    //     S M S M
                    //     3 2 1 0

                    if (deviceExtension->NumberChannels == 1) {
                        if (chan->PrimaryAddress) {
                            deviceNumber = 1 << ldev;
                        } else {
                            deviceNumber = 4 << ldev;
                        }
                    } else {
                        deviceNumber = 1 << ldev;
                    }

                    versionParameters->bIDEDeviceMap = deviceNumber;

                    status = SRB_STATUS_SUCCESS;
                    break;
                }

                case IOCTL_SCSI_MINIPORT_IDENTIFY: {

                    PSENDCMDOUTPARAMS cmdOutParameters = (PSENDCMDOUTPARAMS)(((PUCHAR)Srb->DataBuffer) + sizeof(SRB_IO_CONTROL));
                    SENDCMDINPARAMS   cmdInParameters = *(PSENDCMDINPARAMS)(((PUCHAR)Srb->DataBuffer) + sizeof(SRB_IO_CONTROL));
                    UCHAR             targetId;

                    KdPrint2((PRINT_PREFIX "AtapiStartIo: IOCTL_SCSI_MINIPORT_IDENTIFY\n"));
                    // Extract the target.
                    targetId = cmdInParameters.bDriveNumber;
                    KdPrint2((PRINT_PREFIX "targetId %d\n", targetId));
                    if((targetId >= deviceExtension->NumberChannels*2) ||
                       !(deviceExtension->lun[targetId].DeviceFlags & DFLAGS_DEVICE_PRESENT)) {
                        KdPrint2((PRINT_PREFIX "Error: xxx_ID_CMD for non-existant device\n"));
                        status = SRB_STATUS_SELECTION_TIMEOUT;
                        break;
                    }

                    switch(cmdInParameters.irDriveRegs.bCommandReg) {
                    case ID_CMD:
                        if((deviceExtension->lun[targetId].DeviceFlags & DFLAGS_ATAPI_DEVICE)) {
                            KdPrint2((PRINT_PREFIX "Error: ID_CMD for ATAPI\n"));
                            status = SRB_STATUS_INVALID_REQUEST;
                            break;
                        }
                        /* FALL THROUGH */
                    case ATAPI_ID_CMD:

                        if(!(deviceExtension->lun[targetId].DeviceFlags & DFLAGS_ATAPI_DEVICE) &&
                           (cmdInParameters.irDriveRegs.bCommandReg == ATAPI_ID_CMD)) {
                            KdPrint2((PRINT_PREFIX "Error: ATAPI_ID_CMD for non-ATAPI\n"));
                            status = SRB_STATUS_INVALID_REQUEST;
                            break;
                        }

                        len = min(len, sizeof(SENDCMDOUTPARAMS) - 1 + IDENTIFY_BUFFER_SIZE);
                        // Zero the output buffer
                        RtlZeroMemory(cmdOutParameters, len);
/*                        for (i = 0; i < (sizeof(SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE - 1); i++) {
                            ((PUCHAR)cmdOutParameters)[i] = 0;
                        }*/

                        // Build status block.
                        cmdOutParameters->cBufferSize = min(IDENTIFY_BUFFER_SIZE, len - sizeof(SENDCMDOUTPARAMS) + 1);
                        cmdOutParameters->DriverStatus.bDriverError = 0;
                        cmdOutParameters->DriverStatus.bIDEError = 0;

                        // Extract the identify data from the device extension.
                        ScsiPortMoveMemory (cmdOutParameters->bBuffer, &deviceExtension->lun[targetId].IdentifyData,
                            cmdOutParameters->cBufferSize);

                        KdPrint2((PRINT_PREFIX "AtapiStartIo: IOCTL_SCSI_MINIPORT_IDENTIFY Ok\n"));

                        status = SRB_STATUS_SUCCESS;

                        break;
                    default:
                        KdPrint2((PRINT_PREFIX "AtapiStartIo: not supported ID code %x\n",
                            cmdInParameters.irDriveRegs.bCommandReg));
                        status = SRB_STATUS_INVALID_REQUEST;
                        break;
                    }
                    break;
                }

                case  IOCTL_SCSI_MINIPORT_READ_SMART_ATTRIBS:
                case  IOCTL_SCSI_MINIPORT_READ_SMART_THRESHOLDS:
                case  IOCTL_SCSI_MINIPORT_ENABLE_SMART:
                case  IOCTL_SCSI_MINIPORT_DISABLE_SMART:
                case  IOCTL_SCSI_MINIPORT_RETURN_STATUS:
                case  IOCTL_SCSI_MINIPORT_ENABLE_DISABLE_AUTOSAVE:
                case  IOCTL_SCSI_MINIPORT_SAVE_ATTRIBUTE_VALUES:
                case  IOCTL_SCSI_MINIPORT_EXECUTE_OFFLINE_DIAGS:

                    PostReq = UniataNeedQueueing(deviceExtension, chan, TopLevel);

                    if(PostReq || TopLevel) {
                        UniataQueueRequest(chan, Srb);
                        AtaReq = (PATA_REQ)(Srb->SrbExtension);
                        AtaReq->ReqState = REQ_STATE_QUEUED;
                    }

                    if(PostReq) {

                        KdPrint2((PRINT_PREFIX "Non-empty queue (SMART)\n"));
                        status = SRB_STATUS_PENDING;

                        KdPrint2((PRINT_PREFIX "AtapiStartIo: Already have %d request(s)!\n", chan->queue_depth));
                    } else {

                        status = IdeSendSmartCommand(HwDeviceExtension,Srb);
                    }
                    break;

                default :
                    KdPrint2((PRINT_PREFIX "AtapiStartIo: invalid IoControl %#x for SCSIDISK signature\n",
                                ((PSRB_IO_CONTROL)(Srb->DataBuffer))->ControlCode ));
                    status = SRB_STATUS_INVALID_REQUEST;
                    break;

                }
            } else
            if(!AtapiStringCmp( (PCHAR)(((PSRB_IO_CONTROL)(Srb->DataBuffer))->Signature),"-UNIATA-", sizeof("-UNIATA-")-1)) {

                PUNIATA_CTL AtaCtl = (PUNIATA_CTL)(Srb->DataBuffer);
                ULONG ldev = GET_LDEV2(AtaCtl->addr.PathId, AtaCtl->addr.TargetId, 0);
                PHW_LU_EXTENSION LunExt = &(deviceExtension->lun[ldev]);
                BOOLEAN bad_ldev;
                ULONG i;
                //chan = &(deviceExtension->chan[lChannel]);

                if(AtaCtl->addr.Lun ||
                   ldev >= deviceExtension->NumberChannels*2 ||
                   AtaCtl->addr.PathId > deviceExtension->NumberChannels) {

                    bad_ldev = TRUE;
                    LunExt = NULL;

                } else {
                    bad_ldev = FALSE;
                    LunExt = &(deviceExtension->lun[ldev]);
                }

                KdPrint2((PRINT_PREFIX "AtapiStartIo: -UNIATA- %#x, ldev %#x\n", AtaCtl->hdr.ControlCode, ldev));

                /* check for valid LUN */
                switch (AtaCtl->hdr.ControlCode) {
                case  IOCTL_SCSI_MINIPORT_UNIATA_FIND_DEVICES:
                case  IOCTL_SCSI_MINIPORT_UNIATA_DELETE_DEVICE:
                case  IOCTL_SCSI_MINIPORT_UNIATA_SET_MAX_MODE:
                case  IOCTL_SCSI_MINIPORT_UNIATA_GET_MODE:
                case  IOCTL_SCSI_MINIPORT_UNIATA_RESETBB:
                case  IOCTL_SCSI_MINIPORT_UNIATA_RESET_DEVICE:
                    if(bad_ldev) {
                        KdPrint2((PRINT_PREFIX 
                                   "AtapiStartIo: bad_ldev -> IOCTL SRB rejected\n"));
                        // Indicate no device found at this address.
                        goto reject_srb;
                    }
                }

                /* check if queueing is necessary */
                switch (AtaCtl->hdr.ControlCode) {
                case  IOCTL_SCSI_MINIPORT_UNIATA_RESETBB:
                    if(!LunExt->nBadBlocks) {
                        break;
                    }
                    goto uata_ctl_queue;
                case  IOCTL_SCSI_MINIPORT_UNIATA_SET_MAX_MODE:
                    if(!AtaCtl->SetMode.ApplyImmediately) {
                        break;
                    }
                    goto uata_ctl_queue;
                case  IOCTL_SCSI_MINIPORT_UNIATA_FIND_DEVICES:
                //case  IOCTL_SCSI_MINIPORT_UNIATA_RESET_DEVICE: reset must be processed immediately
uata_ctl_queue:
                    KdPrint2((PRINT_PREFIX "put to queue (UNIATA)\n"));
                    PostReq = UniataNeedQueueing(deviceExtension, chan, TopLevel);

                    if(PostReq || TopLevel) {
                        UniataQueueRequest(chan, Srb);
                        AtaReq = (PATA_REQ)(Srb->SrbExtension);
                        AtaReq->ReqState = REQ_STATE_QUEUED;
                    }
                    if(PostReq) {
                        KdPrint2((PRINT_PREFIX "Non-empty queue (UNIATA)\n"));
                        status = SRB_STATUS_PENDING;

                        KdPrint2((PRINT_PREFIX "AtapiStartIo: Already have %d request(s)!\n", chan->queue_depth));
                        goto complete_req;
                     } 
                }

                /* process request */
                switch (AtaCtl->hdr.ControlCode) {
                case  IOCTL_SCSI_MINIPORT_UNIATA_FIND_DEVICES:

                    KdPrint2((PRINT_PREFIX "AtapiStartIo: rescan bus\n"));

                    for(i=0; i<AtaCtl->FindDelDev.WaitForPhysicalLink && i<30; i++) {
                        AtapiStallExecution(1000 * 1000);
                    }

                    FindDevices(HwDeviceExtension, FALSE, AtaCtl->addr.PathId);
                    status = SRB_STATUS_SUCCESS;

                    break;

                case  IOCTL_SCSI_MINIPORT_UNIATA_DELETE_DEVICE: {

                    KdPrint2((PRINT_PREFIX "AtapiStartIo: remove %#x:%#x\n", AtaCtl->addr.PathId, AtaCtl->addr.TargetId));

                    deviceExtension->lun[ldev].DeviceFlags = 0;

                    for(i=0; i<AtaCtl->FindDelDev.WaitForPhysicalLink && i<30; i++) {
                        AtapiStallExecution(1000 * 1000);
                    }

                    status = SRB_STATUS_SUCCESS;
                    break;
                }
                case  IOCTL_SCSI_MINIPORT_UNIATA_SET_MAX_MODE: {

                    KdPrint2((PRINT_PREFIX "AtapiStartIo: Set transfer mode\n"));

                    if(AtaCtl->SetMode.OrigMode != (ULONG)-1) {
                        LunExt->OrigTransferMode = (UCHAR)(AtaCtl->SetMode.OrigMode);
                    }
                    if(AtaCtl->SetMode.MaxMode != (ULONG)-1) {
                        LunExt->LimitedTransferMode = (UCHAR)(AtaCtl->SetMode.MaxMode);
                        if(LunExt->LimitedTransferMode > 
                           LunExt->OrigTransferMode) {
                            // check for incorrect value
                            LunExt->LimitedTransferMode =
                                LunExt->OrigTransferMode;
                        }
                    }
                    LunExt->TransferMode = LunExt->OrigTransferMode;

                    LunExt->DeviceFlags |= DFLAGS_REINIT_DMA;  // force PIO/DMA reinit
                    if(AtaCtl->SetMode.ApplyImmediately) {
                        AtapiDmaInit__(deviceExtension, ldev);
                    }
/*                    deviceExtension->lun[ldev].TransferMode =
                    deviceExtension->lun[ldev].LimitedTransferMode = (UCHAR)(setTransferMode->Mode);*/
                    status = SRB_STATUS_SUCCESS;
                    break;
                }
                case  IOCTL_SCSI_MINIPORT_UNIATA_GET_MODE: {

                    KdPrint2((PRINT_PREFIX "AtapiStartIo: Get transfer mode\n"));

                    AtaCtl->GetMode.OrigMode    = LunExt->OrigTransferMode;
                    AtaCtl->GetMode.MaxMode     = LunExt->LimitedTransferMode;
                    AtaCtl->GetMode.CurrentMode = LunExt->TransferMode;

                    status = SRB_STATUS_SUCCESS;
                    break;
                }
                case  IOCTL_SCSI_MINIPORT_UNIATA_ADAPTER_INFO: {

                    KdPrint2((PRINT_PREFIX "AtapiStartIo: Get adapter info\n"));

                    AtaCtl->AdapterInfo.HeaderLength = offsetof(ADAPTERINFO, Chan);

                    if(len < AtaCtl->AdapterInfo.HeaderLength + sizeof(AtaCtl->AdapterInfo.Chan)) {
                        KdPrint2((PRINT_PREFIX "AtapiStartIo: Buffer too small: %#x < %#x\n", len,
                            AtaCtl->AdapterInfo.HeaderLength + sizeof(AtaCtl->AdapterInfo.Chan)));
                        status = SRB_STATUS_DATA_OVERRUN;
                        break;
                    }

                    AtaCtl->AdapterInfo.DevID      = deviceExtension->DevID;
                    AtaCtl->AdapterInfo.RevID      = deviceExtension->RevID;
                    AtaCtl->AdapterInfo.slotNumber = deviceExtension->slotNumber;
                    AtaCtl->AdapterInfo.SystemIoBusNumber = deviceExtension->SystemIoBusNumber;
                    AtaCtl->AdapterInfo.DevIndex   = deviceExtension->DevIndex;
                    AtaCtl->AdapterInfo.Channel    = deviceExtension->Channel;
                    AtaCtl->AdapterInfo.HbaCtrlFlags = deviceExtension->HbaCtrlFlags;
                    AtaCtl->AdapterInfo.simplexOnly= deviceExtension->simplexOnly;
                    AtaCtl->AdapterInfo.MemIo      = FALSE;/*deviceExtension->MemIo;*/
                    AtaCtl->AdapterInfo.UnknownDev = deviceExtension->UnknownDev;
                    AtaCtl->AdapterInfo.MasterDev  = deviceExtension->MasterDev;
                    AtaCtl->AdapterInfo.MaxTransferMode = deviceExtension->MaxTransferMode;
                    AtaCtl->AdapterInfo.HwFlags    = deviceExtension->HwFlags;
                    AtaCtl->AdapterInfo.OrigAdapterInterfaceType = deviceExtension->OrigAdapterInterfaceType;
                    AtaCtl->AdapterInfo.BusInterruptLevel = deviceExtension->BusInterruptLevel;
                    AtaCtl->AdapterInfo.InterruptMode = deviceExtension->InterruptMode;
                    AtaCtl->AdapterInfo.BusInterruptVector = deviceExtension->BusInterruptVector;
                    AtaCtl->AdapterInfo.NumberChannels = deviceExtension->NumberChannels;

                    AtaCtl->AdapterInfo.ChanInfoValid = FALSE;

                    RtlZeroMemory(&AtaCtl->AdapterInfo.Chan, sizeof(AtaCtl->AdapterInfo.Chan));

                    status = SRB_STATUS_SUCCESS;
                    break;
                }
                case  IOCTL_SCSI_MINIPORT_UNIATA_RESETBB: {
                    
                    KdPrint2((PRINT_PREFIX "AtapiStartIo: Forget BB list\n"));

                    ForgetBadBlocks(LunExt);

                    status = SRB_STATUS_SUCCESS;
                    break;
                }
                case  IOCTL_SCSI_MINIPORT_UNIATA_RESET_DEVICE: {

                    KdPrint2((PRINT_PREFIX "AtapiStartIo: Reset device\n"));

                    UniataUserDeviceReset(deviceExtension, LunExt, AtaCtl->addr.PathId, ldev);

                    status = SRB_STATUS_SUCCESS;
                    break;
                }
                default :
                    KdPrint2((PRINT_PREFIX "AtapiStartIo: invalid IoControl %#x for -UNIATA- signature\n",
                                AtaCtl->hdr.ControlCode ));
                    status = SRB_STATUS_INVALID_REQUEST;
                    break;
                }

            } else {
                KdPrint2((PRINT_PREFIX "AtapiStartIo: IoControl signature incorrect. Send %s, expected %s or %s\n",
                            ((PSRB_IO_CONTROL)(Srb->DataBuffer))->Signature,
                            "SCSIDISK", "-UNIATA-"));

                status = SRB_STATUS_INVALID_REQUEST;
                break;
            }

            break;
        } // end SRB_FUNCTION_IO_CONTROL
        default:

            KdPrint2((PRINT_PREFIX "AtapiStartIo: Unknown IOCTL\n"));
            // Indicate unsupported command.
            status = SRB_STATUS_INVALID_REQUEST;

//            break;

        } // end switch

complete_req:

        PathId   = Srb->PathId;
        TargetId = Srb->TargetId;
        Lun      = Srb->Lun;

        if (status != SRB_STATUS_PENDING) {

            KdPrint2((PRINT_PREFIX 
                       "AtapiStartIo: Srb %#x complete with status %#x\n",
                       Srb,
                       status));

            // Set status in SRB.
            Srb->SrbStatus = (UCHAR)status;

            AtapiDmaDBSync(chan, Srb);
            UniataRemoveRequest(chan, Srb);
            // Indicate command complete.
            ScsiPortNotification(RequestComplete,
                                 deviceExtension,
                                 Srb);

            // Remove current Srb & get next one
            if((Srb = UniataGetCurRequest(chan))) {
                AtaReq = (PATA_REQ)(Srb->SrbExtension);
                if(AtaReq->ReqState > REQ_STATE_QUEUED) {
                    // current request is under precessing, thus
                    // we should do nothing here
                    Srb = NULL;
                }
            }
            if(!chan) {
                //ASSERT(TopLevel);
            }
        }
        KdPrint2((PRINT_PREFIX "AtapiStartIo: next Srb %x\n", Srb));

    } while (Srb && (status != SRB_STATUS_PENDING));

    KdPrint2((PRINT_PREFIX "AtapiStartIo: query PORT for next request\n"));
    // Indicate ready for next request.
    ScsiPortNotification(NextRequest,
                         deviceExtension,
                         NULL);

    ScsiPortNotification(NextLuRequest,
                         deviceExtension, 
                         PathId,
                         TargetId,
                         Lun);

    return TRUE;

} // end AtapiStartIo__()


void
UniataInitAtaCommands()
{
    int i;
    UCHAR command;
    UCHAR flags = 0;

    for(i=0, command=0; i<256; i++, command++) {

        switch(command) {
        case IDE_COMMAND_READ_DMA48:
        case IDE_COMMAND_READ_DMA_Q48:
        case IDE_COMMAND_READ_STREAM_DMA48:
        case IDE_COMMAND_READ_STREAM48:
        case IDE_COMMAND_WRITE_DMA48:
        case IDE_COMMAND_WRITE_DMA_Q48:
        case IDE_COMMAND_READ_DMA_Q:
        case IDE_COMMAND_READ_DMA:
        case IDE_COMMAND_WRITE_DMA:
        case IDE_COMMAND_WRITE_DMA_Q:
        case IDE_COMMAND_WRITE_STREAM_DMA48:
        case IDE_COMMAND_WRITE_STREAM48:
        case IDE_COMMAND_WRITE_FUA_DMA48:
        case IDE_COMMAND_WRITE_FUA_DMA_Q48:
        case IDE_COMMAND_READ_LOG_DMA48:
        case IDE_COMMAND_WRITE_LOG_DMA48:
        case IDE_COMMAND_TRUSTED_RCV_DMA:
        case IDE_COMMAND_TRUSTED_SEND_DMA:
            flags |= ATA_CMD_FLAG_DMA;
        }

        switch(command) {
        case IDE_COMMAND_READ48:
        case IDE_COMMAND_READ_DMA48:
        case IDE_COMMAND_READ_DMA_Q48:
        case IDE_COMMAND_READ_MUL48:
        case IDE_COMMAND_READ_STREAM_DMA48:
        case IDE_COMMAND_READ_STREAM48:
        case IDE_COMMAND_WRITE48:
        case IDE_COMMAND_WRITE_DMA48:
        case IDE_COMMAND_WRITE_DMA_Q48:
        case IDE_COMMAND_WRITE_MUL48:
        case IDE_COMMAND_WRITE_STREAM_DMA48:
        case IDE_COMMAND_WRITE_STREAM48:
        case IDE_COMMAND_WRITE_FUA_DMA48:
        case IDE_COMMAND_WRITE_FUA_DMA_Q48:
        case IDE_COMMAND_WRITE_MUL_FUA48:
        case IDE_COMMAND_FLUSH_CACHE48:
        case IDE_COMMAND_VERIFY48:

            flags |= ATA_CMD_FLAG_48;
            /* FALL THROUGH */

        case IDE_COMMAND_READ:
        case IDE_COMMAND_READ_MULTIPLE:
        case IDE_COMMAND_READ_DMA:
        case IDE_COMMAND_READ_DMA_Q:
        case IDE_COMMAND_WRITE:
        case IDE_COMMAND_WRITE_MULTIPLE:
        case IDE_COMMAND_WRITE_DMA:
        case IDE_COMMAND_WRITE_DMA_Q:
        case IDE_COMMAND_FLUSH_CACHE:
        case IDE_COMMAND_VERIFY:

            flags |= ATA_CMD_FLAG_LBAIOsupp;
        }

        flags |= ATA_CMD_FLAG_48supp;

        switch (command) {
        case IDE_COMMAND_READ:
            command = IDE_COMMAND_READ48; break;
        case IDE_COMMAND_READ_MULTIPLE:
            command = IDE_COMMAND_READ_MUL48; break;
        case IDE_COMMAND_READ_DMA:
            command = IDE_COMMAND_READ_DMA48; break;
        case IDE_COMMAND_READ_DMA_Q:
            command = IDE_COMMAND_READ_DMA_Q48; break;
        case IDE_COMMAND_WRITE:
            command = IDE_COMMAND_WRITE48; break;
        case IDE_COMMAND_WRITE_MULTIPLE:
            command = IDE_COMMAND_WRITE_MUL48; break;
        case IDE_COMMAND_WRITE_DMA:
            command = IDE_COMMAND_WRITE_DMA48; break;
        case IDE_COMMAND_WRITE_DMA_Q:
            command = IDE_COMMAND_WRITE_DMA_Q48; break;
        case IDE_COMMAND_FLUSH_CACHE:
            command = IDE_COMMAND_FLUSH_CACHE48; break;
        case IDE_COMMAND_READ_NATIVE_SIZE:
    //            command = IDE_COMMAND_READ_NATIVE_SIZE48; break;
        case IDE_COMMAND_SET_NATIVE_SIZE:
            command = IDE_COMMAND_SET_NATIVE_SIZE48; break;
        case IDE_COMMAND_VERIFY:
            command = IDE_COMMAND_VERIFY48; break;
        default:
            flags &= ~ATA_CMD_FLAG_48supp;
        }

        AtaCommands48[i]   = command;
        AtaCommandFlags[i] = flags;
    }
} // end UniataInitAtaCommands()

/*++

Routine Description:

    Installable driver initialization entry point for system.

Arguments:

    Driver Object

Return Value:

    Status from ScsiPortInitialize()

--*/
extern "C"
ULONG
DDKAPI
DriverEntry(
    IN PVOID DriverObject,
    IN PVOID Argument2
    )
{
    HW_INITIALIZATION_DATA_COMMON hwInitializationData;
    ULONG                  adapterCount;
    ULONG                  i, c, alt;
    ULONG                  statusToReturn, newStatus;
    PUNICODE_STRING        RegistryPath = (PUNICODE_STRING)Argument2;
    BOOLEAN                ReEnter = FALSE;
    WCHAR                  a;
    NTSTATUS               status;

    PCONFIGURATION_INFORMATION GlobalConfig = IoGetConfigurationInformation();
    BOOLEAN PrimaryClaimed   = FALSE;
    BOOLEAN SecondaryClaimed = FALSE;

    LARGE_INTEGER t0, t1;

    Connect_DbgPrint();
    KdPrint2((PRINT_PREFIX (PCCHAR)ver_string));
    a = (WCHAR)strlen(ver_string);

    g_opt_Verbose = (BOOLEAN)AtapiRegCheckDevValue(NULL, CHAN_NOT_SPECIFIED, DEVNUM_NOT_SPECIFIED, L"PrintLogo", 0);
    if(g_opt_Verbose) {
        _PrintNtConsole("Universal ATA driver v 0." UNIATA_VER_STR "\n");
    }

    if(!SavedDriverObject) {
        SavedDriverObject = (PDRIVER_OBJECT)DriverObject;
        // we are here for the 1st time
        // init CrossNT and get OS version
        if(!NT_SUCCESS(status = CrNtInit(SavedDriverObject, RegistryPath))) {
            KdPrint(("UniATA Init: CrNtInit failed with status %#x\n", status));
            //HalDisplayString((PUCHAR)"DbgPrnHkInitialize: CrNtInit failed\n");
            return status;
        }
        KdPrint(("UniATA Init: OS ver %x.%x (%d)\n", MajorVersion, MinorVersion, BuildNumber));

        KeQuerySystemTime(&t0);
        do {
            KeQuerySystemTime(&t1);
        } while(t0.QuadPart == t1.QuadPart);
        t0=t1;
        g_Perf=0;
        do {
            KeQuerySystemTime(&t1);
            g_Perf++;
        } while(t0.QuadPart == t1.QuadPart);
        g_PerfDt = (ULONG)((t1.QuadPart - t0.QuadPart)/10);
        KdPrint(("Performance calibration: dt=%d, counter=%I64d\n", g_PerfDt, g_Perf ));

    } else {
        ReEnter = TRUE;
    }

    // (re)read bad block list
    InitBadBlocks(NULL);

    if(!ReEnter) {
        // init ATA command translation table
        UniataInitAtaCommands();
        // get registry path to settings
        RtlCopyMemory(&SavedRegPath, RegistryPath, sizeof(UNICODE_STRING));
        SavedRegPath.Buffer = (PWCHAR)&SavedRegPathBuffer;
        SavedRegPath.Length = min(RegistryPath->Length, 255*sizeof(WCHAR));
        SavedRegPath.MaximumLength = 255*sizeof(WCHAR);
        RtlCopyMemory(SavedRegPath.Buffer, RegistryPath->Buffer, SavedRegPath.Length);
        SavedRegPath.Buffer[SavedRegPath.Length/sizeof(WCHAR)] = 0;
    }

    SkipRaids = AtapiRegCheckDevValue(NULL, CHAN_NOT_SPECIFIED, DEVNUM_NOT_SPECIFIED, L"SkipRaids", 1);
    ForceSimplex = AtapiRegCheckDevValue(NULL, CHAN_NOT_SPECIFIED, DEVNUM_NOT_SPECIFIED, L"ForceSimplex", 0);
#ifdef _DEBUG
    g_LogToDisplay = AtapiRegCheckDevValue(NULL, CHAN_NOT_SPECIFIED, DEVNUM_NOT_SPECIFIED, L"LogToDisplay", 0);
#endif //_DEBUG

    statusToReturn = 0xffffffff;

    // Zero out structure.
    RtlZeroMemory(((PCHAR)&hwInitializationData), sizeof(hwInitializationData));

    // Set size of hwInitializationData.
    hwInitializationData.comm.HwInitializationDataSize =
      sizeof(hwInitializationData.comm) +
//      sizeof(hwInitializationData.nt4) +
      ((WinVer_Id() <= WinVer_NT) ? 0 : sizeof(hwInitializationData.w2k));

    // Set entry points.
    hwInitializationData.comm.HwInitialize = (PHW_INITIALIZE)AtapiHwInitialize;
    hwInitializationData.comm.HwResetBus = (PHW_RESET_BUS)AtapiResetController;
    hwInitializationData.comm.HwStartIo = (PHW_STARTIO)AtapiStartIo;
    hwInitializationData.comm.HwInterrupt = (PHW_INTERRUPT)AtapiInterrupt;

    // Specify size of extensions.
    hwInitializationData.comm.DeviceExtensionSize     = sizeof(HW_DEVICE_EXTENSION);
    hwInitializationData.comm.SpecificLuExtensionSize = sizeof(HW_LU_EXTENSION);
    hwInitializationData.comm.SrbExtensionSize        = sizeof(ATA_REQ);

    // Indicate PIO device.
    hwInitializationData.comm.MapBuffers = TRUE;
    // Set PnP-specific API
    if(WinVer_Id() > WinVer_NT) {
        hwInitializationData.comm.NeedPhysicalAddresses = TRUE;
        hwInitializationData.w2k.HwAdapterControl = (PHW_ADAPTER_CONTROL)AtapiAdapterControl;
    }

    KdPrint2((PRINT_PREFIX "\n\nATAPI IDE enum supported BusMaster Devices\n"));

    if(!ReEnter) {
        UniataEnumBusMasterController(DriverObject, Argument2);
    }

    // Look for legacy ISA-bridged PCI IDE controller (onboard)
    KdPrint2((PRINT_PREFIX "\n\nATAPI IDE: Look for legacy ISA-bridged PCI IDE controller (onboard)\n"));
    KdPrint2((PRINT_PREFIX "\n\nATAPI IDE: BMListLen %d\n", BMListLen));
    for (i=0; i <BMListLen; i++) {

        if(!BMList[i].MasterDev) {
            KdPrint2((PRINT_PREFIX "!BMList[i].MasterDev\n"));
            break;
        }
        if(AtapiRegCheckDevValue(NULL, CHAN_NOT_SPECIFIED, DEVNUM_NOT_SPECIFIED, L"IgnoreIsaCompatiblePci", 0)) {
            break;
        }
        if(ReEnter) {
            KdPrint2((PRINT_PREFIX "ReEnter, skip it\n"));
            if(BMList[i].ChanInitOk & 0x03) {
                KdPrint2((PRINT_PREFIX "Already initialized, skip it\n"));
                statusToReturn =
                newStatus = STATUS_SUCCESS;
            }
            continue;
        }
        BMList[i].AltInitMasterDev = (UCHAR)0xff;

        if(GlobalConfig->AtDiskPrimaryAddressClaimed)
            PrimaryClaimed = TRUE;
        if(GlobalConfig->AtDiskSecondaryAddressClaimed)
            SecondaryClaimed = TRUE;

        if(g_opt_Verbose) {
            _PrintNtConsole("Init standard Dual-channel PCI ATA controller:");
        }
        for(alt = 0; alt < 2; alt++) {

            for(c=0; c<2; c++) {

                if(AtapiRegCheckDevValue(NULL, c, DEVNUM_NOT_SPECIFIED, L"IgnoreIsaCompatiblePci", 0)) {
                    break;
                }
                if(c==0) {
                    if(PrimaryClaimed) {
                        KdPrint2((PRINT_PREFIX "Primary already claimed\n"));
                        continue;
                    }
                } else
                if(c==1) {
                    if(SecondaryClaimed) {
                        KdPrint2((PRINT_PREFIX "Secondary already claimed\n"));
                        continue;
                    }
                }

                if((WinVer_Id() <= WinVer_NT)) {
                    // do not even try if already claimed
                    if(c==0) {
                        GlobalConfig->AtDiskPrimaryAddressClaimed = FALSE;
                    } else
                    if(c==1) {
                        GlobalConfig->AtDiskSecondaryAddressClaimed = FALSE;
                    }
                }
                hwInitializationData.comm.HwFindAdapter = UniataFindBusMasterController;
                hwInitializationData.comm.NumberOfAccessRanges = 6;
                hwInitializationData.comm.AdapterInterfaceType = Isa;

                BMList[i].channel = (UCHAR)c;

                KdPrint2((PRINT_PREFIX "Try init channel %d, method %d\n", c, alt));
                newStatus = ScsiPortInitialize(DriverObject,
                                               Argument2,
                                               &hwInitializationData.comm,
                                               (PVOID)(i | (alt ? 0x80000000 : 0)));
                KdPrint2((PRINT_PREFIX "Status %#x\n", newStatus));
                if (newStatus < statusToReturn) {
                    statusToReturn = newStatus;
                }
                if (newStatus == STATUS_SUCCESS) {
                    BMList[i].ChanInitOk |= 0x01 << c;
/*
                    if(BMList[i].MasterDev && (WinVer_Id() > WinVer_NT)) {
                        c = 1; // this will break our for()
                        BMList[i].ChanInitOk |= 0x01 << c;
                    }
*/
                }
            }
            if(WinVer_Id() > WinVer_NT) {
                // the following doesn't work under higher OSes
                KdPrint2((PRINT_PREFIX "make still one attempt\n"));
                continue;
            }
            if(BMList[i].ChanInitOk & 0x03) {
                // under NT we receive status immediately, so
                // we can omit alternative init method id STATUS_SUCCESS returned
                KdPrint2((PRINT_PREFIX "Ok, no more retries required\n"));
                break;
            }
            // if (WinVer_Id() == WinVer_NT) and some error occured
            // try alternative init method
        }
        if(g_opt_Verbose) {
            if(BMList[i].ChanInitOk & 0x03) {
                _PrintNtConsole("  OK\n");
            } else {
                _PrintNtConsole("  failed\n");
            }
        }

    }

/*    KeBugCheckEx(0xc000000e,
                 (i << 16) | BMList[0].ChanInitOk,
                 c, 
                 newStatus, statusToReturn);*/

    // Look for PCI IDE controller
    KdPrint2((PRINT_PREFIX "\n\nATAPI IDE: Look for PCI IDE controller\n"));
    KdPrint2((PRINT_PREFIX "\n\nATAPI IDE: i %d, BMListLen %d\n", i, BMListLen));
    for (; i <BMListLen; i++) {

        if(AtapiRegCheckDevValue(NULL, CHAN_NOT_SPECIFIED, DEVNUM_NOT_SPECIFIED, L"IgnoreNativePci", 0)) {
            break;
        }
/*        if(BMList[i].MasterDev)
            continue;*/
        if(g_opt_Verbose) {
            _PrintNtConsole("Init PCI ATA controller Vendor/Dev %4.4s//%4.4s at PCI Address %d:%d:%d",
                BMList[i].VendorId, BMList[i].DeviceId,
                BMList[i].busNumber,
                BMList[i].slotNumber % PCI_MAX_FUNCTION,
                (BMList[i].slotNumber / PCI_MAX_FUNCTION) % PCI_MAX_DEVICES);
        }

        hwInitializationData.comm.HwFindAdapter = UniataFindBusMasterController;
        hwInitializationData.comm.NumberOfAccessRanges = 6;
        hwInitializationData.comm.AdapterInterfaceType = PCIBus;

        hwInitializationData.comm.VendorId             = BMList[i].VendorId;
        hwInitializationData.comm.VendorIdLength       = (USHORT) BMList[i].VendorIdLength;
        hwInitializationData.comm.DeviceId             = BMList[i].DeviceId;
        hwInitializationData.comm.DeviceIdLength       = (USHORT) BMList[i].DeviceIdLength;

        BMList[i].channel = 0/*(UCHAR)c*/;

        KdPrint2((PRINT_PREFIX "Try init %4.4s %4.4s \n",
                               hwInitializationData.comm.VendorId,
                               hwInitializationData.comm.DeviceId));
        newStatus = ScsiPortInitialize(DriverObject,
                                       Argument2,
                                       &hwInitializationData.comm,
                                       (PVOID)i);
        if (newStatus < statusToReturn)
            statusToReturn = newStatus;

        if(g_opt_Verbose) {
            if(newStatus == STATUS_SUCCESS) {
                _PrintNtConsole("  OK\n");
            } else {
                _PrintNtConsole("  failed\n");
            }
        }

    }

/*    KeBugCheckEx(0xc000000e,
                 i,
                 c, 
                 newStatus, statusToReturn);*/

    // --------------

    hwInitializationData.comm.VendorId             = 0;
    hwInitializationData.comm.VendorIdLength       = 0;
    hwInitializationData.comm.DeviceId             = 0;
    hwInitializationData.comm.DeviceIdLength       = 0;

    // The adapter count is used by the find adapter routine to track how
    // which adapter addresses have been tested.

    // Indicate 2 access ranges and reset FindAdapter.
    hwInitializationData.comm.NumberOfAccessRanges = 2;
    hwInitializationData.comm.HwFindAdapter = AtapiFindController;

    if(!AtapiRegCheckDevValue(NULL, CHAN_NOT_SPECIFIED, DEVNUM_NOT_SPECIFIED, L"IgnoreIsa", 0)) {
        // Indicate ISA bustype.
        hwInitializationData.comm.AdapterInterfaceType = Isa;
        adapterCount = 0;

        // Call initialization for ISA bustype.
        KdPrint2((PRINT_PREFIX "\n\nATAPI IDE: Look for ISA Controllers\n"));
        newStatus =  ScsiPortInitialize(DriverObject,
                                        Argument2,
                                        &hwInitializationData.comm,
                                        &adapterCount);
        if (newStatus < statusToReturn)
            statusToReturn = newStatus;
    }
    if(!AtapiRegCheckDevValue(NULL, CHAN_NOT_SPECIFIED, DEVNUM_NOT_SPECIFIED, L"IgnoreMca", 0)) {
        // Set up for MCA
        KdPrint2((PRINT_PREFIX "\n\nATAPI IDE: Look for MCA Controllers\n"));
        hwInitializationData.comm.AdapterInterfaceType = MicroChannel;
        adapterCount = 0;

        newStatus =  ScsiPortInitialize(DriverObject,
                                        Argument2,
                                        &hwInitializationData.comm,
                                        &adapterCount);
        if (newStatus < statusToReturn)
            statusToReturn = newStatus;
    }
    InDriverEntry = FALSE;

    KdPrint2((PRINT_PREFIX "\n\nLeave ATAPI IDE MiniPort DriverEntry with status %#x\n", statusToReturn));

    return statusToReturn;

} // end DriverEntry()


PSCSI_REQUEST_BLOCK
BuildMechanismStatusSrb(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    PSCSI_REQUEST_BLOCK srb;
    PCDB cdb;
    PATA_REQ AtaReq = (PATA_REQ)(Srb->SrbExtension);

    srb = &(deviceExtension->chan[GET_CHANNEL(Srb)].InternalSrb);

    RtlZeroMemory((PCHAR) srb, sizeof(SCSI_REQUEST_BLOCK));

    srb->PathId     = (UCHAR)(Srb->PathId);
    srb->TargetId   = (UCHAR)(Srb->TargetId);
    srb->Function   = SRB_FUNCTION_EXECUTE_SCSI;
    srb->Length     = sizeof(SCSI_REQUEST_BLOCK);

    // Set flags to disable synchronous negociation.
    srb->SrbFlags = SRB_FLAGS_DATA_IN | SRB_FLAGS_DISABLE_SYNCH_TRANSFER;

    // Set timeout to 4 seconds.
    srb->TimeOutValue = 4;

    srb->CdbLength          = 6;
    srb->DataBuffer         = &(deviceExtension->chan[GET_CHANNEL(Srb)].MechStatusData);
    srb->DataTransferLength = sizeof(MECHANICAL_STATUS_INFORMATION_HEADER);
    srb->SrbExtension       = AtaReq;

    // Set CDB operation code.
    cdb = (PCDB)srb->Cdb;
    cdb->MECH_STATUS.OperationCode       = SCSIOP_MECHANISM_STATUS;
    cdb->MECH_STATUS.AllocationLength[1] = sizeof(MECHANICAL_STATUS_INFORMATION_HEADER);

    return srb;
} // end BuildMechanismStatusSrb()

#endif //UNIATA_CORE

PSCSI_REQUEST_BLOCK
BuildRequestSenseSrb (
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    PSCSI_REQUEST_BLOCK srb;
    PCDB cdb;
    PATA_REQ AtaReq = (PATA_REQ)(Srb->SrbExtension);

    srb = &(deviceExtension->chan[GET_CHANNEL(Srb)].InternalSrb);

    RtlZeroMemory((PCHAR) srb, sizeof(SCSI_REQUEST_BLOCK));

    srb->PathId     = (UCHAR)(Srb->PathId);
    srb->TargetId   = (UCHAR)(Srb->TargetId);
    srb->Function   = SRB_FUNCTION_EXECUTE_SCSI;
    srb->Length     = sizeof(SCSI_REQUEST_BLOCK);

    // Set flags to disable synchronous negociation.
    srb->SrbFlags = SRB_FLAGS_DATA_IN | SRB_FLAGS_DISABLE_SYNCH_TRANSFER;

    // Set timeout to 2 seconds.
    srb->TimeOutValue = 4;

    srb->CdbLength          = 6;
    srb->DataBuffer         = &(deviceExtension->chan[GET_CHANNEL(Srb)].MechStatusSense);
    srb->DataTransferLength = sizeof(SENSE_DATA);
    srb->SrbExtension       = AtaReq;

    // Set CDB operation code.
    cdb = (PCDB)srb->Cdb;
    cdb->CDB6INQUIRY.OperationCode    = SCSIOP_REQUEST_SENSE;
    cdb->CDB6INQUIRY.AllocationLength = sizeof(SENSE_DATA);

    return srb;
} // end BuildRequestSenseSrb()

#ifndef UNIATA_CORE

ULONG
AtapiRegCheckDevLunValue(
    IN PVOID HwDeviceExtension,
    IN PWCHAR NamePrefix,
    IN ULONG chan,
    IN ULONG dev,
    IN PWSTR Name,
    IN ULONG Default
    )
{
    WCHAR namex[160];
    ULONG val = Default;

    val = AtapiRegCheckParameterValue(
        HwDeviceExtension, NamePrefix, Name, val);

    if(chan != CHAN_NOT_SPECIFIED) {
        swprintf(namex, L"%s\\Chan_%1.1d", NamePrefix, chan);
        val = AtapiRegCheckParameterValue(
            HwDeviceExtension, namex, Name, val);
        if(dev != DEVNUM_NOT_SPECIFIED) {
            swprintf(namex, L"%s\\Chan_%1.1d\\%s", NamePrefix, chan, (dev & 0x01) ? L"Lun_1" : L"Lun_0");
            val = AtapiRegCheckParameterValue(
                HwDeviceExtension, namex, Name, val);
        }
    }
    return val;
} // end AtapiRegCheckDevLunValue()

ULONG
EncodeVendorStr(
   OUT PWCHAR Buffer,
    IN PUCHAR Str,
    IN ULONG  Length
    )
{
    ULONG i,j;
    WCHAR a;

    for(i=0, j=0; i<Length; i++, j++) {
        // fix byte-order
        a = Str[i ^ 0x01];
        if(!a) {
            Buffer[j] = 0;
            return j;
        } else
        if(a == ' ') {
            Buffer[j] = '_';
        } else
        if((a == '_') ||
           (a == '#') ||
           (a == '\\') ||
           (a == '\"') ||
           (a == '\'') ||
           (a <  ' ') ||
           (a >= 127)) {
            Buffer[j] = '#';
            j++;
            swprintf(Buffer+j, L"%2.2x", a);
            j++;
        } else {
            Buffer[j] = a;
        }
    }
    Buffer[j] = 0;
    return j;
} // end EncodeVendorStr()

ULONG
AtapiRegCheckDevValue(
    IN PVOID HwDeviceExtension,
    IN ULONG chan,
    IN ULONG dev,
    IN PWSTR Name,
    IN ULONG Default
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
//    WCHAR name0[11];
//    WCHAR name1[11+4+5];
//    WCHAR name2[11+4+4+10];
//    WCHAR name3[11+4+4+5+20];
//    WCHAR name3[11+4+4+5+20+1];
    WCHAR namex[160];

    WCHAR namev[16];
    WCHAR named[16];
    WCHAR names[20];

    IN ULONG VendorID;
    IN ULONG DeviceID;
    IN ULONG SlotNumber;

    ULONG val = Default;

    KdPrint(( " Parameter %ws\n", Name));

    if(deviceExtension) {
        VendorID   =  deviceExtension->DevID        & 0xffff;
        DeviceID   = (deviceExtension->DevID >> 16) & 0xffff;
        SlotNumber = deviceExtension->slotNumber;
    } else {
        VendorID   = 0xffff;
        DeviceID   = 0xffff;
        SlotNumber = 0xffffffff;
    }

    val = AtapiRegCheckDevLunValue(
        HwDeviceExtension, L"Parameters", chan, dev, Name, val);

    if(deviceExtension) {
        swprintf(namev, L"\\Ven_%4.4x", VendorID);
        swprintf(named, L"\\Dev_%4.4x", DeviceID);
        swprintf(names, L"\\Slot_%8.8x", SlotNumber);

        swprintf(namex, L"Parameters%s", namev);
        val = AtapiRegCheckDevLunValue(
            HwDeviceExtension, namex, chan, dev, Name, val);

        swprintf(namex, L"Parameters%s%s", namev, named);
        val = AtapiRegCheckDevLunValue(
            HwDeviceExtension, namex, chan, dev, Name, val);

        swprintf(namex, L"Parameters%s%s%s", namev, named, names);
        val = AtapiRegCheckDevLunValue(
            HwDeviceExtension, namex, chan, dev, Name, val);
    }

    KdPrint(( " Parameter %ws = %#x\n", Name, val));
    return val;

} // end AtapiRegCheckDevValue()

/*
    The user must specify that Xxx is to run on the platform
    by setting the registry value HKEY_LOCAL_MACHINE\System\CurrentControlSet\
    Services\UniATA\Xxx:REG_DWORD:Zzz.

    The user can override the global setting to enable or disable Xxx on a
    specific cdrom device by setting the key HKEY_LOCAL_MACHINE\System\
    CurrentControlSet\Services\UniATA\Parameters\Device<N>\Xxx:REG_DWORD to one or zero.

    If this registry value does not exist or contains the value zero then
    the timer to check for media change does not run.

    Arguments:

    RegistryPath - pointer to the unicode string inside
                   ...\CurrentControlSet\Services\UniATA
    DeviceNumber - The number of the HBA device object

    Returns:    Registry Key value
 */
ULONG
AtapiRegCheckParameterValue(
    IN PVOID HwDeviceExtension,
    IN PWSTR PathSuffix,
    IN PWSTR Name,
    IN ULONG Default
    )
{
#define ITEMS_TO_QUERY 2 // always 1 greater than what is searched 

//    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    NTSTATUS          status;
    LONG              zero = Default;

    RTL_QUERY_REGISTRY_TABLE parameters[ITEMS_TO_QUERY];

//    LONG              tmp = 0;
    LONG              doRun = Default;

    PUNICODE_STRING   RegistryPath = &SavedRegPath;

    UNICODE_STRING    paramPath;

    // <SavedRegPath>\<PathSuffix> -> <Name>
//    KdPrint(( "AtapiCheckRegValue: %ws -> %ws\n", PathSuffix, Name));
//    KdPrint(( "AtapiCheckRegValue: RegistryPath %ws\n", RegistryPath->Buffer));

    paramPath.Length = 0;
    paramPath.MaximumLength = RegistryPath->Length +
        (wcslen(PathSuffix)+2)*sizeof(WCHAR);
    paramPath.Buffer = (PWCHAR)ExAllocatePool(NonPagedPool, paramPath.MaximumLength);
    if(!paramPath.Buffer) {
        KdPrint(("AtapiCheckRegValue: couldn't allocate paramPath\n"));
        return Default;
    }

    RtlZeroMemory(paramPath.Buffer, paramPath.MaximumLength);
    RtlAppendUnicodeToString(&paramPath, RegistryPath->Buffer);
    RtlAppendUnicodeToString(&paramPath, L"\\");
    RtlAppendUnicodeToString(&paramPath, PathSuffix);

    // Check for the Xxx value.
    RtlZeroMemory(parameters, (sizeof(RTL_QUERY_REGISTRY_TABLE)*ITEMS_TO_QUERY));

    parameters[0].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    parameters[0].Name          = Name;
    parameters[0].EntryContext  = &doRun;
    parameters[0].DefaultType   = REG_DWORD;
    parameters[0].DefaultData   = &zero;
    parameters[0].DefaultLength = sizeof(ULONG);

    status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE /*| RTL_REGISTRY_OPTIONAL*/,
                                    paramPath.Buffer, parameters, NULL, NULL);
    KdPrint(( "AtapiCheckRegValue: %ws -> %ws is %#x\n", PathSuffix, Name, doRun));

    ExFreePool(paramPath.Buffer);

    if(!NT_SUCCESS(status)) {
        doRun = Default;
    }

    return doRun;

#undef ITEMS_TO_QUERY

} // end AtapiRegCheckParameterValue()


SCSI_ADAPTER_CONTROL_STATUS
DDKAPI
AtapiAdapterControl(
    IN PVOID HwDeviceExtension,
    IN SCSI_ADAPTER_CONTROL_TYPE ControlType,
    IN PVOID Parameters
    )
{
    PHW_DEVICE_EXTENSION deviceExtension = (PHW_DEVICE_EXTENSION)HwDeviceExtension;
    PSCSI_SUPPORTED_CONTROL_TYPE_LIST pControlTypeList;
    ULONG                numberChannels  = deviceExtension->NumberChannels;
    ULONG c;
    NTSTATUS status;

    KdPrint(( "AtapiAdapterControl: %#x\n", ControlType));

    switch(ControlType) {
        case ScsiQuerySupportedControlTypes: {
            BOOLEAN supportedTypes[ScsiAdapterControlMax] = {
                TRUE,       // ScsiQuerySupportedControlTypes
                TRUE,       // ScsiStopAdapter
                TRUE,       // ScsiRestartAdapter
                FALSE,      // ScsiSetBootConfig
                FALSE       // ScsiSetRunningConfig
            };

            ULONG lim = ScsiAdapterControlMax;
            ULONG i;

            pControlTypeList = (PSCSI_SUPPORTED_CONTROL_TYPE_LIST) Parameters;

            if(pControlTypeList->MaxControlType < lim) {
                lim = pControlTypeList->MaxControlType;
            }

            for(i = 0; i < lim; i++) {
                pControlTypeList->SupportedTypeList[i] = supportedTypes[i];
            }

            break;

        }
        case ScsiStopAdapter: {

            KdPrint(( "AtapiAdapterControl: ScsiStopAdapter\n"));
            // Shut down all interrupts on the adapter.  They'll get re-enabled
            // by the initialization routines.
            for (c = 0; c < numberChannels; c++) {
                AtapiResetController(deviceExtension, c);
                AtapiDisableInterrupts(deviceExtension, c);
            }
            status = UniataDisconnectIntr2(HwDeviceExtension);
            BMList[deviceExtension->DevIndex].Isr2Enable = FALSE;
            break;
        }
        case ScsiRestartAdapter: {

            KdPrint(( "AtapiAdapterControl: ScsiRestartAdapter\n"));
            // Enable all the interrupts on the adapter while port driver call
            // for power up an HBA that was shut down for power management

            AtapiChipInit(HwDeviceExtension, DEVNUM_NOT_SPECIFIED, CHAN_NOT_SPECIFIED);
            status = UniataConnectIntr2(HwDeviceExtension);
            for (c = 0; c < numberChannels; c++) {
                AtapiChipInit(HwDeviceExtension, DEVNUM_NOT_SPECIFIED, c);
                FindDevices(HwDeviceExtension, FALSE, c);
                AtapiEnableInterrupts(deviceExtension, c);
                AtapiHwInitialize__(deviceExtension, c);
            }
            if(deviceExtension->Isr2DevObj) {
                BMList[deviceExtension->DevIndex].Isr2Enable = TRUE;
            }

            break;
        }

        default: {
            KdPrint(( "AtapiAdapterControl: default => return ScsiAdapterControlUnsuccessful\n"));
            return ScsiAdapterControlUnsuccessful;
        }
    }

    return ScsiAdapterControlSuccess;
} // end AtapiAdapterControl()

#endif //UNIATA_CORE

extern "C"
NTHALAPI
VOID
DDKAPI
HalDisplayString (
    PUCHAR String
    );

extern "C"
VOID
_cdecl
_PrintNtConsole(
    PCHAR DebugMessage,
    ...
    )
{
    int len;
    UCHAR dbg_print_tmp_buff[512];
//    UNICODE_STRING msgBuff;
    va_list ap;
    va_start(ap, DebugMessage);

    len = _vsnprintf((PCHAR)&dbg_print_tmp_buff[0], 511, DebugMessage, ap);

    dbg_print_tmp_buff[511] = 0;

    HalDisplayString(dbg_print_tmp_buff);

    va_end(ap);

} // end PrintNtConsole()

