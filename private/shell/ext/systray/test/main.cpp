#include <windows.h>
#include <objbase.h>
#include <docobj.h>
#include <initguid.h>
#include "stobject.h"

int __cdecl main()
{
    HRESULT hr;
    IOleCommandTarget* ptgt;

    hr = CoInitialize(NULL);

    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(CLSID_SysTray, NULL, CLSCTX_INPROC_SERVER, 
            IID_IOleCommandTarget, (void**) &ptgt);

        if (SUCCEEDED(hr))
        {
            ptgt->Release();
        }

        CoUninitialize();
    }

    return 0;
}