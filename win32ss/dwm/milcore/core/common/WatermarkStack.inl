// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Water mark stack template class.
//
//-----------------------------------------------------------------------------

#pragma once

//------------------------------------------------------------------------------
// CWatermarkStack
//------------------------------------------------------------------------------

template <
    class TValue, 
    UINT TMinCapacity, 
    UINT TGrowFactor, 
    UINT TTrimCount
    >
CWatermarkStack<TValue, TMinCapacity, TGrowFactor, TTrimCount>::CWatermarkStack()
{
    Initialize();
}

template <
    class TValue, 
    UINT TMinCapacity, 
    UINT TGrowFactor, 
    UINT TTrimCount
    >
VOID
CWatermarkStack<TValue, TMinCapacity, TGrowFactor, TTrimCount>::Initialize()
{
    C_ASSERT(TMinCapacity > 0);
    C_ASSERT(TGrowFactor > 1);

    m_uSize = 0;
    m_uCapacity = 0;
    m_pElements = NULL;
    m_uObserveCount = 0;
    m_uHighWatermark = 0;
}

template <
    class TValue, 
    UINT TMinCapacity, 
    UINT TGrowFactor, 
    UINT TTrimCount
    >
CWatermarkStack<TValue, TMinCapacity, TGrowFactor, TTrimCount>::~CWatermarkStack()
{
    WPFFree(ProcessHeap, m_pElements);
}

template <
    class TValue, 
    UINT TMinCapacity, 
    UINT TGrowFactor, 
    UINT TTrimCount
    >
HRESULT CWatermarkStack<TValue, TMinCapacity, TGrowFactor, TTrimCount>::Push(
    __in_ecount(1) const TValue& val
    )
{
    HRESULT hr = S_OK;

    TValue *pNewElements = NULL;

    if (m_uSize == m_uCapacity)
    {
        UINT uNewCapacity = 0, cbOldSize = 0;

        // Make sure that the growth factor makes sense.
        C_ASSERT(TGrowFactor > 1 && TGrowFactor < 32);

        IFC(UIntMult(m_uCapacity, TGrowFactor, &uNewCapacity));
        uNewCapacity = max(uNewCapacity, TMinCapacity);

        IFC(HrMalloc(
            Mt(CWatermarkStack),
            sizeof(TValue),
            uNewCapacity,
            reinterpret_cast<void**>(&pNewElements)
            ));

        IFC(UIntMult(m_uSize, sizeof(TValue), &cbOldSize));

        Assert(cbOldSize <= (sizeof(TValue) * uNewCapacity));

        memcpy(pNewElements, m_pElements, cbOldSize);

        WPFFree(ProcessHeap, m_pElements);

        m_pElements = pNewElements;
        m_uCapacity = uNewCapacity;

        pNewElements = NULL;
    }

    m_pElements[m_uSize] = val;
    m_uSize++;

    m_uHighWatermark = max(m_uHighWatermark, m_uSize);

Cleanup:
    WPFFree(ProcessHeap, pNewElements);

    RRETURN(hr);
}


template <
    class TValue, 
    UINT TMinCapacity, 
    UINT TGrowFactor, 
    UINT TTrimCount
    >
BOOL CWatermarkStack<TValue, TMinCapacity, TGrowFactor, TTrimCount>::Pop(
    __out_ecount_opt(1) TValue* pVal
    )
{
    // If the stack is empty, Pop returns false.
    if (m_uSize == 0)
    {
        return FALSE;
    }

    // Decrement the size.
    m_uSize--;

    // Return top element if the caller provided us with an out argument.
    if (pVal)
    {
        *pVal = m_pElements[m_uSize];
    }

    // In debug overwrite the entry for the value that was just popped.
#if DBG
    memset(&(m_pElements[m_uSize]), 0xef, sizeof(TValue));
#endif

    return TRUE;
}

template <
    class TValue, 
    UINT TMinCapacity, 
    UINT TGrowFactor, 
    UINT TTrimCount
    >
HRESULT CWatermarkStack<TValue, TMinCapacity, TGrowFactor, TTrimCount>::Top(
    __out_ecount(1) TValue* pVal
    )
{
    HRESULT hr = S_OK;

    if (IsEmpty())
    { 
        IFC(E_FAIL);
    }
    else
    {                
        *pVal = m_pElements[m_uSize-1];    
    }
Cleanup:
    RRETURN(hr);
}

template <
    class TValue, 
    UINT TMinCapacity, 
    UINT TGrowFactor, 
    UINT TTrimCount
    >
BOOL CWatermarkStack<TValue, TMinCapacity, TGrowFactor, TTrimCount>::IsEmpty() const
{
    return (m_uSize == 0);
}

template <
    class TValue, 
    UINT TMinCapacity, 
    UINT TGrowFactor, 
    UINT TTrimCount
    >
UINT CWatermarkStack<TValue, TMinCapacity, TGrowFactor, TTrimCount>::GetSize()
{
    return m_uSize;
}

template <
    class TValue, 
    UINT TMinCapacity, 
    UINT TGrowFactor, 
    UINT TTrimCount
    >
VOID CWatermarkStack<TValue, TMinCapacity, TGrowFactor, TTrimCount>::Clear()
{
    m_uSize = 0;
}

template <
    class TValue, 
    UINT TMinCapacity, 
    UINT TGrowFactor, 
    UINT TTrimCount
    >
VOID CWatermarkStack<TValue, TMinCapacity, TGrowFactor, TTrimCount>::Optimize()
{   
    Assert(m_uSize == 0); // The stack must be empty before this is called.
    Assert(m_uHighWatermark <= m_uCapacity);
    
    // After s_trimCount calls to this method we check the past usage of the stack.
    if (m_uObserveCount == TTrimCount)
    {
        HRESULT hr = S_OK;

        UINT uNewCapacity = max(m_uHighWatermark, TMinCapacity);
        UINT uNewCapacityGrown = 0;

        // Make sure that the growth factor makes sense.
        C_ASSERT(TGrowFactor > 1 && TGrowFactor < 32);

        MIL_THR(UIntMult(uNewCapacity, TGrowFactor + 1, &uNewCapacityGrown));

        if (SUCCEEDED(hr) && uNewCapacityGrown <= m_uCapacity)
        {
            // If the water mark is less or equal to capacity divided by the shrink 
            // factor (TGrowfactor+1), then we shrink the stack. Since the shrink factor is greater 
            // than the grow factor, we avoid oscillation of shrinking and growing 
            // the stack if the high water mark goes only slightly up and down.
            
            // Note that we don't need to copy the array because the stack is empty.            
            TValue* pNewElements = NULL;

            MIL_THR(HrMalloc(
                        Mt(CWatermarkStack),
                        sizeof(TValue),
                        uNewCapacity,
                        reinterpret_cast<void**>(&pNewElements)
                        ));

            if (SUCCEEDED(hr))
            {
                // If we are OOM and we can't allocate a new stack, we just keep the old one
                // since it is big enough anyway. We are doing this because it simplifies the error
                // handling significantly for the callers since we don't have to return an HRESULT here.

                // We successfully allocated a new array. Free the old one and then assign the new one.
                WPFFree(ProcessHeap, m_pElements);
                m_pElements = pNewElements;
                m_uCapacity = uNewCapacity;
            }
        }

        m_uHighWatermark = 0;
        m_uObserveCount = 0;
    }
    else
    {
        // Keep incrementing our observe count
        m_uObserveCount++;
    }
}

template <
    class TValue, 
    UINT TMinCapacity, 
    UINT TGrowFactor, 
    UINT TTrimCount
    >
__outro_ecount_opt(1) TValue const *
CWatermarkStack<TValue, TMinCapacity, TGrowFactor, TTrimCount>::GetTopByReference(
    ) const
{
    TValue *pTop = NULL;

    if (!IsEmpty())
    { 
        pTop = &m_pElements[m_uSize-1];
    }

    return pTop;
}


