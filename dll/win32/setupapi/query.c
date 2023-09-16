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

#ifdef __WINESRC__

#ifdef __i386__
static const WCHAR source_disks_names_platform[] =
    {'S','o','u','r','c','e','D','i','s','k','s','N','a','m','e','s','.','x','8','6',0};
static const WCHAR source_disks_files_platform[] =
    {'S','o','u','r','c','e','D','i','s','k','s','F','i','l','e','s','.','x','8','6',0};
#elif defined(__x86_64__)
static const WCHAR source_disks_names_platform[] =
    {'S','o','u','r','c','e','D','i','s','k','s','N','a','m','e','s','.','a','m','d','6','4',0};
static const WCHAR source_disks_files_platform[] =
    {'S','o','u','r','c','e','D','i','s','k','s','F','i','l','e','s','.','a','m','d','6','4',0};
#elif defined(__arm__)
static const WCHAR source_disks_names_platform[] =
    {'S','o','u','r','c','e','D','i','s','k','s','N','a','m','e','s','.','a','r','m',0};
static const WCHAR source_disks_files_platform[] =
    {'S','o','u','r','c','e','D','i','s','k','s','F','i','l','e','s','.','a','r','m',0};
#elif defined(__aarch64__)
static const WCHAR source_disks_names_platform[] =
    {'S','o','u','r','c','e','D','i','s','k','s','N','a','m','e','s','.','a','r','m','6','4',0};
static const WCHAR source_disks_files_platform[] =
    {'S','o','u','r','c','e','D','i','s','k','s','F','i','l','e','s','.','a','r','m','6','4',0};
#else  /* FIXME: other platforms */
static const WCHAR source_disks_names_platform[] =
    {'S','o','u','r','c','e','D','i','s','k','s','N','a','m','e','s',0};
static const WCHAR source_disks_files_platform[] =
    {'S','o','u','r','c','e','D','i','s','k','s','F','i','l','e','s',0};
#endif

#endif // __WINESRC__

static const WCHAR source_disks_names[] =
    {'S','o','u','r','c','e','D','i','s','k','s','N','a','m','e','s',0};
static const WCHAR source_disks_files[] =
    {'S','o','u','r','c','e','D','i','s','k','s','F','i','l','e','s',0};

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
        len = MultiByteToWideChar(CP_ACP, 0, InfSpec, -1, NULL, 0);
        inf = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!inf)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
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
        HeapFree(GetProcessHeap(), 0, filenameW);
        if (ReturnBufferSize)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        return TRUE;
    }

    if (size > ReturnBufferSize)
    {
        HeapFree(GetProcessHeap(), 0, filenameW);
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

    ptr = (LPWSTR)InfInformation->VersionData;
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

/***********************************************************************
 *            SetupGetSourceFileLocationA   (SETUPAPI.@)
 */

BOOL WINAPI SetupGetSourceFileLocationA( HINF hinf, PINFCONTEXT context, PCSTR filename,
                                         PUINT source_id, PSTR buffer, DWORD buffer_size,
                                         PDWORD required_size )
{
    BOOL ret = FALSE;
    WCHAR *filenameW = NULL, *bufferW = NULL;
    DWORD required;
    INT size;

    TRACE("%p, %p, %s, %p, %p, 0x%08lx, %p\n", hinf, context, debugstr_a(filename), source_id,
          buffer, buffer_size, required_size);

    if (filename && *filename && !(filenameW = strdupAtoW( filename )))
        return FALSE;

    if (!SetupGetSourceFileLocationW( hinf, context, filenameW, source_id, NULL, 0, &required ))
        goto done;

    if (!(bufferW = HeapAlloc( GetProcessHeap(), 0, required * sizeof(WCHAR) )))
        goto done;

    if (!SetupGetSourceFileLocationW( hinf, context, filenameW, source_id, bufferW, required, NULL ))
        goto done;

    size = WideCharToMultiByte( CP_ACP, 0, bufferW, -1, NULL, 0, NULL, NULL );
    if (required_size) *required_size = size;

    if (buffer)
    {
        if (buffer_size >= size)
            WideCharToMultiByte( CP_ACP, 0, bufferW, -1, buffer, buffer_size, NULL, NULL );
        else
        {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            goto done;
        }
    }
    ret = TRUE;

 done:
    HeapFree( GetProcessHeap(), 0, filenameW );
    HeapFree( GetProcessHeap(), 0, bufferW );
    return ret;
}

static LPWSTR get_source_id( HINF hinf, PINFCONTEXT context, PCWSTR filename )
{
#ifndef __WINESRC__
    WCHAR Section[MAX_PATH];
#endif
    DWORD size;
    LPWSTR source_id;

#ifndef __WINESRC__
    if (!SetupDiGetActualSectionToInstallW(hinf, source_disks_files, Section, ARRAY_SIZE(Section), NULL, NULL))
        return NULL;

    if (!SetupFindFirstLineW( hinf, Section, filename, context ) &&
        !SetupFindFirstLineW( hinf, source_disks_files, filename, context ))
#else
    if (!SetupFindFirstLineW( hinf, source_disks_files_platform, filename, context ) &&
        !SetupFindFirstLineW( hinf, source_disks_files, filename, context ))
#endif // !__WINESRC__
        return NULL;

    if (!SetupGetStringFieldW( context, 1, NULL, 0, &size ))
        return NULL;

    if (!(source_id = HeapAlloc( GetProcessHeap(), 0, size * sizeof(WCHAR) )))
        return NULL;

    if (!SetupGetStringFieldW( context, 1, source_id, size, NULL ))
    {
        HeapFree( GetProcessHeap(), 0, source_id );
        return NULL;
    }

#ifndef __WINESRC__
    if (!SetupDiGetActualSectionToInstallW(hinf, source_disks_names, Section, ARRAY_SIZE(Section), NULL, NULL))
    {
        HeapFree( GetProcessHeap(), 0, source_id );
        return NULL;
    }

    if (!SetupFindFirstLineW( hinf, Section, source_id, context ) &&
        !SetupFindFirstLineW( hinf, source_disks_names, source_id, context ))
#else
    if (!SetupFindFirstLineW( hinf, source_disks_names_platform, source_id, context ) &&
        !SetupFindFirstLineW( hinf, source_disks_names, source_id, context ))
#endif // !__WINESRC__
    {
        HeapFree( GetProcessHeap(), 0, source_id );
        return NULL;
    }
    return source_id;
}

/***********************************************************************
 *            SetupGetSourceFileLocationW   (SETUPAPI.@)
 */

BOOL WINAPI SetupGetSourceFileLocationW( HINF hinf, PINFCONTEXT context, PCWSTR filename,
                                         PUINT source_id, PWSTR buffer, DWORD buffer_size,
                                         PDWORD required_size )
{
    INFCONTEXT ctx;
    WCHAR *end, *source_id_str;

    TRACE("%p, %p, %s, %p, %p, 0x%08lx, %p\n", hinf, context, debugstr_w(filename), source_id,
          buffer, buffer_size, required_size);

    if (!context) context = &ctx;

    if (!(source_id_str = get_source_id( hinf, context, filename )))
        return FALSE;

    *source_id = wcstol( source_id_str, &end, 10 );
    if (end == source_id_str || *end)
    {
        HeapFree( GetProcessHeap(), 0, source_id_str );
        return FALSE;
    }
    HeapFree( GetProcessHeap(), 0, source_id_str );

    if (SetupGetStringFieldW( context, 4, buffer, buffer_size, required_size ))
        return TRUE;

    if (required_size) *required_size = 1;
    if (buffer)
    {
        if (buffer_size >= 1) buffer[0] = 0;
        else
        {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }
    }
    return TRUE;
}

/***********************************************************************
 *            SetupGetSourceInfoA  (SETUPAPI.@)
 */

BOOL WINAPI SetupGetSourceInfoA( HINF hinf, UINT source_id, UINT info,
                                 PSTR buffer, DWORD buffer_size, LPDWORD required_size )
{
    BOOL ret = FALSE;
    WCHAR *bufferW = NULL;
    DWORD required;
    INT size;

    TRACE("%p, %d, %d, %p, %ld, %p\n", hinf, source_id, info, buffer, buffer_size,
          required_size);

    if (!SetupGetSourceInfoW( hinf, source_id, info, NULL, 0, &required ))
        return FALSE;

    if (!(bufferW = HeapAlloc( GetProcessHeap(), 0, required * sizeof(WCHAR) )))
        return FALSE;

    if (!SetupGetSourceInfoW( hinf, source_id, info, bufferW, required, NULL ))
        goto done;

    size = WideCharToMultiByte( CP_ACP, 0, bufferW, -1, NULL, 0, NULL, NULL );
    if (required_size) *required_size = size;

    if (buffer)
    {
        if (buffer_size >= size)
            WideCharToMultiByte( CP_ACP, 0, bufferW, -1, buffer, buffer_size, NULL, NULL );
        else
        {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            goto done;
        }
    }
    ret = TRUE;

 done:
    HeapFree( GetProcessHeap(), 0, bufferW );
    return ret;
}

/***********************************************************************
 *            SetupGetSourceInfoW  (SETUPAPI.@)
 */

BOOL WINAPI SetupGetSourceInfoW( HINF hinf, UINT source_id, UINT info,
                                 PWSTR buffer, DWORD buffer_size, LPDWORD required_size )
{
#ifndef __WINESRC__
    WCHAR Section[MAX_PATH];
#endif
    INFCONTEXT ctx;
    WCHAR source_id_str[11];
    static const WCHAR fmt[] = {'%','d',0};
    DWORD index;

    TRACE("%p, %d, %d, %p, %ld, %p\n", hinf, source_id, info, buffer, buffer_size,
          required_size);

    swprintf( source_id_str, ARRAY_SIZE(source_id_str), fmt, source_id );

#ifndef __WINESRC__
    if (!SetupDiGetActualSectionToInstallW(hinf, source_disks_names, Section, ARRAY_SIZE(Section), NULL, NULL))
        return FALSE;

    if (!SetupFindFirstLineW( hinf, Section, source_id_str, &ctx ) &&
        !SetupFindFirstLineW( hinf, source_disks_names, source_id_str, &ctx ))
#else
    if (!SetupFindFirstLineW( hinf, source_disks_names_platform, source_id_str, &ctx ) &&
        !SetupFindFirstLineW( hinf, source_disks_names, source_id_str, &ctx ))
#endif // !__WINESRC__
        return FALSE;

    switch (info)
    {
    case SRCINFO_PATH:          index = 4; break;
    case SRCINFO_TAGFILE:       index = 2; break;
    case SRCINFO_DESCRIPTION:   index = 1; break;
    default:
        WARN("unknown info level: %d\n", info);
        return FALSE;
    }

    if (SetupGetStringFieldW( &ctx, index, buffer, buffer_size, required_size ))
        return TRUE;

    if (required_size) *required_size = 1;
    if (buffer)
    {
        if (buffer_size >= 1) buffer[0] = 0;
        else
        {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }
    }
    return TRUE;
}

/***********************************************************************
 *            SetupGetTargetPathA   (SETUPAPI.@)
 */

BOOL WINAPI SetupGetTargetPathA( HINF hinf, PINFCONTEXT context, PCSTR section, PSTR buffer,
                                 DWORD buffer_size, PDWORD required_size )
{
    BOOL ret = FALSE;
    WCHAR *sectionW = NULL, *bufferW = NULL;
    DWORD required;
    INT size;

    TRACE("%p, %p, %s, %p, 0x%08lx, %p\n", hinf, context, debugstr_a(section), buffer,
          buffer_size, required_size);

    if (section && !(sectionW = strdupAtoW( section )))
        return FALSE;

    if (!SetupGetTargetPathW( hinf, context, sectionW, NULL, 0, &required ))
        goto done;

    if (!(bufferW = HeapAlloc( GetProcessHeap(), 0, required * sizeof(WCHAR) )))
        goto done;

    if (!SetupGetTargetPathW( hinf, context, sectionW, bufferW, required, NULL ))
        goto done;

    size = WideCharToMultiByte( CP_ACP, 0, bufferW, -1, NULL, 0, NULL, NULL );
    if (required_size) *required_size = size;

    if (buffer)
    {
        if (buffer_size >= size)
            WideCharToMultiByte( CP_ACP, 0, bufferW, -1, buffer, buffer_size, NULL, NULL );
        else
        {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            goto done;
        }
    }
    ret = TRUE;

 done:
    HeapFree( GetProcessHeap(), 0, sectionW );
    HeapFree( GetProcessHeap(), 0, bufferW );
    return ret;
}

/***********************************************************************
 *            SetupGetTargetPathW   (SETUPAPI.@)
 */

BOOL WINAPI SetupGetTargetPathW( HINF hinf, PINFCONTEXT context, PCWSTR section, PWSTR buffer,
                                 DWORD buffer_size, PDWORD required_size )
{
    static const WCHAR destination_dirs[] =
        {'D','e','s','t','i','n','a','t','i','o','n','D','i','r','s',0};
    static const WCHAR default_dest_dir[]  =
        {'D','e','f','a','u','l','t','D','e','s','t','D','i','r',0};

    INFCONTEXT ctx;
    WCHAR *dir, systemdir[MAX_PATH];
    unsigned int size;
    BOOL ret = FALSE;

    TRACE("%p, %p, %s, %p, 0x%08lx, %p\n", hinf, context, debugstr_w(section), buffer,
          buffer_size, required_size);

    if (context) ret = SetupFindFirstLineW( hinf, destination_dirs, NULL, context );
    else if (section)
    {
        if (!(ret = SetupFindFirstLineW( hinf, destination_dirs, section, &ctx )))
            ret = SetupFindFirstLineW( hinf, destination_dirs, default_dest_dir, &ctx );
    }
    if (!ret || !(dir = PARSER_get_dest_dir( context ? context : &ctx )))
    {
        GetSystemDirectoryW( systemdir, MAX_PATH );
        dir = systemdir;
    }
    size = lstrlenW( dir ) + 1;
    if (required_size) *required_size = size;

    if (buffer)
    {
        if (buffer_size >= size)
            lstrcpyW( buffer, dir );
        else
        {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            if (dir != systemdir) HeapFree( GetProcessHeap(), 0, dir );
            return FALSE;
        }
    }
    if (dir != systemdir) HeapFree( GetProcessHeap(), 0, dir );
    return TRUE;
}

/***********************************************************************
 *            SetupQueryInfOriginalFileInformationA   (SETUPAPI.@)
 */
BOOL WINAPI SetupQueryInfOriginalFileInformationA(
    PSP_INF_INFORMATION InfInformation, UINT InfIndex,
    PSP_ALTPLATFORM_INFO AlternativePlatformInfo,
    PSP_ORIGINAL_FILE_INFO_A OriginalFileInfo)
{
    BOOL ret;
    SP_ORIGINAL_FILE_INFO_W OriginalFileInfoW;

    TRACE("(%p, %d, %p, %p)\n", InfInformation, InfIndex,
        AlternativePlatformInfo, OriginalFileInfo);

    if (OriginalFileInfo->cbSize != sizeof(*OriginalFileInfo))
    {
        WARN("incorrect OriginalFileInfo->cbSize of %ld\n", OriginalFileInfo->cbSize);
        SetLastError( ERROR_INVALID_USER_BUFFER );
        return FALSE;
    }

    OriginalFileInfoW.cbSize = sizeof(OriginalFileInfoW);
    ret = SetupQueryInfOriginalFileInformationW(InfInformation, InfIndex,
        AlternativePlatformInfo, &OriginalFileInfoW);
    if (ret)
    {
        WideCharToMultiByte(CP_ACP, 0, OriginalFileInfoW.OriginalInfName, -1,
            OriginalFileInfo->OriginalInfName, MAX_PATH, NULL, NULL);
        WideCharToMultiByte(CP_ACP, 0, OriginalFileInfoW.OriginalCatalogName, -1,
            OriginalFileInfo->OriginalCatalogName, MAX_PATH, NULL, NULL);
    }

    return ret;
}

/***********************************************************************
 *            SetupQueryInfOriginalFileInformationW   (SETUPAPI.@)
 */
BOOL WINAPI SetupQueryInfOriginalFileInformationW(
    PSP_INF_INFORMATION InfInformation, UINT InfIndex,
    PSP_ALTPLATFORM_INFO AlternativePlatformInfo,
    PSP_ORIGINAL_FILE_INFO_W OriginalFileInfo)
{
    LPCWSTR inf_name;
    LPCWSTR inf_path;
    HINF hinf;
    static const WCHAR wszVersion[] = { 'V','e','r','s','i','o','n',0 };
    static const WCHAR wszCatalogFile[] = { 'C','a','t','a','l','o','g','F','i','l','e',0 };

    FIXME("(%p, %d, %p, %p): semi-stub\n", InfInformation, InfIndex,
        AlternativePlatformInfo, OriginalFileInfo);

    if (OriginalFileInfo->cbSize != sizeof(*OriginalFileInfo))
    {
        WARN("incorrect OriginalFileInfo->cbSize of %ld\n", OriginalFileInfo->cbSize);
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    inf_path = (LPWSTR)InfInformation->VersionData;

    /* FIXME: we should get OriginalCatalogName from CatalogFile line in
     * the original inf file and cache it, but that would require building a
     * .pnf file. */
    hinf = SetupOpenInfFileW(inf_path, NULL, INF_STYLE_WIN4, NULL);
    if (hinf == INVALID_HANDLE_VALUE) return FALSE;

    if (!SetupGetLineTextW(NULL, hinf, wszVersion, wszCatalogFile,
                           OriginalFileInfo->OriginalCatalogName,
                           ARRAY_SIZE(OriginalFileInfo->OriginalCatalogName), NULL))
    {
        OriginalFileInfo->OriginalCatalogName[0] = '\0';
    }
    SetupCloseInfFile(hinf);

    /* FIXME: not quite correct as we just return the same file name as
     * destination (copied) inf file, not the source (original) inf file.
     * to fix it properly would require building a .pnf file */
    /* file name is stored in VersionData field of InfInformation */
    inf_name = wcsrchr(inf_path, '\\');
    if (inf_name) inf_name++;
    else inf_name = inf_path;

    lstrcpyW(OriginalFileInfo->OriginalInfName, inf_name);

    return TRUE;
}

#if (_WIN32_WINNT >= _WIN32_WINNT_VISTA) || (DLL_EXPORT_VERSION >= _WIN32_WINNT_VISTA)

/***********************************************************************
 *      SetupGetInfDriverStoreLocationA (SETUPAPI.@)
 */
BOOL WINAPI SetupGetInfDriverStoreLocationA(
    PCSTR FileName, PSP_ALTPLATFORM_INFO AlternativePlatformInfo,
    PCSTR LocaleName, PSTR ReturnBuffer, DWORD ReturnBufferSize,
    PDWORD RequiredSize)
{
    FIXME("stub: %s %p %s %p %lu %p\n", debugstr_a(FileName), AlternativePlatformInfo, debugstr_a(LocaleName), ReturnBuffer, ReturnBufferSize, RequiredSize);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *      SetupGetInfDriverStoreLocationW (SETUPAPI.@)
 */
BOOL WINAPI SetupGetInfDriverStoreLocationW(
    PCWSTR FileName, PSP_ALTPLATFORM_INFO AlternativePlatformInfo,
    PCWSTR LocaleName, PWSTR ReturnBuffer, DWORD ReturnBufferSize,
    PDWORD RequiredSize)
{
    FIXME("stub: %s %p %s %p %lu %p\n", debugstr_w(FileName), AlternativePlatformInfo, debugstr_w(LocaleName), ReturnBuffer, ReturnBufferSize, RequiredSize);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

#endif // (_WIN32_WINNT >= _WIN32_WINNT_VISTA) || (DLL_EXPORT_VERSION >= _WIN32_WINNT_VISTA)
