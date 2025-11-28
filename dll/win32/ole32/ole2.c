/*
 *	OLE2 library
 *
 * Copyright 1995 Martin von Loewis
 * Copyright 1999 Francis Beaudet
 * Copyright 1999 Noel Borthwick
 * Copyright 1999, 2000 Marcus Meissner
 * Copyright 1999-2000 Abey George
 * Copyright 2005 Juan Lang
 * Copyright 2011 Adam Martinson for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "winreg.h"
#include "ole2.h"
#include "ole2ver.h"

#include "compobj_private.h"
#include "olestd.h"
#include "wine/list.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);
WINE_DECLARE_DEBUG_CHANNEL(accel);

/******************************************************************************
 * These are static/global variables and internal data structures that the
 * OLE module uses to maintain its state.
 */
typedef struct tagTrackerWindowInfo
{
  IDataObject* dataObject;
  IDropSource* dropSource;
  DWORD        dwOKEffect;
  DWORD*       pdwEffect;
  BOOL       trackingDone;
  BOOL         inTrackCall;
  HRESULT      returnValue;

  BOOL       escPressed;
  HWND       curTargetHWND;	/* window the mouse is hovering over */
  IDropTarget* curDragTarget;
  POINTL     curMousePos;       /* current position of the mouse in screen coordinates */
  DWORD      dwKeyState;        /* current state of the shift and ctrl keys and the mouse buttons */
} TrackerWindowInfo;

typedef struct tagOleMenuDescriptor  /* OleMenuDescriptor */
{
  HWND               hwndFrame;         /* The containers frame window */
  HWND               hwndActiveObject;  /* The active objects window */
  OLEMENUGROUPWIDTHS mgw;               /* OLE menu group widths for the shared menu */
  HMENU              hmenuCombined;     /* The combined menu */
  BOOL               bIsServerItem;     /* True if the currently open popup belongs to the server */
} OleMenuDescriptor;

typedef struct tagOleMenuHookItem   /* OleMenu hook item in per thread hook list */
{
  DWORD tid;                /* Thread Id  */
  HANDLE hHeap;             /* Heap this is allocated from */
  HHOOK GetMsg_hHook;       /* message hook for WH_GETMESSAGE */
  HHOOK CallWndProc_hHook;  /* message hook for WH_CALLWNDPROC */
  struct tagOleMenuHookItem *next;
} OleMenuHookItem;

static OleMenuHookItem *hook_list;

/*
 * This is the lock count on the OLE library. It is controlled by the
 * OLEInitialize/OLEUninitialize methods.
 */
static LONG OLE_moduleLockCount = 0;

/*
 * Name of our registered window class.
 */
static const WCHAR OLEDD_DRAGTRACKERCLASS[] = L"WineDragDropTracker32";

/*
 * Name of menu descriptor property.
 */
static const WCHAR prop_olemenuW[] = L"PROP_OLEMenuDescriptor";

/* property to store IDropTarget pointer */
static const WCHAR prop_oledroptarget[] = L"OleDropTargetInterface";

/* property to store Marshalled IDropTarget pointer */
static const WCHAR prop_marshalleddroptarget[] = L"WineMarshalledDropTarget";

/******************************************************************************
 * These are the prototypes of miscellaneous utility methods
 */
static void OLEUTL_ReadRegistryDWORDValue(HKEY regKey, DWORD* pdwValue);

/******************************************************************************
 * These are the prototypes of the utility methods used to manage a shared menu
 */
static void OLEMenu_Initialize(void);
static void OLEMenu_UnInitialize(void);
static BOOL OLEMenu_InstallHooks( DWORD tid );
static BOOL OLEMenu_UnInstallHooks( DWORD tid );
static OleMenuHookItem * OLEMenu_IsHookInstalled( DWORD tid );
static BOOL OLEMenu_FindMainMenuIndex( HMENU hMainMenu, HMENU hPopupMenu, UINT *pnPos );
static BOOL OLEMenu_SetIsServerMenu( HMENU hmenu, OleMenuDescriptor *pOleMenuDescriptor );
static LRESULT CALLBACK OLEMenu_CallWndProc(INT code, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK OLEMenu_GetMsgProc(INT code, WPARAM wParam, LPARAM lParam);

/******************************************************************************
 * This is a prototype of the OLE Clipboard uninitialization method (in clipboard.c)
 */
extern void clipbrd_uninitialize(void);

/******************************************************************************
 * These are the prototypes of the utility methods used for OLE Drag n Drop
 */
static void OLEDD_Initialize(void);
static LRESULT WINAPI  OLEDD_DragTrackerWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static void OLEDD_TrackStateChange(TrackerWindowInfo* trackerInfo);
static DWORD OLEDD_GetButtonState(void);

/******************************************************************************
 *		OleBuildVersion [OLE32.@]
 */
DWORD WINAPI OleBuildVersion(void)
{
    TRACE("Returning version %d, build %d.\n", rmm, rup);
    return (rmm<<16)+rup;
}

/***********************************************************************
 *           OleInitialize       (OLE32.@)
 */
HRESULT WINAPI DECLSPEC_HOTPATCH OleInitialize(LPVOID reserved)
{
  HRESULT hr;

  TRACE("(%p)\n", reserved);

  /*
   * The first duty of the OleInitialize is to initialize the COM libraries.
   */
  hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

  /*
   * If the CoInitializeEx call failed, the OLE libraries can't be
   * initialized.
   */
  if (FAILED(hr))
    return hr;

  if (!COM_CurrentInfo()->ole_inits)
    hr = S_OK;
  else
    hr = S_FALSE;

  /*
   * Then, it has to initialize the OLE specific modules.
   * This includes:
   *     Clipboard
   *     Drag and Drop
   *     Object linking and Embedding
   *     In-place activation
   */
  if (!COM_CurrentInfo()->ole_inits++ &&
      InterlockedIncrement(&OLE_moduleLockCount) == 1)
  {
    /*
     * Initialize the libraries.
     */
    TRACE("() - Initializing the OLE libraries\n");

    /*
     * Drag and Drop
     */
    OLEDD_Initialize();

    /*
     * OLE shared menu
     */
    OLEMenu_Initialize();
  }

  return hr;
}

/******************************************************************************
 *		OleUninitialize	[OLE32.@]
 */
void WINAPI DECLSPEC_HOTPATCH OleUninitialize(void)
{
  TRACE("()\n");

  if (COM_CurrentInfo()->ole_inits == 0)
  {
    WARN("ole_inits is already 0\n");
    return ;
  }
  /*
   * If we hit the bottom of the lock stack, free the libraries.
   */
  if (!--COM_CurrentInfo()->ole_inits && !InterlockedDecrement(&OLE_moduleLockCount))
  {
    /*
     * Actually free the libraries.
     */
    TRACE("() - Freeing the last reference count\n");

    /*
     * OLE Clipboard
     */
    clipbrd_uninitialize();

    /*
     * OLE shared menu
     */
    OLEMenu_UnInitialize();
  }

  /*
   * Then, uninitialize the COM libraries.
   */
  CoUninitialize();
}

/******************************************************************************
 *		OleInitializeWOW	[OLE32.@]
 */
HRESULT WINAPI OleInitializeWOW(DWORD x, DWORD y)
{
    FIXME("%#lx, %#lx stub!\n", x, y);
    return 0;
}

/*************************************************************
 *           get_droptarget_handle
 *
 * Retrieve a handle to the map containing the marshalled IDropTarget.
 * This handle belongs to the process that called RegisterDragDrop.
 * See get_droptarget_local_handle().
 */
static inline HANDLE get_droptarget_handle(HWND hwnd)
{
    return GetPropW(hwnd, prop_marshalleddroptarget);
}

/*************************************************************
 *           is_droptarget
 *
 * Is the window a droptarget.
 */
static inline BOOL is_droptarget(HWND hwnd)
{
    return get_droptarget_handle(hwnd) != 0;
}

/*************************************************************
 *           get_droptarget_local_handle
 *
 * Retrieve a handle to the map containing the marshalled IDropTarget.
 * The handle should be closed when finished with.
 */
static HANDLE get_droptarget_local_handle(HWND hwnd)
{
    HANDLE handle, local_handle = 0;

    handle = get_droptarget_handle(hwnd);

    if(handle)
    {
        DWORD pid;
        HANDLE process;

        GetWindowThreadProcessId(hwnd, &pid);
        process = OpenProcess(PROCESS_DUP_HANDLE, FALSE, pid);
        if(process)
        {
            DuplicateHandle(process, handle, GetCurrentProcess(), &local_handle, 0, FALSE, DUPLICATE_SAME_ACCESS);
            CloseHandle(process);
        }
    }
    return local_handle;
}

/***********************************************************************
 *     create_map_from_stream
 *
 * Helper for RegisterDragDrop.  Creates a file mapping object
 * with the contents of the provided stream.  The stream must
 * be a global memory backed stream.
 */
static HRESULT create_map_from_stream(IStream *stream, HANDLE *map)
{
    HGLOBAL hmem;
    DWORD size;
    HRESULT hr;
    void *data;

    hr = GetHGlobalFromStream(stream, &hmem);
    if(FAILED(hr)) return hr;

    size = GlobalSize(hmem);
    *map = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, size, NULL);
    if(!*map) return E_OUTOFMEMORY;

    data = MapViewOfFile(*map, FILE_MAP_WRITE, 0, 0, size);
    memcpy(data, GlobalLock(hmem), size);
    GlobalUnlock(hmem);
    UnmapViewOfFile(data);
    return S_OK;
}

/***********************************************************************
 *     create_stream_from_map
 *
 * Creates a stream from the provided map.
 */
static HRESULT create_stream_from_map(HANDLE map, IStream **stream)
{
    HRESULT hr = E_OUTOFMEMORY;
    HGLOBAL hmem;
    void *data;
    MEMORY_BASIC_INFORMATION info;

    data = MapViewOfFile(map, FILE_MAP_READ, 0, 0, 0);
    if(!data) return hr;

    VirtualQuery(data, &info, sizeof(info));
    TRACE("size %d\n", (int)info.RegionSize);

    hmem = GlobalAlloc(GMEM_MOVEABLE, info.RegionSize);
    if(hmem)
    {
        memcpy(GlobalLock(hmem), data, info.RegionSize);
        GlobalUnlock(hmem);
        hr = CreateStreamOnHGlobal(hmem, TRUE, stream);
    }
    UnmapViewOfFile(data);
    return hr;
}

/* This is to work around apps which break COM rules by not implementing
 * IDropTarget::QueryInterface().  Windows doesn't expose this because it
 * doesn't call CoMarshallInterface() in RegisterDragDrop().
 * The wrapper is only used internally, and only exists for the life of
 * the marshal.  We don't want to hold a ref on the app provided target
 * as some apps destroy this prior to CoUninitialize without calling
 * RevokeDragDrop.  The only (long-term) ref is held by the window prop. */
typedef struct {
    IDropTarget IDropTarget_iface;
    HWND hwnd;
    LONG refs;
} DropTargetWrapper;

static inline DropTargetWrapper* impl_from_IDropTarget(IDropTarget* iface)
{
    return CONTAINING_RECORD(iface, DropTargetWrapper, IDropTarget_iface);
}

static HRESULT WINAPI DropTargetWrapper_QueryInterface(IDropTarget* iface,
                                                       REFIID riid,
                                                       void** ppvObject)
{
    DropTargetWrapper* This = impl_from_IDropTarget(iface);
    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IDropTarget))
    {
        IDropTarget_AddRef(&This->IDropTarget_iface);
        *ppvObject = &This->IDropTarget_iface;
        return S_OK;
    }
    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI DropTargetWrapper_AddRef(IDropTarget* iface)
{
    DropTargetWrapper* This = impl_from_IDropTarget(iface);
    return InterlockedIncrement(&This->refs);
}

static ULONG WINAPI DropTargetWrapper_Release(IDropTarget* iface)
{
    DropTargetWrapper* This = impl_from_IDropTarget(iface);
    ULONG refs = InterlockedDecrement(&This->refs);
    if (!refs) HeapFree(GetProcessHeap(), 0, This);
    return refs;
}

static inline HRESULT get_target_from_wrapper( IDropTarget *wrapper, IDropTarget **target )
{
    DropTargetWrapper* This = impl_from_IDropTarget( wrapper );
    *target = GetPropW( This->hwnd, prop_oledroptarget );
    if (!*target) return DRAGDROP_E_NOTREGISTERED;
    IDropTarget_AddRef( *target );
    return S_OK;
}

static HRESULT WINAPI DropTargetWrapper_DragEnter(IDropTarget* iface,
                                                  IDataObject* pDataObj,
                                                  DWORD grfKeyState,
                                                  POINTL pt,
                                                  DWORD* pdwEffect)
{
    IDropTarget *target;
    HRESULT r = get_target_from_wrapper( iface, &target );

    if (SUCCEEDED( r ))
    {
        r = IDropTarget_DragEnter( target, pDataObj, grfKeyState, pt, pdwEffect );
        IDropTarget_Release( target );
    }
    return r;
}

static HRESULT WINAPI DropTargetWrapper_DragOver(IDropTarget* iface,
                                                 DWORD grfKeyState,
                                                 POINTL pt,
                                                 DWORD* pdwEffect)
{
    IDropTarget *target;
    HRESULT r = get_target_from_wrapper( iface, &target );

    if (SUCCEEDED( r ))
    {
        r = IDropTarget_DragOver( target, grfKeyState, pt, pdwEffect );
        IDropTarget_Release( target );
    }
    return r;
}

static HRESULT WINAPI DropTargetWrapper_DragLeave(IDropTarget* iface)
{
    IDropTarget *target;
    HRESULT r = get_target_from_wrapper( iface, &target );

    if (SUCCEEDED( r ))
    {
        r = IDropTarget_DragLeave( target );
        IDropTarget_Release( target );
    }
    return r;
}

static HRESULT WINAPI DropTargetWrapper_Drop(IDropTarget* iface,
                                             IDataObject* pDataObj,
                                             DWORD grfKeyState,
                                             POINTL pt,
                                             DWORD* pdwEffect)
{
    IDropTarget *target;
    HRESULT r = get_target_from_wrapper( iface, &target );

    if (SUCCEEDED( r ))
    {
        r = IDropTarget_Drop( target, pDataObj, grfKeyState, pt, pdwEffect );
        IDropTarget_Release( target );
    }
    return r;
}

static const IDropTargetVtbl DropTargetWrapperVTbl =
{
    DropTargetWrapper_QueryInterface,
    DropTargetWrapper_AddRef,
    DropTargetWrapper_Release,
    DropTargetWrapper_DragEnter,
    DropTargetWrapper_DragOver,
    DropTargetWrapper_DragLeave,
    DropTargetWrapper_Drop
};

static IDropTarget* WrapDropTarget( HWND hwnd )
{
    DropTargetWrapper* This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));

    if (This)
    {
        This->IDropTarget_iface.lpVtbl = &DropTargetWrapperVTbl;
        This->hwnd = hwnd;
        This->refs = 1;
    }
    return &This->IDropTarget_iface;
}

/***********************************************************************
 *     get_droptarget_pointer
 *
 * Retrieves the marshalled IDropTarget from the window.
 */
static IDropTarget* get_droptarget_pointer(HWND hwnd)
{
    IDropTarget *droptarget = NULL;
    HANDLE map;
    IStream *stream;

    map = get_droptarget_local_handle(hwnd);
    if(!map) return NULL;

    if(SUCCEEDED(create_stream_from_map(map, &stream)))
    {
        CoUnmarshalInterface(stream, &IID_IDropTarget, (void**)&droptarget);
        IStream_Release(stream);
    }
    CloseHandle(map);
    return droptarget;
}

/***********************************************************************
 *           RegisterDragDrop (OLE32.@)
 */
HRESULT WINAPI RegisterDragDrop(HWND hwnd, LPDROPTARGET pDropTarget)
{
  DWORD pid = 0;
  HRESULT hr;
  IStream *stream;
  HANDLE map;
  IDropTarget *wrapper;

  TRACE("(%p,%p)\n", hwnd, pDropTarget);

  if (!COM_CurrentApt())
  {
    ERR("COM not initialized\n");
    return E_OUTOFMEMORY;
  }

  if (!pDropTarget)
    return E_INVALIDARG;

  if (!IsWindow(hwnd))
  {
    ERR("invalid hwnd %p\n", hwnd);
    return DRAGDROP_E_INVALIDHWND;
  }

  /* block register for other processes windows */
  GetWindowThreadProcessId(hwnd, &pid);
  if (pid != GetCurrentProcessId())
  {
    FIXME("register for another process windows is disabled\n");
    return DRAGDROP_E_INVALIDHWND;
  }

  /* check if the window is already registered */
  if (is_droptarget(hwnd))
    return DRAGDROP_E_ALREADYREGISTERED;

  /*
   * Marshal the drop target pointer into a shared memory map and
   * store the map's handle in a Wine specific window prop.  We also
   * store the drop target pointer itself in the
   * "OleDropTargetInterface" prop for compatibility with Windows.
   */

  hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
  if(FAILED(hr)) return hr;

  /* IDropTarget::QueryInterface() shouldn't be called, some (broken) apps depend on this. */
  wrapper = WrapDropTarget( hwnd );
  if(!wrapper)
  {
    IStream_Release(stream);
    return E_OUTOFMEMORY;
  }
  hr = CoMarshalInterface(stream, &IID_IDropTarget, (IUnknown*)wrapper, MSHCTX_LOCAL, NULL, MSHLFLAGS_TABLESTRONG);
  IDropTarget_Release(wrapper);

  if(SUCCEEDED(hr))
  {
    hr = create_map_from_stream(stream, &map);
    if(SUCCEEDED(hr))
    {
      IDropTarget_AddRef(pDropTarget);
      SetPropW(hwnd, prop_oledroptarget, pDropTarget);
      SetPropW(hwnd, prop_marshalleddroptarget, map);
    }
    else
    {
      LARGE_INTEGER zero;
      zero.QuadPart = 0;
      IStream_Seek(stream, zero, STREAM_SEEK_SET, NULL);
      CoReleaseMarshalData(stream);
    }
  }
  IStream_Release(stream);

  return hr;
}

/***********************************************************************
 *           RevokeDragDrop (OLE32.@)
 */
HRESULT WINAPI RevokeDragDrop(HWND hwnd)
{
  HANDLE map;
  IStream *stream;
  IDropTarget *drop_target;
  HRESULT hr;

  TRACE("(%p)\n", hwnd);

  if (!IsWindow(hwnd))
  {
    ERR("invalid hwnd %p\n", hwnd);
    return DRAGDROP_E_INVALIDHWND;
  }

  /* no registration data */
  if (!(map = get_droptarget_handle(hwnd)))
    return DRAGDROP_E_NOTREGISTERED;

  drop_target = GetPropW(hwnd, prop_oledroptarget);
  if(drop_target) IDropTarget_Release(drop_target);

  RemovePropW(hwnd, prop_oledroptarget);
  RemovePropW(hwnd, prop_marshalleddroptarget);

  hr = create_stream_from_map(map, &stream);
  if(SUCCEEDED(hr))
  {
      CoReleaseMarshalData(stream);
      IStream_Release(stream);
  }
  CloseHandle(map);

  return hr;
}

/***********************************************************************
 *           OleRegGetUserType (OLE32.@)
 */
HRESULT WINAPI OleRegGetUserType(REFCLSID clsid, DWORD form, LPOLESTR *usertype)
{
  DWORD valuetype, valuelen;
  WCHAR auxkeynameW[16];
  HKEY    usertypekey;
  HRESULT hres;
  LONG    ret;

  TRACE("%s, %lu, %p.\n", debugstr_guid(clsid), form, usertype);

  if (!usertype)
    return E_INVALIDARG;

  *usertype = NULL;

  /* Return immediately if it's not registered. */
  hres = COM_OpenKeyForCLSID(clsid, NULL, KEY_READ, &usertypekey);
  if (FAILED(hres))
    return hres;

  valuelen = 0;

  /* Try additional types if requested. If they don't exist fall back to USERCLASSTYPE_FULL. */
  if (form != USERCLASSTYPE_FULL)
  {
    HKEY auxkey;

    swprintf(auxkeynameW, ARRAY_SIZE(auxkeynameW), L"AuxUserType\\%d", form);
    if (COM_OpenKeyForCLSID(clsid, auxkeynameW, KEY_READ, &auxkey) == S_OK)
    {
      if (!RegQueryValueExW(auxkey, L"", NULL, &valuetype, NULL, &valuelen) && valuelen)
      {
        RegCloseKey(usertypekey);
        usertypekey = auxkey;
      }
      else
        RegCloseKey(auxkey);
    }
  }

  valuelen = 0;
  if (RegQueryValueExW(usertypekey, L"", NULL, &valuetype, NULL, &valuelen))
  {
    RegCloseKey(usertypekey);
    return REGDB_E_READREGDB;
  }

  *usertype = CoTaskMemAlloc(valuelen);
  if (!*usertype)
  {
    RegCloseKey(usertypekey);
    return E_OUTOFMEMORY;
  }

  ret = RegQueryValueExW(usertypekey, L"", NULL, &valuetype, (BYTE *)*usertype, &valuelen);
  RegCloseKey(usertypekey);
  if (ret != ERROR_SUCCESS)
  {
    CoTaskMemFree(*usertype);
    *usertype = NULL;
    return REGDB_E_READREGDB;
  }

  return S_OK;
}

/***********************************************************************
 * DoDragDrop [OLE32.@]
 */
HRESULT WINAPI DoDragDrop (
  IDataObject *pDataObject,  /* [in] ptr to the data obj           */
  IDropSource* pDropSource,  /* [in] ptr to the source obj         */
  DWORD       dwOKEffect,    /* [in] effects allowed by the source */
  DWORD       *pdwEffect)    /* [out] ptr to effects of the source */
{
  TrackerWindowInfo trackerInfo;
  HWND            hwndTrackWindow;
  MSG             msg;

  TRACE("%p, %p, %#lx, %p.\n", pDataObject, pDropSource, dwOKEffect, pdwEffect);

  if (!pDataObject || !pDropSource || !pdwEffect)
      return E_INVALIDARG;

  /*
   * Setup the drag n drop tracking window.
   */

  trackerInfo.dataObject        = pDataObject;
  trackerInfo.dropSource        = pDropSource;
  trackerInfo.dwOKEffect        = dwOKEffect;
  trackerInfo.pdwEffect         = pdwEffect;
  trackerInfo.trackingDone      = FALSE;
  trackerInfo.inTrackCall       = FALSE;
  trackerInfo.escPressed        = FALSE;
  trackerInfo.curTargetHWND     = 0;
  trackerInfo.curDragTarget     = 0;

  hwndTrackWindow = CreateWindowW(OLEDD_DRAGTRACKERCLASS, L"TrackerWindow",
                                  WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT,
                                  CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, 0,
                                  &trackerInfo);

  if (hwndTrackWindow)
  {
    /*
     * Capture the mouse input
     */
    SetCapture(hwndTrackWindow);

    msg.message = 0;

    /*
     * Pump messages. All mouse input should go to the capture window.
     */
    while (!trackerInfo.trackingDone && GetMessageW(&msg, 0, 0, 0) )
    {
      trackerInfo.curMousePos.x = msg.pt.x;
      trackerInfo.curMousePos.y = msg.pt.y;
      trackerInfo.dwKeyState = OLEDD_GetButtonState();
	    
      if ( (msg.message >= WM_KEYFIRST) &&
	   (msg.message <= WM_KEYLAST) )
      {
	/*
	 * When keyboard messages are sent to windows on this thread, we
	 * want to ignore notify the drop source that the state changed.
	 * in the case of the Escape key, we also notify the drop source
	 * we give it a special meaning.
	 */
	if ( (msg.message==WM_KEYDOWN) &&
	     (msg.wParam==VK_ESCAPE) )
	{
	  trackerInfo.escPressed = TRUE;
	}

	/*
	 * Notify the drop source.
	 */
        OLEDD_TrackStateChange(&trackerInfo);
      }
      else
      {
	/*
	 * Dispatch the messages only when it's not a keyboard message.
	 */
	DispatchMessageW(&msg);
      }
    }

    /* re-post the quit message to outer message loop */
    if (msg.message == WM_QUIT)
        PostQuitMessage(msg.wParam);
    /*
     * Destroy the temporary window.
     */
    DestroyWindow(hwndTrackWindow);

    return trackerInfo.returnValue;
  }

  return E_FAIL;
}

/***********************************************************************
 * OleQueryLinkFromData [OLE32.@]
 */
HRESULT WINAPI OleQueryLinkFromData(
  IDataObject* pSrcDataObject)
{
  FIXME("(%p),stub!\n", pSrcDataObject);
  return S_FALSE;
}

/***********************************************************************
 * OleRegGetMiscStatus [OLE32.@]
 */
HRESULT WINAPI OleRegGetMiscStatus(
  REFCLSID clsid,
  DWORD    dwAspect,
  DWORD*   pdwStatus)
{
  WCHAR   keyName[16];
  HKEY    miscStatusKey;
  HKEY    aspectKey;
  LONG    result;
  HRESULT hr;

  TRACE("%s, %ld, %p.\n", debugstr_guid(clsid), dwAspect, pdwStatus);

  if (!pdwStatus) return E_INVALIDARG;

  *pdwStatus = 0;

  if (actctx_get_miscstatus(clsid, dwAspect, pdwStatus)) return S_OK;

  hr = COM_OpenKeyForCLSID(clsid, L"MiscStatus", KEY_READ, &miscStatusKey);
  if (FAILED(hr))
    /* missing key is not a failure */
    return hr == REGDB_E_KEYMISSING ? S_OK : hr;

  OLEUTL_ReadRegistryDWORDValue(miscStatusKey, pdwStatus);

  /*
   * Open the key specific to the requested aspect.
   */
  swprintf(keyName, ARRAY_SIZE(keyName), L"%d", dwAspect);

  result = open_classes_key(miscStatusKey, keyName, KEY_READ, &aspectKey);
  if (result == ERROR_SUCCESS)
  {
    OLEUTL_ReadRegistryDWORDValue(aspectKey, pdwStatus);
    RegCloseKey(aspectKey);
  }

  RegCloseKey(miscStatusKey);
  return S_OK;
}

static HRESULT EnumOLEVERB_Construct(HKEY hkeyVerb, ULONG index, IEnumOLEVERB **ppenum);

typedef struct
{
    IEnumOLEVERB IEnumOLEVERB_iface;
    LONG ref;

    HKEY hkeyVerb;
    ULONG index;
} EnumOLEVERB;

static inline EnumOLEVERB *impl_from_IEnumOLEVERB(IEnumOLEVERB *iface)
{
    return CONTAINING_RECORD(iface, EnumOLEVERB, IEnumOLEVERB_iface);
}

static HRESULT WINAPI EnumOLEVERB_QueryInterface(
    IEnumOLEVERB *iface, REFIID riid, void **ppv)
{
    TRACE("(%s, %p)\n", debugstr_guid(riid), ppv);
    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IEnumOLEVERB))
    {
        IEnumOLEVERB_AddRef(iface);
        *ppv = iface;
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI EnumOLEVERB_AddRef(
    IEnumOLEVERB *iface)
{
    EnumOLEVERB *This = impl_from_IEnumOLEVERB(iface);
    TRACE("()\n");
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI EnumOLEVERB_Release(
    IEnumOLEVERB *iface)
{
    EnumOLEVERB *This = impl_from_IEnumOLEVERB(iface);
    LONG refs = InterlockedDecrement(&This->ref);
    TRACE("()\n");
    if (!refs)
    {
        RegCloseKey(This->hkeyVerb);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return refs;
}

static HRESULT WINAPI EnumOLEVERB_Next(
    IEnumOLEVERB *iface, ULONG celt, LPOLEVERB rgelt,
    ULONG *pceltFetched)
{
    EnumOLEVERB *This = impl_from_IEnumOLEVERB(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %lu, %p, %p.\n", iface, celt, rgelt, pceltFetched);

    if (pceltFetched)
        *pceltFetched = 0;

    for (; celt; celt--, rgelt++)
    {
        WCHAR wszSubKey[20];
        LONG cbData;
        LPWSTR pwszOLEVERB;
        LPWSTR pwszMenuFlags;
        LPWSTR pwszAttribs;
        LONG res = RegEnumKeyW(This->hkeyVerb, This->index, wszSubKey, ARRAY_SIZE(wszSubKey));
        if (res == ERROR_NO_MORE_ITEMS)
        {
            hr = S_FALSE;
            break;
        }
        else if (res != ERROR_SUCCESS)
        {
            ERR("RegEnumKeyW failed with error %ld\n", res);
            hr = REGDB_E_READREGDB;
            break;
        }
        res = RegQueryValueW(This->hkeyVerb, wszSubKey, NULL, &cbData);
        if (res != ERROR_SUCCESS)
        {
            ERR("RegQueryValueW failed with error %ld\n", res);
            hr = REGDB_E_READREGDB;
            break;
        }
        pwszOLEVERB = CoTaskMemAlloc(cbData);
        if (!pwszOLEVERB)
        {
            hr = E_OUTOFMEMORY;
            break;
        }
        res = RegQueryValueW(This->hkeyVerb, wszSubKey, pwszOLEVERB, &cbData);
        if (res != ERROR_SUCCESS)
        {
            ERR("RegQueryValueW failed with error %ld\n", res);
            hr = REGDB_E_READREGDB;
            CoTaskMemFree(pwszOLEVERB);
            break;
        }

        TRACE("verb string: %s\n", debugstr_w(pwszOLEVERB));
        pwszMenuFlags = wcschr(pwszOLEVERB, ',');
        if (!pwszMenuFlags)
        {
            hr = OLEOBJ_E_INVALIDVERB;
            CoTaskMemFree(pwszOLEVERB);
            break;
        }
        /* nul terminate the name string and advance to first character */
        *pwszMenuFlags = '\0';
        pwszMenuFlags++;
        pwszAttribs = wcschr(pwszMenuFlags, ',');
        if (!pwszAttribs)
        {
            hr = OLEOBJ_E_INVALIDVERB;
            CoTaskMemFree(pwszOLEVERB);
            break;
        }
        /* nul terminate the menu string and advance to first character */
        *pwszAttribs = '\0';
        pwszAttribs++;

        /* fill out structure for this verb */
        rgelt->lVerb = wcstol(wszSubKey, NULL, 10);
        rgelt->lpszVerbName = pwszOLEVERB; /* user should free */
        rgelt->fuFlags = wcstol(pwszMenuFlags, NULL, 10);
        rgelt->grfAttribs = wcstol(pwszAttribs, NULL, 10);

        if (pceltFetched)
            (*pceltFetched)++;
        This->index++;
    }
    return hr;
}

static HRESULT WINAPI EnumOLEVERB_Skip(
    IEnumOLEVERB *iface, ULONG celt)
{
    EnumOLEVERB *This = impl_from_IEnumOLEVERB(iface);

    TRACE("%p, %lu.\n", iface, celt);

    This->index += celt;
    return S_OK;
}

static HRESULT WINAPI EnumOLEVERB_Reset(
    IEnumOLEVERB *iface)
{
    EnumOLEVERB *This = impl_from_IEnumOLEVERB(iface);

    TRACE("()\n");

    This->index = 0;
    return S_OK;
}

static HRESULT WINAPI EnumOLEVERB_Clone(
    IEnumOLEVERB *iface,
    IEnumOLEVERB **ppenum)
{
    EnumOLEVERB *This = impl_from_IEnumOLEVERB(iface);
    HKEY hkeyVerb;
    TRACE("(%p)\n", ppenum);
    if (!DuplicateHandle(GetCurrentProcess(), This->hkeyVerb, GetCurrentProcess(), (HANDLE *)&hkeyVerb, 0, FALSE, DUPLICATE_SAME_ACCESS))
        return HRESULT_FROM_WIN32(GetLastError());
    return EnumOLEVERB_Construct(hkeyVerb, This->index, ppenum);
}

static const IEnumOLEVERBVtbl EnumOLEVERB_VTable =
{
    EnumOLEVERB_QueryInterface,
    EnumOLEVERB_AddRef,
    EnumOLEVERB_Release,
    EnumOLEVERB_Next,
    EnumOLEVERB_Skip,
    EnumOLEVERB_Reset,
    EnumOLEVERB_Clone
};

static HRESULT EnumOLEVERB_Construct(HKEY hkeyVerb, ULONG index, IEnumOLEVERB **ppenum)
{
    EnumOLEVERB *This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if (!This)
    {
        RegCloseKey(hkeyVerb);
        return E_OUTOFMEMORY;
    }
    This->IEnumOLEVERB_iface.lpVtbl = &EnumOLEVERB_VTable;
    This->ref = 1;
    This->index = index;
    This->hkeyVerb = hkeyVerb;
    *ppenum = &This->IEnumOLEVERB_iface;
    return S_OK;
}

/***********************************************************************
 *           OleRegEnumVerbs    [OLE32.@]
 *
 * Enumerates verbs associated with a class stored in the registry.
 *
 * PARAMS
 *  clsid  [I] Class ID to enumerate the verbs for.
 *  ppenum [O] Enumerator.
 *
 * RETURNS
 *  S_OK: Success.
 *  REGDB_E_CLASSNOTREG: The specified class does not have a key in the registry.
 *  REGDB_E_READREGDB: The class key could not be opened for some other reason.
 *  OLE_E_REGDB_KEY: The Verb subkey for the class is not present.
 *  OLEOBJ_E_NOVERBS: The Verb subkey for the class is empty.
 */
HRESULT WINAPI OleRegEnumVerbs (REFCLSID clsid, LPENUMOLEVERB* ppenum)
{
    LONG res;
    HKEY hkeyVerb;
    DWORD dwSubKeys;

    TRACE("(%s, %p)\n", debugstr_guid(clsid), ppenum);

    res = COM_OpenKeyForCLSID(clsid, L"Verb", KEY_READ, &hkeyVerb);
    if (FAILED(res))
    {
        if (res == REGDB_E_CLASSNOTREG)
            ERR("CLSID %s not registered\n", debugstr_guid(clsid));
        else if (res == REGDB_E_KEYMISSING)
            ERR("no Verbs key for class %s\n", debugstr_guid(clsid));
        else
            ERR("failed to open Verbs key for CLSID %s with error %ld\n",
                debugstr_guid(clsid), res);
        return res;
    }

    res = RegQueryInfoKeyW(hkeyVerb, NULL, NULL, NULL, &dwSubKeys, NULL,
                          NULL, NULL, NULL, NULL, NULL, NULL);
    if (res != ERROR_SUCCESS)
    {
        ERR("failed to get subkey count with error %ld\n", GetLastError());
        return REGDB_E_READREGDB;
    }

    if (!dwSubKeys)
    {
        WARN("class %s has no verbs\n", debugstr_guid(clsid));
        RegCloseKey(hkeyVerb);
        return OLEOBJ_E_NOVERBS;
    }

    return EnumOLEVERB_Construct(hkeyVerb, 0, ppenum);
}

/******************************************************************************
 *              OleSetContainedObject        [OLE32.@]
 */
HRESULT WINAPI OleSetContainedObject(
  LPUNKNOWN pUnknown,
  BOOL      fContained)
{
  IRunnableObject* runnable = NULL;
  HRESULT          hres;

  TRACE("(%p,%x)\n", pUnknown, fContained);

  hres = IUnknown_QueryInterface(pUnknown,
				 &IID_IRunnableObject,
				 (void**)&runnable);

  if (SUCCEEDED(hres))
  {
    hres = IRunnableObject_SetContainedObject(runnable, fContained);

    IRunnableObject_Release(runnable);

    return hres;
  }

  return S_OK;
}

/******************************************************************************
 *              OleRun        [OLE32.@]
 *
 * Set the OLE object to the running state.
 *
 * PARAMS
 *  pUnknown [I] OLE object to run.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: Any HRESULT code.
 */
HRESULT WINAPI DECLSPEC_HOTPATCH OleRun(LPUNKNOWN pUnknown)
{
    IRunnableObject *runable;
    HRESULT hres;

    TRACE("(%p)\n", pUnknown);

    hres = IUnknown_QueryInterface(pUnknown, &IID_IRunnableObject, (void**)&runable);
    if (FAILED(hres))
        return S_OK; /* Appears to return no error. */

    hres = IRunnableObject_Run(runable, NULL);
    IRunnableObject_Release(runable);
    return hres;
}

/******************************************************************************
 *              OleLoad        [OLE32.@]
 */
HRESULT WINAPI OleLoad(
  LPSTORAGE       pStg,
  REFIID          riid,
  LPOLECLIENTSITE pClientSite,
  LPVOID*         ppvObj)
{
  IPersistStorage* persistStorage = NULL;
  IUnknown*        pUnk;
  IOleObject*      pOleObject      = NULL;
  STATSTG          storageInfo;
  HRESULT          hres;

  TRACE("(%p, %s, %p, %p)\n", pStg, debugstr_guid(riid), pClientSite, ppvObj);

  *ppvObj = NULL;

  /*
   * TODO, Conversion ... OleDoAutoConvert
   */

  /*
   * Get the class ID for the object.
   */
  hres = IStorage_Stat(pStg, &storageInfo, STATFLAG_NONAME);
  if (FAILED(hres))
    return hres;

  /*
   * Now, try and create the handler for the object
   */
  hres = CoCreateInstance(&storageInfo.clsid,
			  NULL,
			  CLSCTX_INPROC_HANDLER|CLSCTX_INPROC_SERVER,
			  riid,
			  (void**)&pUnk);

  /*
   * If that fails, as it will most times, load the default
   * OLE handler.
   */
  if (FAILED(hres))
  {
    hres = OleCreateDefaultHandler(&storageInfo.clsid,
				   NULL,
				   riid,
				   (void**)&pUnk);
  }

  /*
   * If we couldn't find a handler... this is bad. Abort the whole thing.
   */
  if (FAILED(hres))
    return hres;

  if (pClientSite)
  {
    hres = IUnknown_QueryInterface(pUnk, &IID_IOleObject, (void **)&pOleObject);
    if (SUCCEEDED(hres))
    {
        DWORD dwStatus;
        hres = IOleObject_GetMiscStatus(pOleObject, DVASPECT_CONTENT, &dwStatus);
    }
  }

  /*
   * Initialize the object with its IPersistStorage interface.
   */
  hres = IUnknown_QueryInterface(pUnk, &IID_IPersistStorage, (void**)&persistStorage);
  if (SUCCEEDED(hres))
  {
    hres = IPersistStorage_Load(persistStorage, pStg);

    IPersistStorage_Release(persistStorage);
    persistStorage = NULL;
  }

  if (SUCCEEDED(hres) && pClientSite)
    /*
     * Inform the new object of its client site.
     */
    hres = IOleObject_SetClientSite(pOleObject, pClientSite);

  /*
   * Cleanup interfaces used internally
   */
  if (pOleObject)
    IOleObject_Release(pOleObject);

  if (SUCCEEDED(hres))
  {
    IOleLink *pOleLink;
    HRESULT hres1;
    hres1 = IUnknown_QueryInterface(pUnk, &IID_IOleLink, (void **)&pOleLink);
    if (SUCCEEDED(hres1))
    {
      FIXME("handle OLE link\n");
      IOleLink_Release(pOleLink);
    }
  }

  if (FAILED(hres))
  {
    IUnknown_Release(pUnk);
    pUnk = NULL;
  }

  *ppvObj = pUnk;

  return hres;
}

/***********************************************************************
 *           OleSave     [OLE32.@]
 */
HRESULT WINAPI OleSave(
  LPPERSISTSTORAGE pPS,
  LPSTORAGE        pStg,
  BOOL             fSameAsLoad)
{
  HRESULT hres;
  CLSID   objectClass;

  TRACE("(%p,%p,%x)\n", pPS, pStg, fSameAsLoad);

  /*
   * First, we transfer the class ID (if available)
   */
  hres = IPersistStorage_GetClassID(pPS, &objectClass);

  if (SUCCEEDED(hres))
  {
    WriteClassStg(pStg, &objectClass);
  }

  /*
   * Then, we ask the object to save itself to the
   * storage. If it is successful, we commit the storage.
   */
  hres = IPersistStorage_Save(pPS, pStg, fSameAsLoad);

  if (SUCCEEDED(hres))
  {
    IStorage_Commit(pStg,
		    STGC_DEFAULT);
  }

  return hres;
}


/******************************************************************************
 *              OleLockRunning        [OLE32.@]
 */
HRESULT WINAPI OleLockRunning(LPUNKNOWN pUnknown, BOOL fLock, BOOL fLastUnlockCloses)
{
  IRunnableObject* runnable = NULL;
  HRESULT          hres;

  TRACE("(%p,%x,%x)\n", pUnknown, fLock, fLastUnlockCloses);

  hres = IUnknown_QueryInterface(pUnknown,
				 &IID_IRunnableObject,
				 (void**)&runnable);

  if (SUCCEEDED(hres))
  {
    hres = IRunnableObject_LockRunning(runnable, fLock, fLastUnlockCloses);

    IRunnableObject_Release(runnable);

    return hres;
  }

  return S_OK;
}


/**************************************************************************
 * Internal methods to manage the shared OLE menu in response to the
 * OLE***MenuDescriptor API
 */

/***
 * OLEMenu_Initialize()
 *
 * Initializes the OLEMENU data structures.
 */
static void OLEMenu_Initialize(void)
{
}

/***
 * OLEMenu_UnInitialize()
 *
 * Releases the OLEMENU data structures.
 */
static void OLEMenu_UnInitialize(void)
{
}

/*************************************************************************
 * OLEMenu_InstallHooks
 * Install thread scope message hooks for WH_GETMESSAGE and WH_CALLWNDPROC
 *
 * RETURNS: TRUE if message hooks were successfully installed
 *          FALSE on failure
 */
static BOOL OLEMenu_InstallHooks( DWORD tid )
{
  OleMenuHookItem *pHookItem;

  /* Create an entry for the hook table */
  if ( !(pHookItem = HeapAlloc(GetProcessHeap(), 0,
                               sizeof(OleMenuHookItem)) ) )
    return FALSE;

  pHookItem->tid = tid;
  pHookItem->hHeap = GetProcessHeap();
  pHookItem->CallWndProc_hHook = NULL;

  /* Install a thread scope message hook for WH_GETMESSAGE */
  pHookItem->GetMsg_hHook = SetWindowsHookExW( WH_GETMESSAGE, OLEMenu_GetMsgProc,
                                               0, GetCurrentThreadId() );
  if ( !pHookItem->GetMsg_hHook )
    goto CLEANUP;

  /* Install a thread scope message hook for WH_CALLWNDPROC */
  pHookItem->CallWndProc_hHook = SetWindowsHookExW( WH_CALLWNDPROC, OLEMenu_CallWndProc,
                                                    0, GetCurrentThreadId() );
  if ( !pHookItem->CallWndProc_hHook )
    goto CLEANUP;

  /* Insert the hook table entry */
  pHookItem->next = hook_list;
  hook_list = pHookItem;

  return TRUE;

CLEANUP:
  /* Unhook any hooks */
  if ( pHookItem->GetMsg_hHook )
    UnhookWindowsHookEx( pHookItem->GetMsg_hHook );
  if ( pHookItem->CallWndProc_hHook )
    UnhookWindowsHookEx( pHookItem->CallWndProc_hHook );
  /* Release the hook table entry */
  HeapFree(pHookItem->hHeap, 0, pHookItem );

  return FALSE;
}

/*************************************************************************
 * OLEMenu_UnInstallHooks
 * UnInstall thread scope message hooks for WH_GETMESSAGE and WH_CALLWNDPROC
 *
 * RETURNS: TRUE if message hooks were successfully installed
 *          FALSE on failure
 */
static BOOL OLEMenu_UnInstallHooks( DWORD tid )
{
  OleMenuHookItem *pHookItem = NULL;
  OleMenuHookItem **ppHook = &hook_list;

  while (*ppHook)
  {
      if ((*ppHook)->tid == tid)
      {
          pHookItem = *ppHook;
          *ppHook = pHookItem->next;
          break;
      }
      ppHook = &(*ppHook)->next;
  }
  if (!pHookItem) return FALSE;

  /* Uninstall the hooks installed for this thread */
  if ( !UnhookWindowsHookEx( pHookItem->GetMsg_hHook ) )
    goto CLEANUP;
  if ( !UnhookWindowsHookEx( pHookItem->CallWndProc_hHook ) )
    goto CLEANUP;

  /* Release the hook table entry */
  HeapFree(pHookItem->hHeap, 0, pHookItem );

  return TRUE;

CLEANUP:
  /* Release the hook table entry */
  HeapFree(pHookItem->hHeap, 0, pHookItem );

  return FALSE;
}

/*************************************************************************
 * OLEMenu_IsHookInstalled
 * Tests if OLEMenu hooks have been installed for a thread
 *
 * RETURNS: The pointer and index of the hook table entry for the tid
 *          NULL and -1 for the index if no hooks were installed for this thread
 */
static OleMenuHookItem * OLEMenu_IsHookInstalled( DWORD tid )
{
  OleMenuHookItem *pHookItem;

  /* Do a simple linear search for an entry whose tid matches ours.
   * We really need a map but efficiency is not a concern here. */
  for (pHookItem = hook_list; pHookItem; pHookItem = pHookItem->next)
  {
    if ( tid == pHookItem->tid )
      return pHookItem;
  }

  return NULL;
}

/***********************************************************************
 *           OLEMenu_FindMainMenuIndex
 *
 * Used by OLEMenu API to find the top level group a menu item belongs to.
 * On success pnPos contains the index of the item in the top level menu group
 *
 * RETURNS: TRUE if the ID was found, FALSE on failure
 */
static BOOL OLEMenu_FindMainMenuIndex( HMENU hMainMenu, HMENU hPopupMenu, UINT *pnPos )
{
  INT i, nItems;

  nItems = GetMenuItemCount( hMainMenu );

  for (i = 0; i < nItems; i++)
  {
    HMENU hsubmenu;

    /*  Is the current item a submenu? */
    if ( (hsubmenu = GetSubMenu(hMainMenu, i)) )
    {
      /* If the handle is the same we're done */
      if ( hsubmenu == hPopupMenu )
      {
        if (pnPos)
          *pnPos = i;
        return TRUE;
      }
      /* Recursively search without updating pnPos */
      else if ( OLEMenu_FindMainMenuIndex( hsubmenu, hPopupMenu, NULL ) )
      {
        if (pnPos)
          *pnPos = i;
        return TRUE;
      }
    }
  }

  return FALSE;
}

/***********************************************************************
 *           OLEMenu_SetIsServerMenu
 *
 * Checks whether a popup menu belongs to a shared menu group which is
 * owned by the server, and sets the menu descriptor state accordingly.
 * All menu messages from these groups should be routed to the server.
 *
 * RETURNS: TRUE if the popup menu is part of a server owned group
 *          FALSE if the popup menu is part of a container owned group
 */
static BOOL OLEMenu_SetIsServerMenu( HMENU hmenu, OleMenuDescriptor *pOleMenuDescriptor )
{
  UINT nPos = 0, nWidth, i;

  pOleMenuDescriptor->bIsServerItem = FALSE;

  /* Don't bother searching if the popup is the combined menu itself */
  if ( hmenu == pOleMenuDescriptor->hmenuCombined )
    return FALSE;

  /* Find the menu item index in the shared OLE menu that this item belongs to */
  if ( !OLEMenu_FindMainMenuIndex( pOleMenuDescriptor->hmenuCombined, hmenu,  &nPos ) )
    return FALSE;

  /* The group widths array has counts for the number of elements
   * in the groups File, Edit, Container, Object, Window, Help.
   * The Edit, Object & Help groups belong to the server object
   * and the other three belong to the container.
   * Loop through the group widths and locate the group we are a member of.
   */
  for ( i = 0, nWidth = 0; i < 6; i++ )
  {
    nWidth += pOleMenuDescriptor->mgw.width[i];
    if ( nPos < nWidth )
    {
      /* Odd elements are server menu widths */
      pOleMenuDescriptor->bIsServerItem = i%2;
      break;
    }
  }

  return pOleMenuDescriptor->bIsServerItem;
}

/*************************************************************************
 * OLEMenu_CallWndProc
 * Thread scope WH_CALLWNDPROC hook proc filter function (callback)
 * This is invoked from a message hook installed in OleSetMenuDescriptor.
 */
static LRESULT CALLBACK OLEMenu_CallWndProc(INT code, WPARAM wParam, LPARAM lParam)
{
  LPCWPSTRUCT pMsg;
  HOLEMENU hOleMenu = 0;
  OleMenuDescriptor *pOleMenuDescriptor = NULL;
  OleMenuHookItem *pHookItem = NULL;
  WORD fuFlags;

  TRACE("%i, %#Ix, %#Ix.\n", code, wParam, lParam );

  /* Check if we're being asked to process the message */
  if ( HC_ACTION != code )
    goto NEXTHOOK;

  /* Retrieve the current message being dispatched from lParam */
  pMsg = (LPCWPSTRUCT)lParam;

  /* Check if the message is destined for a window we are interested in:
   * If the window has an OLEMenu property we may need to dispatch
   * the menu message to its active objects window instead. */

  hOleMenu = GetPropW( pMsg->hwnd, prop_olemenuW );
  if ( !hOleMenu )
    goto NEXTHOOK;

  /* Get the menu descriptor */
  pOleMenuDescriptor = GlobalLock( hOleMenu );
  if ( !pOleMenuDescriptor ) /* Bad descriptor! */
    goto NEXTHOOK;

  /* Process menu messages */
  switch( pMsg->message )
  {
    case WM_INITMENU:
    {
      /* Reset the menu descriptor state */
      pOleMenuDescriptor->bIsServerItem = FALSE;

      /* Send this message to the server as well */
      SendMessageW( pOleMenuDescriptor->hwndActiveObject,
                  pMsg->message, pMsg->wParam, pMsg->lParam );
      goto NEXTHOOK;
    }

    case WM_INITMENUPOPUP:
    {
      /* Save the state for whether this is a server owned menu */
      OLEMenu_SetIsServerMenu( (HMENU)pMsg->wParam, pOleMenuDescriptor );
      break;
    }

    case WM_MENUSELECT:
    {
      fuFlags = HIWORD(pMsg->wParam);  /* Get flags */
      if ( fuFlags & MF_SYSMENU )
         goto NEXTHOOK;

      /* Save the state for whether this is a server owned popup menu */
      else if ( fuFlags & MF_POPUP )
        OLEMenu_SetIsServerMenu( (HMENU)pMsg->lParam, pOleMenuDescriptor );

      break;
    }

    case WM_DRAWITEM:
    {
      LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT) pMsg->lParam;
      if ( pMsg->wParam != 0 || lpdis->CtlType != ODT_MENU )
        goto NEXTHOOK;  /* Not a menu message */

      break;
    }

    default:
      goto NEXTHOOK;
  }

  /* If the message was for the server dispatch it accordingly */
  if ( pOleMenuDescriptor->bIsServerItem )
  {
    SendMessageW( pOleMenuDescriptor->hwndActiveObject,
                  pMsg->message, pMsg->wParam, pMsg->lParam );
  }

NEXTHOOK:
  if ( pOleMenuDescriptor )
    GlobalUnlock( hOleMenu );

  /* Lookup the hook item for the current thread */
  if ( !( pHookItem = OLEMenu_IsHookInstalled( GetCurrentThreadId() ) ) )
  {
    /* This should never fail!! */
    WARN("could not retrieve hHook for current thread!\n" );
    return 0;
  }

  /* Pass on the message to the next hooker */
  return CallNextHookEx( pHookItem->CallWndProc_hHook, code, wParam, lParam );
}

/*************************************************************************
 * OLEMenu_GetMsgProc
 * Thread scope WH_GETMESSAGE hook proc filter function (callback)
 * This is invoked from a message hook installed in OleSetMenuDescriptor.
 */
static LRESULT CALLBACK OLEMenu_GetMsgProc(INT code, WPARAM wParam, LPARAM lParam)
{
  LPMSG pMsg;
  HOLEMENU hOleMenu = 0;
  OleMenuDescriptor *pOleMenuDescriptor = NULL;
  OleMenuHookItem *pHookItem = NULL;
  WORD wCode;

  TRACE("%i, %#Ix, %#Ix.\n", code, wParam, lParam );

  /* Check if we're being asked to process a  messages */
  if ( HC_ACTION != code )
    goto NEXTHOOK;

  /* Retrieve the current message being dispatched from lParam */
  pMsg = (LPMSG)lParam;

  /* Check if the message is destined for a window we are interested in:
   * If the window has an OLEMenu property we may need to dispatch
   * the menu message to its active objects window instead. */

  hOleMenu = GetPropW( pMsg->hwnd, prop_olemenuW );
  if ( !hOleMenu )
    goto NEXTHOOK;

  /* Process menu messages */
  switch( pMsg->message )
  {
    case WM_COMMAND:
    {
      wCode = HIWORD(pMsg->wParam);  /* Get notification code */
      if ( wCode )
        goto NEXTHOOK;  /* Not a menu message */
      break;
    }
    default:
      goto NEXTHOOK;
  }

  /* Get the menu descriptor */
  pOleMenuDescriptor = GlobalLock( hOleMenu );
  if ( !pOleMenuDescriptor ) /* Bad descriptor! */
    goto NEXTHOOK;

  /* If the message was for the server dispatch it accordingly */
  if ( pOleMenuDescriptor->bIsServerItem )
  {
    /* Change the hWnd in the message to the active objects hWnd.
     * The message loop which reads this message will automatically
     * dispatch it to the embedded objects window. */
    pMsg->hwnd = pOleMenuDescriptor->hwndActiveObject;
  }

NEXTHOOK:
  if ( pOleMenuDescriptor )
    GlobalUnlock( hOleMenu );

  /* Lookup the hook item for the current thread */
  if ( !( pHookItem = OLEMenu_IsHookInstalled( GetCurrentThreadId() ) ) )
  {
    /* This should never fail!! */
    WARN("could not retrieve hHook for current thread!\n" );
    return FALSE;
  }

  /* Pass on the message to the next hooker */
  return CallNextHookEx( pHookItem->GetMsg_hHook, code, wParam, lParam );
}

/***********************************************************************
 * OleCreateMenuDescriptor [OLE32.@]
 * Creates an OLE menu descriptor for OLE to use when dispatching
 * menu messages and commands.
 *
 * PARAMS:
 *    hmenuCombined  -  Handle to the objects combined menu
 *    lpMenuWidths   -  Pointer to array of 6 LONG's indicating menus per group
 *
 */
HOLEMENU WINAPI OleCreateMenuDescriptor(
  HMENU                hmenuCombined,
  LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
  HOLEMENU hOleMenu;
  OleMenuDescriptor *pOleMenuDescriptor;
  int i;

  if ( !hmenuCombined || !lpMenuWidths )
    return 0;

  /* Create an OLE menu descriptor */
  if ( !(hOleMenu = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                sizeof(OleMenuDescriptor) ) ) )
  return 0;

  pOleMenuDescriptor = GlobalLock( hOleMenu );
  if ( !pOleMenuDescriptor )
    return 0;

  /* Initialize menu group widths and hmenu */
  for ( i = 0; i < 6; i++ )
    pOleMenuDescriptor->mgw.width[i] = lpMenuWidths->width[i];

  pOleMenuDescriptor->hmenuCombined = hmenuCombined;
  pOleMenuDescriptor->bIsServerItem = FALSE;
  GlobalUnlock( hOleMenu );

  return hOleMenu;
}

/***********************************************************************
 * OleDestroyMenuDescriptor [OLE32.@]
 * Destroy the shared menu descriptor
 */
HRESULT WINAPI OleDestroyMenuDescriptor(
    HOLEMENU hmenuDescriptor)
{
    if ( hmenuDescriptor )
        GlobalFree( hmenuDescriptor );
    return S_OK;
}

/***********************************************************************
 * OleSetMenuDescriptor [OLE32.@]
 * Installs or removes OLE dispatching code for the containers frame window.
 *
 * PARAMS
 *     hOleMenu         Handle to composite menu descriptor
 *     hwndFrame        Handle to containers frame window
 *     hwndActiveObject Handle to objects in-place activation window
 *     lpFrame          Pointer to IOleInPlaceFrame on containers window
 *     lpActiveObject   Pointer to IOleInPlaceActiveObject on active in-place object
 *
 * RETURNS
 *      S_OK                               - menu installed correctly
 *      E_FAIL, E_INVALIDARG, E_UNEXPECTED - failure
 *
 * FIXME
 *      The lpFrame and lpActiveObject parameters are currently ignored
 *      OLE should install context sensitive help F1 filtering for the app when
 *      these are non null.
 */
HRESULT WINAPI OleSetMenuDescriptor(
    HOLEMENU               hOleMenu,
    HWND                   hwndFrame,
    HWND                   hwndActiveObject,
    LPOLEINPLACEFRAME        lpFrame,
    LPOLEINPLACEACTIVEOBJECT lpActiveObject)
{
    OleMenuDescriptor *pOleMenuDescriptor = NULL;

    /* Check args */
    if ( !hwndFrame || (hOleMenu && !hwndActiveObject) )
        return E_INVALIDARG;

    if ( lpFrame || lpActiveObject )
    {
        FIXME("(%p, %p, %p, %p, %p), Context sensitive help filtering not implemented!\n",
        hOleMenu,
        hwndFrame,
        hwndActiveObject,
        lpFrame,
        lpActiveObject);
    }

    /* Set up a message hook to intercept the containers frame window messages.
     * The message filter is responsible for dispatching menu messages from the
     * shared menu which are intended for the object.
     */

    if ( hOleMenu )  /* Want to install dispatching code */
    {
        /* If OLEMenu hooks are already installed for this thread, fail
         * Note: This effectively means that OleSetMenuDescriptor cannot
         * be called twice in succession on the same frame window
         * without first calling it with a null hOleMenu to uninstall
         */
        if ( OLEMenu_IsHookInstalled( GetCurrentThreadId() ) )
            return E_FAIL;

        /* Get the menu descriptor */
        pOleMenuDescriptor = GlobalLock( hOleMenu );
        if ( !pOleMenuDescriptor )
            return E_UNEXPECTED;

        /* Update the menu descriptor */
        pOleMenuDescriptor->hwndFrame = hwndFrame;
        pOleMenuDescriptor->hwndActiveObject = hwndActiveObject;

        GlobalUnlock( hOleMenu );
        pOleMenuDescriptor = NULL;

        /* Add a menu descriptor windows property to the frame window */
        SetPropW( hwndFrame, prop_olemenuW, hOleMenu );

        /* Install thread scope message hooks for WH_GETMESSAGE and WH_CALLWNDPROC */
        if ( !OLEMenu_InstallHooks( GetCurrentThreadId() ) )
            return E_FAIL;
    }
    else  /* Want to uninstall dispatching code */
    {
        /* Uninstall the hooks */
        if ( !OLEMenu_UnInstallHooks( GetCurrentThreadId() ) )
            return E_FAIL;

        /* Remove the menu descriptor property from the frame window */
        RemovePropW( hwndFrame, prop_olemenuW );
    }

    return S_OK;
}

/******************************************************************************
 *              IsAccelerator        [OLE32.@]
 * Mostly copied from controls/menu.c TranslateAccelerator implementation
 */
BOOL WINAPI IsAccelerator(HACCEL hAccel, int cAccelEntries, LPMSG lpMsg, WORD* lpwCmd)
{
    LPACCEL lpAccelTbl;
    int i;

    if(!lpMsg) return FALSE;
    if (!hAccel)
    {
	WARN_(accel)("NULL accel handle\n");
	return FALSE;
    }
    if((lpMsg->message != WM_KEYDOWN &&
	lpMsg->message != WM_SYSKEYDOWN &&
	lpMsg->message != WM_SYSCHAR &&
	lpMsg->message != WM_CHAR)) return FALSE;
    lpAccelTbl = HeapAlloc(GetProcessHeap(), 0, cAccelEntries * sizeof(ACCEL));
    if (NULL == lpAccelTbl)
    {
	return FALSE;
    }
    if (CopyAcceleratorTableW(hAccel, lpAccelTbl, cAccelEntries) != cAccelEntries)
    {
	WARN_(accel)("CopyAcceleratorTableW failed\n");
	HeapFree(GetProcessHeap(), 0, lpAccelTbl);
	return FALSE;
    }

    TRACE_(accel)("hAccel=%p, cAccelEntries=%d,"
		"msg->hwnd=%p, msg->message=%04x, wParam=%#Ix, lParam=%#Ix\n",
		hAccel, cAccelEntries,
		lpMsg->hwnd, lpMsg->message, lpMsg->wParam, lpMsg->lParam);
    for(i = 0; i < cAccelEntries; i++)
    {
	if(lpAccelTbl[i].key != lpMsg->wParam)
	    continue;

	if(lpMsg->message == WM_CHAR)
	{
	    if(!(lpAccelTbl[i].fVirt & FALT) && !(lpAccelTbl[i].fVirt & FVIRTKEY))
	    {
		TRACE_(accel)("found accel for WM_CHAR: ('%c')\n", LOWORD(lpMsg->wParam) & 0xff);
		goto found;
	    }
	}
	else
	{
	    if(lpAccelTbl[i].fVirt & FVIRTKEY)
	    {
		INT mask = 0;
		TRACE_(accel)("found accel for virt_key %Ix (scan %04x)\n",
				lpMsg->wParam, HIWORD(lpMsg->lParam) & 0xff);
		if(GetKeyState(VK_SHIFT) & 0x8000) mask |= FSHIFT;
		if(GetKeyState(VK_CONTROL) & 0x8000) mask |= FCONTROL;
		if(GetKeyState(VK_MENU) & 0x8000) mask |= FALT;
		if(mask == (lpAccelTbl[i].fVirt & (FSHIFT | FCONTROL | FALT))) goto found;
		TRACE_(accel)("incorrect SHIFT/CTRL/ALT-state\n");
	    }
	    else
	    {
		if(!(lpMsg->lParam & 0x01000000))  /* no special_key */
		{
		    if((lpAccelTbl[i].fVirt & FALT) && (lpMsg->lParam & 0x20000000))
		    {						       /* ^^ ALT pressed */
			TRACE_(accel)("found accel for Alt-%c\n", LOWORD(lpMsg->wParam) & 0xff);
			goto found;
		    }
		}
	    }
	}
    }

    WARN_(accel)("couldn't translate accelerator key\n");
    HeapFree(GetProcessHeap(), 0, lpAccelTbl);
    return FALSE;

found:
    if(lpwCmd) *lpwCmd = lpAccelTbl[i].cmd;
    HeapFree(GetProcessHeap(), 0, lpAccelTbl);
    return TRUE;
}

/***********************************************************************
 * ReleaseStgMedium [OLE32.@]
 */
void WINAPI ReleaseStgMedium(
  STGMEDIUM* pmedium)
{
  if (!pmedium) return;

  switch (pmedium->tymed)
  {
    case TYMED_HGLOBAL:
    {
      if ( (pmedium->pUnkForRelease==0) &&
	   (pmedium->hGlobal!=0) )
	GlobalFree(pmedium->hGlobal);
      break;
    }
    case TYMED_FILE:
    {
      if (pmedium->lpszFileName!=0)
      {
	if (pmedium->pUnkForRelease==0)
	{
	  DeleteFileW(pmedium->lpszFileName);
	}

	CoTaskMemFree(pmedium->lpszFileName);
      }
      break;
    }
    case TYMED_ISTREAM:
    {
      if (pmedium->pstm!=0)
      {
	IStream_Release(pmedium->pstm);
      }
      break;
    }
    case TYMED_ISTORAGE:
    {
      if (pmedium->pstg!=0)
      {
	IStorage_Release(pmedium->pstg);
      }
      break;
    }
    case TYMED_GDI:
    {
      if ( (pmedium->pUnkForRelease==0) &&
	   (pmedium->hBitmap!=0) )
	DeleteObject(pmedium->hBitmap);
      break;
    }
    case TYMED_MFPICT:
    {
      if ( (pmedium->pUnkForRelease==0) &&
	   (pmedium->hMetaFilePict!=0) )
      {
	LPMETAFILEPICT pMP = GlobalLock(pmedium->hMetaFilePict);
	DeleteMetaFile(pMP->hMF);
	GlobalUnlock(pmedium->hMetaFilePict);
	GlobalFree(pmedium->hMetaFilePict);
      }
      break;
    }
    case TYMED_ENHMF:
    {
      if ( (pmedium->pUnkForRelease==0) &&
	   (pmedium->hEnhMetaFile!=0) )
      {
	DeleteEnhMetaFile(pmedium->hEnhMetaFile);
      }
      break;
    }
    case TYMED_NULL:
    default:
      break;
  }
  pmedium->tymed=TYMED_NULL;

  /*
   * After cleaning up, the unknown is released
   */
  if (pmedium->pUnkForRelease!=0)
  {
    IUnknown_Release(pmedium->pUnkForRelease);
    pmedium->pUnkForRelease = 0;
  }
}

/***
 * OLEDD_Initialize()
 *
 * Initializes the OLE drag and drop data structures.
 */
static void OLEDD_Initialize(void)
{
    WNDCLASSW wndClass;

    ZeroMemory (&wndClass, sizeof(WNDCLASSW));
    wndClass.style         = CS_GLOBALCLASS;
    wndClass.lpfnWndProc   = OLEDD_DragTrackerWindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(TrackerWindowInfo*);
    wndClass.hCursor       = 0;
    wndClass.hbrBackground = 0;
    wndClass.lpszClassName = OLEDD_DRAGTRACKERCLASS;

    RegisterClassW (&wndClass);
}

/***
 * OLEDD_DragTrackerWindowProc()
 *
 * This method is the WindowProcedure of the drag n drop tracking
 * window. During a drag n Drop operation, an invisible window is created
 * to receive the user input and act upon it. This procedure is in charge
 * of this behavior.
 */

#define DRAG_TIMER_ID 1

static LRESULT WINAPI OLEDD_DragTrackerWindowProc(
			 HWND   hwnd,
			 UINT   uMsg,
			 WPARAM wParam,
			 LPARAM   lParam)
{
  switch (uMsg)
  {
    case WM_CREATE:
    {
      LPCREATESTRUCTA createStruct = (LPCREATESTRUCTA)lParam;

      SetWindowLongPtrW(hwnd, 0, (LONG_PTR)createStruct->lpCreateParams);
      SetTimer(hwnd, DRAG_TIMER_ID, 50, NULL);

      break;
    }
    case WM_TIMER:
    case WM_MOUSEMOVE:
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
    {
      TrackerWindowInfo *trackerInfo = (TrackerWindowInfo*)GetWindowLongPtrA(hwnd, 0);
      if (trackerInfo->trackingDone) break;
      OLEDD_TrackStateChange(trackerInfo);
      break;
    }
    case WM_DESTROY:
    {
      KillTimer(hwnd, DRAG_TIMER_ID);
      break;
    }
  }

  /*
   * This is a window proc after all. Let's call the default.
   */
  return DefWindowProcW (hwnd, uMsg, wParam, lParam);
}

static void drag_enter( TrackerWindowInfo *info, HWND new_target )
{
    HRESULT hr;

    info->curTargetHWND = new_target;

    while (new_target && !is_droptarget( new_target ))
        new_target = GetParent( new_target );

    info->curDragTarget = get_droptarget_pointer( new_target );

    if (info->curDragTarget)
    {
        *info->pdwEffect = info->dwOKEffect;
        hr = IDropTarget_DragEnter( info->curDragTarget, info->dataObject,
                                    info->dwKeyState, info->curMousePos,
                                    info->pdwEffect );
        *info->pdwEffect &= info->dwOKEffect;

        /* failed DragEnter() means invalid target */
        if (hr != S_OK)
        {
            IDropTarget_Release( info->curDragTarget );
            info->curDragTarget = NULL;
            info->curTargetHWND = NULL;
        }
    }
}

static void drag_end( TrackerWindowInfo *info )
{
    HRESULT hr;

    info->trackingDone = TRUE;
    ReleaseCapture();

    if (info->curDragTarget)
    {
        if (info->returnValue == DRAGDROP_S_DROP &&
            *info->pdwEffect != DROPEFFECT_NONE)
        {
            *info->pdwEffect = info->dwOKEffect;
            hr = IDropTarget_Drop( info->curDragTarget, info->dataObject, info->dwKeyState,
                                   info->curMousePos, info->pdwEffect );
            *info->pdwEffect &= info->dwOKEffect;

            if (FAILED( hr ))
                info->returnValue = hr;
        }
        else
        {
            IDropTarget_DragLeave( info->curDragTarget );
            *info->pdwEffect = DROPEFFECT_NONE;
        }
        IDropTarget_Release( info->curDragTarget );
        info->curDragTarget = NULL;
    }
    else
        *info->pdwEffect = DROPEFFECT_NONE;
}

static HRESULT give_feedback( TrackerWindowInfo *info )
{
    HRESULT hr;
    int res;
    HCURSOR cur;

    if (info->curDragTarget == NULL)
        *info->pdwEffect = DROPEFFECT_NONE;

    hr = IDropSource_GiveFeedback( info->dropSource, *info->pdwEffect );

    if (hr == DRAGDROP_S_USEDEFAULTCURSORS)
    {
        if (*info->pdwEffect & DROPEFFECT_MOVE)
            res = CURSOR_MOVE;
        else if (*info->pdwEffect & DROPEFFECT_COPY)
            res = CURSOR_COPY;
        else if (*info->pdwEffect & DROPEFFECT_LINK)
            res = CURSOR_LINK;
        else
            res = CURSOR_NODROP;

        cur = LoadCursorW( hProxyDll, MAKEINTRESOURCEW( res ) );
        SetCursor( cur );
    }

    return hr;
}

/***
 * OLEDD_TrackStateChange()
 *
 * This method is invoked while a drag and drop operation is in effect.
 *
 * params:
 *    trackerInfo - Pointer to the structure identifying the
 *                  drag & drop operation that is currently
 *                  active.
 */
static void OLEDD_TrackStateChange(TrackerWindowInfo* trackerInfo)
{
  HWND   hwndNewTarget = 0;
  POINT pt;

  /*
   * This method may be called from QueryContinueDrag again,
   * (i.e. by running message loop) so avoid recursive call chain.
   */
  if (trackerInfo->inTrackCall) return;
  trackerInfo->inTrackCall = TRUE;

  /*
   * Get the handle of the window under the mouse
   */
  pt.x = trackerInfo->curMousePos.x;
  pt.y = trackerInfo->curMousePos.y;
  hwndNewTarget = WindowFromPoint(pt);

  trackerInfo->returnValue = IDropSource_QueryContinueDrag(trackerInfo->dropSource,
                                                           trackerInfo->escPressed,
                                                           trackerInfo->dwKeyState);

  if (trackerInfo->curTargetHWND != hwndNewTarget &&
      (trackerInfo->returnValue == S_OK ||
       trackerInfo->returnValue == DRAGDROP_S_DROP))
  {
    if (trackerInfo->curDragTarget)
    {
      IDropTarget_DragLeave(trackerInfo->curDragTarget);
      IDropTarget_Release(trackerInfo->curDragTarget);
      trackerInfo->curDragTarget = NULL;
      trackerInfo->curTargetHWND = NULL;
    }

    if (hwndNewTarget)
      drag_enter( trackerInfo, hwndNewTarget );

    give_feedback( trackerInfo );

  }

  if (trackerInfo->returnValue == S_OK)
  {
    if (trackerInfo->curDragTarget)
    {
      *trackerInfo->pdwEffect = trackerInfo->dwOKEffect;
      IDropTarget_DragOver(trackerInfo->curDragTarget,
                           trackerInfo->dwKeyState,
                           trackerInfo->curMousePos,
                           trackerInfo->pdwEffect);
      *trackerInfo->pdwEffect &= trackerInfo->dwOKEffect;
    }
    give_feedback( trackerInfo );
  }
  else
    drag_end( trackerInfo );

  trackerInfo->inTrackCall = FALSE;
}

/***
 * OLEDD_GetButtonState()
 *
 * This method will use the current state of the keyboard to build
 * a button state mask equivalent to the one passed in the
 * WM_MOUSEMOVE wParam.
 */
static DWORD OLEDD_GetButtonState(void)
{
  BYTE  keyboardState[256];
  DWORD keyMask = 0;

  GetKeyboardState(keyboardState);

  if ( (keyboardState[VK_SHIFT] & 0x80) !=0)
    keyMask |= MK_SHIFT;

  if ( (keyboardState[VK_CONTROL] & 0x80) !=0)
    keyMask |= MK_CONTROL;

  if ( (keyboardState[VK_MENU] & 0x80) !=0)
    keyMask |= MK_ALT;

  if ( (keyboardState[VK_LBUTTON] & 0x80) !=0)
    keyMask |= MK_LBUTTON;

  if ( (keyboardState[VK_RBUTTON] & 0x80) !=0)
    keyMask |= MK_RBUTTON;

  if ( (keyboardState[VK_MBUTTON] & 0x80) !=0)
    keyMask |= MK_MBUTTON;

  return keyMask;
}

/***
 * OLEDD_GetButtonState()
 *
 * This method will read the default value of the registry key in
 * parameter and extract a DWORD value from it. The registry key value
 * can be in a string key or a DWORD key.
 *
 * params:
 *     regKey   - Key to read the default value from
 *     pdwValue - Pointer to the location where the DWORD
 *                value is returned. This value is not modified
 *                if the value is not found.
 */

static void OLEUTL_ReadRegistryDWORDValue(
  HKEY   regKey,
  DWORD* pdwValue)
{
  WCHAR buffer[20];
  DWORD cbData = sizeof(buffer);
  DWORD dwKeyType;
  LONG  lres;

  lres = RegQueryValueExW(regKey, L"", NULL, &dwKeyType, (BYTE *)buffer, &cbData);

  if (lres==ERROR_SUCCESS)
  {
    switch (dwKeyType)
    {
      case REG_DWORD:
	*pdwValue = *(DWORD*)buffer;
	break;
      case REG_EXPAND_SZ:
      case REG_MULTI_SZ:
      case REG_SZ:
	*pdwValue = wcstoul(buffer, NULL, 10);
	break;
    }
  }
}

/******************************************************************************
 * OleDraw (OLE32.@)
 *
 * The operation of this function is documented literally in the WinAPI
 * documentation to involve a QueryInterface for the IViewObject interface,
 * followed by a call to IViewObject::Draw.
 */
HRESULT WINAPI OleDraw(
	IUnknown *pUnk,
	DWORD dwAspect,
	HDC hdcDraw,
	LPCRECT rect)
{
  HRESULT hres;
  IViewObject *viewobject;

  if (!pUnk) return E_INVALIDARG;

  hres = IUnknown_QueryInterface(pUnk,
				 &IID_IViewObject,
				 (void**)&viewobject);
  if (SUCCEEDED(hres))
  {
    hres = IViewObject_Draw(viewobject, dwAspect, -1, 0, 0, 0, hdcDraw, (RECTL*)rect, 0, 0, 0);
    IViewObject_Release(viewobject);
    return hres;
  }
  else
    return DV_E_NOIVIEWOBJECT;
}

/***********************************************************************
 *             OleTranslateAccelerator [OLE32.@]
 */
HRESULT WINAPI OleTranslateAccelerator (LPOLEINPLACEFRAME lpFrame,
                   LPOLEINPLACEFRAMEINFO lpFrameInfo, LPMSG lpmsg)
{
    WORD wID;

    TRACE("(%p,%p,%p)\n", lpFrame, lpFrameInfo, lpmsg);

    if (IsAccelerator(lpFrameInfo->haccel,lpFrameInfo->cAccelEntries,lpmsg,&wID))
        return IOleInPlaceFrame_TranslateAccelerator(lpFrame,lpmsg,wID);

    return S_FALSE;
}

/******************************************************************************
 *              OleCreate        [OLE32.@]
 *
 */
HRESULT WINAPI OleCreate(
	REFCLSID rclsid,
	REFIID riid,
	DWORD renderopt,
	LPFORMATETC pFormatEtc,
	LPOLECLIENTSITE pClientSite,
	LPSTORAGE pStg,
	LPVOID* ppvObj)
{
    HRESULT hres;
    IUnknown * pUnk = NULL;
    IOleObject *pOleObject = NULL;

    TRACE("%s, %s, %ld, %p, %p, %p, %p.\n", debugstr_guid(rclsid),
        debugstr_guid(riid), renderopt, pFormatEtc, pClientSite, pStg, ppvObj);

    hres = CoCreateInstance(rclsid, 0, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER, riid, (LPVOID*)&pUnk);

    if (SUCCEEDED(hres))
        hres = IStorage_SetClass(pStg, rclsid);

    if (pClientSite && SUCCEEDED(hres))
    {
        hres = IUnknown_QueryInterface(pUnk, &IID_IOleObject, (LPVOID*)&pOleObject);
        if (SUCCEEDED(hres))
        {
            DWORD dwStatus;
            IOleObject_GetMiscStatus(pOleObject, DVASPECT_CONTENT, &dwStatus);
        }
    }

    if (SUCCEEDED(hres))
    {
        IPersistStorage * pPS;
        if (SUCCEEDED((hres = IUnknown_QueryInterface(pUnk, &IID_IPersistStorage, (LPVOID*)&pPS))))
        {
            TRACE("trying to set stg %p\n", pStg);
            hres = IPersistStorage_InitNew(pPS, pStg);
            TRACE("-- result %#lx\n", hres);
            IPersistStorage_Release(pPS);
        }
    }

    if (pClientSite && SUCCEEDED(hres))
    {
        TRACE("trying to set clientsite %p\n", pClientSite);
        hres = IOleObject_SetClientSite(pOleObject, pClientSite);
        TRACE("-- result %#lx\n", hres);
    }

    if (pOleObject)
        IOleObject_Release(pOleObject);

    if (((renderopt == OLERENDER_DRAW) || (renderopt == OLERENDER_FORMAT)) &&
        SUCCEEDED(hres))
    {
        hres = OleRun(pUnk);
        if (SUCCEEDED(hres))
        {
            IOleCache *pOleCache;

            if (SUCCEEDED(IUnknown_QueryInterface(pUnk, &IID_IOleCache, (void **)&pOleCache)))
            {
                DWORD dwConnection;
                if (renderopt == OLERENDER_DRAW && !pFormatEtc) {
                    FORMATETC pfe;
                    pfe.cfFormat = 0;
                    pfe.ptd = NULL;
                    pfe.dwAspect = DVASPECT_CONTENT;
                    pfe.lindex = -1;
                    pfe.tymed = TYMED_NULL;
                    hres = IOleCache_Cache(pOleCache, &pfe, ADVF_PRIMEFIRST, &dwConnection);
                }
                else
                    hres = IOleCache_Cache(pOleCache, pFormatEtc, ADVF_PRIMEFIRST, &dwConnection);
                IOleCache_Release(pOleCache);
            }
        }
    }

    if (FAILED(hres) && pUnk)
    {
        IUnknown_Release(pUnk);
        pUnk = NULL;
    }

    *ppvObj = pUnk;

    TRACE("-- %p\n", pUnk);
    return hres;
}

/******************************************************************************
 *              OleGetAutoConvert        [OLE32.@]
 */
HRESULT WINAPI OleGetAutoConvert(REFCLSID clsidOld, LPCLSID pClsidNew)
{
    HKEY hkey = NULL;
    WCHAR buf[CHARS_IN_GUID];
    LONG len;
    HRESULT res = S_OK;

    res = COM_OpenKeyForCLSID(clsidOld, L"AutoConvertTo", KEY_READ, &hkey);
    if (FAILED(res))
        goto done;

    len = sizeof(buf);
    if (RegQueryValueW(hkey, NULL, buf, &len))
    {
        res = REGDB_E_KEYMISSING;
        goto done;
    }
    res = CLSIDFromString(buf, pClsidNew);
done:
    if (hkey) RegCloseKey(hkey);
    return res;
}

/******************************************************************************
 *              OleSetAutoConvert        [OLE32.@]
 */
HRESULT WINAPI OleSetAutoConvert(REFCLSID clsidOld, REFCLSID clsidNew)
{
    HKEY hkey = NULL;
    WCHAR szClsidNew[CHARS_IN_GUID];
    HRESULT res = S_OK;

    TRACE("(%s,%s)\n", debugstr_guid(clsidOld), debugstr_guid(clsidNew));
    
    res = COM_OpenKeyForCLSID(clsidOld, NULL, KEY_READ | KEY_WRITE, &hkey);
    if (FAILED(res))
        goto done;
    StringFromGUID2(clsidNew, szClsidNew, CHARS_IN_GUID);
    if (RegSetValueW(hkey, L"AutoConvertTo", REG_SZ, szClsidNew, (lstrlenW(szClsidNew)+1) * sizeof(WCHAR)))
    {
        res = REGDB_E_WRITEREGDB;
	goto done;
    }

done:
    if (hkey) RegCloseKey(hkey);
    return res;
}

/******************************************************************************
 *              OleDoAutoConvert        [OLE32.@]
 */
HRESULT WINAPI OleDoAutoConvert(LPSTORAGE pStg, LPCLSID pClsidNew)
{
    WCHAR *user_type_old, *user_type_new;
    CLIPFORMAT cf;
    STATSTG stat;
    CLSID clsid;
    HRESULT hr;

    TRACE("(%p, %p)\n", pStg, pClsidNew);

    *pClsidNew = CLSID_NULL;
    if(!pStg)
        return E_INVALIDARG;
    hr = IStorage_Stat(pStg, &stat, STATFLAG_NONAME);
    if(FAILED(hr))
        return hr;

    *pClsidNew = stat.clsid;
    hr = OleGetAutoConvert(&stat.clsid, &clsid);
    if(FAILED(hr))
        return hr;

    hr = IStorage_SetClass(pStg, &clsid);
    if(FAILED(hr))
        return hr;

    hr = ReadFmtUserTypeStg(pStg, &cf, &user_type_old);
    if(FAILED(hr)) {
        cf = 0;
        user_type_new = NULL;
    }

    hr = OleRegGetUserType(&clsid, USERCLASSTYPE_FULL, &user_type_new);
    if(FAILED(hr))
        user_type_new = NULL;

    hr = WriteFmtUserTypeStg(pStg, cf, user_type_new);
    CoTaskMemFree(user_type_new);
    if(FAILED(hr))
    {
        CoTaskMemFree(user_type_old);
        IStorage_SetClass(pStg, &stat.clsid);
        return hr;
    }

    hr = SetConvertStg(pStg, TRUE);
    if(FAILED(hr))
    {
        WriteFmtUserTypeStg(pStg, cf, user_type_old);
        IStorage_SetClass(pStg, &stat.clsid);
    }
    else
        *pClsidNew = clsid;
    CoTaskMemFree(user_type_old);
    return hr;
}

/******************************************************************************
 *              OleIsRunning        [OLE32.@]
 */
BOOL WINAPI OleIsRunning(LPOLEOBJECT object)
{
    IRunnableObject *pRunnable;
    HRESULT hr;
    BOOL running;

    TRACE("(%p)\n", object);

    if (!object) return FALSE;

    hr = IOleObject_QueryInterface(object, &IID_IRunnableObject, (void **)&pRunnable);
    if (FAILED(hr))
        return TRUE;
    running = IRunnableObject_IsRunning(pRunnable);
    IRunnableObject_Release(pRunnable);
    return running;
}

/***********************************************************************
 *           OleNoteObjectVisible			    [OLE32.@]
 */
HRESULT WINAPI OleNoteObjectVisible(LPUNKNOWN pUnknown, BOOL bVisible)
{
    TRACE("(%p, %s)\n", pUnknown, bVisible ? "TRUE" : "FALSE");
    return CoLockObjectExternal(pUnknown, bVisible, TRUE);
}

/***********************************************************************
 *           PropSysAllocString			    [OLE32.@]
 * NOTES
 *  Forward to oleaut32.
 */
BSTR WINAPI PropSysAllocString(LPCOLESTR str)
{
    return SysAllocString(str);
}

/***********************************************************************
 *           PropSysFreeString			    [OLE32.@]
 * NOTES
 *  Forward to oleaut32.
 */
void WINAPI PropSysFreeString(LPOLESTR str)
{
    SysFreeString(str);
}

/******************************************************************************
 *           OleQueryCreateFromData [OLE32.@]
 *
 * Checks whether an object can become an embedded object.
 * the clipboard or OLE drag and drop.
 * Returns  : S_OK - Format that supports Embedded object creation are present.
 *            OLE_E_STATIC - Format that supports static object creation are present.
 *            S_FALSE - No acceptable format is available.
 */

HRESULT WINAPI OleQueryCreateFromData(IDataObject *data)
{
    IEnumFORMATETC *enum_fmt;
    FORMATETC fmt;
    BOOL found_static = FALSE;
    HRESULT hr;

    hr = IDataObject_EnumFormatEtc(data, DATADIR_GET, &enum_fmt);

    if(FAILED(hr)) return hr;

    do
    {
        hr = IEnumFORMATETC_Next(enum_fmt, 1, &fmt, NULL);
        if(hr == S_OK)
        {
            if(fmt.cfFormat == embedded_object_clipboard_format ||
               fmt.cfFormat == embed_source_clipboard_format ||
               fmt.cfFormat == filename_clipboard_format)
            {
                IEnumFORMATETC_Release(enum_fmt);
                return S_OK;
            }

            if(fmt.cfFormat == CF_METAFILEPICT ||
               fmt.cfFormat == CF_BITMAP ||
               fmt.cfFormat == CF_DIB)
                found_static = TRUE;
        }
    } while (hr == S_OK);

    IEnumFORMATETC_Release(enum_fmt);

    return found_static ? OLE_S_STATIC : S_FALSE;
}

static inline void init_fmtetc(FORMATETC *fmt, CLIPFORMAT cf, TYMED tymed)
{
    fmt->cfFormat = cf;
    fmt->ptd = NULL;
    fmt->dwAspect = DVASPECT_CONTENT;
    fmt->lindex = -1;
    fmt->tymed = tymed;
}

/***************************************************************************
 *         get_storage
 *
 * Retrieve an object's storage from a variety of sources.
 *
 * FIXME: CF_FILENAME.
 */
static HRESULT get_storage(IDataObject *data, IStorage *stg, UINT *src_cf, BOOL other_fmts)
{
    static const UINT fmt_id[] = { CF_METAFILEPICT, CF_BITMAP, CF_DIB };
    UINT i;
    HRESULT hr;
    FORMATETC fmt;
    STGMEDIUM med;
    IPersistStorage *persist;
    CLSID clsid;

    if (src_cf) *src_cf = 0;

    /* CF_EMBEDEDOBJECT */
    init_fmtetc(&fmt, embedded_object_clipboard_format, TYMED_ISTORAGE);
    med.tymed = TYMED_ISTORAGE;
    med.pstg = stg;
    med.pUnkForRelease = NULL;
    hr = IDataObject_GetDataHere(data, &fmt, &med);
    if(SUCCEEDED(hr))
    {
        if (src_cf) *src_cf = embedded_object_clipboard_format;
        return hr;
    }

    /* CF_EMBEDSOURCE */
    init_fmtetc(&fmt, embed_source_clipboard_format, TYMED_ISTORAGE);
    med.tymed = TYMED_ISTORAGE;
    med.pstg = stg;
    med.pUnkForRelease = NULL;
    hr = IDataObject_GetDataHere(data, &fmt, &med);
    if(SUCCEEDED(hr))
    {
        if (src_cf) *src_cf = embed_source_clipboard_format;
        return hr;
    }

    if (other_fmts)
    {
        for (i = 0; i < ARRAY_SIZE(fmt_id); i++)
        {
            init_fmtetc(&fmt, fmt_id[i], TYMED_ISTORAGE);
            hr = IDataObject_QueryGetData(data, &fmt);
            if (SUCCEEDED(hr))
            {
                if (src_cf) *src_cf = fmt_id[i];
                return hr;
            }
        }
    }

    /* IPersistStorage */
    hr = IDataObject_QueryInterface(data, &IID_IPersistStorage, (void**)&persist);
    if(FAILED(hr)) return hr;

    hr = IPersistStorage_GetClassID(persist, &clsid);
    if(FAILED(hr)) goto end;

    hr = IStorage_SetClass(stg, &clsid);
    if(FAILED(hr)) goto end;

    hr = IPersistStorage_Save(persist, stg, FALSE);
    if(FAILED(hr)) goto end;

    hr = IPersistStorage_SaveCompleted(persist, NULL);

end:
    IPersistStorage_Release(persist);

    return hr;
}

/******************************************************************************
 *            OleCreateFromDataEx        [OLE32.@]
 *
 * Creates an embedded object from data transfer object retrieved from
 * the clipboard or OLE drag and drop.
 */
HRESULT WINAPI OleCreateFromDataEx(IDataObject *data, REFIID iid, DWORD flags,
                                   DWORD renderopt, ULONG num_cache_fmts, DWORD *adv_flags, FORMATETC *cache_fmts,
                                   IAdviseSink *sink, DWORD *conns,
                                   IOleClientSite *client_site, IStorage *stg, void **obj)
{
    HRESULT hr;
    UINT src_cf;

    FIXME("%p, %s, %#lx, %#lx, %ld, %p, %p, %p, %p, %p, %p, %p: stub\n",
          data, debugstr_guid(iid), flags, renderopt, num_cache_fmts, adv_flags, cache_fmts,
          sink, conns, client_site, stg, obj);

    hr = get_storage(data, stg, &src_cf, TRUE);
    if(FAILED(hr)) return hr;

    hr = OleLoad(stg, iid, client_site, obj);
    if(FAILED(hr)) return hr;

    /* FIXME: Init cache */

    return hr;
}

/******************************************************************************
 *            OleCreateFromData        [OLE32.@]
 */
HRESULT WINAPI OleCreateFromData(IDataObject *data, REFIID iid, DWORD renderopt, FORMATETC *fmt,
        IOleClientSite *client_site, IStorage *stg, void **obj)
{
    DWORD advf = ADVF_PRIMEFIRST;

    return OleCreateFromDataEx(data, iid, 0, renderopt, fmt ? 1 : 0, fmt ? &advf : NULL,
                               fmt, NULL, NULL, client_site, stg, obj);
}

/******************************************************************************
 *              OleCreateLinkFromData        [OLE32.@]
 */
HRESULT WINAPI OleCreateLinkFromData(IDataObject *data, REFIID iid, DWORD renderopt, FORMATETC *fmt,
        IOleClientSite *client_site, IStorage *stg, void **obj)
{
    FIXME("%p, %s, %#lx, %p, %p, %p, %p: semi-stub\n",
          data, debugstr_guid(iid), renderopt, fmt, client_site, stg, obj);
    return OleCreateFromData(data, iid, renderopt, fmt, client_site, stg, obj);
}

/******************************************************************************
 *              OleCreateStaticFromData        [OLE32.@]
 */
HRESULT WINAPI OleCreateStaticFromData(IDataObject *data, REFIID iid, DWORD renderopt, FORMATETC *fmt,
        IOleClientSite *client_site, IStorage *stg, void **obj)
{
    HRESULT hr;
    CLSID clsid;
    IOleObject * ole_object = NULL;
    IOleCache2 *ole_cache = NULL;
    IPersistStorage *persist = NULL;
    DWORD connection;
    STGMEDIUM stgmedium;
    LPOLESTR ole_typename;

    TRACE("%p, %s, %#lx, %p, %p, %p, %p.\n", data, debugstr_guid(iid), renderopt, fmt, client_site, stg, obj);

    if (!obj || !stg)
        return E_INVALIDARG;

    if (renderopt != OLERENDER_FORMAT)
    {
        FIXME("semi-stub\n");
        return OleCreateFromData(data, iid, renderopt, fmt, client_site, stg, obj);
    }

    if (!fmt)
        return E_INVALIDARG;

    hr = IDataObject_GetData(data, fmt, &stgmedium);
    if (FAILED(hr)) return hr;

    switch (fmt->cfFormat)
    {
        case CF_BITMAP:
        case CF_DIB:
            clsid = CLSID_Picture_Dib;
            break;
        case CF_ENHMETAFILE:
            clsid = CLSID_Picture_EnhMetafile;
            break;
        case CF_METAFILEPICT:
            clsid = CLSID_Picture_Metafile;
            break;
        default:
            ReleaseStgMedium(&stgmedium);
            return DV_E_CLIPFORMAT;
    }
    hr = OleCreateDefaultHandler(&clsid, NULL, &IID_IOleObject, (void **)&ole_object);
    if (FAILED(hr)) goto end;

    if (client_site)
    {
        hr = IOleObject_SetClientSite(ole_object, client_site);
        if (FAILED(hr)) goto end;
    }

    hr = IOleObject_QueryInterface(ole_object, &IID_IOleCache2, (void **)&ole_cache);
    if (FAILED(hr)) goto end;

    hr = IOleObject_QueryInterface(ole_object, &IID_IPersistStorage, (void **)&persist);
    if (FAILED(hr)) goto end;

    hr = WriteClassStg(stg, &clsid);
    if (FAILED(hr)) goto end;

    hr = IPersistStorage_InitNew(persist, stg);
    if (FAILED(hr)) goto end;

    hr = IOleCache2_Cache(ole_cache, fmt, ADVF_PRIMEFIRST, &connection);
    if (FAILED(hr)) goto end;

    hr = IOleCache2_SetData(ole_cache, fmt, &stgmedium, TRUE);
    if (FAILED(hr)) goto end;
    stgmedium.tymed = TYMED_NULL;

    hr = IOleObject_GetUserType(ole_object, USERCLASSTYPE_FULL, &ole_typename);
    if(FAILED(hr))
        ole_typename = NULL;
    hr = WriteFmtUserTypeStg(stg, fmt->cfFormat, ole_typename);
    CoTaskMemFree(ole_typename);
    if (FAILED(hr)) goto end;

    hr = IPersistStorage_Save(persist, stg, TRUE);
    if (FAILED(hr)) goto end;

    hr = IPersistStorage_SaveCompleted(persist, NULL);
    if (FAILED(hr)) goto end;

    hr = IOleObject_QueryInterface(ole_object, iid, obj);

end:
    if (stgmedium.tymed == TYMED_NULL)
        ReleaseStgMedium(&stgmedium);
    if (persist)
        IPersistStorage_Release(persist);
    if (ole_cache)
        IOleCache2_Release(ole_cache);
    if (ole_object)
        IOleObject_Release(ole_object);
    return hr;
}

/******************************************************************************
 *              OleCreateFromFileEx        [OLE32.@]
 */
HRESULT WINAPI OleCreateFromFileEx(REFCLSID clsid, const OLECHAR *filename, REFIID iid, DWORD flags,
        DWORD renderopt, ULONG num_fmts, DWORD *adv_flags, FORMATETC *fmts, IAdviseSink *sink,
        DWORD *conns, IOleClientSite *client_site, IStorage *stg, void **obj)
{
    HRESULT hr;
    IMoniker *mon;
    IDataObject *data;
    IUnknown *unk = NULL;
    IOleCache *cache = NULL;
    ULONG i;

    TRACE("cls %s, %s, iid %s, flags %ld, render opts %ld, num fmts %ld, adv flags %p, fmts %p\n", debugstr_guid(clsid),
          debugstr_w(filename), debugstr_guid(iid), flags, renderopt, num_fmts, adv_flags, fmts);
    TRACE("sink %p, conns %p, client site %p, storage %p, obj %p\n", sink, conns, client_site, stg, obj);
    for (i = 0; i < num_fmts; i++)
        TRACE("\t%ld: fmt %s adv flags %ld\n", i, debugstr_formatetc(fmts + i), adv_flags[i]);

    hr = CreateFileMoniker( filename, &mon );
    if (FAILED(hr)) return hr;

    hr = BindMoniker( mon, 0, &IID_IDataObject, (void**)&data );
    IMoniker_Release( mon );
    if (FAILED(hr)) return hr;

    hr = get_storage( data, stg, NULL, FALSE );
    if (FAILED(hr)) goto end;

    hr = OleLoad( stg, &IID_IUnknown, client_site, (void**)&unk );
    if (FAILED(hr)) goto end;

    if (renderopt == OLERENDER_FORMAT)
    {
        hr = IUnknown_QueryInterface( unk, &IID_IOleCache, (void**)&cache );
        if (FAILED(hr)) goto end;

        for (i = 0; i < num_fmts; i++)
        {
            STGMEDIUM med;
            DWORD dummy_conn;

            memset( &med, 0, sizeof(med) );
            hr = IDataObject_GetData( data, fmts + i, &med );
            if (FAILED(hr)) goto end;
            hr = IOleCache_Cache( cache, fmts + i, adv_flags[i], &dummy_conn );
            if (SUCCEEDED(hr))
                hr = IOleCache_SetData( cache, fmts + i, &med, TRUE );
            if (FAILED(hr))
            {
                ReleaseStgMedium( &med );
                goto end;
            }
        }
    }

    hr = IUnknown_QueryInterface( unk, iid, obj );

end:
    if (cache) IOleCache_Release( cache );
    if (unk) IUnknown_Release( unk );
    IDataObject_Release( data );
    return hr;
}

/******************************************************************************
 *              OleCreateFromFile        [OLE32.@]
 */
HRESULT WINAPI OleCreateFromFile(REFCLSID clsid, const OLECHAR *filename, REFIID iid, DWORD renderopt,
        FORMATETC *fmt, IOleClientSite *client_site, IStorage *storage, void **obj)
{
    DWORD advf = ADVF_PRIMEFIRST;

    return OleCreateFromFileEx(clsid, filename, iid, 0, renderopt, fmt ? 1 : 0, fmt ? &advf : NULL, fmt,
            NULL, NULL, client_site, storage, obj);
}

/******************************************************************************
 *              OleDuplicateData        [OLE32.@]
 */
HANDLE WINAPI OleDuplicateData(HANDLE hSrc, CLIPFORMAT cfFormat, UINT uiFlags)
{
    HANDLE hDst = NULL;

    TRACE("(%p,%x,%x)\n", hSrc, cfFormat, uiFlags);

    if (!uiFlags) uiFlags = GMEM_MOVEABLE;

    switch (cfFormat)
    {
    case CF_ENHMETAFILE:
        hDst = CopyEnhMetaFileW(hSrc, NULL);
        break;
    case CF_METAFILEPICT:
        hDst = CopyMetaFileW(hSrc, NULL);
        break;
    case CF_PALETTE:
        {
            LOGPALETTE * logpalette;
            UINT nEntries = GetPaletteEntries(hSrc, 0, 0, NULL);
            if (!nEntries) return NULL;
            logpalette = HeapAlloc(GetProcessHeap(), 0,
                FIELD_OFFSET(LOGPALETTE, palPalEntry[nEntries]));
            if (!logpalette) return NULL;
            if (!GetPaletteEntries(hSrc, 0, nEntries, logpalette->palPalEntry))
            {
                HeapFree(GetProcessHeap(), 0, logpalette);
                return NULL;
            }
            logpalette->palVersion = 0x300;
            logpalette->palNumEntries = (WORD)nEntries;

            hDst = CreatePalette(logpalette);

            HeapFree(GetProcessHeap(), 0, logpalette);
            break;
        }
    case CF_BITMAP:
        {
            LONG size;
            BITMAP bm;
            if (!GetObjectW(hSrc, sizeof(bm), &bm))
                return NULL;
            size = GetBitmapBits(hSrc, 0, NULL);
            if (!size) return NULL;
            bm.bmBits = HeapAlloc(GetProcessHeap(), 0, size);
            if (!bm.bmBits) return NULL;
            if (GetBitmapBits(hSrc, size, bm.bmBits))
                hDst = CreateBitmapIndirect(&bm);
            HeapFree(GetProcessHeap(), 0, bm.bmBits);
            break;
        }
    default:
        {
            SIZE_T size = GlobalSize(hSrc);
            LPVOID pvSrc = NULL;
            LPVOID pvDst = NULL;

            /* allocate space for object */
            if (!size) return NULL;
            hDst = GlobalAlloc(uiFlags, size);
            if (!hDst) return NULL;

            /* lock pointers */
            pvSrc = GlobalLock(hSrc);
            if (!pvSrc)
            {
                GlobalFree(hDst);
                return NULL;
            }
            pvDst = GlobalLock(hDst);
            if (!pvDst)
            {
                GlobalUnlock(hSrc);
                GlobalFree(hDst);
                return NULL;
            }
            /* copy data */
            memcpy(pvDst, pvSrc, size);

            /* cleanup */
            GlobalUnlock(hDst);
            GlobalUnlock(hSrc);
        }
    }

    TRACE("returning %p\n", hDst);
    return hDst;
}
