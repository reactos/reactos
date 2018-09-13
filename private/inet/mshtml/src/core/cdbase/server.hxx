//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1993 - 1995.
//
//  File:       server.hxx
//
//  Contents:   CServer private definitions
//
//----------------------------------------------------------------------------

#ifndef I_SERVER_HXX_
#define I_SERVER_HXX_
#pragma INCMSG("--- Beg 'server.hxx'")

MtExtern(CDropTarget)

//+---------------------------------------------------------------------------
//  Class       CDropTarget
//
//  Synopsis:   A variation on tearoff. Created in order to provide
//              IDropTarget functionality by delegating all calls to
//              CServer. Created by IOleInPlaceObjectWindowless and
//              delegates vis _pServer. Maintains it's own qi() addref()
//              and release() and calls SubAddRef() and SubRelease() on
//              CServer. QI returns itself on IUnKnown and IDropTarget
//              and fails on everything else.
//
//----------------------------------------------------------------------------

class CDropTarget : public IDropTarget
{
    public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CDropTarget))

    CServer *_pServer;
    UINT    _ulRef;

    CDropTarget(CServer * pServer);
    ~CDropTarget();

    STDMETHOD(QueryInterface)(REFIID riid, void ** ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    STDMETHOD (DragEnter) (IDataObject * pIDataSource, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);
    STDMETHOD (DragOver) (DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);
    STDMETHOD (DragLeave) (void);
    STDMETHOD (Drop) (IDataObject * pIDataSource, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);
};

#pragma INCMSG("--- End 'server.hxx'")
#else
#pragma INCMSG("*** Dup 'server.hxx'")
#endif
