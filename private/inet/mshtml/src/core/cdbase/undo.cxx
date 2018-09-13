//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       undo.cxx
//
//  Contents:   Implementation of Undo classes
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_UNDO_HXX_
#define X_UNDO_HXX_
#include "undo.hxx"
#endif

#ifndef X_MSHTMLRC_H_
#define X_MSHTMLRC_H_
#include "mshtmlrc.h"
#endif

#ifndef X_QI_IMPL_H
#define X_QI_IMPL_H
#include "qi_impl.h"
#endif

#ifndef X_DISPEX_H_
#define X_DISPEX_H_
#include <dispex.h>
#endif


DeclareTag(tagUndo, "Undo", "Form Undo");
MtDefine(Undo, Mem, "Undo")
MtDefine(UndoStringDescription, Undo, "Undo Description String")
MtDefine(CUndoManager, Undo, "CUndoManager")
MtDefine(CParentUndoUnit, Undo, "CParentUndoUnit")
MtDefine(CParentUndo, Undo, "CParentUndo")
MtDefine(CUndoPropChange, Undo, "CUndoPropChange")
MtDefine(CUndoUnitAry_pv, Undo, "CUndoUnitAry::_pv")

CDummyUndoManager      g_DummyUndoMgr;    // No ctor, dtor or member data
CBlockedParentUnit     g_BlockedUnit;


// Private guid for undo manager - {FABDA060-28C7-11d2-B0A7-00C04FA34D84}
const CLSID CLSID_CUndoManager = { 0xfabda060, 0x28c7, 0x11d2, { 0xb0, 0xa7, 0x0, 0xc0, 0x4f, 0xa3, 0x4d, 0x84 } };

static HRESULT 
SafeUndoAryRelease( CUndoUnitAry * pAry, int indexReleaseFrom, int indexReleaseTo)
{
    HRESULT         hr = S_OK;
    CUndoUnitAry    aryRelease;
    int             c;

    c = indexReleaseTo - indexReleaseFrom + 1;

    if (c <= 0)
        goto Cleanup;

    Assert( c <= pAry->Size() );

    hr = aryRelease.Grow( c );
    if (hr)
        goto Cleanup;

    // Copy the part of the array that we want to release
    {
        IOleUndoUnit ** ppUndoFrom = ((IOleUndoUnit**)(*pAry)) + indexReleaseFrom;
        IOleUndoUnit ** ppUndoTo = aryRelease;

        for ( ; c > 0; c--, ppUndoFrom++, ppUndoTo++ )
            *ppUndoTo = *ppUndoFrom;
    }

    // Remove the pointers from the original array
    pAry->DeleteMultiple( indexReleaseFrom, indexReleaseTo );

    // Release all of the pointers in the copied array
    aryRelease.ReleaseAll();

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CBlockedParentUnit::Close, public
//
//  Synopsis:   Implements the close method for the dummy blocked parent
//              unit.
//
//  Arguments:  [pPUU]    -- Pointer to object being closed.
//              [fCommit] -- ignored
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CBlockedParentUnit::Close(IOleParentUndoUnit *pPUU, BOOL fCommit)
{
    //
    // We're always blocked, so we only handle that case.
    //
    if (pPUU == (IOleParentUndoUnit*)this)
    {
        return S_FALSE;
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CServer::CreateUndoManager, public
//
//  Synopsis:   Creates the undo manager if it hasn't already been created.
//              Does not query our container for the undo service.
//
//  Arguments:  (none)
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CServer::CreateUndoManager(void)
{
    if (_pUndoMgr == &g_DummyUndoMgr)
    {
        TraceTag((tagUndo, "CServer::CreateUndoManager -- creating manager."));

        _pUndoMgr = new CUndoManager();
        if (!_pUndoMgr)
        {
            _pUndoMgr = &g_DummyUndoMgr;
            RRETURN(E_OUTOFMEMORY);;
        }
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CServer::BlockNewUndoUnits, public
//
//  Synopsis:   Causes any new undo units that might be created to be blocked.
//
//  Arguments:  (none)
//
//  Returns:    An HRESULT and a cookie. If the cookie is zero then you don't
//              have to call UnblockNewUndoUnits. All other values are
//              undefined and UnblockNewUndoUnits must be called.
//
//----------------------------------------------------------------------------

HRESULT
CBase::BlockNewUndoUnits(DWORD *pdwCookie)
{
    //
    // Note that QueryCreateUndo _must_ checked for blocked undo units so
    // that nested calls to this method work properly!
    //
    if (QueryCreateUndo(FALSE))
    {
        *pdwCookie = 1;
        RRETURN(UndoManager()->Open(&g_BlockedUnit));
    }

    *pdwCookie = 0;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CServer::UnblockNewUndoUnits, public
//
//  Synopsis:   Unblocks undo units that were blocked by calling
//              BlockNewUndoUnits.
//
//----------------------------------------------------------------------------

void
CBase::UnblockNewUndoUnits(DWORD dwCookie)
{
    if (dwCookie)
    {
        IGNORE_HR(UndoManager()->Close(&g_BlockedUnit, FALSE));
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CBase::QueryCreateUndo, public
//
//  Synopsis:   Indicates whether an undo unit should be created or not.
//
//  Arguments:  [fRequiresParent] -- If TRUE, the return value will be FALSE
//                                   unless an undo unit is open on the
//                                   stack.
//
//  Returns:    TRUE if an undo unit should be created, FALSE if not.
//
//  Notes:      If [fRequiresParent] is TRUE, and there is no open unit on
//              the undo stack, then the undo stack will be flushed.
//
//----------------------------------------------------------------------------

BOOL
CBase::QueryCreateUndo(BOOL fRequiresParent, BOOL fDirtyChange /* = TRUE */)
{
    HRESULT              hr;
    DWORD                dwUndoState;
    IOleUndoManager * pUM;

    pUM = UndoManager();
    Assert(pUM);

    if (pUM == &g_DummyUndoMgr)
        return FALSE;

    hr = pUM->GetOpenParentState(&dwUndoState);
    if (FAILED(hr))
        goto Error;

    if (hr == S_OK)
    {
        //
        // There's an open unit on the stack.
        //
        if ((dwUndoState & UAS_NOPARENTENABLE) && fRequiresParent)
        {
            goto Error;
        }
        else if (dwUndoState & UAS_BLOCKED)
        {
            return FALSE;
        }
        else
        {
            return TRUE;
        }
    }

    if (!fRequiresParent || TLS( fAllowParentLessPropChanges ))
        return TRUE;

    //
    // The unit needs a parent, but there isn't one, so clear the undo
    // stack.
    //
Error:
    if( fDirtyChange )
        pUM->DiscardFrom(NULL);

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CBase::OpenParentUnit, public
//
//  Synopsis:   Opens a parent unit which does nothing but contain other
//              units and puts it on the undo stack.
//
//  Arguments:  [pBase] -- Owning object
//              [uiID]  -- ID for description
//
//  Returns:    Open parent, may be NULL in certain cases, but this is not
//              an error condition.
//
//  Notes:      The caller should not release the interface returned by
//              this function.  It will be released by CloseParentUnit.
//
//----------------------------------------------------------------------------

CParentUndoUnit *
CBase::OpenParentUnit(CBase * pBase, UINT uiID, TCHAR * pchDescription /* = NULL */)
{
    CParentUndoUnit *      pCPUU     = NULL;
    IOleUndoManager *  pUM;
    DWORD                    dwState;
    HRESULT                  hr;

    TraceTag((tagUndo, "CBase::OpenParentUnit"));

    pUM = UndoManager();
    Assert(pUM);

    if (pUM == &g_DummyUndoMgr)
        return NULL;

    //
    // If there is no undo manager or an already open parent unit then
    // just return NULL because we don't need to create a parent object.
    //
    hr = pUM->GetOpenParentState(&dwState);
    if (FAILED(hr))
        return NULL;

    if ((hr == S_FALSE) ||
        ((dwState & UAS_NOPARENTENABLE) && !(dwState & UAS_BLOCKED)))
    {
        //
        // There's no open object on the stack or it's non-enabling, so put
        // an enabling parent on and return it.
        // 
        if (pchDescription)
            pCPUU = new CParentUndoUnit(pBase, pchDescription);
        else
            pCPUU = new CParentUndoUnit(pBase, uiID);

        if (pCPUU)
        {
            if (FAILED(pUM->Open(pCPUU)))
            {
                ClearInterface(&pCPUU);
            }
        }
    }

    return pCPUU;
}

//+---------------------------------------------------------------------------
//
//  Member:     CBase::CloseParentUnit, public
//
//  Synopsis:   Closes a parent unit opened with OpenParentUnit.
//
//  Arguments:  [pCPUU]    -- Unit returned from OpenParentUnit.
//              [hrCommit] -- If S_OK, then the unit is committed.
//                            Otherwise it's released and not added to the
//                            stack.
//
//  Returns:    HRESULT
//
//  Notes:      If the [pCPUU] object is empty (has no children) then it is
//              not committed.
//
//----------------------------------------------------------------------------

HRESULT
CBase::CloseParentUnit(CParentUndoUnit * pCPUU, HRESULT hrCommit)
{
    HRESULT              hr;
    IOleUndoManager * pUM;

    if (!pCPUU)
        return S_OK;

    TraceTag((tagUndo, "CBase::CloseParentUnit"));

    pUM = UndoManager();
    Assert(pUM);

    //
    // Don't commit if the unit is empty.  Later, if needed, we can add a
    // flag to this function which disables this behavior if we ever get any
    // parent units that affect state and don't need children to be useful.
    //
    if (pCPUU->_aryUndo.Size() == 0)
    {
        hrCommit = S_FALSE;
    }

    hr = pUM->Close(pCPUU, (hrCommit == S_OK) ? TRUE : FALSE);
    if (hr == S_FALSE)
    {
        //
        // The open unit was most likely thrown away by a call to
        // DiscardFrom, so we ignore that situation.
        //
        hr = S_OK;
    }

    pCPUU->Release();

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CBase::CreatePropChangeUndo, public
//
//  Synopsis:   Creates a property change undo object for simple property
//              types.
//
//  Arguments:  [dispidProp] -- Dispid of the property
//              [wpiType]    -- Type of the property (must match the
//                              in-memory size of the property).
//              [dwProp]     -- Current (old) value of property.
//              [ppUndo]     -- Undo object returned here. Can be NULL. The
//                              returned value may be NULL in non-error cases.
//
//  Returns:    HRESULT
//
//  Notes:
//
// If [wpiType] is WPI_CSTRING, then [dwProp] should be a pointer to a CStr.
//
// If [ppUndo] is NULL, the current value of the property is stored and the
// unit is given to the undo manager.
//
// If [ppUndo] is not NULL, the current value of the property is stored but
// the unit is not given to the Undo Manager.  [ppUndo] is filled in with a
// pointer to the unit, and the caller is responsible for adding the unit
// to the undo stack by calling Add() on the undo manager, and then releasing
// the unit.  The unit does not have to be given to the undo manager (in
// the case of a later error for example), but the returned object should
// always be released if it was not NULL.  The value returned in [ppUndo] is
// NULL if an error occurs or if there is no open unit on the undo stack.
//
//----------------------------------------------------------------------------

HRESULT
CBase::CreatePropChangeUndo(DISPID             dispidProp,
                            VARIANT *          pVar,
                            CUndoPropChange ** ppUndo)
{
    HRESULT           hr;
    CUndoPropChange * pUndo;

    if (ppUndo)
        *ppUndo = NULL;

    if (!QueryCreateUndo(TRUE))
        return S_OK;

    TraceTag((tagUndo, "CBase::CreatePropChangeUndo creating an object."));

    pUndo = new CUndoPropChange(this, IDS_UNDOPROPCHANGE);
    if (!pUndo)
        RRETURN(E_OUTOFMEMORY);

    hr = THR(pUndo->Init(dispidProp, pVar));
    if (hr)
        goto Error;

    if (!ppUndo)
    {
        IOleUndoManager *pUM = UndoManager();

        Assert(pUM);

        hr = THR(pUM->Add(pUndo));

        ReleaseInterface(pUndo);
    }
    else
    {
        *ppUndo = pUndo;
    }

Cleanup:
    RRETURN(hr);

Error:
    ReleaseInterface(pUndo);
    goto Cleanup;
}


//+-------------------------------------------------------------------------
//
//  Method:     CBase::QueryStatusUndoRedo
//
//  Synopsis:   Helper function for QueryStatus(). Check if in our current
//              state we suport these commands.
//
//--------------------------------------------------------------------------

HRESULT
CBase::QueryStatusUndoRedo(BOOL fUndo, MSOCMD * pcmd, MSOCMDTEXT * pcmdtext)
{
    BSTR        bstr = NULL;
    HRESULT     hr;

    // Get the Undo/Redo state.
    if (fUndo)
        hr = THR_NOTRACE(UndoManager()->GetLastUndoDescription(&bstr));
    else
        hr = THR_NOTRACE(UndoManager()->GetLastRedoDescription(&bstr));

    // Return the command state.
    pcmd->cmdf = hr ? MSOCMDSTATE_DISABLED : MSOCMDSTATE_UP;

    // Return the command text if requested.
    if (pcmdtext && pcmdtext->cmdtextf == MSOCMDTEXTF_NAME)
    {

        // BUGBUG - This code needs to be supported on the MAC. (rodc)
        if (hr)
        {
            pcmdtext->cwActual = LoadString(
                    GetResourceHInst(),
                    fUndo ? IDS_CANTUNDO : IDS_CANTREDO,
                    pcmdtext->rgwz,
                    pcmdtext->cwBuf);
        }
        else
        {
            hr = Format(
                    0,
                    pcmdtext->rgwz,
                    pcmdtext->cwBuf,
                    MAKEINTRESOURCE(fUndo ? IDS_UNDO : IDS_REDO),
                    bstr);
            if (!hr)
                pcmdtext->cwActual = _tcslen(pcmdtext->rgwz);
        }
    }

    if (bstr)
        FormsFreeString(bstr);

    return S_OK;
}


//+---------------------------------------------------------------
//
//  Member:     CBase::EditUndo
//
//  Synopsis:   Performs an Undo
//
//---------------------------------------------------------------

HRESULT
CBase::EditUndo()
{
    TraceTag((tagUndo, "CBase::EditUndo"));

    HRESULT hr = THR(UndoManager()->UndoTo(NULL));

    RRETURN(hr);
}

//+---------------------------------------------------------------
//
//  Member:     CForm::EditRedo
//
//  Synopsis:   Performs a Redo
//
//---------------------------------------------------------------

HRESULT
CBase::EditRedo()
{
    TraceTag((tagUndo, "CBase::EditRedo"));

    HRESULT hr = THR(UndoManager()->RedoTo(NULL));

    RRETURN(hr);
}






//+---------------------------------------------------------------------------
//
//  CComposeUndo Implementation
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CComposeUndo::CComposeUndo, public
//
//  Synopsis:   ctor
//
//----------------------------------------------------------------------------

CComposeUndo::CComposeUndo()
{
    Assert(_pPUUOpen   == NULL);
    Assert(_fDisabled  == FALSE);
    Assert(_UndoState  == UNDO_BASESTATE);
    Assert(_BlockCount == 0);
}

//+---------------------------------------------------------------------------
//
//  Member:     CComposeUndo::~CComposeUndo, public
//
//  Synopsis:   dtor
//
//----------------------------------------------------------------------------

CComposeUndo::~CComposeUndo()
{
    IGNORE_HR(SafeUndoAryRelease(&_aryUndo, 0, _aryUndo.Size()-1));
    IGNORE_HR(SafeUndoAryRelease(&_aryRedo, 0, _aryRedo.Size()-1));

    ReleaseInterface(_pPUUOpen);
}

//+---------------------------------------------------------------------------
//
//  Member:     CComposeUndo::GetTopUndoUnit, protected
//
//  Synopsis:   Returns the undo unit at the top of the stack.
//
//----------------------------------------------------------------------------

IOleUndoUnit *
CComposeUndo::GetTopUndoUnit()
{
    int c = _aryUndo.Size();

    if (c)
        return _aryUndo[c-1];

    return NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CComposeUndo::GetTopRedoUnit, protected
//
//  Synopsis:   Returns the redo unit at the top of the stack.
//
//----------------------------------------------------------------------------

IOleUndoUnit *
CComposeUndo::GetTopRedoUnit()
{
    int c = _aryRedo.Size();

    if (c)
        return _aryRedo[c-1];

    return NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CComposeUndo::Open, public
//
//  Synopsis:   Adds a parent undo unit, and leaves it open. All further
//              calls to the parent undo methods are forwarded to the object
//              until it is closed.
//
//  Arguments:  [pUU] -- Object to add and leave open.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CComposeUndo::Open(IOleParentUndoUnit *pPUU)
{
    TraceTag((tagUndo, "CComposeUndo::Open,  this=%p", this));

    if (_fDisabled || (_BlockCount > 0))
        return S_OK;

    if (!pPUU)
    {
        RRETURN(E_INVALIDARG);
    }

    if (_pPUUOpen)
    {
        RRETURN(_pPUUOpen->Open(pPUU));
    }

    ReplaceInterface(&_pPUUOpen, pPUU);

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CComposeUndo::Close, public
//
//  Synopsis:   Closes an open undo unit, not necessarily the one we have
//              open directly.
//
//  Arguments:  [pPUU]    -- Pointer to currently open object.
//              [fCommit] -- If TRUE, then the closed undo unit is kept,
//                           otherwise it's discarded.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CComposeUndo::Close(IOleParentUndoUnit * pPUU, BOOL fCommit)
{
    HRESULT hr;

    TraceTag((tagUndo, "CComposeUndo::Close,  this=%p", this));

    if (_fDisabled ||
        ((_BlockCount > 0) && (pPUU != (IOleParentUndoUnit*)this)))
    {
        return S_OK;
    }

    if ((!_pPUUOpen) || (_BlockCount > 0))
    {
        Assert(_NonEnableCount == 0);

        hr = OnClose();
        if (hr)
            RRETURN(hr);

        return S_FALSE;
    }

    hr = THR(_pPUUOpen->Close(pPUU, fCommit));

    if (FAILED(hr) || (hr == S_OK))
        RRETURN(hr);

    Assert(hr == S_FALSE);
    // Close returned S_FALSE

    if (_pPUUOpen != pPUU)
        RRETURN(E_INVALIDARG);

    if (fCommit)
    {
        hr = AddUnit(_pPUUOpen);
    }

    ClearInterface(&_pPUUOpen);

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CComposeUndo::Add, public
//
//  Synopsis:   Adds an undo unit to the stack directly. Doesn't leave it
//              open.
//
//  Arguments:  [pUU] -- Unit to add.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CComposeUndo::Add(IOleUndoUnit *pUU)
{
    HRESULT hr;

    TraceTag((tagUndo, "CComposeUndo::Add,  this=%p", this));

    if (_fDisabled || (_BlockCount > 0))
        return S_OK;

    if (!pUU)
    {
        RRETURN(E_INVALIDARG);
    }

    if (_pPUUOpen)
    {
        RRETURN(_pPUUOpen->Add(pUU));
    }

    hr = AddUnit(pUU);

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CComposeUndo::DoTo, protected
//
//  Synopsis:   Helper method that calls do on an array of objects
//
//  Arguments:  [pUM]         -- Pointer to undo manager to pass to units
//              [paryUnit]    -- Undo or Redo stack
//              [pUU]         -- Object to undo or redo to.
//              [fDoRollback] -- If TRUE, rollback will be attempted on an
//                               error. Noone but the undo manager should
//                               pass TRUE for this.
//
//  Returns:    HRESULT
//
//  Notes:      Parent units can use the _fUnitSucceeded flag to determine
//              whether or not they should commit the unit they put on the
//              opposite stack.  If _fUnitSucceeded is TRUE after calling
//              this function, then the unit should commit itself.  If
//              FALSE, the unit does not have to commit itself.  In either
//              case any error code returned by this function should be
//              propagated to the caller.
//
//----------------------------------------------------------------------------

HRESULT
CComposeUndo::DoTo(IOleUndoManager *            pUM,
                   CUndoUnitAry *               paryUnit,
                   IOleUndoUnit *               pUU,
                   BOOL                         fDoRollback)
{
    IOleUndoUnit **         ppUA;
    CUndoUnitAry            aryCopy;
    int                     iUnit;
    HRESULT                 hr;

    TraceTag((tagUndo, "CComposeUndo::DoTo"));

    _fUnitSucceeded = FALSE;
    _fRollbackNeeded = FALSE;

    if (_fDisabled || _pPUUOpen)
        RRETURN(E_UNEXPECTED);

    Assert(paryUnit);

    if (paryUnit->Size() == 0)
        return S_OK;

    hr = THR(aryCopy.Copy(*paryUnit, FALSE));
    if (hr)
        RRETURN(hr);

    if (pUU)
    {
        iUnit = aryCopy.Find(pUU);
        if (iUnit == -1)
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
    }
    else
    {
        iUnit = aryCopy.Size() - 1;
        pUU = aryCopy[iUnit];
    }

    //
    // Delete the units from the original array before we call Do() on those
    // units in case they do something naughty like call DiscardFrom which
    // would Release them.
    //
    paryUnit->DeleteMultiple(iUnit, paryUnit->Size() - 1);

    //
    // Make sure the copy of the array has only the units in it we're
    // processing.
    //
    if (iUnit > 0)
    {
        aryCopy.DeleteMultiple(0, iUnit - 1);
    }

    for(ppUA = &aryCopy.Item(aryCopy.Size() - 1);
        ; // Infinite
        ppUA--)
    {
        hr = THR((*ppUA)->Do(pUM));
        if (hr)
            goto Error;

        _fUnitSucceeded = TRUE;

        if (*ppUA == pUU)
            break;
    }

    Assert(!_pPUUOpen);

Cleanup:
    if (hr == E_ABORT)
    {
        hr = E_FAIL;
    }

Cleanup2:
    aryCopy.ReleaseAll();

    RRETURN(hr);

Error:
    if (fDoRollback)
    {
        HRESULT hr2 = S_OK;

        // Using a local because the calls to Do() may change the state
        // of this flag.
        BOOL    fRollbackNeeded = _fRollbackNeeded;

        if (fRollbackNeeded)
        {
            TraceTag((tagUndo, "Undo failed! Attempting rollback..."));

            if (_UndoState == UNDO_UNDOSTATE)
            {
                hr2 = THR(GetTopRedoUnit()->Do(NULL));
            }
            else
            {
                hr2 = THR(GetTopUndoUnit()->Do(NULL));
            }
        }

        Assert(!_pPUUOpen);

        _aryUndo.ReleaseAll();
        _aryRedo.ReleaseAll();
        ClearInterface(&_pPUUOpen); // For safety in retail builds only.

        if (fRollbackNeeded)
        {
            if (hr2)
            {
                TraceTag((tagUndo, "Rollback failed! Bailing out..."));

                hr = E_ABORT;
                goto Cleanup2;
            }
            else
                TraceTag((tagUndo, "Rollback succeeded! Returning error %hr", hr));
        }
    }
#if DBG == 1
    else
    {
        if (!fDoRollback)
        {
            TraceTag((tagUndo, "Undo failed for parent unit!"));
        }
        else if (!_fRollbackNeeded)
        {
            TraceTag((tagUndo, "Undo failed but rollback not needed!"));
        }
    }
#endif

    goto Cleanup;
}

//+---------------------------------------------------------------------------
//
//  Member:     CComposeUndo::AddUnit, protected
//
//  Synopsis:   Adds a new unit to the appropriate stack, no questions asked.
//
//  Arguments:  [pUU] -- Unit to add
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CComposeUndo::AddUnit(IOleUndoUnit *pUU)
{
    HRESULT hr;

    if (_UndoState == UNDO_UNDOSTATE)
    {
        if (_aryRedo.Size() > 0)
        {
            GetTopRedoUnit()->OnNextAdd();
        }

        Assert(!_fRespectMaxEntries || _aryRedo.Size() <= MAX_STACK_ENTRIES);

        hr = THR(_aryRedo.Append(pUU));
        if (hr)
            goto Cleanup;

        pUU->AddRef();

        if (_aryRedo.Size() > MAX_STACK_ENTRIES && _fRespectMaxEntries )
        {
            _aryRedo.ReleaseAndDelete(0);
        }
    }
    else
    {
        if (_aryUndo.Size() > 0)
        {
            GetTopUndoUnit()->OnNextAdd();
        }

        Assert(!_fRespectMaxEntries || _aryUndo.Size() <= MAX_STACK_ENTRIES);

        hr = THR(_aryUndo.Append(pUU));
        if (hr)
            goto Cleanup;

        pUU->AddRef();

        if (_aryUndo.Size() > MAX_STACK_ENTRIES && _fRespectMaxEntries)
        {
            _aryUndo.ReleaseAndDelete(0);
        }

        if (_UndoState == UNDO_BASESTATE)
        {
            _aryRedo.ReleaseAll();
        }
    }

    _fRollbackNeeded = TRUE;

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CComposeUndo::FindChild, public
//
//  Synopsis:   Searches the children in a given stack for a undo unit.
//
//  Arguments:  [aryUnit] -- Array to look in
//              [pUU]     -- Unit to look for
//
//  Returns:    The index of the element in [aryUnit] that contains [pUU].
//
//----------------------------------------------------------------------------

int
CComposeUndo::FindChild(CUndoUnitAry &aryUnit, IOleUndoUnit *pUU)
{
    IOleParentUndoUnit * pPUU;
    IOleUndoUnit **      ppUA;
    HRESULT              hr     = S_FALSE;
    int                  i;

    for (i = aryUnit.Size(), ppUA = aryUnit;
         i;
         i--, ppUA++)
    {
        if ((*ppUA)->QueryInterface(IID_IOleParentUndoUnit, (LPVOID*)&pPUU) == S_OK)
        {
            hr = pPUU->FindUnit(pUU);

            ReleaseInterface(pPUU);
        }

        if (hr == S_OK)
            break;
    }

    if (i == 0)
        i = -1;
    else
        i = ppUA - (IOleUndoUnit **)aryUnit;

    return i;
}

//+---------------------------------------------------------------------------
//
//  Member:     CComposeUndo::SetBlockedState, public
//
//  Synopsis:   Sets the blocked state of the parent unit. The unit
//              maintains a counter.
//
//  Arguments:  [fBlocked] -- If TRUE, the block count is incremented.
//                            Otherwise it's decremented.
//
//  Notes:      If the block count is non-zero, the unit is blocked.
//
//----------------------------------------------------------------------------

void
CComposeUndo::SetBlockedState(BOOL fBlocked)
{
    if (fBlocked)
    {
        Assert(!_pPUUOpen);

        Assert(_BlockCount < MAX_BLOCK_COUNT);
        _BlockCount++;
    }
    else
    {
        Assert(_BlockCount > 0);
        _BlockCount--;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CComposeUndo::SetNonEnableState, public
//
//  Synopsis:   Sets the non-parent-enable state of the parent unit.  The
//              unit maintains a counter.
//
//  Arguments:  [fNonEnable] -- If TRUE, the non-enable count is incremented.
//                              Otherwise it's decremented.
//
//  Notes:      If the non-enable count is non-zero, the unit returns
//              UAS_NOPARENTENABLE from GetParentState.
//
//----------------------------------------------------------------------------

void
CComposeUndo::SetNonEnableState(BOOL fNonEnable)
{
    if (fNonEnable)
    {
        Assert(_NonEnableCount < MAX_BLOCK_COUNT);
        _NonEnableCount++;
    }
    else
    {
        Assert(_NonEnableCount > 0);
        _NonEnableCount--;
    }
}

//+---------------------------------------------------------------------------
//
//  CUndoManager Implementation
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CUndoManager::CUndoManager, public
//
//  Synopsis:   CUndoManager ctor
//
//----------------------------------------------------------------------------

CUndoManager::CUndoManager()
{
    _ulRefs = 1;
    _fRespectMaxEntries = TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUndoManager::QueryInterface, public
//
//  Synopsis:   Implements QueryInterface for the undo manager
//
//----------------------------------------------------------------------------

HRESULT
CUndoManager::QueryInterface(REFIID iid, void ** ppv)
{
    *ppv = NULL;

    if (iid == CLSID_CUndoManager)
    {
        *ppv = LPVOID(this);
        return S_OK;
    }

    switch (iid.Data1)
    {
        QI_INHERITS(this, IUnknown)
        QI_INHERITS(this, IOleUndoManager)
    }

    if (!*ppv)
        RRETURN_NOTRACE(E_NOINTERFACE);

    (*(IUnknown **)ppv)->AddRef();

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUndoManager::GetOpenParentState, public
//
//  Synopsis:   Indicates whether there's an open unit, and if so whether
//              or not it's blocked.
//
//  Arguments:  [pdwState] -- Place to fill in state.
//
//  Returns:    S_OK for an open object, S_FALSE for not.
//
//----------------------------------------------------------------------------

HRESULT
CUndoManager::GetOpenParentState(DWORD * pdwState)
{
    TraceTag((tagUndo, "CUndoManager::GetOpenParentState"));

    if (_fDisabled)
    {
        *pdwState = UAS_BLOCKED;
        return S_OK;
    }

    *pdwState = 0;

    if (_pPUUOpen)
    {
        return _pPUUOpen->GetParentState(pdwState);
    }

    return S_FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUndoManager::DiscardFrom, public
//
//  Synopsis:   Removes the specified undo unit and all units below it
//              from the undo stack. Checks child undo units for the given
//              unit and deletes the topmost parent of the given unit.
//
//  Arguments:  [pUU] -- Unit to remove. If NULL, the entire undo and redo
//                       stacks are cleared.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CUndoManager::DiscardFrom(IOleUndoUnit * pUU)
{
    TraceTag((tagUndo, "CUndoManager::DiscardFrom"));

    HRESULT hr = S_OK;

    if (_fDisabled)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    if (pUU)
    {
        int indexUndo;
        int indexRedo = -1;

        //
        // The most common scenario is for the given unit to exist somewhere
        // in the top-level undo or redo stack. So, we search those first
        // before checking children.
        //
        indexUndo = _aryUndo.Find(pUU);
        if (indexUndo == -1)
        {
            indexRedo = _aryRedo.Find(pUU);
            if (indexRedo == -1)
            {
                indexUndo = FindChild(_aryUndo, pUU);
                if (indexUndo == -1)
                {
                    indexRedo = FindChild(_aryRedo, pUU);
                }
            }
        }

        if ((indexUndo == -1) && (indexRedo == -1))
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }

        if (indexUndo != -1)
        {
            hr = SafeUndoAryRelease( &_aryUndo, 0, indexUndo );
            if (hr)
                goto Cleanup;
        }
        else
        {
            Assert(indexRedo != -1);
            hr = SafeUndoAryRelease( &_aryRedo, 0, indexRedo );
            if (hr)
                goto Cleanup;
        }
    }
    else
    {
        hr = SafeUndoAryRelease( &_aryUndo, 0, _aryUndo.Size() - 1 );
        if (hr)
            goto Cleanup;

        hr = SafeUndoAryRelease( &_aryRedo, 0, _aryRedo.Size() - 1 );
        if (hr)
            goto Cleanup;

        if (_pPUUOpen)
        {
            ClearInterface(&_pPUUOpen);
        }
    }

Cleanup:
    RRETURN( hr );
}

//+---------------------------------------------------------------------------
//
//  Member:     CUndoManager::UndoTo, public
//
//  Synopsis:   Performs undo operations up to the given unit on the stack.
//
//  Arguments:  [pUU] -- Undo unit to undo up to. If NULL undo the last one.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CUndoManager::UndoTo(IOleUndoUnit *pUU)
{
    HRESULT hr;

    TraceTag((tagUndo, "CUndoManager::UndoTo"));

    if (_fDisabled || (_UndoState != UNDO_BASESTATE))
        RRETURN(E_UNEXPECTED);

    _UndoState = UNDO_UNDOSTATE;

    hr = DoTo(this, &_aryUndo, pUU, TRUE);

    _UndoState = UNDO_BASESTATE;

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CUndoManager::RedoTo, public
//
//  Synopsis:   Performs redo operations up to the given unit on the stack.
//
//  Arguments:  [pUU] -- Redo unit to redo up to. If NULL redo the last one.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CUndoManager::RedoTo(IOleUndoUnit *pUU)
{
    HRESULT hr;

    TraceTag((tagUndo, "CUndoManager::RedoTo"));

    if (_fDisabled || (_UndoState != UNDO_BASESTATE))
        RRETURN(E_UNEXPECTED);

    _UndoState = UNDO_REDOSTATE;

    hr = DoTo(this, &_aryRedo, pUU, TRUE);

    _UndoState = UNDO_BASESTATE;

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CUndoManager::EnumUndoable, public
//
//  Synopsis:   Returns an enumerator that enumerates the undo stack.
//
//  Arguments:  [ppEnum] -- Place to put enumerator.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CUndoManager::EnumUndoable(IEnumOleUndoUnits **ppEnum)
{
    TraceTag((tagUndo, "CUndoManager::EnumUndoable"));

    if (_fDisabled || (_UndoState != UNDO_BASESTATE))
        RRETURN(E_UNEXPECTED);

    RRETURN(_aryUndo.EnumElements(IID_IEnumOleUndoUnits,
                                  (void**)ppEnum,
                                  TRUE, TRUE, TRUE));
}

//+---------------------------------------------------------------------------
//
//  Member:     CUndoManager::EnumRedoable, public
//
//  Synopsis:   Returns an enumerator that enumerates the redo stack.
//
//  Arguments:  [ppEnum] -- Place to put enumerator.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CUndoManager::EnumRedoable(IEnumOleUndoUnits **ppEnum)
{
    TraceTag((tagUndo, "CUndoManager::EnumRedoable"));

    if (_fDisabled || (_UndoState != UNDO_BASESTATE))
        RRETURN(E_UNEXPECTED);

    RRETURN(_aryRedo.EnumElements(IID_IEnumOleUndoUnits,
                                  (void**)ppEnum,
                                  TRUE, TRUE, TRUE));
}

//+---------------------------------------------------------------------------
//
//  Member:     CUndoManager::GetLastUndoDescription, public
//
//  Synopsis:   Returns the description of the top-most undo unit
//
//  Arguments:  [pbstr] -- Place to put description.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CUndoManager::GetLastUndoDescription(BSTR *pbstr)
{
    TraceTag((tagUndo, "CUndoManager::GetLastUndoDescription"));

    RRETURN(GetDescr(GetTopUndoUnit(), pbstr));
}

//+---------------------------------------------------------------------------
//
//  Member:     CUndoManager::GetLastRedoDescription, public
//
//  Synopsis:   Returns the description of the top-most redo unit
//
//  Arguments:  [pstr] -- Place to put description. Should be freed with
//                        the OLE task allocator.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CUndoManager::GetLastRedoDescription(BSTR *pbstr)
{
    TraceTag((tagUndo, "CUndoManager::GetLastRedoDescription"));

    RRETURN(GetDescr(GetTopRedoUnit(), pbstr));
}

//+---------------------------------------------------------------------------
//
//  Member:     CUndoManager::Enable, public
//
//  Synopsis:   Enables or disables the undo manager.
//
//  Arguments:  [fEnable] -- If TRUE the undo manager is enabled.
//
//  Returns:    HRESULT
//
//  Notes:      The manager cannot be disabled when it is in the middle of
//              an undo or redo, or if there is an open unit.
//
//----------------------------------------------------------------------------

HRESULT
CUndoManager::Enable(BOOL fEnable)
{
    TraceTag((tagUndo, "CUndoManager::Enable(%s)", fEnable ? "TRUE" : "FALSE"));

    //
    // Can't disable in the middle of an undo or redo, or when an unit
    // is open.
    //
    if ((_UndoState > UNDO_BASESTATE) || _pPUUOpen)
    {
        RRETURN(E_UNEXPECTED);
    }

    if (fEnable)
    {
        _fDisabled = FALSE;
    }
    else
    {
        DiscardFrom(NULL);
        _fDisabled = TRUE;
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUndoManager::GetDescr, protected
//
//  Synopsis:   Helper method for GetLast{Un|Re}doDescription
//
//  Arguments:  [pUU]   -- Unit to get description
//              [pbstr] -- Place to put it
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CUndoManager::GetDescr(IOleUndoUnit *pUU, BSTR *pbstr)
{
    *pbstr = NULL;

    if (pUU == NULL)
        RRETURN(E_FAIL);

    if (_fDisabled || (_UndoState != UNDO_BASESTATE))
        RRETURN(E_UNEXPECTED);

    RRETURN(pUU->GetDescription(pbstr));
}

//+---------------------------------------------------------------------------
//
//  CUndoUnit Implementation
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CUndoUnit::CUndoUnit, public
//
//  Synopsis:   CUndoUnit ctor
//
//  Effects:    Saves member data.
//
//  Arguments:  [pBase]      -- Owner Class
//              [uiStringID] -- String ID of name. Must be a valid resource ID.
//
//  Notes:      [uiStringID] should be unique for each undo class since it is
//              used by GetUnitType() to identify the undo unit.
//
//----------------------------------------------------------------------------

CUndoUnit::CUndoUnit(CBase * pBase, UINT uiStringID)
{
    _pBase   = pBase;
    _uiResID = uiStringID;
    _pchDescription = NULL;
}


CUndoUnit::CUndoUnit(CBase * pBase, TCHAR * pchDescription )
{
    _pBase   = pBase;
    MemAllocString( Mt(UndoStringDescription), pchDescription, &_pchDescription );
}

CUndoUnit::~CUndoUnit()
{
    if( _pchDescription )
        MemFreeString(_pchDescription);
}

//+---------------------------------------------------------------------------
//
//  Member:     CUndoUnit::GetDescription, public
//
//  Synopsis:   Gets the description for this undo unit.
//
//  Arguments:  [pbstr] -- Place to put description
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CUndoUnit::GetDescription(BSTR *pbstr)
{
    TCHAR szName[FORMS_BUFLEN + 1]; // String for name

    szName[0] = '\0';

    Assert(pbstr);

    if (_pchDescription)
    {
        RRETURN( FormsAllocString( _pchDescription, pbstr ) );
    }
    else
    {
        if (TW32(0, LoadString(GetResourceHInst(), _uiResID, szName, FORMS_BUFLEN)) == 0)
            RRETURN(GetLastWin32Error());
        RRETURN(FormsAllocString(szName, pbstr));
    }

    
}

//+---------------------------------------------------------------------------
//
//  Member:     CUndoUnit::GetUnitType, public
//
//  Synopsis:   Returns the type of this undo unit.
//
//  Arguments:  [pclsid] -- Place to put CLSID (caller allocated).
//              [plID]   -- Place to put identifier.
//
//  Returns:    HRESULT
//
//  Notes:      The classid returned is the form's classid, and the integer
//              is the resource identifier given in the constructor. This
//              information is meaningful only to the form.
//
//----------------------------------------------------------------------------

HRESULT
CUndoUnit::GetUnitType(CLSID *pclsid, LONG *plID)
{
    Assert(pclsid && plID);

    if (_pBase)
    {
        *pclsid = *(_pBase->BaseDesc()->_pclsid);
        *plID = _uiResID;
    }
    else
    {
        *pclsid = CLSID_NULL;
        *plID   = 0;
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  CUndoUnitBase Implementation
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CUndoUnitBase::CUndoUnitBase, public
//
//  Synopsis:   CUndoUnitBase ctor
//
//  Effects:    Saves member data.
//
//  Arguments:  [pBase]      -- Owner class
//              [uiStringID] -- String ID of name. Must be a valid resource ID.
//
//  Notes:      [uiStringID] should be unique for each undo class since it is
//              used by GetUnitType() to identify the undo unit.
//
//----------------------------------------------------------------------------

CUndoUnitBase::CUndoUnitBase(CBase * pBase, UINT uiStringID)
    : CUndoUnit(pBase, uiStringID)
{
    _ulRefs = 1;

    // We are an undo if we in the base state or in the redo state
    // currently.  i.e. we are not in the undo state
    _fUndo = TLS(nUndoState) != UNDO_UNDOSTATE;
}


HRESULT CUndoUnitBase::Do(IOleUndoManager *pUndoManager)
{
    HRESULT hr;
    THREADSTATE * pts = GetThreadState();

    // Do should not be called recursively!
    Assert( TLS(nUndoState) == UNDO_BASESTATE );

    pts->nUndoState = _fUndo ? UNDO_UNDOSTATE : UNDO_REDOSTATE;

    // call the derived class
    hr = PrivateDo( pUndoManager );

    pts->nUndoState = UNDO_BASESTATE;

    RRETURN( hr );
}

//+---------------------------------------------------------------------------
//
//  Member:     CUndoUnitBase::QueryInterface, public
//
//----------------------------------------------------------------------------

HRESULT
CUndoUnitBase::QueryInterface(REFIID iid, LPVOID *ppv)
{
    *ppv = NULL;

    switch(iid.Data1)
    {
        QI_INHERITS(this, IUnknown)
        QI_INHERITS(this, IOleUndoUnit)
    }

    if (!*ppv)
        RRETURN_NOTRACE(E_NOINTERFACE);

    (*(IUnknown **)ppv)->AddRef();

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  CParentUnitBase Implementation
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CParentUnitBase::CParentUnitBase, public
//
//  Synopsis:   ctor
//
//----------------------------------------------------------------------------

CParentUnitBase::CParentUnitBase(CBase * pBase, UINT uiStringID)
    : CUndoUnit(pBase, uiStringID)
{
    _ulRefs  = 1;
}


CParentUnitBase::CParentUnitBase(CBase * pBase, BSTR bstrDescription)
    : CUndoUnit(pBase, bstrDescription)
{
    _ulRefs  = 1;
}
//+---------------------------------------------------------------------------
//
//  Member:     CParentUnitBase::~CParentUnitBase, public
//
//  Synopsis:   dtor
//
//----------------------------------------------------------------------------

CParentUnitBase::~CParentUnitBase()
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CParentUnitBase::QueryInterface, public
//
//----------------------------------------------------------------------------

HRESULT
CParentUnitBase::QueryInterface(REFIID iid, LPVOID *ppv)
{
    *ppv = NULL;

    switch(iid.Data1)
    {
        QI_INHERITS(this,  IUnknown)
        QI_INHERITS(this,  IOleParentUndoUnit)
        QI_INHERITS2(this, IOleUndoUnit, IOleParentUndoUnit)
    }

    if (!*ppv)
        RRETURN_NOTRACE(E_NOINTERFACE);

    (*(IUnknown **)ppv)->AddRef();

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CParentUnitBase::FindUnit, public
//
//  Synopsis:   Indicates if the given unit is in our undo stack or the stack
//              of one of our children. Doesn't check the current open object.
//
//  Arguments:  [pUU] -- Unit to find
//
//  Returns:    TRUE if we found it.
//
//----------------------------------------------------------------------------

HRESULT
CParentUnitBase::FindUnit(IOleUndoUnit *pUU)
{
    int                      i;

    TraceTag((tagUndo, "CParentUnitBase::FindUnit"));

    if (!pUU)
        RRETURN(E_INVALIDARG);

    Assert(_UndoState == UNDO_BASESTATE);

    i = _aryUndo.Find(pUU);
    if (i != -1)
        return S_OK;

    i = FindChild(_aryUndo, pUU);
    if (i != -1)
        return S_OK;

    return S_FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CParentUnitBase::GetParentState, public
//
//  Synopsis:   Indicates whether there's an open unit, and if so whether
//              or not it's blocked.
//
//  Arguments:  [pdwState] -- Place to fill in state.
//
//  Returns:    S_OK always, unless disabled.
//
//----------------------------------------------------------------------------

HRESULT
CParentUnitBase::GetParentState(DWORD * pdwState)
{
    if (!pdwState)
        RRETURN(E_INVALIDARG);

    *pdwState = 0;

    Assert(_UndoState == UNDO_BASESTATE);

    if (_BlockCount > 0)
    {
        Assert(!_pPUUOpen);

        *pdwState = UAS_BLOCKED;
    }
    else if (_pPUUOpen)
    {
        return _pPUUOpen->GetParentState(pdwState);
    }

    if (_NonEnableCount > 0)
    {
        *pdwState |= UAS_NOPARENTENABLE;
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  CParentUndoUnit Implementation
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Function:   CParentUndoUnit::CParentUndoUnit
//
//  Synopsis:   CParentUndoUnit ctor
//
//----------------------------------------------------------------------------

CParentUndoUnit::CParentUndoUnit(CBase * pBase, UINT uiStringID)
    : CParentUnitBase(pBase, uiStringID)
{
    TraceTag((tagUndo, "Creating CParentUndoUnit"));
}


CParentUndoUnit::CParentUndoUnit(CBase * pBase, BSTR bstrDescription)
    : CParentUnitBase(pBase, bstrDescription)
{
    TraceTag((tagUndo, "Creating CParentUndoUnit"));
}

//+---------------------------------------------------------------------------
//
//  Member:     CParentUndoUnit::Do, public
//
//  Synopsis:   Calls undo on our contained undo object.
//
//  Arguments:  [pUndoManager] -- Pointer to Undo Manager
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CParentUndoUnit::Do(IOleUndoManager *pUndoManager)
{
    HRESULT        hr    = S_OK;

    TraceTag((tagUndo, "CParentUndoUnit::Do"));

    if (_aryUndo.Size() == 0)
        return S_OK;

    //
    // Put ourself on the undo manager's Redo stack.
    //
    if (pUndoManager)
    {
        hr = THR(pUndoManager->Open(this));
        if (hr)
            goto Cleanup;
    }

    //
    // Call Do() on all the units. This call makes a copy of the array and
    // removes the units from _aryUndo before making any calls to Do().
    //
    hr = THR(DoTo(pUndoManager, &_aryUndo, _aryUndo[0], FALSE));

    //
    // _fUnitSucceeded will be TRUE after calling DoTo only if at least
    // one of our contained units was successful. In this case we need to
    // commit ourselves, even if an error occurred.
    //

    if (pUndoManager)
    {
        HRESULT hr2;
        BOOL    fCommit = TRUE;

        //
        // If we are empty or none of our contained units succeeded then do
        // not commit ourselves.
        //
        if (!_fUnitSucceeded || (_aryUndo.Size() == 0))
        {
            TraceTag((tagUndo, "Not committing parent unit to redo stack."));
            fCommit = FALSE;
        }

        hr2 = THR(pUndoManager->Close(this, fCommit));
        //
        // Preserve the HRESULT from the call to DoTo() if it failed.
        //
        if (!hr && FAILED(hr2))
        {
            hr = hr2;
        }
    }

Cleanup:

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  CParentUndo Implementation
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CParentUndo::Start, public
//
//  Synopsis:   Create the undo unit and open it
//
//----------------------------------------------------------------------------
HRESULT
CParentUndo::Start(UINT uiStringID)
{
    _puu = _pBase->OpenParentUnit( _pBase, uiStringID );

    return S_OK;
}

HRESULT
CParentUndo::Start(TCHAR * pchDescription)
{
    _puu = _pBase->OpenParentUnit( _pBase, 0, pchDescription );

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CParentUndo::Finish, public
//
//  Synopsis:   Close the parent unit
//
//----------------------------------------------------------------------------
HRESULT
CParentUndo::Finish(HRESULT hrCommit)
{
    HRESULT hr;

    hr = THR( _pBase->CloseParentUnit( _puu, hrCommit ) ); 
    if( hr )
        goto Cleanup;

Cleanup:
    _puu = NULL;

    RRETURN( hr );
}

//+---------------------------------------------------------------------------
//
//  CUndoPropChange Implementation
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CUndoPropChange::CUndoPropChange, public
//
//  Synopsis:   Object ctor
//
//----------------------------------------------------------------------------

CUndoPropChange::CUndoPropChange(CBase * pBase,
                                 UINT    uiStringID)
    : CUndoUnitBase(pBase, uiStringID)
{
    Assert(pBase);

    TraceTag((tagUndo, "CUndoPropChange ctor"));

    pBase->PrivateAddRef();

    VariantInit(&_varData);
}

//+---------------------------------------------------------------------------
//
//  Member:     CUndoPropChange::~CUndoPropChange, public
//
//  Synopsis:   Undo unit dtor
//
//----------------------------------------------------------------------------

CUndoPropChange::~CUndoPropChange()
{
    TraceTag((tagUndo, "CUndoPropChange dtor"));

    _pBase->PrivateRelease();
    
    VariantClear(&_varData);
}

//+---------------------------------------------------------------------------
//
//  Member:     CUndoPropChange::Init, public
//
//  Synopsis:   Initializes the undo unit for an integer property that is
//              changing.  The function takes ownership of the variant
//              passed in.
//
//  Arguments:  [dispidProp] -- Dispid of property.
//              [pvar] -- the old value, as a variant
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CUndoPropChange::Init(DISPID dispidProp, VARIANT* pvar)
{
    TraceTag((tagUndo, "CUndoPropChange::Init"));

    HRESULT hr = S_OK;

    // Take ownership of the variant
    _varData = *pvar;
    VariantInit( pvar );

    _dispid  = dispidProp;

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CUndoPropChange::PrivateDo, public
//
//  Synopsis:   Performs the undo of the property change.
//
//  Arguments:  [pUndoManager] -- Pointer to Undo Manager
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CUndoPropChange::PrivateDo(IOleUndoManager *pUndoManager)
{
    HRESULT       hr;
    IDispatch   * pDisp = NULL;
    DWORD         dwCookie = 0;

    TraceTag((tagUndo, "CUndoPropChange::Do"));

    hr = THR(_pBase->PunkOuter()->QueryInterface(IID_IDispatch, (LPVOID*)&pDisp));
    if (hr)
        goto Cleanup;

    //
    // The redo unit should be put on the stack in this call to Invoke()
    // unless we need to disable it.
    //
    if (!pUndoManager)
    {
        _pBase->BlockNewUndoUnits(&dwCookie);
    }

    // NOTE: might want to have different way to specify unset method
    if (V_VT(&_varData) == VT_NULL)
    {
        PROPERTYDESC *pPropDesc;

        hr = THR(_pBase->FindPropDescFromDispID(_dispid, &pPropDesc, NULL, NULL));
        if (hr)
            goto Cleanup;

        _pBase->removeAttributeDispid( _dispid, pPropDesc );
    }
    else
    {
        hr = THR(SetDispProp(
                       pDisp,
                       _dispid,
                       g_lcidUserDefault,
                       &_varData,
                       NULL));
    }

    if (!pUndoManager)
    {
        _pBase->UnblockNewUndoUnits(dwCookie);
    }

Cleanup:
    ReleaseInterface(pDisp);
    RRETURN(hr);
}



