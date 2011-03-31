//**@@@*@@@****************************************************
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//**@@@*@@@****************************************************

//
// FileName:    filter.h
//
// Abstract:    
//      This is the header file for C++ classes that expose
//      functionality of KS filters
//

#pragma once
#ifndef __KSFILTER_H
#define __KSFILTER_H

// Forward Declarations
class CKsPin;
class CKsNode;

////////////////////////////////////////////////////////////////////////////////
//
//  class CKsFilter
//
//  Class Description:
//      This is the base class for classes that proxy Ks filters from user mode.
//      Basic usage is 
//          instantiate a CKsFilter (or derived class)
//          call Instantiate, which creates a file object (instantiates the KS filter)
//                  whose handle is stored as m_handle
//          call EnumeratePins, EnumerateNodes, to deduce the filter's topology
//          call CKsIrpTarget functions to get/set properties
//
//
//


class CKsFilter : public CKsIrpTarget
{
public:
    CKsFilter(
        IN LPCTSTR pszName,
        OUT HRESULT* phr);

    virtual ~CKsFilter(void);
    virtual HRESULT Instantiate(void);
    virtual HRESULT EnumerateNodes(void);
    virtual HRESULT EnumeratePins(void);

    HRESULT GetPinPropertySimple(
        IN  ULONG   nPinID,
        IN  REFGUID guidPropertySet,
        IN  ULONG   nProperty,
        OUT PVOID   pvValue,
        OUT ULONG   cbValue);

    HRESULT GetPinPropertyMulti(
        IN  ULONG   nPinID,
        IN  REFGUID guidPropertySet,
        IN  ULONG   nProperty,
        OUT PKSMULTIPLE_ITEM* ppKsMultipleItem OPTIONAL);


protected:

    HRESULT ClassifyPins();

     // Lists of nodes
    TList<CKsNode>      m_listNodes;

     // This is the "ROOT LIST" for all pins
    TList<CKsPin>   m_listPins;

    // These lists only contain copies of the pointers in m_listPins.
    // Don't delete the memory that they point
    TList<CKsPin>   m_listRenderSinkPins;
    TList<CKsPin>   m_listRenderSourcePins;
    TList<CKsPin>   m_listCaptureSinkPins;
    TList<CKsPin>   m_listCaptureSourcePins;
    TList<CKsPin>   m_listNoCommPins;
    ETechnology m_eType;


private:
    TCHAR       m_szFilterName[MAX_PATH];   // Filter Path
    
    virtual HRESULT DestroyLists(void);

    HRESULT InternalInit(void);
    
// Friends
friend class CKsPin;
friend class CKsAudPin;
friend class CKsNode;

};

typedef CKsFilter *             PCKsFilter;

#endif //__KSFILTER_H
