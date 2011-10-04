#ifndef _PXE_
#define _PXE_

/* Basic types */

typedef UINT16 OFF16;
typedef UINT16 PXENV_EXIT;
typedef UINT16 PXENV_STATUS;
typedef UINT16 SEGSEL;
typedef UINT16 UDP_PORT;
typedef UINT32 ADDR32;

#include <pshpack1.h>

#define IP_ADDR_LEN 4
typedef union
{
    UINT32 num;
    UINT8 array[IP_ADDR_LEN];
} IP4;

#define MAC_ADDR_LEN 16
typedef UINT8 MAC_ADDR[MAC_ADDR_LEN];

typedef struct s_SEGDESC
{
    UINT16 segment_address;
    UINT32 physical_address;
    UINT16 seg_size;
} SEGDESC;

typedef struct s_SEGOFF16
{
    OFF16 offset;
    SEGSEL segment;
} SEGOFF16;

typedef struct s_PXE
{
    UINT8 Signature[4];
    UINT8 StructLength;
    UINT8 StructCksum;
    UINT8 StructRev;
    UINT8 reserved1;
    SEGOFF16 UNDIROMID;
    SEGOFF16 BaseROMID;
    SEGOFF16 EntryPointSP;
    SEGOFF16 EntryPointESP;
    SEGOFF16 StatusCallout;
    UINT8 reserved2;
    UINT8 SegDescCnt;
    SEGSEL FirstSelector;
    SEGDESC Stack;
    SEGDESC UNDIData;
    SEGDESC UNDICode;
    SEGDESC UNDICodeWrite;
    SEGDESC BC_Data;
    SEGDESC BC_Code;
    SEGDESC BC_CodeWrite;
} PXE, *PPXE;

/* PXENV structures */

typedef struct s_PXENV_START_UNDI
{
    PXENV_STATUS Status;
    UINT16 AX;
    UINT16 BX;
    UINT16 DX;
    UINT16 DI;
    UINT16 ES;
} t_PXENV_START_UNDI;

typedef struct s_PXENV_UNDI_STARTUP
{
    PXENV_STATUS Status;
} t_PXENV_UNDI_STARTUP;

typedef struct s_PXENV_UNDI_CLEANUP
{
    PXENV_STATUS Status;
} t_PXENV_UNDI_CLEANUP;

typedef struct s_PXENV_UNDI_INITIALIZE
{
    PXENV_STATUS Status;
    ADDR32 ProtocolIni;
    UINT8 reserved[8];
} t_PXENV_UNDI_INITIALIZE;

#define MAXNUM_MCADDR 8
typedef struct s_PXENV_UNDI_MCAST_ADDRESS
{
    UINT16 MCastAddrCount;
    MAC_ADDR McastAddr[MAXNUM_MCADDR];
} t_PXENV_UNDI_MCAST_ADDRESS;

typedef struct s_PXENV_UNDI_RESET
{
    PXENV_STATUS Status;
    t_PXENV_UNDI_MCAST_ADDRESS R_Mcast_Buf;
} t_PXENV_UNDI_RESET;

typedef struct s_PXENV_UNDI_SHUTDOWN
{
    PXENV_STATUS Status;
} t_PXENV_UNDI_SHUTDOWN;

typedef struct s_PXENV_UNDI_OPEN
{
    PXENV_STATUS Status;
    UINT16 OpenFlag;
    UINT16 PktFilter;
#define FLTR_DIRECTED 0x01
#define FLTR_BRDCST   0x02
#define FLTR_PRMSCS   0x04
#define FLTR_SRC_RTG  0x08

    t_PXENV_UNDI_MCAST_ADDRESS R_Mcast_Buf;
} t_PXENV_UNDI_OPEN;

typedef struct s_PXENV_UNDI_CLOSE
{
    PXENV_STATUS Status;
} t_PXENV_UNDI_CLOSE;

typedef struct s_PXENV_UNDI_TRANSMIT
{
    PXENV_STATUS Status;
    UINT8 Protocol;
#define P_UNKNOWN 0
#define P_IP      1
#define P_ARP     2
#define P_RARP    3

    UINT8 XmitFlag;
#define XMT_DESTADDR  0x00
#define XMT_BROADCAST 0x01

    SEGOFF16 DestAddr;
    SEGOFF16 TBD;
    UINT32 Reserved[2];
} t_PXENV_UNDI_TRANSMIT;

#define MAX_DATA_BLKS 8
typedef struct s_PXENV_UNDI_TBD
{
    UINT16 ImmedLength;
    SEGOFF16 Xmit;
    UINT16 DataBlkCount;
    struct DataBlk
    {
        UINT8 TDPtrType;
        UINT8 TDRsvdByte;
        UINT8 TDDataLen;
        SEGOFF16 TDDataPtr;
    } DataBlock[MAX_DATA_BLKS];
} t_PXENV_UNDI_TBD;

typedef struct s_PXENV_UNDI_SET_MCAST_ADDRESS
{
    PXENV_STATUS Status;
    t_PXENV_UNDI_MCAST_ADDRESS R_Mcast_Buf;
} t_PXENV_UNDI_SET_MCAST_ADDRESS;

typedef struct s_PXENV_UNDI_SET_STATION_ADDRESS
{
    PXENV_STATUS Status;
    MAC_ADDR StationAddress;
} t_PXENV_UNDI_SET_STATION_ADDRESS;

typedef struct s_PXENV_UNDI_SET_PACKET_FILTER
{
    PXENV_STATUS Status;
    UINT8 filter;
} t_PXENV_UNDI_SET_PACKET_FILTER;

typedef struct s_PXENV_UNDI_GET_INFORMATION
{
    PXENV_STATUS Status;
    UINT16 BaseIo;
    UINT16 IntNumber;
    UINT16 MaxTranUnit;
    UINT16 HwType;
#define ETHER_TYPE     1
#define EXP_ETHER_TYPE 2
#define IEEE_TYPE      3
#define ARCNET_TYPE    4

    UINT16 HwAddrLen;
    MAC_ADDR CurrentNodeAddress;
    MAC_ADDR PermNodeAddress;
    SEGSEL ROMAddress;
    UINT16 RxBufCt;
    UINT16 TxBufCt;
} t_PXENV_UNDI_GET_INFORMATION;

typedef struct s_PXENV_UNDI_GET_STATISTICS
{
    PXENV_STATUS Status;
    UINT32 XmtGoodFrames;
    UINT32 RcvGoodFrames;
    UINT32 RcvCRCErrors;
    UINT32 RcvResourceErrors;
} t_PXENV_UNDI_GET_STATISTICS;

typedef struct s_PXENV_UNDI_CLEAR_STATISTICS
{
    PXENV_STATUS Status;
} t_PXENV_UNDI_CLEAR_STATISTICS;

typedef struct s_PXENV_UNDI_INITIATE_DIAGS
{
    PXENV_STATUS Status;
} t_PXENV_UNDI_INITIATE_DIAGS;

typedef struct s_PXENV_UNDI_FORCE_INTERRUPT
{
    PXENV_STATUS Status;
} t_PXENV_UNDI_FORCE_INTERRUPT;

typedef struct s_PXENV_UNDI_GET_MCAST_ADDRESS
{
    PXENV_STATUS Status;
    IP4 InetAddr;
    MAC_ADDR MediaAddr;
} t_PXENV_UNDI_GET_MCAST_ADDRESS;

typedef struct s_PXENV_UNDI_GET_NIC_TYPE
{
    PXENV_STATUS Status;
    UINT8 NicType;
#define PCI_NIC 2
#define PnP_NIC 3
#define CardBus_NIC 4

    union
    {
        struct
        {
            UINT16 Vendor_ID;
            UINT16 Dev_ID;
            UINT8 Base_Class;
            UINT8 Sub_Class;
            UINT8 Prog_Intf;
            UINT8 Rev;
            UINT16 BusDevFunc;
            UINT16 SubVendor_ID;
            UINT16 SubDevice_ID;
        } pci, cardbus;
        struct
        {
            UINT32 EISA_Dev_ID;
            UINT8 Base_Class;
            UINT8 Sub_Class;
            UINT8 Prog_Intf;
            UINT16 CardSelNum;
        } pnp;
    } info;
} t_PXENV_UNDI_GET_NIC_TYPE;

typedef struct s_PXENV_UNDI_GET_IFACE_INFO
{
    PXENV_STATUS Status;
    UINT8 IfaceType[16];
    UINT32 LinkSpeed;
    UINT32 ServiceFlags;
    UINT32 Reserved[4];
} t_PXENV_UNDI_GET_IFACE_INFO;

typedef struct s_PXENV_UNDI_ISR
{
    PXENV_STATUS Status;
    UINT16 FuncFlag;
    UINT16 BufferLength;
    UINT16 FrameLength;
    UINT16 FrameHeaderLength;
    SEGOFF16 Frame;
    UINT8 ProtType;
    UINT8 PktType;
} t_PXENV_UNDI_ISR;

#define PXENV_UNDI_ISR_IN_START     1
#define PXENV_UNDI_ISR_IN_PROCESS   2
#define PXENV_UNDI_ISR_IN_GET_NEXT  3

/* One of these will be returned for PXENV_UNDI_ISR_IN_START */
#define PXENV_UNDI_ISR_OUT_OURS     0
#define PXENV_UNDI_ISR_OUT_NOT_OURS 1

/* One of these will be returned for PXENV_UNDI_ISR_IN_PROCESS and PXENV_UNDI_ISR_IN_GET_NEXT */
#define PXENV_UNDI_ISR_OUT_DONE     0
#define PXENV_UNDI_ISR_OUT_TRANSMIT 2
#define PXENV_UNDI_ISR_OUT_RECEIVE  3
#define PXENV_UNDI_ISR_OUT_BUSY     4

typedef struct s_PXENV_UNDI_GET_STATE
{
    PXENV_STATUS Status;
#define PXE_UNDI_GET_STATE_STARTED     1
#define PXE_UNDI_GET_STATE_INITIALIZED 2
#define PXE_UNDI_GET_STATE_OPENED      3
    UINT8 UNDIState;
} t_PXENV_UNDI_GET_STATE;

typedef struct s_PXENV_STOP_UNDI
{
    PXENV_STATUS Status;
} t_PXENV_STOP_UNDI;

typedef struct s_PXENV_TFTP_OPEN
{
    PXENV_STATUS Status;
    IP4 ServerIPAddress;
    IP4 GatewayIPAddress;
    UINT8 FileName[128];
    UDP_PORT TFTPPort;
    UINT16 PacketSize;
} t_PXENV_TFTP_OPEN;

typedef struct s_PXENV_TFTP_CLOSE
{
    PXENV_STATUS Status;
} t_PXENV_TFTP_CLOSE;

typedef struct s_PXENV_TFTP_READ
{
    PXENV_STATUS Status;
    UINT16 PacketNumber;
    UINT16 BufferSize;
    SEGOFF16 Buffer;
} t_PXENV_TFTP_READ;

typedef struct s_PXENV_TFTP_READ_FILE
{
    PXENV_STATUS Status;
    UINT8 FileName[128];
    UINT32 BufferSize;
    ADDR32 Buffer;
    IP4 ServerIPAddress;
    IP4 GatewayIPAddress;
    IP4 McastIPAddress;
    UDP_PORT TFTPClntPort;
    UDP_PORT TFTPSvrPort;
    UINT16 TFTPOpenTimeOut;
    UINT16 TFTPReopenDelay;
} t_PXENV_TFTP_READ_FILE;

typedef struct s_PXENV_TFTP_GET_FSIZE
{
    PXENV_STATUS Status;
    IP4 ServerIPAddress;
    IP4 GatewayIPAddress;
    UINT8 FileName[128];
    UINT32 FileSize;
} t_PXENV_TFTP_GET_FSIZE;

typedef struct s_PXENV_UDP_OPEN
{
    PXENV_STATUS Status;
    IP4 src_ip;
} t_PXENV_UDP_OPEN;

typedef struct s_PXENV_UDP_CLOSE
{
    PXENV_STATUS Status;
} t_PXENV_UDP_CLOSE;

typedef struct s_PXENV_UDP_READ
{
    PXENV_STATUS Status;
    IP4 ip;
    IP4 dest_ip;
    UDP_PORT s_port;
    UDP_PORT d_port;
    UINT16 buffer_size;
    SEGOFF16 buffer;
} t_PXENV_UDP_READ;

typedef struct s_PXENV_UDP_WRITE
{
    PXENV_STATUS Status;
    IP4 ip;
    IP4 gw;
    UDP_PORT src_port;
    UDP_PORT dst_port;
    UINT16 buffer_size;
    SEGOFF16 buffer;
} t_PXENV_UDP_WRITE;

typedef struct s_PXENV_UNLOAD_STACK
{
    PXENV_STATUS Status;
    UINT8 reserved[10];
} t_PXENV_UNLOAD_STACK;

typedef struct s_PXENV_GET_CACHED_INFO
{
    PXENV_STATUS Status;
    UINT16 PacketType;
#define PXENV_PACKET_TYPE_DHCP_DISCOVER 1
#define PXENV_PACKET_TYPE_DHCP_ACK      2
#define PXENV_PACKET_TYPE_CACHED_REPLY  3
    UINT16 BufferSize;
    SEGOFF16 Buffer;
    UINT16 BufferLimit;
} t_PXENV_GET_CACHED_INFO;

typedef struct s_PXENV_START_BASE
{
    PXENV_STATUS Status;
} t_PXENV_START_BASE;

typedef struct s_PXENV_STOP_BASE
{
    PXENV_STATUS Status;
} t_PXENV_STOP_BASE;

typedef struct bootph
{
    UINT8 opcode;
#define BOOTP_REQ 1
#define BOOTP_REP 2

    UINT8 Hardware;
    UINT8 Hardlen;
    UINT8 Gatehops;
    UINT32 ident;
    UINT16 seconds;
    UINT16 Flags;
#define BOOTP_BCAST 0x8000

    IP4 cip;
    IP4 yip;
    IP4 sip;
    IP4 gip;
    MAC_ADDR CAddr;
    UINT8 Sname[64];
    UINT8 bootfile[128];
    union
    {
#define BOOTP_DHCPVEND 1024 /* DHCP extended vendor field size */
        UINT8 d[BOOTP_DHCPVEND];
        struct
        {
            UINT8 magic[4];
#define VM_RFC1048 0x63825363
            UINT32 flags;
            UINT8 pad[56];
        } v;
    } vendor;
} BOOTPLAYER;

#include <poppack.h>

/* Exit codes returned in AX by a PXENV API service */
#define PXENV_EXIT_SUCCESS  0x0000
#define PXENV_EXIT_FAILURE  0x0001

/* Generic API status & error codes that are reported by the loader */
#define PXENV_STATUS_SUCCESS                          0x00
#define PXENV_STATUS_FAILURE                          0x01 /* General failure */
#define PXENV_STATUS_BAD_FUNC                         0x02 /* Invalid function number */
#define PXENV_STATUS_UNSUPPORTED                      0x03 /* Function is not yet supported */
#define PXENV_STATUS_KEEP_UNDI                        0x04 /* UNDI must not be unloaded from base memory */
#define PXENV_STATUS_KEEP_ALL                         0x05
#define PXENV_STATUS_OUT_OF_RESOURCES                 0x06 /* Base code and UNDI must not be unloaded from base memory */

/* ARP errors (0x10 to 0x1f) */
#define PXENV_STATUS_ARP_TIMEOUT                      0x11

/* Base code state errors */
#define PXENV_STATUS_UDP_CLOSED                       0x18
#define PXENV_STATUS_UDP_OPEN                         0x19
#define PXENV_STATUS_TFTP_CLOSED                      0x1a
#define PXENV_STATUS_TFTP_OPEN                        0x1b

/* BIOS/system errors (0x20 to 0x2f) */
#define PXENV_STATUS_MCOPY_PROBLEM                    0x20
#define PXENV_STATUS_BIS_INTEGRITY_FAILURE            0x21
#define PXENV_STATUS_BIS_VALIDATE_FAILURE             0x22
#define PXENV_STATUS_BIS_INIT_FAILURE                 0x23
#define PXENV_STATUS_BIS_SHUTDOWN_FAILURE             0x24
#define PXENV_STATUS_BIS_GBOA_FAILURE                 0x25
#define PXENV_STATUS_BIS_FREE_FAILURE                 0x26
#define PXENV_STATUS_BIS_GSI_FAILURE                  0x27
#define PXENV_STATUS_BIS_BAD_CKSUM                    0x28

/* TFTP/MTFTP errors (0x30 to 0x3f) */
#define PXENV_STATUS_TFTP_CANNOT_ARP_ADDRESS          0x30
#define PXENV_STATUS_TFTP_OPEN_TIMEOUT                0x32
#define PXENV_STATUS_TFTP_UNKNOWN_OPCODE              0x33
#define PXENV_STATUS_TFTP_READ_TIMEOUT                0x35
#define PXENV_STATUS_TFTP_ERROR_OPCODE                0x36
#define PXENV_STATUS_TFTP_CANNOT_OPEN_CONNECTION      0x38
#define PXENV_STATUS_TFTP_CANNOT_READ_FROM_CONNECTION 0x39
#define PXENV_STATUS_TFTP_TOO_MANY_PACKAGES           0x3a
#define PXENV_STATUS_TFTP_FILE_NOT_FOUND              0x3b
#define PXENV_STATUS_TFTP_ACCESS_VIOLATION            0x3c
#define PXENV_STATUS_TFTP_NO_MCAST_ADDRESS            0x3d
#define PXENV_STATUS_TFTP_NO_FILESIZE                 0x3e
#define PXENV_STATUS_TFTP_INVALID_PACKET_SIZE         0x3f

/* Reserved errors (0x40 to 0x4f) */

/* DHCP/BOOTP errors (0x50 to 0x5f) */
#define PXENV_STATUS_DHCP_TIMEOUT                     0x51
#define PXENV_STATUS_DHCP_NO_IP_ADDRESS               0x52
#define PXENV_STATUS_DHCP_NO_BOOTFILE_NAME            0x53
#define PXENV_STATUS_DHCP_BAD_IP_ADDRESS              0x54

/* Driver errors (0x60 to 0x6f) */
/* These errors are for UNDI compatible NIC drivers */
#define PXENV_STATUS_UNDI_INVALID_FUNCTION            0x60
#define PXENV_STATUS_UNDI_MEDIATEST_FAILED            0x61
#define PXENV_STATUS_UNDI_CANNOT_INIT_NIC_FOR_MCAST   0x62
#define PXENV_STATUS_UNDI_CANNOT_INITIALIZE_NIC       0x63
#define PXENV_STATUS_UNDI_CANNOT_INITIALIZE_PHY       0x64
#define PXENV_STATUS_UNDI_CANNOT_READ_CONFIG_DATA     0x65
#define PXENV_STATUS_UNDI_CANNOT_READ_INIT_DATA       0x66
#define PXENV_STATUS_UNDI_BAD_MAC_ADDRESS             0x67
#define PXENV_STATUS_UNDI_BAD_EEPROM_CHECKSUM         0x68
#define PXENV_STATUS_UNDI_ERROR_SETTING_ISR           0x69
#define PXENV_STATUS_UNDI_INVALID_STATE               0x6a
#define PXENV_STATUS_UNDI_TRANSMIT_ERROR              0x6b
#define PXENV_STATUS_UNDI_INVALID_PARAMETER           0x6c

/* ROM and NBP bootstrap errors (0x70 to 0x7f) */
#define PXENV_STATUS_BSTRAP_PROMPT_MENU               0x74
#define PXENV_STATUS_BSTRAP_MCAST_ADDR                0x76
#define PXENV_STATUS_BSTRAP_MISSING_LIST              0x77
#define PXENV_STATUS_BSTRAP_NO_RESPONSE               0x78
#define PXENV_STATUS_BSTRAP_FILE_TOO_BIG              0x79

/* Environment NBP errors (0x80 to 0x8f) */

/* Reserved errors (0x90 to 0x9f) */

/* Misc. errors (0xa0 to 0xaf) */
#define PXENV_STATUS_BINL_CANCELED_BY_KEYSTROKE       0xa0
#define PXENV_STATUS_BINL_NO_PXE_SERVER               0xa1
#define PXENV_STATUS_NOT_AVAILABLE_IN_PMODE           0xa2
#define PXENV_STATUS_NOT_AVAILABLE_IN_RMODE           0xa3

/* BUSD errors (0xb0 to 0xbf) */
#define PXENV_STATUS_BUSD_DEVICE_NOT_SUPPORTED        0xb0

/* Loader errors (0xc0 to 0xcf) */
#define PXENV_STATUS_LOADER_NO_FREE_BASE_MEMORY       0xc0
#define PXENV_STATUS_LOADER_NO_BC_ROMID               0xc1
#define PXENV_STATUS_LOADER_BAD_BC_ROMID              0xc2
#define PXENV_STATUS_LOADER_BAD_BC_RUNTIME_IMAGE      0xc3
#define PXENV_STATUS_LOADER_NO_UNDI_ROMID             0xc4
#define PXENV_STATUS_LOADER_BAD_UNDI_ROMID            0xc5
#define PXENV_STATUS_LOADER_BAD_UNDI_DRIVER_IMAGE     0xc6
#define PXENV_STATUS_LOADER_NO_PXE_STRUCT             0xc8
#define PXENV_STATUS_LOADER_NO_PXENV_STRUCT           0xc9
#define PXENV_STATUS_LOADER_UNDI_START                0xca
#define PXENV_STATUS_LOADER_BC_START                  0xcb

/* Vendor errors (0xd0 to 0xff) */

/* PXENV API services */
#define PXENV_START_UNDI               0x00
#define PXENV_UNDI_STARTUP             0x01
#define PXENV_UNDI_CLEANUP             0x02
#define PXENV_UNDI_INITIALIZE          0x03
#define PXENV_UNDI_RESET_ADAPTER       0x04
#define PXENV_UNDI_SHUTDOWN            0x05
#define PXENV_UNDI_OPEN                0x06
#define PXENV_UNDI_CLOSE               0x07
#define PXENV_UNDI_TRANSMIT            0x08
#define PXENV_UNDI_SET_MCAST_ADDRESS   0x09
#define PXENV_UNDI_SET_STATION_ADDRESS 0x0a
#define PXENV_UNDI_SET_PACKET_FILTER   0x0b
#define PXENV_UNDI_GET_INFORMATION     0x0c
#define PXENV_UNDI_GET_STATISTICS      0x0d
#define PXENV_UNDI_CLEAR_STATISTICS    0x0e
#define PXENV_UNDI_INITIATE_DIAGS      0x0f
#define PXENV_UNDI_FORCE_INTERRUPT     0x10
#define PXENV_UNDI_GET_MCAST_ADDRESS   0x11
#define PXENV_UNDI_GET_NIC_TYPE        0x12
#define PXENV_UNDI_GET_IFACE_INFO      0x13
#define PXENV_UNDI_ISR                 0x14
#define PXENV_UNDI_GET_STATE           0x15
#define PXENV_STOP_UNDI                0x15
#define PXENV_TFTP_OPEN                0x20
#define PXENV_TFTP_CLOSE               0x21
#define PXENV_TFTP_READ                0x22
#define PXENV_TFTP_READ_FILE           0x23
#define PXENV_TFTP_GET_FSIZE           0x25
#define PXENV_UDP_OPEN                 0x30
#define PXENV_UDP_CLOSE                0x31
#define PXENV_UDP_READ                 0x32
#define PXENV_UDP_WRITE                0x33
#define PXENV_UNLOAD_STACK             0x70
#define PXENV_GET_CACHED_INFO          0x71
#define PXENV_RESTART_TFTP             0x73
#define PXENV_START_BASE               0x75
#define PXENV_STOP_BASE                0x76

#endif
