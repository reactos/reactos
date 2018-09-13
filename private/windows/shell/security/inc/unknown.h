//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       unknown.h
//
//--------------------------------------------------------------------------

#ifndef _unknown_h
#define _unknown_h


// Global count of number of active objects

extern LONG g_cRefCount;
#define GLOBAL_REFCOUNT     (g_cRefCount)


// CUnknown

typedef struct
{
    const IID* piid;            // interface ID
    LPVOID  pvObject;           // pointer to the object
} INTERFACES, * LPINTERFACES;

class CUnknown 
{
    protected:
        LONG m_cRefCount;

    public:
        CUnknown();
        virtual ~CUnknown();
        
        STDMETHODIMP         HandleQueryInterface(REFIID riid, LPVOID* ppvObject, LPINTERFACES aInterfaces, int cif);
        STDMETHODIMP_(ULONG) HandleAddRef();
        STDMETHODIMP_(ULONG) HandleRelease();
};


#endif
