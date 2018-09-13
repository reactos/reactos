//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       krnlundo.hxx
//
//  Contents:   Form Undo classes
//
//----------------------------------------------------------------------------

#ifndef I_KRNLUNDO_HXX_
#define I_KRNLUNDO_HXX_
#pragma INCMSG("--- Beg 'krnlundo.hxx'")

#ifndef X_UNDO_HXX_
#define X_UNDO_HXX_
#include "undo.hxx"
#endif

//+---------------------------------------------------------------------------
//
//  Class:      CUndoDeleteControl (UDC)
//
//  Purpose:    Undo action to undo deleting a control from the form.
//
//----------------------------------------------------------------------------

class CUndoDeleteControl : public CUndoUnitBase
{
public:
    CUndoDeleteControl(CSite * pSite);
   ~CUndoDeleteControl();

    HRESULT Init(int c, CSite ** ppSite);

    STDMETHOD(Do)(IOleUndoManager * pUndoMgr);

    CSite * Site() { return (CSite *) _pBase; }

    IDataObject *_pDOBag;   // Pointer to Object Bag
};

//+---------------------------------------------------------------------------
//
//  Class:      CUndoNewControl (UNC)
//
//  Purpose:    Undo action to undo adding a new control to the form.
//
//----------------------------------------------------------------------------

class CUndoNewControl : public CUndoUnitBase
{
public:
    CUndoNewControl(CSite * pSite, CSite * pSiteNew);
    CUndoNewControl(CSite * pSite, int c, CSite ** ppSite);

    STDMETHOD(Do)(IOleUndoManager * pUndoMgr);

    CSite * Site() { return (CSite *) _pBase; }

    CPtrAry<CSite *>    _arySites;
};

//+---------------------------------------------------------------------------
//
//  Class:      CUndoMove (UNM)
//
//  Purpose:    Undo action to undo moving a control on the form.
//
//----------------------------------------------------------------------------

class CUndoMove: public CUndoUnitBase
{
public:
    CUndoMove(CSite * pSite, RECT *prc, DWORD dwFlags);

    STDMETHOD(Do)(IOleUndoManager * pUndoMgr);

    CSite * Site() { return (CSite *) _pBase; }

    RECT   _rc;
    DWORD  _dwFlags;
};

#if 0
//+---------------------------------------------------------------------------
//
//  Class:      CUndoActivate (undoact)
//
//  Purpose:    Encapsulates the OLE Undo behavior for being an embedded
//              object that supports undo.
//
//  Interface:  CUndoActivate -- ctor
//             ~CUndoActivate -- calls DiscardUndoState on our client site
//              Undo          -- Calls DeactivateAndUndo on our client site
//
//  History:    02-Jun-94     LyleC    Created
//
//  Notes:      An instance of this class is created everytime the form
//              goes UI Active so that the user can select undo and we will
//              correctly call DeactivateAndUndo on our client site, as
//              specified in the OLE contract.  Note that this class should
//              be used in both run mode and design mode.
//
//----------------------------------------------------------------------------

class CUndoActivate : public CUndoUnitBase
{
public:
    CUndoActivate(CSite * pSite, LPOLEINPLACESITE pSite);
    virtual ~CUndoActivate(void);

    virtual HRESULT Undo(void);

private:
    LPOLEINPLACESITE _pIPSite;   // Un-addref'd pointer
};


//+---------------------------------------------------------------------------
//
//  Class:      CUndoDeactivate (undodeact)
//
//  Purpose:    Encapsulates the OLE Undo behavior for dealing with an
//              embedded object.
//
//  Interface:  CUndoDeactivate -- ctor, stores the site which just deactivated
//             ~CUndoDeactivate -- calls DoVerb(...DISCARDUNDOSTATE) on _pSite
//              Undo            -- Calls ReactivateAndUndo on the site
//
//  History:    02-Jun-94     LyleC    Created
//
//  Notes:      An instance of this object is created anytime an embedding
//              is UI deactivated, so that we correctly fulfill the OLE Undo
//              contract w/r/t embedded objects.  Note that this class should
//              be used in both run mode and design mode.
//
//----------------------------------------------------------------------------

class CUndoDeactivate : public CUndoUnitBase
{
public:
    CUndoDeactivate(CSite * pSite);
    virtual ~CUndoDeactivate(void);

    virtual HRESULT Undo(void);

private:
    CSite * _pSite;   // Un-addref'd pointer
};


//+---------------------------------------------------------------------------
//
//  Class:      CUndoEditDelete (undodel)
//
//  Purpose:    Undo class for the Edit-Delete action
//
//  Interface:  CUndoEditDelete -- ctor
//             ~CUndoEditDelete -- dtor
//              Init            -- Snapshots the arrays of sites it needs
//                                   from the form and addref's the sites
//                                   which are being deleted.
//              Undo            -- Restores the deleted controls
//
//----------------------------------------------------------------------------

class CUndoEditDelete : public CUndoUnitBase
{
public:
                    CUndoEditDelete(CSite * pSite);
    virtual         ~CUndoEditDelete(void);
    HRESULT         Init(CPtrAry<CSite *> * pArySites);
    virtual HRESULT Undo(void);

private:
    CPtrAry<CSite *> *  _sites;         // Sites in this array are NOT addref'd
    CPtrAry<CSite *> *  _sitesSelected; // Sites in this array are NOT addref'd
    CPtrAry<CSite *> *  _sitesDeleted;  // Sites in this array ARE addref'd
    USHORT *            _pusTabIndices; // array of control tab indices
};
#endif

#pragma INCMSG("--- End 'krnlundo.hxx'")
#else
#pragma INCMSG("*** Dup 'krnlundo.hxx'")
#endif
