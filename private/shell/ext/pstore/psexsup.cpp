#include <windows.h>
#include <objbase.h>

#include "psexsup.h"

#include "pstore.h"


BOOL
InitializePStoreSupport(
    VOID
    )
{
    HRESULT hr;

    hr = CoInitialize(NULL);

    //
    // since explorer is likely to have init'ed OLE already, treat that
    // case as success
    //

    if(hr != S_OK && hr != S_FALSE)
        return FALSE;

    return TRUE;
}

BOOL
ShutdownPStoreSupport(
    VOID
    )
{
    CoUninitialize();

    return TRUE;
}

