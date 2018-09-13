// Provider.cpp : Implementation of CVsengineApp and DLL registration.

#include "stdafx.h"
#include "vsengine.h"
#include "vrsscan.h"
#include "Provider.h"
#include "util.h"

/////////////////////////////////////////////////////////////////////////////
//

Provider::Provider()
{
}

STDMETHODIMP Provider::ScanForVirus(HWND hWnd, STGMEDIUM *pstgMedium,
    LPWSTR pwszItemDescription, DWORD dwFlags, DWORD dwReserved, LPVIRUSINFO pvrsinfo)
{
   if(MessageBox(NULL, TEXT("Do you want to report a virus?"), TEXT("Provider 1"), MB_YESNO | MB_ICONQUESTION | MB_APPLMODAL) == IDYES)
   {
       if(pvrsinfo != NULL)
       {
          CopyWideStr((pvrsinfo)->wszVirusName, L"Ebola Virus");
          CopyWideStr((pvrsinfo)->wszVirusDescription, L"A nasty little virus");
       }
       return S_FALSE;
   }
   else
      return S_OK;
}

STDMETHODIMP Provider::DisplayCustomInfo(void) 
{
    MessageBox(NULL, TEXT("DummyVirus Scanner V0.0.0"), TEXT("DummyScanner"), MB_OK);
    return S_OK;
}


