//**@@@*@@@****************************************************
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//**@@@*@@@****************************************************

//
// FileName:    audfilter.cpp
//
// Abstract:    
//      This is the implementation file for C++ classes that expose
//      functionality of KS filters that support streaming of PCM Audio
//

#include "kssample.h"

// ============================================================================
//
//   CKsAudFilter
//
//=============================================================================

////////////////////////////////////////////////////////////////////////////////
//
//  CKsAudFilter::CKsAudFilter()
//
//  Routine Description:
//      CKsAudFilter constructor
//  
//  Arguments: 
//      xxxx  
//
//
//  Return Value:
//  
//     S_OK on success
//


CKsAudFilter::CKsAudFilter
(
    LPCTSTR  pszName,
    HRESULT  *phr
) : CKsFilter(pszName, phr)
{
    TRACE_ENTER();
    TRACE_LEAVE_HRESULT(*phr);
    return;
}


////////////////////////////////////////////////////////////////////////////////
//
//  CKsAudFilter::EnumeratePins()
//
//  Routine Description:
//      EnumeratePins
//  
//  Arguments: 
//      xxxx  
//
//
//  Return Value:
//  
//     S_OK on success
//     E_FAIL on failure
//


HRESULT CKsAudFilter::EnumeratePins(void)
{
    TRACE_ENTER();
    HRESULT hr = S_OK;
    ULONG   cPins = 0;
    BOOL    fOutOfMemory = FALSE;
    KSPIN_COMMUNICATION ksPinCommunication = KSPIN_COMMUNICATION_NONE;
    KSPIN_DATAFLOW ksPinDataflow = (KSPIN_DATAFLOW)0;

    // get the number of pins supported by SAD
    hr = 
        GetPinPropertySimple
        (  
            0,
            KSPROPSETID_Pin,
            KSPROPERTY_PIN_CTYPES,
            &cPins,
            sizeof(cPins)
        );

    if (FAILED(hr))
    {
        DebugPrintf(TRACE_ERROR, TEXT("Failed to retrieve count of pins in audio filter"));
    }
    else // (SUCCEEDED(hr))
    {
        //
        // loop through the pins, looking for audio pins
        //
        for(ULONG nPinId = 0; nPinId < cPins; nPinId++)
        {

            // This information is needed to create the right type of pin
            
            //
            // get COMMUNICATION ---------------
            //
            ksPinCommunication = KSPIN_COMMUNICATION_NONE;
            hr = 
                GetPinPropertySimple
                ( 
                    nPinId,
                    KSPROPSETID_Pin,
                    KSPROPERTY_PIN_COMMUNICATION,
                    &ksPinCommunication,
                    sizeof(KSPIN_COMMUNICATION)
                );
            if (FAILED(hr))
            {
                DebugPrintf(TRACE_ERROR, TEXT("Failed to retrieve pin property KSPROPERTY_PIN_COMMUNICATION"));
            }

            if (SUCCEEDED(hr))
            {
                // ILL communication
                if ( (ksPinCommunication != KSPIN_COMMUNICATION_SOURCE) &&
                    (ksPinCommunication != KSPIN_COMMUNICATION_SINK) &&
                    (ksPinCommunication != KSPIN_COMMUNICATION_BOTH) )
                {
                    DebugPrintf(TRACE_ERROR, TEXT("Pin communication value doesn't make sense"));
                    hr = E_FAIL;
                }
            }

            ksPinDataflow = (KSPIN_DATAFLOW)0;
            if (SUCCEEDED(hr))
            {
                //
                // Get the data flow property
                //
                hr = 
                    GetPinPropertySimple
                    (             
                        nPinId,
                        KSPROPSETID_Pin,
                        KSPROPERTY_PIN_DATAFLOW,
                        &ksPinDataflow,
                        sizeof(KSPIN_DATAFLOW)
                    );
                if (FAILED(hr))
                {
                    DebugPrintf(TRACE_ERROR, TEXT("Failed to retrieve pin property KSPROPERTY_PIN_DATAFLOW"));
                }
            }

            //
            // create a new pin
            //
            CKsAudPin* pNewPin = NULL;

            if (SUCCEEDED(hr))
            {
                // If the pin is an IRP sink and inputs data, it's a render pin
                if ((KSPIN_COMMUNICATION_SINK == ksPinCommunication)
                &&  (KSPIN_DATAFLOW_IN == ksPinDataflow))
                {
                    pNewPin = new CKsAudRenPin(this, nPinId, &hr);
                }
                // If the pin is an IRP sink and outputs data, it's a capture pin
                else if ((KSPIN_COMMUNICATION_SINK == ksPinCommunication)
                     &&  (KSPIN_DATAFLOW_OUT == ksPinDataflow))
                {
                    pNewPin = new CKsAudCapPin(this, nPinId, &hr);
                }
                // Otherwise, it's just a pin
                else
                {
                    pNewPin = new CKsAudPin(this, nPinId, &hr);
                }

                if (NULL == pNewPin)
                {
                    DebugPrintf(TRACE_ERROR, TEXT("Failed to create audio pin"));
                    hr = E_OUTOFMEMORY;
                    fOutOfMemory = TRUE;
                    break;
                }
            }

            if (SUCCEEDED(hr))
            {
                if (NULL == m_listPins.AddTail(pNewPin))
                {
                    delete pNewPin;
                    DebugPrintf(TRACE_ERROR, TEXT("Unable to allocate list entry to save pin in"));
                    fOutOfMemory = TRUE;
                    hr = E_OUTOFMEMORY;
                    break;
                }
            }
            else //if (FAILED(hr))
            {
                delete pNewPin;
            }
        }
    }

    // If we ran out of memory, delete all the pins, as we're in a bad state
    if (fOutOfMemory)
    {
        DebugPrintf(TRACE_ERROR, TEXT("Ran out of memory enumerating pins - deleting all pins"));

        CKsPin* pPin;
        while (m_listPins.RemoveHead( &pPin ))
        {
            delete pPin;
        }

        hr = E_OUTOFMEMORY;
    }
    else
    {
        ClassifyPins();
        
        if( m_listRenderSinkPins.IsEmpty()
        &&  m_listRenderSourcePins.IsEmpty()
        &&  m_listCaptureSinkPins.IsEmpty()
        &&  m_listCaptureSourcePins.IsEmpty() )
        {
            DebugPrintf(TRACE_ERROR, TEXT("No valid pins found on the filter"));
            hr = E_FAIL;
        }
        else
        {
            hr = S_OK;
        }
    }

    TRACE_LEAVE_HRESULT(hr);
    return hr;
}


// ============================================================================
//
//   CKsAudRenFilter
//
//=============================================================================

////////////////////////////////////////////////////////////////////////////////
//
//  CKsAudRenFilter::CKsAudRenFilter()
//
//  Routine Description:
//      CKsAudRenFilter constructor
//  
//  Arguments: 
//      xxxx  
//
//
//  Return Value:
//  
//     S_OK on success
//
CKsAudRenFilter::CKsAudRenFilter
(
    LPCTSTR  pszName,
    HRESULT  *phr
) : CKsAudFilter(pszName, phr)
{
    TRACE_ENTER();

    m_eType = eAudRen;

    TRACE_LEAVE_HRESULT(*phr);
    return;
}


////////////////////////////////////////////////////////////////////////////////
//
//  CKsAudRenFilter::FindViablePin()
//
//  Routine Description:
//      Look through the given list and find one that can do pwfx
//  
//  Arguments: 
//      WAVEFORMATEX that the pin must support
//
//
//  Return Value:
//  
//     S_OK on success
//
CKsAudRenPin*
CKsAudRenFilter::FindViablePin
(
    const WAVEFORMATEX*   pwfx
)
{
    TRACE_ENTER();
    assert( pwfx );

    CKsPin*  pNode;
    LISTPOS listPos = m_listRenderSinkPins.GetHeadPosition();
    while(m_listRenderSinkPins.GetNext( listPos, &pNode ))
    {
        CKsAudRenPin* pPin = (CKsAudRenPin*)pNode;

        // To only look at non-digital output pins, check that pPin->IsVolumeSupported() is TRUE,
        // as digital output pins don't have volume controls associated with them.
        if( pPin->IsFormatSupported( pwfx ) )
        {
            // This should be a valid pin
            TRACE_LEAVE();
            return pPin;
        }
    }

    TRACE_LEAVE();
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////
//
//  CKsAudRenFilter::CreateRenderPin()
//
//  Routine Description:
//      Look through m_listRenderPins and find one that can do pwfx and create it
//  
//  Arguments: 
//      WAVEFORMATEX that the pin must support  
//      Flag whether or not the pin plays backed looped buffers or streamed buffers
//
//  Return Value:
//  
//     S_OK on success
//
CKsAudRenPin*
CKsAudRenFilter::CreateRenderPin
(
    const WAVEFORMATEX*   pwfx,
    BOOL            fLooped
)
{
    TRACE_ENTER();
    HRESULT hr = S_OK;

    CKsAudRenPin* pPin = FindViablePin(pwfx);

    if (!pPin)
    {
        DebugPrintf(TRACE_NORMAL, TEXT("Could not find a Render pin that supports the given wave format"));
        hr = E_FAIL;
    }
    else
    {
        hr = pPin->SetFormat(pwfx);

        if (FAILED(hr))
        {
            DebugPrintf(TRACE_ERROR, TEXT("Failed to set Render Pin format - the pin lied about its supported formats"));
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = pPin->Instantiate(fLooped);

        if (SUCCEEDED(hr))
        {
            DebugPrintf(TRACE_LOW, TEXT("Successfully instantiated Render Pin.  Handle = 0x%08x"), pPin->GetHandle());
        }
        else
        {
            DebugPrintf(TRACE_ERROR, TEXT("Failed to instantiate Render Pin"));
        }
    }

    if (FAILED(hr))
    {
        // Initialize pPin to NULL again
        pPin = NULL;

        // Try to intstantiate all the pins, one at a time
        CKsPin *pKsPin;
        LISTPOS listPosPin = m_listRenderSinkPins.GetHeadPosition();
        while( !pPin && m_listRenderSinkPins.GetNext( listPosPin, &pKsPin ))
        {
            CKsAudRenPin *pKsAudRenPin = (CKsAudRenPin *)pKsPin;
            hr = pKsAudRenPin->SetFormat( pwfx );
            if (SUCCEEDED(hr))
            {
                hr = pKsAudRenPin->Instantiate(fLooped);
            }

            if (SUCCEEDED(hr))
            {
                // Save the pin in pPin
                pPin = pKsAudRenPin;
                break;
            }
        }
    }

    if (FAILED(hr))
    {
        // Don't delete the pin - it's still in m_listRenderPins
        //delete pPin;
        pPin = NULL;
    }
    else
    {
        // Remove the pin from the filter's list of pins
        LISTPOS listPosPinNode = m_listPins.Find( pPin );
        assert(listPosPinNode);
        m_listPins.RemoveAt( listPosPinNode );

        listPosPinNode = m_listRenderSinkPins.Find( pPin );
        assert(listPosPinNode);
        m_listRenderSinkPins.RemoveAt( listPosPinNode );
    }

    TRACE_LEAVE_HRESULT(hr);
    return pPin;
}

////////////////////////////////////////////////////////////////////////////////
//
//  CKsAudRenFilter::CanCreateRenderPin()
//
//  Routine Description:
//      Look through m_listRenderPins and check if one that supports the given wave format
//  
//  Arguments: 
//      WAVEFORMATEX that the pin must support  
//
//
//  Return Value:
//  
//     TRUE if there is a pin that supports the format, FALSE otherwise
//
BOOL
CKsAudRenFilter::CanCreateRenderPin
(
    const WAVEFORMATEX*   pwfx
)
{
    TRACE_ENTER();
    HRESULT hr = S_OK;

    CKsAudRenPin* pPin = FindViablePin(pwfx);

    if (!pPin)
    {
        DebugPrintf(TRACE_NORMAL, TEXT("Could not find a Render pin that supports the given wave format"));
        hr = E_FAIL;
    }

    TRACE_LEAVE_HRESULT(hr);
    return SUCCEEDED(hr);
}


// ============================================================================
//
//   CKsAudCapFilter
//
//=============================================================================


////////////////////////////////////////////////////////////////////////////////
//
//  CKsAudCapFilter::CKsAudCapFilter()
//
//  Routine Description:
//      CKsAudCapFilter constructor
//  
//  Arguments: 
//      xxxx  
//
//
//  Return Value:
//  
//     S_OK on success
//


CKsAudCapFilter::CKsAudCapFilter
(
    LPCTSTR  pszName,
    HRESULT  *phr
) : CKsAudFilter(pszName, phr)
{
    TRACE_ENTER();

    m_eType = eAudCap;

    TRACE_LEAVE_HRESULT(*phr);
    return;
}


////////////////////////////////////////////////////////////////////////////////
//
//  CKsAudCapFilter::FindViablePin()
//
//  Routine Description:
//      Look through the given list and find one that can do pwfx
//  
//  Arguments: 
//      xxxx  
//
//
//  Return Value:
//  
//     S_OK on success
//


CKsAudCapPin*
CKsAudCapFilter::FindViablePin
(
    WAVEFORMATEX*   pwfx
)
{
    TRACE_ENTER();
    assert( pwfx );

    CKsPin*  pNode;
    LISTPOS listPos = m_listCaptureSinkPins.GetHeadPosition();
    while(m_listCaptureSinkPins.GetNext( listPos, &pNode ))
    {
        CKsAudCapPin* pPin = (CKsAudCapPin*)pNode;

        if( pPin->IsFormatSupported( pwfx ) )
        {
            // This should be a valid pin
            TRACE_LEAVE();
            return pPin;
        }
    }

    TRACE_LEAVE();
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////
//
//  CKsAudCapFilter::CreateCapturePin()
//
//  Routine Description:
//      Look through m_listCapturePins and find one that can do pwfx and create it
//  
//  Arguments: 
//      xxxx  
//
//
//  Return Value:
//  
//     S_OK on success
//


CKsAudCapPin*
CKsAudCapFilter::CreateCapturePin
(
    WAVEFORMATEX*   pwfx,
    BOOL            fLooped
)
{
    TRACE_ENTER();
    HRESULT hr = S_OK;

    CKsAudCapPin*   pPin = FindViablePin(pwfx);

    if (!pPin)
    {
        DebugPrintf(TRACE_ERROR, TEXT("Could not find a Capture pin that supports the given wave format"));
        hr = E_FAIL;
    }
    else
    {
        hr = pPin->SetFormat(pwfx);

        if (FAILED(hr))
        {
            DebugPrintf(TRACE_ERROR, TEXT("Failed to set Capture Pin format"));
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = pPin->Instantiate(fLooped);

        if (SUCCEEDED(hr))
        {
            DebugPrintf(TRACE_LOW, TEXT("Successfully instantiated Capture Pin.  Handle = 0x%08x"), pPin->GetHandle());
        }
        else
        {
            DebugPrintf(TRACE_ERROR, TEXT("Failed to instantiate Capture Pin"));
        }
    }

    if (FAILED(hr))
    {
        delete pPin;
        pPin = NULL;
    }

    TRACE_LEAVE_HRESULT(hr);
    return pPin;
}


////////////////////////////////////////////////////////////////////////////////
//
//  CKsAudCapFilter::GetCapturePin()
//
//  Routine Description:
//      Returns the Capture Pin
//  
//  Arguments: 
//      ppPin -- A pointer to a pointer to a CKsAudCapPin 
//
//
//  Return Value:
//  
//     S_OK on success
//


HRESULT CKsAudCapFilter::GetCapturePin(CKsAudCapPin** ppPin)
{
    TRACE_ENTER();
    HRESULT hr = S_OK;
    CKsPin* pPin = NULL;

    if (NULL == ppPin)
    {
        hr = E_POINTER;
    }

    if (SUCCEEDED(hr))
    {
        // Get the first pin on the list
        if (!m_listPins.GetHead(&pPin))
        {
            hr = E_FAIL;
        }
    }

    if (SUCCEEDED(hr))
    {
        *ppPin = static_cast<CKsAudCapPin*>(pPin);
    }
    
    TRACE_LEAVE_HRESULT(hr);
    return hr;
}


