/*
 * Copyright 2009 Henri Verbeet for CodeWeavers
 * Copyright 2010 Rico Schüller
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

#include "d3d10_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d10);

HRESULT WINAPI D3D10CompileShader(const char *data, SIZE_T data_size, const char *filename,
        const D3D10_SHADER_MACRO *defines, ID3D10Include *include, const char *entrypoint,
        const char *profile, UINT flags, ID3D10Blob **shader, ID3D10Blob **error_messages)
{
    return D3DCompileFromMemory(data, data_size, filename, defines, include,
            entrypoint, profile, flags, 0, shader, error_messages);
}

HRESULT WINAPI D3D10DisassembleShader(const void *data, SIZE_T data_size,
        BOOL color_code, const char *comments, ID3D10Blob **disassembly)
{
    TRACE("data %p, data_size %#Ix, color_code %#x, comments %p, disassembly %p.\n",
            data, data_size, color_code, comments, disassembly);

    return D3DDisassembleCode(data, data_size, color_code ? D3D_DISASM_ENABLE_COLOR_CODE : 0, comments, disassembly);
}
