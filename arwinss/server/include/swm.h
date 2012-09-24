#ifndef SWM__H
#define SWM__H

typedef struct _SWM_WINDOW
{
    HWND hwnd;
    rectangle_t Window;
    struct region *Visible;
    BOOLEAN Hidden;
    BOOLEAN Topmost;

    LIST_ENTRY Entry;
} SWM_WINDOW, *PSWM_WINDOW;

extern SWM_WINDOW SwmRoot;

VOID NTAPI SwmInitialize();
VOID NTAPI SwmUpdateRootWindow(SURFOBJ *SurfObj);
PSWM_WINDOW NTAPI SwmFindByHwnd(HWND hWnd);
VOID NTAPI SwmAcquire(VOID);
VOID NTAPI SwmRelease(VOID);
HDC SwmGetScreenDC();
PSWM_WINDOW NTAPI SwmGetWindowById(SWM_WINDOW_ID Wid);

#endif
