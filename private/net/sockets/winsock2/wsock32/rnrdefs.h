#ifndef __RNRDEFS_H__
#define __RNRDEFS_H__

//
// Bit defs for the protocols
//

#define UDP_BIT            1
#define TCP_BIT            2
#define ATM_BIT            4


typedef struct _DNS_RNR_CONTEXT
{
    LIST_ENTRY ListEntry;
    LONG      lSig;
    LONG      lInUse;
    LONG      lInstance;              // counter to tell if we've
                                      //  resolved this yet
    DWORD     fFlags;                 // always nice to have
    DWORD     dwControlFlags;
    DWORD     dwUdpPort;
    DWORD     dwTcpPort;
    DWORD     dwNameSpace;
    HANDLE    Handle;                 // the corresponding RnR handle
    DWORD     nProt;
    GUID      gdType;                // the type we are seeking
    GUID      gdProviderId;           // the provider being used
    DWORD     dwHostSize;
    struct hostent * phent;
    BOOL      fUnicodeHostent;
    DWORD     DnsRR;
    BLOB      blAnswer;
    WCHAR     wcName[1];             // the name
} DNS_RNR_CONTEXT, *PDNS_RNR_CONTEXT;

#define RNR_SIG 0xaabbccdd
      
#define DNS_F_END_CALLED  0x1             // generic  cancel

#define REVERSELOOK 0x2
#define LOCALLOOK 0x4
#define NEEDDOMAIN 0x8
#define IANALOOK  0x10
#define LOOPLOOK  0x20

#if 0
#define DNSGUID      {0x22059d40, \
                      0x7e9e,     \
                      0x11cf,     \
                      0xae,       \
                      0x5a,       \
                      0x00,       \
                      0xaa,       \
                      0x00,       \
                      0xa7,       \
                      0x11,       \
                      0x2b}
#else
#define DNSGUID {0}
#endif

#define NBTGUID  {0}
#define LCLGUID  {0}

//
// registry definitions
//

#define TCPKEY TEXT("System\\CurrentControlSet\\Services\\Tcpip\\Parameters")
#define LOOKUPORDER TEXT("DnsNbtLookupOrder")

#endif
