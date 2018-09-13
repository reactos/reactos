/*
 * @(#)eventhelp.hxx 1.0 07/06/98
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. *
 * Classes for XML connection points
 * 
 */

#ifndef _EVENTHELP_HXX_
#define _EVENTHELP_HXX_


#define ISNULLIID(X) (X.Data1 == 0)

// Structure to keep track of connections 

typedef enum
{
    CP_Invalid,
    CP_Unknown,
    CP_Dispatch,
    CP_PropertyNotifySink
} CPTYPE;

typedef _tsreference<IUnknown> TSRUnknown;
typedef _tsreference<IDispatch> TSRDispatch;
typedef _tsreference<IPropertyNotifySink> TSRPropertyNotifySink;

typedef struct tagCPNode
{
    // Type of interface pointer (CP_XXXX)
    CPTYPE cpt;

    // IUnknown
    TSRUnknown punkConnector;
    // IDispatch
    TSRDispatch pdispConnector;  
    // IPropertyNotifySink
    TSRPropertyNotifySink ppnsConnector;

    // Pointer to next node
    struct tagCPNode *pNext;

#ifdef _WIN64
    // For Win64, we can't use the pointer to the node as the cookie, 
    // since the cookie remains 32 bits, while the pointer is now 64 bits.
    // So instead, we store a DWORD cookie per node
    DWORD dwCookie;
#endif // _WIN64

#ifdef _DEBUG
    // ThreadID of the thread on which the Advise was done
    DWORD dwThreadId;
#endif

    ~tagCPNode()
        {
            punkConnector = NULL;
            pdispConnector = NULL;
            ppnsConnector = NULL;
        }

} CPNODE, *PCPNODE;

// What outgoing interfaces do we support 
typedef struct tagCPCInfo
{
    IID iidOutgoingInterface;
    CPTYPE cpt;
} CPCINFO;


// Helper functions

HRESULT 
FireEventThroughCP(
    DISPID dispid,
    PCPNODE pConnections,
    LONG_PTR *plSpinLock,
    ...);

HRESULT 
FireEventWithNoArgsThroughCP(
    DISPID dispid,
    PCPNODE pConnections,
    ULONG_PTR *plSpinLock);

void 
ReleaseCPNODEList(
    PCPNODE pRootNode);

HRESULT 
FireEventThroughInvoke0(
    VARIANT *pvarRes,
    IDispatch *pDisp,
    IDispatch *pSelf,
    ...);



/////////////////////////////////////////////////////////////////
// CXMLConnectionPt - implements IConnectionPoint, fires events //
/////////////////////////////////////////////////////////////////

class CXMLConnectionPt : 
    public _unknown<IConnectionPoint, &IID_IConnectionPoint>
{
private:
    PCPNODE *_ppRootNode;
    IUnknown *_punkHost;
    ULONG_PTR *_plSpinLock;
    REFIID _EventIID;
    CPTYPE _cpt;

public:
    CXMLConnectionPt(REFIID iid, IUnknown *punkHost, PCPNODE *ppCPList, ULONG_PTR *plSpinLock, CPTYPE cptPreferredInterface);
    virtual ~CXMLConnectionPt();
    
    ///////////////////////////////////////////////////////////
    // IConnectionPoint Interface
    ///////////////////////////////////////////////////////////
public:
    HRESULT STDMETHODCALLTYPE GetConnectionInterface(IID *pIID);
    HRESULT STDMETHODCALLTYPE GetConnectionPointContainer(IConnectionPointContainer **ppCPC);
    HRESULT STDMETHODCALLTYPE Advise(IUnknown *pUnkSink, DWORD *pdwCookie);
    HRESULT STDMETHODCALLTYPE Unadvise(DWORD dwCookie);
    HRESULT STDMETHODCALLTYPE EnumConnections(IEnumConnections **ppEnum);
};


class CXMLConnectionPtContainer : public _unknown<IConnectionPointContainer, &IID_IConnectionPointContainer>
{
private:
    IUnknown *_punkOuter;
    PCPNODE *_ppNodeList;
    ULONG_PTR *_plSpinLock;
    IID _EventIID;

public:
    CXMLConnectionPtContainer(REFIID iid, IUnknown *pUnk, PCPNODE *ppNodeList, ULONG_PTR *plSpinLock);
    virtual ~CXMLConnectionPtContainer();
    CXMLConnectionPt* CreateConnectionPoint(REFIID riid,CPTYPE cpt);

public:
    // Over-ride QI
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void ** ppvObject);

        ////////////////////////////////////////////////
    // IConnectionPointContainer methods
    ////////////////////////////////////////////////

    HRESULT STDMETHODCALLTYPE EnumConnectionPoints(IEnumConnectionPoints  **ppEnum);
    HRESULT STDMETHODCALLTYPE FindConnectionPoint(REFIID riid, IConnectionPoint **ppCP);
};

//////////////////////////////////////////////////////////////////////////
// CXMLEnumConnectionPt
//////////////////////////////////////////////////////////////////////////

class CXMLEnumConnectionPt : 
    public _unknown<IEnumConnectionPoints,&IID_IEnumConnectionPoints>
{
private:
    CXMLConnectionPtContainer* _pCPC;
    CPCINFO _Interfaces[2];
    UINT _iIndex;
    
public:
    CXMLEnumConnectionPt(REFIID iid, CXMLConnectionPtContainer *pCPC);
    virtual ~CXMLEnumConnectionPt();

public:
    ///////////////////////////////////////////////////
    // IEnumConnectionPoints methods
    ///////////////////////////////////////////////////
    
    HRESULT STDMETHODCALLTYPE Next(ULONG cConnections, LPCONNECTIONPOINT *ppCP, ULONG *pcFetched);
    HRESULT STDMETHODCALLTYPE Skip(ULONG cConnections);
    HRESULT STDMETHODCALLTYPE Reset();
    HRESULT STDMETHODCALLTYPE Clone(IEnumConnectionPoints **ppEnum);

};


//////////////////////////////////////////////////////////////////////////
// CXMLEnumConnections
//////////////////////////////////////////////////////////////////////////

class CXMLEnumConnections : 
    public _unknown<IEnumConnections, &IID_IEnumConnections>
{
private:
    PCPNODE _pRootNode;
    PCPNODE _pCurrentConnection;
    IUnknown *_punkObj;
    ULONG_PTR *_plSpinLock;
    IID _EventIID;

private:
    HRESULT CopyConnectionList(PCPNODE pRootNode);
    
public:
    CXMLEnumConnections(REFIID iid, PCPNODE pRootNode, IUnknown *punkParent, ULONG_PTR *plSpinLock);
    virtual ~CXMLEnumConnections();
    
public:

    HRESULT STDMETHODCALLTYPE Next(ULONG cConnections, LPCONNECTDATA rgcd, ULONG *pcFetched);
    HRESULT STDMETHODCALLTYPE Skip(ULONG cConnections);
    HRESULT STDMETHODCALLTYPE Reset();
    HRESULT STDMETHODCALLTYPE Clone(IEnumConnections **ppEnum);
};


#endif // _EVENTHELP_HXX_
