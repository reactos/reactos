#include <user32.h>

#include <wine/debug.h>

#ifndef _CFGMGR32_H_
#define CR_SUCCESS                        0x00000000
#define CR_OUT_OF_MEMORY                  0x00000002
#define CR_INVALID_POINTER                0x00000003
#define CR_INVALID_DATA                   0x0000001F
#endif

typedef DWORD (WINAPI *CMP_REGNOTIFY) (HANDLE, LPVOID, DWORD, PULONG);
typedef DWORD (WINAPI *CMP_UNREGNOTIFY) (ULONG );

/* FIXME: Currently IsBadWritePtr is implemented using VirtualQuery which
          does not seem to work properly for stack address space. */
/* kill `left-hand operand of comma expression has no effect' warning */
#define IsBadWritePtr(lp, n) ((DWORD)lp==n?0:0)


static HINSTANCE hSetupApi = NULL;


/*
 * @implemented (Synced with Wine 08.01.2009)
 */
INT
WINAPI
LoadStringA(HINSTANCE instance, UINT resource_id, LPSTR buffer, INT buflen)
{
    HGLOBAL hmem;
    HRSRC hrsrc;
    DWORD retval = 0;

    if (!buflen) return -1;

    /* Use loword (incremented by 1) as resourceid */
    if ((hrsrc = FindResourceW( instance, MAKEINTRESOURCEW((LOWORD(resource_id) >> 4) + 1),
                                (LPWSTR)RT_STRING )) &&
        (hmem = LoadResource( instance, hrsrc )))
    {
        const WCHAR *p = LockResource(hmem);
        unsigned int id = resource_id & 0x000f;

        while (id--) p += *p + 1;

        RtlUnicodeToMultiByteN( buffer, buflen - 1, &retval, (PWSTR)(p + 1), *p * sizeof(WCHAR) );
    }
    buffer[retval] = 0;
    return retval;
}


/*
 * @implemented (Synced with Wine 08.01.2009)
 */
INT
WINAPI
LoadStringW(HINSTANCE instance, UINT resource_id, LPWSTR buffer, INT buflen)
{
    HGLOBAL hmem;
    HRSRC hrsrc;
    WCHAR *p;
    int string_num;
    int i;

    if(buffer == NULL)
        return 0;

    /* Use loword (incremented by 1) as resourceid */
    hrsrc = FindResourceW( instance, MAKEINTRESOURCEW((LOWORD(resource_id) >> 4) + 1),
                           (LPWSTR)RT_STRING );
    if (!hrsrc) return 0;
    hmem = LoadResource( instance, hrsrc );
    if (!hmem) return 0;

    p = LockResource(hmem);
    string_num = resource_id & 0x000f;
    for (i = 0; i < string_num; i++)
    p += *p + 1;

    /*if buflen == 0, then return a read-only pointer to the resource itself in buffer
    it is assumed that buffer is actually a (LPWSTR *) */
    if(buflen == 0)
    {
        *((LPWSTR *)buffer) = p + 1;
        return *p;
    }

    i = min(buflen - 1, *p);
    if (i > 0) {
    memcpy(buffer, p + 1, i * sizeof (WCHAR));
        buffer[i] = 0;
    } else {
    if (buflen > 1) {
            buffer[0] = 0;
        return 0;
    }
    }

    return i;
}


/*
 * @implemented
 */
HDEVNOTIFY
WINAPI
RegisterDeviceNotificationW(HANDLE hRecipient,
                            LPVOID NotificationFilter,
                            DWORD Flags)
{
    DWORD ConfigRet = 0;
    CMP_REGNOTIFY RegNotify = NULL;
    HDEVNOTIFY hDevNotify = NULL;

    if (hSetupApi == NULL) hSetupApi = LoadLibraryA("SETUPAPI.DLL");
    if (hSetupApi == NULL) return NULL;

    RegNotify = (CMP_REGNOTIFY) GetProcAddress(hSetupApi, "CMP_RegisterNotification");
    if (RegNotify == NULL)
    {
        FreeLibrary(hSetupApi);
        hSetupApi = NULL;
        return NULL;
    }

    ConfigRet  = RegNotify(hRecipient, NotificationFilter, Flags, (PULONG) &hDevNotify);
    if (ConfigRet != CR_SUCCESS)
    {
        switch (ConfigRet)
        {
            case CR_OUT_OF_MEMORY:
                SetLastError (ERROR_NOT_ENOUGH_MEMORY);
                break;

            case CR_INVALID_POINTER:
                SetLastError (ERROR_INVALID_PARAMETER);
                break;

            case CR_INVALID_DATA:
                SetLastError (ERROR_INVALID_DATA);
                break;

            default:
                SetLastError (ERROR_SERVICE_SPECIFIC_ERROR);
                break;
        }
    }

    return hDevNotify;
}


/*
 * @implemented
 */
BOOL
WINAPI
UnregisterDeviceNotification(HDEVNOTIFY Handle)
{
    DWORD ConfigRet = 0;
    CMP_UNREGNOTIFY UnRegNotify = NULL;

    if (hSetupApi == NULL) hSetupApi = LoadLibraryA("SETUPAPI.DLL");
    if (hSetupApi == NULL) return FALSE;

    UnRegNotify = (CMP_UNREGNOTIFY) GetProcAddress(hSetupApi, "CMP_UnregisterNotification");
    if (UnRegNotify == NULL)
    {
        FreeLibrary(hSetupApi);
        hSetupApi = NULL;
        return FALSE;
    }

    ConfigRet  = UnRegNotify((ULONG) Handle );
    if (ConfigRet != CR_SUCCESS)
    {
        switch (ConfigRet)
        {
            case CR_INVALID_POINTER:
                SetLastError (ERROR_INVALID_PARAMETER);
                break;

            case CR_INVALID_DATA:
                SetLastError (ERROR_INVALID_DATA);
                break;

            default:
                SetLastError (ERROR_SERVICE_SPECIFIC_ERROR);
                break;
        }
        return FALSE;
    }

    return TRUE;
}

/* EOF */
