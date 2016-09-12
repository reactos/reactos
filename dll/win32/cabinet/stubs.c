#include <windef.h>
#include <reactos/debug.h>

typedef PVOID PSESSION;

VOID
WINAPI
DeleteExtractedFiles(
    PSESSION ps)
{
    UNIMPLEMENTED;
}

LPCSTR
WINAPI
GetDllVersion(void)
{
    /* This is what it returns on Windows XP SP3 */
    return "5.1.2600.5512";
}
