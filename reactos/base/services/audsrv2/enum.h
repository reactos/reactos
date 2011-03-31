//**@@@*@@@****************************************************
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//**@@@*@@@****************************************************

//
// FileName:    enum.h
//
// Abstract:    
//      This is the header file for the C++ class which encapsulates
//      the setupDi functions needed to enumerate KS Filters
//

#pragma once
#ifndef __KSENUM_H
#define __KSENUM_H

// Forward Declarations
class CKsFilter;
struct _SP_DEVICE_INTERFACE_DATA;
struct _SP_DEVINFO_DATA;

////////////////////////////////////////////////////////////////////////////////
//
//  class CKsEnumFilters
//
//  Class Description:
//      This is a utility class used for enumerating KS filters.
//      Basic usage is 
//          instantiate a CKsEnumFilters
//          call EnumFilters, which enumerates a list of filters
//          pick a filter to use from m_listFilters
//          either call RemoveFilterFromList to remove the filter from the list,
//          or duplicate the filter and use the duplicate copy.
//
//
//


class CKsEnumFilters
{
public:
    // This is the list of the filters
    TList<CKsFilter> m_listFilters;

public:
    CKsEnumFilters(
        OUT HRESULT* phr);

    virtual ~CKsEnumFilters(void);

    virtual HRESULT DestroyLists(void);   

    HRESULT EnumFilters(   
        IN  ETechnology eFilterType,
        IN  GUID*   aguidCategories,
        IN  ULONG   cguidCategories,
        IN  BOOL    fNeedPins,          // = TRUE // Enumerates devices for sysaudio.
        IN  BOOL    fNeedNodes,         // = TRUE
        IN  BOOL    fInstantiate);      // = TRUE // Should we instantiate.

    HRESULT RemoveFilterFromList(
        IN  CKsFilter* pKsFilter);
};

#endif //__KSENUM_H
