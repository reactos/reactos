//**@@@*@@@****************************************************
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//**@@@*@@@****************************************************

//
// FileName:    audpin.cpp
//
// Abstract:    
//      This is the implementation file for C++ classes that expose
//      functionality of KS pins that support streaming of PCM Audio
//

#include "kssample.h"

// ============================================================================
//
//   CKsAudPin
//
//=============================================================================

// Utitlity Function

inline BOOL
IsEqualGUIDAligned(GUID guid1, GUID guid2)
{
    return ((*(PLONGLONG)(&guid1) == *(PLONGLONG)(&guid2)) && (*((PLONGLONG)(&guid1) + 1) == *((PLONGLONG)(&guid2) + 1)));
}




////////////////////////////////////////////////////////////////////////////////
//
//  CKsAudPin::CKsAudPin()
//
//  Routine Description:
//      CKsAudPin
//  
//  Arguments: 
//      Parent filter  
//      Pin ID
//      Result of creation
//
//
//  Return Value:
//  
//     S_OK on success
//


CKsAudPin::CKsAudPin
(
    CKsAudFilter*  pFilter,
    ULONG       nId,
    HRESULT* phr
) : CKsPin(pFilter, nId, phr),
    m_pAudFilter(pFilter),
    m_pWaveFormatEx(NULL),
    m_pksDataFormatWfx(NULL)
{
    TRACE_ENTER();
    HRESULT hr = *phr;

    if (SUCCEEDED(hr))
    {
        hr = m_listDataRange.Initialize(1);
        if (FAILED(hr))
            DebugPrintf(TRACE_ERROR,TEXT("Failed to Initialize m_listDataRange"));
    }

    if (SUCCEEDED(hr))
    {
        hr = m_listNodes.Initialize(1);
        if (FAILED(hr))
            DebugPrintf(TRACE_ERROR,TEXT("Failed to Initialize m_listNodes"));
    }

    // create a KSPIN_CONNECT structure to describe a waveformatex pin
    if (SUCCEEDED(hr))
    {
        m_cbPinCreateSize = sizeof(KSPIN_CONNECT) + sizeof(KSDATAFORMAT_WAVEFORMATEX);
        m_pksPinCreate = (PKSPIN_CONNECT)new BYTE[m_cbPinCreateSize];
        if (!m_pksPinCreate)
        {
            DebugPrintf(TRACE_ERROR,TEXT("Failed to allocate m_pksPinCreate"));
            hr = E_OUTOFMEMORY;
        }
    }

    if (SUCCEEDED(hr))
    {
        m_pksPinCreate->Interface.Set              = KSINTERFACESETID_Standard;
        m_pksPinCreate->Interface.Id               = KSINTERFACE_STANDARD_STREAMING;
        m_pksPinCreate->Interface.Flags            = 0;
        m_pksPinCreate->Medium.Set                 = KSMEDIUMSETID_Standard;
        m_pksPinCreate->Medium.Id                  = KSMEDIUM_TYPE_ANYINSTANCE;
        m_pksPinCreate->Medium.Flags               = 0;
        m_pksPinCreate->PinId                      = nId;
        m_pksPinCreate->PinToHandle                = NULL;
        m_pksPinCreate->Priority.PriorityClass     = KSPRIORITY_NORMAL;
        m_pksPinCreate->Priority.PrioritySubClass  = 1;

        // point m_pksDataFormatWfx to just after the pConnect struct
        PKSDATAFORMAT_WAVEFORMATEX pksDataFormatWfx = (PKSDATAFORMAT_WAVEFORMATEX)(m_pksPinCreate + 1);

        // set up format for KSDATAFORMAT_WAVEFORMATEX
        pksDataFormatWfx->DataFormat.FormatSize = sizeof(KSDATAFORMAT_WAVEFORMATEX);
        pksDataFormatWfx->DataFormat.Flags = 0;
        pksDataFormatWfx->DataFormat.Reserved = 0;
        pksDataFormatWfx->DataFormat.MajorFormat = KSDATAFORMAT_TYPE_AUDIO;
        pksDataFormatWfx->DataFormat.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
        pksDataFormatWfx->DataFormat.Specifier = KSDATAFORMAT_SPECIFIER_WAVEFORMATEX;

        m_pksDataFormatWfx = pksDataFormatWfx;
    }


    // Initialize the Pin;
    if (SUCCEEDED(hr))
    {
        hr = CKsAudPin::Init();
    }

    
    TRACE_LEAVE_HRESULT(hr);
    *phr = hr;
    return;
}

////////////////////////////////////////////////////////////////////////////////
//
//  CKsAudPin::~CKsAudPin()
//
//  Routine Description:
//      xxxx
//  
//  Arguments: 
//      xxxx  
//
//
//  Return Value:
//  
//     none
//


CKsAudPin::~CKsAudPin(void)
{
    TRACE_ENTER();
    KSDATARANGE_AUDIO *pKSDATARANGE_AUDIO;
    CKsNode *pKsNode;

    // Clear datarange list
    while( m_listDataRange.RemoveHead(&pKSDATARANGE_AUDIO) )
    {
        delete pKSDATARANGE_AUDIO;
    }

    // Clear the node list
    while( m_listNodes.RemoveHead(&pKsNode) )
    {
        delete pKsNode;
    }

    delete[] (BYTE *)m_pWaveFormatEx;

    TRACE_LEAVE();
    return;
}

////////////////////////////////////////////////////////////////////////////////
//
//  CKsAudPin::Init()
//
//  Routine Description:
//      Initialize internal data structures and does some sanity-checking
//  
//  Arguments: 
//      None  
//
//  Return Value:
//  
//     S_OK on success
//

HRESULT CKsAudPin::Init()
{
    TRACE_ENTER();
    HRESULT hr = S_OK;
    BOOL fViablePin = FALSE;

    // Make sure at least one interface is standard streaming
    if (SUCCEEDED(hr))
    {
        fViablePin = FALSE;
        for(ULONG i = 0; i < m_Descriptor.cInterfaces && !fViablePin; i++)
        {
            fViablePin = 
                fViablePin ||
                IsEqualGUIDAligned(m_Descriptor.pInterfaces[i].Set, KSINTERFACESETID_Standard) && 
                (m_Descriptor.pInterfaces[i].Id == KSINTERFACE_STANDARD_STREAMING) ;
        }

        if (!fViablePin)
        {
            DebugPrintf(TRACE_ERROR, TEXT("No standard streaming interfaces on the pin"));
            hr = E_FAIL;
        }
    }

    // Make sure at least one medium is standard streaming
    if (SUCCEEDED(hr))
    {
        fViablePin = FALSE;
        for(ULONG i = 0; i < m_Descriptor.cInterfaces && !fViablePin; i++)
        {
            fViablePin = 
                fViablePin ||
                IsEqualGUIDAligned(m_Descriptor.pMediums[i].Set, KSMEDIUMSETID_Standard) && 
                (m_Descriptor.pMediums[i].Id == KSMEDIUM_STANDARD_DEVIO) ;
        }

        if (!fViablePin)
        {
            DebugPrintf(TRACE_ERROR, TEXT("No standard streaming mediums on the pin"));
            hr = E_FAIL;
        }
    }
    

    // Make sure at least one datarange supports audio    
    if (SUCCEEDED(hr))
    {
        fViablePin = FALSE;
        PKSDATARANGE pDataRange = m_Descriptor.pDataRanges;
        
        for(ULONG i = 0; i < m_Descriptor.cDataRanges; i++)
        {
            // SubType should either be compatible with WAVEFORMATEX or 
            // it should be WILDCARD
            fViablePin = 
                fViablePin || 
                IS_VALID_WAVEFORMATEX_GUID(&pDataRange->SubFormat) ||
                IsEqualGUIDAligned(pDataRange->SubFormat, KSDATAFORMAT_SUBTYPE_PCM) ||
                IsEqualGUIDAligned(pDataRange->SubFormat, KSDATAFORMAT_SUBTYPE_WILDCARD);

            if (fViablePin && IsEqualGUIDAligned(pDataRange->MajorFormat, KSDATAFORMAT_TYPE_AUDIO))
            {
                // Copy the data range into the pin
                PKSDATARANGE_AUDIO pCopyDataRangeAudio = new KSDATARANGE_AUDIO;
                if( pCopyDataRangeAudio )
                {
                    PKSDATARANGE_AUDIO pDataRangeAudio = (PKSDATARANGE_AUDIO)pDataRange;
                    CopyMemory( pCopyDataRangeAudio, pDataRangeAudio, sizeof(KSDATARANGE_AUDIO) );
                    if (NULL == m_listDataRange.AddTail( pCopyDataRangeAudio ))
                    {
                        delete pCopyDataRangeAudio;
                        pCopyDataRangeAudio = NULL;
                        DebugPrintf(TRACE_ERROR, TEXT("Unable to allocate list entry to save datarange in"));
                        hr = E_OUTOFMEMORY;
                    }
                }
                else
                {
                    DebugPrintf(TRACE_ERROR, TEXT("Unable to allocate memory to save datarange in"));
                    hr = E_OUTOFMEMORY;
                }
            }

            pDataRange = (PKSDATARANGE)( ((PBYTE)pDataRange) + pDataRange->FormatSize);
        }

        if (!fViablePin)
        {
            DebugPrintf(TRACE_ERROR, TEXT("No audio dataranges on the pin"));
            hr = E_FAIL;
        }
    }

    TRACE_LEAVE_HRESULT(hr);
    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
//  CKsAudPin::SetFormat()
//
//  Routine Description:
//      Sets the format
//  
//  Arguments: 
//      Format to use when creating pin  
//
//
//  Return Value:
//  
//     S_OK on success
//


HRESULT    
CKsAudPin::SetFormat
(
    const WAVEFORMATEX* pwfx
)
{
    TRACE_ENTER();
    HRESULT hr = S_OK;

    if (!(pwfx && m_pksDataFormatWfx))
    {
        DebugPrintf(TRACE_ERROR,TEXT("pwfx or m_pksDataFormatWfx are NULL"));
        hr = E_FAIL;
    }

    DWORD dwNewFormatSize = 0;
    if (SUCCEEDED(hr))
    {
        dwNewFormatSize = GetWfxSize(pwfx);

        // If the new format differs in size from the old format, re-allocate m_pksPinCreate
        if( m_cbPinCreateSize !=
            dwNewFormatSize + sizeof(KSPIN_CONNECT) + sizeof(KSDATAFORMAT_WAVEFORMATEX) - sizeof(WAVEFORMATEX) )
        {
            // create a KSPIN_CONNECT structure to describe a waveformatex pin
            DWORD dwNewSize = dwNewFormatSize + sizeof(KSPIN_CONNECT) + sizeof(KSDATAFORMAT_WAVEFORMATEX) - sizeof(WAVEFORMATEX);
            void *pPinCreate = new BYTE[dwNewSize];
            if (!pPinCreate)
            {
                DebugPrintf(TRACE_ERROR,TEXT("Unable to allocate KSPIN_CONNECT structure"));
                hr = E_OUTOFMEMORY;
            }
            else
            {
                // Copy the old pin structure to the new one
                CopyMemory( pPinCreate, m_pksPinCreate, min(dwNewSize, m_cbPinCreateSize) );

                // Free the old structure
                delete[] m_pksPinCreate;
                m_pksPinCreate = NULL;

                // point m_pksPinCreate at the new structure
                m_pksPinCreate = (PKSPIN_CONNECT)pPinCreate;

                // point m_pksDataFormatWfx to just after the pConnect struct
                m_pksDataFormatWfx = (PKSDATAFORMAT_WAVEFORMATEX)(m_pksPinCreate + 1);

                // Set the new format size parameter
                m_pksDataFormatWfx->DataFormat.FormatSize = dwNewFormatSize + sizeof(KSDATAFORMAT_WAVEFORMATEX) - sizeof(WAVEFORMATEX);
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        // Copy the new format into pksDataFormatWfx
        PKSDATAFORMAT_WAVEFORMATEX pksDataFormatWfx = m_pksDataFormatWfx;
        CopyMemory(&pksDataFormatWfx->WaveFormatEx, pwfx, dwNewFormatSize);

        // Set the sample size in pksDataFormatWfx
        pksDataFormatWfx->DataFormat.SampleSize = (USHORT)(pwfx->nChannels * pwfx->wBitsPerSample / 8);
     
        // Save the wave format
        delete[] (BYTE*)m_pWaveFormatEx;
        m_pWaveFormatEx = (WAVEFORMATEX *)new BYTE[dwNewFormatSize];
        if( m_pWaveFormatEx )
        {
            CopyMemory(m_pWaveFormatEx, pwfx, dwNewFormatSize);
        }
    }

    TRACE_LEAVE_HRESULT(hr);
    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
//  CKsAudPin::GetPosition()
//
//  Routine Description:
//      Gets the current position
//  
//  Arguments: 
//      Address to store the position  
//
//
//  Return Value:
//  
//     S_OK on success
//


HRESULT
CKsAudPin::GetPosition
(
    KSAUDIO_POSITION* pPos
)
{
    TRACE_ENTER();
    HRESULT hr = S_OK;

    if (NULL == pPos)
    {
        hr = E_INVALIDARG;
    }

    if (SUCCEEDED(hr))
    {
        hr = GetPropertySimple(KSPROPSETID_Audio, KSPROPERTY_AUDIO_POSITION, pPos, sizeof(KSAUDIO_POSITION));

        if (FAILED(hr))
        {
            DebugPrintf(TRACE_ERROR, TEXT("Failed to retrieve audio stream position - %#08x"), hr);
        }
    }

    TRACE_LEAVE_HRESULT(hr);
    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
//  CKsAudPin::SetPosition()
//
//  Routine Description:
//      Sets the current position
//  
//  Arguments: 
//      Address of new position
//
//
//  Return Value:
//  
//     S_OK on success
//


HRESULT
CKsAudPin::SetPosition
(
    KSAUDIO_POSITION* pPos
)
{
    TRACE_ENTER();
    HRESULT hr = S_OK;

    if (NULL == pPos)
    {
        hr = E_INVALIDARG;
    }

    if (SUCCEEDED(hr))
    {
        hr = SetPropertySimple(KSPROPSETID_Audio, KSPROPERTY_AUDIO_POSITION, pPos, sizeof(KSAUDIO_POSITION));

        if (FAILED(hr))
        {
            DebugPrintf(TRACE_ERROR, TEXT("Failed to set audio stream position - %#08x"), hr);
        }
    }

    TRACE_LEAVE_HRESULT(hr);
    return hr;
}


////////////////////////////////////////////////////////////////////////////////
//
//  CKsAudPin::GetNode()
//
//  Routine Description:
//
//      Get the CKsNode* in m_listNodes for a given ID value
//  
//  Arguments:
//  
//      Node ID
//  
//  Return Value:
//  
//      Pointer to the matching CKsNode, if any
//


CKsNode* CKsAudPin::GetNode(ULONG ulNodeID)
{
    TRACE_ENTER();
    CKsNode *pksNode = NULL;
    LISTPOS listPos = m_listNodes.GetHeadPosition();
    while( m_listNodes.GetNext( listPos, &pksNode ) )
    {
        if( ulNodeID == pksNode->GetId())
        {
            break;
        }
    }

    TRACE_LEAVE();
    return pksNode;
}

////////////////////////////////////////////////////////////////////////////////
//
//  CKsAudPin::IsFormatSupported()
//
//  Routine Description:
//      Check if the given format is supported
//  
//  Arguments: 
//      Waveformat to check against
//
//
//  Return Value:
//  
//     TRUE if format is supported
//


BOOL CKsAudPin::IsFormatSupported(const WAVEFORMATEX* pwfx)
{
    TRACE_ENTER();

    LISTPOS             listPosRange = m_listDataRange.GetHeadPosition();
    KSDATARANGE_AUDIO*  pKSDATARANGE_AUDIO;

    while( m_listDataRange.GetNext( listPosRange, &pKSDATARANGE_AUDIO )  )
    {
        if( KSDATAFORMAT_TYPE_WILDCARD == pKSDATARANGE_AUDIO->DataRange.MajorFormat
        ||  KSDATAFORMAT_TYPE_AUDIO == pKSDATARANGE_AUDIO->DataRange.MajorFormat )
        {
            // Set the format to search for
            GUID guidFormat = {DEFINE_WAVEFORMATEX_GUID(pwfx->wFormatTag)};

            // If this is a WaveFormatExtensible structure, then use its defined SubFormat
            if( WAVE_FORMAT_EXTENSIBLE == pwfx->wFormatTag )
            {
                guidFormat = ((WAVEFORMATEXTENSIBLE *)pwfx)->SubFormat;
            }

            if( KSDATAFORMAT_SUBTYPE_WILDCARD == pKSDATARANGE_AUDIO->DataRange.SubFormat
            ||  guidFormat == pKSDATARANGE_AUDIO->DataRange.SubFormat )
            {
                if( KSDATAFORMAT_SPECIFIER_WILDCARD == pKSDATARANGE_AUDIO->DataRange.Specifier
                ||  KSDATAFORMAT_SPECIFIER_WAVEFORMATEX == pKSDATARANGE_AUDIO->DataRange.Specifier )
                {
                    if( pKSDATARANGE_AUDIO->MaximumChannels >= pwfx->nChannels
                    &&  pKSDATARANGE_AUDIO->MinimumBitsPerSample <= pwfx->wBitsPerSample
                    &&  pKSDATARANGE_AUDIO->MaximumBitsPerSample >= pwfx->wBitsPerSample
                    &&  pKSDATARANGE_AUDIO->MinimumSampleFrequency <= pwfx->nSamplesPerSec
                    &&  pKSDATARANGE_AUDIO->MaximumSampleFrequency >= pwfx->nSamplesPerSec )
                    {
                        // This should be a valid pin
                        TRACE_LEAVE();
                        return TRUE;
                    }
                }
            }
        }
    }

    TRACE_LEAVE();
    return FALSE;
}



// ============================================================================
//
//   CKsAudRenPin
//
//=============================================================================

////////////////////////////////////////////////////////////////////////////////
//
//  CKsAudRenPin::CKsAudRenPin()
//
//  Routine Description:
//      CKsAudRenPin
//  
//  Arguments: 
//      Parent filter  
//      Pin ID
//      Result of creation
//
//  Return Value:
//  
//     None
//
CKsAudRenPin::CKsAudRenPin
(
    CKsAudFilter*  pFilter,
    ULONG       nId,
    HRESULT* phr
) : CKsAudPin(pFilter, nId, phr)
{
    TRACE_ENTER();
    m_eType = eAudRen;

    TRACE_LEAVE_HRESULT(*phr);
    return;
}

////////////////////////////////////////////////////////////////////////////////
//
//  CKsAudRenPin::~CKsAudRenPin()
//
//  Routine Description:
//      xxxx
//  
//  Arguments: 
//      xxxx  
//
//
//  Return Value:
//  
//     none
//
CKsAudRenPin::~CKsAudRenPin(void)
{
    TRACE_ENTER();

    TRACE_LEAVE();
    return;
}


// ============================================================================
//
//   CKsAudCapPin
//
//=============================================================================


////////////////////////////////////////////////////////////////////////////////
//
//  CKsAudCapPin::CKsAudCapPin()
//
//  Routine Description:
//      CKsAudCapPin
//  
//  Arguments: 
//      Parent filter  
//      Pin ID
//      Result of creation
//
//  Return Value:
//  
//     S_OK on success
//


CKsAudCapPin::CKsAudCapPin
(
    CKsAudFilter*  pFilter,
    ULONG       nId,
    HRESULT* phr
) : CKsAudPin(pFilter, nId, phr)
{
    TRACE_ENTER();
    m_eType = eAudCap;

    TRACE_LEAVE_HRESULT(*phr);
    return;
}

////////////////////////////////////////////////////////////////////////////////
//
//  CKsAudCapPin::~CKsAudCapPin()
//
//  Routine Description:
//      xxxx
//  
//  Arguments: 
//      xxxx  
//
//
//  Return Value:
//  
//     none
//


CKsAudCapPin::~CKsAudCapPin(void)
{
    TRACE_ENTER();
    TRACE_LEAVE();
    return;
}

