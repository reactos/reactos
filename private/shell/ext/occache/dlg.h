#ifndef DLG_H
#define DLG_H

#include <windows.h>
#include <windowsx.h>
#include <debug.h>

typedef void (*PFN)();

typedef union tagMMF {
    PFN pfn;

    LRESULT (*pfn_lwwwl)(HWND, UINT, WPARAM, LPARAM);
    BOOL    (*pfn_bwwwl)(HWND, UINT, WPARAM, LPARAM);
    void    (*pfn_vv)();
    BOOL    (*pfn_bv)();
    void    (*pfn_vw)(WPARAM);
    BOOL    (*pfn_bw)(WPARAM);
    void    (*pfn_vh)(HANDLE);
    BOOL    (*pfn_bh)(HANDLE);
    BOOL    (*pfn_bhl)(HANDLE, LPARAM);
    void    (*pfn_vhww)(HANDLE, UINT, WORD);
    void    (*pfn_vhhw)(HANDLE, HANDLE, WORD);
} MMF;

typedef enum tagMSIG {
    ms_end = 0,

    ms_lwwwl,   // LRESULT (HWND, UINT, WORD, LPARAM)
    ms_bwwwl,   // BOOL    (HWND, UINT, WORD, LPARAM)
    ms_vv,      // void    (void)
    ms_bv,      // BOOL    (void)
    ms_vw,      // void    (WPARAM)
    ms_bw,      // BOOL    (WPARAM)
    ms_vh,      // void    (HANDLE)
    ms_bh,      // BOOL    (HANDLE)
    ms_bhl,     // BOOL    (HANDLE, LPARAM)
    ms_vhww,    // void    (HANDLE, UINT,   WORD)
    ms_vhhw,    // void    (HANDLE, HANDLE, WORD)
} MSIG;

typedef struct tagMSD {
    UINT msg;
    MSIG ms;
    PFN  pfn;
} MSD;
typedef MSD *PMSD;

typedef struct tagCMD {
    UINT nID, nLastID;
    MSIG ms;
    PFN  pfn;
} CMD;
typedef CMD *PCMD;

INT_PTR Dlg_MsgProc(const MSD *pmsd, HWND hDlg, UINT msg, WPARAM wp, LPARAM lp);
BOOL Msg_OnCmd(const CMD *pcmd, HWND hDlg, UINT msg, WPARAM wp, LPARAM lp);
#endif
