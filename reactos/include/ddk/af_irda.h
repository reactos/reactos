/*
 * af_irda.h
 *
 * IrDa ports interface
 *
 * This file is part of the MinGW package.
 *
 * Contributors:
 *   Created by Robert Dickenson <robd@users.sourceforge.net>
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef __AF_IRDA_H
#define __AF_IRDA_H

#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push,4)


/* GUIDs */

#ifdef DEFINE_GUID
DEFINE_GUID(GUID_DEVINTERFACE_IRDAPORT,
  0x86e0d1e0L, 0x8089, 0x11d0, 0x9c, 0xe4, 0x08, 0x00, 0x3e, 0x30, 0x1f, 0x74);
DEFINE_GUID(GUID_DEVINTERFACE_IRDAENUM_BUS_ENUMERATOR,
  0x4D36E978L, 0xE325, 0x11CE, 0xBF, 0xC1, 0x08, 0x00, 0x2B, 0xE1, 0x03, 0x19);
#endif // DEFINE_GUID

#define WINDOWS_AF_IRDA 26
#define WINDOWS_PF_IRDA WINDOWS_AF_IRDA

#ifndef AF_IRDA
#define AF_IRDA WINDOWS_AF_IRDA
#endif

#define IRDA_PROTO_SOCK_STREAM 1
#define PF_IRDA AF_IRDA
#define SOL_IRLMP 0x00FF
#define SIO_LAZY_DISCOVERY _IOR('t', 127, ULONG)   


#define IAS_MAX_USER_STRING  256
#define IAS_MAX_OCTET_STRING 1024
#define IAS_MAX_CLASSNAME    64
#define IAS_MAX_ATTRIBNAME   256

#define IAS_ATTRIB_NO_CLASS  ((ULONG)0x10)
#define IAS_ATTRIB_NO_ATTRIB ((ULONG)0x00)
#define IAS_ATTRIB_INT       ((ULONG)0x01)
#define IAS_ATTRIB_OCTETSEQ  ((ULONG)0x02)
#define IAS_ATTRIB_STR       ((ULONG)0x03)

#define IRLMP_ENUMDEVICES    ((ULONG)0x10)
#define IRLMP_IAS_SET        ((ULONG)0x11)
#define IRLMP_IAS_QUERY      ((ULONG)0x12)
#define IRLMP_SEND_PDU_LEN   ((ULONG)0x13)
#define IRLMP_EXCLUSIVE_MODE ((ULONG)0x14)
#define IRLMP_IRLPT_MODE     ((ULONG)0x15)
#define IRLMP_9WIRE_MODE     ((ULONG)0x16)

#if 0
// Available/Used on Windows 98 only ???
#define IRLMP_TINYTP_MODE    ((ULONG)0x17)
#define IRLMP_PARAMETERS     ((ULONG)0x18)
#define IRLMP_DISCOVERY_MODE ((ULONG)0x19)
// Available/Used on Windows CE only ???
#define IRLMP_SHARP_MODE     ((ULONG)0x20)
#endif

enum {
// First hint byte
  LM_HB1_PnP =         0x01,
  LM_HB1_PDA_Palmtop = 0x02,
  LM_HB1_Computer =    0x04,
  LM_HB1_Printer =     0x08,
  LM_HB1_Modem =       0x10,
  LM_HB1_Fax =         0x20,
  LM_HB1_LANAccess =   0x40,
// Second hint byte
  LM_HB2_Telephony =   0x01,
  LM_HB2_FileServer =  0x02,
// Any hint byte
  LM_HB_Extension =    0x80,
};

#define LmCharSetASCII       0x00
#define LmCharSetISO_8859_1  0x01
#define LmCharSetISO_8859_2  0x02
#define LmCharSetISO_8859_3  0x03
#define LmCharSetISO_8859_4  0x04
#define LmCharSetISO_8859_5  0x05
#define LmCharSetISO_8859_6  0x06
#define LmCharSetISO_8859_7  0x07
#define LmCharSetISO_8859_8  0x08
#define LmCharSetISO_8859_9  0x09
#define LmCharSetUNICODE     0xFF

#define  LM_BAUD_1200   1200
#define  LM_BAUD_2400   2400
#define  LM_BAUD_9600   9600
#define  LM_BAUD_19200  19200
#define  LM_BAUD_38400  38400
#define  LM_BAUD_57600  57600
#define  LM_BAUD_115200 115200
#define  LM_BAUD_576K   576000
#define  LM_BAUD_1152K  1152000
#define  LM_BAUD_4M     4000000

#if 0 // Available/Used on Windows 98 only ???
typedef ULONG LM_BAUD_RATE;
typedef struct {
    ULONG        nTXDataBytes;  // packet transmit receive bytes
    ULONG        nRXDataBytes;  // packet maximum receive bytes
    LM_BAUD_RATE nBaudRate;     // link negotiated baud
    ULONG        thresholdTime; // milliseconds for threshold time
    ULONG        discTime;      // milliseconds for disconnect time
    USHORT       nMSLinkTurn;   // milliseconds for link turn around time
    UCHAR        nTXPackets;    // transmit window packets
    UCHAR        nRXPackets;    // receive window packets
} LM_IRPARMS;
typedef LM_IRPARMS *PLM_IRPARMS;
#endif

typedef struct _SOCKADDR_IRDA {
    USHORT irdaAddressFamily;
    UCHAR  irdaDeviceID[4];
    char   irdaServiceName[25];
} SOCKADDR_IRDA;

typedef struct _WINDOWS_IRDA_DEVICE_INFO {
    UCHAR irdaDeviceID[4];
    char  irdaDeviceName[22];
    UCHAR irdaDeviceHints1;
    UCHAR irdaDeviceHints2;
    UCHAR irdaCharSet;
} WINDOWS_IRDA_DEVICE_INFO;

typedef struct _WINDOWS_IAS_SET {
    char  irdaClassName[IAS_MAX_CLASSNAME];
    char  irdaAttribName[IAS_MAX_ATTRIBNAME];
    ULONG irdaAttribType;
    union {
        LONG irdaAttribInt;
        struct {
            USHORT Len;
            UCHAR OctetSeq[IAS_MAX_OCTET_STRING];
        } irdaAttribOctetSeq;
        struct {
            UCHAR Len;
            UCHAR CharSet;
            UCHAR UsrStr[IAS_MAX_USER_STRING];
        } irdaAttribUsrStr;
    } irdaAttribute;
} WINDOWS_IAS_SET;

typedef struct _WINDOWS_IAS_QUERY {
    UCHAR irdaDeviceID[4];
    char  irdaClassName[IAS_MAX_CLASSNAME];
    char  irdaAttribName[IAS_MAX_ATTRIBNAME];
    ULONG irdaAttribType;
    union {
        LONG irdaAttribInt;
        struct {
            ULONG Len;
            UCHAR OctetSeq[IAS_MAX_OCTET_STRING];
        } irdaAttribOctetSeq;
        struct {
            ULONG Len;
            ULONG CharSet;
            UCHAR UsrStr[IAS_MAX_USER_STRING];
        } irdaAttribUsrStr;
    } irdaAttribute;
} WINDOWS_IAS_QUERY;

typedef struct _WINDOWS_DEVICELIST {
    ULONG numDevice;
    WINDOWS_IRDA_DEVICE_INFO Device[1];
} WINDOWS_DEVICELIST;

typedef WINDOWS_DEVICELIST DEVICELIST;
typedef WINDOWS_DEVICELIST *PDEVICELIST;
typedef WINDOWS_DEVICELIST *PWINDOWS_DEVICELIST;

typedef WINDOWS_IRDA_DEVICE_INFO IRDA_DEVICE_INFO;
typedef WINDOWS_IRDA_DEVICE_INFO *PIRDA_DEVICE_INFO;
typedef WINDOWS_IRDA_DEVICE_INFO *PWINDOWS_IRDA_DEVICE_INFO;

typedef WINDOWS_IAS_SET IAS_SET;
typedef WINDOWS_IAS_SET *PIAS_SET;
typedef WINDOWS_IAS_SET *PWINDOWS_IAS_SET;

typedef WINDOWS_IAS_QUERY IAS_QUERY;
typedef WINDOWS_IAS_QUERY *PIAS_QUERY;
typedef WINDOWS_IAS_QUERY *PWINDOWS_IAS_QUERY;

typedef SOCKADDR_IRDA *PSOCKADDR_IRDA;

#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif /* __AF_IRDA_H */
