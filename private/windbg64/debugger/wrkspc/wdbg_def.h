#ifndef __WDBG_DEF_H__
#define __WDBG_DEF_H__




//
// These structures need to be defined since 
//  we are storing this data in binary format.
//
// This file is also included by the 
//  workspace code and windbg.
//




//
//  Window state
//
typedef enum _WINDOW_STATE {
    WSTATE_ICONIC = SW_MINIMIZE,            //  Iconized
    WSTATE_NORMAL = SW_SHOWNORMAL,          //  Normal
    WSTATE_MAXIMIZED = SW_SHOWMAXIMIZED     //  Maximized
} WINDOW_STATE, *PWINDOW_STATE;


typedef struct paneflags {
    DWORD Expand1st :1;       // Vib contains no data
} PFLAGS;







#endif
