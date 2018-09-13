#ifndef _SHELLP_H_
#define _SHELLP_H_

//
// Define API decoration for direct importing of DLL references.
//
#ifndef WINSHELLAPI
#if !defined(_SHELL32_)
#define WINSHELLAPI DECLSPEC_IMPORT
#else
#define WINSHELLAPI
#endif
#endif // WINSHELLAPI

//
// shell private header
//

#ifndef NOPRAGMAS
#ifndef RC_INVOKED
#pragma pack(1)         /* Assume byte packing throughout */
#endif /* !RC_INVOKED */
#endif

#include <sfview.h>
#include <dvocx.h>
#include <sherror.h>

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

#define DECLAREWAITCURSOR  HCURSOR hcursor_wait_cursor_save
#define SetWaitCursor()   hcursor_wait_cursor_save = SetCursor(LoadCursor(NULL, IDC_WAIT))
#define ResetWaitCursor() SetCursor(hcursor_wait_cursor_save)

//
// Interface: IShellFolderTask
//
// Purpose: Initializes a task that does something by enumerating a shellfolder
//

#undef  INTERFACE
#define INTERFACE   IShellFolderTask

DECLARE_INTERFACE_(IOldShellFolderTask, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS)  PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    // *** IShellFolderTask methods ***
    STDMETHOD(InitTaskSFT)(THIS_ IShellFolder *psfParent, LPITEMIDLIST pidlFull,
                           DWORD dwFlags, DWORD dwTaskPriority) PURE;
};

// Flags for InitTaskSFT
#define ITSFT_RECURSE   0x00000001      // recurse into subfolders


//
// Interface: IStartMenuTask
//
// Purpose: Initializes a task that does something for the start menu
//

#undef  INTERFACE
#define INTERFACE   IStartMenuTask

DECLARE_INTERFACE_(IOldStartMenuTask, IOldShellFolderTask)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS)  PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    // *** IShellFolderTask methods ***
    STDMETHOD(InitTaskSFT)(THIS_ IShellFolder *psfParent, LPITEMIDLIST pidlFolder,
                           DWORD dwFlags, DWORD dwTaskPriority) PURE;

    // *** IStartMenuTask methods ***
    STDMETHOD(InitTaskSMT)(THIS_ IShellHotKey * photkey, int iThreadPriority) PURE;
};


#ifdef __cplusplus
}
#endif  /* __cplusplus */

#ifndef NOPRAGMAS
#ifndef RC_INVOKED
#pragma pack()
#endif  /* !RC_INVOKED */
#endif /* NOPRAGMAS */

#endif // _SHELLP_H_
