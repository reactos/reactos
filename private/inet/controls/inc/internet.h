//=--------------------------------------------------------------------------=
// Internet.H
//=--------------------------------------------------------------------------=
// Copyright 1995-1996 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// class declaration for the URLDib2 control.
//
#ifndef _INTERNET_H
#define _INTERNET_H

#define DISPID_PROGRESS 1958

#ifndef __MKTYPLIB__

#include "urlmon.H"
#include "ocidl.h"
#include "datapath.h" // for IBindHost
#include "docobj.h"   // for IServiceProvider

#include "IPServer.H"
#include "CtrlObj.H"



class CInternetControl : public COleControl
{
public:
	CInternetControl(IUnknown *     pUnkOuter, 
					int                     iPrimaryDispatch, 
					void *          pMainInterface);

	virtual ~CInternetControl();

    // IUnknown methods
    //
    DECLARE_STANDARD_UNKNOWN();

	// Call this method to start the download of a URL. 'propId' will 
	//      be passed back to you OnData below.
	HRESULT SetupDownload( LPOLESTR url, DISPID propId);


	//      Derived classes implement this method. It will be called when
	//      data has arrived for a given dispid.
	virtual HRESULT OnData( DISPID id, DWORD grfBSCF,
					IStream * bitstrm, DWORD amount );


	//      Derived classes can implement this method. It will be
	//      called at various times during the download.
	virtual HRESULT OnProgress( DISPID id, ULONG ulProgress,
					ULONG ulProgressMax,
					ULONG ulStatusCode,
					LPCWSTR pwzStatusText);

	//      Call this method to turn a URL into a Moniker.
	HRESULT GetAMoniker( LPOLESTR   url, IMoniker ** );


    HRESULT FireReadyStateChange( long newState );
	HRESULT FireProgress( ULONG dwAmount );


	// Override base class implementation...

    virtual HRESULT InternalQueryInterface(REFIID, void **);

protected:
	HRESULT GetBindHost();

    IBindHost *             m_host;
    long                    m_readyState;

    // BUGBUG: We should track all the downloads

};

#endif __MKTYPLIB__

#endif _INTERNET_H
