/*
 * Copyright (C) 2009 Tony Wasserka
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
 *
 */

#include "wine/debug.h"
#include "d3dx9_36_private.h"

/************************************************************
 * map_view_of_file
 *
 * Loads a file into buffer and stores the number of read bytes in length.
 *
 * PARAMS
 *   filename [I] name of the file to be loaded
 *   buffer   [O] pointer to destination buffer
 *   length   [O] size of the obtained data
 *
 * RETURNS
 *   Success: D3D_OK
 *   Failure:
 *     see error codes for CreateFileW, GetFileSize, CreateFileMapping and MapViewOfFile
 *
 * NOTES
 *   The caller must UnmapViewOfFile when it doesn't need the data anymore
 *
 */
HRESULT map_view_of_file(LPCWSTR filename, LPVOID *buffer, DWORD *length)
{
    HANDLE hfile, hmapping = NULL;

    hfile = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if(hfile == INVALID_HANDLE_VALUE) goto error;

    *length = GetFileSize(hfile, NULL);
    if(*length == INVALID_FILE_SIZE) goto error;

    hmapping = CreateFileMappingW(hfile, NULL, PAGE_READONLY, 0, 0, NULL);
    if(!hmapping) goto error;

    *buffer = MapViewOfFile(hmapping, FILE_MAP_READ, 0, 0, 0);
    if(*buffer == NULL) goto error;

    CloseHandle(hmapping);
    CloseHandle(hfile);

    return S_OK;

error:
    CloseHandle(hmapping);
    CloseHandle(hfile);
    return HRESULT_FROM_WIN32(GetLastError());
}

/************************************************************
 * load_resource_into_memory
 *
 * Loads a resource into buffer and stores the number of
 * read bytes in length.
 *
 * PARAMS
 *   module  [I] handle to the module
 *   resinfo [I] handle to the resource's information block
 *   buffer  [O] pointer to destination buffer
 *   length  [O] size of the obtained data
 *
 * RETURNS
 *   Success: D3D_OK
 *   Failure:
 *     See error codes for SizeofResource, LoadResource and LockResource
 *
 * NOTES
 *   The memory doesn't need to be freed by the caller manually
 *
 */
HRESULT load_resource_into_memory(HMODULE module, HRSRC resinfo, LPVOID *buffer, DWORD *length)
{
    HGLOBAL resource;

    *length = SizeofResource(module, resinfo);
    if(*length == 0) return HRESULT_FROM_WIN32(GetLastError());

    resource = LoadResource(module, resinfo);
    if( !resource ) return HRESULT_FROM_WIN32(GetLastError());

    *buffer = LockResource(resource);
    if(*buffer == NULL) return HRESULT_FROM_WIN32(GetLastError());

    return S_OK;
}
