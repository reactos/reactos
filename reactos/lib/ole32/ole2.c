/*
 *	OLE2 library
 *
 *	Copyright 1995	Martin von Loewis
 *      Copyright 1999  Francis Beaudet
 *      Copyright 1999  Noel Borthwick 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>
#include <ole32/ole32.h>
#include <compobj.h>
#include <storage32.h>
#include "ole32_main.h"

#include <debug.h>

#if 0

/******************************************************************************
 * These are static/global variables and internal data structures that the 
 * OLE module uses to maintain it's state.
 */
typedef struct tagDropTargetNode
{
  HWND          hwndTarget;
  IDropTarget*    dropTarget;
  struct tagDropTargetNode* prevDropTarget;
  struct tagDropTargetNode* nextDropTarget;
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
  HWND       curDragTargetHWND;
  IDropTarget* curDragTarget;
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
static ULONG OLE_moduleLockCount = 0;

/*
 * Name of our registered window class.
 */
static const char OLEDD_DRAGTRACKERCLASS[] = "WineDragDropTracker32";

/*
 * This is the head of the Drop target container.
 */
static DropTargetNode* targetListHead = NULL;

/******************************************************************************
 * These are the prototypes of miscelaneous utility methods 
 */
static void OLEUTL_ReadRegistryDWORDValue(HKEY regKey, DWORD* pdwValue);

/******************************************************************************
 * These are the prototypes of the utility methods used to manage a shared menu
 */
static void OLEMenu_Initialize();
static void OLEMenu_UnInitialize();
BOOL OLEMenu_InstallHooks( DWORD tid );
BOOL OLEMenu_UnInstallHooks( DWORD tid );
OleMenuHookItem * OLEMenu_IsHookInstalled( DWORD tid );
static BOOL OLEMenu_FindMainMenuIndex( HMENU hMainMenu, HMENU hPopupMenu, UINT *pnPos );
BOOL OLEMenu_SetIsServerMenu( HMENU hmenu, OleMenuDescriptor *pOleMenuDescriptor );
LRESULT CALLBACK OLEMenu_CallWndProc(INT code, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK OLEMenu_GetMsgProc(INT code, WPARAM wParam, LPARAM lParam);

/******************************************************************************
 * These are the prototypes of the OLE Clipboard initialization methods (in clipboard.c)
 */
void OLEClipbrd_UnInitialize();
void OLEClipbrd_Initialize();

/******************************************************************************
 * These are the prototypes of the utility methods used for OLE Drag n Drop
 */
static void            OLEDD_Initialize();
static void            OLEDD_UnInitialize();
static void            OLEDD_InsertDropTarget(
			 DropTargetNode* nodeToAdd);
static DropTargetNode* OLEDD_ExtractDropTarget(
                         HWND hwndOfTarget);
static DropTargetNode* OLEDD_FindDropTarget(
                         HWND hwndOfTarget);
static LRESULT WINAPI  OLEDD_DragTrackerWindowProc(
			 HWND   hwnd, 
			 UINT   uMsg,
			 WPARAM wParam, 
			 LPARAM   lParam);
static void OLEDD_TrackMouseMove(
                         TrackerWindowInfo* trackerInfo,
			 POINT            mousePos,
			 DWORD              keyState);
static void OLEDD_TrackStateChange(
                         TrackerWindowInfo* trackerInfo,
			 POINT            mousePos,
			 DWORD              keyState);
static DWORD OLEDD_GetButtonState();


/******************************************************************************
 *		OleBuildVersion	[OLE2.1]
 */
DWORD WINAPI OleBuildVersion(void)
{
    Print(MAX_TRACE, ("Returning version %d, build %d.\n", 1, 0));
    return (1<<16)+0;
}

/***********************************************************************
 *           OleInitialize       (OLE2.2) (OLE32.108)
 */
HRESULT WINAPI OleInitialize(LPVOID reserved)
{
  HRESULT hr;

  Print(MAX_TRACE, ("(%p)\n", reserved));

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
  if (OLE_moduleLockCount==0)
{
    /* 
     * Initialize the libraries.
     */
    Print(MAX_TRACE, ("() - Initializing the OLE libraries\n"));

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

  /*
   * Then, we increase the lock count on the OLE module.
   */
  OLE_moduleLockCount++;  

  return hr;
}

/******************************************************************************
 *		CoGetCurrentProcess	[COMPOBJ.34] [OLE2.2][OLE32.108]
 *
 * NOTES
 *   Is DWORD really the correct return type for this function?
 */
DWORD WINAPI CoGetCurrentProcess(void)
{
	return GetCurrentProcessId();
}

/******************************************************************************
 *		OleUninitialize	[OLE2.3] [OLE32.131]
 */
void WINAPI OleUninitialize(void)
{
  Print(MAX_TRACE, ("()\n"));

  /*
   * Decrease the lock count on the OLE module.
   */
  OLE_moduleLockCount--;

  /*
   * If we hit the bottom of the lock stack, free the libraries.
   */
  if (OLE_moduleLockCount==0)
  {
    /*
     * Actually free the libraries.
     */
    Print(MAX_TRACE, ("() - Freeing the last reference count\n"));

    /*
     * OLE Clipboard
     */
    OLEClipbrd_UnInitialize();

    /*
     * Drag and Drop
     */
    OLEDD_UnInitialize();
    
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
 *		CoRegisterMessageFilter	[OLE32.38]
 */
HRESULT WINAPI CoRegisterMessageFilter(
    LPMESSAGEFILTER lpMessageFilter,	/* [in] Pointer to interface */
    LPMESSAGEFILTER *lplpMessageFilter	/* [out] Indirect pointer to prior instance if non-NULL */
) {
    Print(MIN_TRACE, ("stub\n"));
    if (lplpMessageFilter) {
	*lplpMessageFilter = NULL;
    }
    return S_OK;
}

/******************************************************************************
 *		OleInitializeWOW	[OLE32.109]
 */
HRESULT WINAPI OleInitializeWOW(DWORD x) {
        Print(MIN_TRACE, ("(0x%08lx),stub!\n",x));
        return 0;
}

/***********************************************************************
 *           RegisterDragDrop16 (OLE2.35)
 */
HRESULT WINAPI RegisterDragDrop16(
	DWORD hwnd,
	LPDROPTARGET pDropTarget
) {
	UNIMPLEMENTED;
	return S_OK;
}

/***********************************************************************
 *           RegisterDragDrop (OLE32.139)
 */
HRESULT WINAPI RegisterDragDrop(
	HWND hwnd,
	LPDROPTARGET pDropTarget) 
{
  DropTargetNode* dropTargetInfo;

  Print(MAX_TRACE, ("(0x%x,%p)\n", hwnd, pDropTarget));

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
  dropTargetInfo->prevDropTarget = NULL;
  dropTargetInfo->nextDropTarget = NULL;

  /*
   * Don't forget that this is an interface pointer, need to nail it down since
   * we keep a copy of it.
   */
  dropTargetInfo->dropTarget  = pDropTarget;
  IDropTarget_AddRef(dropTargetInfo->dropTarget);
  
  OLEDD_InsertDropTarget(dropTargetInfo);

	return S_OK;
}

/***********************************************************************
 *           RevokeDragDrop16 (OLE2.36)
 */
HRESULT WINAPI RevokeDragDrop16(
	DWORD hwnd
) {
	UNIMPLEMENTED;
	return S_OK;
}

/***********************************************************************
 *           RevokeDragDrop (OLE32.141)
 */
HRESULT WINAPI RevokeDragDrop(
	HWND hwnd)
{
  DropTargetNode* dropTargetInfo;

  Print(MAX_TRACE, ("(0x%x)\n", hwnd));

  /*
   * First, check if the window is already registered.
   */
  dropTargetInfo = OLEDD_ExtractDropTarget(hwnd);

  /*
   * If it ain't in there, it's an error.
   */
  if (dropTargetInfo==NULL)
    return DRAGDROP_E_NOTREGISTERED;

  /*
   * If it's in there, clean-up it's used memory and
   * references
   */
  IDropTarget_Release(dropTargetInfo->dropTarget);
  HeapFree(GetProcessHeap(), 0, dropTargetInfo);  

	return S_OK;
}

/***********************************************************************
 *           OleRegGetUserType (OLE32.122)
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
  LPBYTE  buffer;
  HRESULT retVal;
  /*
   * Initialize the out parameter.
   */
  *pszUserType = NULL;

  /*
   * Build the key name we're looking for
   */
  sprintf( keyName, "CLSID\\{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\\",
           clsid->Data1, clsid->Data2, clsid->Data3,
           clsid->Data4[0], clsid->Data4[1], clsid->Data4[2], clsid->Data4[3],
           clsid->Data4[4], clsid->Data4[5], clsid->Data4[6], clsid->Data4[7] );

  Print(MAX_TRACE, ("(%s, %ld, %p)\n", keyName, dwFormOfType, pszUserType));

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
			  buffer,
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
 * DoDragDrop [OLE32.65]
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

  Print(MAX_TRACE, ("(DataObject %p, DropSource %p)\n", pDataObject, pDropSource));

  /*
   * Setup the drag n drop tracking window.
   */
  trackerInfo.dataObject        = pDataObject;
  trackerInfo.dropSource        = pDropSource;
  trackerInfo.dwOKEffect        = dwOKEffect;
  trackerInfo.pdwEffect         = pdwEffect;
  trackerInfo.trackingDone      = FALSE;
  trackerInfo.escPressed        = FALSE;
  trackerInfo.curDragTargetHWND = 0;
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

    /*
     * Pump messages. All mouse input should go the the capture window.
     */
    while (!trackerInfo.trackingDone && GetMessageA(&msg, 0, 0, 0) )
    {
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
	OLEDD_TrackStateChange(&trackerInfo,
			       msg.pt,
			       OLEDD_GetButtonState());
      }
      else
      {
	/*
	 * Dispatch the messages only when it's not a keyboard message.
	 */
	DispatchMessageA(&msg);
      }
    }

    /*
     * Destroy the temporary window.
     */
    DestroyWindow(hwndTrackWindow);

    return trackerInfo.returnValue;
  }

  return E_FAIL;
}

/***********************************************************************
 * OleQueryLinkFromData [OLE32.118]
 */
HRESULT WINAPI OleQueryLinkFromData(
  IDataObject* pSrcDataObject)
{
  Print(MIN_TRACE, ("(%p),stub!\n", pSrcDataObject));
  return S_OK;
}

/***********************************************************************
 * OleRegGetMiscStatus [OLE32.121]
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
  sprintf( keyName, "CLSID\\{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\\",
           clsid->Data1, clsid->Data2, clsid->Data3,
           clsid->Data4[0], clsid->Data4[1], clsid->Data4[2], clsid->Data4[3],
           clsid->Data4[4], clsid->Data4[5], clsid->Data4[6], clsid->Data4[7] );

  Print(MAX_TRACE, ("(%s, %ld, %p)\n", keyName, dwAspect, pdwStatus));

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
  sprintf(keyName, "%ld", dwAspect);

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

/******************************************************************************
 *              OleSetContainedObject        [OLE32.128]
 */
HRESULT WINAPI OleSetContainedObject(
  LPUNKNOWN pUnknown, 
  BOOL      fContained)
{
  IRunnableObject* runnable = NULL;
  HRESULT          hres;

  Print(MAX_TRACE, ("(%p,%x), stub!\n", pUnknown, fContained));

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
 *              OleLoad        [OLE32.112]
 */
HRESULT WINAPI OleLoad(
  LPSTORAGE       pStg, 
  REFIID          riid, 
  LPOLECLIENTSITE pClientSite, 
  LPVOID*         ppvObj)
{
  IPersistStorage* persistStorage = NULL;
  IOleObject*      oleObject      = NULL;
  STATSTG          storageInfo;
  HRESULT          hres;

  Print(MAX_TRACE, ("(%p,%p,%p,%p)\n", pStg, riid, pClientSite, ppvObj));
  
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
			  CLSCTX_INPROC_HANDLER,
			  &IID_IOleObject,
			  (void**)&oleObject);

  /*
   * If that fails, as it will most times, load the default
   * OLE handler.
   */
  if (FAILED(hres))
  {
    hres = OleCreateDefaultHandler(&storageInfo.clsid,
				   NULL,
				   &IID_IOleObject,
				   (void**)&oleObject);
  }

  /*
   * If we couldn't find a handler... this is bad. Abort the whole thing.
   */
  if (FAILED(hres))
    return hres;

  /*
   * Inform the new object of it's client site.
   */
  hres = IOleObject_SetClientSite(oleObject, pClientSite);

  /*
   * Initialize the object with it's IPersistStorage interface.
   */
  hres = IOleObject_QueryInterface(oleObject,
				   &IID_IPersistStorage,
				   (void**)&persistStorage);

  if (SUCCEEDED(hres)) 
  {
    IPersistStorage_Load(persistStorage, pStg);

    IPersistStorage_Release(persistStorage);
    persistStorage = NULL;
  }

  /*
   * Return the requested interface to the caller.
   */
  hres = IOleObject_QueryInterface(oleObject, riid, ppvObj);

  /*
   * Cleanup interfaces used internally
   */
  IOleObject_Release(oleObject);

  return hres;
}

/***********************************************************************
 *           OleSave     [OLE32.124]
 */
HRESULT WINAPI OleSave(
  LPPERSISTSTORAGE pPS,
  LPSTORAGE        pStg,
  BOOL             fSameAsLoad)
{
  HRESULT hres;
  CLSID   objectClass;

  Print(MAX_TRACE, ("(%p,%p,%x)\n", pPS, pStg, fSameAsLoad));

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
 *              OleLockRunning        [OLE32.114]
 */
HRESULT WINAPI OleLockRunning(LPUNKNOWN pUnknown, BOOL fLock, BOOL fLastUnlockCloses) 
{
  IRunnableObject* runnable = NULL;
  HRESULT          hres;

  Print(MAX_TRACE, ("(%p,%x,%x)\n", pUnknown, fLock, fLastUnlockCloses));

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
static void OLEMenu_Initialize()
{
}

/***
 * OLEMenu_UnInitialize()
 *
 * Releases the OLEMENU data structures.
 */
static void OLEMenu_UnInitialize()
{
}

/*************************************************************************
 * OLEMenu_InstallHooks
 * Install thread scope message hooks for WH_GETMESSAGE and WH_CALLWNDPROC
 *
 * RETURNS: TRUE if message hooks were succesfully installed
 *          FALSE on failure
 */
BOOL OLEMenu_InstallHooks( DWORD tid )
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
 * RETURNS: TRUE if message hooks were succesfully installed
 *          FALSE on failure
 */
BOOL OLEMenu_UnInstallHooks( DWORD tid )
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
  if (pHookItem)
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
OleMenuHookItem * OLEMenu_IsHookInstalled( DWORD tid )
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
  UINT i, nItems;

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
 *          FASE if the popup menu is part of a container owned group
 */
BOOL OLEMenu_SetIsServerMenu( HMENU hmenu, OleMenuDescriptor *pOleMenuDescriptor )
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
   * Loop thru the group widths and locate the group we are a member of.
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
LRESULT CALLBACK OLEMenu_CallWndProc(INT code, WPARAM wParam, LPARAM lParam)
{
#if 0
  LPCWPSTRUCT pMsg = NULL;
  HOLEMENU hOleMenu = 0;
  OleMenuDescriptor *pOleMenuDescriptor = NULL;
  OleMenuHookItem *pHookItem = NULL;
  WORD fuFlags;
  
  Print(MAX_TRACE, ("%i, %04x, %08x\n", code, wParam, (unsigned)lParam ));

  /* Check if we're being asked to process the message */
  if ( HC_ACTION != code )
    goto NEXTHOOK;
      
  /* Retrieve the current message being dispatched from lParam */
  pMsg = (LPCWPSTRUCT)lParam;

  /* Check if the message is destined for a window we are interested in:
   * If the window has an OLEMenu property we may need to dispatch
   * the menu message to its active objects window instead. */

  hOleMenu = (HOLEMENU)GetPropA( pMsg->hwnd, "PROP_OLEMenuDescriptor" );
  if ( !hOleMenu )
    goto NEXTHOOK;

  /* Get the menu descriptor */
  pOleMenuDescriptor = (OleMenuDescriptor *) GlobalLock( hOleMenu );
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
    Print(MID_TRACE, ("could not retrieve hHook for current thread!\n" ));
    return 0;
  }
  
  /* Pass on the message to the next hooker */
  return CallNextHookEx( pHookItem->CallWndProc_hHook, code, wParam, lParam );

#else
  UNIMPLEMENTED;
  return 0;
#endif
}

/*************************************************************************
 * OLEMenu_GetMsgProc
 * Thread scope WH_GETMESSAGE hook proc filter function (callback)
 * This is invoked from a message hook installed in OleSetMenuDescriptor.
 */
LRESULT CALLBACK OLEMenu_GetMsgProc(INT code, WPARAM wParam, LPARAM lParam)
{
  LPMSG pMsg = NULL;
  HOLEMENU hOleMenu = 0;
  OleMenuDescriptor *pOleMenuDescriptor = NULL;
  OleMenuHookItem *pHookItem = NULL;
  WORD wCode;
  
  Print(MAX_TRACE, ("%i, %04x, %08x\n", code, wParam, (unsigned)lParam ));

  /* Check if we're being asked to process a  messages */
  if ( HC_ACTION != code )
    goto NEXTHOOK;
      
  /* Retrieve the current message being dispatched from lParam */
  pMsg = (LPMSG)lParam;

  /* Check if the message is destined for a window we are interested in:
   * If the window has an OLEMenu property we may need to dispatch
   * the menu message to its active objects window instead. */

  hOleMenu = (HOLEMENU)GetPropA( pMsg->hwnd, "PROP_OLEMenuDescriptor" );
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
  pOleMenuDescriptor = (OleMenuDescriptor *) GlobalLock( hOleMenu );
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
    Print(MID_TRACE, ("could not retrieve hHook for current thread!\n" ));
    return FALSE;
  }
  
  /* Pass on the message to the next hooker */
  return CallNextHookEx( pHookItem->GetMsg_hHook, code, wParam, lParam );
}

/***********************************************************************
 * OleCreateMenuDescriptor [OLE32.97]
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

  pOleMenuDescriptor = (OleMenuDescriptor *) GlobalLock( hOleMenu );
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
 * OleDestroyMenuDescriptor [OLE32.99]
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
 * OleSetMenuDescriptor [OLE32.129]
 * Installs or removes OLE dispatching code for the containers frame window
 * FIXME: The lpFrame and lpActiveObject parameters are currently ignored
 * OLE should install context sensitive help F1 filtering for the app when
 * these are non null.
 * 
 * PARAMS:
 *     hOleMenu         Handle to composite menu descriptor
 *     hwndFrame        Handle to containers frame window
 *     hwndActiveObject Handle to objects in-place activation window
 *     lpFrame          Pointer to IOleInPlaceFrame on containers window
 *     lpActiveObject   Pointer to IOleInPlaceActiveObject on active in-place object
 *
 * RETURNS:
 *      S_OK                               - menu installed correctly
 *      E_FAIL, E_INVALIDARG, E_UNEXPECTED - failure
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
     Print(MIN_TRACE, ("(%x, %x, %x, %p, %p), Context sensitive help filtering not implemented!\n",
	(unsigned int)hOleMenu,
	hwndFrame,
	hwndActiveObject,
	lpFrame,
	lpActiveObject));
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
    pOleMenuDescriptor = (OleMenuDescriptor *) GlobalLock( hOleMenu );
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
 *              IsAccelerator        [OLE32.75]
 * Mostly copied from controls/menu.c TranslateAccelerator implementation
 */
BOOL WINAPI IsAccelerator(HACCEL hAccel, int cAccelEntries, LPMSG lpMsg, WORD* lpwCmd)
{
#if 0
    /* YES, Accel16! */
    LPACCEL16 lpAccelTbl;
    int i;

    if(!lpMsg) return FALSE;
    if (!hAccel || !(lpAccelTbl = (LPACCEL16)LockResource16(hAccel)))
    {
	WARN_(accel)("invalid accel handle=%04x\n", hAccel);
	return FALSE;
    }
    if((lpMsg->message != WM_KEYDOWN &&
	lpMsg->message != WM_KEYUP &&
	lpMsg->message != WM_SYSKEYDOWN &&
	lpMsg->message != WM_SYSKEYUP &&
	lpMsg->message != WM_CHAR)) return FALSE;

    Print(MAX_TRACE, ("hAccel=%04x, cAccelEntries=%d,"
		"msg->hwnd=%04x, msg->message=%04x, wParam=%08x, lParam=%08lx\n",
		hAccel, cAccelEntries,
		lpMsg->hwnd, lpMsg->message, lpMsg->wParam, lpMsg->lParam));
    for(i = 0; i < cAccelEntries; i++)
    {
	if(lpAccelTbl[i].key != lpMsg->wParam)
	    continue;

	if(lpMsg->message == WM_CHAR)
	{
	    if(!(lpAccelTbl[i].fVirt & FALT) && !(lpAccelTbl[i].fVirt & FVIRTKEY))
	    {
		Print(MAX_TRACE, ("found accel for WM_CHAR: ('%c')\n", lpMsg->wParam & 0xff));
		goto found;
	    }
	}
	else
	{
	    if(lpAccelTbl[i].fVirt & FVIRTKEY)
	    {
		INT mask = 0;
		Print(MAX_TRACE, ("found accel for virt_key %04x (scan %04x)\n",
				lpMsg->wParam, HIWORD(lpMsg->lParam) & 0xff));
		if(GetKeyState(VK_SHIFT) & 0x8000) mask |= FSHIFT;
		if(GetKeyState(VK_CONTROL) & 0x8000) mask |= FCONTROL;
		if(GetKeyState(VK_MENU) & 0x8000) mask |= FALT;
		if(mask == (lpAccelTbl[i].fVirt & (FSHIFT | FCONTROL | FALT))) goto found;
		Print(MAX_TRACE, ("incorrect SHIFT/CTRL/ALT-state\n"));
	    }
	    else
	    {
		if(!(lpMsg->lParam & 0x01000000))  /* no special_key */
		{
		    if((lpAccelTbl[i].fVirt & FALT) && (lpMsg->lParam & 0x20000000))
		    {						       /* ^^ ALT pressed */
			Print(MAX_TRACE, ("found accel for Alt-%c\n", lpMsg->wParam & 0xff));
			goto found;
		    }
		}
	    }
	}
    }	

    WARN_(accel)("couldn't translate accelerator key\n");
    return FALSE;

found:
    if(lpwCmd) *lpwCmd = lpAccelTbl[i].cmd;
    return TRUE;
#else
  return FALSE;
#endif
}

/***********************************************************************
 * ReleaseStgMedium [OLE32.140]
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

      pmedium->u.hGlobal = 0;
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

      pmedium->u.lpszFileName = 0;
      break;
    }
    case TYMED_ISTREAM:
    {
      if (pmedium->u.pstm!=0)
      {
	IStream_Release(pmedium->u.pstm);
      }

      pmedium->u.pstm = 0;
      break;
    }
    case TYMED_ISTORAGE:
    {
      if (pmedium->u.pstg!=0)
      {
	IStorage_Release(pmedium->u.pstg);
      }

      pmedium->u.pstg = 0;
      break;
    }
    case TYMED_GDI:
    {
      if ( (pmedium->pUnkForRelease==0) && 
	   (pmedium->u.hGlobal!=0) )
	DeleteObject(pmedium->u.hGlobal);

      pmedium->u.hGlobal = 0;
      break;
    }
    case TYMED_MFPICT:
    {
      if ( (pmedium->pUnkForRelease==0) && 
	   (pmedium->u.hMetaFilePict!=0) )
      {
#if 0
	LPMETAFILEPICT pMP = GlobalLock(pmedium->u.hMetaFilePict);
	DeleteMetaFile(pMP->hMF);
#else
        Print(MIN_TRACE, ("Depending on MetaFile implementation\n"));
#endif
	GlobalUnlock(pmedium->u.hMetaFilePict);
	GlobalFree(pmedium->u.hMetaFilePict);
      }

      pmedium->u.hMetaFilePict = 0;
      break;
    }
    case TYMED_ENHMF:
    {
      if ( (pmedium->pUnkForRelease==0) && 
	   (pmedium->u.hEnhMetaFile!=0) )
      {
	DeleteEnhMetaFile(pmedium->u.hEnhMetaFile);
      }

      pmedium->u.hEnhMetaFile = 0;
      break;
    }
    case TYMED_NULL:
    default:
      break;
  }

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
static void OLEDD_Initialize()
{
    WNDCLASS wndClass;

    ZeroMemory (&wndClass, sizeof(WNDCLASS));
    wndClass.style         = CS_GLOBALCLASS;
    wndClass.lpfnWndProc   = (WNDPROC)OLEDD_DragTrackerWindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(TrackerWindowInfo*);
    wndClass.hCursor       = 0;
    wndClass.hbrBackground = 0;
    wndClass.lpszClassName = OLEDD_DRAGTRACKERCLASS;
 
    RegisterClassA (&wndClass);
}

/***
 * OLEDD_UnInitialize()
 *
 * Releases the OLE drag and drop data structures.
 */
static void OLEDD_UnInitialize()
{
  /*
   * Simply empty the list.
   */
  while (targetListHead!=NULL)
  {
    RevokeDragDrop(targetListHead->hwndTarget);
  }
}

/***
 * OLEDD_InsertDropTarget()
 *
 * Insert the target node in the tree.
 */
static void OLEDD_InsertDropTarget(DropTargetNode* nodeToAdd)
{
  DropTargetNode*  curNode;
  DropTargetNode** parentNodeLink;

  /*
   * Iterate the tree to find the insertion point.
   */
  curNode        = targetListHead;
  parentNodeLink = &targetListHead;

  while (curNode!=NULL)
  {
    if (nodeToAdd->hwndTarget<curNode->hwndTarget)
    {
      /*
       * If the node we want to add has a smaller HWND, go left
       */
      parentNodeLink = &curNode->prevDropTarget;
      curNode        =  curNode->prevDropTarget;
    }
    else if (nodeToAdd->hwndTarget>curNode->hwndTarget)
    {
      /*
       * If the node we want to add has a larger HWND, go right
       */
      parentNodeLink = &curNode->nextDropTarget;
      curNode        =  curNode->nextDropTarget;
    }
    else
    {
      /*
       * The item was found in the list. It shouldn't have been there
       */
      assert(FALSE);
      return;
    }
  }

  /*
   * If we get here, we have found a spot for our item. The parentNodeLink
   * pointer points to the pointer that we have to modify. 
   * The curNode should be NULL. We just have to establish the link and Voila!
   */
  assert(curNode==NULL);
  assert(parentNodeLink!=NULL);
  assert(*parentNodeLink==NULL);

  *parentNodeLink=nodeToAdd;
}

/***
 * OLEDD_ExtractDropTarget()
 *
 * Removes the target node from the tree.
 */
static DropTargetNode* OLEDD_ExtractDropTarget(HWND hwndOfTarget)
{
  DropTargetNode*  curNode;
  DropTargetNode** parentNodeLink;

  /*
   * Iterate the tree to find the insertion point.
   */
  curNode        = targetListHead;
  parentNodeLink = &targetListHead;

  while (curNode!=NULL)
  {
    if (hwndOfTarget<curNode->hwndTarget)
    {
      /*
       * If the node we want to add has a smaller HWND, go left
       */
      parentNodeLink = &curNode->prevDropTarget;
      curNode        =  curNode->prevDropTarget;
    }
    else if (hwndOfTarget>curNode->hwndTarget)
    {
      /*
       * If the node we want to add has a larger HWND, go right
       */
      parentNodeLink = &curNode->nextDropTarget;
      curNode        =  curNode->nextDropTarget;
    }
    else
    {
      /*
       * The item was found in the list. Detach it from it's parent and 
       * re-insert it's kids in the tree.
       */
      assert(parentNodeLink!=NULL);
      assert(*parentNodeLink==curNode);

      /*
       * We arbitrately re-attach the left sub-tree to the parent.
       */
      *parentNodeLink = curNode->prevDropTarget;

      /*
       * And we re-insert the right subtree
       */
      if (curNode->nextDropTarget!=NULL)
      {
	OLEDD_InsertDropTarget(curNode->nextDropTarget);
      }

      /*
       * The node we found is still a valid node once we complete
       * the unlinking of the kids.
       */
      curNode->nextDropTarget=NULL;
      curNode->prevDropTarget=NULL;

      return curNode;
    }
  }

  /*
   * If we get here, the node is not in the tree
   */
  return NULL;
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
   * Iterate the tree to find the HWND value.
   */
  curNode        = targetListHead;

  while (curNode!=NULL)
  {
    if (hwndOfTarget<curNode->hwndTarget)
    {
      /*
       * If the node we want to add has a smaller HWND, go left
       */
      curNode =  curNode->prevDropTarget;
    }
    else if (hwndOfTarget>curNode->hwndTarget)
    {
      /*
       * If the node we want to add has a larger HWND, go right
       */
      curNode =  curNode->nextDropTarget;
    }
    else
    {
      /*
       * The item was found in the list.
       */
      return curNode;
    }
  }

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
      LPCREATESTRUCT createStruct = (LPCREATESTRUCT)lParam;

      SetWindowLongA(hwnd, 0, (LONG)createStruct->lpCreateParams); 

      
      break;
    }
    case WM_MOUSEMOVE:
    {
      TrackerWindowInfo* trackerInfo = (TrackerWindowInfo*)GetWindowLongA(hwnd, 0);
      POINT            mousePos;

      /*
       * Get the current mouse position in screen coordinates.
       */
      mousePos.x = LOWORD(lParam);
      mousePos.y = HIWORD(lParam);
      ClientToScreen(hwnd, &mousePos);

      /*
       * Track the movement of the mouse.
       */
      OLEDD_TrackMouseMove(trackerInfo, mousePos, wParam);

      break;
    }
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
    {
      TrackerWindowInfo* trackerInfo = (TrackerWindowInfo*)GetWindowLongA(hwnd, 0);
      POINT            mousePos;

      /*
       * Get the current mouse position in screen coordinates.
       */
      mousePos.x = LOWORD(lParam);
      mousePos.y = HIWORD(lParam);
      ClientToScreen(hwnd, &mousePos);

      /*
       * Notify everyone that the button state changed
       * TODO: Check if the "escape" key was pressed.
       */
      OLEDD_TrackStateChange(trackerInfo, mousePos, wParam);

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
 *    mousePos    - Current position of the mouse in screen
 *                  coordinates.
 *    keyState    - Contains the state of the shift keys and the
 *                  mouse buttons (MK_LBUTTON and the like)
 */
static void OLEDD_TrackMouseMove(
  TrackerWindowInfo* trackerInfo,
  POINT            mousePos,
  DWORD              keyState)
{
  HWND   hwndNewTarget = 0;
  HRESULT  hr = S_OK;

  /*
   * Get the handle of the window under the mouse
   */
  hwndNewTarget = WindowFromPoint(mousePos);

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
       (trackerInfo->curDragTargetHWND==hwndNewTarget) )
  {
    POINTL  mousePosParam;
    
    /*
     * The documentation tells me that the coordinate should be in the target
     * window's coordinate space. However, the tests I made tell me the
     * coordinates should be in screen coordinates.
     */
    mousePosParam.x = mousePos.x;
    mousePosParam.y = mousePos.y;
    
    IDropTarget_DragOver(trackerInfo->curDragTarget,
			 keyState,
			 mousePosParam,
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
	POINTL  mousePosParam;
	
	/*
	 * The documentation tells me that the coordinate should be in the target
	 * window's coordinate space. However, the tests I made tell me the
	 * coordinates should be in screen coordinates.
	 */
	mousePosParam.x = mousePos.x;
	mousePosParam.y = mousePos.y;
	
	IDropTarget_DragEnter(trackerInfo->curDragTarget,
			      trackerInfo->dataObject,
			      keyState,
			      mousePosParam,
			      trackerInfo->pdwEffect);
      }
    }
    else
    {
      /*
       * The mouse is not over a window so we don't track anything.
       */
      trackerInfo->curDragTargetHWND = 0;
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
      SetCursor(LoadCursorA(OLE32_hInstance, MAKEINTRESOURCE(1)));
    }
    else if (*trackerInfo->pdwEffect & DROPEFFECT_COPY)
    {
      SetCursor(LoadCursorA(OLE32_hInstance, MAKEINTRESOURCE(2)));
    }
    else if (*trackerInfo->pdwEffect & DROPEFFECT_LINK)
    {
      SetCursor(LoadCursorA(OLE32_hInstance, MAKEINTRESOURCE(3)));
    }
    else
    {
      SetCursor(LoadCursorA(OLE32_hInstance, MAKEINTRESOURCE(0)));
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
 *    mousePos    - Current position of the mouse in screen
 *                  coordinates.
 *    keyState    - Contains the state of the shift keys and the
 *                  mouse buttons (MK_LBUTTON and the like)
 */
static void OLEDD_TrackStateChange(
  TrackerWindowInfo* trackerInfo,
  POINT            mousePos,
  DWORD              keyState)
{
  /*
   * Ask the drop source what to do with the operation.
   */
  trackerInfo->returnValue = IDropSource_QueryContinueDrag(
			       trackerInfo->dropSource,
			       trackerInfo->escPressed, 
			       keyState);
  
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
	  POINTL  mousePosParam;
	
	  /*
	   * The documentation tells me that the coordinate should be 
	   * in the target window's coordinate space. However, the tests
	   * I made tell me the coordinates should be in screen coordinates.
	   */
	  mousePosParam.x = mousePos.x;
	  mousePosParam.y = mousePos.y;
	  
	  IDropTarget_Drop(trackerInfo->curDragTarget,
			   trackerInfo->dataObject,
			   keyState,
			   mousePosParam,
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
static DWORD OLEDD_GetButtonState()
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
 * OleMetaFilePictFromIconAndLabel
 *
 * Returns a global memory handle to a metafile which contains the icon and
 * label given.
 * I guess the result of that should look somehow like desktop icons.
 * If no hIcon is given, we load the icon via lpszSourceFile and iIconIndex.
 * This code might be wrong at some places.
 */
DWORD WINAPI OleMetaFilePictFromIconAndLabel16(
	DWORD hIcon,
	PVOID lpszLabel,
	PVOID lpszSourceFile,
	DWORD iIconIndex
) {
  UNIMPLEMENTED;
  return 0;
}

/******************************************************************************
 * DllDebugObjectRPCHook
 * turns on and off internal debugging,  pointer is only used on macintosh
 */

BOOL WINAPI DllDebugObjectRPCHook(BOOL b, void *dummy)
{
  Print(MIN_TRACE, ("stub\n"));
  return TRUE;
}

#endif
