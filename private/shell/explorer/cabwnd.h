#ifdef __cplusplus
extern "C" {        // start extern "C"
#endif

// in initcab.cpp
BOOL Cabinet_IsExplorerWindow(HWND hwnd);
BOOL Cabinet_IsFolderWindow(HWND hwnd);

//
// Moved from FCEXT.H because they are not public anymore.
//
// IShellView::MenuInit flags
#define MI_MAIN         0x0000
#define MI_UNKNOWN      0x0000
#define MI_SYSTEM       0x0001
#define MI_CONTEXT      0x0002
#define MI_FILE         0x0003
#define MI_EDIT         0x0004
#define MI_VIEW         0x0005
#define MI_TOOLS        0x0006
#define MI_HELP         0x0007
#define MI_POPUPMASK    0x000F
#define MI_POPUP        0x8000

#define MH_POPUP        0x0010
#define MH_TOOLBAR      0x0020

#ifdef __cplusplus
}                   // end extern "C"
#endif
