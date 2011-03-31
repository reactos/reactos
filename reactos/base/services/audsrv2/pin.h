//**@@@*@@@****************************************************
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//**@@@*@@@****************************************************

//
// FileName:    pin.h
//
// Abstract:    
//      This is the header file for C++ classes that expose
//      functionality of KS pins 
//

#pragma once
#ifndef __KSPIN_H
#define __KSPIN_H

typedef struct 
{
    ULONG                       cInterfaces;
    PKSIDENTIFIER               pInterfaces;
    ULONG                       cMediums;
    PKSIDENTIFIER               pMediums;
    ULONG                       cDataRanges;
    PKSDATARANGE                pDataRanges;
    ULONG                       cConstrDataRanges;
    PKSDATARANGE                pConstrDataRanges;
    KSPIN_DATAFLOW              DataFlow;
    KSPIN_COMMUNICATION         Communication;
    KSPIN_CINSTANCES            CInstances;
    KSPIN_CINSTANCES            CInstancesGlobal;
    KSPIN_CINSTANCES            CInstancesNecessary;
    PKSPIN_PHYSICALCONNECTION   PhysicalConnection;
    GUID                        Category;
    PWCHAR                      Name;

    // these are redundant.  Free them when you are done with pInterfaces, pMediums, pDataRanges
    PKSMULTIPLE_ITEM            pmiDataRanges;
    PKSMULTIPLE_ITEM            pmiMediums;
    PKSMULTIPLE_ITEM            pmiInterfaces;

} 
PIN_DESCRIPTOR, *PPIN_DESCRIPTOR;

////////////////////////////////////////////////////////////////////////////////
//
//  class CKsPin
//
//  Class Description:
//      This is the base class for classes that proxy Ks filters from user mode.
//      The pin class is still virtual and must be overidden to be useful.
//      specifically, the mpksPinCreate, m_eType need to be propperly
//      initialized in the derived class.
//
//
//

class CKsPin : public CKsIrpTarget
{
public:

    CKsPin(
        IN  CKsFilter* pFilter, 
        IN  ULONG nId,
        OUT HRESULT* phr);

    virtual ~CKsPin(void);

    virtual HRESULT Instantiate(
        IN  BOOL fLooped = FALSE);

    void ClosePin(void);

    HRESULT SetFormat(KSDATAFORMAT* pFormat);

    HRESULT GetState(
        OUT PKSSTATE pksState);

    virtual HRESULT SetState(
        IN KSSTATE ksState);

    HRESULT Reset(void);

    HRESULT WriteData(
        IN  KSSTREAM_HEADER *pKSSTREAM_HEADER,
        IN  OVERLAPPED *pOVERLAPPED);

    HRESULT ReadData(
        IN  KSSTREAM_HEADER *pKSSTREAM_HEADER,
        IN  OVERLAPPED *pOVERLAPPED);

    ULONG GetId();
    ETechnology GetType(void);
    
    PPIN_DESCRIPTOR GetPinDescriptor();

    HRESULT SetPinConnect(PKSPIN_CONNECT pksPinConnect, ULONG cbKsPinConnect);

    HRESULT GetDataFlow(KSPIN_DATAFLOW* pDataFlow);
    HRESULT GetCommunication(KSPIN_COMMUNICATION* pCommunication);
    BOOL HasDataRangeWithSpecifier(REFGUID guidFormatSpecifier);

protected:
    DWORD           m_dwAlignment;
    PKSPIN_CONNECT  m_pksPinCreate;     // creation parameters of pin
    DWORD           m_cbPinCreateSize;  // size of the creation parameters
    ULONG           m_nId;
    CKsFilter*      m_pFilter;
    GUID            m_guidCategory;
    PIN_DESCRIPTOR  m_Descriptor;       // description of Pin
    ETechnology     m_eType;

private:
    HRESULT Init(void);

    // Private Data Members


    KSSTATE         m_ksState;          // state of pin
    TCHAR           m_szFriendlyName[MAX_PATH]; // friendly name of the pin
    BOOL            m_fLooped;
    CKsPin*         m_pLinkPin;         // optional pin to connect to
    BOOL            m_fCompleteOnlyOnRunState;


};

#endif //__KSPIN_H
