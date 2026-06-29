/*
 * PROJECT:     ReactOS PSDK
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     UDPMIB Header
 * COPYRIGHT:   Copyright 2025 Carl Bialorucki <carl.bialorucki@reactos.org>
 */

#ifndef _UDPMIB_
#define _UDPMIB_

#define TCPIP_OWNING_MODULE_SIZE 16

typedef struct _MIB_UDPROW {
    DWORD dwLocalAddr;
    DWORD dwLocalPort;
} MIB_UDPROW, *PMIB_UDPROW;

typedef struct _MIB_UDPTABLE {
    DWORD      dwNumEntries;
    MIB_UDPROW table[1];
} MIB_UDPTABLE, *PMIB_UDPTABLE;

#define SIZEOF_UDPTABLE(x) \
    (FIELD_OFFSET(MIB_UDPTABLE, table[0]) + ((x) * sizeof(MIB_UDPROW)) + ALIGN_SIZE)

typedef struct _MIB_UDPROW_OWNER_PID {
    DWORD dwLocalAddr;
    DWORD dwLocalPort;
    DWORD dwOwningPid;
} MIB_UDPROW_OWNER_PID, *PMIB_UDPROW_OWNER_PID;

typedef struct _MIB_UDPTABLE_OWNER_PID
{
    DWORD                dwNumEntries;
    MIB_UDPROW_OWNER_PID table[1];
} MIB_UDPTABLE_OWNER_PID, *PMIB_UDPTABLE_OWNER_PID;

#define SIZEOF_UDPTABLE_OWNER_PID(x) \
    (FIELD_OFFSET(MIB_UDPTABLE_OWNER_PID, table[0]) + ((x) * sizeof(MIB_UDPROW_OWNER_PID)) + ALIGN_SIZE)

typedef struct _MIB_UDPROW_OWNER_MODULE {
    DWORD         dwLocalAddr;
    DWORD         dwLocalPort;
    DWORD         dwOwningPid;
    LARGE_INTEGER liCreateTimestamp;
    union {
        struct {
            int SpecificPortBind : 1;
        };
        int dwFlags;
    };
    ULONGLONG     OwningModuleInfo[TCPIP_OWNING_MODULE_SIZE];
} MIB_UDPROW_OWNER_MODULE, *PMIB_UDPROW_OWNER_MODULE;

typedef struct _MIB_UDPTABLE_OWNER_MODULE {
    DWORD                   dwNumEntries;
    MIB_UDPROW_OWNER_MODULE table[1];
} MIB_UDPTABLE_OWNER_MODULE, *PMIB_UDPTABLE_OWNER_MODULE;

#define SIZEOF_UDPTABLE_OWNER_MODULE(x) \
    (FIELD_OFFSET(MIB_UDPTABLE_OWNER_MODULE, table[0]) + ((x) * sizeof(MIB_UDPROW_OWNER_MODULE)) + ALIGN_SIZE)

typedef struct _MIB_UDPROW2 {
    DWORD         dwLocalAddr;
    DWORD         dwLocalPort;
    DWORD         dwOwningPid;
    LARGE_INTEGER liCreateTimestamp;
    union {
        struct {
            int SpecificPortBind : 1;
        };
        int dwFlags;
    };
    ULONGLONG     OwningModuleInfo[TCPIP_OWNING_MODULE_SIZE];
    DWORD         dwRemoteAddr;
    DWORD         dwRemotePort;
} MIB_UDPROW2, *PMIB_UDPROW2;

typedef struct _MIB_UDPTABLE2 {
    DWORD       dwNumEntries;
    MIB_UDPROW2 table[1];
} MIB_UDPTABLE2, *PMIB_UDPTABLE2;

#define SIZEOF_UDPTABLE2(x) \
    (FIELD_OFFSET(MIB_UDPTABLE2, table[0]) + ((x) * sizeof(MIB_UDPROW2)) + ALIGN_SIZE)

typedef struct _MIB_UDPSTATS {
    DWORD dwInDatagrams;
    DWORD dwNoPorts;
    DWORD dwInErrors;
    DWORD dwOutDatagrams;
    DWORD dwNumAddrs;
} MIB_UDPSTATS,*PMIB_UDPSTATS;

#if (NTDDI_VERSION >= NTDDI_WIN10_RS3)
typedef struct _MIB_UDPSTATS2 {
    DWORD64 dw64InDatagrams;
    DWORD   dwNoPorts;
    DWORD   dwInErrors;
    DWORD64 dw64OutDatagrams;
    DWORD   dwNumAddrs;
} MIB_UDPSTATS2, *PMIB_UDPSTATS2;
#endif // (NTDDI_VERSION >= NTDDI_WIN10_RS3)

#ifdef _WS2IPDEF_
typedef struct _MIB_UDP6ROW {
    IN6_ADDR dwLocalAddr;
    DWORD    dwLocalScopeId;
    DWORD    dwLocalPort;
} MIB_UDP6ROW, *PMIB_UDP6ROW;

typedef struct _MIB_UDP6TABLE {
    DWORD       dwNumEntries;
    MIB_UDP6ROW table[1];
} MIB_UDP6TABLE, *PMIB_UDP6TABLE;

#define SIZEOF_UDP6TABLE(X) (FIELD_OFFSET(MIB_UDP6TABLE, table[0]) + \
                             ((X) * sizeof(MIB_UDP6ROW)) + ALIGN_SIZE)

typedef struct _MIB_UDP6ROW_OWNER_PID {
    UCHAR ucLocalAddr[16];
    DWORD dwLocalScopeId;
    DWORD dwLocalPort;
    DWORD dwOwningPid;
} MIB_UDP6ROW_OWNER_PID, *PMIB_UDP6ROW_OWNER_PID;

typedef struct _MIB_UDP6TABLE_OWNER_PID {
    DWORD                   dwNumEntries;
    _Field_size_(dwNumEntries)
    MIB_UDP6ROW_OWNER_PID   table[1];
} MIB_UDP6TABLE_OWNER_PID, *PMIB_UDP6TABLE_OWNER_PID;

#define SIZEOF_UDP6TABLE_OWNER_PID(x) \
    (FIELD_OFFSET(MIB_UDP6TABLE_OWNER_PID, table[0]) + ((x) * sizeof(MIB_UDP6ROW_OWNER_PID)) + ALIGN_SIZE)

typedef struct _MIB_UDP6ROW_OWNER_MODULE {
    UCHAR         ucLocalAddr[16];
    DWORD         dwLocalScopeId;
    DWORD         dwLocalPort;
    DWORD         dwOwningPid;
    LARGE_INTEGER liCreateTimestamp;
    union {
        struct {
            int SpecificPortBind : 1;
        };
        int dwFlags;
    };
    ULONGLONG     OwningModuleInfo[TCPIP_OWNING_MODULE_SIZE];
} MIB_UDP6ROW_OWNER_MODULE, *PMIB_UDP6ROW_OWNER_MODULE;

typedef struct _MIB_UDP6TABLE_OWNER_MODULE {
    DWORD                    dwNumEntries;
    MIB_UDP6ROW_OWNER_MODULE table[1];
} MIB_UDP6TABLE_OWNER_MODULE, *PMIB_UDP6TABLE_OWNER_MODULE;

#define SIZEOF_UDP6TABLE_OWNER_MODULE(x) \
    (FIELD_OFFSET(MIB_UDP6TABLE_OWNER_MODULE, table[0]) + ((x) * sizeof(MIB_UDP6ROW_OWNER_MODULE)) + ALIGN_SIZE)

typedef struct _MIB_UDP6ROW2 {
    UCHAR         ucLocalAddr[16];
    DWORD         dwLocalScopeId;
    DWORD         dwLocalPort;
    DWORD         dwOwningPid;
    LARGE_INTEGER liCreateTimestamp;
    union {
        struct {
            int SpecificPortBind : 1;
        };
        int dwFlags;
    };
    ULONGLONG     OwningModuleInfo[TCPIP_OWNING_MODULE_SIZE];
    UCHAR         ucRemoteAddr[16];
    DWORD         dwRemoteScopeId;
    DWORD         dwRemotePort;
} MIB_UDP6ROW2, *PMIB_UDP6ROW2;

typedef struct _MIB_UDP6TABLE2 {
    DWORD        dwNumEntries;
    MIB_UDP6ROW2 table[1];
} MIB_UDP6TABLE2, *PMIB_UDP6TABLE2;

#define SIZEOF_UDP6TABLE2(x) \
    (FIELD_OFFSET(MIB_UDP6TABLE2, table[0]) + ((x) * sizeof(MIB_UDP6ROW2)) + ALIGN_SIZE)
#endif // _WS2IPDEF_

#endif // _UDPMIB_
