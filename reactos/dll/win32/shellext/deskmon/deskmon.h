#ifndef __DESKMON__H
#define __DESKMON__H

typedef struct _DESKMONINFO
{
    DISPLAY_DEVICE dd;
    struct _DESKMONINFO *Next;
} DESKMONINFO, *PDESKMONINFO;

typedef struct _DESKMONITOR
{
    const struct IShellPropSheetExtVtbl *lpIShellPropSheetExtVtbl;
    const struct IShellExtInitVtbl *lpIShellExtInitVtbl;
    const struct IClassFactoryVtbl *lpIClassFactoryVtbl;
    DWORD ref;

    HWND hwndDlg;
    PDESK_EXT_INTERFACE DeskExtInterface;
    IDataObject *pdtobj;
    LPTSTR lpDisplayDevice;
    DWORD dwMonitorCount;
    PDESKMONINFO Monitors;
    PDESKMONINFO SelMonitor;
    PDEVMODEW lpDevModeOnInit;
} DESKMONITOR, *PDESKMONITOR;

extern LONG dll_refs;

#define impl_to_interface(impl,iface) (struct iface *)(&(impl)->lp##iface##Vtbl)
#define interface_to_impl(instance,iface) ((PDESKMONITOR)((ULONG_PTR)instance - FIELD_OFFSET(DESKMONITOR,lp##iface##Vtbl)))

HRESULT
IDeskMonitor_Constructor(REFIID riid,
                         LPVOID *ppv);

VOID
IDeskMonitor_InitIface(PDESKMONITOR This);

HRESULT STDMETHODCALLTYPE
IDeskMonitor_QueryInterface(PDESKMONITOR This,
                            REFIID iid,
                            PVOID *pvObject);

ULONG
IDeskMonitor_AddRef(PDESKMONITOR This);

ULONG
IDeskMonitor_Release(PDESKMONITOR This);

HRESULT
IDeskMonitor_Initialize(PDESKMONITOR This,
                        LPCITEMIDLIST pidlFolder,
                        IDataObject *pdtobj,
                        HKEY hkeyProgID);

HRESULT
IDeskMonitor_AddPages(PDESKMONITOR This,
                      LPFNADDPROPSHEETPAGE pfnAddPage,
                      LPARAM lParam);

HRESULT
IDeskMonitor_ReplacePage(PDESKMONITOR This,
                         EXPPS uPageID,
                         LPFNADDPROPSHEETPAGE pfnReplacePage,
                         LPARAM lParam);

static const GUID CLSID_IDeskMonitor = {0x42071713,0x76d4,0x11d1,{0x8b,0x24,0x00,0xa0,0xc9,0x06,0x8f,0xf3}};

ULONG __cdecl DbgPrint(PCCH Format,...);

#endif /* __DESKMON__H */
