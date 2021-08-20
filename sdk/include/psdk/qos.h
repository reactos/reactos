#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef ULONG SERVICETYPE;

#define SERVICETYPE_NOTRAFFIC               0x00000000
#define SERVICETYPE_BESTEFFORT              0x00000001
#define SERVICETYPE_CONTROLLEDLOAD          0x00000002
#define SERVICETYPE_GUARANTEED              0x00000003
#define SERVICETYPE_NETWORK_UNAVAILABLE     0x00000004
#define SERVICETYPE_GENERAL_INFORMATION     0x00000005
#define SERVICETYPE_NOCHANGE                0x00000006
#define SERVICETYPE_NONCONFORMING           0x00000009
#define SERVICETYPE_NETWORK_CONTROL         0x0000000A
#define SERVICETYPE_QUALITATIVE             0x0000000D

#define SERVICE_BESTEFFORT                  0x80010000
#define SERVICE_CONTROLLEDLOAD              0x80020000
#define SERVICE_GUARANTEED                  0x80040000
#define SERVICE_QUALITATIVE                 0x80200000

#define SERVICE_NO_TRAFFIC_CONTROL          0x81000000

#define SERVICE_NO_QOS_SIGNALING            0x40000000

#define QOS_NOT_SPECIFIED                   0xFFFFFFFF

#define POSITIVE_INFINITY_RATE              0xFFFFFFFE

#define QOS_GENERAL_ID_BASE                 2000

#define QOS_OBJECT_END_OF_LIST              (0x00000001 + QOS_GENERAL_ID_BASE)
#define   QOS_OBJECT_SD_MODE                (0x00000002 + QOS_GENERAL_ID_BASE)
#define   QOS_OBJECT_SHAPING_RATE           (0x00000003 + QOS_GENERAL_ID_BASE)
#define   QOS_OBJECT_DESTADDR               (0x00000004 + QOS_GENERAL_ID_BASE)

#define TC_NONCONF_BORROW                   0
#define TC_NONCONF_SHAPE                    1
#define TC_NONCONF_DISCARD                  2
#define TC_NONCONF_BORROW_PLUS              3

typedef struct _flowspec {
  ULONG TokenRate;
  ULONG TokenBucketSize;
  ULONG PeakBandwidth;
  ULONG Latency;
  ULONG DelayVariation;
  SERVICETYPE ServiceType;
  ULONG MaxSduSize;
  ULONG MinimumPolicedSize;
} FLOWSPEC, *PFLOWSPEC, *LPFLOWSPEC;

typedef struct _QOS_OBJECT_HDR {
  ULONG ObjectType;
  ULONG ObjectLength;
} QOS_OBJECT_HDR, *LPQOS_OBJECT_HDR;

typedef struct _QOS_SD_MODE {
  QOS_OBJECT_HDR ObjectHdr;
  ULONG ShapeDiscardMode;
} QOS_SD_MODE, *LPQOS_SD_MODE;

typedef struct _QOS_SHAPING_RATE {
  QOS_OBJECT_HDR ObjectHdr;
  ULONG ShapingRate;
} QOS_SHAPING_RATE, *LPQOS_SHAPING_RATE;

#ifdef __cplusplus
}
#endif
