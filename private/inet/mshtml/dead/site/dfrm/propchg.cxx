//----------------------------------------------------------------------------
//
//  Maintained  by   frankman
//
//  Copyright   (C) Microsoft Corporation, 1994-1995.
//              All rights Reserved.
//              Information contained herein is Proprietary and Confidential.
//
//  File        src\ddoc\datadoc\propchg.cxx
//
//  Contents    Implements CPropertyChange
//
//
//----------------------------------------------------------------------------

#include "headers.hxx"
#include "dfrm.hxx"

#include <propchg.hxx>
#include <sitedisp.h>


//+------------------------------------------------------------------------
//
//  Member:     CPropertyChange constructor
//
//  Synopsis:   initializes a propchange object
//
//-------------------------------------------------------------------------
CPropertyChange::CPropertyChange()
{
    Passivate();
}
//-End of Method ----------------------------------------------------------



//+------------------------------------------------------------------------
//
//  Member:     CPropertyChange destructor
//
//  Synopsis:   frees the allocated memory, should normally have happened
//              after UpdateProperties on the frame, so we will assert
//              but nether the less do the right thing
//
//-------------------------------------------------------------------------
CPropertyChange::~CPropertyChange()
{
    Passivate();
}
//-End of Method ----------------------------------------------------------






//+------------------------------------------------------------------------
//
//  Member:     Passivate
//
//  Synopsis:   cleans the object up, by freeing all used memory and resetting
//              state
//
//-------------------------------------------------------------------------
void CPropertyChange::Passivate()
{
    _fTemplateModified      = FALSE;
    _fDataSourceModified    = FALSE;
    _fRegenerate            = FALSE;
    _fCreateToFit           = FALSE;
    _fLinkInfoChanged       = FALSE;
    _fScroll                = FALSE;
    _fInvalidate            = FALSE;
    _iCurrentDispid         = 0;
    _DispidArray.DeleteAll();
    _fNeedGridResize        = FALSE;

}
//-End of Method ----------------------------------------------------------





//+------------------------------------------------------------------------
//
//  Member:     EnumReset
//
//  Synopsis:   should be called before starting enumerating the props
//
//-------------------------------------------------------------------------
void CPropertyChange::EnumReset()
{
    _iCurrentDispid = 0;
}
//-End of Method ----------------------------------------------------------




//+------------------------------------------------------------------------
//
//  Member:     EnumNextDispID
//
//  Synopsis:   should be called before starting enumerating the props
//
//-------------------------------------------------------------------------
BOOL CPropertyChange::EnumNextDispID(DISPID *pDispid)
{
    if (_DispidArray.Size() && _iCurrentDispid < _DispidArray.Size())
    {
        *pDispid = _DispidArray[_iCurrentDispid++];
        return (TRUE);
    }
    return FALSE;
}
//-End of Method ----------------------------------------------------------






//+------------------------------------------------------------------------
//
//  Member:     AddDispID
//
//  Synopsis:   adds a dispid to remember
//
//-------------------------------------------------------------------------
HRESULT CPropertyChange::AddDispID(DISPID dispid)
{
    HRESULT hr = S_OK;

    _fTemplateModified |= (dispid>=DISPID_TemplateModifyStart && dispid <= DISPID_TemplateModifyEnd);

    _fInvalidate = TRUE;
    switch (dispid)
    {
        case DISPID_ShowHeaders:
        case DISPID_ShowFooters:
            _fTemplateModified = TRUE;
            break;

        case DISPID_LinkChildFields:
        case DISPID_LinkMasterFields:
            _fLinkInfoChanged = TRUE;
            break;

        case DISPID_ControlSource:
        case DISPID_RowSource:
            _fDataSourceModified = TRUE;
            _fTemplateModified = TRUE;
            goto Cleanup;

        case STDPROPID_XOBJ_LEFT:
        case STDPROPID_XOBJ_TOP:
        case STDPROPID_XOBJ_WIDTH:
        case STDPROPID_XOBJ_HEIGHT:
        case DISPID_ItemsAcross:
        case DISPID_ColumnSpacing:
        case DISPID_RowSpacing:
        case DISPID_Direction:
        case DISPID_Repeat:
        case DISPID_DATADOC_Autosize:
        case DISPID_MinRows:
        case DISPID_MaxRows:
        case DISPID_MinCols:
        case DISPID_MaxCols:
        case DISPID_NewRecordShow:
            _fCreateToFit = TRUE;
            goto Cleanup;

        case DISPID_FONT:
            _fCreateToFit = TRUE;
            break; 


        case DISPID_ScrollLeft:
        case DISPID_ScrollTop:
            _fScroll = TRUE;
            goto Cleanup;

//      case DISPID_Scrollbars:
//          goto Cleanup;           // all we need is invalidate.
    }

    hr = _DispidArray.AppendIndirect(&dispid);
Cleanup:
    RRETURN(hr);

}
//-End of Method ----------------------------------------------------------











////////////////////////////////////////////////////////////////////////////////
//
//  CControlPropertyChange class section
//
////////////////////////////////////////////////////////////////////////////////


//+------------------------------------------------------------------------
//
//  Member:     CControlPropertyChange constructor
//
//  Synopsis:   initializes a propchange object
//
//-------------------------------------------------------------------------
CControlPropertyChange::CControlPropertyChange()
{
    _iCurrentVariant    = 0;
}
//-End of Method ----------------------------------------------------------


//+------------------------------------------------------------------------
//
//  Member:     Passivate
//
//  Synopsis:   cleans the object up, by freeing all used memory and resetting
//              state
//
//-------------------------------------------------------------------------
void CControlPropertyChange::Passivate()
{
    ULONG   cVars = _VariantArray.Size();

    VARIANT **ppvar;

    for (ppvar = _VariantArray;
         cVars > 0;
         cVars--, ppvar++)
    {

        VariantClear(*ppvar);
        delete (*ppvar);
    }
    _iCurrentVariant = 0 ;
    _VariantArray.DeleteAll();
    CPropertyChange::Passivate();
}
//-End of Method ----------------------------------------------------------



//+------------------------------------------------------------------------
//
//  Member:     EnumReset
//
//  Synopsis:   should be called before starting enumerating the props
//
//-------------------------------------------------------------------------
void CControlPropertyChange::EnumReset()
{
    CPropertyChange::EnumReset();
    _iCurrentVariant = 0;
}
//-End of Method ----------------------------------------------------------





//+------------------------------------------------------------------------
//
//  Member:     EnumNextDispID
//
//  Synopsis:   should be called before starting enumerating the props
//
//-------------------------------------------------------------------------
BOOL CControlPropertyChange::EnumNextVariant(VARIANT **ppvar)
{

    if (_VariantArray.Size() && _iCurrentVariant < _VariantArray.Size())
    {
        *ppvar = _VariantArray[_iCurrentVariant++];
        return (TRUE);
    }
    return FALSE;
}
//-End of Method ----------------------------------------------------------




//+------------------------------------------------------------------------
//
//  Member:     AddDispIDAndVariant
//
//  Synopsis:   adds a dispid to remember
//
//-------------------------------------------------------------------------
HRESULT CControlPropertyChange::AddDispIDAndVariant(DISPID dispid, VARIANT *pvar)
{
    if (SUCCEEDED(AddDispID(dispid)))
    {
        VARIANT *pvarNew = new VARIANT;
        if (pvarNew)
        {
            VariantInit(pvarNew);
            if (SUCCEEDED(VariantCopy(pvarNew, pvar)))
            {
                RRETURN(_VariantArray.Append(pvarNew));
            }
        }
    }
    RRETURN(E_OUTOFMEMORY);
}
//-End of Method ----------------------------------------------------------



