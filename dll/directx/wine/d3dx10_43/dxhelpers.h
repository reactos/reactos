/*
 * Copyright 2022 Piotr Caban
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

extern HRESULT load_file(const WCHAR *path, void **data, DWORD *size);
extern HRESULT load_resourceA(HMODULE module, const char *resource,
        void **data, DWORD *size);
extern HRESULT load_resourceW(HMODULE module, const WCHAR *resource,
        void **data, DWORD *size);

extern HRESULT get_image_info(const void *data, SIZE_T size, D3DX10_IMAGE_INFO *img_info);

extern void init_load_info(const D3DX10_IMAGE_LOAD_INFO *load_info,
        D3DX10_IMAGE_LOAD_INFO *out);
/* Returns array of D3D10_SUBRESOURCE_DATA structures followed by textures data. */
extern HRESULT load_texture_data(const void *data, SIZE_T size, D3DX10_IMAGE_LOAD_INFO *load_info,
        D3D10_SUBRESOURCE_DATA **resource_data);
extern HRESULT create_d3d_texture(ID3D10Device *device, D3DX10_IMAGE_LOAD_INFO *load_info,
        D3D10_SUBRESOURCE_DATA *resource_data, ID3D10Resource **texture);
