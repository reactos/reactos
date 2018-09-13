#ifndef GUARD_D70787804D9C11d28784F6E920524153
#define GUARD_D70787804D9C11d28784F6E920524153

#include "comctrlp.h"

template <class T> class CDPA
{

public:
    // Functions

    CDPA() {}
    HDPA GetHDPA() {return m_hdpa;}
    void SetHDPA(const HDPA hdpa) {m_hdpa = hdpa;}

    BOOL    Create(int cItemGrow)
    {return (m_hdpa = DPA_Create(cItemGrow)) != NULL;}

    BOOL    CreateEx(int cpGrow, HANDLE hheap)
    {return (m_hdpa = DPA_CreateEx(cpGrow, hheap)) != NULL;}

    BOOL    Destroy()
    {return DPA_Destroy(m_hdpa);}

    HDPA    Clone(HDPA hdpaNew)
    {return DPA_Clone(m_hdpa, hdpaNew);}

    T*      GetPtr(INT_PTR i)
    {return (T*) DPA_GetPtr(m_hdpa, i);}

    int     GetPtrIndex(T* p)
    {return DPA_GetPtrIndex(m_hdpa, (LPVOID) p);}

    BOOL    Grow(int cp)
    {return DPA_Grow(m_hdpa, cp);}

    BOOL    SetPtr(int i, T* p)
    {return DPA_SetPtr(m_hdpa, i, (LPVOID) p);}

    int     InsertPtr(int i, T* p)
    {return DPA_InsertPtr(m_hdpa, i, (LPVOID) p);}

    T*      DeletePtr(int i)
    {return (T*) DPA_DeletePtr(m_hdpa, i);}

    BOOL    DPA_DeleteAllPtrs()
    {return DeleteAllPtrs(m_hdpa);}

    void    EnumCallback(PFNDPAENUMCALLBACK pfnCB, LPVOID pData)
    {DPA_EnumCallback(m_hdpa, pfnCB, pData);}

    void    DestroyCallback(PFNDPAENUMCALLBACK pfnCB, LPVOID pData)
    {DPA_DestroyCallback(m_hdpa, pfnCB, pData);}
    
    int     GetPtrCount()
    {return DPA_GetPtrCount(m_hdpa);}

    T*      GetPtrPtr()
    {return DPA_GetPtrPtr(m_hdpa);}

    T*      FastGetPtr(int i)
    {return DPA_FastGetPtr(m_hdpa, i);}
    
    int     AppendPtr(T* pitem)
    {return DPA_AppendPtr(m_hdpa, (LPVOID) pitem);}

#ifdef __IStream_INTERFACE_DEFINED__
    HRESULT LoadStream(PFNDPASTREAM pfn, IStream * pstream, LPVOID pvInstData)
    {return DPA_LoadStream(&m_hdpa, pfn, pstream, pvInstData);}

    HRESULT SaveStream(PFNDPASTREAM pfn, IStream * pstream, LPVOID pvInstData)
    {return DPA_SaveStream(m_hdpa, pfn, pstream, pvInstData);}
#endif

    BOOL    Sort(PFNDPACOMPARE pfnCompare, LPARAM lParam)
    {return DPA_Sort(m_hdpa, pfnCompare, lParam);}

    // Merge not supported through this object; use DPA_Merge

    int     Search(T* pFind, int iStart, PFNDPACOMPARE pfnCompare,
                    LPARAM lParam, UINT options)
    {return DPA_Search(m_hdpa, (LPVOID) pFind, iStart, pfnCompare, lParam, options);}
    
    int     SortedInsertPtr(T* pFind, int iStart, PFNDPACOMPARE pfnCompare, 
                    LPARAM lParam, UINT options, T* pitem)
    {return DPA_SortedInsertPtr(m_hdpa, (LPVOID) pFind, iStart, pfnCompare, lParam, options, (LPVOID) pitem);}

private:
    HDPA m_hdpa;
};

#endif // !GUARD_D70787804D9C11d28784F6E920524153