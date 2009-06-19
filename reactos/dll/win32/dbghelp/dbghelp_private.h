/*
 * File dbghelp_private.h - dbghelp internal definitions
 *
 * Copyright (C) 1995, Alexandre Julliard
 * Copyright (C) 1996, Eric Youngdale.
 * Copyright (C) 1999-2000, Ulrich Weigand.
 * Copyright (C) 2004-2007, Eric Pouech.
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

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winver.h"
#include "dbghelp.h"
#include "objbase.h"
#include "oaidl.h"
#include "winnls.h"
#include "wine/unicode.h"

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
char*    pool_strdup(struct pool* a, const char* str);

struct vector
{
    void**      buckets;
    unsigned    elt_size;
    unsigned    shift;
    unsigned    num_elts;
    unsigned    num_buckets;
    unsigned    buckets_allocated;
};

void     vector_init(struct vector* v, unsigned elt_sz, unsigned bucket_sz);
unsigned vector_length(const struct vector* v);
void*    vector_at(const struct vector* v, unsigned pos);
void*    vector_add(struct vector* v, struct pool* pool);

struct sparse_array
{
    struct vector               key2index;
    struct vector               elements;
};

void     sparse_array_init(struct sparse_array* sa, unsigned elt_sz, unsigned bucket_sz);
void*    sparse_array_find(const struct sparse_array* sa, unsigned long idx);
void*    sparse_array_add(struct sparse_array* sa, unsigned long key, struct pool* pool);
unsigned sparse_array_length(const struct sparse_array* sa);

struct hash_table_elt
{
    const char*                 name;
    struct hash_table_elt*      next;
};

struct hash_table_bucket
{
    struct hash_table_elt*      first;
    struct hash_table_elt*      last;
};

struct hash_table
{
    unsigned                    num_elts;
    unsigned                    num_buckets;
    struct hash_table_bucket*   buckets;
    struct pool*                pool;
};

void     hash_table_init(struct pool* pool, struct hash_table* ht,
                         unsigned num_buckets);
void     hash_table_destroy(struct hash_table* ht);
void     hash_table_add(struct hash_table* ht, struct hash_table_elt* elt);
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
    ((__t*)((char*)(__i) - FIELD_OFFSET(__t,__f)))


extern unsigned dbghelp_options;
/* some more Wine extensions */
#define SYMOPT_WINE_WITH_ELF_MODULES 0x40000000

enum location_kind {loc_error,          /* reg is the error code */
                    loc_absolute,       /* offset is the location */
                    loc_register,       /* reg is the location */
                    loc_regrel,         /* [reg+offset] is the location */
                    loc_user,           /* value is debug information dependent,
                                           reg & offset can be used ad libidem */
};

enum location_error {loc_err_internal = -1,     /* internal while computing */
                     loc_err_too_complex = -2,  /* couldn't compute location (even at runtime) */
                     loc_err_out_of_scope = -3, /* variable isn't available at current address */
                     loc_err_cant_read = -4,    /* couldn't read memory at given address */
};

struct location
{
    unsigned            kind : 8,
                        reg;
    unsigned long       offset;
};

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
    unsigned long               address;
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
        /* DataIs{Global, FileStatic}:
         *      loc.kind is loc_absolute
         *      loc.offset is address
         * DataIs{Local,Param}:
         *      with loc.kind
         *              loc_absolute    not supported
         *              loc_register    location is in register loc.reg
         *              loc_regrel      location is at address loc.reg + loc.offset
         *              >= loc_user     ask debug info provider for resolution
         */
        struct location         var;
        /* DataIs{Member} (all values are in bits, not bytes) */
        struct
        {
            long                        offset;
            unsigned long               length;
        } member;
        /* DataIsConstant */
        VARIANT                 value;
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

struct symt_hierarchy_point
{
    struct symt                 symt;           /* either SymTagFunctionDebugStart, SymTagFunctionDebugEnd, SymTagLabel */
    struct hash_table_elt       hash_elt;       /* if label (and in compiland's hash table if global) */
    struct symt*                parent;         /* symt_function or symt_compiland */
    struct location             loc;
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
    struct symt*                base_type;
    struct symt*                index_type;
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
    struct symt*                base_type;
    const char*                 name;
    struct vector               vchildren;
};

struct symt_function_signature
{
    struct symt                 symt;
    struct symt*                rettype;
    struct vector               vchildren;
    enum CV_call_e              call_conv;
};

struct symt_function_arg_type
{
    struct symt                 symt;
    struct symt*                arg_type;
    struct symt*                container;
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
    DMT_PDB,            /* .PDB file */
    DMT_DBG,            /* .DBG file */
};

struct process;

struct module
{
    IMAGEHLP_MODULEW64          module;
    /* ANSI copy of module.ModuleName for efficiency */
    char                        module_name[MAX_PATH];
    struct module*              next;
    enum module_type		type : 16;
    unsigned short              is_virtual : 1;

    /* specific information for debug types */
    struct elf_module_info*	elf_info;
    struct dwarf2_module_info_s*dwarf2_info;

    /* memory allocation pool */
    struct pool                 pool;

    /* symbols & symbol tables */
    int                         sortlist_valid;
    unsigned                    num_sorttab;    /* number of symbols with addresses */
    struct symt_ht**            addr_sorttab;
    struct hash_table           ht_symbols;
    void                        (*loc_compute)(struct process* pcs,
                                               const struct module* module,
                                               const struct symt_function* func,
                                               struct location* loc);

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
    WCHAR*                      search_path;
    
    PSYMBOL_REGISTERED_CALLBACK64       reg_cb;
    BOOL                        reg_is_unicode;
    DWORD64                     reg_user;

    struct module*              lmodules;
    unsigned long               dbg_hdr_addr;

    IMAGEHLP_STACK_FRAME        ctx_frame;

    unsigned                    buffer_size;
    void*                       buffer;
};

struct line_info
{
    unsigned long               is_first : 1,
                                is_last : 1,
                                is_source_file : 1,
                                line_number;
    union
    {
        unsigned long               pc_offset;   /* if is_source_file isn't set */
        unsigned                    source_file; /* if is_source_file is set */
    } u;
};

struct module_pair
{
    struct process*             pcs;
    struct module*              requested; /* in:  to module_get_debug() */
    struct module*              effective; /* out: module with debug info */
};

enum pdb_kind {PDB_JG, PDB_DS};

struct pdb_lookup
{
    const char*                 filename;
    DWORD                       age;
    enum pdb_kind               kind;
    union
    {
        struct
        {
            DWORD               timestamp;
            struct PDB_JG_TOC*  toc;
        } jg;
        struct
        {
            GUID                guid;
            struct PDB_DS_TOC*  toc;
        } ds;
    } u;
};

/* dbghelp.c */
extern struct process* process_find_by_handle(HANDLE hProcess);
extern HANDLE hMsvcrt;
extern BOOL         validate_addr64(DWORD64 addr);
extern BOOL         pcs_callback(const struct process* pcs, ULONG action, void* data);
extern void*        fetch_buffer(struct process* pcs, unsigned size);

/* elf_module.c */
#define ELF_NO_MAP      ((const void*)0xffffffff)
typedef BOOL (*elf_enum_modules_cb)(const WCHAR*, unsigned long addr, void* user);
extern BOOL         elf_enum_modules(HANDLE hProc, elf_enum_modules_cb, void*);
extern BOOL         elf_fetch_file_info(const WCHAR* name, DWORD* base, DWORD* size, DWORD* checksum);
struct elf_file_map;
extern BOOL         elf_load_debug_info(struct module* module, struct elf_file_map* fmap);
extern struct module*
                    elf_load_module(struct process* pcs, const WCHAR* name, unsigned long);
extern BOOL         elf_read_wine_loader_dbg_info(struct process* pcs);
extern BOOL         elf_synchronize_module_list(struct process* pcs);
struct elf_thunk_area;
extern int          elf_is_in_thunk_area(unsigned long addr, const struct elf_thunk_area* thunks);
extern DWORD WINAPI addr_to_linear(HANDLE hProcess, HANDLE hThread, ADDRESS* addr);

/* module.c */
extern const WCHAR      S_ElfW[];
extern const WCHAR      S_WineLoaderW[];
extern const WCHAR      S_WinePThreadW[];
extern const WCHAR      S_WineKThreadW[];
extern const WCHAR      S_SlashW[];

extern struct module*
                    module_find_by_addr(const struct process* pcs, unsigned long addr,
                                        enum module_type type);
extern struct module*
                    module_find_by_name(const struct process* pcs,
                                        const WCHAR* name);
extern struct module*
                    module_find_by_nameA(const struct process* pcs,
                                         const char* name);
extern struct module*
                    module_is_already_loaded(const struct process* pcs,
                                             const WCHAR* imgname);
extern BOOL         module_get_debug(struct module_pair*);
extern struct module*
                    module_new(struct process* pcs, const WCHAR* name,
                               enum module_type type, BOOL virtual,
                               unsigned long addr, unsigned long size,
                               unsigned long stamp, unsigned long checksum);
extern struct module*
                    module_get_container(const struct process* pcs,
                                         const struct module* inner);
extern struct module*
                    module_get_containee(const struct process* pcs,
                                         const struct module* inner);
extern enum module_type
                    module_get_type_by_name(const WCHAR* name);
extern void         module_reset_debug_info(struct module* module);
extern BOOL         module_remove(struct process* pcs,
                                  struct module* module);
extern void         module_set_module(struct module* module, const WCHAR* name);

/* msc.c */
extern BOOL         pe_load_debug_directory(const struct process* pcs,
                                            struct module* module, 
                                            const BYTE* mapping,
                                            const IMAGE_SECTION_HEADER* sectp, DWORD nsect,
                                            const IMAGE_DEBUG_DIRECTORY* dbg, int nDbg);
extern BOOL         pdb_fetch_file_info(struct pdb_lookup* pdb_lookup);

/* path.c */
extern BOOL         path_find_symbol_file(const struct process* pcs, PCSTR full_path,
                                          const GUID* guid, DWORD dw1, DWORD dw2, PSTR buffer,
                                          BOOL* is_unmatched);

/* pe_module.c */
extern BOOL         pe_load_nt_header(HANDLE hProc, DWORD base, IMAGE_NT_HEADERS* nth);
extern struct module*
                    pe_load_native_module(struct process* pcs, const WCHAR* name,
                                          HANDLE hFile, DWORD base, DWORD size);
extern struct module*
                    pe_load_builtin_module(struct process* pcs, const WCHAR* name,
                                           DWORD base, DWORD size);
extern BOOL         pe_load_debug_info(const struct process* pcs,
                                       struct module* module);
/* source.c */
extern unsigned     source_new(struct module* module, const char* basedir, const char* source);
extern const char*  source_get(const struct module* module, unsigned idx);

/* stabs.c */
extern BOOL         stabs_parse(struct module* module, unsigned long load_offset,
                                const void* stabs, int stablen,
                                const char* strs, int strtablen);

/* dwarf.c */
extern BOOL         dwarf2_parse(struct module* module, unsigned long load_offset,
                                 const struct elf_thunk_area* thunks,
				 const unsigned char* debug, unsigned int debug_size, 
				 const unsigned char* abbrev, unsigned int abbrev_size, 
				 const unsigned char* str, unsigned int str_size,
                                 const unsigned char* line, unsigned int line_size,
                                 const unsigned char* loclist, unsigned int loclist_size);

/* symbol.c */
extern const char*  symt_get_name(const struct symt* sym);
extern int          symt_cmp_addr(const void* p1, const void* p2);
extern void         copy_symbolW(SYMBOL_INFOW* siw, const SYMBOL_INFO* si);
extern struct symt_ht*
                    symt_find_nearest(struct module* module, DWORD addr);
extern struct symt_compiland*
                    symt_new_compiland(struct module* module, unsigned long address,
                                       unsigned src_idx);
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
                                        enum DataKind dt, const struct location* loc,
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
extern struct symt_hierarchy_point*
                    symt_add_function_point(struct module* module, 
                                            struct symt_function* func,
                                            enum SymTagEnum point, 
                                            const struct location* loc,
                                            const char* name);
extern BOOL         symt_fill_func_line_info(const struct module* module,
                                             const struct symt_function* func,
                                             DWORD addr, IMAGEHLP_LINE* line);
extern BOOL         symt_get_func_line_next(const struct module* module, PIMAGEHLP_LINE line);
extern struct symt_thunk*
                    symt_new_thunk(struct module* module, 
                                   struct symt_compiland* parent,
                                   const char* name, THUNK_ORDINAL ord,
                                   unsigned long addr, unsigned long size);
extern struct symt_data*
                    symt_new_constant(struct module* module,
                                      struct symt_compiland* parent,
                                      const char* name, struct symt* type,
                                      const VARIANT* v);
extern struct symt_hierarchy_point*
                    symt_new_label(struct module* module,
                                   struct symt_compiland* compiland,
                                   const char* name, unsigned long address);

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
                    symt_new_enum(struct module* module, const char* typename,
                                  struct symt* basetype);
extern BOOL         symt_add_enum_element(struct module* module, 
                                          struct symt_enum* enum_type, 
                                          const char* name, int value);
extern struct symt_array*
                    symt_new_array(struct module* module, int min, int max, 
                                   struct symt* base, struct symt* index);
extern struct symt_function_signature*
                    symt_new_function_signature(struct module* module, 
                                                struct symt* ret_type,
                                                enum CV_call_e call_conv);
extern BOOL         symt_add_function_signature_parameter(struct module* module,
                                                          struct symt_function_signature* sig,
                                                          struct symt* param);
extern struct symt_pointer*
                    symt_new_pointer(struct module* module, 
                                     struct symt* ref_type);
extern struct symt_typedef*
                    symt_new_typedef(struct module* module, struct symt* ref, 
                                     const char* name);
