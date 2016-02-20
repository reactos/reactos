#include <windows.h>
#include <stdio.h>

DWORD debug_shift = 0;

#define INC_SHIFT ++debug_shift;
#define DEC_SHIFT --debug_shift;
#define PRT_SHIFT do { DWORD cur = 0; for (; cur < debug_shift; ++cur) printf("\t"); } while (0);

void np_enum(NETRESOURCEW * resource)
{
    DWORD ret;
    HANDLE handle;
    DWORD size = 0x1000;
    NETRESOURCEW * out;
    BOOL check = FALSE;

    if (resource && resource->lpRemoteName)
        check = TRUE;

    ret = WNetOpenEnum(RESOURCE_GLOBALNET, RESOURCETYPE_DISK, 0, resource, &handle);
    if (ret != WN_SUCCESS)
        return;

    out = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
    if (!out)
    {
        WNetCloseEnum(handle);
        return;
    }

    INC_SHIFT

    if (check)
    {
        printf("Called with lpRemoteName not null, current value: %S\n", resource->lpRemoteName);
    }

    do
    {
        DWORD count = -1;

        ret = WNetEnumResource(handle, &count, out, &size);
        if (ret == WN_SUCCESS || ret == WN_MORE_DATA)
        {
            NETRESOURCEW * current;

            current = out;
            for (; count; count--)
            {
                PRT_SHIFT;
                printf("lpRemoteName: %S\n", current->lpRemoteName);

                if ((current->dwUsage & RESOURCEUSAGE_CONTAINER) == RESOURCEUSAGE_CONTAINER)
                {
                    PRT_SHIFT;
                    printf("Found provider: %S\n", current->lpProvider);
                    np_enum(current);
                }

                current++;
            }
        }
    } while (ret != WN_NO_MORE_ENTRIES);
    DEC_SHIFT;

    HeapFree(GetProcessHeap(), 0, out);
    WNetCloseEnum(handle);
}

int wmain(int argc, const WCHAR *argv[])
{
    np_enum(NULL);

    return 0;
}
