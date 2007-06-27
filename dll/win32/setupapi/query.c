/*
 * setupapi query functions
 *
 * Copyright 2006 James Hawkins
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

#include "setupapi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(setupapi);

/* fills the PSP_INF_INFORMATION struct fill_info is TRUE
 * always returns the required size of the information
 */
static BOOL fill_inf_info(HINF inf, PSP_INF_INFORMATION buffer, DWORD size, DWORD *required)
{
    LPCWSTR filename = PARSER_get_inf_filename(inf);
    DWORD total_size = FIELD_OFFSET(SP_INF_INFORMATION, VersionData)
                        + (lstrlenW(filename) + 1) * sizeof(WCHAR);

    if (required) *required = total_size;

    /* FIXME: we need to parse the INF file to find the correct version info */
    if (buffer)
    {
        if (size < total_size)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }
        buffer->InfStyle = INF_STYLE_WIN4;
        buffer->InfCount = 1;
        /* put the filename in buffer->VersionData */
        lstrcpyW((LPWSTR)&buffer->VersionData[0], filename);
    }
    return TRUE;
}

static HINF search_for_inf(LPCVOID InfSpec, DWORD SearchControl)
{
    HINF hInf = INVALID_HANDLE_VALUE;
    WCHAR inf_path[MAX_PATH];

    static const WCHAR infW[] = {'\\','i','n','f','\\',0};
    static const WCHAR system32W[] = {'\\','s','y','s','t','e','m','3','2','\\',0};

    if (SearchControl == INFINFO_REVERSE_DEFAULT_SEARCH)
    {
        GetWindowsDirectoryW(inf_path, MAX_PATH);
        lstrcatW(inf_path, system32W);
        lstrcatW(inf_path, InfSpec);

        hInf = SetupOpenInfFileW(inf_path, NULL,
                                 INF_STYLE_OLDNT | INF_STYLE_WIN4, NULL);
        if (hInf != INVALID_HANDLE_VALUE)
            return hInf;

        GetWindowsDirectoryW(inf_path, MAX_PATH);
        lstrcpyW(inf_path, infW);
        lstrcatW(inf_path, InfSpec);

        return SetupOpenInfFileW(inf_path, NULL,
                                 INF_STYLE_OLDNT | INF_STYLE_WIN4, NULL);
    }

    return INVALID_HANDLE_VALUE;
}

/***********************************************************************
 *      SetupGetInfInformationA    (SETUPAPI.@)
 *
 */
BOOL WINAPI SetupGetInfInformationA(LPCVOID InfSpec, DWORD SearchControl,
                                    PSP_INF_INFORMATION ReturnBuffer,
                                    DWORD ReturnBufferSize, PDWORD RequiredSize)
{
    LPWSTR inf = (LPWSTR)InfSpec;
    DWORD len;
    BOOL ret;

    if (InfSpec && SearchControl >= INFINFO_INF_NAME_IS_ABSOLUTE)
    {
        len = lstrlenA(InfSpec) + 1;
        inf = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, InfSpec, -1, inf, len);
    }

    ret = SetupGetInfInformationW(inf, SearchControl, ReturnBuffer,
                                  ReturnBufferSize, RequiredSize);

    if (SearchControl >= INFINFO_INF_NAME_IS_ABSOLUTE)
        HeapFree(GetProcessHeap(), 0, inf);

    return ret;
}

/***********************************************************************
 *      SetupGetInfInformationW    (SETUPAPI.@)
 * 
 * BUGS
 *   Only handles the case when InfSpec is an INF handle.
 */
BOOL WINAPI SetupGetInfInformationW(LPCVOID InfSpec, DWORD SearchControl,
                                     PSP_INF_INFORMATION ReturnBuffer,
                                     DWORD ReturnBufferSize, PDWORD RequiredSize)
{
    HINF inf;
    BOOL ret;
    DWORD infSize;

    TRACE("(%p, %ld, %p, %ld, %p)\n", InfSpec, SearchControl, ReturnBuffer,
           ReturnBufferSize, RequiredSize);

    if (!InfSpec)
    {
        if (SearchControl == INFINFO_INF_SPEC_IS_HINF)
            SetLastError(ERROR_INVALID_HANDLE);
        else
            SetLastError(ERROR_INVALID_PARAMETER);

        return FALSE;
    }

    switch (SearchControl)
    {
        case INFINFO_INF_SPEC_IS_HINF:
            inf = (HINF)InfSpec;
            break;
        case INFINFO_INF_NAME_IS_ABSOLUTE:
        case INFINFO_DEFAULT_SEARCH:
            inf = SetupOpenInfFileW(InfSpec, NULL,
                                    INF_STYLE_OLDNT | INF_STYLE_WIN4, NULL);
            break;
        case INFINFO_REVERSE_DEFAULT_SEARCH:
            inf = search_for_inf(InfSpec, SearchControl);
            break;
        case INFINFO_INF_PATH_LIST_SEARCH:
            FIXME("Unhandled search control: %ld\n", SearchControl);

            if (RequiredSize)
                *RequiredSize = 0;

            return FALSE;
        default:
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
    }

    if (inf == INVALID_HANDLE_VALUE)
    {
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    ret = fill_inf_info(inf, ReturnBuffer, ReturnBufferSize, &infSize);
    if (!ReturnBuffer && (ReturnBufferSize >= infSize))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        ret = FALSE;
    }
    if (RequiredSize) *RequiredSize = infSize;

    if (SearchControl >= INFINFO_INF_NAME_IS_ABSOLUTE)
        SetupCloseInfFile(inf);

    return ret;
}

/***********************************************************************
 *      SetupQueryInfFileInformationA    (SETUPAPI.@)
 */
BOOL WINAPI SetupQueryInfFileInformationA(PSP_INF_INFORMATION InfInformation,
                                          UINT InfIndex, PSTR ReturnBuffer,
                                          DWORD ReturnBufferSize, PDWORD RequiredSize)
{
    LPWSTR filenameW;
    DWORD size;
    BOOL ret;

    ret = SetupQueryInfFileInformationW(InfInformation, InfIndex, NULL, 0, &size);
    if (!ret)
        return FALSE;

    filenameW = HeapAlloc(GetProcessHeap(), 0, size * sizeof(WCHAR));

    ret = SetupQueryInfFileInformationW(InfInformation, InfIndex,
                                        filenameW, size, &size);
    if (!ret)
    {
        HeapFree(GetProcessHeap(), 0, filenameW);
        return FALSE;
    }

    if (RequiredSize)
        *RequiredSize = size;

    if (!ReturnBuffer)
    {
        if (ReturnBufferSize)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        return TRUE;
    }

    if (size > ReturnBufferSize)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    WideCharToMultiByte(CP_ACP, 0, filenameW, -1, ReturnBuffer, size, NULL, NULL);
    HeapFree(GetProcessHeap(), 0, filenameW);

    return ret;
}

/***********************************************************************
 *      SetupQueryInfFileInformationW    (SETUPAPI.@)
 */
BOOL WINAPI SetupQueryInfFileInformationW(PSP_INF_INFORMATION InfInformation,
                                          UINT InfIndex, PWSTR ReturnBuffer,
                                          DWORD ReturnBufferSize, PDWORD RequiredSize) 
{
    DWORD len;
    LPWSTR ptr;

    TRACE("(%p, %u, %p, %ld, %p) Stub!\n", InfInformation, InfIndex,
          ReturnBuffer, ReturnBufferSize, RequiredSize);

    if (!InfInformation)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (InfIndex != 0)
        FIXME("Appended INF files are not handled\n");

    ptr = (LPWSTR)&InfInformation->VersionData[0];
    len = lstrlenW(ptr);

    if (RequiredSize)
        *RequiredSize = len + 1;

    if (!ReturnBuffer)
        return TRUE;

    if (ReturnBufferSize < len)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    lstrcpyW(ReturnBuffer, ptr);
    return TRUE;
}
