/*
 * @(#)safectrl.hxx 1.0 8/14/98
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */
 
#ifndef _SAFECTRL_HXX
#define _SAFECTRL_HXX

#ifndef _XML_OM_IOBJECTWITHSITE
#include "xml/om/iobjectwithsite.hxx"
#endif

#ifndef _XML_OM_IOBJECTSAFETY
#include "xml/om/iobjectsafety.hxx"
#endif

// for security checking
#include <mshtml.h>
#include <docobj.h>

DEFINE_STRUCT(IUnknown);

typedef _gitpointer<IUnknown,&IID_IUnknown> GITUnknown;

// This class can be used as a base by any class that needs to act as a safe
// control.  It implements IObjectWithSite and IObjectSafety.

class NOVTABLE CSafeControl:
                    public GenericBase,
                    public ObjectWithSite,
                    public ObjectSafety
{
    DECLARE_CLASS_MEMBERS_I2(CSafeControl, GenericBase,
                                        ObjectWithSite, ObjectSafety);
    DECLARE_CLASS_INSTANCE(CSafeControl, GenericBase);

public:

    // ObjectWithSite interface.

    virtual void setSite(IUnknown* u);
    virtual void getSite(REFIID riid, void** pUnk);

    void SetBaseURLFromSite(IUnknown* pSite);
    HRESULT SetSecureBaseURLFromSite(IUnknown* pSite); // can be different from base URL

    // ObjectSafety interface.

    virtual void getInterfaceSafetyOptions(REFIID riid, DWORD* dwOptionSetMask, DWORD* dwEnabledOptions);
    virtual void setInterfaceSafetyOptions(REFIID riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions);

    String*     GetBaseURL() { return _pBaseURL; }
    String*     GetSecureBaseURL() { return _pSecureBaseURL; }

    bool isSecure() const { return (_dwSafetyOptions != 0); }

protected:

    // general purpose stuff
    CSafeControl();
    virtual void    CloneBase(void *pvClone);   // arg should be (CSafeControl*)
    virtual void    finalize();

    // get base URL for resolving relative URL's.
    void        getBaseURL();

    // get base URL for use in security checking (may be different)
    void        getSecureBaseURL();

    // for IObjectWithSite
    GITUnknown  _pSite;
    RString     _pBaseURL; 
    RString     _pSecureBaseURL;

    // for IObjectSafety
    DWORD       _dwSafetyOptions;
};

#endif _SAFECTRL_HXX

