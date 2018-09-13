typedef struct _vdminternalinfo {
    DWORD           dwLdtBase;
    DWORD           dwLdtLimit;
    DWORD           dwIntelBase;
    DWORD           dwReserved;
    WORD            wKernelSeg;
    DWORD           dwOffsetTHHOOK;
    LPVOID          vdmContext;
    LPVOID          lpRemoteAddress;
    DWORD           lpRemoteBlock;
    BOOL            f386;
    LPVOID          lpNtvdmState;
    LPVOID          lpVdmDbgFlags;
    LPVOID          lpNtCpuInfo;
    LPVOID          lpVdmBreakPoints;
} VDMINTERNALINFO;
typedef VDMINTERNALINFO *LPVDMINTERNALINFO;


#define MAX_VDM_BREAKPOINTS 16
#define VDM_TEMPBP 0
typedef struct _VDM_BREAKPOINT {   /* VDMBP */
    BYTE  Flags;
    BYTE  Opcode;
    WORD  Count;
    WORD  Seg;
    DWORD Offset;
} VDM_BREAKPOINT;

//
// Bits defined in Flags
//
#define VDMBP_SET     0x01
#define VDMBP_ENABLED 0x02
#define VDMBP_FLUSH   0x04
#define VDMBP_PENDING 0x08
#define VDMBP_V86     0x10


typedef struct _com_header {
    DWORD           dwBlockAddress;
    DWORD           dwReturnValue;
    WORD            wArgsPassed;
    WORD            wArgsSize;
    WORD            wBlockLength;
    WORD            wSuccess;
} COM_HEADER;
typedef COM_HEADER FAR *LPCOM_HEADER;
