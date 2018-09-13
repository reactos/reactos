//================================================================================
//      File:   SELECT.CPP
//      Date:   7/9/97
//      Desc:   contains implementation of CSelectAO class.  CSelectAO implements
//              the Accessible Object proxy for an HTML Select element.
//      
//      Notes:  this is a unique AO implementation : a SELECT element has a window
//              handle, and its accessibility is therefore implemented by OLEACC.DLL.
//              a CSelectAO wraps a standard hwnd based accessible object that is 
//              created at initialization time.  All IAccessible methods except
//              get_accParent() are implemented, with CSelectAO wrapping the calls
//              to the standard IAccessible object.
//              
//
//================================================================================


//================================================================================
// includes
//================================================================================

#include "stdafx.h"
#include "select.h"
#include "oleacapi.h"

#ifndef _X_RESOURCE_H_
#define _X_RESOURCE_H_
#include "resource.h"
#endif

#ifndef _X_UTILS_H_
#define _X_UTILS_H_
#include "utils.h"
#endif


//================================================================================
// CSelectAO class implementation : public methods
//================================================================================

//-----------------------------------------------------------------------
//  CSelectAO::CSelectAO()
//
//  DESCRIPTION:
//
//      constructor
//
//  PARAMETERS:
//
//      pAOParent           pointer to the parent accessible object in 
//                          the AOM tree
//
//      nTOMIndex           index of the element from the TOM document.all 
//                          collection.
//      
//      nChildID            unique Child ID
//
//      hWnd                pointer to the window of the trident object that 
//                          this object corresponds to.
//  RETURNS:
//
//      None.
//
// ----------------------------------------------------------------------

CSelectAO::CSelectAO(CTridentAO * pAOMParent,
                     CDocumentAO * pDocAO, 
                     UINT nTOMIndex,
                     UINT nChildID,
                     HWND hWnd)
: CTridentAO(pAOMParent,pDocAO,nTOMIndex,nChildID,hWnd)
{
    //------------------------------------------------
    // assign the delegating IUnknown to CSelectAO :
    // this member will be overridden in derived class
    // constructors so that the delegating IUnknown 
    // will always be at the derived class level.
    //------------------------------------------------

    m_pIUnknown     = (IUnknown *)this;
    
    m_pIAccessible = NULL;


    //--------------------------------------------------
    // before we successfully create a proxy for the 
    // Select window, we assume that it is a combobox.
    // This is the best that we can do given the information
    // that we have at this point.  The role will change
    // if the created proxy is a different type of window.
    //--------------------------------------------------

    m_lRole = ROLE_SYSTEM_COMBOBOX;

    //--------------------------------------------------
    // set the item type so that it can be accessed
    // via base class pointer.
    //--------------------------------------------------

    m_lAOMType = AOMITEM_SELECTLIST;
}



//-----------------------------------------------------------------------
//  CSelectAO::~CSelectAO()
//
//  DESCRIPTION:
//
//      CSelectAO class destructor.
//
//  PARAMETERS:
//
//  RETURNS:
//
//      None.
//
// ----------------------------------------------------------------------

CSelectAO::~CSelectAO()
{
    //--------------------------------------------------
    // release IAccessible pointer of embedded object 
    // if it was allocated via call to 
    // CreateStdAccessibleObject()
    //--------------------------------------------------

    if(m_pIAccessible)
        m_pIAccessible->Release();
}


//-----------------------------------------------------------------------
//  CSelectAO::Init()
//
//  DESCRIPTION:
//
//      Initialization : set values of data members
//
//  PARAMETERS:
//
//      pTOMObjUnk      pointer to IUnknown of TOM object.
//
//  RETURNS:
//
//      S_OK | E_FAIL
//
// ----------------------------------------------------------------------

HRESULT CSelectAO::Init(IUnknown * pTOMObjIUnk)
{
    HRESULT hr          = E_FAIL;

    assert( pTOMObjIUnk );

    //
    // call down to base class to set unknown pointer.
    //--------------------------------------------------

    hr = CTridentAO::Init(pTOMObjIUnk);
    if(hr)
        goto Cleanup;

    //
    // create an accessible object for the select list
    // element.
    //
    // (1) get the window at the location of the select
    // list element
    //--------------------------------------------------

    hr = createStdAccessibleObjectIfVisible(&m_pIAccessible);
    if (hr==S_FALSE)
        hr = S_OK;

Cleanup:
    return( hr );
}


//=======================================================================
// IUnknown interface
//=======================================================================

//-----------------------------------------------------------------------
//  CSelectAO::QueryInterface()
//
//  DESCRIPTION:
//
//      Standard QI implementation : the CSelectAO object only implements
//      IUnknown. BUGBUG - this function acutally can break com IUnknown 
//      identity, which is bad. but due to the way MSAA clients use these
//      objects it should be safe to do.  They don't cache pointeres, and
//      the only case where this break happens is if a select's IUnknown 
//      is accessed before its HWND has been created (e.g. not scrolled 
//      into view yet),and that pointer compared to one retrieved after.
//
//  PARAMETERS:
//
//      riid        REFIID of requested interface.
//      ppv         pointer to interface in.
//
//  RETURNS:
//
//      E_NOINTERFACE | NOERROR.
//
// ----------------------------------------------------------------------

STDMETHODIMP CSelectAO::QueryInterface(REFIID riid, void** ppv)
{
    HRESULT hr = E_FAIL;

    if ( !ppv )
        return( E_INVALIDARG );


    *ppv = NULL;

    if ( !m_pIAccessible )
    {
        hr = createStdAccessibleObjectIfVisible(&m_pIAccessible);
        if (hr == S_FALSE)
            hr = S_OK;
        if (hr)
        {
            hr = E_NOINTERFACE;
            goto Cleanup;
        }
    }

    // if we actually have an inner object, delegate to it
    if (m_pIAccessible)
    {
        hr = m_pIAccessible->QueryInterface(riid,ppv);
    }
    else if (riid == IID_IEnumVARIANT)
    {
        // we can't get to the std proxy object so
        // don't allow acces to this interface..
        //--------------------------------------------------
        hr = E_NOINTERFACE;
    }
    else
    {
        // let the base AO class try the rest 
        //--------------------------------------------------
        hr = CTridentAO::QueryInterface(riid,ppv);
    }

Cleanup:
    return( hr );
}


//================================================================================
// CSelectAO Accessible Interface helper methods
//================================================================================

//-----------------------------------------------------------------------
//  CSelectAO::GetAccName()
//
//  DESCRIPTION:
//  
//      returns accessible name to client.  
///     This method wraps embedded AO call.
//
//  PARAMETERS:
//
//      lChild          child ID
//      pbstrName       pointer to array to return child name in.
//
//  RETURNS:
//
//      HRESULT :   S_OK | E_FAIL | DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT 
CSelectAO::GetAccName(long lChild, BSTR * pbstrName)
{
    HRESULT hr = S_OK;
    
    assert( pbstrName );
    
    if(!pbstrName)
        return(E_INVALIDARG);

    *pbstrName = NULL;

    //--------------------------------------------------
    // if we can't get to the std proxy object, 
    // we can't get the name.
    //--------------------------------------------------

    if(!m_pIAccessible)
    {
        hr = createStdAccessibleObjectIfVisible(&m_pIAccessible);
        if (FAILED(hr))
            goto Cleanup;
        hr = S_OK;
    }

    if (!(m_bstrName))
    {
        if(m_pIAccessible)
        {
            // delegate to embedded object 
            //--------------------------------------------------
            VARIANT varID;

            hr = loadVariant(&varID,CHILDID_SELF); 
            if(hr)
                goto Cleanup;

            hr = m_pIAccessible->get_accName(varID,pbstrName);
            if(hr)
                goto Cleanup;

            m_bstrName = SysAllocString(*pbstrName);
        }
        else
        {
            hr = getTitleFromIHTMLElement( pbstrName );
            if (hr == S_OK && !*pbstrName )
                hr = DISP_E_MEMBERNOTFOUND;
        }
    }
    else
    {
        *pbstrName = SysAllocString(m_bstrName);
    }

Cleanup:            
    return( hr );
}

//-----------------------------------------------------------------------
//  CSelectAO::GetAccValue(long lChild, BSTR * pbstrValue)
//
//  DESCRIPTION:
//
//      returns value string to client.
//      This method wraps embedded AO call.
//
//  PARAMETERS:
//
//      lChild      ChildID/SelfID
//
//      pbstrValue  returned value string
//
//  RETURNS:
//
//      HRESULT :   S_OK if success, S_FALSE if fail, DISP_E_MEMBERNOTFOUND
//                  for no implement.
//
// ----------------------------------------------------------------------

HRESULT 
CSelectAO::GetAccValue(long lChild, BSTR * pbstrValue)
{
    
    HRESULT hr = S_OK;

    assert( pbstrValue );

    if(!pbstrValue)
        return(E_INVALIDARG);

    *pbstrValue = NULL;

    // if no std proxied object, try to create.
    // if create fails, we cant get the value.
    //--------------------------------------------------
    if(!m_pIAccessible)
    {
        hr = createStdAccessibleObjectIfVisible(&m_pIAccessible);
        if (FAILED(hr))
            goto Cleanup;
        hr = S_OK;
    }

    //--------------------------------------------------
    // delegate to embedded object 
    //--------------------------------------------------

    if(!(m_bstrValue))
    {
        if (m_pIAccessible)
        {
            VARIANT varID;

            hr = loadVariant(&varID,CHILDID_SELF);
            if (hr)
                goto Cleanup;

            hr = m_pIAccessible->get_accValue(varID,pbstrValue);
            if (hr)
                goto Cleanup;

            m_bstrValue = SysAllocString(*pbstrValue);
        }
        // else ????
    }
    else
    {
        *pbstrValue = SysAllocString(m_bstrValue);
    }

    
Cleanup:
    return( hr );
}


//-----------------------------------------------------------------------
//  CSelectAO::GetAccDescription()
//
//  DESCRIPTION:
//
//      returns description string to client.
//      This method wraps embedded AO call.
//
//  PARAMETERS:
//
//      lChild              Child/Self ID
//
//      pbstrDescription        Description string returned to client.
//  
//  RETURNS:
//
//      HRESULT :   S_OK | E_FAIL | DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT 
CSelectAO::GetAccDescription(long lChild, BSTR * pbstrDescription)
{
    HRESULT hr  = E_FAIL;
    
    assert( pbstrDescription );

    if (!pbstrDescription)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *pbstrDescription = NULL;

    //--------------------------------------------------
    // if no std proxied object, try to create.
    // if create fails, we cant get the description.
    //--------------------------------------------------

    if(!m_pIAccessible)
    {
        hr = createStdAccessibleObjectIfVisible(&m_pIAccessible);
        if (FAILED(hr))
            goto Cleanup;
        hr = S_OK;
    }

    if(!(m_bstrDescription))
    {
        if(m_pIAccessible)
        {
            // delegate to embedded object 
            //--------------------------------------------------
            VARIANT varID;

            hr = loadVariant(&varID,CHILDID_SELF); 
            if(hr)
                goto Cleanup;

            hr = m_pIAccessible->get_accDescription(varID,pbstrDescription);
            if (hr)
                goto Cleanup;

            m_bstrDescription = SysAllocString(*pbstrDescription);
        }
        else
        {
            // no std object, make a guess
            hr = GetResourceStringValue(IDS_SELECT_DESCRIPTION, pbstrDescription);
        }
    }
    else
    {
        *pbstrDescription = SysAllocString(m_bstrDescription);
    }


Cleanup:
    return(S_OK);
}

//-----------------------------------------------------------------------
//  CSelectAO::GetAccState()
//
//  DESCRIPTION:
//
//      returns state of area : always linked, maybe focusable/focused.
//      This method wraps embedded AO call
//
//  PARAMETERS:
//      
//      lChild      ChildID
//      plState     long to store returned state var in.
//
//  RETURNS:
//
//      HRESULT :   S_OK | E_FAIL | DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------
    
HRESULT 
CSelectAO::GetAccState(long lChild, long *plState)
{
    HRESULT hr = S_OK;
    VARIANT varID;
    VARIANT varState;

        
    if(!plState)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //--------------------------------------------------
    // the state is set to invisible until proven
    // otherwise.
    //--------------------------------------------------

    *plState = STATE_SYSTEM_INVISIBLE;

    //--------------------------------------------------
    // if no std proxied object, try to create.
    // if create fails, we cant get the state.
    //--------------------------------------------------

    if(!m_pIAccessible)
    {
        hr = createStdAccessibleObjectIfVisible(&m_pIAccessible);
        if (hr==S_FALSE)
        {
            *plState |= STATE_SYSTEM_UNAVAILABLE;
            hr = S_OK;
        }

        //--------------------------------------------------
        // the state is already set to invisible : if the 
        // object creation failed because object was offscreen,
        // return S_OK with invisible state. else return more
        // serious error.
        //--------------------------------------------------
        if (hr)
            goto Cleanup;

        //--------------------------------------------------
        // if we can create a std accessible object from here, 
        // we know its not invisible any more. Reset state 
        // var to 'normal' state.
        //--------------------------------------------------

        *plState = 0;
    }

    if (m_pIAccessible)
    {
        //--------------------------------------------------
        // delegate to embedded object 
        //--------------------------------------------------

        hr = loadVariant(&varID,CHILDID_SELF);
        if (hr)
            goto Cleanup;


        hr = m_pIAccessible->get_accState(varID,&varState);
        if (hr)
            goto Cleanup;

        hr = unpackVariant(varState,plState);
    }

Cleanup:
    return( hr );
}


//-----------------------------------------------------------------------
//  CSelectAO::GetAccDefaultAction()
//
//  DESCRIPTION:
//      returns description string for default action
//      This method wraps embedded AO call.
//  
//  PARAMETERS:
//
//      lChild          child /self ID
//
//      pbstrDefAction  returned description string.
//
//  RETURNS:
//
//      HRESULT :   S_OK if success, E_FAIL if fail, DISP_E_MEMBERNOTFOUND
//                  for no implement.
//
// ----------------------------------------------------------------------

HRESULT 
CSelectAO::GetAccDefaultAction(long lChild, BSTR * pbstrDefAction)
{
    HRESULT hr = S_OK;


    if(!pbstrDefAction)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *pbstrDefAction = NULL;

    //--------------------------------------------------
    // if no std proxied object, try to create.
    // if create fails, we cant get the default action.
    //--------------------------------------------------

    if(!m_pIAccessible)
    {
        hr = createStdAccessibleObjectIfVisible(&m_pIAccessible);
        if (FAILED(hr))
            goto Cleanup;
        hr = S_OK;
    }

    //--------------------------------------------------
    // delegate to embedded object if it implements
    // IAccessible
    //--------------------------------------------------

    if(!(m_bstrDefaultAction))
    {
        if (m_pIAccessible)
        {
            VARIANT varID;

            hr = loadVariant(&varID,CHILDID_SELF);
            if (hr)
                goto Cleanup;

            hr = m_pIAccessible->get_accDefaultAction(varID,pbstrDefAction);
            if (hr)
                goto Cleanup;

            m_bstrDefaultAction = SysAllocString(*pbstrDefAction);
        }
        else
        {
            hr = GetResourceStringValue(IDS_SELECT_ACTION, pbstrDefAction );
        }
    }
    else
    {
        *pbstrDefAction = SysAllocString(m_bstrDefaultAction);
    }

Cleanup:
    return( hr );
}


//-----------------------------------------------------------------------
//  CSelectAO::AccDoDefaultAction()
//
//  DESCRIPTION:
//      selects plugin.
//      This method wraps embedded AO call.
//      
//  PARAMETERS:
//
//      lChild      child / self ID
//
//  RETURNS:
//
//      HRESULT :   S_OK | E_FAIL | DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT 
CSelectAO::AccDoDefaultAction(long lChild)
{
    HRESULT hr=S_OK;
    VARIANT varID;


    // if no std proxied object, try to create.
    // if create fails, we cant do the default action.
    if(!m_pIAccessible)
    {
        hr = createStdAccessibleObjectIfVisible(&m_pIAccessible);
        if (FAILED(hr))
            goto Cleanup;
        hr = S_OK;
    }

    if (m_pIAccessible)
    {
        //--------------------------------------------------
        // delegate to embedded object
        //--------------------------------------------------

        hr = loadVariant(&varID,CHILDID_SELF);
        if (hr)
            goto Cleanup;

        hr = m_pIAccessible->accDoDefaultAction(varID);
    }
    else
        hr = E_PENDING;

Cleanup:
    return( hr );
}


//-----------------------------------------------------------------------
//  CSelectAO::GetAccChildCount()
//
//  DESCRIPTION:
//      returns child count to client.
//      This method wraps embedded AO call.
//
//  PARAMETERS:
//
//      pChildCount pointer to long var to fill out w/child count.
//
//  RETURNS:
//
//      HRESULT :   S_OK | E_FAIL 
//
// ----------------------------------------------------------------------

HRESULT 
CSelectAO::GetAccChildCount(long* pChildCount)
{
    HRESULT hr = E_FAIL;

    assert( pChildCount );

    if(!pChildCount)
        return(E_INVALIDARG);

    *pChildCount = 0;

    //--------------------------------------------------
    // if no std proxied object, try to create.
    // if create fails, we cant do the default action.
    //--------------------------------------------------

    if(!m_pIAccessible)
    {
        hr = createStdAccessibleObjectIfVisible(&m_pIAccessible);
        if (FAILED(hr))
            goto Cleanup;
        hr = S_OK;
    }

    //--------------------------------------------------
    // delegate to embedded object
    //--------------------------------------------------
    if (m_pIAccessible)
        hr = m_pIAccessible->get_accChildCount(pChildCount);
    // else just return

Cleanup:
    return( hr );   
}

//-----------------------------------------------------------------------
//  CSelectAO::GetAccChild()
//
//  DESCRIPTION:
//      returns IDispatch * of requested child to client.
//      This method wraps embedded AO call.
//      
//  PARAMETERS:
//
//      ppdispChild IDispatch pointer to return to caller.
//
//  RETURNS:
//
//      HRESULT :   S_OK | E_FAIL 
//
// ----------------------------------------------------------------------

HRESULT 
CSelectAO::GetAccChild(long lChild, IDispatch ** ppdispChild)
{
    HRESULT hr;

    assert( ppdispChild );

    if(!ppdispChild)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppdispChild = NULL;

    //--------------------------------------------------
    // if no std proxied object, try to create.
    // if create fails, we cant get children
    //--------------------------------------------------

    if(!m_pIAccessible)
    {
        hr = createStdAccessibleObjectIfVisible(&m_pIAccessible);
        if (FAILED(hr))
            goto Cleanup;
        hr = S_OK;
    }

    if (m_pIAccessible)
    {
        //--------------------------------------------------
        // delegate to embedded object
        //--------------------------------------------------

        VARIANT varID;

        hr = loadVariant(&varID,CHILDID_SELF);
        if (hr)
            goto Cleanup;
        
        hr = m_pIAccessible->get_accChild(varID,ppdispChild);
    }
    else 
        hr = S_FALSE;

Cleanup:
    return( hr );
}


//-----------------------------------------------------------------------
//  CSelectAO::GetAccHelp()
//
//  DESCRIPTION:
//      returns help string to client.
//      This method wraps embedded AO call.
//      
//  PARAMETERS:
//
//      lChild      child ID
//      pbstrHelp   help string to return to caller..
//
//  RETURNS:
//
//      HRESULT :   S_OK | E_FAIL 
//
// ----------------------------------------------------------------------

HRESULT 
CSelectAO::GetAccHelp(long lChild, BSTR * pbstrHelp)
{
    HRESULT hr = S_OK;

    assert( pbstrHelp );

    if(!pbstrHelp)
        return(E_INVALIDARG);

    *pbstrHelp = NULL;

    //--------------------------------------------------
    // if no std proxied object, try to create.
    // if create fails, we cant get help string
    //--------------------------------------------------

    if(!m_pIAccessible)
    {
        hr = createStdAccessibleObjectIfVisible(&m_pIAccessible);
        if (FAILED(hr))
            goto Cleanup;
        hr = S_OK;
    }

    if (m_pIAccessible)
    {
        //--------------------------------------------------
        // delegate to embedded object
        //--------------------------------------------------
        VARIANT varID;
        
        hr = loadVariant(&varID,CHILDID_SELF);
        if (hr)
            goto Cleanup;

        hr = m_pIAccessible->get_accHelp(varID,pbstrHelp);
    }
    //else just return 

Cleanup:
    return( hr );
}

//-----------------------------------------------------------------------
//  CSelectAO::GetAccHelpTopic()
//
//  DESCRIPTION:
//      returns help file name and topic to client.
//      This method wraps embedded AO call.
//      
//  PARAMETERS:
//
//      pbstrHelpFile   help string to return to caller..
//      lChild          child ID
//      pidTopic        long var to return to caller.
//      
//
//  RETURNS:
//
//      HRESULT :   S_OK | E_FAIL 
//
// ----------------------------------------------------------------------

HRESULT 
CSelectAO::GetAccHelpTopic(BSTR * pbstrHelpFile, long lChild,long * pidTopic)
{
    HRESULT hr = S_OK;

    assert ( pbstrHelpFile );
    assert ( pidTopic );

    if( !pbstrHelpFile || !pidTopic )
        return E_INVALIDARG;

    *pbstrHelpFile = NULL;
    *pidTopic = 0;

    //--------------------------------------------------
    // if no std proxied object, try to create.
    // if create fails, we cant get help topic
    //--------------------------------------------------
    if(!m_pIAccessible)
    {
        hr = createStdAccessibleObjectIfVisible(&m_pIAccessible);
        if (FAILED(hr))
            goto Cleanup;
        hr = S_OK;
    }

    if (m_pIAccessible)
    {
        //--------------------------------------------------
        // delegate to embedded object
        //--------------------------------------------------
        VARIANT varID;

        hr = loadVariant(&varID,CHILDID_SELF);
        if (hr)
            goto Cleanup;

        hr = m_pIAccessible->get_accHelpTopic(pbstrHelpFile,varID,pidTopic);
    }
    // else just return

Cleanup:
    return( hr );
}

//-----------------------------------------------------------------------
//  CSelectAO::AccLocation()
//
//  DESCRIPTION:
//      returns location of app to client.
//      This method wraps embedded AO call.
//      
//  PARAMETERS:
//
//      pxLeft
//      pyTop
//      pcxWidth
//      pcyHeight       pointers to return coords in.
//      
//      lChild          child ID    
//
//  RETURNS:
//
//      HRESULT :   S_OK | E_FAIL 
//
// ----------------------------------------------------------------------

HRESULT 
CSelectAO::AccLocation(long * pxLeft, long * pyTop, long * pcxWidth,long * pcyHeight, long lChild)
{
    HRESULT hr=S_OK;

    assert( pxLeft );
    assert( pyTop );
    assert( pcxWidth );
    assert( pcyHeight );

    if(!pxLeft || !pyTop || !pcyHeight || !pcxWidth )
        return(E_INVALIDARG);

    //--------------------------------------------------
    // if no std proxied object, try to create.
    // if create fails, we cant get help string
    //--------------------------------------------------

    if(!m_pIAccessible)
    {
        hr = createStdAccessibleObjectIfVisible(&m_pIAccessible);
        if (FAILED(hr))
            goto Cleanup;
        hr = S_OK;
    }

    if (m_pIAccessible)
    {
        //--------------------------------------------------
        // delegate to embedded object
        //--------------------------------------------------
        
        VARIANT varID;
            
        hr = loadVariant(&varID,CHILDID_SELF);
        if (hr)
            goto Cleanup;

        hr = m_pIAccessible->accLocation(pxLeft,
                                         pyTop,
                                         pcxWidth,
                                         pcyHeight,
                                         varID);
    }
    else 
    {
        //--------------------------------------------------
        // if we can't create a proxy object, 
        // get the location from the default method.
        //--------------------------------------------------

        hr = CTridentAO::AccLocation(pxLeft,
                                     pyTop,
                                     pcxWidth,
                                     pcyHeight,
                                     CHILDID_SELF);
    }

Cleanup:
    return( hr );
}

//-----------------------------------------------------------------------
//  CSelectAO::GetAccRole()
//
//  DESCRIPTION:
//      returns role to client.
//      This method wraps embedded AO call.
//      
//  PARAMETERS:
//
//      lChild          child ID    
//      plRole          pointer to role var
//
//  RETURNS:
//
//      HRESULT :   S_OK | E_FAIL 
//
// ----------------------------------------------------------------------

HRESULT 
CSelectAO::GetAccRole(long lChild, long *plRole)
{
    HRESULT hr=S_OK;

    assert( plRole );

    if(!plRole)
        return(E_INVALIDARG);

    *plRole = m_lRole;

    //--------------------------------------------------
    // if no std proxied object, try to create.
    // if create fails w/S_FALSE return code, 
    // we return default role.  If create fails in a non
    // standard (more serious) manner, we notify
    // the user.
    //--------------------------------------------------

    if(!m_pIAccessible)
    {
        hr = createStdAccessibleObjectIfVisible(&m_pIAccessible);
        if (hr == S_FALSE)
            hr = S_OK;
    }

    if (m_pIAccessible)
    {
        VARIANT varID;   
        VARIANT varRole;

        // delegate to embedded object
        hr = loadVariant(&varID,CHILDID_SELF);
        if (hr)
            goto Cleanup;

        hr = m_pIAccessible->get_accRole(varID,&varRole);
        if (hr)
            goto Cleanup;

        hr = unpackVariant(varRole,plRole);
    }
    else
    {
        long lObj;

        hr = getObjectId( &lObj );
        if (hr)
            goto Cleanup;

        if (lObj == OBJID_WINDOW)
            *plRole = ROLE_SYSTEM_LIST;
        else
            *plRole = ROLE_SYSTEM_COMBOBOX;
    }

Cleanup:
    return( hr );
}

//-----------------------------------------------------------------------
//  CSelectAO::GetAccFocus()
//
//  DESCRIPTION:
//      returns IUnknown * of object that has focus to client.
//      This method wraps embedded AO call.
//      
//  PARAMETERS:
//
//      ppIUnknown  pointer to focused element to return to caller.
//
//  RETURNS:
//
//      HRESULT :   S_OK | E_FAIL 
//
// ----------------------------------------------------------------------

HRESULT 
CSelectAO::GetAccFocus(IUnknown **ppIUnknown)
{
    HRESULT hr=S_OK;

    assert( ppIUnknown );

    if(!ppIUnknown)
        return(E_INVALIDARG);

    *ppIUnknown  = NULL;

    //--------------------------------------------------
    // if no std proxied object, try to create.
    // if create fails, we cant get focus
    //--------------------------------------------------
    if(!m_pIAccessible)
    {
        hr = createStdAccessibleObjectIfVisible(&m_pIAccessible);
        if (FAILED(hr))
            goto Cleanup;
        hr = S_OK;
    }

    if (m_pIAccessible)
    {
        //--------------------------------------------------
        // delegate to embedded object & get IUnknown from method.
        //--------------------------------------------------
        VARIANT varIUnk;

        hr = m_pIAccessible->get_accFocus(&varIUnk);
        if (hr)
            goto Cleanup;

        hr = unpackVariant(varIUnk,ppIUnknown);
    }
    else
    {
        hr = CTridentAO::GetAccFocus(ppIUnknown);
    }

Cleanup:
    return( hr );
}


    
//-----------------------------------------------------------------------
//  CSelectAO::GetAccSelection()
//
//  DESCRIPTION:
//      returns IUnknown * of selected object to client, or IUnknown
//      of IEnumVariant interface that contains selection of objects.
//      This method wraps embedded AO call.
//      
//  PARAMETERS:
//
//      ppIUnknown  pointer to focused element to return to caller.
//
//  RETURNS:
//
//      HRESULT :   S_OK | E_FAIL 
//
// ----------------------------------------------------------------------

HRESULT 
CSelectAO::GetAccSelection(IUnknown **ppIUnknown)
{
    HRESULT hr;

    assert( ppIUnknown );

    if(!ppIUnknown)
        return(E_INVALIDARG);

    *ppIUnknown  = NULL;

    //--------------------------------------------------
    // if no std proxied object, try to create.
    // if create fails, we cant get selection
    //--------------------------------------------------
    if(!m_pIAccessible)
    {
        hr = createStdAccessibleObjectIfVisible(&m_pIAccessible);
        if (FAILED(hr))
            goto Cleanup;
        hr = S_OK;
    }

    if (m_pIAccessible)
    {
        //--------------------------------------------------
        // delegate to embedded object.
        //--------------------------------------------------

        VARIANT varIUnk;

        hr = m_pIAccessible->get_accSelection(&varIUnk);
        if (hr)
            goto Cleanup;


        //--------------------------------------------------
        // get IUnknown from method
        //--------------------------------------------------

        hr = unpackVariant(varIUnk,ppIUnknown);
    }
    else
    {
        hr = CTridentAO::GetAccSelection(ppIUnknown);
    }

Cleanup:
    return( hr );
}

//-----------------------------------------------------------------------
//  CSelectAO::AccNavigate()
//
//  DESCRIPTION:
//      navigates from one object to next in specified direction.
//      This method wraps embedded AO call.
//      
//  PARAMETERS:
//
//      navDir      constant : direction to navigate in
//      lStart      id of object to start navigation from.
//      ppIUnknown  pointer to focused element to return to caller.
//
//  RETURNS:
//
//      HRESULT :   S_OK | E_FAIL 
//
// ----------------------------------------------------------------------

HRESULT CSelectAO::AccNavigate(long navDir, long lStart,IUnknown **ppIUnknown)
{
    HRESULT hr;

    assert( ppIUnknown );

    if(!ppIUnknown)
        return(E_INVALIDARG);

    *ppIUnknown  = NULL;

    // navigate in relation to the object 
    // itself or one of its specified children
    if ( lStart != CHILDID_SELF )
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    // if no std proxied object, try to create.
    // if create fails, we can only navigate to siblings
    if(!m_pIAccessible )
    {

        hr = createStdAccessibleObjectIfVisible(&m_pIAccessible);
        if (FAILED(hr))
            goto Cleanup;
        hr = S_OK;
    }


    // Navigate w/ respect to the object, so only
    //  first/lastchild navigation is valid
    switch( navDir )
    {
    case NAVDIR_FIRSTCHILD :
    case NAVDIR_LASTCHILD :
        if (m_pIAccessible )
        {
            // delegate to embedded object
            VARIANT varStart;
            VARIANT varIUnk;

            // load start value variant.
            hr = loadVariant(&varStart,lStart);
            if (hr)
                goto Cleanup;

            hr = m_pIAccessible->accNavigate(navDir, varStart, &varIUnk);
            if (hr)
                goto Cleanup;

            // get IUnknown from method.
            hr = unpackVariant(varIUnk,ppIUnknown);
        }
        else
        {
            // do not support navigation into children if
            // the std accessible object proxy has not
            // been created yet.
            hr = S_FALSE;
        }
        break;

    case NAVDIR_NEXT :
    case NAVDIR_PREVIOUS :
        // sibling navigation: we need to delegate to parent
        // passing this object's child ID to the parent.
        if(m_pParent)
            hr = m_pParent->AccNavigate(navDir,GetChildID(),ppIUnknown);
        else
            hr = S_FALSE;
            break;  

    default :
        hr = DISP_E_MEMBERNOTFOUND;
        break;
    }


Cleanup:
    return( hr );
}


//-----------------------------------------------------------------------
//  CSelectAO::AccHitTest()
//
//  DESCRIPTION:
//      returns IUnknown * of object at tested coordinates.
//      This method wraps embedded AO call.
//      
//  PARAMETERS:
//
//      xLeft       x coord.
//      yTop        y coord.
//      ppIUnknown  pointer to focused element to return to caller.
//
//  RETURNS:
//
//      HRESULT :   S_OK | E_FAIL 
//
//
//  BUGBUG : our current architecture makes no provisions
//  for Accessible element children of an embedded
//  HWND object. In the case of select lists, this is 
//  fine.  However, other objects that implement 
//  accessible elements will not be able to resolve
//  those children.  This should be KB'd for the
//  4.01 release and fixed in IE5.
// ----------------------------------------------------------------------

HRESULT 
CSelectAO::AccHitTest(long xLeft, long yTop,IUnknown **ppIUnknown)
{
    HRESULT hr=S_OK;
    VARIANT varIUnk;

    assert( ppIUnknown );

    if(!ppIUnknown)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppIUnknown = NULL;

    //--------------------------------------------------
    // if no std proxied object, try to create.
    // if create fails, we can't process AccHitTest
    //--------------------------------------------------
    if(!m_pIAccessible)
    {
        hr = createStdAccessibleObjectIfVisible(&m_pIAccessible);
        if (FAILED(hr))
            goto Cleanup;
        hr = S_OK;
    }

    if (m_pIAccessible)
    {
        hr = m_pIAccessible->accHitTest(xLeft,yTop,&varIUnk);
        if (hr)
            goto Cleanup;

        if(varIUnk.vt == VT_I4)
        {
            // for now, all element children are mapped to the 
            // parent object.
            *ppIUnknown = m_pIAccessible;
            hr = S_OK;
        }
        else
        {
            // get IDispatch/lVal from method
            hr = unpackVariant(varIUnk,(IDispatch **)ppIUnknown);
        }
    }
    else
        hr = CTridentAO::AccHitTest(xLeft,yTop,ppIUnknown);

Cleanup:
    return( hr );

}

//-----------------------------------------------------------------------
//  CSelectAO::AccSelect()
//
//  DESCRIPTION:
//      selects object.
//      This method wraps embedded AO call.
//      
//  PARAMETERS:
//
//      lflagsSel   selection flags
//      lChild      child to select.
//
//  RETURNS:
//
//      HRESULT :   S_OK | E_FAIL 
//
// ----------------------------------------------------------------------

HRESULT 
CSelectAO::AccSelect(long flagsSel, long lChild)
{
    HRESULT hr=S_OK;

    //--------------------------------------------------
    // if no std proxied object, try to create.
    // if create fails, we can only select this object
    //--------------------------------------------------

    if( !m_pIAccessible )
    {

        hr = createStdAccessibleObjectIfVisible(&m_pIAccessible);
        if (FAILED(hr))
            goto Cleanup;
        hr = S_OK;
    }


    if (m_pIAccessible)
    {
        // delegate to embedded object
        VARIANT varID;

        hr = loadVariant(&varID,CHILDID_SELF);
        if (hr)
            goto Cleanup;

        hr = m_pIAccessible->accSelect(flagsSel,varID);
    }
    else
    {
        // only SELFLAG_TAKEFOCUS is supported.
        if ( !(flagsSel & SELFLAG_TAKEFOCUS) )
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }

        // set focus on select list.
        CComQIPtr<IHTMLControlElement,&IID_IHTMLControlElement> pIHTMLControlElement(m_pTOMObjIUnk);

        if ( !pIHTMLControlElement )
            hr = E_NOINTERFACE;
        else
            hr = pIHTMLControlElement->focus();
    }

Cleanup:
    return( hr );
}


//-----------------------------------------------------------------------
//  CSelectAO::GetAccKeyboardShortcut()
//
//  DESCRIPTION:
//      returns keystroke associated with object.
//      This method wraps embedded AO call.
//      
//  PARAMETERS:
//
//      xLeft       x coord.
//      yTop        y coord.
//      ppIUnknown  pointer to focused element to return to caller.
//
//  RETURNS:
//
//      HRESULT :   S_OK | E_FAIL 
//
// ----------------------------------------------------------------------

HRESULT 
CSelectAO::GetAccKeyboardShortcut(long lChild, BSTR * pbstrKeyboardShortcut)
{
    HRESULT hr=S_OK;

    assert( pbstrKeyboardShortcut );

    if(!pbstrKeyboardShortcut)
        return(E_INVALIDARG);

    *pbstrKeyboardShortcut = NULL;
        
    //--------------------------------------------------
    // if no std proxied object, try to create.
    // if create fails, we can't get keyboard shortcut.
    //--------------------------------------------------
    if(!m_pIAccessible)
    {
        hr = createStdAccessibleObjectIfVisible(&m_pIAccessible);
        if (FAILED(hr))
            goto Cleanup;
        hr = S_OK;
    }

    if (m_pIAccessible)
    {
        // delegate to embedded object
        VARIANT varID;

        hr = loadVariant(&varID,CHILDID_SELF);
        if (hr)
            goto Cleanup;

        hr = m_pIAccessible->get_accKeyboardShortcut(varID,pbstrKeyboardShortcut);
    }
    // else just return

Cleanup:
    return( hr );
}


//-----------------------------------------------------------------------
//  CSelectAO::SetAccName()
//
//  DESCRIPTION:
//      sends name string to object from client.
//      This method wraps embedded AO call.
//      
//  PARAMETERS:
//
//      lChild      Child ID
//      szName      BSTR to submit to object.
//
//  RETURNS:
//
//      HRESULT :   S_OK | E_FAIL 
//
// ----------------------------------------------------------------------

HRESULT 
CSelectAO::SetAccName(long lChild, BSTR bstrName)
{
    HRESULT hr=S_OK;

    assert ( bstrName );

    if(!bstrName)
        return(E_INVALIDARG);

    bstrName = NULL;

    //--------------------------------------------------
    // if no std proxied object, try to create.
    // if create fails, we can't set the name.
    //--------------------------------------------------
    if(!m_pIAccessible)
    {
        hr = createStdAccessibleObjectIfVisible(&m_pIAccessible);
        if (FAILED(hr))
            goto Cleanup;
    }

    if (m_pIAccessible)
    {
        // delegate to embedded object
        VARIANT varID;

        hr = loadVariant(&varID,CHILDID_SELF);
        if (hr)
            goto Cleanup;

        hr = m_pIAccessible->put_accName(varID,bstrName);
    }
    // else just return

Cleanup:
    return( hr );
}


//-----------------------------------------------------------------------
//  CSelectAO::SetAccValue()
//
//  DESCRIPTION:
//      sends value string from client to object.
//      This method wraps embedded AO call.
//      
//  PARAMETERS:
//
//      lChild      Child ID
//      szValue     BSTR to submit to object.
//
//  RETURNS:
//
//      HRESULT :   S_OK | E_FAIL 
//
// ----------------------------------------------------------------------

HRESULT 
CSelectAO::SetAccValue(long lChild, BSTR bstrValue)
{
    HRESULT hr=S_OK;

    assert ( bstrValue );

    if(!bstrValue)
        return(E_INVALIDARG);

    bstrValue = NULL;

    //--------------------------------------------------
    // if no std proxied object, try to create.
    // if create fails, we can't set the value
    //--------------------------------------------------
    if(!m_pIAccessible)
    {
        hr = createStdAccessibleObjectIfVisible(&m_pIAccessible);
        if (FAILED(hr))
            goto Cleanup;
    }

    if (m_pIAccessible)
    {
        // delegate to embedded object
        VARIANT varID;

        hr = loadVariant(&varID,CHILDID_SELF);
        if (hr)
            goto Cleanup;

        hr = m_pIAccessible->put_accValue(varID,bstrValue);
    }
    // else just return

Cleanup:
    return( hr );
}


//================================================================================
// CSelectAO class implementation : protected methods
//================================================================================

//--------------------------------------------------------------------------------
//  CSelectAO::loadVariant()
//
//  DESCRIPTION:
//
//  loads variant with specified long value.
//
//  PARAMETERS:
//
//      pVar    pointer to variant.
//      lvar    long to store in variant.
//  
//  RETURNS:
//
//      HRESULT :   S_OK | E_FAIL 
//
//--------------------------------------------------------------------------------

HRESULT CSelectAO::loadVariant(VARIANT * pVar,long lVar)
{
    assert(pVar);

    VariantInit(pVar);

    pVar->vt = VT_I4;
    pVar->lVal = lVar;

    return(S_OK);
}


//--------------------------------------------------------------------------------
//  CSelectAO::unpackVariant()
//
//  DESCRIPTION:
//
//  unpacks contents of variant, verifies that it contains a 
//  pointer to IUnknown, and sets the passed in parameter to that pointer value.
//
//  PARAMETERS:
//
//      pVarToUnpack    pointer to variant to unpack.
//      ppIUnknown      pointer to store IUnknown * in.
//  
//  RETURNS:
//
//      HRESULT :   S_OK | E_FAIL 
//
//--------------------------------------------------------------------------------

HRESULT CSelectAO::unpackVariant(VARIANT varToUnpack,IUnknown ** ppIUnknown)
{

    assert( ppIUnknown );


    //--------------------------------------------------
    // this method only handles IUnknown pointers
    //--------------------------------------------------

    if(varToUnpack.vt != VT_UNKNOWN)
        return(E_FAIL);

    //--------------------------------------------------
    // 'unpacking' means that we are assigning contents 
    // of variant to passed in pointer
    // (this will be returned to calling method)
    //--------------------------------------------------

    *ppIUnknown = varToUnpack.punkVal;

    return(S_OK);
}

//--------------------------------------------------------------------------------
//  CSelectAO::unpackVariant()
//
//  DESCRIPTION:
//
//  unpacks contents of variant, verifies that it contains a 
//  pointer to IDispatch, and sets the passed in parameter to that pointer value.
//
//  PARAMETERS:
//
//      pVarToUnpack    pointer to variant to unpack.
//      ppIDispatch     pointer to store IDispatch * in.
//  
//  RETURNS:
//
//      HRESULT :   S_OK | E_FAIL 
//
//--------------------------------------------------------------------------------

HRESULT CSelectAO::unpackVariant(VARIANT varToUnpack,IDispatch ** ppIDispatch)
{

    assert( ppIDispatch );


    //--------------------------------------------------
    // this method only handles IDispatch pointers
    //--------------------------------------------------

    if(varToUnpack.vt != VT_DISPATCH)
        return(E_FAIL);

    //--------------------------------------------------
    // 'unpacking' means that we are assigning contents 
    // of variant to passed in pointer
    // (this will be returned to calling method)
    //--------------------------------------------------

    *ppIDispatch = varToUnpack.pdispVal;

    return(S_OK);
}

//--------------------------------------------------------------------------------
//  CSelectAO::unpackVariant()
//
//  DESCRIPTION:
//
//  unpacks contents of variant, verifies that it contains a 
//  pointer to IUnknown, and sets the passed in parameter to that pointer value.
//
//  PARAMETERS:
//
//      pVarToUnpack    pointer to variant to unpack.
//      pLong           pointer to store returned long in
//  
//  RETURNS:
//
//      HRESULT :   S_OK | E_FAIL 
//
//--------------------------------------------------------------------------------

HRESULT CSelectAO::unpackVariant(VARIANT varToUnpack,long * plong)
{

    assert( plong );


    //--------------------------------------------------
    // this method only handles longs.
    //--------------------------------------------------

    if(varToUnpack.vt != VT_I4)
        return(E_FAIL);

    //--------------------------------------------------
    // 'unpacking' means that we are assigning contents 
    // of variant to passed in pointer
    // (this will be returned to calling method)
    //--------------------------------------------------

    *plong = varToUnpack.lVal;

    return(S_OK);
}


//-----------------------------------------------------------------------
//  CSelectAO::createStdAccessibleObjectIfVisible()
//
//  DESCRIPTION:
//  creates std accessible object if select list is visible.
//
//  PARAMETERS:
//
//  ppIAccessible   pointer to return IAccessible * in.
//
//  RETURNS:
//
//  S_OK if IAccessible interface created, 
//  S_FALSE if object not visible,
//  else std COM error.
// ----------------------------------------------------------------------
HRESULT 
CSelectAO::createStdAccessibleObjectIfVisible(IAccessible ** ppIAccessible)
{
    HRESULT hr = E_FAIL;
    DWORD   dwCorner  = 0;
    POINT   ptOnWindow;
    HWND    hwndCombo = NULL;
    long    lObjId;


    ptOnWindow.x = ptOnWindow.y = 0;

    assert(ppIAccessible);

    *ppIAccessible = NULL;

    //--------------------------------------------------
    // get a visible point from the window. If the window
    // isn't visible, stop here.
    //--------------------------------------------------

    hr = getVisibleCorner(&ptOnWindow,&dwCorner);
    if (hr)
        goto Cleanup;
    
    //--------------------------------------------------
    // otherwise, create the std accessible object from 
    // the top and left window coordinates.
    //--------------------------------------------------

    ::ScreenToClient( m_hWnd, &ptOnWindow );

    hwndCombo = ChildWindowFromPoint( m_hWnd, ptOnWindow );

    if(hwndCombo == m_hWnd)
    {
        hr = S_FALSE;
        goto Cleanup;
    }


    hr = getObjectId( &lObjId );
    if (hr)
        goto Cleanup;

    // this returns E_FAIL if there is no HWND yet (select is not UIActive
    // we want to mask this as an S_FALSE
    hr = CreateStdAccessibleObject(hwndCombo,lObjId,IID_IAccessible,(void **)ppIAccessible);
    if (hr == E_FAIL)
        hr = S_FALSE;

    assert( hr==S_OK ? (!! *ppIAccessible) : (!*ppIAccessible) );

Cleanup:
    return hr;
}


//-----------------------------------------------------------------------
//  CSelectAO::getObjectId()
//
//  DESCRIPTION:
//
//  Determines whether or not the SELECT object represents a list box
//  or a combo box and returns the appropriate object ID for correct
//  MSAA standard Accessible object creation.  If the SELECT is a list
//  box, return OBJID_WINDOW; otherwise, the SELECT is a combo box so
//  return OBJID_CLIENT.
//
//  A SELECT object is determined to be a list box if its multiple
//  property is TRUE or if its size property is greater than 1.
//
//  PARAMETERS:
//
//  plObjId         [out] long pointer to the object ID
//
//  RETURNS:
//
//  S_OK if object ID set or standard COM error as returned by Trident.
// ----------------------------------------------------------------------
HRESULT 
CSelectAO::getObjectId( /* out */ long* plObjId )
{
    HRESULT         hr;
    VARIANT_BOOL    bMult;
    long            lSize;


    //--------------------------------------------------
    //  Since MSAA system object IDs are 0 and negative
    //  numbers, initialize the out parameter to 1.
    //--------------------------------------------------

    *plObjId = 1;


    CComQIPtr<IHTMLSelectElement,&IID_IHTMLSelectElement> pIHTMLSelectElement(m_pTOMObjIUnk);

    if ( !pIHTMLSelectElement )
        return E_NOINTERFACE;

    hr = pIHTMLSelectElement->get_multiple( &bMult );

    if ( hr == S_OK )
    {
        if ( bMult )
            *plObjId = OBJID_WINDOW;
        else
        {
            hr = pIHTMLSelectElement->get_size( &lSize );

            if ( hr == S_OK )
            {
                if ( lSize > 1 )
                    *plObjId = OBJID_WINDOW;
                else
                    *plObjId = OBJID_CLIENT;
            }
        }
    }


    return hr;
}

//----  End of SELECT.CPP  ----