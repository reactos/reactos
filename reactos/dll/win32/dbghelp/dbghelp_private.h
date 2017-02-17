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

#ifndef _DBGHELP_PRIVATE_H_
#define _DBGHELP_PRIVATE_H_

#include <config.h>

#include <assert.h>
#include <stdio.h>

#ifdef HAVE_SYS_MMAN_H
# include <sys/mman.h>
#endif
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#ifndef DBGHELP_STATIC_LIB

#include <wine/port.h>

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winver.h>
#include <winternl.h>
#include <dbghelp.h>
#include <objbase.h>
#include <cvconst.h>
#include <psapi.h>

#include <wine/debug.h>
#include <wine/mscvpdb.h>
#include <wine/unicode.h>

#else /* DBGHELP_STATIC_LIB */

#include <string.h>
#include "compat.h"

#endif /* DBGHELP_STATIC_LIB */

#include <wine/list.h>
#include <wine/rbtree.h>

/* #define USE_STATS */

struct pool /* poor's man */
{
    struct list arena_list;
    struct list arena_full;
    size_t      arena_size;
};

void     pool_init(struct pool* a, size_t arena_size) DECLSPEC_HIDDEN;
void     pool_destroy(struct pool* a) DECLSPEC_HIDDEN;
void*    pool_alloc(struct pool* a, size_t len) DECLSPEC_HIDDEN;
char*    pool_strdup(struct pool* a, const char* str) DECLSPEC_HIDDEN;

struct vector
{
    void**      buckets;
    unsigned    elt_size;
    unsigned    shift;
    unsigned    num_elts;
    unsigned    num_buckets;
    unsigned    buckets_allocated;
};

void     vector_init(struct vector* v, unsigned elt_sz, unsigned bucket_sz) DECLSPEC_HIDDEN;
unsigned vector_length(const struct vector* v) DECLSPEC_HIDDEN;
void*    vector_at(const struct vector* v, unsigned pos) DECLSPEC_HIDDEN;
void*    vector_add(struct vector* v, struct pool* pool) DECLSPEC_HIDDEN;

struct sparse_array
{
    struct vector               key2index;
    struct vector               elements;
};

void     sparse_array_init(struct sparse_array* sa, unsigned elt_sz, unsigned bucket_sz) DECLSPEC_HIDDEN;
void*    sparse_array_find(const struct sparse_array* sa, unsigned long idx) DECLSPEC_HIDDEN;
void*    sparse_array_add(struct sparse_array* sa, unsigned long key, struct pool* pool) DECLSPEC_HIDDEN;
unsigned sparse_array_length(const struct sparse_array* sa) DECLSPEC_HIDDEN;

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
                         unsigned num_buckets) DECLSPEC_HIDDEN;
void     hash_table_destroy(struct hash_table* ht) DECLSPEC_HIDDEN;
void     hash_table_add(struct hash_table* ht, struct hash_table_elt* elt) DECLSPEC_HIDDEN;

struct hash_table_iter
{
    const struct hash_table*    ht;
    struct hash_table_elt*      element;
    int                         index;
    int                         last;
};

void     hash_table_iter_init(const struct hash_table* ht,
                              struct hash_table_iter* hti, const char* name) DECLSPEC_HIDDEN;
void*    hash_table_iter_up(struct hash_table_iter* hti) DECLSPEC_HIDDEN;


extern unsigned dbghelp_options DECLSPEC_HIDDEN;
/* some more Wine extensions */
#define SYMOPT_WINE_WITH_NATIVE_MODULES 0x40000000

enum location_kind {loc_error,          /* reg is the error code */
                    loc_unavailable,    /* location is not available */
                    loc_absolute,       /* offset is the location */
                    loc_register,       /* reg is the location */
                    loc_regrel,         /* [reg+offset] is the location */
                    loc_tlsrel,         /* offset is the address of the TLS index */
                    loc_user,           /* value is debug information dependent,
                                           reg & offset can be used ad libidem */
};

enum location_error {loc_err_internal = -1,     /* internal while computing */
                     loc_err_too_complex = -2,  /* couldn't compute location (even at runtime) */
                     loc_err_out_of_scope = -3, /* variable isn't available at current address */
                     loc_err_cant_read = -4,    /* couldn't read memory at given address */
                     loc_err_no_location = -5,  /* likely optimized away (by compiler) */
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
         *      with loc.kind
         *              loc_absolute    loc.offset is address
         *              loc_tlsrel      loc.offset is TLS index address
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
    int		                end;            /* end index if > 0, or -array_len (in bytes) if < 0 */
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
    unsigned long               size;
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
    DMT_MACHO,          /* a real Mach-O shared module */
    DMT_PDB,            /* .PDB file */
    DMT_DBG,            /* .DBG file */
};

struct process;
struct module;

/* a module can be made of several debug information formats, so we have to
 * support them all
 */
enum format_info
{
    DFI_ELF,
    DFI_PE,
    DFI_MACHO,
    DFI_DWARF,
    DFI_PDB,
    DFI_LAST
};

struct module_format
{
    struct module*              module;
    void                        (*remove)(struct process* pcs, struct module_format* modfmt);
    void                        (*loc_compute)(struct process* pcs,
                                               const struct module_format* modfmt,
                                               const struct symt_function* func,
                                               struct location* loc);
    union
    {
        struct elf_module_info*         elf_info;
        struct dwarf2_module_info_s*    dwarf2_info;
        struct pe_module_info*          pe_info;
        struct macho_module_info*	macho_info;
        struct pdb_module_info*         pdb_info;
    } u;
};

#ifdef __REACTOS__
struct symt_idx_to_ptr
{
    struct hash_table_elt hash_elt;
    DWORD idx;
    const struct symt *sym;
};
#endif

struct module
{
    struct process*             process;
    IMAGEHLP_MODULEW64          module;
    struct module*              next;
    enum module_type		type : 16;
    unsigned short              is_virtual : 1;
    DWORD64                     reloc_delta;

    /* specific information for debug types */
    struct module_format*       format_info[DFI_LAST];

    /* memory allocation pool */
    struct pool                 pool;

    /* symbols & symbol tables */
    struct vector               vsymt;
    int                         sortlist_valid;
    unsigned                    num_sorttab;    /* number of symbols with addresses */
    unsigned                    num_symbols;
    unsigned                    sorttab_size;
    struct symt_ht**            addr_sorttab;
    struct hash_table           ht_symbols;
#ifdef __x86_64__
    struct hash_table           ht_symaddr;
#endif

    /* types */
    struct hash_table           ht_types;
    struct vector               vtypes;

    /* source files */
    unsigned                    sources_used;
    unsigned                    sources_alloc;
    char*                       sources;
    struct wine_rb_tree         sources_offsets_tree;
};

struct process 
{
    struct process*             next;
    HANDLE                      handle;
    WCHAR*                      search_path;

    PSYMBOL_REGISTERED_CALLBACK64       reg_cb;
    PSYMBOL_REGISTERED_CALLBACK reg_cb32;
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
    enum pdb_kind               kind;
    DWORD                       age;
    DWORD                       timestamp;
    GUID                        guid;
};

struct cpu_stack_walk
{
    HANDLE                      hProcess;
    HANDLE                      hThread;
    BOOL                        is32;
    union
    {
        struct
        {
            PREAD_PROCESS_MEMORY_ROUTINE        f_read_mem;
            PTRANSLATE_ADDRESS_ROUTINE          f_xlat_adr;
            PFUNCTION_TABLE_ACCESS_ROUTINE      f_tabl_acs;
            PGET_MODULE_BASE_ROUTINE            f_modl_bas;
        } s32;
        struct
        {
            PREAD_PROCESS_MEMORY_ROUTINE64      f_read_mem;
            PTRANSLATE_ADDRESS_ROUTINE64        f_xlat_adr;
            PFUNCTION_TABLE_ACCESS_ROUTINE64    f_tabl_acs;
            PGET_MODULE_BASE_ROUTINE64          f_modl_bas;
        } s64;
    } u;
};

struct dump_memory
{
    ULONG64                             base;
    ULONG                               size;
    ULONG                               rva;
};

struct dump_module
{
    unsigned                            is_elf;
    ULONG64                             base;
    ULONG                               size;
    DWORD                               timestamp;
    DWORD                               checksum;
    WCHAR                               name[MAX_PATH];
};

struct dump_thread
{
    ULONG                               tid;
    ULONG                               prio_class;
    ULONG                               curr_prio;
};

struct dump_context
{
    /* process & thread information */
    HANDLE                              hProcess;
    DWORD                               pid;
    unsigned                            flags_out;
    /* thread information */
    struct dump_thread*                 threads;
    unsigned                            num_threads;
    /* module information */
    struct dump_module*                 modules;
    unsigned                            num_modules;
    unsigned                            alloc_modules;
    /* exception information */
    /* output information */
    MINIDUMP_TYPE                       type;
    HANDLE                              hFile;
    RVA                                 rva;
    struct dump_memory*                 mem;
    unsigned                            num_mem;
    unsigned                            alloc_mem;
    /* callback information */
    MINIDUMP_CALLBACK_INFORMATION*      cb;
};

enum cpu_addr {cpu_addr_pc, cpu_addr_stack, cpu_addr_frame};
struct cpu
{
    DWORD       machine;
    DWORD       word_size;
    DWORD       frame_regno;

    /* address manipulation */
    BOOL        (*get_addr)(HANDLE hThread, const CONTEXT* ctx,
                            enum cpu_addr, ADDRESS64* addr);

    /* stack manipulation */
    BOOL        (*stack_walk)(struct cpu_stack_walk* csw, LPSTACKFRAME64 frame, CONTEXT* context);

    /* module manipulation */
    void*       (*find_runtime_function)(struct module*, DWORD64 addr);

    /* dwarf dedicated information */
    unsigned    (*map_dwarf_register)(unsigned regno, BOOL eh_frame);

    /* context related manipulation */
    void*       (*fetch_context_reg)(CONTEXT* context, unsigned regno, unsigned* size);
    const char* (*fetch_regname)(unsigned regno);

    /* minidump per CPU extension */
    BOOL        (*fetch_minidump_thread)(struct dump_context* dc, unsigned index, unsigned flags, const CONTEXT* ctx);
    BOOL        (*fetch_minidump_module)(struct dump_context* dc, unsigned index, unsigned flags);
};

extern struct cpu*      dbghelp_current_cpu DECLSPEC_HIDDEN;

/* dbghelp.c */
extern struct process* process_find_by_handle(HANDLE hProcess) DECLSPEC_HIDDEN;
extern BOOL         validate_addr64(DWORD64 addr) DECLSPEC_HIDDEN;
extern BOOL         pcs_callback(const struct process* pcs, ULONG action, void* data) DECLSPEC_HIDDEN;
extern void*        fetch_buffer(struct process* pcs, unsigned size) DECLSPEC_HIDDEN;
extern const char*  wine_dbgstr_addr(const ADDRESS64* addr) DECLSPEC_HIDDEN;
extern struct cpu*  cpu_find(DWORD) DECLSPEC_HIDDEN;

/* crc32.c */
extern DWORD calc_crc32(int fd) DECLSPEC_HIDDEN;

typedef BOOL (*enum_modules_cb)(const WCHAR*, unsigned long addr, void* user);

/* elf_module.c */
extern BOOL         elf_enum_modules(HANDLE hProc, enum_modules_cb, void*) DECLSPEC_HIDDEN;
extern BOOL         elf_fetch_file_info(const WCHAR* name, DWORD_PTR* base, DWORD* size, DWORD* checksum) DECLSPEC_HIDDEN;
struct image_file_map;
extern BOOL         elf_load_debug_info(struct module* module) DECLSPEC_HIDDEN;
extern struct module*
                    elf_load_module(struct process* pcs, const WCHAR* name, unsigned long) DECLSPEC_HIDDEN;
extern BOOL         elf_read_wine_loader_dbg_info(struct process* pcs) DECLSPEC_HIDDEN;
extern BOOL         elf_synchronize_module_list(struct process* pcs) DECLSPEC_HIDDEN;
struct elf_thunk_area;
extern int          elf_is_in_thunk_area(unsigned long addr, const struct elf_thunk_area* thunks) DECLSPEC_HIDDEN;

/* macho_module.c */
extern BOOL         macho_enum_modules(HANDLE hProc, enum_modules_cb, void*) DECLSPEC_HIDDEN;
extern BOOL         macho_fetch_file_info(HANDLE process, const WCHAR* name, unsigned long load_addr, DWORD_PTR* base, DWORD* size, DWORD* checksum) DECLSPEC_HIDDEN;
extern BOOL         macho_load_debug_info(struct module* module) DECLSPEC_HIDDEN;
extern struct module*
                    macho_load_module(struct process* pcs, const WCHAR* name, unsigned long) DECLSPEC_HIDDEN;
extern BOOL         macho_read_wine_loader_dbg_info(struct process* pcs) DECLSPEC_HIDDEN;
extern BOOL         macho_synchronize_module_list(struct process* pcs) DECLSPEC_HIDDEN;

/* minidump.c */
void minidump_add_memory_block(struct dump_context* dc, ULONG64 base, ULONG size, ULONG rva) DECLSPEC_HIDDEN;

/* module.c */
extern const WCHAR      S_ElfW[] DECLSPEC_HIDDEN;
extern const WCHAR      S_WineLoaderW[] DECLSPEC_HIDDEN;
extern const WCHAR      S_SlashW[] DECLSPEC_HIDDEN;

extern struct module*
                    module_find_by_addr(const struct process* pcs, DWORD64 addr,
                                        enum module_type type) DECLSPEC_HIDDEN;
extern struct module*
                    module_find_by_nameW(const struct process* pcs,
                                         const WCHAR* name) DECLSPEC_HIDDEN;
extern struct module*
                    module_find_by_nameA(const struct process* pcs,
                                         const char* name) DECLSPEC_HIDDEN;
extern struct module*
                    module_is_already_loaded(const struct process* pcs,
                                             const WCHAR* imgname) DECLSPEC_HIDDEN;
extern BOOL         module_get_debug(struct module_pair*) DECLSPEC_HIDDEN;
extern struct module*
                    module_new(struct process* pcs, const WCHAR* name,
                               enum module_type type, BOOL virtual,
                               DWORD64 addr, DWORD64 size,
                               unsigned long stamp, unsigned long checksum) DECLSPEC_HIDDEN;
extern struct module*
                    module_get_containee(const struct process* pcs,
                                         const struct module* inner) DECLSPEC_HIDDEN;
extern enum module_type
                    module_get_type_by_name(const WCHAR* name) DECLSPEC_HIDDEN;
extern void         module_reset_debug_info(struct module* module) DECLSPEC_HIDDEN;
extern BOOL         module_remove(struct process* pcs,
                                  struct module* module) DECLSPEC_HIDDEN;
extern void         module_set_module(struct module* module, const WCHAR* name) DECLSPEC_HIDDEN;
extern const WCHAR *get_wine_loader_name(void) DECLSPEC_HIDDEN;

/* msc.c */
extern BOOL         pe_load_debug_directory(const struct process* pcs,
                                            struct module* module, 
                                            const BYTE* mapping,
                                            const IMAGE_SECTION_HEADER* sectp, DWORD nsect,
                                            const IMAGE_DEBUG_DIRECTORY* dbg, int nDbg) DECLSPEC_HIDDEN;
extern BOOL         pdb_fetch_file_info(const struct pdb_lookup* pdb_lookup, unsigned* matched) DECLSPEC_HIDDEN;
struct pdb_cmd_pair {
    const char*         name;
    DWORD*              pvalue;
};
extern BOOL         pdb_virtual_unwind(struct cpu_stack_walk* csw, DWORD_PTR ip,
                                       CONTEXT* context, struct pdb_cmd_pair* cpair) DECLSPEC_HIDDEN;

/* path.c */
extern BOOL         path_find_symbol_file(const struct process* pcs, PCSTR full_path,
                                          const GUID* guid, DWORD dw1, DWORD dw2, PSTR buffer,
                                          BOOL* is_unmatched) DECLSPEC_HIDDEN;

/* pe_module.c */
extern BOOL         pe_load_nt_header(HANDLE hProc, DWORD64 base, IMAGE_NT_HEADERS* nth) DECLSPEC_HIDDEN;
extern struct module*
                    pe_load_native_module(struct process* pcs, const WCHAR* name,
                                          HANDLE hFile, DWORD64 base, DWORD size) DECLSPEC_HIDDEN;
extern struct module*
                    pe_load_builtin_module(struct process* pcs, const WCHAR* name,
                                           DWORD64 base, DWORD64 size) DECLSPEC_HIDDEN;
extern BOOL         pe_load_debug_info(const struct process* pcs,
                                       struct module* module) DECLSPEC_HIDDEN;
extern const char*  pe_map_directory(struct module* module, int dirno, DWORD* size) DECLSPEC_HIDDEN;

/* source.c */
extern unsigned     source_new(struct module* module, const char* basedir, const char* source) DECLSPEC_HIDDEN;
extern const char*  source_get(const struct module* module, unsigned idx) DECLSPEC_HIDDEN;
extern int          source_rb_compare(const void *key, const struct wine_rb_entry *entry) DECLSPEC_HIDDEN;

/* stabs.c */
typedef void (*stabs_def_cb)(struct module* module, unsigned long load_offset,
                                const char* name, unsigned long offset,
                                BOOL is_public, BOOL is_global, unsigned char other,
                                struct symt_compiland* compiland, void* user);
extern BOOL         stabs_parse(struct module* module, unsigned long load_offset,
                                const void* stabs, int stablen,
                                const char* strs, int strtablen,
                                stabs_def_cb callback, void* user) DECLSPEC_HIDDEN;

/* dwarf.c */
extern BOOL         dwarf2_parse(struct module* module, unsigned long load_offset,
                                 const struct elf_thunk_area* thunks,
                                 struct image_file_map* fmap) DECLSPEC_HIDDEN;
extern BOOL         dwarf2_virtual_unwind(struct cpu_stack_walk* csw, DWORD_PTR ip,
                                          CONTEXT* context, ULONG_PTR* cfa) DECLSPEC_HIDDEN;

/* rsym.c */
extern BOOL         rsym_parse(struct module* module, unsigned long load_offset,
                                const void* rsym, int rsymlen) DECLSPEC_HIDDEN;



/* stack.c */
#ifndef DBGHELP_STATIC_LIB
extern BOOL         sw_read_mem(struct cpu_stack_walk* csw, DWORD64 addr, void* ptr, DWORD sz) DECLSPEC_HIDDEN;
#endif
extern DWORD64      sw_xlat_addr(struct cpu_stack_walk* csw, ADDRESS64* addr) DECLSPEC_HIDDEN;
extern void*        sw_table_access(struct cpu_stack_walk* csw, DWORD64 addr) DECLSPEC_HIDDEN;
extern DWORD64      sw_module_base(struct cpu_stack_walk* csw, DWORD64 addr) DECLSPEC_HIDDEN;

/* symbol.c */
extern const char*  symt_get_name(const struct symt* sym) DECLSPEC_HIDDEN;
extern WCHAR*       symt_get_nameW(const struct symt* sym) DECLSPEC_HIDDEN;
extern BOOL         symt_get_address(const struct symt* type, ULONG64* addr) DECLSPEC_HIDDEN;
extern int          symt_cmp_addr(const void* p1, const void* p2) DECLSPEC_HIDDEN;
extern void         copy_symbolW(SYMBOL_INFOW* siw, const SYMBOL_INFO* si) DECLSPEC_HIDDEN;
extern struct symt_ht*
                    symt_find_nearest(struct module* module, DWORD_PTR addr) DECLSPEC_HIDDEN;
extern struct symt_compiland*
                    symt_new_compiland(struct module* module, unsigned long address,
                                       unsigned src_idx) DECLSPEC_HIDDEN;
extern struct symt_public*
                    symt_new_public(struct module* module, 
                                    struct symt_compiland* parent, 
                                    const char* typename,
                                    unsigned long address, unsigned size) DECLSPEC_HIDDEN;
extern struct symt_data*
                    symt_new_global_variable(struct module* module, 
                                             struct symt_compiland* parent,
                                             const char* name, unsigned is_static,
                                             struct location loc, unsigned long size,
                                             struct symt* type) DECLSPEC_HIDDEN;
extern struct symt_function*
                    symt_new_function(struct module* module,
                                      struct symt_compiland* parent,
                                      const char* name,
                                      unsigned long addr, unsigned long size,
                                      struct symt* type) DECLSPEC_HIDDEN;
extern BOOL         symt_normalize_function(struct module* module, 
                                            const struct symt_function* func) DECLSPEC_HIDDEN;
extern void         symt_add_func_line(struct module* module,
                                       struct symt_function* func, 
                                       unsigned source_idx, int line_num, 
                                       unsigned long offset) DECLSPEC_HIDDEN;
extern struct symt_data*
                    symt_add_func_local(struct module* module, 
                                        struct symt_function* func, 
                                        enum DataKind dt, const struct location* loc,
                                        struct symt_block* block,
                                        struct symt* type, const char* name) DECLSPEC_HIDDEN;
extern struct symt_block*
                    symt_open_func_block(struct module* module, 
                                         struct symt_function* func,
                                         struct symt_block* block, 
                                         unsigned pc, unsigned len) DECLSPEC_HIDDEN;
extern struct symt_block*
                    symt_close_func_block(struct module* module, 
                                          const struct symt_function* func,
                                          struct symt_block* block, unsigned pc) DECLSPEC_HIDDEN;
extern struct symt_hierarchy_point*
                    symt_add_function_point(struct module* module, 
                                            struct symt_function* func,
                                            enum SymTagEnum point, 
                                            const struct location* loc,
                                            const char* name) DECLSPEC_HIDDEN;
extern BOOL         symt_fill_func_line_info(const struct module* module,
                                             const struct symt_function* func,
                                             DWORD64 addr, IMAGEHLP_LINE64* line) DECLSPEC_HIDDEN;
extern BOOL         symt_get_func_line_next(const struct module* module, PIMAGEHLP_LINE64 line) DECLSPEC_HIDDEN;
extern struct symt_thunk*
                    symt_new_thunk(struct module* module, 
                                   struct symt_compiland* parent,
                                   const char* name, THUNK_ORDINAL ord,
                                   unsigned long addr, unsigned long size) DECLSPEC_HIDDEN;
extern struct symt_data*
                    symt_new_constant(struct module* module,
                                      struct symt_compiland* parent,
                                      const char* name, struct symt* type,
                                      const VARIANT* v) DECLSPEC_HIDDEN;
extern struct symt_hierarchy_point*
                    symt_new_label(struct module* module,
                                   struct symt_compiland* compiland,
                                   const char* name, unsigned long address) DECLSPEC_HIDDEN;
extern struct symt* symt_index2ptr(struct module* module, DWORD id) DECLSPEC_HIDDEN;
extern DWORD        symt_ptr2index(struct module* module, const struct symt* sym) DECLSPEC_HIDDEN;

/* type.c */
extern void         symt_init_basic(struct module* module) DECLSPEC_HIDDEN;
extern BOOL         symt_get_info(struct module* module, const struct symt* type,
                                  IMAGEHLP_SYMBOL_TYPE_INFO req, void* pInfo) DECLSPEC_HIDDEN;
extern struct symt_basic*
                    symt_new_basic(struct module* module, enum BasicType, 
                                   const char* typename, unsigned size) DECLSPEC_HIDDEN;
extern struct symt_udt*
                    symt_new_udt(struct module* module, const char* typename,
                                 unsigned size, enum UdtKind kind) DECLSPEC_HIDDEN;
extern BOOL         symt_set_udt_size(struct module* module,
                                      struct symt_udt* type, unsigned size) DECLSPEC_HIDDEN;
extern BOOL         symt_add_udt_element(struct module* module, 
                                         struct symt_udt* udt_type, 
                                         const char* name,
                                         struct symt* elt_type, unsigned offset, 
                                         unsigned size) DECLSPEC_HIDDEN;
extern struct symt_enum*
                    symt_new_enum(struct module* module, const char* typename,
                                  struct symt* basetype) DECLSPEC_HIDDEN;
extern BOOL         symt_add_enum_element(struct module* module, 
                                          struct symt_enum* enum_type, 
                                          const char* name, int value) DECLSPEC_HIDDEN;
extern struct symt_array*
                    symt_new_array(struct module* module, int min, int max, 
                                   struct symt* base, struct symt* index) DECLSPEC_HIDDEN;
extern struct symt_function_signature*
                    symt_new_function_signature(struct module* module, 
                                                struct symt* ret_type,
                                                enum CV_call_e call_conv) DECLSPEC_HIDDEN;
extern BOOL         symt_add_function_signature_parameter(struct module* module,
                                                          struct symt_function_signature* sig,
                                                          struct symt* param) DECLSPEC_HIDDEN;
extern struct symt_pointer*
                    symt_new_pointer(struct module* module, 
                                     struct symt* ref_type,
                                     unsigned long size) DECLSPEC_HIDDEN;
extern struct symt_typedef*
                    symt_new_typedef(struct module* module, struct symt* ref, 
                                     const char* name) DECLSPEC_HIDDEN;

#include "image_private.h"

#endif /* _DBGHELP_PRIVATE_H_ */
