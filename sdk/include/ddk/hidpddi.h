#ifndef _HIDPDDI_H
#define _HIDPDDI_H

#include "hidusage.h"
#include "hidpi.h"

typedef struct _HIDP_COLLECTION_DESC
{
    USAGE  UsagePage;
    USAGE  Usage;
    UCHAR  CollectionNumber;
    UCHAR  Reserved [15];
    USHORT InputLength;
    USHORT OutputLength;
    USHORT FeatureLength;
    USHORT PreparsedDataLength;
    PHIDP_PREPARSED_DATA PreparsedData;
}HIDP_COLLECTION_DESC, *PHIDP_COLLECTION_DESC;

typedef struct _HIDP_REPORT_IDS
{
    UCHAR  ReportID;
    UCHAR  CollectionNumber;
    USHORT InputLength;
    USHORT OutputLength;
    USHORT FeatureLength;
}HIDP_REPORT_IDS, *PHIDP_REPORT_IDS;

typedef struct _HIDP_GETCOLDESC_DBG
{
    ULONG BreakOffset;
    ULONG ErrorCode;
    ULONG Args[6];
}HIDP_GETCOLDESC_DBG, *PHIDP_GETCOLDESC_DBG;

typedef struct _HIDP_DEVICE_DESC
{
    PHIDP_COLLECTION_DESC CollectionDesc;
    ULONG                 CollectionDescLength;
    PHIDP_REPORT_IDS      ReportIDs;
    ULONG                 ReportIDsLength;
    HIDP_GETCOLDESC_DBG   Dbg;
}HIDP_DEVICE_DESC, *PHIDP_DEVICE_DESC;

NTSTATUS
NTAPI
HidP_GetCollectionDescription(
    IN PHIDP_REPORT_DESCRIPTOR ReportDesc,
    IN ULONG DescLength,
    IN POOL_TYPE PoolType,
    OUT PHIDP_DEVICE_DESC DeviceDescription
);

VOID
NTAPI
HidP_FreeCollectionDescription (
    IN PHIDP_DEVICE_DESC DeviceDescription
);

NTSTATUS
NTAPI
HidP_SysPowerEvent (
    IN PCHAR HidPacket,
    IN USHORT HidPacketLength,
    IN PHIDP_PREPARSED_DATA Ppd,
    OUT PULONG OutputBuffer
);

NTSTATUS
NTAPI
HidP_SysPowerCaps (
    IN PHIDP_PREPARSED_DATA Ppd,
    OUT PULONG OutputBuffer
);

#define HIDP_GETCOLDESC_SUCCESS               0x00
#define HIDP_GETCOLDESC_RESOURCES             0x01
#define HIDP_GETCOLDESC_BUFFER                0x02
#define HIDP_GETCOLDESC_LINK_RESOURCES        0x03
#define HIDP_GETCOLDESC_UNEXP_END_COL         0x04
#define HIDP_GETCOLDESC_PREPARSE_RESOURCES    0x05
#define HIDP_GETCOLDESC_ONE_BYTE              0x06
#define HIDP_GETCOLDESC_TWO_BYTE              0x07
#define HIDP_GETCOLDESC_FOUR_BYTE             0x08
#define HIDP_GETCOLDESC_BYTE_ALLIGN           0x09
#define HIDP_GETCOLDESC_MAIN_ITEM_NO_USAGE    0x0A
#define HIDP_GETCOLDESC_TOP_COLLECTION_USAGE  0x0B
#define HIDP_GETCOLDESC_PUSH_RESOURCES        0x10
#define HIDP_GETCOLDESC_ITEM_UNKNOWN          0x12
#define HIDP_GETCOLDESC_REPORT_ID             0x13
#define HIDP_GETCOLDESC_BAD_REPORT_ID         0x14
#define HIDP_GETCOLDESC_NO_REPORT_ID          0x15
#define HIDP_GETCOLDESC_DEFAULT_ID_ERROR      0x16
#define HIDP_GETCOLDESC_NO_DATA               0x1A
#define HIDP_GETCOLDESC_INVALID_MAIN_ITEM     0x1B
#define HIDP_GETCOLDESC_NO_CLOSE_DELIMITER    0x20
#define HIDP_GETCOLDESC_NOT_VALID_DELIMITER   0x21
#define HIDP_GETCOLDESC_MISMATCH_OC_DELIMITER 0x22
#define HIDP_GETCOLDESC_UNSUPPORTED           0x40

#endif
