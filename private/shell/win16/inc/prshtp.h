typedef struct _PSP {
    PROPSHEETPAGE psp;
    // NOTE: the above member can be variable size so don't add any
    // structure members here
} PSP, FAR *HPROPSHEETPAGE;

typedef struct
{
    HWND hDlg;		// the dialog for this instance data
    PROPSHEETHEADER psh;

    HWND hwndCurPage;   // current page hwnd
    HWND hwndTabs;	// tab control window
    int nCurItem;	// index of current item in tab control
    int idDefaultFallback; // the default id to set as DEFID if page doesn't have one

    int nReturn;
    UINT nRestart;

    int xSubDlg, ySubDlg;	// dimensions of sub dialog
    int cxSubDlg, cySubDlg;

    BOOL fFlags;

} PROPDATA, FAR *LPPROPDATA;
// defines for fFlags
#define PD_NOERASE	 0x0001
#define PD_CANCELTOCLOSE 0x0002
#define PD_DESTROY       0x0004

#ifndef WIN32
//
// BUGBUG: It should be defined in WINDOWS.H!
//
typedef struct
{
        DWORD style;
        BYTE  cdit;
        WORD  x;
        WORD  y;
        WORD  cx;
        WORD  cy;
} DLGTEMPLATE, FAR *LPDLGTEMPLATE;
typedef const DLGTEMPLATE FAR *LPCDLGTEMPLATE;
#endif

