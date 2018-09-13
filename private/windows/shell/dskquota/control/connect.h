#ifndef __CONNECTION_POINT_STUFF_H
#define __CONNECTION_POINT_STUFF_H
///////////////////////////////////////////////////////////////////////////////
/*  File: connect.h

    Description: Provides declarations required for the quota controller to
        support OLE connection points.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/19/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#ifndef _INC_DSKQUOTA_H
#   include "dskquota.h"
#endif
#ifndef _INC_DSKQUOTA_USER_H
#   include "user.h"       // MAX_USERNAME
#endif
#ifndef _INC_DSKQUOTA_OADISP_H
#   include "oadisp.h"     // OleAutoDispatch class.
#endif
#ifndef _INC_DSKQUOTA_DISPATCH_H
#   include "dispatch.h"
#endif

class ConnectionPoint : public IConnectionPoint, public IDispatch
{
    private:
        LONG          m_cRef;           // Class object ref count.
        DWORD         m_cConnections;   // Number of connections.
        DWORD         m_dwCookieNext;   // Next cookie value to hand out.
        LPUNKNOWN     m_pUnkContainer;  // IUnknown of ConnectionPointEnumerator
        REFIID        m_riid;           // Reference to IID supported by conn pt.
        HANDLE        m_hMutex;         
        OleAutoDispatch m_Dispatch;
        CArray<CONNECTDATA> m_ConnectionList;

        void Lock(void)
            { WaitForSingleObject(m_hMutex, INFINITE); }
        void ReleaseLock(void)
            { ReleaseMutex(m_hMutex); }

        //
        // Prevent copying.
        //
        ConnectionPoint(const ConnectionPoint&);
        void operator = (const ConnectionPoint&);


    public:
        ConnectionPoint(LPUNKNOWN pUnkContainer, REFIID riid);
        ~ConnectionPoint(void);


        //
        // IUnknown methods.
        //
        STDMETHODIMP
        QueryInterface(
            REFIID, 
            LPVOID *);

        STDMETHODIMP_(ULONG) 
        AddRef(
            VOID);

        STDMETHODIMP_(ULONG) 
        Release(
            VOID);

        //
        // IConnectionPoint methods.
        //
        STDMETHODIMP
        GetConnectionInterface(
            LPIID pIID);
   
        STDMETHODIMP
        GetConnectionPointContainer(
            PCONNECTIONPOINTCONTAINER *ppCPC);

        STDMETHODIMP
        Advise(
            LPUNKNOWN pUnkSink,
            LPDWORD pdwCookie);

        STDMETHODIMP
        Unadvise(
            DWORD dwCookie);

        STDMETHODIMP
        EnumConnections(
            PENUMCONNECTIONS *ppEnum);

        //
        // IDispatch methods.
        //
        STDMETHODIMP
        GetIDsOfNames(
            REFIID riid,  
            OLECHAR ** rgszNames,  
            UINT cNames,  
            LCID lcid,  
            DISPID *rgDispId);

        STDMETHODIMP
        GetTypeInfo(
            UINT iTInfo,  
            LCID lcid,  
            ITypeInfo **ppTInfo);

        STDMETHODIMP
        GetTypeInfoCount(
            UINT *pctinfo);

        STDMETHODIMP
        Invoke(
            DISPID dispIdMember,  
            REFIID riid,  
            LCID lcid,  
            WORD wFlags,  
            DISPPARAMS *pDispParams,  
            VARIANT *pVarResult,  
            EXCEPINFO *pExcepInfo,  
            UINT *puArgErr);
};


class ConnectionEnum : public IEnumConnections
{
    private:
        LONG         m_cRef;          // Object ref count.
        UINT         m_iCurrent;      // "Current" enum index.
        UINT         m_cConnections;  // Connection count.
        PCONNECTDATA m_rgConnections; // Array of connection info.
        LPUNKNOWN    m_pUnkContainer; // Connection pt container.

        //
        // Prevent assignment.
        //
        void operator = (const ConnectionEnum&);

    public:
        ConnectionEnum(
            LPUNKNOWN pUnkContainer,
            UINT cConnection, 
            PCONNECTDATA rgConnections);

        ConnectionEnum(
            const ConnectionEnum& refEnum);

        ~ConnectionEnum(void);

        HRESULT 
        Initialize(
            UINT cConnection, 
            PCONNECTDATA rgConnections);

        //
        // IUnknown methods.
        //
        STDMETHODIMP
        QueryInterface(
            REFIID, 
            LPVOID *);

        STDMETHODIMP_(ULONG) 
        AddRef(
            VOID);

        STDMETHODIMP_(ULONG) 
        Release(
            VOID);

        //
        // IEnumConnections methods.
        //
        STDMETHODIMP 
        Next(
            DWORD, 
            PCONNECTDATA, 
            LPDWORD);

        STDMETHODIMP 
        Skip(
            DWORD);

        STDMETHODIMP 
        Reset(
            VOID);

        STDMETHODIMP 
        Clone(
            PENUMCONNECTIONS *);
};


class ConnectionPointEnum : public IEnumConnectionPoints 
{
    private:
        LONG         m_cRef;           // Object ref count.
        UINT         m_iCurrent;       // "Current" enum index.
        UINT         m_cConnPts;       // Connection point count.
        PCONNECTIONPOINT *m_rgConnPts; // Array of connection info.
        LPUNKNOWN    m_pUnkContainer;  // IUnknown of DiskQuotaController.

        //
        // Prevent assignment.
        //
        void operator = (const ConnectionPointEnum&);

    public:
        ConnectionPointEnum(
            LPUNKNOWN pUnkContainer,
            UINT cConnPts, 
            PCONNECTIONPOINT *rgConnPts);

        ConnectionPointEnum(
            const ConnectionPointEnum& refEnum);

        ~ConnectionPointEnum(void);

        //
        // IUnknown methods.
        //
        STDMETHODIMP
        QueryInterface(
            REFIID, 
            LPVOID *);

        STDMETHODIMP_(ULONG) 
        AddRef(
            VOID);

        STDMETHODIMP_(ULONG) 
        Release(
            VOID);

        //
        // IEnumConnections methods.
        //
        STDMETHODIMP 
        Next(
            DWORD, 
            PCONNECTIONPOINT *, 
            LPDWORD);

        STDMETHODIMP 
        Skip(
            DWORD);

        STDMETHODIMP 
        Reset(
            VOID);

        STDMETHODIMP 
        Clone(
            PENUMCONNECTIONPOINTS *);
};


#endif // CONNECTION_POINT_STUFF_H
