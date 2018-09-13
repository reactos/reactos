#define INITGUID

#include "mwversion.h"
#include <compobj.h>
#include <initguid.h>
#include <msstkppg.h>

#include <shlguid.h>

#undef INITGUID
#include "unix/guids.h"

// Must be after "unix/guids.h" to expand the GUIDs for olectl.h.
#define INITGUID

// DllMain needed due to this being a shared library

extern "C" BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID)
{
    return TRUE;
}
