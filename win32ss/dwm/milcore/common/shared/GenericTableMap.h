// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//---------------------------------------------------------------------------------

//
// File: GenericTablMap.h
//
// Description: Encapsulates a map data structure based on an operating system 
//              generic table.
//---------------------------------------------------------------------------------
#pragma once

//---------------------------------------------------------------------------------
MtExtern(CGenericTableMapData)


//+-----------------------------------------------------------------------
//
//  Class:  CGenericTableElement
//
//  Synopsis:  Base element type for use in CGenericTableMap
//
//------------------------------------------------------------------------
template<typename Key>
struct CGenericTableElement
{
    Key m_key;

    CGenericTableElement() : m_key(0){};
    CGenericTableElement(Key key) :  m_key(key) {};
    Key GetKey() {return m_key;};
    void SetKey(Key key) {m_key = key;};
};

//---------------------------------------------------------------------------------
// Class:       CGenericTableMap
//
// Synopsis:    Implements a map data structure, within an opeating system generic 
//              table that maps keys into value elements.  A value element is a 
//              data structure that embeds the key along with any additional needed 
//              value data.
//
//              For example:
//
//              typedef Key key;
//              struct Value
//              {
//                  Key key;
//                  // Value data
//              };
//---------------------------------------------------------------------------------
template<typename Key, typename Element>
class CGenericTableMap : public RTL_GENERIC_TABLE
{
public:
    CGenericTableMap()
    {
        RtlInitializeGenericTable(this, CompareTableData, AllocTableData, FreeTableData, NULL);
    }

    ~CGenericTableMap()
    {
        // Clear out the table.  This is sufficient to clean everything
        // in the list up because inserting into the table allocated
        // the memory for elements.      
        Key *RestartKey = NULL;
        Element *pElement;
        while ((pElement = EnumerateElement(&RestartKey)) != NULL)
        {
            RemoveElement(pElement);

            // RestartKey should go back to the beginning
            RestartKey = NULL;
        }
    }

    Element *EnumerateElement(__deref_opt_inout_ecount_opt(1) Key **ppKey)
    {
        return static_cast<Element *>(RtlEnumerateGenericTableWithoutSplaying(this, reinterpret_cast<LPVOID *>(ppKey)));
    }

    bool EnumerateElement(__deref_opt_inout_ecount_opt(1) Key **ppKey, __deref_out_ecount_opt(1) Element **ppElement)
    {
        *ppElement = static_cast<Element *>(RtlEnumerateGenericTableWithoutSplaying(this, reinterpret_cast<LPVOID *>(ppKey)));

        return *ppElement != NULL;
    }

    Element *FindElement(Key key)
    {
        Element element;
        element.SetKey(key);
            
        return static_cast<Element *>(RtlLookupElementGenericTable(this, &element));
    }

    ULONG GetCount()
    {
        return RtlNumberGenericTableElements(this);
    }

    // A new element is allocated and filled with the passed in data
    Element *InsertElement(__in_ecount(1) Element *pElement, __out_ecount(1) BOOLEAN *pfIsNewElement = NULL)
    {
        return static_cast<Element *>(RtlInsertElementGenericTable(this, pElement, sizeof(Element), pfIsNewElement));
    }

    BOOL IsEmpty()
    { 
        return RtlIsGenericTableEmpty(this);
    }

    BOOL RemoveElement(__in_ecount(1) Element *pElement)
    {
        pElement->~Element();
        return RtlDeleteElementGenericTable(this, pElement);
    }

    BOOL RemoveElement(Key key)
    {
        Element element;
        element.SetKey(key);

        return RemoveElement(&element);
    }

private:
    static LPVOID NTAPI AllocTableData(
        __in_ecount(1) RTL_GENERIC_TABLE * /*pTable*/,
        CLONG cBytes
        )
    {
        return WPFAlloc(ProcessHeap, Mt(CGenericTableMapData), cBytes);
    }

    static VOID NTAPI FreeTableData(
        __in_ecount(1) RTL_GENERIC_TABLE * /*pTable*/,
        LPVOID pBuffer
        )
    {
        WPFFree(ProcessHeap, pBuffer);
    }

    // Default comparison implementation - use template specialization to override
    static RTL_GENERIC_COMPARE_RESULTS NTAPI CompareTableData(
        __in_ecount(1) RTL_GENERIC_TABLE * /*pTable*/,
        __in_xcount(sizeof(Element)) LPVOID pBuffer1,
        __in_xcount(sizeof(Element)) LPVOID pBuffer2
        )
    {
        Key key1 = reinterpret_cast<Element *>(pBuffer1)->GetKey();
        Key key2 = reinterpret_cast<Element *>(pBuffer2)->GetKey();
            
        return ((key1 < key2) ? GenericLessThan : ((key1 == key2) ? GenericEqual : GenericGreaterThan));
    }
};





