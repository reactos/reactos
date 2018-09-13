
#ifndef _TRAYP_INC
#define _TRAYP_INC

#define TM_WINDOWDESTROYED	(WM_USER+0x100)
#define TM_POSTEDRCLICK 	(WM_USER+0x101)
#define TM_CONTEXTMENU          (WM_USER+0x102)
#define TM_ACTASTASKSW          (WM_USER+0x104)
#define TM_SYSMENUCOUNT         (WM_USER+0x105)
#define TM_TASKTAB              (WM_USER+0x106)

#ifdef ABM_NEW

typedef struct _TRAYAPPBARDATA
{
    APPBARDATA abd;
    DWORD dwMessage;
    LPRECT lprc;
} TRAYAPPBARDATA, *PTRAYAPPBARDATA;

#endif

#endif // _TRAYP_INC


