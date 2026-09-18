// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains the definition for a simple block allocator.
//
//  Classes:
//      TMemBlock, TMemBlockBase
//
//------------------------------------------------------------------------------

#pragma once

MtExtern(TMemBlock);
MtExtern(TMemBlockBase);

// Fill pattern for free'd blocks
// Particular byte pattern chosen to ensure that doubles will be filled with NaN.
#define TMEMBLOCK_FILL_DWORD 0xFFFFABCD

#ifdef __cplusplus
extern "C" {
#endif

// copied from ntrtl_x.h

__checkReturn
NTSYSAPI
SIZE_T
NTAPI
RtlCompareMemoryUlong (
    __in_bcount(Length) PVOID Source,
    __in SIZE_T Length,
    __in ULONG Pattern
    );

#if defined(_M_AMD64)

FORCEINLINE
VOID
RtlFillMemoryUlong (
    __out_bcount_full(Length) PVOID Destination,
    __in SIZE_T Length,
    __in ULONG Pattern
    )

{

    PULONG Address = (PULONG)Destination;

    //
    // If the number of DWORDs is not zero, then fill the specified buffer
    // with the specified pattern.
    //

    if ((Length /= 4) != 0) {

        //
        // If the destination is not quadword aligned (ignoring low bits),
        // then align the destination by storing one DWORD.
        //

        if (((ULONG64)Address & 4) != 0) {
            *Address = Pattern;
            if ((Length -= 1) == 0) {
                return;
            }

            Address += 1;
        }

        //
        // If the number of QWORDs is not zero, then fill the destination
        // buffer a QWORD at a time.
        //

         __stosq((PULONG64)(Address),
                 Pattern | ((ULONG64)Pattern << 32),
                 Length / 2);

        if ((Length & 1) != 0) {
            Address[Length - 1] = Pattern;
        }
    }

    return;
}

#else

NTSYSAPI
VOID
NTAPI
RtlFillMemoryUlong (
    __out_bcount_full(Length) PVOID Destination,
    __in SIZE_T Length,
    __in ULONG Pattern
   );

#endif

#ifdef __cplusplus
}       // extern "C"
#endif


//+------------------------------------------------------------------------
//
//  Class:
//      template <class TElement> TMemBlockBase
//
//  Synopsis:
//      The instance of TMemBlockBase serves as a storage for many
//      instances of TElement. It works like regular operators "new"
//      and "delete", but consumes less processor ticks and decreases
//      memory fragmentation.
//
//      Memory is allocated by 4Kb blocks so every block contains
//      many elements.
//
//      Freed elements are stored in temporary salvage stack and
//      can be reused on next Allocate() calls.
//
//  Usage pattern:
//      class CSomeClass
//      {
//          CSomeClass();   // constructor is called on Allocate()
//          ~CSomeClass();  // destructor is called on Free()
//          ...
//      }
//
//      TMemBlockBase<CSomeClass> storage;
//
//      TElement *pElement, *pAnotherElement;
//
//      IFC(storage.Allocate(&pElement);
//      IFC(storage.Allocate(&pAnotherElement);
//      storage.Free(pElement);
//      etc.
//      
//  Note:
//      It is not necessary to Free() all the allocated elements;
//      TMemBlockBase destructor will free all the memory occupied
//      during Allocate() calls.
//
//-------------------------------------------------------------------------
template <class TElement>
class TMemBlockBase
{
public:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(TMemBlockBase));

    TMemBlockBase();
    ~TMemBlockBase();

    HRESULT Allocate(
        __deref_out_ecount(1) TElement **ppElement
        );
    void Free(__out_ecount(1) TElement *pElement);

private:

    // Struct TMemBlock: storage for TElements.
    struct TMemBlock
    {
        DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(TMemBlock));
        TMemBlock();
        ~TMemBlock(){};

        // This is used by TMemBlockBase for chaining TMemBlocks together
        TMemBlock *m_pNextBlock;
        
        //
        // Future Consideration:  This constant has been chosen rather
        // arbitrarily to be the size of an x86 page. It's possible one could
        // improve performance by tweaking it.
        //
        static const UINT sc_uMaxSize = 0x1000;

        static const UINT sc_uCapacity = sc_uMaxSize/sizeof(TElement);

        // Data storage
        //
        // (Stored as a byte array, since we do not want TElement's
        // constructor to be called.)
        //
        C_ASSERT(__alignof(TElement) <= 8);

// 'TMemBlockBase<TElement>::TMemBlock' : structure was padded due to __declspec(align())
#pragma warning(push)
#pragma warning(disable:4324)
        __declspec(align(8)) BYTE m_rgStorage[sc_uCapacity * sizeof(TElement)];
#pragma warning(pop)
    };


    // The list of all blocks (initially NULL) is headed here.
    TMemBlock *m_pBlockList;

    // How many elements are available in the current block.
    UINT m_uElementCount;

    // Struct CStackEntry: we cast TElements to this structure
    // when storing freed elements in salvage stack.
    struct CStackEntry
    {
        CStackEntry *m_pNext;
    };


    // Salvage stack: freed elements are hooked up here and so can be reused.
    CStackEntry *m_pSalvage;
};

//+------------------------------------------------------------------------
//
//  Method:
//      TMemBlockBase<TElement>::TMemBlock::TMemBlock
//
//  Synopsis:
//      Constructor.
//
//-------------------------------------------------------------------------
template <class TElement>
TMemBlockBase<TElement>::TMemBlock::TMemBlock()
{
    // A MemBlock that has no capacity is useless
    C_ASSERT(sc_uCapacity != 0);

    // We cast TElements to CStackEntry when storing free'd elements.
    C_ASSERT(sizeof(TElement) >= sizeof(CStackEntry));

#if DBG
    //
    // Fill in each TMemBlock with a test pattern that we can check during
    // allocation.
    //
    // When we check, we skip the first sizeof(CStackEntry) bytes.
    // Hence, we need to ensure that TElement and CStackEntry are both a
    // multiple of 4 bytes in size.
    //
    C_ASSERT(sizeof(TElement) % sizeof(ULONG) == 0);
    C_ASSERT(sizeof(CStackEntry) % sizeof(ULONG) == 0);

    RtlFillMemoryUlong(m_rgStorage, sizeof(m_rgStorage), TMEMBLOCK_FILL_DWORD);
#endif
}
    
//+------------------------------------------------------------------------
//
//  Method:
//      TMemBlockBase<TElement>::TMemBlockBase<TElement>
//
//  Synopsis:
//      Constructor.
//
//-------------------------------------------------------------------------
template <class TElement>
TMemBlockBase<TElement>::TMemBlockBase()
{
    m_pBlockList = NULL;
    m_pSalvage = NULL;
    m_uElementCount = 0;
}

//+------------------------------------------------------------------------
//
//  Method:
//      TMemBlockBase<TElement>::~TMemBlockBase<TElement>
//
//  Synopsis:
//      Destructor
//
//-------------------------------------------------------------------------
template <class TElement>
TMemBlockBase<TElement>::~TMemBlockBase()
{
#if DBG
    for (CStackEntry *pEntry = m_pSalvage; pEntry; pEntry = pEntry->m_pNext)
    {
        if (!RtlCompareMemoryUlong(reinterpret_cast<BYTE *>(pEntry)+sizeof(CStackEntry),
                sizeof(TElement)-sizeof(CStackEntry), TMEMBLOCK_FILL_DWORD))
        {
            RIP("Block has been used since free'ing.");
        }
    }
#endif

    while (m_pBlockList != NULL)
    {
        TMemBlock *pBlock = m_pBlockList;
        m_pBlockList = pBlock->m_pNextBlock;

        delete pBlock;
    }
}

//+------------------------------------------------------------------------
//
//  Method:
//      TMemBlockBase<TElement>::Allocate
//
//  Synopsis:
//      Allocate the memory for TElement instance.
//
//-------------------------------------------------------------------------
template <class TElement>
HRESULT
TMemBlockBase<TElement>::Allocate(
    __deref_out_ecount(1) TElement **ppElement
    )
{
    HRESULT hr = S_OK;
    TElement *pElement = 0;

    if (m_pSalvage != NULL)
    {
        // If we have freed elements then reuse last freed.
        CStackEntry *pEntry = m_pSalvage;
        m_pSalvage = pEntry->m_pNext;
        pElement = reinterpret_cast<TElement*>(pEntry);
    }
    else
    {
        if (m_uElementCount == 0)
        {
            // Allocate new block
            TMemBlock *pNewBlock = new TMemBlock;
            IFCOOM(pNewBlock);

            // Hook up as the first in the list.
            pNewBlock->m_pNextBlock = m_pBlockList;
            m_pBlockList = pNewBlock;

            m_uElementCount = TMemBlock::sc_uCapacity;
        }

        // Allocate element memory in current memory block.
        Assert(m_pBlockList != NULL);
        TElement *prgStorage = reinterpret_cast<TElement*>(m_pBlockList->m_rgStorage);

        Assert(m_uElementCount > 0); // ensured above
        pElement = &prgStorage[--m_uElementCount];
    }

#if DBG
    if (!RtlCompareMemoryUlong(reinterpret_cast<BYTE *>(pElement)+sizeof(CStackEntry),
            sizeof(TElement)-sizeof(CStackEntry), TMEMBLOCK_FILL_DWORD))
    {
        RIP("Block has been used since free'ing.");
    }
#endif

    // Call TElement constructor and store result.
    *ppElement = new(pElement) TElement;

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Method:
//      TMemBlockBase<TElement>::Free
//
//  Synopsis:
//      Return TElement recently obtained by Allocate() to reusable storage.
//
//-------------------------------------------------------------------------
template <class TElement>
void 
TMemBlockBase<TElement>::Free(__out_ecount(1) TElement *pElement)
{
    // Call TElement destructor.
    pElement->~TElement();

#if DBG
    RtlFillMemoryUlong(pElement, sizeof(TElement), TMEMBLOCK_FILL_DWORD);
#endif

    // Hook up to salvage stack.
    CStackEntry *pEntry = reinterpret_cast<CStackEntry*>(pElement);
    pEntry->m_pNext = m_pSalvage;
    m_pSalvage = pEntry;
}


