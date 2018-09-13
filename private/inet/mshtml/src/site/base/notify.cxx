//+----------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994, 1995, 1996, 1997, 1998
//
//  File:       notify.cxx
//
//  Contents:   Notification base classes
//
//  Classes:    CNotification, et. al.
//
//-----------------------------------------------------------------------------


#include "headers.hxx"

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif


MtDefine(CNotification, Tree, "CNotification")

#if DBG==1
      DWORD CNotification::s_snNext = 0;
#endif

const DWORD CNotification::s_aryFlags[NTYPE_MAX] =
{
#define  _NOTIFYTYPE_TABLE_
#include "notifytype.h"
};

//-----------------------------------------------------------------------------
//
//  Member:     SetElement
//
//  Synopsis:   Change the CElement associated with a notification and
//              optionally update the associated range
//
//  Arguments:  pElement   - Pointer to new affected CElement
//              fKeepRange - Flag to keep/update range
//
//-----------------------------------------------------------------------------
void
CNotification::SetElement(
    CElement *  pElement,
    BOOL        fKeepRange)
{
    Assert(fKeepRange || pElement || IsFlagSet(NFLAGS_TREECHANGE));

    _pElement = pElement;

    if (_pElement)
    {
        //
        //  Set the associated range
        //

        if (!fKeepRange)
        {
            if (!IsFlagSet(NFLAGS_LAZYRANGE))
            {
                EnsureRange();
            }
            else
            {
                SetTextRange(-1, -1);
            }
        }
        
        //
        //  Determine the starting node in the tree
        //  (If not already set)
        //

        if (!_pNode)
        {
            _pNode = _pElement->GetFirstBranch();
        }

        //
        //  Ensure the handler is cleared
        //  (Changing the pElement changes the notification and implies that
        //   it is no longer "handled")
        //

        _pHandler = NULL;
    }
}

//-----------------------------------------------------------------------------
//
//  Member:     EnsureRange
//
//  Synopsis:   Update the associated range with the given pElement
//
//-----------------------------------------------------------------------------

void
CNotification::EnsureRange()
{
    BOOL    fInTree = !!(_pElement->GetFirstBranch());
    
    if (fInTree)
    {
        _pElement->GetRange(&_cp, &_cch);
    }

    if (    !fInTree
        ||  _cp < 0)
    {
        _cp  = -1;
        _cch = -1;
    }

    ClearFlag(NFLAGS_LAZYRANGE);
}


//+----------------------------------------------------------------------------
//
//  Member:     LayoutFlags
//
//  Synopsis:   Convert internal NFLAGS_xxxx to external LAYOUT_xxxx flags
//
//-----------------------------------------------------------------------------
DWORD
CNotification::LayoutFlags() const
{
    DWORD   grfLayout = 0;

    if (_grfFlags & NFLAGS_FORCE)
        grfLayout |= LAYOUT_FORCE;

    return grfLayout;
}


#if DBG==1
//+----------------------------------------------------------------------------
//
//  Member:     Name
//
//  Synopsis:   Returns a static string describing the notification
//
//-----------------------------------------------------------------------------

LPCTSTR
CNotification::Name() const
{
    LPCTSTR pch;

    switch (_ntype)
    {
    #define  _NOTIFYTYPE_NAMES_
    #include "notifytype.h"

        default:
            AssertSz(FALSE, "Unknown CNotification type");
            pch = _T("???");
            break;
    }

    return pch;
}
#endif


//+----------------------------------------------------------------------------
//
//  Member:     Accumulate
//
//  Synopsis:   Accumulate the dirty tree region described by a CNotification
//
//  Arguments:  pnf - Notification documenting the change
//
//-----------------------------------------------------------------------------
void
CDirtyTreeRegion::Accumulate(
    CNotification * pnf)
{
    Assert(pnf);
    Assert(pnf->IsTreeChange());

    AssertSz(FALSE, "CDirtyTreeRegion::Accumulate is not written");
}


//+----------------------------------------------------------------------------
//
//  Member:     Adjust
//
//  Synopsis:   Adjust the recorded dirty region to take into account
//              nested changes
//
//  Arguments:  pnf - Notification documenting the change
//
//-----------------------------------------------------------------------------
void
CDirtyTreeRegion::Adjust(
    CNotification * pnf)
{
    Assert(pnf);
    Assert(pnf->IsTreeChange());

    AssertSz(FALSE, "CDirtyTreeRegion::Adjust is not written");

    if (IsDirty())
    {
    }
}


//+----------------------------------------------------------------------------
//
//  Member:     Accumulate
//
//  Synopsis:   Accumulate the dirty text region described by a CNotification
//
//  Arguments:  pnf - Notification documenting the change
//
//-----------------------------------------------------------------------------
void
CDirtyTextRegion::Accumulate(
    CNotification * pnf,
    long            cpFirst,
    long            cpLast,
    BOOL            fInnerRange)
{
    Assert(pnf);
    Assert(     pnf->IsTextChange()
            ||  pnf->IsLayoutChange());
    Assert(cpFirst >= 0);

    long    cp  = pnf->Cp(cpFirst) - cpFirst;
    long    cch = pnf->Cch();

    //
    //  Adjust the range to exclude the WCH_NODE chars
    //  (The cp is correctly bounded using the passed cpFirst,
    //   only the cch needs correction at this point)
    //

    if(fInnerRange)
    {
        cch -= 2;
    }

    switch (pnf->Type())
    {
    case NTYPE_CHARS_ADDED:
        TextAdded(cp, cch);
        break;

    case NTYPE_CHARS_DELETED:
        TextDeleted(cp, cch);
        break;

    default:
        TextChanged(cp, cch);
        break;
    }

    _cchNew = min(_cchNew, (cpLast - (_cp + cpFirst)));

#if DBG==1
    if (_cp != -1)
    {
        Assert(_cp >= 0);
        Assert(_cchNew >= 0);
        Assert(_cchOld >= 0);
    }
#endif
}


//+----------------------------------------------------------------------------
//
//  Member:     Adjust
//
//  Synopsis:   Adjust the recorded dirty region to take into account
//              nested changes
//
//  Arguments:  pnf - Notification documenting the change
//
//-----------------------------------------------------------------------------
void
CDirtyTextRegion::Adjust(
    CNotification * pnf,
    long            cpFirst,
    long            cpLast)
{
    if (IsDirty())
    {
        long cp         = pnf->Cp(cpFirst) - cpFirst;
        long cchChanged = pnf->CchChanged(cpLast);

        Assert(pnf->IsTextChange());

        if (cp < _cp)
        {
            _cp += cchChanged;
        }
        else if (cp < _cp + _cchNew)
        {
            _cchNew += cchChanged;
        }
    }
}


//-----------------------------------------------------------------------------
//
//  Member:     TextAdded
//
//  Synopsis:   Accumulate information about a text addition
//
//  Arguments:  pnf - Pointer to CNotification describing the change
//
//-----------------------------------------------------------------------------
void
CDirtyTextRegion::TextAdded(
    long cp,
    long cch)
{
    if (_cp == -1)
    {
        _cp     = cp;
        _cchNew = cch;
    }
    else
    {
        if (cp < _cp)
        {
            long dch = _cp - cp;
            _cp      = cp;
            _cchNew += dch + cch;
            _cchOld += dch;
        }
        else if (cp > _cp + _cchNew)
        {
            long dch = cp - (_cp + _cchNew);
            _cchNew += dch + cch;
            _cchOld += dch;
        }
        else
        {
            _cchNew += cch;
        }
    }
}


//-----------------------------------------------------------------------------
//
//  Member:     TextDeleted
//
//  Synopsis:   Accumulate information about a text deletion
//
//  Arguments:  pnf - Pointer to CNotification describing the change
//
//-----------------------------------------------------------------------------
void
CDirtyTextRegion::TextDeleted(
    long cp,
    long cch)
{
    if (_cp == -1)
    {
        _cp     = cp;
        _cchOld = cch;
    }
    else
    {
        long dch = _cchOld - _cchNew;

        if (cp < _cp)
        {
            _cchNew = max((_cp + _cchNew) - (cp + cch), 0L);
            _cp     = cp;
        }
        else if (cp < _cp + _cchNew)
        {
            _cchNew = cp - _cp + max((_cp + _cchNew) - (cp + cch), 0L);
        }
        else
        {
            _cchNew = cp - _cp;
        }

        _cchOld = _cchNew + dch + cch;
    }
}


//-----------------------------------------------------------------------------
//
//  Member:     TextChanged
//
//  Synopsis:   Accumulate information about a text change
//
//  Arguments:  pnf - Pointer to CNotification describing the change
//
//-----------------------------------------------------------------------------
void
CDirtyTextRegion::TextChanged(
    long cp,
    long cch)
{
    if (_cp == -1)
    {
        _cp     = cp;
        _cchOld = cch;
        _cchNew = cch;
    }
    else
    {
        long dch = _cchOld - _cchNew;

        if (cp < _cp)
        {
            _cchNew = max(_cp - cp + _cchNew, cch);
            _cp     = cp;
        }
        else
        {
            _cchNew = max(cp - _cp + cch, _cchNew);
        }

        _cchOld = _cchNew + dch;
    }
}
