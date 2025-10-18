/*
 * Format String Generator for IDL Compiler
 *
 * Copyright 2005-2006 Eric Kohl
 * Copyright 2005-2006 Robert Shearman
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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <limits.h>

#include "widl.h"
#include "utils.h"
#include "parser.h"
#include "header.h"
#include "typetree.h"

#include "typegen.h"
#include "expr.h"

/* round size up to multiple of alignment */
#define ROUND_SIZE(size, alignment) (((size) + ((alignment) - 1)) & ~((alignment) - 1))
/* value to add on to round size up to a multiple of alignment */
#define ROUNDING(size, alignment) (((alignment) - 1) - (((size) + ((alignment) - 1)) & ((alignment) - 1)))

static const type_t *current_structure;
static const var_t *current_func;
static const var_t *current_arg;
static const type_t *current_iface;

static struct list expr_eval_routines = LIST_INIT(expr_eval_routines);
struct expr_eval_routine
{
    struct list   entry;
    const type_t *iface;
    const type_t *cont_type;
    char         *name;
    unsigned int  baseoff;
    const expr_t *expr;
};

enum type_context
{
    TYPE_CONTEXT_TOPLEVELPARAM,
    TYPE_CONTEXT_PARAM,
    TYPE_CONTEXT_CONTAINER,
    TYPE_CONTEXT_CONTAINER_NO_POINTERS,
    TYPE_CONTEXT_RETVAL,
};

/* parameter flags in Oif mode */
static const unsigned short MustSize = 0x0001;
static const unsigned short MustFree = 0x0002;
static const unsigned short IsPipe = 0x0004;
static const unsigned short IsIn = 0x0008;
static const unsigned short IsOut = 0x0010;
static const unsigned short IsReturn = 0x0020;
static const unsigned short IsBasetype = 0x0040;
static const unsigned short IsByValue = 0x0080;
static const unsigned short IsSimpleRef = 0x0100;
/* static const unsigned short IsDontCallFreeInst = 0x0200; */
/* static const unsigned short SaveForAsyncFinish = 0x0400; */

/* robust flags in correlation descriptors */
static const unsigned short RobustEarly = 0x0001;
/* static const unsigned short RobustSplit = 0x0002; */
static const unsigned short RobustIsIIdIs = 0x0004;
/* static const unsigned short RobustDontCheck = 0x0008; */

static unsigned int field_memsize(const type_t *type, unsigned int *offset);
static unsigned int fields_memsize(const var_list_t *fields, unsigned int *align);
static unsigned int write_array_tfs(FILE *file, const attr_list_t *attrs, int toplevel_attrs, type_t *type,
                                    const char *name, unsigned int *typestring_offset);
static const var_t *find_array_or_string_in_struct(const type_t *type);
static unsigned int write_string_tfs(FILE *file, const attr_list_t *attrs, int toplevel_attrs,
                                     type_t *type, enum type_context context,
                                     const char *name, unsigned int *typestring_offset);
static unsigned int write_type_tfs(FILE *file, const attr_list_t *attrs, int toplevel_attrs,
                                   type_t *type, const char *name,
                                   enum type_context context,
                                   unsigned int *typeformat_offset);
static unsigned int get_required_buffer_size_type( const type_t *type, const char *name,
        const attr_list_t *attrs, int toplevel_attrs, int toplevel_param, unsigned int *alignment );
static unsigned int get_function_buffer_size( const var_t *func, enum pass pass );

static const char *string_of_type(unsigned char type)
{
    switch (type)
    {
    case FC_BYTE: return "FC_BYTE";
    case FC_CHAR: return "FC_CHAR";
    case FC_SMALL: return "FC_SMALL";
    case FC_USMALL: return "FC_USMALL";
    case FC_WCHAR: return "FC_WCHAR";
    case FC_SHORT: return "FC_SHORT";
    case FC_USHORT: return "FC_USHORT";
    case FC_LONG: return "FC_LONG";
    case FC_ULONG: return "FC_ULONG";
    case FC_FLOAT: return "FC_FLOAT";
    case FC_HYPER: return "FC_HYPER";
    case FC_DOUBLE: return "FC_DOUBLE";
    case FC_ENUM16: return "FC_ENUM16";
    case FC_ENUM32: return "FC_ENUM32";
    case FC_IGNORE: return "FC_IGNORE";
    case FC_ERROR_STATUS_T: return "FC_ERROR_STATUS_T";
    case FC_RP: return "FC_RP";
    case FC_UP: return "FC_UP";
    case FC_OP: return "FC_OP";
    case FC_FP: return "FC_FP";
    case FC_ENCAPSULATED_UNION: return "FC_ENCAPSULATED_UNION";
    case FC_NON_ENCAPSULATED_UNION: return "FC_NON_ENCAPSULATED_UNION";
    case FC_STRUCT: return "FC_STRUCT";
    case FC_PSTRUCT: return "FC_PSTRUCT";
    case FC_CSTRUCT: return "FC_CSTRUCT";
    case FC_CPSTRUCT: return "FC_CPSTRUCT";
    case FC_CVSTRUCT: return "FC_CVSTRUCT";
    case FC_BOGUS_STRUCT: return "FC_BOGUS_STRUCT";
    case FC_SMFARRAY: return "FC_SMFARRAY";
    case FC_LGFARRAY: return "FC_LGFARRAY";
    case FC_SMVARRAY: return "FC_SMVARRAY";
    case FC_LGVARRAY: return "FC_LGVARRAY";
    case FC_CARRAY: return "FC_CARRAY";
    case FC_CVARRAY: return "FC_CVARRAY";
    case FC_BOGUS_ARRAY: return "FC_BOGUS_ARRAY";
    case FC_ALIGNM2: return "FC_ALIGNM2";
    case FC_ALIGNM4: return "FC_ALIGNM4";
    case FC_ALIGNM8: return "FC_ALIGNM8";
    case FC_POINTER: return "FC_POINTER";
    case FC_C_CSTRING: return "FC_C_CSTRING";
    case FC_C_WSTRING: return "FC_C_WSTRING";
    case FC_CSTRING: return "FC_CSTRING";
    case FC_WSTRING: return "FC_WSTRING";
    case FC_BYTE_COUNT_POINTER: return "FC_BYTE_COUNT_POINTER";
    case FC_TRANSMIT_AS: return "FC_TRANSMIT_AS";
    case FC_REPRESENT_AS: return "FC_REPRESENT_AS";
    case FC_IP: return "FC_IP";
    case FC_BIND_CONTEXT: return "FC_BIND_CONTEXT";
    case FC_BIND_GENERIC: return "FC_BIND_GENERIC";
    case FC_BIND_PRIMITIVE: return "FC_BIND_PRIMITIVE";
    case FC_AUTO_HANDLE: return "FC_AUTO_HANDLE";
    case FC_CALLBACK_HANDLE: return "FC_CALLBACK_HANDLE";
    case FC_STRUCTPAD1: return "FC_STRUCTPAD1";
    case FC_STRUCTPAD2: return "FC_STRUCTPAD2";
    case FC_STRUCTPAD3: return "FC_STRUCTPAD3";
    case FC_STRUCTPAD4: return "FC_STRUCTPAD4";
    case FC_STRUCTPAD5: return "FC_STRUCTPAD5";
    case FC_STRUCTPAD6: return "FC_STRUCTPAD6";
    case FC_STRUCTPAD7: return "FC_STRUCTPAD7";
    case FC_STRING_SIZED: return "FC_STRING_SIZED";
    case FC_NO_REPEAT: return "FC_NO_REPEAT";
    case FC_FIXED_REPEAT: return "FC_FIXED_REPEAT";
    case FC_VARIABLE_REPEAT: return "FC_VARIABLE_REPEAT";
    case FC_FIXED_OFFSET: return "FC_FIXED_OFFSET";
    case FC_VARIABLE_OFFSET: return "FC_VARIABLE_OFFSET";
    case FC_PP: return "FC_PP";
    case FC_EMBEDDED_COMPLEX: return "FC_EMBEDDED_COMPLEX";
    case FC_DEREFERENCE: return "FC_DEREFERENCE";
    case FC_DIV_2: return "FC_DIV_2";
    case FC_MULT_2: return "FC_MULT_2";
    case FC_ADD_1: return "FC_ADD_1";
    case FC_SUB_1: return "FC_SUB_1";
    case FC_CALLBACK: return "FC_CALLBACK";
    case FC_CONSTANT_IID: return "FC_CONSTANT_IID";
    case FC_END: return "FC_END";
    case FC_PAD: return "FC_PAD";
    case FC_USER_MARSHAL: return "FC_USER_MARSHAL";
    case FC_RANGE: return "FC_RANGE";
    case FC_INT3264: return "FC_INT3264";
    case FC_UINT3264: return "FC_UINT3264";
    default:
        error("string_of_type: unknown type 0x%02x\n", type);
        return NULL;
    }
}

unsigned char get_basic_fc(const type_t *type)
{
    int sign = type_basic_get_sign(type);
    switch (type_basic_get_type(type))
    {
    case TYPE_BASIC_INT8: return (sign <= 0 ? FC_SMALL : FC_USMALL);
    case TYPE_BASIC_INT16: return (sign <= 0 ? FC_SHORT : FC_USHORT);
    case TYPE_BASIC_INT32:
    case TYPE_BASIC_LONG: return (sign <= 0 ? FC_LONG : FC_ULONG);
    case TYPE_BASIC_INT64: return FC_HYPER;
    case TYPE_BASIC_INT: return (sign <= 0 ? FC_LONG : FC_ULONG);
    case TYPE_BASIC_INT3264: return (sign <= 0 ? FC_INT3264 : FC_UINT3264);
    case TYPE_BASIC_BYTE: return FC_BYTE;
    case TYPE_BASIC_CHAR: return FC_CHAR;
    case TYPE_BASIC_WCHAR: return FC_WCHAR;
    case TYPE_BASIC_HYPER: return FC_HYPER;
    case TYPE_BASIC_FLOAT: return FC_FLOAT;
    case TYPE_BASIC_DOUBLE: return FC_DOUBLE;
    case TYPE_BASIC_ERROR_STATUS_T: return FC_ERROR_STATUS_T;
    case TYPE_BASIC_HANDLE: return FC_BIND_PRIMITIVE;
    }
    return 0;
}

static unsigned char get_basic_fc_signed(const type_t *type)
{
    switch (type_basic_get_type(type))
    {
    case TYPE_BASIC_INT8: return FC_SMALL;
    case TYPE_BASIC_INT16: return FC_SHORT;
    case TYPE_BASIC_INT32: return FC_LONG;
    case TYPE_BASIC_INT64: return FC_HYPER;
    case TYPE_BASIC_INT: return FC_LONG;
    case TYPE_BASIC_INT3264: return FC_INT3264;
    case TYPE_BASIC_LONG: return FC_LONG;
    case TYPE_BASIC_BYTE: return FC_BYTE;
    case TYPE_BASIC_CHAR: return FC_CHAR;
    case TYPE_BASIC_WCHAR: return FC_WCHAR;
    case TYPE_BASIC_HYPER: return FC_HYPER;
    case TYPE_BASIC_FLOAT: return FC_FLOAT;
    case TYPE_BASIC_DOUBLE: return FC_DOUBLE;
    case TYPE_BASIC_ERROR_STATUS_T: return FC_ERROR_STATUS_T;
    case TYPE_BASIC_HANDLE: return FC_BIND_PRIMITIVE;
    }
    return 0;
}

static inline unsigned int clamp_align(unsigned int align)
{
    if(align > packing) align = packing;
    return align;
}

static unsigned char get_pointer_fc(const type_t *type, const attr_list_t *attrs,
        int toplevel_attrs, int toplevel_param)
{
    const type_t *t;
    int pointer_type;

    assert(is_ptr(type) || is_array(type));

    if (toplevel_attrs)
    {
        pointer_type = get_attrv(attrs, ATTR_POINTERTYPE);
        if (pointer_type)
            return pointer_type;
    }

    for (t = type; type_is_alias(t); t = type_alias_get_aliasee_type(t))
    {
        pointer_type = get_attrv(t->attrs, ATTR_POINTERTYPE);
        if (pointer_type)
            return pointer_type;
    }

    if (toplevel_param)
        return FC_RP;

    if ((pointer_type = get_attrv(current_iface->attrs, ATTR_POINTERDEFAULT)))
        return pointer_type;

    return FC_UP;
}

static unsigned char get_pointer_fc_context( const type_t *type, const attr_list_t *attrs,
                                             int toplevel_attrs, enum type_context context )
{
    int pointer_fc = get_pointer_fc(type, attrs, toplevel_attrs, context == TYPE_CONTEXT_TOPLEVELPARAM);

    if (pointer_fc == FC_UP && is_attr( attrs, ATTR_OUT ) &&
        (context == TYPE_CONTEXT_PARAM || context == TYPE_CONTEXT_RETVAL) && is_object( current_iface ))
        pointer_fc = FC_OP;

    return pointer_fc;
}

static unsigned char get_enum_fc(const type_t *type)
{
    assert(type_get_type(type) == TYPE_ENUM);
    if (is_aliaschain_attr(type, ATTR_V1ENUM))
        return FC_ENUM32;
    else
        return FC_ENUM16;
}

static type_t *get_user_type(const type_t *t, const char **pname)
{
    for (;;)
    {
        type_t *ut = get_attrp(t->attrs, ATTR_WIREMARSHAL);
        if (ut)
        {
            if (pname)
                *pname = t->name;
            return ut;
        }

        if (type_is_alias(t))
            t = type_alias_get_aliasee_type(t);
        else
            return NULL;
    }
}

static int is_user_type(const type_t *t)
{
    return get_user_type(t, NULL) != NULL;
}

enum typegen_type typegen_detect_type(const type_t *type, const attr_list_t *attrs, unsigned int flags)
{
    if (is_user_type(type))
        return TGT_USER_TYPE;

    if (is_aliaschain_attr(type, ATTR_CONTEXTHANDLE))
        return TGT_CTXT_HANDLE;

    if (!(flags & TDT_IGNORE_STRINGS) && is_string_type(attrs, type))
        return TGT_STRING;

    switch (type_get_type(type))
    {
    case TYPE_BASIC:
        if (!(flags & TDT_IGNORE_RANGES) &&
            (is_attr(attrs, ATTR_RANGE) || is_aliaschain_attr(type, ATTR_RANGE)))
            return TGT_RANGE;
        return TGT_BASIC;
    case TYPE_ENUM:
        if (!(flags & TDT_IGNORE_RANGES) &&
            (is_attr(attrs, ATTR_RANGE) || is_aliaschain_attr(type, ATTR_RANGE)))
            return TGT_RANGE;
        return TGT_ENUM;
    case TYPE_POINTER:
        if (type_get_type(type_pointer_get_ref_type(type)) == TYPE_INTERFACE ||
            type_get_type(type_pointer_get_ref_type(type)) == TYPE_RUNTIMECLASS ||
            type_get_type(type_pointer_get_ref_type(type)) == TYPE_DELEGATE ||
            (type_get_type(type_pointer_get_ref_type(type)) == TYPE_VOID && is_attr(attrs, ATTR_IIDIS)))
            return TGT_IFACE_POINTER;
        else if (is_aliaschain_attr(type_pointer_get_ref_type(type), ATTR_CONTEXTHANDLE))
            return TGT_CTXT_HANDLE_POINTER;
        else
            return TGT_POINTER;
    case TYPE_STRUCT:
        return TGT_STRUCT;
    case TYPE_ENCAPSULATED_UNION:
    case TYPE_UNION:
        return TGT_UNION;
    case TYPE_ARRAY:
        return TGT_ARRAY;
    case TYPE_FUNCTION:
    case TYPE_COCLASS:
    case TYPE_INTERFACE:
    case TYPE_MODULE:
    case TYPE_VOID:
    case TYPE_ALIAS:
    case TYPE_BITFIELD:
    case TYPE_RUNTIMECLASS:
    case TYPE_DELEGATE:
        break;
    case TYPE_APICONTRACT:
    case TYPE_PARAMETERIZED_TYPE:
    case TYPE_PARAMETER:
        /* not supposed to be here */
        assert(0);
        break;
    }
    return TGT_INVALID;
}

static int cant_be_null(const var_t *v)
{
    switch (typegen_detect_type(v->declspec.type, v->attrs, TDT_IGNORE_STRINGS))
    {
    case TGT_ARRAY:
        if (!type_array_is_decl_as_ptr( v->declspec.type )) return 0;
        /* fall through */
    case TGT_POINTER:
        return (get_pointer_fc(v->declspec.type, v->attrs, TRUE, TRUE) == FC_RP);
    case TGT_CTXT_HANDLE_POINTER:
        return TRUE;
    default:
        return 0;
    }

}

static int get_padding(const var_list_t *fields)
{
    unsigned short offset = 0;
    unsigned int salign = 1;
    const var_t *f;

    if (!fields)
        return 0;

    LIST_FOR_EACH_ENTRY(f, fields, const var_t, entry)
    {
        type_t *ft = f->declspec.type;
        unsigned int align = 0;
        unsigned int size = type_memsize_and_alignment(ft, &align);
        align = clamp_align(align);
        if (align > salign) salign = align;
        offset = ROUND_SIZE(offset, align);
        offset += size;
    }

    return ROUNDING(offset, salign);
}

static unsigned int get_stack_size( const var_t *var, unsigned int *stack_align, int *by_value )
{
    unsigned int stack_size, align = 0;
    int by_val = 0;

    switch (typegen_detect_type( var->declspec.type, var->attrs, TDT_ALL_TYPES ))
    {
    case TGT_BASIC:
        if (target.cpu == CPU_ARM)
        {
            switch (type_basic_get_type( var->declspec.type ))
            {
            case TYPE_BASIC_FLOAT:
            case TYPE_BASIC_DOUBLE:
            case TYPE_BASIC_INT64:
            case TYPE_BASIC_HYPER:
                align = 8;
                break;
            default:
                break;
            }
        }
        /* fall through */
    case TGT_ENUM:
    case TGT_RANGE:
    case TGT_STRUCT:
    case TGT_UNION:
    case TGT_USER_TYPE:
        stack_size = type_memsize_and_alignment( var->declspec.type, &align );
        switch (target.cpu)
        {
        case CPU_x86_64:
        case CPU_ARM64EC:
            by_val = (stack_size == 1 || stack_size == 2 || stack_size == 4 || stack_size == 8);
            break;
        case CPU_ARM64:
            by_val = (stack_size <= 2 * pointer_size);
            break;
        case CPU_ARM:
            by_val = 1;
            break;
        case CPU_i386:
            align = pointer_size;
            by_val = 1;
            break;
        }
        break;
    default:
        break;
    }
    if (align < pointer_size) align = pointer_size;
    if (!by_val) stack_size = align = pointer_size;

    if (by_value) *by_value = by_val;
    if (stack_align) *stack_align = align;
    return ROUND_SIZE( stack_size, align );
}

static unsigned char get_contexthandle_flags( const type_t *iface, const attr_list_t *attrs,
                                              const type_t *type, int is_return )
{
    unsigned char flags = 0;
    int is_out;

    if (is_attr(iface->attrs, ATTR_STRICTCONTEXTHANDLE)) flags |= NDR_STRICT_CONTEXT_HANDLE;

    if (is_ptr(type) &&
        !is_attr( type->attrs, ATTR_CONTEXTHANDLE ) &&
        !is_attr( attrs, ATTR_CONTEXTHANDLE ))
        flags |= HANDLE_PARAM_IS_VIA_PTR;

    if (is_return) return flags | HANDLE_PARAM_IS_OUT | HANDLE_PARAM_IS_RETURN;

    is_out = is_attr(attrs, ATTR_OUT);
    if (is_attr(attrs, ATTR_IN) || !is_out)
    {
        flags |= HANDLE_PARAM_IS_IN;
        if (!is_out) flags |= NDR_CONTEXT_HANDLE_CANNOT_BE_NULL;
    }
    if (is_out) flags |= HANDLE_PARAM_IS_OUT;

    return flags;
}

static unsigned int get_rpc_flags( const attr_list_t *attrs )
{
    unsigned int flags = 0;

    if (is_attr( attrs, ATTR_IDEMPOTENT )) flags |= 0x0001;
    if (is_attr( attrs, ATTR_BROADCAST )) flags |= 0x0002;
    if (is_attr( attrs, ATTR_MAYBE )) flags |= 0x0004;
    if (is_attr( attrs, ATTR_MESSAGE )) flags |= 0x0100;
    if (is_attr( attrs, ATTR_ASYNC )) flags |= 0x4000;
    return flags;
}

unsigned char get_struct_fc(const type_t *type)
{
  int has_pointer = 0;
  int has_conformance = 0;
  int has_variance = 0;
  var_t *field;
  var_list_t *fields;

  fields = type_struct_get_fields(type);

  if (get_padding(fields))
    return FC_BOGUS_STRUCT;

  if (fields) LIST_FOR_EACH_ENTRY( field, fields, var_t, entry )
  {
    type_t *t = field->declspec.type;
    enum typegen_type typegen_type;

    typegen_type = typegen_detect_type(t, field->attrs, TDT_IGNORE_STRINGS);

    if (typegen_type == TGT_ARRAY && !type_array_is_decl_as_ptr(t))
    {
        if (is_string_type(field->attrs, field->declspec.type))
        {
            if (is_conformant_array(t))
                has_conformance = 1;
            has_variance = 1;
            continue;
        }

        if (is_array(type_array_get_element_type(field->declspec.type)))
            return FC_BOGUS_STRUCT;

        if (type_array_has_conformance(field->declspec.type))
        {
            has_conformance = 1;
            if (list_next(fields, &field->entry))
                error_loc("field '%s' deriving from a conformant array must be the last field in the structure\n",
                        field->name);
        }
        if (type_array_has_variance(t))
            has_variance = 1;

        t = type_array_get_element_type(t);
        typegen_type = typegen_detect_type(t, field->attrs, TDT_IGNORE_STRINGS);
    }

    switch (typegen_type)
    {
    case TGT_USER_TYPE:
    case TGT_IFACE_POINTER:
        return FC_BOGUS_STRUCT;
    case TGT_BASIC:
        if (type_basic_get_type(t) == TYPE_BASIC_INT3264 && pointer_size != 4)
            return FC_BOGUS_STRUCT;
        break;
    case TGT_ENUM:
        if (get_enum_fc(t) == FC_ENUM16)
            return FC_BOGUS_STRUCT;
        break;
    case TGT_POINTER:
    case TGT_ARRAY:
        if (get_pointer_fc(t, field->attrs, TRUE, FALSE) == FC_RP || pointer_size != 4)
            return FC_BOGUS_STRUCT;
        has_pointer = 1;
        break;
    case TGT_UNION:
        return FC_BOGUS_STRUCT;
    case TGT_STRUCT:
    {
        unsigned char fc = get_struct_fc(t);
        switch (fc)
        {
        case FC_STRUCT:
            break;
        case FC_CVSTRUCT:
            has_conformance = 1;
            has_variance = 1;
            has_pointer = 1;
            break;

        case FC_CPSTRUCT:
            has_conformance = 1;
            if (list_next( fields, &field->entry ))
                error_loc("field '%s' deriving from a conformant array must be the last field in the structure\n",
                        field->name);
            has_pointer = 1;
            break;

        case FC_CSTRUCT:
            has_conformance = 1;
            if (list_next( fields, &field->entry ))
                error_loc("field '%s' deriving from a conformant array must be the last field in the structure\n",
                      field->name);
            break;

        case FC_PSTRUCT:
            has_pointer = 1;
            break;

        default:
            error_loc("Unknown struct member %s with type (0x%02x)\n", field->name, fc);
            /* fallthru - treat it as complex */

        /* as soon as we see one of these these members, it's bogus... */
        case FC_BOGUS_STRUCT:
            return FC_BOGUS_STRUCT;
        }
        break;
    }
    case TGT_RANGE:
        return FC_BOGUS_STRUCT;
    case TGT_STRING:
        /* shouldn't get here because of TDT_IGNORE_STRINGS above. fall through */
    case TGT_INVALID:
    case TGT_CTXT_HANDLE:
    case TGT_CTXT_HANDLE_POINTER:
        /* checking after parsing should mean that we don't get here. if we do,
         * it's a checker bug */
        assert(0);
    }
  }

  if( has_variance )
  {
    if ( has_conformance )
      return FC_CVSTRUCT;
    else
      return FC_BOGUS_STRUCT;
  }
  if( has_conformance && has_pointer )
    return FC_CPSTRUCT;
  if( has_conformance )
    return FC_CSTRUCT;
  if( has_pointer )
    return FC_PSTRUCT;
  return FC_STRUCT;
}

static unsigned char get_array_fc(const type_t *type, const attr_list_t *attrs)
{
    unsigned char fc;
    const expr_t *size_is;
    const type_t *elem_type;

    elem_type = type_array_get_element_type(type);
    size_is = type_array_get_conformance(type);

    if (!size_is)
    {
        unsigned int size = type_memsize(elem_type);
        if (size * type_array_get_dim(type) > 0xffffuL)
            fc = FC_LGFARRAY;
        else
            fc = FC_SMFARRAY;
    }
    else
        fc = FC_CARRAY;

    if (type_array_has_variance(type))
    {
        if (fc == FC_SMFARRAY)
            fc = FC_SMVARRAY;
        else if (fc == FC_LGFARRAY)
            fc = FC_LGVARRAY;
        else if (fc == FC_CARRAY)
            fc = FC_CVARRAY;
    }

    switch (typegen_detect_type(elem_type, attrs, TDT_IGNORE_STRINGS))
    {
    case TGT_USER_TYPE:
        fc = FC_BOGUS_ARRAY;
        break;
    case TGT_BASIC:
        if (type_basic_get_type(elem_type) == TYPE_BASIC_INT3264 &&
            pointer_size != 4)
            fc = FC_BOGUS_ARRAY;
        break;
    case TGT_STRUCT:
        switch (get_struct_fc(elem_type))
        {
        case FC_BOGUS_STRUCT:
            fc = FC_BOGUS_ARRAY;
            break;
        }
        break;
    case TGT_ENUM:
        /* is 16-bit enum - if so, wire size differs from mem size and so
         * the array cannot be block copied, which means the array is complex */
        if (get_enum_fc(elem_type) == FC_ENUM16)
            fc = FC_BOGUS_ARRAY;
        break;
    case TGT_UNION:
    case TGT_IFACE_POINTER:
        fc = FC_BOGUS_ARRAY;
        break;
    case TGT_POINTER:
        /* ref pointers cannot just be block copied. unique pointers to
         * interfaces need special treatment. either case means the array is
         * complex */
        if (get_pointer_fc(elem_type, attrs, FALSE, FALSE) == FC_RP || pointer_size != 4)
            fc = FC_BOGUS_ARRAY;
        break;
    case TGT_RANGE:
        fc = FC_BOGUS_ARRAY;
        break;
    case TGT_CTXT_HANDLE:
    case TGT_CTXT_HANDLE_POINTER:
    case TGT_STRING:
    case TGT_INVALID:
    case TGT_ARRAY:
        /* nothing to do for everything else */
        break;
    }

    return fc;
}

static int is_non_complex_struct(const type_t *type)
{
    return (type_get_type(type) == TYPE_STRUCT &&
            get_struct_fc(type) != FC_BOGUS_STRUCT);
}

static int type_has_pointers(const type_t *type, const attr_list_t *attrs)
{
    switch (typegen_detect_type(type, attrs, TDT_IGNORE_STRINGS))
    {
    case TGT_USER_TYPE:
        return FALSE;
    case TGT_POINTER:
        return TRUE;
    case TGT_ARRAY:
        return type_array_is_decl_as_ptr(type) || type_has_pointers(type_array_get_element_type(type), attrs);
    case TGT_STRUCT:
    {
        var_list_t *fields = type_struct_get_fields(type);
        const var_t *field;
        if (fields) LIST_FOR_EACH_ENTRY( field, fields, const var_t, entry )
        {
            if (type_has_pointers(field->declspec.type, attrs))
                return TRUE;
        }
        break;
    }
    case TGT_UNION:
    {
        var_list_t *fields;
        const var_t *field;
        fields = type_union_get_cases(type);
        if (fields) LIST_FOR_EACH_ENTRY( field, fields, const var_t, entry )
        {
            if (field->declspec.type && type_has_pointers(field->declspec.type, attrs))
                return TRUE;
        }
        break;
    }
    case TGT_CTXT_HANDLE:
    case TGT_CTXT_HANDLE_POINTER:
    case TGT_STRING:
    case TGT_IFACE_POINTER:
    case TGT_BASIC:
    case TGT_ENUM:
    case TGT_RANGE:
    case TGT_INVALID:
        break;
    }

    return FALSE;
}

struct visited_struct_array
{
    const type_t **structs;
    size_t count;
    size_t capacity;
};

static inline int array_reserve(void **elements, size_t *capacity, size_t count, size_t size)
{
    size_t new_capacity, max_capacity;
    void *new_elements;

    if (count <= *capacity)
        return TRUE;

    max_capacity = ~(size_t)0 / size;
    if (count > max_capacity)
        return FALSE;

    new_capacity = max(4, *capacity);
    while (new_capacity < count && new_capacity <= max_capacity / 2)
        new_capacity *= 2;
    if (new_capacity < count)
        new_capacity = max_capacity;

    if (!(new_elements = realloc(*elements, new_capacity * size)))
        return FALSE;

    *elements = new_elements;
    *capacity = new_capacity;
    return TRUE;
}

static int type_has_full_pointer_recurse(const type_t *type, const attr_list_t *attrs, int toplevel_attrs,
                                 int toplevel_param, struct visited_struct_array *visited_structs)
{
    switch (typegen_detect_type(type, attrs, TDT_IGNORE_STRINGS))
    {
    case TGT_USER_TYPE:
        return FALSE;
    case TGT_POINTER:
        if (get_pointer_fc(type, attrs, toplevel_attrs, toplevel_param) == FC_FP)
            return TRUE;
        else
            return type_has_full_pointer_recurse(type_pointer_get_ref_type(type), attrs, FALSE, FALSE, visited_structs);
    case TGT_ARRAY:
        if (get_pointer_fc(type, attrs, toplevel_attrs, toplevel_param) == FC_FP)
            return TRUE;
        else
            return type_has_full_pointer_recurse(type_array_get_element_type(type), attrs, FALSE, FALSE, visited_structs);
    case TGT_STRUCT:
    {
        unsigned int i;
        int ret = FALSE;
        var_list_t *fields = type_struct_get_fields(type);
        const var_t *field;

        for (i = 0; i < visited_structs->count; i++)
        {
            if (visited_structs->structs[i] == type)
            {
                /* Found struct we visited already, abort to prevent infinite loop.
                 * Can't be at the first struct we visit, so we can skip cleanup and just return */
               return FALSE;
            }
        }

        array_reserve((void**)&visited_structs->structs, &visited_structs->capacity, visited_structs->count + 1, sizeof(struct type_t*));
        visited_structs->structs[visited_structs->count] = type;

        visited_structs->count++;
        if (fields) LIST_FOR_EACH_ENTRY( field, fields, const var_t, entry )
        {
            if (type_has_full_pointer_recurse(field->declspec.type, field->attrs, TRUE, FALSE, visited_structs))
            {
                ret = TRUE;
                break;
            }
        }
        visited_structs->count--;
        return ret;
    }
    case TGT_UNION:
    {
        var_list_t *fields;
        const var_t *field;
        fields = type_union_get_cases(type);
        if (fields) LIST_FOR_EACH_ENTRY( field, fields, const var_t, entry )
        {
            if (field->declspec.type && type_has_full_pointer_recurse(field->declspec.type, field->attrs, TRUE, FALSE, visited_structs))
                return TRUE;
        }
        break;
    }
    case TGT_CTXT_HANDLE:
    case TGT_CTXT_HANDLE_POINTER:
    case TGT_STRING:
    case TGT_IFACE_POINTER:
    case TGT_BASIC:
    case TGT_ENUM:
    case TGT_RANGE:
    case TGT_INVALID:
        break;
    }

    return FALSE;
}

static int type_has_full_pointer(const type_t *type, const attr_list_t *attrs, int toplevel_attrs, int toplevel_param)
{
    int ret;
    struct visited_struct_array visited_structs = {0};
    ret = type_has_full_pointer_recurse(type, attrs, toplevel_attrs, toplevel_param, &visited_structs);
    free(visited_structs.structs);
    return ret;
}

static unsigned short user_type_offset(const char *name)
{
    user_type_t *ut;
    unsigned short off = 0;
    LIST_FOR_EACH_ENTRY(ut, &user_type_list, user_type_t, entry)
    {
        if (strcmp(name, ut->name) == 0)
            return off;
        ++off;
    }
    error("user_type_offset: couldn't find type (%s)\n", name);
    return 0;
}

static void update_tfsoff(type_t *type, unsigned int offset, FILE *file)
{
    type->typestring_offset = offset;
    if (file) type->tfswrite = FALSE;
}

static void guard_rec(type_t *type)
{
    /* types that contain references to themselves (like a linked list),
       need to be shielded from infinite recursion when writing embedded
       types  */
    if (type->typestring_offset)
        type->tfswrite = FALSE;
    else
        type->typestring_offset = 1;
}

static int is_embedded_complex(const type_t *type, const attr_list_t *attrs)
{
    switch (typegen_detect_type(type, attrs, TDT_IGNORE_STRINGS))
    {
    case TGT_USER_TYPE:
    case TGT_STRUCT:
    case TGT_UNION:
    case TGT_ARRAY:
    case TGT_IFACE_POINTER:
        return TRUE;
    default:
        return FALSE;
    }
}

static const char *get_context_handle_type_name(const type_t *type)
{
    const type_t *t;
    for (t = type;
         is_ptr(t) || type_is_alias(t);
         t = type_is_alias(t) ? type_alias_get_aliasee_type(t) : type_pointer_get_ref_type(t))
        if (is_attr(t->attrs, ATTR_CONTEXTHANDLE))
            return t->name;
    assert(0);
    return NULL;
}

#define WRITE_FCTYPE(file, fctype, typestring_offset) \
    do { \
        if (file) \
            fprintf(file, "/* %2u */\n", typestring_offset); \
        print_file((file), 2, "0x%02x,\t/* " #fctype " */\n", fctype); \
    } \
    while (0)

static void print_file(FILE *file, int indent, const char *format, ...) __attribute__((format (printf, 3, 4)));
static void print_file(FILE *file, int indent, const char *format, ...)
{
    va_list va;
    va_start(va, format);
    print(file, indent, format, va);
    va_end(va);
}

void print(FILE *file, int indent, const char *format, va_list va)
{
    if (file)
    {
        if (format[0] != '\n')
            while (0 < indent--)
                fprintf(file, "    ");
        vfprintf(file, format, va);
    }
}


static void write_var_init(FILE *file, int indent, const type_t *t, const char *n, const char *local_var_prefix)
{
    if (decl_indirect(t))
    {
        print_file(file, indent, "MIDL_memset(&%s%s, 0, sizeof(%s%s));\n",
                   local_var_prefix, n, local_var_prefix, n);
        print_file(file, indent, "%s_p_%s = &%s%s;\n", local_var_prefix, n, local_var_prefix, n);
    }
    else if (is_ptr(t) || is_array(t))
        print_file(file, indent, "%s%s = 0;\n", local_var_prefix, n);
}

void write_parameters_init(FILE *file, int indent, const var_t *func, const char *local_var_prefix)
{
    const var_t *var = type_function_get_retval(func->declspec.type);

    if (!is_void(var->declspec.type))
        write_var_init(file, indent, var->declspec.type, var->name, local_var_prefix);

    if (!type_function_get_args(func->declspec.type))
        return;

    LIST_FOR_EACH_ENTRY( var, type_function_get_args(func->declspec.type), const var_t, entry )
        write_var_init(file, indent, var->declspec.type, var->name, local_var_prefix);

    fprintf(file, "\n");
}

static void write_formatdesc(FILE *f, int indent, const char *str)
{
    print_file(f, indent, "typedef struct _MIDL_%s_FORMAT_STRING\n", str);
    print_file(f, indent, "{\n");
    print_file(f, indent + 1, "short Pad;\n");
    print_file(f, indent + 1, "unsigned char Format[%s_FORMAT_STRING_SIZE];\n", str);
    print_file(f, indent, "} MIDL_%s_FORMAT_STRING;\n", str);
    print_file(f, indent, "\n");
}

void write_formatstringsdecl(FILE *f, int indent, const statement_list_t *stmts, type_pred_t pred)
{
    clear_all_offsets();

    print_file(f, indent, "#define TYPE_FORMAT_STRING_SIZE %d\n",
               get_size_typeformatstring(stmts, pred));

    print_file(f, indent, "#define PROC_FORMAT_STRING_SIZE %d\n",
               get_size_procformatstring(stmts, pred));

    fprintf(f, "\n");
    write_formatdesc(f, indent, "TYPE");
    write_formatdesc(f, indent, "PROC");
    fprintf(f, "\n");
    print_file(f, indent, "static const MIDL_TYPE_FORMAT_STRING __MIDL_TypeFormatString;\n");
    print_file(f, indent, "static const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString;\n");
    print_file(f, indent, "\n");
}

int decl_indirect(const type_t *t)
{
    if (is_user_type(t))
        return TRUE;
    return (type_get_type(t) != TYPE_BASIC &&
            type_get_type(t) != TYPE_ENUM &&
            type_get_type(t) != TYPE_POINTER &&
            type_get_type(t) != TYPE_ARRAY);
}

static unsigned char get_parameter_fc( const var_t *var, int is_return, unsigned short *flags,
                                       unsigned int *stack_size, unsigned int *stack_align,
                                       unsigned int *typestring_offset )
{
    unsigned int alignment, server_size = 0, buffer_size = 0;
    unsigned char fc = 0;
    int is_byval;
    int is_in = is_attr(var->attrs, ATTR_IN);
    int is_out = is_attr(var->attrs, ATTR_OUT);

    if (is_return) is_out = TRUE;
    else if (!is_in && !is_out) is_in = TRUE;

    *flags = 0;
    *stack_size = get_stack_size( var, stack_align, &is_byval );
    *typestring_offset = var->typestring_offset;

    if (is_in)     *flags |= IsIn;
    if (is_out)    *flags |= IsOut;
    if (is_return) *flags |= IsReturn;

    if (!is_string_type( var->attrs, var->declspec.type ))
        buffer_size = get_required_buffer_size_type( var->declspec.type, NULL, var->attrs, TRUE, TRUE, &alignment );

    switch (typegen_detect_type( var->declspec.type, var->attrs, TDT_ALL_TYPES ))
    {
    case TGT_BASIC:
        *flags |= IsBasetype;
        fc = get_basic_fc_signed( var->declspec.type );
        if (fc == FC_BIND_PRIMITIVE) buffer_size = 4;  /* actually 0 but avoids setting MustSize */
        break;
    case TGT_ENUM:
        *flags |= IsBasetype;
        fc = get_enum_fc( var->declspec.type );
        break;
    case TGT_RANGE:
        *flags |= IsByValue;
        break;
    case TGT_STRUCT:
    case TGT_UNION:
    case TGT_USER_TYPE:
        *flags |= MustFree | (is_byval ? IsByValue : IsSimpleRef);
        break;
    case TGT_IFACE_POINTER:
        *flags |= MustFree;
        break;
    case TGT_ARRAY:
        *flags |= MustFree;
        if (type_array_is_decl_as_ptr(var->declspec.type)
                && type_array_get_ptr_tfsoff(var->declspec.type)
                && get_pointer_fc(var->declspec.type, var->attrs, TRUE, !is_return) == FC_RP)
        {
            *typestring_offset = var->declspec.type->typestring_offset;
            *flags |= IsSimpleRef;
        }
        break;
    case TGT_STRING:
        *flags |= MustFree;
        if (is_declptr( var->declspec.type ) && get_pointer_fc( var->declspec.type, var->attrs, TRUE, !is_return ) == FC_RP)
        {
            /* skip over pointer description straight to string description */
            if (is_conformant_array( var->declspec.type )) *typestring_offset += 4;
            else *typestring_offset += 2;
            *flags |= IsSimpleRef;
        }
        break;
    case TGT_CTXT_HANDLE_POINTER:
        *flags |= IsSimpleRef;
        *typestring_offset += 4;
        /* fall through */
    case TGT_CTXT_HANDLE:
        buffer_size = 20;
        break;
    case TGT_POINTER:
        if (get_pointer_fc( var->declspec.type, var->attrs, TRUE, !is_return ) == FC_RP)
        {
            const type_t *ref = type_pointer_get_ref_type( var->declspec.type );

            if (!is_string_type( var->attrs, ref ))
                buffer_size = get_required_buffer_size_type( ref, NULL, var->attrs, FALSE, TRUE, &alignment );

            switch (typegen_detect_type( ref, var->attrs, TDT_ALL_TYPES ))
            {
            case TGT_BASIC:
                *flags |= IsSimpleRef | IsBasetype;
                fc = get_basic_fc( ref );
                if (!is_in && is_out) server_size = pointer_size;
                break;
            case TGT_ENUM:
                if ((fc = get_enum_fc( ref )) == FC_ENUM32)
                {
                    *flags |= IsSimpleRef | IsBasetype;
                    if (!is_in && is_out) server_size = pointer_size;
                }
                else
                {
                    server_size = pointer_size;
                }
                break;
            case TGT_UNION:
            case TGT_USER_TYPE:
            case TGT_RANGE:
                *flags |= MustFree | IsSimpleRef;
                *typestring_offset = ref->typestring_offset;
                if (!is_in && is_out) server_size = type_memsize( ref );
                break;
            case TGT_ARRAY:
                *flags |= MustFree;
                if (!type_array_is_decl_as_ptr(ref))
                {
                    *flags |= IsSimpleRef;
                    *typestring_offset = ref->typestring_offset;
                }
                if (!is_in && is_out) server_size = type_memsize( ref );
                break;
            case TGT_STRING:
            case TGT_POINTER:
            case TGT_CTXT_HANDLE:
            case TGT_CTXT_HANDLE_POINTER:
                *flags |= MustFree;
                server_size = pointer_size;
                break;
            case TGT_IFACE_POINTER:
                *flags |= MustFree;
                if (is_in && is_out) server_size = pointer_size;
                break;
            case TGT_STRUCT:
                *flags |= IsSimpleRef | MustFree;
                *typestring_offset = ref->typestring_offset;
                switch (get_struct_fc(ref))
                {
                case FC_STRUCT:
                case FC_PSTRUCT:
                case FC_BOGUS_STRUCT:
                    if (!is_in && is_out) server_size = type_memsize( ref );
                    break;
                default:
                    break;
                }
                break;
            case TGT_INVALID:
                assert(0);
            }
        }
        else  /* not ref pointer */
        {
            *flags |= MustFree;
        }
        break;
    case TGT_INVALID:
        assert(0);
    }

    if (!buffer_size) *flags |= MustSize;

    if (server_size)
    {
        server_size = (server_size + 7) / 8;
        if (server_size < 8) *flags |= server_size << 13;
    }
    return fc;
}

static unsigned char get_func_oi2_flags( const var_t *func )
{
    const var_t *var;
    var_list_t *args = type_function_get_args( func->declspec.type );
    var_t *retval = type_function_get_retval( func->declspec.type );
    unsigned char oi2_flags = 0x40;  /* HasExtensions */
    unsigned short flags;
    unsigned int stack_size, stack_align, typestring_offset;

    if (args) LIST_FOR_EACH_ENTRY( var, args, const var_t, entry )
    {
        get_parameter_fc( var, 0, &flags, &stack_size, &stack_align, &typestring_offset );
        if (flags & MustSize)
        {
            if (flags & IsIn) oi2_flags |= 0x02; /* ClientMustSize */
            if (flags & IsOut) oi2_flags |= 0x01;  /* ServerMustSize */
        }
    }

    if (!is_void( retval->declspec.type ))
    {
        oi2_flags |= 0x04;  /* HasRet */
        get_parameter_fc( retval, 1, &flags, &stack_size, &stack_align, &typestring_offset );
        if (flags & MustSize) oi2_flags |= 0x01;  /* ServerMustSize */
    }
    return oi2_flags;
}

static unsigned int write_new_procformatstring_type(FILE *file, int indent, const var_t *var,
                                                    int is_return, unsigned int *stack_offset)
{
    char buffer[128];
    unsigned int stack_size, stack_align, typestring_offset;
    unsigned short flags;
    unsigned char fc = get_parameter_fc( var, is_return, &flags, &stack_size, &stack_align, &typestring_offset );

    *stack_offset = ROUND_SIZE( *stack_offset, stack_align );
    strcpy( buffer, "/* flags:" );
    if (flags & MustSize) strcat( buffer, " must size," );
    if (flags & MustFree) strcat( buffer, " must free," );
    if (flags & IsPipe) strcat( buffer, " pipe," );
    if (flags & IsIn) strcat( buffer, " in," );
    if (flags & IsOut) strcat( buffer, " out," );
    if (flags & IsReturn) strcat( buffer, " return," );
    if (flags & IsBasetype) strcat( buffer, " base type," );
    if (flags & IsByValue) strcat( buffer, " by value," );
    if (flags & IsSimpleRef) strcat( buffer, " simple ref," );
    if (flags >> 13) snprintf( buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), " srv size=%u,", (flags >> 13) * 8 );
    strcpy( buffer + strlen( buffer ) - 1, " */" );
    print_file( file, indent, "NdrFcShort(0x%hx),\t%s\n", flags, buffer );
    print_file( file, indent, "NdrFcShort(0x%x),	/* stack offset = %u */\n",
                *stack_offset, *stack_offset );
    if (flags & IsBasetype)
    {
        print_file( file, indent, "0x%02x,	/* %s */\n", fc, string_of_type(fc) );
        print_file( file, indent, "0x0,\n" );
    }
    else
        print_file( file, indent, "NdrFcShort(0x%x),	/* type offset = %u */\n",
                    typestring_offset, typestring_offset );
    *stack_offset += stack_size;
    return 6;
}

static unsigned int write_old_procformatstring_type(FILE *file, int indent, const var_t *var,
                                                    int is_return)
{
    unsigned int size;

    int is_in = is_attr(var->attrs, ATTR_IN);
    int is_out = is_attr(var->attrs, ATTR_OUT);

    if (!is_in && !is_out) is_in = TRUE;

    if (type_get_type(var->declspec.type) == TYPE_BASIC ||
        type_get_type(var->declspec.type) == TYPE_ENUM)
    {
        unsigned char fc;

        if (is_return)
            print_file(file, indent, "0x53,    /* FC_RETURN_PARAM_BASETYPE */\n");
        else
            print_file(file, indent, "0x4e,    /* FC_IN_PARAM_BASETYPE */\n");

        if (type_get_type(var->declspec.type) == TYPE_ENUM)
        {
            fc = get_enum_fc(var->declspec.type);
        }
        else
        {
            fc = get_basic_fc_signed(var->declspec.type);

            if (fc == FC_BIND_PRIMITIVE)
                fc = FC_IGNORE;
        }

        print_file(file, indent, "0x%02x,    /* %s */\n",
                   fc, string_of_type(fc));
        size = 2; /* includes param type prefix */
    }
    else
    {
        unsigned short offset = var->typestring_offset;

        if (is_array(var->declspec.type)
                && type_array_is_decl_as_ptr(var->declspec.type)
                && type_array_get_ptr_tfsoff(var->declspec.type))
            offset = var->declspec.type->typestring_offset;

        if (is_return)
            print_file(file, indent, "0x52,    /* FC_RETURN_PARAM */\n");
        else if (is_in && is_out)
            print_file(file, indent, "0x50,    /* FC_IN_OUT_PARAM */\n");
        else if (is_out)
            print_file(file, indent, "0x51,    /* FC_OUT_PARAM */\n");
        else
            print_file(file, indent, "0x4d,    /* FC_IN_PARAM */\n");

        size = get_stack_size( var, NULL, NULL );
        print_file(file, indent, "0x%02x,\n", size / pointer_size );
        print_file(file, indent, "NdrFcShort(0x%x),	/* type offset = %u */\n", offset, offset);
        size = 4; /* includes param type prefix */
    }
    return size;
}

int is_interpreted_func( const type_t *iface, const var_t *func )
{
    const char *str;
    const type_t *ret_type = type_function_get_rettype( func->declspec.type );

    if (type_get_type( ret_type ) == TYPE_BASIC)
    {
        switch (type_basic_get_type( ret_type ))
        {
        case TYPE_BASIC_INT64:
        case TYPE_BASIC_HYPER:
            /* return value must fit in a long_ptr */
            if (pointer_size < 8) return 0;
            break;
        case TYPE_BASIC_FLOAT:
        case TYPE_BASIC_DOUBLE:
            /* floating point values can't be returned */
            return 0;
        default:
            break;
        }
    }
    if ((str = get_attrp( func->attrs, ATTR_OPTIMIZE ))) return !strcmp( str, "i" );
    if ((str = get_attrp( iface->attrs, ATTR_OPTIMIZE ))) return !strcmp( str, "i" );
    return interpreted_mode;
}

/* replace consecutive params code by a repeat sequence: 0x9d code<1> repeat_count<2> */
static unsigned int compress_params_array( unsigned char *params, unsigned int count )
{
    unsigned int i, j;

    for (i = 0; i + 4 <= count; i++)
    {
        for (j = 1; i + j < count; j++) if (params[i + j] != params[i]) break;
        if (j < 4) continue;
        params[i] = 0x9d;
        params[i + 2] = j & 0xff;
        params[i + 3] = j >> 8;
        memmove( params + i + 4, params + i + j, count - (i + j) );
        count -= j - 4;
        i += 3;
    }
    return count;
}

/* fill the parameters array for the procedure extra data on ARM platforms */
static unsigned int fill_params_array( const type_t *iface, const var_t *func,
                                       unsigned char *params, unsigned int count )
{
    unsigned int reg_count = 0, float_count = 0, double_count = 0, stack_pos = 0, offset = 0;
    var_list_t *args = type_function_get_args( func->declspec.type );
    enum type_basic_type type;
    unsigned int size, pos, align;
    var_t *var;

    memset( params, 0x9f /* padding */, count );

    if (is_object( iface ))
    {
        params[0] = 0x80 + reg_count++;
        offset += pointer_size;
    }

    if (args) LIST_FOR_EACH_ENTRY( var, args, var_t, entry )
    {
        type = TYPE_BASIC_LONG;
        if (type_get_type( var->declspec.type ) == TYPE_BASIC)
            type = type_basic_get_type( var->declspec.type );

        size = get_stack_size( var, &align, NULL );
        offset = ROUND_SIZE( offset, align );
        pos = offset / pointer_size;

        if (target.cpu == CPU_ARM64)
        {
            switch (type)
            {
            case TYPE_BASIC_FLOAT:
            case TYPE_BASIC_DOUBLE:
                if (double_count >= 8) break;
                params[pos] = 0x88 + double_count++;
                offset += size;
                continue;

            default:
                reg_count = ROUND_SIZE( reg_count, align / pointer_size );
                if (reg_count > 8 - size / pointer_size) break;
                while (size)
                {
                    params[pos++] = 0x80 + reg_count++;
                    offset += pointer_size;
                    size -= pointer_size;
                }
                continue;
            }
        }
        else  /* CPU_ARM */
        {
            switch (type)
            {
            case TYPE_BASIC_FLOAT:
                if (!(float_count % 2)) float_count = max( float_count, double_count * 2 );
                if (float_count >= 16)
                {
                    stack_pos = ROUND_SIZE( stack_pos, align );
                    params[pos] = 0x100 - (offset - stack_pos) / pointer_size;
                    stack_pos += size;
                }
                else
                {
                    params[pos] = 0x84 + float_count++;
                }
                offset += size;
                continue;

            case TYPE_BASIC_DOUBLE:
                double_count = max( double_count, (float_count + 1) / 2 );
                if (double_count >= 8) break;
                params[pos] = 0x84 + 2 * double_count;
                params[pos + 1] = 0x84 + 2 * double_count + 1;
                double_count++;
                offset += size;
                continue;

            default:
                reg_count = ROUND_SIZE( reg_count, align / pointer_size );
                if (reg_count <= 4 - size / pointer_size || !stack_pos)
                {
                    while (size && reg_count < 4)
                    {
                        params[pos++] = 0x80 + reg_count++;
                        offset += pointer_size;
                        size -= pointer_size;
                    }
                }
                break;
            }
        }

        stack_pos = ROUND_SIZE( stack_pos, align );
        memset( params + pos, 0x100 - (offset - stack_pos) / pointer_size, size / pointer_size );
        stack_pos += size;
        offset += size;
    }

    while (count && params[count - 1] == 0x9f) count--;
    return count;
}

static void write_proc_func_interp( FILE *file, int indent, const type_t *iface,
                                    const var_t *func, unsigned int *offset,
                                    unsigned short num_proc )
{
    var_t *var;
    var_list_t *args = type_function_get_args( func->declspec.type );
    unsigned char explicit_fc, implicit_fc;
    unsigned char handle_flags;
    var_t *retval = type_function_get_retval( func->declspec.type );
    const var_t *handle_var = get_func_handle_var( iface, func, &explicit_fc, &implicit_fc );
    unsigned char oi_flags = Oi_HAS_RPCFLAGS | Oi_USE_NEW_INIT_ROUTINES;
    unsigned char oi2_flags = get_func_oi2_flags( func );
    unsigned char ext_flags = 0x01; /* HasNewCorrDesc */
    unsigned int rpc_flags = get_rpc_flags( func->attrs );
    unsigned int nb_args = 0;
    unsigned int stack_size = 0;
    unsigned int stack_offset = 0;
    unsigned int stack_align;
    unsigned int extra_size = 0;
    unsigned short param_num = 0;
    unsigned short handle_stack_offset = 0;
    unsigned short handle_param_num = 0;
    unsigned int size;

    if (is_full_pointer_function( func )) oi_flags |= Oi_FULL_PTR_USED;
    if (is_object( iface ))
    {
        oi_flags |= Oi_OBJECT_PROC | Oi_OBJ_USE_V2_INTERPRETER;
        stack_offset = pointer_size;
        stack_size += pointer_size;
    }

    if (args) LIST_FOR_EACH_ENTRY( var, args, var_t, entry )
    {
        if (var == handle_var)
        {
            handle_stack_offset = stack_size;
            handle_param_num = param_num;
        }
        size = get_stack_size( var, &stack_align, NULL );
        stack_size = ROUND_SIZE( stack_size, stack_align );
        stack_size += size;
        param_num++;
        nb_args++;
    }
    if (!is_void( retval->declspec.type ))
    {
        stack_size += pointer_size;
        nb_args++;
    }

    print_file( file, 0, "/* %u (procedure %s::%s) */\n", *offset, iface->name, func->name );
    print_file( file, indent, "0x%02x,\t/* %s */\n", implicit_fc,
                implicit_fc ? string_of_type(implicit_fc) : "explicit handle" );
    print_file( file, indent, "0x%02x,\n", oi_flags );
    print_file( file, indent, "NdrFcLong(0x%x),\n", rpc_flags );
    print_file( file, indent, "NdrFcShort(0x%hx),\t/* method %hu */\n", num_proc, num_proc );
    print_file( file, indent, "NdrFcShort(0x%x),\t/* stack size = %u */\n", stack_size, stack_size );
    *offset += 10;

    if (!implicit_fc)
    {
        switch (explicit_fc)
        {
        case FC_BIND_PRIMITIVE:
            handle_flags = 0;
            print_file( file, indent, "0x%02x,\t/* %s */\n", explicit_fc, string_of_type(explicit_fc) );
            print_file( file, indent, "0x%02x,\n", handle_flags );
            print_file( file, indent, "NdrFcShort(0x%hx),\t/* stack offset = %hu */\n",
                        handle_stack_offset, handle_stack_offset );
            *offset += 4;
            nb_args--;
            break;
        case FC_BIND_GENERIC:
            handle_flags = type_memsize( handle_var->declspec.type );
            print_file( file, indent, "0x%02x,\t/* %s */\n", explicit_fc, string_of_type(explicit_fc) );
            print_file( file, indent, "0x%02x,\n", handle_flags );
            print_file( file, indent, "NdrFcShort(0x%hx),\t/* stack offset = %hu */\n",
                        handle_stack_offset, handle_stack_offset );
            print_file( file, indent, "0x%02x,\n", get_generic_handle_offset( handle_var->declspec.type ) );
            print_file( file, indent, "0x%x,\t/* FC_PAD */\n", FC_PAD);
            *offset += 6;
            break;
        case FC_BIND_CONTEXT:
            handle_flags = get_contexthandle_flags( iface, handle_var->attrs, handle_var->declspec.type, 0 );
            print_file( file, indent, "0x%02x,\t/* %s */\n", explicit_fc, string_of_type(explicit_fc) );
            print_file( file, indent, "0x%02x,\n", handle_flags );
            print_file( file, indent, "NdrFcShort(0x%hx),\t/* stack offset = %hu */\n",
                        handle_stack_offset, handle_stack_offset );
            print_file( file, indent, "0x%02x,\n", get_context_handle_offset( handle_var->declspec.type ) );
            print_file( file, indent, "0x%02x,\t/* param %hu */\n", handle_param_num, handle_param_num );
            *offset += 6;
            break;
        }
    }

    if (is_attr( func->attrs, ATTR_NOTIFY )) ext_flags |= 0x08;  /* HasNotify */
    if (is_attr( func->attrs, ATTR_NOTIFYFLAG )) ext_flags |= 0x10;  /* HasNotify2 */
    if (iface == type_iface_get_async_iface(iface)) oi2_flags |= 0x20;

    size = get_function_buffer_size( func, PASS_IN );
    print_file( file, indent, "NdrFcShort(0x%x),\t/* client buffer = %u */\n", size, size );
    size = get_function_buffer_size( func, PASS_OUT );
    print_file( file, indent, "NdrFcShort(0x%x),\t/* server buffer = %u */\n", size, size );
    print_file( file, indent, "0x%02x,\n", oi2_flags );
    print_file( file, indent, "0x%02x,\t/* %u params */\n", nb_args, nb_args );
    *offset += 6;
    extra_size = 8;

    switch (target.cpu)
    {
    case CPU_x86_64:
    case CPU_ARM64EC:
    {
        unsigned short pos = 0, fpu_mask = 0;

        extra_size += 2;
        print_file( file, indent, "0x%02x,\n", extra_size );
        print_file( file, indent, "0x%02x,\n", ext_flags );
        print_file( file, indent, "NdrFcShort(0x0),\n" );  /* server corr hint */
        print_file( file, indent, "NdrFcShort(0x0),\n" );  /* client corr hint */
        print_file( file, indent, "NdrFcShort(0x0),\n" );  /* FIXME: notify index */
        if (is_object( iface )) pos += 2;
        if (args) LIST_FOR_EACH_ENTRY( var, args, var_t, entry )
        {
            if (type_get_type( var->declspec.type ) == TYPE_BASIC)
            {
                switch (type_basic_get_type( var->declspec.type ))
                {
                case TYPE_BASIC_FLOAT:  fpu_mask |= 1 << pos; break;
                case TYPE_BASIC_DOUBLE: fpu_mask |= 2 << pos; break;
                default: break;
                }
            }
            pos += 2;
            if (pos >= 16) break;
        }
        print_file( file, indent, "NdrFcShort(0x%x),\n", fpu_mask );  /* floating point mask */
        break;
    }
    case CPU_ARM:
    case CPU_ARM64:
    {
        unsigned int i, len, count = stack_size / pointer_size;
        unsigned char *params = xmalloc( count );

        count = fill_params_array( iface, func, params, count );
        len = compress_params_array( params, count );

        extra_size += 3 + len + !(len % 2);
        print_file( file, indent, "0x%02x,\n", extra_size );
        print_file( file, indent, "0x%02x,\n", ext_flags );
        print_file( file, indent, "NdrFcShort(0x0),\n" );  /* server corr hint */
        print_file( file, indent, "NdrFcShort(0x0),\n" );  /* client corr hint */
        print_file( file, indent, "NdrFcShort(0x0),\n" );  /* FIXME: notify index */
        print_file( file, indent, "NdrFcShort(0x%02x),\n", count );
        print_file( file, indent, "0x%02x,\n", len );
        for (i = 0; i < len; i++) print_file( file, indent, "0x%02x,\n", params[i] );
        if (!(len % 2)) print_file( file, indent, "0x00,\n" );
        free( params );
        break;
    }
    case CPU_i386:
        print_file( file, indent, "0x%02x,\n", extra_size );
        print_file( file, indent, "0x%02x,\n", ext_flags );
        print_file( file, indent, "NdrFcShort(0x0),\n" );  /* server corr hint */
        print_file( file, indent, "NdrFcShort(0x0),\n" );  /* client corr hint */
        print_file( file, indent, "NdrFcShort(0x0),\n" );  /* FIXME: notify index */
        break;
    }
    *offset += extra_size;

    /* emit argument data */
    if (args) LIST_FOR_EACH_ENTRY( var, args, var_t, entry )
    {
        if (explicit_fc == FC_BIND_PRIMITIVE && var == handle_var)
        {
            stack_offset += pointer_size;
            continue;
        }
        print_file( file, 0, "/* %u (parameter %s) */\n", *offset, var->name );
        *offset += write_new_procformatstring_type(file, indent, var, FALSE, &stack_offset);
    }

    /* emit return value data */
    if (!is_void( retval->declspec.type ))
    {
        print_file( file, 0, "/* %u (return value) */\n", *offset );
        *offset += write_new_procformatstring_type(file, indent, retval, TRUE, &stack_offset);
    }
}

static void write_procformatstring_func( FILE *file, int indent, const type_t *iface,
                                         const var_t *func, unsigned int *offset,
                                         unsigned short num_proc )
{
    var_t *retval;

    if (is_interpreted_func( iface, func ))
    {
        write_proc_func_interp( file, indent, iface, func, offset, num_proc );
        return;
    }

    /* emit argument data */
    if (type_function_get_args(func->declspec.type))
    {
        const var_t *var;
        LIST_FOR_EACH_ENTRY( var, type_function_get_args(func->declspec.type), const var_t, entry )
        {
            print_file( file, 0, "/* %u (parameter %s) */\n", *offset, var->name );
            *offset += write_old_procformatstring_type(file, indent, var, FALSE);
        }
    }

    /* emit return value data */
    retval = type_function_get_retval( func->declspec.type );
    if (is_void(retval->declspec.type))
    {
        print_file(file, 0, "/* %u (void) */\n", *offset);
        print_file(file, indent, "0x5b,\t/* FC_END */\n");
        print_file(file, indent, "0x5c,\t/* FC_PAD */\n");
        *offset += 2;
    }
    else
    {
        print_file( file, 0, "/* %u (return value) */\n", *offset );
        *offset += write_old_procformatstring_type(file, indent, retval, TRUE);
    }
}

static void for_each_iface(const statement_list_t *stmts,
                           void (*proc)(type_t *iface, FILE *file, int indent, unsigned int *offset),
                           type_pred_t pred, FILE *file, int indent, unsigned int *offset)
{
    const statement_t *stmt;
    type_t *iface;

    if (stmts) LIST_FOR_EACH_ENTRY( stmt, stmts, const statement_t, entry )
    {
        if (stmt->type != STMT_TYPE || type_get_type(stmt->u.type) != TYPE_INTERFACE)
            continue;
        iface = stmt->u.type;
        if (!pred(iface)) continue;
        proc(iface, file, indent, offset);
        if (type_iface_get_async_iface(iface))
            proc(type_iface_get_async_iface(iface), file, indent, offset);
    }
}

static void write_iface_procformatstring(type_t *iface, FILE *file, int indent, unsigned int *offset)
{
    const statement_t *stmt;
    const type_t *parent = type_iface_get_inherit( iface );
    int count = parent ? count_methods( parent ) : 0;

    STATEMENTS_FOR_EACH_FUNC(stmt, type_iface_get_stmts(iface))
    {
        var_t *func = stmt->u.var;
        if (is_local(func->attrs))
        {
            if (!get_callas_source(iface, func)) count++;
            continue;
        }
        write_procformatstring_func( file, indent, iface, func, offset, count++ );
    }
}

void write_procformatstring(FILE *file, const statement_list_t *stmts, type_pred_t pred)
{
    int indent = 0;
    unsigned int offset = 0;

    print_file(file, indent, "static const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString =\n");
    print_file(file, indent, "{\n");
    indent++;
    print_file(file, indent, "0,\n");
    print_file(file, indent, "{\n");
    indent++;

    for_each_iface(stmts, write_iface_procformatstring, pred, file, indent, &offset);

    print_file(file, indent, "0x0\n");
    indent--;
    print_file(file, indent, "}\n");
    indent--;
    print_file(file, indent, "};\n");
    print_file(file, indent, "\n");
}

void write_procformatstring_offsets( FILE *file, const type_t *iface )
{
    const statement_t *stmt;
    int indent = 0;

    print_file( file, indent,  "static const unsigned short %s_FormatStringOffsetTable[] =\n",
                iface->name );
    print_file( file, indent,  "{\n" );
    indent++;
    STATEMENTS_FOR_EACH_FUNC( stmt, type_iface_get_stmts(iface) )
    {
        var_t *func = stmt->u.var;
        if (is_local( func->attrs )) continue;
        print_file( file, indent,  "%u,  /* %s */\n", func->procstring_offset, func->name );
    }
    indent--;
    print_file( file, indent,  "};\n\n" );
}

static int write_base_type(FILE *file, const type_t *type, unsigned int *typestring_offset)
{
    unsigned char fc;

    if (type_get_type(type) == TYPE_BASIC)
        fc = get_basic_fc_signed(type);
    else if (type_get_type(type) == TYPE_ENUM)
        fc = get_enum_fc(type);
    else
        return 0;

    print_file(file, 2, "0x%02x,\t/* %s */\n", fc, string_of_type(fc));
    *typestring_offset += 1;
    return 1;
}

static unsigned char get_correlation_type( const type_t *correlation_var )
{
    unsigned char fc;

    switch (type_get_type(correlation_var))
    {
    case TYPE_BASIC:
        fc = get_basic_fc(correlation_var);
        switch (fc)
        {
        case FC_SMALL:
        case FC_USMALL:
        case FC_SHORT:
        case FC_USHORT:
        case FC_LONG:
        case FC_ULONG:
            return fc;
        case FC_CHAR:
            return FC_SMALL;
        case FC_BYTE:
            return FC_USMALL;
        case FC_WCHAR:
            return FC_SHORT;
        }
        break;
    case TYPE_ENUM:
        return (get_enum_fc(correlation_var) == FC_ENUM32) ? FC_LONG : FC_SHORT;
    case TYPE_POINTER:
        return (pointer_size == 8) ? FC_HYPER : FC_LONG;
    default:
        break;
    }
    return 0;
}

/* write conformance / variance descriptor */
static unsigned int write_conf_or_var_desc(FILE *file, const type_t *cont_type,
                                           unsigned int baseoff, const type_t *type,
                                           const expr_t *expr, unsigned short robust_flags)
{
    unsigned char operator_type = 0;
    unsigned char conftype = FC_NORMAL_CONFORMANCE;
    const char *conftype_string = "field";
    const expr_t *subexpr;
    const type_t *iface = NULL;
    const char *name;

    if (!expr)
    {
        print_file(file, 2, "NdrFcLong(0xffffffff),\t/* -1 */\n");
        robust_flags = 0;
        goto done;
    }
    robust_flags |= RobustEarly;

    if (expr->is_const)
    {
        if (expr->cval > UCHAR_MAX * (USHRT_MAX + 1) + USHRT_MAX)
            error("write_conf_or_var_desc: constant value %d is greater than "
                  "the maximum constant size of %d\n", expr->cval,
                  UCHAR_MAX * (USHRT_MAX + 1) + USHRT_MAX);

        print_file(file, 2, "0x%x, /* Corr desc: constant, val = %d */\n",
                   FC_CONSTANT_CONFORMANCE, expr->cval);
        print_file(file, 2, "0x%x,\n", expr->cval >> 16);
        print_file(file, 2, "NdrFcShort(0x%hx),\n", (unsigned short)expr->cval);
        goto done;
    }

    if (!cont_type)  /* top-level conformance */
    {
        conftype = FC_TOP_LEVEL_CONFORMANCE;
        conftype_string = "parameter";
        cont_type = current_func->declspec.type;
        name = current_func->name;
        iface = current_iface;
    }
    else
    {
        name = cont_type->name;
        if (is_ptr(type) || (is_array(type) && type_array_is_decl_as_ptr(type)))
        {
            conftype = FC_POINTER_CONFORMANCE;
            conftype_string = "field pointer";
        }
    }

    subexpr = expr;
    switch (subexpr->type)
    {
    case EXPR_PPTR:
        subexpr = subexpr->ref;
        operator_type = FC_DEREFERENCE;
        break;
    case EXPR_DIV:
        if (subexpr->u.ext->is_const && (subexpr->u.ext->cval == 2))
        {
            subexpr = subexpr->ref;
            operator_type = FC_DIV_2;
        }
        break;
    case EXPR_MUL:
        if (subexpr->u.ext->is_const && (subexpr->u.ext->cval == 2))
        {
            subexpr = subexpr->ref;
            operator_type = FC_MULT_2;
        }
        break;
    case EXPR_SUB:
        if (subexpr->u.ext->is_const && (subexpr->u.ext->cval == 1))
        {
            subexpr = subexpr->ref;
            operator_type = FC_SUB_1;
        }
        break;
    case EXPR_ADD:
        if (subexpr->u.ext->is_const && (subexpr->u.ext->cval == 1))
        {
            subexpr = subexpr->ref;
            operator_type = FC_ADD_1;
        }
        break;
    default:
        break;
    }

    if (subexpr->type == EXPR_IDENTIFIER)
    {
        const type_t *correlation_variable = NULL;
        unsigned char param_type;
        unsigned int offset = 0;
        const var_t *var;
        struct expr_loc expr_loc;

        if (type_get_type(cont_type) == TYPE_FUNCTION)
        {
            var_list_t *args = type_function_get_args( cont_type );

            if (is_object( iface )) offset += pointer_size;
            if (args) LIST_FOR_EACH_ENTRY( var, args, const var_t, entry )
            {
                unsigned int align, size = get_stack_size( var, &align, NULL );
                offset = ROUND_SIZE( offset, align );
                if (var->name && !strcmp(var->name, subexpr->u.sval))
                {
                    expr_loc.v = var;
                    correlation_variable = var->declspec.type;
                    break;
                }
                offset += size;
                if (var == current_arg) robust_flags &= ~RobustEarly;
            }
        }
        else
        {
            var_list_t *fields = type_struct_get_fields( cont_type );

            if (fields) LIST_FOR_EACH_ENTRY( var, fields, const var_t, entry )
            {
                unsigned int size = field_memsize( var->declspec.type, &offset );
                if (var->name && !strcmp(var->name, subexpr->u.sval))
                {
                    expr_loc.v = var;
                    correlation_variable = var->declspec.type;
                    break;
                }
                offset += size;
                if (offset > baseoff) robust_flags &= ~RobustEarly;
            }
        }

        if (!correlation_variable)
            error("write_conf_or_var_desc: couldn't find variable %s in %s\n", subexpr->u.sval, name);
        expr_loc.attr = NULL;
        correlation_variable = expr_resolve_type(&expr_loc, cont_type, expr);

        offset -= baseoff;

        param_type = get_correlation_type( correlation_variable );
        if (!param_type)
        {
            error("write_conf_or_var_desc: non-arithmetic type used as correlation variable %s\n",
                  subexpr->u.sval);
            return 0;
        }

        print_file(file, 2, "0x%x,\t/* Corr desc: %s %s, %s */\n",
                   conftype | param_type, conftype_string, subexpr->u.sval, string_of_type(param_type));
        print_file(file, 2, "0x%x,\t/* %s */\n", operator_type,
                   operator_type ? string_of_type(operator_type) : "no operators");
        print_file(file, 2, "NdrFcShort(0x%hx),\t/* offset = %d */\n",
                   (unsigned short)offset, offset);
    }
    else if (!iface || is_interpreted_func( iface, current_func ))
    {
        unsigned int callback_offset = 0;
        struct expr_eval_routine *eval;
        int found = 0;

        LIST_FOR_EACH_ENTRY(eval, &expr_eval_routines, struct expr_eval_routine, entry)
        {
            if (eval->cont_type == cont_type ||
                (type_get_type( eval->cont_type ) == type_get_type( cont_type ) &&
                 eval->iface == iface &&
                 eval->name && name && !strcmp(eval->name, name) &&
                 !compare_expr(eval->expr, expr)))
            {
                found = 1;
                break;
            }
            callback_offset++;
        }

        if (!found)
        {
            eval = xmalloc (sizeof(*eval));
            eval->iface = iface;
            eval->cont_type = cont_type;
            eval->name = xstrdup( name );
            eval->baseoff = baseoff;
            eval->expr = expr;
            list_add_tail (&expr_eval_routines, &eval->entry);
        }
        robust_flags &= ~RobustEarly;

        if (callback_offset > USHRT_MAX)
            error("Maximum number of callback routines reached\n");

        print_file(file, 2, "0x%x,\t/* Corr desc: %s in %s */\n", conftype, conftype_string, name);
        print_file(file, 2, "0x%x,\t/* %s */\n", FC_CALLBACK, "FC_CALLBACK");
        print_file(file, 2, "NdrFcShort(0x%hx),\t/* %u */\n", (unsigned short)callback_offset, callback_offset);
    }
    else  /* output a dummy corr desc that isn't used */
    {
        print_file(file, 2, "0x%x,\t/* Corr desc: unused for %s */\n", conftype, name);
        print_file(file, 2, "0x0,\n" );
        print_file(file, 2, "NdrFcShort(0x0),\n" );
    }
done:
    if (!interpreted_mode) return 4;
    print_file(file, 2, "NdrFcShort(0x%hx),\n", robust_flags);
    return 6;
}

/* return size and start offset of a data field based on current offset */
static unsigned int field_memsize(const type_t *type, unsigned int *offset)
{
    unsigned int align = 0;
    unsigned int size = type_memsize_and_alignment( type, &align );

    *offset = ROUND_SIZE( *offset, align );
    return size;
}

static unsigned int fields_memsize(const var_list_t *fields, unsigned int *align)
{
    unsigned int size = 0;
    unsigned int max_align;
    const var_t *v;

    if (!fields) return 0;
    LIST_FOR_EACH_ENTRY( v, fields, const var_t, entry )
    {
        unsigned int falign = 0;
        unsigned int fsize = type_memsize_and_alignment(v->declspec.type, &falign);
        if (*align < falign) *align = falign;
        falign = clamp_align(falign);
        size = ROUND_SIZE(size, falign);
        size += fsize;
    }

    max_align = clamp_align(*align);
    size = ROUND_SIZE(size, max_align);

    return size;
}

static unsigned int union_memsize(const var_list_t *fields, unsigned int *pmaxa)
{
    unsigned int size, maxs = 0;
    unsigned int align = *pmaxa;
    const var_t *v;

    if (fields) LIST_FOR_EACH_ENTRY( v, fields, const var_t, entry )
    {
        /* we could have an empty default field with NULL type */
        if (v->declspec.type)
        {
            size = type_memsize_and_alignment(v->declspec.type, &align);
            if (maxs < size) maxs = size;
            if (*pmaxa < align) *pmaxa = align;
        }
    }

    return maxs;
}

unsigned int type_memsize_and_alignment(const type_t *t, unsigned int *align)
{
    unsigned int size = 0;

    switch (type_get_type(t))
    {
    case TYPE_BASIC:
        switch (get_basic_fc(t))
        {
        case FC_BYTE:
        case FC_CHAR:
        case FC_USMALL:
        case FC_SMALL:
            size = 1;
            if (size > *align) *align = size;
            break;
        case FC_WCHAR:
        case FC_USHORT:
        case FC_SHORT:
            size = 2;
            if (size > *align) *align = size;
            break;
        case FC_ULONG:
        case FC_LONG:
        case FC_ERROR_STATUS_T:
        case FC_FLOAT:
            size = 4;
            if (size > *align) *align = size;
            break;
        case FC_HYPER:
        case FC_DOUBLE:
            size = 8;
            if (size > *align) *align = size;
            break;
        case FC_INT3264:
        case FC_UINT3264:
        case FC_BIND_PRIMITIVE:
            assert( pointer_size );
            size = pointer_size;
            if (size > *align) *align = size;
            break;
        default:
            error("type_memsize: Unknown type 0x%x\n", get_basic_fc(t));
            size = 0;
        }
        break;
    case TYPE_ENUM:
        switch (get_enum_fc(t))
        {
        case FC_ENUM16:
        case FC_ENUM32:
            size = 4;
            if (size > *align) *align = size;
            break;
        default:
            error("type_memsize: Unknown enum type\n");
            size = 0;
        }
        break;
    case TYPE_STRUCT:
        size = fields_memsize(type_struct_get_fields(t), align);
        break;
    case TYPE_ENCAPSULATED_UNION:
        size = fields_memsize(type_encapsulated_union_get_fields(t), align);
        break;
    case TYPE_UNION:
        size = union_memsize(type_union_get_cases(t), align);
        break;
    case TYPE_POINTER:
    case TYPE_INTERFACE:
        assert( pointer_size );
        size = pointer_size;
        if (size > *align) *align = size;
        break;
    case TYPE_ARRAY:
        if (!type_array_is_decl_as_ptr(t))
        {
            if (is_conformant_array(t))
            {
                type_memsize_and_alignment(type_array_get_element_type(t), align);
                size = 0;
            }
            else
                size = type_array_get_dim(t) *
                    type_memsize_and_alignment(type_array_get_element_type(t), align);
        }
        else /* declared as a pointer */
        {
            assert( pointer_size );
            size = pointer_size;
            if (size > *align) *align = size;
        }
        break;
    case TYPE_ALIAS:
    case TYPE_VOID:
    case TYPE_COCLASS:
    case TYPE_MODULE:
    case TYPE_FUNCTION:
    case TYPE_BITFIELD:
    case TYPE_APICONTRACT:
    case TYPE_RUNTIMECLASS:
    case TYPE_PARAMETERIZED_TYPE:
    case TYPE_PARAMETER:
    case TYPE_DELEGATE:
        /* these types should not be encountered here due to language
         * restrictions (interface, void, coclass, module), logical
         * restrictions (alias - due to type_get_type call above) or
         * checking restrictions (function, bitfield). */
        assert(0);
    }

    return size;
}

unsigned int type_memsize(const type_t *t)
{
    unsigned int align = 0;
    return type_memsize_and_alignment( t, &align );
}

static unsigned int type_buffer_alignment(const type_t *t)
{
    const var_list_t *fields;
    const var_t *var;
    unsigned int max = 0, align;

    switch (type_get_type(t))
    {
    case TYPE_BASIC:
        switch (get_basic_fc(t))
        {
        case FC_BYTE:
        case FC_CHAR:
        case FC_USMALL:
        case FC_SMALL:
            return 1;
        case FC_WCHAR:
        case FC_USHORT:
        case FC_SHORT:
            return 2;
        case FC_ULONG:
        case FC_LONG:
        case FC_ERROR_STATUS_T:
        case FC_FLOAT:
        case FC_INT3264:
        case FC_UINT3264:
            return 4;
        case FC_HYPER:
        case FC_DOUBLE:
            return 8;
        default:
            error("type_buffer_alignment: Unknown type 0x%x\n", get_basic_fc(t));
        }
        break;
    case TYPE_ENUM:
        switch (get_enum_fc(t))
        {
        case FC_ENUM16:
            return 2;
        case FC_ENUM32:
            return 4;
        default:
            error("type_buffer_alignment: Unknown enum type\n");
        }
        break;
    case TYPE_STRUCT:
        if (!(fields = type_struct_get_fields(t))) break;
        LIST_FOR_EACH_ENTRY( var, fields, const var_t, entry )
        {
            if (!var->declspec.type) continue;
            align = type_buffer_alignment( var->declspec.type );
            if (max < align) max = align;
        }
        break;
    case TYPE_ENCAPSULATED_UNION:
        if (!(fields = type_encapsulated_union_get_fields(t))) break;
        LIST_FOR_EACH_ENTRY( var, fields, const var_t, entry )
        {
            if (!var->declspec.type) continue;
            align = type_buffer_alignment( var->declspec.type );
            if (max < align) max = align;
        }
        break;
    case TYPE_UNION:
        if (!(fields = type_union_get_cases(t))) break;
        LIST_FOR_EACH_ENTRY( var, fields, const var_t, entry )
        {
            if (!var->declspec.type) continue;
            align = type_buffer_alignment( var->declspec.type );
            if (max < align) max = align;
        }
        break;
    case TYPE_ARRAY:
        if (!type_array_is_decl_as_ptr(t))
            return type_buffer_alignment( type_array_get_element_type(t) );
        /* else fall through */
    case TYPE_POINTER:
        return 4;
    case TYPE_INTERFACE:
    case TYPE_ALIAS:
    case TYPE_VOID:
    case TYPE_COCLASS:
    case TYPE_MODULE:
    case TYPE_FUNCTION:
    case TYPE_BITFIELD:
    case TYPE_APICONTRACT:
    case TYPE_RUNTIMECLASS:
    case TYPE_PARAMETERIZED_TYPE:
    case TYPE_PARAMETER:
    case TYPE_DELEGATE:
        /* these types should not be encountered here due to language
         * restrictions (interface, void, coclass, module), logical
         * restrictions (alias - due to type_get_type call above) or
         * checking restrictions (function, bitfield). */
        assert(0);
    }
    return max;
}

int is_full_pointer_function(const var_t *func)
{
    const var_t *var;
    if (type_has_full_pointer(type_function_get_rettype(func->declspec.type), func->attrs, TRUE, TRUE))
        return TRUE;
    if (!type_function_get_args(func->declspec.type))
        return FALSE;
    LIST_FOR_EACH_ENTRY( var, type_function_get_args(func->declspec.type), const var_t, entry )
        if (type_has_full_pointer( var->declspec.type, var->attrs, TRUE, TRUE ))
            return TRUE;
    return FALSE;
}

void write_full_pointer_init(FILE *file, int indent, const var_t *func, int is_server)
{
    print_file(file, indent, "__frame->_StubMsg.FullPtrXlatTables = NdrFullPointerXlatInit(0,%s);\n",
                   is_server ? "XLAT_SERVER" : "XLAT_CLIENT");
    fprintf(file, "\n");
}

void write_full_pointer_free(FILE *file, int indent, const var_t *func)
{
    print_file(file, indent, "NdrFullPointerXlatFree(__frame->_StubMsg.FullPtrXlatTables);\n");
    fprintf(file, "\n");
}

static unsigned int write_nonsimple_pointer(FILE *file, const attr_list_t *attrs,
                                            int toplevel_attrs, const type_t *type,
                                            enum type_context context,
                                            unsigned int offset,
                                            unsigned int *typeformat_offset)
{
    unsigned int start_offset = *typeformat_offset;
    short reloff = offset - (*typeformat_offset + 2);
    int in_attr, out_attr;
    int pointer_type;
    unsigned char flags = 0;

    pointer_type = get_pointer_fc_context(type, attrs, toplevel_attrs, context);

    in_attr = is_attr(attrs, ATTR_IN);
    out_attr = is_attr(attrs, ATTR_OUT);
    if (!in_attr && !out_attr) in_attr = 1;

    if (!is_interpreted_func(current_iface, current_func))
    {
        if (context == TYPE_CONTEXT_TOPLEVELPARAM && out_attr && !in_attr && pointer_type == FC_RP)
            flags |= FC_ALLOCED_ON_STACK;
    }
    else
    {
        if (context == TYPE_CONTEXT_TOPLEVELPARAM && is_ptr(type) && pointer_type == FC_RP)
        {
            switch (typegen_detect_type(type_pointer_get_ref_type(type), attrs, TDT_ALL_TYPES))
            {
            case TGT_STRING:
            case TGT_POINTER:
            case TGT_CTXT_HANDLE:
            case TGT_CTXT_HANDLE_POINTER:
            case TGT_ARRAY:
                flags |= FC_ALLOCED_ON_STACK;
                break;
            case TGT_IFACE_POINTER:
                if (in_attr && out_attr)
                    flags |= FC_ALLOCED_ON_STACK;
                break;
            default:
                break;
            }
        }
    }

    if (is_ptr(type))
    {
        type_t *ref = type_pointer_get_ref_type(type);
        if(is_declptr(ref) && !is_user_type(ref))
            flags |= FC_POINTER_DEREF;
        if (pointer_type != FC_RP) {
            flags |= get_attrv(type->attrs, ATTR_ALLOCATE);
        }
    }

    print_file(file, 2, "0x%x, 0x%x,\t\t/* %s",
               pointer_type,
               flags,
               string_of_type(pointer_type));
    if (file)
    {
        if (flags & FC_ALLOCED_ON_STACK)
            fprintf(file, " [allocated_on_stack]");
        if (flags & FC_POINTER_DEREF)
            fprintf(file, " [pointer_deref]");
        if (flags & FC_DONT_FREE)
            fprintf(file, " [dont_free]");
        if (flags & FC_ALLOCATE_ALL_NODES)
            fprintf(file, " [all_nodes]");
        fprintf(file, " */\n");
    }

    print_file(file, 2, "NdrFcShort(0x%hx),\t/* Offset= %hd (%u) */\n", reloff, reloff, offset);
    *typeformat_offset += 4;

    return start_offset;
}

static unsigned int write_simple_pointer(FILE *file, const attr_list_t *attrs, int toplevel_attrs,
                                         const type_t *type, enum type_context context)
{
    unsigned char fc;
    unsigned char pointer_fc;
    const type_t *ref;
    int in_attr = is_attr(attrs, ATTR_IN);
    int out_attr = is_attr(attrs, ATTR_OUT);
    unsigned char flags = FC_SIMPLE_POINTER;

    /* for historical reasons, write_simple_pointer also handled string types,
     * but no longer does. catch bad uses of the function with this check */
    if (is_string_type(attrs, type))
        error("write_simple_pointer: can't handle type %s which is a string type\n", type->name);

    pointer_fc = get_pointer_fc_context(type, attrs, toplevel_attrs, context);

    ref = type_pointer_get_ref_type(type);
    if (type_get_type(ref) == TYPE_ENUM)
        fc = get_enum_fc(ref);
    else
        fc = get_basic_fc(ref);

    if (!is_interpreted_func(current_iface, current_func))
    {
        if (context == TYPE_CONTEXT_TOPLEVELPARAM && out_attr && !in_attr && pointer_fc == FC_RP)
            flags |= FC_ALLOCED_ON_STACK;
    }
    else
    {
        if (context == TYPE_CONTEXT_TOPLEVELPARAM && fc == FC_ENUM16 && pointer_fc == FC_RP)
            flags |= FC_ALLOCED_ON_STACK;
    }

    print_file(file, 2, "0x%02x, 0x%x,\t/* %s %s[simple_pointer] */\n",
               pointer_fc, flags, string_of_type(pointer_fc),
               flags & FC_ALLOCED_ON_STACK ? "[allocated_on_stack] " : "");
    print_file(file, 2, "0x%02x,\t/* %s */\n", fc, string_of_type(fc));
    print_file(file, 2, "0x5c,\t/* FC_PAD */\n");
    return 4;
}

static void print_start_tfs_comment(FILE *file, type_t *t, unsigned int tfsoff)
{
    const decl_spec_t ds = {.type = t};
    print_file(file, 0, "/* %u (", tfsoff);
    write_type_decl(file, &ds, NULL);
    print_file(file, 0, ") */\n");
}

static unsigned int write_pointer_tfs(FILE *file, const attr_list_t *attrs, int toplevel_attrs,
                                      type_t *type, unsigned int ref_offset,
                                      enum type_context context,
                                      unsigned int *typestring_offset)
{
    unsigned int offset = *typestring_offset;
    type_t *ref = type_pointer_get_ref_type(type);

    print_start_tfs_comment(file, type, offset);
    update_tfsoff(type, offset, file);

    switch (typegen_detect_type(ref, attrs, TDT_ALL_TYPES))
    {
    case TGT_BASIC:
    case TGT_ENUM:
        *typestring_offset += write_simple_pointer(file, attrs, toplevel_attrs, type, context);
        break;
    default:
        if (ref_offset)
            write_nonsimple_pointer(file, attrs, toplevel_attrs, type, context, ref_offset, typestring_offset);
        break;
    }

    return offset;
}

static int processed(const type_t *type)
{
    return type->typestring_offset && !type->tfswrite;
}

static int user_type_has_variable_size(const type_t *t)
{
    if (is_ptr(t))
        return TRUE;
    else if (type_get_type(t) == TYPE_STRUCT)
    {
        switch (get_struct_fc(t))
        {
        case FC_PSTRUCT:
        case FC_CSTRUCT:
        case FC_CPSTRUCT:
        case FC_CVSTRUCT:
            return TRUE;
        }
    }
    /* Note: Since this only applies to user types, we can't have a conformant
       array here, and strings should get filed under pointer in this case.  */
    return FALSE;
}

static unsigned int write_user_tfs(FILE *file, type_t *type, unsigned int *tfsoff)
{
    unsigned int start, absoff, flags;
    const char *name = NULL;
    type_t *utype = get_user_type(type, &name);
    unsigned int usize = type_memsize(utype);
    unsigned int ualign = type_buffer_alignment(utype);
    unsigned int size = type_memsize(type);
    unsigned short funoff = user_type_offset(name);
    short reloff;

    if (processed(type)) return type->typestring_offset;

    guard_rec(type);

    if(user_type_has_variable_size(utype)) usize = 0;

    if (type_get_type(utype) == TYPE_BASIC ||
        type_get_type(utype) == TYPE_ENUM)
    {
        unsigned char fc;

        if (type_get_type(utype) == TYPE_ENUM)
            fc = get_enum_fc(utype);
        else
            fc = get_basic_fc(utype);

        absoff = *tfsoff;
        print_start_tfs_comment(file, utype, absoff);
        print_file(file, 2, "0x%x,\t/* %s */\n", fc, string_of_type(fc));
        print_file(file, 2, "0x5c,\t/* FC_PAD */\n");
        *tfsoff += 2;
    }
    else
    {
        if (!processed(utype))
            write_type_tfs(file, NULL, FALSE, utype, utype->name, TYPE_CONTEXT_CONTAINER, tfsoff);
        absoff = utype->typestring_offset;
    }

    if (type_get_type(utype) == TYPE_POINTER && get_pointer_fc(utype, NULL, FALSE, FALSE) == FC_RP)
        flags = 0x40;
    else if (type_get_type(utype) == TYPE_POINTER && get_pointer_fc(utype, NULL, FALSE, FALSE) == FC_UP)
        flags = 0x80;
    else
        flags = 0;

    start = *tfsoff;
    update_tfsoff(type, start, file);
    print_start_tfs_comment(file, type, start);
    print_file(file, 2, "0x%x,\t/* FC_USER_MARSHAL */\n", FC_USER_MARSHAL);
    print_file(file, 2, "0x%x,\t/* Alignment= %d, Flags= %02x */\n",
               flags | (ualign - 1), ualign - 1, flags);
    print_file(file, 2, "NdrFcShort(0x%hx),\t/* Function offset= %hu */\n", funoff, funoff);
    print_file(file, 2, "NdrFcShort(0x%hx),\t/* %u */\n", (unsigned short)size, size);
    print_file(file, 2, "NdrFcShort(0x%hx),\t/* %u */\n", (unsigned short)usize, usize);
    *tfsoff += 8;
    reloff = absoff - *tfsoff;
    print_file(file, 2, "NdrFcShort(0x%hx),\t/* Offset= %hd (%u) */\n", reloff, reloff, absoff);
    *tfsoff += 2;
    return start;
}

static void write_member_type(FILE *file, const type_t *cont,
                              int cont_is_complex, const attr_list_t *attrs,
                              const type_t *type, unsigned int *corroff,
                              unsigned int *tfsoff)
{
    if (is_embedded_complex(type, attrs) && !is_conformant_array(type))
    {
        unsigned int absoff;
        short reloff;

        if (type_get_type(type) == TYPE_UNION && is_attr(attrs, ATTR_SWITCHIS))
        {
            absoff = *corroff;
            *corroff += interpreted_mode ? 10 : 8;
        }
        else
        {
            absoff = type->typestring_offset;
        }
        reloff = absoff - (*tfsoff + 2);

        print_file(file, 2, "0x4c,\t/* FC_EMBEDDED_COMPLEX */\n");
        /* padding is represented using FC_STRUCTPAD* types, so presumably
         * this is left over in the format for historical purposes in MIDL
         * or rpcrt4. */
        print_file(file, 2, "0x0,\n");
        print_file(file, 2, "NdrFcShort(0x%hx),\t/* Offset= %hd (%u) */\n",
                   reloff, reloff, absoff);
        *tfsoff += 4;
    }
    else if (is_ptr(type) || is_conformant_array(type))
    {
        unsigned char fc = cont_is_complex ? FC_POINTER : FC_LONG;
        print_file(file, 2, "0x%x,\t/* %s */\n", fc, string_of_type(fc));
        *tfsoff += 1;
    }
    else if (!write_base_type(file, type, tfsoff))
        error("Unsupported member type %d\n", type_get_type(type));
}

static void write_array_element_type(FILE *file, const attr_list_t *attrs, const type_t *type,
                                     int cont_is_complex, unsigned int *tfsoff)
{
    type_t *elem = type_array_get_element_type(type);

    if (!is_embedded_complex(elem, attrs) && is_ptr(elem))
    {
        type_t *ref = type_pointer_get_ref_type(elem);

        if (processed(ref))
        {
            write_nonsimple_pointer(file, attrs, FALSE, elem, TYPE_CONTEXT_CONTAINER,
                                    ref->typestring_offset, tfsoff);
            return;
        }
        if (cont_is_complex && is_string_type(attrs, elem))
        {
            write_string_tfs(file, attrs, FALSE, elem, TYPE_CONTEXT_CONTAINER, NULL, tfsoff);
            return;
        }
        if (!is_string_type(attrs, elem) &&
            (type_get_type(ref) == TYPE_BASIC || type_get_type(ref) == TYPE_ENUM))
        {
            *tfsoff += write_simple_pointer(file, attrs, FALSE, elem, TYPE_CONTEXT_CONTAINER);
            return;
        }
    }
    write_member_type(file, type, cont_is_complex, attrs, elem, NULL, tfsoff);
}

static void write_end(FILE *file, unsigned int *tfsoff)
{
    if (*tfsoff % 2 == 0)
    {
        print_file(file, 2, "0x%x,\t/* FC_PAD */\n", FC_PAD);
        *tfsoff += 1;
    }
    print_file(file, 2, "0x%x,\t/* FC_END */\n", FC_END);
    *tfsoff += 1;
}

static void write_descriptors(FILE *file, type_t *type, unsigned int *tfsoff)
{
    unsigned int offset = 0;
    var_list_t *fs = type_struct_get_fields(type);
    var_t *f;

    if (fs) LIST_FOR_EACH_ENTRY(f, fs, var_t, entry)
    {
        type_t *ft = f->declspec.type;
        unsigned int size = field_memsize( ft, &offset );
        if (type_get_type(ft) == TYPE_UNION && is_attr(f->attrs, ATTR_SWITCHIS))
        {
            short reloff;
            unsigned int absoff = ft->typestring_offset;
            unsigned char fc;
            const expr_t *switch_is = get_attrp(f->attrs, ATTR_SWITCHIS);
            struct expr_loc expr_loc = { .v = f };

            fc = get_correlation_type( expr_resolve_type( &expr_loc, current_structure, switch_is ));
            if (!fc) fc = FC_LONG;

            if (is_attr(ft->attrs, ATTR_SWITCHTYPE))
                absoff += interpreted_mode ? 10 : 8; /* we already have a corr descr, skip it */
            print_file(file, 0, "/* %d */\n", *tfsoff);
            print_file(file, 2, "0x%x,\t/* FC_NON_ENCAPSULATED_UNION */\n", FC_NON_ENCAPSULATED_UNION);
            print_file(file, 2, "0x%x,\t/* %s */\n", fc, string_of_type(fc));

            *tfsoff += 2 + write_conf_or_var_desc(file, current_structure, offset, ft, switch_is, 0);
            reloff = absoff - *tfsoff;
            print_file(file, 2, "NdrFcShort(0x%hx),\t/* Offset= %hd (%u) */\n",
                       (unsigned short)reloff, reloff, absoff);
            *tfsoff += 2;
        }
        offset += size;
    }
}

static int write_pointer_description_offsets(
    FILE *file, const attr_list_t *attrs, int toplevel_attrs, type_t *type,
    unsigned int *offset_in_memory, unsigned int *offset_in_buffer,
    unsigned int *typestring_offset)
{
    int written = 0;

    if ((is_ptr(type) && type_get_type(type_pointer_get_ref_type(type)) != TYPE_INTERFACE) ||
        (is_array(type) && type_array_is_decl_as_ptr(type)))
    {
        if (offset_in_memory && offset_in_buffer)
        {
            unsigned int memsize;

            /* pointer instance
             *
             * note that MSDN states that for pointer layouts in structures,
             * this is a negative offset from the end of the structure, but
             * this statement is incorrect. all offsets are positive */
            print_file(file, 2, "NdrFcShort(0x%hx),\t/* Memory offset = %d */\n", (unsigned short)*offset_in_memory, *offset_in_memory);
            print_file(file, 2, "NdrFcShort(0x%hx),\t/* Buffer offset = %d */\n", (unsigned short)*offset_in_buffer, *offset_in_buffer);

            memsize = type_memsize(type);
            *offset_in_memory += memsize;
            /* increment these separately as in the case of conformant (varying)
             * structures these start at different values */
            *offset_in_buffer += memsize;
        }
        *typestring_offset += 4;

        if (is_ptr(type))
        {
            type_t *ref = type_pointer_get_ref_type(type);

            if (is_string_type(attrs, type))
                write_string_tfs(file, attrs, toplevel_attrs, type, TYPE_CONTEXT_CONTAINER, NULL, typestring_offset);
            else if (processed(ref))
                write_nonsimple_pointer(file, attrs, toplevel_attrs, type, TYPE_CONTEXT_CONTAINER,
                                        ref->typestring_offset, typestring_offset);
            else if (type_get_type(ref) == TYPE_BASIC || type_get_type(ref) == TYPE_ENUM)
                *typestring_offset += write_simple_pointer(file, attrs, toplevel_attrs, type, TYPE_CONTEXT_CONTAINER);
            else
                error("write_pointer_description_offsets: type format string unknown\n");
        }
        else
        {
            unsigned int offset = type->typestring_offset;
            /* skip over the pointer that is written for strings, since a
             * pointer has to be written in-place here */
            if (is_string_type(attrs, type))
                offset += 4;
            write_nonsimple_pointer(file, attrs, toplevel_attrs, type,
                    TYPE_CONTEXT_CONTAINER, offset, typestring_offset);
        }

        return 1;
    }

    if (is_array(type))
    {
        return write_pointer_description_offsets(
            file, attrs, FALSE, type_array_get_element_type(type), offset_in_memory,
            offset_in_buffer, typestring_offset);
    }
    else if (is_non_complex_struct(type))
    {
        /* otherwise search for interesting fields to parse */
        const var_t *v;
        LIST_FOR_EACH_ENTRY( v, type_struct_get_fields(type), const var_t, entry )
        {
            if (offset_in_memory && offset_in_buffer)
            {
                unsigned int padding;
                unsigned int align = 0;
                type_memsize_and_alignment(v->declspec.type, &align);
                padding = ROUNDING(*offset_in_memory, align);
                *offset_in_memory += padding;
                *offset_in_buffer += padding;
            }
            written += write_pointer_description_offsets(
                file, v->attrs, TRUE, v->declspec.type, offset_in_memory, offset_in_buffer,
                typestring_offset);
        }
    }
    else
    {
        if (offset_in_memory && offset_in_buffer)
        {
            unsigned int memsize = type_memsize(type);
            *offset_in_memory += memsize;
            /* increment these separately as in the case of conformant (varying)
             * structures these start at different values */
            *offset_in_buffer += memsize;
        }
    }

    return written;
}

static int write_no_repeat_pointer_descriptions(
    FILE *file, const attr_list_t *attrs, int toplevel_attrs, type_t *type,
    unsigned int *offset_in_memory, unsigned int *offset_in_buffer,
    unsigned int *typestring_offset)
{
    int written = 0;

    if (is_ptr(type) ||
        (is_conformant_array(type) && type_array_is_decl_as_ptr(type)))
    {
        print_file(file, 2, "0x%02x, /* FC_NO_REPEAT */\n", FC_NO_REPEAT);
        print_file(file, 2, "0x%02x, /* FC_PAD */\n", FC_PAD);
        *typestring_offset += 2;

        return write_pointer_description_offsets(file, attrs, toplevel_attrs, type,
                       offset_in_memory, offset_in_buffer, typestring_offset);
    }

    if (is_non_complex_struct(type))
    {
        const var_t *v;
        LIST_FOR_EACH_ENTRY( v, type_struct_get_fields(type), const var_t, entry )
        {
            if (offset_in_memory && offset_in_buffer)
            {
                unsigned int padding;
                unsigned int align = 0;
                type_memsize_and_alignment(v->declspec.type, &align);
                padding = ROUNDING(*offset_in_memory, align);
                *offset_in_memory += padding;
                *offset_in_buffer += padding;
            }
            written += write_no_repeat_pointer_descriptions(
                file, v->attrs, TRUE, v->declspec.type,
                offset_in_memory, offset_in_buffer, typestring_offset);
        }
    }
    else
    {
        unsigned int memsize = type_memsize(type);
        *offset_in_memory += memsize;
        /* increment these separately as in the case of conformant (varying)
         * structures these start at different values */
        *offset_in_buffer += memsize;
    }

    return written;
}

/* Note: if file is NULL return value is number of pointers to write, else
 * it is the number of type format characters written */
static int write_fixed_array_pointer_descriptions(
    FILE *file, const attr_list_t *attrs, int toplevel_attrs, type_t *type,
    unsigned int *offset_in_memory, unsigned int *offset_in_buffer,
    unsigned int *typestring_offset)
{
    int pointer_count = 0;

    if (type_get_type(type) == TYPE_ARRAY &&
        !type_array_has_conformance(type) && !type_array_has_variance(type))
    {
        unsigned int temp = 0;
        /* unfortunately, this needs to be done in two passes to avoid
         * writing out redundant FC_FIXED_REPEAT descriptions */
        pointer_count = write_pointer_description_offsets(
            NULL, attrs, FALSE, type_array_get_element_type(type), NULL, NULL, &temp);
        if (pointer_count > 0)
        {
            unsigned int increment_size;
            unsigned int offset_of_array_pointer_mem = 0;
            unsigned int offset_of_array_pointer_buf = 0;

            increment_size = type_memsize(type_array_get_element_type(type));

            print_file(file, 2, "0x%02x, /* FC_FIXED_REPEAT */\n", FC_FIXED_REPEAT);
            print_file(file, 2, "0x%02x, /* FC_PAD */\n", FC_PAD);
            print_file(file, 2, "NdrFcShort(0x%hx),\t/* Iterations = %d */\n", (unsigned short)type_array_get_dim(type), type_array_get_dim(type));
            print_file(file, 2, "NdrFcShort(0x%hx),\t/* Increment = %d */\n", (unsigned short)increment_size, increment_size);
            print_file(file, 2, "NdrFcShort(0x%hx),\t/* Offset to array = %d */\n", (unsigned short)*offset_in_memory, *offset_in_memory);
            print_file(file, 2, "NdrFcShort(0x%hx),\t/* Number of pointers = %d */\n", (unsigned short)pointer_count, pointer_count);
            *typestring_offset += 10;

            pointer_count = write_pointer_description_offsets(
                file, attrs, toplevel_attrs, type, &offset_of_array_pointer_mem,
                &offset_of_array_pointer_buf, typestring_offset);
        }
    }
    else if (type_get_type(type) == TYPE_STRUCT)
    {
        const var_t *v;
        LIST_FOR_EACH_ENTRY( v, type_struct_get_fields(type), const var_t, entry )
        {
            if (offset_in_memory && offset_in_buffer)
            {
                unsigned int padding;
                unsigned int align = 0;
                type_memsize_and_alignment(v->declspec.type, &align);
                padding = ROUNDING(*offset_in_memory, align);
                *offset_in_memory += padding;
                *offset_in_buffer += padding;
            }
            pointer_count += write_fixed_array_pointer_descriptions(
                file, v->attrs, TRUE, v->declspec.type, offset_in_memory, offset_in_buffer,
                typestring_offset);
        }
    }
    else
    {
        if (offset_in_memory && offset_in_buffer)
        {
            unsigned int memsize;
            memsize = type_memsize(type);
            *offset_in_memory += memsize;
            /* increment these separately as in the case of conformant (varying)
             * structures these start at different values */
            *offset_in_buffer += memsize;
        }
    }

    return pointer_count;
}

/* Note: if file is NULL return value is number of pointers to write, else
 * it is the number of type format characters written */
static int write_conformant_array_pointer_descriptions(
    FILE *file, const attr_list_t *attrs, type_t *type,
    unsigned int offset_in_memory, unsigned int *typestring_offset)
{
    int pointer_count = 0;

    if (is_conformant_array(type) && !type_array_has_variance(type))
    {
        unsigned int temp = 0;
        /* unfortunately, this needs to be done in two passes to avoid
         * writing out redundant FC_VARIABLE_REPEAT descriptions */
        pointer_count = write_pointer_description_offsets(
            NULL, attrs, FALSE, type_array_get_element_type(type), NULL, NULL, &temp);
        if (pointer_count > 0)
        {
            unsigned int increment_size;
            unsigned int offset_of_array_pointer_mem = offset_in_memory;
            unsigned int offset_of_array_pointer_buf = offset_in_memory;

            increment_size = type_memsize(type_array_get_element_type(type));

            if (increment_size > USHRT_MAX)
                error("array size of %u bytes is too large\n", increment_size);

            print_file(file, 2, "0x%02x, /* FC_VARIABLE_REPEAT */\n", FC_VARIABLE_REPEAT);
            print_file(file, 2, "0x%02x, /* FC_FIXED_OFFSET */\n", FC_FIXED_OFFSET);
            print_file(file, 2, "NdrFcShort(0x%hx),\t/* Increment = %d */\n", (unsigned short)increment_size, increment_size);
            print_file(file, 2, "NdrFcShort(0x%hx),\t/* Offset to array = %d */\n", (unsigned short)offset_in_memory, offset_in_memory);
            print_file(file, 2, "NdrFcShort(0x%hx),\t/* Number of pointers = %d */\n", (unsigned short)pointer_count, pointer_count);
            *typestring_offset += 8;

            pointer_count = write_pointer_description_offsets(
                file, attrs, FALSE, type_array_get_element_type(type),
                &offset_of_array_pointer_mem, &offset_of_array_pointer_buf,
                typestring_offset);
        }
    }

    return pointer_count;
}

/* Note: if file is NULL return value is number of pointers to write, else
 * it is the number of type format characters written */
static int write_varying_array_pointer_descriptions(
    FILE *file, const attr_list_t *attrs, type_t *type,
    unsigned int *offset_in_memory, unsigned int *offset_in_buffer,
    unsigned int *typestring_offset)
{
    int pointer_count = 0;

    if (is_array(type) && type_array_has_variance(type))
    {
        unsigned int temp = 0;
        /* unfortunately, this needs to be done in two passes to avoid
         * writing out redundant FC_VARIABLE_REPEAT descriptions */
        pointer_count = write_pointer_description_offsets(
            NULL, attrs, FALSE, type_array_get_element_type(type), NULL, NULL, &temp);
        if (pointer_count > 0)
        {
            unsigned int increment_size;

            increment_size = type_memsize(type_array_get_element_type(type));

            if (increment_size > USHRT_MAX)
                error("array size of %u bytes is too large\n", increment_size);

            print_file(file, 2, "0x%02x, /* FC_VARIABLE_REPEAT */\n", FC_VARIABLE_REPEAT);
            print_file(file, 2, "0x%02x, /* FC_VARIABLE_OFFSET */\n", FC_VARIABLE_OFFSET);
            print_file(file, 2, "NdrFcShort(0x%hx),\t/* Increment = %d */\n", (unsigned short)increment_size, increment_size);
            print_file(file, 2, "NdrFcShort(0x%hx),\t/* Offset to array = %d */\n", (unsigned short)*offset_in_memory, *offset_in_memory);
            print_file(file, 2, "NdrFcShort(0x%hx),\t/* Number of pointers = %d */\n", (unsigned short)pointer_count, pointer_count);
            *typestring_offset += 8;

            pointer_count = write_pointer_description_offsets(
                file, attrs, FALSE, type_array_get_element_type(type), offset_in_memory,
                offset_in_buffer, typestring_offset);
        }
    }
    else if (type_get_type(type) == TYPE_STRUCT)
    {
        const var_t *v;
        LIST_FOR_EACH_ENTRY( v, type_struct_get_fields(type), const var_t, entry )
        {
            if (offset_in_memory && offset_in_buffer)
            {
                unsigned int align = 0, padding;

                if (is_array(v->declspec.type) && type_array_has_variance(v->declspec.type))
                {
                    *offset_in_buffer = ROUND_SIZE(*offset_in_buffer, 4);
                    /* skip over variance and offset in buffer */
                    *offset_in_buffer += 8;
                }

                type_memsize_and_alignment(v->declspec.type, &align);
                padding = ROUNDING(*offset_in_memory, align);
                *offset_in_memory += padding;
                *offset_in_buffer += padding;
            }
            pointer_count += write_varying_array_pointer_descriptions(
                file, v->attrs, v->declspec.type, offset_in_memory, offset_in_buffer,
                typestring_offset);
        }
    }
    else
    {
        if (offset_in_memory && offset_in_buffer)
        {
            unsigned int memsize = type_memsize(type);
            *offset_in_memory += memsize;
            /* increment these separately as in the case of conformant (varying)
             * structures these start at different values */
            *offset_in_buffer += memsize;
        }
    }

    return pointer_count;
}

static void write_pointer_description(FILE *file, const attr_list_t *attrs, int toplevel_attrs, type_t *type,
                                      unsigned int *typestring_offset)
{
    unsigned int offset_in_buffer;
    unsigned int offset_in_memory;

    /* pass 1: search for single instance of a pointer (i.e. don't descend
     * into arrays) */
    if (!is_array(type))
    {
        offset_in_memory = 0;
        offset_in_buffer = 0;
        write_no_repeat_pointer_descriptions(
            file, attrs, toplevel_attrs, type,
            &offset_in_memory, &offset_in_buffer, typestring_offset);
    }

    /* pass 2: search for pointers in fixed arrays */
    offset_in_memory = 0;
    offset_in_buffer = 0;
    write_fixed_array_pointer_descriptions(
        file, attrs, toplevel_attrs, type,
        &offset_in_memory, &offset_in_buffer, typestring_offset);

    /* pass 3: search for pointers in conformant only arrays (but don't descend
     * into conformant varying or varying arrays) */
    if (is_conformant_array(type) &&
        (type_array_is_decl_as_ptr(type) || !current_structure))
        write_conformant_array_pointer_descriptions(
            file, attrs, type, 0, typestring_offset);
    else if (type_get_type(type) == TYPE_STRUCT &&
             get_struct_fc(type) == FC_CPSTRUCT)
    {
        type_t *carray = find_array_or_string_in_struct(type)->declspec.type;
        write_conformant_array_pointer_descriptions( file, NULL, carray,
                                                     type_memsize(type), typestring_offset);
    }

    /* pass 4: search for pointers in varying arrays */
    offset_in_memory = 0;
    offset_in_buffer = 0;
    write_varying_array_pointer_descriptions(
            file, NULL, type,
            &offset_in_memory, &offset_in_buffer, typestring_offset);
}

static unsigned int write_string_tfs(FILE *file, const attr_list_t *attrs, int toplevel_attrs,
                                     type_t *type, enum type_context context,
                                     const char *name, unsigned int *typestring_offset)
{
    unsigned int start_offset;
    unsigned char rtype;
    type_t *elem_type;
    int is_processed = processed(type);

    start_offset = *typestring_offset;

    if (is_declptr(type))
    {
        unsigned char flag = is_conformant_array(type) ? 0 : FC_SIMPLE_POINTER;
        int pointer_type = get_pointer_fc_context(type, attrs, toplevel_attrs, context);
        if (!pointer_type)
            pointer_type = FC_RP;
        print_start_tfs_comment(file, type, *typestring_offset);
        print_file(file, 2,"0x%x, 0x%x,\t/* %s%s */\n",
                   pointer_type, flag, string_of_type(pointer_type),
                   flag ? " [simple_pointer]" : "");
        *typestring_offset += 2;
        if (!flag)
        {
            print_file(file, 2, "NdrFcShort(0x2),\n");
            *typestring_offset += 2;
        }
        is_processed = FALSE;
    }

    if (is_array(type))
        elem_type = type_array_get_element_type(type);
    else
        elem_type = type_pointer_get_ref_type(type);

    if (type_get_type(elem_type) == TYPE_POINTER && is_array(type))
    {
        write_array_tfs(file, attrs, toplevel_attrs, type, name, typestring_offset);
        return start_offset;
    }

    if (type_get_type(elem_type) != TYPE_BASIC)
    {
        error("write_string_tfs: Unimplemented for non-basic type %s\n", name);
        return start_offset;
    }

    rtype = get_basic_fc(elem_type);
    if ((rtype != FC_BYTE) && (rtype != FC_CHAR) && (rtype != FC_WCHAR))
    {
        error("write_string_tfs: Unimplemented for type 0x%x of name: %s\n", rtype, name);
        return start_offset;
    }

    if (type_get_type(type) == TYPE_ARRAY && !type_array_has_conformance(type))
    {
        unsigned int dim = type_array_get_dim(type);

        if (is_processed) return start_offset;

        /* FIXME: multi-dimensional array */
        if (0xffffu < dim)
            error("array size for parameter %s exceeds %u bytes by %u bytes\n",
                  name, 0xffffu, dim - 0xffffu);

        if (rtype == FC_WCHAR)
            WRITE_FCTYPE(file, FC_WSTRING, *typestring_offset);
        else
            WRITE_FCTYPE(file, FC_CSTRING, *typestring_offset);
        print_file(file, 2, "0x%x,\t/* FC_PAD */\n", FC_PAD);
        *typestring_offset += 2;

        print_file(file, 2, "NdrFcShort(0x%hx),\t/* %d */\n", (unsigned short)dim, dim);
        *typestring_offset += 2;

        update_tfsoff(type, start_offset, file);
        return start_offset;
    }
    else if (is_conformant_array(type))
    {
        if (rtype == FC_WCHAR)
            WRITE_FCTYPE(file, FC_C_WSTRING, *typestring_offset);
        else
            WRITE_FCTYPE(file, FC_C_CSTRING, *typestring_offset);
        print_file(file, 2, "0x%x,\t/* FC_STRING_SIZED */\n", FC_STRING_SIZED);
        *typestring_offset += 2;

        *typestring_offset += write_conf_or_var_desc(
            file, current_structure,
            (!type_array_is_decl_as_ptr(type) && current_structure
             ? type_memsize(current_structure)
             : 0),
            type, type_array_get_conformance(type), 0);

        update_tfsoff(type, start_offset, file);
        return start_offset;
    }
    else
    {
        if (is_processed) return start_offset;

        if (rtype == FC_WCHAR)
            WRITE_FCTYPE(file, FC_C_WSTRING, *typestring_offset);
        else
            WRITE_FCTYPE(file, FC_C_CSTRING, *typestring_offset);
        print_file(file, 2, "0x%x,\t/* FC_PAD */\n", FC_PAD);
        *typestring_offset += 2;

        update_tfsoff(type, start_offset, file);
        return start_offset;
    }
}

static unsigned int write_array_tfs(FILE *file, const attr_list_t *attrs, int toplevel_attrs, type_t *type,
                                    const char *name, unsigned int *typestring_offset)
{
    const expr_t *length_is = type_array_get_variance(type);
    const expr_t *size_is = type_array_get_conformance(type);
    unsigned int align;
    unsigned int size;
    unsigned int start_offset;
    unsigned char fc;
    unsigned int baseoff
        = !type_array_is_decl_as_ptr(type) && current_structure
        ? type_memsize(current_structure)
        : 0;

    if (!is_string_type(attrs, type_array_get_element_type(type)))
        write_type_tfs(file, attrs, FALSE, type_array_get_element_type(type),
                name, TYPE_CONTEXT_CONTAINER_NO_POINTERS, typestring_offset);

    size = type_memsize(is_conformant_array(type) ? type_array_get_element_type(type) : type);
    align = type_buffer_alignment(is_conformant_array(type) ? type_array_get_element_type(type) : type);
    fc = get_array_fc(type, attrs);

    start_offset = *typestring_offset;
    update_tfsoff(type, start_offset, file);
    print_start_tfs_comment(file, type, start_offset);
    print_file(file, 2, "0x%02x,\t/* %s */\n", fc, string_of_type(fc));
    print_file(file, 2, "0x%x,\t/* %d */\n", align - 1, align - 1);
    *typestring_offset += 2;

    align = 0;
    if (fc != FC_BOGUS_ARRAY)
    {
        if (fc == FC_LGFARRAY || fc == FC_LGVARRAY)
        {
            print_file(file, 2, "NdrFcLong(0x%x),\t/* %u */\n", size, size);
            *typestring_offset += 4;
        }
        else
        {
            print_file(file, 2, "NdrFcShort(0x%hx),\t/* %u */\n", (unsigned short)size, size);
            *typestring_offset += 2;
        }

        if (is_conformant_array(type))
            *typestring_offset
                += write_conf_or_var_desc(file, current_structure, baseoff,
                                          type, size_is, 0);

        if (fc == FC_SMVARRAY || fc == FC_LGVARRAY)
        {
            unsigned int elsize = type_memsize(type_array_get_element_type(type));
            unsigned int dim = type_array_get_dim(type);

            if (fc == FC_LGVARRAY)
            {
                print_file(file, 2, "NdrFcLong(0x%x),\t/* %u */\n", dim, dim);
                *typestring_offset += 4;
            }
            else
            {
                print_file(file, 2, "NdrFcShort(0x%hx),\t/* %u */\n", (unsigned short)dim, dim);
                *typestring_offset += 2;
            }

            print_file(file, 2, "NdrFcShort(0x%hx),\t/* %u */\n", (unsigned short)elsize, elsize);
            *typestring_offset += 2;
        }

        if (length_is)
            *typestring_offset
                += write_conf_or_var_desc(file, current_structure, baseoff, type, length_is, 0);

        if (type_has_pointers(type_array_get_element_type(type), attrs) &&
            (type_array_is_decl_as_ptr(type) || !current_structure))
        {
            print_file(file, 2, "0x%x,\t/* FC_PP */\n", FC_PP);
            print_file(file, 2, "0x%x,\t/* FC_PAD */\n", FC_PAD);
            *typestring_offset += 2;
            write_pointer_description(file, attrs, toplevel_attrs, type, typestring_offset);
            print_file(file, 2, "0x%x,\t/* FC_END */\n", FC_END);
            *typestring_offset += 1;
        }

        write_array_element_type(file, attrs, type, FALSE, typestring_offset);
        write_end(file, typestring_offset);
    }
    else
    {
        unsigned int dim = size_is ? 0 : type_array_get_dim(type);
        print_file(file, 2, "NdrFcShort(0x%hx),\t/* %u */\n", (unsigned short)dim, dim);
        *typestring_offset += 2;
        *typestring_offset += write_conf_or_var_desc(file, current_structure, baseoff, type, size_is, 0);
        *typestring_offset += write_conf_or_var_desc(file, current_structure, baseoff, type, length_is, 0);

        write_array_element_type(file, attrs, type, TRUE, typestring_offset);
        write_end(file, typestring_offset);
    }

    return start_offset;
}

static const var_t *find_array_or_string_in_struct(const type_t *type)
{
    const var_list_t *fields = type_struct_get_fields(type);
    const var_t *last_field;
    const type_t *ft;

    if (!fields || list_empty(fields))
        return NULL;

    last_field = LIST_ENTRY( list_tail(fields), const var_t, entry );
    ft = last_field->declspec.type;

    if (is_conformant_array(ft) && !type_array_is_decl_as_ptr(ft))
        return last_field;

    if (type_get_type(ft) == TYPE_STRUCT)
        return find_array_or_string_in_struct(ft);
    else
        return NULL;
}

static void write_struct_members(FILE *file, const type_t *type,
                                 int is_complex, unsigned int *corroff,
                                 unsigned int *typestring_offset)
{
    const var_t *field;
    unsigned short offset = 0;
    unsigned int salign = 1;
    int padding;
    var_list_t *fields = type_struct_get_fields(type);

    if (fields) LIST_FOR_EACH_ENTRY( field, fields, const var_t, entry )
    {
        type_t *ft = field->declspec.type;
        unsigned int align = 0;
        unsigned int size = type_memsize_and_alignment(ft, &align);
        align = clamp_align(align);
        if (salign < align) salign = align;

        if (!is_conformant_array(ft) || type_array_is_decl_as_ptr(ft))
        {
            unsigned short aligned = ROUND_SIZE(offset, align);
            if (aligned > offset)
            {
                unsigned char fc = FC_STRUCTPAD1 + (aligned - offset) - 1;
                print_file(file, 2, "0x%x,\t/* %s */\n", fc, string_of_type(fc));
                offset = aligned;
                *typestring_offset += 1;
            }
            write_member_type(file, type, is_complex, field->attrs, field->declspec.type, corroff,
                              typestring_offset);
            offset += size;
        }
    }

    padding = ROUNDING(offset, salign);
    if (padding)
    {
        print_file(file, 2, "0x%x,\t/* FC_STRUCTPAD%d */\n",
                   FC_STRUCTPAD1 + padding - 1,
                   padding);
        *typestring_offset += 1;
    }

    write_end(file, typestring_offset);
}

static unsigned int write_struct_tfs(FILE *file, type_t *type, const attr_list_t *attrs,
                                     const char *name, unsigned int *tfsoff)
{
    const type_t *save_current_structure = current_structure;
    unsigned int total_size;
    const var_t *array;
    unsigned int start_offset;
    unsigned int align;
    unsigned int corroff;
    var_t *f;
    unsigned char fc = get_struct_fc(type);
    var_list_t *fields = type_struct_get_fields(type);

    if (processed(type)) return type->typestring_offset;

    guard_rec(type);
    current_structure = type;

    total_size = type_memsize(type);
    align = type_buffer_alignment(type);
    if (total_size > USHRT_MAX)
        error("structure size for %s exceeds %d bytes by %d bytes\n",
              name, USHRT_MAX, total_size - USHRT_MAX);

    if (fields) LIST_FOR_EACH_ENTRY(f, fields, var_t, entry)
        write_type_tfs(file, f->attrs, TRUE, f->declspec.type, f->name, TYPE_CONTEXT_CONTAINER_NO_POINTERS, tfsoff);

    array = find_array_or_string_in_struct(type);
    if (array && !processed(array->declspec.type))
    {
        if(is_string_type(array->attrs, array->declspec.type))
            write_string_tfs(file, array->attrs, TRUE, array->declspec.type, TYPE_CONTEXT_CONTAINER, array->name, tfsoff);
        else
            write_array_tfs(file, array->attrs, TRUE, array->declspec.type, array->name, tfsoff);
    }

    corroff = *tfsoff;
    write_descriptors(file, type, tfsoff);

    start_offset = *tfsoff;
    update_tfsoff(type, start_offset, file);
    print_start_tfs_comment(file, type, start_offset);
    print_file(file, 2, "0x%x,\t/* %s */\n", fc, string_of_type(fc));
    print_file(file, 2, "0x%x,\t/* %d */\n", align - 1, align - 1);
    print_file(file, 2, "NdrFcShort(0x%hx),\t/* %d */\n", (unsigned short)total_size, total_size);
    *tfsoff += 4;

    if (array)
    {
        unsigned int absoff = array->declspec.type->typestring_offset;
        short reloff = absoff - *tfsoff;
        print_file(file, 2, "NdrFcShort(0x%hx),\t/* Offset= %hd (%u) */\n",
                   reloff, reloff, absoff);
        *tfsoff += 2;
    }
    else if (fc == FC_BOGUS_STRUCT)
    {
        print_file(file, 2, "NdrFcShort(0x0),\n");
        *tfsoff += 2;
    }

    if (fc == FC_BOGUS_STRUCT)
    {
        /* On the sizing pass, type->ptrdesc may be zero, but it's ok as
           nothing is written to file yet.  On the actual writing pass,
           this will have been updated.  */
        unsigned int absoff = type->ptrdesc ? type->ptrdesc : *tfsoff;
        int reloff = absoff - *tfsoff;
        assert( reloff >= 0 );
        print_file(file, 2, "NdrFcShort(0x%hx),\t/* Offset= %d (%u) */\n",
                   (unsigned short)reloff, reloff, absoff);
        *tfsoff += 2;
    }
    else if ((fc == FC_PSTRUCT) ||
             (fc == FC_CPSTRUCT) ||
             (fc == FC_CVSTRUCT && type_has_pointers(type, attrs)))
    {
        print_file(file, 2, "0x%x,\t/* FC_PP */\n", FC_PP);
        print_file(file, 2, "0x%x,\t/* FC_PAD */\n", FC_PAD);
        *tfsoff += 2;
        write_pointer_description(file, NULL, FALSE, type, tfsoff);
        print_file(file, 2, "0x%x,\t/* FC_END */\n", FC_END);
        *tfsoff += 1;
    }

    write_struct_members(file, type, fc == FC_BOGUS_STRUCT, &corroff,
                         tfsoff);

    if (fc == FC_BOGUS_STRUCT)
    {
        const var_t *f;

        type->ptrdesc = *tfsoff;
        if (fields) LIST_FOR_EACH_ENTRY(f, fields, const var_t, entry)
        {
            type_t *ft = f->declspec.type;
            switch (typegen_detect_type(ft, f->attrs, TDT_IGNORE_STRINGS))
            {
            case TGT_POINTER:
                if (is_string_type(f->attrs, ft))
                    write_string_tfs(file, f->attrs, TRUE, ft, TYPE_CONTEXT_CONTAINER, f->name, tfsoff);
                else
                    write_pointer_tfs(file, f->attrs, TRUE, ft,
                                      type_pointer_get_ref_type(ft)->typestring_offset,
                                      TYPE_CONTEXT_CONTAINER, tfsoff);
                break;
            case TGT_ARRAY:
                if (type_array_is_decl_as_ptr(ft))
                {
                    unsigned int offset;

                    print_file(file, 0, "/* %d */\n", *tfsoff);

                    offset = ft->typestring_offset;
                    /* skip over the pointer that is written for strings, since a
                     * pointer has to be written in-place here */
                    if (is_string_type(f->attrs, ft))
                        offset += 4;
                    write_nonsimple_pointer(file, f->attrs, TRUE, ft, TYPE_CONTEXT_CONTAINER, offset, tfsoff);
                }
                break;
            default:
                break;
            }
        }
        if (type->ptrdesc == *tfsoff)
            type->ptrdesc = 0;
    }

    current_structure = save_current_structure;
    return start_offset;
}

static void write_branch_type(FILE *file, const type_t *t, unsigned int *tfsoff)
{
    if (t == NULL)
    {
        print_file(file, 2, "NdrFcShort(0x0),\t/* No type */\n");
    }
    else
    {
        if (type_get_type(t) == TYPE_BASIC || type_get_type(t) == TYPE_ENUM)
        {
            unsigned char fc;
            if (type_get_type(t) == TYPE_BASIC)
                fc = get_basic_fc(t);
            else
                fc = get_enum_fc(t);
            print_file(file, 2, "NdrFcShort(0x80%02x),\t/* Simple arm type: %s */\n",
                       fc, string_of_type(fc));
        }
        else if (t->typestring_offset)
        {
            short reloff = t->typestring_offset - *tfsoff;
            print_file(file, 2, "NdrFcShort(0x%hx),\t/* Offset= %d (%d) */\n",
                       reloff, reloff, t->typestring_offset);
        }
        else
            error("write_branch_type: type unimplemented %d\n", type_get_type(t));
    }

    *tfsoff += 2;
}

static unsigned int write_union_tfs(FILE *file, const attr_list_t *attrs,
                                    type_t *type, unsigned int *tfsoff)
{
    unsigned int start_offset;
    unsigned int size;
    var_list_t *fields;
    unsigned int nbranch = 0;
    type_t *deftype = NULL;
    short nodeftype = 0xffff;
    unsigned int dummy;
    var_t *f;

    if (processed(type) &&
        (type_get_type(type) == TYPE_ENCAPSULATED_UNION || !is_attr(type->attrs, ATTR_SWITCHTYPE)))
        return type->typestring_offset;

    guard_rec(type);

    fields = type_union_get_cases(type);

    size = union_memsize(fields, &dummy);

    if (fields) LIST_FOR_EACH_ENTRY(f, fields, var_t, entry)
    {
        expr_list_t *cases = get_attrp(f->attrs, ATTR_CASE);
        if (cases)
            nbranch += list_count(cases);
        if (f->declspec.type)
            write_type_tfs(file, f->attrs, TRUE, f->declspec.type, f->name, TYPE_CONTEXT_CONTAINER, tfsoff);
    }

    start_offset = *tfsoff;
    update_tfsoff(type, start_offset, file);
    print_start_tfs_comment(file, type, start_offset);
    if (type_get_type(type) == TYPE_ENCAPSULATED_UNION)
    {
        const var_t *sv = type_union_get_switch_value(type);
        const type_t *st = sv->declspec.type;
        unsigned int align = 0;
        unsigned char fc;

        if (type_get_type(st) == TYPE_BASIC)
        {
            fc = get_basic_fc(st);
            switch (fc)
            {
            case FC_CHAR:
            case FC_SMALL:
            case FC_BYTE:
            case FC_USMALL:
            case FC_WCHAR:
            case FC_SHORT:
            case FC_USHORT:
            case FC_LONG:
            case FC_ULONG:
                break;
            default:
                fc = 0;
                error("union switch type must be an integer, char, or enum\n");
            }
        }
        else if (type_get_type(st) == TYPE_ENUM)
            fc = get_enum_fc(st);
        else
            error("union switch type must be an integer, char, or enum\n");

        type_memsize_and_alignment(st, &align);
        if (fields) LIST_FOR_EACH_ENTRY(f, fields, var_t, entry)
        {
            if (f->declspec.type)
                type_memsize_and_alignment(f->declspec.type, &align);
        }

        print_file(file, 2, "0x%x,\t/* FC_ENCAPSULATED_UNION */\n", FC_ENCAPSULATED_UNION);
        print_file(file, 2, "0x%x,\t/* Switch type= %s */\n",
                   (align << 4) | fc, string_of_type(fc));
        *tfsoff += 2;
    }
    else if (is_attr(type->attrs, ATTR_SWITCHTYPE))
    {
        const expr_t *switch_is = get_attrp(attrs, ATTR_SWITCHIS);
        const type_t *st = get_attrp(type->attrs, ATTR_SWITCHTYPE);
        unsigned char fc;

        if (type_get_type(st) == TYPE_BASIC)
        {
            fc = get_basic_fc(st);
            switch (fc)
            {
            case FC_CHAR:
            case FC_SMALL:
            case FC_USMALL:
            case FC_SHORT:
            case FC_USHORT:
            case FC_LONG:
            case FC_ULONG:
            case FC_ENUM16:
            case FC_ENUM32:
                break;
            default:
                fc = 0;
                error("union switch type must be an integer, char, or enum\n");
            }
        }
        else if (type_get_type(st) == TYPE_ENUM)
            fc = get_enum_fc(st);
        else
            error("union switch type must be an integer, char, or enum\n");

        print_file(file, 2, "0x%x,\t/* FC_NON_ENCAPSULATED_UNION */\n", FC_NON_ENCAPSULATED_UNION);
        print_file(file, 2, "0x%x,\t/* Switch type= %s */\n",
                   fc, string_of_type(fc));
        *tfsoff += 2;
        *tfsoff += write_conf_or_var_desc(file, current_structure, 0, st, switch_is, 0);
        print_file(file, 2, "NdrFcShort(0x2),\t/* Offset= 2 (%u) */\n", *tfsoff + 2);
        *tfsoff += 2;
        print_file(file, 0, "/* %u */\n", *tfsoff);
    }

    print_file(file, 2, "NdrFcShort(0x%hx),\t/* %d */\n", (unsigned short)size, size);
    print_file(file, 2, "NdrFcShort(0x%hx),\t/* %d */\n", (unsigned short)nbranch, nbranch);
    *tfsoff += 4;

    if (fields) LIST_FOR_EACH_ENTRY(f, fields, var_t, entry)
    {
        type_t *ft = f->declspec.type;
        expr_list_t *cases = get_attrp(f->attrs, ATTR_CASE);
        int deflt = is_attr(f->attrs, ATTR_DEFAULT);
        expr_t *c;

        if (cases == NULL && !deflt)
            error("union field %s with neither case nor default attribute\n", f->name);

        if (cases) LIST_FOR_EACH_ENTRY(c, cases, expr_t, entry)
        {
            /* MIDL doesn't check for duplicate cases, even though that seems
               like a reasonable thing to do, it just dumps them to the TFS
               like we're going to do here.  */
            print_file(file, 2, "NdrFcLong(0x%x),\t/* %d */\n", c->cval, c->cval);
            *tfsoff += 4;
            write_branch_type(file, ft, tfsoff);
        }

        /* MIDL allows multiple default branches, even though that seems
           illogical, it just chooses the last one, which is what we will
           do.  */
        if (deflt)
        {
            deftype = ft;
            nodeftype = 0;
        }
    }

    if (deftype)
    {
        write_branch_type(file, deftype, tfsoff);
    }
    else
    {
        print_file(file, 2, "NdrFcShort(0x%hx),\n", nodeftype);
        *tfsoff += 2;
    }

    return start_offset;
}

static unsigned int write_ip_tfs(FILE *file, const attr_list_t *attrs, type_t *type,
                                 unsigned int *typeformat_offset)
{
    unsigned int i;
    unsigned int start_offset = *typeformat_offset;
    expr_t *iid = get_attrp(attrs, ATTR_IIDIS);

    if (!iid && processed(type)) return type->typestring_offset;

    print_start_tfs_comment(file, type, start_offset);
    update_tfsoff(type, start_offset, file);

    if (iid)
    {
        print_file(file, 2, "0x2f,  /* FC_IP */\n");
        print_file(file, 2, "0x5c,  /* FC_PAD */\n");
        *typeformat_offset += 2 + write_conf_or_var_desc(file, current_structure, 0,
                                                         type, iid, RobustIsIIdIs);
    }
    else
    {
        const type_t *base = is_ptr(type) ? type_pointer_get_ref_type(type) : type;
        const struct uuid *uuid = get_attrp(base->attrs, ATTR_UUID);

        if (! uuid)
            error("%s: interface %s missing UUID\n", __FUNCTION__, base->name);

        print_file(file, 2, "0x2f,\t/* FC_IP */\n");
        print_file(file, 2, "0x5a,\t/* FC_CONSTANT_IID */\n");
        print_file(file, 2, "NdrFcLong(0x%08x),\n", uuid->Data1);
        print_file(file, 2, "NdrFcShort(0x%04x),\n", uuid->Data2);
        print_file(file, 2, "NdrFcShort(0x%04x),\n", uuid->Data3);
        for (i = 0; i < 8; ++i)
            print_file(file, 2, "0x%02x,\n", uuid->Data4[i]);

        if (file)
            fprintf(file, "\n");

        *typeformat_offset += 18;
    }
    return start_offset;
}

static unsigned int write_contexthandle_tfs(FILE *file,
                                            const attr_list_t *attrs,
                                            type_t *type,
                                            enum type_context context,
                                            unsigned int *typeformat_offset)
{
    unsigned int start_offset = *typeformat_offset;
    unsigned char flags = get_contexthandle_flags( current_iface, attrs, type, context == TYPE_CONTEXT_RETVAL );

    print_start_tfs_comment(file, type, start_offset);

    if (flags & 0x80)  /* via ptr */
    {
        int pointer_type = get_pointer_fc( type, attrs, TRUE, context == TYPE_CONTEXT_TOPLEVELPARAM );
        if (!pointer_type) pointer_type = FC_RP;
        *typeformat_offset += 4;
        print_file(file, 2,"0x%x, 0x0,\t/* %s */\n", pointer_type, string_of_type(pointer_type) );
        print_file(file, 2, "NdrFcShort(0x2),\t /* Offset= 2 (%u) */\n", *typeformat_offset);
        print_file(file, 0, "/* %2u */\n", *typeformat_offset);
    }

    print_file(file, 2, "0x%02x,\t/* FC_BIND_CONTEXT */\n", FC_BIND_CONTEXT);
    print_file(file, 2, "0x%x,\t/* Context flags: ", flags);
    if (flags & NDR_CONTEXT_HANDLE_CANNOT_BE_NULL)
        print_file(file, 0, "can't be null, ");
    if (flags & NDR_CONTEXT_HANDLE_SERIALIZE)
        print_file(file, 0, "serialize, ");
    if (flags & NDR_CONTEXT_HANDLE_NOSERIALIZE)
        print_file(file, 0, "no serialize, ");
    if (flags & NDR_STRICT_CONTEXT_HANDLE)
        print_file(file, 0, "strict, ");
    if (flags & HANDLE_PARAM_IS_RETURN)
        print_file(file, 0, "return, ");
    if (flags & HANDLE_PARAM_IS_OUT)
        print_file(file, 0, "out, ");
    if (flags & HANDLE_PARAM_IS_IN)
        print_file(file, 0, "in, ");
    if (flags & HANDLE_PARAM_IS_VIA_PTR)
        print_file(file, 0, "via ptr, ");
    print_file(file, 0, "*/\n");
    print_file(file, 2, "0x%x,\t/* rundown routine */\n", get_context_handle_offset( type ));
    print_file(file, 2, "0, /* FIXME: param num */\n");
    *typeformat_offset += 4;

    update_tfsoff( type, start_offset, file );
    return start_offset;
}

static unsigned int write_range_tfs(FILE *file, const attr_list_t *attrs,
                                    type_t *type, expr_list_t *range_list,
                                    unsigned int *typeformat_offset)
{
    unsigned char fc;
    unsigned int start_offset = *typeformat_offset;
    const expr_t *range_min = LIST_ENTRY(list_head(range_list), const expr_t, entry);
    const expr_t *range_max = LIST_ENTRY(list_next(range_list, list_head(range_list)), const expr_t, entry);

    if (type_get_type(type) == TYPE_BASIC)
        fc = get_basic_fc(type);
    else
        fc = get_enum_fc(type);

    /* fc must fit in lower 4-bits of 8-bit field below */
    assert(fc <= 0xf);

    print_file(file, 0, "/* %u */\n", *typeformat_offset);
    print_file(file, 2, "0x%x,\t/* FC_RANGE */\n", FC_RANGE);
    print_file(file, 2, "0x%x,\t/* %s */\n", fc, string_of_type(fc));
    print_file(file, 2, "NdrFcLong(0x%x),\t/* %u */\n", range_min->cval, range_min->cval);
    print_file(file, 2, "NdrFcLong(0x%x),\t/* %u */\n", range_max->cval, range_max->cval);
    update_tfsoff( type, start_offset, file );
    *typeformat_offset += 10;

    return start_offset;
}

static unsigned int write_type_tfs(FILE *file, const attr_list_t *attrs, int toplevel_attrs,
                                   type_t *type, const char *name,
                                   enum type_context context,
                                   unsigned int *typeformat_offset)
{
    unsigned int offset;

    switch (typegen_detect_type(type, attrs, TDT_ALL_TYPES))
    {
    case TGT_CTXT_HANDLE:
    case TGT_CTXT_HANDLE_POINTER:
        return write_contexthandle_tfs(file, attrs, type, context, typeformat_offset);
    case TGT_USER_TYPE:
        return write_user_tfs(file, type, typeformat_offset);
    case TGT_STRING:
        return write_string_tfs(file, attrs, toplevel_attrs, type, context, name, typeformat_offset);
    case TGT_ARRAY:
    {
        unsigned int off;
        /* conformant and pointer arrays are handled specially */
        if ((context != TYPE_CONTEXT_CONTAINER &&
             context != TYPE_CONTEXT_CONTAINER_NO_POINTERS) ||
            !is_conformant_array(type) || type_array_is_decl_as_ptr(type))
            off = write_array_tfs(file, attrs, toplevel_attrs, type, name, typeformat_offset);
        else
            off = 0;
        if (context != TYPE_CONTEXT_CONTAINER &&
            context != TYPE_CONTEXT_CONTAINER_NO_POINTERS)
        {
            int ptr_type;
            ptr_type = get_pointer_fc_context(type, attrs, toplevel_attrs, context);
            if (type_array_is_decl_as_ptr(type)
                    || (ptr_type != FC_RP && context == TYPE_CONTEXT_TOPLEVELPARAM))
            {
                unsigned int absoff = type->typestring_offset;
                short reloff = absoff - (*typeformat_offset + 2);
                off = *typeformat_offset;
                print_file(file, 0, "/* %d */\n", off);
                print_file(file, 2, "0x%x, 0x0,\t/* %s */\n", ptr_type,
                           string_of_type(ptr_type));
                print_file(file, 2, "NdrFcShort(0x%hx),\t/* Offset= %hd (%u) */\n",
                           reloff, reloff, absoff);
                if (ptr_type != FC_RP) update_tfsoff( type, off, file );
                *typeformat_offset += 4;
            }
            type_array_set_ptr_tfsoff(type, off);
        }
        return off;
    }
    case TGT_STRUCT:
        return write_struct_tfs(file, type, attrs, name, typeformat_offset);
    case TGT_UNION:
        return write_union_tfs(file, attrs, type, typeformat_offset);
    case TGT_ENUM:
    case TGT_BASIC:
        /* nothing to do */
        return 0;
    case TGT_RANGE:
    {
        expr_list_t *range_list = get_attrp(attrs, ATTR_RANGE);
        if (!range_list)
            range_list = get_aliaschain_attrp(type, ATTR_RANGE);
        return write_range_tfs(file, attrs, type, range_list, typeformat_offset);
    }
    case TGT_IFACE_POINTER:
        return write_ip_tfs(file, attrs, type, typeformat_offset);
    case TGT_POINTER:
    {
        enum type_context ref_context;
        type_t *ref = type_pointer_get_ref_type(type);

        if (context == TYPE_CONTEXT_TOPLEVELPARAM)
            ref_context = TYPE_CONTEXT_PARAM;
        else if (context == TYPE_CONTEXT_CONTAINER_NO_POINTERS)
            ref_context = TYPE_CONTEXT_CONTAINER;
        else
            ref_context = context;

        offset = write_type_tfs( file, attrs, FALSE, ref, name, ref_context, typeformat_offset);
        if (context == TYPE_CONTEXT_CONTAINER_NO_POINTERS)
            return 0;
        return write_pointer_tfs(file, attrs, toplevel_attrs, type, offset, context, typeformat_offset);
    }
    case TGT_INVALID:
        break;
    }
    error("invalid type %s for var %s\n", type->name, name);
    return 0;
}

static void process_tfs_iface(type_t *iface, FILE *file, int indent, unsigned int *offset)
{
    const statement_list_t *stmts = type_iface_get_stmts(iface);
    const statement_t *stmt;
    var_t *var;

    current_iface = iface;
    if (stmts) LIST_FOR_EACH_ENTRY( stmt, stmts, statement_t, entry )
    {
        switch(stmt->type)
        {
        case STMT_DECLARATION:
        {
            const var_t *func;
            const var_list_t *args;

            if(stmt->u.var->declspec.stgclass != STG_NONE
               || type_get_type_detect_alias(stmt->u.var->declspec.type) != TYPE_FUNCTION)
                continue;

            current_func = func = stmt->u.var;
            if (is_local(func->attrs)) continue;

            current_arg = var = type_function_get_retval(func->declspec.type);
            if (!is_void(var->declspec.type))
                var->typestring_offset = write_type_tfs( file, var->attrs, TRUE, var->declspec.type, func->name,
                                                         TYPE_CONTEXT_RETVAL, offset);

            args = type_function_get_args(func->declspec.type);
            if (args) LIST_FOR_EACH_ENTRY( var, args, var_t, entry )
            {
                current_arg = var;
                var->typestring_offset = write_type_tfs( file, var->attrs, TRUE, var->declspec.type, var->name,
                                                         TYPE_CONTEXT_TOPLEVELPARAM, offset );
            }
            break;

        }
        case STMT_TYPEDEF:
        {
            typeref_t *ref;
            if (stmt->u.type_list) LIST_FOR_EACH_ENTRY(ref, stmt->u.type_list, typeref_t, entry)
            {
                if (is_attr(ref->type->attrs, ATTR_ENCODE)
                    || is_attr(ref->type->attrs, ATTR_DECODE))
                    ref->type->typestring_offset = write_type_tfs( file,
                            ref->type->attrs, TRUE, ref->type, ref->type->name,
                            TYPE_CONTEXT_CONTAINER, offset);
            }
            break;
        }
        default:
            break;
        }
    }
}

static unsigned int process_tfs(FILE *file, const statement_list_t *stmts, type_pred_t pred)
{
    unsigned int typeformat_offset = 2;
    for_each_iface(stmts, process_tfs_iface, pred, file, 0, &typeformat_offset);
    return typeformat_offset + 1;
}


void write_typeformatstring(FILE *file, const statement_list_t *stmts, type_pred_t pred)
{
    int indent = 0;

    print_file(file, indent, "static const MIDL_TYPE_FORMAT_STRING __MIDL_TypeFormatString =\n");
    print_file(file, indent, "{\n");
    indent++;
    print_file(file, indent, "0,\n");
    print_file(file, indent, "{\n");
    indent++;
    print_file(file, indent, "NdrFcShort(0x0),\n");

    set_all_tfswrite(TRUE);
    process_tfs(file, stmts, pred);

    print_file(file, indent, "0x0\n");
    indent--;
    print_file(file, indent, "}\n");
    indent--;
    print_file(file, indent, "};\n");
    print_file(file, indent, "\n");
}

static unsigned int get_required_buffer_size_type(const type_t *type, const char *name,
        const attr_list_t *attrs, int toplevel_attrs, int toplevel_param, unsigned int *alignment)
{
    *alignment = 0;
    switch (typegen_detect_type(type, attrs, TDT_IGNORE_RANGES))
    {
    case TGT_USER_TYPE:
    {
        const char *uname = NULL;
        const type_t *utype = get_user_type(type, &uname);
        return get_required_buffer_size_type(utype, uname, NULL, FALSE, FALSE, alignment);
    }
    case TGT_BASIC:
        switch (get_basic_fc(type))
        {
        case FC_BYTE:
        case FC_CHAR:
        case FC_USMALL:
        case FC_SMALL:
            *alignment = 4;
            return 1;

        case FC_WCHAR:
        case FC_USHORT:
        case FC_SHORT:
            *alignment = 4;
            return 2;

        case FC_ULONG:
        case FC_LONG:
        case FC_FLOAT:
        case FC_ERROR_STATUS_T:
            *alignment = 4;
            return 4;

        case FC_HYPER:
        case FC_DOUBLE:
            *alignment = 8;
            return 8;

        case FC_INT3264:
        case FC_UINT3264:
            assert( pointer_size );
            *alignment = pointer_size;
            return pointer_size;

        case FC_IGNORE:
        case FC_BIND_PRIMITIVE:
            return 0;

        default:
            error("get_required_buffer_size: unknown basic type 0x%02x\n",
                  get_basic_fc(type));
            return 0;
        }
        break;

    case TGT_ENUM:
        switch (get_enum_fc(type))
        {
        case FC_ENUM32:
            *alignment = 4;
            return 4;
        case FC_ENUM16:
            *alignment = 4;
            return 2;
        }
        break;

    case TGT_STRUCT:
        if (get_struct_fc(type) == FC_STRUCT)
        {
            if (!type_struct_get_fields(type)) return 0;
            return fields_memsize(type_struct_get_fields(type), alignment);
        }
        break;

    case TGT_POINTER:
        {
            unsigned int size, align;
            const type_t *ref = type_pointer_get_ref_type(type);
            if (is_string_type( attrs, ref )) break;
            if (!(size = get_required_buffer_size_type( ref, name, attrs, FALSE, FALSE, &align ))) break;
            if (get_pointer_fc(type, attrs, toplevel_attrs, toplevel_param) != FC_RP)
            {
                size += 4 + align;
                align = 4;
            }
            *alignment = align;
            return size;
        }

    case TGT_ARRAY:
        switch (get_array_fc(type, attrs))
        {
        case FC_SMFARRAY:
        case FC_LGFARRAY:
            return type_array_get_dim(type) *
                get_required_buffer_size_type(type_array_get_element_type(type), name,
                                              attrs, FALSE, FALSE, alignment);
        }
        break;

    default:
        break;
    }
    return 0;
}

static unsigned int get_required_buffer_size(const var_t *var, unsigned int *alignment, enum pass pass)
{
    int in_attr = is_attr(var->attrs, ATTR_IN);
    int out_attr = is_attr(var->attrs, ATTR_OUT);

    if (!in_attr && !out_attr)
        in_attr = 1;

    *alignment = 0;

    if ((pass == PASS_IN && in_attr) || (pass == PASS_OUT && out_attr) ||
        pass == PASS_RETURN)
    {
        if (is_ptrchain_attr(var, ATTR_CONTEXTHANDLE))
        {
            *alignment = 4;
            return 20;
        }

        if (!is_string_type(var->attrs, var->declspec.type))
            return get_required_buffer_size_type(var->declspec.type, var->name,
                                                 var->attrs, TRUE, TRUE, alignment);
    }
    return 0;
}

static unsigned int get_function_buffer_size( const var_t *func, enum pass pass )
{
    const var_t *var;
    unsigned int total_size = 0, alignment;

    if (type_function_get_args(func->declspec.type))
    {
        LIST_FOR_EACH_ENTRY( var, type_function_get_args(func->declspec.type), const var_t, entry )
        {
            total_size += get_required_buffer_size(var, &alignment, pass);
            total_size += alignment;
        }
    }

    if (pass == PASS_OUT && !is_void(type_function_get_rettype(func->declspec.type)))
    {
        var_t v = *func;
        v.declspec.type = type_function_get_rettype(func->declspec.type);
        total_size += get_required_buffer_size(&v, &alignment, PASS_RETURN);
        total_size += alignment;
    }
    return total_size;
}

static void print_phase_function(FILE *file, int indent, const char *type,
                                 const char *local_var_prefix, enum remoting_phase phase,
                                 const var_t *var, unsigned int type_offset)
{
    const char *function;
    switch (phase)
    {
    case PHASE_BUFFERSIZE:
        function = "BufferSize";
        break;
    case PHASE_MARSHAL:
        function = "Marshall";
        break;
    case PHASE_UNMARSHAL:
        function = "Unmarshall";
        break;
    case PHASE_FREE:
        function = "Free";
        break;
    default:
        assert(0);
        return;
    }

    print_file(file, indent, "Ndr%s%s(\n", type, function);
    indent++;
    print_file(file, indent, "&__frame->_StubMsg,\n");
    print_file(file, indent, "%s%s%s%s%s,\n",
               (phase == PHASE_UNMARSHAL) ? "(unsigned char **)" : "(unsigned char *)",
               (phase == PHASE_UNMARSHAL || decl_indirect(var->declspec.type)) ? "&" : "",
               local_var_prefix,
               (phase == PHASE_UNMARSHAL && decl_indirect(var->declspec.type)) ? "_p_" : "",
               var->name);
    print_file(file, indent, "(PFORMAT_STRING)&__MIDL_TypeFormatString.Format[%d]%s\n",
               type_offset, (phase == PHASE_UNMARSHAL) ? "," : ");");
    if (phase == PHASE_UNMARSHAL)
        print_file(file, indent, "0);\n");
    indent--;
}

void print_phase_basetype(FILE *file, int indent, const char *local_var_prefix,
                          enum remoting_phase phase, enum pass pass, const var_t *var,
                          const char *varname)
{
    type_t *type = var->declspec.type;
    unsigned int alignment = 0;

    /* no work to do for other phases, buffer sizing is done elsewhere */
    if (phase != PHASE_MARSHAL && phase != PHASE_UNMARSHAL)
        return;

    if (type_get_type(type) == TYPE_ENUM ||
        (type_get_type(type) == TYPE_BASIC &&
         type_basic_get_type(type) == TYPE_BASIC_INT3264 &&
         pointer_size != 4))
    {
        unsigned char fc;

        if (type_get_type(type) == TYPE_ENUM)
            fc = get_enum_fc(type);
        else
            fc = get_basic_fc(type);

        if (phase == PHASE_MARSHAL)
            print_file(file, indent, "NdrSimpleTypeMarshall(\n");
        else
            print_file(file, indent, "NdrSimpleTypeUnmarshall(\n");
        print_file(file, indent+1, "&__frame->_StubMsg,\n");
        print_file(file, indent+1, "(unsigned char *)&%s%s,\n",
                   local_var_prefix,
                   var->name);
        print_file(file, indent+1, "0x%02x /* %s */);\n", fc, string_of_type(fc));
    }
    else
    {
        const decl_spec_t *ref = is_ptr(type) ? type_pointer_get_ref(type) : &var->declspec;

        switch (get_basic_fc(ref->type))
        {
        case FC_BYTE:
        case FC_CHAR:
        case FC_SMALL:
        case FC_USMALL:
            alignment = 1;
            break;

        case FC_WCHAR:
        case FC_USHORT:
        case FC_SHORT:
            alignment = 2;
            break;

        case FC_ULONG:
        case FC_LONG:
        case FC_FLOAT:
        case FC_ERROR_STATUS_T:
        /* pointer_size must be 4 if we got here in these two cases */
        case FC_INT3264:
        case FC_UINT3264:
            alignment = 4;
            break;

        case FC_HYPER:
        case FC_DOUBLE:
            alignment = 8;
            break;

        case FC_IGNORE:
        case FC_BIND_PRIMITIVE:
            /* no marshalling needed */
            return;

        default:
            error("print_phase_basetype: Unsupported type: %s (0x%02x, ptr_level: 0)\n",
                  var->name, get_basic_fc(ref->type));
        }

        if (phase == PHASE_MARSHAL && alignment > 1)
            print_file(file, indent, "MIDL_memset(__frame->_StubMsg.Buffer, 0, (0x%x - (ULONG_PTR)__frame->_StubMsg.Buffer) & 0x%x);\n", alignment, alignment - 1);
        print_file(file, indent, "__frame->_StubMsg.Buffer = (unsigned char *)(((ULONG_PTR)__frame->_StubMsg.Buffer + %u) & ~0x%x);\n",
                    alignment - 1, alignment - 1);

        if (phase == PHASE_MARSHAL)
        {
            print_file(file, indent, "*(");
            write_type_decl(file, ref, NULL);
            if (is_ptr(type))
                fprintf(file, " *)__frame->_StubMsg.Buffer = *");
            else
                fprintf(file, " *)__frame->_StubMsg.Buffer = ");
            fprintf(file, "%s%s", local_var_prefix, varname);
            fprintf(file, ";\n");
        }
        else if (phase == PHASE_UNMARSHAL)
        {
            print_file(file, indent, "if (__frame->_StubMsg.Buffer + sizeof(");
            write_type_decl(file, ref, NULL);
            fprintf(file, ") > __frame->_StubMsg.BufferEnd)\n");
            print_file(file, indent, "{\n");
            print_file(file, indent + 1, "RpcRaiseException(RPC_X_BAD_STUB_DATA);\n");
            print_file(file, indent, "}\n");
            print_file(file, indent, "%s%s%s",
                       (pass == PASS_IN || pass == PASS_RETURN) ? "" : "*",
                       local_var_prefix, varname);
            if (pass == PASS_IN && is_ptr(type))
                fprintf(file, " = (");
            else
                fprintf(file, " = *(");
            write_type_decl(file, ref, NULL);
            fprintf(file, " *)__frame->_StubMsg.Buffer;\n");
        }

        print_file(file, indent, "__frame->_StubMsg.Buffer += sizeof(");
        write_type_decl(file, ref, NULL);
        fprintf(file, ");\n");
    }
}

/* returns whether the MaxCount, Offset or ActualCount members need to be
 * filled in for the specified phase */
static inline int is_conformance_needed_for_phase(enum remoting_phase phase)
{
    return (phase != PHASE_UNMARSHAL);
}

expr_t *get_size_is_expr(const type_t *t, const char *name)
{
    expr_t *x = NULL;

    for ( ; is_array(t); t = type_array_get_element_type(t))
        if (type_array_has_conformance(t) &&
            type_array_get_conformance(t)->type != EXPR_VOID)
        {
            if (!x)
                x = type_array_get_conformance(t);
            else
                error("%s: multidimensional conformant"
                      " arrays not supported at the top level\n",
                      name);
        }

    return x;
}

void write_parameter_conf_or_var_exprs(FILE *file, int indent, const char *local_var_prefix,
                                       enum remoting_phase phase, const var_t *var, int valid_variance)
{
    const type_t *type = var->declspec.type;
    /* get fundamental type for the argument */
    for (;;)
    {
        switch (typegen_detect_type(type, var->attrs, TDT_IGNORE_STRINGS|TDT_IGNORE_RANGES))
        {
        case TGT_ARRAY:
            if (is_conformance_needed_for_phase(phase))
            {
                if (type_array_has_conformance(type) &&
                    type_array_get_conformance(type)->type != EXPR_VOID)
                {
                    print_file(file, indent, "__frame->_StubMsg.MaxCount = (ULONG_PTR)");
                    write_expr(file, type_array_get_conformance(type), 1, 1, NULL, NULL, local_var_prefix);
                    fprintf(file, ";\n\n");
                }
                if (type_array_has_variance(type))
                {
                    print_file(file, indent, "__frame->_StubMsg.Offset = 0;\n"); /* FIXME */
                    if (valid_variance)
                    {
                        print_file(file, indent, "__frame->_StubMsg.ActualCount = (ULONG_PTR)");
                        write_expr(file, type_array_get_variance(type), 1, 1, NULL, NULL, local_var_prefix);
                        fprintf(file, ";\n\n");
                    }
                    else
                        print_file(file, indent, "__frame->_StubMsg.ActualCount = __frame->_StubMsg.MaxCount;\n\n");
                }
            }
            break;
        case TGT_UNION:
            if (type_get_type(type) == TYPE_UNION &&
                is_conformance_needed_for_phase(phase))
            {
                print_file(file, indent, "__frame->_StubMsg.MaxCount = (ULONG_PTR)");
                write_expr(file, get_attrp(var->attrs, ATTR_SWITCHIS), 1, 1, NULL, NULL, local_var_prefix);
                fprintf(file, ";\n\n");
            }
            break;
        case TGT_IFACE_POINTER:
        {
            expr_t *iid;

            if (is_conformance_needed_for_phase(phase) && (iid = get_attrp( var->attrs, ATTR_IIDIS )))
            {
                print_file( file, indent, "__frame->_StubMsg.MaxCount = (ULONG_PTR) " );
                write_expr( file, iid, 1, 1, NULL, NULL, local_var_prefix );
                fprintf( file, ";\n\n" );
            }
            break;
        }
        case TGT_POINTER:
            type = type_pointer_get_ref_type(type);
            continue;
        case TGT_INVALID:
        case TGT_USER_TYPE:
        case TGT_CTXT_HANDLE:
        case TGT_CTXT_HANDLE_POINTER:
        case TGT_STRING:
        case TGT_BASIC:
        case TGT_ENUM:
        case TGT_STRUCT:
        case TGT_RANGE:
            break;
        }
        break;
    }
}

static void write_remoting_arg(FILE *file, int indent, const var_t *func, const char *local_var_prefix,
                               enum pass pass, enum remoting_phase phase, const var_t *var)
{
    int in_attr, out_attr, pointer_type;
    const char *type_str = NULL;
    const type_t *type = var->declspec.type;
    unsigned int alignment, start_offset = type->typestring_offset;

    if (is_ptr(type) || is_array(type))
        pointer_type = get_pointer_fc(type, var->attrs, TRUE, pass != PASS_RETURN);
    else
        pointer_type = 0;

    in_attr = is_attr(var->attrs, ATTR_IN);
    out_attr = is_attr(var->attrs, ATTR_OUT);
    if (!in_attr && !out_attr)
        in_attr = 1;

    if (phase != PHASE_FREE)
        switch (pass)
        {
        case PASS_IN:
            if (!in_attr) return;
            break;
        case PASS_OUT:
            if (!out_attr) return;
            break;
        case PASS_RETURN:
            break;
        }

    if (phase == PHASE_BUFFERSIZE && get_required_buffer_size( var, &alignment, pass )) return;

    write_parameter_conf_or_var_exprs(file, indent, local_var_prefix, phase, var, TRUE);

    switch (typegen_detect_type(type, var->attrs, TDT_ALL_TYPES))
    {
    case TGT_CTXT_HANDLE:
    case TGT_CTXT_HANDLE_POINTER:
        if (phase == PHASE_MARSHAL)
        {
            if (pass == PASS_IN)
            {
                /* if the context_handle attribute appears in the chain of types
                 * without pointers being followed, then the context handle must
                 * be direct, otherwise it is a pointer */
                const char *ch_ptr = is_aliaschain_attr(type, ATTR_CONTEXTHANDLE) ? "" : "*";
                print_file(file, indent, "NdrClientContextMarshall(\n");
                print_file(file, indent + 1, "&__frame->_StubMsg,\n");
                print_file(file, indent + 1, "(NDR_CCONTEXT)%s%s%s,\n", ch_ptr, local_var_prefix,
                           var->name);
                print_file(file, indent + 1, "%s);\n", in_attr && out_attr ? "1" : "0");
            }
            else
            {
                print_file(file, indent, "NdrServerContextNewMarshall(\n");
                print_file(file, indent + 1, "&__frame->_StubMsg,\n");
                print_file(file, indent + 1, "(NDR_SCONTEXT)%s%s,\n", local_var_prefix, var->name);
                print_file(file, indent + 1, "(NDR_RUNDOWN)%s_rundown,\n", get_context_handle_type_name(var->declspec.type));
                print_file(file, indent + 1, "(PFORMAT_STRING)&__MIDL_TypeFormatString.Format[%d]);\n", start_offset);
            }
        }
        else if (phase == PHASE_UNMARSHAL)
        {
            if (pass == PASS_OUT || pass == PASS_RETURN)
            {
                if (!in_attr)
                    print_file(file, indent, "*%s%s = 0;\n", local_var_prefix, var->name);
                print_file(file, indent, "NdrClientContextUnmarshall(\n");
                print_file(file, indent + 1, "&__frame->_StubMsg,\n");
                print_file(file, indent + 1, "(NDR_CCONTEXT *)%s%s%s,\n",
                           pass == PASS_RETURN ? "&" : "", local_var_prefix, var->name);
                print_file(file, indent + 1, "__frame->_Handle);\n");
            }
            else
            {
                print_file(file, indent, "%s%s = NdrServerContextNewUnmarshall(\n", local_var_prefix, var->name);
                print_file(file, indent + 1, "&__frame->_StubMsg,\n");
                print_file(file, indent + 1, "(PFORMAT_STRING)&__MIDL_TypeFormatString.Format[%d]);\n", start_offset);
            }
        }
        break;
    case TGT_USER_TYPE:
        print_phase_function(file, indent, "UserMarshal", local_var_prefix, phase, var, start_offset);
        break;
    case TGT_STRING:
        if (phase == PHASE_FREE || pass == PASS_RETURN ||
            pointer_type != FC_RP)
        {
            /* strings returned are assumed to be global and hence don't
             * need freeing */
            if (is_declptr(type) && !(phase == PHASE_FREE && pass == PASS_RETURN))
                print_phase_function(file, indent, "Pointer", local_var_prefix,
                                     phase, var, start_offset);
            else if (pointer_type == FC_RP && phase == PHASE_FREE &&
                !in_attr && is_conformant_array(type))
            {
                print_file(file, indent, "if (%s%s)\n", local_var_prefix, var->name);
                indent++;
                print_file(file, indent, "__frame->_StubMsg.pfnFree((void*)%s%s);\n", local_var_prefix, var->name);
            }
        }
        else
        {
            unsigned int real_start_offset = start_offset;
            /* skip over pointer description straight to string description */
            if (is_declptr(type))
            {
                if (is_conformant_array(type))
                    real_start_offset += 4;
                else
                    real_start_offset += 2;
            }
            if (is_array(type) && !is_conformant_array(type))
                print_phase_function(file, indent, "NonConformantString",
                                     local_var_prefix, phase, var,
                                     real_start_offset);
            else
                print_phase_function(file, indent, "ConformantString", local_var_prefix,
                                     phase, var, real_start_offset);
        }
        break;
    case TGT_ARRAY:
    {
        unsigned char tc = get_array_fc(type, var->attrs);
        const char *array_type = NULL;

        /* We already have the size_is expression since it's at the
           top level, but do checks for multidimensional conformant
           arrays.  When we handle them, we'll need to extend this
           function to return a list, and then we'll actually use
           the return value.  */
        get_size_is_expr(type, var->name);

        switch (tc)
        {
        case FC_SMFARRAY:
        case FC_LGFARRAY:
            array_type = "FixedArray";
            break;
        case FC_SMVARRAY:
        case FC_LGVARRAY:
            array_type = "VaryingArray";
            break;
        case FC_CARRAY:
            array_type = "ConformantArray";
            break;
        case FC_CVARRAY:
            array_type = "ConformantVaryingArray";
            break;
        case FC_BOGUS_ARRAY:
            array_type = "ComplexArray";
            break;
        }

        if (pointer_type != FC_RP) array_type = "Pointer";

        if (phase == PHASE_FREE && pointer_type == FC_RP)
        {
            /* these are all unmarshalled by allocating memory */
            if (tc == FC_BOGUS_ARRAY ||
                tc == FC_CVARRAY ||
                ((tc == FC_SMVARRAY || tc == FC_LGVARRAY) && in_attr) ||
                (tc == FC_CARRAY && !in_attr))
            {
                if (type_array_is_decl_as_ptr(type) && type_array_get_ptr_tfsoff(type))
                {
                    print_phase_function(file, indent, "Pointer", local_var_prefix, phase, var,
                                         type_array_get_ptr_tfsoff(type));
                    break;
                }
                print_phase_function(file, indent, array_type, local_var_prefix, phase, var, start_offset);
                print_file(file, indent, "if (%s%s)\n", local_var_prefix, var->name);
                indent++;
                print_file(file, indent, "__frame->_StubMsg.pfnFree((void*)%s%s);\n", local_var_prefix, var->name);
                break;
            }
        }
        print_phase_function(file, indent, array_type, local_var_prefix, phase, var, start_offset);
        break;
    }
    case TGT_BASIC:
        print_phase_basetype(file, indent, local_var_prefix, phase, pass, var, var->name);
        break;
    case TGT_ENUM:
        print_phase_basetype(file, indent, local_var_prefix, phase, pass, var, var->name);
        break;
    case TGT_RANGE:
        print_phase_basetype(file, indent, local_var_prefix, phase, pass, var, var->name);
        /* Note: this goes beyond what MIDL does - it only supports arguments
         * with the [range] attribute in Oicf mode */
        if (phase == PHASE_UNMARSHAL)
        {
            const expr_t *range_min;
            const expr_t *range_max;
            expr_list_t *range_list = get_attrp(var->attrs, ATTR_RANGE);
            if (!range_list)
                range_list = get_aliaschain_attrp(type, ATTR_RANGE);
            range_min = LIST_ENTRY(list_head(range_list), const expr_t, entry);
            range_max = LIST_ENTRY(list_next(range_list, list_head(range_list)), const expr_t, entry);

            print_file(file, indent, "if ((%s%s < (", local_var_prefix, var->name);
            write_type_decl(file, &var->declspec, NULL);
            fprintf(file, ")0x%x) || (%s%s > (", range_min->cval, local_var_prefix, var->name);
            write_type_decl(file, &var->declspec, NULL);
            fprintf(file, ")0x%x))\n", range_max->cval);
            print_file(file, indent, "{\n");
            print_file(file, indent+1, "RpcRaiseException(RPC_S_INVALID_BOUND);\n");
            print_file(file, indent, "}\n");
        }
        break;
    case TGT_STRUCT:
        switch (get_struct_fc(type))
        {
        case FC_STRUCT:
            if (phase == PHASE_MARSHAL || phase == PHASE_UNMARSHAL)
                print_phase_function(file, indent, "SimpleStruct", local_var_prefix, phase, var, start_offset);
            break;
        case FC_PSTRUCT:
            print_phase_function(file, indent, "SimpleStruct", local_var_prefix, phase, var, start_offset);
            break;
        case FC_CSTRUCT:
        case FC_CPSTRUCT:
            print_phase_function(file, indent, "ConformantStruct", local_var_prefix, phase, var, start_offset);
            break;
        case FC_CVSTRUCT:
            print_phase_function(file, indent, "ConformantVaryingStruct", local_var_prefix, phase, var, start_offset);
            break;
        case FC_BOGUS_STRUCT:
            print_phase_function(file, indent, "ComplexStruct", local_var_prefix, phase, var, start_offset);
            break;
        default:
            error("write_remoting_arguments: Unsupported type: %s (0x%02x)\n", var->name, get_struct_fc(type));
        }
        break;
    case TGT_UNION:
    {
        const char *union_type = NULL;

        if (type_get_type(type) == TYPE_UNION)
            union_type = "NonEncapsulatedUnion";
        else if (type_get_type(type) == TYPE_ENCAPSULATED_UNION)
            union_type = "EncapsulatedUnion";

        print_phase_function(file, indent, union_type, local_var_prefix,
                             phase, var, start_offset);
        break;
    }
    case TGT_POINTER:
    {
        const type_t *ref = type_pointer_get_ref_type(type);
        if (pointer_type == FC_RP) switch (typegen_detect_type(ref, var->attrs, TDT_ALL_TYPES))
        {
        case TGT_BASIC:
            print_phase_basetype(file, indent, local_var_prefix, phase, pass, var, var->name);
            break;
        case TGT_ENUM:
            /* base types have known sizes, so don't need a sizing pass
             * and don't have any memory to free and so don't need a
             * freeing pass */
            if (phase == PHASE_MARSHAL || phase == PHASE_UNMARSHAL)
                print_phase_function(file, indent, "Pointer", local_var_prefix, phase, var, start_offset);
            break;
        case TGT_STRUCT:
            switch (get_struct_fc(ref))
            {
            case FC_STRUCT:
                /* simple structs have known sizes, so don't need a sizing
                 * pass and don't have any memory to free and so don't
                 * need a freeing pass */
                if (phase == PHASE_MARSHAL || phase == PHASE_UNMARSHAL)
                    type_str = "SimpleStruct";
                else if (phase == PHASE_FREE && pass == PASS_RETURN)
                {
                    print_file(file, indent, "if (%s%s)\n", local_var_prefix, var->name);
                    indent++;
                    print_file(file, indent, "__frame->_StubMsg.pfnFree((void*)%s%s);\n", local_var_prefix, var->name);
                    indent--;
                }
                break;
            case FC_PSTRUCT:
                type_str = "SimpleStruct";
                break;
            case FC_CSTRUCT:
            case FC_CPSTRUCT:
                type_str = "ConformantStruct";
                break;
            case FC_CVSTRUCT:
                type_str = "ConformantVaryingStruct";
                break;
            case FC_BOGUS_STRUCT:
                type_str = "ComplexStruct";
                break;
            default:
                error("write_remoting_arguments: Unsupported type: %s (0x%02x)\n", var->name, get_struct_fc(ref));
            }

            if (type_str)
            {
                if (phase == PHASE_FREE)
                    type_str = "Pointer";
                else
                    start_offset = ref->typestring_offset;
                print_phase_function(file, indent, type_str, local_var_prefix, phase, var, start_offset);
            }
            break;
        case TGT_UNION:
            if (phase == PHASE_FREE)
                type_str = "Pointer";
            else
            {
                if (type_get_type(ref) == TYPE_UNION)
                    type_str = "NonEncapsulatedUnion";
                else if (type_get_type(ref) == TYPE_ENCAPSULATED_UNION)
                    type_str = "EncapsulatedUnion";

                start_offset = ref->typestring_offset;
            }

            print_phase_function(file, indent, type_str, local_var_prefix,
                                 phase, var, start_offset);
            break;
        case TGT_USER_TYPE:
            if (phase != PHASE_FREE)
            {
                type_str = "UserMarshal";
                start_offset = ref->typestring_offset;
            }
            else type_str = "Pointer";

            print_phase_function(file, indent, type_str, local_var_prefix, phase, var, start_offset);
            break;
        case TGT_STRING:
        case TGT_POINTER:
        case TGT_ARRAY:
        case TGT_RANGE:
        case TGT_IFACE_POINTER:
        case TGT_CTXT_HANDLE:
        case TGT_CTXT_HANDLE_POINTER:
            print_phase_function(file, indent, "Pointer", local_var_prefix, phase, var, start_offset);
            break;
        case TGT_INVALID:
            assert(0);
            break;
        }
        else
            print_phase_function(file, indent, "Pointer", local_var_prefix, phase, var, start_offset);
        break;
    }
    case TGT_IFACE_POINTER:
        print_phase_function(file, indent, "InterfacePointer", local_var_prefix, phase, var, start_offset);
        break;
    case TGT_INVALID:
        assert(0);
        break;
    }
    fprintf(file, "\n");
}

void write_remoting_arguments(FILE *file, int indent, const var_t *func, const char *local_var_prefix,
                              enum pass pass, enum remoting_phase phase)
{
    if (phase == PHASE_BUFFERSIZE && pass != PASS_RETURN)
    {
        unsigned int size = get_function_buffer_size( func, pass );
        print_file(file, indent, "__frame->_StubMsg.BufferLength = %u;\n", size);
    }

    if (pass == PASS_RETURN)
    {
        write_remoting_arg( file, indent, func, local_var_prefix, pass, phase,
                            type_function_get_retval(func->declspec.type) );
    }
    else
    {
        const var_t *var;
        if (!type_function_get_args(func->declspec.type))
            return;
        LIST_FOR_EACH_ENTRY( var, type_function_get_args(func->declspec.type), const var_t, entry )
            write_remoting_arg( file, indent, func, local_var_prefix, pass, phase, var );
    }
}


unsigned int get_size_procformatstring_func(const type_t *iface, const var_t *func)
{
    unsigned int offset = 0;
    write_procformatstring_func( NULL, 0, iface, func, &offset, 0 );
    return offset;
}

static void get_size_procformatstring_iface(type_t *iface, FILE *file, int indent, unsigned int *size)
{
    const statement_t *stmt;
    STATEMENTS_FOR_EACH_FUNC( stmt, type_iface_get_stmts(iface) )
    {
        const var_t *func = stmt->u.var;
        if (!is_local(func->attrs))
            *size += get_size_procformatstring_func( iface, func );
    }
}

unsigned int get_size_procformatstring(const statement_list_t *stmts, type_pred_t pred)
{
    unsigned int size = 1;
    for_each_iface(stmts, get_size_procformatstring_iface, pred, NULL, 0, &size);
    return size;
}

unsigned int get_size_typeformatstring(const statement_list_t *stmts, type_pred_t pred)
{
    set_all_tfswrite(FALSE);
    return process_tfs(NULL, stmts, pred);
}

void declare_stub_args( FILE *file, int indent, const var_t *func )
{
    int in_attr, out_attr;
    int i = 0;
    var_t *var = type_function_get_retval(func->declspec.type);

    /* declare return value */
    if (!is_void(var->declspec.type))
    {
        if (is_context_handle(var->declspec.type))
            print_file(file, indent, "NDR_SCONTEXT %s;\n", var->name);
        else
        {
            print_file(file, indent, "%s", "");
            write_type_decl(file, &var->declspec, var->name);
            fprintf(file, ";\n");
        }
    }

    if (!type_function_get_args(func->declspec.type))
        return;

    LIST_FOR_EACH_ENTRY( var, type_function_get_args(func->declspec.type), var_t, entry )
    {
        in_attr = is_attr(var->attrs, ATTR_IN);
        out_attr = is_attr(var->attrs, ATTR_OUT);
        if (!out_attr && !in_attr)
            in_attr = 1;

        if (is_context_handle(var->declspec.type))
            print_file(file, indent, "NDR_SCONTEXT %s;\n", var->name);
        else
        {
            if (!in_attr && !is_conformant_array(var->declspec.type))
            {
                const decl_spec_t *type_to_print;
                char name[16];
                print_file(file, indent, "%s", "");
                if (type_get_type(var->declspec.type) == TYPE_ARRAY &&
                    !type_array_is_decl_as_ptr(var->declspec.type))
                    type_to_print = &var->declspec;
                else
                    type_to_print = type_pointer_get_ref(var->declspec.type);
                snprintf(name, sizeof(name), "_W%u", i++);
                write_type_decl(file, type_to_print, name);
                fprintf(file, ";\n");
            }

            print_file(file, indent, "%s", "");
            write_type_decl_left(file, &var->declspec);
            fprintf(file, " ");
            if (type_get_type(var->declspec.type) == TYPE_ARRAY &&
                !type_array_is_decl_as_ptr(var->declspec.type)) {
                fprintf(file, "(*%s)", var->name);
            } else
                fprintf(file, "%s", var->name);
            write_type_right(file, var->declspec.type, FALSE);
            fprintf(file, ";\n");

            if (decl_indirect(var->declspec.type))
                print_file(file, indent, "void *_p_%s;\n", var->name);
        }
    }
}


void assign_stub_out_args( FILE *file, int indent, const var_t *func, const char *local_var_prefix )
{
    int in_attr, out_attr;
    int i = 0, sep = 0;
    const var_t *var;
    type_t *ref;

    if (!type_function_get_args(func->declspec.type))
        return;

    LIST_FOR_EACH_ENTRY( var, type_function_get_args(func->declspec.type), const var_t, entry )
    {
        in_attr = is_attr(var->attrs, ATTR_IN);
        out_attr = is_attr(var->attrs, ATTR_OUT);
        if (!out_attr && !in_attr)
            in_attr = 1;

        if (!in_attr)
        {
            print_file(file, indent, "%s%s", local_var_prefix, var->name);

            switch (typegen_detect_type(var->declspec.type, var->attrs, TDT_IGNORE_STRINGS))
            {
            case TGT_CTXT_HANDLE_POINTER:
                fprintf(file, " = NdrContextHandleInitialize(\n");
                print_file(file, indent + 1, "&__frame->_StubMsg,\n");
                print_file(file, indent + 1, "(PFORMAT_STRING)&__MIDL_TypeFormatString.Format[%d]);\n",
                           var->typestring_offset);
                break;
            case TGT_ARRAY:
                if (type_array_has_conformance(var->declspec.type))
                {
                    unsigned int size;
                    type_t *type;

                    fprintf(file, " = NdrAllocate(&__frame->_StubMsg, ");
                    for (type = var->declspec.type;
                         is_array(type) && type_array_has_conformance(type);
                         type = type_array_get_element_type(type))
                    {
                        write_expr(file, type_array_get_conformance(type), TRUE,
                                   TRUE, NULL, NULL, local_var_prefix);
                        fprintf(file, " * ");
                    }
                    size = type_memsize(type);
                    fprintf(file, "%u);\n", size);

                    print_file(file, indent, "memset(%s%s, 0, ", local_var_prefix, var->name);
                    for (type = var->declspec.type;
                         is_array(type) && type_array_has_conformance(type);
                         type = type_array_get_element_type(type))
                    {
                        write_expr(file, type_array_get_conformance(type), TRUE,
                                   TRUE, NULL, NULL, local_var_prefix);
                        fprintf(file, " * ");
                    }
                    size = type_memsize(type);
                    fprintf(file, "%u);\n", size);
                }
                else
                    fprintf(file, " = &%s_W%u;\n", local_var_prefix, i++);
                break;
            case TGT_POINTER:
                fprintf(file, " = &%s_W%u;\n", local_var_prefix, i);
                ref = type_pointer_get_ref_type(var->declspec.type);
                switch (typegen_detect_type(ref, var->attrs, TDT_IGNORE_STRINGS))
                {
                case TGT_BASIC:
                case TGT_ENUM:
                case TGT_POINTER:
                case TGT_RANGE:
                case TGT_IFACE_POINTER:
                    print_file(file, indent, "%s_W%u = 0;\n", local_var_prefix, i);
                    break;
                case TGT_USER_TYPE:
                    print_file(file, indent, "memset(&%s_W%u, 0, sizeof(%s_W%u));\n",
                               local_var_prefix, i, local_var_prefix, i);
                    break;
                case TGT_ARRAY:
                    if (type_array_is_decl_as_ptr(ref))
                    {
                        print_file(file, indent, "%s_W%u = 0;\n", local_var_prefix, i);
                        break;
                    }
                    ref = type_array_get_element_type(ref);
                    /* fall through */
                case TGT_STRUCT:
                case TGT_UNION:
                    if (type_has_pointers(ref, var->attrs))
                        print_file(file, indent, "memset(&%s_W%u, 0, sizeof(%s_W%u));\n",
                                   local_var_prefix, i, local_var_prefix, i);
                    break;
                case TGT_CTXT_HANDLE:
                case TGT_CTXT_HANDLE_POINTER:
                case TGT_INVALID:
                case TGT_STRING:
                    /* not initialised */
                    break;
                }
                i++;
                break;
            default:
                break;
            }

            sep = 1;
        }
    }
    if (sep)
        fprintf(file, "\n");
}


void write_func_param_struct( FILE *file, const type_t *iface, const type_t *func,
                              const char *var_decl, int add_retval )
{
    var_t *retval = type_function_get_retval( func );
    const var_list_t *args = type_function_get_args( func );
    const var_t *arg;
    int needs_packing;
    unsigned int align = 0;

    if (args)
        LIST_FOR_EACH_ENTRY( arg, args, const var_t, entry )
            if (!is_array( arg->declspec.type )) type_memsize_and_alignment( arg->declspec.type, &align );

    needs_packing = (align > pointer_size);

    if (needs_packing) print_file( file, 0, "#include <pshpack%u.h>\n", pointer_size );
    print_file(file, 1, "struct _PARAM_STRUCT\n" );
    print_file(file, 1, "{\n" );
    if (is_object( iface )) print_file(file, 2, "%s *This;\n", iface->name );

    if (args) LIST_FOR_EACH_ENTRY( arg, args, const var_t, entry )
    {
        print_file(file, 2, "%s", "");
        write_type_left( file, &arg->declspec, NAME_DEFAULT, false, TRUE );
        if (needs_space_after( arg->declspec.type )) fputc( ' ', file );
        if (is_array( arg->declspec.type ) && !type_array_is_decl_as_ptr( arg->declspec.type )) fputc( '*', file );

        /* FIXME: should check for large args being passed by pointer */
        align = 0;
        if (is_array( arg->declspec.type ) || is_ptr( arg->declspec.type )) align = pointer_size;
        else type_memsize_and_alignment( arg->declspec.type, &align );

        if (align < pointer_size)
            fprintf( file, "DECLSPEC_ALIGN(%u) ", pointer_size );
        fprintf( file, "%s;\n", arg->name );
    }
    if (add_retval && !is_void( retval->declspec.type ))
    {
        print_file(file, 2, "%s", "");
        write_type_left( file, &retval->declspec, NAME_DEFAULT, false, TRUE );
        if (needs_space_after( retval->declspec.type )) fputc( ' ', file );
        if (!is_array( retval->declspec.type ) && !is_ptr( retval->declspec.type ) &&
            type_memsize( retval->declspec.type ) != pointer_size)
        {
            fprintf( file, "DECLSPEC_ALIGN(%u) ", pointer_size );
        }
        fprintf( file, "%s;\n", retval->name );
    }
    print_file(file, 1, "} %s;\n", var_decl );
    if (needs_packing) print_file( file, 0, "#include <poppack.h>\n" );
    print_file( file, 0, "\n" );
}

void write_pointer_checks( FILE *file, int indent, const var_t *func )
{
    const var_list_t *args = type_function_get_args( func->declspec.type );
    const var_t *var;

    if (!args) return;

    LIST_FOR_EACH_ENTRY( var, args, const var_t, entry )
        if (cant_be_null( var ))
            print_file( file, indent, "if (!%s) RpcRaiseException(RPC_X_NULL_REF_POINTER);\n", var->name );
}

int write_expr_eval_routines(FILE *file, const char *iface)
{
    static const char *var_name = "pS";
    static const char *var_name_expr = "pS->";
    int result = 0;
    struct expr_eval_routine *eval;
    unsigned short callback_offset = 0;

    LIST_FOR_EACH_ENTRY(eval, &expr_eval_routines, struct expr_eval_routine, entry)
    {
        const char *name = eval->name;
        result = 1;

        print_file(file, 0, "static void __RPC_USER %s_%sExprEval_%04u(PMIDL_STUB_MESSAGE pStubMsg)\n",
                   eval->iface ? eval->iface->name : iface, name, callback_offset);
        print_file(file, 0, "{\n");
        if (type_get_type( eval->cont_type ) == TYPE_FUNCTION)
        {
            write_func_param_struct( file, eval->iface, eval->cont_type,
                                     "*pS = (struct _PARAM_STRUCT *)pStubMsg->StackTop", FALSE );
        }
        else
        {
            decl_spec_t ds = {.type = (type_t *)eval->cont_type};
            print_file(file, 1, "%s", "");
            write_type_left(file, &ds, NAME_DEFAULT, false, TRUE);
            fprintf(file, " *%s = (", var_name);
            write_type_left(file, &ds, NAME_DEFAULT, false, TRUE);
            fprintf(file, " *)(pStubMsg->StackTop - %u);\n", eval->baseoff);
        }
        print_file(file, 1, "pStubMsg->Offset = 0;\n"); /* FIXME */
        print_file(file, 1, "pStubMsg->MaxCount = (ULONG_PTR)");
        write_expr(file, eval->expr, 1, 1, var_name_expr, eval->cont_type, "");
        fprintf(file, ";\n");
        print_file(file, 0, "}\n\n");
        callback_offset++;
    }
    return result;
}

void write_expr_eval_routine_list(FILE *file, const char *iface)
{
    struct expr_eval_routine *eval;
    struct expr_eval_routine *cursor;
    unsigned short callback_offset = 0;

    fprintf(file, "static const EXPR_EVAL ExprEvalRoutines[] =\n");
    fprintf(file, "{\n");

    LIST_FOR_EACH_ENTRY_SAFE(eval, cursor, &expr_eval_routines, struct expr_eval_routine, entry)
    {
        print_file(file, 1, "%s_%sExprEval_%04u,\n",
                   eval->iface ? eval->iface->name : iface, eval->name, callback_offset);
        callback_offset++;
        list_remove(&eval->entry);
        free(eval->name);
        free(eval);
    }

    fprintf(file, "};\n\n");
}

void write_user_quad_list(FILE *file)
{
    user_type_t *ut;

    if (list_empty(&user_type_list))
        return;

    fprintf(file, "static const USER_MARSHAL_ROUTINE_QUADRUPLE UserMarshalRoutines[] =\n");
    fprintf(file, "{\n");
    LIST_FOR_EACH_ENTRY(ut, &user_type_list, user_type_t, entry)
    {
        const char *sep = &ut->entry == list_tail(&user_type_list) ? "" : ",";
        print_file(file, 1, "{\n");
        print_file(file, 2, "(USER_MARSHAL_SIZING_ROUTINE)%s_UserSize,\n", ut->name);
        print_file(file, 2, "(USER_MARSHAL_MARSHALLING_ROUTINE)%s_UserMarshal,\n", ut->name);
        print_file(file, 2, "(USER_MARSHAL_UNMARSHALLING_ROUTINE)%s_UserUnmarshal,\n", ut->name);
        print_file(file, 2, "(USER_MARSHAL_FREEING_ROUTINE)%s_UserFree\n", ut->name);
        print_file(file, 1, "}%s\n", sep);
    }
    fprintf(file, "};\n\n");
}

void write_endpoints( FILE *f, const char *prefix, const str_list_t *list )
{
    const struct str_list_entry_t *endpoint;
    const char *p;

    /* this should be an array of RPC_PROTSEQ_ENDPOINT but we want const strings */
    print_file( f, 0, "static const unsigned char * const %s__RpcProtseqEndpoint[][2] =\n{\n", prefix );
    LIST_FOR_EACH_ENTRY( endpoint, list, const struct str_list_entry_t, entry )
    {
        print_file( f, 1, "{ (const unsigned char *)\"" );
        for (p = endpoint->str; *p && *p != ':'; p++)
        {
            if (*p == '"' || *p == '\\') fputc( '\\', f );
            fputc( *p, f );
        }
        if (!*p) goto error;
        if (p[1] != '[') goto error;

        fprintf( f, "\", (const unsigned char *)\"" );
        for (p += 2; *p && *p != ']'; p++)
        {
            if (*p == '"' || *p == '\\') fputc( '\\', f );
            fputc( *p, f );
        }
        if (*p != ']') goto error;
        fprintf( f, "\" },\n" );
    }
    print_file( f, 0, "};\n\n" );
    return;

error:
    error("Invalid endpoint syntax '%s'\n", endpoint->str);
}

void write_client_call_routine( FILE *file, const type_t *iface, const var_t *func,
                                const char *prefix, unsigned int proc_offset )
{
    const decl_spec_t *rettype = type_function_get_ret( func->declspec.type );
    int has_ret = !is_void( rettype->type );
    const var_list_t *args = type_function_get_args( func->declspec.type );
    const var_t *arg;
    int len, needs_params = 0;

    /* we need a param structure if we have more than one arg */
    if (target.cpu == CPU_i386 && args) needs_params = is_object( iface ) || list_count( args ) > 1;

    print_file( file, 0, "{\n");
    if (needs_params)
    {
        if (has_ret) print_file( file, 1, "%s", "CLIENT_CALL_RETURN _RetVal;\n" );
        write_func_param_struct( file, iface, func->declspec.type, "__params", FALSE );
        if (is_object( iface )) print_file( file, 1, "__params.This = This;\n" );
        if (args)
            LIST_FOR_EACH_ENTRY( arg, args, const var_t, entry )
                print_file( file, 1, "__params.%s = %s;\n", arg->name, arg->name );
    }
    else if (has_ret) print_file( file, 1, "%s", "CLIENT_CALL_RETURN _RetVal;\n\n" );

    len = fprintf( file, "    %s%s( ",
                   has_ret ? "_RetVal = " : "",
                   interpreted_mode ? "NdrClientCall2" : "NdrClientCall" );
    fprintf( file, "&%s_StubDesc,", prefix );
    fprintf( file, "\n%*s&__MIDL_ProcFormatString.Format[%u]", len, "", proc_offset );
    if (needs_params)
    {
        fprintf( file, ",\n%*s&__params", len, "" );
    }
    else if (target.cpu != CPU_i386)
    {
        if (is_object( iface )) fprintf( file, ",\n%*sThis", len, "" );
        if (args)
            LIST_FOR_EACH_ENTRY( arg, args, const var_t, entry )
                fprintf( file, ",\n%*s%s", len, "", arg->name );
    }
    else
    {
        if (is_object( iface )) fprintf( file, ",\n%*s&This", len, "" );
        else if (args)
        {
            arg = LIST_ENTRY( list_head(args), const var_t, entry );
            fprintf( file, ",\n%*s&%s", len, "", arg->name );
        }
    }
    fprintf( file, " );\n" );
    if (has_ret)
    {
        print_file( file, 1, "return (" );
        write_type_decl_left(file, rettype);
        fprintf( file, ")%s;\n", target.cpu != CPU_i386 ? "_RetVal.Simple" : "*(LONG_PTR *)&_RetVal" );
    }
    print_file( file, 0, "}\n\n");
}

void write_exceptions( FILE *file )
{
    fprintf( file, "#ifndef USE_COMPILER_EXCEPTIONS\n");
    fprintf( file, "\n");
    fprintf( file, "#include \"wine/exception.h\"\n");
    fprintf( file, "#undef RpcTryExcept\n");
    fprintf( file, "#undef RpcExcept\n");
    fprintf( file, "#undef RpcEndExcept\n");
    fprintf( file, "#undef RpcTryFinally\n");
    fprintf( file, "#undef RpcFinally\n");
    fprintf( file, "#undef RpcEndFinally\n");
    fprintf( file, "#undef RpcExceptionCode\n");
    fprintf( file, "#undef RpcAbnormalTermination\n");
    fprintf( file, "\n");
    fprintf( file, "struct __exception_frame;\n");
    fprintf( file, "typedef int (*__filter_func)(struct __exception_frame *);\n");
    fprintf( file, "typedef void (*__finally_func)(struct __exception_frame *);\n");
    fprintf( file, "\n");
    fprintf( file, "#define __DECL_EXCEPTION_FRAME \\\n");
    fprintf( file, "    EXCEPTION_REGISTRATION_RECORD frame; \\\n");
    fprintf( file, "    __filter_func                 filter; \\\n");
    fprintf( file, "    __finally_func                finally; \\\n");
    fprintf( file, "    __wine_jmp_buf                jmp; \\\n");
    fprintf( file, "    DWORD                         code; \\\n");
    fprintf( file, "    unsigned char                 abnormal_termination; \\\n");
    fprintf( file, "    unsigned char                 filter_level; \\\n");
    fprintf( file, "    unsigned char                 finally_level;\n");
    fprintf( file, "\n");
    fprintf( file, "struct __exception_frame\n{\n");
    fprintf( file, "    __DECL_EXCEPTION_FRAME\n");
    fprintf( file, "};\n");
    fprintf( file, "\n");
    fprintf( file, "static inline void __widl_unwind_target(void)\n" );
    fprintf( file, "{\n");
    fprintf( file, "    struct __exception_frame *exc_frame = (struct __exception_frame *)__wine_get_frame();\n" );
    fprintf( file, "    if (exc_frame->finally_level > exc_frame->filter_level)\n" );
    fprintf( file, "    {\n");
    fprintf( file, "        exc_frame->abnormal_termination = 1;\n");
    fprintf( file, "        exc_frame->finally( exc_frame );\n");
    fprintf( file, "        __wine_pop_frame( &exc_frame->frame );\n");
    fprintf( file, "    }\n");
    fprintf( file, "    exc_frame->filter_level = 0;\n");
    fprintf( file, "    __wine_longjmp( &exc_frame->jmp, 1 );\n");
    fprintf( file, "}\n");
    fprintf( file, "\n");
    fprintf( file, "static DWORD __cdecl __widl_exception_handler( EXCEPTION_RECORD *record,\n");
    fprintf( file, "                                               EXCEPTION_REGISTRATION_RECORD *frame,\n");
    fprintf( file, "                                               CONTEXT *context,\n");
    fprintf( file, "                                               EXCEPTION_REGISTRATION_RECORD **pdispatcher )\n");
    fprintf( file, "{\n");
    fprintf( file, "    struct __exception_frame *exc_frame = (struct __exception_frame *)frame;\n");
    fprintf( file, "\n");
    fprintf( file, "    if (record->ExceptionFlags & (EXCEPTION_UNWINDING | EXCEPTION_EXIT_UNWIND | EXCEPTION_NESTED_CALL))\n");
    fprintf( file, "    {\n" );
    fprintf( file, "        if (exc_frame->finally_level && (record->ExceptionFlags & (EXCEPTION_UNWINDING | EXCEPTION_EXIT_UNWIND)))\n");
    fprintf( file, "        {\n" );
    fprintf( file, "            exc_frame->abnormal_termination = 1;\n");
    fprintf( file, "            exc_frame->finally( exc_frame );\n");
    fprintf( file, "        }\n" );
    fprintf( file, "        return ExceptionContinueSearch;\n");
    fprintf( file, "    }\n" );
    fprintf( file, "    exc_frame->code = record->ExceptionCode;\n");
    fprintf( file, "    if (exc_frame->filter_level && exc_frame->filter( exc_frame ) == EXCEPTION_EXECUTE_HANDLER)\n" );
    fprintf( file, "        __wine_rtl_unwind( frame, record, __widl_unwind_target );\n");
    fprintf( file, "    return ExceptionContinueSearch;\n");
    fprintf( file, "}\n");
    fprintf( file, "\n");
    fprintf( file, "#define RpcTryExcept \\\n");
    fprintf( file, "    if (!__wine_setjmpex( &__frame->jmp, &__frame->frame )) \\\n");
    fprintf( file, "    { \\\n");
    fprintf( file, "        if (!__frame->finally_level) \\\n" );
    fprintf( file, "            __wine_push_frame( &__frame->frame ); \\\n");
    fprintf( file, "        __frame->filter_level = __frame->finally_level + 1;\n" );
    fprintf( file, "\n");
    fprintf( file, "#define RpcExcept(expr) \\\n");
    fprintf( file, "        if (!__frame->finally_level) \\\n" );
    fprintf( file, "            __wine_pop_frame( &__frame->frame ); \\\n");
    fprintf( file, "        __frame->filter_level = 0; \\\n" );
    fprintf( file, "    } \\\n");
    fprintf( file, "    else \\\n");
    fprintf( file, "\n");
    fprintf( file, "#define RpcEndExcept\n");
    fprintf( file, "\n");
    fprintf( file, "#define RpcExceptionCode() (__frame->code)\n");
    fprintf( file, "\n");
    fprintf( file, "#define RpcTryFinally \\\n");
    fprintf( file, "    if (!__frame->filter_level) \\\n");
    fprintf( file, "        __wine_push_frame( &__frame->frame ); \\\n");
    fprintf( file, "    __frame->finally_level = __frame->filter_level + 1;\n");
    fprintf( file, "\n");
    fprintf( file, "#define RpcFinally \\\n");
    fprintf( file, "    if (!__frame->filter_level) \\\n");
    fprintf( file, "        __wine_pop_frame( &__frame->frame ); \\\n");
    fprintf( file, "    __frame->finally_level = 0;\n");
    fprintf( file, "\n");
    fprintf( file, "#define RpcEndFinally\n");
    fprintf( file, "\n");
    fprintf( file, "#define RpcAbnormalTermination() (__frame->abnormal_termination)\n");
    fprintf( file, "\n");
    fprintf( file, "#define RpcExceptionInit(filter_func,finally_func) \\\n");
    fprintf( file, "    do { \\\n");
    fprintf( file, "        __frame->frame.Handler = __widl_exception_handler; \\\n");
    fprintf( file, "        __frame->filter = (__filter_func)(filter_func); \\\n" );
    fprintf( file, "        __frame->finally = (__finally_func)(finally_func); \\\n");
    fprintf( file, "        __frame->abnormal_termination = 0; \\\n");
    fprintf( file, "        __frame->filter_level = 0; \\\n");
    fprintf( file, "        __frame->finally_level = 0; \\\n");
    fprintf( file, "    } while (0)\n");
    fprintf( file, "\n");
    fprintf( file, "#else /* USE_COMPILER_EXCEPTIONS */\n");
    fprintf( file, "\n");
    fprintf( file, "#define RpcExceptionInit(filter_func,finally_func) \\\n");
    fprintf( file, "    do { (void)(filter_func); } while(0)\n");
    fprintf( file, "\n");
    fprintf( file, "#define __DECL_EXCEPTION_FRAME \\\n");
    fprintf( file, "    DWORD code;\n");
    fprintf( file, "\n");
    fprintf( file, "#endif /* USE_COMPILER_EXCEPTIONS */\n");
}
