//**@@@*@@@****************************************************
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//**@@@*@@@****************************************************

//
// FileName:    irptgt.h
//
// Abstract:    
//      This is the header file for the C++ base class which abstracts
//      common functionality in CKsFilter and CKsPin classes
//
#pragma once
#ifndef __KSIRPTGT_H
#define __KSIRPTGT_H

////////////////////////////////////////////////////////////////////////////////
//
//  class CKsIrpTarget
//
//  Class Description:
//      This is a base class for controlling Ks file objects, i.e. filters and pins\
//      CKsFilter and CKsPin derive from this class
//  



class CKsIrpTarget
{

public:

    // Constructor
    CKsIrpTarget(
            IN  HANDLE handle);

    // Static Utility Functions
    
    static HRESULT SyncIoctl(
            IN  HANDLE  handle,
            IN  ULONG   ulIoctl,
            IN  PVOID   pvInBuffer,
            IN  ULONG   cbInBuffer,
            OUT PVOID   pvOutBuffer,
            OUT ULONG   cbOutBuffer,
            OUT PULONG  pulBytesReturned);

   static BOOL SafeCloseHandle(HANDLE&);
   static BOOL IsValidHandle(HANDLE);
   HANDLE GetHandle(void);
   BOOL Close(void);
       
 // Protected Utility Functions available to derived classes
protected:

    // Protected Data Members
    HANDLE  m_handle;     // allocated handle of the filter instance

    // Support
    HRESULT PropertySetSupport(
        IN REFGUID  guidPropertySet);

    HRESULT PropertyBasicSupport(
        IN  REFGUID guidPropertySet,
        IN  ULONG   nPropertySet,
        OUT PDWORD  pdwSupport);

    // GET
    HRESULT GetPropertySimple(
        IN  REFGUID guidPropertySet,
        IN  ULONG   nProperty,
        OUT PVOID   pvValue,
        IN  ULONG   cbValue,
        OPTIONAL IN PVOID pvInstance = NULL,
        OPTIONAL IN ULONG cbInstance = 0);
    
    HRESULT GetPropertyMulti(
        IN  REFGUID guidPropertySet,
        IN  ULONG   nProperty,
        OUT PKSMULTIPLE_ITEM* ppKsMultipleItem);
        
    HRESULT GetPropertyMulti(
        IN  REFGUID guidPropertySet,
        IN  ULONG   nProperty,
        IN  PVOID   pvData,
        IN  ULONG   cbData,
        OUT PKSMULTIPLE_ITEM* ppksMultipleItem);
            
    HRESULT GetNodePropertySimple(
        IN  ULONG               nNodeID,
        IN  REFGUID             guidPropertySet,
        IN  ULONG               nProperty,
        OUT PVOID               pvValue,
        IN  ULONG               cbValue,
        OPTIONAL IN PVOID       pvInstance = NULL,
        OPTIONAL IN ULONG       cbInstance = 0);

    HRESULT GetNodePropertyChannel(
        IN  ULONG               nNodeID,
        IN  ULONG               nChannel,
        IN  REFGUID             guidPropertySet,
        IN  ULONG               nProperty,
        OUT PVOID               pvValue,
        IN  ULONG               cbValue,
        OPTIONAL IN PVOID       pvInstance = NULL,
        OPTIONAL IN ULONG       cbInstance = 0);

    // Set
    HRESULT SetPropertySimple(
        IN  REFGUID guidPropertySet,
        IN  ULONG   nProperty,
        OUT PVOID   pvValue,
        IN  ULONG   cbValue,
        OPTIONAL IN PVOID pvInstance = NULL,
        OPTIONAL IN ULONG cbInstance = 0);
        
    HRESULT SetPropertyMulti(
        IN  REFGUID guidPropertySet,
        IN  ULONG   nProperty,
        OUT PKSMULTIPLE_ITEM* ppKsMultipleItem);
    
    HRESULT SetNodePropertySimple(
        IN  ULONG               nNodeID,
        IN  REFGUID             guidPropertySet,
        IN  ULONG               nProperty,
        IN  PVOID               pvValue,
        IN  ULONG               cbValue,
        OPTIONAL IN PVOID       pvInstance = NULL,
        OPTIONAL IN ULONG       cbInstance = 0);

    HRESULT SetNodePropertyChannel(
        IN  ULONG               nNodeID,
        IN  ULONG               nChannel,
        IN  REFGUID             guidPropertySet,
        IN  ULONG               nProperty,
        IN  PVOID               pvValue,
        IN  ULONG               cbValue,
        OPTIONAL IN PVOID       pvInstance = NULL,
        OPTIONAL IN ULONG       cbInstance = 0);

};

#endif //__KSIRPTGT_H
