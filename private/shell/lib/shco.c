#include "proj.h"

// Need to put into separate file because for some reason the /Gy compiler
// option doesn't work.

//
//  COM Initialization is weird due to multithreaded apartments.
//
//  If this thread has not called CoInitialize yet, but some other thread
//  in the process has called CoInitialize with the COINIT_MULTITHREADED,
//  then that infects our thread with the multithreaded virus, and a
//  COINIT_APARTMENTTHREADED will fail.
//
//  In this case, we must turn around and re-init ourselves as
//  COINIT_MULTITHREADED to increment the COM refcount on our thread.
//  If we didn't do that, and that other thread decided to do a
//  CoUninitialize, that >secretly< uninitializes COM on our own thread
//  and we fall over and die.
//
STDAPI SHCoInitialize(void)
{
    HRESULT hres;

    hres = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hres)) {
        hres = CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE);
    }
    return hres;
}
