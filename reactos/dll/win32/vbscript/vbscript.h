/*
 * Copyright 2011 Jacek Caban for CodeWeavers
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

#ifndef _VBSCRIPT_H
#define _VBSCRIPT_H

#include <assert.h>
#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS

#include <windef.h>
#include <winbase.h>
#include <objbase.h>
#include <oleauto.h>
#include <objsafe.h>
#include <dispex.h>
#include <activscp.h>

#include <wine/debug.h>
#include <wine/list.h>
#include <wine/unicode.h>

WINE_DEFAULT_DEBUG_CHANNEL(vbscript);

typedef struct {
    void **blocks;
    DWORD block_cnt;
    DWORD last_block;
    DWORD offset;
    BOOL mark;
    struct list custom_blocks;
} heap_pool_t;

void heap_pool_init(heap_pool_t*) DECLSPEC_HIDDEN;
void *heap_pool_alloc(heap_pool_t*,size_t) __WINE_ALLOC_SIZE(2) DECLSPEC_HIDDEN;
void *heap_pool_grow(heap_pool_t*,void*,DWORD,DWORD) DECLSPEC_HIDDEN;
void heap_pool_clear(heap_pool_t*) DECLSPEC_HIDDEN;
void heap_pool_free(heap_pool_t*) DECLSPEC_HIDDEN;
heap_pool_t *heap_pool_mark(heap_pool_t*) DECLSPEC_HIDDEN;

typedef struct _function_t function_t;
typedef struct _vbscode_t vbscode_t;
typedef struct _script_ctx_t script_ctx_t;
typedef struct _vbdisp_t vbdisp_t;

typedef struct named_item_t {
    IDispatch *disp;
    DWORD flags;
    LPWSTR name;

    struct list entry;
} named_item_t;

typedef enum {
    VBDISP_CALLGET,
    VBDISP_LET,
    VBDISP_SET,
    VBDISP_ANY
} vbdisp_invoke_type_t;

typedef struct {
    unsigned dim_cnt;
    SAFEARRAYBOUND *bounds;
} array_desc_t;

typedef struct {
    BOOL is_public;
    BOOL is_array;
    const WCHAR *name;
} vbdisp_prop_desc_t;

typedef struct {
    const WCHAR *name;
    BOOL is_public;
    BOOL is_array;
    function_t *entries[VBDISP_ANY];
} vbdisp_funcprop_desc_t;

#define BP_GET      1
#define BP_GETPUT   2

typedef struct {
    DISPID id;
    HRESULT (*proc)(vbdisp_t*,VARIANT*,unsigned,VARIANT*);
    DWORD flags;
    unsigned min_args;
    UINT_PTR max_args;
} builtin_prop_t;

typedef struct _class_desc_t {
    const WCHAR *name;
    script_ctx_t *ctx;

    unsigned class_initialize_id;
    unsigned class_terminate_id;
    unsigned func_cnt;
    vbdisp_funcprop_desc_t *funcs;

    unsigned prop_cnt;
    vbdisp_prop_desc_t *props;

    unsigned array_cnt;
    array_desc_t *array_descs;

    unsigned builtin_prop_cnt;
    const builtin_prop_t *builtin_props;
    ITypeInfo *typeinfo;
    function_t *value_func;

    struct _class_desc_t *next;
} class_desc_t;

struct _vbdisp_t {
    IDispatchEx IDispatchEx_iface;

    LONG ref;
    BOOL terminator_ran;
    struct list entry;

    const class_desc_t *desc;
    SAFEARRAY **arrays;
    VARIANT props[1];
};

typedef struct _ident_map_t ident_map_t;

typedef struct {
    IDispatchEx IDispatchEx_iface;
    LONG ref;

    ident_map_t *ident_map;
    unsigned ident_map_cnt;
    unsigned ident_map_size;

    script_ctx_t *ctx;
} ScriptDisp;

HRESULT create_vbdisp(const class_desc_t*,vbdisp_t**) DECLSPEC_HIDDEN;
HRESULT disp_get_id(IDispatch*,BSTR,vbdisp_invoke_type_t,BOOL,DISPID*) DECLSPEC_HIDDEN;
HRESULT vbdisp_get_id(vbdisp_t*,BSTR,vbdisp_invoke_type_t,BOOL,DISPID*) DECLSPEC_HIDDEN;
HRESULT disp_call(script_ctx_t*,IDispatch*,DISPID,DISPPARAMS*,VARIANT*) DECLSPEC_HIDDEN;
HRESULT disp_propput(script_ctx_t*,IDispatch*,DISPID,DISPPARAMS*) DECLSPEC_HIDDEN;
void collect_objects(script_ctx_t*) DECLSPEC_HIDDEN;
HRESULT create_procedure_disp(script_ctx_t*,vbscode_t*,IDispatch**) DECLSPEC_HIDDEN;
HRESULT create_script_disp(script_ctx_t*,ScriptDisp**) DECLSPEC_HIDDEN;

HRESULT to_int(VARIANT*,int*) DECLSPEC_HIDDEN;

static inline unsigned arg_cnt(const DISPPARAMS *dp)
{
    return dp->cArgs - dp->cNamedArgs;
}

static inline VARIANT *get_arg(DISPPARAMS *dp, DWORD i)
{
    return dp->rgvarg + dp->cArgs-i-1;
}

typedef struct _dynamic_var_t {
    struct _dynamic_var_t *next;
    VARIANT v;
    const WCHAR *name;
    BOOL is_const;
} dynamic_var_t;

struct _script_ctx_t {
    IActiveScriptSite *site;
    LCID lcid;

    IInternetHostSecurityManager *secmgr;
    DWORD safeopt;

    IDispatch *host_global;

    ScriptDisp *script_obj;

    class_desc_t global_desc;
    vbdisp_t *global_obj;

    class_desc_t err_desc;
    vbdisp_t *err_obj;

    HRESULT err_number;

    dynamic_var_t *global_vars;
    function_t *global_funcs;
    class_desc_t *classes;
    class_desc_t *procs;

    heap_pool_t heap;

    struct list objects;
    struct list code_list;
    struct list named_items;
};

HRESULT init_global(script_ctx_t*) DECLSPEC_HIDDEN;
HRESULT init_err(script_ctx_t*) DECLSPEC_HIDDEN;

IUnknown *create_ax_site(script_ctx_t*) DECLSPEC_HIDDEN;

typedef enum {
    ARG_NONE = 0,
    ARG_STR,
    ARG_BSTR,
    ARG_INT,
    ARG_UINT,
    ARG_ADDR,
    ARG_DOUBLE
} instr_arg_type_t;

#define OP_LIST                                   \
    X(add,            1, 0,           0)          \
    X(and,            1, 0,           0)          \
    X(assign_ident,   1, ARG_BSTR,    ARG_UINT)   \
    X(assign_member,  1, ARG_BSTR,    ARG_UINT)   \
    X(bool,           1, ARG_INT,     0)          \
    X(catch,          1, ARG_ADDR,    ARG_UINT)    \
    X(case,           0, ARG_ADDR,    0)          \
    X(concat,         1, 0,           0)          \
    X(const,          1, ARG_BSTR,    0)          \
    X(dim,            1, ARG_BSTR,    ARG_UINT)   \
    X(div,            1, 0,           0)          \
    X(double,         1, ARG_DOUBLE,  0)          \
    X(empty,          1, 0,           0)          \
    X(enumnext,       0, ARG_ADDR,    ARG_BSTR)   \
    X(equal,          1, 0,           0)          \
    X(hres,           1, ARG_UINT,    0)          \
    X(errmode,        1, ARG_INT,     0)          \
    X(eqv,            1, 0,           0)          \
    X(exp,            1, 0,           0)          \
    X(gt,             1, 0,           0)          \
    X(gteq,           1, 0,           0)          \
    X(icall,          1, ARG_BSTR,    ARG_UINT)   \
    X(icallv,         1, ARG_BSTR,    ARG_UINT)   \
    X(idiv,           1, 0,           0)          \
    X(imp,            1, 0,           0)          \
    X(incc,           1, ARG_BSTR,    0)          \
    X(is,             1, 0,           0)          \
    X(jmp,            0, ARG_ADDR,    0)          \
    X(jmp_false,      0, ARG_ADDR,    0)          \
    X(jmp_true,       0, ARG_ADDR,    0)          \
    X(long,           1, ARG_INT,     0)          \
    X(lt,             1, 0,           0)          \
    X(lteq,           1, 0,           0)          \
    X(mcall,          1, ARG_BSTR,    ARG_UINT)   \
    X(mcallv,         1, ARG_BSTR,    ARG_UINT)   \
    X(me,             1, 0,           0)          \
    X(mod,            1, 0,           0)          \
    X(mul,            1, 0,           0)          \
    X(neg,            1, 0,           0)          \
    X(nequal,         1, 0,           0)          \
    X(new,            1, ARG_STR,     0)          \
    X(newenum,        1, 0,           0)          \
    X(not,            1, 0,           0)          \
    X(nothing,        1, 0,           0)          \
    X(null,           1, 0,           0)          \
    X(or,             1, 0,           0)          \
    X(pop,            1, ARG_UINT,    0)          \
    X(ret,            0, 0,           0)          \
    X(set_ident,      1, ARG_BSTR,    ARG_UINT)   \
    X(set_member,     1, ARG_BSTR,    ARG_UINT)   \
    X(short,          1, ARG_INT,     0)          \
    X(step,           0, ARG_ADDR,    ARG_BSTR)   \
    X(stop,           1, 0,           0)          \
    X(string,         1, ARG_STR,     0)          \
    X(sub,            1, 0,           0)          \
    X(val,            1, 0,           0)          \
    X(xor,            1, 0,           0)

typedef enum {
#define X(x,n,a,b) OP_##x,
OP_LIST
#undef X
    OP_LAST
} vbsop_t;

typedef union {
    const WCHAR *str;
    BSTR bstr;
    unsigned uint;
    LONG lng;
    double *dbl;
} instr_arg_t;

typedef struct {
    vbsop_t op;
    instr_arg_t arg1;
    instr_arg_t arg2;
} instr_t;

typedef struct {
    const WCHAR *name;
    BOOL by_ref;
} arg_desc_t;

typedef enum {
    FUNC_GLOBAL,
    FUNC_FUNCTION,
    FUNC_SUB,
    FUNC_PROPGET,
    FUNC_PROPLET,
    FUNC_PROPSET,
    FUNC_DEFGET
} function_type_t;

typedef struct {
    const WCHAR *name;
} var_desc_t;

struct _function_t {
    function_type_t type;
    const WCHAR *name;
    BOOL is_public;
    arg_desc_t *args;
    unsigned arg_cnt;
    var_desc_t *vars;
    unsigned var_cnt;
    array_desc_t *array_descs;
    unsigned array_cnt;
    unsigned code_off;
    vbscode_t *code_ctx;
    function_t *next;
};

struct _vbscode_t {
    instr_t *instrs;
    WCHAR *source;

    BOOL option_explicit;

    BOOL pending_exec;
    function_t main_code;

    BSTR *bstr_pool;
    unsigned bstr_pool_size;
    unsigned bstr_cnt;
    heap_pool_t heap;

    struct list entry;
};

void release_vbscode(vbscode_t*) DECLSPEC_HIDDEN;
HRESULT compile_script(script_ctx_t*,const WCHAR*,const WCHAR*,vbscode_t**) DECLSPEC_HIDDEN;
HRESULT exec_script(script_ctx_t*,function_t*,vbdisp_t*,DISPPARAMS*,VARIANT*) DECLSPEC_HIDDEN;
void release_dynamic_vars(dynamic_var_t*) DECLSPEC_HIDDEN;

typedef struct {
    UINT16 len;
    WCHAR buf[7];
} string_constant_t;

#define TID_LIST \
    XDIID(ErrObj) \
    XDIID(GlobalObj)

typedef enum {
#define XDIID(iface) iface ## _tid,
TID_LIST
#undef XDIID
    LAST_tid
} tid_t;

HRESULT get_typeinfo(tid_t,ITypeInfo**) DECLSPEC_HIDDEN;
void release_regexp_typelib(void) DECLSPEC_HIDDEN;

#ifndef INT32_MIN
#define INT32_MIN (-2147483647-1)
#endif

#ifndef INT32_MAX
#define INT32_MAX (2147483647)
#endif

static inline BOOL is_int32(double d)
{
    return INT32_MIN <= d && d <= INT32_MAX && (double)(int)d == d;
}

HRESULT create_regexp(IDispatch**) DECLSPEC_HIDDEN;

HRESULT map_hres(HRESULT) DECLSPEC_HIDDEN;

#define FACILITY_VBS 0xa
#define MAKE_VBSERROR(code) MAKE_HRESULT(SEVERITY_ERROR, FACILITY_VBS, code)

#define VBSE_ILLEGAL_FUNC_CALL              5
#define VBSE_OVERFLOW                       6
#define VBSE_OUT_OF_MEMORY                  7
#define VBSE_OUT_OF_BOUNDS                  9
#define VBSE_ARRAY_LOCKED                  10
#define VBSE_TYPE_MISMATCH                 13
#define VBSE_FILE_NOT_FOUND                53
#define VBSE_IO_ERROR                      57
#define VBSE_FILE_ALREADY_EXISTS           58
#define VBSE_DISK_FULL                     61
#define VBSE_TOO_MANY_FILES                67
#define VBSE_PERMISSION_DENIED             70
#define VBSE_PATH_FILE_ACCESS              75
#define VBSE_PATH_NOT_FOUND                76
#define VBSE_ILLEGAL_NULL_USE              94
#define VBSE_OLE_NOT_SUPPORTED            430
#define VBSE_OLE_NO_PROP_OR_METHOD        438
#define VBSE_ACTION_NOT_SUPPORTED         445
#define VBSE_NAMED_ARGS_NOT_SUPPORTED     446
#define VBSE_LOCALE_SETTING_NOT_SUPPORTED 447
#define VBSE_NAMED_PARAM_NOT_FOUND        448
#define VBSE_INVALID_TYPELIB_VARIABLE     458
#define VBSE_FUNC_ARITY_MISMATCH          450
#define VBSE_PARAMETER_NOT_OPTIONAL       449
#define VBSE_NOT_ENUM                     451
#define VBSE_INVALID_DLL_FUNCTION_NAME    453
#define VBSE_CANT_CREATE_TMP_FILE         322
#define VBSE_OLE_FILE_NOT_FOUND           432
#define VBSE_CANT_CREATE_OBJECT           429
#define VBSE_SERVER_NOT_FOUND             462

HRESULT WINAPI VBScriptFactory_CreateInstance(IClassFactory*,IUnknown*,REFIID,void**) DECLSPEC_HIDDEN;
HRESULT WINAPI VBScriptRegExpFactory_CreateInstance(IClassFactory*,IUnknown*,REFIID,void**) DECLSPEC_HIDDEN;

static inline void *heap_alloc(size_t len)
{
    return HeapAlloc(GetProcessHeap(), 0, len);
}

static inline void *heap_alloc_zero(size_t len)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
}

static inline void *heap_realloc(void *mem, size_t len)
{
    return HeapReAlloc(GetProcessHeap(), 0, mem, len);
}

static inline BOOL heap_free(void *mem)
{
    return HeapFree(GetProcessHeap(), 0, mem);
}

static inline LPWSTR heap_strdupW(LPCWSTR str)
{
    LPWSTR ret = NULL;

    if(str) {
        DWORD size;

        size = (strlenW(str)+1)*sizeof(WCHAR);
        ret = heap_alloc(size);
        if(ret)
            memcpy(ret, str, size);
    }

    return ret;
}

#define VBSCRIPT_BUILD_VERSION 16978
#define VBSCRIPT_MAJOR_VERSION 5
#define VBSCRIPT_MINOR_VERSION 8

#include "parse.h"
#include "regexp.h"
#include "vbscript_defs.h"

#endif /* _VBSCRIPT_H */
