/*
 *  Demangle VC++ symbols into C function prototypes
 *
 *  Copyright 2000 Jon Griffiths
 *            2004 Eric Pouech
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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "msvcrt.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

#ifdef __REACTOS__
#define MSVCRT_atoi atoi
#define MSVCRT_isdigit isdigit
#define MSVCRT_sprintf sprintf
#endif

/* TODO:
 * - document a bit (grammar + functions)
 * - back-port this new code into tools/winedump/msmangle.c
 */

/* How data types modifiers are stored:
 * M (in the following definitions) is defined for 
 * 'A', 'B', 'C' and 'D' as follows
 *      {<A>}:  ""
 *      {<B>}:  "const "
 *      {<C>}:  "volatile "
 *      {<D>}:  "const volatile "
 *
 *      in arguments:
 *              P<M>x   {<M>}x*
 *              Q<M>x   {<M>}x* const
 *              A<M>x   {<M>}x&
 *      in data fields:
 *              same as for arguments and also the following
 *              ?<M>x   {<M>}x
 *              
 */

struct array
{
    unsigned            start;          /* first valid reference in array */
    unsigned            num;            /* total number of used elts */
    unsigned            max;
    unsigned            alloc;
    char**              elts;
};

/* Structure holding a parsed symbol */
struct parsed_symbol
{
    unsigned            flags;          /* the UNDNAME_ flags used for demangling */
    malloc_func_t       mem_alloc_ptr;  /* internal allocator */
    free_func_t         mem_free_ptr;   /* internal deallocator */

    const char*         current;        /* pointer in input (mangled) string */
    char*               result;         /* demangled string */

    struct array        names;          /* array of names for back reference */
    struct array        stack;          /* stack of parsed strings */

    void*               alloc_list;     /* linked list of allocated blocks */
    unsigned            avail_in_first; /* number of available bytes in head block */
};

/* Type for parsing mangled types */
struct datatype_t
{
    const char*         left;
    const char*         right;
};

static BOOL symbol_demangle(struct parsed_symbol* sym);

/******************************************************************
 *		und_alloc
 *
 * Internal allocator. Uses a simple linked list of large blocks
 * where we use a poor-man allocator. It's fast, and since all
 * allocation is pool, memory management is easy (esp. freeing).
 */
static void*    und_alloc(struct parsed_symbol* sym, unsigned int len)
{
    void*       ptr;

#define BLOCK_SIZE      1024
#define AVAIL_SIZE      (1024 - sizeof(void*))

    if (len > AVAIL_SIZE)
    {
        /* allocate a specific block */
        ptr = sym->mem_alloc_ptr(sizeof(void*) + len);
        if (!ptr) return NULL;
        *(void**)ptr = sym->alloc_list;
        sym->alloc_list = ptr;
        sym->avail_in_first = 0;
        ptr = (char*)sym->alloc_list + sizeof(void*);
    }
    else 
    {
        if (len > sym->avail_in_first)
        {
            /* add a new block */
            ptr = sym->mem_alloc_ptr(BLOCK_SIZE);
            if (!ptr) return NULL;
            *(void**)ptr = sym->alloc_list;
            sym->alloc_list = ptr;
            sym->avail_in_first = AVAIL_SIZE;
        }
        /* grab memory from head block */
        ptr = (char*)sym->alloc_list + BLOCK_SIZE - sym->avail_in_first;
        sym->avail_in_first -= len;
    }
    return ptr;
#undef BLOCK_SIZE
#undef AVAIL_SIZE
}

/******************************************************************
 *		und_free
 * Frees all the blocks in the list of large blocks allocated by
 * und_alloc.
 */
static void und_free_all(struct parsed_symbol* sym)
{
    void*       next;

    while (sym->alloc_list)
    {
        next = *(void**)sym->alloc_list;
        if(sym->mem_free_ptr) sym->mem_free_ptr(sym->alloc_list);
        sym->alloc_list = next;
    }
    sym->avail_in_first = 0;
}

/******************************************************************
 *		str_array_init
 * Initialises an array of strings
 */
static void str_array_init(struct array* a)
{
    a->start = a->num = a->max = a->alloc = 0;
    a->elts = NULL;
}

/******************************************************************
 *		str_array_push
 * Adding a new string to an array
 */
static BOOL str_array_push(struct parsed_symbol* sym, const char* ptr, int len,
                           struct array* a)
{
    char**      new;

    assert(ptr);
    assert(a);

    if (!a->alloc)
    {
        new = und_alloc(sym, (a->alloc = 32) * sizeof(a->elts[0]));
        if (!new) return FALSE;
        a->elts = new;
    }
    else if (a->max >= a->alloc)
    {
        new = und_alloc(sym, (a->alloc * 2) * sizeof(a->elts[0]));
        if (!new) return FALSE;
        memcpy(new, a->elts, a->alloc * sizeof(a->elts[0]));
        a->alloc *= 2;
        a->elts = new;
    }
    if (len == -1) len = strlen(ptr);
    a->elts[a->num] = und_alloc(sym, len + 1);
    assert(a->elts[a->num]);
    memcpy(a->elts[a->num], ptr, len);
    a->elts[a->num][len] = '\0'; 
    if (++a->num >= a->max) a->max = a->num;
    {
        int i;
        char c;

        for (i = a->max - 1; i >= 0; i--)
        {
            c = '>';
            if (i < a->start) c = '-';
            else if (i >= a->num) c = '}';
            TRACE("%p\t%d%c %s\n", a, i, c, debugstr_a(a->elts[i]));
        }
    }

    return TRUE;
}

/******************************************************************
 *		str_array_get_ref
 * Extracts a reference from an existing array (doing proper type
 * checking)
 */
static char* str_array_get_ref(struct array* cref, unsigned idx)
{
    assert(cref);
    if (cref->start + idx >= cref->max)
    {
        WARN("Out of bounds: %p %d + %d >= %d\n", 
              cref, cref->start, idx, cref->max);
        return NULL;
    }
    TRACE("Returning %p[%d] => %s\n", 
          cref, idx, debugstr_a(cref->elts[cref->start + idx]));
    return cref->elts[cref->start + idx];
}

/******************************************************************
 *		str_printf
 * Helper for printf type of command (only %s and %c are implemented) 
 * while dynamically allocating the buffer
 */
static char* WINAPIV str_printf(struct parsed_symbol* sym, const char* format, ...)
{
    va_list      args;
    unsigned int len = 1, i, sz;
    char*        tmp;
    char*        p;
    char*        t;

    va_start(args, format);
    for (i = 0; format[i]; i++)
    {
        if (format[i] == '%')
        {
            switch (format[++i])
            {
            case 's': t = va_arg(args, char*); if (t) len += strlen(t); break;
            case 'c': (void)va_arg(args, int); len++; break;
            default: i--; /* fall through */
            case '%': len++; break;
            }
        }
        else len++;
    }
    va_end(args);
    if (!(tmp = und_alloc(sym, len))) return NULL;
    va_start(args, format);
    for (p = tmp, i = 0; format[i]; i++)
    {
        if (format[i] == '%')
        {
            switch (format[++i])
            {
            case 's':
                t = va_arg(args, char*);
                if (t)
                {
                    sz = strlen(t);
                    memcpy(p, t, sz);
                    p += sz;
                }
                break;
            case 'c':
                *p++ = (char)va_arg(args, int);
                break;
            default: i--; /* fall through */
            case '%': *p++ = '%'; break;
            }
        }
        else *p++ = format[i];
    }
    va_end(args);
    *p = '\0';
    return tmp;
}

/* forward declaration */
static BOOL demangle_datatype(struct parsed_symbol* sym, struct datatype_t* ct,
                              struct array* pmt, BOOL in_args);

static const char* get_number(struct parsed_symbol* sym)
{
    char*       ptr;
    BOOL        sgn = FALSE;

    if (*sym->current == '?')
    {
        sgn = TRUE;
        sym->current++;
    }
    if (*sym->current >= '0' && *sym->current <= '8')
    {
        ptr = und_alloc(sym, 3);
        if (sgn) ptr[0] = '-';
        ptr[sgn ? 1 : 0] = *sym->current + 1;
        ptr[sgn ? 2 : 1] = '\0';
        sym->current++;
    }
    else if (*sym->current == '9')
    {
        ptr = und_alloc(sym, 4);
        if (sgn) ptr[0] = '-';
        ptr[sgn ? 1 : 0] = '1';
        ptr[sgn ? 2 : 1] = '0';
        ptr[sgn ? 3 : 2] = '\0';
        sym->current++;
    }
    else if (*sym->current >= 'A' && *sym->current <= 'P')
    {
        int ret = 0;

        while (*sym->current >= 'A' && *sym->current <= 'P')
        {
            ret *= 16;
            ret += *sym->current++ - 'A';
        }
        if (*sym->current != '@') return NULL;

        ptr = und_alloc(sym, 17);
        sprintf(ptr, "%s%u", sgn ? "-" : "", ret);
        sym->current++;
    }
    else return NULL;
    return ptr;
}

/******************************************************************
 *		get_args
 * Parses a list of function/method arguments, creates a string corresponding
 * to the arguments' list.
 */
static char* get_args(struct parsed_symbol* sym, struct array* pmt_ref, BOOL z_term, 
                      char open_char, char close_char)

{
    struct datatype_t   ct;
    struct array        arg_collect;
    char*               args_str = NULL;
    char*               last;
    unsigned int        i;

    str_array_init(&arg_collect);

    /* Now come the function arguments */
    while (*sym->current)
    {
        /* Decode each data type and append it to the argument list */
        if (*sym->current == '@')
        {
            sym->current++;
            break;
        }
        if (!demangle_datatype(sym, &ct, pmt_ref, TRUE))
            return NULL;
        /* 'void' terminates an argument list in a function */
        if (z_term && !strcmp(ct.left, "void")) break;
        if (!str_array_push(sym, str_printf(sym, "%s%s", ct.left, ct.right), -1,
                            &arg_collect))
            return NULL;
        if (!strcmp(ct.left, "...")) break;
    }
    /* Functions are always terminated by 'Z'. If we made it this far and
     * don't find it, we have incorrectly identified a data type.
     */
    if (z_term && *sym->current++ != 'Z') return NULL;

    if (arg_collect.num == 0 || 
        (arg_collect.num == 1 && !strcmp(arg_collect.elts[0], "void")))        
        return str_printf(sym, "%cvoid%c", open_char, close_char);
    for (i = 1; i < arg_collect.num; i++)
    {
        args_str = str_printf(sym, "%s,%s", args_str, arg_collect.elts[i]);
    }

    last = args_str ? args_str : arg_collect.elts[0];
    if (close_char == '>' && last[strlen(last) - 1] == '>')
        args_str = str_printf(sym, "%c%s%s %c", 
                              open_char, arg_collect.elts[0], args_str, close_char);
    else
        args_str = str_printf(sym, "%c%s%s%c", 
                              open_char, arg_collect.elts[0], args_str, close_char);
    
    return args_str;
}

/******************************************************************
 *		get_modifier
 * Parses the type modifier. Always returns static strings.
 */
static BOOL get_modifier(struct parsed_symbol *sym, const char **ret, const char **ptr_modif)
{
    *ptr_modif = NULL;
    if (*sym->current == 'E')
    {
        if (!(sym->flags & UNDNAME_NO_MS_KEYWORDS))
        {
            *ptr_modif = "__ptr64";
            if (sym->flags & UNDNAME_NO_LEADING_UNDERSCORES)
                *ptr_modif = *ptr_modif + 2;
        }
        sym->current++;
    }
    switch (*sym->current++)
    {
    case 'A': *ret = NULL; break;
    case 'B': *ret = "const"; break;
    case 'C': *ret = "volatile"; break;
    case 'D': *ret = "const volatile"; break;
    default: return FALSE;
    }
    return TRUE;
}

static BOOL get_modified_type(struct datatype_t *ct, struct parsed_symbol* sym,
                              struct array *pmt_ref, char modif, BOOL in_args)
{
    const char* modifier;
    const char* str_modif;
    const char *ptr_modif = "";

    if (*sym->current == 'E')
    {
        if (!(sym->flags & UNDNAME_NO_MS_KEYWORDS))
        {
            if (sym->flags & UNDNAME_NO_LEADING_UNDERSCORES)
                ptr_modif = " ptr64";
            else
                ptr_modif = " __ptr64";
        }
        sym->current++;
    }

    switch (modif)
    {
    case 'A': str_modif = str_printf(sym, " &%s", ptr_modif); break;
    case 'B': str_modif = str_printf(sym, " &%s volatile", ptr_modif); break;
    case 'P': str_modif = str_printf(sym, " *%s", ptr_modif); break;
    case 'Q': str_modif = str_printf(sym, " *%s const", ptr_modif); break;
    case 'R': str_modif = str_printf(sym, " *%s volatile", ptr_modif); break;
    case 'S': str_modif = str_printf(sym, " *%s const volatile", ptr_modif); break;
    case '?': str_modif = ""; break;
    default: return FALSE;
    }

    if (get_modifier(sym, &modifier, &ptr_modif))
    {
        unsigned            mark = sym->stack.num;
        struct datatype_t   sub_ct;

        /* multidimensional arrays */
        if (*sym->current == 'Y')
        {
            const char* n1;
            int num;

            sym->current++;
            if (!(n1 = get_number(sym))) return FALSE;
            num = atoi(n1);

            if (str_modif[0] == ' ' && !modifier)
                str_modif++;

            if (modifier)
            {
                str_modif = str_printf(sym, " (%s%s)", modifier, str_modif);
                modifier = NULL;
            }
            else
                str_modif = str_printf(sym, " (%s)", str_modif);

            while (num--)
                str_modif = str_printf(sym, "%s[%s]", str_modif, get_number(sym));
        }

        /* Recurse to get the referred-to type */
        if (!demangle_datatype(sym, &sub_ct, pmt_ref, FALSE))
            return FALSE;
        if (modifier)
            ct->left = str_printf(sym, "%s %s%s", sub_ct.left, modifier, str_modif );
        else
        {
            /* don't insert a space between duplicate '*' */
            if (!in_args && str_modif[0] && str_modif[1] == '*' && sub_ct.left[strlen(sub_ct.left)-1] == '*')
                str_modif++;
            ct->left = str_printf(sym, "%s%s", sub_ct.left, str_modif );
        }
        ct->right = sub_ct.right;
        sym->stack.num = mark;
    }
    return TRUE;
}

/******************************************************************
 *             get_literal_string
 * Gets the literal name from the current position in the mangled
 * symbol to the first '@' character. It pushes the parsed name to
 * the symbol names stack and returns a pointer to it or NULL in
 * case of an error.
 */
static char* get_literal_string(struct parsed_symbol* sym)
{
    const char *ptr = sym->current;

    do {
        if (!((*sym->current >= 'A' && *sym->current <= 'Z') ||
              (*sym->current >= 'a' && *sym->current <= 'z') ||
              (*sym->current >= '0' && *sym->current <= '9') ||
              *sym->current == '_' || *sym->current == '$')) {
            TRACE("Failed at '%c' in %s\n", *sym->current, debugstr_a(ptr));
            return NULL;
        }
    } while (*++sym->current != '@');
    sym->current++;
    if (!str_array_push(sym, ptr, sym->current - 1 - ptr, &sym->names))
        return NULL;

    return str_array_get_ref(&sym->names, sym->names.num - sym->names.start - 1);
}

/******************************************************************
 *		get_template_name
 * Parses a name with a template argument list and returns it as
 * a string.
 * In a template argument list the back reference to the names
 * table is separately created. '0' points to the class component
 * name with the template arguments.  We use the same stack array
 * to hold the names but save/restore the stack state before/after
 * parsing the template argument list.
 */
static char* get_template_name(struct parsed_symbol* sym)
{
    char *name, *args;
    unsigned num_mark = sym->names.num;
    unsigned start_mark = sym->names.start;
    unsigned stack_mark = sym->stack.num;
    struct array array_pmt;

    sym->names.start = sym->names.num;
    if (!(name = get_literal_string(sym))) {
        sym->names.start = start_mark;
        return FALSE;
    }
    str_array_init(&array_pmt);
    args = get_args(sym, &array_pmt, FALSE, '<', '>');
    if (args != NULL)
        name = str_printf(sym, "%s%s", name, args);
    sym->names.num = num_mark;
    sym->names.start = start_mark;
    sym->stack.num = stack_mark;
    return name;
}

/******************************************************************
 *		get_class
 * Parses class as a list of parent-classes, terminated by '@' and stores the
 * result in 'a' array. Each parent-classes, as well as the inner element
 * (either field/method name or class name), are represented in the mangled
 * name by a literal name ([a-zA-Z0-9_]+ terminated by '@') or a back reference
 * ([0-9]) or a name with template arguments ('?$' literal name followed by the
 * template argument list). The class name components appear in the reverse
 * order in the mangled name, e.g aaa@bbb@ccc@@ will be demangled to
 * ccc::bbb::aaa
 * For each of these class name components a string will be allocated in the
 * array.
 */
static BOOL get_class(struct parsed_symbol* sym)
{
    const char* name = NULL;

    while (*sym->current != '@')
    {
        switch (*sym->current)
        {
        case '\0': return FALSE;

        case '0': case '1': case '2': case '3':
        case '4': case '5': case '6': case '7':
        case '8': case '9':
            name = str_array_get_ref(&sym->names, *sym->current++ - '0');
            break;
        case '?':
            switch (*++sym->current)
            {
            case '$':
                sym->current++;
                if ((name = get_template_name(sym)) &&
                    !str_array_push(sym, name, -1, &sym->names))
                    return FALSE;
                break;
            case '?':
                {
                    struct array stack = sym->stack;
                    unsigned int start = sym->names.start;
                    unsigned int num = sym->names.num;

                    str_array_init( &sym->stack );
                    if (symbol_demangle( sym )) name = str_printf( sym, "`%s'", sym->result );
                    sym->names.start = start;
                    sym->names.num = num;
                    sym->stack = stack;
                }
                break;
            default:
                if (!(name = get_number( sym ))) return FALSE;
                name = str_printf( sym, "`%s'", name );
                break;
            }
            break;
        default:
            name = get_literal_string(sym);
            break;
        }
        if (!name || !str_array_push(sym, name, -1, &sym->stack))
            return FALSE;
    }
    sym->current++;
    return TRUE;
}

/******************************************************************
 *		get_class_string
 * From an array collected by get_class in sym->stack, constructs the
 * corresponding (allocated) string
 */
static char* get_class_string(struct parsed_symbol* sym, int start)
{
    int          i;
    unsigned int len, sz;
    char*        ret;
    struct array *a = &sym->stack;

    for (len = 0, i = start; i < a->num; i++)
    {
        assert(a->elts[i]);
        len += 2 + strlen(a->elts[i]);
    }
    if (!(ret = und_alloc(sym, len - 1))) return NULL;
    for (len = 0, i = a->num - 1; i >= start; i--)
    {
        sz = strlen(a->elts[i]);
        memcpy(ret + len, a->elts[i], sz);
        len += sz;
        if (i > start)
        {
            ret[len++] = ':';
            ret[len++] = ':';
        }
    }
    ret[len] = '\0';
    return ret;
}

/******************************************************************
 *            get_class_name
 * Wrapper around get_class and get_class_string.
 */
static char* get_class_name(struct parsed_symbol* sym)
{
    unsigned    mark = sym->stack.num;
    char*       s = NULL;

    if (get_class(sym))
        s = get_class_string(sym, mark);
    sym->stack.num = mark;
    return s;
}

/******************************************************************
 *		get_calling_convention
 * Returns a static string corresponding to the calling convention described
 * by char 'ch'. Sets export to TRUE iff the calling convention is exported.
 */
static BOOL get_calling_convention(char ch, const char** call_conv,
                                   const char** exported, unsigned flags)
{
    *call_conv = *exported = NULL;

    if (!(flags & (UNDNAME_NO_MS_KEYWORDS | UNDNAME_NO_ALLOCATION_LANGUAGE)))
    {
        if (flags & UNDNAME_NO_LEADING_UNDERSCORES)
        {
            if (((ch - 'A') % 2) == 1) *exported = "dll_export ";
            switch (ch)
            {
            case 'A': case 'B': *call_conv = "cdecl"; break;
            case 'C': case 'D': *call_conv = "pascal"; break;
            case 'E': case 'F': *call_conv = "thiscall"; break;
            case 'G': case 'H': *call_conv = "stdcall"; break;
            case 'I': case 'J': *call_conv = "fastcall"; break;
            case 'K': case 'L': break;
            case 'M': *call_conv = "clrcall"; break;
            default: ERR("Unknown calling convention %c\n", ch); return FALSE;
            }
        }
        else
        {
            if (((ch - 'A') % 2) == 1) *exported = "__dll_export ";
            switch (ch)
            {
            case 'A': case 'B': *call_conv = "__cdecl"; break;
            case 'C': case 'D': *call_conv = "__pascal"; break;
            case 'E': case 'F': *call_conv = "__thiscall"; break;
            case 'G': case 'H': *call_conv = "__stdcall"; break;
            case 'I': case 'J': *call_conv = "__fastcall"; break;
            case 'K': case 'L': break;
            case 'M': *call_conv = "__clrcall"; break;
            default: ERR("Unknown calling convention %c\n", ch); return FALSE;
            }
        }
    }
    return TRUE;
}

/*******************************************************************
 *         get_simple_type
 * Return a string containing an allocated string for a simple data type
 */
static const char* get_simple_type(char c)
{
    const char* type_string;
    
    switch (c)
    {
    case 'C': type_string = "signed char"; break;
    case 'D': type_string = "char"; break;
    case 'E': type_string = "unsigned char"; break;
    case 'F': type_string = "short"; break;
    case 'G': type_string = "unsigned short"; break;
    case 'H': type_string = "int"; break;
    case 'I': type_string = "unsigned int"; break;
    case 'J': type_string = "long"; break;
    case 'K': type_string = "unsigned long"; break;
    case 'M': type_string = "float"; break;
    case 'N': type_string = "double"; break;
    case 'O': type_string = "long double"; break;
    case 'X': type_string = "void"; break;
    case 'Z': type_string = "..."; break;
    default:  type_string = NULL; break;
    }
    return type_string;
}

/*******************************************************************
 *         get_extended_type
 * Return a string containing an allocated string for a simple data type
 */
static const char* get_extended_type(char c)
{
    const char* type_string;
    
    switch (c)
    {
    case 'D': type_string = "__int8"; break;
    case 'E': type_string = "unsigned __int8"; break;
    case 'F': type_string = "__int16"; break;
    case 'G': type_string = "unsigned __int16"; break;
    case 'H': type_string = "__int32"; break;
    case 'I': type_string = "unsigned __int32"; break;
    case 'J': type_string = "__int64"; break;
    case 'K': type_string = "unsigned __int64"; break;
    case 'L': type_string = "__int128"; break;
    case 'M': type_string = "unsigned __int128"; break;
    case 'N': type_string = "bool"; break;
    case 'W': type_string = "wchar_t"; break;
    default:  type_string = NULL; break;
    }
    return type_string;
}

/*******************************************************************
 *         demangle_datatype
 *
 * Attempt to demangle a C++ data type, which may be datatype.
 * a datatype type is made up of a number of simple types. e.g:
 * char** = (pointer to (pointer to (char)))
 */
static BOOL demangle_datatype(struct parsed_symbol* sym, struct datatype_t* ct,
                              struct array* pmt_ref, BOOL in_args)
{
    char                dt;
    BOOL                add_pmt = TRUE;

    assert(ct);
    ct->left = ct->right = NULL;
    
    switch (dt = *sym->current++)
    {
    case '_':
        /* MS type: __int8,__int16 etc */
        ct->left = get_extended_type(*sym->current++);
        break;
    case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'M':
    case 'N': case 'O': case 'X': case 'Z':
        /* Simple data types */
        ct->left = get_simple_type(dt);
        add_pmt = FALSE;
        break;
    case 'T': /* union */
    case 'U': /* struct */
    case 'V': /* class */
    case 'Y': /* cointerface */
        /* Class/struct/union/cointerface */
        {
            const char* struct_name = NULL;
            const char* type_name = NULL;

            if (!(struct_name = get_class_name(sym)))
                goto done;
            if (!(sym->flags & UNDNAME_NO_COMPLEX_TYPE)) 
            {
                switch (dt)
                {
                case 'T': type_name = "union ";  break;
                case 'U': type_name = "struct "; break;
                case 'V': type_name = "class ";  break;
                case 'Y': type_name = "cointerface "; break;
                }
            }
            ct->left = str_printf(sym, "%s%s", type_name, struct_name);
        }
        break;
    case '?':
        /* not all the time is seems */
        if (in_args)
        {
            const char*   ptr;
            if (!(ptr = get_number(sym))) goto done;
            ct->left = str_printf(sym, "`template-parameter-%s'", ptr);
        }
        else
        {
            if (!get_modified_type(ct, sym, pmt_ref, '?', in_args)) goto done;
        }
        break;
    case 'A': /* reference */
    case 'B': /* volatile reference */
        if (!get_modified_type(ct, sym, pmt_ref, dt, in_args)) goto done;
        break;
    case 'Q': /* const pointer */
    case 'R': /* volatile pointer */
    case 'S': /* const volatile pointer */
        if (!get_modified_type(ct, sym, pmt_ref, in_args ? dt : 'P', in_args)) goto done;
        break;
    case 'P': /* Pointer */
        if (isdigit(*sym->current))
	{
            /* FIXME:
             *   P6 = Function pointer
             *   P8 = Member function pointer
             *   others who knows.. */
            if (*sym->current == '8')
            {
                char*                   args = NULL;
                const char*             call_conv;
                const char*             exported;
                struct datatype_t       sub_ct;
                unsigned                mark = sym->stack.num;
                const char*             class;
                const char*             modifier;
                const char*             ptr_modif;

                sym->current++;

                if (!(class = get_class_name(sym)))
                    goto done;
                if (!get_modifier(sym, &modifier, &ptr_modif))
                    goto done;
                if (modifier)
                    modifier = str_printf(sym, "%s %s", modifier, ptr_modif);
                else if(ptr_modif)
                    modifier = str_printf(sym, " %s", ptr_modif);
                if (!get_calling_convention(*sym->current++,
                            &call_conv, &exported,
                            sym->flags & ~UNDNAME_NO_ALLOCATION_LANGUAGE))
                    goto done;
                if (!demangle_datatype(sym, &sub_ct, pmt_ref, FALSE))
                    goto done;

                args = get_args(sym, pmt_ref, TRUE, '(', ')');
                if (!args) goto done;
                sym->stack.num = mark;

                ct->left  = str_printf(sym, "%s%s (%s %s::*",
                        sub_ct.left, sub_ct.right, call_conv, class);
                ct->right = str_printf(sym, ")%s%s", args, modifier);
            }
            else if (*sym->current == '6')
            {
                char*                   args = NULL;
                const char*             call_conv;
                const char*             exported;
                struct datatype_t       sub_ct;
                unsigned                mark = sym->stack.num;

                sym->current++;

                if (!get_calling_convention(*sym->current++,
                                            &call_conv, &exported, 
                                            sym->flags & ~UNDNAME_NO_ALLOCATION_LANGUAGE) ||
                    !demangle_datatype(sym, &sub_ct, pmt_ref, FALSE))
                    goto done;

                args = get_args(sym, pmt_ref, TRUE, '(', ')');
                if (!args) goto done;
                sym->stack.num = mark;

                ct->left  = str_printf(sym, "%s%s (%s*", 
                                       sub_ct.left, sub_ct.right, call_conv);
                ct->right = str_printf(sym, ")%s", args);
            }
            else goto done;
	}
	else if (!get_modified_type(ct, sym, pmt_ref, 'P', in_args)) goto done;
        break;
    case 'W':
        if (*sym->current == '4')
        {
            char*               enum_name;
            sym->current++;
            if (!(enum_name = get_class_name(sym)))
                goto done;
            if (sym->flags & UNDNAME_NO_COMPLEX_TYPE)
                ct->left = enum_name;
            else
                ct->left = str_printf(sym, "enum %s", enum_name);
        }
        else goto done;
        break;
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
        /* Referring back to previously parsed type */
        /* left and right are pushed as two separate strings */
        if (!pmt_ref) goto done;
        ct->left = str_array_get_ref(pmt_ref, (dt - '0') * 2);
        ct->right = str_array_get_ref(pmt_ref, (dt - '0') * 2 + 1);
        if (!ct->left) goto done;
        add_pmt = FALSE;
        break;
    case '$':
        switch (*sym->current++)
        {
        case '0':
            if (!(ct->left = get_number(sym))) goto done;
            break;
        case 'D':
            {
                const char*   ptr;
                if (!(ptr = get_number(sym))) goto done;
                ct->left = str_printf(sym, "`template-parameter%s'", ptr);
            }
            break;
        case 'F':
            {
                const char*   p1;
                const char*   p2;
                if (!(p1 = get_number(sym))) goto done;
                if (!(p2 = get_number(sym))) goto done;
                ct->left = str_printf(sym, "{%s,%s}", p1, p2);
            }
            break;
        case 'G':
            {
                const char*   p1;
                const char*   p2;
                const char*   p3;
                if (!(p1 = get_number(sym))) goto done;
                if (!(p2 = get_number(sym))) goto done;
                if (!(p3 = get_number(sym))) goto done;
                ct->left = str_printf(sym, "{%s,%s,%s}", p1, p2, p3);
            }
            break;
        case 'Q':
            {
                const char*   ptr;
                if (!(ptr = get_number(sym))) goto done;
                ct->left = str_printf(sym, "`non-type-template-parameter%s'", ptr);
            }
            break;
        case '$':
            if (*sym->current == 'B')
            {
                unsigned            mark = sym->stack.num;
                struct datatype_t   sub_ct;
                const char*         arr = NULL;
                sym->current++;

                /* multidimensional arrays */
                if (*sym->current == 'Y')
                {
                    const char* n1;
                    int num;

                    sym->current++;
                    if (!(n1 = get_number(sym))) goto done;
                    num = atoi(n1);

                    while (num--)
                        arr = str_printf(sym, "%s[%s]", arr, get_number(sym));
                }

                if (!demangle_datatype(sym, &sub_ct, pmt_ref, FALSE)) goto done;

                if (arr)
                    ct->left = str_printf(sym, "%s %s", sub_ct.left, arr);
                else
                    ct->left = sub_ct.left;
                ct->right = sub_ct.right;
                sym->stack.num = mark;
            }
            else if (*sym->current == 'C')
            {
                const char *ptr, *ptr_modif;

                sym->current++;
                if (!get_modifier(sym, &ptr, &ptr_modif)) goto done;
                if (!demangle_datatype(sym, ct, pmt_ref, in_args)) goto done;
                ct->left = str_printf(sym, "%s %s", ct->left, ptr);
            }
            break;
        }
        break;
    default :
        ERR("Unknown type %c\n", dt);
        break;
    }
    if (add_pmt && pmt_ref && in_args)
    {
        /* left and right are pushed as two separate strings */
        if (!str_array_push(sym, ct->left ? ct->left : "", -1, pmt_ref) ||
            !str_array_push(sym, ct->right ? ct->right : "", -1, pmt_ref))
            return FALSE;
    }
done:
    
    return ct->left != NULL;
}

/******************************************************************
 *		handle_data
 * Does the final parsing and handling for a variable or a field in
 * a class.
 */
static BOOL handle_data(struct parsed_symbol* sym)
{
    const char*         access = NULL;
    const char*         member_type = NULL;
    const char*         modifier = NULL;
    const char*         ptr_modif;
    struct datatype_t   ct;
    char*               name = NULL;
    BOOL                ret = FALSE;

    /* 0 private static
     * 1 protected static
     * 2 public static
     * 3 private non-static
     * 4 protected non-static
     * 5 public non-static
     * 6 ?? static
     * 7 ?? static
     */

    if (!(sym->flags & UNDNAME_NO_ACCESS_SPECIFIERS))
    {
        /* we only print the access for static members */
        switch (*sym->current)
        {
        case '0': access = "private: "; break;
        case '1': access = "protected: "; break;
        case '2': access = "public: "; break;
        } 
    }

    if (!(sym->flags & UNDNAME_NO_MEMBER_TYPE))
    {
        if (*sym->current >= '0' && *sym->current <= '2')
            member_type = "static ";
    }

    name = get_class_string(sym, 0);

    switch (*sym->current++)
    {
    case '0': case '1': case '2':
    case '3': case '4': case '5':
        {
            unsigned mark = sym->stack.num;
            struct array pmt;

            str_array_init(&pmt);

            if (!demangle_datatype(sym, &ct, &pmt, FALSE)) goto done;
            if (!get_modifier(sym, &modifier, &ptr_modif)) goto done;
            if (modifier && ptr_modif) modifier = str_printf(sym, "%s %s", modifier, ptr_modif);
            else if (!modifier) modifier = ptr_modif;
            sym->stack.num = mark;
        }
        break;
    case '6' : /* compiler generated static */
    case '7' : /* compiler generated static */
        ct.left = ct.right = NULL;
        if (!get_modifier(sym, &modifier, &ptr_modif)) goto done;
        if (*sym->current != '@')
        {
            char*       cls = NULL;

            if (!(cls = get_class_name(sym)))
                goto done;
            ct.right = str_printf(sym, "{for `%s'}", cls);
        }
        break;
    case '8':
    case '9':
        modifier = ct.left = ct.right = NULL;
        break;
    default: goto done;
    }
    if (sym->flags & UNDNAME_NAME_ONLY) ct.left = ct.right = modifier = NULL;

    sym->result = str_printf(sym, "%s%s%s%s%s%s%s%s", access,
                             member_type, ct.left, 
                             modifier && ct.left ? " " : NULL, modifier, 
                             modifier || ct.left ? " " : NULL, name, ct.right);
    ret = TRUE;
done:
    return ret;
}

/******************************************************************
 *		handle_method
 * Does the final parsing and handling for a function or a method in
 * a class.
 */
static BOOL handle_method(struct parsed_symbol* sym, BOOL cast_op)
{
    char                accmem;
    const char*         access = NULL;
    int                 access_id = -1;
    const char*         member_type = NULL;
    struct datatype_t   ct_ret;
    const char*         call_conv;
    const char*         modifier = NULL;
    const char*         exported;
    const char*         args_str = NULL;
    const char*         name = NULL;
    BOOL                ret = FALSE, has_args = TRUE, has_ret = TRUE;
    unsigned            mark;
    struct array        array_pmt;

    /* FIXME: why 2 possible letters for each option?
     * 'A' private:
     * 'B' private:
     * 'C' private: static
     * 'D' private: static
     * 'E' private: virtual
     * 'F' private: virtual
     * 'G' private: thunk
     * 'H' private: thunk
     * 'I' protected:
     * 'J' protected:
     * 'K' protected: static
     * 'L' protected: static
     * 'M' protected: virtual
     * 'N' protected: virtual
     * 'O' protected: thunk
     * 'P' protected: thunk
     * 'Q' public:
     * 'R' public:
     * 'S' public: static
     * 'T' public: static
     * 'U' public: virtual
     * 'V' public: virtual
     * 'W' public: thunk
     * 'X' public: thunk
     * 'Y'
     * 'Z'
     * "$0" private: thunk vtordisp
     * "$1" private: thunk vtordisp
     * "$2" protected: thunk vtordisp
     * "$3" protected: thunk vtordisp
     * "$4" public: thunk vtordisp
     * "$5" public: thunk vtordisp
     * "$B" vcall thunk
     * "$R" thunk vtordispex
     */
    accmem = *sym->current++;
    if (accmem == '$')
    {
        if (*sym->current >= '0' && *sym->current <= '5')
            access_id = (*sym->current - '0') / 2;
        else if (*sym->current == 'R')
            access_id = (sym->current[1] - '0') / 2;
        else if (*sym->current != 'B')
            goto done;
    }
    else if (accmem >= 'A' && accmem <= 'Z')
        access_id = (accmem - 'A') / 8;
    else
        goto done;

    switch (access_id)
    {
    case 0: access = "private: "; break;
    case 1: access = "protected: "; break;
    case 2: access = "public: "; break;
    }
    if (accmem == '$' || (accmem - 'A') % 8 == 6 || (accmem - 'A') % 8 == 7)
        access = str_printf(sym, "[thunk]:%s", access ? access : " ");

    if (accmem == '$' && *sym->current != 'B')
        member_type = "virtual ";
    else if (accmem <= 'X')
    {
        switch ((accmem - 'A') % 8)
        {
        case 2: case 3: member_type = "static "; break;
        case 4: case 5: case 6: case 7: member_type = "virtual "; break;
        }
    }

    if (sym->flags & UNDNAME_NO_ACCESS_SPECIFIERS)
        access = NULL;
    if (sym->flags & UNDNAME_NO_MEMBER_TYPE)
        member_type = NULL;

    name = get_class_string(sym, 0);

    if (accmem == '$' && *sym->current == 'B') /* vcall thunk */
    {
        const char *n;

        sym->current++;
        n = get_number(sym);

        if(!n || *sym->current++ != 'A') goto done;
        name = str_printf(sym, "%s{%s,{flat}}' }'", name, n);
        has_args = FALSE;
        has_ret = FALSE;
    }
    else if (accmem == '$' && *sym->current == 'R') /* vtordispex thunk */
    {
        const char *n1, *n2, *n3, *n4;

        sym->current += 2;
        n1 = get_number(sym);
        n2 = get_number(sym);
        n3 = get_number(sym);
        n4 = get_number(sym);

        if(!n1 || !n2 || !n3 || !n4) goto done;
        name = str_printf(sym, "%s`vtordispex{%s,%s,%s,%s}' ", name, n1, n2, n3, n4);
    }
    else if (accmem == '$') /* vtordisp thunk */
    {
        const char *n1, *n2;

        sym->current++;
        n1 = get_number(sym);
        n2 = get_number(sym);

        if (!n1 || !n2) goto done;
        name = str_printf(sym, "%s`vtordisp{%s,%s}' ", name, n1, n2);
    }
    else if ((accmem - 'A') % 8 == 6 || (accmem - 'A') % 8 == 7) /* a thunk */
        name = str_printf(sym, "%s`adjustor{%s}' ", name, get_number(sym));

    if (has_args && (accmem == '$' ||
                (accmem <= 'X' && (accmem - 'A') % 8 != 2 && (accmem - 'A') % 8 != 3)))
    {
        const char *ptr_modif;
        /* Implicit 'this' pointer */
        /* If there is an implicit this pointer, const modifier follows */
        if (!get_modifier(sym, &modifier, &ptr_modif)) goto done;
        if (modifier || ptr_modif) modifier = str_printf(sym, "%s %s", modifier, ptr_modif);
    }

    if (!get_calling_convention(*sym->current++, &call_conv, &exported,
                                sym->flags))
        goto done;

    str_array_init(&array_pmt);

    /* Return type, or @ if 'void' */
    if (has_ret && *sym->current == '@')
    {
        ct_ret.left = "void";
        ct_ret.right = NULL;
        sym->current++;
    }
    else if (has_ret)
    {
        if (!demangle_datatype(sym, &ct_ret, &array_pmt, FALSE))
            goto done;
    }
    if (!has_ret || sym->flags & UNDNAME_NO_FUNCTION_RETURNS)
        ct_ret.left = ct_ret.right = NULL;
    if (cast_op)
    {
        name = str_printf(sym, "%s%s%s", name, ct_ret.left, ct_ret.right);
        ct_ret.left = ct_ret.right = NULL;
    }

    mark = sym->stack.num;
    if (has_args && !(args_str = get_args(sym, &array_pmt, TRUE, '(', ')'))) goto done;
    if (sym->flags & UNDNAME_NAME_ONLY) args_str = modifier = NULL;
    if (sym->flags & UNDNAME_NO_THISTYPE) modifier = NULL;
    sym->stack.num = mark;

    /* Note: '()' after 'Z' means 'throws', but we don't care here
     * Yet!!! FIXME
     */
    sym->result = str_printf(sym, "%s%s%s%s%s%s%s%s%s%s%s",
                             access, member_type, ct_ret.left,
                             (ct_ret.left && !ct_ret.right) ? " " : NULL,
                             call_conv, call_conv ? " " : NULL, exported,
                             name, args_str, modifier, ct_ret.right);
    ret = TRUE;
done:
    return ret;
}

/*******************************************************************
 *         symbol_demangle
 * Demangle a C++ linker symbol
 */
static BOOL symbol_demangle(struct parsed_symbol* sym)
{
    BOOL                ret = FALSE;
    unsigned            do_after = 0;
    static CHAR         dashed_null[] = "--null--";

    /* FIXME seems wrong as name, as it demangles a simple data type */
    if (sym->flags & UNDNAME_NO_ARGUMENTS)
    {
        struct datatype_t   ct;

        if (demangle_datatype(sym, &ct, NULL, FALSE))
        {
            sym->result = str_printf(sym, "%s%s", ct.left, ct.right);
            ret = TRUE;
        }
        goto done;
    }

    /* MS mangled names always begin with '?' */
    if (*sym->current != '?') return FALSE;
    sym->current++;

    /* Then function name or operator code */
    if (*sym->current == '?' && (sym->current[1] != '$' || sym->current[2] == '?'))
    {
        const char* function_name = NULL;

        if (sym->current[1] == '$')
        {
            do_after = 6;
            sym->current += 2;
        }

        /* C++ operator code (one character, or two if the first is '_') */
        switch (*++sym->current)
        {
        case '0': do_after = 1; break;
        case '1': do_after = 2; break;
        case '2': function_name = "operator new"; break;
        case '3': function_name = "operator delete"; break;
        case '4': function_name = "operator="; break;
        case '5': function_name = "operator>>"; break;
        case '6': function_name = "operator<<"; break;
        case '7': function_name = "operator!"; break;
        case '8': function_name = "operator=="; break;
        case '9': function_name = "operator!="; break;
        case 'A': function_name = "operator[]"; break;
        case 'B': function_name = "operator "; do_after = 3; break;
        case 'C': function_name = "operator->"; break;
        case 'D': function_name = "operator*"; break;
        case 'E': function_name = "operator++"; break;
        case 'F': function_name = "operator--"; break;
        case 'G': function_name = "operator-"; break;
        case 'H': function_name = "operator+"; break;
        case 'I': function_name = "operator&"; break;
        case 'J': function_name = "operator->*"; break;
        case 'K': function_name = "operator/"; break;
        case 'L': function_name = "operator%"; break;
        case 'M': function_name = "operator<"; break;
        case 'N': function_name = "operator<="; break;
        case 'O': function_name = "operator>"; break;
        case 'P': function_name = "operator>="; break;
        case 'Q': function_name = "operator,"; break;
        case 'R': function_name = "operator()"; break;
        case 'S': function_name = "operator~"; break;
        case 'T': function_name = "operator^"; break;
        case 'U': function_name = "operator|"; break;
        case 'V': function_name = "operator&&"; break;
        case 'W': function_name = "operator||"; break;
        case 'X': function_name = "operator*="; break;
        case 'Y': function_name = "operator+="; break;
        case 'Z': function_name = "operator-="; break;
        case '_':
            switch (*++sym->current)
            {
            case '0': function_name = "operator/="; break;
            case '1': function_name = "operator%="; break;
            case '2': function_name = "operator>>="; break;
            case '3': function_name = "operator<<="; break;
            case '4': function_name = "operator&="; break;
            case '5': function_name = "operator|="; break;
            case '6': function_name = "operator^="; break;
            case '7': function_name = "`vftable'"; break;
            case '8': function_name = "`vbtable'"; break;
            case '9': function_name = "`vcall'"; break;
            case 'A': function_name = "`typeof'"; break;
            case 'B': function_name = "`local static guard'"; break;
            case 'C': function_name = "`string'"; do_after = 4; break;
            case 'D': function_name = "`vbase destructor'"; break;
            case 'E': function_name = "`vector deleting destructor'"; break;
            case 'F': function_name = "`default constructor closure'"; break;
            case 'G': function_name = "`scalar deleting destructor'"; break;
            case 'H': function_name = "`vector constructor iterator'"; break;
            case 'I': function_name = "`vector destructor iterator'"; break;
            case 'J': function_name = "`vector vbase constructor iterator'"; break;
            case 'K': function_name = "`virtual displacement map'"; break;
            case 'L': function_name = "`eh vector constructor iterator'"; break;
            case 'M': function_name = "`eh vector destructor iterator'"; break;
            case 'N': function_name = "`eh vector vbase constructor iterator'"; break;
            case 'O': function_name = "`copy constructor closure'"; break;
            case 'R':
                sym->flags |= UNDNAME_NO_FUNCTION_RETURNS;
                switch (*++sym->current)
                {
                case '0':
                    {
                        struct datatype_t       ct;
                        struct array pmt;

                        sym->current++;
                        str_array_init(&pmt);
                        demangle_datatype(sym, &ct, &pmt, FALSE);
                        if (!demangle_datatype(sym, &ct, NULL, FALSE))
                            goto done;
                        function_name = str_printf(sym, "%s%s `RTTI Type Descriptor'",
                                                   ct.left, ct.right);
                        sym->current--;
                    }
                    break;
                case '1':
                    {
                        const char* n1, *n2, *n3, *n4;
                        sym->current++;
                        n1 = get_number(sym);
                        n2 = get_number(sym);
                        n3 = get_number(sym);
                        n4 = get_number(sym);
                        sym->current--;
                        function_name = str_printf(sym, "`RTTI Base Class Descriptor at (%s,%s,%s,%s)'",
                                                   n1, n2, n3, n4);
                    }
                    break;
                case '2': function_name = "`RTTI Base Class Array'"; break;
                case '3': function_name = "`RTTI Class Hierarchy Descriptor'"; break;
                case '4': function_name = "`RTTI Complete Object Locator'"; break;
                default:
                    ERR("Unknown RTTI operator: _R%c\n", *sym->current);
                    break;
                }
                break;
            case 'S': function_name = "`local vftable'"; break;
            case 'T': function_name = "`local vftable constructor closure'"; break;
            case 'U': function_name = "operator new[]"; break;
            case 'V': function_name = "operator delete[]"; break;
            case 'X': function_name = "`placement delete closure'"; break;
            case 'Y': function_name = "`placement delete[] closure'"; break;
            default:
                ERR("Unknown operator: _%c\n", *sym->current);
                return FALSE;
            }
            break;
        default:
            /* FIXME: Other operators */
            ERR("Unknown operator: %c\n", *sym->current);
            return FALSE;
        }
        sym->current++;
        switch (do_after)
        {
        case 1: case 2:
            if (!str_array_push(sym, dashed_null, -1, &sym->stack))
                return FALSE;
            break;
        case 4:
            sym->result = (char*)function_name;
            ret = TRUE;
            goto done;
        case 6:
            {
                char *args;
                struct array array_pmt;

                str_array_init(&array_pmt);
                args = get_args(sym, &array_pmt, FALSE, '<', '>');
                if (args != NULL) function_name = str_printf(sym, "%s%s", function_name, args);
                sym->names.num = 0;
            }
            /* fall through */
        default:
            if (!str_array_push(sym, function_name, -1, &sym->stack))
                return FALSE;
            break;
        }
    }
    else if (*sym->current == '$')
    {
        /* Strange construct, it's a name with a template argument list
           and that's all. */
        sym->current++;
        ret = (sym->result = get_template_name(sym)) != NULL;
        goto done;
    }
    else if (*sym->current == '?' && sym->current[1] == '$')
        do_after = 5;

    /* Either a class name, or '@' if the symbol is not a class member */
    switch (*sym->current)
    {
    case '@': sym->current++; break;
    case '$': break;
    default:
        /* Class the function is associated with, terminated by '@@' */
        if (!get_class(sym)) goto done;
        break;
    }

    switch (do_after)
    {
    case 0: default: break;
    case 1: case 2:
        /* it's time to set the member name for ctor & dtor */
        if (sym->stack.num <= 1) goto done;
        if (do_after == 1)
            sym->stack.elts[0] = sym->stack.elts[1];
        else
            sym->stack.elts[0] = str_printf(sym, "~%s", sym->stack.elts[1]);
        /* ctors and dtors don't have return type */
        sym->flags |= UNDNAME_NO_FUNCTION_RETURNS;
        break;
    case 3:
        sym->flags &= ~UNDNAME_NO_FUNCTION_RETURNS;
        break;
    case 5:
        sym->names.start++;
        break;
    }

    /* Function/Data type and access level */
    if (*sym->current >= '0' && *sym->current <= '9')
        ret = handle_data(sym);
    else if ((*sym->current >= 'A' && *sym->current <= 'Z') || *sym->current == '$')
        ret = handle_method(sym, do_after == 3);
    else ret = FALSE;
done:
    if (ret) assert(sym->result);
    else WARN("Failed at %s\n", debugstr_a(sym->current));

    return ret;
}

/*********************************************************************
 *		__unDNameEx (MSVCRT.@)
 *
 * Demangle a C++ identifier.
 *
 * PARAMS
 *  buffer   [O] If not NULL, the place to put the demangled string
 *  mangled  [I] Mangled name of the function
 *  buflen   [I] Length of buffer
 *  memget   [I] Function to allocate memory with
 *  memfree  [I] Function to free memory with
 *  unknown  [?] Unknown, possibly a call back
 *  flags    [I] Flags determining demangled format
 *
 * RETURNS
 *  Success: A string pointing to the unmangled name, allocated with memget.
 *  Failure: NULL.
 */
char* CDECL __unDNameEx(char* buffer, const char* mangled, int buflen,
                        malloc_func_t memget, free_func_t memfree,
                        void* unknown, unsigned short int flags)
{
    struct parsed_symbol        sym;
    const char*                 result;

    TRACE("(%p,%s,%d,%p,%p,%p,%x)\n",
          buffer, debugstr_a(mangled), buflen, memget, memfree, unknown, flags);
    
    /* The flags details is not documented by MS. However, it looks exactly
     * like the UNDNAME_ manifest constants from imagehlp.h and dbghelp.h
     * So, we copied those (on top of the file)
     */
    memset(&sym, 0, sizeof(struct parsed_symbol));
    if (flags & UNDNAME_NAME_ONLY)
        flags |= UNDNAME_NO_FUNCTION_RETURNS | UNDNAME_NO_ACCESS_SPECIFIERS |
            UNDNAME_NO_MEMBER_TYPE | UNDNAME_NO_ALLOCATION_LANGUAGE |
            UNDNAME_NO_COMPLEX_TYPE;

    sym.flags         = flags;
    sym.mem_alloc_ptr = memget;
    sym.mem_free_ptr  = memfree;
    sym.current       = mangled;
    str_array_init( &sym.names );
    str_array_init( &sym.stack );

    result = symbol_demangle(&sym) ? sym.result : mangled;
    if (buffer && buflen)
    {
        lstrcpynA( buffer, result, buflen);
    }
    else
    {
        buffer = memget(strlen(result) + 1);
        if (buffer) strcpy(buffer, result);
    }

    und_free_all(&sym);

    return buffer;
}


/*********************************************************************
 *		__unDName (MSVCRT.@)
 */
char* CDECL __unDName(char* buffer, const char* mangled, int buflen,
                      malloc_func_t memget, free_func_t memfree,
                      unsigned short int flags)
{
    return __unDNameEx(buffer, mangled, buflen, memget, memfree, NULL, flags);
}
