// iface.h : Definition of private interfaces


// {139D4EBC-DF7D-11d1-8661-00C04FD91972}
DEFINE_GUID(IID_IWorkerEvent, 0x139d4ebc, 0xdf7d, 0x11d1, 0x86, 0x61, 0x0, 0xc0, 0x4f, 0xd9, 0x19, 0x72);

// {EF4F9629-FC00-11d1-8677-00C04FD91972}
DEFINE_GUID(IID_IARPEvent, 0xef4f9629, 0xfc00, 0x11d1, 0x86, 0x77, 0x0, 0xc0, 0x4f, 0xd9, 0x19, 0x72);

// {C3E05A89-FBDB-11d1-8677-00C04FD91972}
DEFINE_GUID(IID_IARPSimpleProvider, 0xc3e05a89, 0xfbdb, 0x11d1, 0x86, 0x77, 0x0, 0xc0, 0x4f, 0xd9, 0x19, 0x72);

// {DB89BD6D-FCCD-11d1-8677-00C04FD91972}
DEFINE_GUID(IID_IAppData, 0xdb89bd6d, 0xfccd, 0x11d1, 0x86, 0x77, 0x0, 0xc0, 0x4f, 0xd9, 0x19, 0x72);

// {C2D3A971-FC11-11d1-8677-00C04FD91972}
DEFINE_GUID(IID_IMtxArray, 0xc2d3a971, 0xfc11, 0x11d1, 0x86, 0x77, 0x0, 0xc0, 0x4f, 0xd9, 0x19, 0x72);

// {AAEC4A45-FCCD-11d1-8677-00C04FD91972}
DEFINE_GUID(IID_IARPWorker, 0xaaec4a45, 0xfccd, 0x11d1, 0x86, 0x77, 0x0, 0xc0, 0x4f, 0xd9, 0x19, 0x72);


#ifndef __IFACE_H_
#define __IFACE_H_

#include "simpdata.h"       // for OLEDBSimpleProviderListener and DBROWCOUNT et al

// IWorkerEvent
//      This interface is used by CWorkerThread. /CWorkerThread calls 
//      IWorkerEvent methods to fire events.

#undef  INTERFACE
#define INTERFACE   IWorkerEvent

DECLARE_INTERFACE_(IWorkerEvent, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

    // *** IWorkerEvent methods ***
    STDMETHOD(FireOnDataReady)      (THIS_ DBROWCOUNT iRow) PURE;
    STDMETHOD(FireOnFinished)       (THIS) PURE;
    STDMETHOD(FireOnDatasetChanged) (THIS) PURE;
};


// IARPEvent
//      This interface is implemented by CEventBroker, and called
//      by anyone who wants to fire events to the databinding listeners.

#include "msdatsrc.h"

#undef  INTERFACE
#define INTERFACE   IARPEvent

DECLARE_INTERFACE_(IARPEvent, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

    // *** IARPEvent methods ***
    STDMETHOD(SetDataSourceListener)(THIS_ DataSourceListener *) PURE;
    STDMETHOD(IsOSPListener)        (THIS_ OLEDBSimpleProviderListener * posp) PURE;
    STDMETHOD(SetOSPListener)       (THIS_ OLEDBSimpleProviderListener * posp) PURE;
    STDMETHOD(AboutToDeleteRows)    (THIS_ DBROWCOUNT iRowStart, DBROWCOUNT cRows) PURE;
    STDMETHOD(DeletedRows)          (THIS_ DBROWCOUNT iRowStart, DBROWCOUNT cRows) PURE;
    STDMETHOD(RowsAvailable)        (THIS_ DBROWCOUNT iRowStart, DBROWCOUNT cRows) PURE;
    STDMETHOD(RowChanged)           (THIS_ DBROWCOUNT iRow) PURE;
    STDMETHOD(LoadCompleted)        (THIS) PURE;
    STDMETHOD(LoadAborted)          (THIS) PURE;
    STDMETHOD(DataSetChanged)       (THIS) PURE;
};


interface IMtxArray;        // forward reference


// IAppData
//      This provides an interface to an appdata object.

// commands for DoCommand()
typedef enum tagAPPCMD 
{
    APPCMD_UNKNOWN          = 0,
    APPCMD_INSTALL          = 1,        // "install"
    APPCMD_UNINSTALL        = 2,        // "uninstall"
    APPCMD_MODIFY           = 3,        // "modify"
    APPCMD_REPAIR           = 4,        // "repair"
    APPCMD_UPGRADE          = 5,        // "upgrade"
    APPCMD_GENERICINSTALL   = 6,        // "generic install" (install from floppy or CD)
    APPCMD_NTOPTIONS        = 7,        // "nt options"
    APPCMD_WINUPDATE        = 8,        // "update windows"
    APPCMD_ADDLATER         = 9,        // "add later"
} APPCMD;


#undef  INTERFACE
#define INTERFACE   IAppData

DECLARE_INTERFACE_(IAppData, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)    (THIS) PURE;
    STDMETHOD_(ULONG,Release)   (THIS) PURE;

    // *** IAppData ***
    STDMETHOD(DoCommand)        (THIS_ HWND hwndParent, APPCMD appcmd) PURE;
    STDMETHOD(ReadSlowData)     (THIS) PURE;
    STDMETHOD(GetVariant)       (THIS_ DB_LORDINAL iField, VARIANT * pvar) PURE;
    STDMETHOD(SetMtxParent)     (THIS_ IMtxArray * pmtxParent) PURE;
    STDMETHOD_(APPINFODATA *, GetDataPtr)(THIS) PURE;
    STDMETHOD_(SLOWAPPINFO *, GetSlowDataPtr)(THIS) PURE;
    STDMETHOD(GetFrequencyOfUse)(THIS_ LPWSTR pszBuf, int cchBuf) PURE;
    STDMETHOD(SetNameDupe)      (THIS_ BOOL bDupe) PURE;
};


// IMtxArray
//      This provides an interface to the matrix array, which the
//      handles the data for the data source object.

#undef  INTERFACE
#define INTERFACE   IMtxArray

DECLARE_INTERFACE_(IMtxArray, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)    (THIS) PURE;
    STDMETHOD_(ULONG,Release)   (THIS) PURE;

    // *** IMtxArray methods ***
    STDMETHOD(Initialize)       (THIS_ DWORD dwEnum) PURE;
    STDMETHOD(AddItem)          (THIS_ IAppData * pappdata, DBROWCOUNT * piRow) PURE;
    STDMETHOD(DeleteItems)      (THIS_ DBROWCOUNT iRow, DBROWCOUNT cRows) PURE;
    STDMETHOD(GetAppData)       (THIS_ DBROWCOUNT iRow, IAppData ** ppappdata) PURE;
    STDMETHOD(GetItemCount)     (THIS_ DBROWCOUNT * pcItems) PURE;
    STDMETHOD(GetFieldCount)    (THIS_ DB_LORDINAL * pcFields) PURE;
    STDMETHOD(GetFieldName)     (THIS_ DB_LORDINAL iField, VARIANT * pvar) PURE;
    STDMETHOD(GetSortIndex)     (THIS_ DWORD * pdwSort) PURE;
    STDMETHOD(SetSortCriteria)  (THIS_ LPCWSTR pszSortField) PURE;
    STDMETHOD(SortItems)        (THIS) PURE;
    STDMETHOD_(int,CompareItems)(THIS_ IAppData * pappdata1, IAppData * pappdata2) PURE;
    STDMETHOD(MarkDupEntries)   (void) PURE;    
};


// IARPWorker
//      This provides an interface to the matrix array's worker thread.

#undef  INTERFACE
#define INTERFACE   IARPWorker

DECLARE_INTERFACE_(IARPWorker, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)    (THIS) PURE;
    STDMETHOD_(ULONG,Release)   (THIS) PURE;

    // *** IARPWorker methods ***
    STDMETHOD(KillWT)           (THIS) PURE;
    STDMETHOD(StartWT)          (THIS_ int iPriority) PURE;
    STDMETHOD(SetListenerWT)    (THIS_ IWorkerEvent * pwe) PURE;
};


// IARPSimpleProvider
//      This provides an interface between ARP's OSP object and its
//      main control.

#undef  INTERFACE
#define INTERFACE   IARPSimpleProvider

DECLARE_INTERFACE_(IARPSimpleProvider, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)    (THIS) PURE;
    STDMETHOD_(ULONG,Release)   (THIS) PURE;

    // *** IARPSimpleProvider methods ***
    STDMETHOD(Initialize)       (THIS_ IShellAppManager * psam, IARPEvent * parpevt, DWORD dwEnum) PURE;
    STDMETHOD(EnumerateItemsAsync)   (THIS) PURE;
    STDMETHOD(Recalculate)      (THIS) PURE;
    STDMETHOD(SetSortCriteria)  (THIS_ BSTR bstrSortExpr) PURE;
    STDMETHOD(SetFilter)        (THIS_ BSTR bstrFilter) PURE;
    STDMETHOD(Sort)             (THIS) PURE;
    STDMETHOD(DoCommand)        (THIS_ HWND hwndParent, APPCMD appcmd, DBROWCOUNT iRow) PURE;
    STDMETHOD(TransferData)     (THIS_ IARPSimpleProvider * parposp) PURE;
};


#endif //__IFACE_H_
