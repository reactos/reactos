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

    TRACE("(%p, %d, %p, %d, %p)\n", InfSpec, SearchControl, ReturnBuffer,
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
            FIXME("Unhandled search control: %d\n", SearchControl);

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

    TRACE("(%p, %u, %p, %d, %p) Stub!\n", InfInformation, InfIndex,
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

    TRACE("%p, %p, %s, %p, %p, 0x%08x, %p\n", hinf, context, debugstr_a(filename), source_id,
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
    WCHAR Section[MAX_PATH];
    DWORD size;
    LPWSTR source_id;

    if (!SetupDiGetActualSectionToInstallW(hinf, source_disks_files, Section, MAX_PATH, NULL, NULL))
        return NULL;

    if (!SetupFindFirstLineW( hinf, Section, filename, context ) &&
        !SetupFindFirstLineW( hinf, source_disks_files, filename, context ))
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

    if (!SetupDiGetActualSectionToInstallW(hinf, source_disks_names, Section, MAX_PATH, NULL, NULL))
    {
        HeapFree( GetProcessHeap(), 0, source_id );
        return NULL;
    }

    if (!SetupFindFirstLineW( hinf, Section, source_id, context ) &&
        !SetupFindFirstLineW( hinf, source_disks_names, source_id, context ))
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

    TRACE("%p, %p, %s, %p, %p, 0x%08x, %p\n", hinf, context, debugstr_w(filename), source_id,
          buffer, buffer_size, required_size);

    if (!context) context = &ctx;

    if (!(source_id_str = get_source_id( hinf, context, filename )))
        return FALSE;

    *source_id = strtolW( source_id_str, &end, 10 );
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

    TRACE("%p, %d, %d, %p, %d, %p\n", hinf, source_id, info, buffer, buffer_size,
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
    WCHAR Section[MAX_PATH];
    INFCONTEXT ctx;
    WCHAR source_id_str[11];
    static const WCHAR fmt[] = {'%','d',0};
    DWORD index;

    TRACE("%p, %d, %d, %p, %d, %p\n", hinf, source_id, info, buffer, buffer_size,
          required_size);

    sprintfW( source_id_str, fmt, source_id );

    if (!SetupDiGetActualSectionToInstallW(hinf, source_disks_names, Section, MAX_PATH, NULL, NULL))
        return FALSE;

    if (!SetupFindFirstLineW( hinf, Section, source_id_str, &ctx ) &&
        !SetupFindFirstLineW( hinf, source_disks_names, source_id_str, &ctx ))
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

    TRACE("%p, %p, %s, %p, 0x%08x, %p\n", hinf, context, debugstr_a(section), buffer,
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

    TRACE("%p, %p, %s, %p, 0x%08x, %p\n", hinf, context, debugstr_w(section), buffer,
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
    size = strlenW( dir ) + 1;
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
        WARN("incorrect OriginalFileInfo->cbSize of %d\n", OriginalFileInfo->cbSize);
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
        WARN("incorrect OriginalFileInfo->cbSize of %d\n", OriginalFileInfo->cbSize);
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
                           sizeof(OriginalFileInfo->OriginalCatalogName)/sizeof(OriginalFileInfo->OriginalCatalogName[0]),
                           NULL))
    {
        OriginalFileInfo->OriginalCatalogName[0] = '\0';
    }
    SetupCloseInfFile(hinf);

    /* FIXME: not quite correct as we just return the same file name as
     * destination (copied) inf file, not the source (original) inf file.
     * to fix it properly would require building a .pnf file */
    /* file name is stored in VersionData field of InfInformation */
    inf_name = strrchrW(inf_path, '\\');
    if (inf_name) inf_name++;
    else inf_name = inf_path;

    strcpyW(OriginalFileInfo->OriginalInfName, inf_name);

    return TRUE;
}

/***********************************************************************
 *      pSetupGetRealSystemTime (SETUPAPI.@)
 */

VOID
WINAPI
pSetupGetRealSystemTime(
    _Out_ LPSYSTEMTIME lpRealSystemTime)
{
    GetSystemTime(lpRealSystemTime);
}


/***********************************************************************
 *      SetupFreeSourceListA (SETUPAPI.@)
 */

BOOL WINAPI SetupFreeSourceListA(
  PCSTR **List,
  UINT  Count)
{
  TRACE("(%p, %d)\n", List, Count);

  for(int i=0; i<Count;i++){
    if(!HeapFree(GetProcessHeap(),0,(LPVOID)(*(List[i]))))
        return FALSE;
  }

  if(!HeapFree(GetProcessHeap(),0,(LPVOID)*List))
    return FALSE;

  *List = NULL;
  return TRUE;
}

/***********************************************************************
 *      SetupFreeSourceListW (SETUPAPI.@)
 */

BOOL WINAPI SetupFreeSourceListW(
  PCWSTR **List,
  UINT  Count)
{
  TRACE("(%p, %d)\n", List, Count);

  for(int i=0; i<Count;i++){
    if(!HeapFree(GetProcessHeap(),0,(LPVOID)(*(List[i]))))
        return FALSE;
  }

  if(!HeapFree(GetProcessHeap(),0,(LPVOID)*List))
    return FALSE;

  *List = NULL;
  return TRUE;
}

/***********************************************************************
 *      SetupQuerySourceListW (SETUPAPI.@)
 */

BOOL WINAPI SetupQuerySourceListW(
  DWORD Flags,
  PCWSTR **List,
  PUINT Count
){
    TRACE("(%X, %p, %d)\n", Flags, List, Count);

    WCHAR buffer[MAX_PATH * 2] = {0};
    PWSTR szInstallationSource = buffer;
    PCWSTR* listSources = NULL;
    UINT iCount = 0;

    if(srclist_temporary_sources != NULL){
        iCount = srclist_temporary_sources_count;
        for(int i=0; i<srclist_temporary_sources_count; i++){
            wcscpy(szInstallationSource,srclist_temporary_sources[i]);
            szInstallationSource += wcslen(szInstallationSource)+1;
        }

    }
    else{
        LSTATUS statusUser = 0,statusSystem = 0;
        HKEY handle;
        DWORD len = 0, nRead = 0;

        if(Flags & SRCLIST_SYSTEM || Flags == 0){
            statusSystem = RegOpenKeyExW(HKEY_LOCAL_MACHINE,L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup",0,KEY_QUERY_VALUE,&handle);
            if(!statusSystem){
                statusSystem = RegQueryValueExW(handle,L"Installation Sources",0,NULL,(LPBYTE)szInstallationSource,&len);
                RegCloseKey(handle);
            }
            if(statusSystem){
                SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, szInstallationSource);
                len = wcslen(szInstallationSource);
            }


            nRead = 0;
            while(*szInstallationSource && nRead < len){
                nRead += wcslen(szInstallationSource) + 1;
                szInstallationSource += wcslen(szInstallationSource) + 1;
                iCount++;
            }
        }

        if(Flags & SRCLIST_USER || Flags == 0){
            statusUser = RegOpenKeyExW(HKEY_CURRENT_USER,L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup",0,KEY_QUERY_VALUE,&handle);
            if(!statusUser){
                statusUser = RegQueryValueExW(handle,L"Installation Sources",0,NULL,(LPBYTE)szInstallationSource,&len);
                RegCloseKey(handle);
            }
            if(statusUser && !statusSystem){
                SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, szInstallationSource);
                len = wcslen(szInstallationSource);
            }


            nRead = 0;
            while(*szInstallationSource && nRead < len){
                nRead += wcslen(szInstallationSource) + 1;
                szInstallationSource += wcslen(szInstallationSource) + 1;
                iCount++;
            }
        }
    }

    if(!(Flags & SRCLIST_NOSTRIPPLATFORM)){
        szInstallationSource = buffer;
        for(int i=0; i<iCount; i++){
            PWSTR platformSource = wcsstr(szInstallationSource,L"\\x");
            if(platformSource && (platformSource[4] == L'\\' || platformSource[4] == 0)  && ((platformSource[2] == L'8' && platformSource[3] ==  L'6') || (platformSource[2] == L'6' && platformSource[3] == L'4')))
                memcpy(platformSource,&platformSource[4],platformSource-szInstallationSource- 4 * sizeof(WCHAR));
            szInstallationSource += wcslen(szInstallationSource)+1;
        }
    }

    listSources = (PCWSTR*)HeapAlloc(GetProcessHeap(),0, iCount * sizeof(PCWSTR));
    if(!listSources)
        return FALSE;

    szInstallationSource = buffer;
    for(int i=0; i<iCount; i++){
        listSources[i]  = HeapAlloc(GetProcessHeap(),0,(wcslen(szInstallationSource)+1)*sizeof(WCHAR));
        if(!listSources[i]){
            SetupFreeSourceListW(&listSources,i);
            return FALSE;
        }
        wcscpy(listSources[i],szInstallationSource);
        szInstallationSource += wcslen(szInstallationSource) + 1;
    }

    *List = ((const WCHAR ***))listSources;
    *Count = iCount;

    return TRUE;
}

/***********************************************************************
 *      SetupQuerySourceListA (SETUPAPI.@)
 */
BOOL WINAPI SetupQuerySourceListA(
  DWORD Flags,
  PCSTR **List,
  PUINT Count
)
{
    TRACE("(%X, %p, %d)\n", Flags, List, Count);

    CHAR buffer[MAX_PATH * 2] = {0}; // FIXME - how much...?
    PSTR szInstallationSource = buffer;
    PCSTR* listSources = NULL;
    UINT iCount = 0;

    if(srclist_temporary_sources != NULL){
        iCount = srclist_temporary_sources_count;
        for(int i=0; i<srclist_temporary_sources_count; i++){
            //WideCharToMultiByte(CP_ACP,0,srclist_temporary_sources[i],wcslen(srclist_temporary_sources[i])+1,szInstallationSource,wcslen(srclist_temporary_sources[i])+1,NULL,NULL);
            strcpy(szInstallationSource,(PSTR)srclist_temporary_sources[i]);
            szInstallationSource += strlen(szInstallationSource)+1;
        }

    }
    else{
        LSTATUS statusUser = 0, statusSystem = 0;
        HKEY handle;
        DWORD len = 0, nRead = 0;

        if(Flags & SRCLIST_SYSTEM || Flags == 0){
            statusSystem = RegOpenKeyExA(HKEY_LOCAL_MACHINE,"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup",0,KEY_QUERY_VALUE,&handle);
            if(!statusSystem){
                statusSystem = RegQueryValueExA(handle,"Installation Sources",0,NULL,(LPBYTE)szInstallationSource,&len);
                RegCloseKey(handle);
            }
            if(statusSystem){
                SHGetFolderPathA(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, szInstallationSource);
                len = strlen(szInstallationSource);
            }

            nRead = 0;
            while(*szInstallationSource && nRead < len){
                nRead += strlen(szInstallationSource) + 1;
                szInstallationSource += strlen(szInstallationSource) + 1;
                iCount++;
            }
        }

        if(Flags & SRCLIST_USER || Flags == 0){
            statusUser = RegOpenKeyExA(HKEY_CURRENT_USER,"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup",0,KEY_QUERY_VALUE,&handle);
            if(!statusUser){
                statusUser = RegQueryValueExA(handle,"Installation Sources",0,NULL,(LPBYTE)szInstallationSource,&len);
                RegCloseKey(handle);
            }
            if(statusUser && !statusSystem){
                SHGetFolderPathA(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, szInstallationSource);
                len = strlen(szInstallationSource);
            }

            nRead = 0;
            while(*szInstallationSource && nRead < len){
                nRead += strlen(szInstallationSource) + 1;
                szInstallationSource += strlen(szInstallationSource) + 1;
                iCount++;
            }
        }
    }

    if(!(Flags & SRCLIST_NOSTRIPPLATFORM)){
        szInstallationSource = buffer;
        for(int i=0; i<iCount; i++){
            PSTR platformSource = strstr(szInstallationSource,"\\x");
            if(platformSource && (platformSource[4] == '\\' || platformSource[4] == 0)  && ((platformSource[2] == '8' && platformSource[3] ==  '6') || (platformSource[2] == '6' && platformSource[3] == '4')))
                memcpy(platformSource,&platformSource[4],platformSource - szInstallationSource - 4 * sizeof(WCHAR));
            szInstallationSource += strlen(szInstallationSource)+1;
        }
    }

    listSources = (PCSTR*)HeapAlloc(GetProcessHeap(),0,iCount*sizeof(PCSTR));
    if(!listSources)
        return FALSE;

    szInstallationSource = buffer;
    for(int i=0; i<iCount; i++){
        listSources[i]  = (PCSTR*)HeapAlloc(GetProcessHeap(),0,(strlen(szInstallationSource)+1)*sizeof(CHAR));
        if(!listSources[i]){
            SetupFreeSourceListA(&listSources,i);
            return FALSE;
        }
        strcpy(listSources[i],szInstallationSource);
        szInstallationSource += strlen(szInstallationSource) + 1;
    }

    *List = listSources;
    *Count = iCount;

    return TRUE;
}

/***********************************************************************
 *      SetupCancelTemporarySourceList (SETUPAPI.@)
 */
BOOL WINAPI SetupCancelTemporarySourceList(){
    if(srclist_temporary_sources == NULL)
        return FALSE;
    srclist_temporary_sources = NULL;
    srclist_temporary_sources_count = 0;
    noBrowse = FALSE;
    return TRUE;
}

/***********************************************************************
 *      SetupSetSourceListA (SETUPAPI.@)
 */
BOOL WINAPI SetupSetSourceListA(DWORD flags, PCSTR *list, UINT count)
{
    TRACE("(%X, %p, %d)\n", flags, list, count);

    if(flags & SRCLIST_TEMPORARY){
        srclist_temporary_sources = (PVOID*)*list;
        srclist_temporary_sources_count = count;
    }
    else{
        HKEY handle;
        LSTATUS status = 0;
        UINT len = 0;
        BYTE buffer[MAX_PATH]; // size?
        PCHAR currentPos = NULL;
        currentPos = buffer;
        for(int i=0; i < count; i++){
            strcpy(currentPos,list[i]);
            currentPos += strlen(list[i]) + 1;
        }
        len = currentPos - buffer;

        if(flags & SRCLIST_SYSTEM){
            status = RegOpenKeyExA(HKEY_LOCAL_MACHINE,"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup",0,KEY_SET_VALUE,&handle);
            if(status)
                return FALSE;
            status = RegSetValueExA(handle,"Installation Sources",0,REG_MULTI_SZ,buffer,len);
            if(status)
                return FALSE;
            RegCloseKey(handle);
        }

        if(flags & SRCLIST_USER){
            status = RegOpenKeyExA(HKEY_CURRENT_USER,"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup",0,KEY_SET_VALUE,&handle);
            if(status)
                return FALSE;
            status = RegSetValueExA(handle,"Installation Sources",0,REG_MULTI_SZ,buffer,len);
            RegCloseKey(handle);
            if(status)
                return FALSE;
        }
    }

    if(flags & SRCLIST_NOBROWSE)
        noBrowse = TRUE;

    return TRUE;
}

/***********************************************************************
 *      SetupSetSourceListW (SETUPAPI.@)
 */
BOOL WINAPI
SetupSetSourceListW(DWORD flags, PCWSTR *list, UINT count)
{
    TRACE("(%X, %p, %d)\n", flags, list, count);

    if (flags & SRCLIST_TEMPORARY)
    {
        srclist_temporary_sources = (PVOID *)*list;
        srclist_temporary_sources_count = count;
    }
    else
    {
        HKEY handle = NULL;
        LSTATUS status = 0;
        UINT len = 0;
        BYTE buffer[MAX_PATH];
        BYTE *currentPos = NULL;

        currentPos = buffer;
        for (int i = 0; i < count; i++)
        {
            wcscpy((wchar_t *)currentPos, list[i]);
            currentPos += wcslen(list[i]) + 1;
        }
        len = currentPos - buffer;

        if (flags & SRCLIST_SYSTEM)
        {
            status = RegOpenKeyExW(
                HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup", 0, KEY_SET_VALUE, &handle);
            if (status)
                return FALSE;
            status = RegSetValueExW(handle, L"Installation Sources", 0, REG_MULTI_SZ, buffer, len);
            if (status)
                return FALSE;
            RegCloseKey(handle);
        }

        if (flags & SRCLIST_USER)
        {
            status = RegOpenKeyExW(
                HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup", 0, KEY_SET_VALUE, &handle);
            if (status)
                return FALSE;
            status = RegSetValueExW(handle, L"Installation Sources", 0, REG_MULTI_SZ, buffer, len);
            RegCloseKey(handle);
            if (status)
                return FALSE;
        }
    }

    if (flags & SRCLIST_NOBROWSE)
        noBrowse = TRUE;

    return TRUE;
}
