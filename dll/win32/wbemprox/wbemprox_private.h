/*
 * Copyright 2009 Hans Leidekker for CodeWeavers
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

#include "wine/debug.h"
#include "wine/list.h"
#ifdef __REACTOS__
#include <winnls.h>
#endif

enum wbm_namespace
{
    WBEMPROX_NAMESPACE_CIMV2,
    WBEMPROX_NAMESPACE_MS_WINDOWS_STORAGE,
    WBEMPROX_NAMESPACE_STANDARDCIMV2,
    WBEMPROX_NAMESPACE_WMI,
    WBEMPROX_NAMESPACE_LAST,
};

extern IClientSecurity client_security;
extern struct list *table_list[WBEMPROX_NAMESPACE_LAST];

enum param_direction
{
    PARAM_OUT   = -1,
    PARAM_INOUT = 0,
    PARAM_IN    = 1
};

#define CIM_TYPE_MASK    0x00000fff

#define COL_TYPE_MASK    0x0000ffff
#define COL_FLAG_DYNAMIC 0x00010000
#define COL_FLAG_KEY     0x00020000
#define COL_FLAG_METHOD  0x00040000

typedef HRESULT (class_method)(IWbemClassObject *object, IWbemContext *context, IWbemClassObject *in_params,
        IWbemClassObject **out_params);

enum operator
{
    OP_EQ      = 1,
    OP_AND     = 2,
    OP_OR      = 3,
    OP_GT      = 4,
    OP_LT      = 5,
    OP_LE      = 6,
    OP_GE      = 7,
    OP_NE      = 8,
    OP_ISNULL  = 9,
    OP_NOTNULL = 10,
    OP_LIKE    = 11,
    OP_NOT     = 12
};

struct expr;
struct complex_expr
{
    enum operator op;
    struct expr *left;
    struct expr *right;
};

enum expr_type
{
    EXPR_COMPLEX = 1,
    EXPR_UNARY   = 2,
    EXPR_PROPVAL = 3,
    EXPR_SVAL    = 4,
    EXPR_IVAL    = 5,
    EXPR_BVAL    = 6
};

struct expr
{
    enum expr_type type;
    union
    {
        struct complex_expr expr;
        const struct property *propval;
        const WCHAR *sval;
        int ival;
    } u;
};

struct column
{
    const WCHAR *name;
    UINT type;
};

enum fill_status
{
    FILL_STATUS_FAILED = -1,
    FILL_STATUS_UNFILTERED,
    FILL_STATUS_FILTERED
};

#define TABLE_FLAG_DYNAMIC 0x00000001

struct table
{
    const WCHAR *name;
    UINT num_cols;
    const struct column *columns;
    UINT num_rows;
    UINT num_rows_allocated;
    BYTE *data;
    enum fill_status (*fill)(struct table *, const struct expr *cond);
    UINT flags;
    struct list entry;
    LONG refs;
    CRITICAL_SECTION cs;
    BOOL removed;
};

struct property
{
    const WCHAR *name;
    const WCHAR *class;
    const struct property *next;
};

struct array
{
    UINT elem_size;
    UINT count;
    void *ptr;
};

struct field
{
    UINT type;
    union
    {
        LONGLONG ival;
        WCHAR *sval;
        struct array *aval;
    } u;
};

struct record
{
    UINT count;
    struct field *fields;
    struct table *table;
};

struct keyword
{
    const WCHAR *name;
    const WCHAR *value;
    const struct keyword *next;
};

enum view_type
{
    VIEW_TYPE_SELECT,
    VIEW_TYPE_ASSOCIATORS,
};

struct view
{
    enum wbm_namespace ns;
    enum view_type type;
    const WCHAR *path;                      /* ASSOCIATORS OF query */
    const struct keyword *keywordlist;
    const struct property *proplist;        /* SELECT query */
    const struct expr *cond;
    UINT table_count;
    struct table **table;
    UINT result_count;
    UINT *result;
};

struct query
{
    LONG refs;
    enum wbm_namespace ns;
    struct view *view;
    struct list mem;
};

struct path
{
    WCHAR *class;
    UINT   class_len;
    WCHAR *filter;
    UINT   filter_len;
};

HRESULT parse_path( const WCHAR *, struct path ** );
void free_path( struct path * );
WCHAR *query_from_path( const struct path * );

struct query *create_query( enum wbm_namespace );
void free_query( struct query * );
struct query *addref_query( struct query * );
void release_query( struct query *query );
HRESULT exec_query( enum wbm_namespace, const WCHAR *, IEnumWbemClassObject ** );
HRESULT parse_query( enum wbm_namespace, const WCHAR *, struct view **, struct list * );
HRESULT create_view( enum view_type, enum wbm_namespace, const WCHAR *, const struct keyword *, const WCHAR *,
                     const struct property *, const struct expr *, struct view ** );
void destroy_view( struct view * );
HRESULT execute_view( struct view * );
struct table *get_view_table( const struct view *, UINT );
void init_table_list( void );
enum wbm_namespace get_namespace_from_string( const WCHAR *namespace );
struct table *find_table( enum wbm_namespace, const WCHAR * );
struct table *grab_table( struct table * );
void release_table( struct table * );
struct table *create_table( const WCHAR *, UINT, const struct column *, UINT, UINT, BYTE *,
                            enum fill_status (*)(struct table *, const struct expr *) );
BOOL add_table( enum wbm_namespace, struct table * );
void free_columns( struct column *, UINT );
void free_row_values( const struct table *, UINT );
void clear_table( struct table * );
void free_table( struct table * );
UINT get_type_size( CIMTYPE );
HRESULT eval_cond( const struct table *, UINT, const struct expr *, LONGLONG *, UINT * );
HRESULT get_column_index( const struct table *, const WCHAR *, UINT * );
HRESULT get_value( const struct table *, UINT, UINT, LONGLONG * );
BSTR get_value_bstr( const struct table *, UINT, UINT );
HRESULT set_value( const struct table *, UINT, UINT, LONGLONG, CIMTYPE );
BOOL is_method( const struct table *, UINT );
HRESULT get_method( const struct table *, const WCHAR *, class_method ** );
HRESULT get_propval( const struct view *, UINT, const WCHAR *, VARIANT *, CIMTYPE *, LONG * );
HRESULT put_propval( const struct view *, UINT, const WCHAR *, VARIANT *, CIMTYPE );
HRESULT to_longlong( VARIANT *, LONGLONG *, CIMTYPE * );
SAFEARRAY *to_safearray( const struct array *, CIMTYPE );
VARTYPE to_vartype( CIMTYPE );
void destroy_array( struct array *, CIMTYPE );
BOOL is_result_prop( const struct view *, const WCHAR * );
HRESULT get_properties( const struct view *, UINT, LONG, SAFEARRAY ** );
HRESULT get_object( enum wbm_namespace, const WCHAR *, IWbemClassObject ** );
BSTR get_method_name( enum wbm_namespace, const WCHAR *, UINT );
WCHAR *get_first_key_property( enum wbm_namespace, const WCHAR * );
void set_variant( VARTYPE, LONGLONG, void *, VARIANT * );
HRESULT create_signature( enum wbm_namespace ns, const WCHAR *, const WCHAR *, enum param_direction,
                          IWbemClassObject ** );

HRESULT WbemLocator_create(LPVOID *, REFIID);
HRESULT WbemServices_create(const WCHAR *, IWbemContext *, LPVOID *);
HRESULT WbemContext_create(void **, REFIID);
HRESULT create_class_object(enum wbm_namespace ns, const WCHAR *, IEnumWbemClassObject *, UINT,
                            struct record *, IWbemClassObject **);
HRESULT EnumWbemClassObject_create(struct query *, LPVOID *);
HRESULT WbemQualifierSet_create(enum wbm_namespace, const WCHAR *, const WCHAR *, LPVOID *);

HRESULT process_get_owner(IWbemClassObject *obj, IWbemContext *context, IWbemClassObject *in, IWbemClassObject **out);
HRESULT process_create(IWbemClassObject *obj, IWbemContext *context, IWbemClassObject *in, IWbemClassObject **out);
HRESULT reg_create_key(IWbemClassObject *obj, IWbemContext *context, IWbemClassObject *in, IWbemClassObject **out);
HRESULT reg_enum_key(IWbemClassObject *obj, IWbemContext *context, IWbemClassObject *in, IWbemClassObject **out);
HRESULT reg_enum_values(IWbemClassObject *obj, IWbemContext *context, IWbemClassObject *in, IWbemClassObject **out);
HRESULT reg_get_binaryvalue(IWbemClassObject *obj, IWbemContext *context, IWbemClassObject *in, IWbemClassObject **out);
HRESULT reg_get_stringvalue(IWbemClassObject *obj, IWbemContext *context, IWbemClassObject *in, IWbemClassObject **out);
HRESULT reg_set_stringvalue(IWbemClassObject *obj, IWbemContext *context, IWbemClassObject *in, IWbemClassObject **out);
HRESULT reg_set_dwordvalue(IWbemClassObject *obj, IWbemContext *context, IWbemClassObject *in, IWbemClassObject **out);
HRESULT reg_delete_key(IWbemClassObject *obj, IWbemContext *context, IWbemClassObject *in, IWbemClassObject **out);
HRESULT service_pause_service(IWbemClassObject *obj, IWbemContext *context, IWbemClassObject *in, IWbemClassObject **out);
HRESULT service_resume_service(IWbemClassObject *obj, IWbemContext *context, IWbemClassObject *in, IWbemClassObject **out);
HRESULT service_start_service(IWbemClassObject *obj, IWbemContext *context, IWbemClassObject *in, IWbemClassObject **out);
HRESULT service_stop_service(IWbemClassObject *obj, IWbemContext *context, IWbemClassObject *in, IWbemClassObject **out);
HRESULT security_get_sd(IWbemClassObject *obj, IWbemContext *context, IWbemClassObject *in, IWbemClassObject **out);
HRESULT security_set_sd(IWbemClassObject *obj, IWbemContext *context, IWbemClassObject *in, IWbemClassObject **out);
HRESULT sysrestore_create(IWbemClassObject *obj, IWbemContext *context, IWbemClassObject *in, IWbemClassObject **out);
HRESULT sysrestore_disable(IWbemClassObject *obj, IWbemContext *context, IWbemClassObject *in, IWbemClassObject **out);
HRESULT sysrestore_enable(IWbemClassObject *obj, IWbemContext *context, IWbemClassObject *in, IWbemClassObject **out);
HRESULT sysrestore_get_last_status(IWbemClassObject *obj, IWbemContext *context, IWbemClassObject *in, IWbemClassObject **out);
HRESULT sysrestore_restore(IWbemClassObject *obj, IWbemContext *context, IWbemClassObject *in, IWbemClassObject **out);

static inline WCHAR *heap_strdupAW( const char *src )
{
    int len;
    WCHAR *dst;
    if (!src) return NULL;
    len = MultiByteToWideChar( CP_ACP, 0, src, -1, NULL, 0 );
    if ((dst = malloc( len * sizeof(*dst) ))) MultiByteToWideChar( CP_ACP, 0, src, -1, dst, len );
    return dst;
}

static inline BOOL is_digit(WCHAR c)
{
    return '0' <= c && c <= '9';
}
