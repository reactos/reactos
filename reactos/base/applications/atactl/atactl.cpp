#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winioctl.h>
#include <stdlib.h>
//#include <ntdddisk.h>
//#include <ntddscsi.h>
#include <ntddscsi.h>
#include <atapi.h>
#include <bm_devs.h>
#include <uata_ctl.h>
#include <tools.h>
#include <uniata_ver.h>

#include "helper.h"

#define DEFAULT_REMOVAL_LOCK_TIMEOUT 20

#define MOV_DW_SWP(a,b)                                                  \
do                                                                       \
{                                                                        \
    *(unsigned short *)&(a) = _byteswap_ushort(*(unsigned short *)&(b)); \
}                                                                        \
while (0)

#define MOV_DD_SWP(a,b)           \
{                                 \
    PFOUR_BYTE _from_, _to_;      \
    _from_ = ((PFOUR_BYTE)&(b));  \
    _to_ =   ((PFOUR_BYTE)&(a));  \
    __asm mov ebx,_from_          \
    __asm mov eax,[ebx]           \
    __asm bswap eax               \
    __asm mov ebx,_to_            \
    __asm mov [ebx],eax           \
}

int g_extended = 0;
int g_adapter_info = 0;
char* g_bb_list = NULL;
int gRadix = 16;
PADAPTERINFO g_AdapterInfo = NULL;

BOOLEAN
ata_power_mode(
    int bus_id,
    int dev_id,
    int power_mode
    );

void print_help() {
    printf("Usage:\n"
           "  atactl -<switches> c|s<controller id>:b<bus id>:d<device id>[:l<lun>]\n"
           "Switches:\n"
           "  l         (L)ist devices on SCSI and ATA controllers bus(es)\n"
           "              Note: ATA Pri/Sec controller are usually represented\n"
           "              as Scsi0/Scsi1 under NT-family OSes\n"
           "  x         show e(X)tended info\n"
           "  a         show (A)dapter info\n"
           "  s         (S)can for new devices on ATA/SATA bus(es) (experimental)\n"
           "  S         (S)can for new devices on ATA/SATA bus(es) (experimental)\n"
           "              device, hidden with 'H' can be redetected\n"
           "  h         (H)ide device on ATA/SATA bus for removal (experimental)\n"
           "              device can be redetected\n"
           "  H         (H)ide device on ATA/SATA bus (experimental)\n"
           "              device can not be redetected until 'h' or 'S' is issued\n"
           "  m [MODE]  set i/o (M)ode for device or revert to default\n"
           "              available MODEs are PIO, PIO0-PIO5, DMA, WDMA0-WDMA2,\n"
           "              UDMA33/44/66/100/133, UDMA0-UDMA5\n"
           "  d [XXX]   lock ATA/SATA bus for device removal for XXX seconds or\n"
           "              for %d seconds if no lock timeout specified.\n"
           "              can be used with -h, -m or standalone.\n"
           "  D [XXX]   disable device (turn into sleep mode) and lock ATA/SATA bus \n"
           "              for device removal for XXX seconds or\n"
           "              for %d seconds if no lock timeout specified.\n"
           "              can be used with -h, -m or standalone.\n"
           "  pX        change power state to X, where X is\n"
           "              0 - active, 1 - idle, 2 - standby, 3 - sleep\n"
           "  r         (R)eset device\n"
           "  ba        (A)ssign (B)ad-block list\n"
           "  bl        get assigned (B)ad-block (L)ist\n"
           "  br        (R)eset assigned (B)ad-block list\n"
           "  f         specify (F)ile for bad-block list\n"
           "  n XXX     block (n)ubmering radix. XXX can be hex or dec\n"
           "------\n"
           "Examples:\n"
           "  atactl -l\n"
           "    will list all scsi buses and all connected devices\n"
           "  atactl -m udma0 s2:b1:d1\n"
           "    will switch device at Scsi2, bus 1, taget_id 1 to UDMA0 mode\n"
           "  atactl -h -d 30 c1:b0:d0:l0 \n"
           "    will hide Master (d0:l0) device on secondary (c1:b0) IDE channel\n"
           "    and lock i/o on this channel for 30 seconds to ensure safety\n"
           "    of removal process"
           "------\n"
           "Device address format:\n"
           "\n"
           "s<controller id> number of controller in system. Is assigned during hardware\n"
           "                   detection. Usually s0/s1 are ATA Pri/Sec.\n"
           "                   Note, due do NT internal design ATA controllers are represented\n"
           "                   as SCSI controllers.\n"
           "b<bus id>        For ATA controllers it is channel number.\n"
           "                   Note, usually onboard controller is represented as 2 legacy\n"
           "                   ISA-compatible single-channel controllers (Scsi9/Scsi1). Additional\n"
           "                   ATA, ATA-RAID and some specific onboard controllers are represented\n"
           "                   as multichannel controllers.\n"
           "d<device id>     For ATA controllers d0 is Master, d1 is Slave.\n"
           "l<lun>           Not used in ATA controller drivers, alway 0\n"
           "------\n"
           "Bad-block list format:\n"
           "\n"
           "# Comment\n"
           "; Still one comment\n"
           "hex: switch to hexadecimal mode\n"
           "<Bad Area 1 Start LBA, e.g. FD50> <Block count 1, e.g. 60>\n"
           "<Bad Area 2 Start LBA> <Block count 2>\n"
           "...\n"
           "dec: switch to decimal mode\n"
           "<Bad Area N Start LBA, e.g. 16384> <Block count N, e.g. 48>\n"
           "...\n"
           "------\n"
           "",
           DEFAULT_REMOVAL_LOCK_TIMEOUT,
           DEFAULT_REMOVAL_LOCK_TIMEOUT
           );
    exit(0);
}

#define CMD_ATA_LIST  0x01
#define CMD_ATA_FIND  0x02
#define CMD_ATA_HIDE  0x03
#define CMD_ATA_MODE  0x04
#define CMD_ATA_RESET 0x05
#define CMD_ATA_BBLK  0x06
#define CMD_ATA_POWER 0x07

HANDLE
ata_open_dev(
    char* Name
    )
{
    ULONG i;
    HANDLE h;

    for(i=0; i<4; i++) {
        h = CreateFile(Name,
                       READ_CONTROL | GENERIC_READ | GENERIC_WRITE ,
                       ((i & 1) ? 0 : FILE_SHARE_READ) | ((i & 2) ? 0 : FILE_SHARE_WRITE),
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);
        if(h && (h != ((HANDLE)(-1))) ) {
            return h;
        }
    }

    for(i=0; i<4; i++) {
        h = CreateFile(Name,
                       GENERIC_READ | GENERIC_WRITE ,
                       ((i & 1) ? 0 : FILE_SHARE_READ) | ((i & 2) ? 0 : FILE_SHARE_WRITE),
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);
        if(h && (h != ((HANDLE)(-1))) ) {
            return h;
        }
    }

    for(i=0; i<4; i++) {
        h = CreateFile(Name,
                          GENERIC_READ,
                          ((i & 1) ? 0 : FILE_SHARE_READ) | ((i & 2) ? 0 : FILE_SHARE_WRITE),
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);
        if(h && (h != ((HANDLE)(-1))) ) {
            return h;
        }
    }

    for(i=0; i<4; i++) {
        h = CreateFile(Name,
                       READ_CONTROL,
                       ((i & 1) ? 0 : FILE_SHARE_READ) | ((i & 2) ? 0 : FILE_SHARE_WRITE),
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);
        if(h && (h != ((HANDLE)(-1))) ) {
            return h;
        }
    }

    return NULL;
} // end ata_open_dev()

HANDLE
ata_open_file(
    char* Name,
    BOOLEAN create
    )
{
    ULONG i;
    HANDLE h;

    if(!Name) {
        if(create) {
            return GetStdHandle(STD_OUTPUT_HANDLE);
        } else {
            return GetStdHandle(STD_INPUT_HANDLE);
        }
    }

    for(i=0; i<4; i++) {
        h = CreateFile(Name,
                       create ? GENERIC_WRITE : GENERIC_READ ,
                       ((i & 1) ? 0 : FILE_SHARE_READ) | ((i & 2) ? 0 : FILE_SHARE_WRITE),
                       NULL,
                       create ? CREATE_NEW : OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);
        if(h && (h != ((HANDLE)(-1))) ) {
            return h;
        }
    }

    return NULL;
} // end ata_open_file()

void
ata_close_dev(
    HANDLE h
    )
{
    CloseHandle(h);
} // end ata_close_dev()

int
ata_send_ioctl(
    HANDLE h,
    PSCSI_ADDRESS addr,
    PCCH   Signature,
    ULONG  Ioctl,
    PVOID  inBuffer,
    ULONG  inBufferLength,
    PVOID  outBuffer,
    ULONG  outBufferLength,
    PULONG returned
    )
{
    ULONG status;
    PUNIATA_CTL AtaCtl;
    ULONG data_len = max(inBufferLength, outBufferLength);
    ULONG len;

    if(addr) {
        len = data_len + offsetof(UNIATA_CTL, RawData);
    } else {
        len = data_len + sizeof(AtaCtl->hdr);
    }
    AtaCtl = (PUNIATA_CTL)GlobalAlloc(GMEM_FIXED, len);
    AtaCtl->hdr.HeaderLength = sizeof(SRB_IO_CONTROL);
    if(addr) {
        AtaCtl->hdr.Length = data_len + offsetof(UNIATA_CTL, RawData) - sizeof(AtaCtl->hdr);
    } else {
        AtaCtl->hdr.Length = data_len;
    }

    memcpy(&AtaCtl->hdr.Signature, Signature, 8);

    AtaCtl->hdr.Timeout = 10000;
    AtaCtl->hdr.ControlCode = Ioctl;
    AtaCtl->hdr.ReturnCode = 0;
    
    if(addr) {
        AtaCtl->addr = *addr;
        AtaCtl->addr.Length = sizeof(AtaCtl->addr);
    }

    if(outBufferLength) {
        if(addr) {
            memset(&AtaCtl->RawData, 0, outBufferLength);
        } else {
            memset(&AtaCtl->addr, 0, outBufferLength);
        }
    }

    if(inBuffer && inBufferLength) {
        if(addr) {
            memcpy(&AtaCtl->RawData, inBuffer, inBufferLength);
        } else {
            memcpy(&AtaCtl->addr, inBuffer, inBufferLength);
        }
    }

    status = DeviceIoControl(h,
                             IOCTL_SCSI_MINIPORT,
                             AtaCtl,
                             len,
                             AtaCtl,
                             len,
                             returned,
                             FALSE);

    if(outBuffer && outBufferLength) {
        if(addr) {
            memcpy(outBuffer, &AtaCtl->RawData, outBufferLength);
        } else {
            memcpy(outBuffer, &AtaCtl->addr, outBufferLength);
        }
    }
    GlobalFree(AtaCtl);

    if(!status) {
        status = GetLastError();
        return FALSE;
    }
    return TRUE;
} // end ata_send_ioctl()

int
ata_send_scsi(
    HANDLE h,
    PSCSI_ADDRESS addr,
    PCDB   cdb,
    UCHAR  cdbLength,
    PVOID  Buffer,
    ULONG  BufferLength,
    BOOLEAN DataIn,
    PSENSE_DATA senseData,
    PULONG returned
    )
{
    ULONG status;
    PSCSI_PASS_THROUGH_WITH_BUFFERS sptwb;
    //ULONG data_len = BufferLength;
    ULONG len;

    len = BufferLength + offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS, ucDataBuf);

    sptwb = (PSCSI_PASS_THROUGH_WITH_BUFFERS)GlobalAlloc(GMEM_FIXED, len);
    if(!sptwb) {
        return FALSE;
    }
    memset(sptwb, 0, offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS, ucDataBuf));

    sptwb->spt.Length = sizeof(SCSI_PASS_THROUGH);
    sptwb->spt.PathId   = addr->PathId;
    sptwb->spt.TargetId = addr->TargetId;
    sptwb->spt.Lun      = addr->Lun;
    sptwb->spt.CdbLength = cdbLength;
    sptwb->spt.SenseInfoLength = 24;
    sptwb->spt.DataIn = Buffer ? (DataIn ? SCSI_IOCTL_DATA_IN : SCSI_IOCTL_DATA_OUT) : 0;
    sptwb->spt.DataTransferLength = BufferLength;
    sptwb->spt.TimeOutValue = 10;
    sptwb->spt.DataBufferOffset =
       offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucDataBuf);
    sptwb->spt.SenseInfoOffset = 
       offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucSenseBuf);
    memcpy(&sptwb->spt.Cdb, cdb, cdbLength);

    if(Buffer && !DataIn) {
        memcpy(&sptwb->ucSenseBuf, Buffer, BufferLength);
    }

    status = DeviceIoControl(h,
                             IOCTL_SCSI_PASS_THROUGH,
                             sptwb,
                             (Buffer && !DataIn) ? len : sizeof(SCSI_PASS_THROUGH),
                             sptwb,
                             (Buffer && DataIn) ? len : offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS, ucDataBuf),
                             returned,
                             FALSE);

    if(Buffer && DataIn) {
        memcpy(Buffer, &sptwb->ucDataBuf, BufferLength);
    }
    if(senseData) {
        memcpy(senseData, &sptwb->ucSenseBuf, sizeof(sptwb->ucSenseBuf));
    }

    GlobalFree(sptwb);

    if(!status) {
        status = GetLastError();
        return FALSE;
    }
    return TRUE;
} // end ata_send_scsi()

IO_SCSI_CAPABILITIES g_capabilities;
UCHAR g_inquiry_buffer[2048];

void
ata_mode_to_str(
    char* str,
    int mode
    )
{
    if(mode > ATA_SA600) {
        sprintf(str, "SATA-600+");
    } else
    if(mode >= ATA_SA600) {
        sprintf(str, "SATA-600");
    } else
    if(mode >= ATA_SA300) {
        sprintf(str, "SATA-300");
    } else
    if(mode >= ATA_SA150) {
        sprintf(str, "SATA-150");
    } else
    if(mode >= ATA_UDMA0) {
        sprintf(str, "UDMA%d", mode-ATA_UDMA0);
    } else
    if(mode >= ATA_WDMA0) {
        sprintf(str, "WDMA%d", mode-ATA_WDMA0);
    } else
    if(mode >= ATA_SDMA0) {
        sprintf(str, "SDMA%d", mode-ATA_SDMA0);
    } else
    if(mode >= ATA_PIO0) {
        sprintf(str, "PIO%d", mode-ATA_PIO0);
    } else
    if(mode == ATA_PIO_NRDY) {
        sprintf(str, "PIO nRDY");
    } else
    {
        sprintf(str, "PIO");
    }
} // end ata_mode_to_str()

#define check_atamode_str(str, mode) \
   (!_stricmp(str, "UDMA" #mode) || \
    !_stricmp(str, "UDMA-" #mode) || \
    !_stricmp(str, "ATA-" #mode) || \
    !_stricmp(str, "ATA#" #mode))

int
ata_str_to_mode(
    char* str
    )
{
    int mode;
    int len;

    if(!_stricmp(str, "SATA600"))
        return ATA_SA600;
    if(!_stricmp(str, "SATA300"))
        return ATA_SA300;
    if(!_stricmp(str, "SATA150"))
        return ATA_SA150;
    if(!_stricmp(str, "SATA"))
        return ATA_SA150;

    if(check_atamode_str(str, 16))
        return ATA_UDMA0;
    if(check_atamode_str(str, 25))
        return ATA_UDMA1;
    if(check_atamode_str(str, 33))
        return ATA_UDMA2;
    if(check_atamode_str(str, 44))
        return ATA_UDMA3;
    if(check_atamode_str(str, 66))
        return ATA_UDMA4;
    if(check_atamode_str(str, 100))
        return ATA_UDMA5;
    if(check_atamode_str(str, 122))
        return ATA_UDMA6;

    len = strlen(str);

    if(len >= 4 && !_memicmp(str, "UDMA", 4)) {
        if(len == 4)
            return ATA_UDMA0;
        if(len > 5)
            return -1;
        mode = str[4] - '0';
        if(mode < 0 || mode > 7)
            return -1;
        return ATA_UDMA0+mode;
    }
    if(len >= 4 && !_memicmp(str, "WDMA", 4)) {
        if(len == 4)
            return ATA_WDMA0;
        if(len > 5)
            return -1;
        mode = str[4] - '0';
        if(mode < 0 || mode > 2)
            return -1;
        return ATA_WDMA0+mode;
    }
    if(len >= 4 && !_memicmp(str, "SDMA", 4)) {
        if(len == 4)
            return ATA_SDMA0;
        if(len > 5)
            return -1;
        mode = str[4] - '0';
        if(mode < 0 || mode > 2)
            return -1;
        return ATA_SDMA0+mode;
    }
    if(len == 4 && !_memicmp(str, "DMA", 4)) {
        return ATA_SDMA0;
    }
    if(len >= 3 && !_memicmp(str, "PIO", 3)) {
        if(len == 3)
            return ATA_PIO;
        if(len > 4)
            return -1;
        mode = str[3] - '0';
        if(mode < 0 || mode > 5)
            return -1;
        return ATA_PIO0+mode;
    }

    return -1;
} // end ata_str_to_mode()

ULONG
EncodeVendorStr(
   OUT char*  Buffer,
    IN PUCHAR Str,
    IN ULONG  Length,
    IN ULONG  Xorer
    )
{
    ULONG i,j;
    UCHAR a;

    for(i=0, j=0; i<Length; i++, j++) {
        a = Str[i ^ Xorer];
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
            sprintf(Buffer+j, "%2.2x", a);
            j++;
        } else {
            Buffer[j] = a;
        }
    }
    Buffer[j] = 0;
    return j;
} // end EncodeVendorStr()

HKEY
ata_get_bblist_regh(
    IN PIDENTIFY_DATA ident,
    OUT char* DevSerial,
    BOOLEAN read_only
    )
{
    HKEY hKey = NULL;
    HKEY hKey2 = NULL;
    ULONG Length;
    REGSAM access = read_only ? KEY_READ : KEY_ALL_ACCESS;

    Length = EncodeVendorStr(DevSerial, (PUCHAR)ident->ModelNumber, sizeof(ident->ModelNumber), 0x01);
    DevSerial[Length] = '-';
    Length++;
    Length += EncodeVendorStr(DevSerial+Length, ident->SerialNumber, sizeof(ident->SerialNumber), 0x01);

    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\UniATA", NULL, access, &hKey) != ERROR_SUCCESS) {
        hKey = NULL;
        goto exit;
    }
    if(RegOpenKey(hKey, "Parameters", &hKey2) != ERROR_SUCCESS) {
        hKey2 = NULL;
        if(read_only || (RegCreateKey(hKey, "Parameters", &hKey2) != ERROR_SUCCESS)) {
            hKey2 = NULL;
            goto exit;
        }
    }
    RegCloseKey(hKey2);
    if(RegOpenKey(hKey, "Parameters\\BadBlocks", &hKey2) != ERROR_SUCCESS) {
        hKey2 = NULL;
        if(read_only || (RegCreateKey(hKey, "Parameters\\BadBlocks", &hKey2) != ERROR_SUCCESS)) {
            hKey2 = NULL;
            goto exit;
        }
    }

exit:
    if(hKey)
        RegCloseKey(hKey);

    return hKey2;
} // end ata_get_bblist_regh()

IDENTIFY_DATA   g_ident;

int
ata_check_unit(
    HANDLE h,    // handle to ScsiXXX:
    int dev_id
    )
{
    ULONG status;
    ULONG returned;

    PSCSI_ADAPTER_BUS_INFO  adapterInfo;
    PSCSI_INQUIRY_DATA inquiryData;
    SCSI_ADDRESS addr;
    ULONG i, j;
    int l_dev_id;
    ULONG len;
    GETTRANSFERMODE IoMode;
    PSENDCMDOUTPARAMS pout;
    PIDENTIFY_DATA   ident;
    PINQUIRYDATA     scsi_ident;
    char buff[sizeof(SENDCMDOUTPARAMS)+/*sizeof(IDENTIFY_DATA)*/2048];
    char mode_str[12];
    //ULONG bus_id = (dev_id >> 24) & 0xff;
    BOOLEAN found = FALSE;
    SENDCMDINPARAMS pin;
    int io_mode = -1;
    char SerNum[128];
    char DevSerial[128];
    char lun_str[10];
    HKEY hKey2;
    ULONGLONG max_lba = -1;
    USHORT chs[3] = { 0 };

    if(dev_id != -1) {
        dev_id &= 0x00ffffff;
    }
    if(dev_id == 0x007f7f7f) {
        return TRUE;
    }

    pout = (PSENDCMDOUTPARAMS)buff;
    ident = (PIDENTIFY_DATA)&(pout->bBuffer);

    status = DeviceIoControl(h,
                             IOCTL_SCSI_GET_INQUIRY_DATA,
                             NULL,
                             0,
                             g_inquiry_buffer,
                             sizeof(g_inquiry_buffer),
                             &returned,
                             FALSE);

    if(!status) {
        printf("Can't get device info\n");
        return FALSE;
    }

    // Note: adapterInfo->NumberOfBuses is 1 greater than g_AdapterInfo->NumberChannels
    // because of virtual communication port
    adapterInfo = (PSCSI_ADAPTER_BUS_INFO)g_inquiry_buffer;
    for (i = 0; i+1 < adapterInfo->NumberOfBuses; i++) {
        inquiryData = (PSCSI_INQUIRY_DATA) (g_inquiry_buffer +
            adapterInfo->BusData[i].InquiryDataOffset);

        if(g_extended && g_AdapterInfo && g_AdapterInfo->ChanHeaderLengthValid &&
              g_AdapterInfo->NumberChannels < i) {
            PCHANINFO ChanInfo;

            ChanInfo = (PCHANINFO)
                         (((PCHAR)g_AdapterInfo)+
                            sizeof(ADAPTERINFO)+
                            g_AdapterInfo->ChanHeaderLength*i);

            io_mode = ChanInfo->MaxTransferMode;
            if(io_mode != -1) {
                ata_mode_to_str(mode_str, io_mode);
            } else {
                mode_str[0] = 0;
            }
            printf(" b%lu [%s]\n",
                i,
                mode_str
                );
        }

        while (adapterInfo->BusData[i].InquiryDataOffset) {
            /*
            if(dev_id/adapterInfo->BusData[i].NumberOfLogicalUnits ==
               inquiryData->TargetId &&
               dev_id%adapterInfo->BusData[i].NumberOfLogicalUnits ==
               inquiryData->Lun) {
                printf(" %d   %d  %3d    %s    %.28s ",
                i,
                inquiryData->TargetId,
                inquiryData->Lun,
                (inquiryData->DeviceClaimed) ? "Y" : "N",
                &inquiryData->InquiryData[8]);
            }*/
            l_dev_id = (i << 16) | ((ULONG)(inquiryData->TargetId) << 8) | inquiryData->Lun;
            
            if(l_dev_id == dev_id || dev_id == -1) {

                scsi_ident = (PINQUIRYDATA)&(inquiryData->InquiryData);
                if(!memcmp(&(scsi_ident->VendorId[0]), UNIATA_COMM_PORT_VENDOR_STR, 24)) {
                    // skip communication port
                    goto next_dev;
                }

                found = TRUE;

                if(inquiryData->Lun) {
                    sprintf(lun_str, ":l%d", inquiryData->Lun);
                } else {
                    sprintf(lun_str, "  ");
                }


                /*
                for (j = 0; j < 8; j++) {
                    printf("%02X ", inquiryData->InquiryData[j]);
                }
                */

                addr.Length = sizeof(addr);
                addr.PortNumber = -1;
                addr.PathId   = inquiryData->PathId;
                addr.TargetId = inquiryData->TargetId;
                addr.Lun      = inquiryData->Lun;
                status = ata_send_ioctl(h, &addr, "-UNIATA-",
                                        IOCTL_SCSI_MINIPORT_UNIATA_GET_MODE,
                                        NULL, 0,
                                        &IoMode, sizeof(IoMode),
                                        &returned);
                if(status) {
                    //io_mode = min(IoMode.CurrentMode, IoMode.MaxMode);
                    io_mode = IoMode.PhyMode;
                    if(!io_mode) {
                        io_mode = min(max(IoMode.CurrentMode,IoMode.OrigMode),IoMode.MaxMode);
                    }
                } else {
                    io_mode = -1;
                }
    
                memset(&pin, 0, sizeof(pin));
                memset(buff, 0, sizeof(buff));
                pin.irDriveRegs.bCommandReg = ID_CMD;
                // this is valid for IDE/ATA, where only 2 devices can be attached to the bus.
                // probably, we shall change this in future to support SATA splitters
                pin.bDriveNumber = inquiryData->PathId*2+inquiryData->TargetId;

                status = ata_send_ioctl(h, NULL, "SCSIDISK",
                                        IOCTL_SCSI_MINIPORT_IDENTIFY,
                                        &pin, sizeof(pin),
                                        buff, sizeof(buff),
                                        &returned);

                if(!status) {
                    memset(&pin, 0, sizeof(pin));
                    memset(buff, 0, sizeof(buff));
                    pin.irDriveRegs.bCommandReg = ATAPI_ID_CMD;
                    // this is valid for IDE/ATA, where only 2 devices can be attached to the bus.
                    // probably, we shall change this in future to support SATA splitters
                    pin.bDriveNumber = inquiryData->PathId*2+inquiryData->TargetId;

                    status = ata_send_ioctl(h, NULL, "SCSIDISK",
                                            IOCTL_SCSI_MINIPORT_IDENTIFY,
                                            &pin, sizeof(pin),
                                            buff, sizeof(buff),
                                            &returned);

                }

                if(!g_extended) {
                    printf("  b%lu:d%d%s    %24.24s %4.4s ",
                        i,
                        inquiryData->TargetId,
                        lun_str,
                        /*(inquiryData->DeviceClaimed) ? "Y" : "N",*/
                        (g_extended ? (PUCHAR)"" : &scsi_ident->VendorId[0]),
                        (g_extended ? (PUCHAR)"" : &scsi_ident->ProductRevisionLevel[0])
                        );
                } else {
                    printf("  b%lu:d%d%s ",
                        i,
                        inquiryData->TargetId,
                        lun_str
                        );
                }

                if(status) {
                    if(io_mode == -1) {
                        io_mode = ata_cur_mode_from_ident(ident, IDENT_MODE_ACTIVE);
                    }
                }
                if(io_mode != -1) {
                    ata_mode_to_str(mode_str, io_mode);
                }
                if(!g_extended || !status) {
                    if(g_extended) {
                        printf("   %24.24s %4.4s ",
                            (&inquiryData->InquiryData[8]),
                            (&inquiryData->InquiryData[8+24])
                            );
                    }
                    if(io_mode != -1) {
                        printf(" %.12s ", mode_str);
                    }
                }
                printf("\n");

                if(g_extended) {
                    if(status) {

                        BOOLEAN BlockMode_valid = TRUE;
                        BOOLEAN print_geom = FALSE;

                        switch(ident->DeviceType) {
                        case ATAPI_TYPE_DIRECT:
                            if(ident->Removable) {
                                printf("    Floppy        ");
                            } else {
                                printf("    Hard Drive    ");
                            }
                            break;
                        case ATAPI_TYPE_TAPE:
                            printf("    Tape Drive    ");
                            break;
                        case ATAPI_TYPE_CDROM:
                            printf("    CD/DVD Drive  ");
                            BlockMode_valid = FALSE;
                            break;
                        case ATAPI_TYPE_OPTICAL:
                            printf("    Optical Drive ");
                            BlockMode_valid = FALSE;
                            break;
                        default:
                            printf("    Hard Drive    ");
                            print_geom = TRUE;
                            //MOV_DD_SWP(max_lba, ident->UserAddressableSectors);
                            max_lba = ident->UserAddressableSectors;
                            if(ident->FeaturesSupport.Address48) {
                               max_lba = ident->UserAddressableSectors48;
                            }
                            //MOV_DW_SWP(chs[0], ident->NumberOfCylinders);
                            //MOV_DW_SWP(chs[1], ident->NumberOfHeads);
                            //MOV_DW_SWP(chs[2], ident->SectorsPerTrack);
                            chs[0] = ident->NumberOfCylinders;
                            chs[1] = ident->NumberOfHeads;
                            chs[2] = ident->SectorsPerTrack;
                            if(!max_lba) {
                                max_lba = (ULONG)(chs[0])*(ULONG)(chs[1])*(ULONG)(chs[2]);
                            }
                        }
                        if(io_mode != -1) {
                            printf("           %.12s\n", mode_str);
                        }
                        for (j = 0; j < 40; j += 2) {
                            MOV_DW_SWP(SerNum[j], ((PUCHAR)ident->ModelNumber)[j]);
                        }
                        printf("    Mod: %40.40s\n", SerNum);
                        for (j = 0; j < 8; j += 2) {
                            MOV_DW_SWP(SerNum[j], ((PUCHAR)ident->FirmwareRevision)[j]);
                        }
                        printf("    Rev: %8.8s\n", SerNum);
                        for (j = 0; j < 20; j += 2) {
                            MOV_DW_SWP(SerNum[j], ((PUCHAR)ident->SerialNumber)[j]);
                        }
                        printf("    S/N: %20.20s\n", SerNum);

                        if(BlockMode_valid) {
                            if(ident->MaximumBlockTransfer) {
                                printf("    Multi-block mode:        %u block%s\n", ident->MaximumBlockTransfer, ident->MaximumBlockTransfer == 1 ? "" : "s");
                            } else {
                                printf("    Multi-block mode:        N/A\n");
                            }
                        }
                        if(print_geom) {
                            printf("    C/H/S:                   %u/%u/%u \n", chs[0], chs[1], chs[2]);
                            printf("    LBA:                     %I64u \n", max_lba);
                            if(max_lba < 2) {
                                printf("    Size:                    %lu kb\n", (ULONG)(max_lba/2));
                            } else
                            if(max_lba < 2*1024*1024) {
                                printf("    Size:                    %lu Mb\n", (ULONG)(max_lba/2048));
                            } else
                            if(max_lba < (ULONG)2*1024*1024*1024) {
                                printf("    Size:                    %lu.%lu (%lu) Gb\n", (ULONG)(max_lba/2048/1024),
                                                                                  (ULONG)(((max_lba/2048)%1024)/10),
                                                                                  (ULONG)(max_lba*512/1000/1000/1000)
                                );
                            } else {
                                printf("    Size:                    %lu.%lu (%lu) Tb\n", (ULONG)(max_lba/2048/1024/1024),
                                                                                  (ULONG)((max_lba/2048/1024)%1024)/10,
                                                                                  (ULONG)(max_lba*512/1000/1000/1000)
                                );
                            }
                        }
                        len = 0;
                        if((hKey2 = ata_get_bblist_regh(ident, DevSerial, TRUE))) {
                            if(RegQueryValueEx(hKey2, DevSerial, NULL, NULL, NULL, &len) == ERROR_SUCCESS) {
                                printf("    !!! Assigned bad-block list !!!\n");
                            }
                            RegCloseKey(hKey2);
                        }
                    } else {
                        switch(scsi_ident->DeviceType) {
                        case DIRECT_ACCESS_DEVICE:
                            if(scsi_ident->RemovableMedia) {
                                printf("    Floppy        ");
                            } else {
                                printf("    Hard Drive    ");
                            }
                            break;
                        case SEQUENTIAL_ACCESS_DEVICE:
                            printf("    Tape Drive    ");
                            break;
                        case PRINTER_DEVICE:
                            printf("    Printer       ");
                            break;
                        case PROCESSOR_DEVICE:
                            printf("    Processor     ");
                            break;
                        case WRITE_ONCE_READ_MULTIPLE_DEVICE:
                            printf("    WORM Drive    ");
                            break;
                        case READ_ONLY_DIRECT_ACCESS_DEVICE:
                            printf("    CDROM Drive   ");
                            break;
                        case SCANNER_DEVICE:
                            printf("    Scanner       ");
                            break;
                        case OPTICAL_DEVICE:
                            printf("    Optical Drive ");
                            break;
                        case MEDIUM_CHANGER:
                            printf("    Changer       ");
                            break;
                        case COMMUNICATION_DEVICE:
                            printf("    Comm. device  ");
                            break;
                        }
                        printf("\n");
                    }
                }
                memcpy(&g_ident, ident, sizeof(IDENTIFY_DATA));
            }
next_dev:
            if (inquiryData->NextInquiryDataOffset == 0) {
                break;
            }
            
            inquiryData = (PSCSI_INQUIRY_DATA) (g_inquiry_buffer +
                inquiryData->NextInquiryDataOffset);
        }
    }
    if(!found) {
        printf("  No device(s) found.\n");
        return FALSE;
    }

    return TRUE;
} // end ata_check_unit()

BOOLEAN
ata_adapter_info(
    int bus_id,
    int print_info
    )
{
    char dev_name[64];
    HANDLE h;
    PADAPTERINFO AdapterInfo;
    ULONG status;
    ULONG returned;
    SCSI_ADDRESS addr;
    PCI_SLOT_NUMBER       slotData;
    char mode_str[12];
    ULONG len;

    sprintf(dev_name, "\\\\.\\Scsi%d:", bus_id);
    h = ata_open_dev(dev_name);
    if(!h)
        return FALSE;
    addr.Length     = sizeof(addr);
    addr.PortNumber = bus_id;

    len = sizeof(ADAPTERINFO)+sizeof(CHANINFO)*AHCI_MAX_PORT;
    if(!g_AdapterInfo) {
        AdapterInfo = (PADAPTERINFO)GlobalAlloc(GMEM_FIXED, len);
        if(!AdapterInfo) {
            ata_close_dev(h);
            return FALSE;
        }
    } else {
        AdapterInfo = g_AdapterInfo; 
    }
    memset(AdapterInfo, 0, len);

    status = ata_send_ioctl(h, &addr, "-UNIATA-",
                            IOCTL_SCSI_MINIPORT_UNIATA_ADAPTER_INFO,
                            AdapterInfo, len,
                            AdapterInfo, len,
                            &returned);
    if(status) {
        ata_mode_to_str(mode_str, AdapterInfo->MaxTransferMode);
    }
    printf("Scsi%d: %s     %s\n", bus_id, status ? "[UniATA]" : "", status ? mode_str : "");
    if(print_info) {
        if(!status) {
            printf("Can't get adapter info\n");
        } else {
            if(AdapterInfo->AdapterInterfaceType == PCIBus) {
                slotData.u.AsULONG = AdapterInfo->slotNumber;
                printf("  PCI Bus/Dev/Func:   %lu/%lu/%lu%s\n",
                    AdapterInfo->SystemIoBusNumber, slotData.u.bits.DeviceNumber, slotData.u.bits.FunctionNumber,
                    AdapterInfo->AdapterInterfaceType == AdapterInfo->OrigAdapterInterfaceType ? "" : " (ISA-Bridged)");
                printf("  VendorId/DevId/Rev: %#04x/%#04x/%#02x\n",
                    (USHORT)(AdapterInfo->DevID >> 16),
                    (USHORT)(AdapterInfo->DevID & 0xffff),
                    (UCHAR)(AdapterInfo->RevID));
                if(AdapterInfo->DeviceName[0]) {
                    printf("  Name:               %s\n", AdapterInfo->DeviceName);
                }
            } else
            if(AdapterInfo->AdapterInterfaceType == Isa) {
                printf("  ISA Bus\n");
            }
            printf("  IRQ: %ld\n", AdapterInfo->BusInterruptLevel);
        }
    }
    ata_close_dev(h);
    //GlobalFree(AdapterInfo);
    g_AdapterInfo = AdapterInfo;
    return status ? TRUE : FALSE;
} // end ata_adapter_info()

int
ata_check_controller(
    HANDLE h,    // handle to ScsiXXX:
    PIO_SCSI_CAPABILITIES capabilities
    )
{
    ULONG status;
    ULONG returned;

    status = DeviceIoControl(h,
                             IOCTL_SCSI_GET_CAPABILITIES,
                             NULL,
                             0,
                             capabilities,
                             sizeof(IO_SCSI_CAPABILITIES),
                             &returned,
                             FALSE);
    return status;
} // end ata_check_controller()

BOOLEAN
ata_list(
    int bus_id,
    int dev_id
    )
{
    char dev_name[64];
    HANDLE h;
    //BOOLEAN uniata_driven;

    if(bus_id == -1) {
        for(bus_id=0; TRUE; bus_id++) {
            if(!ata_list(bus_id, dev_id))
                break;
        }
        return TRUE;
    }
    /*uniata_driven =*/ ata_adapter_info(bus_id, g_adapter_info);
    sprintf(dev_name, "\\\\.\\Scsi%d:", bus_id);
    h = ata_open_dev(dev_name);
    if(!h)
        return FALSE;
    if(dev_id == -1) {
        ata_check_controller(h, &g_capabilities);
        ata_check_unit(h, -1);
        ata_close_dev(h);
        return TRUE;
    }
    ata_check_unit(h, dev_id | (bus_id << 24));
    ata_close_dev(h);
    return TRUE;
} // end ata_list()

BOOLEAN
ata_mode(
    int bus_id,
    int dev_id,
    int mode
    )
{
    char dev_name[64];
    HANDLE h;
    SETTRANSFERMODE IoMode;
    ULONG status;
    ULONG returned;
    SCSI_ADDRESS addr;

    if(dev_id == -1) {
        return FALSE;
    }
    sprintf(dev_name, "\\\\.\\Scsi%d:", bus_id);
    h = ata_open_dev(dev_name);
    if(!h)
        return FALSE;
    addr.Length   = sizeof(addr);
    addr.PortNumber = bus_id;
    addr.PathId   = (UCHAR)(dev_id >> 16);
    addr.TargetId = (UCHAR)(dev_id >> 8);
    addr.Lun      = (UCHAR)(dev_id);

    IoMode.MaxMode = mode;
    IoMode.ApplyImmediately = FALSE;
//    IoMode.ApplyImmediately = TRUE;
    IoMode.OrigMode = mode;

    status = ata_send_ioctl(h, &addr, "-UNIATA-",
                            IOCTL_SCSI_MINIPORT_UNIATA_SET_MAX_MODE,
                            &IoMode, sizeof(IoMode),
                            NULL, 0,
                            &returned);
    if(!status) {
        printf("Can't apply specified transfer mode\n");
    } else {
        ata_mode_to_str(dev_name, mode);
        printf("Transfer rate switched to %s\n", dev_name);
    }
    ata_close_dev(h);
    return status ? TRUE : FALSE;
} // end ata_mode()

BOOLEAN
ata_reset(
    int bus_id,
    int dev_id
    )
{
    char dev_name[64];
    HANDLE h;
    ULONG status;
    ULONG returned;
    SCSI_ADDRESS addr;

    if(dev_id == -1) {
        return FALSE;
    }
    sprintf(dev_name, "\\\\.\\Scsi%d:", bus_id);
    h = ata_open_dev(dev_name);
    if(!h)
        return FALSE;
    addr.Length   = sizeof(addr);
    addr.PortNumber = bus_id;
    addr.PathId   = (UCHAR)(dev_id >> 16);
    addr.TargetId = (UCHAR)(dev_id >> 8);
    addr.Lun      = (UCHAR)(dev_id);

    if(addr.TargetId == 0x7f && addr.Lun == 0x7f) {
        addr.TargetId = (UCHAR)0xff;
        addr.Lun      = 0;
        printf("Resetting channel...\n");
    } else {
        printf("Resetting device...\n");
    }

    status = ata_send_ioctl(h, &addr, "-UNIATA-",
                            IOCTL_SCSI_MINIPORT_UNIATA_RESET_DEVICE,
                            NULL, 0,
                            NULL, 0,
                            &returned);
    if(!status) {
        printf("Reset failed\n");
    } else {
        printf("Channel reset done\n");
    }
    ata_close_dev(h);
    return TRUE;
} // end ata_reset()

BOOLEAN
ata_hide(
    int bus_id,
    int dev_id,
    int lock,
    int persistent_hide,
    int power_mode
    )
{
    char dev_name[64];
    HANDLE h;
    ULONG status;
    ULONG returned;
    SCSI_ADDRESS addr;
    ADDREMOVEDEV to;

    if(dev_id == -1) {
        return FALSE;
    }

    if(power_mode) {
        ata_power_mode(bus_id, dev_id, power_mode);
    }

    if(lock < 0) {
        lock = DEFAULT_REMOVAL_LOCK_TIMEOUT;
    }
    sprintf(dev_name, "\\\\.\\Scsi%d:", bus_id);
    h = ata_open_dev(dev_name);
    if(!h)
        return FALSE;
    addr.Length   = sizeof(addr);
    addr.PortNumber = bus_id;
    addr.PathId   = (UCHAR)(dev_id >> 16);
    addr.TargetId = (UCHAR)(dev_id >> 8);
    addr.Lun      = (UCHAR)(dev_id);

    to.WaitForPhysicalLink = lock;
    to.Flags = persistent_hide ? UNIATA_REMOVE_FLAGS_HIDE : 0;

    printf("Deleting device.\n");
    if(lock) {
        printf("ATTENTION: you have %d seconds to disconnect cable\n", lock);
    }
    status = ata_send_ioctl(h, &addr, "-UNIATA-",
                            IOCTL_SCSI_MINIPORT_UNIATA_DELETE_DEVICE,
                            &to, sizeof(to),
                            NULL, 0,
                            &returned);
    if(!status) {
        printf("Delete failed\n");
    } else {
        printf("Device is detached\n");
    }
    ata_close_dev(h);
    return status ? TRUE : FALSE;
} // end ata_hide()

BOOLEAN
ata_scan(
    int bus_id,
    int dev_id,
    int lock,
    int unhide
    )
{
    char dev_name[64];
    HANDLE h;
    ULONG status;
    ULONG returned;
    SCSI_ADDRESS addr;
    ADDREMOVEDEV to;

    if(dev_id == -1) {
        return FALSE;
    }
    if(lock < 0) {
        lock = DEFAULT_REMOVAL_LOCK_TIMEOUT;
    }
    sprintf(dev_name, "\\\\.\\Scsi%d:", bus_id);
    h = ata_open_dev(dev_name);
    if(!h)
        return FALSE;

    if((UCHAR)(dev_id) != 0xff &&
       (UCHAR)(dev_id >> 8) != 0xff) {

        addr.Length   = sizeof(addr);
        addr.PortNumber = bus_id;
        addr.PathId   = (UCHAR)(dev_id >> 16);
        addr.TargetId = 0;
        addr.Lun      = 0;

        to.WaitForPhysicalLink = lock;
        to.Flags = unhide ? UNIATA_ADD_FLAGS_UNHIDE : 0;

        printf("Scanning bus for new devices.\n");
        if(lock) {
            printf("You have %d seconds to connect device.\n", lock);
        }
        status = ata_send_ioctl(h, &addr, "-UNIATA-",
                                IOCTL_SCSI_MINIPORT_UNIATA_FIND_DEVICES,
                                &to, sizeof(to),
                                NULL, 0,
                                &returned);
    } else {
        status = DeviceIoControl(h,
                                 IOCTL_SCSI_RESCAN_BUS,
                                 NULL, 0,
                                 NULL, 0,
                                 &returned,
                                 FALSE);
    }
    ata_close_dev(h);
    return status ? TRUE : FALSE;
} // end ata_scan()

CHAR*
_fgets(
    CHAR *string,
    int count,
    HANDLE stream
    )
{
    CHAR *pointer = string;
    ULONG read_bytes;

    CHAR *retval = string;
    int ch = 0;

    if (count <= 0)
        return(NULL);

    while (--count)
    {
        if(!ReadFile(stream, &ch, 1, &read_bytes, NULL) ||
           !read_bytes)
        {
            if (pointer == string) {
                retval=NULL;
                goto done;
            }
            break;
        }

        if ((*pointer++ = (CHAR)ch) == '\n') {
            break;
        }
    }

    *pointer = '\0';

/* Common return */
done:
    return(retval);
} // end _fgets()

BOOLEAN
ata_bblk(
    int bus_id,
    int dev_id,
    int list_bb
    )
{
    char dev_name[64];
    char tmp[64];
    char DevSerial[128];
    HANDLE h = NULL;
    HANDLE hf = NULL;
    ULONG status;
    ULONG returned;
    SCSI_ADDRESS addr;
    ULONG len;
    ULONG Length;
    BOOLEAN retval = FALSE;
    HKEY hKey2 = NULL;
    char* bblist = NULL;
    LONGLONG tmp_bb_lba;
    LONGLONG tmp_bb_len;
    char BB_Msg[256];
    int radix=gRadix;
    int i, j;
    ULONG b;

    if(dev_id == -1) {
        printf("\nERROR: Target device/bus ID must be specified\n\n");
        print_help();
        return FALSE;
    }
    if(((dev_id >> 16) & 0xff) == 0xff) {
        printf("\nERROR: Target device bus number (channel) must be specified with b:<bus id>\n\n");
        print_help();
        return FALSE;
    }
    if(((dev_id >> 8) & 0xff) == 0xff) {
        printf("\nERROR: Target device ID must be specified with d:<device id>\n\n");
        print_help();
        return FALSE;
    }
    sprintf(dev_name, "\\\\.\\Scsi%d:", bus_id);
    h = ata_open_dev(dev_name);
    if(!h) {
        if(bus_id == -1) {
            printf("Controller number must be specified\n");
        } else {
            printf("Can't open Controller %d\n", bus_id);
        }
        return FALSE;
    }

    if(list_bb == 0) {
        hf = ata_open_file(g_bb_list, FALSE);
        if(!hf) {
            printf("Can't open bad block list file:\n  %s\n", g_bb_list);
            ata_close_dev(h);
            return FALSE;
        }

        len = GetFileSize(hf, NULL);
        if(!len || len == INVALID_FILE_SIZE)
            goto exit;
        bblist = (char*)GlobalAlloc(GMEM_FIXED, len*8);
    }

    if(!ata_check_unit(h, dev_id | (bus_id << 24))) {
        goto exit;
    }

    hKey2 = ata_get_bblist_regh(&g_ident, DevSerial, list_bb==1);
    if(!hKey2) {
        printf("Can't open registry key:\n  HKLM\\SYSTEM\\CurrentControlSet\\Services\\UniATA\\Parameters\\BadBlocks\n");
        goto exit;
    }

    if(list_bb == -1) {
        if(RegDeleteValue(hKey2, DevSerial) != ERROR_SUCCESS) {
            printf("Can't delete registry value:\n  %s\n", DevSerial);
            goto exit;
        }

        addr.PortNumber = bus_id;
        addr.PathId   = (UCHAR)(dev_id >> 16);
        addr.TargetId = (UCHAR)(dev_id >> 8);
        addr.Lun      = (UCHAR)(dev_id);

        status = ata_send_ioctl(h, &addr, "-UNIATA-",
                                IOCTL_SCSI_MINIPORT_UNIATA_RESETBB,
                                NULL, 0,
                                NULL, 0,
                                &returned);
        if(!status) {
            printf("Bad block list shall be cleared after reboot.\n");
        } else {
            printf("Bad block list cleared\n");
        }
    } else
    if(list_bb == 0) {
        LONGLONG* pData = ((LONGLONG*)bblist);
        char a;
        int k, k0;
        Length=0;
        i=0;
        j=0;
        k=0;
        while(_fgets(BB_Msg, sizeof(BB_Msg), hf)) {
            j++;
            BB_Msg[sizeof(BB_Msg)-1] = 0;
            k=0;
            while((a = BB_Msg[k])) {
                if(a == ' ' || a == '\t' || a == '\r') {
                    k++;
                    continue;
                }
                break;
            }
            if(!a || a == ';' || a == '#') {
                continue;
            }
            if(!strncmp(BB_Msg+k, "hex:", 4)) {
                radix=16;
                continue;
            }
            if(!strncmp(BB_Msg+k, "dec:", 4)) {
                radix=10;
                continue;
            }
            k0 = k;
            while((a = BB_Msg[k])) {
                if(a == ' ' || a == '\t' || a == '\r') {
                    BB_Msg[k] = '\t';
                }
                k++;
                if(a == ';' || a == '#') {
                    break;
                }
                if(a >= '0' && a <= '9') {
                    continue;
                }
                if(radix == 16 && ((a >= 'A' && a <= 'F') || (a >= 'a' && a <= 'f'))) {
                    continue;
                }
                printf("Bad input BB list file:\n  %s\n", g_bb_list);
                printf("Illegal character '%1.1s' in line %d:\n%s\n", BB_Msg+k-1, j, BB_Msg);
                k0=-1;
                break;
            }
            if(k0 == -1) {
                continue;
            }
            k = k0;
            if(radix == 10) {
                b = sscanf(BB_Msg+k, "%I64u\t%I64u", &tmp_bb_lba, &tmp_bb_len);
            } else {
                b = sscanf(BB_Msg+k, "%I64x\t%I64x", &tmp_bb_lba, &tmp_bb_len);
            }
            if(b == 1) {
                tmp_bb_len = 1;
            } else
            if(b != 2) {
                printf("Bad input BB list file:\n  %s\n", g_bb_list);
                printf("Can't parse line %d:\n%s\n", j, BB_Msg);
                continue;
            } 
            if(!tmp_bb_len) {
                printf("Bad input BB list file:\n  %s\n", g_bb_list);
                printf("BlockCount evaluated to 0 in line %d:\n%s\n", j, BB_Msg);
                continue;
            }
            if(tmp_bb_lba < 0) {
                printf("Bad input BB list file:\n  %s\n", g_bb_list);
                printf("Start LBA evaluated to negative in line %d:\n%s\n", j, BB_Msg);
                continue;
            }
            if(tmp_bb_len < 0) {
                printf("Bad input BB list file:\n  %s\n", g_bb_list);
                printf("BlockCount evaluated to negative in line %d:\n%s\n", j, BB_Msg);
                continue;
            }

            if(i &&
                (pData[(i-1)*2+1] == tmp_bb_lba)) {
                pData[(i-1)*2+1]+=tmp_bb_len;
            } else {
                pData[i*2+0]=tmp_bb_lba;
                pData[i*2+1]=tmp_bb_lba+tmp_bb_len;
                i++;
                Length += sizeof(LONGLONG)*2;
            }
        }

        if(RegSetValueEx(hKey2, DevSerial, NULL, REG_BINARY, (const UCHAR*)bblist, Length) != ERROR_SUCCESS) {
            printf("Can't set registry value:\n  %s\n", DevSerial);
            goto exit;
        }
/*
        addr.PortNumber = bus_id;
        addr.PathId   = (UCHAR)(dev_id >> 16);
        addr.TargetId = (UCHAR)(dev_id >> 8);
        addr.Lun      = (UCHAR)(dev_id);

        status = ata_send_ioctl(h, &addr, "-UNIATA-",
                                IOCTL_SCSI_MINIPORT_UNIATA_SETBB,
                                NULL, 0,
                                NULL, 0,
                                &returned);
*/
        printf("Bad block list shall be applied after reboot\n");
    } else {
        len = 0;
        returned = RegQueryValueEx(hKey2, DevSerial, NULL, NULL, NULL, &len);
        if(returned == 2) {
            printf("No bad block list assigned\n");
            goto exit;
        } else
        if(returned != ERROR_SUCCESS) {
            printf("Can't get registry value:\n  %s\n", DevSerial);
            goto exit;
        }

        hf = ata_open_file(g_bb_list, TRUE);
        if(!hf) {
            printf("Can't create bad block list file:\n  %s\n", g_bb_list);
            goto exit;
        }

        bblist = (char*)GlobalAlloc(GMEM_FIXED, len);
        if(RegQueryValueEx(hKey2, DevSerial, NULL, NULL, (UCHAR*)bblist, &len) != ERROR_SUCCESS) {
            printf("Can't get registry value:\n  %s\n", DevSerial);
            goto exit;
        }
        if(g_bb_list) {
            for (j = 0; j < 20; j += 2) {
                MOV_DW_SWP(tmp[j], ((PUCHAR)(&g_ident.ModelNumber))[j]);
            }
            b = sprintf(BB_Msg, "#model: %20.20s\n", tmp);
            WriteFile(hf, BB_Msg, b, &returned, NULL);
            for (j = 0; j < 4; j += 2) {
                MOV_DW_SWP(tmp[j], ((PUCHAR)(&g_ident.FirmwareRevision))[j]);
            }
            b = sprintf(BB_Msg, "#rev:   %4.4s\n",   tmp);
            WriteFile(hf, BB_Msg, b, &returned, NULL);
            for (j = 0; j < 20; j += 2) {
                MOV_DW_SWP(tmp[j], ((PUCHAR)(&g_ident.SerialNumber))[j]);
            }
            b = sprintf(BB_Msg, "#s/n:   %20.20s\n", tmp);
            WriteFile(hf, BB_Msg, b, &returned, NULL);
            b = sprintf(BB_Msg, "#%s\n", DevSerial);
            WriteFile(hf, BB_Msg, b, &returned, NULL);
            b = sprintf(BB_Msg, "#Starting LBA\tNum. of Blocks\n");
            WriteFile(hf, BB_Msg, b, &returned, NULL);
            b = sprintf(BB_Msg, "hex:\n");
            WriteFile(hf, BB_Msg, b, &returned, NULL);
        } else {
            b = sprintf(BB_Msg, "Starting LBA\tNum. of Blocks (HEX)\n");
            WriteFile(hf, BB_Msg, b, &returned, NULL);
        }
        i = 0;
        while(len >= sizeof(LONGLONG)*2) {
            tmp_bb_lba = ((LONGLONG*)bblist)[i*2+0];
            tmp_bb_len = ((LONGLONG*)bblist)[i*2+1] - tmp_bb_lba;
            b = sprintf(BB_Msg, "%I64u\t%I64u\n", tmp_bb_lba, tmp_bb_len);
            WriteFile(hf, BB_Msg, b, &returned, NULL);
            i++;
            len -= sizeof(LONGLONG)*2;
        }
    }
    retval = TRUE;
exit:
    if(hKey2)
        RegCloseKey(hKey2);
    if(bblist) {
        GlobalFree(bblist);
    }
    ata_close_dev(hf);
    ata_close_dev(h);
    return retval;
} // end ata_bblk()

BOOLEAN
ata_power_mode(
    int bus_id,
    int dev_id,
    int power_mode
    )
{
    char dev_name[64];
    HANDLE h;
    ULONG status;
    ULONG returned;
    SCSI_ADDRESS addr;
    CDB cdb;
    SENSE_DATA senseData;

    if(dev_id == -1) {
        return FALSE;
    }
    if(!power_mode) {
        return TRUE;
    }

    sprintf(dev_name, "\\\\.\\Scsi%d:", bus_id);
    h = ata_open_dev(dev_name);
    if(!h)
        return FALSE;
    addr.PortNumber = bus_id;
    addr.PathId   = (UCHAR)(dev_id >> 16);
    addr.TargetId = (UCHAR)(dev_id >> 8);
    addr.Lun      = (UCHAR)(dev_id);

    memset(&cdb, 0, sizeof(cdb));
    cdb.START_STOP.OperationCode = SCSIOP_START_STOP_UNIT;
    cdb.START_STOP.Immediate = 1;
    cdb.START_STOP.PowerConditions = power_mode;
    cdb.START_STOP.Start = (power_mode != StartStop_Power_Sleep);

    printf("Changing power state to ...\n");

    status = ata_send_scsi(h, &addr, &cdb, 6,
                            NULL, 0, FALSE,
                            &senseData, &returned);
    ata_close_dev(h);
    return status ? TRUE : FALSE;
} // end ata_power_mode()

int
ata_num_to_x_dev(
    char a
    )
{
    if(a >= '0' && a <= '9')
        return a-'0';
    return -1;
}

int
main (
    int argc,
    char* argv[]
    )
{
    //ULONG Flags = 0;
    int i, j;
    char a;
    int bus_id = -1;
    int dev_id = -1;
    int cmd = 0;
    int lock = -1;
    int b_dev=-1, d_dev=-1, l_dev=0;
    int mode=-1;
    int list_bb=0;
    int persistent_hide=0;
    int power_mode=StartStop_Power_NoChg;

    printf("Console ATA control utility for Windows NT3.51/NT4/2000/XP/2003\n"
           "Version 0." UNIATA_VER_STR ", Copyright (c) Alexander A. Telyatnikov, 2003-2012\n"
           "Home site: http://alter.org.ua\n");

    for(i=1; i<argc; i++) {
        if(!argv[i])
            continue;
        if((a = argv[i][0]) != '-') {
            for(j=0; (a = argv[i][j]); j++) {
                switch(a) {
                case 'a' :
                case 's' :
                case 'c' :
                    j++;
                    bus_id = ata_num_to_x_dev(argv[i][j]);
                    break;
                case 'b' :
                    j++;
                    b_dev = ata_num_to_x_dev(argv[i][j]);
                    break;
                case 'd' :
                    j++;
                    d_dev = ata_num_to_x_dev(argv[i][j]);
                    break;
                case 'l' :
                    j++;
                    l_dev = ata_num_to_x_dev(argv[i][j]);
                    break;
                case ':' :
                    break;
                default:
                    print_help();
                }
            }
            continue;
        }
        j=1;
        while(argv[i] && (a = argv[i][j]) && (a != ' ') && (a != '\t')) {
            switch(a) {
            case 'l' :
                if(cmd || lock>0) {
                    print_help();
                }
                cmd = CMD_ATA_LIST;
                break;
            case 'x' :
                g_extended = 1;
                break;
            case 'a' :
                g_adapter_info = 1;
                break;
            case 'S' :
                persistent_hide = 1;
            case 's' :
                if(cmd || lock>0) {
                    print_help();
                }
                cmd = CMD_ATA_FIND;
                d_dev = 0;
                break;
            case 'H' :
                persistent_hide = 1;
            case 'h' :
                if(cmd) {
                    print_help();
                }
                cmd = CMD_ATA_HIDE;
                d_dev = 0;
                break;
            case 'm' :
                if(cmd) {
                    print_help();
                }
                cmd = CMD_ATA_MODE;
                i++;
                if(!argv[i]) {
                    print_help();
                }
                mode = ata_str_to_mode(argv[i]);
                if(mode == -1) {
                    i--;
                } else {
                    j = strlen(argv[i])-1;
                }
                break;
            case 'r' :
                if(cmd) {
                    print_help();
                }
                cmd = CMD_ATA_RESET;
                break;
            case 'b' :
                if(cmd) {
                    print_help();
                }
                switch(argv[i][j+1]) {
                case 'l':
                    list_bb = 1;
                    break;
                case 'a':
                    list_bb = 0;
                    break;
                case 'r':
                    list_bb = -1;
                    break;
                default:
                    j--;
                }
                j++;
                cmd = CMD_ATA_BBLK;
                break;
            case 'f' :
                if(cmd != CMD_ATA_BBLK) {
                    print_help();
                }
                i++;
                if(!argv[i]) {
                    print_help();
                }
                g_bb_list=argv[i];
                j = strlen(argv[i])-1;
                break;
            case 'p' :
                if(cmd && (cmd != CMD_ATA_FIND) && (cmd != CMD_ATA_HIDE)) {
                    print_help();
                }
                switch(argv[i][j+1]) {
                case '0':
                case 'a':
                    // do nothing
                    break;
                case '1':
                case 'i':
                    power_mode = StartStop_Power_Idle;
                    break;
                case '2':
                case 's':
                    power_mode = StartStop_Power_Standby;
                    break;
                case '3':
                case 'p':
                    power_mode = StartStop_Power_Sleep;
                    break;
                default:
                    j--;
                }
                j++;
                if(power_mode && !cmd) {
                    cmd = CMD_ATA_POWER;
                }
                break;
            case 'D' :
                power_mode = StartStop_Power_Sleep;
                if(cmd && (cmd != CMD_ATA_HIDE)) {
                    print_help();
                }
            case 'd' :
                if(cmd && (cmd != CMD_ATA_FIND) && (cmd != CMD_ATA_HIDE) && (cmd != CMD_ATA_POWER)) {
                    print_help();
                }
                if(!cmd) {
                    cmd = CMD_ATA_HIDE;
                }
                i++;
                if(!argv[i]) {
                    print_help();
                }
                if(!sscanf(argv[i], "%d", &lock)) {
                    lock = DEFAULT_REMOVAL_LOCK_TIMEOUT;
                    i--;
                }
                j = strlen(argv[i])-1;
                break;
            case 'n' :
                if(cmd != CMD_ATA_BBLK) {
                    print_help();
                }
                i++;
                if(!argv[i]) {
                    print_help();
                }
                if(!strcmp(argv[i], "hex") ||
                   !strcmp(argv[i], "16")) {
                    gRadix = 16;
                } else
                if(!strcmp(argv[i], "dec") ||
                   !strcmp(argv[i], "10")) {
                    gRadix = 10;
                } else {
                    print_help();
                }
                j = strlen(argv[i])-1;
                break;
            case '?' :
            default:
                print_help();
            }
            j++;
        }
    }

    if(g_adapter_info && !cmd) {
        cmd = CMD_ATA_LIST;
        b_dev = 127;
        d_dev = 127;
        l_dev = 127;
    } else
    if((d_dev == -1) && (b_dev != -1)) {
        d_dev = 127;
        l_dev = 127;
    }

    if((d_dev != -1) && (b_dev != -1)) {
        dev_id = (b_dev << 16) | (d_dev << 8) | l_dev;
    }
    if(cmd == CMD_ATA_LIST) {
        ata_list(bus_id, dev_id);
    } else
    if(cmd == CMD_ATA_MODE) {
        ata_mode(bus_id, dev_id, mode);
    } else
    if(cmd == CMD_ATA_RESET) {
        ata_reset(bus_id, dev_id);
    } else
    if(cmd == CMD_ATA_FIND) {
        ata_scan(bus_id, dev_id, lock, persistent_hide);
    } else
    if(cmd == CMD_ATA_HIDE) {
        ata_hide(bus_id, dev_id, lock, persistent_hide, power_mode);
    } else
    if(cmd == CMD_ATA_BBLK) {
        ata_bblk(bus_id, dev_id, list_bb);
    } else
    if(cmd == CMD_ATA_POWER) {
        ata_power_mode(bus_id, dev_id, power_mode);
    } else {
        print_help();
    }
    exit(0);
}

