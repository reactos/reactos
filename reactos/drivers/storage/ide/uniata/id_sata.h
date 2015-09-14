/*++

Copyright (c) 2008-2012 Alexandr A. Telyatnikov (Alter)

Module Name:
    id_probe.cpp

Abstract:
    This module handles SATA-related staff

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

--*/

#ifndef __UNIATA_SATA__H__
#define __UNIATA_SATA__H__

UCHAR
NTAPI
UniataSataConnect(
    IN PVOID HwDeviceExtension,
    IN ULONG lChannel,          // logical channel
    IN ULONG pm_port = 0 /* for port multipliers */
    );

#define UNIATA_SATA_RESET_ENABLE   TRUE
#define UNIATA_SATA_FAST_ENABLE    FALSE

UCHAR
NTAPI
UniataSataPhyEnable(
    IN PVOID HwDeviceExtension,
    IN ULONG lChannel,          // logical channel
    IN ULONG pm_port = 0, /* for port multipliers */
    IN BOOLEAN doReset = UNIATA_SATA_FAST_ENABLE
    );

#define UNIATA_SATA_DO_CONNECT        TRUE
#define UNIATA_SATA_IGNORE_CONNECT    FALSE

BOOLEAN
NTAPI
UniataSataClearErr(
    IN PVOID HwDeviceExtension,
    IN ULONG lChannel,          // logical channel
    IN BOOLEAN do_connect,
    IN ULONG pm_port = 0 /* for port multipliers */
    );

#define UNIATA_SATA_EVENT_ATTACH      0x01
#define UNIATA_SATA_EVENT_DETACH      0x02

BOOLEAN
NTAPI
UniataSataEvent(
    IN PVOID HwDeviceExtension,
    IN ULONG lChannel,          // logical channel
    IN ULONG Action,
    IN ULONG pm_port = 0 /* for port multipliers */
    );
/*
#define UniataIsSATARangeAvailable(deviceExtension, lChannel) \
    ((deviceExtension->BaseIoAddressSATA_0.Addr || \
      deviceExtension->BaseIoAHCI_0.Addr) && \
        (deviceExtension->chan[lChannel].RegTranslation[IDX_SATA_SStatus].Addr))
*/
__inline
BOOLEAN
UniataIsSATARangeAvailable(
    IN PHW_DEVICE_EXTENSION deviceExtension,
    IN ULONG lChannel
    )
{
    // seems, check for deviceExtension->BaseIoAddressSATA_0.Addr and
    // deviceExtension->BaseIoAHCI_0.Addr is not necessary now
    if(deviceExtension->chan[lChannel].RegTranslation[IDX_SATA_SStatus].Addr ||
       deviceExtension->chan[lChannel].RegTranslation[IDX_SATA_SStatus].Proc) {
        return TRUE;
    }
    return FALSE;
} // end UniataIsSATARangeAvailable()


ULONG
NTAPI
UniataSataReadPort4(
    IN PHW_CHANNEL chan,
    IN ULONG io_port_ndx,
    IN ULONG pm_port=0 /* for port multipliers */
    );

VOID
NTAPI
UniataSataWritePort4(
    IN PHW_CHANNEL chan,
    IN ULONG io_port_ndx,
    IN ULONG data,
    IN ULONG pm_port=0 /* for port multipliers */
    );

BOOLEAN
NTAPI
UniataAhciInit(
    IN PVOID HwDeviceExtension
    );

#ifdef _DEBUG
VOID
NTAPI
UniataDumpAhciPortRegs(
    IN PHW_CHANNEL chan
    );
#endif

BOOLEAN
NTAPI
UniataAhciDetect(
    IN PVOID HwDeviceExtension,
    IN PPCI_COMMON_CONFIG pciData, // optional
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo
    );

UCHAR
NTAPI
UniataAhciStatus(
    IN PVOID HwDeviceExtension,
    IN ULONG lChannel,
    IN ULONG DeviceNumber
    );

VOID
NTAPI
UniataAhciSnapAtaRegs(
    IN PHW_CHANNEL chan,
    IN ULONG DeviceNumber,
 IN OUT PIDEREGS_EX regs
    );

ULONG
NTAPI
UniataAhciSetupFIS_H2D(
    IN PHW_DEVICE_EXTENSION deviceExtension,
    IN ULONG DeviceNumber,
    IN ULONG lChannel,
   OUT PUCHAR fis,
    IN UCHAR command,
    IN ULONGLONG lba,
    IN USHORT count,
    IN USHORT feature
    );

UCHAR
NTAPI
UniataAhciWaitCommandReady(
    IN PHW_CHANNEL chan,
    IN ULONG timeout
    );

UCHAR
NTAPI
UniataAhciSendCommand(
    IN PVOID HwDeviceExtension,
    IN ULONG lChannel,
    IN ULONG DeviceNumber,
    IN USHORT ahci_flags,
    IN ULONG timeout
    );

UCHAR
NTAPI
UniataAhciSendPIOCommand(
    IN PVOID HwDeviceExtension,
    IN ULONG lChannel,
    IN ULONG DeviceNumber,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PUCHAR data,
    IN ULONG length,
    IN UCHAR command,
    IN ULONGLONG lba,
    IN USHORT count,
    IN USHORT feature,
    IN USHORT ahci_flags,
    IN ULONG flags,
    IN ULONG timeout
    );

UCHAR
NTAPI
UniataAhciSendPIOCommandDirect(
    IN PVOID HwDeviceExtension,
    IN ULONG lChannel,
    IN ULONG DeviceNumber,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PIDEREGS_EX regs,
    IN ULONG wait_flags,
    IN ULONG timeout
    );

BOOLEAN
NTAPI
UniataAhciAbortOperation(
    IN PHW_CHANNEL chan
    );

ULONG
NTAPI
UniataAhciSoftReset(
    IN PVOID HwDeviceExtension,
    IN ULONG lChannel,
    IN ULONG DeviceNumber
    );

ULONG
NTAPI
UniataAhciWaitReady(
    IN PHW_CHANNEL chan,
    IN ULONG timeout
    );

ULONG
NTAPI
UniataAhciHardReset(
    IN PVOID HwDeviceExtension,
    IN ULONG lChannel,
   OUT PULONG signature
    );

VOID
NTAPI
UniataAhciReset(
    IN PVOID HwDeviceExtension,
    IN ULONG lChannel
    );

VOID
NTAPI
UniataAhciStartFR(
    IN PHW_CHANNEL chan
    );

BOOLEAN
NTAPI
UniataAhciStopFR(
    IN PHW_CHANNEL chan
    );

VOID
NTAPI
UniataAhciStart(
    IN PHW_CHANNEL chan
    );

BOOLEAN
NTAPI
UniataAhciCLO(
    IN PHW_CHANNEL chan
    );

BOOLEAN
NTAPI
UniataAhciStop(
    IN PHW_CHANNEL chan
    );


__inline
ULONG
UniataAhciReadChannelPort4(
    IN PHW_CHANNEL chan,
    IN ULONG io_port_ndx
    )
{
    ULONG v = AtapiReadPortEx4(NULL, (ULONGIO_PTR)&((chan)->BaseIoAHCI_Port), io_port_ndx);
    KdPrint3((PRINT_PREFIX "ReadChannelPort4 ch%d[%x] = %x\n", chan->lChannel, io_port_ndx, v));
    return v;
} // end UniataAhciReadChannelPort4()

__inline
VOID
UniataAhciWriteChannelPort4(
    IN PHW_CHANNEL chan,
    IN ULONG io_port_ndx,
    IN ULONG data
    )
{
    KdPrint3((PRINT_PREFIX "WriteChannelPort4 %x => ch%d[%x]\n", data, chan->lChannel, io_port_ndx));
    AtapiWritePortEx4(NULL, (ULONGIO_PTR)&((chan)->BaseIoAHCI_Port), io_port_ndx, data);
} // end UniataAhciWriteChannelPort4()


#define UniataAhciReadHostPort4(deviceExtension, io_port_ndx) \
    AtapiReadPortEx4(NULL, (ULONGIO_PTR)&((deviceExtension)->BaseIoAHCI_0), io_port_ndx)

#define UniataAhciWriteHostPort4(deviceExtension, io_port_ndx, data) \
    AtapiWritePortEx4(NULL, (ULONGIO_PTR)&((deviceExtension)->BaseIoAHCI_0), io_port_ndx, data)

UCHAR
NTAPI
UniataAhciBeginTransaction(
    IN PVOID HwDeviceExtension,
    IN ULONG lChannel,
    IN ULONG DeviceNumber,
    IN PSCSI_REQUEST_BLOCK Srb
    );

UCHAR
NTAPI
UniataAhciEndTransaction(
    IN PVOID HwDeviceExtension,
    IN ULONG lChannel,
    IN ULONG DeviceNumber,
    IN PSCSI_REQUEST_BLOCK Srb
    );

VOID
NTAPI
UniataAhciResume(
    IN PHW_CHANNEL chan
    );

__inline
ULONG
UniataAhciUlongFromRFIS(
    PUCHAR RCV_FIS
    )
{
    return ( (((ULONG)(RCV_FIS[6])) << 24) |
	     (((ULONG)(RCV_FIS[5])) << 16) |
	     (((ULONG)(RCV_FIS[4])) << 8) |
	      ((ULONG)(RCV_FIS[12])) );
} // end UniataAhciUlongFromRFIS()

__inline
USHORT
UniAtaAhciAdjustIoFlags(
    IN UCHAR command,
    IN USHORT ahci_flags,
    IN ULONG fis_size,
    IN ULONG DeviceNumber
    )
{
    ahci_flags |= (fis_size / sizeof(ULONG)) | (DeviceNumber << 12);
    if(!command) {
        return ahci_flags;
    }

    if(AtaCommandFlags[command] & ATA_CMD_FLAG_Out) {
        ahci_flags |= ATA_AHCI_CMD_WRITE;
    }
/*
    if(AtaCommandFlags[command] & ATA_CMD_FLAG_In) {
        ahci_flags |= ATA_AHCI_CMD_READ;
    }
*/
    return ahci_flags;
} // end UniAtaAhciAdjustIoFlags()

BOOLEAN
NTAPI
UniataAhciReadPM(
    IN PHW_CHANNEL chan,
    IN ULONG DeviceNumber,
    IN ULONG Reg,
   OUT PULONG result
    );

UCHAR
NTAPI
UniataAhciWritePM(
    IN PHW_CHANNEL chan,
    IN ULONG DeviceNumber,
    IN ULONG Reg,
    IN ULONG value
    );

VOID
UniataAhciSetupCmdPtr(
IN OUT PATA_REQ AtaReq
    );

PSCSI_REQUEST_BLOCK
NTAPI
BuildAhciInternalSrb (
    IN PVOID HwDeviceExtension,
    IN ULONG DeviceNumber,
    IN ULONG lChannel,
    IN PUCHAR Buffer = NULL,
    IN ULONG Length = 0
    );

__inline
BOOLEAN
UniataAhciChanImplemented(
    IN PHW_DEVICE_EXTENSION deviceExtension,
    IN ULONG c
    )
{
#ifdef DBG
    KdPrint2((PRINT_PREFIX "imp: %#x & %#x\n", (deviceExtension)->AHCI_PI, (1<<c) ));
#endif
    return (((deviceExtension)->AHCI_PI) & ((ULONG)1 << c)) ? TRUE : FALSE;
} // end UniataAhciChanImplemented()


#endif //__UNIATA_SATA__H__
