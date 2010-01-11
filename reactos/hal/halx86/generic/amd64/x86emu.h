
typedef union
{
    USHORT Short;
    ULONG Long;
    struct
    {
        ULONG Cf:1;
        ULONG Pf:1;
        ULONG Af:1;
        ULONG Zf:1;
        ULONG Sf:1;
        ULONG Tf:1;
        ULONG If:1;
        ULONG Df:1;
        ULONG Of:1;
        ULONG Iopl:3;
        ULONG Nt:1;
        ULONG Rf:1;
        ULONG Vm:1;
        ULONG Ac:1;
        ULONG Vif:1;
        ULONG Vip:1;
        ULONG Id:1;
    };
} EFLAGS;

typedef union
{
    ULONG Dword;
    USHORT Word;
    UCHAR Byte;
    struct
    {
        UCHAR Low;
        UCHAR High;
    };
} REGU;

typedef union
{
    struct 
    {
        UCHAR mod:2;
        UCHAR reg:3;
        UCHAR rm:3;
    };
    UCHAR Byte;
} MODRM;

typedef struct
{
    union
    {
        ULONG Eax;
        USHORT Ax;
        struct
        {
            UCHAR Al;
            UCHAR Ah;
        };
    };
    union
    {
        ULONG Ecx;
        USHORT Cx;
        struct
        {
            UCHAR Cl;
            UCHAR Ch;
        };
    };
    union
    {
        ULONG Edx;
        USHORT Dx;
        struct
        {
            UCHAR Dl;
            UCHAR Dh;
        };
    };
    union
    {
        ULONG Ebx;
        USHORT Bx;
        struct
        {
            UCHAR Bl;
            UCHAR Bh;
        };
    };
    union
    {
        ULONG Ebp;
        USHORT Bp;
    };
    union
    {
        ULONG Esi;
        USHORT Si;
    };
    union
    {
        ULONG Edi;
        USHORT Di;
    };
    union
    {
        struct
        {
            ULONG ReservedDsMBZ:4;
            ULONG SegDs:16;
        };
        ULONG ShiftedDs;
    };
    union
    {
        struct
        {
            ULONG ReservedEsMBZ:4;
            ULONG SegEs:16;
        };
        ULONG ShiftedEs;
    };

    /* Extended */
    union
    {
        struct
        {
            ULONG ReservedCsMBZ:4;
            ULONG SegCs:16;
        };
        ULONG ShiftedCs;
    };
    union
    {
        struct
        {
            ULONG ReservedSsMBZ:4;
            ULONG SegSs:16;
        };
        ULONG ShiftedSs;
    };
    
    union
    {
        struct
        {
            ULONG ReservedMsMBZ:4;
            ULONG SegMs:16;
        };
        ULONG ShiftedMs;
    };

    union
    {
        ULONG Eip;
        USHORT Ip;
    };

    union
    {
        ULONG Esp;
        USHORT Sp;
    };
    
    EFLAGS Eflags;

} X86_REGISTERS, *PX86_REGISTERS;

enum
{
    X86_VMFLAGS_RETURN_ON_IRET = 1,
};

typedef struct
{
    union
    {
        X86_BIOS_REGISTERS BiosRegisters;
        X86_REGISTERS Registers;
        REGU IndexedRegisters[8];
    };
    
    struct
    {
        ULONG ReturnOnIret:1;
    } Flags;
    
    PVOID MemBuffer;

#if 1
    PCHAR Mnemonic;
    PCHAR DstReg;
    PCHAR SrcReg;
    ULONG SrcEncodung;
    ULONG DstEncoding;
    ULONG Length;
#endif
} X86_VM_STATE, *PX86_VM_STATE;

enum
{
    PREFIX_SIZE_OVERRIDE    = 0x010001,
    PREFIX_ADDRESS_OVERRIDE = 0x020002,
    PREFIX_SEGMENT_CS       = 0x040004,
    PREFIX_SEGMENT_DS       = 0x040008,
    PREFIX_SEGMENT_ES       = 0x040010,
    PREFIX_SEGMNET_FS       = 0x040020,
    PREFIX_SEGMENT_GS       = 0x040040,
    PREFIX_SEGMENT_SS       = 0x040080,
    PREFIX_LOCK             = 0x080100,
    PREFIX_REP              = 0x100200,
} PREFIX_STATE;


VOID
NTAPI
x86Emulator(PX86_VM_STATE VmState);
