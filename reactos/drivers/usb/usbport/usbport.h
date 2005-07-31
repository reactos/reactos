/*
 * Declarations for undocumented usbport.sys calls
 *
 * Written by Filip Navara <xnavara@volny.cz>
 * Updates by Mark Tempel
 */

#ifndef _USBPORT_H
#define _USBPORT_H

#define USB_CONTROLLER_INTERFACE_TAG 0x001E1E10

/**** B E G I N   M S   I N T E R N A L   P R O T O C O L ****/ 
typedef DWORD (*POPEN_ENDPOINT)(
    DWORD Unknown1,
    DWORD Unknown2,
    DWORD Unknown3
);

typedef NTSTATUS (*PPOKE_ENDPOINT)(
    DWORD Unknown1,
    DWORD Unknown2,
    DWORD Unknown3
);

typedef DWORD (*PQUERY_ENDPOINT_REQUIREMENTS)(
    DWORD Unknown1,
    DWORD Unknown2,
    DWORD Unknown3
);

typedef VOID (*PCLOSE_ENDPOINT)(
    DWORD Unknown1,
    DWORD Unknown2
);



typedef struct {
    DWORD Unknown1; /* 2 (UHCI), 3 (EHCI) */
    DWORD Unknown2; /* 2C3 (UHCI), 95 (EHCI) */
    DWORD Unknown3; /* 2EE0 (UHCI), 61A80 (EHCI) */
    DWORD Unknown4; /* - */
    DWORD Unknown5; /* 164 (UHCI), 178 (EHCI) */
    DWORD Unknown6; /* 8C (UHCI), A0 (EHCI) */
    DWORD Unknown7; /* 1C (UHCI), 30 (EHCI) */ /* Offset: 118 */
    DWORD Unknown8; /* - */
    DWORD Unknown9; /* - */
    DWORD Unknown10; /* 2280 (UHCI), 2C800 (EHCI) */ /* Offset: 124 */
    POPEN_ENDPOINT OpenEndpoint;
    PPOKE_ENDPOINT PokeEndpoint;
    PQUERY_ENDPOINT_REQUIREMENTS QueryEndpointRequirements;
    PCLOSE_ENDPOINT CloseEndpoint;
    PVOID StartController;           /* P00010A1C (2) */ /* Offset: 138 */
    PVOID StopController;            /* P00010952 */ /* Offset: 13C */
    PVOID SuspendController;         /* P00011584 */ /* Offset: 140 */
    PVOID ResumeController;          /* P0001164C */ /* Offset: 144 */
    PVOID InterruptService;          /* P00013C72 */ /* Offset: 148 */
    PVOID InterruptDpc;              /* P00013D8E */ /* Offset: 14C */
    PVOID SubmitTransfer;            /* P00011010 */ /* Offset: 150 */
    PVOID IsochTransfer;             /* P000136E8 */ /* Offset: 154 */
    PVOID AbortTransfer;             /* P00011092 */ /* Offset: 158 */
    PVOID GetEndpointState;          /* P00010F48 */ /* Offset: 15C */
    PVOID SetEndpointState;          /* P00010EFA */ /* Offset: 160 */
    PVOID PollEndpoint;              /* P00010D32 */ /* Offset: 164 */
    PVOID CheckController;           /* P00011794 */ /* Offset: 168 */
    PVOID Get32BitFrameNumber;       /* P00010F86 */ /* Offset: 16C */
    PVOID InterruptNextSOF;          /* P00013F56 */ /* Offset: 170 */
    PVOID EnableInterrupts;          /* P00013ED0 */ /* Offset: 174 */
    PVOID DisableInterrupts;         /* P00013E18 */ /* Offset: 178 */
    PVOID PollController;            /* P00010FF2 */ /* Offset: 17C */
    PVOID SetEndpointDataToggle;     /* P000110E6 */ /* Offset: 180 */
    PVOID GetEndpointStatus;         /* P00010ECE */ /* Offset: 184 */
    PVOID SetEndpointStatus;         /* P00010E52 */ /* Offset: 188 */
    DWORD Unknown36; /* - */
    PVOID RHGetRootHubData;          /* P00011AC6 */  /* Offset: 190 */ 
    PVOID RHGetStatus;               /* P00011B1A */  /* Offset: 194 */ 
    PVOID RHGetPortStatus;           /* P00011BBA */  /* Offset: 198 */ 
    PVOID RHGetHubStatus;            /* P00011B28 */  /* Offset: 19C */ 
    PVOID RHSetFeaturePortReset;     /* P00011F84 */  /* Offset: 1A0 */ 
    PVOID RHSetFeaturePortPower;     /* P00011BB4 */  /* Offset: 1A4 */ 
    PVOID RHSetFeaturePortEnable;    /* P00011BA2 */  /* Offset: 1A8 */ 
    PVOID RHSetFeaturePortSuspend;   /* P00011FF8 */  /* Offset: 1AC */ 
    PVOID RHClearFeaturePortEnable;  /* P00011B90 */  /* Offset: 1B0 */ 
    PVOID RHClearFeaturePortPower;     /* P00011BB4 */  /* Offset: 1B4 */ 
    PVOID RHClearFeaturePortSuspend; /* P0001210E */  /* Offset: 1B8 */ 
    PVOID RHClearFeaturePortEnableChange; /* P00012236 */  /* Offset: 1BC */ 
    PVOID RHClearFeaturePortConnectChange; /* P000121DE */  /* Offset: 1C0 */ 
    PVOID RHClearFeaturePortResetChange; /* P00012284 */  /* Offset: 1C4 */ 
    PVOID RHClearFeaturePortSuspendChange; /* P0001229C */  /* Offset: 1C8 */ 
    PVOID RHClearFeaturePortOvercurrentChange; /* P000122B4 */  /* Offset: 1CC */ 
    PVOID RHDisableIrq;              /* P00013F52 */  /* Offset: 1D0 */ 
    PVOID RHDisableIrq2;             /* P00013F52 */  /* Offset: 1D4 */ 
    PVOID StartSendOnePacket;        /* P00011144 */  /* Offset: 1D8 */ 
    PVOID EndSendOnePacket;          /* P000119B6 */  /* Offset: 1DC */ 
    PVOID PassThru;                  /* P000110E0 */  /* Offset: 1E0 */ 
    DWORD Unknown58[17];             /* - */
    PVOID FlushInterrupts;           /* P00013EA0 */  /* Offset: 228 */
    /* ... */
} USB_CONTROLLER_INTERFACE, *PUSB_CONTROLLER_INTERFACE;
/**** E N D   M S   I N T E R N A L   P R O T O C O L ****/

/*
 * With this call USB miniport driver registers itself with usbport.sys
 *
 * Unknown1 - Could be 0x64 or 0xC8. (0x9A also ?)
 * Unknown2 - Pointer to structure which contains function entry points
 */
NTSTATUS STDCALL
USBPORT_RegisterUSBPortDriver(PDRIVER_OBJECT DriverObject, DWORD Unknown1,
    PUSB_CONTROLLER_INTERFACE Interface);

/*
 * This function always returns 0x10000001 in Windows XP SP1
 */
NTSTATUS STDCALL
USBPORT_GetHciMn(VOID);

/*
 * This method is provided for miniports to use to allocate their USB_CONTROLLER_INTERFACEs.
 */
NTSTATUS STDCALL
USBPORT_AllocateUsbControllerInterface(OUT PUSB_CONTROLLER_INTERFACE *pControllerInterface);

/*
 * We can't have an allocate without a free.
 */
NTSTATUS STDCALL
USBPORT_FreeUsbControllerInterface(IN PUSB_CONTROLLER_INTERFACE ControllerInterface);
#endif /* _USBPORT_H */
