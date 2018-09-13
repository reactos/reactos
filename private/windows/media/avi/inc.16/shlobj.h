//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1991-1994
//
// File: shlobj.h
//
//  Definitions of IShellUI interface. Eventually this file should be merged
// into COMMUI.H.
//
// History:
//  12-30-92 SatoNa     Created.
//  01-06-93 SatoNa     Added this comment block.
//  01-13-93 SatoNa     Added DragFilesOver & DropFiles
//  01-27-93 SatoNa     Created by combining shellui.h and handler.h
//  01-28-93 SatoNa     OLE 2.0 beta 2
//  03-12-93 SatoNa     Removed IFileDropTarget (we use IDropTarget)
//
//---------------------------------------------------------------------------

#ifndef _SHLOBJ_H_
#define _SHLOBJ_H_

#include <ole2.h>
#include <prsht.h>
#include <shell2.h>

#ifndef INITGUID
#include <shlguid.h>
#endif

typedef void const FAR*       LPCVOID;

//----------------------------------------------------------------------------
//
// Shell Extension API
//
//----------------------------------------------------------------------------

//
// Task allocator
//
//  All the shell extensions MUST use this allocator when they allocate
// or free memory objects that are passed across any shell interface
// boundary.
//
// REVIEW:
//  It would be really nice if we can guarantee that shell's task
// allocator and OLE's task allocator is always the same. it is,
// however, not so easy to do, because:
//
//  1. We don't want to load COMPOBJ unless a shell extension DLL
//    loads it. We need to be notified when COMPOBJ is loaded.
//  2. We need to register our task allocator to the COMPOBJ
//    if one of shell extension DLLs loads it into the shell
//    process.
//  3. We need to get the task allocator from the COMPOBJ, if
//    the shell dll is loaded by non-shell process that registers
//    the task allocator to the COMPOBJ.
//

LPVOID WINAPI SHAlloc(ULONG cb);
LPVOID WINAPI SHRealloc(LPVOID pv, ULONG cbNew);
ULONG  WINAPI SHGetSize(LPVOID pv);
void   WINAPI SHFree(LPVOID pv);

//
// Helper macro definitions
//
#define S_BOOL(f)   MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_NULL, f)


//----------------------------------------------------------------------------
//
// Interface:   IContextMenu
//
// History:
//  02-24-93 SatoNa     Created.
//
//----------------------------------------------------------------------------

#undef  INTERFACE
#define INTERFACE   IContextMenu

#define CMF_DEFAULTONLY  0x00000001

DECLARE_INTERFACE_(IContextMenu, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    STDMETHOD(QueryContextMenu)(THIS_
                                HMENU hmenu,
                                UINT indexMenu,
                                UINT idCmdFirst,
                                UINT idCmdLast,
                                UINT uFlags) PURE;

    STDMETHOD(InvokeCommand)(THIS_
                             HWND   hwndParent,
                             LPCSTR pszWorkingDir,
                             LPCSTR pszCmd,
                             LPCSTR pszParam,
                             int    iShowCmd) PURE;

    STDMETHOD(GetCommandString)(THIS_
                                UINT        idCmd,
                                UINT        wReserved,
                                UINT FAR *  pwReserved,
                                LPSTR       pszName,
                                UINT        cchMax) PURE;
};

typedef IContextMenu FAR*	LPCONTEXTMENU;

// GetIconLocation() input flags

#define GIL_OPENICON     0x0001      // allows containers to specify an "open" look
                                     // return FALSE to get the standard look

// GetIconLocation() return flags

#define GIL_SIMULATEDOC  0x0001      // simulate this document icon for this
#define GIL_PERINSTANCE  0x0002      // icons from this class are per instance (each file has its own)
#define GIL_PERCLASS     0x0004      // icons from this class per class (shared for all files of this type)

#include <fcext.h>      // Browser extension interfaces are defined in FCEXT.H


//==========================================================================
// Helper macro for C programmers

//#ifdef WIN32
#define LPTONP(p)       (p)
//#else
//#define LPTONP(p)       OFFSETOF(p)
//#endif // WIN32

#define _IOffset(class, itf)         ((UINT)&(((class *)0)->itf))
#define IToClass(class, itf, pitf)   ((class FAR *)(((LPSTR)pitf)-_IOffset(class, itf)))
#define IToClassN(class, itf, pitf)  ((class *)LPTONP(((LPSTR)pitf)-_IOffset(class, itf)))

//===========================================================================

HRESULT STDAPICALLTYPE Link_CreateInstance(LPUNKNOWN punkOuter, REFIID riid, LPVOID FAR* ppvOut);
//
// Helper functions for component object DLLs
//
//===========================================================================

typedef HRESULT (CALLBACK FAR * LPFNCREATEINSTANCE)(
                                                  LPUNKNOWN pUnkOuter,
                                                  REFIID riid,
                                                  LPVOID FAR* ppvObject);

STDAPI Shell_CreateDefClassObject(REFIID riid, LPVOID FAR* ppv,
			 LPFNCREATEINSTANCE lpfn, UINT FAR * pcRefDll,
			 REFIID riidInstance);

//===========================================================================
//
// Interface: IShellExtInit
//
//  This interface is used to initialize shell extension objects.
//
//===========================================================================
#undef  INTERFACE
#define INTERFACE   IShellExtInit

DECLARE_INTERFACE_(IShellExtInit, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IShellExtInit methods ***
    STDMETHOD(Initialize)(THIS_ LPCITEMIDLIST pidlFolder,
		          LPDATAOBJECT lpdobj, HKEY hkeyProgID) PURE;
};
									
typedef IShellExtInit FAR*	LPSHELLEXTINIT;

#undef  INTERFACE
#define INTERFACE   IShellPropSheetExt

DECLARE_INTERFACE_(IShellPropSheetExt, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IShellPropSheetExt methods ***
    STDMETHOD(AddPages)(THIS_ LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam) PURE;
};

typedef IShellPropSheetExt FAR* LPSHELLPROPSHEETEXT;


//===========================================================================
//
// IPersistFolder Interface
//
//  This interface is used by the Folder implementation of
// IMoniker::BindToObject when it is initializing a folder object.
//
//===========================================================================

#undef  INTERFACE
#define INTERFACE   IPersistFolder

DECLARE_INTERFACE_(IPersistFolder, IPersist)	// fld
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IPersist methods ***
    STDMETHOD(GetClassID) (THIS_ LPCLSID lpClassID) PURE;

    // *** IPersistFolder methods ***
    STDMETHOD(Initialize) (THIS_
			   LPCITEMIDLIST pidl) PURE;
};

typedef IPersistFolder FAR*	LPPERSISTFOLDER;


//
// IExtractIcon interface
//
#undef  INTERFACE
#define INTERFACE   IExtractIcon

DECLARE_INTERFACE_(IExtractIcon, IUnknown)	// exic
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IExtractIcon methods ***
    STDMETHOD(GetIconLocation)(THIS_
                         UINT   uFlags,
                         LPSTR  szIconFile,
                         UINT   cchMax,
                         int  FAR * piIndex,
                         UINT FAR * pwFlags) PURE;

    STDMETHOD(ExtractIcon)(THIS_
                           LPCSTR pszFile,
			   UINT	  nIconIndex,
			   HICON  FAR *phiconLarge,
			   HICON  FAR *phiconSmall,
			   UINT   nIcons) PURE;
};

typedef IExtractIcon FAR*	LPEXTRACTICON;



//===========================================================================
// Network resource array handle
//===========================================================================
typedef HANDLE HNRES;
typedef struct _NETRESOURCE FAR *LPNETRESOURCE;
UINT WINAPI SHGetNetResource(HNRES hnres, UINT iItem, LPNETRESOURCE pnres, UINT cbMax);


//
// IShellLink Interface
//

#undef  INTERFACE
#define INTERFACE   IShellLink

DECLARE_INTERFACE_(IShellLink, IUnknown)	// sl
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    STDMETHOD(IsLinkToFile)(THIS) PURE;

    STDMETHOD(GetSubject)(THIS_ LPSTR pszFile, int cchMaxPath, WIN32_FIND_DATA *pfd, UINT fFlags) PURE;
    STDMETHOD(SetSubject)(THIS_ LPCSTR pszFile, const WIN32_FIND_DATA *pfd) PURE;

    STDMETHOD(GetWorkingDirectory)(THIS_ LPSTR pszDir, int cchMaxPath) PURE;
    STDMETHOD(SetWorkingDirectory)(THIS_ LPCSTR pszDir) PURE;

    STDMETHOD(GetArguments)(THIS_ LPSTR pszArgs, int cchMaxPath) PURE;
    STDMETHOD(SetArguments)(THIS_ LPCSTR pszArgs) PURE;

    STDMETHOD(GetHotkey)(THIS_ WORD *pwHotkey) PURE;
    STDMETHOD(SetHotkey)(THIS_ WORD wHotkey) PURE;

    STDMETHOD(GetShowCmd)(THIS_ int *piShowCmd) PURE;
    STDMETHOD(SetShowCmd)(THIS_ int iShowCmd) PURE;

    STDMETHOD(GetIconLocation)(THIS_ LPSTR pszIconPath, int cchIconPath, int *piIcon) PURE;
    STDMETHOD(SetIconLocation)(THIS_ LPCSTR pszIconPath, int iIcon) PURE;

    STDMETHOD(Resolve)(THIS_ HWND hwnd, UINT fFlags) PURE;

    STDMETHOD(Update)(THIS_ UINT fFlags) PURE;
};



#endif // _SHELLUI_H_
