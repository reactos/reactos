#ifndef __DESKADP__H
#define __DESKADP__H

typedef struct _DESKDISPLAYADAPTER
{
    const struct IShellPropSheetExtVtbl *lpIShellPropSheetExtVtbl;
    const struct IShellExtInitVtbl *lpIShellExtInitVtbl;
    const struct IClassFactoryVtbl *lpIClassFactoryVtbl;
    DWORD ref;

    HWND hwndDlg;
    PDESK_EXT_INTERFACE DeskExtInterface;
    IDataObject *pdtobj;
    LPTSTR lpDeviceId;
} DESKDISPLAYADAPTER, *PDESKDISPLAYADAPTER;

extern LONG dll_refs;

#define impl_to_interface(impl,iface) (struct iface *)(&(impl)->lp##iface##Vtbl)
#define interface_to_impl(instance,iface) ((PDESKDISPLAYADAPTER)((ULONG_PTR)instance - FIELD_OFFSET(DESKDISPLAYADAPTER,lp##iface##Vtbl)))

HRESULT
IDeskDisplayAdapter_Constructor(REFIID riid,
                                LPVOID *ppv);

VOID
IDeskDisplayAdapter_InitIface(PDESKDISPLAYADAPTER This);

HRESULT STDMETHODCALLTYPE
IDeskDisplayAdapter_QueryInterface(PDESKDISPLAYADAPTER This,
                                   REFIID iid,
                                   PVOID *pvObject);

ULONG
IDeskDisplayAdapter_AddRef(PDESKDISPLAYADAPTER This);

ULONG
IDeskDisplayAdapter_Release(PDESKDISPLAYADAPTER This);

HRESULT
IDeskDisplayAdapter_Initialize(PDESKDISPLAYADAPTER This,
                               LPCITEMIDLIST pidlFolder,
                               IDataObject *pdtobj,
                               HKEY hkeyProgID);

HRESULT
IDeskDisplayAdapter_AddPages(PDESKDISPLAYADAPTER This,
                             LPFNADDPROPSHEETPAGE pfnAddPage,
                             LPARAM lParam);

HRESULT
IDeskDisplayAdapter_ReplacePage(PDESKDISPLAYADAPTER This,
                                EXPPS uPageID,
                                LPFNADDPROPSHEETPAGE pfnReplacePage,
                                LPARAM lParam);

static const GUID CLSID_IDeskDisplayAdapter = {0x42071712,0x76d4,0x11d1,{0x8b,0x24,0x00,0xa0,0xc9,0x06,0x8f,0xf9}};

ULONG __cdecl DbgPrint(PCCH Format,...);

#endif /* __DESKADP__H */
