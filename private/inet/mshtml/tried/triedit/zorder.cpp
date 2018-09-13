//------------------------------------------------------------------------------
// zorder.cpp
// Copyright (c)1997-1999 Microsoft Corporation, All Rights Reserved
//
// Author
//     V-BMohan
//
// History
//      8-15-97     created     (ThomasOl)
//     10-31-97     rewritten   (V-BMohan)
//
//
//------------------------------------------------------------------------------

#include "stdafx.h"

#include <stdlib.h>

//#include "mfcincl.h"
#include "triedit.h"
#include "document.h"
#include "zorder.h"
#include "dispatch.h"

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::CompareProc
//
// Compare the Z-order of the two items (which must be CZOrder pointers)
// and return:
//
//      -1 if the Z-order of item 1 preceeds that of item 2
//       1 if the Z-order of item 1 succeeds or is the same as that of item 2
//

int CTriEditDocument::CompareProc(const void *arg1, const void *arg2)
{
    CZOrder* pcz1 = (CZOrder*)arg1;
    CZOrder* pcz2 = (CZOrder*)arg2;

    _ASSERTE(pcz1 != NULL);
    _ASSERTE(pcz2 != NULL);
    if (pcz1->m_zOrder < pcz2->m_zOrder)
        return -1;
    else
    if (pcz1->m_zOrder > pcz2->m_zOrder)
        return 1;

    // if arg1's Z-order is qual to arg2's zorder then return a one 
    // instead of a zero  so that the qsort function treats it as 
    // arg1's Z-Order > arg2's Z-order and keeps arg1 in top of the
    // sort order. 
    // 
    // This actually helps us to sort elements in such a  way that among
    // the elements having the same Z-order the recently created one will be
    // in top of the order. This way we make sure that when propagating
    // Z-order it doesn't affect the existing Z-order appearance of the
    // elements.
    return 1; 
}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::IsEqualZIndex
//
// Given a sorted array of CZorder objects and the number of elements
// in the array, return TRUE if any two consecutive objects have the
// same Z-order. Return FALSE if this is not the case.
//

BOOL CTriEditDocument::IsEqualZIndex(CZOrder* pczOrder, LONG lIndex)
{
    for (LONG lLoop = 0; lLoop < (lIndex - 1); ++lLoop)
    {
        if (pczOrder[lLoop].m_zOrder == pczOrder[lLoop+1].m_zOrder)
            return TRUE;
    }
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::GetZIndex
//
// Fetch the Z-order value from the given HTML element and return
// it under *plZindex. Return S_OK or a Trident error.
//

HRESULT CTriEditDocument::GetZIndex(IHTMLElement* pihtmlElement, LONG* plZindex)
{
    HRESULT hr;
    IHTMLStyle* pihtmlStyle=NULL;
    VARIANT var;

    _ASSERTE(pihtmlElement);
    _ASSERTE(plZindex);
    hr = pihtmlElement->get_style(&pihtmlStyle);
    _ASSERTE(SUCCEEDED(hr));
    _ASSERTE(pihtmlStyle);

    if (SUCCEEDED(hr) && pihtmlStyle)
    {
        VariantInit(&var);
        hr = pihtmlStyle->get_zIndex(&var);
        hr = VariantChangeType(&var, &var, 0, VT_I4);

        if (SUCCEEDED(hr))
        {
            *plZindex = var.lVal;
        }
    }

    SAFERELEASE(pihtmlStyle);
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::SetZIndex
//
// Set the Z-order of the given HTML element as indicated. Return S_OK
// or a Trident error.
//

HRESULT CTriEditDocument::SetZIndex(IHTMLElement* pihtmlElement, LONG lZindex)
{
    HRESULT hr;
    IHTMLStyle* pihtmlStyle=NULL;
    VARIANT var;

    _ASSERTE(pihtmlElement);
    
    hr = pihtmlElement->get_style(&pihtmlStyle);
    _ASSERTE(SUCCEEDED(hr));
    _ASSERTE(pihtmlStyle);

    if (SUCCEEDED(hr) && pihtmlStyle)
    {
        VariantInit(&var);
        var.vt = VT_I4;
        var.lVal = lZindex;
        hr = pihtmlStyle->put_zIndex(var);
        _ASSERTE(SUCCEEDED(hr));
    }

    SAFERELEASE(pihtmlStyle);
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::AssignZIndex
//
// Set the Z-order of the given HTML element according to the index mode:
//
//      SEND_BACKWARD
//      SEND_FORWARD
//      SEND_TO_BACK
//      SEND_TO_FRONT
//      SEND_BEHIND_1D
//      SEND_FRONT_1D
//      MADE_ABSOLUTE
//
// The Z-order of the element's sibling will be adjusted as necessary
// in order to keep them unique. Returns S_OK or a Trident error.
//

HRESULT CTriEditDocument::AssignZIndex(IHTMLElement* pihtmlElement, int nZIndexMode)
{
    HRESULT hr = E_FAIL;
    IHTMLElementCollection* pihtmlCollection = NULL;
    IHTMLElement* pihtmlElementTemp = NULL;
    IHTMLElement* pihtmlElementParent = NULL;
    LONG iIndex, lLoop, lZindex;
    LONG lSourceIndexTemp, lSourceIndexElement, lSourceIndexParent;
    LONG cElements = 0;
    BOOL f2d = FALSE;
    BOOL f2dCapable = FALSE;
    BOOL fZeroIndex = FALSE;
    BOOL fSorted = FALSE;
    BOOL fZIndexNegative = FALSE; // FALSE means we need to deal with
			    	  // elements having +ve Z-INDEX and
                                  // vice versa.
    CZOrder* pczOrder=NULL;
    
    _ASSERTE(pihtmlElement);

    if ( !pihtmlElement)
    {
        return E_FAIL;
    }

    hr = pihtmlElement->get_offsetParent(&pihtmlElementParent);

    if (FAILED(hr) || !pihtmlElementParent)
    {
        return E_FAIL;
    }

    // we get the source index of the passed element's parent to
    // be used in the following for loop to identify the elements
    // belonging to this parent.
    
    hr = pihtmlElementParent->get_sourceIndex(&lSourceIndexParent);
    SAFERELEASE(pihtmlElementParent);
    _ASSERTE(SUCCEEDED(hr) && (lSourceIndexParent != -1));
    if (FAILED(hr) || (lSourceIndexParent == -1))
    {
        return E_FAIL;
    }

    // we get the source index of the element to be used in the
    // following for loop to identify the current element in
    // the collection.

    hr = pihtmlElement->get_sourceIndex(&lSourceIndexElement);

    _ASSERTE(SUCCEEDED(hr) && (lSourceIndexElement != -1));
    if (FAILED(hr) || (lSourceIndexElement == -1))
    {
        return E_FAIL;
    }

    hr = GetZIndex(pihtmlElement, &lZindex);
    _ASSERTE(SUCCEEDED(hr));

    if (FAILED(hr))
    {
        return E_FAIL;
    }

    if (lZindex < 0) 
    {
        if (nZIndexMode == SEND_BEHIND_1D)    // If Z-order is negative then
                                              // its already behind 1D.
        {                                     // hence return. 
            return S_OK;
        }
        else if(nZIndexMode != SEND_FRONT_1D) 
        {
            fZIndexNegative = TRUE; // If the passed element has negative
                                    // Z-order and if mode is anything
        }                           // other than send front then we
                                    // need to deal only with negative
                                    // elements.
    }
    else
    {
        if (nZIndexMode == SEND_FRONT_1D)     // If Z-order is positive then
                                              // its already in front of 1D
        {                                     // hence return.
            if (lZindex > 0)
                return S_OK;
        }
        else if(nZIndexMode == SEND_BEHIND_1D)
        {
            fZIndexNegative = TRUE; // If the passed element has positive
                                    // Z-order and if mode is send behind
        }                           // then we need to deal only with
                                    // negative elements.
    }

    hr = GetAllCollection(&pihtmlCollection);
    _ASSERTE(SUCCEEDED(hr));
    _ASSERTE(pihtmlCollection);

    if (FAILED(hr) || !pihtmlCollection)    // If we dont have a collection
                                            // then exit
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = pihtmlCollection->get_length(&cElements);  // Get number of elements
                                                    // in the collection
    _ASSERTE(SUCCEEDED(hr));
    _ASSERTE(cElements > 0);

    if ( FAILED(hr) || cElements <= 0 )     
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    pczOrder = new CZOrder[cElements];      // Allocate an array of CZOrder
                                            // big enough for all

    if (!pczOrder)                          
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // Now we collect all elements which are children of the parent of
    // the element passed to this function, including the passed element
    // itself.

    for (lLoop=0, iIndex=0; lLoop < cElements; lLoop++)
    {
        hr = GetCollectionElement(pihtmlCollection, lLoop, &pihtmlElementTemp); 
        _ASSERTE(SUCCEEDED(hr));
        _ASSERTE(pihtmlElementTemp);

        if (FAILED(hr) || !pihtmlElementTemp)
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        hr = Is2DCapable(pihtmlElementTemp, &f2dCapable);

        if (FAILED(hr))
            goto Cleanup;

        if (f2dCapable) 
        {
            hr = Is2DElement(pihtmlElementTemp, &f2d);

            if (FAILED(hr))
                goto Cleanup;

            if (f2d) // If the element is a 2D element
            {   
                hr = pihtmlElementTemp->get_offsetParent(&pihtmlElementParent);
                _ASSERTE(SUCCEEDED(hr));
                _ASSERTE(pihtmlElementParent);

                if (FAILED(hr) || !pihtmlElementParent)
                    goto Cleanup;

                hr = pihtmlElementParent->get_sourceIndex(&lSourceIndexTemp);
                SAFERELEASE(pihtmlElementParent);
                _ASSERTE(SUCCEEDED(hr) && lSourceIndexElement != -1);

                if (FAILED(hr) || (lSourceIndexTemp == -1))
                {
                    hr = E_FAIL;
                    goto Cleanup;
                }

                // Is it a child of the same parent as that of the
                // parent of the element passed to this function?

                if (lSourceIndexTemp == lSourceIndexParent) 
                {
                     hr = GetZIndex(pihtmlElementTemp, &lZindex);
                    _ASSERTE(SUCCEEDED(hr));

                    if (FAILED(hr))
                        goto Cleanup;

                    if (lZindex == 0)
                    {
                        hr = pihtmlElementTemp->get_sourceIndex(&lSourceIndexTemp);
    
                        if (FAILED(hr) || (lSourceIndexTemp == -1))
                        {
                            hr = E_FAIL;
                            goto Cleanup;
                        }

                        // General scenario is that we set fZeroIndex to
                        // TRUE when we encounter a child with no Z-order
                        // index.
                        // 
                        // So that after we have collected all the children
                        // we could assign Z-order to all the children.
                        //
                        // However, when this function is called after
                        // making a 2D element we need to ensure that we
                        // don't set fZeroIndex to TRUE when the current
                        // child is the one which is made absolute, hence
                        // the following check.
                
                        if (!((lSourceIndexTemp == lSourceIndexElement) &&
                              (nZIndexMode == MADE_ABSOLUTE)))
                            fZeroIndex = TRUE;
                    }
                                 
                    if (fZIndexNegative)
                    {
                        if (lZindex < 0) // Collect only children with
                                         // negative Z-order index.
                        {
                            CZOrder z(pihtmlElementTemp, lZindex);
                            pczOrder[iIndex++] = z;
                        }
                    }
                    else
                    {
                        if (lZindex >= 0) // collect only children with
                                          // positive or no Z-order index.
                        {
                            CZOrder z(pihtmlElementTemp, lZindex);
                            pczOrder[iIndex++] = z;
                        }
                    }

                }
                
            }

        }
        
        SAFERELEASE(pihtmlElementTemp);
    }

    // If we have at least one child with no Z-order index and if we are
    // dealing with an element with a positive Z-order index, then we
    // assign new Z-order indexes to all the children collected above.

    if (fZeroIndex && !fZIndexNegative)
    {
        LONG lZOrder = ZINDEX_BASE;

        for ( lLoop = 0; lLoop < iIndex; lLoop++, lZOrder++)
        {
            if (pczOrder[lLoop].m_zOrder != 0)
            {
                // Maintain the existing Z-order index
                pczOrder[lLoop].m_zOrder += (iIndex+ZINDEX_BASE); 
            }
            else
            {
               pczOrder[lLoop].m_zOrder += lZOrder;
            }
            
        }
        
        if (iIndex > 1) 
        {
            // Wwe have at least two children; sort by Zorder index,
            // and propagate starting from ZINDEX_BASE.
            qsort( (LPVOID)pczOrder, iIndex, sizeof(CZOrder), CompareProc);
            hr = PropagateZIndex(pczOrder, iIndex);
            _ASSERTE(SUCCEEDED(hr));

            if (FAILED(hr))
                goto Cleanup;
            fSorted = TRUE;
        }

        
    }

    // If we have at least two children and not already sorted then sort
    // by Z-order index.
    if ((iIndex > 1) && !fSorted) 
        qsort( (LPVOID)pczOrder, iIndex, sizeof(CZOrder), CompareProc);

    if (IsEqualZIndex(pczOrder, iIndex))
    {
        hr = PropagateZIndex(pczOrder, iIndex);
        if (FAILED(hr))
            goto Cleanup;
    }

    if ((nZIndexMode == MADE_ABSOLUTE) ||
        (nZIndexMode == SEND_TO_FRONT) ||
        (nZIndexMode == SEND_BEHIND_1D))
    {
        LONG lZIndex;
        LONG lmaxZIndex = pczOrder[iIndex - 1].m_zOrder;

        if (fZIndexNegative)
        {
            if (iIndex == 0) // If we have no children with negative
                             // Z-order index.
            {
                hr = SetZIndex(pihtmlElement, -ZINDEX_BASE);
                goto Cleanup;
            }
            else 
            {
                // when we are dealing with elements with negative Z-order
                // we need to ensure that the maximum Z-order index (to be
                // assigned to the current element) can never become
                // greater than or equal to 0. If so then propagate
                // the Z-order index starting from ZINDEX_BASE.

                if ((lmaxZIndex + 1) >=0) 
                {
                    hr = PropagateZIndex(pczOrder, iIndex, fZIndexNegative);

                    if (FAILED(hr))
                        goto Cleanup;
                }
    
                lmaxZIndex = pczOrder[iIndex - 1].m_zOrder;
            }
        }

        if(SUCCEEDED(hr = GetZIndex(pihtmlElement, &lZIndex)))
        {
            if(lZIndex != lmaxZIndex) 
            {
                // The current element is not the top most element
                hr = SetZIndex(pihtmlElement, lmaxZIndex+1);
                _ASSERTE(SUCCEEDED(hr));
            }
            else if(lmaxZIndex == 0) 
            {
                // if the current element has no Z-order index
                hr = SetZIndex(pihtmlElement, ZINDEX_BASE);
                _ASSERTE(SUCCEEDED(hr));
            }
        }
    }
    else if ((nZIndexMode == SEND_BACKWARD) || (nZIndexMode == SEND_FORWARD))
    {
        LONG lPrevOrNextZIndex;
        LONG lIndexBuf = iIndex;

        hr = GetZIndex(pihtmlElement, &lPrevOrNextZIndex);
        
        if (FAILED(hr))
            goto Cleanup;

        if (iIndex == 1)
            goto Cleanup;
        
        while(--iIndex>=0)
        {
            if  (pczOrder[iIndex].m_zOrder == lPrevOrNextZIndex)
            {
        
                if (nZIndexMode == SEND_BACKWARD)
                {
                    if ( (iIndex - 1) < 0)
                        // The element already has the lowest Z-order index
                        // so exit.
                        goto Cleanup;
                    else
                        iIndex--;  
                }
                else
                {
                    if ((iIndex + 1) == lIndexBuf)
                        // The element already has the highest Z-order index
                        // so exit.
                        goto Cleanup;
                    else
                        iIndex++;
                }

                hr = SetZIndex(pihtmlElement, pczOrder[iIndex].m_zOrder);
                _ASSERTE(SUCCEEDED(hr));
                    
                if (FAILED(hr))
                    goto Cleanup;
                    
                hr = SetZIndex(pczOrder[iIndex].m_pihtmlElement, lPrevOrNextZIndex);
                _ASSERTE(SUCCEEDED(hr));
                    
                if (FAILED(hr))
                    goto Cleanup;

                break;
            }

        }

    }
    else if((nZIndexMode == SEND_TO_BACK) || (nZIndexMode == SEND_FRONT_1D)) 
    {
        LONG lZIndex;
        LONG lminZIndex = pczOrder[0].m_zOrder;

        if (iIndex == 0) 
        {
            // We have no children with a positive Z-order index
            hr = SetZIndex(pihtmlElement, ZINDEX_BASE);
            goto Cleanup;
        }

        if (!fZIndexNegative)
        {
            // When we are dealing with elements with positive Z-order
            // index, we need to ensure that the minimum Z-order index
            // (to be assigned to the current element) should never become
            // less than or equal to 0. If so then propagate the
            // Z-order index starting from ZINDEX_BASE.

            if ((lminZIndex - 1) <= 0) 
            {
                hr = PropagateZIndex(pczOrder, iIndex);
    
                if (FAILED(hr))
                    goto Cleanup;
            }

            lminZIndex = pczOrder[0].m_zOrder;
        }

        if(SUCCEEDED(hr = GetZIndex(pihtmlElement, &lZIndex)))
        {
            if(lZIndex != lminZIndex)
            {
                // The current element is not the bottom most element
                hr = SetZIndex(pihtmlElement, lminZIndex - 1);
                _ASSERTE(SUCCEEDED(hr));
            }
        }
    }
       
    if (SUCCEEDED(hr))
    {
        RECT rcElement;

        if (SUCCEEDED(GetElementPosition(pihtmlElement, &rcElement)))
        {
             InflateRect(&rcElement, ELEMENT_GRAB_SIZE, ELEMENT_GRAB_SIZE);
             if( SUCCEEDED(GetTridentWindow()))
             {
                 _ASSERTE(m_hwndTrident);
                 InvalidateRect(m_hwndTrident,&rcElement, FALSE);
             }
        }
    }
       
Cleanup:

    if (pczOrder)
        delete [] pczOrder;
    SAFERELEASE(pihtmlElementTemp);
    SAFERELEASE(pihtmlElementParent);
    SAFERELEASE(pihtmlCollection);

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::PropagateZIndex
//
// Set the Z-order index for each element in the given array. Return S_OK
// or a Trident error.
//

HRESULT CTriEditDocument::PropagateZIndex(CZOrder* pczOrder, LONG lZIndex, BOOL fZIndexNegative)
{
    HRESULT hr = S_OK; // init
    LONG lLoop;
    LONG lZOrder;

    // if fZIndexNegative is true means that we have a collection of
    // negative ZOrder elements and hence the initial ZOrder needs to
    // be ZINDEX_BASE + number of elments in the array.

    lZOrder = fZIndexNegative ? -(ZINDEX_BASE+lZIndex) : ZINDEX_BASE;

    for ( lLoop = 0; lLoop < lZIndex; lLoop++, lZOrder+=1)
    {
        hr = SetZIndex(pczOrder[lLoop].m_pihtmlElement, lZOrder);
        _ASSERTE(SUCCEEDED(hr));
    
        if (FAILED(hr))
            return hr;

        pczOrder[lLoop].m_zOrder = lZOrder;
    }

    return hr;
}
