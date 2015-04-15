/*
 * Copyright (C) 2006 Dmitry Timoshkov
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define WIN32_NO_STATUS

#include <stdarg.h>

#include <windef.h>
//#include "winbase.h"
//#include "winerror.h"
#include <winuser.h>
#include <ntdsapi.h>
#include <wine/debug.h>
#include <wine/unicode.h>

WINE_DEFAULT_DEBUG_CHANNEL(ntdsapi);

/*****************************************************
 *      DllMain
 */
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
    TRACE("(%p, %d, %p)\n", hinst, reason, reserved);

    switch(reason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;  /* prefer native version */

    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( hinst );
        break;
    }
    return TRUE;
}

/***********************************************************************
 *             DsBindA (NTDSAPI.@)
 */
DWORD WINAPI DsBindA(LPCSTR controller, LPCSTR domain, HANDLE *handle)
 {
    FIXME("(%s,%s, %p): stub!\n", debugstr_a(controller), debugstr_a(domain), handle);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 *             DsBindW (NTDSAPI.@)
 */
DWORD WINAPI DsBindW(LPCWSTR controller, LPCWSTR domain, HANDLE *handle)
 {
    FIXME("(%s,%s, %p): stub!\n", debugstr_w(controller), debugstr_w(domain), handle);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 *             DsMakeSpnW (NTDSAPI.@)
 */
DWORD WINAPI DsMakeSpnW(LPCWSTR svc_class, LPCWSTR svc_name,
                        LPCWSTR inst_name, USHORT inst_port,
                        LPCWSTR ref, DWORD *spn_length, LPWSTR spn)
{
    DWORD new_spn_length;
    INT len;
    LPWSTR p;

    TRACE("(%s,%s,%s,%d,%s,%p,%p)\n", debugstr_w(svc_class),
            debugstr_w(svc_name), debugstr_w(inst_name), inst_port,
            debugstr_w(ref), spn_length, spn);

    if (!svc_class || !svc_name)
        return ERROR_INVALID_PARAMETER;

    new_spn_length = strlenW(svc_class) + 1 /* for '/' */ + 1 /* for terminating '\0' */;
    if (inst_name)
        new_spn_length += strlenW(inst_name);
    else
        new_spn_length += strlenW(svc_name);
    if (inst_port)
    {
        USHORT n = inst_port;
        new_spn_length += 1 /* for ':' */;
        do
        {
            n /= 10;
            new_spn_length++;
        } while (n != 0);
    }
    if (inst_name)
        new_spn_length += 1 /* for '/' */ + strlenW(svc_name);

    if (*spn_length < new_spn_length)
    {
        *spn_length = new_spn_length;
        return ERROR_BUFFER_OVERFLOW;
    }
    *spn_length = new_spn_length;

    p = spn;
    len = strlenW(svc_class);
    memcpy(p, svc_class, len * sizeof(WCHAR));
    p += len;
    *p = '/';
    p++;
    if (inst_name)
    {
        len = strlenW(inst_name);
        memcpy(p, inst_name, len * sizeof(WCHAR));
        p += len;
        *p = '\0';
    }
    else
    {
        len = strlenW(svc_name);
        memcpy(p, svc_name, len * sizeof(WCHAR));
        p += len;
        *p = '\0';
    }

    if (inst_port)
    {
        static const WCHAR percentU[] = {'%','u',0};
        *p = ':';
        p++;
        wsprintfW(p, percentU, inst_port);
        p += strlenW(p);
    }

    if (inst_name)
    {
        *p = '/';
        p++;
        len = strlenW(svc_name);
        memcpy(p, svc_name, len * sizeof(WCHAR));
        p += len;
        *p = '\0';
    }

    TRACE("spn = %s\n", debugstr_w(spn));

    return ERROR_SUCCESS;
}

/***********************************************************************
 *             DsMakeSpnA (NTDSAPI.@)
 *
 * See DsMakeSpnW.
 */
DWORD WINAPI DsMakeSpnA(LPCSTR svc_class, LPCSTR svc_name,
                        LPCSTR inst_name, USHORT inst_port,
                        LPCSTR ref, DWORD *spn_length, LPSTR spn)
{
    FIXME("(%s,%s,%s,%d,%s,%p,%p): stub!\n", debugstr_a(svc_class),
            debugstr_a(svc_name), debugstr_a(inst_name), inst_port,
            debugstr_a(ref), spn_length, spn);

    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 *             DsMakeSpnA (NTDSAPI.@)
 */
DWORD WINAPI DsGetSpnA(DS_SPN_NAME_TYPE ServType, LPCSTR Servlass, LPCSTR ServName,
                       USHORT InstPort, USHORT nInstanceNames,
                       LPCSTR *pInstanceNames, const USHORT *pInstancePorts,
                       DWORD *pSpn, LPSTR **pszSpn)
{
    FIXME("(%d,%s,%s,%d,%d,%p,%p,%p,%p): stub!\n", ServType,
            debugstr_a(Servlass), debugstr_a(ServName), InstPort,
            nInstanceNames, pInstanceNames, pInstancePorts, pSpn, pszSpn);

    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 *             DsServerRegisterSpnA (NTDSAPI.@)
 */
DWORD WINAPI DsServerRegisterSpnA(DS_SPN_WRITE_OP operation, LPCSTR ServiceClass, LPCSTR UserObjectDN)
{
    FIXME("(%d,%s,%s): stub!\n", operation,
            debugstr_a(ServiceClass), debugstr_a(UserObjectDN));
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 *             DsServerRegisterSpnW (NTDSAPI.@)
 */
DWORD WINAPI DsServerRegisterSpnW(DS_SPN_WRITE_OP operation, LPCWSTR ServiceClass, LPCWSTR UserObjectDN)
{
    FIXME("(%d,%s,%s): stub!\n", operation,
            debugstr_w(ServiceClass), debugstr_w(UserObjectDN));
    return ERROR_CALL_NOT_IMPLEMENTED;
}
