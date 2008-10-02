/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Win32 Graphical Subsystem (WIN32K)
 * FILE:            include/win32k/ntusrtyp.h
 * PURPOSE:         Win32 Shared USER Types for NtUser*
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#ifndef _NTUSRTYP_
#define _NTUSRTYP_

/* ENUMERATIONS **************************************************************/

/* TYPES *********************************************************************/

typedef struct _PATRECT
{
    RECT r;
    HBRUSH hBrush;
} PATRECT, * PPATRECT;

/* Structures for reading icon/cursor files and resources */
#pragma pack(push,1)
typedef struct _ICONIMAGE
{
    BITMAPINFOHEADER icHeader;      // DIB header
    RGBQUAD icColors[1];            // Color table
    BYTE icXOR[1];                  // DIB bits for XOR mask
    BYTE icAND[1];                  // DIB bits for AND mask
} ICONIMAGE, *LPICONIMAGE;

typedef struct _CURSORIMAGE
{
    BITMAPINFOHEADER icHeader;       // DIB header
    RGBQUAD icColors[1];             // Color table
    BYTE icXOR[1];                   // DIB bits for XOR mask
    BYTE icAND[1];                   // DIB bits for AND mask
} CURSORIMAGE, *LPCURSORIMAGE;

typedef struct
{
    BYTE bWidth;
    BYTE bHeight;
    BYTE bColorCount;
    BYTE bReserved;
} ICONRESDIR;

typedef struct
{
    WORD wWidth;
    WORD wHeight;
} CURSORRESDIR;

typedef struct
{
    WORD wPlanes;                   // Number of Color Planes in the XOR image
    WORD wBitCount;                 // Bits per pixel in the XOR image
} ICONDIR;

typedef struct
{
    WORD wXHotspot;                 // Number of Color Planes in the XOR image
    WORD wYHotspot;                 // Bits per pixel in the XOR image
} CURSORDIR;

typedef struct
{
    BYTE bWidth;                    // Width, in pixels, of the icon image
    BYTE bHeight;                   // Height, in pixels, of the icon image
    BYTE bColorCount;               // Number of colors in image (0 if >=8bpp)
    BYTE bReserved;                 // Reserved ( must be 0)
    union
    {
        ICONDIR icon;
        CURSORDIR  cursor;
    } Info;
    DWORD dwBytesInRes;             // How many bytes in this resource?
    DWORD dwImageOffset;            // Where in the file is this image?
} CURSORICONDIRENTRY;

typedef struct
{
    WORD idReserved;                // Reserved (must be 0)
    WORD idType;                    // Resource Type (1 for icons, 0 for cursors)
    WORD idCount;                   // How many images?
    CURSORICONDIRENTRY idEntries[1];// An entry for idCount number of images
} CURSORICONDIR;

typedef struct
{
    union
    {
        ICONRESDIR icon;
        CURSORRESDIR cursor;
    } ResInfo;
    WORD wPlanes;                   // Color Planes
    WORD wBitCount;                 // Bits per pixel
    DWORD dwBytesInRes;             // how many bytes in this resource?
    WORD nID;                       // the ID
} GRPCURSORICONDIRENTRY;

typedef struct
{
    WORD idReserved;                    // Reserved (must be 0)
    WORD idType;                        // Resource type (1 for icons)
    WORD idCount;                       // How many images?
    GRPCURSORICONDIRENTRY idEntries[1]; // The entries for each image
} GRPCURSORICONDIR;
#pragma pack(pop)

typedef struct _THRDCARETINFO
{
    HWND hWnd;
    HBITMAP Bitmap;
    POINT Pos;
    SIZE Size;
    BYTE Visible;
    BYTE Showing;
} THRDCARETINFO, *PTHRDCARETINFO;




/* Cleanup TODO, just putting it in appropriate place for now */

typedef struct _PROPLISTITEM
{
  ATOM Atom;
  HANDLE Data;
} PROPLISTITEM, *PPROPLISTITEM;

typedef struct _PROPERTY
{
  LIST_ENTRY PropListEntry;
  HANDLE Data;
  ATOM Atom;
} PROPERTY, *PPROPERTY;

#define CTI_INSENDMESSAGE 0x0002

typedef struct _CLIENTTHREADINFO
{
    UINT  CTIF_flags;
    WORD  fsChangeBits;
    WORD  fsWakeBits;
    WORD  fsWakeBitsJournal;
    WORD  fsWakeMask;
    LONG  timeLastRead;
    DWORD dwcPumpHook;
} CLIENTTHREADINFO, *PCLIENTTHREADINFO;

#define SRVINFO_METRICS 0x0020

typedef struct _SERVERINFO
{
  DWORD    SRVINFO_Flags;
  DWORD    SystemMetrics[SM_CMETRICS];       // System Metrics
  COLORREF SysColors[COLOR_MENUBAR+1];       // GetSysColor
  HBRUSH   SysColorBrushes[COLOR_MENUBAR+1]; // GetSysColorBrush
  HPEN     SysColorPens[COLOR_MENUBAR+1];    // ReactOS exclusive
  DWORD    SrvEventActivity;
} SERVERINFO, *PSERVERINFO;

typedef struct _PFNCLIENT
{
   PROC pfnScrollBarC;
   PROC pfnDefWndC;
   PROC pfnMenuC;
   PROC pfnDesktopC;
   PROC pfnDefWnd1C;
   PROC pfnDefWnd2C;
   PROC pfnDefWnd3C;
   PROC pfnButtomC;
   PROC pfnComboBoxC;
   PROC pfnComboListBoxC;
   PROC pfnDefDlgC;
   PROC pfnEditC;
   PROC pfnListBoxC;
   PROC pfnMDIClientC;
   PROC pfnStaticC;
   PROC pfnImeC;
   PROC pfnHkINLPCWPSTRUCT;
   PROC pfnHkINLPCWPRETSTRUCT;
   PROC pfnDispatchHookC;
   PROC pfnDispatchDefC;
} PFNCLIENT, *PPFNCLIENT;

typedef struct _PFNCLIENTWORKER
{
   PROC pfnButtonCW;
   PROC pfnComboBoxCW;
   PROC pfnComboListBoxCW;
   PROC pfnDefDlgCW;
   PROC pfnEditCW;
   PROC pfnListBoxCW;
   PROC pfnMDIClientCW;
   PROC pfnStaticCW;
   PROC pfnImeCW;
} PFNCLIENTWORKER, *PPFNCLIENTWORKER;


struct  _ETHREAD;

#define CI_CURTHPRHOOK    0x00000010

typedef struct tagHOOK
{
  LIST_ENTRY     Chain;      /* Hook chain entry */
  HHOOK          Self;       /* user handle for this hook */
  struct _ETHREAD* Thread;   /* Thread owning the hook */
  int            HookId;     /* Hook table index */
  HOOKPROC       Proc;       /* Hook function */
  BOOLEAN        Ansi;       /* Is it an Ansi hook? */
  ULONG          Flags;      /* Some internal flags */
  UNICODE_STRING ModuleName; /* Module name for global hooks */
} HOOK, *PHOOK;

/* Window Client Information structure */

typedef struct _CALLBACKWND
{
     HWND hWnd;
     PVOID pvWnd;
} CALLBACKWND, *PCALLBACKWND;

typedef struct _W32CLIENTINFO
{
    ULONG CI_flags;
    ULONG cSpins;
    ULONG ulWindowsVersion;
    ULONG ulAppCompatFlags;
    ULONG ulAppCompatFlags2;
    DWORD dwTIFlags;
    PVOID pDeskInfo;
    ULONG_PTR ulClientDelta;
    PHOOK phkCurrent;
    ULONG fsHooks;
    HWND  hWND;  // Will be replaced with CALLBACKWND.
    PVOID pvWND; // " "
    ULONG Win32ClientInfo;
    DWORD dwHookCurrent;
    ULONG Win32ClientInfo1;
    PCLIENTTHREADINFO pClientThreadInfo;
    DWORD dwHookData;
    DWORD dwKeyCache;
    ULONG Win32ClientInfo2[7];
    USHORT CodePage;
    USHORT csCF;
    HANDLE hKL;
    ULONG Win32ClientInfo3[35];
} W32CLIENTINFO, *PW32CLIENTINFO;

/* Stuff below appears to have...dubious Windows compatibility */

/* lParam of DDE messages */
typedef struct tagKMDDEEXECUTEDATA
{
  HWND Sender;
  HGLOBAL ClientMem;
  /* BYTE Data[DataSize] */
} KMDDEEXECUTEDATA, *PKMDDEEXECUTEDATA;

typedef struct tagKMDDELPARAM
{
  BOOL Packed;
  union
    {
      struct
        {
          UINT uiLo;
          UINT uiHi;
        } Packed;
      LPARAM Unpacked;
    } Value;
} KMDDELPARAM, *PKMDDELPARAM;

typedef struct _REGISTER_SYSCLASS
{
    /* This is a reactos specific class used to initialize the
       system window classes during user32 initialization */
    UNICODE_STRING ClassName;
    UINT Style;
    WNDPROC ProcW;
    WNDPROC ProcA;
    UINT ExtraBytes;
    HICON hCursor;
    HBRUSH hBrush;
    UINT ClassId;
} REGISTER_SYSCLASS, *PREGISTER_SYSCLASS;

typedef struct _CLSMENUNAME
{
  LPSTR     pszClientAnsiMenuName;
  LPWSTR    pwszClientUnicodeMenuName;
  PUNICODE_STRING pusMenuName;
} CLSMENUNAME, *PCLSMENUNAME;

typedef struct _USER_OBJHDR
{
    /* This is the common header for all user handle objects */
    HANDLE Handle;
} USER_OBJHDR, PUSER_OBJHDR;

typedef struct _DESKTOP
{
    HANDLE hKernelHeap;
    ULONG_PTR HeapLimit;
    HWND hTaskManWindow;
    HWND hProgmanWindow;
    HWND hShellWindow;
    struct _WINDOW *Wnd;

    union
    {
        UINT Dummy;
        struct
        {
            UINT LastInputWasKbd : 1;
        };
    };

    WCHAR szDesktopName[1];
} DESKTOP, *PDESKTOP;

typedef struct _CALLPROC
{
    USER_OBJHDR hdr; /* FIXME: Move out of the structure once new handle manager is implemented */
    struct _W32PROCESSINFO *pi;
    WNDPROC WndProc;
    struct _CALLPROC *Next;
    UINT Unicode : 1;
} CALLPROC, *PCALLPROC;

typedef struct _WINDOWCLASS
{
    struct _WINDOWCLASS *Next;
    struct _WINDOWCLASS *Clone;
    struct _WINDOWCLASS *Base;
    PDESKTOP Desktop;
    RTL_ATOM Atom;
    ULONG Windows;

    UINT Style;
    WNDPROC WndProc;
    union
    {
        WNDPROC WndProcExtra;
        PCALLPROC CallProc;
    };
    PCALLPROC CallProcList;
    INT ClsExtra;
    INT WndExtra;
    PVOID Dce;
    DWORD fnID; // New ClassId
    HINSTANCE hInstance;
    HANDLE hIcon; /* FIXME - Use pointer! */
    HANDLE hIconSm; /* FIXME - Use pointer! */
    HANDLE hCursor; /* FIXME - Use pointer! */
    HBRUSH hbrBackground;
    HANDLE hMenu; /* FIXME - Use pointer! */
    PWSTR MenuName;
    PSTR AnsiMenuName;

    UINT Destroying : 1;
    UINT Unicode : 1;
    UINT System : 1;
    UINT Global : 1;
    UINT MenuNameIsString : 1;
    UINT NotUsed : 27;
} WINDOWCLASS, *PWINDOWCLASS;

typedef struct _WINDOW
{
    USER_OBJHDR hdr; /* FIXME: Move out of the structure once new handle manager is implemented */

    /* NOTE: This structure is located in the desktop heap and will
             eventually replace WINDOW_OBJECT. Right now WINDOW_OBJECT
             keeps a reference to this structure until all the information
             is moved to this structure */
    struct _W32PROCESSINFO *pi; /* FIXME: Move to object header some day */
    struct _W32THREADINFO *ti;
    RECT WindowRect;
    RECT ClientRect;

    WNDPROC WndProc;
    union
    {
        /* Pointer to a call procedure handle */
        PCALLPROC CallProc;
        /* Extra Wnd proc (windows of system classes) */
        WNDPROC WndProcExtra;
    };

    struct _WINDOW *Parent;
    struct _WINDOW *Owner;

    /* Size of the extra data associated with the window. */
    ULONG ExtraDataSize;
    /* Style. */
    DWORD Style;
    /* Extended style. */
    DWORD ExStyle;
    /* Handle of the module that created the window. */
    HINSTANCE Instance;
    /* Window menu handle or window id */
    UINT IDMenu;
    LONG UserData;
    /* Pointer to the window class. */
    PWINDOWCLASS Class;
    /* Window name. */
    UNICODE_STRING WindowName;
    /* Context help id */
    DWORD ContextHelpId;

    HWND hWndLastActive;
    /* Property list head.*/
    LIST_ENTRY PropListHead;
    ULONG PropListItems;

    struct
    {
        RECT NormalRect;
        POINT IconPos;
        POINT MaxPos;
    } InternalPos;

    UINT Unicode : 1;
    /* Indicates whether the window is derived from a system class */
    UINT IsSystem : 1;
    UINT InternalPosInitialized : 1;
    UINT HideFocus : 1;
    UINT HideAccel : 1;
} WINDOW, *PWINDOW;

typedef struct _W32PROCESSINFO
{
    PVOID UserHandleTable;
    HANDLE hUserHeap;
    ULONG_PTR UserHeapDelta;
    HINSTANCE hModUser;
    PWINDOWCLASS LocalClassList;
    PWINDOWCLASS GlobalClassList;
    PWINDOWCLASS SystemClassList;

    UINT RegisteredSysClasses : 1;

    PSERVERINFO psi;

} W32PROCESSINFO, *PW32PROCESSINFO;

typedef struct _W32THREADINFO
{
    PW32PROCESSINFO pi; /* [USER] */
    PW32PROCESSINFO kpi; /* [KERNEL] */
    PDESKTOP Desktop;
    PVOID DesktopHeapBase;
    ULONG_PTR DesktopHeapLimit;
    ULONG_PTR DesktopHeapDelta;
    /* A mask of what hooks are currently active */
    ULONG Hooks;
    /* Application compatibility flags */
    DWORD AppCompatFlags;
    DWORD AppCompatFlags2;
    CLIENTTHREADINFO ClientThreadInfo;
} W32THREADINFO, *PW32THREADINFO;






#endif
