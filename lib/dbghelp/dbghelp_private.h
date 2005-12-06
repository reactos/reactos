/*
 * File dbghelp_private.h - dbghelp internal definitions
 *
 * Copyright (C) 1995, Alexandre Julliard
 * Copyright (C) 1996, Eric Youngdale.
 * Copyright (C) 1999-2000, Ulrich Weigand.
 * Copyright (C) 2004, Eric Pouech.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define WIN32_NO_STATUS
#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winver.h"
#include "dbghelp.h"
#include "objbase.h"
#include "oaidl.h"

#include "cvconst.h"

/* #define USE_STATS */

struct pool /* poor's man */
{
    struct pool_arena*  first;
    unsigned            arena_size;
};

void     pool_init(struct pool* a, unsigned arena_size);
void     pool_destroy(struct pool* a);
void*    pool_alloc(struct pool* a, unsigned len);
/* void*    pool_realloc(struct pool* a, void* p,
   unsigned old_size, unsigned new_size); */
char*    pool_strdup(struct pool* a, const char* str);

struct vector
{
    void**      buckets;
    unsigned    elt_size;
    unsigned    shift;
    unsigned    num_elts;
    unsigned    num_buckets;
};

void     vector_init(struct vector* v, unsigned elt_sz, unsigned bucket_sz);
unsigned vector_length(const struct vector* v);
void*    vector_at(const struct vector* v, unsigned pos);
void*    vector_add(struct vector* v, struct pool* pool);
/*void     vector_pool_normalize(struct vector* v, struct pool* pool); */
void*    vector_iter_up(const struct vector* v, void* elt);
void*    vector_iter_down(const struct vector* v, void* elt);

struct hash_table_elt
{
    const char*                 name;
    struct hash_table_elt*      next;
};

struct hash_table
{
    unsigned                    num_buckets;
    struct hash_table_elt**     buckets;
};

void     hash_table_init(struct pool* pool, struct hash_table* ht,
                         unsigned num_buckets);
void     hash_table_destroy(struct hash_table* ht);
void     hash_table_add(struct hash_table* ht, struct hash_table_elt* elt);
void*    hash_table_find(const struct hash_table* ht, const char* name);
unsigned hash_table_hash(const char* name, unsigned num_buckets);

struct hash_table_iter
{
    const struct hash_table*    ht;
    struct hash_table_elt*      element;
    int                         index;
    int                         last;
};

void     hash_table_iter_init(const struct hash_table* ht,
                              struct hash_table_iter* hti, const char* name);
void*    hash_table_iter_up(struct hash_table_iter* hti);

#define GET_ENTRY(__i, __t, __f) \
    ((__t*)((char*)(__i) - (unsigned int)(&((__t*)0)->__f)))


extern unsigned dbghelp_options;
/* some more Wine extensions */
#define SYMOPT_WINE_WITH_ELF_MODULES 0x40000000

struct symt
{
    enum SymTagEnum             tag;
};

struct symt_ht
{
    struct symt                 symt;
    struct hash_table_elt       hash_elt;        /* if global symbol or type */
};

/* lexical tree */
struct symt_block
{
    struct symt                 symt;
    unsigned long               address;
    unsigned long               size;
    struct symt*                container;      /* block, or func */
    struct vector               vchildren;      /* sub-blocks & local variables */
};

struct symt_compiland
{
    struct symt                 symt;
    unsigned                    source;
    struct vector               vchildren;      /* global variables & functions */
};

struct symt_data
{
    struct symt                 symt;
    struct hash_table_elt       hash_elt;       /* if global symbol */
    enum DataKind               kind;
    struct symt*                container;
    struct symt*                type;
    union                                       /* depends on kind */
    {
        unsigned long           address;        /* DataIs{Global, FileStatic} */
        struct
        {
            long                        offset; /* DataIs{Member,Local,Param} in bits*/
            unsigned long               length; /* DataIs{Member} in bits */
            unsigned long               reg_id; /* DataIs{Local} (0 if frame relative) */
        } s;
        VARIANT                 value;          /* DataIsConstant */
    } u;
};

struct symt_function
{
    struct symt                 symt;
    struct hash_table_elt       hash_elt;       /* if global symbol */
    unsigned long               address;
    struct symt*                container;      /* compiland */
    struct symt*                type;           /* points to function_signature */
    unsigned long               size;
    struct vector               vlines;
    struct vector               vchildren;      /* locals, params, blocks, start/end, labels */
};

struct symt_function_point
{
    struct symt                 symt;           /* either SymTagFunctionDebugStart, SymTagFunctionDebugEnd, SymTagLabel */
    struct symt_function*       parent;
    unsigned long               offset;
    const char*                 name;           /* for labels */
};

struct symt_public
{
    struct symt                 symt;
    struct hash_table_elt       hash_elt;
    struct symt*                container;      /* compiland */
    unsigned long               address;
    unsigned long               size;
    unsigned                    in_code : 1,
                                is_function : 1;
};

struct symt_thunk
{
    struct symt                 symt;
    struct hash_table_elt       hash_elt;
    struct symt*                container;      /* compiland */
    unsigned long               address;
    unsigned long               size;
    THUNK_ORDINAL               ordinal;        /* FIXME: doesn't seem to be accessible */
};

/* class tree */
struct symt_array
{
    struct symt                 symt;
    int		                start;
    int		                end;
    struct symt*                basetype;
};

struct symt_basic
{
    struct symt                 symt;
    struct hash_table_elt       hash_elt;
    enum BasicType              bt;
    unsigned long               size;
};

struct symt_enum
{
    struct symt                 symt;
    const char*                 name;
    struct vector               vchildren;
};

struct symt_function_signature
{
    struct symt                 symt;
    struct symt*                rettype;
    struct vector               vchildren;
};

struct symt_pointer
{
    struct symt                 symt;
    struct symt*                pointsto;
};

struct symt_typedef
{
    struct symt                 symt;
    struct hash_table_elt       hash_elt;
    struct symt*                type;
};

struct symt_udt
{
    struct symt                 symt;
    struct hash_table_elt       hash_elt;
    enum UdtKind                kind;
    int		                size;
    struct vector               vchildren;
};

enum module_type
{
    DMT_UNKNOWN,        /* for lookup, not actually used for a module */
    DMT_ELF,            /* a real ELF shared module */
    DMT_PE,             /* a native or builtin PE module */
};

struct module
{
    IMAGEHLP_MODULE             module;
    struct module*              next;
    enum module_type		type;
    struct elf_module_info*	elf_info;
    
    /* memory allocation pool */
    struct pool                 pool;

    /* symbol tables */
    int                         sortlist_valid;
    struct symt_ht**            addr_sorttab;
    struct hash_table           ht_symbols;

    /* types */
    struct hash_table           ht_types;
    struct vector               vtypes;

    /* source files */
    unsigned                    sources_used;
    unsigned                    sources_alloc;
    char*                       sources;
};

struct process 
{
    struct process*             next;
    HANDLE                      handle;
    char*                       search_path;

    struct module*              lmodules;
    unsigned long               dbg_hdr_addr;

    IMAGEHLP_STACK_FRAME        ctx_frame;
};

/* dbghelp.c */
extern struct process* process_find_by_handle(HANDLE hProcess);
extern HANDLE hMsvcrt;

/* elf_module.c */
extern BOOL         elf_load_debug_info(struct module* module);
extern struct module*
                    elf_load_module(struct process* pcs, const char* name);
extern BOOL         elf_read_wine_loader_dbg_info(struct process* pcs);
extern BOOL         elf_synchronize_module_list(struct process* pcs);

extern DWORD WINAPI addr_to_linear(HANDLE hProcess, HANDLE hThread, ADDRESS* addr);

/* module.c */
extern struct module*
                    module_find_by_addr(const struct process* pcs, unsigned long addr,
                                        enum module_type type);
extern struct module*
                    module_find_by_name(const struct process* pcs, 
                                        const char* name, enum module_type type);
extern struct module*
                    module_get_debug(const struct process* pcs, struct module*);
extern struct module*
                    module_new(struct process* pcs, const char* name, 
                               enum module_type type, unsigned long addr, 
                               unsigned long size, unsigned long stamp, 
                               unsigned long checksum);
extern struct module*
                    module_get_container(const struct process* pcs,
                                         const struct module* inner);
extern struct module*
                    module_get_containee(const struct process* pcs,
                                         const struct module* inner);
extern void         module_reset_debug_info(struct module* module);
extern BOOL         module_remove(struct process* pcs, 
                                  struct module* module);
/* msc.c */
extern BOOL         pe_load_debug_directory(const struct process* pcs, 
                                            struct module* module, 
                                            const BYTE* mapping,
                                            const IMAGE_SECTION_HEADER* sectp, DWORD nsect,
                                            const IMAGE_DEBUG_DIRECTORY* dbg, int nDbg);
/* pe_module.c */
extern struct module*
                    pe_load_module(struct process* pcs, char* name,
                                   HANDLE hFile, DWORD base, DWORD size);
extern struct module*
                    pe_load_module_from_pcs(struct process* pcs, const char* name, 
                                            const char* mod_name, DWORD base, DWORD size);
extern BOOL         pe_load_debug_info(const struct process* pcs, 
                                       struct module* module);
/* source.c */
extern unsigned     source_new(struct module* module, const char* source);
extern const char*  source_get(const struct module* module, unsigned idx);

/* stabs.c */
extern BOOL         stabs_parse(struct module* module, unsigned long load_offset,
                                const void* stabs, int stablen,
                                const char* strs, int strtablen);

/* symbol.c */
extern const char*  symt_get_name(const struct symt* sym);
extern int          symt_cmp_addr(const void* p1, const void* p2);
extern int          symt_find_nearest(struct module* module, DWORD addr);
extern struct symt_compiland*
                    symt_new_compiland(struct module* module, 
                                       const char* filename);
extern struct symt_public*
                    symt_new_public(struct module* module, 
                                    struct symt_compiland* parent, 
                                    const char* typename,
                                    unsigned long address, unsigned size,
                                    BOOL in_code, BOOL is_func);
extern struct symt_data*
                    symt_new_global_variable(struct module* module, 
                                             struct symt_compiland* parent,
                                             const char* name, unsigned is_static,
                                             unsigned long addr, unsigned long size, 
                                             struct symt* type);
extern struct symt_function*
                    symt_new_function(struct module* module,
                                      struct symt_compiland* parent,
                                      const char* name,
                                      unsigned long addr, unsigned long size,
                                      struct symt* type);
extern BOOL         symt_normalize_function(struct module* module, 
                                            struct symt_function* func);
extern void         symt_add_func_line(struct module* module,
                                       struct symt_function* func, 
                                       unsigned source_idx, int line_num, 
                                       unsigned long offset);
extern struct symt_data*
                    symt_add_func_local(struct module* module, 
                                        struct symt_function* func, 
                                        int regno, int offset,
                                        struct symt_block* block,
                                        struct symt* type, const char* name);
extern struct symt_block*
                    symt_open_func_block(struct module* module, 
                                         struct symt_function* func,
                                         struct symt_block* block, 
                                         unsigned pc, unsigned len);
extern struct symt_block*
                    symt_close_func_block(struct module* module, 
                                          struct symt_function* func,
                                          struct symt_block* block, unsigned pc);
extern struct symt_function_point*
                    symt_add_function_point(struct module* module, 
                                            struct symt_function* func,
                                            enum SymTagEnum point, 
                                            unsigned offset, const char* name);
extern BOOL         symt_fill_func_line_info(struct module* module,
                                             struct symt_function* func, 
                                             DWORD addr, IMAGEHLP_LINE* line);
extern BOOL         symt_get_func_line_next(struct module* module, PIMAGEHLP_LINE line);
extern struct symt_thunk*
                    symt_new_thunk(struct module* module, 
                                   struct symt_compiland* parent,
                                   const char* name, THUNK_ORDINAL ord,
                                   unsigned long addr, unsigned long size);

/* type.c */
extern void         symt_init_basic(struct module* module);
extern BOOL         symt_get_info(const struct symt* type,
                                  IMAGEHLP_SYMBOL_TYPE_INFO req, void* pInfo);
extern struct symt_basic*
                    symt_new_basic(struct module* module, enum BasicType, 
                                   const char* typename, unsigned size);
extern struct symt_udt*
                    symt_new_udt(struct module* module, const char* typename,
                                 unsigned size, enum UdtKind kind);
extern BOOL         symt_set_udt_size(struct module* module,
                                      struct symt_udt* type, unsigned size);
extern BOOL         symt_add_udt_element(struct module* module, 
                                         struct symt_udt* udt_type, 
                                         const char* name,
                                         struct symt* elt_type, unsigned offset, 
                                         unsigned size);
extern struct symt_enum*
                    symt_new_enum(struct module* module, const char* typename);
extern BOOL         symt_add_enum_element(struct module* module, 
                                          struct symt_enum* enum_type, 
                                          const char* name, int value);
extern struct symt_array*
                    symt_new_array(struct module* module, int min, int max, 
                                   struct symt* base);
extern struct symt_function_signature*
                    symt_new_function_signature(struct module* module, 
                                                struct symt* ret_type);
extern BOOL         symt_add_function_signature_parameter(struct module* module,
                                                          struct symt_function_signature* sig,
                                                          struct symt* param);
extern struct symt_pointer*
                    symt_new_pointer(struct module* module, 
                                     struct symt* ref_type);
extern struct symt_typedef*
                    symt_new_typedef(struct module* module, struct symt* ref, 
                                     const char* name);
