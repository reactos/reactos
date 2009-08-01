#ifndef _WIN32K_MONITOR_H
#define _WIN32K_MONITOR_H

typedef struct _MONITOR_OBJECT
{
    HANDLE         Handle;     /* system object handle */
    FAST_MUTEX     Lock;       /* R/W lock */

    BOOL           IsPrimary;  /* wether this is the primary monitor */
    UNICODE_STRING DeviceName; /* name of the monitor */
    PDEVOBJ     *GdiDevice;    /* pointer to the GDI device to
                                  which this monitor is attached */
    struct _MONITOR_OBJECT *Prev, *Next; /* double linked list */

    RECT    rcMonitor;
    RECT    rcWork;
} MONITOR, *PMONITOR;

NTSTATUS
AttachMonitor(IN PDEVOBJ *pGdiDevice,
                 IN ULONG DisplayNumber);

NTSTATUS
DetachMonitor(IN PDEVOBJ *pGdiDevice);

#endif /* _WIN32K_MONITOR_H */
