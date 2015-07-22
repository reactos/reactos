/*
 * Copyright 2012 Hans Leidekker for CodeWeavers
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

#include "wbemprox_private.h"

HRESULT create_view( const struct property *proplist, const WCHAR *class,
                     const struct expr *cond, struct view **ret )
{
    struct view *view = heap_alloc( sizeof(struct view) );

    if (!view) return E_OUTOFMEMORY;
    view->proplist = proplist;
    view->table    = grab_table( class );
    view->cond     = cond;
    view->result   = NULL;
    view->count    = 0;
    *ret = view;
    return S_OK;
}

void destroy_view( struct view *view )
{
    if (!view) return;
    if (view->table) release_table( view->table );
    heap_free( view->result );
    heap_free( view );
}

static BOOL eval_like( const WCHAR *lstr, const WCHAR *rstr )
{
    const WCHAR *p = lstr, *q = rstr;

    while (*p && *q)
    {
        if (*q == '%')
        {
            while (*q == '%') q++;
            if (!*q) return TRUE;
            while (*p && toupperW( p[1] ) != toupperW( q[1] )) p++;
            if (!*p) return TRUE;
        }
        if (toupperW( *p++ ) != toupperW( *q++ )) return FALSE;
    }
    return TRUE;
}

static HRESULT eval_strcmp( UINT op, const WCHAR *lstr, const WCHAR *rstr, LONGLONG *val )
{
    if (!lstr || !rstr)
    {
        *val = 0;
        return S_OK;
    }
    switch (op)
    {
    case OP_EQ:
        *val = !strcmpW( lstr, rstr );
        break;
    case OP_GT:
        *val = strcmpW( lstr, rstr ) > 0;
        break;
    case OP_LT:
        *val = strcmpW( lstr, rstr ) < 0;
        break;
    case OP_LE:
        *val = strcmpW( lstr, rstr ) <= 0;
        break;
    case OP_GE:
        *val = strcmpW( lstr, rstr ) >= 0;
        break;
    case OP_NE:
        *val = strcmpW( lstr, rstr );
        break;
    case OP_LIKE:
        *val = eval_like( lstr, rstr );
        break;
    default:
        ERR("unhandled operator %u\n", op);
        return WBEM_E_INVALID_QUERY;
    }
    return S_OK;
}

static inline BOOL is_strcmp( const struct complex_expr *expr )
{
    return ((expr->left->type == EXPR_PROPVAL && expr->right->type == EXPR_SVAL) ||
            (expr->left->type == EXPR_SVAL && expr->right->type == EXPR_PROPVAL));
}

static inline BOOL is_boolcmp( const struct complex_expr *expr, UINT ltype, UINT rtype )
{
    if (ltype == CIM_BOOLEAN && expr->left->type == EXPR_PROPVAL &&
        (expr->right->type == EXPR_SVAL || expr->right->type == EXPR_BVAL)) return TRUE;
    else if (rtype == CIM_BOOLEAN && expr->right->type == EXPR_PROPVAL &&
             (expr->left->type == EXPR_SVAL || expr->left->type == EXPR_BVAL)) return TRUE;
    return FALSE;
}

static HRESULT eval_boolcmp( UINT op, LONGLONG lval, LONGLONG rval, UINT ltype, UINT rtype, LONGLONG *val )
{
    static const WCHAR trueW[] = {'T','r','u','e',0};

    if (ltype == CIM_STRING) lval = !strcmpiW( (const WCHAR *)(INT_PTR)lval, trueW ) ? -1 : 0;
    else if (rtype == CIM_STRING) rval = !strcmpiW( (const WCHAR *)(INT_PTR)rval, trueW ) ? -1 : 0;

    switch (op)
    {
    case OP_EQ:
        *val = (lval == rval);
        break;
    case OP_NE:
        *val = (lval != rval);
        break;
    default:
        ERR("unhandled operator %u\n", op);
        return WBEM_E_INVALID_QUERY;
    }
    return S_OK;
}

static UINT resolve_type( UINT left, UINT right )
{
    switch (left)
    {
    case CIM_SINT8:
    case CIM_SINT16:
    case CIM_SINT32:
    case CIM_SINT64:
    case CIM_UINT8:
    case CIM_UINT16:
    case CIM_UINT32:
    case CIM_UINT64:
        switch (right)
        {
            case CIM_SINT8:
            case CIM_SINT16:
            case CIM_SINT32:
            case CIM_SINT64:
            case CIM_UINT8:
            case CIM_UINT16:
            case CIM_UINT32:
            case CIM_UINT64:
                return CIM_UINT64;
            default: break;
        }
        break;

    case CIM_STRING:
        if (right == CIM_STRING) return CIM_STRING;
        break;

    case CIM_BOOLEAN:
        if (right == CIM_BOOLEAN) return CIM_BOOLEAN;
        break;

    default:
        break;
    }
    return CIM_ILLEGAL;
}

static HRESULT eval_binary( const struct table *table, UINT row, const struct complex_expr *expr,
                            LONGLONG *val, UINT *type )
{
    HRESULT lret, rret;
    LONGLONG lval, rval;
    UINT ltype, rtype;

    lret = eval_cond( table, row, expr->left, &lval, &ltype );
    rret = eval_cond( table, row, expr->right, &rval, &rtype );
    if (lret != S_OK || rret != S_OK) return WBEM_E_INVALID_QUERY;

    *type = resolve_type( ltype, rtype );

    if (is_boolcmp( expr, ltype, rtype ))
        return eval_boolcmp( expr->op, lval, rval, ltype, rtype, val );

    if (is_strcmp( expr ))
    {
        const WCHAR *lstr = (const WCHAR *)(INT_PTR)lval;
        const WCHAR *rstr = (const WCHAR *)(INT_PTR)rval;

        return eval_strcmp( expr->op, lstr, rstr, val );
    }
    switch (expr->op)
    {
    case OP_EQ:
        *val = (lval == rval);
        break;
    case OP_AND:
        *val = (lval && rval);
        break;
    case OP_OR:
        *val = (lval || rval);
        break;
    case OP_GT:
        *val = (lval > rval);
        break;
    case OP_LT:
        *val = (lval < rval);
        break;
    case OP_LE:
        *val = (lval <= rval);
        break;
    case OP_GE:
        *val = (lval >= rval);
        break;
    case OP_NE:
        *val = (lval != rval);
        break;
    default:
        ERR("unhandled operator %u\n", expr->op);
        return WBEM_E_INVALID_QUERY;
    }
    return S_OK;
}

static HRESULT eval_unary( const struct table *table, UINT row, const struct complex_expr *expr,
                           LONGLONG *val, UINT *type )

{
    HRESULT hr;
    UINT column;
    LONGLONG lval;

    if (expr->op == OP_NOT)
    {
        hr = eval_cond( table, row, expr->left, &lval, type );
        if (hr != S_OK)
            return hr;
        *val = !lval;
        return S_OK;
    }

    hr = get_column_index( table, expr->left->u.propval->name, &column );
    if (hr != S_OK)
        return hr;

    hr = get_value( table, row, column, &lval );
    if (hr != S_OK)
        return hr;

    switch (expr->op)
    {
    case OP_ISNULL:
        *val = !lval;
        break;
    case OP_NOTNULL:
        *val = lval;
        break;
    default:
        ERR("unknown operator %u\n", expr->op);
        return WBEM_E_INVALID_QUERY;
    }

    *type = table->columns[column].type & CIM_TYPE_MASK;
    return S_OK;
}

static HRESULT eval_propval( const struct table *table, UINT row, const struct property *propval,
                             LONGLONG *val, UINT *type )

{
    HRESULT hr;
    UINT column;

    hr = get_column_index( table, propval->name, &column );
    if (hr != S_OK)
        return hr;

    *type = table->columns[column].type & CIM_TYPE_MASK;
    return get_value( table, row, column, val );
}

HRESULT eval_cond( const struct table *table, UINT row, const struct expr *cond, LONGLONG *val, UINT *type )
{
    if (!cond)
    {
        *val = 1;
        *type = CIM_UINT64;
        return S_OK;
    }
    switch (cond->type)
    {
    case EXPR_COMPLEX:
        return eval_binary( table, row, &cond->u.expr, val, type );

    case EXPR_UNARY:
        return eval_unary( table, row, &cond->u.expr, val, type );

    case EXPR_PROPVAL:
        return eval_propval( table, row, cond->u.propval, val, type );

    case EXPR_SVAL:
        *val = (INT_PTR)cond->u.sval;
        *type = CIM_STRING;
        return S_OK;

    case EXPR_IVAL:
        *val = cond->u.ival;
        *type = CIM_UINT64;
        return S_OK;

    case EXPR_BVAL:
        *val = cond->u.ival;
        *type = CIM_BOOLEAN;
        return S_OK;

    default:
        ERR("invalid expression type\n");
        break;
    }
    return WBEM_E_INVALID_QUERY;
}

HRESULT execute_view( struct view *view )
{
    UINT i, j = 0, len;

    if (!view->table) return S_OK;
    if (view->table->fill)
    {
        clear_table( view->table );
        view->table->fill( view->table, view->cond );
    }
    if (!view->table->num_rows) return S_OK;

    len = min( view->table->num_rows, 16 );
    if (!(view->result = heap_alloc( len * sizeof(UINT) ))) return E_OUTOFMEMORY;

    for (i = 0; i < view->table->num_rows; i++)
    {
        HRESULT hr;
        LONGLONG val = 0;
        UINT type;

        if (j >= len)
        {
            UINT *tmp;
            len *= 2;
            if (!(tmp = heap_realloc( view->result, len * sizeof(UINT) ))) return E_OUTOFMEMORY;
            view->result = tmp;
        }
        if ((hr = eval_cond( view->table, i, view->cond, &val, &type )) != S_OK) return hr;
        if (val) view->result[j++] = i;
    }
    view->count = j;
    return S_OK;
}

struct query *create_query(void)
{
    struct query *query;

    if (!(query = heap_alloc( sizeof(*query) ))) return NULL;
    list_init( &query->mem );
    query->refs = 1;
    return query;
}

void free_query( struct query *query )
{
    struct list *mem, *next;

    if (!query) return;
    destroy_view( query->view );
    LIST_FOR_EACH_SAFE( mem, next, &query->mem ) { heap_free( mem ); }
    heap_free( query );
}

struct query *addref_query( struct query *query )
{
    InterlockedIncrement( &query->refs );
    return query;
}

void release_query( struct query *query )
{
    if (!InterlockedDecrement( &query->refs )) free_query( query );
}

HRESULT exec_query( const WCHAR *str, IEnumWbemClassObject **result )
{
    HRESULT hr;
    struct query *query;

    *result = NULL;
    if (!(query = create_query())) return E_OUTOFMEMORY;
    hr = parse_query( str, &query->view, &query->mem );
    if (hr != S_OK) goto done;
    hr = execute_view( query->view );
    if (hr != S_OK) goto done;
    hr = EnumWbemClassObject_create( query, (void **)result );

done:
    release_query( query );
    return hr;
}

BOOL is_selected_prop( const struct view *view, const WCHAR *name )
{
    const struct property *prop = view->proplist;

    if (!prop) return TRUE;
    while (prop)
    {
        if (!strcmpiW( prop->name, name )) return TRUE;
        prop = prop->next;
    }
    return FALSE;
}

static BOOL is_system_prop( const WCHAR *name )
{
    return (name[0] == '_' && name[1] == '_');
}

static BSTR build_servername( const struct view *view )
{
    WCHAR server[MAX_COMPUTERNAME_LENGTH + 1], *p;
    DWORD len = sizeof(server)/sizeof(server[0]);

    if (view->proplist) return NULL;

    if (!(GetComputerNameW( server, &len ))) return NULL;
    for (p = server; *p; p++) *p = toupperW( *p );
    return SysAllocString( server );
}

static BSTR build_classname( const struct view *view )
{
    return SysAllocString( view->table->name );
}

static BSTR build_namespace( const struct view *view )
{
    static const WCHAR cimv2W[] = {'R','O','O','T','\\','C','I','M','V','2',0};

    if (view->proplist) return NULL;
    return SysAllocString( cimv2W );
}

static BSTR build_proplist( const struct view *view, UINT index, UINT count, UINT *len )
{
    static const WCHAR fmtW[] = {'%','s','=','%','s',0};
    UINT i, j, offset, row = view->result[index];
    BSTR *values, ret = NULL;

    if (!(values = heap_alloc( count * sizeof(BSTR) ))) return NULL;

    *len = j = 0;
    for (i = 0; i < view->table->num_cols; i++)
    {
        if (view->table->columns[i].type & COL_FLAG_KEY)
        {
            const WCHAR *name = view->table->columns[i].name;

            values[j] = get_value_bstr( view->table, row, i );
            *len += strlenW( fmtW ) + strlenW( name ) + strlenW( values[j] );
            j++;
        }
    }
    if ((ret = SysAllocStringLen( NULL, *len )))
    {
        offset = j = 0;
        for (i = 0; i < view->table->num_cols; i++)
        {
            if (view->table->columns[i].type & COL_FLAG_KEY)
            {
                const WCHAR *name = view->table->columns[i].name;

                offset += sprintfW( ret + offset, fmtW, name, values[j] );
                if (j < count - 1) ret[offset++] = ',';
                j++;
            }
        }
    }
    for (i = 0; i < count; i++) SysFreeString( values[i] );
    heap_free( values );
    return ret;
}

static UINT count_key_columns( const struct view *view )
{
    UINT i, num_keys = 0;

    for (i = 0; i < view->table->num_cols; i++)
    {
        if (view->table->columns[i].type & COL_FLAG_KEY) num_keys++;
    }
    return num_keys;
}

static BSTR build_relpath( const struct view *view, UINT index, const WCHAR *name )
{
    static const WCHAR fmtW[] = {'%','s','.','%','s',0};
    BSTR class, proplist, ret = NULL;
    UINT num_keys, len;

    if (view->proplist) return NULL;

    if (!(class = build_classname( view ))) return NULL;
    if (!(num_keys = count_key_columns( view ))) return class;
    if (!(proplist = build_proplist( view, index, num_keys, &len ))) goto done;

    len += strlenW( fmtW ) + SysStringLen( class );
    if (!(ret = SysAllocStringLen( NULL, len ))) goto done;
    sprintfW( ret, fmtW, class, proplist );

done:
    SysFreeString( class );
    SysFreeString( proplist );
    return ret;
}

static BSTR build_path( const struct view *view, UINT index, const WCHAR *name )
{
    static const WCHAR fmtW[] = {'\\','\\','%','s','\\','%','s',':','%','s',0};
    BSTR server, namespace = NULL, relpath = NULL, ret = NULL;
    UINT len;

    if (view->proplist) return NULL;

    if (!(server = build_servername( view ))) return NULL;
    if (!(namespace = build_namespace( view ))) goto done;
    if (!(relpath = build_relpath( view, index, name ))) goto done;

    len = strlenW( fmtW ) + SysStringLen( server ) + SysStringLen( namespace ) + SysStringLen( relpath );
    if (!(ret = SysAllocStringLen( NULL, len ))) goto done;
    sprintfW( ret, fmtW, server, namespace, relpath );

done:
    SysFreeString( server );
    SysFreeString( namespace );
    SysFreeString( relpath );
    return ret;
}

BOOL is_method( const struct table *table, UINT column )
{
    return table->columns[column].type & COL_FLAG_METHOD;
}

static UINT count_properties( const struct view *view )
{
    UINT i, num_props = 0;

    for (i = 0; i < view->table->num_cols; i++)
    {
        if (!is_method( view->table, i)) num_props++;
    }
    return num_props;
}

static UINT count_selected_properties( const struct view *view )
{
    const struct property *prop = view->proplist;
    UINT count;

    if (!prop) return count_properties( view );

    count = 1;
    while ((prop = prop->next)) count++;
    return count;
}

static HRESULT get_system_propval( const struct view *view, UINT index, const WCHAR *name,
                                   VARIANT *ret, CIMTYPE *type, LONG *flavor )
{
    static const WCHAR classW[] = {'_','_','C','L','A','S','S',0};
    static const WCHAR genusW[] = {'_','_','G','E','N','U','S',0};
    static const WCHAR pathW[] = {'_','_','P','A','T','H',0};
    static const WCHAR namespaceW[] = {'_','_','N','A','M','E','S','P','A','C','E',0};
    static const WCHAR propcountW[] = {'_','_','P','R','O','P','E','R','T','Y','_','C','O','U','N','T',0};
    static const WCHAR relpathW[] = {'_','_','R','E','L','P','A','T','H',0};
    static const WCHAR serverW[] = {'_','_','S','E','R','V','E','R',0};

    if (flavor) *flavor = WBEM_FLAVOR_ORIGIN_SYSTEM;

    if (!strcmpiW( name, classW ))
    {
        V_VT( ret ) = VT_BSTR;
        V_BSTR( ret ) = build_classname( view );
        if (type) *type = CIM_STRING;
        return S_OK;
    }
    if (!strcmpiW( name, genusW ))
    {
        V_VT( ret ) = VT_I4;
        V_I4( ret ) = WBEM_GENUS_INSTANCE; /* FIXME */
        if (type) *type = CIM_SINT32;
        return S_OK;
    }
    else if (!strcmpiW( name, namespaceW ))
    {
        V_VT( ret ) = VT_BSTR;
        V_BSTR( ret ) = build_namespace( view );
        if (type) *type = CIM_STRING;
        return S_OK;
    }
    else if (!strcmpiW( name, pathW ))
    {
        V_VT( ret ) = VT_BSTR;
        V_BSTR( ret ) = build_path( view, index, name );
        if (type) *type = CIM_STRING;
        return S_OK;
    }
    if (!strcmpiW( name, propcountW ))
    {
        V_VT( ret ) = VT_I4;
        V_I4( ret ) = count_selected_properties( view );
        if (type) *type = CIM_SINT32;
        return S_OK;
    }
    else if (!strcmpiW( name, relpathW ))
    {
        V_VT( ret ) = VT_BSTR;
        V_BSTR( ret ) = build_relpath( view, index, name );
        if (type) *type = CIM_STRING;
        return S_OK;
    }
    else if (!strcmpiW( name, serverW ))
    {
        V_VT( ret ) = VT_BSTR;
        V_BSTR( ret ) = build_servername( view );
        if (type) *type = CIM_STRING;
        return S_OK;
    }
    FIXME("system property %s not implemented\n", debugstr_w(name));
    return WBEM_E_NOT_FOUND;
}

VARTYPE to_vartype( CIMTYPE type )
{
    switch (type)
    {
    case CIM_BOOLEAN:  return VT_BOOL;
    case CIM_STRING:
    case CIM_DATETIME: return VT_BSTR;
    case CIM_SINT8:    return VT_I1;
    case CIM_UINT8:    return VT_UI1;
    case CIM_SINT16:   return VT_I2;
    case CIM_UINT16:   return VT_UI2;
    case CIM_SINT32:   return VT_I4;
    case CIM_UINT32:   return VT_UI4;
    case CIM_SINT64:   return VT_I8;
    case CIM_UINT64:   return VT_UI8;
    default:
        ERR("unhandled type %u\n", type);
        break;
    }
    return 0;
}

SAFEARRAY *to_safearray( const struct array *array, CIMTYPE type )
{
    SAFEARRAY *ret;
    UINT size = get_type_size( type );
    VARTYPE vartype = to_vartype( type );
    LONG i;

    if (!array || !(ret = SafeArrayCreateVector( vartype, 0, array->count ))) return NULL;

    for (i = 0; i < array->count; i++)
    {
        void *ptr = (char *)array->ptr + i * size;
        if (vartype == VT_BSTR)
        {
            BSTR str = SysAllocString( *(const WCHAR **)ptr );
            if (!str || SafeArrayPutElement( ret, &i, str ) != S_OK)
            {
                SysFreeString( str );
                SafeArrayDestroy( ret );
                return NULL;
            }
            SysFreeString( str );
        }
        else if (SafeArrayPutElement( ret, &i, ptr ) != S_OK)
        {
            SafeArrayDestroy( ret );
            return NULL;
        }
    }
    return ret;
}

void set_variant( VARTYPE type, LONGLONG val, void *val_ptr, VARIANT *ret )
{
    if (type & VT_ARRAY)
    {
        V_VT( ret ) = type;
        V_ARRAY( ret ) = val_ptr;
        return;
    }
    switch (type)
    {
    case VT_BOOL:
        V_BOOL( ret ) = val;
        break;
    case VT_BSTR:
        V_BSTR( ret ) = val_ptr;
        break;
    case VT_I1:
        V_I1( ret ) = val;
        break;
    case VT_UI1:
        V_UI1( ret ) = val;
        break;
    case VT_I2:
        V_I2( ret ) = val;
        break;
    case VT_UI2:
        V_UI2( ret ) = val;
        break;
    case VT_I4:
        V_I4( ret ) = val;
        break;
    case VT_UI4:
        V_UI4( ret ) = val;
        break;
    case VT_NULL:
        break;
    default:
        ERR("unhandled variant type %u\n", type);
        return;
    }
    V_VT( ret ) = type;
}

HRESULT get_propval( const struct view *view, UINT index, const WCHAR *name, VARIANT *ret,
                     CIMTYPE *type, LONG *flavor )
{
    HRESULT hr;
    UINT column, row;
    VARTYPE vartype;
    void *val_ptr = NULL;
    LONGLONG val;

    if (is_system_prop( name )) return get_system_propval( view, index, name, ret, type, flavor );
    if (!view->count || !is_selected_prop( view, name )) return WBEM_E_NOT_FOUND;

    hr = get_column_index( view->table, name, &column );
    if (hr != S_OK || is_method( view->table, column )) return WBEM_E_NOT_FOUND;

    row = view->result[index];
    hr = get_value( view->table, row, column, &val );
    if (hr != S_OK) return hr;

    vartype = view->table->columns[column].vartype;
    if (view->table->columns[column].type & CIM_FLAG_ARRAY)
    {
        CIMTYPE basetype = view->table->columns[column].type & CIM_TYPE_MASK;

        val_ptr = to_safearray( (const struct array *)(INT_PTR)val, basetype );
        if (!vartype) vartype = to_vartype( basetype ) | VT_ARRAY;
        goto done;
    }
    switch (view->table->columns[column].type & COL_TYPE_MASK)
    {
    case CIM_BOOLEAN:
        if (!vartype) vartype = VT_BOOL;
        break;
    case CIM_STRING:
    case CIM_DATETIME:
        if (val)
        {
            vartype = VT_BSTR;
            val_ptr = SysAllocString( (const WCHAR *)(INT_PTR)val );
        }
        else
            vartype = VT_NULL;
        break;
    case CIM_SINT8:
        if (!vartype) vartype = VT_I1;
        break;
    case CIM_UINT8:
        if (!vartype) vartype = VT_UI1;
        break;
    case CIM_SINT16:
        if (!vartype) vartype = VT_I2;
        break;
    case CIM_UINT16:
        if (!vartype) vartype = VT_UI2;
        break;
    case CIM_SINT32:
        if (!vartype) vartype = VT_I4;
        break;
    case CIM_UINT32:
        if (!vartype) vartype = VT_UI4;
        break;
    case CIM_SINT64:
        vartype = VT_BSTR;
        val_ptr = get_value_bstr( view->table, row, column );
        break;
    case CIM_UINT64:
        vartype = VT_BSTR;
        val_ptr = get_value_bstr( view->table, row, column );
        break;
    default:
        ERR("unhandled column type %u\n", view->table->columns[column].type);
        return WBEM_E_FAILED;
    }

done:
    set_variant( vartype, val, val_ptr, ret );
    if (type) *type = view->table->columns[column].type & COL_TYPE_MASK;
    if (flavor) *flavor = 0;
    return S_OK;
}

static CIMTYPE to_cimtype( VARTYPE type )
{
    switch (type)
    {
    case VT_BOOL:  return CIM_BOOLEAN;
    case VT_BSTR:  return CIM_STRING;
    case VT_I1:    return CIM_SINT8;
    case VT_UI1:   return CIM_UINT8;
    case VT_I2:    return CIM_SINT16;
    case VT_UI2:   return CIM_UINT16;
    case VT_I4:    return CIM_SINT32;
    case VT_UI4:   return CIM_UINT32;
    case VT_I8:    return CIM_SINT64;
    case VT_UI8:   return CIM_UINT64;
    default:
        ERR("unhandled type %u\n", type);
        break;
    }
    return 0;
}

static struct array *to_array( VARIANT *var, CIMTYPE *type )
{
    struct array *ret;
    LONG bound, i;
    VARTYPE vartype;
    CIMTYPE basetype;
    UINT size;

    if (SafeArrayGetVartype( V_ARRAY( var ), &vartype ) != S_OK) return NULL;
    if (!(basetype = to_cimtype( vartype ))) return NULL;
    if (SafeArrayGetUBound( V_ARRAY( var ), 1, &bound ) != S_OK) return NULL;
    if (!(ret = heap_alloc( sizeof(struct array) ))) return NULL;

    ret->count = bound + 1;
    size = get_type_size( basetype );
    if (!(ret->ptr = heap_alloc_zero( ret->count * size )))
    {
        heap_free( ret );
        return NULL;
    }
    for (i = 0; i < ret->count; i++)
    {
        void *ptr = (char *)ret->ptr + i * size;
        if (vartype == VT_BSTR)
        {
            BSTR str;
            if (SafeArrayGetElement( V_ARRAY( var ), &i, &str ) != S_OK)
            {
                destroy_array( ret, basetype );
                return NULL;
            }
            *(WCHAR **)ptr = heap_strdupW( str );
            SysFreeString( str );
            if (!*(WCHAR **)ptr)
            {
                destroy_array( ret, basetype );
                return NULL;
            }
        }
        else if (SafeArrayGetElement( V_ARRAY( var ), &i, ptr ) != S_OK)
        {
            destroy_array( ret, basetype );
            return NULL;
        }
    }
    *type = basetype | CIM_FLAG_ARRAY;
    return ret;
}

HRESULT to_longlong( VARIANT *var, LONGLONG *val, CIMTYPE *type )
{
    if (!var)
    {
        *val = 0;
        return S_OK;
    }
    if (V_VT( var ) & VT_ARRAY)
    {
        *val = (INT_PTR)to_array( var, type );
        if (!*val) return E_OUTOFMEMORY;
        return S_OK;
    }
    switch (V_VT( var ))
    {
    case VT_BOOL:
        *val = V_BOOL( var );
        *type = CIM_BOOLEAN;
        break;
    case VT_BSTR:
        *val = (INT_PTR)heap_strdupW( V_BSTR( var ) );
        if (!*val) return E_OUTOFMEMORY;
        *type = CIM_STRING;
        break;
    case VT_I2:
        *val = V_I2( var );
        *type = CIM_SINT16;
        break;
    case VT_UI2:
        *val = V_UI2( var );
        *type = CIM_UINT16;
        break;
    case VT_I4:
        *val = V_I4( var );
        *type = CIM_SINT32;
        break;
    case VT_UI4:
        *val = V_UI4( var );
        *type = CIM_UINT32;
        break;
    case VT_NULL:
        *val = 0;
        break;
    default:
        ERR("unhandled type %u\n", V_VT( var ));
        return WBEM_E_FAILED;
    }
    return S_OK;
}

HRESULT put_propval( const struct view *view, UINT index, const WCHAR *name, VARIANT *var, CIMTYPE type )
{
    HRESULT hr;
    UINT column, row = view->result[index];
    LONGLONG val;

    hr = get_column_index( view->table, name, &column );
    if (hr != S_OK)
    {
        FIXME("no support for creating new properties\n");
        return WBEM_E_FAILED;
    }
    if (is_method( view->table, column ) || !(view->table->columns[column].type & COL_FLAG_DYNAMIC))
        return WBEM_E_FAILED;

    hr = to_longlong( var, &val, &type );
    if (hr != S_OK) return hr;

    return set_value( view->table, row, column, val, type );
}

HRESULT get_properties( const struct view *view, LONG flags, SAFEARRAY **props )
{
    SAFEARRAY *sa;
    BSTR str;
    UINT i, num_props = count_selected_properties( view );
    LONG j;

    if (!(sa = SafeArrayCreateVector( VT_BSTR, 0, num_props ))) return E_OUTOFMEMORY;

    for (i = 0, j = 0; i < view->table->num_cols; i++)
    {
        BOOL is_system;

        if (is_method( view->table, i )) continue;
        if (!is_selected_prop( view, view->table->columns[i].name )) continue;

        is_system = is_system_prop( view->table->columns[i].name );
        if ((flags & WBEM_FLAG_NONSYSTEM_ONLY) && is_system) continue;
        else if ((flags & WBEM_FLAG_SYSTEM_ONLY) && !is_system) continue;

        str = SysAllocString( view->table->columns[i].name );
        if (!str || SafeArrayPutElement( sa, &j, str ) != S_OK)
        {
            SysFreeString( str );
            SafeArrayDestroy( sa );
            return E_OUTOFMEMORY;
        }
        SysFreeString( str );
        j++;
    }
    *props = sa;
    return S_OK;
}
