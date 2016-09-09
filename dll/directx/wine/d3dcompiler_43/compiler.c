/*
 * Copyright 2009 Matteo Bruni
 * Copyright 2010 Matteo Bruni for CodeWeavers
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

#include "d3dcompiler_private.h"
#include "wine/unicode.h"
#include "wine/wpp.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dcompiler);

#define D3DXERR_INVALIDDATA                      0x88760b59

#define BUFFER_INITIAL_CAPACITY 256

struct mem_file_desc
{
    const char *buffer;
    unsigned int size;
    unsigned int pos;
};

static struct mem_file_desc current_shader;
static ID3DInclude *current_include;
static const char *initial_filename;

#define INCLUDES_INITIAL_CAPACITY 4

struct loaded_include
{
    const char *name;
    const char *data;
};

static struct loaded_include *includes;
static int includes_capacity, includes_size;
static const char *parent_include;

static char *wpp_output;
static int wpp_output_capacity, wpp_output_size;

static char *wpp_messages;
static int wpp_messages_capacity, wpp_messages_size;

/* Mutex used to guarantee a single invocation
   of the D3DXAssembleShader function (or its variants) at a time.
   This is needed as wpp isn't thread-safe */
static CRITICAL_SECTION wpp_mutex;
static CRITICAL_SECTION_DEBUG wpp_mutex_debug =
{
    0, 0, &wpp_mutex,
    { &wpp_mutex_debug.ProcessLocksList,
      &wpp_mutex_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": wpp_mutex") }
};
static CRITICAL_SECTION wpp_mutex = { &wpp_mutex_debug, -1, 0, 0, 0, 0 };

/* Preprocessor error reporting functions */
static void wpp_write_message(const char *fmt, va_list args)
{
    char* newbuffer;
    int rc, newsize;

    if(wpp_messages_capacity == 0)
    {
        wpp_messages = HeapAlloc(GetProcessHeap(), 0, MESSAGEBUFFER_INITIAL_SIZE);
        if(wpp_messages == NULL)
            return;

        wpp_messages_capacity = MESSAGEBUFFER_INITIAL_SIZE;
    }

    while(1)
    {
        rc = vsnprintf(wpp_messages + wpp_messages_size,
                       wpp_messages_capacity - wpp_messages_size, fmt, args);

        if (rc < 0 ||                                           /* C89 */
            rc >= wpp_messages_capacity - wpp_messages_size) {  /* C99 */
            /* Resize the buffer */
            newsize = wpp_messages_capacity * 2;
            newbuffer = HeapReAlloc(GetProcessHeap(), 0, wpp_messages, newsize);
            if(newbuffer == NULL)
            {
                ERR("Error reallocating memory for parser messages\n");
                return;
            }
            wpp_messages = newbuffer;
            wpp_messages_capacity = newsize;
        }
        else
        {
            wpp_messages_size += rc;
            return;
        }
    }
}

static void PRINTF_ATTR(1,2) wpp_write_message_var(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    wpp_write_message(fmt, args);
    va_end(args);
}

static void wpp_error(const char *file, int line, int col, const char *_near,
                      const char *msg, va_list ap)
{
    wpp_write_message_var("%s:%d:%d: %s: ", file ? file : "'main file'",
                          line, col, "Error");
    wpp_write_message(msg, ap);
    wpp_write_message_var("\n");
}

static void wpp_warning(const char *file, int line, int col, const char *_near,
                        const char *msg, va_list ap)
{
    wpp_write_message_var("%s:%d:%d: %s: ", file ? file : "'main file'",
                          line, col, "Warning");
    wpp_write_message(msg, ap);
    wpp_write_message_var("\n");
}

static char *wpp_lookup_mem(const char *filename, int type, const char *parent_name,
                            char **include_path, int include_path_count)
{
    /* We don't check for file existence here. We will potentially fail on
     * the following wpp_open_mem(). */
    char *path;
    int i;

    TRACE("Looking for include %s, parent %s.\n", debugstr_a(filename), debugstr_a(parent_name));

    parent_include = NULL;
    if (strcmp(parent_name, initial_filename))
    {
        for(i = 0; i < includes_size; i++)
        {
            if(!strcmp(parent_name, includes[i].name))
            {
                parent_include = includes[i].data;
                break;
            }
        }
        if(parent_include == NULL)
        {
            ERR("Parent include %s missing.\n", debugstr_a(parent_name));
            return NULL;
        }
    }

    path = malloc(strlen(filename) + 1);
    if(path)
        memcpy(path, filename, strlen(filename) + 1);
    return path;
}

static void *wpp_open_mem(const char *filename, int type)
{
    struct mem_file_desc *desc;
    HRESULT hr;

    TRACE("Opening include %s.\n", debugstr_a(filename));

    if(!strcmp(filename, initial_filename))
    {
        current_shader.pos = 0;
        return &current_shader;
    }

    if(current_include == NULL) return NULL;
    desc = HeapAlloc(GetProcessHeap(), 0, sizeof(*desc));
    if(!desc)
        return NULL;

    if (FAILED(hr = ID3DInclude_Open(current_include, type ? D3D_INCLUDE_LOCAL : D3D_INCLUDE_SYSTEM,
            filename, parent_include, (const void **)&desc->buffer, &desc->size)))
    {
        HeapFree(GetProcessHeap(), 0, desc);
        return NULL;
    }

    if(includes_capacity == includes_size)
    {
        if(includes_capacity == 0)
        {
            includes = HeapAlloc(GetProcessHeap(), 0, INCLUDES_INITIAL_CAPACITY * sizeof(*includes));
            if(includes == NULL)
            {
                ERR("Error allocating memory for the loaded includes structure\n");
                goto error;
            }
            includes_capacity = INCLUDES_INITIAL_CAPACITY * sizeof(*includes);
        }
        else
        {
            int newcapacity = includes_capacity * 2;
            struct loaded_include *newincludes =
                HeapReAlloc(GetProcessHeap(), 0, includes, newcapacity);
            if(newincludes == NULL)
            {
                ERR("Error reallocating memory for the loaded includes structure\n");
                goto error;
            }
            includes = newincludes;
            includes_capacity = newcapacity;
        }
    }
    includes[includes_size].name = filename;
    includes[includes_size++].data = desc->buffer;

    desc->pos = 0;
    return desc;

error:
    ID3DInclude_Close(current_include, desc->buffer);
    HeapFree(GetProcessHeap(), 0, desc);
    return NULL;
}

static void wpp_close_mem(void *file)
{
    struct mem_file_desc *desc = file;

    if(desc != &current_shader)
    {
        if(current_include)
            ID3DInclude_Close(current_include, desc->buffer);
        else
            ERR("current_include == NULL, desc == %p, buffer = %s\n",
                desc, desc->buffer);

        HeapFree(GetProcessHeap(), 0, desc);
    }
}

static int wpp_read_mem(void *file, char *buffer, unsigned int len)
{
    struct mem_file_desc *desc = file;

    len = min(len, desc->size - desc->pos);
    memcpy(buffer, desc->buffer + desc->pos, len);
    desc->pos += len;
    return len;
}

static void wpp_write_mem(const char *buffer, unsigned int len)
{
    char *new_wpp_output;

    if(wpp_output_capacity == 0)
    {
        wpp_output = HeapAlloc(GetProcessHeap(), 0, BUFFER_INITIAL_CAPACITY);
        if(!wpp_output)
            return;

        wpp_output_capacity = BUFFER_INITIAL_CAPACITY;
    }
    if(len > wpp_output_capacity - wpp_output_size)
    {
        while(len > wpp_output_capacity - wpp_output_size)
        {
            wpp_output_capacity *= 2;
        }
        new_wpp_output = HeapReAlloc(GetProcessHeap(), 0, wpp_output,
                                     wpp_output_capacity);
        if(!new_wpp_output)
        {
            ERR("Error allocating memory\n");
            return;
        }
        wpp_output = new_wpp_output;
    }
    memcpy(wpp_output + wpp_output_size, buffer, len);
    wpp_output_size += len;
}

static int wpp_close_output(void)
{
    char *new_wpp_output = HeapReAlloc(GetProcessHeap(), 0, wpp_output,
                                       wpp_output_size + 1);
    if(!new_wpp_output) return 0;
    wpp_output = new_wpp_output;
    wpp_output[wpp_output_size]='\0';
    wpp_output_size++;
    return 1;
}

static HRESULT preprocess_shader(const void *data, SIZE_T data_size, const char *filename,
        const D3D_SHADER_MACRO *defines, ID3DInclude *include, ID3DBlob **error_messages)
{
    int ret;
    HRESULT hr = S_OK;
    const D3D_SHADER_MACRO *def = defines;

    static const struct wpp_callbacks wpp_callbacks =
    {
        wpp_lookup_mem,
        wpp_open_mem,
        wpp_close_mem,
        wpp_read_mem,
        wpp_write_mem,
        wpp_error,
        wpp_warning,
    };

    if (def != NULL)
    {
        while (def->Name != NULL)
        {
            wpp_add_define(def->Name, def->Definition);
            def++;
        }
    }
    current_include = include;
    includes_size = 0;

    wpp_output_size = wpp_output_capacity = 0;
    wpp_output = NULL;

    wpp_set_callbacks(&wpp_callbacks);
    wpp_messages_size = wpp_messages_capacity = 0;
    wpp_messages = NULL;
    current_shader.buffer = data;
    current_shader.size = data_size;
    initial_filename = filename ? filename : "";

    ret = wpp_parse(initial_filename, NULL);
    if (!wpp_close_output())
        ret = 1;
    if (ret)
    {
        TRACE("Error during shader preprocessing\n");
        if (wpp_messages)
        {
            int size;
            ID3DBlob *buffer;

            TRACE("Preprocessor messages:\n%s\n", debugstr_a(wpp_messages));

            if (error_messages)
            {
                size = strlen(wpp_messages) + 1;
                hr = D3DCreateBlob(size, &buffer);
                if (FAILED(hr))
                    goto cleanup;
                CopyMemory(ID3D10Blob_GetBufferPointer(buffer), wpp_messages, size);
                *error_messages = buffer;
            }
        }
        if (data)
            TRACE("Shader source:\n%s\n", debugstr_an(data, data_size));
        hr = E_FAIL;
    }

cleanup:
    /* Remove the previously added defines */
    if (defines != NULL)
    {
        while (defines->Name != NULL)
        {
            wpp_del_define(defines->Name);
            defines++;
        }
    }
    HeapFree(GetProcessHeap(), 0, wpp_messages);
    return hr;
}

static HRESULT assemble_shader(const char *preproc_shader,
        ID3DBlob **shader_blob, ID3DBlob **error_messages)
{
    struct bwriter_shader *shader;
    char *messages = NULL;
    HRESULT hr;
    DWORD *res, size;
    ID3DBlob *buffer;
    char *pos;

    shader = SlAssembleShader(preproc_shader, &messages);

    if (messages)
    {
        TRACE("Assembler messages:\n");
        TRACE("%s\n", debugstr_a(messages));

        TRACE("Shader source:\n");
        TRACE("%s\n", debugstr_a(preproc_shader));

        if (error_messages)
        {
            const char *preproc_messages = *error_messages ? ID3D10Blob_GetBufferPointer(*error_messages) : NULL;

            size = strlen(messages) + (preproc_messages ? strlen(preproc_messages) : 0) + 1;
            hr = D3DCreateBlob(size, &buffer);
            if (FAILED(hr))
            {
                HeapFree(GetProcessHeap(), 0, messages);
                if (shader) SlDeleteShader(shader);
                return hr;
            }
            pos = ID3D10Blob_GetBufferPointer(buffer);
            if (preproc_messages)
            {
                CopyMemory(pos, preproc_messages, strlen(preproc_messages) + 1);
                pos += strlen(preproc_messages);
            }
            CopyMemory(pos, messages, strlen(messages) + 1);

            if (*error_messages) ID3D10Blob_Release(*error_messages);
            *error_messages = buffer;
        }
        HeapFree(GetProcessHeap(), 0, messages);
    }

    if (shader == NULL)
    {
        ERR("Asm reading failed\n");
        return D3DXERR_INVALIDDATA;
    }

    hr = SlWriteBytecode(shader, 9, &res, &size);
    SlDeleteShader(shader);
    if (FAILED(hr))
    {
        ERR("SlWriteBytecode failed with 0x%08x\n", hr);
        return D3DXERR_INVALIDDATA;
    }

    if (shader_blob)
    {
        hr = D3DCreateBlob(size, &buffer);
        if (FAILED(hr))
        {
            HeapFree(GetProcessHeap(), 0, res);
            return hr;
        }
        CopyMemory(ID3D10Blob_GetBufferPointer(buffer), res, size);
        *shader_blob = buffer;
    }

    HeapFree(GetProcessHeap(), 0, res);

    return S_OK;
}

HRESULT WINAPI D3DAssemble(const void *data, SIZE_T datasize, const char *filename,
        const D3D_SHADER_MACRO *defines, ID3DInclude *include, UINT flags,
        ID3DBlob **shader, ID3DBlob **error_messages)
{
    HRESULT hr;

    TRACE("data %p, datasize %lu, filename %s, defines %p, include %p, sflags %#x,\n"
            "shader %p, error_messages %p\n",
            data, datasize, debugstr_a(filename), defines, include, flags, shader, error_messages);

    EnterCriticalSection(&wpp_mutex);

    /* TODO: flags */
    if (flags) FIXME("flags %x\n", flags);

    if (shader) *shader = NULL;
    if (error_messages) *error_messages = NULL;

    hr = preprocess_shader(data, datasize, filename, defines, include, error_messages);
    if (SUCCEEDED(hr))
        hr = assemble_shader(wpp_output, shader, error_messages);

    HeapFree(GetProcessHeap(), 0, wpp_output);
    LeaveCriticalSection(&wpp_mutex);
    return hr;
}

struct target_info {
    const char *name;
    enum shader_type type;
    DWORD sm_major;
    DWORD sm_minor;
    DWORD level_major;
    DWORD level_minor;
    BOOL sw;
    BOOL support;
};

/* Must be kept sorted for binary search */
static const struct target_info targets_info[] = {
    { "cs_4_0",            ST_UNKNOWN, 4, 0, 0, 0, FALSE, FALSE },
    { "cs_4_1",            ST_UNKNOWN, 4, 1, 0, 0, FALSE, FALSE },
    { "cs_5_0",            ST_UNKNOWN, 5, 0, 0, 0, FALSE, FALSE },
    { "ds_5_0",            ST_UNKNOWN, 5, 0, 0, 0, FALSE, FALSE },
    { "fx_2_0",            ST_UNKNOWN, 2, 0, 0, 0, FALSE, FALSE },
    { "fx_4_0",            ST_UNKNOWN, 4, 0, 0, 0, FALSE, FALSE },
    { "fx_4_1",            ST_UNKNOWN, 4, 1, 0, 0, FALSE, FALSE },
    { "fx_5_0",            ST_UNKNOWN, 5, 0, 0, 0, FALSE, FALSE },
    { "gs_4_0",            ST_UNKNOWN, 4, 0, 0, 0, FALSE, FALSE },
    { "gs_4_1",            ST_UNKNOWN, 4, 1, 0, 0, FALSE, FALSE },
    { "gs_5_0",            ST_UNKNOWN, 5, 0, 0, 0, FALSE, FALSE },
    { "hs_5_0",            ST_UNKNOWN, 5, 0, 0, 0, FALSE, FALSE },
    { "ps.1.0",            ST_PIXEL,   1, 0, 0, 0, FALSE, TRUE  },
    { "ps.1.1",            ST_PIXEL,   1, 1, 0, 0, FALSE, FALSE },
    { "ps.1.2",            ST_PIXEL,   1, 2, 0, 0, FALSE, FALSE },
    { "ps.1.3",            ST_PIXEL,   1, 3, 0, 0, FALSE, FALSE },
    { "ps.1.4",            ST_PIXEL,   1, 4, 0, 0, FALSE, FALSE },
    { "ps.2.0",            ST_PIXEL,   2, 0, 0, 0, FALSE, TRUE  },
    { "ps.2.a",            ST_PIXEL,   2, 1, 0, 0, FALSE, FALSE },
    { "ps.2.b",            ST_PIXEL,   2, 2, 0, 0, FALSE, FALSE },
    { "ps.2.sw",           ST_PIXEL,   2, 0, 0, 0, TRUE,  FALSE },
    { "ps.3.0",            ST_PIXEL,   3, 0, 0, 0, FALSE, TRUE  },
    { "ps_1_0",            ST_PIXEL,   1, 0, 0, 0, FALSE, TRUE  },
    { "ps_1_1",            ST_PIXEL,   1, 1, 0, 0, FALSE, FALSE },
    { "ps_1_2",            ST_PIXEL,   1, 2, 0, 0, FALSE, FALSE },
    { "ps_1_3",            ST_PIXEL,   1, 3, 0, 0, FALSE, FALSE },
    { "ps_1_4",            ST_PIXEL,   1, 4, 0, 0, FALSE, FALSE },
    { "ps_2_0",            ST_PIXEL,   2, 0, 0, 0, FALSE, TRUE  },
    { "ps_2_a",            ST_PIXEL,   2, 1, 0, 0, FALSE, FALSE },
    { "ps_2_b",            ST_PIXEL,   2, 2, 0, 0, FALSE, FALSE },
    { "ps_2_sw",           ST_PIXEL,   2, 0, 0, 0, TRUE,  FALSE },
    { "ps_3_0",            ST_PIXEL,   3, 0, 0, 0, FALSE, TRUE  },
    { "ps_3_sw",           ST_PIXEL,   3, 0, 0, 0, TRUE,  FALSE },
    { "ps_4_0",            ST_PIXEL,   4, 0, 0, 0, FALSE, TRUE  },
    { "ps_4_0_level_9_0",  ST_PIXEL,   4, 0, 9, 0, FALSE, FALSE },
    { "ps_4_0_level_9_1",  ST_PIXEL,   4, 0, 9, 1, FALSE, FALSE },
    { "ps_4_0_level_9_3",  ST_PIXEL,   4, 0, 9, 3, FALSE, FALSE },
    { "ps_4_1",            ST_PIXEL,   4, 1, 0, 0, FALSE, TRUE  },
    { "ps_5_0",            ST_PIXEL,   5, 0, 0, 0, FALSE, TRUE  },
    { "tx_1_0",            ST_UNKNOWN, 1, 0, 0, 0, FALSE, FALSE },
    { "vs.1.0",            ST_VERTEX,  1, 0, 0, 0, FALSE, TRUE  },
    { "vs.1.1",            ST_VERTEX,  1, 1, 0, 0, FALSE, TRUE  },
    { "vs.2.0",            ST_VERTEX,  2, 0, 0, 0, FALSE, TRUE  },
    { "vs.2.a",            ST_VERTEX,  2, 1, 0, 0, FALSE, FALSE },
    { "vs.2.sw",           ST_VERTEX,  2, 0, 0, 0, TRUE,  FALSE },
    { "vs.3.0",            ST_VERTEX,  3, 0, 0, 0, FALSE, TRUE  },
    { "vs.3.sw",           ST_VERTEX,  3, 0, 0, 0, TRUE,  FALSE },
    { "vs_1_0",            ST_VERTEX,  1, 0, 0, 0, FALSE, TRUE  },
    { "vs_1_1",            ST_VERTEX,  1, 1, 0, 0, FALSE, TRUE  },
    { "vs_2_0",            ST_VERTEX,  2, 0, 0, 0, FALSE, TRUE  },
    { "vs_2_a",            ST_VERTEX,  2, 1, 0, 0, FALSE, FALSE },
    { "vs_2_sw",           ST_VERTEX,  2, 0, 0, 0, TRUE,  FALSE },
    { "vs_3_0",            ST_VERTEX,  3, 0, 0, 0, FALSE, TRUE  },
    { "vs_3_sw",           ST_VERTEX,  3, 0, 0, 0, TRUE,  FALSE },
    { "vs_4_0",            ST_VERTEX,  4, 0, 0, 0, FALSE, TRUE  },
    { "vs_4_0_level_9_0",  ST_VERTEX,  4, 0, 9, 0, FALSE, FALSE },
    { "vs_4_0_level_9_1",  ST_VERTEX,  4, 0, 9, 1, FALSE, FALSE },
    { "vs_4_0_level_9_3",  ST_VERTEX,  4, 0, 9, 3, FALSE, FALSE },
    { "vs_4_1",            ST_VERTEX,  4, 1, 0, 0, FALSE, TRUE  },
    { "vs_5_0",            ST_VERTEX,  5, 0, 0, 0, FALSE, TRUE  },
};

static const struct target_info * get_target_info(const char *target)
{
    LONG min = 0;
    LONG max = sizeof(targets_info) / sizeof(targets_info[0]) - 1;
    LONG cur;
    int res;

    while (min <= max)
    {
        cur = (min + max) / 2;
        res = strcmp(target, targets_info[cur].name);
        if (res < 0)
            max = cur - 1;
        else if (res > 0)
            min = cur + 1;
        else
            return &targets_info[cur];
    }

    return NULL;
}

static HRESULT compile_shader(const char *preproc_shader, const char *target, const char *entrypoint,
        ID3DBlob **shader_blob, ID3DBlob **error_messages)
{
    struct bwriter_shader *shader;
    char *messages = NULL;
    HRESULT hr;
    DWORD *res, size, major, minor;
    ID3DBlob *buffer;
    char *pos;
    enum shader_type shader_type;
    const struct target_info *info;

    TRACE("Preprocessed shader source: %s\n", debugstr_a(preproc_shader));

    TRACE("Checking compilation target %s\n", debugstr_a(target));
    info = get_target_info(target);
    if (!info)
    {
        FIXME("Unknown compilation target %s\n", debugstr_a(target));
        return D3DERR_INVALIDCALL;
    }
    else
    {
        if (!info->support)
        {
            FIXME("Compilation target %s not yet supported\n", debugstr_a(target));
            return D3DERR_INVALIDCALL;
        }
        else
        {
            shader_type = info->type;
            major = info->sm_major;
            minor = info->sm_minor;
        }
    }

    shader = parse_hlsl_shader(preproc_shader, shader_type, major, minor, entrypoint, &messages);

    if (messages)
    {
        TRACE("Compiler messages:\n");
        TRACE("%s\n", debugstr_a(messages));

        TRACE("Shader source:\n");
        TRACE("%s\n", debugstr_a(preproc_shader));

        if (error_messages)
        {
            const char *preproc_messages = *error_messages ? ID3D10Blob_GetBufferPointer(*error_messages) : NULL;

            size = strlen(messages) + (preproc_messages ? strlen(preproc_messages) : 0) + 1;
            hr = D3DCreateBlob(size, &buffer);
            if (FAILED(hr))
            {
                HeapFree(GetProcessHeap(), 0, messages);
                if (shader) SlDeleteShader(shader);
                return hr;
            }
            pos = ID3D10Blob_GetBufferPointer(buffer);
            if (preproc_messages)
            {
                memcpy(pos, preproc_messages, strlen(preproc_messages) + 1);
                pos += strlen(preproc_messages);
            }
            memcpy(pos, messages, strlen(messages) + 1);

            if (*error_messages) ID3D10Blob_Release(*error_messages);
            *error_messages = buffer;
        }
        HeapFree(GetProcessHeap(), 0, messages);
    }

    if (!shader)
    {
        ERR("HLSL shader parsing failed.\n");
        return D3DXERR_INVALIDDATA;
    }

    hr = SlWriteBytecode(shader, 9, &res, &size);
    SlDeleteShader(shader);
    if (FAILED(hr))
    {
        ERR("SlWriteBytecode failed with error 0x%08x.\n", hr);
        return D3DXERR_INVALIDDATA;
    }

    if (shader_blob)
    {
        hr = D3DCreateBlob(size, &buffer);
        if (FAILED(hr))
        {
            HeapFree(GetProcessHeap(), 0, res);
            return hr;
        }
        memcpy(ID3D10Blob_GetBufferPointer(buffer), res, size);
        *shader_blob = buffer;
    }

    HeapFree(GetProcessHeap(), 0, res);

    return S_OK;
}

HRESULT WINAPI D3DCompile2(const void *data, SIZE_T data_size, const char *filename,
        const D3D_SHADER_MACRO *defines, ID3DInclude *include, const char *entrypoint,
        const char *target, UINT sflags, UINT eflags, UINT secondary_flags,
        const void *secondary_data, SIZE_T secondary_data_size, ID3DBlob **shader,
        ID3DBlob **error_messages)
{
    HRESULT hr;

    TRACE("data %p, data_size %lu, filename %s, defines %p, include %p, entrypoint %s,\n"
            "target %s, sflags %#x, eflags %#x, secondary_flags %#x, secondary_data %p,\n"
            "secondary_data_size %lu, shader %p, error_messages %p\n",
            data, data_size, debugstr_a(filename), defines, include, debugstr_a(entrypoint),
            debugstr_a(target), sflags, eflags, secondary_flags, secondary_data,
            secondary_data_size, shader, error_messages);

    if (secondary_data)
        FIXME("secondary data not implemented yet\n");

    if (shader) *shader = NULL;
    if (error_messages) *error_messages = NULL;

    EnterCriticalSection(&wpp_mutex);

    hr = preprocess_shader(data, data_size, filename, defines, include, error_messages);
    if (SUCCEEDED(hr))
        hr = compile_shader(wpp_output, target, entrypoint, shader, error_messages);

    HeapFree(GetProcessHeap(), 0, wpp_output);
    LeaveCriticalSection(&wpp_mutex);
    return hr;
}

HRESULT WINAPI D3DCompile(const void *data, SIZE_T data_size, const char *filename,
        const D3D_SHADER_MACRO *defines, ID3DInclude *include, const char *entrypoint,
        const char *target, UINT sflags, UINT eflags, ID3DBlob **shader, ID3DBlob **error_messages)
{
    TRACE("data %p, data_size %lu, filename %s, defines %p, include %p, entrypoint %s,\n"
            "target %s, sflags %#x, eflags %#x, shader %p, error_messages %p\n",
            data, data_size, debugstr_a(filename), defines, include, debugstr_a(entrypoint),
            debugstr_a(target), sflags, eflags, shader, error_messages);

    return D3DCompile2(data, data_size, filename, defines, include, entrypoint, target, sflags,
            eflags, 0, NULL, 0, shader, error_messages);
}

HRESULT WINAPI D3DPreprocess(const void *data, SIZE_T size, const char *filename,
        const D3D_SHADER_MACRO *defines, ID3DInclude *include,
        ID3DBlob **shader, ID3DBlob **error_messages)
{
    HRESULT hr;
    ID3DBlob *buffer;

    TRACE("data %p, size %lu, filename %s, defines %p, include %p, shader %p, error_messages %p\n",
          data, size, debugstr_a(filename), defines, include, shader, error_messages);

    if (!data)
        return E_INVALIDARG;

    EnterCriticalSection(&wpp_mutex);

    if (shader) *shader = NULL;
    if (error_messages) *error_messages = NULL;

    hr = preprocess_shader(data, size, filename, defines, include, error_messages);

    if (SUCCEEDED(hr))
    {
        if (shader)
        {
            hr = D3DCreateBlob(wpp_output_size, &buffer);
            if (FAILED(hr))
                goto cleanup;
            CopyMemory(ID3D10Blob_GetBufferPointer(buffer), wpp_output, wpp_output_size);
            *shader = buffer;
        }
        else
            hr = E_INVALIDARG;
    }

cleanup:
    HeapFree(GetProcessHeap(), 0, wpp_output);
    LeaveCriticalSection(&wpp_mutex);
    return hr;
}

HRESULT WINAPI D3DDisassemble(const void *data, SIZE_T size, UINT flags, const char *comments, ID3DBlob **disassembly)
{
    FIXME("data %p, size %lu, flags %#x, comments %p, disassembly %p stub!\n",
            data, size, flags, comments, disassembly);
    return E_NOTIMPL;
}

HRESULT WINAPI D3DCompileFromFile(const WCHAR *filename, const D3D_SHADER_MACRO *defines, ID3DInclude *includes,
        const char *entrypoint, const char *target, UINT flags1, UINT flags2, ID3DBlob **code, ID3DBlob **errors)
{
    FIXME("filename %s, defines %p, includes %p, entrypoint %s, target %s, flags1 %x, flags2 %x, code %p, errors %p\n",
            debugstr_w(filename), defines, includes, debugstr_a(entrypoint), debugstr_a(target), flags1, flags2, code, errors);

    return E_NOTIMPL;
}
