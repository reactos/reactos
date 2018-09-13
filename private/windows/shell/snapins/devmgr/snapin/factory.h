/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    cfactory.cpp

Abstract:

    Header file for CFactory.cpp

Author:

    William Hsieh (williamh) created

Revision History:


--*/

#ifndef __CFACTORY_H_
#define __CFACTORY_H_


typedef enum tagdDMClassType{
    DM_CLASS_TYPE_SNAPIN = 0,
    DM_CLASS_TYPE_SNAPIN_EXTENSION,
    DM_CLASS_TYPE_SNAPIN_ABOUT,
    DM_CLASS_TYPE_UNKNOWN
}DM_CLASS_TYPE, *PDM_CLASS_TYPE;

class CClassFactory : public IClassFactory
{
public:

    CClassFactory(DM_CLASS_TYPE ClassType)
    : m_Ref(1), m_ClassType(ClassType)
    {}

    STDMETHOD(QueryInterface) (REFIID riid, LPVOID FAR* ppvObj);

    STDMETHOD_(ULONG, AddRef) ();

    STDMETHOD_(ULONG, Release) ();

    STDMETHOD(CreateInstance)(IUnknown* pUnkOuter, REFIID riid, LPVOID* ppvObj);

    STDMETHOD(LockServer)(BOOL fLock);

    static HRESULT GetClassObject(REFCLSID rclsid, REFIID riid, void** ppv);
    static HRESULT RegisterAll();
    static HRESULT UnregisterAll();
    static HRESULT CanUnloadNow(void);
    static  LONG    s_Locks;
    static  LONG    s_Objects;

private:
    ULONG   m_Ref;
    DM_CLASS_TYPE  m_ClassType;
};

#endif // __CFACTORY_H_
