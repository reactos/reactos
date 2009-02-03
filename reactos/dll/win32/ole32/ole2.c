/*
 *	OLE2 library
 *
 * Copyright 1995 Martin von Loewis
 * Copyright 1999 Francis Beaudet
 * Copyright 1999 Noel Borthwick
 * Copyright 1999, 2000 Marcus Meissner
 * Copyright 2005 Juan Lang
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

#include "config.h"

#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "winreg.h"
#include "ole2.h"
#include "ole2ver.h"

#include "wine/unicode.h"
#include "compobj_private.h"
#include "wine/list.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);
WINE_DECLARE_DEBUG_CHANNEL(accel);

/******************************************************************************
 * These are static/global variables and internal data structures that the
 * OLE module uses to maintain it's state.
 */
typedef struct tagDropTargetNode
{
  HWND          hwndTarget;
  IDropTarget*  dropTarget;
  struct list   entry;
} DropTargetNode;

typedef struct tagTrackerWindowInfo
{
  IDataObject* dataObject;
  IDropSource* dropSource;
  DWORD        dwOKEffect;
  DWORD*       pdwEffect;
  BOOL       trackingDone;
  HRESULT      returnValue;

  BOOL       escPressed;
  HWND       curTargetHWND;	/* window the mouse is hovering over */
  HWND       curDragTargetHWND; /* might be a ancestor of curTargetHWND */
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
static const char OLEDD_DRAGTRACKERCLASS[] = "WineDragDropTracker32";

/*
 * This is the head of the Drop target container.
 */
static struct list targetListHead = LIST_INIT(targetListHead);

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
 * These are the prototypes of the OLE Clipboard initialization methods (in clipboard.c)
 */
extern void OLEClipbrd_UnInitialize(void);
extern void OLEClipbrd_Initialize(void);

/******************************************************************************
 * These are the prototypes of the utility methods used for OLE Drag n Drop
 */
static void            OLEDD_Initialize(void);
static DropTargetNode* OLEDD_FindDropTarget(
                         HWND hwndOfTarget);
static void            OLEDD_FreeDropTarget(DropTargetNode*, BOOL);
static LRESULT WINAPI  OLEDD_DragTrackerWindowProc(
			 HWND   hwnd,
			 UINT   uMsg,
			 WPARAM wParam,
			 LPARAM   lParam);
static void OLEDD_TrackMouseMove(
                         TrackerWindowInfo* trackerInfo);
static void OLEDD_TrackStateChange(
                         TrackerWindowInfo* trackerInfo);
static DWORD OLEDD_GetButtonState(void);


/******************************************************************************
 *		OleBuildVersion	[OLE2.1]
 *		OleBuildVersion [OLE32.@]
 */
DWORD WINAPI OleBuildVersion(void)
{
    TRACE("Returning version %d, build %d.\n", rmm, rup);
    return (rmm<<16)+rup;
}

/***********************************************************************
 *           OleInitialize       (OLE2.2)
 *           OleInitialize       (OLE32.@)
 */
HRESULT WINAPI OleInitialize(LPVOID reserved)
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
     * OLE Clipboard
     */
    OLEClipbrd_Initialize();

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
 *		OleUninitialize	[OLE2.3]
 *		OleUninitialize	[OLE32.@]
 */
void WINAPI OleUninitialize(void)
{
  TRACE("()\n");

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
    OLEClipbrd_UnInitialize();

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
HRESULT WINAPI OleInitializeWOW(DWORD x, DWORD y) {
        FIXME("(0x%08x, 0x%08x),stub!\n",x, y);
        return 0;
}

/***********************************************************************
 *           RegisterDragDrop (OLE32.@)
 */
HRESULT WINAPI RegisterDragDrop(
	HWND hwnd,
	LPDROPTARGET pDropTarget)
{
  DropTargetNode* dropTargetInfo;

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

  /*
   * First, check if the window is already registered.
   */
  dropTargetInfo = OLEDD_FindDropTarget(hwnd);

  if (dropTargetInfo!=NULL)
    return DRAGDROP_E_ALREADYREGISTERED;

  /*
   * If it's not there, we can add it. We first create a node for it.
   */
  dropTargetInfo = HeapAlloc(GetProcessHeap(), 0, sizeof(DropTargetNode));

  if (dropTargetInfo==NULL)
    return E_OUTOFMEMORY;

  dropTargetInfo->hwndTarget     = hwnd;

  /*
   * Don't forget that this is an interface pointer, need to nail it down since
   * we keep a copy of it.
   */
  IDropTarget_AddRef(pDropTarget);
  dropTargetInfo->dropTarget  = pDropTarget;

  list_add_tail(&targetListHead, &dropTargetInfo->entry);

  return S_OK;
}

/***********************************************************************
 *           RevokeDragDrop (OLE32.@)
 */
HRESULT WINAPI RevokeDragDrop(
	HWND hwnd)
{
  DropTargetNode* dropTargetInfo;

  TRACE("(%p)\n", hwnd);

  if (!IsWindow(hwnd))
  {
    ERR("invalid hwnd %p\n", hwnd);
    return DRAGDROP_E_INVALIDHWND;
  }

  /*
   * First, check if the window is already registered.
   */
  dropTargetInfo = OLEDD_FindDropTarget(hwnd);

  /*
   * If it ain't in there, it's an error.
   */
  if (dropTargetInfo==NULL)
    return DRAGDROP_E_NOTREGISTERED;

  OLEDD_FreeDropTarget(dropTargetInfo, TRUE);

  return S_OK;
}

/***********************************************************************
 *           OleRegGetUserType (OLE32.@)
 *
 * This implementation of OleRegGetUserType ignores the dwFormOfType
 * parameter and always returns the full name of the object. This is
 * not too bad since this is the case for many objects because of the
 * way they are registered.
 */
HRESULT WINAPI OleRegGetUserType(
	REFCLSID clsid,
	DWORD dwFormOfType,
	LPOLESTR* pszUserType)
{
  char    keyName[60];
  DWORD   dwKeyType;
  DWORD   cbData;
  HKEY    clsidKey;
  LONG    hres;
  LPSTR   buffer;
  HRESULT retVal;
  /*
   * Initialize the out parameter.
   */
  *pszUserType = NULL;

  /*
   * Build the key name we're looking for
   */
  sprintf( keyName, "CLSID\\{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\\",
           clsid->Data1, clsid->Data2, clsid->Data3,
           clsid->Data4[0], clsid->Data4[1], clsid->Data4[2], clsid->Data4[3],
           clsid->Data4[4], clsid->Data4[5], clsid->Data4[6], clsid->Data4[7] );

  TRACE("(%s, %d, %p)\n", keyName, dwFormOfType, pszUserType);

  /*
   * Open the class id Key
   */
  hres = RegOpenKeyA(HKEY_CLASSES_ROOT,
		     keyName,
		     &clsidKey);

  if (hres != ERROR_SUCCESS)
    return REGDB_E_CLASSNOTREG;

  /*
   * Retrieve the size of the name string.
   */
  cbData = 0;

  hres = RegQueryValueExA(clsidKey,
			  "",
			  NULL,
			  &dwKeyType,
			  NULL,
			  &cbData);

  if (hres!=ERROR_SUCCESS)
  {
    RegCloseKey(clsidKey);
    return REGDB_E_READREGDB;
  }

  /*
   * Allocate a buffer for the registry value.
   */
  *pszUserType = CoTaskMemAlloc(cbData*2);

  if (*pszUserType==NULL)
  {
    RegCloseKey(clsidKey);
    return E_OUTOFMEMORY;
  }

  buffer = HeapAlloc(GetProcessHeap(), 0, cbData);

  if (buffer == NULL)
  {
    RegCloseKey(clsidKey);
    CoTaskMemFree(*pszUserType);
    *pszUserType=NULL;
    return E_OUTOFMEMORY;
  }

  hres = RegQueryValueExA(clsidKey,
			  "",
			  NULL,
			  &dwKeyType,
			  (LPBYTE) buffer,
			  &cbData);

  RegCloseKey(clsidKey);


  if (hres!=ERROR_SUCCESS)
  {
    CoTaskMemFree(*pszUserType);
    *pszUserType=NULL;

    retVal = REGDB_E_READREGDB;
  }
  else
  {
    MultiByteToWideChar( CP_ACP, 0, buffer, -1, *pszUserType, cbData /*FIXME*/ );
    retVal = S_OK;
  }
  HeapFree(GetProcessHeap(), 0, buffer);

  return retVal;
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

  TRACE("(DataObject %p, DropSource %p)\n", pDataObject, pDropSource);

  /*
   * Setup the drag n drop tracking window.
   */
  if (!IsValidInterface((LPUNKNOWN)pDropSource))
      return E_INVALIDARG;

  trackerInfo.dataObject        = pDataObject;
  trackerInfo.dropSource        = pDropSource;
  trackerInfo.dwOKEffect        = dwOKEffect;
  trackerInfo.pdwEffect         = pdwEffect;
  trackerInfo.trackingDone      = FALSE;
  trackerInfo.escPressed        = FALSE;
  trackerInfo.curDragTargetHWND = 0;
  trackerInfo.curTargetHWND     = 0;
  trackerInfo.curDragTarget     = 0;

  hwndTrackWindow = CreateWindowA(OLEDD_DRAGTRACKERCLASS,
				    "TrackerWindow",
				    WS_POPUP,
				    CW_USEDEFAULT, CW_USEDEFAULT,
				    CW_USEDEFAULT, CW_USEDEFAULT,
				    0,
				    0,
				    0,
				    (LPVOID)&trackerInfo);

  if (hwndTrackWindow!=0)
  {
    /*
     * Capture the mouse input
     */
    SetCapture(hwndTrackWindow);

    msg.message = 0;

    /*
     * Pump messages. All mouse input should go to the capture window.
     */
    while (!trackerInfo.trackingDone && GetMessageA(&msg, 0, 0, 0) )
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
	DispatchMessageA(&msg);
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
  return S_OK;
}

/***********************************************************************
 * OleRegGetMiscStatus [OLE32.@]
 */
HRESULT WINAPI OleRegGetMiscStatus(
  REFCLSID clsid,
  DWORD    dwAspect,
  DWORD*   pdwStatus)
{
  char    keyName[60];
  HKEY    clsidKey;
  HKEY    miscStatusKey;
  HKEY    aspectKey;
  LONG    result;

  /*
   * Initialize the out parameter.
   */
  *pdwStatus = 0;

  /*
   * Build the key name we're looking for
   */
  sprintf( keyName, "CLSID\\{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\\",
           clsid->Data1, clsid->Data2, clsid->Data3,
           clsid->Data4[0], clsid->Data4[1], clsid->Data4[2], clsid->Data4[3],
           clsid->Data4[4], clsid->Data4[5], clsid->Data4[6], clsid->Data4[7] );

  TRACE("(%s, %d, %p)\n", keyName, dwAspect, pdwStatus);

  /*
   * Open the class id Key
   */
  result = RegOpenKeyA(HKEY_CLASSES_ROOT,
		       keyName,
		       &clsidKey);

  if (result != ERROR_SUCCESS)
    return REGDB_E_CLASSNOTREG;

  /*
   * Get the MiscStatus
   */
  result = RegOpenKeyA(clsidKey,
		       "MiscStatus",
		       &miscStatusKey);


  if (result != ERROR_SUCCESS)
  {
    RegCloseKey(clsidKey);
    return REGDB_E_READREGDB;
  }

  /*
   * Read the default value
   */
  OLEUTL_ReadRegistryDWORDValue(miscStatusKey, pdwStatus);

  /*
   * Open the key specific to the requested aspect.
   */
  sprintf(keyName, "%d", dwAspect);

  result = RegOpenKeyA(miscStatusKey,
		       keyName,
		       &aspectKey);

  if (result == ERROR_SUCCESS)
  {
    OLEUTL_ReadRegistryDWORDValue(aspectKey, pdwStatus);
    RegCloseKey(aspectKey);
  }

  /*
   * Cleanup
   */
  RegCloseKey(miscStatusKey);
  RegCloseKey(clsidKey);

  return S_OK;
}

static HRESULT EnumOLEVERB_Construct(HKEY hkeyVerb, ULONG index, IEnumOLEVERB **ppenum);

typedef struct
{
    const IEnumOLEVERBVtbl *lpvtbl;
    LONG ref;

    HKEY hkeyVerb;
    ULONG index;
} EnumOLEVERB;

static HRESULT WINAPI EnumOLEVERB_QueryInterface(
    IEnumOLEVERB *iface, REFIID riid, void **ppv)
{
    TRACE("(%s, %p)\n", debugstr_guid(riid), ppv);
    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IEnumOLEVERB))
    {
        IUnknown_AddRef(iface);
        *ppv = iface;
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI EnumOLEVERB_AddRef(
    IEnumOLEVERB *iface)
{
    EnumOLEVERB *This = (EnumOLEVERB *)iface;
    TRACE("()\n");
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI EnumOLEVERB_Release(
    IEnumOLEVERB *iface)
{
    EnumOLEVERB *This = (EnumOLEVERB *)iface;
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
    EnumOLEVERB *This = (EnumOLEVERB *)iface;
    HRESULT hr = S_OK;

    TRACE("(%d, %p, %p)\n", celt, rgelt, pceltFetched);

    if (pceltFetched)
        *pceltFetched = 0;

    for (; celt; celt--, rgelt++)
    {
        WCHAR wszSubKey[20];
        LONG cbData;
        LPWSTR pwszOLEVERB;
        LPWSTR pwszMenuFlags;
        LPWSTR pwszAttribs;
        LONG res = RegEnumKeyW(This->hkeyVerb, This->index, wszSubKey, sizeof(wszSubKey)/sizeof(wszSubKey[0]));
        if (res == ERROR_NO_MORE_ITEMS)
        {
            hr = S_FALSE;
            break;
        }
        else if (res != ERROR_SUCCESS)
        {
            ERR("RegEnumKeyW failed with error %d\n", res);
            hr = REGDB_E_READREGDB;
            break;
        }
        res = RegQueryValueW(This->hkeyVerb, wszSubKey, NULL, &cbData);
        if (res != ERROR_SUCCESS)
        {
            ERR("RegQueryValueW failed with error %d\n", res);
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
            ERR("RegQueryValueW failed with error %d\n", res);
            hr = REGDB_E_READREGDB;
            CoTaskMemFree(pwszOLEVERB);
            break;
        }

        TRACE("verb string: %s\n", debugstr_w(pwszOLEVERB));
        pwszMenuFlags = strchrW(pwszOLEVERB, ',');
        if (!pwszMenuFlags)
        {
            hr = OLEOBJ_E_INVALIDVERB;
            CoTaskMemFree(pwszOLEVERB);
            break;
        }
        /* nul terminate the name string and advance to first character */
        *pwszMenuFlags = '\0';
        pwszMenuFlags++;
        pwszAttribs = strchrW(pwszMenuFlags, ',');
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
        rgelt->lVerb = atolW(wszSubKey);
        rgelt->lpszVerbName = pwszOLEVERB; /* user should free */
        rgelt->fuFlags = atolW(pwszMenuFlags);
        rgelt->grfAttribs = atolW(pwszAttribs);

        if (pceltFetched)
            (*pceltFetched)++;
        This->index++;
    }
    return hr;
}

static HRESULT WINAPI EnumOLEVERB_Skip(
    IEnumOLEVERB *iface, ULONG celt)
{
    EnumOLEVERB *This = (EnumOLEVERB *)iface;

    TRACE("(%d)\n", celt);

    This->index += celt;
    return S_OK;
}

static HRESULT WINAPI EnumOLEVERB_Reset(
    IEnumOLEVERB *iface)
{
    EnumOLEVERB *This = (EnumOLEVERB *)iface;

    TRACE("()\n");

    This->index = 0;
    return S_OK;
}

static HRESULT WINAPI EnumOLEVERB_Clone(
    IEnumOLEVERB *iface,
    IEnumOLEVERB **ppenum)
{
    EnumOLEVERB *This = (EnumOLEVERB *)iface;
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
    This->lpvtbl = &EnumOLEVERB_VTable;
    This->ref = 1;
    This->index = index;
    This->hkeyVerb = hkeyVerb;
    *ppenum = (IEnumOLEVERB *)&This->lpvtbl;
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
    static const WCHAR wszVerb[] = {'V','e','r','b',0};

    TRACE("(%s, %p)\n", debugstr_guid(clsid), ppenum);

    res = COM_OpenKeyForCLSID(clsid, wszVerb, KEY_READ, &hkeyVerb);
    if (FAILED(res))
    {
        if (res == REGDB_E_CLASSNOTREG)
            ERR("CLSID %s not registered\n", debugstr_guid(clsid));
        else if (res == REGDB_E_KEYMISSING)
            ERR("no Verbs key for class %s\n", debugstr_guid(clsid));
        else
            ERR("failed to open Verbs key for CLSID %s with error %d\n",
                debugstr_guid(clsid), res);
        return res;
    }

    res = RegQueryInfoKeyW(hkeyVerb, NULL, NULL, NULL, &dwSubKeys, NULL,
                          NULL, NULL, NULL, NULL, NULL, NULL);
    if (res != ERROR_SUCCESS)
    {
        ERR("failed to get subkey count with error %d\n", GetLastError());
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
HRESULT WINAPI OleRun(LPUNKNOWN pUnknown)
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

  if (SUCCEEDED(hres))
    /*
     * Initialize the object with it's IPersistStorage interface.
     */
    hres = IOleObject_QueryInterface(pUnk,
				     &IID_IPersistStorage,
				     (void**)&persistStorage);

  if (SUCCEEDED(hres))
  {
    hres = IPersistStorage_Load(persistStorage, pStg);

    IPersistStorage_Release(persistStorage);
    persistStorage = NULL;
  }

  if (SUCCEEDED(hres) && pClientSite)
    /*
     * Inform the new object of it's client site.
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
  else
    return E_INVALIDARG;
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
  OleMenuHookItem *pHookItem = NULL;

  /* Create an entry for the hook table */
  if ( !(pHookItem = HeapAlloc(GetProcessHeap(), 0,
                               sizeof(OleMenuHookItem)) ) )
    return FALSE;

  pHookItem->tid = tid;
  pHookItem->hHeap = GetProcessHeap();

  /* Install a thread scope message hook for WH_GETMESSAGE */
  pHookItem->GetMsg_hHook = SetWindowsHookExA( WH_GETMESSAGE, OLEMenu_GetMsgProc,
                                               0, GetCurrentThreadId() );
  if ( !pHookItem->GetMsg_hHook )
    goto CLEANUP;

  /* Install a thread scope message hook for WH_CALLWNDPROC */
  pHookItem->CallWndProc_hHook = SetWindowsHookExA( WH_CALLWNDPROC, OLEMenu_CallWndProc,
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
  OleMenuHookItem *pHookItem = NULL;

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
      pOleMenuDescriptor->bIsServerItem = (i%2) ? TRUE : FALSE;
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
  LPCWPSTRUCT pMsg = NULL;
  HOLEMENU hOleMenu = 0;
  OleMenuDescriptor *pOleMenuDescriptor = NULL;
  OleMenuHookItem *pHookItem = NULL;
  WORD fuFlags;

  TRACE("%i, %04lx, %08lx\n", code, wParam, lParam );

  /* Check if we're being asked to process the message */
  if ( HC_ACTION != code )
    goto NEXTHOOK;

  /* Retrieve the current message being dispatched from lParam */
  pMsg = (LPCWPSTRUCT)lParam;

  /* Check if the message is destined for a window we are interested in:
   * If the window has an OLEMenu property we may need to dispatch
   * the menu message to its active objects window instead. */

  hOleMenu = GetPropA( pMsg->hwnd, "PROP_OLEMenuDescriptor" );
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
      SendMessageA( pOleMenuDescriptor->hwndActiveObject,
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
    SendMessageA( pOleMenuDescriptor->hwndActiveObject,
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
  LPMSG pMsg = NULL;
  HOLEMENU hOleMenu = 0;
  OleMenuDescriptor *pOleMenuDescriptor = NULL;
  OleMenuHookItem *pHookItem = NULL;
  WORD wCode;

  TRACE("%i, %04lx, %08lx\n", code, wParam, lParam );

  /* Check if we're being asked to process a  messages */
  if ( HC_ACTION != code )
    goto NEXTHOOK;

  /* Retrieve the current message being dispatched from lParam */
  pMsg = (LPMSG)lParam;

  /* Check if the message is destined for a window we are interested in:
   * If the window has an OLEMenu property we may need to dispatch
   * the menu message to its active objects window instead. */

  hOleMenu = GetPropA( pMsg->hwnd, "PROP_OLEMenuDescriptor" );
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
     * without first calling it with a null hOleMenu to uninstall */
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
    SetPropA( hwndFrame, "PROP_OLEMenuDescriptor", hOleMenu );

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
    RemovePropA( hwndFrame, "PROP_OLEMenuDescriptor" );
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
		"msg->hwnd=%p, msg->message=%04x, wParam=%08lx, lParam=%08lx\n",
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
		TRACE_(accel)("found accel for virt_key %04lx (scan %04x)\n",
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
  switch (pmedium->tymed)
  {
    case TYMED_HGLOBAL:
    {
      if ( (pmedium->pUnkForRelease==0) &&
	   (pmedium->u.hGlobal!=0) )
	GlobalFree(pmedium->u.hGlobal);
      break;
    }
    case TYMED_FILE:
    {
      if (pmedium->u.lpszFileName!=0)
      {
	if (pmedium->pUnkForRelease==0)
	{
	  DeleteFileW(pmedium->u.lpszFileName);
	}

	CoTaskMemFree(pmedium->u.lpszFileName);
      }
      break;
    }
    case TYMED_ISTREAM:
    {
      if (pmedium->u.pstm!=0)
      {
	IStream_Release(pmedium->u.pstm);
      }
      break;
    }
    case TYMED_ISTORAGE:
    {
      if (pmedium->u.pstg!=0)
      {
	IStorage_Release(pmedium->u.pstg);
      }
      break;
    }
    case TYMED_GDI:
    {
      if ( (pmedium->pUnkForRelease==0) &&
	   (pmedium->u.hBitmap!=0) )
	DeleteObject(pmedium->u.hBitmap);
      break;
    }
    case TYMED_MFPICT:
    {
      if ( (pmedium->pUnkForRelease==0) &&
	   (pmedium->u.hMetaFilePict!=0) )
      {
	LPMETAFILEPICT pMP = GlobalLock(pmedium->u.hMetaFilePict);
	DeleteMetaFile(pMP->hMF);
	GlobalUnlock(pmedium->u.hMetaFilePict);
	GlobalFree(pmedium->u.hMetaFilePict);
      }
      break;
    }
    case TYMED_ENHMF:
    {
      if ( (pmedium->pUnkForRelease==0) &&
	   (pmedium->u.hEnhMetaFile!=0) )
      {
	DeleteEnhMetaFile(pmedium->u.hEnhMetaFile);
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
    WNDCLASSA wndClass;

    ZeroMemory (&wndClass, sizeof(WNDCLASSA));
    wndClass.style         = CS_GLOBALCLASS;
    wndClass.lpfnWndProc   = OLEDD_DragTrackerWindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(TrackerWindowInfo*);
    wndClass.hCursor       = 0;
    wndClass.hbrBackground = 0;
    wndClass.lpszClassName = OLEDD_DRAGTRACKERCLASS;

    RegisterClassA (&wndClass);
}

/***
 * OLEDD_FreeDropTarget()
 *
 * Frees the drag and drop data structure
 */
static void OLEDD_FreeDropTarget(DropTargetNode *dropTargetInfo, BOOL release_drop_target)
{
  list_remove(&dropTargetInfo->entry);
  if (release_drop_target) IDropTarget_Release(dropTargetInfo->dropTarget);
  HeapFree(GetProcessHeap(), 0, dropTargetInfo);
}

/***
 * OLEDD_UnInitialize()
 *
 * Releases the OLE drag and drop data structures.
 */
void OLEDD_UnInitialize(void)
{
  /*
   * Simply empty the list.
   */
  while (!list_empty(&targetListHead))
  {
    DropTargetNode* curNode = LIST_ENTRY(list_head(&targetListHead), DropTargetNode, entry);
    OLEDD_FreeDropTarget(curNode, FALSE);
  }
}

/***
 * OLEDD_FindDropTarget()
 *
 * Finds information about the drop target.
 */
static DropTargetNode* OLEDD_FindDropTarget(HWND hwndOfTarget)
{
  DropTargetNode*  curNode;

  /*
   * Iterate the list to find the HWND value.
   */
  LIST_FOR_EACH_ENTRY(curNode, &targetListHead, DropTargetNode, entry)
    if (hwndOfTarget==curNode->hwndTarget)
      return curNode;

  /*
   * If we get here, the item is not in the list
   */
  return NULL;
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

      SetWindowLongA(hwnd, 0, (LONG)createStruct->lpCreateParams);
      SetTimer(hwnd, DRAG_TIMER_ID, 50, NULL);

      break;
    }
    case WM_TIMER:
    case WM_MOUSEMOVE:
    {
      OLEDD_TrackMouseMove((TrackerWindowInfo*)GetWindowLongA(hwnd, 0));
      break;
    }
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
    {
      OLEDD_TrackStateChange((TrackerWindowInfo*)GetWindowLongA(hwnd, 0));
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
  return DefWindowProcA (hwnd, uMsg, wParam, lParam);
}

/***
 * OLEDD_TrackMouseMove()
 *
 * This method is invoked while a drag and drop operation is in effect.
 * it will generate the appropriate callbacks in the drop source
 * and drop target. It will also provide the expected feedback to
 * the user.
 *
 * params:
 *    trackerInfo - Pointer to the structure identifying the
 *                  drag & drop operation that is currently
 *                  active.
 */
static void OLEDD_TrackMouseMove(TrackerWindowInfo* trackerInfo)
{
  HWND   hwndNewTarget = 0;
  HRESULT  hr = S_OK;
  POINT pt;

  /*
   * Get the handle of the window under the mouse
   */
  pt.x = trackerInfo->curMousePos.x;
  pt.y = trackerInfo->curMousePos.y;
  hwndNewTarget = WindowFromPoint(pt);

  /*
   * Every time, we re-initialize the effects passed to the
   * IDropTarget to the effects allowed by the source.
   */
  *trackerInfo->pdwEffect = trackerInfo->dwOKEffect;

  /*
   * If we are hovering over the same target as before, send the
   * DragOver notification
   */
  if ( (trackerInfo->curDragTarget != 0) &&
       (trackerInfo->curTargetHWND == hwndNewTarget) )
  {
    IDropTarget_DragOver(trackerInfo->curDragTarget,
                         trackerInfo->dwKeyState,
                         trackerInfo->curMousePos,
			 trackerInfo->pdwEffect);
  }
  else
  {
    DropTargetNode* newDropTargetNode = 0;

    /*
     * If we changed window, we have to notify our old target and check for
     * the new one.
     */
    if (trackerInfo->curDragTarget!=0)
    {
      IDropTarget_DragLeave(trackerInfo->curDragTarget);
    }

    /*
     * Make sure we're hovering over a window.
     */
    if (hwndNewTarget!=0)
    {
      /*
       * Find-out if there is a drag target under the mouse
       */
      HWND nexttar = hwndNewTarget;
      trackerInfo->curTargetHWND = hwndNewTarget;

      do {
	newDropTargetNode = OLEDD_FindDropTarget(nexttar);
      } while (!newDropTargetNode && (nexttar = GetParent(nexttar)) != 0);
      if(nexttar) hwndNewTarget = nexttar;

      trackerInfo->curDragTargetHWND = hwndNewTarget;
      trackerInfo->curDragTarget     = newDropTargetNode ? newDropTargetNode->dropTarget : 0;

      /*
       * If there is, notify it that we just dragged-in
       */
      if (trackerInfo->curDragTarget!=0)
      {
	IDropTarget_DragEnter(trackerInfo->curDragTarget,
			      trackerInfo->dataObject,
                              trackerInfo->dwKeyState,
                              trackerInfo->curMousePos,
			      trackerInfo->pdwEffect);
      }
    }
    else
    {
      /*
       * The mouse is not over a window so we don't track anything.
       */
      trackerInfo->curDragTargetHWND = 0;
      trackerInfo->curTargetHWND     = 0;
      trackerInfo->curDragTarget     = 0;
    }
  }

  /*
   * Now that we have done that, we have to tell the source to give
   * us feedback on the work being done by the target.  If we don't
   * have a target, simulate no effect.
   */
  if (trackerInfo->curDragTarget==0)
  {
    *trackerInfo->pdwEffect = DROPEFFECT_NONE;
  }

  hr = IDropSource_GiveFeedback(trackerInfo->dropSource,
  				*trackerInfo->pdwEffect);

  /*
   * When we ask for feedback from the drop source, sometimes it will
   * do all the necessary work and sometimes it will not handle it
   * when that's the case, we must display the standard drag and drop
   * cursors.
   */
  if (hr==DRAGDROP_S_USEDEFAULTCURSORS)
  {
    if (*trackerInfo->pdwEffect & DROPEFFECT_MOVE)
    {
      SetCursor(LoadCursorA(hProxyDll, MAKEINTRESOURCEA(1)));
    }
    else if (*trackerInfo->pdwEffect & DROPEFFECT_COPY)
    {
      SetCursor(LoadCursorA(hProxyDll, MAKEINTRESOURCEA(2)));
    }
    else if (*trackerInfo->pdwEffect & DROPEFFECT_LINK)
    {
      SetCursor(LoadCursorA(hProxyDll, MAKEINTRESOURCEA(3)));
    }
    else
    {
      SetCursor(LoadCursorA(hProxyDll, MAKEINTRESOURCEA(0)));
    }
  }
}

/***
 * OLEDD_TrackStateChange()
 *
 * This method is invoked while a drag and drop operation is in effect.
 * It is used to notify the drop target/drop source callbacks when
 * the state of the keyboard or mouse button change.
 *
 * params:
 *    trackerInfo - Pointer to the structure identifying the
 *                  drag & drop operation that is currently
 *                  active.
 */
static void OLEDD_TrackStateChange(TrackerWindowInfo* trackerInfo)
{
  /*
   * Ask the drop source what to do with the operation.
   */
  trackerInfo->returnValue = IDropSource_QueryContinueDrag(
			       trackerInfo->dropSource,
			       trackerInfo->escPressed,
                               trackerInfo->dwKeyState);

  /*
   * All the return valued will stop the operation except the S_OK
   * return value.
   */
  if (trackerInfo->returnValue!=S_OK)
  {
    /*
     * Make sure the message loop in DoDragDrop stops
     */
    trackerInfo->trackingDone = TRUE;

    /*
     * Release the mouse in case the drop target decides to show a popup
     * or a menu or something.
     */
    ReleaseCapture();

    /*
     * If we end-up over a target, drop the object in the target or
     * inform the target that the operation was cancelled.
     */
    if (trackerInfo->curDragTarget!=0)
    {
      switch (trackerInfo->returnValue)
      {
	/*
	 * If the source wants us to complete the operation, we tell
	 * the drop target that we just dropped the object in it.
	 */
        case DRAGDROP_S_DROP:
	{
	  IDropTarget_Drop(trackerInfo->curDragTarget,
			   trackerInfo->dataObject,
                           trackerInfo->dwKeyState,
                           trackerInfo->curMousePos,
			   trackerInfo->pdwEffect);
	  break;
	}
	/*
	 * If the source told us that we should cancel, fool the drop
	 * target by telling it that the mouse left it's window.
	 * Also set the drop effect to "NONE" in case the application
	 * ignores the result of DoDragDrop.
	 */
        case DRAGDROP_S_CANCEL:
	  IDropTarget_DragLeave(trackerInfo->curDragTarget);
	  *trackerInfo->pdwEffect = DROPEFFECT_NONE;
	  break;
      }
    }
  }
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
  char  buffer[20];
  DWORD dwKeyType;
  DWORD cbData = 20;
  LONG  lres;

  lres = RegQueryValueExA(regKey,
			  "",
			  NULL,
			  &dwKeyType,
			  (LPBYTE)buffer,
			  &cbData);

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
	*pdwValue = (DWORD)strtoul(buffer, NULL, 10);
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
	LPCRECT lprcBounds)
{
  HRESULT hres;
  IViewObject *viewobject;

  hres = IUnknown_QueryInterface(pUnk,
				 &IID_IViewObject,
				 (void**)&viewobject);

  if (SUCCEEDED(hres))
  {
    RECTL rectl;

    rectl.left = lprcBounds->left;
    rectl.right = lprcBounds->right;
    rectl.top = lprcBounds->top;
    rectl.bottom = lprcBounds->bottom;
    hres = IViewObject_Draw(viewobject, dwAspect, -1, 0, 0, 0, hdcDraw, &rectl, 0, 0, 0);

    IViewObject_Release(viewobject);
    return hres;
  }
  else
  {
    return DV_E_NOIVIEWOBJECT;
  }
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

    TRACE("(%s, %s, %d, %p, %p, %p, %p)\n", debugstr_guid(rclsid),
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
            hres = IOleObject_GetMiscStatus(pOleObject, DVASPECT_CONTENT, &dwStatus);
        }
    }

    if (SUCCEEDED(hres))
    {
        IPersistStorage * pPS;
        if (SUCCEEDED((hres = IUnknown_QueryInterface(pUnk, &IID_IPersistStorage, (LPVOID*)&pPS))))
        {
            TRACE("trying to set stg %p\n", pStg);
            hres = IPersistStorage_InitNew(pPS, pStg);
            TRACE("-- result 0x%08x\n", hres);
            IPersistStorage_Release(pPS);
        }
    }

    if (pClientSite && SUCCEEDED(hres))
    {
        TRACE("trying to set clientsite %p\n", pClientSite);
        hres = IOleObject_SetClientSite(pOleObject, pClientSite);
        TRACE("-- result 0x%08x\n", hres);
    }

    if (pOleObject)
        IOleObject_Release(pOleObject);

    if (((renderopt == OLERENDER_DRAW) || (renderopt == OLERENDER_FORMAT)) &&
        SUCCEEDED(hres))
    {
        IRunnableObject *pRunnable;
        IOleCache *pOleCache;
        HRESULT hres2;

        hres2 = IUnknown_QueryInterface(pUnk, &IID_IRunnableObject, (void **)&pRunnable);
        if (SUCCEEDED(hres2))
        {
            hres = IRunnableObject_Run(pRunnable, NULL);
            IRunnableObject_Release(pRunnable);
        }

        if (SUCCEEDED(hres))
        {
            hres2 = IUnknown_QueryInterface(pUnk, &IID_IOleCache, (void **)&pOleCache);
            if (SUCCEEDED(hres2))
            {
                DWORD dwConnection;
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
    static const WCHAR wszAutoConvertTo[] = {'A','u','t','o','C','o','n','v','e','r','t','T','o',0};
    HKEY hkey = NULL;
    WCHAR buf[CHARS_IN_GUID];
    LONG len;
    HRESULT res = S_OK;

    res = COM_OpenKeyForCLSID(clsidOld, wszAutoConvertTo, KEY_READ, &hkey);
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
    static const WCHAR wszAutoConvertTo[] = {'A','u','t','o','C','o','n','v','e','r','t','T','o',0};
    HKEY hkey = NULL;
    WCHAR szClsidNew[CHARS_IN_GUID];
    HRESULT res = S_OK;

    TRACE("(%s,%s)\n", debugstr_guid(clsidOld), debugstr_guid(clsidNew));
    
    res = COM_OpenKeyForCLSID(clsidOld, NULL, KEY_READ | KEY_WRITE, &hkey);
    if (FAILED(res))
        goto done;
    StringFromGUID2(clsidNew, szClsidNew, CHARS_IN_GUID);
    if (RegSetValueW(hkey, wszAutoConvertTo, REG_SZ, szClsidNew, (strlenW(szClsidNew)+1) * sizeof(WCHAR)))
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
    FIXME("(%p,%p) : stub\n",pStg,pClsidNew);
    return E_NOTIMPL;
}

/******************************************************************************
 *              OleIsRunning        [OLE32.@]
 */
BOOL WINAPI OleIsRunning(LPOLEOBJECT pObject)
{
    IRunnableObject *pRunnable;
    HRESULT hr;
    BOOL running;

    TRACE("(%p)\n", pObject);

    hr = IOleObject_QueryInterface(pObject, &IID_IRunnableObject, (void **)&pRunnable);
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
 *           OLE_FreeClipDataArray   [internal]
 *
 * NOTES:
 *  frees the data associated with an array of CLIPDATAs
 */
static void OLE_FreeClipDataArray(ULONG count, CLIPDATA * pClipDataArray)
{
    ULONG i;
    for (i = 0; i < count; i++)
        if (pClipDataArray[i].pClipData)
            CoTaskMemFree(pClipDataArray[i].pClipData);
}

/***********************************************************************
 *           PropSysAllocString			    [OLE32.@]
 * NOTES:
 *  Basically a copy of SysAllocStringLen.
 */
BSTR WINAPI PropSysAllocString(LPCOLESTR str)
{
    DWORD  bufferSize;
    DWORD* newBuffer;
    WCHAR* stringBuffer;
    int len;

    if (!str) return 0;

    len = lstrlenW(str);
    /*
     * Find the length of the buffer passed-in, in bytes.
     */
    bufferSize = len * sizeof (WCHAR);

    /*
     * Allocate a new buffer to hold the string.
     * Don't forget to keep an empty spot at the beginning of the
     * buffer for the character count and an extra character at the
     * end for the NULL.
     */
    newBuffer = HeapAlloc(GetProcessHeap(), 0,
                          bufferSize + sizeof(WCHAR) + sizeof(DWORD));

    /*
     * If the memory allocation failed, return a null pointer.
     */
    if (newBuffer==0)
      return 0;

    /*
     * Copy the length of the string in the placeholder.
     */
    *newBuffer = bufferSize;

    /*
     * Skip the byte count.
     */
    newBuffer++;

    memcpy(newBuffer, str, bufferSize);

    /*
     * Make sure that there is a nul character at the end of the
     * string.
     */
    stringBuffer = (WCHAR*)newBuffer;
    stringBuffer[len] = '\0';

    return (LPWSTR)stringBuffer;
}

/***********************************************************************
 *           PropSysFreeString			    [OLE32.@]
 * NOTES
 *  Copy of SysFreeString.
 */
void WINAPI PropSysFreeString(LPOLESTR str)
{
    DWORD* bufferPointer;

    /* NULL is a valid parameter */
    if(!str) return;

    /*
     * We have to be careful when we free a BSTR pointer, it points to
     * the beginning of the string but it skips the byte count contained
     * before the string.
     */
    bufferPointer = (DWORD*)str;

    bufferPointer--;

    /*
     * Free the memory from its "real" origin.
     */
    HeapFree(GetProcessHeap(), 0, bufferPointer);
}

/******************************************************************************
 * Check if a PROPVARIANT's type is valid.
 */
static inline HRESULT PROPVARIANT_ValidateType(VARTYPE vt)
{
    switch (vt)
    {
    case VT_EMPTY:
    case VT_NULL:
    case VT_I2:
    case VT_I4:
    case VT_R4:
    case VT_R8:
    case VT_CY:
    case VT_DATE:
    case VT_BSTR:
    case VT_ERROR:
    case VT_BOOL:
    case VT_UI1:
    case VT_UI2:
    case VT_UI4:
    case VT_I8:
    case VT_UI8:
    case VT_LPSTR:
    case VT_LPWSTR:
    case VT_FILETIME:
    case VT_BLOB:
    case VT_STREAM:
    case VT_STORAGE:
    case VT_STREAMED_OBJECT:
    case VT_STORED_OBJECT:
    case VT_BLOB_OBJECT:
    case VT_CF:
    case VT_CLSID:
    case VT_I2|VT_VECTOR:
    case VT_I4|VT_VECTOR:
    case VT_R4|VT_VECTOR:
    case VT_R8|VT_VECTOR:
    case VT_CY|VT_VECTOR:
    case VT_DATE|VT_VECTOR:
    case VT_BSTR|VT_VECTOR:
    case VT_ERROR|VT_VECTOR:
    case VT_BOOL|VT_VECTOR:
    case VT_VARIANT|VT_VECTOR:
    case VT_UI1|VT_VECTOR:
    case VT_UI2|VT_VECTOR:
    case VT_UI4|VT_VECTOR:
    case VT_I8|VT_VECTOR:
    case VT_UI8|VT_VECTOR:
    case VT_LPSTR|VT_VECTOR:
    case VT_LPWSTR|VT_VECTOR:
    case VT_FILETIME|VT_VECTOR:
    case VT_CF|VT_VECTOR:
    case VT_CLSID|VT_VECTOR:
        return S_OK;
    }
    WARN("Bad type %d\n", vt);
    return STG_E_INVALIDPARAMETER;
}

/***********************************************************************
 *           PropVariantClear			    [OLE32.@]
 */
HRESULT WINAPI PropVariantClear(PROPVARIANT * pvar) /* [in/out] */
{
    HRESULT hr;

    TRACE("(%p)\n", pvar);

    if (!pvar)
        return S_OK;

    hr = PROPVARIANT_ValidateType(pvar->vt);
    if (FAILED(hr))
        return hr;

    switch(pvar->vt)
    {
    case VT_EMPTY:
    case VT_NULL:
    case VT_I2:
    case VT_I4:
    case VT_R4:
    case VT_R8:
    case VT_CY:
    case VT_DATE:
    case VT_ERROR:
    case VT_BOOL:
    case VT_UI1:
    case VT_UI2:
    case VT_UI4:
    case VT_I8:
    case VT_UI8:
    case VT_FILETIME:
        break;
    case VT_STREAM:
    case VT_STREAMED_OBJECT:
    case VT_STORAGE:
    case VT_STORED_OBJECT:
        if (pvar->u.pStream)
            IUnknown_Release(pvar->u.pStream);
        break;
    case VT_CLSID:
    case VT_LPSTR:
    case VT_LPWSTR:
        /* pick an arbitrary typed pointer - we don't care about the type
         * as we are just freeing it */
        CoTaskMemFree(pvar->u.puuid);
        break;
    case VT_BLOB:
    case VT_BLOB_OBJECT:
        CoTaskMemFree(pvar->u.blob.pBlobData);
        break;
    case VT_BSTR:
        if (pvar->u.bstrVal)
            PropSysFreeString(pvar->u.bstrVal);
        break;
    case VT_CF:
        if (pvar->u.pclipdata)
        {
            OLE_FreeClipDataArray(1, pvar->u.pclipdata);
            CoTaskMemFree(pvar->u.pclipdata);
        }
        break;
    default:
        if (pvar->vt & VT_VECTOR)
        {
            ULONG i;

            switch (pvar->vt & ~VT_VECTOR)
            {
            case VT_VARIANT:
                FreePropVariantArray(pvar->u.capropvar.cElems, pvar->u.capropvar.pElems);
                break;
            case VT_CF:
                OLE_FreeClipDataArray(pvar->u.caclipdata.cElems, pvar->u.caclipdata.pElems);
                break;
            case VT_BSTR:
                for (i = 0; i < pvar->u.cabstr.cElems; i++)
                    PropSysFreeString(pvar->u.cabstr.pElems[i]);
                break;
            case VT_LPSTR:
                for (i = 0; i < pvar->u.calpstr.cElems; i++)
                    CoTaskMemFree(pvar->u.calpstr.pElems[i]);
                break;
            case VT_LPWSTR:
                for (i = 0; i < pvar->u.calpwstr.cElems; i++)
                    CoTaskMemFree(pvar->u.calpwstr.pElems[i]);
                break;
            }
            if (pvar->vt & ~VT_VECTOR)
            {
                /* pick an arbitrary VT_VECTOR structure - they all have the same
                 * memory layout */
                CoTaskMemFree(pvar->u.capropvar.pElems);
            }
        }
        else
            WARN("Invalid/unsupported type %d\n", pvar->vt);
    }

    ZeroMemory(pvar, sizeof(*pvar));

    return S_OK;
}

/***********************************************************************
 *           PropVariantCopy			    [OLE32.@]
 */
HRESULT WINAPI PropVariantCopy(PROPVARIANT *pvarDest,      /* [out] */
                               const PROPVARIANT *pvarSrc) /* [in] */
{
    ULONG len;
    HRESULT hr;

    TRACE("(%p, %p)\n", pvarDest, pvarSrc);

    hr = PROPVARIANT_ValidateType(pvarSrc->vt);
    if (FAILED(hr))
        return hr;

    /* this will deal with most cases */
    *pvarDest = *pvarSrc;

    switch(pvarSrc->vt)
    {
    case VT_EMPTY:
    case VT_NULL:
    case VT_I1:
    case VT_UI1:
    case VT_I2:
    case VT_UI2:
    case VT_BOOL:
    case VT_I4:
    case VT_UI4:
    case VT_R4:
    case VT_ERROR:
    case VT_I8:
    case VT_UI8:
    case VT_R8:
    case VT_CY:
    case VT_DATE:
    case VT_FILETIME:
        break;
    case VT_STREAM:
    case VT_STREAMED_OBJECT:
    case VT_STORAGE:
    case VT_STORED_OBJECT:
        IUnknown_AddRef((LPUNKNOWN)pvarDest->u.pStream);
        break;
    case VT_CLSID:
        pvarDest->u.puuid = CoTaskMemAlloc(sizeof(CLSID));
        *pvarDest->u.puuid = *pvarSrc->u.puuid;
        break;
    case VT_LPSTR:
        len = strlen(pvarSrc->u.pszVal);
        pvarDest->u.pszVal = CoTaskMemAlloc((len+1)*sizeof(CHAR));
        CopyMemory(pvarDest->u.pszVal, pvarSrc->u.pszVal, (len+1)*sizeof(CHAR));
        break;
    case VT_LPWSTR:
        len = lstrlenW(pvarSrc->u.pwszVal);
        pvarDest->u.pwszVal = CoTaskMemAlloc((len+1)*sizeof(WCHAR));
        CopyMemory(pvarDest->u.pwszVal, pvarSrc->u.pwszVal, (len+1)*sizeof(WCHAR));
        break;
    case VT_BLOB:
    case VT_BLOB_OBJECT:
        if (pvarSrc->u.blob.pBlobData)
        {
            len = pvarSrc->u.blob.cbSize;
            pvarDest->u.blob.pBlobData = CoTaskMemAlloc(len);
            CopyMemory(pvarDest->u.blob.pBlobData, pvarSrc->u.blob.pBlobData, len);
        }
        break;
    case VT_BSTR:
        pvarDest->u.bstrVal = PropSysAllocString(pvarSrc->u.bstrVal);
        break;
    case VT_CF:
        if (pvarSrc->u.pclipdata)
        {
            len = pvarSrc->u.pclipdata->cbSize - sizeof(pvarSrc->u.pclipdata->ulClipFmt);
            pvarDest->u.pclipdata = CoTaskMemAlloc(sizeof (CLIPDATA));
            pvarDest->u.pclipdata->cbSize = pvarSrc->u.pclipdata->cbSize;
            pvarDest->u.pclipdata->ulClipFmt = pvarSrc->u.pclipdata->ulClipFmt;
            pvarDest->u.pclipdata->pClipData = CoTaskMemAlloc(len);
            CopyMemory(pvarDest->u.pclipdata->pClipData, pvarSrc->u.pclipdata->pClipData, len);
        }
        break;
    default:
        if (pvarSrc->vt & VT_VECTOR)
        {
            int elemSize;
            ULONG i;

            switch(pvarSrc->vt & ~VT_VECTOR)
            {
            case VT_I1:       elemSize = sizeof(pvarSrc->u.cVal); break;
            case VT_UI1:      elemSize = sizeof(pvarSrc->u.bVal); break;
            case VT_I2:       elemSize = sizeof(pvarSrc->u.iVal); break;
            case VT_UI2:      elemSize = sizeof(pvarSrc->u.uiVal); break;
            case VT_BOOL:     elemSize = sizeof(pvarSrc->u.boolVal); break;
            case VT_I4:       elemSize = sizeof(pvarSrc->u.lVal); break;
            case VT_UI4:      elemSize = sizeof(pvarSrc->u.ulVal); break;
            case VT_R4:       elemSize = sizeof(pvarSrc->u.fltVal); break;
            case VT_R8:       elemSize = sizeof(pvarSrc->u.dblVal); break;
            case VT_ERROR:    elemSize = sizeof(pvarSrc->u.scode); break;
            case VT_I8:       elemSize = sizeof(pvarSrc->u.hVal); break;
            case VT_UI8:      elemSize = sizeof(pvarSrc->u.uhVal); break;
            case VT_CY:       elemSize = sizeof(pvarSrc->u.cyVal); break;
            case VT_DATE:     elemSize = sizeof(pvarSrc->u.date); break;
            case VT_FILETIME: elemSize = sizeof(pvarSrc->u.filetime); break;
            case VT_CLSID:    elemSize = sizeof(*pvarSrc->u.puuid); break;
            case VT_CF:       elemSize = sizeof(*pvarSrc->u.pclipdata); break;
            case VT_BSTR:     elemSize = sizeof(*pvarSrc->u.bstrVal); break;
            case VT_LPSTR:    elemSize = sizeof(*pvarSrc->u.pszVal); break;
            case VT_LPWSTR:   elemSize = sizeof(*pvarSrc->u.pwszVal); break;
            case VT_VARIANT:  elemSize = sizeof(*pvarSrc->u.pvarVal); break;

            default:
                FIXME("Invalid element type: %ul\n", pvarSrc->vt & ~VT_VECTOR);
                return E_INVALIDARG;
            }
            len = pvarSrc->u.capropvar.cElems;
            pvarDest->u.capropvar.pElems = CoTaskMemAlloc(len * elemSize);
            if (pvarSrc->vt == (VT_VECTOR | VT_VARIANT))
            {
                for (i = 0; i < len; i++)
                    PropVariantCopy(&pvarDest->u.capropvar.pElems[i], &pvarSrc->u.capropvar.pElems[i]);
            }
            else if (pvarSrc->vt == (VT_VECTOR | VT_CF))
            {
                FIXME("Copy clipformats\n");
            }
            else if (pvarSrc->vt == (VT_VECTOR | VT_BSTR))
            {
                for (i = 0; i < len; i++)
                    pvarDest->u.cabstr.pElems[i] = PropSysAllocString(pvarSrc->u.cabstr.pElems[i]);
            }
            else if (pvarSrc->vt == (VT_VECTOR | VT_LPSTR))
            {
                size_t strLen;
                for (i = 0; i < len; i++)
                {
                    strLen = lstrlenA(pvarSrc->u.calpstr.pElems[i]) + 1;
                    pvarDest->u.calpstr.pElems[i] = CoTaskMemAlloc(strLen);
                    memcpy(pvarDest->u.calpstr.pElems[i],
                     pvarSrc->u.calpstr.pElems[i], strLen);
                }
            }
            else if (pvarSrc->vt == (VT_VECTOR | VT_LPWSTR))
            {
                size_t strLen;
                for (i = 0; i < len; i++)
                {
                    strLen = (lstrlenW(pvarSrc->u.calpwstr.pElems[i]) + 1) *
                     sizeof(WCHAR);
                    pvarDest->u.calpstr.pElems[i] = CoTaskMemAlloc(strLen);
                    memcpy(pvarDest->u.calpstr.pElems[i],
                     pvarSrc->u.calpstr.pElems[i], strLen);
                }
            }
            else
                CopyMemory(pvarDest->u.capropvar.pElems, pvarSrc->u.capropvar.pElems, len * elemSize);
        }
        else
            WARN("Invalid/unsupported type %d\n", pvarSrc->vt);
    }

    return S_OK;
}

/***********************************************************************
 *           FreePropVariantArray			    [OLE32.@]
 */
HRESULT WINAPI FreePropVariantArray(ULONG cVariants, /* [in] */
                                    PROPVARIANT *rgvars)    /* [in/out] */
{
    ULONG i;

    TRACE("(%u, %p)\n", cVariants, rgvars);

    if (!rgvars)
        return E_INVALIDARG;

    for(i = 0; i < cVariants; i++)
        PropVariantClear(&rgvars[i]);

    return S_OK;
}

/******************************************************************************
 * DllDebugObjectRPCHook (OLE32.@)
 * turns on and off internal debugging,  pointer is only used on macintosh
 */

BOOL WINAPI DllDebugObjectRPCHook(BOOL b, void *dummy)
{
  FIXME("stub\n");
  return TRUE;
}
