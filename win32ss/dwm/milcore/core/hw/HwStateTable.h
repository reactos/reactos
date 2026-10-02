// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_d3d
//      $Keywords:
//
//  $Description:
//      Contains CStateTable declaration
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

MtExtern(StateTableMemory);

//+-----------------------------------------------------------------------------
//
//  Class:
//      CStateTable
//
//  Synopsis:
//      Keeps track of unpredictable states
//
//      It provides functionality to track state objects which can be unknown.
//
//  Responsibilites:
//      - Track whether we know the value of each state
//      - Provide common functionality for testing if a state is set, retrieving
//        state values, invalidating state, and making sure we set only supported
//        values in Debug.
//
//  Not responsible for:
//      - Making the state setting calls to D3D
//      - Keeping references to AddRef'able objects
//
//  Inputs required:
//      - Number of states to track buffer
//
//  Implementation Details:
//      - Keeps an array of StateKnown/StateValue pairs.  This appeared to give a
//        small performance win, although smaller resolution perf tests are
//        necessary to confirm this.
//
//       [ StateKnown | StateValue ] [ StateKnown | StateValue ] [ StateKnown
//       ..]
//
//  Reimplementing Table Caches:
//      - Table caches could be reimplemented by keeping a table pointer along we
//        each state value.  If the value is ever set, the table pointer would be
//        set to NULL along with it.  The structure of data stored would then be:
//
//        [ StateKnown | StateValue | StateTable ] [ StateKnown | StateValue |
//        StateTable ] [ StateKnown ...]
//

template<class StateType>
class CStateTable
{
private:

    class CEntry;

public:

    CStateTable();
    ~CStateTable();

    HRESULT Init(
        __in_range(1, 999) DWORD dwStateTableSize
        );

    MIL_FORCEINLINE bool IsStateSet(
        DWORD dwStateNum,
        __in_ecount(1) const StateType &oStateValue
        ) const
    {
        #if DBG
        Assert(dwStateNum < m_uDbgNumStates);
        #endif

        const CEntry &oEntry = m_rgStateData[dwStateNum];
        #if DBG
        Assert(oEntry.IsDbgSupported());
        #endif

        return oEntry.IsStateSet(oStateValue);
    }

    HRESULT GetState(
        DWORD dwStateNum,
        __out_ecount(1) StateType *pStateValue
        ) const;

    HRESULT GetStateNoAddRef(
        DWORD dwStateNum,
        __out_ecount(1) StateType *pStateValue
        ) const;

    void SetToUnknown(
        DWORD dwStateNum
        );

#if DBG
    void SetSupported(
        DWORD dwStateNum
        );
#endif

    void UpdateState(
        HRESULT hrStateChange,
        DWORD dwStateNum,
        __in_ecount(1) const StateType &oStateValue
        );

private:

    CEntry *m_rgStateData;  // The real state table
    
#if DBG
    UINT m_uDbgNumStates;   // Number of states tracked
#endif

};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CStateTable::CEntry<StateType>
//
//  Synopsis:
//      Data and methods for tracking a particular D3D state
//

template<class StateType>
class CStateTable<StateType>::CEntry
{
public:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(StateTableMemory));

    MIL_FORCEINLINE bool IsStateSet(
        __in_ecount(1) const StateType &oStateValue
        ) const
    {
        return IsKnown() && IsEqual(oStateValue);
    }

    MIL_FORCEINLINE bool IsEqual(__in_ecount(1) const StateType &oTestValue) const
    {
        if (m_oValue == oTestValue)
        {   
            return true;
        }
        else
        {
            return false;
        }
    }        

    MIL_FORCEINLINE void SetValue(
        __in_ecount(1) const StateType &oNewValue
        )
    {
        m_oValue = oNewValue;
    }

    MIL_FORCEINLINE void GetValue(
        __out_ecount(1) StateType &oOutValue
        ) const
    {
        oOutValue = m_oValue;
    }

    MIL_FORCEINLINE void GetValueNoAddRef(
        __out_ecount(1) StateType &oOutValue
        ) const
    {
        oOutValue = m_oValue;
    }

    bool IsKnown() const { return m_fKnown; }
    void SetKnown() { m_fKnown = true; }
    void SetUnknown() { m_fKnown = false; }

#if DBG
    bool IsDbgSupported() const { return m_fDbgSupported; }
    void SetSupported() { m_fDbgSupported = true; }
#endif

private:
    StateType m_oValue;     // When fKnown the state set in D3D

    bool m_fKnown;          // True if value of the state set in D3D is known

#if DBG
    bool m_fDbgSupported;   // True only for states we expect to track
#endif

};

//+-----------------------------------------------------------------------------
//
//  Member:
//      CStateTable<StateType>::ctor
//
//  Synopsis:
//      Initialize the StateData to NULL.
//
//------------------------------------------------------------------------------
template<class StateType>
CStateTable<StateType>::CStateTable()
{
    m_rgStateData = NULL;
#if DBG
    m_uDbgNumStates = 0;
#endif
};

//+-----------------------------------------------------------------------------
//
//  Member:
//      CStateTable<StateType>::dtor
//
//  Synopsis:
//      Release Memory.
//
//------------------------------------------------------------------------------
template<class StateType>
CStateTable<StateType>::~CStateTable()
{
    WPFFree(ProcessHeap, m_rgStateData);
};

//+-----------------------------------------------------------------------------
//
//  Member:
//      CStateTable<StateType>::Init
//
//  Synopsis:
//      Allocate the array, set it's known values to FALSE, and set supported
//      values to FALSE if in DBG.
//
//------------------------------------------------------------------------------
template<class StateType>
HRESULT
CStateTable<StateType>::Init(
    __in_range(1, 999) DWORD dwStateTableSize
    )
{
    HRESULT hr = S_OK;

    Assert(dwStateTableSize > 0);
    Assert(dwStateTableSize < 1000);

#if DBG
    m_uDbgNumStates = dwStateTableSize;
#endif

    //
    // Allocate the actual state table
    //

    m_rgStateData = new CEntry[dwStateTableSize];
    IFCOOM(m_rgStateData);

    //
    // Because we set all the known values to FALSE, we don't need to set the
    // values of the states themselves.  DECLARE_METERHEAP_CLEAR actually
    // does the set to FALSE.
    //

    for (UINT dwStateNum = 0; dwStateNum < dwStateTableSize; dwStateNum++)
    {
        Assert(!m_rgStateData[dwStateNum].IsKnown());
        #if DBG
        Assert(!m_rgStateData[dwStateNum].IsDbgSupported());
        #endif
    }

Cleanup:

    RRETURN(hr);

}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CStateTable<StateType>::GetState
//
//  Synopsis:
//      Returns the value of the State, or returns E_FAIL if the state value is
//      not known.
//
//------------------------------------------------------------------------------
template<class StateType>
HRESULT
CStateTable<StateType>::GetState(
    DWORD dwStateNum,
    __out_ecount(1) StateType *pStateValue
    ) const
{
    HRESULT hr = S_OK;

    #if DBG
    Assert(dwStateNum < m_uDbgNumStates);
    #endif
    Assert(pStateValue);

    CEntry const &oEntry = m_rgStateData[dwStateNum];

    #if DBG
    Assert(oEntry.IsDbgSupported());
    #endif

    if (!oEntry.IsKnown())
    {
        IFC(E_FAIL);
    }

    oEntry.GetValue(*pStateValue);

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CStateTable<StateType>::GetStateNoAddRef
//
//  Synopsis:
//      Returns the value of the State, or returns E_FAIL if the state value is
//      not known just like GetState.  However if the state is a ref counted
//      object no reference will be acquired.
//
//------------------------------------------------------------------------------
template<class StateType>
HRESULT
CStateTable<StateType>::GetStateNoAddRef(
    DWORD dwStateNum,
    __out_ecount(1) StateType *pStateValue
    ) const
{
    HRESULT hr = S_OK;

    #if DBG
    Assert(dwStateNum < m_uDbgNumStates);
    #endif
    Assert(pStateValue);

    CEntry const &oEntry = m_rgStateData[dwStateNum];

    #if DBG
    Assert(oEntry.IsDbgSupported());
    #endif

    if (!oEntry.IsKnown())
    {
        IFC(E_FAIL);
    }

    oEntry.GetValueNoAddRef(*pStateValue);

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CStateTable<StateType>::SetToUnknown
//
//  Synopsis:
//      Sets the state to Unknown.
//
//------------------------------------------------------------------------------
template<class StateType>
void
CStateTable<StateType>::SetToUnknown(
    DWORD dwStateNum
    )
{
    #if DBG
    Assert(dwStateNum < m_uDbgNumStates);
    #endif

    CEntry &oEntry = m_rgStateData[dwStateNum];

    #if DBG
    Assert(oEntry.IsDbgSupported());
    #endif

    oEntry.SetUnknown();
}

#if DBG
//+-----------------------------------------------------------------------------
//
//  Member:
//      CStateTable<StateType>::SetSupported
//
//  Synopsis:
//      Sets the supported value to true.  This will allow setting of this
//      state.
//
//------------------------------------------------------------------------------
template<class StateType>
void
CStateTable<StateType>::SetSupported(
    DWORD dwStateNum
    )
{
    Assert(dwStateNum < m_uDbgNumStates);
    m_rgStateData[dwStateNum].SetSupported();
}
#endif

//+-----------------------------------------------------------------------------
//
//  Member:
//      CStateTable<StateType>::UpdateState
//
//  Synopsis:
//      This function should be called after the state is set.  It will update
//      the value of the state and whether it is known based on the error code
//      returned from the state setting operation.
//
//------------------------------------------------------------------------------
template<class StateType>
void
CStateTable<StateType>::UpdateState(
    HRESULT hrStateChange,
    DWORD dwStateNum,
    __in_ecount(1) const StateType &oStateValue
    )
{
    #if DBG
    Assert(dwStateNum < m_uDbgNumStates);
    #endif

    CEntry &oEntry = m_rgStateData[dwStateNum];

    //
    // NOTE-2004/05/21-chrisra We don't assert on supported here
    //
    // This function is called on ForceSet**** which means this gets called even
    // when we're setting default state.
    //
    
    //
    // If the state change operation failed, we set the state to unknown.
    //

    if (SUCCEEDED(hrStateChange))
    {
        oEntry.SetKnown();
        oEntry.SetValue(oStateValue);
    }
    else
    {
        oEntry.SetUnknown();
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CStateTable<StateType>::CCEntry::GetValue specializations
//
//  Synopsis:
//      Save a new state value
//
//      NOTE: The HwStateTable does not keep references to AddRef'able
//            objects.  It assumes another reference is kept in another
//            location.  It will AddRef an object however, if it is
//            retrieved through GetValue.
//

template<>
MIL_FORCEINLINE void
CStateTable<IDirect3DBaseTexture9 *>::CEntry::GetValue(
    __out_ecount(1) IDirect3DBaseTexture9 *&oOutValue
    ) const
{
    oOutValue = m_oValue;

    if (m_oValue)
    {
        m_oValue->AddRef();
    }
}

template<>
MIL_FORCEINLINE void
CStateTable<IDirect3DVertexShader9 *>::CEntry::GetValue(
    __out_ecount(1) IDirect3DVertexShader9 *&oOutValue
    ) const
{
    oOutValue = m_oValue;

    if (m_oValue)
    {
        m_oValue->AddRef();
    }
}

template<>
MIL_FORCEINLINE void
CStateTable<IDirect3DPixelShader9 *>::CEntry::GetValue(
    __out_ecount(1) IDirect3DPixelShader9 *&oOutValue
    ) const
{
    oOutValue = m_oValue;

    if (m_oValue)
    {
        m_oValue->AddRef();
    }
}

template<>
MIL_FORCEINLINE void
CStateTable<IDirect3DIndexBuffer9 *>::CEntry::GetValue(
    __out_ecount(1) IDirect3DIndexBuffer9 *&oOutValue
    ) const
{
    oOutValue = m_oValue;

    if (m_oValue)
    {
        m_oValue->AddRef();
    }
}

template<>
MIL_FORCEINLINE void
CStateTable<IDirect3DVertexBuffer9 *>::CEntry::GetValue(
    __out_ecount(1) IDirect3DVertexBuffer9 *&oOutValue
    ) const
{
    oOutValue = m_oValue;

    if (m_oValue)
    {
        m_oValue->AddRef();
    }
}

template<>
MIL_FORCEINLINE void
CStateTable<IDirect3DSurface9 *>::CEntry::GetValue(
    __out_ecount(1) IDirect3DSurface9 *&oOutValue
    ) const
{
    oOutValue = m_oValue;

    if (m_oValue)
    {
        m_oValue->AddRef();
    }
}





