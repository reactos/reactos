#ifndef I_CONNECT_HXX_
#define I_CONNECT_HXX_
#pragma INCMSG("--- Beg 'connect.hxx'")

//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       connect.hxx
//
//  Contents:   Connection point implementation, shared amongst various
//              parts of Core.  Do not put in headers.hxx.
//
//----------------------------------------------------------------------------


class CConnectionPt;
class CConnectionPointContainer;

//+---------------------------------------------------------------------------
//
//  Class:      CConnectionPt (CCP)
//
//  Purpose:    Manages a connection.
//
//----------------------------------------------------------------------------

class CConnectionPt : public IConnectionPoint
{
public:
    // IUnknown methods
    STDMETHOD(QueryInterface)   (REFIID riid, LPVOID * ppvObj);
    STDMETHOD_(ULONG,AddRef)    ();
    STDMETHOD_(ULONG,Release)   ();

    // IConnectionPoint methods
    STDMETHOD(GetConnectionInterface)       (IID * pIID);
    STDMETHOD(GetConnectionPointContainer)  (
            IConnectionPointContainer ** ppCPC);

    STDMETHOD(Advise)                       (
            LPUNKNOWN pUnkSink, DWORD * pdwCookie);

    STDMETHOD(Unadvise)                     (DWORD dwCookie);
    STDMETHOD(EnumConnections)              (LPENUMCONNECTIONS * ppEnum);

    inline CConnectionPointContainer *      MyCPC();
    inline const CONNECTION_POINT_INFO *    MyCPI();

    int _index;
};



//+---------------------------------------------------------------------------
//
//  Class:      CConnectionPointContainer (CCPC)
//
//  Purpose:    Manages classes of connections.
//
//----------------------------------------------------------------------------

#define CONNECTION_POINTS_MAX 10     // Number of entries + 1

MtExtern(CConnectionPointContainer)

class CConnectionPointContainer : public IConnectionPointContainer
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CConnectionPointContainer))
    
    CConnectionPointContainer(CBase * pBase, IUnknown *pUnkOuter);
    ~CConnectionPointContainer();

    // IUnknown
    STDMETHOD(QueryInterface)   (REFIID riid, LPVOID * ppv);
    STDMETHOD_(ULONG, AddRef)   ();
    STDMETHOD_(ULONG, Release)  ();

    // IConnectionPointContainer methods
    STDMETHOD(EnumConnectionPoints) (IEnumConnectionPoints ** ppEnum);
    STDMETHOD(FindConnectionPoint)  (REFIID riid, IConnectionPoint ** ppCP);

    virtual CONNECTION_POINT_INFO * GetCPI();
    
    ULONG                           _ulRef;     // refcount
    CBase *                         _pBase;     // parent class to this class
    IUnknown *                      _pUnkOuter; // delegate QI to this object
    CConnectionPt                   _aCP[CONNECTION_POINTS_MAX];    // the connection points
};


inline CConnectionPointContainer *
CConnectionPt::MyCPC()
{
#ifdef WIN16
    // it just can't be that easy!! the Win16 compiler needs 
    // the field to be a constant expression, so we make
    // the field _aCP and change the adjust the 'this' pointer
    // passed to the macro
    char *pRoot = ((char *)this) - sizeof(CConnectionPt)*_index;
    return CONTAINING_RECORD(
            pRoot,
            CConnectionPointContainer,
            _aCP);
#else
    return CONTAINING_RECORD(
            this,
            CConnectionPointContainer,
            _aCP[_index]);
#endif
}



inline const CONNECTION_POINT_INFO *
CConnectionPt::MyCPI()
{
    return &(MyCPC()->GetCPI()[_index]);
}

#pragma INCMSG("--- End 'connect.hxx'")
#else
#pragma INCMSG("*** Dup 'connect.hxx'")
#endif
