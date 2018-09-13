//  OLESTUFF.CPP
//  Implementation of OLE delay-loaded stuff. This is needed for the
//  classes defined in TBDROP.CPP and TABDROP.CPP.
//	More OLE stuff could go here in the future.
//	==
//	Technically this code is not C++ however, both files that call us
//	are C++, and this is where we call for the C++ glue in crtfree.h.
//	So for now this remains a .CPP file.
//
//  History:
//      8/22/96 -   t-mkim: created
//
#include "ctlspriv.h"
#include "olestuff.h"

// Allow C++ files to be linked in w/o error
#define CPP_FUNCTIONS
#include <crtfree.h>

#define OLELIBNAME  TEXT ("OLE32.DLL")

// function pointers for GetProcAddress.
typedef HRESULT (STDAPICALLTYPE *LPFNCOINITIALIZE)(LPMALLOC pMalloc);
typedef void    (STDAPICALLTYPE *LPFNCOUNINITIALIZE)(void);
typedef HRESULT (STDAPICALLTYPE *LPFNREGISTERDRAGDROP)(HWND hwnd, LPDROPTARGET pDropTarget);
typedef HRESULT (STDAPICALLTYPE *LPFNREVOKEDRAGDROP)(HWND hwnd);

HMODULE PrivLoadOleLibrary ()
{
    // We call GetModuleHandle first so we don't map the library if we don't
    // need to. We would like to avoid the overhead necessary to do so.
    return GetModuleHandle(OLELIBNAME) ? LoadLibrary (OLELIBNAME) : NULL;
}

BOOL PrivFreeOleLibrary(HMODULE hmodOle)
{
    return FreeLibrary(hmodOle);
}

HRESULT PrivCoInitialize (HMODULE hmodOle)
{
    LPFNCOINITIALIZE pfnCoInitialize = (LPFNCOINITIALIZE) GetProcAddress (hmodOle, "CoInitialize");
    return pfnCoInitialize (NULL);
}

void PrivCoUninitialize (HMODULE hmodOle)
{
    LPFNCOUNINITIALIZE pfnCoUninitialize = (LPFNCOUNINITIALIZE) GetProcAddress (hmodOle, "CoUninitialize");
    pfnCoUninitialize ();
}

HRESULT PrivRegisterDragDrop (HMODULE hmodOle, HWND hwnd, IDropTarget *pDropTarget)
{
    LPFNREGISTERDRAGDROP pfnRegisterDragDrop = (LPFNREGISTERDRAGDROP) GetProcAddress (hmodOle, "RegisterDragDrop");
    return pfnRegisterDragDrop(hwnd, pDropTarget);
}

HRESULT PrivRevokeDragDrop (HMODULE hmodOle, HWND hwnd)
{
    LPFNREVOKEDRAGDROP pfnRevokeDragDrop = (LPFNREVOKEDRAGDROP) GetProcAddress (hmodOle, "RevokeDragDrop");
    return pfnRevokeDragDrop (hwnd);
}
