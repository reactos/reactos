#pragma once

/* monitor object */
typedef struct _MONITOR
{
    HEAD head;
    struct _MONITOR* pMonitorNext;
    union
    {
        DWORD   dwMONFlags;
        struct
        {
            DWORD   IsVisible: 1;
            DWORD   IsPalette: 1;
            DWORD   IsPrimary: 1; /* wether this is the primary monitor */
        };
    };
    RECT    rcMonitor;
    RECT    rcWork;
    HRGN    hrgnMonitor;
    SHORT   cFullScreen;
    SHORT   cWndStack;
    HDEV    hDev;

    // ReactOS specific fields:
    UNICODE_STRING DeviceName;  /* name of the monitor */
    PDEVOBJ        *GdiDevice;  /* pointer to the GDI device to
                                   which this monitor is attached */
} MONITOR, *PMONITOR;

NTSTATUS IntAttachMonitor(PDEVOBJ *pGdiDevice, ULONG DisplayNumber);
NTSTATUS IntDetachMonitor(PDEVOBJ *pGdiDevice);
NTSTATUS IntUpdateMonitorSize(IN PDEVOBJ *pGdiDevice);
PMONITOR FASTCALL UserGetMonitorObject(IN HMONITOR);
PMONITOR FASTCALL IntGetPrimaryMonitor(VOID);
PMONITOR FASTCALL IntMonitorFromRect(PRECTL,DWORD);

/* EOF */
