//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1993-1994
//
// File: brfcasep.h
//
//  Internal header shared between SHELL232 and SYNCUI
//
// History:
//  01-27-94 ScottH     Copied from brfcase.h
//
//---------------------------------------------------------------------------

#ifndef _BRFCASEP_H_
#define _BRFCASEP_H_

//===========================================================================
//
// IBriefcaseStg Interface
//
//  This is a private interface for use between the shell and the briefcase.
//
//  This interface is used by the Shell's IShellFolder implementation 
// when it is bound to a folder that is (in) a briefcase.  The IShellView
// of the particular folder binds to this interface to open the briefcase
// database storage and optionally make modifications.  File-objects that
// are added to a folder in a briefcase are not added to the storage 
// database until IBriefcaseStg::AddObject is called.  Keep in mind the
// difference between IBriefcaseStg and IShellFolder.  IBriefcaseStg 
// simply provides access to the briefcase storage database--the file-system
// objects are only affected upon subsequent reconciliation using 
// IBriefcaseStg::UpdateObject, unless otherwise noted.
// 
//
// [Member functions]
//
//
// IBriefcaseStg::Initialize(pszFolder, hwndOwner)
//
//   Initializes the interface by specifying the folder for this storage
//   instance.  If the folder does not exist somewhere in a briefcase
//   storage hierarchy, then a briefcase database is created for this 
//   folder.
//
// IBriefcaseStg::AddObject(pdtobj, pszFolderEx, uFlags, hwndOwner)
//
//   Adds a file-object(s) to the briefcase storage.  This function also
//   performs an update of the specific files to immediately make them
//   up-to-date.  
//
//   Typically pdtobj refers to a file-object(s) outside the
//   briefcase.  Calling this function implies adding the object(s) to 
//   the briefcase storage database in the folder that was specified by
//   IBriefcaseStg::Initialize().  This rule holds unless pszFolderEx is
//   non-NULL, in which case pdtobj is sync-associated to pszFolderEx 
//   instead.
//
//   AddObject() returns NOERROR if the object(s) were added.  S_FALSE
//   is returned if the caller should handle the action (eg, moving
//   files from folder-to-folder within the briefcase).
//
// IBriefcaseStg::ReleaseObject(pdtobj, hwndOwner)
//
//   Releases a file-object(s) from the briefcase storage database.  This 
//   does not delete the file from the file-system.
//
// IBriefcaseStg::UpdateObject(pdtobj, hwndOwner)
//
//   Performs a file-synchronization update to pdtobj.  If pdtobj refers to
//   the root of a briefcase storage hierarchy, the entire storage is updated.
//
// IBriefcaseStg::UpdateOnEvent(uEvent, hwndOwner)
//
//   Performs a complete update of the briefcase storage based on the 
//   indicated event.  The event ordinals may be:
//
//      UOE_CONFIGCHANGED       Indicates a PnP DBT_CONFIGCHANGED message wants
//                              to be processed.  This occurs when a machine
//                              hot-docks.
//
//      UOE_QUERYCHANGECONFIG   Indicates a PnP DBT_QUERYCHANGECONFIG message
//                              wants to be processed.  This occurs when a
//                              machine is about to hot-undock.
//
// IBriefcaseStg::GetExtraInfo(pszName, uInfo, wParam, lParam)
//
//   Gets some specified extra info from the briefcase storage.  The
//   info is determined by uInfo, which is one of GEI_* values.
//
// IBriefcaseStg::Notify(pszPath, lEvent, puFlags, hwndOwner)
//
//   Sends a notify event to the briefcase storage, so it can mark
//   cached items stale.  If lEvent is NOE_RENAME, pszPath must be a double
//   null-terminated string, where the first is the old pathname, and the
//   second is the new pathname.  *puFlags is filled with flags pertaining
//   to what the member function did.  NF_REDRAWWINDOW means the window
//   needs to be redrawn.  NF_ITEMMARKED means the cached item in the
//   briefcase storage associated with pszPath was marked stale.
//
// IBriefcaseStg::GetRootOf(pszBuffer, cbBuffer)
//
//   Queries the briefcase storage for the root of the briefcase storage
//   hierarchy.
//
// IBriefcaseStg::FindFirst(pszBuffer, cbBuffer)
//
//   Finds the root of the first briefcase storage on the system.  The 
//   buffer is filled with the fully qualified pathname.  This function
//   returns S_OK if a briefcase was found.  S_FALSE is returned to end
//   enumeration.
//
// IBriefcaseStg::FindNext(pszBuffer, cbBuffer)
//
//   Finds the root of the next briefcase storage on the system.  The 
//   buffer is filled with the fully qualified pathname.  This function 
//   returns S_OK if a briefcase was found.  S_FALSE is returned to end
//   enumeration.
//   
//   
//
//===========================================================================

#undef  INTERFACE
#define INTERFACE   IBriefcaseStg

DECLARE_INTERFACE_(IBriefcaseStg, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IBriefcaseStg methods ***
    STDMETHOD(Initialize) (THIS_ LPCSTR pszFolder, HWND hwnd) PURE;
    STDMETHOD(AddObject) (THIS_ LPDATAOBJECT lpdobj, LPCSTR pszFolderEx, UINT uFlags, HWND hwnd) PURE;
    STDMETHOD(ReleaseObject) (THIS_ LPDATAOBJECT lpdobj, HWND hwnd) PURE;
    STDMETHOD(UpdateObject) (THIS_ LPDATAOBJECT lpdobj, HWND hwnd) PURE;
    STDMETHOD(UpdateOnEvent) (THIS_ UINT uEvent, HWND hwnd) PURE;
    STDMETHOD(GetExtraInfo) (THIS_ LPCSTR pszName, UINT uInfo, WPARAM wParam, LPARAM lParam) PURE;
    STDMETHOD(Notify) (THIS_ LPCSTR pszPath, LONG lEvent, UINT * puFlags, HWND hwndOwner) PURE;
    STDMETHOD(FindFirst) (THIS_ LPSTR pszPath, int cchMax) PURE;
    STDMETHOD(FindNext) (THIS_ LPSTR pszPath, int cchMax) PURE;
};

// Events for UpdateOnEvent member function
#define UOE_CONFIGCHANGED       1
#define UOE_QUERYCHANGECONFIG   2

// Flags for AddObject
#define AOF_DEFAULT             0x0000
#define AOF_UPDATEONREPLACE     0x0001
#define AOF_FILTERPROMPT        0x0002

// Notify events
#define NOE_RENAME              1L
#define NOE_RENAMEFOLDER        2L
#define NOE_CREATE              3L
#define NOE_CREATEFOLDER        4L
#define NOE_DELETE              5L
#define NOE_DELETEFOLDER        6L
#define NOE_DIRTY               7L
#define NOE_DIRTYFOLDER         8L
#define NOE_DIRTYALL            9L

// Flags that are returned by Notify member function
#define NF_REDRAWWINDOW     0x0001
#define NF_ITEMMARKED       0x0002

// Flags for GetExtraInfo                wParam        lParam
#define GEI_ROOT            1       //   cchBuf         pszBuf
#define GEI_ORIGIN          2       //   cchBuf         pszBuf
#define GEI_STATUS          3       //   cchBuf         pszBuf
#define GEI_DELAYHANDLE     4       //     --           phandle
#define GEI_DATABASENAME    5       //   cchBuf         pszBuf

typedef IBriefcaseStg FAR*   LPBRIEFCASESTG;

// Special briefcase object struct
//
typedef struct _BriefObj
    {
    UINT    cbSize;             // size of allocated struct
    UINT    ibFileList;         // offset of file list in struct
    UINT    ibBriefPath;        // offset of briefcase path in struct
    UINT    cItems;             // count of file-system objects
    UINT    cbListSize;         // size of file list
    char    data[1];            // data
    } BriefObj, * PBRIEFOBJ;

// Helper macros for briefcase object struct
#define BOBriefcasePath(pbo)    ((LPSTR)(pbo) + (pbo)->ibBriefPath)
#define BOFileList(pbo)         ((LPSTR)(pbo) + (pbo)->ibFileList)
#define BOFileCount(pbo)        ((pbo)->cItems)
#define BOFileListSize(pbo)     ((pbo)->cbListSize)

// Clipboard format for above struct
//
#define CFSTR_BRIEFOBJECT  "Briefcase File Object"

#endif // _BRFCASEP_H_


