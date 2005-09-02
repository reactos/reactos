#ifndef _WIN32K_WINDOW_H
#define _WIN32K_WINDOW_H

struct _PROPERTY;
struct _WINDOW_OBJECT;
typedef struct _WINDOW_OBJECT *PWINDOW_OBJECT;

#include <include/object.h>
#include <include/class.h>
#include <include/msgqueue.h>
#include <include/winsta.h>
#include <include/dce.h>
#include <include/prop.h>
#include <include/scroll.h>


VOID FASTCALL
WinPosSetupInternalPos(VOID);

typedef struct _INTERNALPOS
{
  RECT NormalRect;
  POINT IconPos;
  POINT MaxPos;
} INTERNALPOS, *PINTERNALPOS;

typedef struct _WINDOW_OBJECT
{
   union
   {
      USER_OBJECT_HDR hdr;
      struct
      {
      /*---------- USER_OBJECT_HDR --------------*/
        HWND hSelf; /* typesafe handle */
        BYTE hdrFlags_placeholder;
      /*---------- USER_OBJECT_HDR --------------*/

        /* Pointer to the window class. */
        PWNDCLASS_OBJECT Class;
        /* entry in the window list of the class object */
        LIST_ENTRY ClassListEntry;
        /* Extended style. */
        DWORD ExStyle;
        /* Window name. */
        UNICODE_STRING WindowName;
        /* Style. */
        DWORD Style;
        /* Context help id */
        DWORD ContextHelpId;
        /* system menu handle. */
        HMENU SystemMenu;
        /* Handle of the module that created the window. */
        HINSTANCE Instance;
        /* Entry in the thread's list of windows. */
        LIST_ENTRY ListEntry;
        /* Pointer to the extra data associated with the window. */
        PCHAR ExtraData;
        /* Size of the extra data associated with the window. */
        ULONG ExtraDataSize;
        /* Position of the window. */
        RECT WindowRect;
        /* Position of the window's client area. */
        RECT ClientRect;
        /* Window flags. */
        ULONG Flags;
        /* Window menu handle or window id */
        UINT IDMenu;
        /* Handle of region of the window to be updated. */
        HANDLE UpdateRegion;
        HANDLE NCUpdateRegion;
        /* Handle of the window region. */
        HANDLE WindowRegion;
        /* Lock to be held when manipulating (NC)UpdateRegion */
       // FAST_MUTEX UpdateLock;
        PWINDOW_OBJECT FirstChild;
        PWINDOW_OBJECT LastChild;
        PWINDOW_OBJECT NextSibling;
        PWINDOW_OBJECT PrevSibling;
        /* Entry in the list of thread windows. */
        LIST_ENTRY ThreadListEntry;
        /* Handle to the parent window. */
        PWINDOW_OBJECT ParentWnd;
        /* the owner window. */
        HWND hOwnerWnd;
        /* DC Entries (DCE) */
        PDCE Dce;

      //FIXME: add window desktop ptr.  

        /* Property list head.*/
        LIST_ENTRY PropListHead;
        ULONG PropListItems;
        /* Scrollbar info */
        PWINDOW_SCROLLINFO Scroll;
        LONG UserData;
        BOOL Unicode;
        WNDPROC WndProcA;
        WNDPROC WndProcW;
        /* owner queue/thread */
        union {
        PUSER_MESSAGE_QUEUE Queue;
        PW32THREAD WThread;
         };
        HWND hLastActiveWnd; /* handle to last active popup window (wine doesn't use pointer, for unk. reason)*/
        PINTERNALPOS InternalPos;
      //  ULONG Status;
        /* counter for tiled child windows */
        ULONG TiledCounter;
        /* WNDOBJ list */
        LIST_ENTRY WndObjListHead;
     };
  };
} WINDOW_OBJECT; /* PWINDOW_OBJECT already declared at top of file */

/* Window flags. */
#define WINDOWOBJECT_NEED_SIZE            (0x00000001)
#define WINDOWOBJECT_NEED_ERASEBKGND      (0x00000002)
#define WINDOWOBJECT_NEED_NCPAINT         (0x00000004)
#define WINDOWOBJECT_NEED_INTERNALPAINT   (0x00000008)
#define WINDOWOBJECT_RESTOREMAX           (0x00000020)




#endif /* _WIN32K_WINDOW_H */

/* EOF */
