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

#ifndef _WBEMPROX_PRIVATE_H_
#define _WBEMPROX_PRIVATE_H_

#include <config.h>

#include <stdarg.h>

#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winsvc.h>
#include <objbase.h>
#include <oleauto.h>
#include <wbemcli.h>

#include <wine/debug.h>
#include <wine/list.h>
#include <wine/unicode.h>

WINE_DEFAULT_DEBUG_CHANNEL(wbemprox);

IClientSecurity client_security DECLSPEC_HIDDEN;
struct list *table_list DECLSPEC_HIDDEN;

#define SIZEOF(array) (sizeof(array)/sizeof((array)[0]))

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

typedef HRESULT (class_method)(IWbemClassObject *, IWbemClassObject *, IWbemClassObject **);

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
    VARTYPE vartype; /* 0 for default mapping */
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
};

struct property
{
    const WCHAR *name;
    const WCHAR *class;
    const struct property *next;
};

struct array
{
    UINT count;
    void *ptr;
};

struct field
{
    UINT type;
    VARTYPE vartype; /* 0 for default mapping */
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

struct view
{
    const struct property *proplist;
    struct table *table;
    const struct expr *cond;
    UINT *result;
    UINT  count;
};

struct query
{
    LONG refs;
    struct view *view;
    struct list mem;
};

struct query *create_query(void) DECLSPEC_HIDDEN;
void free_query( struct query * ) DECLSPEC_HIDDEN;
struct query *addref_query( struct query * ) DECLSPEC_HIDDEN;
void release_query( struct query *query ) DECLSPEC_HIDDEN;
HRESULT exec_query( const WCHAR *, IEnumWbemClassObject ** ) DECLSPEC_HIDDEN;
HRESULT parse_query( const WCHAR *, struct view **, struct list * ) DECLSPEC_HIDDEN;
HRESULT create_view( const struct property *, const WCHAR *, const struct expr *,
                     struct view ** ) DECLSPEC_HIDDEN;
void destroy_view( struct view * ) DECLSPEC_HIDDEN;
HRESULT execute_view( struct view * ) DECLSPEC_HIDDEN;
void init_table_list( void ) DECLSPEC_HIDDEN;
struct table *grab_table( const WCHAR * ) DECLSPEC_HIDDEN;
struct table *addref_table( struct table * ) DECLSPEC_HIDDEN;
void release_table( struct table * ) DECLSPEC_HIDDEN;
struct table *create_table( const WCHAR *, UINT, const struct column *, UINT, UINT, BYTE *,
                            enum fill_status (*)(struct table *, const struct expr *) ) DECLSPEC_HIDDEN;
BOOL add_table( struct table * ) DECLSPEC_HIDDEN;
void free_columns( struct column *, UINT ) DECLSPEC_HIDDEN;
void free_row_values( const struct table *, UINT ) DECLSPEC_HIDDEN;
void clear_table( struct table * ) DECLSPEC_HIDDEN;
void free_table( struct table * ) DECLSPEC_HIDDEN;
UINT get_type_size( CIMTYPE ) DECLSPEC_HIDDEN;
HRESULT eval_cond( const struct table *, UINT, const struct expr *, LONGLONG *, UINT * ) DECLSPEC_HIDDEN;
HRESULT get_column_index( const struct table *, const WCHAR *, UINT * ) DECLSPEC_HIDDEN;
HRESULT get_value( const struct table *, UINT, UINT, LONGLONG * ) DECLSPEC_HIDDEN;
BSTR get_value_bstr( const struct table *, UINT, UINT ) DECLSPEC_HIDDEN;
HRESULT set_value( const struct table *, UINT, UINT, LONGLONG, CIMTYPE ) DECLSPEC_HIDDEN;
BOOL is_method( const struct table *, UINT ) DECLSPEC_HIDDEN;
HRESULT get_method( const struct table *, const WCHAR *, class_method ** ) DECLSPEC_HIDDEN;
HRESULT get_propval( const struct view *, UINT, const WCHAR *, VARIANT *,
                     CIMTYPE *, LONG * ) DECLSPEC_HIDDEN;
HRESULT put_propval( const struct view *, UINT, const WCHAR *, VARIANT *, CIMTYPE ) DECLSPEC_HIDDEN;
HRESULT to_longlong( VARIANT *, LONGLONG *, CIMTYPE * ) DECLSPEC_HIDDEN;
SAFEARRAY *to_safearray( const struct array *, CIMTYPE ) DECLSPEC_HIDDEN;
VARTYPE to_vartype( CIMTYPE ) DECLSPEC_HIDDEN;
void destroy_array( struct array *, CIMTYPE ) DECLSPEC_HIDDEN;
BOOL is_selected_prop( const struct view *, const WCHAR * ) DECLSPEC_HIDDEN;
HRESULT get_properties( const struct view *, LONG, SAFEARRAY ** ) DECLSPEC_HIDDEN;
HRESULT get_object( const WCHAR *, IWbemClassObject ** ) DECLSPEC_HIDDEN;
BSTR get_method_name( const WCHAR *, UINT ) DECLSPEC_HIDDEN;
void set_variant( VARTYPE, LONGLONG, void *, VARIANT * ) DECLSPEC_HIDDEN;
HRESULT create_signature( const WCHAR *, const WCHAR *, enum param_direction,
                          IWbemClassObject ** ) DECLSPEC_HIDDEN;

HRESULT WbemLocator_create(LPVOID *) DECLSPEC_HIDDEN;
HRESULT WbemServices_create(const WCHAR *, LPVOID *) DECLSPEC_HIDDEN;
HRESULT create_class_object(const WCHAR *, IEnumWbemClassObject *, UINT,
                            struct record *, IWbemClassObject **) DECLSPEC_HIDDEN;
HRESULT EnumWbemClassObject_create(struct query *, LPVOID *) DECLSPEC_HIDDEN;
HRESULT WbemQualifierSet_create(const WCHAR *, const WCHAR *, LPVOID *) DECLSPEC_HIDDEN;

HRESULT process_get_owner(IWbemClassObject *, IWbemClassObject *, IWbemClassObject **) DECLSPEC_HIDDEN;
HRESULT reg_enum_key(IWbemClassObject *, IWbemClassObject *, IWbemClassObject **) DECLSPEC_HIDDEN;
HRESULT reg_enum_values(IWbemClassObject *, IWbemClassObject *, IWbemClassObject **) DECLSPEC_HIDDEN;
HRESULT reg_get_stringvalue(IWbemClassObject *, IWbemClassObject *, IWbemClassObject **) DECLSPEC_HIDDEN;
HRESULT service_pause_service(IWbemClassObject *, IWbemClassObject *, IWbemClassObject **) DECLSPEC_HIDDEN;
HRESULT service_resume_service(IWbemClassObject *, IWbemClassObject *, IWbemClassObject **) DECLSPEC_HIDDEN;
HRESULT service_start_service(IWbemClassObject *, IWbemClassObject *, IWbemClassObject **) DECLSPEC_HIDDEN;
HRESULT service_stop_service(IWbemClassObject *, IWbemClassObject *, IWbemClassObject **) DECLSPEC_HIDDEN;
HRESULT security_get_sd(IWbemClassObject *, IWbemClassObject *, IWbemClassObject **) DECLSPEC_HIDDEN;
HRESULT security_set_sd(IWbemClassObject *, IWbemClassObject *, IWbemClassObject **) DECLSPEC_HIDDEN;

static void *heap_alloc( size_t len ) __WINE_ALLOC_SIZE(1);
static inline void *heap_alloc( size_t len )
{
    return HeapAlloc( GetProcessHeap(), 0, len );
}

static void *heap_realloc( void *mem, size_t len ) __WINE_ALLOC_SIZE(2);
static inline void *heap_realloc( void *mem, size_t len )
{
    return HeapReAlloc( GetProcessHeap(), 0, mem, len );
}

static void *heap_alloc_zero( size_t len ) __WINE_ALLOC_SIZE(1);
static inline void *heap_alloc_zero( size_t len )
{
    return HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, len );
}

static inline BOOL heap_free( void *mem )
{
    return HeapFree( GetProcessHeap(), 0, mem );
}

static inline WCHAR *heap_strdupW( const WCHAR *src )
{
    WCHAR *dst;
    if (!src) return NULL;
    if ((dst = heap_alloc( (strlenW( src ) + 1) * sizeof(WCHAR) ))) strcpyW( dst, src );
    return dst;
}

static const WCHAR class_processW[] = {'W','i','n','3','2','_','P','r','o','c','e','s','s',0};
static const WCHAR class_serviceW[] = {'W','i','n','3','2','_','S','e','r','v','i','c','e',0};
static const WCHAR class_stdregprovW[] = {'S','t','d','R','e','g','P','r','o','v',0};
static const WCHAR class_systemsecurityW[] = {'_','_','S','y','s','t','e','m','S','e','c','u','r','i','t','y',0};

static const WCHAR prop_nameW[] = {'N','a','m','e',0};

static const WCHAR method_enumkeyW[] = {'E','n','u','m','K','e','y',0};
static const WCHAR method_enumvaluesW[] = {'E','n','u','m','V','a','l','u','e','s',0};
static const WCHAR method_getownerW[] = {'G','e','t','O','w','n','e','r',0};
static const WCHAR method_getsdW[] = {'G','e','t','S','D',0};
static const WCHAR method_getstringvalueW[] = {'G','e','t','S','t','r','i','n','g','V','a','l','u','e',0};
static const WCHAR method_pauseserviceW[] = {'P','a','u','s','e','S','e','r','v','i','c','e',0};
static const WCHAR method_resumeserviceW[] = {'R','e','s','u','m','e','S','e','r','v','i','c','e',0};
static const WCHAR method_setsdW[] = {'S','e','t','S','D',0};
static const WCHAR method_startserviceW[] = {'S','t','a','r','t','S','e','r','v','i','c','e',0};
static const WCHAR method_stopserviceW[] = {'S','t','o','p','S','e','r','v','i','c','e',0};

static const WCHAR param_defkeyW[] = {'h','D','e','f','K','e','y',0};
static const WCHAR param_domainW[] = {'D','o','m','a','i','n',0};
static const WCHAR param_namesW[] = {'s','N','a','m','e','s',0};
static const WCHAR param_returnvalueW[] = {'R','e','t','u','r','n','V','a','l','u','e',0};
static const WCHAR param_sdW[] = {'S','D',0};
static const WCHAR param_subkeynameW[] = {'s','S','u','b','K','e','y','N','a','m','e',0};
static const WCHAR param_typesW[] = {'T','y','p','e','s',0};
static const WCHAR param_userW[] = {'U','s','e','r',0};
static const WCHAR param_valueW[] = {'s','V','a','l','u','e',0};
static const WCHAR param_valuenameW[] = {'s','V','a','l','u','e','N','a','m','e',0};

#endif /* _WBEMPROX_PRIVATE_H_ */
