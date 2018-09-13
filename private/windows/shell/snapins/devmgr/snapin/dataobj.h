/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    dataobj.h

Abstract:

    header file defines CDataObject class

Author:

    William Hsieh (williamh) created

Revision History:


--*/

#ifndef _DATAOBJ_H
#define _DATAOBJ_H


class CDataObject;
class CDataObjectRoot;
class CDataObjectDeviceTreeByType;
class CDataObjectDeviceTreeByConnection;
class CCookie;


class CDataObject : public IDataObject
{
public:
    CDataObject() : m_Ref(1)
    {}
    ~CDataObject()
    {}

// IUnknown interface
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    STDMETHOD(QueryInterface)(REFIID riid, void** ppv);

//  IDataObject interface
    STDMETHOD(GetData)(LPFORMATETC lpFormatetcIn, LPSTGMEDIUM lpMedium);
    STDMETHOD(GetDataHere)(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium);
    STDMETHOD(EnumFormatEtc)(DWORD dwDirection, LPENUMFORMATETC* ppEnumFormatEtc);
    virtual HRESULT Initialize(DATA_OBJECT_TYPES Type, COOKIE_TYPE ct, CCookie* pCookie, String& strMachineName);

// Not Implemented
private:
    STDMETHOD(QueryGetData)(LPFORMATETC lpFormatetc) 
    { return E_NOTIMPL; };

    STDMETHOD(GetCanonicalFormatEtc)(LPFORMATETC lpFormatetcIn, LPFORMATETC lpFormatetcOut)
    { return E_NOTIMPL; };

    STDMETHOD(SetData)(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium, BOOL bRelease)
    { return E_NOTIMPL; };

    STDMETHOD(DAdvise)(LPFORMATETC lpFormatetc, DWORD advf,
                LPADVISESINK pAdvSink, LPDWORD pdwConnection)
    { return E_NOTIMPL; };
    
    STDMETHOD(DUnadvise)(DWORD dwConnection)
    { return E_NOTIMPL; };

    STDMETHOD(EnumDAdvise)(LPENUMSTATDATA* ppEnumAdvise)
    { return E_NOTIMPL; };


public:

// Clipboard formats that are required by the console
    static unsigned int    m_cfNodeType;
    static unsigned int    m_cfNodeTypeString;  
    static unsigned int    m_cfDisplayName;
    static unsigned int    m_cfSnapinInternal;
    static unsigned int    m_cfCoClass;
    static unsigned int    m_cfMachineName;
    static unsigned int    m_cfClassGuid;
    static unsigned int    m_cfDeviceID;
protected:
    COOKIE_TYPE        m_ct;
    CCookie*           m_pCookie;
    int            m_idsNodeName;
private:
    DATA_OBJECT_TYPES      m_Type;
    String         m_strMachineName;
    ULONG          m_Ref;
};

extern HRESULT ExtractData(IDataObject* pIDataObject, unsigned int cfClipFormat,
               BYTE* pBuffer, DWORD cbBuffer
               );
#endif
