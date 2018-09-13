#ifndef __IFACEPTR_ENUMERATOR_TEMPLATE_H
#define __IFACEPTR_ENUMERATOR_TEMPLATE_H

//
// Instantiate with T as the pointer type.
// i.e. EnumIFacePtrs<PSETTINGS_FOLDER_CATEGORY>
//
template <class T>
class EnumIFacePtrs
{
    private:
        UINT        m_iCurrent;
        PointerList m_List;

    public:
        EnumIFacePtrs(T *ppIFace, UINT cItems
            ) : m_iCurrent(0)
        {
            DebugMsg(DM_NOW, TEXT("EnumIFacePtrs::EnumIFacePtrs"));

            for (UINT i = 0; i < cItems; i++)
            {
                Assert(NULL != *(ppIFace + i));
                m_List.Append((LPVOID)(*(ppIFace + i))); // Can throw OutOfMemory.
                DebugMsg(DM_NOW, TEXT("EnumIFacePtrs added 0x%08X at item %d"), *(ppIFace+i), i);
            }
            DebugMsg(DM_NOW, TEXT("EnumIFacePtrs::EnumIFacePtrs [LEAVE]"));
        }

        EnumIFacePtrs(EnumIFacePtrs& rhs);
        ~EnumIFacePtrs(VOID);

        //
        // IEnumXXXXX methods.
        //
        HRESULT
        Next(
            DWORD cRequested, 
            T *ppIFace, 
            LPDWORD pcCreated);
            
        HRESULT
        Skip(
            DWORD cSkip);

        HRESULT
        Reset(
            VOID);

        HRESULT
        Clone(
            EnumIFacePtrs **);
};



template <class T>
EnumIFacePtrs<T>::EnumIFacePtrs(
    EnumIFacePtrs& rhs
    ) : m_iCurrent(0)
{
    DebugMsg(DM_NOW, TEXT("EnumIFacePtrs::EnumIFacePtrs"));
    INT cItems = rhs.m_List.Count();
    for (INT i = 0; i < cItems; i++)
    {
        T pIFace = NULL;
        rhs.m_List.Retrieve((LPVOID *)&pIFace, i);
        Assert(NULL != pIFace);

        m_List.Append((LPVOID)pIFace); // Can throw OutOfMemory.
        pIFace->AddRef();
        DebugMsg(DM_NOW, TEXT("EnumIFacePtrs added 0x%08X at item %d"), pIFace, i);
    }
    DebugMsg(DM_NOW, TEXT("EnumIFacePtrs::EnumIFacePtrs [LEAVE]"));
}

template<class T>
EnumIFacePtrs<T>::~EnumIFacePtrs(
    VOID
    )
{
    T pIFace = NULL;

    DebugMsg(DM_NOW, TEXT("EnumIFacePtrs::~EnumIFacePtrs"));
    //
    // Release all objects held in member list.
    //
    while(m_List.Remove((LPVOID *)&pIFace, 0))
    {
        Assert(NULL != pIFace);
        DebugMsg(DM_NOW, TEXT("Releasing ptr 0x%08X from PointerList"), pIFace);
        pIFace->Release();
    }
    DebugMsg(DM_NOW, TEXT("EnumIFacePtrs::~EnumIFacePtrs [LEAVE]"));
}

template<class T>
HRESULT
EnumIFacePtrs<T>::Next(
    DWORD cRequested, 
    T *ppIFace, 
    LPDWORD pcCreated
    )
{
    HRESULT hResult = S_OK;

    DebugMsg(DM_NOW, TEXT("EnumIFacePtrs::Next %d"), cRequested);
    if (NULL != ppIFace)
    {
        DWORD cCreated = 0;
        UINT cItems = m_List.Count();

        //
        // Transfer data to caller's array.
        // Stop when at the end of the enumeration or we've
        // returned all that the caller asked for.
        //
        while(m_iCurrent < cItems && cRequested > 0)
        {
            DebugMsg(DM_NOW, TEXT("Enumerating, iCurrent = %d,  cItems = %d, cRequested = %d"),
                                  m_iCurrent, cItems, cRequested);
            T pIFace = NULL;
            m_List.Retrieve((LPVOID *)&pIFace, m_iCurrent++);
            Assert(NULL != pIFace);

            pIFace->AddRef();
            *(ppIFace + cCreated) = pIFace;
            DebugMsg(DM_NOW, TEXT("Enumerated 0x%08X"), pIFace);

            cCreated++;
            cRequested--;
        }

        //
        // If requested, return the count of categories enumerated.
        //
        if (NULL != pcCreated)
            *pcCreated = cCreated;

        if (cRequested > 0)
        {
            //
            // Less than requested number of categories were retrieved.
            // 
            hResult = S_FALSE;
        }
        DebugMsg(DM_NOW, TEXT("EnumIFacePtrs::Next [LEAVE] %d enumerated"), cCreated);
    }
    else
        hResult = E_POINTER;

    return hResult;
}

    
template<class T>
HRESULT
EnumIFacePtrs<T>::Skip(
    DWORD cSkip)
{
    UINT cItems = m_List.Count();
    while(m_iCurrent < cItems && cSkip > 0)
    {
        m_iCurrent++;
        cSkip--;
    }

    return cSkip == 0 ? S_OK : S_FALSE;
}

template<class T>
HRESULT
EnumIFacePtrs<T>::Reset(
    VOID)
{
    m_iCurrent = 0;
    return S_OK;
}


template<class T>
HRESULT
EnumIFacePtrs<T>::Clone(
    EnumIFacePtrs<T> **ppEnum
    )
{
    HRESULT hResult         = NO_ERROR;
    EnumIFacePtrs<T> *pEnum = NULL;

    if (NULL != ppEnum)
    {
        *ppEnum = NULL;

        try
        {        
            pEnum = new EnumIFacePtrs<T>((const EnumIFacePtrs<T>&)*this);
            *ppEnum = pEnum;
        }
        catch(OutOfMemory)
        {
            hResult = E_OUTOFMEMORY;
        }
        catch(...)
        {
            hResult = E_UNEXPECTED;
        }

        if (FAILED(hResult) && NULL != pEnum)
        {
            delete pEnum;
            *ppEnum = NULL;
        }

    }
    else
        hResult = E_INVALIDARG;

    return hResult;
}

#endif // __IFACEPTR_ENUMERATOR_TEMPLATE_H
