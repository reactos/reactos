/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 1996  Microsoft Corporation

Module Name:

    vars.c

Abstract:

    Globals for winsock browser util.

Author:

    Dan Knudson (DanKn)    29-Jul-1996

Revision History:

--*/


#include <stdio.h>
#include "sockeye.h"
#include "wsipx.h"
#include "nspapi.h"

#ifdef WIN32
#define my_far
#else
#define my_far _far
#endif


FILE        *hLogFile = (FILE *) NULL;
HANDLE      ghInst;
HWND        ghwndMain, ghwndEdit, ghwndList1, ghwndList2, ghwndList3;
BOOL        bShowParams = FALSE;
BOOL        gbDisableHandleChecking;
LPBYTE      pBigBuf;
DWORD       dwBigBufSize;
BOOL        bDumpParams = FALSE;
BOOL        gbWideStringParams;
BOOL        bTimeStamp;
DWORD       dwDumpStructsFlags;

DWORD       aUserButtonFuncs[MAX_USER_BUTTONS];
char        aUserButtonsText[MAX_USER_BUTTONS][MAX_USER_BUTTON_TEXT_SIZE];

char my_far szTab[] = "  ";

char aAscii[] =
{
     46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46,
     46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46,
     32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
     48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
     64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
     80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
     96, 97, 98, 99,100,101,102,103,104,105,106,107,108,109,110,111,
    112,113,114,115,116,117,118,119,120,121,122,123,124,125,126, 46,

     46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46,
     46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46,
     46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46,
     46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46,
     46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46,
     46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46,
     46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46,
     46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46
};


BYTE aHex[] =
{
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
      0,  1,  2,  3,  4,  5,  6,  7,  8,  9,255,255,255,255,255,255,
    255, 10, 11, 12, 13, 14, 15,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255, 10, 11, 12, 13, 14, 15,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
};

LOOKUP aAddressFamilies[] =
{
     { AF_UNSPEC                    ,"AF_UNSPEC"                },
     { AF_UNIX                      ,"AF_UNIX"                  },
     { AF_INET                      ,"AF_INET"                  },
     { AF_IMPLINK                   ,"AF_IMPLINK"               },
     { AF_PUP                       ,"AF_PUP"                   },
     { AF_CHAOS                     ,"AF_CHAOS"                 },
     { AF_NS                        ,"AF_NS (AF_IPX)"           },
     { AF_ISO                       ,"AF_ISO (AF_OSI)"          },
     { AF_ECMA                      ,"AF_ECMA"                  },
     { AF_DATAKIT                   ,"AF_DATAKIT"               },
     { AF_CCITT                     ,"AF_CCITT"                 },
     { AF_SNA                       ,"AF_SNA"                   },
     { AF_DECnet                    ,"AF_DECnet"                },
     { AF_DLI                       ,"AF_DLI"                   },
     { AF_LAT                       ,"AF_LAT"                   },
     { AF_HYLINK                    ,"AF_HYLINK"                },
     { AF_APPLETALK                 ,"AF_APPLETALK"             },
     { AF_NETBIOS                   ,"AF_NETBIOS"               },
     { AF_VOICEVIEW                 ,"AF_VOICEVIEW"             },
     { AF_FIREFOX                   ,"AF_FIREFOX"               },
     { AF_UNKNOWN1                  ,"AF_UNKNOWN1"              },
     { AF_BAN                       ,"AF_BAN"                   },
     { AF_ATM                       ,"AF_ATM"                   },
     { AF_INET6                     ,"AF_INET6"                 },
     { 0xffffffff                   ,NULL                       }
};

LOOKUP aIoctlCmds[] =
{
     { FIONBIO                      ,"FIONBIO"                  },
     { FIONREAD                     ,"FIONREAD"                 },
     { SIOCATMARK                   ,"SIOCATMARK"               },
     { 0xffffffff                   ,NULL                       }
};

LOOKUP aJLFlags[] =
{
     { JL_SENDER_ONLY               ,"JL_SENDER_ONLY"           },
     { JL_RECEIVER_ONLY             ,"JL_RECEIVER_ONLY"         },
     { JL_BOTH                      ,"JL_BOTH"                  },
     { 0xffffffff                   ,NULL                       }
};

LOOKUP aNameSpaces[] =
{
     { NS_ALL                       ,"NS_ALL"                   },
     { NS_SAP                       ,"NS_SAP"                   },
     { NS_NDS                       ,"NS_NDS"                   },
     { NS_PEER_BROWSE               ,"NS_PEER_BROWSE"           },
     { NS_TCPIP_LOCAL               ,"NS_TCPIP_LOCAL"           },
     { NS_TCPIP_HOSTS               ,"NS_TCPIP_HOSTS"           },
     { NS_DNS                       ,"NS_DNS"                   },
     { NS_NETBT                     ,"NS_NETBT"                 },
     { NS_WINS                      ,"NS_WINS"                  },
     { NS_NBP                       ,"NS_NBP"                   },
     { NS_MS                        ,"NS_MS"                    },
     { NS_STDA                      ,"NS_STDA"                  },
     { NS_NTDS                      ,"NS_NTDS"                 },
     { NS_X500                      ,"NS_X500"                  },
     { NS_NIS                       ,"NS_NIS"                   },
     { NS_NISPLUS                   ,"NS_NISPLUS"               },
     { NS_WRQ                       ,"NS_WRQ"                   },
     { 0xffffffff                   ,NULL                       }
};

LOOKUP aNetworkByteOrders[] =
{
     { BIGENDIAN                    ,"BIGENDIAN"                },
     { LITTLEENDIAN                 ,"LITTLEENDIAN"             },
     { 0xffffffff                   ,NULL                       }
};

LOOKUP aNetworkEvents[] =
{
     { FD_READ                      ,"FD_READ"                  },
     { FD_WRITE                     ,"FD_WRITE"                 },
     { FD_OOB                       ,"FD_OOB"                   },
     { FD_ACCEPT                    ,"FD_ACCEPT"                },
     { FD_CONNECT                   ,"FD_CONNECT"               },
     { FD_CLOSE                     ,"FD_CLOSE"                 },
     { FD_QOS                       ,"FD_QOS"                   },
     { FD_GROUP_QOS                 ,"FD_GROUP_QOS"             },
     { 0xffffffff                   ,NULL                       }
};

LOOKUP aProperties[] =
{
     { PROP_COMMENT                 ,"PROP_COMMENT     "        },
     { PROP_LOCALE                  ,"PROP_LOCALE      "        },
     { PROP_DISPLAY_HINT            ,"PROP_DISPLAY_HINT"        },
     { PROP_VERSION                 ,"PROP_VERSION     "        },
     { PROP_START_TIME              ,"PROP_START_TIME  "        },
     { PROP_MACHINE                 ,"PROP_MACHINE     "        },
     { PROP_ADDRESSES               ,"PROP_ADDRESSES   "        },
     { PROP_SD                      ,"PROP_SD          "        },
     { PROP_ALL                     ,"PROP_ALL         "        },
     { 0xffffffff                   ,NULL                       }
};

LOOKUP aProtocols[] =
{
     { IPPROTO_IP                   ,"IPPROTO_IP"               },
     { IPPROTO_ICMP                 ,"IPPROTO_ICMP"             },
     { IPPROTO_IGMP                 ,"IPPROTO_IGMP"             },
     { IPPROTO_GGP                  ,"IPPROTO_GGP"              },
     { IPPROTO_TCP                  ,"IPPROTO_TCP"              },
     { IPPROTO_PUP                  ,"IPPROTO_PUP"              },
     { IPPROTO_UDP                  ,"IPPROTO_UDP"              },
     { IPPROTO_IDP                  ,"IPPROTO_IDP"              },
     { IPPROTO_ND                   ,"IPPROTO_ND"               },
     { IPPROTO_RAW                  ,"IPPROTO_RAW"              },
     { IPPROTO_MAX                  ,"IPPROTO_MAX"              },
     { NSPROTO_IPX                  ,"NSPROTO_IPX"              },
     { NSPROTO_SPX                  ,"NSPROTO_SPX"              },
     { NSPROTO_SPXII                ,"NSPROTO_SPXII"            },
     { 0xffffffff                   ,NULL                       }
};

LOOKUP aProviderFlags[] =
{
     { PFL_MULTIPLE_PROTO_ENTRIES   ,"MULTIPLE_PROTO_ENTRIES"   },
     { PFL_RECOMMENDED_PROTO_ENTRY  ,"RECOMMENDED_PROTO_ENTRY"  },
     { PFL_HIDDEN                   ,"HIDDEN"                   },
     { PFL_MATCHES_PROTOCOL_ZERO    ,"MATCHES_PROTOCOL_ZERO"    },
     { 0xffffffff                   ,NULL                       }
};

LOOKUP aQOSServiceTypes[] =
{
     { SERVICETYPE_NOTRAFFIC            ,"NOTRAFFIC"            },
     { SERVICETYPE_BESTEFFORT           ,"BESTEFFORT"           },
     { SERVICETYPE_CONTROLLEDLOAD       ,"CONTROLLEDLOAD"       },
     { SERVICETYPE_GUARANTEED           ,"GUARANTEED"           },
     { SERVICETYPE_NETWORK_UNAVAILABLE  ,"NETWORK_UNAVAILABLE"  },
     { SERVICETYPE_GENERAL_INFORMATION  ,"GENERAL_INFORMATION"  },
     { SERVICETYPE_NOCHANGE             ,"NOCHANGE"             },
     { 0xffffffff                       ,NULL                   }
};

LOOKUP aRecvFlags[] =
{
     { MSG_PEEK                     ,"MSG_PEEK"                 },
     { MSG_OOB                      ,"MSG_OOB"                  },
     { 0xffffffff                   ,NULL                       }
};

LOOKUP aResDisplayTypes[] =
{
     { RESOURCEDISPLAYTYPE_GENERIC      ,"RESOURCEDISPLAYTYPE_GENERIC"      },
     { RESOURCEDISPLAYTYPE_DOMAIN       ,"RESOURCEDISPLAYTYPE_DOMAIN"       },
     { RESOURCEDISPLAYTYPE_SERVER       ,"RESOURCEDISPLAYTYPE_SERVER"       },
     { RESOURCEDISPLAYTYPE_SHARE        ,"RESOURCEDISPLAYTYPE_SHARE"        },
     { RESOURCEDISPLAYTYPE_FILE         ,"RESOURCEDISPLAYTYPE_FILE"         },
     { RESOURCEDISPLAYTYPE_GROUP        ,"RESOURCEDISPLAYTYPE_GROUP"        },
     { RESOURCEDISPLAYTYPE_NETWORK      ,"RESOURCEDISPLAYTYPE_NETWORK"      },
     { RESOURCEDISPLAYTYPE_ROOT         ,"RESOURCEDISPLAYTYPE_ROOT"         },
     { RESOURCEDISPLAYTYPE_SHAREADMIN   ,"RESOURCEDISPLAYTYPE_SHAREADMIN"   },
     { RESOURCEDISPLAYTYPE_DIRECTORY    ,"RESOURCEDISPLAYTYPE_DIRECTORY"    },
     { RESOURCEDISPLAYTYPE_TREE         ,"RESOURCEDISPLAYTYPE_TREE"         },
     { RESOURCEDISPLAYTYPE_NDSCONTAINER ,"RESOURCEDISPLAYTYPE_NDSCONTAINER" },
     { 0xffffffff                       ,NULL                               }
};

LOOKUP aResFlags[] =
{
     { RES_SERVICE                  ,"RES_SERVICE"              },
     { RES_FIND_MULTIPLE            ,"RES_FIND_MULTIPLE"        },
     { RES_SOFT_SEARCH              ,"RES_SOFT_SEARCH"          },
     { 0xffffffff                   ,NULL                       }
};

LOOKUP aSendFlags[] =
{
     { MSG_DONTROUTE                ,"MSG_DONTROUTE"            },
     { MSG_OOB                      ,"MSG_OOB"                  },
     { 0xffffffff                   ,NULL                       }
};

LOOKUP aShutdownOps[] =
{
     { SD_RECEIVE                   ,"SD_RECEIVE"               },
     { SD_SEND                      ,"SD_SEND"                  },
     { SD_BOTH                      ,"SD_BOTH"                  },
     { 0xffffffff                   ,NULL                       }
};

LOOKUP aSocketTypes[] =
{
     { SOCK_STREAM                  ,"SOCK_STREAM"              },
     { SOCK_DGRAM                   ,"SOCK_DGRAM"               },
     { SOCK_RAW                     ,"SOCK_RAW"                 },
     { SOCK_RDM                     ,"SOCK_RDM"                 },
     { SOCK_SEQPACKET               ,"SOCK_SEQPACKET"           },
     { 0xffffffff                   ,NULL                       }
};

LOOKUP aSockOptLevels[] =
{
     { SOL_SOCKET                   ,"SOL_SOCKET"               },
     { IPPROTO_TCP                  ,"IPPROTO_TCP"              },
     { 0xffffffff                   ,NULL                       }
};

LOOKUP aSockOpts[] =
{
     { SO_ACCEPTCONN                ,"SO_ACCEPTCONN"            },
     { SO_BROADCAST                 ,"SO_BROADCAST"             },
     { SO_DEBUG                     ,"SO_DEBUG"                 },
     { (DWORD) SO_DONTLINGER        ,"SO_DONTLINGER"            },
     { SO_DONTROUTE                 ,"SO_DONTROUTE"             },
     { SO_ERROR                     ,"SO_ERROR"                 },
     { SO_GROUP_ID                  ,"SO_GROUP_ID"              },
     { SO_GROUP_PRIORITY            ,"SO_GROUP_PRIORITY"        },
     { SO_KEEPALIVE                 ,"SO_KEEPALIVE"             },
     { SO_LINGER                    ,"SO_LINGER"                },
     { SO_MAX_MSG_SIZE              ,"SO_MAX_MSG_SIZE"          },
     { SO_OOBINLINE                 ,"SO_OOBINLINE"             },
     { SO_PROTOCOL_INFO             ,"SO_PROTOCOL_INFO"         },
     { SO_RCVBUF                    ,"SO_RCVBUF"                },
     { SO_REUSEADDR                 ,"SO_REUSEADDR"             },
     { SO_SNDBUF                    ,"SO_SNDBUF"                },
     { SO_TYPE                      ,"SO_TYPE"                  },
     { PVD_CONFIG                   ,"PVD_CONFIG"               },

     { TCP_NODELAY                  ,"TCP_NODELAY"              },
     { 0xffffffff                   ,NULL                       }
};

LOOKUP aServiceFlags[] =
{
     { XP1_CONNECTIONLESS           ,"CONNECTIONLESS"           },
     { XP1_GUARANTEED_DELIVERY      ,"GUARANTEED_DELIVERY"      },
     { XP1_GUARANTEED_ORDER         ,"GUARANTEED_ORDER"         },
     { XP1_MESSAGE_ORIENTED         ,"MESSAGE_ORIENTED"         },
     { XP1_PSEUDO_STREAM            ,"PSEUDO_STREAM"            },
     { XP1_GRACEFUL_CLOSE           ,"GRACEFUL_CLOSE"           },
     { XP1_EXPEDITED_DATA           ,"EXPEDITED_DATA"           },
     { XP1_CONNECT_DATA             ,"CONNECT_DATA"             },
     { XP1_DISCONNECT_DATA          ,"DISCONNECT_DATA"          },
     { XP1_SUPPORT_BROADCAST        ,"SUPPORT_BROADCAST"        },
     { XP1_SUPPORT_MULTIPOINT       ,"SUPPORT_MULTIPOINT"       },
     { XP1_MULTIPOINT_CONTROL_PLANE ,"MULTIPOINT_CONTROL_PLANE" },
     { XP1_MULTIPOINT_DATA_PLANE    ,"MULTIPOINT_DATA_PLANE"    },
     { XP1_QOS_SUPPORTED            ,"QOS_SUPPORTED"            },
     { XP1_INTERRUPT                ,"INTERRUPT"                },
     { XP1_UNI_SEND                 ,"UNI_SEND"                 },
     { XP1_UNI_RECV                 ,"UNI_RECV"                 },
     { XP1_IFS_HANDLES              ,"IFS_HANDLES"              },
     { XP1_PARTIAL_MESSAGE          ,"PARTIAL_MESSAGE"          },
     { 0xffffffff                   ,NULL                       }
};

LOOKUP aServiceOps[] =
{
     { SERVICE_REGISTER             ,"SERVICE_REGISTER"         },
     { SERVICE_DEREGISTER           ,"SERVICE_DEREGISTER"       },
     { SERVICE_FLUSH                ,"SERVICE_FLUSH"            },
     { SERVICE_ADD_TYPE             ,"SERVICE_ADD_TYPE"         },
     { SERVICE_DELETE_TYPE          ,"SERVICE_DELETE_TYPE"      },
     { 0xffffffff                   ,NULL                       }
};

LOOKUP aSvcFlags[] =
{
     { SERVICE_FLAG_DEFER           ,"SERVICE_FLAG_DEFER"       },
     { SERVICE_FLAG_HARD            ,"SERVICE_FLAG_HARD"        },
     { 0xffffffff                   ,NULL                       }
};

LOOKUP aWSAFlags[] =
{
     { WSA_FLAG_OVERLAPPED          ,"OVERLAPPED"               },
     { WSA_FLAG_MULTIPOINT_C_ROOT   ,"MULTIPOINT_C_ROOT"        },
     { WSA_FLAG_MULTIPOINT_C_LEAF   ,"MULTIPOINT_C_LEAF"        },
     { WSA_FLAG_MULTIPOINT_D_ROOT   ,"MULTIPOINT_D_ROOT"        },
     { WSA_FLAG_MULTIPOINT_D_LEAF   ,"MULTIPOINT_D_LEAF"        },
     { 0xffffffff                   ,NULL                       }
};

LOOKUP aWSAIoctlCmds[] =
{
     { FIONBIO                      ,"FIONBIO"                  },
     { FIONREAD                     ,"FIONREAD"                 },
     { SIOCATMARK                   ,"SIOCATMARK"               },
     { SIO_ASSOCIATE_HANDLE         ,"SIO_ASSOCIATE_HANDLE"     },
     { SIO_ENABLE_CIRCULAR_QUEUEING ,"SIO_ENABLE_CIRCULAR_QUEUEING" },
     { SIO_FIND_ROUTE               ,"SIO_FIND_ROUTE"           },
     { SIO_FLUSH                    ,"SIO_FLUSH"                },
     { SIO_GET_BROADCAST_ADDRESS    ,"SIO_GET_BROADCAST_ADDRESS" },
     { SIO_GET_EXTENSION_FUNCTION_POINTER
                                    ,"SIO_GET_EXTENSION_FUNCTION_POINTER" },
     { SIO_GET_QOS                  ,"SIO_GET_QOS"              },
     { SIO_GET_GROUP_QOS            ,"SIO_GET_GROUP_QOS"        },
     { SIO_MULTIPOINT_LOOPBACK      ,"SIO_MULTIPOINT_LOOPBACK"  },
     { SIO_MULTICAST_SCOPE          ,"SIO_MULTICAST_SCOPE"      },
     { SIO_SET_QOS                  ,"SIO_SET_QOS"              },
     { SIO_SET_GROUP_QOS            ,"SIO_SET_GROUP_QOS"        },
     { SIO_TRANSLATE_HANDLE         ,"SIO_TRANSLATE_HANDLE"     },
     { 0xffffffff                   ,NULL                       }
};

LOOKUP aWSARecvFlags[] =
{
     { MSG_PEEK                     ,"MSG_PEEK"                 },
     { MSG_OOB                      ,"MSG_OOB"                  },
     { MSG_PARTIAL                  ,"MSG_PARTIAL"              },
     { 0xffffffff                   ,NULL                       }
};

LOOKUP aWSASendFlags[] =
{
     { MSG_DONTROUTE                ,"MSG_DONTROUTE"            },
     { MSG_OOB                      ,"MSG_OOB"                  },
     { MSG_PARTIAL                  ,"MSG_PARTIAL"              },
     { 0xffffffff                   ,NULL                       }
};

LOOKUP aWSASendAndRecvFlags[] =
{
     { MSG_PEEK                     ,"MSG_PEEK"                 },
     { MSG_DONTROUTE                ,"MSG_DONTROUTE"            },
     { MSG_OOB                      ,"MSG_OOB"                  },
     { MSG_PARTIAL                  ,"MSG_PARTIAL"              },
     { 0xffffffff                   ,NULL                       }
};

LOOKUP aWSAErrors[] =
{
    { WSAEINTR                      ,"WSAEINTR"                 },
    { WSAEBADF                      ,"WSAEBADF"                 },
    { WSAEACCES                     ,"WSAEACCES"                },
    { WSAEFAULT                     ,"WSAEFAULT"                },
    { WSAEINVAL                     ,"WSAEINVAL"                },
    { WSAEMFILE                     ,"WSAEMFILE"                },

    { WSAEWOULDBLOCK                ,"WSAEWOULDBLOCK"           },
    { WSAEINPROGRESS                ,"WSAEINPROGRESS"           },
    { WSAEALREADY                   ,"WSAEALREADY"              },
    { WSAENOTSOCK                   ,"WSAENOTSOCK"              },
    { WSAEDESTADDRREQ               ,"WSAEDESTADDRREQ"          },
    { WSAEMSGSIZE                   ,"WSAEMSGSIZE"              },
    { WSAEPROTOTYPE                 ,"WSAEPROTOTYPE"            },
    { WSAENOPROTOOPT                ,"WSAENOPROTOOPT"           },
    { WSAEPROTONOSUPPORT            ,"WSAEPROTONOSUPPORT"       },
    { WSAESOCKTNOSUPPORT            ,"WSAESOCKTNOSUPPORT"       },
    { WSAEOPNOTSUPP                 ,"WSAEOPNOTSUPP"            },
    { WSAEPFNOSUPPORT               ,"WSAEPFNOSUPPORT"          },
    { WSAEAFNOSUPPORT               ,"WSAEAFNOSUPPORT"          },
    { WSAEADDRINUSE                 ,"WSAEADDRINUSE"            },
    { WSAEADDRNOTAVAIL              ,"WSAEADDRNOTAVAIL"         },
    { WSAENETDOWN                   ,"WSAENETDOWN"              },
    { WSAENETUNREACH                ,"WSAENETUNREACH"           },
    { WSAENETRESET                  ,"WSAENETRESET"             },
    { WSAECONNABORTED               ,"WSAECONNABORTED"          },
    { WSAECONNRESET                 ,"WSAECONNRESET"            },
    { WSAENOBUFS                    ,"WSAENOBUFS"               },
    { WSAEISCONN                    ,"WSAEISCONN"               },
    { WSAENOTCONN                   ,"WSAENOTCONN"              },
    { WSAESHUTDOWN                  ,"WSAESHUTDOWN"             },
    { WSAETOOMANYREFS               ,"WSAETOOMANYREFS"          },
    { WSAETIMEDOUT                  ,"WSAETIMEDOUT"             },
    { WSAECONNREFUSED               ,"WSAECONNREFUSED"          },
    { WSAELOOP                      ,"WSAELOOP"                 },
    { WSAENAMETOOLONG               ,"WSAENAMETOOLONG"          },
    { WSAEHOSTDOWN                  ,"WSAEHOSTDOWN"             },
    { WSAEHOSTUNREACH               ,"WSAEHOSTUNREACH"          },
    { WSAENOTEMPTY                  ,"WSAENOTEMPTY"             },
    { WSAEPROCLIM                   ,"WSAEPROCLIM"              },
    { WSAEUSERS                     ,"WSAEUSERS"                },
    { WSAEDQUOT                     ,"WSAEDQUOT"                },
    { WSAESTALE                     ,"WSAESTALE"                },
    { WSAEREMOTE                    ,"WSAEREMOTE"               },

    { WSASYSNOTREADY                ,"WSASYSNOTREADY"           },
    { WSAVERNOTSUPPORTED            ,"WSAVERNOTSUPPORTED"       },
    { WSANOTINITIALISED             ,"WSANOTINITIALISED"        },
    { WSAEDISCON                    ,"WSAEDISCON"               },
    { WSAENOMORE                    ,"WSAENOMORE"               },
    { WSAECANCELLED                 ,"WSAECANCELLED"            },
    { WSAEINVALIDPROCTABLE          ,"WSAEINVALIDPROCTABLE"     },
    { WSAEINVALIDPROVIDER           ,"WSAEINVALIDPROVIDER"      },
    { WSAEPROVIDERFAILEDINIT        ,"WSAEPROVIDERFAILEDINIT"   },
    { WSASYSCALLFAILURE             ,"WSASYSCALLFAILURE"        },
    { WSASERVICE_NOT_FOUND          ,"WSASERVICE_NOT_FOUND"     },
    { WSATYPE_NOT_FOUND             ,"WSATYPE_NOT_FOUND"        },
    { WSA_E_NO_MORE                 ,"WSA_E_NO_MORE"            },
    { WSA_E_CANCELLED               ,"WSA_E_CANCELLED"          },
    { WSAEREFUSED                   ,"WSAEREFUSED"              },

    { WSAHOST_NOT_FOUND             ,"WSAHOST_NOT_FOUND"        },
    { WSATRY_AGAIN                  ,"WSATRY_AGAIN"             },
    { WSANO_RECOVERY                ,"WSANO_RECOVERY"           },
    { WSANO_DATA                    ,"WSANO_DATA"               },
#ifdef WIN32
    { WSA_IO_PENDING                ,"WSA_IO_PENDING"           },
    { WSA_IO_INCOMPLETE             ,"WSA_IO_INCOMPLETE"        },
    { WSA_INVALID_HANDLE            ,"WSA_INVALID_HANDLE"       },
    { WSA_INVALID_PARAMETER         ,"WSA_INVALID_PARAMETER"    },
    { WSA_NOT_ENOUGH_MEMORY         ,"WSA_NOT_ENOUGH_MEMORY"    },
    { WSA_OPERATION_ABORTED         ,"WSA_OPERATION_ABORTED"    },

    { 0                             ,"WSA_INVALID_EVENT"        },
    { WSA_MAXIMUM_WAIT_EVENTS       ,"WSA_MAXIMUM_WAIT_EVENTS"  },
    { WSA_WAIT_FAILED               ,"WSA_WAIT_FAILED"          },
//    { WSA_WAIT_EVENT_0              ,"WSA_WAIT_EVENT_0"         },
    { WSA_WAIT_IO_COMPLETION        ,"WSA_WAIT_IO_COMPLETION"   },
    { WSA_WAIT_TIMEOUT              ,"WSA_WAIT_TIMEOUT"         },
    { WSA_INFINITE                  ,"WSA_INFINITE"             },
#else /* WIN16 */
    { WSA_IO_PENDING                ,"WSA_IO_PENDING"           },
    { WSA_IO_INCOMPLETE             ,"WSA_IO_INCOMPLETE"        },
    { WSA_INVALID_HANDLE            ,"WSA_INVALID_HANDLE"       },
    { WSA_INVALID_PARAMETER         ,"WSA_INVALID_PARAMETER"    },
    { WSA_NOT_ENOUGH_MEMORY         ,"WSA_NOT_ENOUGH_MEMORY"    },
    { WSA_OPERATION_ABORTED         ,"WSA_OPERATION_ABORTED"    },
                                    ,"
    { (DWORD) WSA_INVALID_EVENT     ,"WSA_INVALID_EVENT"        },
    { WSA_MAXIMUM_WAIT_EVENTS       ,"WSA_MAXIMUM_WAIT_EVENTS"  },
    { WSA_WAIT_FAILED               ,"WSA_WAIT_FAILED"          },
//    { WSA_WAIT_EVENT_0              ,"WSA_WAIT_EVENT_0"         },
    { WSA_WAIT_TIMEOUT              ,"WSA_WAIT_TIMEOUT"         },
    { WSA_INFINITE                  ,"WSA_INFINITE"             },

    { ERROR_INSUFFICIENT_BUFFER,    ,"ERROR_INSUFFICIENT_BUFFER" },

#endif
    { 0                             ,NULL                       }
};


char *aFuncNames[] =
{
    "accept",
    "bind",
    "closesocket",
    "connect",
    "gethostbyaddr",
    "gethostbyname",
    "gethostname",
    "getpeername",
    "getprotobyname",
    "getprotobynumber",
    "getservbyname",
    "getservbyport",
    "getsockname",
    "getsockopt",
    "htonl",
    "htons",
    "inet_addr",
    "inet_ntoa",
    "ioctlsocket",
    "listen",
    "ntohl",
    "ntohs",
    "recv",
    "recvfrom",
    "//select",
    "send",
    "sendto",
    "setsockopt",
    "shutdown",
    "socket",
    "WSAAccept",
    "WSAAddressToStringA",
    "WSAAddressToStringW",
    "WSAAsyncGetHostByAddr",
    "WSAAsyncGetHostByName",
    "WSAAsyncGetProtoByName",
    "WSAAsyncGetProtoByNumber",
    "WSAAsyncGetServByName",
    "WSAAsyncGetServByPort",
    "WSAAsyncSelect",
    "WSACancelAsyncRequest",
//    "WSACancelBlockingCall",
    "WSACleanup",
    "WSACloseEvent",
    "WSAConnect",
    "WSACreateEvent",
    "WSADuplicateSocketA",
    "WSADuplicateSocketW",
    "WSAEnumNameSpaceProvidersA",
    "WSAEnumNameSpaceProvidersW",
    "WSAEnumNetworkEvents",
    "WSAEnumProtocolsA",
    "WSAEnumProtocolsW",
    "WSAEventSelect",
    "WSAGetLastError",
    "WSAGetOverlappedResult",
    "WSAGetQOSByName",
    "WSAGetServiceClassInfoA",
    "WSAGetServiceClassInfoW",
    "WSAGetServiceClassNameByClassIdA",
    "WSAGetServiceClassNameByClassIdW",
    "WSAHtonl",
    "WSAHtons",
    "//WSAInstallServiceClassA",
    "//WSAInstallServiceClassW",
    "WSAIoctl",
//    "WSAIsBlocking",
    "WSAJoinLeaf",
    "//WSALookupServiceBeginA",
    "//WSALookupServiceBeginW",
    "WSALookupServiceEnd",
    "WSALookupServiceNextA",
    "WSALookupServiceNextW",
    "WSANtohl",
    "WSANtohs",
    "WSARecv",
    "WSARecvDisconnect",
    "WSARecvFrom",
    "WSARemoveServiceClass",
    "WSAResetEvent",
    "WSASend",
    "WSASendDisconnect",
    "WSASendTo",
//    "WSASetBlockingHook",
    "WSASetEvent",
    "WSASetLastError",
    "//WSASetServiceA",
    "//WSASetServiceW",
    "WSASocketA",
    "WSASocketW",
    "WSAStartup",
    "WSAStringToAddressA",
    "WSAStringToAddressW",
//    "WSAUnhookBlockingHook",
    "//WSAWaitForMultipleEvents",

    "WSCEnumProtocols",
    "WSCGetProviderPath",

    "EnumProtocolsA",
    "EnumProtocolsW",
    "GetAddressByNameA",
    "GetAddressByNameW",
    "GetNameByTypeA",
    "GetNameByTypeW",
    "GetServiceA",
    "GetServiceW",
    "GetTypeByNameA",
    "GetTypeByNameW",
    "SetServiceA",
    "SetServiceW",

//    "Close handle (comm, etc)",
//    "Dump buffer contents",
    NULL,
    "Default values",
    "WSAPROTOCOL_INFO",
    "QOS",
    NULL
};
