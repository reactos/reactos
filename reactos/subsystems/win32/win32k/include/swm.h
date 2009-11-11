#ifndef SWM__H
#define SWM__H

typedef struct _SWM_WINDOW
{
    HWND hwnd;
    rectangle_t Window;
    struct region *Visible;

    LIST_ENTRY Entry;
} SWM_WINDOW, *PSWM_WINDOW;


VOID NTAPI SwmInitialize();
PSWM_WINDOW NTAPI SwmFindByHwnd(HWND hWnd);
VOID NTAPI SwmAcquire(VOID);
VOID NTAPI SwmRelease(VOID);


#endif
