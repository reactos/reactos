//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       avundo.hxx
//
//  Contents:   CAttrValue Undo class
//
//----------------------------------------------------------------------------

#ifndef I_AVUNDO_HXX_
#define I_AVUNDO_HXX_
#pragma INCMSG("--- Beg 'avundo.hxx'")

#ifndef X_UNDO_HXX_
#define X_UNDO_HXX_
#include "undo.hxx"
#endif

#ifndef X_MARKUPUNDO_HXX_
#define X_MARKUPUNDO_HXX_
#include "markupundo.hxx"
#endif

#ifndef X_MSHTMLRC_H_
#define X_MSHTMLRC_H_
#include "mshtmlrc.h"
#endif

#ifndef NO_EDIT // {

MtExtern(CUndoAttrValueSimpleChange)
MtExtern(CUndoPropChangeNotification)
MtExtern(CUndoPropChangeNotificationPlaceHolder)
MtExtern(CMergeAttributesUndoUnit)

//+---------------------------------------------------------------------------
//
//  Class:      CUndoAttrValueSimpleChange (UAVSC)
//
//  Purpose:    Class for AttrValue undo support.  Not to be used with
//              properties with abstract set method.
//
//----------------------------------------------------------------------------

class CUndoAttrValueSimpleChange : public CUndoPropChange
{
    typedef CUndoPropChange super;
public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CUndoAttrValueSimpleChange))

    CUndoAttrValueSimpleChange(CBase * pBase)
            : CUndoPropChange( pBase, IDS_UNDOPROPCHANGE ) {}

    HRESULT Init( DISPID             dispidProp,
                  VARIANT &          varProp,
                  BOOL               fInlineStyle,
                  CAttrValue::AATYPE aaType );

    HRESULT PrivateDo(IOleUndoManager *pUndoManager);

protected:

    BOOL                _fInlineStyle;
    CAttrValue::AATYPE  _aaType;
};

//+---------------------------------------------------------------------------
//
//  Class:      CUndoPropChangeNotification (UPCN)
//
//  Purpose:    Class for AttrValue undo support.  Not to be used with
//              properties with abstract set method.
//
//----------------------------------------------------------------------------

class CUndoPropChangeNotification : public CUndoUnitBase
{
    typedef CUndoPropChange super;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CUndoPropChangeNotification))

    CUndoPropChangeNotification( CBase * pBase )
            : CUndoUnitBase( pBase, IDS_UNDOPROPCHANGE ) {}

    HRESULT Init( DISPID dispid, DWORD dwFlags, BOOL fPlaceHolder )
            {
                _dispid = dispid;
                _dwFlags = dwFlags;
                _fPlaceHolder = fPlaceHolder;

                return S_OK;
            }

    HRESULT PrivateDo(IOleUndoManager *pUndoManager);

protected:

    DISPID  _dispid;
    DWORD   _dwFlags;
    BOOL    _fPlaceHolder;
};

// We have to create a UndoPropChangeNotification object twice: once before
// we make the various property changes, and once after.  When undoing, we
// execute the former, and not the latter.  We also, for the purpose of Redo,
// turn the latter from a placeholder object to a real one, and vice versa.
// This class simplifies this task.

class CUndoPropChangeNotificationPlaceHolder : CUndoPropChangeNotification
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CUndoPropChangeNotificationPlaceHolder))

    CUndoPropChangeNotificationPlaceHolder( BOOL          fPost,
                                            CElement *    pElement,
                                            DISPID        dispid,
                                            DWORD         dwFlags );

    ~CUndoPropChangeNotificationPlaceHolder();
    
    void SetHR(HRESULT hr) { _hr = hr; }

protected:

    BOOL              _fPost;
    CParentUndoUnit * _pPUU;
    HRESULT           _hr;
};

//+---------------------------------------------------------------------------
//
//  Class:      CMergeAttributesUndo
//
//  Purpose:    Class to help build CMergeAttributesUndoUnits
//
//----------------------------------------------------------------------------
class CMergeAttributesUndo : public CUndoHelper
{
public:
    CMergeAttributesUndo( CElement * pElement );
    ~CMergeAttributesUndo();

    BOOL    AcceptingUndo() { return _fAcceptingUndo; }

    virtual IOleUndoUnit * CreateUnit();

    CAttrArray *        GetAA();
    CAttrArray *        GetAAStyle();
    void                SetWasNamed(BOOL fNamed) { _fWasNamed = fNamed; }
    void                SetCopyId(BOOL fCopyId) { _fCopyId = fCopyId; }
    void                SetRedo() { _fRedo = TRUE; }
    void                SetClearAttr(BOOL fClearAttr) { _fClearAttr = fClearAttr; }
    void                SetPassElTarget(BOOL fPassElTarget) { _fPassElTarget = fPassElTarget; }

protected:
    CElement *          _pElement;
    CAttrArray *        _pAA;
    CAttrArray *        _pAAStyle;

    DWORD               _fAcceptingUndo : 1;
    DWORD               _fWasNamed      : 1;
    DWORD               _fCopyId        : 1;
    DWORD               _fRedo          : 1;
    DWORD               _fClearAttr     : 1;
    DWORD               _fPassElTarget  : 1;
};

//+---------------------------------------------------------------------------
//
//  Class:      CMergeAttributesUndoUnit
//
//  Purpose:    Class to store undo information for merging of attributes
//
//----------------------------------------------------------------------------

class CMergeAttributesUndoUnit : public CUndoUnitBase
{
    typedef CUndoUnitBase super;
public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CMergeAttributesUndoUnit))

    CMergeAttributesUndoUnit(CElement * pElement);
    ~CMergeAttributesUndoUnit();

    void SetData( CAttrArray * pAA, CAttrArray * pAAStyle, BOOL fWasNamed, BOOL fCopyId, 
                  BOOL fRedo, BOOL fClearAttr, BOOL fPAssElTarget );

    HRESULT PrivateDo(IOleUndoManager *pUndoManager);

protected:

    CElement *      _pElement;
    CAttrArray *    _pAA;
    CAttrArray *    _pAAStyle;
    DWORD           _fWasNamed      : 1;
    DWORD           _fCopyId        : 1;
    DWORD           _fRedo          : 1;
    DWORD           _fClearAttr     : 1;
    DWORD           _fPassElTarget  : 1;
};


#endif // NO_EDIT }

#pragma INCMSG("--- End 'avundo.hxx'")
#else
#pragma INCMSG("*** Dup 'avundo.hxx'")
#endif
