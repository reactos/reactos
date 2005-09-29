#ifndef __CLBDLL_H
#define __CLBDLL_H

#define CLBS_NOTIFY 0x1
#define CLBS_SORT   0x2
#define CLBS_DISABLENOSCROLL    0x1000
#define CLBS_VSCROLL    0x200000
#define CLBS_BORDER 0x800000
#define CLBS_POPOUT_HEADINGS    0x200
#define CLBS_SPRINGLY_COLUMNS   0x0

typedef struct _CLBS_INFO
{
    DWORD Style;
    DWORD Unknown; /* FIXME - ExStyle??? */
    LPCWSTR StyleName;
} CLBS_INFO, *LPCLBS_INFO;

typedef struct _CUSTOM_CONTROL_INFO
{
    WCHAR ClassName[32];
    DWORD Zero1; /* sizeof(DWORD) or sizeof(PVOID)? */
    WCHAR ClassName2[32];
    DWORD Unknown1; /* FIXME - size correct? */
    DWORD Unknown2; /* FIXME - size correct? */
    DWORD Unknown3; /* FIXME - size correct? */
    DWORD Zero2; /* FIXME - size correct? */
    DWORD Zero3; /* FIXME - size correct? */
    DWORD StylesCount;
    const CLBS_INFO *SupportedStyles;
    WCHAR Columns[256];
    INT_PTR (WINAPI *ClbStyleW)(IN HWND hWndParent,
                                IN LPARAM dwInitParam);
    DWORD Zero4; /* FIXME - size correct? */
    DWORD Zero5; /* FIXME - size correct? */
    DWORD Zero6; /* FIXME - size correct? */
} CUSTOM_CONTROL_INFO, *LPCUSTOM_CONTROL_INFO;

LRESULT CALLBACK ClbWndProc(HWND,UINT,WPARAM,LPARAM);
INT_PTR WINAPI ClbStyleW(HWND,LPARAM);
BOOL WINAPI CustomControlInfoW(LPCUSTOM_CONTROL_INFO);

#endif /* __CLBDLL_H */
