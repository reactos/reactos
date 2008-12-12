/*
 * File stabs.c - read stabs information from the modules
 *
 * Copyright (C) 1996,      Eric Youngdale.
 *		 1999-2005, Eric Pouech
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
 *
 *
 * Maintenance Information
 * -----------------------
 *
 * For documentation on the stabs format see for example
 *   The "stabs" debug format
 *     by Julia Menapace, Jim Kingdon, David Mackenzie
 *     of Cygnus Support
 *     available (hopefully) from http:\\sources.redhat.com\gdb\onlinedocs
 */

#include "config.h"
#include "wine/port.h"

#include <sys/types.h>
#include <fcntl.h>
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <stdio.h>
#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif
#include <assert.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"

#include "dbghelp_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dbghelp_stabs);

_CRTIMP UINT64 _strtoui64( const char *nptr, char **endptr, int base );
#define strtoull _strtoui64

#ifndef N_UNDF
#define N_UNDF		0x00
#endif

#define N_GSYM		0x20
#define N_FUN		0x24
#define N_STSYM		0x26
#define N_LCSYM		0x28
#define N_MAIN		0x2a
#define N_ROSYM		0x2c
#define N_BNSYM		0x2e
#define N_OPT		0x3c
#define N_RSYM		0x40
#define N_SLINE		0x44
#define N_ENSYM		0x4e
#define N_SO		0x64
#define N_LSYM		0x80
#define N_BINCL		0x82
#define N_SOL		0x84
#define N_PSYM		0xa0
#define N_EINCL		0xa2
#define N_LBRAC		0xc0
#define N_EXCL		0xc2
#define N_RBRAC		0xe0

struct stab_nlist
{
    union
    {
        char*                   n_name;
        struct stab_nlist*      n_next;
        long                    n_strx;
    } n_un;
    unsigned char       n_type;
    char                n_other;
    short               n_desc;
    unsigned long       n_value;
};

static void stab_strcpy(char* dest, int sz, const char* source)
{
    char*       ptr = dest;
    /*
     * A strcpy routine that stops when we hit the ':' character.
     * Faster than copying the whole thing, and then nuking the
     * ':'.
     * Takes also care of (valid) a::b constructs
     */
    while (*source != '\0')
    {
        if (source[0] != ':' && sz-- > 0) *ptr++ = *source++;
        else if (source[1] == ':' && (sz -= 2) > 0)
        {
            *ptr++ = *source++;
            *ptr++ = *source++;
        }
        else break;
    }
    *ptr-- = '\0';
    /* GCC emits, in some cases, a .<digit>+ suffix.
     * This is used for static variable inside functions, so
     * that we can have several such variables with same name in
     * the same compilation unit
     * We simply ignore that suffix when present (we also get rid
     * of it in ELF symtab parsing)
     */
    if (ptr >= dest && isdigit(*ptr))
    {
        while (ptr > dest && isdigit(*ptr)) ptr--;
        if (*ptr == '.') *ptr = '\0';
    }
    assert(sz > 0);
}

typedef struct
{
   char*		name;
   unsigned long	value;
   struct symt**        vector;
   int			nrofentries;
} include_def;

#define MAX_INCLUDES	5120

static include_def* 	        include_defs = NULL;
static int	     	        num_include_def = 0;
static int		        num_alloc_include_def = 0;
static int                      cu_include_stack[MAX_INCLUDES];
static int		        cu_include_stk_idx = 0;
static struct symt**            cu_vector = NULL;
static int 		        cu_nrofentries = 0;
static struct symt_basic*       stabs_basic[36];

static int stabs_new_include(const char* file, unsigned long val)
{
    if (num_include_def == num_alloc_include_def)
    {
        num_alloc_include_def += 256;
        if (!include_defs)
            include_defs = HeapAlloc(GetProcessHeap(), 0, 
                                     sizeof(include_defs[0]) * num_alloc_include_def);
        else
            include_defs = HeapReAlloc(GetProcessHeap(), 0, include_defs,
                                       sizeof(include_defs[0]) * num_alloc_include_def);
        memset(include_defs + num_include_def, 0, sizeof(include_defs[0]) * 256);
    }
    include_defs[num_include_def].name = strcpy(HeapAlloc(GetProcessHeap(), 0, strlen(file) + 1), file);
    include_defs[num_include_def].value = val;
    include_defs[num_include_def].vector = NULL;
    include_defs[num_include_def].nrofentries = 0;

    return num_include_def++;
}

static int stabs_find_include(const char* file, unsigned long val)
{
    int		i;

    for (i = 0; i < num_include_def; i++)
    {
        if (val == include_defs[i].value &&
            strcmp(file, include_defs[i].name) == 0)
            return i;
    }
    return -1;
}

static int stabs_add_include(int idx)
{
    if (idx < 0) return -1;
    cu_include_stk_idx++;

    /* if this happens, just bump MAX_INCLUDES */
    /* we could also handle this as another dynarray */
    assert(cu_include_stk_idx < MAX_INCLUDES);
    cu_include_stack[cu_include_stk_idx] = idx;
    return cu_include_stk_idx;
}

static void stabs_reset_includes(void)
{
    /*
     * The struct symt:s that we would need to use are reset when
     * we start a new file. (at least the ones in filenr == 0)
     */
    cu_include_stk_idx = 0;/* keep 0 as index for the .c file itself */
    memset(cu_vector, 0, sizeof(cu_vector[0]) * cu_nrofentries);
}

static void stabs_free_includes(void)
{
    int i;

    stabs_reset_includes();
    for (i = 0; i < num_include_def; i++)
    {
        HeapFree(GetProcessHeap(), 0, include_defs[i].name);
        HeapFree(GetProcessHeap(), 0, include_defs[i].vector);
    }
    HeapFree(GetProcessHeap(), 0, include_defs);
    include_defs = NULL;
    num_include_def = 0;
    num_alloc_include_def = 0;
    HeapFree(GetProcessHeap(), 0, cu_vector);
    cu_vector = NULL;
    cu_nrofentries = 0;
}

static struct symt** stabs_find_ref(long filenr, long subnr)
{
    struct symt**       ret;

    /* FIXME: I could perhaps create a dummy include_def for each compilation
     * unit which would allow not to handle those two cases separately
     */
    if (filenr == 0)
    {
        if (cu_nrofentries <= subnr)
	{
            if (!cu_vector)
                cu_vector = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 
                                      sizeof(cu_vector[0]) * (subnr+1));
            else
                cu_vector = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 
                                        cu_vector, sizeof(cu_vector[0]) * (subnr+1));
            cu_nrofentries = subnr + 1;
	}
        ret = &cu_vector[subnr];
    }
    else
    {
        include_def*	idef;

        assert(filenr <= cu_include_stk_idx);
        idef = &include_defs[cu_include_stack[filenr]];

        if (idef->nrofentries <= subnr)
	{
            if (!idef->vector)
                idef->vector = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 
                                         sizeof(idef->vector[0]) * (subnr+1));
            else
                idef->vector = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 
                                           idef->vector, sizeof(idef->vector[0]) * (subnr+1));
            idef->nrofentries = subnr + 1;
	}
        ret = &idef->vector[subnr];
    }
    TRACE("(%ld,%ld) => %p (%p)\n", filenr, subnr, ret, *ret);
    return ret;
}

static struct symt** stabs_read_type_enum(const char** x)
{
    long        filenr, subnr;
    const char* iter;
    char*       end;

    iter = *x;
    if (*iter == '(')
    {
        ++iter;                             /* '('   */
        filenr = strtol(iter, &end, 10);    /* <int> */
        iter = ++end;                       /* ','   */
        subnr = strtol(iter, &end, 10);     /* <int> */
        iter = ++end;                       /* ')'   */
    }
    else
    {
        filenr = 0;
        subnr = strtol(iter, &end, 10);     /* <int> */
        iter = end;
    }
    *x = iter;
    return stabs_find_ref(filenr, subnr);
}

#define PTS_DEBUG
struct ParseTypedefData
{
    const char*		ptr;
    char		buf[1024];
    int			idx;
    struct module*      module;
#ifdef PTS_DEBUG
    struct PTS_Error 
    {
        const char*         ptr;
        unsigned            line;
    } errors[16];
    int                 err_idx;
#endif
};

#ifdef PTS_DEBUG
static void stabs_pts_push(struct ParseTypedefData* ptd, unsigned line)
{
    assert(ptd->err_idx < sizeof(ptd->errors) / sizeof(ptd->errors[0]));
    ptd->errors[ptd->err_idx].line = line;
    ptd->errors[ptd->err_idx].ptr = ptd->ptr;
    ptd->err_idx++;
}
#define PTS_ABORTIF(ptd, t) do { if (t) { stabs_pts_push((ptd), __LINE__); return -1;} } while (0)
#else
#define PTS_ABORTIF(ptd, t) do { if (t) return -1; } while (0)
#endif

static int stabs_get_basic(struct ParseTypedefData* ptd, unsigned basic, struct symt** symt)
{
    PTS_ABORTIF(ptd, basic >= sizeof(stabs_basic) / sizeof(stabs_basic[0]));

    if (!stabs_basic[basic])
    {
        switch (basic)
        {
        case  1: stabs_basic[basic] = symt_new_basic(ptd->module, btInt,     "int", 4); break;
        case  2: stabs_basic[basic] = symt_new_basic(ptd->module, btChar,    "char", 1); break;
        case  3: stabs_basic[basic] = symt_new_basic(ptd->module, btInt,     "short int", 2); break;
        case  4: stabs_basic[basic] = symt_new_basic(ptd->module, btInt,     "long int", 4); break;
        case  5: stabs_basic[basic] = symt_new_basic(ptd->module, btUInt,    "unsigned char", 1); break;
        case  6: stabs_basic[basic] = symt_new_basic(ptd->module, btInt,     "signed char", 1); break;
        case  7: stabs_basic[basic] = symt_new_basic(ptd->module, btUInt,    "unsigned short int", 2); break;
        case  8: stabs_basic[basic] = symt_new_basic(ptd->module, btUInt,    "unsigned int", 4); break;
        case  9: stabs_basic[basic] = symt_new_basic(ptd->module, btUInt,    "unsigned", 2); break;
        case 10: stabs_basic[basic] = symt_new_basic(ptd->module, btUInt,    "unsigned long int", 2); break;
        case 11: stabs_basic[basic] = symt_new_basic(ptd->module, btVoid,    "void", 0); break;
        case 12: stabs_basic[basic] = symt_new_basic(ptd->module, btFloat,   "float", 4); break;
        case 13: stabs_basic[basic] = symt_new_basic(ptd->module, btFloat,   "double", 8); break;
        case 14: stabs_basic[basic] = symt_new_basic(ptd->module, btFloat,   "long double", 12); break;
        case 15: stabs_basic[basic] = symt_new_basic(ptd->module, btInt,     "integer", 4); break;
        case 16: stabs_basic[basic] = symt_new_basic(ptd->module, btBool,    "bool", 1); break;
        /*    case 17: short real */
        /*    case 18: real */
        case 25: stabs_basic[basic] = symt_new_basic(ptd->module, btComplex, "float complex", 8); break;
        case 26: stabs_basic[basic] = symt_new_basic(ptd->module, btComplex, "double complex", 16); break;
        case 30: stabs_basic[basic] = symt_new_basic(ptd->module, btWChar,   "wchar_t", 2); break;
        case 31: stabs_basic[basic] = symt_new_basic(ptd->module, btInt,     "long long int", 8); break;
        case 32: stabs_basic[basic] = symt_new_basic(ptd->module, btUInt,    "long long unsigned", 8); break;
            /* starting at 35 are wine extensions (especially for R implementation) */
        case 35: stabs_basic[basic] = symt_new_basic(ptd->module, btComplex, "long double complex", 24); break;
        default: PTS_ABORTIF(ptd, 1);
        }
    }   
    *symt = &stabs_basic[basic]->symt;
    return 0;
}

static int stabs_pts_read_type_def(struct ParseTypedefData* ptd, 
                                   const char* typename, struct symt** dt);

static int stabs_pts_read_id(struct ParseTypedefData* ptd)
{
    const char*	        first = ptd->ptr;
    unsigned int	template = 0;
    char                ch;

    while ((ch = *ptd->ptr++) != '\0')
    {
        switch (ch)
        {
        case ':':
            if (template == 0)
            {
                unsigned int len = ptd->ptr - first - 1;
                PTS_ABORTIF(ptd, len >= sizeof(ptd->buf) - ptd->idx);
                memcpy(ptd->buf + ptd->idx, first, len);
                ptd->buf[ptd->idx + len] = '\0';
                ptd->idx += len + 1;
                return 0;
            }
            break;
        case '<': template++; break;
        case '>': PTS_ABORTIF(ptd, template == 0); template--; break;
        }
    }
    return -1;
}

static int stabs_pts_read_number(struct ParseTypedefData* ptd, long* v)
{
    char*	last;

    *v = strtol(ptd->ptr, &last, 10);
    PTS_ABORTIF(ptd, last == ptd->ptr);
    ptd->ptr = last;
    return 0;
}

static int stabs_pts_read_type_reference(struct ParseTypedefData* ptd,
                                         long* filenr, long* subnr)
{
    if (*ptd->ptr == '(')
    {
	/* '(' <int> ',' <int> ')' */
	ptd->ptr++;
	PTS_ABORTIF(ptd, stabs_pts_read_number(ptd, filenr) == -1);
	PTS_ABORTIF(ptd, *ptd->ptr++ != ',');
	PTS_ABORTIF(ptd, stabs_pts_read_number(ptd, subnr) == -1);
	PTS_ABORTIF(ptd, *ptd->ptr++ != ')');
    }
    else
    {
    	*filenr = 0;
	PTS_ABORTIF(ptd, stabs_pts_read_number(ptd, subnr) == -1);
    }
    return 0;
}

struct pts_range_value
{
    ULONGLONG           val;
    int                 sign;
};

static int stabs_pts_read_range_value(struct ParseTypedefData* ptd, struct pts_range_value* prv)
{
    char*	last;

    switch (*ptd->ptr)
    {
    case '0':
        while (*ptd->ptr == '0') ptd->ptr++;
        if (*ptd->ptr >= '1' && *ptd->ptr <= '7')
        {
            switch (ptd->ptr[1])
            {
            case '0': 
                PTS_ABORTIF(ptd, ptd->ptr[0] != '1');
                prv->sign = -1;
                prv->val = 0;
                while (isdigit(*ptd->ptr)) prv->val = (prv->val << 3) + *ptd->ptr++ - '0';
                break;
            case '7':
                prv->sign = 1;
                prv->val = 0;
                while (isdigit(*ptd->ptr)) prv->val = (prv->val << 3) + *ptd->ptr++ - '0';
                break;
            default: PTS_ABORTIF(ptd, 1); break;
            }
        } else prv->sign = 0;
        break;
    case '-':
        prv->sign = -1;
        prv->val = strtoull(++ptd->ptr, &last, 10);
        ptd->ptr = last;
        break;
    case '+':
    default:    
        prv->sign = 1;
        prv->val = strtoull(ptd->ptr, &last, 10);
        ptd->ptr = last;
        break;
    }
    return 0;
}

static int stabs_pts_read_range(struct ParseTypedefData* ptd, const char* typename,
                                struct symt** dt)
{
    struct symt*                ref;
    struct pts_range_value      lo;
    struct pts_range_value      hi;
    unsigned                    size;
    enum BasicType              bt;
    int                         i;
    ULONGLONG                   v;

    /* type ';' <int> ';' <int> ';' */
    PTS_ABORTIF(ptd, stabs_pts_read_type_def(ptd, NULL, &ref) == -1);
    PTS_ABORTIF(ptd, *ptd->ptr++ != ';');	/* ';' */
    PTS_ABORTIF(ptd, stabs_pts_read_range_value(ptd, &lo) == -1);
    PTS_ABORTIF(ptd, *ptd->ptr++ != ';');	/* ';' */
    PTS_ABORTIF(ptd, stabs_pts_read_range_value(ptd, &hi) == -1);
    PTS_ABORTIF(ptd, *ptd->ptr++ != ';');	/* ';' */

    /* basically, we don't use ref... in some cases, for example, float is declared
     * as a derived type of int... which won't help us... so we guess the types
     * from the various formats
     */
    if (lo.sign == 0 && hi.sign < 0)
    {
        bt = btUInt;
        size = hi.val;
    }
    else if (lo.sign < 0 && hi.sign == 0)
    {
        bt = btUInt;
        size = lo.val;
    }
    else if (lo.sign > 0 && hi.sign == 0)
    {
        bt = btFloat;
        size = lo.val;
    }
    else if (lo.sign < 0 && hi.sign > 0)
    {
        v = 1 << 7;
        for (i = 7; i < 64; i += 8)
        {
            if (lo.val == v && hi.val == v - 1)
            {
                bt = btInt;
                size = (i + 1) / 8;
                break;
            }
            v <<= 8;
        }
        PTS_ABORTIF(ptd, i >= 64);
    }
    else if (lo.sign == 0 && hi.sign > 0)
    {
        if (hi.val == 127) /* specific case for char... */
        {
            bt = btChar;
            size = 1;
        }
        else
        {
            v = 1;
            for (i = 8; i <= 64; i += 8)
            {
                v <<= 8;
                if (hi.val + 1 == v)
                {
                    bt = btUInt;
                    size = (i + 1) / 8;
                    break;
                }
            }
            PTS_ABORTIF(ptd, i > 64);
        }
    }
    else PTS_ABORTIF(ptd, 1);

    *dt = &symt_new_basic(ptd->module, bt, typename, size)->symt;
    return 0;
}

static inline int stabs_pts_read_method_info(struct ParseTypedefData* ptd)
{
    struct symt*        dt;
    char*               tmp;
    char                mthd;

    do
    {
        /* get type of return value */
        PTS_ABORTIF(ptd, stabs_pts_read_type_def(ptd, NULL, &dt) == -1);
        if (*ptd->ptr == ';') ptd->ptr++;

        /* get types of parameters */
        if (*ptd->ptr == ':')
        {
            PTS_ABORTIF(ptd, !(tmp = strchr(ptd->ptr + 1, ';')));
            ptd->ptr = tmp + 1;
        }
        PTS_ABORTIF(ptd, !(*ptd->ptr >= '0' && *ptd->ptr <= '9'));
        ptd->ptr++;
        PTS_ABORTIF(ptd, !(ptd->ptr[0] >= 'A' && *ptd->ptr <= 'D'));
        mthd = *++ptd->ptr;
        PTS_ABORTIF(ptd, mthd != '.' && mthd != '?' && mthd != '*');
        ptd->ptr++;
        if (mthd == '*')
        {
            long int            ofs;
            struct symt*        dt;

            PTS_ABORTIF(ptd, stabs_pts_read_number(ptd, &ofs) == -1);
            PTS_ABORTIF(ptd, *ptd->ptr++ != ';');
            PTS_ABORTIF(ptd, stabs_pts_read_type_def(ptd, NULL, &dt) == -1);
            PTS_ABORTIF(ptd, *ptd->ptr++ != ';');
        }
    } while (*ptd->ptr != ';');
    ptd->ptr++;

    return 0;
}

static inline int stabs_pts_read_aggregate(struct ParseTypedefData* ptd, 
                                           struct symt_udt* sdt)
{
    long        	sz, ofs;
    struct symt*        adt;
    struct symt*        dt = NULL;
    int			idx;
    int			doadd;

    PTS_ABORTIF(ptd, stabs_pts_read_number(ptd, &sz) == -1);

    doadd = symt_set_udt_size(ptd->module, sdt, sz);
    if (*ptd->ptr == '!') /* C++ inheritence */
    {
        long     num_classes;

        ptd->ptr++;
        PTS_ABORTIF(ptd, stabs_pts_read_number(ptd, &num_classes) == -1);
        PTS_ABORTIF(ptd, *ptd->ptr++ != ',');
        while (--num_classes >= 0)
        {
            ptd->ptr += 2; /* skip visibility and inheritence */
            PTS_ABORTIF(ptd, stabs_pts_read_number(ptd, &ofs) == -1);
            PTS_ABORTIF(ptd, *ptd->ptr++ != ',');

            PTS_ABORTIF(ptd, stabs_pts_read_type_def(ptd, NULL, &adt) == -1);

            if (doadd)
            {
                char    tmp[256];
                WCHAR*  name;
                DWORD64 size;

                symt_get_info(adt, TI_GET_SYMNAME, &name);
                strcpy(tmp, "__inherited_class_");
                WideCharToMultiByte(CP_ACP, 0, name, -1, 
                                    tmp + strlen(tmp), sizeof(tmp) - strlen(tmp),
                                    NULL, NULL);
                HeapFree(GetProcessHeap(), 0, name);
                /* FIXME: TI_GET_LENGTH will not always work, especially when adt
                 * has just been seen as a forward definition and not the real stuff
                 * yet.
                 * As we don't use much the size of members in structs, this may not
                 * be much of a problem
                 */
                symt_get_info(adt, TI_GET_LENGTH, &size);
                symt_add_udt_element(ptd->module, sdt, tmp, adt, ofs, (DWORD)size * 8);
            }
            PTS_ABORTIF(ptd, *ptd->ptr++ != ';');
        }
        
    }
    /* if the structure has already been filled, just redo the parsing
     * but don't store results into the struct
     * FIXME: there's a quite ugly memory leak in there...
     */

    /* Now parse the individual elements of the structure/union. */
    while (*ptd->ptr != ';') 
    {
	/* agg_name : type ',' <int:offset> ',' <int:size> */
	idx = ptd->idx;

        if (ptd->ptr[0] == '$' && ptd->ptr[1] == 'v')
        {
            long        x;

            if (ptd->ptr[2] == 'f')
            {
                /* C++ virtual method table */
                ptd->ptr += 3;
                stabs_read_type_enum(&ptd->ptr);
                PTS_ABORTIF(ptd, *ptd->ptr++ != ':');
                PTS_ABORTIF(ptd, stabs_pts_read_type_def(ptd, NULL, &dt) == -1);
                PTS_ABORTIF(ptd, *ptd->ptr++ != ',');
                PTS_ABORTIF(ptd, stabs_pts_read_number(ptd, &x) == -1);
                PTS_ABORTIF(ptd, *ptd->ptr++ != ';');
                ptd->idx = idx;
                continue;
            }
            else if (ptd->ptr[2] == 'b')
            {
                ptd->ptr += 3;
                PTS_ABORTIF(ptd, stabs_pts_read_type_def(ptd, NULL, &dt) == -1);
                PTS_ABORTIF(ptd, *ptd->ptr++ != ':');
                PTS_ABORTIF(ptd, stabs_pts_read_type_def(ptd, NULL, &dt) == -1);
                PTS_ABORTIF(ptd, *ptd->ptr++ != ',');
                PTS_ABORTIF(ptd, stabs_pts_read_number(ptd, &x) == -1);
                PTS_ABORTIF(ptd, *ptd->ptr++ != ';');
                ptd->idx = idx;
                continue;
            }
        }

	PTS_ABORTIF(ptd, stabs_pts_read_id(ptd) == -1);
        /* Ref. TSDF R2.130 Section 7.4.  When the field name is a method name
         * it is followed by two colons rather than one.
         */
        if (*ptd->ptr == ':')
        {
            ptd->ptr++; 
            stabs_pts_read_method_info(ptd);
            ptd->idx = idx;
            continue;
        }
        else
        {
            /* skip C++ member protection /0 /1 or /2 */
            if (*ptd->ptr == '/') ptd->ptr += 2;
        }
	PTS_ABORTIF(ptd, stabs_pts_read_type_def(ptd, NULL, &adt) == -1);

        switch (*ptd->ptr++)
        {
        case ',':
            PTS_ABORTIF(ptd, stabs_pts_read_number(ptd, &ofs) == -1);
            PTS_ABORTIF(ptd, *ptd->ptr++ != ',');
            PTS_ABORTIF(ptd, stabs_pts_read_number(ptd, &sz) == -1);
            PTS_ABORTIF(ptd, *ptd->ptr++ != ';');

            if (doadd) symt_add_udt_element(ptd->module, sdt, ptd->buf + idx, adt, ofs, sz);
            break;
        case ':':
            {
                char* tmp;
                /* method parameters... terminated by ';' */
                PTS_ABORTIF(ptd, !(tmp = strchr(ptd->ptr, ';')));
                ptd->ptr = tmp + 1;
            }
            break;
        default:
            PTS_ABORTIF(ptd, TRUE);
        }
	ptd->idx = idx;
    }
    PTS_ABORTIF(ptd, *ptd->ptr++ != ';');
    if (*ptd->ptr == '~')
    {
        ptd->ptr++;
        PTS_ABORTIF(ptd, *ptd->ptr++ != '%');
        PTS_ABORTIF(ptd, stabs_pts_read_type_def(ptd, NULL, &dt) == -1);
        PTS_ABORTIF(ptd, *ptd->ptr++ != ';');
    }
    return 0;
}

static inline int stabs_pts_read_enum(struct ParseTypedefData* ptd, 
                                      struct symt_enum* edt)
{
    long        value;
    int		idx;

    while (*ptd->ptr != ';')
    {
	idx = ptd->idx;
	PTS_ABORTIF(ptd, stabs_pts_read_id(ptd) == -1);
	PTS_ABORTIF(ptd, stabs_pts_read_number(ptd, &value) == -1);
	PTS_ABORTIF(ptd, *ptd->ptr++ != ',');
	symt_add_enum_element(ptd->module, edt, ptd->buf + idx, value);
	ptd->idx = idx;
    }
    ptd->ptr++;
    return 0;
}

static inline int stabs_pts_read_array(struct ParseTypedefData* ptd,
                                       struct symt** adt)
{
    long                lo, hi;
    struct symt*        range_dt;
    struct symt*        base_dt;

    /* ar<typeinfo_nodef>;<int>;<int>;<typeinfo> */

    PTS_ABORTIF(ptd, *ptd->ptr++ != 'r');

    PTS_ABORTIF(ptd, stabs_pts_read_type_def(ptd, NULL, &range_dt) == -1);
    PTS_ABORTIF(ptd, *ptd->ptr++ != ';');	/* ';' */
    PTS_ABORTIF(ptd, stabs_pts_read_number(ptd, &lo) == -1);
    PTS_ABORTIF(ptd, *ptd->ptr++ != ';');	/* ';' */
    PTS_ABORTIF(ptd, stabs_pts_read_number(ptd, &hi) == -1);
    PTS_ABORTIF(ptd, *ptd->ptr++ != ';');	/* ';' */

    PTS_ABORTIF(ptd, stabs_pts_read_type_def(ptd, NULL, &base_dt) == -1);

    *adt = &symt_new_array(ptd->module, lo, hi, base_dt, range_dt)->symt;
    return 0;
}

static int stabs_pts_read_type_def(struct ParseTypedefData* ptd, const char* typename,
                                   struct symt** ret_dt)
{
    int			idx;
    long		sz = -1;
    struct symt*	new_dt = NULL;     /* newly created data type */
    struct symt*	ref_dt;		   /* referenced data type (pointer...) */
    long		filenr1, subnr1, tmp;

    /* things are a bit complicated because of the way the typedefs are stored inside
     * the file, because addresses can change when realloc is done, so we must call
     * over and over stabs_find_ref() to keep the correct values around
     */
    PTS_ABORTIF(ptd, stabs_pts_read_type_reference(ptd, &filenr1, &subnr1) == -1);

    while (*ptd->ptr == '=')
    {
	ptd->ptr++;
	PTS_ABORTIF(ptd, new_dt != NULL);

	/* first handle attribute if any */
	switch (*ptd->ptr)      
        {
	case '@':
	    if (*++ptd->ptr == 's')
            {
		ptd->ptr++;
		if (stabs_pts_read_number(ptd, &sz) == -1)
                {
		    ERR("Not an attribute... NIY\n");
		    ptd->ptr -= 2;
		    return -1;
		}
		PTS_ABORTIF(ptd, *ptd->ptr++ != ';');
	    }
	    break;
	}
	/* then the real definitions */
	switch (*ptd->ptr++)
        {
	case '*':
        case '&':
	    PTS_ABORTIF(ptd, stabs_pts_read_type_def(ptd, NULL, &ref_dt) == -1);
	    new_dt = &symt_new_pointer(ptd->module, ref_dt)->symt;
           break;
        case 'k': /* 'const' modifier */
        case 'B': /* 'volatile' modifier */
            /* just kinda ignore the modifier, I guess -gmt */
            PTS_ABORTIF(ptd, stabs_pts_read_type_def(ptd, typename, &new_dt) == -1);
	    break;
	case '(':
	    ptd->ptr--;
            PTS_ABORTIF(ptd, stabs_pts_read_type_def(ptd, typename, &new_dt) == -1);
	    break;
	case 'a':
	    PTS_ABORTIF(ptd, stabs_pts_read_array(ptd, &new_dt) == -1);
	    break;
	case 'r':
	    PTS_ABORTIF(ptd, stabs_pts_read_range(ptd, typename, &new_dt) == -1);
	    assert(!*stabs_find_ref(filenr1, subnr1));
	    *stabs_find_ref(filenr1, subnr1) = new_dt;
	    break;
	case 'f':
	    PTS_ABORTIF(ptd, stabs_pts_read_type_def(ptd, NULL, &ref_dt) == -1);
	    new_dt = &symt_new_function_signature(ptd->module, ref_dt, -1)->symt;
	    break;
	case 'e':
            stabs_get_basic(ptd, 1 /* int */, &ref_dt);
            new_dt = &symt_new_enum(ptd->module, typename, ref_dt)->symt;
	    PTS_ABORTIF(ptd, stabs_pts_read_enum(ptd, (struct symt_enum*)new_dt) == -1);
	    break;
	case 's':
	case 'u':
            {
                struct symt_udt*    udt;
                enum UdtKind kind = (ptd->ptr[-1] == 's') ? UdtStruct : UdtUnion;
                /* udt can have been already defined in a forward definition */
                udt = (struct symt_udt*)*stabs_find_ref(filenr1, subnr1);
                if (!udt)
                {
                    udt = symt_new_udt(ptd->module, typename, 0, kind);
                    /* we need to set it here, because a struct can hold a pointer
                     * to itself
                     */
                    new_dt = *stabs_find_ref(filenr1, subnr1) = &udt->symt;
                }
                else
                {
                    unsigned l1, l2;
                    if (udt->symt.tag != SymTagUDT)
                    {
                        ERR("Forward declaration (%p/%s) is not an aggregate (%u)\n",
                            udt, symt_get_name(&udt->symt), udt->symt.tag);
                        return -1;
                    }
                    /* FIXME: we currently don't correctly construct nested C++
                     * classes names. Therefore, we could be here with either:
                     * - typename and udt->hash_elt.name being the same string
                     *   (non embedded case)
                     * - typename being foo::bar while udt->hash_elt.name being 
                     *   just bar
                     * So, we twist the comparison to test both occurrences. When
                     * we have proper C++ types in this file, this twist has to be
                     * removed
                     */
                    l1 = strlen(udt->hash_elt.name);
                    l2 = strlen(typename);
                    if (l1 > l2 || strcmp(udt->hash_elt.name, typename + l2 - l1))
                        ERR("Forward declaration name mismatch %s <> %s\n",
                            udt->hash_elt.name, typename);
                    new_dt = &udt->symt;
                }
                PTS_ABORTIF(ptd, stabs_pts_read_aggregate(ptd, udt) == -1);
	    }
	    break;
	case 'x':
	    idx = ptd->idx;
            tmp = *ptd->ptr++;
	    PTS_ABORTIF(ptd, stabs_pts_read_id(ptd) == -1);
	    switch (tmp)
            {
	    case 'e':
                stabs_get_basic(ptd, 1 /* int */, &ref_dt);
                new_dt = &symt_new_enum(ptd->module, ptd->buf + idx, ref_dt)->symt;
                break;
	    case 's':
                new_dt = &symt_new_udt(ptd->module, ptd->buf + idx, 0, UdtStruct)->symt;
	        break;
            case 'u':
                new_dt = &symt_new_udt(ptd->module, ptd->buf + idx, 0, UdtUnion)->symt;
	        break;
	    default:
                return -1;
	    }
	    ptd->idx = idx;
	    break;
	case '-':
            {
                PTS_ABORTIF(ptd, stabs_pts_read_number(ptd, &tmp) == -1);
                PTS_ABORTIF(ptd, stabs_get_basic(ptd, tmp, &new_dt) == -1);
                PTS_ABORTIF(ptd, *ptd->ptr++ != ';');
            }
	    break;
        case '#':
            if (*ptd->ptr == '#')
            {
                ptd->ptr++;
                PTS_ABORTIF(ptd, stabs_pts_read_type_def(ptd, NULL, &ref_dt) == -1);
                new_dt = &symt_new_function_signature(ptd->module, ref_dt, -1)->symt;
            }
            else
            {
                struct symt*    cls_dt;
                struct symt*    pmt_dt;

                PTS_ABORTIF(ptd, stabs_pts_read_type_def(ptd, NULL, &cls_dt) == -1);
                PTS_ABORTIF(ptd, *ptd->ptr++ != ',');
                PTS_ABORTIF(ptd, stabs_pts_read_type_def(ptd, NULL, &ref_dt) == -1);
                new_dt = &symt_new_function_signature(ptd->module, ref_dt, -1)->symt;
                while (*ptd->ptr == ',')
                {
                    ptd->ptr++;
                    PTS_ABORTIF(ptd, stabs_pts_read_type_def(ptd, NULL, &pmt_dt) == -1);
                }
            }
            break;
        case 'R':
            {
                long    type, len, unk;
                int     basic;
                
                PTS_ABORTIF(ptd, stabs_pts_read_number(ptd, &type) == -1);
                PTS_ABORTIF(ptd, *ptd->ptr++ != ';');	/* ';' */
                PTS_ABORTIF(ptd, stabs_pts_read_number(ptd, &len) == -1);
                PTS_ABORTIF(ptd, *ptd->ptr++ != ';');	/* ';' */
                PTS_ABORTIF(ptd, stabs_pts_read_number(ptd, &unk) == -1);
                PTS_ABORTIF(ptd, *ptd->ptr++ != ';');	/* ';' */

                switch (type) /* see stabs_get_basic for the details */
                {
                case 1: basic = 12; break;
                case 2: basic = 13; break;
                case 3: basic = 25; break;
                case 4: basic = 26; break;
                case 5: basic = 35; break;
                case 6: basic = 14; break;
                default: PTS_ABORTIF(ptd, 1);
                }
                PTS_ABORTIF(ptd, stabs_get_basic(ptd, basic, &new_dt) == -1);
            }
            break;
	default:
	    ERR("Unknown type '%c'\n", ptd->ptr[-1]);
	    return -1;
	}
    }

    if (!new_dt)
    {
        /* is it a forward declaration that has been filled ? */
	new_dt = *stabs_find_ref(filenr1, subnr1);
        /* if not, this should be void (which is defined as a ref to itself, but we
         * don't correctly catch it)
         */
        if (!new_dt && typename)
        {
            new_dt = &symt_new_basic(ptd->module, btVoid, typename, 0)->symt;
            PTS_ABORTIF(ptd, strcmp(typename, "void"));
        }
    }            

    *stabs_find_ref(filenr1, subnr1) = *ret_dt = new_dt;

    TRACE("Adding (%ld,%ld) %s\n", filenr1, subnr1, debugstr_a(typename));

    return 0;
}

static int stabs_parse_typedef(struct module* module, const char* ptr, 
                               const char* typename)
{
    struct ParseTypedefData	ptd;
    struct symt*                dt;
    int				ret = -1;

    /* check for already existing definition */

    TRACE("%s => %s\n", typename, debugstr_a(ptr));
    ptd.module = module;
    ptd.idx = 0;
#ifdef PTS_DEBUG
    ptd.err_idx = 0;
#endif
    for (ptd.ptr = ptr - 1; ;)
    {
        ptd.ptr = strchr(ptd.ptr + 1, ':');
        if (ptd.ptr == NULL || *++ptd.ptr != ':') break;
    }
    if (ptd.ptr)
    {
	if (*ptd.ptr != '(') ptd.ptr++;
        /* most of type definitions take one char, except Tt */
	if (*ptd.ptr != '(') ptd.ptr++;
	ret = stabs_pts_read_type_def(&ptd, typename, &dt);
    }

    if (ret == -1 || *ptd.ptr) 
    {
#ifdef PTS_DEBUG
        int     i;
	TRACE("Failure on %s\n", debugstr_a(ptr));
        if (ret == -1)
        {
            for (i = 0; i < ptd.err_idx; i++)
            {
                TRACE("[%d]: line %d => %s\n", 
                      i, ptd.errors[i].line, debugstr_a(ptd.errors[i].ptr));
            }
        }
        else
            TRACE("[0]: => %s\n", debugstr_a(ptd.ptr));
            
#else
	ERR("Failure on %s at %s\n", debugstr_a(ptr), debugstr_a(ptd.ptr));
#endif
	return FALSE;
    }

    return TRUE;
}

static struct symt* stabs_parse_type(const char* stab)
{
    const char* c = stab - 1;

    /*
     * Look through the stab definition, and figure out what struct symt
     * this represents.  If we have something we know about, assign the
     * type.
     * According to "The \"stabs\" debug format" (Rev 2.130) the name may be
     * a C++ name and contain double colons e.g. foo::bar::baz:t5=*6.
     */
    do
    {
        if ((c = strchr(c + 1, ':')) == NULL) return NULL;
    } while (*++c == ':');

    /*
     * The next characters say more about the type (i.e. data, function, etc)
     * of symbol.  Skip them.  (C++ for example may have Tt).
     * Actually this is a very weak description; I think Tt is the only
     * multiple combination we should see.
     */
    while (*c && *c != '(' && !isdigit(*c))
        c++;
    /*
     * The next is either an integer or a (integer,integer).
     * The stabs_read_type_enum() takes care that stab_types is large enough.
     */
    return *stabs_read_type_enum(&c);
}

struct pending_loc_var
{
    char                name[256];
    struct symt*        type;
    enum DataKind       kind;
    struct location     loc;
};

struct pending_block
{
    struct pending_loc_var*     vars;
    unsigned                    num;
    unsigned                    allocated;
};

static inline void pending_add(struct pending_block* pending, const char* name,
                               enum DataKind dt, const struct location* loc)
{
    if (pending->num == pending->allocated)
    {
        pending->allocated += 8;
        if (!pending->vars)
            pending->vars = HeapAlloc(GetProcessHeap(), 0, 
                                     pending->allocated * sizeof(pending->vars[0]));
        else    
            pending->vars = HeapReAlloc(GetProcessHeap(), 0, pending->vars,
                                       pending->allocated * sizeof(pending->vars[0]));
    }
    stab_strcpy(pending->vars[pending->num].name, 
                sizeof(pending->vars[pending->num].name), name);
    pending->vars[pending->num].type   = stabs_parse_type(name);
    pending->vars[pending->num].kind   = dt;
    pending->vars[pending->num].loc    = *loc;
    pending->num++;
}

static void pending_flush(struct pending_block* pending, struct module* module, 
                          struct symt_function* func, struct symt_block* block)
{
    int i;

    for (i = 0; i < pending->num; i++)
    {
        symt_add_func_local(module, func, 
                            pending->vars[i].kind, &pending->vars[i].loc,
                            block, pending->vars[i].type, pending->vars[i].name);
    }
    pending->num = 0;
}

/******************************************************************
 *		stabs_finalize_function
 *
 * Ends function creation: mainly:
 * - cleans up line number information
 * - tries to set up a debug-start tag (FIXME: heuristic to be enhanced)
 * - for stabs which have absolute address in them, initializes the size of the
 *   function (assuming that current function ends where next function starts)
 */
static void stabs_finalize_function(struct module* module, struct symt_function* func,
                                    unsigned long size)
{
    IMAGEHLP_LINE       il;
    struct location     loc;

    if (!func) return;
    symt_normalize_function(module, func);
    /* To define the debug-start of the function, we use the second line number.
     * Not 100% bullet proof, but better than nothing
     */
    if (symt_fill_func_line_info(module, func, func->address, &il) &&
        symt_get_func_line_next(module, &il))
    {
        loc.kind = loc_absolute;
        loc.offset = il.Address - func->address;
        symt_add_function_point(module, func, SymTagFuncDebugStart, 
                                &loc, NULL);
    }
    if (size) func->size = size;
}

BOOL stabs_parse(struct module* module, unsigned long load_offset, 
                 const void* pv_stab_ptr, int stablen,
                 const char* strs, int strtablen)
{
    struct symt_function*       curr_func = NULL;
    struct symt_block*          block = NULL;
    struct symt_compiland*      compiland = NULL;
    char                        srcpath[PATH_MAX]; /* path to directory source file is in */
    int                         i;
    int                         nstab;
    const char*                 ptr;
    char*                       stabbuff;
    unsigned int                stabbufflen;
    const struct stab_nlist*    stab_ptr = pv_stab_ptr;
    const char*                 strs_end;
    int                         strtabinc;
    char                        symname[4096];
    unsigned                    incl[32];
    int                         incl_stk = -1;
    int                         source_idx = -1;
    struct pending_block        pending;
    BOOL                        ret = TRUE;
    struct location             loc;

    nstab = stablen / sizeof(struct stab_nlist);
    strs_end = strs + strtablen;

    memset(srcpath, 0, sizeof(srcpath));
    memset(stabs_basic, 0, sizeof(stabs_basic));
    memset(&pending, 0, sizeof(pending));

    /*
     * Allocate a buffer into which we can build stab strings for cases
     * where the stab is continued over multiple lines.
     */
    stabbufflen = 65536;
    stabbuff = HeapAlloc(GetProcessHeap(), 0, stabbufflen);

    strtabinc = 0;
    stabbuff[0] = '\0';
    for (i = 0; i < nstab; i++, stab_ptr++)
    {
        ptr = strs + stab_ptr->n_un.n_strx;
        if ((ptr > strs_end) || (ptr + strlen(ptr) > strs_end))
        {
            WARN("Bad stabs string %p\n", ptr);
            continue;
        }
        if (*ptr != '\0' && (ptr[strlen(ptr) - 1] == '\\'))
        {
            /*
             * Indicates continuation.  Append this to the buffer, and go onto the
             * next record.  Repeat the process until we find a stab without the
             * '/' character, as this indicates we have the whole thing.
             */
            unsigned    len = strlen(ptr);
            if (strlen(stabbuff) + len > stabbufflen)
            {
                stabbufflen += 65536;
                stabbuff = HeapReAlloc(GetProcessHeap(), 0, stabbuff, stabbufflen);
            }
            strncat(stabbuff, ptr, len - 1);
            continue;
        }
        else if (stabbuff[0] != '\0')
        {
            strcat(stabbuff, ptr);
            ptr = stabbuff;
        }

        /* only symbol entries contain a typedef */
        switch (stab_ptr->n_type)
        {
        case N_GSYM:
        case N_LCSYM:
        case N_STSYM:
        case N_RSYM:
        case N_LSYM:
        case N_ROSYM:
        case N_PSYM:
            if (strchr(ptr, '=') != NULL)
            {
                /*
                 * The stabs aren't in writable memory, so copy it over so we are
                 * sure we can scribble on it.
                 */
                if (ptr != stabbuff)
                {
                    strcpy(stabbuff, ptr);
                    ptr = stabbuff;
                }
                stab_strcpy(symname, sizeof(symname), ptr);
                if (!stabs_parse_typedef(module, ptr, symname))
                {
                    /* skip this definition */
                    stabbuff[0] = '\0';
                    continue;
                }
            }
        }

        switch (stab_ptr->n_type)
        {
        case N_GSYM:
            /*
             * These are useless with ELF.  They have no value, and you have to
             * read the normal symbol table to get the address.  Thus we
             * ignore them, and when we process the normal symbol table
             * we should do the right thing.
             *
             * With a.out or mingw, they actually do make some amount of sense.
             */
            stab_strcpy(symname, sizeof(symname), ptr);
            symt_new_global_variable(module, compiland, symname, TRUE /* FIXME */,
                                     load_offset + stab_ptr->n_value, 0,
                                     stabs_parse_type(ptr));
            break;
        case N_LCSYM:
        case N_STSYM:
            /* These are static symbols and BSS symbols. */
            stab_strcpy(symname, sizeof(symname), ptr);
            symt_new_global_variable(module, compiland, symname, TRUE /* FIXME */,
                                     load_offset + stab_ptr->n_value, 0,
                                     stabs_parse_type(ptr));
            break;
        case N_LBRAC:
            if (curr_func)
            {
                block = symt_open_func_block(module, curr_func, block,
                                             stab_ptr->n_value, 0);
                pending_flush(&pending, module, curr_func, block);
            }
            break;
        case N_RBRAC:
            if (curr_func)
                block = symt_close_func_block(module, curr_func, block,
                                              stab_ptr->n_value);
            break;
        case N_PSYM:
            /* These are function parameters. */
            if (curr_func != NULL)
            {
                struct symt*    param_type = stabs_parse_type(ptr);
                stab_strcpy(symname, sizeof(symname), ptr);
                loc.kind = loc_regrel;
                loc.reg = 0; /* FIXME */
                loc.offset = stab_ptr->n_value;
                symt_add_func_local(module, curr_func,
                                    (long)stab_ptr->n_value >= 0 ? DataIsParam : DataIsLocal,
                                    &loc, NULL, param_type, symname);
                symt_add_function_signature_parameter(module, 
                                                      (struct symt_function_signature*)curr_func->type, 
                                                      param_type);
            }
            break;
        case N_RSYM:
            /* These are registers (as local variables) */
            if (curr_func != NULL)
            {
                loc.kind = loc_register;
                loc.offset = 0;

                switch (stab_ptr->n_value)
                {
                case  0: loc.reg = CV_REG_EAX; break;
                case  1: loc.reg = CV_REG_ECX; break;
                case  2: loc.reg = CV_REG_EDX; break;
                case  3: loc.reg = CV_REG_EBX; break;
                case  4: loc.reg = CV_REG_ESP; break;
                case  5: loc.reg = CV_REG_EBP; break;
                case  6: loc.reg = CV_REG_ESI; break;
                case  7: loc.reg = CV_REG_EDI; break;
                case 11:
                case 12:
                case 13:
                case 14:
                case 15:
                case 16:
                case 17:
                case 18:
                case 19: loc.reg = CV_REG_ST0 + stab_ptr->n_value - 12; break;
                default:
                    FIXME("Unknown register value (%lu)\n", stab_ptr->n_value);
                    loc.reg = CV_REG_NONE;
                    break;
                }
                stab_strcpy(symname, sizeof(symname), ptr);
                if (ptr[strlen(symname) + 1] == 'P')
                {
                    struct symt*    param_type = stabs_parse_type(ptr);
                    stab_strcpy(symname, sizeof(symname), ptr);
                    symt_add_func_local(module, curr_func, DataIsParam, &loc,
                                        NULL, param_type, symname);
                    symt_add_function_signature_parameter(module, 
                                                          (struct symt_function_signature*)curr_func->type, 
                                                          param_type);
                }
                else
                    pending_add(&pending, ptr, DataIsLocal, &loc);
            }
            break;
        case N_LSYM:
            /* These are local variables */
            loc.kind = loc_regrel;
            loc.reg = 0; /* FIXME */
            loc.offset = stab_ptr->n_value;
            if (curr_func != NULL) pending_add(&pending, ptr, DataIsLocal, &loc);
            break;
        case N_SLINE:
            /*
             * This is a line number.  These are always relative to the start
             * of the function (N_FUN), and this makes the lookup easier.
             */
            if (curr_func != NULL)
            {
                assert(source_idx >= 0);
                symt_add_func_line(module, curr_func, source_idx, 
                                   stab_ptr->n_desc, stab_ptr->n_value);
            }
            break;
        case N_FUN:
            /*
             * For now, just declare the various functions.  Later
             * on, we will add the line number information and the
             * local symbols.
             */
            /*
             * Copy the string to a temp buffer so we
             * can kill everything after the ':'.  We do
             * it this way because otherwise we end up dirtying
             * all of the pages related to the stabs, and that
             * sucks up swap space like crazy.
             */
            stab_strcpy(symname, sizeof(symname), ptr);
            if (*symname)
            {
                struct symt_function_signature* func_type;

                if (curr_func)
                {
                    /* First, clean up the previous function we were working on.
                     * Assume size of the func is the delta between current offset
                     * and offset of last function
                     */
                    stabs_finalize_function(module, curr_func, 
                                            stab_ptr->n_value ?
                                                (load_offset + stab_ptr->n_value - curr_func->address) : 0);
                }
                func_type = symt_new_function_signature(module, 
                                                        stabs_parse_type(ptr), -1);
                curr_func = symt_new_function(module, compiland, symname, 
                                              load_offset + stab_ptr->n_value, 0,
                                              &func_type->symt);
            }
            else
            {
                /* some versions of GCC to use a N_FUN "" to mark the end of a function
                 * and n_value contains the size of the func
                 */
                stabs_finalize_function(module, curr_func, stab_ptr->n_value);
                curr_func = NULL;
            }
            break;
        case N_SO:
            /*
             * This indicates a new source file.  Append the records
             * together, to build the correct path name.
             */
            if (*ptr == '\0') /* end of N_SO file */
            {
                /* Nuke old path. */
                srcpath[0] = '\0';
                stabs_finalize_function(module, curr_func, 0);
                curr_func = NULL;
                source_idx = -1;
                incl_stk = -1;
                assert(block == NULL);
                compiland = NULL;
            }
            else
            {
                int len = strlen(ptr);
                if (ptr[len-1] != '/')
                {
                    stabs_reset_includes();
                    source_idx = source_new(module, srcpath, ptr);
                    compiland = symt_new_compiland(module, 0 /* FIXME */, source_idx);
                }
                else
                    strcpy(srcpath, ptr);
            }
            break;
        case N_SOL:
            source_idx = source_new(module, srcpath, ptr);
            break;
        case N_UNDF:
            strs += strtabinc;
            strtabinc = stab_ptr->n_value;
            /* I'm not sure this is needed, so trace it before we obsolete it */
            if (curr_func)
            {
                FIXME("UNDF: curr_func %s\n", curr_func->hash_elt.name);
                stabs_finalize_function(module, curr_func, 0); /* FIXME */
                curr_func = NULL;
            }
            break;
        case N_OPT:
            /* Ignore this. We don't care what it points to. */
            break;
        case N_BINCL:
            stabs_add_include(stabs_new_include(ptr, stab_ptr->n_value));
            assert(incl_stk < (int)(sizeof(incl) / sizeof(incl[0])) - 1);
            incl[++incl_stk] = source_idx;
            source_idx = source_new(module, NULL, ptr);
            break;
        case N_EINCL:
            assert(incl_stk >= 0);
            source_idx = incl[incl_stk--];
            break;
	case N_EXCL:
            if (stabs_add_include(stabs_find_include(ptr, stab_ptr->n_value)) < 0)
            {
                ERR("Excluded header not found (%s,%ld)\n", ptr, stab_ptr->n_value);
                module_reset_debug_info(module);
                ret = FALSE;
                goto done;
            }
            break;
        case N_MAIN:
            /* Always ignore these. GCC doesn't even generate them. */
            break;
        case N_BNSYM:
        case N_ENSYM:
            /* Always ignore these, they seem to be used only on Darwin. */
            break;
        default:
            ERR("Unknown stab type 0x%02x\n", stab_ptr->n_type);
            break;
        }
        stabbuff[0] = '\0';
        TRACE("0x%02x %lx %s\n", 
              stab_ptr->n_type, stab_ptr->n_value, debugstr_a(strs + stab_ptr->n_un.n_strx));
    }
    module->module.SymType = SymDia;
    module->module.CVSig = 'S' | ('T' << 8) | ('A' << 16) | ('B' << 24);
    /* FIXME: we could have a finer grain here */
    module->module.LineNumbers = TRUE;
    module->module.GlobalSymbols = TRUE;
    module->module.TypeInfo = TRUE;
    module->module.SourceIndexed = TRUE;
    module->module.Publics = TRUE;
done:
    HeapFree(GetProcessHeap(), 0, stabbuff);
    stabs_free_includes();
    HeapFree(GetProcessHeap(), 0, pending.vars);

    return ret;
}
