//=--------------------------------------------------------------------------=
// AutoObj.H
//=--------------------------------------------------------------------------=
// Copyright 1995-1996 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// all of our objects will inherit from this class to share as much of the same
// code as possible.  this super-class contains the unknown and dispatch
// implementations for them.
//
#ifndef _AUTOOBJ_H_

// all automation objects will use the Unknown object that supports aggegation.
//
#include "Unknown.H"

//=--------------------------------------------------------------------------=
// the constants in this header file uniquely identify your automation objects.
// make sure that for each object you have in the g_ObjectInfo table, you have
// a constant in this header file.
//
#include "LocalSrv.H"
#include "extobj.h"

//=--------------------------------------------------------------------------=
// AUTOMATIONOBJECTINFO
//=--------------------------------------------------------------------------=
// for each automation object type you wish to expose to the programmer/user
// that is not a control, you must fill out one of these structures.  if the
// object isn't CoCreatable, then the first four fields should be empty.
// otherwise, they should be filled in with the appropriate information.
// use the macro DEFINE_AUTOMATIONOBJECT to both declare and define your object.
// make sure you have an entry in the global table of objects, g_ObjectInfo
// in the main .Cpp file for your InProc server.
//
typedef struct {

    UNKNOWNOBJECTINFO unknowninfo;               // fill in with 0's if we're not CoCreatable
    long         lVersion;                       // Version number of Object.  ONLY USE IF YOU'RE CoCreatable!
    const IID   *riid;                           // object's type
    LPCSTR       pszHelpFile;                    // the helpfile for this automation object.
    ITypeInfo   *pTypeInfo;                      // typeinfo for this object
    UINT         cTypeInfo;                      // number of refs to the type info

} AUTOMATIONOBJECTINFO;

// macros to manipulate the AUTOMATIONOBJECTINFO in the global table table.
//
#define VERSIONOFOBJECT(index)         ((AUTOMATIONOBJECTINFO *)(g_ObjectInfo[(index)]).pInfo)->lVersion
#define INTERFACEOFOBJECT(index)       *(((AUTOMATIONOBJECTINFO *)(g_ObjectInfo[(index)]).pInfo)->riid)
#define PPTYPEINFOOFOBJECT(index)      &((((AUTOMATIONOBJECTINFO *)(g_ObjectInfo[(index)]).pInfo)->pTypeInfo))
#define PTYPEINFOOFOBJECT(index)       ((AUTOMATIONOBJECTINFO *)(g_ObjectInfo[(index)]).pInfo)->pTypeInfo
#define CTYPEINFOOFOBJECT(index)       ((AUTOMATIONOBJECTINFO *)(g_ObjectInfo[(index)]).pInfo)->cTypeInfo
#define HELPFILEOFOBJECT(index)        ((AUTOMATIONOBJECTINFO *)(g_ObjectInfo[(index)]).pInfo)->pszHelpFile


#ifndef INITOBJECTS

#define DEFINE_AUTOMATIONOBJECT(name, clsid, objname, fn, ver, riid, pszh) \
extern AUTOMATIONOBJECTINFO name##Object \

#else
#define DEFINE_AUTOMATIONOBJECT(name, clsid, objname, fn, ver, riid, pszh) \
    AUTOMATIONOBJECTINFO name##Object = { { clsid, objname, fn }, ver, riid, pszh, NULL, 0} \

#endif // INITOBJECTS

//=--------------------------------------------------------------------------=
// Standard Dispatch and SupportErrorInfo
//=--------------------------------------------------------------------------=
// all objects should declare these in their class definitions so that they
// get standard implementations of IDispatch and ISupportErrorInfo.
//
#define DECLARE_STANDARD_DISPATCH() \
    STDMETHOD(GetTypeInfoCount)(UINT *pctinfo) { \
        return CAutomationObject::GetTypeInfoCount(pctinfo); \
    } \
    STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo **ppTypeInfoOut) { \
        return CAutomationObject::GetTypeInfo(itinfo, lcid, ppTypeInfoOut); \
    } \
    STDMETHOD(GetIDsOfNames)(REFIID riid, OLECHAR **rgszNames, UINT cnames, LCID lcid, DISPID *rgdispid) { \
        return CAutomationObject::GetIDsOfNames(riid, rgszNames, cnames, lcid, rgdispid); \
    } \
    STDMETHOD(Invoke)(DISPID dispid, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pdispparams, VARIANT *pVarResult, EXCEPINFO *pexcepinfo, UINT *puArgErr) { \
        return CAutomationObject::Invoke(dispid, riid, lcid, wFlags, pdispparams, pVarResult, pexcepinfo, puArgErr); \
    } \


#define DECLARE_STANDARD_SUPPORTERRORINFO() \
    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid) { \
        return CAutomationObject::InterfaceSupportsErrorInfo(riid); \
    } \

enum {EXPANDO_DISABLED=FALSE, EXPANDO_ENABLED=TRUE};

//=--------------------------------------------------------------------------=
// CAutomationObject
//=--------------------------------------------------------------------------=
// global class that all automation objects can inherit from to give them a
// bunch of implementation for free, namely IDispatch and ISupportsErrorInfo
//
//
class CAutomationObject : public CUnknownObject {

  public:
    // aggreation query interface support
    //
    virtual HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);

    // IDispatch methods
    //
    STDMETHOD(GetTypeInfoCount)(UINT *);
    STDMETHOD(GetTypeInfo)(UINT, LCID, ITypeInfo **);
    STDMETHOD(GetIDsOfNames)(REFIID, OLECHAR **, UINT, LCID, DISPID *);
    STDMETHOD(Invoke)(DISPID, REFIID, LCID, WORD, DISPPARAMS *, VARIANT *, EXCEPINFO *, UINT *);

    //  ISupportErrorInfo methods
    //
    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID);

    CAutomationObject(IUnknown *, int , void *, BOOL fExpandoEnabled=FALSE);
    virtual ~CAutomationObject();

    // callable functions -- things that most people will find useful.
    //
    virtual HINSTANCE GetResourceHandle(void);
    HRESULT Exception(HRESULT hr, WORD idException, DWORD dwHelpContextID);

  protected:
    // member variables that derived objects might need to get at information in the
    // global object table
    //
    int   m_ObjectType;

  private:
    // member variables we don't share.
    //
    BYTE  m_fLoadedTypeInfo;
	BYTE  m_fExpandoEnabled;
	CExpandoObject* m_pexpando;
};


#define _AUTOOBJ_H_
#endif // _AUTOOBJ_H_



