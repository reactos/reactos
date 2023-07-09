#define VDM_APP_MODE            0x00000001L
#define VDM_INTERRUPT_PENDING   0x00000002L
#define VDM_STATE_CHANGE        0x00000004L
#define VDM_VIRTUAL_INTERRUPTS  0x00000200L
#define VDM_PE_MASK             0x80000000L

typedef enum _VdmEventClass {
    VdmIO,
    VdmStringIO,
    VdmMemAccess,
    VdmIntAck,
    VdmBop,
    VdmError,
    VdmIrq13
} VDMEVENTCLASS, *PVDMEVENTCLASS;

typedef struct _VdmIoInfo {
    USHORT PortNumber;
    USHORT Size;
    BOOLEAN Read;
} VDMIOINFO, *PVDMIOINFO;

typedef struct _VdmStringIoInfo {
    USHORT PortNumber;
    USHORT Size;
    BOOLEAN Read;
    ULONG Count;
    ULONG Address;
} VDMSTRINGIOINFO, *PVDMSTRINGIOINFO;

typedef ULONG VDMBOPINFO;
typedef NTSTATUS VDMERRORINFO;

typedef struct _VdmEventInfo {
    ULONG Size;
    VDMEVENTCLASS Event;
    ULONG InstructionSize;
    union {
        VDMIOINFO IoInfo;
        VDMSTRINGIOINFO StringIoInfo;
        VDMBOPINFO BopNumber;
        VDMERRORINFO ErrorStatus;
    };
} VDMEVENTINFO, *PVDMEVENTINFO;

typedef struct _Vdm_InterruptHandler {
    USHORT  CsSelector;
    ULONG   Eip;
    USHORT  SsSelector;
    ULONG   Esp;
} VDM_INTERRUPTHANDLER, *PVDM_INTERRUPTHANDLER;

typedef struct _Vdm_Tib {
    ULONG Size;
    ULONG Flags;
    VDM_INTERRUPTHANDLER VdmInterruptHandlers[255];
    CONTEXT MonitorContext;
    CONTEXT VdmContext;
    VDMEVENTINFO EventInfo;
} VDM_TIB, *PVDM_TIB;

NTSTATUS
NtStartVdmExecution(
    );

// Flags that don't belong here

#define SEL_TYPE_READ       0x00000001
#define SEL_TYPE_WRITE      0x00000002
#define SEL_TYPE_EXECUTE    0x00000004
#define SEL_TYPE_BIG        0x00000008
#define SEL_TYPE_ED         0x00000010
#define SEL_TYPE_2GIG       0x00000020
