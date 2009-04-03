#ifndef _IFDEF_
#define _IFDEF_

#define IF_MAX_STRING_SIZE 256
#define IF_MAX_PHYS_ADDRESS_LENGTH 32

typedef union _NET_LUID_LH
{
    ULONG64 Value;
    struct
    {
        ULONG64 Reserved:24;
        ULONG64 NetLuidIndex:24;
        ULONG64 IfType:16;
    }Info;
} NET_LUID_LH, *PNET_LUID_LH;

typedef NET_LUID_LH NET_LUID;
typedef NET_LUID* PNET_LUID;

typedef ULONG NET_IFINDEX, *PNET_IFINDEX;

#endif
