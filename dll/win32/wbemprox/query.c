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

#define COBJMACROS

#include <stdarg.h>
#ifdef __REACTOS__
#include <wchar.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "wbemcli.h"

#include "wine/debug.h"
#include "wbemprox_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(wbemprox);

static HRESULT append_table( struct view *view, struct table *table )
{
    struct table **tmp;
    if (!(tmp = heap_realloc( view->table, (view->table_count + 1) * sizeof(*tmp) ))) return E_OUTOFMEMORY;
    view->table = tmp;
    view->table[view->table_count++] = table;
    return S_OK;
}

HRESULT create_view( enum view_type type, const WCHAR *path, const struct keyword *keywordlist, const WCHAR *class,
                     const struct property *proplist, const struct expr *cond, struct view **ret )
{
    struct view *view = heap_alloc_zero( sizeof(*view) );

    if (!view) return E_OUTOFMEMORY;

    switch (type)
    {
    case VIEW_TYPE_ASSOCIATORS:
        view->path        = path;
        view->keywordlist = keywordlist;
        break;

    case VIEW_TYPE_SELECT:
    {
        struct table *table = grab_table( class );
        HRESULT hr;

        if (table && (hr = append_table( view, table )) != S_OK)
        {
            heap_free( view );
            return hr;
        }
        view->proplist = proplist;
        view->cond     = cond;
        break;
    }
    default:
        ERR( "unhandled type %u\n", type );
        heap_free( view );
        return E_INVALIDARG;
    }

    view->type = type;
    *ret = view;
    return S_OK;
}

void destroy_view( struct view *view )
{
    ULONG i;
    if (!view) return;
    for (i = 0; i < view->table_count; i++) release_table( view->table[i] );
    heap_free( view->table );
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
            while (*p && *q && towupper( *p ) == towupper( *q )) { p++; q++; };
            if (!*p && !*q) return TRUE;
        }
        if (*q != '%' && towupper( *p++ ) != towupper( *q++ )) return FALSE;
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
        *val = !wcscmp( lstr, rstr );
        break;
    case OP_GT:
        *val = wcscmp( lstr, rstr ) > 0;
        break;
    case OP_LT:
        *val = wcscmp( lstr, rstr ) < 0;
        break;
    case OP_LE:
        *val = wcscmp( lstr, rstr ) <= 0;
        break;
    case OP_GE:
        *val = wcscmp( lstr, rstr ) >= 0;
        break;
    case OP_NE:
        *val = wcscmp( lstr, rstr );
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

static BOOL is_int( CIMTYPE type )
{
    switch (type)
    {
    case CIM_SINT8:
    case CIM_SINT16:
    case CIM_SINT32:
    case CIM_SINT64:
    case CIM_UINT8:
    case CIM_UINT16:
    case CIM_UINT32:
    case CIM_UINT64:
        return TRUE;
    default:
        return FALSE;
    }
}

static inline BOOL is_strcmp( const struct complex_expr *expr, UINT ltype, UINT rtype )
{
    if ((ltype == CIM_STRING || is_int( ltype )) && expr->left->type == EXPR_PROPVAL &&
        expr->right->type == EXPR_SVAL) return TRUE;
    else if ((rtype == CIM_STRING || is_int( rtype )) && expr->right->type == EXPR_PROPVAL &&
             expr->left->type == EXPR_SVAL) return TRUE;
    return FALSE;
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

    if (ltype == CIM_STRING) lval = !wcsicmp( (const WCHAR *)(INT_PTR)lval, trueW ) ? -1 : 0;
    else if (rtype == CIM_STRING) rval = !wcsicmp( (const WCHAR *)(INT_PTR)rval, trueW ) ? -1 : 0;

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

static inline BOOL is_refcmp( const struct complex_expr *expr, UINT ltype, UINT rtype )
{
    if (ltype == CIM_REFERENCE && expr->left->type == EXPR_PROPVAL && expr->right->type == EXPR_SVAL) return TRUE;
    else if (rtype == CIM_REFERENCE && expr->right->type == EXPR_PROPVAL && expr->left->type == EXPR_SVAL) return TRUE;
    return FALSE;
}

static HRESULT eval_refcmp( UINT op, const WCHAR *lstr, const WCHAR *rstr, LONGLONG *val )
{
    if (!lstr || !rstr)
    {
        *val = 0;
        return S_OK;
    }
    switch (op)
    {
    case OP_EQ:
        *val = !wcsicmp( lstr, rstr );
        break;
    case OP_NE:
        *val = wcsicmp( lstr, rstr );
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

    case CIM_REFERENCE:
        if (right == CIM_REFERENCE) return CIM_REFERENCE;
        break;

    default:
        break;
    }
    return CIM_ILLEGAL;
}

static const WCHAR *format_int( WCHAR *buf, UINT len, CIMTYPE type, LONGLONG val )
{
    static const WCHAR fmt_signedW[] = {'%','d',0};
    static const WCHAR fmt_unsignedW[] = {'%','u',0};
    static const WCHAR fmt_signed64W[] = {'%','I','6','4','d',0};
    static const WCHAR fmt_unsigned64W[] = {'%','I','6','4','u',0};

    switch (type)
    {
    case CIM_SINT8:
    case CIM_SINT16:
    case CIM_SINT32:
        swprintf( buf, fmt_signedW, val );
        return buf;

    case CIM_UINT8:
    case CIM_UINT16:
    case CIM_UINT32:
        swprintf( buf, fmt_unsignedW, val );
        return buf;

    case CIM_SINT64:
        wsprintfW( buf, fmt_signed64W, val );
        return buf;

    case CIM_UINT64:
        wsprintfW( buf, fmt_unsigned64W, val );
        return buf;

    default:
        ERR( "unhandled type %u\n", type );
        return NULL;
    }
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

    if (is_strcmp( expr, ltype, rtype ))
    {
        const WCHAR *lstr, *rstr;
        WCHAR lbuf[21], rbuf[21];

        if (is_int( ltype )) lstr = format_int( lbuf, ARRAY_SIZE( lbuf ), ltype, lval );
        else lstr = (const WCHAR *)(INT_PTR)lval;

        if (is_int( rtype )) rstr = format_int( rbuf, ARRAY_SIZE( rbuf ), rtype, rval );
        else rstr = (const WCHAR *)(INT_PTR)rval;

        return eval_strcmp( expr->op, lstr, rstr, val );
    }
    if (is_boolcmp( expr, ltype, rtype ))
    {
        return eval_boolcmp( expr->op, lval, rval, ltype, rtype, val );
    }
    if (is_refcmp( expr, ltype, rtype ))
    {
        return eval_refcmp( expr->op, (const WCHAR *)(INT_PTR)lval, (const WCHAR *)(INT_PTR)rval, val );
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

static WCHAR *build_assoc_query( const WCHAR *class, UINT class_len )
{
    static const WCHAR fmtW[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ','_','_','A','S','S','O','C','I','A','T','O','R','S',
         ' ','W','H','E','R','E',' ','C','l','a','s','s','=','\'','%','s','\'',0};
    UINT len = class_len + ARRAY_SIZE(fmtW);
    WCHAR *ret;

    if (!(ret = heap_alloc( len * sizeof(WCHAR) ))) return NULL;
    swprintf( ret, fmtW, class );
    return ret;
}

static HRESULT create_assoc_enum( const WCHAR *class, UINT class_len, IEnumWbemClassObject **iter )
{
    WCHAR *query;
    HRESULT hr;

    if (!(query = build_assoc_query( class, class_len ))) return E_OUTOFMEMORY;
    hr = exec_query( query, iter );
    heap_free( query );
    return hr;
}

static WCHAR *build_antecedent_query( const WCHAR *assocclass, const WCHAR *dependent )
{
    static const WCHAR fmtW[] =
        {'S','E','L','E','C','T',' ','A','n','t','e','c','e','d','e','n','t',' ','F','R','O','M',' ','%','s',' ',
         'W','H','E','R','E',' ','D','e','p','e','n','d','e','n','t','=','\'','%','s','\'',0};
    UINT len = lstrlenW(assocclass) + lstrlenW(dependent) + ARRAY_SIZE(fmtW);
    WCHAR *ret;

    if (!(ret = heap_alloc( len * sizeof(WCHAR) ))) return NULL;
    swprintf( ret, fmtW, assocclass, dependent );
    return ret;
}

static BSTR build_servername(void)
{
    WCHAR server[MAX_COMPUTERNAME_LENGTH + 1], *p;
    DWORD len = ARRAY_SIZE( server );

    if (!(GetComputerNameW( server, &len ))) return NULL;
    for (p = server; *p; p++) *p = towupper( *p );
    return SysAllocString( server );
}

static BSTR build_namespace(void)
{
    static const WCHAR cimv2W[] = {'R','O','O','T','\\','C','I','M','V','2',0};
    return SysAllocString( cimv2W );
}

static WCHAR *build_canonical_path( const WCHAR *relpath )
{
    static const WCHAR fmtW[] = {'\\','\\','%','s','\\','%','s',':',0};
    BSTR server, namespace;
    WCHAR *ret;
    UINT len, i;

    if (!(server = build_servername())) return NULL;
    if (!(namespace = build_namespace()))
    {
        SysFreeString( server );
        return NULL;
    }

    len = ARRAY_SIZE( fmtW ) + SysStringLen( server ) + SysStringLen( namespace ) + lstrlenW( relpath );
    if ((ret = heap_alloc( len * sizeof(WCHAR ) )))
    {
        len = swprintf( ret, fmtW, server, namespace );
        for (i = 0; i < lstrlenW( relpath ); i ++)
        {
            if (relpath[i] == '\'') ret[len++] = '"';
            else ret[len++] = relpath[i];
        }
        ret[len] = 0;
    }

    SysFreeString( server );
    SysFreeString( namespace );
    return ret;
}

static HRESULT get_antecedent( const WCHAR *assocclass, const WCHAR *dependent, BSTR *ret )
{
    static const WCHAR antecedentW[] = {'A','n','t','e','c','e','d','e','n','t',0};
    WCHAR *fullpath, *str;
    IEnumWbemClassObject *iter = NULL;
    IWbemClassObject *obj;
    HRESULT hr = E_OUTOFMEMORY;
    ULONG count;
    VARIANT var;

    if (!(fullpath = build_canonical_path( dependent ))) return E_OUTOFMEMORY;
    if (!(str = build_antecedent_query( assocclass, fullpath ))) goto done;
    if ((hr = exec_query( str, &iter )) != S_OK) goto done;

    IEnumWbemClassObject_Next( iter, WBEM_INFINITE, 1, &obj, &count );
    if (!count)
    {
        *ret = NULL;
        goto done;
    }

    hr = IWbemClassObject_Get( obj, antecedentW, 0, &var, NULL, NULL );
    IWbemClassObject_Release( obj );
    if (hr != S_OK) goto done;
    *ret = V_BSTR( &var );

done:
    if (iter) IEnumWbemClassObject_Release( iter );
    heap_free( str );
    heap_free( fullpath );
    return hr;
}

static HRESULT do_query( const WCHAR *str, struct query **ret_query )
{
    struct query *query;
    HRESULT hr;

    if (!(query = create_query())) return E_OUTOFMEMORY;
    if ((hr = parse_query( str, &query->view, &query->mem )) != S_OK || (hr = execute_view( query->view )) != S_OK)
    {
        release_query( query );
        return hr;
    }
    *ret_query = query;
    return S_OK;
}

static HRESULT get_antecedent_table( const WCHAR *assocclass, const WCHAR *dependent, struct table **table )
{
    BSTR antecedent = NULL;
    struct path *path = NULL;
    WCHAR *str = NULL;
    struct query *query = NULL;
    HRESULT hr;

    if ((hr = get_antecedent( assocclass, dependent, &antecedent )) != S_OK) return hr;
    if (!antecedent)
    {
        *table = NULL;
        return S_OK;
    }
    if ((hr = parse_path( antecedent, &path )) != S_OK) goto done;
    if (!(str = query_from_path( path )))
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    if ((hr = do_query( str, &query )) != S_OK) goto done;
    if (query->view->table_count) *table = addref_table( query->view->table[0] );
    else *table = NULL;

done:
    if (query) release_query( query );
    free_path( path );
    SysFreeString( antecedent );
    heap_free( str );
    return hr;
}

static HRESULT exec_assoc_view( struct view *view )
{
    static const WCHAR assocclassW[] = {'A','s','s','o','c','C','l','a','s','s',0};
    IEnumWbemClassObject *iter = NULL;
    struct path *path;
    HRESULT hr;

    if (view->keywordlist) FIXME( "ignoring keywords\n" );
    if ((hr = parse_path( view->path, &path )) != S_OK) return hr;

    if ((hr = create_assoc_enum( path->class, path->class_len, &iter )) != S_OK) goto done;
    for (;;)
    {
        ULONG count;
        IWbemClassObject *obj;
        struct table *table;
        VARIANT var;

        IEnumWbemClassObject_Next( iter, WBEM_INFINITE, 1, &obj, &count );
        if (!count) break;

        if ((hr = IWbemClassObject_Get( obj, assocclassW, 0, &var, NULL, NULL )) != S_OK)
        {
            IWbemClassObject_Release( obj );
            goto done;
        }
        IWbemClassObject_Release( obj );

        hr = get_antecedent_table( V_BSTR(&var), view->path, &table );
        VariantClear( &var );
        if (hr != S_OK) goto done;

        if (table && (hr = append_table( view, table )) != S_OK)
        {
            release_table( table );
            goto done;
        }
    }

    if (view->table_count)
    {
        if (!(view->result = heap_alloc_zero( view->table_count * sizeof(UINT) ))) hr = E_OUTOFMEMORY;
        else view->result_count = view->table_count;
    }

done:
    if (iter) IEnumWbemClassObject_Release( iter );
    free_path( path );
    return hr;
}

static HRESULT exec_select_view( struct view *view )
{
    UINT i, j = 0, len;
    enum fill_status status = FILL_STATUS_UNFILTERED;
    struct table *table;

    if (!view->table_count) return S_OK;

    table = view->table[0];
    if (table->fill)
    {
        clear_table( table );
        status = table->fill( table, view->cond );
    }
    if (status == FILL_STATUS_FAILED) return WBEM_E_FAILED;
    if (!table->num_rows) return S_OK;

    len = min( table->num_rows, 16 );
    if (!(view->result = heap_alloc( len * sizeof(UINT) ))) return E_OUTOFMEMORY;

    for (i = 0; i < table->num_rows; i++)
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
        if (status == FILL_STATUS_FILTERED) val = 1;
        else if ((hr = eval_cond( table, i, view->cond, &val, &type )) != S_OK) return hr;
        if (val) view->result[j++] = i;
    }

    view->result_count = j;
    return S_OK;
}

HRESULT execute_view( struct view *view )
{
    switch (view->type)
    {
    case VIEW_TYPE_ASSOCIATORS:
        return exec_assoc_view( view );

    case VIEW_TYPE_SELECT:
        return exec_select_view( view );

    default:
        ERR( "unhandled type %u\n", view->type );
        return E_INVALIDARG;
    }
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

BOOL is_result_prop( const struct view *view, const WCHAR *name )
{
    const struct property *prop = view->proplist;
    if (!prop) return TRUE;
    while (prop)
    {
        if (!wcsicmp( prop->name, name )) return TRUE;
        prop = prop->next;
    }
    return FALSE;
}

static BOOL is_system_prop( const WCHAR *name )
{
    return (name[0] == '_' && name[1] == '_');
}

static BSTR build_proplist( const struct table *table, UINT row, UINT count, UINT *len )
{
    static const WCHAR fmtW[] = {'%','s','=','%','s',0};
    UINT i, j, offset;
    BSTR *values, ret = NULL;

    if (!(values = heap_alloc( count * sizeof(BSTR) ))) return NULL;

    *len = j = 0;
    for (i = 0; i < table->num_cols; i++)
    {
        if (table->columns[i].type & COL_FLAG_KEY)
        {
            const WCHAR *name = table->columns[i].name;
            values[j] = get_value_bstr( table, row, i );
            *len += lstrlenW( fmtW ) + lstrlenW( name ) + lstrlenW( values[j] );
            j++;
        }
    }
    if ((ret = SysAllocStringLen( NULL, *len )))
    {
        offset = j = 0;
        for (i = 0; i < table->num_cols; i++)
        {
            if (table->columns[i].type & COL_FLAG_KEY)
            {
                const WCHAR *name = table->columns[i].name;
                offset += swprintf( ret + offset, fmtW, name, values[j] );
                if (j < count - 1) ret[offset++] = ',';
                j++;
            }
        }
    }
    for (i = 0; i < count; i++) SysFreeString( values[i] );
    heap_free( values );
    return ret;
}

static UINT count_key_columns( const struct table *table )
{
    UINT i, num_keys = 0;

    for (i = 0; i < table->num_cols; i++)
    {
        if (table->columns[i].type & COL_FLAG_KEY) num_keys++;
    }
    return num_keys;
}

static BSTR build_relpath( const struct view *view, UINT table_index, UINT result_index, const WCHAR *name )
{
    static const WCHAR fmtW[] = {'%','s','.','%','s',0};
    BSTR class, proplist, ret = NULL;
    struct table *table = view->table[table_index];
    UINT row = view->result[result_index];
    UINT num_keys, len;

    if (view->proplist) return NULL;

    if (!(class = SysAllocString( view->table[table_index]->name ))) return NULL;
    if (!(num_keys = count_key_columns( table ))) return class;
    if (!(proplist = build_proplist( table, row, num_keys, &len ))) goto done;

    len += lstrlenW( fmtW ) + SysStringLen( class );
    if (!(ret = SysAllocStringLen( NULL, len ))) goto done;
    swprintf( ret, fmtW, class, proplist );

done:
    SysFreeString( class );
    SysFreeString( proplist );
    return ret;
}

static BSTR build_path( const struct view *view, UINT table_index, UINT result_index, const WCHAR *name )
{
    static const WCHAR fmtW[] = {'\\','\\','%','s','\\','%','s',':','%','s',0};
    BSTR server, namespace = NULL, relpath = NULL, ret = NULL;
    UINT len;

    if (view->proplist) return NULL;

    if (!(server = build_servername())) return NULL;
    if (!(namespace = build_namespace())) goto done;
    if (!(relpath = build_relpath( view, table_index, result_index, name ))) goto done;

    len = lstrlenW( fmtW ) + SysStringLen( server ) + SysStringLen( namespace ) + SysStringLen( relpath );
    if (!(ret = SysAllocStringLen( NULL, len ))) goto done;
    swprintf( ret, fmtW, server, namespace, relpath );

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

static UINT count_properties( const struct table *table )
{
    UINT i, num_props = 0;

    for (i = 0; i < table->num_cols; i++)
    {
        if (!is_method( table, i )) num_props++;
    }
    return num_props;
}

static UINT count_result_properties( const struct view *view, UINT table_index )
{
    const struct property *prop = view->proplist;
    UINT count;

    if (!prop) return count_properties( view->table[table_index] );

    count = 1;
    while ((prop = prop->next)) count++;
    return count;
}

static HRESULT get_system_propval( const struct view *view, UINT table_index, UINT result_index, const WCHAR *name,
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

    if (!wcsicmp( name, classW ))
    {
        if (ret)
        {
            V_VT( ret ) = VT_BSTR;
            V_BSTR( ret ) = SysAllocString( view->table[table_index]->name );
        }
        if (type) *type = CIM_STRING;
        return S_OK;
    }
    if (!wcsicmp( name, genusW ))
    {
        if (ret)
        {
            V_VT( ret ) = VT_I4;
            V_I4( ret ) = WBEM_GENUS_INSTANCE; /* FIXME */
        }
        if (type) *type = CIM_SINT32;
        return S_OK;
    }
    else if (!wcsicmp( name, namespaceW ))
    {
        if (ret)
        {
            V_VT( ret ) = VT_BSTR;
            V_BSTR( ret ) = view->proplist ? NULL : build_namespace();
        }
        if (type) *type = CIM_STRING;
        return S_OK;
    }
    else if (!wcsicmp( name, pathW ))
    {
        if (ret)
        {
            V_VT( ret ) = VT_BSTR;
            V_BSTR( ret ) = build_path( view, table_index, result_index, name );
        }
        if (type) *type = CIM_STRING;
        return S_OK;
    }
    if (!wcsicmp( name, propcountW ))
    {
        if (ret)
        {
            V_VT( ret ) = VT_I4;
            V_I4( ret ) = count_result_properties( view, table_index );
        }
        if (type) *type = CIM_SINT32;
        return S_OK;
    }
    else if (!wcsicmp( name, relpathW ))
    {
        if (ret)
        {
            V_VT( ret ) = VT_BSTR;
            V_BSTR( ret ) = build_relpath( view, table_index, result_index, name );
        }
        if (type) *type = CIM_STRING;
        return S_OK;
    }
    else if (!wcsicmp( name, serverW ))
    {
        if (ret)
        {
            V_VT( ret ) = VT_BSTR;
            V_BSTR( ret ) = view->proplist ? NULL : build_servername();
        }
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
    case CIM_BOOLEAN:   return VT_BOOL;

    case CIM_STRING:
    case CIM_REFERENCE:
    case CIM_DATETIME:  return VT_BSTR;

    case CIM_SINT8:     return VT_I1;
    case CIM_UINT8:     return VT_UI1;
    case CIM_SINT16:    return VT_I2;

    case CIM_UINT16:
    case CIM_SINT32:
    case CIM_UINT32:    return VT_I4;

    case CIM_SINT64:    return VT_I8;
    case CIM_UINT64:    return VT_UI8;

    case CIM_REAL32:    return VT_R4;

    default:
        ERR("unhandled type %u\n", type);
        break;
    }
    return 0;
}

SAFEARRAY *to_safearray( const struct array *array, CIMTYPE basetype )
{
    SAFEARRAY *ret;
    VARTYPE vartype = to_vartype( basetype );
    LONG i;

    if (!array || !(ret = SafeArrayCreateVector( vartype, 0, array->count ))) return NULL;

    for (i = 0; i < array->count; i++)
    {
        void *ptr = (char *)array->ptr + i * array->elem_size;
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
    case VT_R4:
        V_R4( ret ) = *(FLOAT *)&val;
        break;
    default:
        ERR("unhandled variant type %u\n", type);
        return;
    }
    V_VT( ret ) = type;
}

static HRESULT map_view_index( const struct view *view, UINT index, UINT *table_index, UINT *result_index )
{
    if (!view->table) return WBEM_E_NOT_FOUND;

    switch (view->type)
    {
    case VIEW_TYPE_SELECT:
        *table_index = 0;
        *result_index = index;
        break;

    case VIEW_TYPE_ASSOCIATORS:
        *table_index = *result_index = index;
        break;

    default:
        ERR( "unhandled view type %u\n", view->type );
        return WBEM_E_FAILED;
    }
    return S_OK;
}

struct table *get_view_table( const struct view *view, UINT index )
{
    switch (view->type)
    {
    case VIEW_TYPE_SELECT:
        return view->table[0];

    case VIEW_TYPE_ASSOCIATORS:
        return view->table[index];

    default:
        ERR( "unhandled view type %u\n", view->type );
        return NULL;
    }
}

HRESULT get_propval( const struct view *view, UINT index, const WCHAR *name, VARIANT *ret, CIMTYPE *type,
                     LONG *flavor )
{
    HRESULT hr;
    UINT column, row, table_index, result_index;
    struct table *table;
    VARTYPE vartype;
    void *val_ptr = NULL;
    LONGLONG val;

    if ((hr = map_view_index( view, index, &table_index, &result_index )) != S_OK) return hr;

    if (is_system_prop( name )) return get_system_propval( view, table_index, result_index, name, ret, type, flavor );
    if (!view->result_count || !is_result_prop( view, name )) return WBEM_E_NOT_FOUND;

    table = view->table[table_index];
    hr = get_column_index( table, name, &column );
    if (hr != S_OK || is_method( table, column )) return WBEM_E_NOT_FOUND;

    row = view->result[result_index];
    hr = get_value( table, row, column, &val );
    if (hr != S_OK) return hr;

    if (type) *type = table->columns[column].type & COL_TYPE_MASK;
    if (flavor) *flavor = 0;

    if (!ret) return S_OK;

    vartype = to_vartype( table->columns[column].type & CIM_TYPE_MASK );
    if (table->columns[column].type & CIM_FLAG_ARRAY)
    {
        CIMTYPE basetype = table->columns[column].type & CIM_TYPE_MASK;

        val_ptr = to_safearray( (const struct array *)(INT_PTR)val, basetype );
        if (!val_ptr) vartype = VT_NULL;
        else vartype |= VT_ARRAY;
        set_variant( vartype, val, val_ptr, ret );
        return S_OK;
    }

    switch (table->columns[column].type & COL_TYPE_MASK)
    {
    case CIM_STRING:
    case CIM_REFERENCE:
    case CIM_DATETIME:
        if (val)
        {
            vartype = VT_BSTR;
            val_ptr = SysAllocString( (const WCHAR *)(INT_PTR)val );
        }
        else
            vartype = VT_NULL;
        break;
    case CIM_SINT64:
        vartype = VT_BSTR;
        val_ptr = get_value_bstr( table, row, column );
        break;
    case CIM_UINT64:
        vartype = VT_BSTR;
        val_ptr = get_value_bstr( table, row, column );
        break;
    case CIM_BOOLEAN:
    case CIM_SINT8:
    case CIM_UINT8:
    case CIM_SINT16:
    case CIM_UINT16:
    case CIM_SINT32:
    case CIM_UINT32:
    case CIM_REAL32:
        break;
    default:
        ERR("unhandled column type %u\n", table->columns[column].type);
        return WBEM_E_FAILED;
    }

    set_variant( vartype, val, val_ptr, ret );
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

    if (SafeArrayGetVartype( V_ARRAY( var ), &vartype ) != S_OK) return NULL;
    if (!(basetype = to_cimtype( vartype ))) return NULL;
    if (SafeArrayGetUBound( V_ARRAY( var ), 1, &bound ) != S_OK) return NULL;
    if (!(ret = heap_alloc( sizeof(struct array) ))) return NULL;

    ret->count     = bound + 1;
    ret->elem_size = get_type_size( basetype );
    if (!(ret->ptr = heap_alloc_zero( ret->count * ret->elem_size )))
    {
        heap_free( ret );
        return NULL;
    }
    for (i = 0; i < ret->count; i++)
    {
        void *ptr = (char *)ret->ptr + i * ret->elem_size;
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
    UINT row, column, table_index, result_index;
    struct table *table;
    LONGLONG val;

    if ((hr = map_view_index( view, index, &table_index, &result_index )) != S_OK) return hr;

    table = view->table[table_index];
    hr = get_column_index( table, name, &column );
    if (hr != S_OK)
    {
        FIXME("no support for creating new properties\n");
        return WBEM_E_FAILED;
    }
    if (is_method( table, column ) || !(table->columns[column].type & COL_FLAG_DYNAMIC))
        return WBEM_E_FAILED;

    hr = to_longlong( var, &val, &type );
    if (hr != S_OK) return hr;

    row = view->result[result_index];
    return set_value( table, row, column, val, type );
}

HRESULT get_properties( const struct view *view, UINT index, LONG flags, SAFEARRAY **props )
{
    SAFEARRAY *sa;
    BSTR str;
    UINT i, table_index, result_index, num_props;
    struct table *table;
    HRESULT hr;
    LONG j;

    if ((hr = map_view_index( view, index, &table_index, &result_index )) != S_OK) return hr;

    num_props = count_result_properties( view, table_index );
    if (!(sa = SafeArrayCreateVector( VT_BSTR, 0, num_props ))) return E_OUTOFMEMORY;

    table = view->table[table_index];
    for (i = 0, j = 0; i < table->num_cols; i++)
    {
        BOOL is_system;

        if (is_method( table, i )) continue;
        if (!is_result_prop( view, table->columns[i].name )) continue;

        is_system = is_system_prop( table->columns[i].name );
        if ((flags & WBEM_FLAG_NONSYSTEM_ONLY) && is_system) continue;
        else if ((flags & WBEM_FLAG_SYSTEM_ONLY) && !is_system) continue;

        str = SysAllocString( table->columns[i].name );
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
