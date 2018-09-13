

typedef DWORDLONG ULONGLONG;


#define ADDRFLAT(paddr, x) {             \
    (paddr)->type = ADDR_FLAT;           \
    (paddr)->seg  = 0;                 \
    (paddr)->off  = (x);               \
    ComputeFlatAddress((paddr), NULL); \
}

#define ADDR64(paddr, x) {                       \
    (paddr)->type = ADDR_64;                     \
    (paddr)->seg  = (USHORT)(((x) >> 32) & 0x7); \
    (paddr)->off  = (ULONG)(x);                  \
    ComputeFlatAddress((paddr), NULL);           \
}

#define ADDR_NONE       ((USHORT)0x0000)
#define ADDR_UNKNOWN    ((USHORT)0x0001)
#define ADDR_V86        ((USHORT)0x0002)
#define ADDR_16         ((USHORT)0x0004)
#define ADDR_FLAT       ((USHORT)0x0008)
#define ADDR_1632       ((USHORT)0x0010)
#define FLAT_COMPUTED   ((USHORT)0x0020)
#define INSTR_POINTER   ((USHORT)0x0040)
#define NO_DEFAULT      0xFFFF
#define fnotFlat(x)     (!(((x).type)&FLAT_COMPUTED))
#define fFlat(x)        (((x).type)&FLAT_COMPUTED)
#define fInstrPtr(x)    (((x).type)&INSTR_POINTER)
#define AddrEqu(x,y)    (((x).flat == (y).flat))
#define AddrLt(x,y)     (((x).flat < (y).flat))
#define AddrGt(x,y)     (((x).flat > (y).flat))
#define AddrDiff(x,y)   (((x).flat - (y).flat))
#define Flat(x)         ((x).flat)
#define Off(x)          ((x).off)
#define Type(x)         ((x).type)
#define NotFlat(x)      (x).type&=~FLAT_COMPUTED


typedef struct _NTSDADDR {
    USHORT      type;
    USHORT      seg;
    ULONG64     off;
    ULONG64     flat;
} NTSDADDR, *PNTSDADDR;

void
ComputeFlatAddress(
    PNTSDADDR,
    PVOID
    );

ULONGLONG
GetRegFlagValue(
    int flag
    );

VOID
GetRegPCValue(
    PNTSDADDR addr
    );

int
GetRegString(
    PUCHAR regname,
    RD * prd
    );

BOOL
GetOffsetFromSym(
    PTCHAR  pString,
    PADDR   paddr
    );

PNTSDADDR
AddrAdd(
    PNTSDADDR,
    ULONG64
    );

PNTSDADDR
AddrSub(
    PNTSDADDR,
    ULONG64
    );

BOOL
GetMemByte(
    PNTSDADDR Address,
    PBYTE buf
    );

BOOL
GetMemWord(
    PNTSDADDR Address,
    PWORD buf
    );

BOOL
GetMemDword(
    PNTSDADDR Address,
    PULONG buf
    );

#ifndef _X86_

typedef struct _XLDT_ENTRY {
    WORD    LimitLow;
    WORD    BaseLow;
    union {
        struct {
            BYTE    BaseMid;
            BYTE    Flags1;     // Declare as bytes to avoid alignment
            BYTE    Flags2;     // Problems.
            BYTE    BaseHi;
        } Bytes;
        struct {
            DWORD   BaseMid : 8;
            DWORD   Type : 5;
            DWORD   Dpl : 2;
            DWORD   Pres : 1;
            DWORD   LimitHi : 4;
            DWORD   Sys : 1;
            DWORD   Reserved_0 : 1;
            DWORD   Default_Big : 1;
            DWORD   Granularity : 1;
            DWORD   BaseHi : 8;
        } Bits;
    } HighWord;
} XLDT_ENTRY, *XPLDT_ENTRY;

typedef struct _XDESCRIPTOR_TABLE_ENTRY {
    DWORD Selector;
    XLDT_ENTRY Descriptor;
} XDESCRIPTOR_TABLE_ENTRY, *PXDESCRIPTOR_TABLE_ENTRY;

#else

typedef DESCRIPTOR_TABLE_ENTRY XDESCRIPTOR_TABLE_ENTRY, *PXDESCRIPTOR_TABLE_ENTRY;
typedef LDT_ENTRY XLDT_ENTRY, *PXLDT_ENTRY;

#endif


BOOL
LookupSelector(
    LPTD lptd,
    PXDESCRIPTOR_TABLE_ENTRY pdesc
    );

void
NtsdAddrToAddr(
    PNTSDADDR NtsdAddr,
    PADDR Addr
    );

void
AddrToNtsdAddr(
    PADDR Addr,
    PNTSDADDR NtsdAddr
    );


///////////////////////////////////////////////////////////////////////////////


ULONG
GetExpression(
    PUCHAR szExpr,
    PULONG64 pResult,
    int radix,
    MPT mptProcessor,
    PUCHAR *lpNext
    );

ULONG
GetAddrExpression (
    PUCHAR szExpr,
    PNTSDADDR Address,
    int Radix,
    MPT mptProcessorType,
    ULONG defaultSeg,
    PUCHAR *lpNext
    );

BOOL
GetRange (
    PUCHAR szExpr,
    PNTSDADDR addr,
    PULONG64 value,
    ULONG size,
    BOOL * fLength,
    int radix,
    MPT mptProcessorType,
    ULONG defaultSeg,
    PUCHAR * lpNext,
    LPBOOL  lpbSecondParamIsALength
    );

