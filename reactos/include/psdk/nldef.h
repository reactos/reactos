#ifndef _NLDEF_
#define _NLDEF_

#pragma once

#define NL_MAX_METRIC_COMPONENT ((((ULONG) 1) << 31) - 1)

typedef enum {
  IpPrefixOriginOther = 0,
  IpPrefixOriginManual,
  IpPrefixOriginWellKnown,
  IpPrefixOriginDhcp,
  IpPrefixOriginRouterAdvertisement,
  IpPrefixOriginUnchanged = 1 << 4
} NL_PREFIX_ORIGIN;

#define NlpoOther               IpPrefixOriginOther
#define NlpoManual              IpPrefixOriginManual
#define NlpoWellKnown           IpPrefixOriginWellKnown
#define NlpoDhcp                IpPrefixOriginDhcp
#define NlpoRouterAdvertisement IpPrefixOriginRouterAdvertisement

typedef enum {
  NlsoOther = 0,
  NlsoManual,
  NlsoWellKnown,
  NlsoDhcp,
  NlsoLinkLayerAddress,
  NlsoRandom,
  IpSuffixOriginOther = 0,
  IpSuffixOriginManual,
  IpSuffixOriginWellKnown,
  IpSuffixOriginDhcp,
  IpSuffixOriginLinkLayerAddress,
  IpSuffixOriginRandom,
  IpSuffixOriginUnchanged = 1 << 4
} NL_SUFFIX_ORIGIN;

typedef enum {
  NldsInvalid,
  NldsTentative,
  NldsDuplicate,
  NldsDeprecated,
  NldsPreferred,
  IpDadStateInvalid = 0,
  IpDadStateTentative,
  IpDadStateDuplicate,
  IpDadStateDeprecated,
  IpDadStatePreferred,
} NL_DAD_STATE;

#define MAKE_ROUTE_PROTOCOL(suffix, value) \
    MIB_IPPROTO_ ## suffix = value,        \
    PROTO_IP_ ## suffix    = value

typedef enum {
  RouteProtocolOther = 1,
  RouteProtocolLocal = 2,
  RouteProtocolNetMgmt = 3,
  RouteProtocolIcmp = 4,
  RouteProtocolEgp = 5,
  RouteProtocolGgp = 6,
  RouteProtocolHello = 7,
  RouteProtocolRip = 8,
  RouteProtocolIsIs = 9,
  RouteProtocolEsIs = 10,
  RouteProtocolCisco = 11,
  RouteProtocolBbn = 12,
  RouteProtocolOspf = 13,
  RouteProtocolBgp = 14,
  MAKE_ROUTE_PROTOCOL(OTHER, 1),
  MAKE_ROUTE_PROTOCOL(LOCAL, 2),
  MAKE_ROUTE_PROTOCOL(NETMGMT, 3),
  MAKE_ROUTE_PROTOCOL(ICMP, 4),
  MAKE_ROUTE_PROTOCOL(EGP, 5),
  MAKE_ROUTE_PROTOCOL(GGP, 6),
  MAKE_ROUTE_PROTOCOL(HELLO, 7),
  MAKE_ROUTE_PROTOCOL(RIP, 8),
  MAKE_ROUTE_PROTOCOL(IS_IS, 9),
  MAKE_ROUTE_PROTOCOL(ES_IS, 10),
  MAKE_ROUTE_PROTOCOL(CISCO, 11),
  MAKE_ROUTE_PROTOCOL(BBN, 12),
  MAKE_ROUTE_PROTOCOL(OSPF, 13),
  MAKE_ROUTE_PROTOCOL(BGP, 14),
  MAKE_ROUTE_PROTOCOL(NT_AUTOSTATIC, 10002),
  MAKE_ROUTE_PROTOCOL(NT_STATIC, 10006),
  MAKE_ROUTE_PROTOCOL(NT_STATIC_NON_DOD, 10007),
} NL_ROUTE_PROTOCOL, *PNL_ROUTE_PROTOCOL;

typedef enum {
  NlatUnspecified,
  NlatUnicast,
  NlatAnycast,
  NlatMulticast,
  NlatBroadcast,
  NlatInvalid
} NL_ADDRESS_TYPE, *PNL_ADDRESS_TYPE;

typedef enum _NL_ROUTE_ORIGIN {
  NlroManual,
  NlroWellKnown,
  NlroDHCP,
  NlroRouterAdvertisement,
  Nlro6to4,
} NL_ROUTE_ORIGIN, *PNL_ROUTE_ORIGIN;

typedef enum _NL_NEIGHBOR_STATE {
  NlnsUnreachable,
  NlnsIncomplete,
  NlnsProbe,
  NlnsDelay,
  NlnsStale,
  NlnsReachable,
  NlnsPermanent,
  NlnsMaximum,
} NL_NEIGHBOR_STATE, *PNL_NEIGHBOR_STATE;

typedef enum _NL_LINK_LOCAL_ADDRESS_BEHAVIOR{
  LinkLocalAlwaysOff = 0,
  LinkLocalDelayed,
  LinkLocalAlwaysOn,
  LinkLocalUnchanged = -1
} NL_LINK_LOCAL_ADDRESS_BEHAVIOR;

typedef struct _NL_INTERFACE_OFFLOAD_ROD {
  BOOLEAN NlChecksumSupported:1;
  BOOLEAN NlOptionsSupported:1;
  BOOLEAN TlDatagramChecksumSupported:1;
  BOOLEAN TlStreamChecksumSupported:1;
  BOOLEAN TlStreamOptionsSupported:1;
  BOOLEAN FastPathCompatible:1;
  BOOLEAN TlLargeSendOffloadSupported:1;
  BOOLEAN TlGiantSendOffloadSupported:1;
} NL_INTERFACE_OFFLOAD_ROD, *PNL_INTERFACE_OFFLOAD_ROD;

typedef enum _NL_ROUTER_DISCOVERY_BEHAVIOR {
  RouterDiscoveryDisabled = 0,
  RouterDiscoveryEnabled,
  RouterDiscoveryDhcp,
  RouterDiscoveryUnchanged = -1
} NL_ROUTER_DISCOVERY_BEHAVIOR;

typedef enum _NL_BANDWIDTH_FLAG {
  NlbwDisabled = 0,
  NlbwEnabled,
  NlbwUnchanged = -1
} NL_BANDWIDTH_FLAG, *PNL_BANDWIDTH_FLAG;

typedef struct _NL_PATH_BANDWIDTH_ROD {
  ULONG64 Bandwidth;
  ULONG64 Instability;
  BOOLEAN BandwidthPeaked;
} NL_PATH_BANDWIDTH_ROD, *PNL_PATH_BANDWIDTH_ROD;

typedef enum _NL_NETWORK_CATEGORY {
  NetworkCategoryPublic,
  NetworkCategoryPrivate,
  NetworkCategoryDomainAuthenticated,
  NetworkCategoryUnchanged = -1,
  NetworkCategoryUnknown = -1
} NL_NETWORK_CATEGORY, *PNL_NETWORK_CATEGORY;

#define NET_IF_CURRENT_SESSION ((ULONG)-1)

#endif /* _NLDEF_ */

