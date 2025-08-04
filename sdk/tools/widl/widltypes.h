/*
 * IDL Compiler
 *
 * Copyright 2002 Ove Kaaven
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

#ifndef __WIDL_WIDLTYPES_H
#define __WIDL_WIDLTYPES_H

#include <stdarg.h>
#include <stdbool.h>
#include <assert.h>
#include "ndrtypes.h"
#include "wine/list.h"

struct uuid
{
    unsigned int   Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char  Data4[8];
};

#define TRUE 1
#define FALSE 0

typedef struct _attr_t attr_t;
typedef struct _attr_custdata_t attr_custdata_t;
typedef struct _expr_t expr_t;
typedef struct _type_t type_t;
typedef struct _var_t var_t;
typedef struct _decl_spec_t decl_spec_t;
typedef struct _declarator_t declarator_t;
typedef struct _typeref_t typeref_t;
typedef struct _typelib_entry_t typelib_entry_t;
typedef struct _importlib_t importlib_t;
typedef struct _importinfo_t importinfo_t;
typedef struct _typelib_t typelib_t;
typedef struct _user_type_t user_type_t;
typedef struct _user_type_t context_handle_t;
typedef struct _user_type_t generic_handle_t;
typedef struct _statement_t statement_t;
typedef struct _warning_t warning_t;

typedef struct list attr_list_t;
typedef struct list str_list_t;
typedef struct list expr_list_t;
typedef struct list var_list_t;
typedef struct list declarator_list_t;
typedef struct list typeref_list_t;
typedef struct list user_type_list_t;
typedef struct list context_handle_list_t;
typedef struct list generic_handle_list_t;
typedef struct list statement_list_t;
typedef struct list warning_list_t;

enum attr_type
{
    ATTR_ACTIVATABLE,
    ATTR_AGGREGATABLE,
    ATTR_ALLOCATE,
    ATTR_ANNOTATION,
    ATTR_APPOBJECT,
    ATTR_ASYNC,
    ATTR_ASYNCUUID,
    ATTR_AUTO_HANDLE,
    ATTR_BINDABLE,
    ATTR_BROADCAST,
    ATTR_CALLAS,
    ATTR_CALLCONV, /* calling convention pseudo-attribute */
    ATTR_CASE,
    ATTR_CODE,
    ATTR_COMMSTATUS,
    ATTR_COMPOSABLE,
    ATTR_CONTEXTHANDLE,
    ATTR_CONTRACT,
    ATTR_CONTRACTVERSION,
    ATTR_CONTROL,
    ATTR_CUSTOM,
    ATTR_DECODE,
    ATTR_DEFAULT,
    ATTR_DEFAULT_OVERLOAD,
    ATTR_DEFAULTBIND,
    ATTR_DEFAULTCOLLELEM,
    ATTR_DEFAULTVALUE,
    ATTR_DEFAULTVTABLE,
    ATTR_DEPRECATED,
    ATTR_DISABLECONSISTENCYCHECK,
    ATTR_DISPINTERFACE,
    ATTR_DISPLAYBIND,
    ATTR_DLLNAME,
    ATTR_DUAL,
    ATTR_ENABLEALLOCATE,
    ATTR_ENCODE,
    ATTR_ENDPOINT,
    ATTR_ENTRY,
    ATTR_EVENTADD,
    ATTR_EVENTREMOVE,
    ATTR_EXCLUSIVETO,
    ATTR_EXPLICIT_HANDLE,
    ATTR_FAULTSTATUS,
    ATTR_FLAGS,
    ATTR_FORCEALLOCATE,
    ATTR_HANDLE,
    ATTR_HELPCONTEXT,
    ATTR_HELPFILE,
    ATTR_HELPSTRING,
    ATTR_HELPSTRINGCONTEXT,
    ATTR_HELPSTRINGDLL,
    ATTR_HIDDEN,
    ATTR_ID,
    ATTR_IDEMPOTENT,
    ATTR_IGNORE,
    ATTR_IIDIS,
    ATTR_IMMEDIATEBIND,
    ATTR_IMPLICIT_HANDLE,
    ATTR_IN,
    ATTR_INPUTSYNC,
    ATTR_LENGTHIS,
    ATTR_LIBLCID,
    ATTR_LICENSED,
    ATTR_LOCAL,
    ATTR_MARSHALING_BEHAVIOR,
    ATTR_MAYBE,
    ATTR_MESSAGE,
    ATTR_NOCODE,
    ATTR_NONBROWSABLE,
    ATTR_NONCREATABLE,
    ATTR_NONEXTENSIBLE,
    ATTR_NOTIFY,
    ATTR_NOTIFYFLAG,
    ATTR_OBJECT,
    ATTR_ODL,
    ATTR_OLEAUTOMATION,
    ATTR_OPTIMIZE,
    ATTR_OPTIONAL,
    ATTR_OUT,
    ATTR_OVERLOAD,
    ATTR_PARAMLCID,
    ATTR_PARTIALIGNORE,
    ATTR_POINTERDEFAULT,
    ATTR_POINTERTYPE,
    ATTR_PROGID,
    ATTR_PROPGET,
    ATTR_PROPPUT,
    ATTR_PROPPUTREF,
    ATTR_PROTECTED,
    ATTR_PROXY,
    ATTR_PUBLIC,
    ATTR_RANGE,
    ATTR_READONLY,
    ATTR_REPRESENTAS,
    ATTR_REQUESTEDIT,
    ATTR_RESTRICTED,
    ATTR_RETVAL,
    ATTR_SIZEIS,
    ATTR_SOURCE,
    ATTR_STATIC,
    ATTR_STRICTCONTEXTHANDLE,
    ATTR_STRING,
    ATTR_SWITCHIS,
    ATTR_SWITCHTYPE,
    ATTR_THREADING,
    ATTR_TRANSMITAS,
    ATTR_UIDEFAULT,
    ATTR_USERMARSHAL,
    ATTR_USESGETLASTERROR,
    ATTR_UUID,
    ATTR_V1ENUM,
    ATTR_VARARG,
    ATTR_VERSION,
    ATTR_VIPROGID,
    ATTR_WIREMARSHAL
};

enum expr_type
{
    EXPR_VOID,
    EXPR_NUM,
    EXPR_DOUBLE,
    EXPR_IDENTIFIER,
    EXPR_NEG,
    EXPR_NOT,
    EXPR_PPTR,
    EXPR_CAST,
    EXPR_SIZEOF,
    EXPR_SHL,
    EXPR_SHR,
    EXPR_MUL,
    EXPR_DIV,
    EXPR_ADD,
    EXPR_SUB,
    EXPR_AND,
    EXPR_OR,
    EXPR_COND,
    EXPR_TRUEFALSE,
    EXPR_ADDRESSOF,
    EXPR_MEMBER,
    EXPR_ARRAY,
    EXPR_MOD,
    EXPR_LOGOR,
    EXPR_LOGAND,
    EXPR_XOR,
    EXPR_EQUALITY,
    EXPR_INEQUALITY,
    EXPR_GTR,
    EXPR_LESS,
    EXPR_GTREQL,
    EXPR_LESSEQL,
    EXPR_LOGNOT,
    EXPR_POS,
    EXPR_STRLIT,
    EXPR_WSTRLIT,
    EXPR_CHARCONST,
};

enum type_kind
{
    TKIND_PRIMITIVE = -1,
    TKIND_ENUM,
    TKIND_RECORD,
    TKIND_MODULE,
    TKIND_INTERFACE,
    TKIND_DISPATCH,
    TKIND_COCLASS,
    TKIND_ALIAS,
    TKIND_UNION,
    TKIND_MAX
};

enum storage_class
{
    STG_NONE,
    STG_STATIC,
    STG_EXTERN,
    STG_REGISTER,
};

enum type_qualifier
{
    TYPE_QUALIFIER_CONST = 1,
};

enum function_specifier
{
    FUNCTION_SPECIFIER_INLINE = 1,
};

enum statement_type
{
    STMT_LIBRARY,
    STMT_DECLARATION,
    STMT_TYPE,
    STMT_TYPEREF,
    STMT_MODULE,
    STMT_TYPEDEF,
    STMT_IMPORT,
    STMT_IMPORTLIB,
    STMT_PRAGMA,
    STMT_CPPQUOTE
};

enum threading_type
{
    THREADING_APARTMENT = 1,
    THREADING_NEUTRAL,
    THREADING_SINGLE,
    THREADING_FREE,
    THREADING_BOTH
};

enum marshaling_type
{
    MARSHALING_INVALID = 0,
    MARSHALING_NONE,
    MARSHALING_AGILE,
    MARSHALING_STANDARD,
};

enum type_basic_type
{
    TYPE_BASIC_INT8 = 1,
    TYPE_BASIC_INT16,
    TYPE_BASIC_INT32,
    TYPE_BASIC_INT64,
    TYPE_BASIC_INT,
    TYPE_BASIC_INT3264,
    TYPE_BASIC_LONG,
    TYPE_BASIC_CHAR,
    TYPE_BASIC_HYPER,
    TYPE_BASIC_BYTE,
    TYPE_BASIC_WCHAR,
    TYPE_BASIC_FLOAT,
    TYPE_BASIC_DOUBLE,
    TYPE_BASIC_ERROR_STATUS_T,
    TYPE_BASIC_HANDLE,
};

#define TYPE_BASIC_MAX TYPE_BASIC_HANDLE
#define TYPE_BASIC_INT_MIN TYPE_BASIC_INT8
#define TYPE_BASIC_INT_MAX TYPE_BASIC_HYPER

struct location
{
    const char *input_name;
    int first_line;
    int last_line;
    int first_column;
    int last_column;
};

struct str_list_entry_t
{
    char *str;
    struct list entry;
};

struct _decl_spec_t
{
  type_t *type;
  enum storage_class stgclass;
  enum type_qualifier qualifier;
  enum function_specifier func_specifier;
};

struct _attr_t {
  enum attr_type type;
  union {
    unsigned int ival;
    void *pval;
  } u;
  /* parser-internal */
  struct list entry;
  struct location where;
};

struct integer
{
    int value;
    int is_unsigned;
    int is_long;
    int is_hex;
};

struct _expr_t {
  enum expr_type type;
  const expr_t *ref;
  union {
    struct integer integer;
    double dval;
    const char *sval;
    const expr_t *ext;
    decl_spec_t tref;
  } u;
  const expr_t *ext2;
  int is_const;
  int cval;
  /* parser-internal */
  struct list entry;
};

struct _attr_custdata_t {
  struct uuid id;
  expr_t *pval;
};

struct struct_details
{
  var_list_t *fields;
};

struct enumeration_details
{
  var_list_t *enums;
};

struct func_details
{
  var_list_t *args;
  struct _var_t *retval;
};

struct iface_details
{
  statement_list_t *stmts;
  var_list_t *disp_methods;
  var_list_t *disp_props;
  struct _type_t *inherit;
  struct _type_t *disp_inherit;
  struct _type_t *async_iface;
  typeref_list_t *requires;
};

struct module_details
{
  statement_list_t *stmts;
};

struct array_details
{
  expr_t *size_is;
  expr_t *length_is;
  struct _decl_spec_t elem;
  unsigned int dim;
  unsigned char declptr; /* if declared as a pointer */
  unsigned short ptr_tfsoff;  /* offset of pointer definition for declptr */
};

struct coclass_details
{
  typeref_list_t *ifaces;
};

struct basic_details
{
  enum type_basic_type type;
  int sign;
};

struct pointer_details
{
  struct _decl_spec_t ref;
};

struct bitfield_details
{
  struct _type_t *field;
  const expr_t *bits;
};

struct alias_details
{
    struct _decl_spec_t aliasee;
};

struct runtimeclass_details
{
    typeref_list_t *ifaces;
};

struct parameterized_details
{
    type_t *type;
    typeref_list_t *params;
};

struct delegate_details
{
    type_t *iface;
};

#define HASHMAX 64

struct namespace {
    const char *name;
    struct namespace *parent;
    struct list entry;
    struct list children;
    struct rtype *type_hash[HASHMAX];
};

enum type_type
{
    TYPE_VOID,
    TYPE_BASIC, /* ints, floats and handles */
    TYPE_ENUM,
    TYPE_STRUCT,
    TYPE_ENCAPSULATED_UNION,
    TYPE_UNION,
    TYPE_ALIAS,
    TYPE_MODULE,
    TYPE_COCLASS,
    TYPE_FUNCTION,
    TYPE_INTERFACE,
    TYPE_POINTER,
    TYPE_ARRAY,
    TYPE_BITFIELD,
    TYPE_APICONTRACT,
    TYPE_RUNTIMECLASS,
    TYPE_PARAMETERIZED_TYPE,
    TYPE_PARAMETER,
    TYPE_DELEGATE,
};

struct _type_t {
  const char *name;               /* C++ name with parameters in brackets */
  struct namespace *namespace;
  enum type_type type_type;
  attr_list_t *attrs;
  union
  {
    struct struct_details *structure;
    struct enumeration_details *enumeration;
    struct func_details *function;
    struct iface_details *iface;
    struct module_details *module;
    struct array_details array;
    struct coclass_details coclass;
    struct basic_details basic;
    struct pointer_details pointer;
    struct bitfield_details bitfield;
    struct alias_details alias;
    struct runtimeclass_details runtimeclass;
    struct parameterized_details parameterized;
    struct delegate_details delegate;
  } details;
  const char *c_name;             /* mangled C name, with namespaces and parameters */
  const char *signature;
  const char *qualified_name;     /* C++ fully qualified name */
  const char *impl_name;          /* C++ parameterized types impl base class name */
  const char *param_name;         /* used to build c_name of a parameterized type, when used as a parameter */
  const char *short_name;         /* widl specific short name */
  unsigned int typestring_offset;
  unsigned int ptrdesc;           /* used for complex structs */
  int typelib_idx;
  struct location where;
  unsigned int ignore : 1;
  unsigned int defined : 1;
  unsigned int defined_in_import : 1;
  unsigned int written : 1;
  unsigned int user_types_registered : 1;
  unsigned int tfswrite : 1;   /* if the type needs to be written to the TFS */
  unsigned int checked : 1;
};

struct _var_t {
  char *name;
  decl_spec_t declspec;
  attr_list_t *attrs;
  expr_t *eval;

  unsigned int typestring_offset;

  /* fields specific to functions */
  unsigned int procstring_offset, func_idx;

  struct location where;

  /* Should we define the UDT in this var, when writing a header? */
  unsigned int is_defined : 1;

  /* parser-internal */
  struct list entry;
};

struct _declarator_t {
  var_t *var;
  type_t *type;
  enum type_qualifier qualifier;
  expr_t *bits;

  /* parser-internal */
  struct list entry;
};

struct _typeref_t {
  type_t *type;
  attr_list_t *attrs;

  /* parser-internal */
  struct list entry;
};

struct _typelib_entry_t {
    type_t *type;
    struct list entry;
};

struct _importinfo_t {
    int offset;
    struct uuid guid;
    int flags;
    int id;

    char *name;

    importlib_t *importlib;
};

struct _importlib_t {
    int offset;
    char *name;

    int version;
    struct uuid guid;

    importinfo_t *importinfos;
    int ntypeinfos;

    int allocated;

    struct list entry;
};

struct _typelib_t {
    char *name;
    const attr_list_t *attrs;
    struct list importlibs;
    statement_list_t *stmts;

    type_t **reg_ifaces;
    unsigned int reg_iface_count;
};

struct _user_type_t {
    struct list entry;
    const char *name;
};

struct _statement_t {
    struct list entry;
    enum statement_type type;
    union
    {
        type_t *type;
        const char *str;
        var_t *var;
        typelib_t *lib;
        typeref_list_t *type_list;
    } u;
    /* For STMT_TYPE and STMT_TYPEDEF, should we define the UDT in this
     * statement, when writing a header? */
    unsigned int is_defined : 1;
};

struct _warning_t {
    int num;
    struct list entry;
};

typedef enum {
    SYS_WIN16,
    SYS_WIN32,
    SYS_MAC,
    SYS_WIN64
} syskind_t;

extern user_type_list_t user_type_list;
extern context_handle_list_t context_handle_list;
extern generic_handle_list_t generic_handle_list;
void check_for_additional_prototype_types(type_t *type);

void init_types(void);
type_t *alloc_type(void);
void set_all_tfswrite(int val);
void clear_all_offsets(void);

#define tsENUM   1
#define tsSTRUCT 2
#define tsUNION  3

var_t *find_const(const char *name, int f);
type_t *find_type(const char *name, struct namespace *namespace, int t);
type_t *make_type(enum type_type type);
type_t *get_type(enum type_type type, char *name, struct namespace *namespace, int t);
type_t *reg_type(type_t *type, const char *name, struct namespace *namespace, int t);

var_t *make_var(char *name);
var_list_t *append_var(var_list_t *list, var_t *var);

char *format_namespace(struct namespace *namespace, const char *prefix, const char *separator, const char *suffix,
                       const char *abi_prefix);
char *format_parameterized_type_name(type_t *type, typeref_list_t *params);

static inline enum type_type type_get_type_detect_alias(const type_t *type)
{
    return type->type_type;
}

#define STATEMENTS_FOR_EACH_FUNC(stmt, stmts) \
  if (stmts) LIST_FOR_EACH_ENTRY( stmt, stmts, statement_t, entry ) \
    if (stmt->type == STMT_DECLARATION && stmt->u.var->declspec.stgclass == STG_NONE && \
        type_get_type_detect_alias(stmt->u.var->declspec.type) == TYPE_FUNCTION)

static inline int statements_has_func(const statement_list_t *stmts)
{
  const statement_t *stmt;
  int has_func = 0;
  STATEMENTS_FOR_EACH_FUNC(stmt, stmts)
  {
    has_func = 1;
    break;
  }
  return has_func;
}

static inline int is_global_namespace(const struct namespace *namespace)
{
    return !namespace->name;
}

static inline decl_spec_t *init_declspec(decl_spec_t *declspec, type_t *type)
{
  declspec->type = type;
  declspec->stgclass = STG_NONE;
  declspec->qualifier = 0;
  declspec->func_specifier = 0;

  return declspec;
}

#endif
