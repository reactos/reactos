/*
 * Copyright 2008,2011 Jacek Caban for CodeWeavers
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

#pragma once

#define OP_LIST                            \
    X(add,        1, 0,0)                  \
    X(and,        1, 0,0)                  \
    X(array,      1, 0,0)                  \
    X(assign,     1, 0,0)                  \
    X(assign_call,1, ARG_UINT,   0)       \
    X(bool,       1, ARG_INT,    0)        \
    X(bneg,       1, 0,0)                  \
    X(call,       1, ARG_UINT,   ARG_UINT) \
    X(call_member,1, ARG_UINT,   ARG_UINT) \
    X(carray,     1, ARG_UINT,   0)        \
    X(case,       0, ARG_ADDR,   0)        \
    X(cnd_nz,     0, ARG_ADDR,   0)        \
    X(cnd_z,      0, ARG_ADDR,   0)        \
    X(delete,     1, 0,0)                  \
    X(delete_ident,1,ARG_BSTR,   0)        \
    X(div,        1, 0,0)                  \
    X(double,     1, ARG_DBL,    0)        \
    X(end_finally,1, 0,0)                  \
    X(eq,         1, 0,0)                  \
    X(eq2,        1, 0,0)                  \
    X(forin,      0, ARG_ADDR,   0)        \
    X(func,       1, ARG_UINT,   0)        \
    X(gt,         1, 0,0)                  \
    X(gteq,       1, 0,0)                  \
    X(ident,      1, ARG_BSTR,   0)        \
    X(identid,    1, ARG_BSTR,   ARG_INT)  \
    X(in,         1, 0,0)                  \
    X(instanceof, 1, 0,0)                  \
    X(int,        1, ARG_INT,    0)        \
    X(jmp,        0, ARG_ADDR,   0)        \
    X(jmp_z,      0, ARG_ADDR,   0)        \
    X(lshift,     1, 0,0)                  \
    X(lt,         1, 0,0)                  \
    X(lteq,       1, 0,0)                  \
    X(member,     1, ARG_BSTR,   0)        \
    X(memberid,   1, ARG_UINT,   0)        \
    X(minus,      1, 0,0)                  \
    X(mod,        1, 0,0)                  \
    X(mul,        1, 0,0)                  \
    X(neg,        1, 0,0)                  \
    X(neq,        1, 0,0)                  \
    X(neq2,       1, 0,0)                  \
    X(new,        1, ARG_UINT,   0)        \
    X(new_obj,    1, 0,0)                  \
    X(null,       1, 0,0)                  \
    X(obj_prop,   1, ARG_BSTR,   0)        \
    X(or,         1, 0,0)                  \
    X(pop,        1, ARG_UINT,   0)        \
    X(pop_except, 1, 0,0)                  \
    X(pop_scope,  1, 0,0)                  \
    X(postinc,    1, ARG_INT,    0)        \
    X(preinc,     1, ARG_INT,    0)        \
    X(push_except,1, ARG_ADDR,   ARG_BSTR) \
    X(push_scope, 1, 0,0)                  \
    X(regexp,     1, ARG_STR,    ARG_UINT) \
    X(rshift,     1, 0,0)                  \
    X(rshift2,    1, 0,0)                  \
    X(str,        1, ARG_STR,    0)        \
    X(this,       1, 0,0)                  \
    X(throw,      0, 0,0)                  \
    X(throw_ref,  0, ARG_UINT,   0)        \
    X(throw_type, 0, ARG_UINT,   ARG_STR)  \
    X(tonum,      1, 0,0)                  \
    X(typeof,     1, 0,0)                  \
    X(typeofid,   1, 0,0)                  \
    X(typeofident,1, 0,0)                  \
    X(refval,     1, 0,0)                  \
    X(ret,        0, 0,0)                  \
    X(setret,     1, 0,0)                  \
    X(sub,        1, 0,0)                  \
    X(undefined,  1, 0,0)                  \
    X(var_set,    1, ARG_BSTR,   0)        \
    X(void,       1, 0,0)                  \
    X(xor,        1, 0,0)

typedef enum {
#define X(x,a,b,c) OP_##x,
OP_LIST
#undef X
    OP_LAST
} jsop_t;

typedef union {
    BSTR bstr;
    LONG lng;
    jsstr_t *str;
    unsigned uint;
} instr_arg_t;

typedef enum {
    ARG_NONE = 0,
    ARG_ADDR,
    ARG_BSTR,
    ARG_DBL,
    ARG_FUNC,
    ARG_INT,
    ARG_STR,
    ARG_UINT
} instr_arg_type_t;

typedef struct {
    jsop_t op;
    union {
        instr_arg_t arg[2];
        double dbl;
    } u;
} instr_t;

typedef struct _function_code_t {
    BSTR name;
    BSTR event_target;
    unsigned instr_off;

    const WCHAR *source;
    unsigned source_len;

    unsigned func_cnt;
    struct _function_code_t *funcs;

    unsigned var_cnt;
    BSTR *variables;

    unsigned param_cnt;
    BSTR *params;
} function_code_t;

typedef struct _bytecode_t {
    LONG ref;

    instr_t *instrs;
    heap_pool_t heap;

    function_code_t global_code;

    WCHAR *source;

    BSTR *bstr_pool;
    unsigned bstr_pool_size;
    unsigned bstr_cnt;

    jsstr_t **str_pool;
    unsigned str_pool_size;
    unsigned str_cnt;

    struct _bytecode_t *next;
} bytecode_t;

HRESULT compile_script(script_ctx_t*,const WCHAR*,const WCHAR*,const WCHAR*,BOOL,BOOL,bytecode_t**) DECLSPEC_HIDDEN;
void release_bytecode(bytecode_t*) DECLSPEC_HIDDEN;

static inline void bytecode_addref(bytecode_t *code)
{
    code->ref++;
}

typedef struct _scope_chain_t {
    LONG ref;
    jsdisp_t *jsobj;
    IDispatch *obj;
    struct _scope_chain_t *next;
} scope_chain_t;

HRESULT scope_push(scope_chain_t*,jsdisp_t*,IDispatch*,scope_chain_t**) DECLSPEC_HIDDEN;
void scope_release(scope_chain_t*) DECLSPEC_HIDDEN;

static inline void scope_addref(scope_chain_t *scope)
{
    scope->ref++;
}

typedef struct _except_frame_t except_frame_t;
struct _parser_ctx_t;

struct _exec_ctx_t {
    LONG ref;

    struct _parser_ctx_t *parser;
    bytecode_t *code;
    script_ctx_t *script;
    scope_chain_t *scope_chain;
    jsdisp_t *var_disp;
    IDispatch *this_obj;
    function_code_t *func_code;
    BOOL is_global;

    jsval_t *stack;
    unsigned stack_size;
    unsigned top;
    except_frame_t *except_frame;
    jsval_t ret;

    unsigned ip;
};

static inline void exec_addref(exec_ctx_t *ctx)
{
    ctx->ref++;
}

void exec_release(exec_ctx_t*) DECLSPEC_HIDDEN;
HRESULT create_exec_ctx(script_ctx_t*,IDispatch*,jsdisp_t*,scope_chain_t*,BOOL,exec_ctx_t**) DECLSPEC_HIDDEN;
HRESULT exec_source(exec_ctx_t*,bytecode_t*,function_code_t*,BOOL,jsval_t*) DECLSPEC_HIDDEN;
HRESULT create_source_function(script_ctx_t*,bytecode_t*,function_code_t*,scope_chain_t*,jsdisp_t**) DECLSPEC_HIDDEN;
