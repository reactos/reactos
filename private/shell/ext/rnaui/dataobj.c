//****************************************************************************
//
//  Module:     RNAUI.DLL
//  File:       dataobj.c
//  Content:    This file contains DataObject and DropTarget interfaces.
//              objects.
//  History:
//      Wed 03-Jan-1996 09:21:10  -by-  Viroon  Touranachun [viroont]
//
//  Copyright (c) Microsoft Corporation 1991-1996
//
//****************************************************************************

#include "rnaui.h"
#include "contain.h"
#include "subobj.h"

//===========================================================================

#define EXPORT_FILE_EXT ".DUN"

typedef struct {
    IDataObject	dtobj;
    UINT cRef;
    DWORD cbData;
    DWORD cbidl;
    DWORD idlOffset;
    LPSTR pszFileName;
    char szEntryName[RAS_MaxEntryName+1];
    char szPathName[MAX_PATH];
} CDataObj;

// registered clipboard formats
UINT g_cfFileContents = 0;
UINT g_cfFileGroupDescriptor = 0;
UINT g_cfIDList = 0;

#pragma data_seg(DATASEG_READONLY)
const char c_szFileContents[] = CFSTR_FILECONTENTS;	    // "FileContents"
const char c_szFileGroupDescriptor[] = CFSTR_FILEDESCRIPTOR;// "FileGroupDescriptor"
const char c_szIDList[]       = CFSTR_SHELLIDLIST;          // "Shell ID Array"
#pragma data_seg()

IDataObjectVtbl c_CDataObjVtbl;		// forward decl

extern ErrTbl const c_Import[3];

DWORD GetExportedFile(CDataObj *this)
{
  LPSTR pszPathName;
  DWORD cb;
  DWORD dwRet;

  // If we already have the exported file, do nothing
  //
  if (*this->szPathName != '\0')
    return ERROR_SUCCESS;

  // Extract the connection info
  //
  pszPathName = this->szPathName;
  cb = GetTempPath(MAX_PATH, pszPathName);
  if (*CharPrev(pszPathName, &pszPathName[cb]) != '\\')
  {
    lstrcat(pszPathName, "\\");
  };

  this->pszFileName = pszPathName + lstrlen(pszPathName);
  lstrcpy(this->pszFileName, this->szEntryName);
  lstrcat(this->pszFileName, EXPORT_FILE_EXT);

  if ((dwRet = RnaExportEntry(this->szEntryName, pszPathName)) == ERROR_SUCCESS)
  {
    HANDLE hFile;

    // Cached the file size
    //
    if ((hFile = CreateFile(pszPathName, GENERIC_READ, FILE_SHARE_READ,
                            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                            NULL)) != INVALID_HANDLE_VALUE)
    {
      this->cbData = GetFileSize(hFile, NULL);
      CloseHandle(hFile);
    }
    else
    {
      dwRet = GetLastError();
      DeleteFile(pszPathName);
      *pszPathName = '\0';
    };
  }
  else
  {
    *pszPathName = '\0';
  };
  return dwRet;
}

STDMETHODIMP CDataObj_CreateInstance(
    LPCITEMIDLIST pidlFolder,
    UINT cidl,
    LPCITEMIDLIST FAR * ppidl,
    IDataObject **ppdtobj)
{
    CDataObj *this;
    DWORD    cbidl;

    TRACE_MSG(TF_SUBOBJ, "Creating new data obj");

    cbidl = sizeof(CIDA)+sizeof(UINT)+ILGetSize(ppidl[0])+ILGetSize(pidlFolder);
    this = (CDataObj *)LocalAlloc(LPTR, sizeof(CDataObj) + cbidl);

    if (this)
    {
        DWORD cb;
        LPIDA pida;

        this->dtobj.lpVtbl = &c_CDataObjVtbl;
        this->cRef = 1;
        lstrcpyn(this->szEntryName, Subobj_GetName((PSUBOBJ)ppidl[0]),
                 sizeof(this->szEntryName));

        this->cbidl = cbidl;
        this->idlOffset = sizeof(*this);
        pida = (LPIDA)(((LPBYTE)(this))+this->idlOffset);
        pida->cidl = 1;
        pida->aoffset[0] = sizeof(CIDA)+sizeof(UINT);

        cb = ILGetSize(pidlFolder);
        pida->aoffset[1] = pida->aoffset[0]+cb;

        RtlMoveMemory(((LPBYTE)pida)+pida->aoffset[0], pidlFolder, cb);
        RtlMoveMemory(((LPBYTE)pida)+pida->aoffset[1], ppidl[0], ILGetSize(ppidl[0]));

        if (g_cfFileContents == 0)
        {
          g_cfFileContents = RegisterClipboardFormat(c_szFileContents);
          g_cfFileGroupDescriptor = RegisterClipboardFormat(c_szFileGroupDescriptor);
          g_cfIDList = RegisterClipboardFormat(c_szIDList);
        }

        *ppdtobj = &this->dtobj;
        return S_OK;
    }

    *ppdtobj = NULL;
    LocalFree(this);
    return E_OUTOFMEMORY;
}

STDMETHODIMP CDataObj_QueryInterface(IDataObject *pdtobj, REFIID riid, LPVOID * ppvObj)
{
    CDataObj *this = IToClass(CDataObj, dtobj, pdtobj);

    TRACE_MSG(TF_SUBOBJ, "Query interface for the data obj");

    if (IsEqualIID(riid, &IID_IDataObject) || IsEqualIID(riid, &IID_IUnknown))
    {
        *ppvObj = this;
        this->cRef++;
        return S_OK;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CDataObj_AddRef(IDataObject *pdtobj)
{
    CDataObj *this = IToClass(CDataObj, dtobj, pdtobj);

    this->cRef++;
    return this->cRef;
}

STDMETHODIMP_(ULONG) CDataObj_Release(IDataObject *pdtobj)
{
    CDataObj *this = IToClass(CDataObj, dtobj, pdtobj);

    this->cRef--;
    if (this->cRef > 0)
	return this->cRef;

    if (*this->szPathName != '\0')
    {
      DeleteFile(this->szPathName);
    };
    LocalFree((HLOCAL)this);

    return 0;
}

STDMETHODIMP CDataObj_GetData(IDataObject *pdtobj, FORMATETC *pformatetcIn, STGMEDIUM *pmedium)
{
    CDataObj *this = IToClass(CDataObj, dtobj, pdtobj);
    HRESULT hres = E_INVALIDARG;

    TRACE_MSG(TF_SUBOBJ, "Get data for the data obj");

    pmedium->hGlobal = NULL;
    pmedium->pUnkForRelease = NULL;

    if ((g_cfFileGroupDescriptor == pformatetcIn->cfFormat) &&
        (pformatetcIn->tymed & TYMED_HGLOBAL))
    {
        if (GetExportedFile(this) == ERROR_SUCCESS)
        {
            pmedium->hGlobal = GlobalAlloc(GPTR, sizeof(FILEGROUPDESCRIPTOR));
            if (pmedium->hGlobal)
            {
                #define pdesc ((FILEGROUPDESCRIPTOR *)pmedium->hGlobal)

                lstrcpy(pdesc->fgd[0].cFileName, this->pszFileName);

                // specify the file for our HGLOBAL since GlobalSize() will round up
                //
                pdesc->fgd[0].dwFlags = FD_FILESIZE;
                pdesc->fgd[0].nFileSizeLow = this->cbData;
                pdesc->cItems = 1;  // one file in this
                #undef pdesc

                pmedium->tymed = TYMED_HGLOBAL;
                hres = S_OK;
            }
            else
                hres = E_OUTOFMEMORY;
        }
        else
            hres = E_OUTOFMEMORY;
    }
    else if ((g_cfFileContents == pformatetcIn->cfFormat) &&
             (pformatetcIn->tymed & TYMED_HGLOBAL))
    {
        if (GetExportedFile(this) == ERROR_SUCCESS)
        {
            pmedium->hGlobal = GlobalAlloc(GPTR, this->cbData);
            if (pmedium->hGlobal)
            {
                HANDLE hFile;

                // Cached the file size
                //
                if ((hFile = CreateFile(this->szPathName, GENERIC_READ,
                                        FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                        FILE_ATTRIBUTE_NORMAL, NULL))
                     != INVALID_HANDLE_VALUE)
                {
                    DWORD cb;

                    ReadFile(hFile, pmedium->hGlobal, this->cbData, &cb, NULL);
                    CloseHandle(hFile);

                    pmedium->tymed = TYMED_HGLOBAL;
                    hres = S_OK;
                }
                else
                {
                    GlobalFree(pmedium->hGlobal);
                    hres = E_OUTOFMEMORY;
                };
            }
            else
                hres = E_OUTOFMEMORY;
        }
        else
            hres = E_OUTOFMEMORY;
    }
    else if ((g_cfIDList == pformatetcIn->cfFormat) &&
             (pformatetcIn->tymed & TYMED_HGLOBAL))
    {
        pmedium->hGlobal = GlobalAlloc(GPTR, this->cbidl);
    	if (pmedium->hGlobal)
	{
            RtlMoveMemory((LPBYTE)(pmedium->hGlobal),
                    ((LPBYTE)this)+this->idlOffset, this->cbidl);

            pmedium->tymed = TYMED_HGLOBAL;
            hres = S_OK;
	}
	else
            hres = E_OUTOFMEMORY;
    }
    return hres;
}

STDMETHODIMP CDataObj_GetDataHere(IDataObject *pdtobj, FORMATETC *pformatetc, STGMEDIUM *pmedium)
{
    // CDataObj *this = IToClass(CDataObj, dtobj, pdtobj);
    return E_NOTIMPL;
}

STDMETHODIMP CDataObj_QueryGetData(IDataObject *pdtobj, LPFORMATETC pformatetcIn)
{
    CDataObj *this = IToClass(CDataObj, dtobj, pdtobj);

    TRACE_MSG(TF_SUBOBJ, "Query get data for the data obj");

    if (pformatetcIn->cfFormat == g_cfFileContents ||
        pformatetcIn->cfFormat == g_cfFileGroupDescriptor ||
        pformatetcIn->cfFormat == g_cfIDList)
    {
        DebugMsg(DM_TRACE, "QueryGetData says S_OK");
	return S_OK;
    }
    else
    {
        return S_FALSE;
    }
}

STDMETHODIMP CDataObj_GetCanonicalFormatEtc(IDataObject *pdtobj, FORMATETC *pformatetc, FORMATETC *pformatetcOut)
{
    return DATA_S_SAMEFORMATETC;
}

STDMETHODIMP CDataObj_SetData(IDataObject *pdtobj, FORMATETC *pformatetc, STGMEDIUM *pmedium, BOOL fRelease)
{
    DebugMsg(DM_ERROR, "CShellLink_SetData not implemented");
    return E_NOTIMPL;
}

STDMETHODIMP CDataObj_EnumFormatEtc(IDataObject *pdtobj, DWORD dwDirection, LPENUMFORMATETC *ppenumFormatEtc)
{
    CDataObj *this = IToClass(CDataObj, dtobj, pdtobj);

    FORMATETC fmte[3] = {
        {g_cfFileContents, 	  NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
        {g_cfFileGroupDescriptor, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
        {g_cfIDList,              NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
    };

    return SHCreateStdEnumFmtEtc(ARRAYSIZE(fmte), fmte, ppenumFormatEtc);
}

STDMETHODIMP CDataObj_Advise(IDataObject *pdtobj, FORMATETC *pFormatetc, DWORD advf, LPADVISESINK pAdvSink, DWORD *pdwConnection)
{
    return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP CDataObj_Unadvise(IDataObject *pdtobj, DWORD dwConnection)
{
    return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP CDataObj_EnumAdvise(IDataObject *pdtobj, LPENUMSTATDATA *ppenumAdvise)
{
    return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP CDataObj_DidDragDrop(IDataObject *pdtobj, DWORD dwEffect)
{
    CDataObj *this = IToClass(CDataObj, dtobj, pdtobj);
    HRESULT  hres;

    switch (dwEffect)
    {
      case DROPEFFECT_MOVE:
      {
        //
        // Need to remove the connection object
        //
        PSUBOBJ pso;

        if (pso = Sos_FindItem(this->szEntryName))
        {
          Remote_DeleteObject(pso);
        };
        hres = S_OK;
        break;
      }

      default:
        hres = S_OK;
        break;
    };
    return hres;
}

#pragma data_seg(DATASEG_READONLY)
IDataObjectVtbl c_CDataObjVtbl = {
    CDataObj_QueryInterface,
    CDataObj_AddRef,
    CDataObj_Release,
    CDataObj_GetData,
    CDataObj_GetDataHere,
    CDataObj_QueryGetData,
    CDataObj_GetCanonicalFormatEtc,
    CDataObj_SetData,
    CDataObj_EnumFormatEtc,
    CDataObj_Advise,
    CDataObj_Unadvise,
    CDataObj_EnumAdvise
};
#pragma data_seg()


//===========================================================================
// CIDLDropTarget: class definition
//===========================================================================

typedef struct _CIDLDropTarget	// idldt
{
    IDropTarget		dropt;
    int			cRef;
    LPITEMIDLIST	pidl;			// IDList to the target folder
    HWND		hwndOwner;
    DWORD		grfKeyStateLast;	// for previous DragOver/Enter
    IDataObject *       pdtobj;
    DWORD               dwEffectLastReturned;   // stashed effect that's returned by base class's dragover
    LPDROPTARGET	pdtgAgr;		// aggregate drop target
    DWORD		dwData;			// DTID_*
} CIDLDropTarget, * LPIDLDROPTARGET;

#define DTID_HDROP	0x00000001L

//===========================================================================
// CIDLDropTarget: constructors
//===========================================================================

IDropTargetVtbl c_DropTargetVtbl;

STDMETHODIMP CIDLDropTarget_Create(HWND hwndOwner,
                                   LPCITEMIDLIST pidl, LPDROPTARGET *ppdropt)
{
    LPIDLDROPTARGET pidldt = (void*)LocalAlloc(LPTR, sizeof(CIDLDropTarget));

    TRACE_MSG(TF_SUBOBJ, "Creating drop-target object");

    if (pidldt)
    {
	pidldt->pidl = ILClone(pidl);
	if (pidldt->pidl)
	{
            pidldt->dropt.lpVtbl = &c_DropTargetVtbl;
	    pidldt->cRef = 1;
	    pidldt->hwndOwner = hwndOwner;

	    Assert(pidldt->pdtgAgr == NULL);

	    *ppdropt = &pidldt->dropt;

	    return S_OK;
	}
	else
	    LocalFree((HLOCAL)pidldt);
    }
    *ppdropt = NULL;
    return E_OUTOFMEMORY;
}

//===========================================================================
// CIDLDropTarget: member function
//===========================================================================

STDMETHODIMP CIDLDropTarget_QueryInterface(LPDROPTARGET pdropt, REFIID riid, LPVOID *ppvObj)
{
    LPIDLDROPTARGET this = IToClass(CIDLDropTarget, dropt, pdropt);

    TRACE_MSG(TF_SUBOBJ, "Querying drop-target object interface");

    if (IsEqualIID(riid, 	&IID_IUnknown)
	|| IsEqualIID(riid, &IID_IDropTarget))
    {
	*ppvObj = pdropt;
	pdropt->lpVtbl->AddRef(pdropt);
	return S_OK;
    }

    *ppvObj = NULL;

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CIDLDropTarget_AddRef(LPDROPTARGET pdropt)
{
    LPIDLDROPTARGET this = IToClass(CIDLDropTarget, dropt, pdropt);
    this->cRef++;
    return this->cRef;
}

STDMETHODIMP_(ULONG) CIDLDropTarget_Release(LPDROPTARGET pdropt)
{
    LPIDLDROPTARGET this = IToClass(CIDLDropTarget, dropt, pdropt);

    this->cRef--;
    if (this->cRef > 0)
	return this->cRef;

    if (this->pdtgAgr)
        this->pdtgAgr->lpVtbl->Release(this->pdtgAgr);

    if (this->pidl)
	ILFree(this->pidl);

    // if we hit this a lot maybe we should just release it
    AssertMsg(this->pdtobj == NULL, "didn't get matching DragLeave");

    LocalFree((HLOCAL)this);
    return 0;
}

STDMETHODIMP CIDLDropTarget_DragEnter(LPDROPTARGET pdropt, LPDATAOBJECT pDataObj, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect)
{
    LPIDLDROPTARGET this = IToClass(CIDLDropTarget, dropt, pdropt);

    Assert(this->pdtobj == NULL);		// avoid a leak

    TRACE_MSG(TF_SUBOBJ, "drop-target object drag enter");

    this->grfKeyStateLast = grfKeyState;
    this->pdtobj = pDataObj;
    this->dwData = 0;

    if (pDataObj)
    {
	LPENUMFORMATETC penum;
        HRESULT hres;
        char    szFileName[MAX_PATH];

        pDataObj->lpVtbl->AddRef(pDataObj);
	hres = pDataObj->lpVtbl->EnumFormatEtc(pDataObj, DATADIR_GET, &penum);
	if (SUCCEEDED(hres))
	{
	    FORMATETC fmte;
	    LONG celt;

            *pdwEffect = 0;

	    while (penum->lpVtbl->Next(penum,1,&fmte,&celt)==S_OK)
	    {
            if (fmte.cfFormat==CF_HDROP && (fmte.tymed&TYMED_HGLOBAL)) 
            {
                STGMEDIUM stmd;

                if (pDataObj->lpVtbl->GetData(pDataObj, &fmte, &stmd) == S_OK) 
                {
                    if (DragQueryFile(stmd.hGlobal, 0, szFileName, sizeof(szFileName))) 
                    {
                        if (RnaValidateImportEntry(szFileName) == ERROR_SUCCESS)
                        {
                          this->dwData |= DTID_HDROP;
                          *pdwEffect = DROPEFFECT_COPY;
                        }
                    }

                    // GlobalFree(stmd->hGlobal);
                    ReleaseStgMedium(&stmd);
                }
            }
	    }
	    penum->lpVtbl->Release(penum);
	}

	DebugMsg(DM_TRACE, "sh TR - CIDL::DragEnter this->dwData = %x", this->dwData);
    }

    // stash this away
    this->dwEffectLastReturned = *pdwEffect;

    return S_OK;
}

// subclasses can prevent us from assigning in the dwEffect by not passing in pdwEffect
//
STDMETHODIMP CIDLDropTarget_DragOver(LPDROPTARGET pdropt, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect)
{
    LPIDLDROPTARGET this = IToClass(CIDLDropTarget, dropt, pdropt);

    TRACE_MSG(TF_SUBOBJ, "drop-target object drag over");

    this->grfKeyStateLast = grfKeyState;
    if (pdwEffect)
        *pdwEffect = this->dwEffectLastReturned;
    return S_OK;
}

STDMETHODIMP CIDLDropTarget_DragLeave(LPDROPTARGET pdropt)
{
    LPIDLDROPTARGET this = IToClass(CIDLDropTarget, dropt, pdropt);

    TRACE_MSG(TF_SUBOBJ, "drop-target object drag leave");

    if (this->pdtobj)
    {
	DebugMsg(DM_TRACE, "sh TR - CIDL::DragLeave called when pdtobj!=NULL (%x)", this->pdtobj);
        this->pdtobj->lpVtbl->Release(this->pdtobj);
	this->pdtobj = NULL;
    }

    return S_OK;
}

//
// TrackDropMenu
//
//  This function creates and tracks the right-button drop menu
//
DWORD TrackDropMenu(LPIDLDROPTARGET this, LPDWORD lpdwDropEffect)
{
    HMENU hmenu, hPopup;
    UINT  idCmd;
    POINT pt;
    DWORD dwRet = ERROR_OUTOFMEMORY;

    // Load the menu
    //
    hmenu = LoadMenu(ghInstance, MAKEINTRESOURCE(POPUP_DROP));
    if (hmenu)
    {
        hPopup = GetSubMenu (hmenu, 0);

        if (hPopup)
        {
          SetMenuDefaultItem(hPopup, IDM_DROP_COPY, MF_BYCOMMAND);
          GetCursorPos(&pt);
          idCmd = TrackPopupMenu(hPopup,
                                 TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
                                 pt.x, pt.y, 0, this->hwndOwner, NULL);
          switch (idCmd)
          {
              case IDM_DROP_COPY:
                  *lpdwDropEffect = DROPEFFECT_COPY;  break;

              case IDM_DROP_MOVE:
                  *lpdwDropEffect = DROPEFFECT_MOVE;  break;

              case IDM_DROP_CANCEL:
              default:
                  *lpdwDropEffect = 0;                break;
          };
        };
        DestroyMenu(hmenu);
    };

    return dwRet;
}

//
// CIDLDropTarget_Drop
//
//  This function creates a connection to a dropped net resource object.
//
STDMETHODIMP CIDLDropTarget_Drop(IDropTarget * pdropt, IDataObject * pDataObj, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect)
{
    LPIDLDROPTARGET this = IToClass(CIDLDropTarget, dropt, pdropt);
    HRESULT hres;

    TRACE_MSG(TF_SUBOBJ, "drop-target object drops");

    if ((this->dwData & DTID_HDROP) &&
        (*pdwEffect & (DROPEFFECT_COPY | DROPEFFECT_MOVE)))
    {
        FORMATETC fmte;
        STGMEDIUM stmd;
        char      szFileName[MAX_PATH];
        char      szEntryName[RAS_MaxEntryName+1];
        DWORD     dwDropEffect;

        if ((this->grfKeyStateLast & MK_LBUTTON) ||
            (!this->grfKeyStateLast && (*pdwEffect == DROPEFFECT_COPY)))
        {
          dwDropEffect = DROPEFFECT_COPY;
        }
        else
        {
          dwDropEffect = 0;
          TrackDropMenu(this, &dwDropEffect);
        };
        *pdwEffect = 0;

        if (dwDropEffect != 0)
        {
          // Get filename from the data object
          //
          ZeroMemory(&fmte, sizeof(fmte));
          fmte.cfFormat = CF_HDROP;
          fmte.tymed    = TYMED_HGLOBAL;
          if ((hres = this->pdtobj->lpVtbl->GetData(this->pdtobj, &fmte, &stmd))
              == S_OK)
          {
            // Import the file
            //
            hres = E_OUTOFMEMORY;
            if (DragQueryFile(stmd.hGlobal, 0, szFileName, sizeof(szFileName)))
            {
              DWORD dwRet;
              if (((dwRet = RnaImportEntry(szFileName, szEntryName,
                                           sizeof(szEntryName))) == ERROR_SUCCESS) ||
                  (dwRet == ERROR_ALREADY_EXISTS))
              {
                if (dwRet == ERROR_SUCCESS)
                {
                  PSUBOBJ pso;

                  // Create a new subobject with no name
                  //
                  if (Subobj_New(&pso, szEntryName, IDI_REMOTE, 0))
                  {
                    // Notify the event
                    //
                    Remote_GenerateEvent(SHCNE_CREATE, pso, NULL);
                    Subobj_Destroy(pso);
                  };
                };

                if (dwDropEffect == DROPEFFECT_MOVE)
                {
                  DeleteFile(szFileName);
                };

                *pdwEffect |= dwDropEffect;
                hres = S_OK;
              }
              else
              {
                ETMsgBox(NULL, IDS_CAP_REMOTE, dwRet, c_Import, ARRAYSIZE(c_Import));
              };
            };
            // GlobalFree(stmd.hGlobal);
            ReleaseStgMedium(&stmd);
          };
        }
        else
        {
          hres = S_OK;
        };
    }
    else
    {
        // if it was not files, we just say we moved the data, letting the
	// source deleted it. lets hope they support undo...

        *pdwEffect &= DROPEFFECT_COPY;
        hres = S_OK;
    }

    CIDLDropTarget_DragLeave(pdropt);

    return hres;
}

#pragma data_seg(DATASEG_READONLY)
IDropTargetVtbl c_DropTargetVtbl =
{
    CIDLDropTarget_QueryInterface,
    CIDLDropTarget_AddRef,
    CIDLDropTarget_Release,
    CIDLDropTarget_DragEnter,
    CIDLDropTarget_DragOver,
    CIDLDropTarget_DragLeave,
    CIDLDropTarget_Drop,
};
#pragma data_seg()
