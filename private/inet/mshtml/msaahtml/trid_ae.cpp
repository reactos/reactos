//================================================================================
//		File:	TRID_AE.CPP
//		Date: 	5/21/97
//		Desc:	contains implementation of CTridentAE class.  CTridentAE 
//				is the base class for all accessible Trident elements and 
//				objects.
//
//		Author:	Arunj
//
//================================================================================

//================================================================================
// includes
//================================================================================

#include "stdafx.h"
#include "trid_ae.h"
#include "trid_ao.h"
#include "document.h"

#define RELEASE_INTERFACE(ptr) if (ptr) ptr->Release();

//================================================================================
//	CTridentAE class implementation : public methods.
//================================================================================

//-----------------------------------------------------------------------
//	CTridentAE::CTridentAE()
//
//	DESCRIPTION:
//
//		CTridentAE class constructor.
//
//	PARAMETERS:
//
//		pAOParent			pointer to the parent accessible object in 
//							the AO tree
//
//		nTOMIndex			index of the element from the TOM document.all 
//							collection.
//		
//		nChildID			Child ID -- this ID is guaranteed to be unique 
//							in the AOM tree.
//
//		hWnd				pointer to the window of the trident object that 
//							this object corresponds to.
//
//
//	RETURNS:
//
//		None.
//
// ----------------------------------------------------------------------

CTridentAE::CTridentAE(CTridentAO * pAOParent,UINT nTOMIndex,UINT nChildID,HWND hWnd) :
CAccElement(nChildID,hWnd)
{

	//--------------------------------------------------
	// all elements MUST have parents
	//--------------------------------------------------

	assert( pAOParent );
	
	m_pParent	= pAOParent;

	//------------------------------------------------
	// assign the delegating IUnknown to CTridentAE :
	// this member will be overridden in derived class
	// constructors so that the delegating IUnknown 
	// will always be at the derived class level.
	//------------------------------------------------
	
	m_pIUnknown		= (IUnknown *)this;
	m_pTOMObjIUnk	= NULL;
	m_nTOMIndex		= nTOMIndex;	// all collection index



	//--------------------------------------------------
	// init all property data values.
	//--------------------------------------------------

	m_bstrName				= NULL;
	m_bstrValue				= NULL;          
	m_bstrDescription		= NULL;    
	m_bstrDefaultAction		= NULL;  
	m_bstrKbdShortcut		= NULL;    


	//--------------------------------------------------
	// set OFFSCREEN flag to false (it will be set to
	// TRUE if the element is actually offscreen)
	//--------------------------------------------------

	m_bOffScreen = FALSE;


}

//-----------------------------------------------------------------------
//	CTridentAE::~CTridentAE()
//
//	DESCRIPTION:
//
//		CTridentAE class destructor.
//
//	PARAMETERS:
//
//		none.
//
//	RETURNS:
//
//		None.
//
// ----------------------------------------------------------------------

CTridentAE::~CTridentAE()
{

	//--------------------------------------------------
	// free all allocated BSTRs.
	//--------------------------------------------------

	if(m_bstrName)
		SysFreeString(m_bstrName);

	if(m_bstrValue)
		SysFreeString(m_bstrValue);          
	
	if(m_bstrDescription)
		SysFreeString(m_bstrDescription);    

	if(m_bstrDefaultAction)
		SysFreeString(m_bstrDefaultAction);  
	
	if(m_bstrKbdShortcut)
		SysFreeString(m_bstrKbdShortcut);    


	//--------------------------------------------------
	//  Release the IUnknown 
	//--------------------------------------------------

	if (m_pTOMObjIUnk)
	{
		m_pTOMObjIUnk->Release();
		m_pTOMObjIUnk = NULL;
	}
}


//-----------------------------------------------------------------------
//	CTridentAE::Init()
//
//	DESCRIPTION:
//
//		Initialization : set values of data members
//
//		**NOTE** this is one place where we use a standard COM interface
//		pointer instead of an ATL CComQIPtr.  This is because this pointer 
//		needs to exist for the lifetime of the app -- its existence must
//		ABSOLUTELY be guaranteed for the duration of the lifetime of the object.
//		It is a lot more obvious to explicitly release the interface at object 
//		destruction, instead of having it implicitly released.
//
//	PARAMETERS:
//
//		pTOMObjIUnk		pointer to IUnknown of TOM object.
//
//	RETURNS:
//
//		S_OK | E_FAIL | E_NOINTEFACE
//
//
// ----------------------------------------------------------------------

HRESULT CTridentAE::Init(IUnknown * pTOMObjIUnk)
{
	HRESULT hr = E_FAIL;

	assert( pTOMObjIUnk );
	
	//--------------------------------------------------
	// QI on passed in pointer for base Trident object
	// interface from which all other interfaces will
	// be QI'd.
	//
	// **NOTE** derived objects that are not TOM element
	// objects need to override Init and QI for the
	// interface that they need.
	//--------------------------------------------------

	hr = pTOMObjIUnk->QueryInterface( IID_IUnknown, (void**)&m_pTOMObjIUnk );

	return(hr);
}



//=======================================================================
// helper methods (override these to provide custom	functionality for
// CTridentAE - derived methods )
//=======================================================================

//-----------------------------------------------------------------------
//	CTridentAE::GetAccParent()
//
//	DESCRIPTION:
//
//	gets the parent of this object and returns its IDispatch pointer
//								 
//	PARAMETERS:
//
//	ppdispParent : pointer to parent interface to return to caller.
//
//	RETURNS:
//
//		HRESULT :	S_OK | E_FAIL | DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT	CTridentAE::GetAccParent(IDispatch ** ppdispParent)
{
	HRESULT hr	= E_FAIL;

	
	assert( ppdispParent );

	//------------------------------------------------
	// check for parent : **NOTE** it is still 
	// valid not to have a parent.
	//------------------------------------------------

	if(!m_pParent)
	{
		*ppdispParent = NULL;
		return(E_FAIL);
	}

	hr = m_pParent->QueryInterface(IID_IAccessible,(void **)ppdispParent);

	return(hr);
}


//-----------------------------------------------------------------------
//	CTridentAE::GetAccName()
//
//	DESCRIPTION:
//
//		returns name of object/element
//
//	PARAMETERS:
//
//		lChild			child ID / Self ID
//		pbstrName			returned name.
//		
//	RETURNS:
//
//		HRESULT :	DISP_E_MEMBERNOTFOUND
//
// ----------------------------------------------------------------------

HRESULT	CTridentAE::GetAccName(long lChild, BSTR * pbstrName)
{
	return(DISP_E_MEMBERNOTFOUND);
}


//-----------------------------------------------------------------------
//	CTridentAE::GetAccValue()
//
//	DESCRIPTION:
//
//		returns value string to client.
//
//	PARAMETERS:
//
//		lChild		ChildID/SelfID
//
//		pbstrValue	returned value string
//
//	RETURNS:
//
//		HRESULT :	DISP_E_MEMBERNOTFOUND
//
// ----------------------------------------------------------------------

HRESULT	CTridentAE::GetAccValue(long lChild, BSTR * pbstrValue)
{
	return(DISP_E_MEMBERNOTFOUND);
}

//-----------------------------------------------------------------------
//	CTridentAE::GetAccDescription()
//
//	DESCRIPTION:
//
//		returns description string to client.
//
//	PARAMETERS:
//
//		lChild				Child/Self ID
//
//		pbstrDescription		Description string returned to client.
//	
//	RETURNS:
//
//		HRESULT :	DISP_E_MEMBERNOTFOUND
//
// ----------------------------------------------------------------------

HRESULT	CTridentAE::GetAccDescription( long lChild, BSTR* pbstrDescription )
{
	return(DISP_E_MEMBERNOTFOUND);
}


//-----------------------------------------------------------------------
//	CTridentAE::GetAccState()
//
//	DESCRIPTION:
//
//		returns object state to client
//
//	PARAMETERS:
//
//		lChild		Child/Self ID 
//
//		lState		returned state constant
//
//	RETURNS:
//
//		HRESULT :	S_OK | DISP_E_MEMBERNOTFOUND
//
//	NOTES:
//		This base class implementation determines whether or
//		not the AE's corresponding TEO is visible and, if not,
//		ors STATE_SYSTEM_INVISIBLE to the state of the AE.
//
//-----------------------------------------------------------------------

HRESULT	CTridentAE::GetAccState( long lChild, long* plState )
{
	HRESULT hr = E_FAIL;


	//--------------------------------------------------
	// Determine if the AE is visible
	//--------------------------------------------------

	CComQIPtr<IHTMLElement,&IID_IHTMLElement> pIHTMLElement(m_pTOMObjIUnk);
	if ( !pIHTMLElement )
		return E_NOINTERFACE;


	CComPtr<IHTMLStyle> pIHTMLStyle;

	hr = pIHTMLElement->get_style( &pIHTMLStyle );
	if ( hr != S_OK )
		return hr;
	if ( !pIHTMLStyle )
		return E_NOINTERFACE;


	BSTR bstrStyle;

	//--------------------------------------------------
	// Check the style.visibility property
	//--------------------------------------------------

	hr = pIHTMLStyle->get_visibility( &bstrStyle );
	if ( hr != S_OK )
		return hr;

	if ( bstrStyle )
	{
		if ( !_wcsicmp( bstrStyle, L"VISIBLE" ) )
		{
			SysFreeString( bstrStyle );

			*plState |= STATE_SYSTEM_INVISIBLE;
			return S_OK;
		}
	}

	//--------------------------------------------------
	//	Check the style.display property.
	//
	//	The display property has two values: "" and
	//	"none".  If display equals "", bstrStyle will
	//	be NULL after get_display().  So, if bstrStyle
	//	is non-NULL, it's value is "none" and we know
	//	that the element object is not visible.
	//--------------------------------------------------

	hr = pIHTMLStyle->get_display( &bstrStyle );
	if ( hr != S_OK )
		return hr;

	if ( bstrStyle )
	{
		SysFreeString( bstrStyle );

		*plState |= STATE_SYSTEM_INVISIBLE;
		return S_OK;
	}


	return DISP_E_MEMBERNOTFOUND;
}


//-----------------------------------------------------------------------
//	CTridentAE::GetAccHelp()
//
//	DESCRIPTION:
//
//		Get Help string
//
//	PARAMETERS:
//
//		lChild		child/self ID
//		pbstrHelp		help string
//
//	RETURNS:
//
//		HRESULT :	S_OK | E_FAIL | DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT	CTridentAE::GetAccHelp(long lChild, BSTR * pbstrHelp)
{
	return(DISP_E_MEMBERNOTFOUND);	
}


//-----------------------------------------------------------------------
//	CTridentAE::GetAccHelpTopic()
//
//	DESCRIPTION:
//
//		Get Help file and topic in file
//
//	PARAMETERS:
//
//		pbstrHelpFile		help file
//
//		lChild			child/self ID
//
//		pidTopic		topic index
//
//	RETURNS:
//
//		HRESULT :	DISP_E_MEMBERNOTFOUND
//
// ----------------------------------------------------------------------

HRESULT	CTridentAE::GetAccHelpTopic(BSTR * pbstrHelpFile, long lChild,long * pidTopic)
{
	return(DISP_E_MEMBERNOTFOUND);
}


//-----------------------------------------------------------------------
//	CTridentAE::GetAccKeyboardShortcut()
//
//	DESCRIPTION:
//
//		Get shortcut string
//
//	PARAMETERS:
//
//		lChild					child/self ID 
//
//		pbstrKeyboardShortcut		returned string containing kbd shortcut.
//
//	RETURNS:
//
//		HRESULT :	S_OK | DISP_E_MEMBERNOTFOUND
//
// ----------------------------------------------------------------------

HRESULT	CTridentAE::GetAccKeyboardShortcut( long lChild, BSTR* pbstrKeyboardShortcut )
{
	return(DISP_E_MEMBERNOTFOUND);
}


//-----------------------------------------------------------------------
//	CTridentAE::GetAccDefaultAction()
//
//	DESCRIPTION:
//		returns description string for default action
//	
//	PARAMETERS:
//
//		lChild			child /self ID
//
//		pbstrDefAction	returned description string.
//
//	RETURNS:
//
//		HRESULT :	DISP_E_MEMBERNOTFOUND
//
// ----------------------------------------------------------------------

HRESULT	CTridentAE::GetAccDefaultAction(long lChild, BSTR * pbstrDefAction)
{
	return(DISP_E_MEMBERNOTFOUND);
}


//-----------------------------------------------------------------------
//	CTridentAE::AccSelect()
//
//	DESCRIPTION:
//		selects specified object: selection based on flags
//	
//	PARAMETERS:
//
//		flagsSel	selection flags : 
//
//			SELFLAG_NONE            = 0,
//			SELFLAG_TAKEFOCUS       = 1,
//			SELFLAG_TAKESELECTION   = 2,
//			SELFLAG_EXTENDSELECTION = 4,
//			SELFLAG_ADDSELECTION    = 8,
//			SELFLAG_REMOVESELECTION = 16
//
//		lChild		child /self ID 
//
//
//	RETURNS:
//
//		S_OK | E_FAIL | E_INVALIDARG | E_NOINTERFACE
//
// ----------------------------------------------------------------------

HRESULT	CTridentAE::AccSelect(long flagsSel, long lChild)
{
	return(DISP_E_MEMBERNOTFOUND);

}


//-----------------------------------------------------------------------
//	CTridentAE::AccLocation()
//
//	DESCRIPTION:
//		returns location of specified object
//	 
//	PARAMETERS:
//
//		pxLeft		left screen coordinate
//		pyTop		top screen coordinate
//		pcxWidth	screen width of object
//		pcyHeight	screen height of object
//		lChild		child/self ID
//
//
//	RETURNS:
//
//		HRESULT :	S_OK | E_FAIL | DISP_E_MEMBERNOTFOUND | E_NOINTERFACE
//					or other COM error
//
// ----------------------------------------------------------------------

HRESULT	CTridentAE::AccLocation(long * pxLeft, long * pyTop, long * pcxWidth,long * pcyHeight, long lChild)
{
	HRESULT hr;

	long xLeft		=0;
	long yTop		=0;
	long cxWidth	=0;
	long cyHeight	=0;

	assert( pxLeft );
	assert( pyTop );
	assert( pcxWidth );
	assert( pcyHeight );

	//--------------------------------------------------
	// initialize the out parameters
	//--------------------------------------------------

	*pxLeft		= 0;
	*pyTop		= 0;
	*pcxWidth	= 0;
	*pcyHeight	= 0;

	//--------------------------------------------------
	// get screen locations
	//--------------------------------------------------

	CComQIPtr<IHTMLElement,&IID_IHTMLElement> pIHTMLElement(m_pTOMObjIUnk);

	if(!(pIHTMLElement))
		return(E_NOINTERFACE);

	hr = pIHTMLElement->get_offsetLeft(&xLeft);
	if(hr != S_OK)
		return(hr);

	hr = pIHTMLElement->get_offsetTop(&yTop);
	if(hr != S_OK)
		return(hr);

	hr = pIHTMLElement->get_offsetWidth(&cxWidth);
	if(hr != S_OK)
		return(hr);

	hr = pIHTMLElement->get_offsetHeight(&cyHeight);
	if(hr != S_OK)
		return(hr);

	//--------------------------------------------------
	// if the width or the height are zero, the object
	// is not visible.  otherwise, assume the object is
    // visible and wait to be proven wrong.
	//--------------------------------------------------

	if ( cxWidth == 0 || cyHeight == 0 )
	{
		m_bOffScreen = TRUE;
		return S_OK;
	}

    m_bOffScreen = FALSE;

	//--------------------------------------------------
	// get offset from top left corner of HTML document.
	//--------------------------------------------------

	hr = adjustOffsetToRootParent(&xLeft,&yTop);

	if(hr != S_OK)
		return(hr);
	
	//--------------------------------------------------
	// adjust point to account for client area
	//--------------------------------------------------
	
	hr = adjustOffsetForClientArea(&xLeft,&yTop);

	//--------------------------------------------------
	// if the adjusted x and y values are not on the 
	// screen, toggle on screen state boolean.
	//--------------------------------------------------

	if(hr != S_OK)
	{
		if(hr == S_FALSE)
			m_bOffScreen = TRUE;
		else
			return(hr);
	}


    //--------------------------------------------------
    // Set out parameters with calculated location.
    //--------------------------------------------------

	*pxLeft		= xLeft;
	*pyTop		= yTop;
	*pcxWidth	= cxWidth;
	*pcyHeight	= cyHeight;


	return(S_OK);

}

//-----------------------------------------------------------------------
//	CTridentAE::AccDoDefaultAction()
//
//	DESCRIPTION:
//		executes default action of object.
//	
//	PARAMETERS:
//
//		lChild		child / self ID
//
//	RETURNS:
//
//		HRESULT :	DISP_E_MEMBERNOTFOUND
//
// ----------------------------------------------------------------------

HRESULT	CTridentAE::AccDoDefaultAction(long lChild)
{
	return(DISP_E_MEMBERNOTFOUND);
}


//-----------------------------------------------------------------------
//	CTridentAE::SetAccName()
//
//	DESCRIPTION:
//		sets name string of object
//	
//	PARAMETERS:
//
//		lChild			child / self ID 
//		bstrName		name string
//
//	RETURNS:
//
//		HRESULT :	S_FALSE
//
// ----------------------------------------------------------------------

HRESULT	CTridentAE::SetAccName(long lChild, BSTR  bstrName)
{
	//--------------------------------------------------
	// as per MSAA spec, if object/element doesn't
	// support setting name, return S_FALSE
	//--------------------------------------------------

	return(S_FALSE);
}


//-----------------------------------------------------------------------
//	CTridentAE::SetAccValue()
//
//	DESCRIPTION:
//		sets name string of object
//	
//	PARAMETERS:
//
//		lChild		child / self ID 
//		bstrValue		name string
//
//	RETURNS:
//
//		HRESULT :	S_FALSE
//
// ----------------------------------------------------------------------

HRESULT	CTridentAE::SetAccValue(long lChild, BSTR bstrValue)
{
	//--------------------------------------------------
	// as per MSAA spec, if object/element doesn't
	// support setting value, return S_FALSE
	//--------------------------------------------------

	return(S_FALSE);
}


//================================================================================
// object state/selection methods
//================================================================================



//--------------------------------------------------------------------------------
// CTridentAE::GetFocusedTOMElementIndex()
//
//	DESCRIPTION:
//		returns TOM index of focused element.
//	
//	PARAMETERS:
//
//		plTOMIndexID		pointer to UINT to store focused element ID in.
//						Element ID = TOM index of focused element.
//
//	RETURNS:
//
//	HRESULT	S_OK | E_FAIL
//	
//	NOTES:
//		
//		the caller of this method needs to validate the out parameter even
//		if the return value is good.
//--------------------------------------------------------------------------------
	
HRESULT CTridentAE::GetFocusedTOMElementIndex(UINT * puTOMIndexID)
{
	
	assert(puTOMIndexID);

	if(m_pParent)
	{
		//--------------------------------------------------
		// find out who has the focus
		// in the TOM all collection, then return that ID
		// to this object.
		//--------------------------------------------------

		return(m_pParent->GetFocusedTOMElementIndex(puTOMIndexID));

	}
	else
	{		
		
		return(E_FAIL);
	}

}


//================================================================================
//	CTridentAE class implementation : protected methods.
//================================================================================

//-----------------------------------------------------------------------
//	CTridentAE::getTitleFromIHTMLElement()
//
//	DESCRIPTION:
//		  this method gets the title, but the contents of that title
//		  need to be validated by the caller.
//	
//	PARAMETERS:
//
//		pbstrTitle.
//
//	RETURNS:
//
//		S_OK if title call succeeds, else standard COM error
//
//	NOTES:
//		
//		the caller of this method needs to validate the out parameter even
//		if the return value is good.
// ----------------------------------------------------------------------

HRESULT CTridentAE::getTitleFromIHTMLElement(BSTR *pbstrTitle)
{
	HRESULT hr	= E_FAIL;

	assert( pbstrTitle );

	CComQIPtr<IHTMLElement,&IID_IHTMLElement> pIHTMLElement(m_pTOMObjIUnk);

	if ( !pIHTMLElement )
	{	
		//--------------------------------------------------
		// this should NEVER happen - a TOM element should
		// always implement this interface.
		//--------------------------------------------------

		assert(pIHTMLElement);
		hr = E_NOINTERFACE;
	}
	else
		hr = pIHTMLElement->get_title( pbstrTitle );


	return hr;
}


//-----------------------------------------------------------------------
//	CTridentAE::clickAE()
//
//	DESCRIPTION:
//		this method calls the 'click' method of the IHTMLElement.
//		the 'click' action is the default action for many AEs
//	
//	PARAMETERS:
//
//		none..
//
//	RETURNS:
//
//		S_OK if title call succeeds, else standard COM error
//
// ----------------------------------------------------------------------

HRESULT CTridentAE::clickAE(void)
{
	CComQIPtr<IHTMLElement,&IID_IHTMLElement> pIHTMLElement(m_pTOMObjIUnk);

	if ( !pIHTMLElement )
		return E_NOINTERFACE;

	return pIHTMLElement->click();
}


//--------------------------------------------------------------------------------
//	CTridentAE::adjustOffsetToRootParent()
//
//	DESCRIPTION:
//		adjust input points to root parent window
//	
//	PARAMETERS:
//
//		pxLeft	- pointer to x coord
//		pyTop	- pointer to y coord
//
//	RETURNS:
//
//		S_OK | E_FAIL | E_NOINTERFACE
//
//--------------------------------------------------------------------------------

HRESULT CTridentAE::adjustOffsetToRootParent(long * pxLeft,long * pyTop)
{
	HRESULT hr;

	assert( pxLeft );
	assert( pyTop );

	long xLeft	= *pxLeft;
	long yTop	= *pyTop;


	//--------------------------------------------------
	// now adjust by parent coordinates : move up parent
	// list to the root and add left, top parent coords.
	//-------------------------------------------------
					   
    IHTMLElement        * pIHTMLElement = NULL;
    IHTMLElement        * pIHTMLParentElement = NULL;
    IHTMLControlElement * pControlElem = NULL;
    long                  lTempLeft = 0;
    long                  lTempTop  = 0;
    long                  lClientLeft = 0;
    long                  lClientTop  = 0;

	hr = m_pTOMObjIUnk->QueryInterface( IID_IHTMLElement, (void**)&pIHTMLElement );

	if ( hr != S_OK )
		goto CleanUp;
	if ( !pIHTMLElement )
	{
		hr = E_NOINTERFACE;
		goto CleanUp;
	}

	
	while ( (hr = pIHTMLElement->get_offsetParent( &pIHTMLParentElement )) == S_OK )
	{
		//--------------------------------------------------
		// method can return S_OK, with NULL parent, so 
		// check for valid parent
		//--------------------------------------------------

		if ( pIHTMLParentElement == NULL )
			break;

		//--------------------------------------------------
		// get the offsetLeft and offsetTop metrics of
		// the parent
		//--------------------------------------------------

		hr = pIHTMLParentElement->get_offsetLeft( &lTempLeft );
		if ( hr != S_OK )
			goto CleanUp;
		
		hr = pIHTMLParentElement->get_offsetTop( &lTempTop );
		if ( hr != S_OK )
			goto CleanUp;


		//--------------------------------------------------
		// Get the clientTop and clientWidth ... to adjust 
		//   for border thickness
		//--------------------------------------------------

		hr = pIHTMLParentElement->QueryInterface(IID_IHTMLControlElement,
			                                     (VOID**) &pControlElem);
		if (!pControlElem)
			hr = E_NOINTERFACE;

		if (hr != S_OK)
			goto CleanUp;

		hr = pControlElem->get_clientTop( &lClientTop );
		if (hr != S_OK)
			goto CleanUp;

		hr = pControlElem->get_clientLeft( &lClientLeft);
		if (hr != S_OK)
			goto CleanUp;

		pControlElem->Release();
        pControlElem=NULL;

		xLeft += lTempLeft + lClientLeft;
		yTop += lTempTop + lClientTop;

		//--------------------------------------------------
		// make the parent the child
		//--------------------------------------------------

		pIHTMLElement->Release();
		pIHTMLElement = pIHTMLParentElement;
		pIHTMLParentElement = NULL;
	}

	*pxLeft = xLeft;
	*pyTop	= yTop;


CleanUp:
    RELEASE_INTERFACE( pIHTMLElement );
    RELEASE_INTERFACE( pIHTMLParentElement );
    RELEASE_INTERFACE( pControlElem );

    return hr;
}

//--------------------------------------------------------------------------------
//  CTridentAE::adjustOffsetForClientArea()
//
//	DESCRIPTION:
//		modify input point to account for client area. If it is above/below/right/left
//		of current client area, then return S_FALSE, 
//	
//	PARAMETERS:
//
//		pxLeft		- pointer to x coord
//		pyTop		- pointer to y coord
//		
//
//	RETURNS:
//
//		S_OK | E_FAIL | E_NOINTERFACE
//		S_FALSE if point is not in current client area

//--------------------------------------------------------------------------------

HRESULT CTridentAE::adjustOffsetForClientArea( long *pxLeft, long *pyTop )
{
    HRESULT hr;
    long xLeft	= *pxLeft;
    long yTop	= *pyTop;

    CDocumentAO * pDocAO = m_pParent->GetDocumentAO();

    assert( pDocAO );

    //--------------------------------------------------
    // get the scroll offsets
    //--------------------------------------------------
	
    POINT ptScrollOffset;

    hr = pDocAO->GetScrollOffset( &ptScrollOffset );

    if ( hr != S_OK )
        return hr;

	//--------------------------------------------------
	// subtracting the scroll offsets from the total
	// offset will give the position of the element 
	// in the client area of the owner document
	//--------------------------------------------------

    xLeft   -= ptScrollOffset.x;
    yTop    -= ptScrollOffset.y;


    //--------------------------------------------------
    // determine if xLeft or yTop is to the right 
    // | below the current client area.
    // if unadjusted value > scroll offset + 
    // client (width | height) then the point is right 
    // | below the current client area
    //--------------------------------------------------

    long lDocLeft    =0;
    long lDocTop     =0;
    long lDocWidth   =0;
    long lDocHeight  =0;
    long lcxWidth    =0;
    long lcyHeight   =0;

    //--------------------------------------------------
    // the document accLocation defers to the body to return 
    //   its offset properties. This is outside of the bodies
    //   borders and so we can safely add the top,left to our
    //   point, without duplicating the border thickness
    //
    //--------------------------------------------------

    hr = pDocAO->AccLocation(&lDocLeft,
                             &lDocTop,
                             &lDocWidth,
                             &lDocHeight,
                             CHILDID_SELF);

    if ( hr != S_OK )
        return hr;

    xLeft += lDocLeft;
    yTop  += lDocTop; 

    //--------------------------------------------------
    // compare the 'fully adjusted' xLeft and yTop with
    // the 'fully adjusted' client area.  If any
    // of these expressions evaluate to TRUE, that 
    // means that the point is offscreen.
    //
    // unfortunately this requires that we get the width 
    // and height
	//--------------------------------------------------

    CComQIPtr<IHTMLElement,&IID_IHTMLElement> pIHTMLElement(m_pTOMObjIUnk);

    if(pIHTMLElement)
    {
        hr = pIHTMLElement->get_offsetWidth(&lcxWidth);
        if(hr != S_OK)
            return(hr);

        hr = pIHTMLElement->get_offsetHeight(&lcyHeight);
        if(hr != S_OK)
            return(hr);
    }

	if( (xLeft > lDocLeft + lDocWidth)  ||    // off to the right
		(yTop  > lDocTop  + lDocHeight) ||    // off to the bottom
        (xLeft + lcxWidth  < lDocLeft)  ||    // off to the left
        (yTop  + lcyHeight < lDocTop)    )    // off to the top
    {
        hr = S_FALSE;
    }


    *pxLeft	= xLeft;
    *pyTop	= yTop;

    return hr;
}



//----  End of TRID_AE.CPP  ----
