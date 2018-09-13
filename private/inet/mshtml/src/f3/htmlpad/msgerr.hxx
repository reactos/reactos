/////////////////////////////////////////////////////////////////
//  LASTERR.H
//
// Copyright 1986-1996 Microsoft Corporation. All Rights Reserved.
//
//
// last err support object.
//

//There should be no global objects of this class
// because by the time the dectructor of a global CLastError
// is called, MAPIFreeBuffer does not work.

#ifndef __LASTERR_H__
#define __LASTERR_H__

class CLastError
{
public:
    ~CLastError(void);

    HRESULT     Init(LPCTSTR szComponent);

    // standard OLE or MAPI errors.
    HRESULT     SetLastError(HRESULT hr);

    // our internal extended error codes or a non-standard string for OLE or MAPI errors
    // scFORM is one of the errors defined by MAKE_FOR_X_SCODE macro family
    HRESULT     SetLastError(HRESULT hr, SCODE scFORM, ...);

    // errors returned from underlying objects.
    HRESULT     SetLastError(HRESULT hr, IUnknown* punk);

    // our implementation of GetLastError
    HRESULT     GetLastError(HRESULT hr, DWORD dwFlags,
                               LPMAPIERROR * lppMAPIError);
    
    //displays the last error info
    int         ShowError(HWND);


private:
    // we have three possible error types: our internal errors which
    //  we signify by MAPI_E_EXTENDED to the user, standard errors
    //  defined by MAPI and errors returned by objects we keep and utilize.

    enum {eNoError, eExtended, eMAPI, eObject} _eLastErr;

    HRESULT     _hrLast;

    HRESULT     _hrGLE;  // what GetLastError on the object returned; mostly 0
    LPMAPIERROR _pmapierr;
    LPSTR       _szComponent;
};

#endif // __LASTERR_H__
