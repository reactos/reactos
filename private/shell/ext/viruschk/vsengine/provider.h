// Provider.h: Definition of the Provider class
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PROVIDER_H__68E721E1_CD58_11D0_BD3D_00AA00B92AF1__INCLUDED_)
#define AFX_PROVIDER_H__68E721E1_CD58_11D0_BD3D_00AA00B92AF1__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// Provider

class Provider : 
	public IVirusScanEngine,
	public CComObjectRoot,
	public CComCoClass<Provider,&CLSID_Provider>
{
public:
	Provider();

BEGIN_COM_MAP(Provider)
	COM_INTERFACE_ENTRY(IVirusScanEngine)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(Provider) 

//DECLARE_NO_REGISTRY()

DECLARE_REGISTRY_RESOURCEID(IDR_Provider)

      /* IVirusScanEngine methods */
      STDMETHODIMP ScanForVirus(HWND hWnd, STGMEDIUM *pstgMedium,
                                LPWSTR pwszItemDescription, DWORD dwFlags,
                                DWORD dwReserved, LPVIRUSINFO pVirusInfo);

      STDMETHODIMP DisplayCustomInfo();

// IVirusScanEngine
public:
};

#endif // !defined(AFX_PROVIDER_H__68E721E1_CD58_11D0_BD3D_00AA00B92AF1__INCLUDED_)
