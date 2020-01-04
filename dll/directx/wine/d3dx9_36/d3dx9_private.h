/*
 * Copyright (C) 2002 Raphael Junqueira
 * Copyright (C) 2008 David Adam
 * Copyright (C) 2008 Tony Wasserka
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

#ifndef __WINE_D3DX9_PRIVATE_H
#define __WINE_D3DX9_PRIVATE_H

#define NONAMELESSUNION
#include "wine/debug.h"
#include "wine/heap.h"
#include "wine/rbtree.h"

#define COBJMACROS
#include "d3dx9.h"

#define ULONG64_MAX (~(ULONG64)0)

struct vec4
{
    float x, y, z, w;
};

struct volume
{
    UINT width;
    UINT height;
    UINT depth;
};

/* for internal use */
enum format_type {
    FORMAT_ARGB,   /* unsigned */
    FORMAT_ARGBF16,/* float 16 */
    FORMAT_ARGBF,  /* float */
    FORMAT_DXT,
    FORMAT_INDEX,
    FORMAT_UNKNOWN
};

struct pixel_format_desc {
    D3DFORMAT format;
    BYTE bits[4];
    BYTE shift[4];
    UINT bytes_per_pixel;
    UINT block_width;
    UINT block_height;
    UINT block_byte_count;
    enum format_type type;
    void (*from_rgba)(const struct vec4 *src, struct vec4 *dst);
    void (*to_rgba)(const struct vec4 *src, struct vec4 *dst, const PALETTEENTRY *palette);
};

struct d3dx_include_from_file
{
    ID3DXInclude ID3DXInclude_iface;
};

extern CRITICAL_SECTION from_file_mutex DECLSPEC_HIDDEN;
extern const struct ID3DXIncludeVtbl d3dx_include_from_file_vtbl DECLSPEC_HIDDEN;

static inline BOOL is_conversion_from_supported(const struct pixel_format_desc *format)
{
    if (format->type == FORMAT_ARGB || format->type == FORMAT_ARGBF16
            || format->type == FORMAT_ARGBF)
        return TRUE;
    return !!format->to_rgba;
}

static inline BOOL is_conversion_to_supported(const struct pixel_format_desc *format)
{
    if (format->type == FORMAT_ARGB || format->type == FORMAT_ARGBF16
            || format->type == FORMAT_ARGBF)
        return TRUE;
    return !!format->from_rgba;
}

HRESULT map_view_of_file(const WCHAR *filename, void **buffer, DWORD *length) DECLSPEC_HIDDEN;
HRESULT load_resource_into_memory(HMODULE module, HRSRC resinfo, void **buffer, DWORD *length) DECLSPEC_HIDDEN;

HRESULT write_buffer_to_file(const WCHAR *filename, ID3DXBuffer *buffer) DECLSPEC_HIDDEN;

const struct pixel_format_desc *get_format_info(D3DFORMAT format) DECLSPEC_HIDDEN;
const struct pixel_format_desc *get_format_info_idx(int idx) DECLSPEC_HIDDEN;

void copy_pixels(const BYTE *src, UINT src_row_pitch, UINT src_slice_pitch,
    BYTE *dst, UINT dst_row_pitch, UINT dst_slice_pitch, const struct volume *size,
    const struct pixel_format_desc *format) DECLSPEC_HIDDEN;
void convert_argb_pixels(const BYTE *src, UINT src_row_pitch, UINT src_slice_pitch,
    const struct volume *src_size, const struct pixel_format_desc *src_format,
    BYTE *dst, UINT dst_row_pitch, UINT dst_slice_pitch, const struct volume *dst_size,
    const struct pixel_format_desc *dst_format, D3DCOLOR color_key, const PALETTEENTRY *palette) DECLSPEC_HIDDEN;
void point_filter_argb_pixels(const BYTE *src, UINT src_row_pitch, UINT src_slice_pitch,
    const struct volume *src_size, const struct pixel_format_desc *src_format,
    BYTE *dst, UINT dst_row_pitch, UINT dst_slice_pitch, const struct volume *dst_size,
    const struct pixel_format_desc *dst_format, D3DCOLOR color_key, const PALETTEENTRY *palette) DECLSPEC_HIDDEN;

HRESULT load_texture_from_dds(IDirect3DTexture9 *texture, const void *src_data, const PALETTEENTRY *palette,
        DWORD filter, D3DCOLOR color_key, const D3DXIMAGE_INFO *src_info, unsigned int skip_levels,
        unsigned int *loaded_miplevels) DECLSPEC_HIDDEN;
HRESULT load_cube_texture_from_dds(IDirect3DCubeTexture9 *cube_texture, const void *src_data,
    const PALETTEENTRY *palette, DWORD filter, D3DCOLOR color_key, const D3DXIMAGE_INFO *src_info) DECLSPEC_HIDDEN;
HRESULT load_volume_from_dds(IDirect3DVolume9 *dst_volume, const PALETTEENTRY *dst_palette,
    const D3DBOX *dst_box, const void *src_data, const D3DBOX *src_box, DWORD filter, D3DCOLOR color_key,
    const D3DXIMAGE_INFO *src_info) DECLSPEC_HIDDEN;
HRESULT load_volume_texture_from_dds(IDirect3DVolumeTexture9 *volume_texture, const void *src_data,
    const PALETTEENTRY *palette, DWORD filter, DWORD color_key, const D3DXIMAGE_INFO *src_info) DECLSPEC_HIDDEN;
HRESULT lock_surface(IDirect3DSurface9 *surface, D3DLOCKED_RECT *lock,
        IDirect3DSurface9 **temp_surface, BOOL write) DECLSPEC_HIDDEN;
HRESULT unlock_surface(IDirect3DSurface9 *surface, D3DLOCKED_RECT *lock,
        IDirect3DSurface9 *temp_surface, BOOL update) DECLSPEC_HIDDEN;
HRESULT save_dds_texture_to_memory(ID3DXBuffer **dst_buffer, IDirect3DBaseTexture9 *src_texture,
    const PALETTEENTRY *src_palette) DECLSPEC_HIDDEN;

unsigned short float_32_to_16(const float in) DECLSPEC_HIDDEN;
float float_16_to_32(const unsigned short in) DECLSPEC_HIDDEN;

/* debug helpers */
const char *debug_d3dxparameter_class(D3DXPARAMETER_CLASS c) DECLSPEC_HIDDEN;
const char *debug_d3dxparameter_type(D3DXPARAMETER_TYPE t) DECLSPEC_HIDDEN;
const char *debug_d3dxparameter_registerset(D3DXREGISTER_SET r) DECLSPEC_HIDDEN;

/* parameter type conversion helpers */
static inline BOOL get_bool(D3DXPARAMETER_TYPE type, const void *data)
{
    switch (type)
    {
        case D3DXPT_FLOAT:
        case D3DXPT_INT:
        case D3DXPT_BOOL:
            return !!*(DWORD *)data;

        case D3DXPT_VOID:
            return *(BOOL *)data;

        default:
            return FALSE;
    }
}

static inline int get_int(D3DXPARAMETER_TYPE type, const void *data)
{
    switch (type)
    {
        case D3DXPT_FLOAT:
            return (int)(*(float *)data);

        case D3DXPT_INT:
        case D3DXPT_VOID:
            return *(int *)data;

        case D3DXPT_BOOL:
            return get_bool(type, data);

        default:
            return 0;
    }
}

static inline float get_float(D3DXPARAMETER_TYPE type, const void *data)
{
    switch (type)
    {
        case D3DXPT_FLOAT:
        case D3DXPT_VOID:
            return *(float *)data;

        case D3DXPT_INT:
            return (float)(*(int *)data);

        case D3DXPT_BOOL:
            return (float)get_bool(type, data);

        default:
            return 0.0f;
    }
}

static inline void set_number(void *outdata, D3DXPARAMETER_TYPE outtype, const void *indata, D3DXPARAMETER_TYPE intype)
{
    if (outtype == intype)
    {
        *(DWORD *)outdata = *(DWORD *)indata;
        return;
    }

    switch (outtype)
    {
        case D3DXPT_FLOAT:
            *(float *)outdata = get_float(intype, indata);
            break;

        case D3DXPT_BOOL:
            *(BOOL *)outdata = get_bool(intype, indata);
            break;

        case D3DXPT_INT:
            *(int *)outdata = get_int(intype, indata);
            break;

        default:
            *(DWORD *)outdata = 0;
            break;
    }
}

static inline BOOL is_param_type_sampler(D3DXPARAMETER_TYPE type)
{
    return type == D3DXPT_SAMPLER
            || type == D3DXPT_SAMPLER1D || type == D3DXPT_SAMPLER2D
            || type == D3DXPT_SAMPLER3D || type == D3DXPT_SAMPLERCUBE;
}

struct d3dx_parameter;

enum pres_reg_tables
{
    PRES_REGTAB_IMMED,
    PRES_REGTAB_CONST,
    PRES_REGTAB_OCONST,
    PRES_REGTAB_OBCONST,
    PRES_REGTAB_OICONST,
    PRES_REGTAB_TEMP,
    PRES_REGTAB_COUNT,
    PRES_REGTAB_FIRST_SHADER = PRES_REGTAB_CONST,
};

struct d3dx_const_param_eval_output
{
    struct d3dx_parameter *param;
    enum pres_reg_tables table;
    enum D3DXPARAMETER_CLASS constant_class;
    unsigned int register_index;
    unsigned int register_count;
    BOOL direct_copy;
    unsigned int element_count;
};

struct d3dx_const_tab
{
    unsigned int input_count;
    D3DXCONSTANT_DESC *inputs;
    struct d3dx_parameter **inputs_param;
    unsigned int const_set_count;
    unsigned int const_set_size;
    struct d3dx_const_param_eval_output *const_set;
    const enum pres_reg_tables *regset2table;
    ULONG64 update_version;
};

struct d3dx_regstore
{
    void *tables[PRES_REGTAB_COUNT];
    unsigned int table_sizes[PRES_REGTAB_COUNT]; /* registers count */
};

struct d3dx_pres_ins;

struct d3dx_preshader
{
    struct d3dx_regstore regs;

    unsigned int ins_count;
    struct d3dx_pres_ins *ins;

    struct d3dx_const_tab inputs;
};

struct d3dx_param_eval
{
    D3DXPARAMETER_TYPE param_type;

    struct d3dx_preshader pres;
    struct d3dx_const_tab shader_inputs;

    ULONG64 *version_counter;
};

struct param_rb_entry
{
    struct wine_rb_entry entry;
    char *full_name;
    struct d3dx_parameter *param;
};

struct d3dx_shared_data;
struct d3dx_top_level_parameter;

struct d3dx_parameter
{
    char magic_string[4];
    struct d3dx_top_level_parameter *top_level_param;
    struct d3dx_param_eval *param_eval;
    char *name;
    void *data;
    D3DXPARAMETER_CLASS class;
    D3DXPARAMETER_TYPE  type;
    UINT rows;
    UINT columns;
    UINT element_count;
    UINT member_count;
    DWORD flags;
    UINT bytes;
    DWORD object_id;

    struct d3dx_parameter *members;
    char *semantic;

    char *full_name;
    struct wine_rb_entry rb_entry;
};

struct d3dx_top_level_parameter
{
    struct d3dx_parameter param;
    UINT annotation_count;
    struct d3dx_parameter *annotations;
    ULONG64 update_version;
    ULONG64 *version_counter;
    struct d3dx_shared_data *shared_data;
};

struct d3dx_shared_data
{
    void *data;
    struct d3dx_top_level_parameter **parameters;
    unsigned int size, count;
    ULONG64 update_version;
};

struct d3dx9_base_effect;

static inline BOOL is_top_level_parameter(struct d3dx_parameter *param)
{
    return &param->top_level_param->param == param;
}

static inline struct d3dx_top_level_parameter
        *top_level_parameter_from_parameter(struct d3dx_parameter *param)
{
    return CONTAINING_RECORD(param, struct d3dx_top_level_parameter, param);
}

static inline ULONG64 next_update_version(ULONG64 *version_counter)
{
    return ++*version_counter;
}

static inline BOOL is_top_level_param_dirty(struct d3dx_top_level_parameter *param, ULONG64 update_version)
{
    struct d3dx_shared_data *shared_data;

    if ((shared_data = param->shared_data))
        return update_version < shared_data->update_version;
    else
        return update_version < param->update_version;
}

static inline BOOL is_param_dirty(struct d3dx_parameter *param, ULONG64 update_version)
{
    return is_top_level_param_dirty(param->top_level_param, update_version);
}

struct d3dx_parameter *get_parameter_by_name(struct d3dx9_base_effect *base,
        struct d3dx_parameter *parameter, const char *name) DECLSPEC_HIDDEN;

#ifdef __REACTOS__
#define SET_D3D_STATE_(_manager, _device, _method, ...) ((_manager) ? (_manager)->lpVtbl->_method((_manager), __VA_ARGS__) \
        : (_device)->lpVtbl->_method((_device), __VA_ARGS__))
#define SET_D3D_STATE(_base_effect, _method, ...) SET_D3D_STATE_((_base_effect)->manager, (_base_effect)->device, _method, __VA_ARGS__)
#else
#define SET_D3D_STATE_(manager, device, method, args...) (manager ? manager->lpVtbl->method(manager, args) \
        : device->lpVtbl->method(device, args))
#define SET_D3D_STATE(base_effect, args...) SET_D3D_STATE_(base_effect->manager, base_effect->device, args)
#endif

HRESULT d3dx_create_param_eval(struct d3dx9_base_effect *base_effect, void *byte_code,
        unsigned int byte_code_size, D3DXPARAMETER_TYPE type,
        struct d3dx_param_eval **peval, ULONG64 *version_counter,
        const char **skip_constants, unsigned int skip_constants_count) DECLSPEC_HIDDEN;
void d3dx_free_param_eval(struct d3dx_param_eval *peval) DECLSPEC_HIDDEN;
HRESULT d3dx_evaluate_parameter(struct d3dx_param_eval *peval,
        const struct d3dx_parameter *param, void *param_value) DECLSPEC_HIDDEN;
HRESULT d3dx_param_eval_set_shader_constants(ID3DXEffectStateManager *manager, struct IDirect3DDevice9 *device,
        struct d3dx_param_eval *peval, BOOL update_all) DECLSPEC_HIDDEN;
BOOL is_param_eval_input_dirty(struct d3dx_param_eval *peval, ULONG64 update_version) DECLSPEC_HIDDEN;

struct ctab_constant {
    D3DXCONSTANT_DESC desc;
    WORD constantinfo_reserved;
    struct ctab_constant *constants;
};

const struct ctab_constant *d3dx_shader_get_ctab_constant(ID3DXConstantTable *iface,
        D3DXHANDLE constant) DECLSPEC_HIDDEN;

HRESULT create_dummy_skin(ID3DXSkinInfo **iface) DECLSPEC_HIDDEN;

#endif /* __WINE_D3DX9_PRIVATE_H */
