#ifndef _IDLCOMM_H_
#define _IDLCOMM_H_

#ifndef _SHELLP_H_
#include <shellp.h>
#endif

//===========================================================================
// HIDA -- IDList Array handle
//===========================================================================

STDAPI_(void) IDLData_InitializeClipboardFormats(void);

typedef HGLOBAL HIDA;

STDAPI_(HIDA) HIDA_Create(LPCITEMIDLIST pidlFolder, UINT cidl, LPCITEMIDLIST *apidl);
STDAPI_(void) HIDA_Free(HIDA hida);
STDAPI_(HIDA) HIDA_Clone(HIDA hida);
STDAPI_(UINT) HIDA_GetCount(HIDA hida);
STDAPI_(UINT) HIDA_GetIDList(HIDA hida, UINT i, LPITEMIDLIST pidlOut, UINT cbMax);

STDAPI_(LPITEMIDLIST) HIDA_ILClone(HIDA hida, UINT i);
STDAPI_(LPITEMIDLIST) HIDA_FillIDList(HIDA hida, UINT i, LPITEMIDLIST pidl);

//===========================================================================
// CIDLDropTarget: class definition
//===========================================================================

typedef struct _CIDLDropTarget  // idldt
{
#ifdef __cplusplus
    void *              dropt;
#else
    IDropTarget         dropt;
#endif
    int                 cRef;
    LPITEMIDLIST        pidl;                   // IDList to the target folder
    HWND                hwndOwner;
    DWORD               grfKeyStateLast;        // for previous DragOver/Enter
    IDataObject *       pdtobj;
    DWORD               dwEffectLastReturned;   // stashed effect that's returned by base class's dragover
    IDropTarget *       pdtgAgr;                // aggregate drop target
    DWORD               dwData;                 // DTID_*
    DWORD               dwEffectPreferred;      // if dwData & DTID_PREFERREDEFFECT
    TCHAR               szPath[MAX_PATH];       // optionl path for target folder
} CIDLDropTarget;

#define DTID_HDROP              0x00000001L
#define DTID_HIDA               0x00000002L
#define DTID_NETRES             0x00000004L
#define DTID_CONTENTS           0x00000008L
#define DTID_FDESCA             0x00000010L
#define DTID_OLEOBJ             0x00000020L
#define DTID_OLELINK            0x00000040L
#define DTID_FD_LINKUI          0x00000080L
#define DTID_FDESCW             0x00000100L
#define DTID_PREFERREDEFFECT    0x00000200L
#define DTID_EMBEDDEDOBJECT     0x00000400L

//===========================================================================
// CIDLDropTarget: member function prototypes
//===========================================================================

STDMETHODIMP CIDLDropTarget_QueryInterface(IDropTarget *pdropt, REFIID riid, void **ppvObj);
STDMETHODIMP_(ULONG) CIDLDropTarget_AddRef(IDropTarget *pdropt);
STDMETHODIMP_(ULONG) CIDLDropTarget_Release(IDropTarget *pdropt);
STDMETHODIMP CIDLDropTarget_DragEnter(IDropTarget *pdropt, IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
STDMETHODIMP CIDLDropTarget_DragOver(IDropTarget *pdropt, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
STDMETHODIMP CIDLDropTarget_DragLeave(IDropTarget *pdropt);
STDMETHODIMP CIDLDropTarget_Drop(IDropTarget *pdropt, IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
//
// This macro checks if pdtgt is a subclass of CIDLDropTarget.
// (HACK: We assume nobody overrides QueryInterface).
//
STDAPI_(BOOL) DoesDropTargetSupportDAD(IDropTarget *pdtgt);

//===========================================================================
// CIDLDropTarget: constructor prototype
//===========================================================================
#ifdef __cplusplus
//BUGBUG IDropTargetVtbl doesn't get defined in c++, make it void * for now
typedef void * IDropTargetVtbl;
#endif
STDAPI CIDLDropTarget_Create(HWND hwnd, const IDropTargetVtbl *lpVtbl, LPCITEMIDLIST pidl, IDropTarget ** ppdropt);
STDAPI CIDLDropTarget_Create2(HWND hwnd, const IDropTargetVtbl *lpVtbl, LPCTSTR pszPath, LPCITEMIDLIST pidl, IDropTarget **ppdropt);

//===========================================================================
// CIDLData : Member function prototypes
//===========================================================================
STDMETHODIMP CIDLData_QueryInterface(IDataObject *pdtobj, REFIID riid, void **ppvObj);
STDMETHODIMP_(ULONG) CIDLData_AddRef(IDataObject *pdtobj);
STDMETHODIMP_(ULONG) CIDLData_Release(IDataObject *pdtobj);
STDMETHODIMP CIDLData_GetData(IDataObject *pdtobj, FORMATETC *pformatetcIn, STGMEDIUM *pmedium );
STDMETHODIMP CIDLData_GetDataHere(IDataObject *pdtobj, FORMATETC *pformatetc, STGMEDIUM *pmedium );
STDMETHODIMP CIDLData_QueryGetData(IDataObject *pdtobj, FORMATETC *pformatetc);
STDMETHODIMP CIDLData_GetCanonicalFormatEtc(IDataObject *pdtobj, FORMATETC *pformatetc, FORMATETC *pformatetcOut);
STDMETHODIMP CIDLData_SetData(IDataObject *pdtobj, FORMATETC *pformatetc, STGMEDIUM  * pmedium, BOOL fRelease);
STDMETHODIMP CIDLData_EnumFormatEtc(IDataObject *pdtobj, DWORD dwDirection, LPENUMFORMATETC * ppenumFormatEtc);
STDMETHODIMP CIDLData_Advise(IDataObject *pdtobj, FORMATETC * pFormatetc, DWORD advf, LPADVISESINK pAdvSink, DWORD * pdwConnection);
STDMETHODIMP CIDLData_Unadvise(IDataObject *pdtobj, DWORD dwConnection);
STDMETHODIMP CIDLData_EnumAdvise(IDataObject *pdtobj, LPENUMSTATDATA * ppenumAdvise);

// idldata.c

//===========================================================================
// CIDLData : Constructor for subclasses
//===========================================================================

#ifdef __cplusplus
//BUGBUG IDataObjectVtbl doesn't get defined in c++, make it void * for now
typedef void * IDataObjectVtbl;
#endif
STDAPI CIDLData_CreateInstance(const IDataObjectVtbl *lpVtbl, IDataObject **ppdtobj, IDataObject *pdtInner);
STDAPI CIDLData_CreateFromIDArray2(const IDataObjectVtbl *lpVtbl, LPCITEMIDLIST pidlFolder,
                                    UINT cidl, LPCITEMIDLIST apidl[], IDataObject **ppdtobj);
STDAPI CIDLData_CreateFromIDArray3(const IDataObjectVtbl *lpVtbl, LPCITEMIDLIST pidlFolder,
                                    UINT cidl, LPCITEMIDLIST apidl[],
                                    IDataObject *pdtInner, IDataObject **ppdtobj);

//===========================================================================
// CIDLDropTarget : Drag & Drop helper
//===========================================================================
STDAPI CIDLDropTarget_DragDropMenu(CIDLDropTarget *_this,
                                    DWORD dwDefaultEffect,
                                    IDataObject * pdtobj,
                                    POINTL pt, DWORD *pdwEffect,
                                    HKEY hkeyProgID, HKEY hkeyBase,
                                    UINT idMenu, DWORD grfKeyState);

typedef struct _DRAGDROPMENUPARAM {     // ddm
    DWORD        dwDefEffect;
    IDataObject *pdtobj;
    POINTL       pt;
    DWORD *      pdwEffect;
    HKEY         hkeyProgID;
    HKEY         hkeyBase;
    UINT         idMenu;
    UINT         idCmd;
    DWORD        grfKeyState;
} DRAGDROPMENUPARAM, *LPDRAGDROPMENUPARAM;

STDAPI CIDLDropTarget_DragDropMenuEx(CIDLDropTarget *_this, LPDRAGDROPMENUPARAM pddm);
STDAPI CIDLDropTarget_GetPath(CIDLDropTarget *_this, LPTSTR pszPath);

#define MK_FAKEDROP 0x8000      // Real keys being held down?

//===========================================================================
// HDKA
//===========================================================================
//
// Struct:  ContextMenuInfo:
//
//  This data structure is used by FileView_DoContextMenu (and its private
// function, _AppendMenuItems) to handler multiple context menu handlers.
//
// History:
//  02-25-93 SatoNa     Created
//
typedef struct { // cmi
    IContextMenu  *pcm;
    UINT        idCmdFirst;
    UINT        idCmdMax;
    DWORD       dwCompat;
#ifdef DEBUG
    TCHAR       szKeyDebug[40]; // key name
#endif
} ContextMenuInfo;


//------------------------------------------------------------------------
// Dynamic class array
//

STDAPI_(int) DCA_AppendClassSheetInfo(HDCA hdca, HKEY hkeyProgID, LPPROPSHEETHEADER ppsh, IDataObject * pdtobj);

//===========================================================================
// HDXA
//===========================================================================
typedef HDSA    HDXA;   // hdma

#define HDXA_Create()   ((HDXA)DSA_Create(SIZEOF(ContextMenuInfo), 4))

STDAPI_(UINT) HDXA_AppendMenuItems(
                        HDXA hdxa, IDataObject * pdtobj,
                        UINT nKeys, HKEY *ahkeyClsKeys,
                        LPCITEMIDLIST pidlFolder,
                        HMENU hmenu, UINT uInsert,
                        UINT idCmdFirst,  UINT idCmdLast,
                        UINT fFlags,
                        HDCA hdca);
STDAPI_(UINT) HDXA_AppendMenuItems2(HDXA hdxa, IDataObject *pdtobj,
                        UINT nKeys, HKEY *ahkeyClsKeys,
                        LPCITEMIDLIST pidlFolder,
                        QCMINFO* pqcm,
                        UINT fFlags,
                        HDCA hdca,
                        IUnknown* pSite);

STDAPI HDXA_LetHandlerProcessCommandEx(HDXA hdxa, LPCMINVOKECOMMANDINFOEX pici, UINT_PTR * pidCmd);
STDAPI HDXA_GetCommandString(HDXA hdxa, UINT_PTR idCmd, UINT wFlags, UINT * pwReserved, LPSTR pszName, UINT cchMax);
STDAPI_(void) HDXA_DeleteAll(HDXA hdxa);
STDAPI_(void) HDXA_Destroy(HDXA hdxa);

//
// Clipboard Format for IDLData object.
//
#define ICFHDROP                         0
#define ICFFILENAME                      1
#define ICFNETRESOURCE                   2
#define ICFFILECONTENTS                  3
#define ICFFILEGROUPDESCRIPTORA          4
#define ICFFILENAMEMAPW                  5
#define ICFFILENAMEMAP                   6
#define ICFHIDA                          7
#define ICFOFFSETS                       8
#define ICFPRINTERFRIENDLYNAME           9
#define ICFPRIVATESHELLDATA             10
#define ICFHTML                         11
#define ICFFILENAMEW                    12
#define ICFFILEGROUPDESCRIPTORW         13
#define ICFPREFERREDDROPEFFECT          14
#define ICFPERFORMEDDROPEFFECT          15
#define ICFPASTESUCCEEDED               16
#define ICFSHELLURL                     17
#define ICFINDRAGLOOP                   18
#define ICF_DRAGCONTEXT                 19
#define ICF_TARGETCLSID                 20
#define ICF_EMBEDDEDOBJECT              21
#define ICF_OBJECTDESCRIPTOR            22
#define ICF_NOTRECYCLABLE               23
#define ICFLOGICALPERFORMEDDROPEFFECT   24
#define ICF_MAX                         25

EXTERN_C CLIPFORMAT g_acfIDLData[];

#define g_cfNetResource                 g_acfIDLData[ICFNETRESOURCE]
#define g_cfHIDA                        g_acfIDLData[ICFHIDA]
#define g_cfOFFSETS                     g_acfIDLData[ICFOFFSETS]
#define g_cfPrinterFriendlyName         g_acfIDLData[ICFPRINTERFRIENDLYNAME]
#define g_cfFileName                    g_acfIDLData[ICFFILENAME]
#define g_cfFileContents                g_acfIDLData[ICFFILECONTENTS]
#define g_cfFileGroupDescriptorA        g_acfIDLData[ICFFILEGROUPDESCRIPTORA]
#define g_cfFileGroupDescriptorW        g_acfIDLData[ICFFILEGROUPDESCRIPTORW]
#define g_cfFileNameMapW                g_acfIDLData[ICFFILENAMEMAPW]
#define g_cfFileNameMapA                g_acfIDLData[ICFFILENAMEMAP]
#define g_cfPrivateShellData            g_acfIDLData[ICFPRIVATESHELLDATA]
#define g_cfHTML                        g_acfIDLData[ICFHTML]
#define g_cfFileNameW                   g_acfIDLData[ICFFILENAMEW]
#define g_cfPreferredDropEffect         g_acfIDLData[ICFPREFERREDDROPEFFECT]
#define g_cfPerformedDropEffect         g_acfIDLData[ICFPERFORMEDDROPEFFECT]
#define g_cfLogicalPerformedDropEffect  g_acfIDLData[ICFLOGICALPERFORMEDDROPEFFECT]
#define g_cfPasteSucceeded              g_acfIDLData[ICFPASTESUCCEEDED]
#define g_cfShellURL                    g_acfIDLData[ICFSHELLURL]
#define g_cfInDragLoop                  g_acfIDLData[ICFINDRAGLOOP]
#define g_cfDragContext                 g_acfIDLData[ICF_DRAGCONTEXT]
#define g_cfTargetCLSID                 g_acfIDLData[ICF_TARGETCLSID]
#define g_cfEmbeddedObject              g_acfIDLData[ICF_EMBEDDEDOBJECT]
#define g_cfObjectDescriptor            g_acfIDLData[ICF_OBJECTDESCRIPTOR]
#define g_cfNotRecyclable               g_acfIDLData[ICF_NOTRECYCLABLE]

// Most places will only generate one so minimize the number of changes in the code (bad idea!)
#ifdef UNICODE
#define g_cfFileNameMap         g_cfFileNameMapW
#else
#define g_cfFileNameMap         g_cfFileNameMapA
#endif

STDAPI_(LPCITEMIDLIST) IDA_GetIDListPtr(LPIDA pida, UINT i);
STDAPI_(LPCITEMIDLIST) IDA_GetRelativeIDListPtr(LPIDA pida, UINT i, BOOL * pfAllocated);

#endif // _IDLCOMM_H_
