#pragma once

/* monitor object */
typedef struct _MONITOR
{
    HEAD head;
//
    FAST_MUTEX     Lock;       /* R/W lock */
    UNICODE_STRING DeviceName; /* name of the monitor */
    PDEVOBJ     *GdiDevice;    /* pointer to the GDI device to
	                          which this monitor is attached */
// This is the structure Windows uses:
//    struct _MONITOR* pMonitorNext;
    union  {        
    DWORD   dwMONFlags;
    struct {
    DWORD   IsVisible:1;
    DWORD   IsPalette:1;
    DWORD   IsPrimary:1;  /* wether this is the primary monitor */
    };};
    RECT    rcMonitor;
    RECT    rcWork;
    HRGN    hrgnMonitor;
    SHORT   Spare0;
    SHORT   cWndStack;
    HDEV    hDev;
    HDEV    hDevReal;
//    BYTE    DockTargets[4][7];
// Use LIST_ENTRY
    struct _MONITOR* Next; //Flink;
    struct _MONITOR* Prev; //Blink;
} MONITOR, *PMONITOR;

/* functions */
NTSTATUS InitMonitorImpl();
NTSTATUS CleanupMonitorImpl();

NTSTATUS IntAttachMonitor(PDEVOBJ *pGdiDevice, ULONG DisplayNumber);
NTSTATUS IntDetachMonitor(PDEVOBJ *pGdiDevice);
PMONITOR FASTCALL UserGetMonitorObject(IN HMONITOR);
PMONITOR FASTCALL IntGetPrimaryMonitor(VOID);

/* EOF */
