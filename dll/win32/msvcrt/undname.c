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
#include "winver.h"
#include "imagehlp.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

/* How data types qualifiers are stored:
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

#define UNDNAME_NO_COMPLEX_TYPE (0x8000)

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
    struct array        args;           /* array of arguments for back reference */
    struct array        stack;          /* stack of parsed strings */

    void*               alloc_list;     /* linked list of allocated blocks */
    unsigned            avail_in_first; /* number of available bytes in head block */
};

enum datatype_e
{
    DT_NO_LEADING_WS = 0x01,
    DT_NO_LRSEP_WS = 0x02,
};

/* Type for parsing mangled types */
struct datatype_t
{
    const char*         left;
    const char*         right;
    enum datatype_e     flags;
};

static BOOL symbol_demangle(struct parsed_symbol* sym);
static char* get_class_name(struct parsed_symbol* sym);

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

enum datatype_flags
{
    IN_ARGS = 0x01,
    WS_AFTER_QUAL_IF = 0x02,
};

/* forward declaration */
static BOOL demangle_datatype(struct parsed_symbol* sym, struct datatype_t* ct,
                              enum datatype_flags flags);

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
static char* get_args(struct parsed_symbol* sym, BOOL z_term,
                      char open_char, char close_char)

{
    struct datatype_t   ct;
    struct array        arg_collect;
    char*               args_str = NULL;
    char*               last;
    unsigned int        i;
    const char *p;

    if (z_term && sym->current[0] == 'X' && sym->current[1] == 'Z')
    {
        sym->current += 2;
        return str_printf(sym, "%cvoid%c", open_char, close_char);
    }

    str_array_init(&arg_collect);

    /* Now come the function arguments */
    while (*sym->current)
    {
        p = sym->current;

        /* Decode each data type and append it to the argument list */
        if (*sym->current == '@')
        {
            sym->current++;
            break;
        }
        if (z_term && sym->current[0] == 'Z')
        {
            sym->current++;
            if (!str_array_push(sym, "...", -1, &arg_collect))
                return NULL;
            break;
        }
        /* Handle empty list in variadic template */
        if (!z_term && sym->current[0] == '$' && sym->current[1] == '$' && sym->current[2] == 'V')
        {
            sym->current += 3;
            continue;
        }
        if (!demangle_datatype(sym, &ct, IN_ARGS))
            return NULL;
        if (!str_array_push(sym, str_printf(sym, "%s%s", ct.left, ct.right), -1,
                            &arg_collect))
            return NULL;
        if (z_term && sym->current - p > 1 && sym->args.num < 20)
        {
            if (!str_array_push(sym, ct.left ? ct.left : "", -1, &sym->args) ||
                    !str_array_push(sym, ct.right ? ct.right : "", -1, &sym->args))
                return NULL;
        }
    }
    /* Functions are always terminated by 'Z'. If we made it this far and
     * don't find it, we have incorrectly identified a data type.
     */
    if (z_term && *sym->current++ != 'Z') return NULL;

    if (!arg_collect.num) return str_printf(sym, "%c%c", open_char, close_char);
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

static void append_extended_qualifier(struct parsed_symbol *sym, const char **where,
                                      const char *str, BOOL is_ms_keyword)
{
    if (!is_ms_keyword || !(sym->flags & UNDNAME_NO_MS_KEYWORDS))
    {
        if (is_ms_keyword && (sym->flags & UNDNAME_NO_LEADING_UNDERSCORES))
            str += 2;
        *where = *where ? str_printf(sym, "%s%s%s%s", *where, is_ms_keyword ? " " : "", str, is_ms_keyword ? "" : " ") :
            str_printf(sym, "%s%s", str, is_ms_keyword ? "" : " ");
    }
}

static void get_extended_qualifier(struct parsed_symbol *sym, struct datatype_t *xdt)
{
    unsigned fl = 0;
    xdt->left = xdt->right = NULL;
    xdt->flags = 0;
    for (;;)
    {
        switch (*sym->current)
        {
        case 'E': append_extended_qualifier(sym, &xdt->right, "__ptr64", TRUE);     fl |= 2; break;
        case 'F': append_extended_qualifier(sym, &xdt->left,  "__unaligned", TRUE); fl |= 2; break;
#ifdef _UCRT
        case 'G': append_extended_qualifier(sym, &xdt->right, "&", FALSE);          fl |= 1; break;
        case 'H': append_extended_qualifier(sym, &xdt->right, "&&", FALSE);         fl |= 1; break;
#endif
        case 'I': append_extended_qualifier(sym, &xdt->right, "__restrict", TRUE);  fl |= 2; break;
        default: if (fl == 1 || (fl == 3 && (sym->flags & UNDNAME_NO_MS_KEYWORDS))) xdt->flags = DT_NO_LRSEP_WS; return;
        }
        sym->current++;
    }
}

/******************************************************************
 *		get_qualifier
 * Parses the type qualifier. Always returns static strings.
 */
static BOOL get_qualifier(struct parsed_symbol *sym, struct datatype_t *xdt, const char** pclass)
{
    char ch;
    const char* qualif;

    get_extended_qualifier(sym, xdt);
    switch (ch = *sym->current++)
    {
    case 'A': qualif = NULL; break;
    case 'B': qualif = "const"; break;
    case 'C': qualif = "volatile"; break;
    case 'D': qualif = "const volatile"; break;
    case 'Q': qualif = NULL; break;
    case 'R': qualif = "const"; break;
    case 'S': qualif = "volatile"; break;
    case 'T': qualif = "const volatile"; break;
    default: return FALSE;
    }
    if (qualif)
    {
        xdt->flags &= ~DT_NO_LRSEP_WS;
        xdt->left = xdt->left ? str_printf(sym, "%s %s", qualif, xdt->left) : qualif;
    }
    if (ch >= 'Q' && ch <= 'T') /* pointer to member, fetch class */
    {
        const char* class = get_class_name(sym);
        if (!class) return FALSE;
        if (!pclass)
        {
            FIXME("Got pointer to class %s member without storage\n", class);
            return FALSE;
        }
        *pclass = class;
    }
    else if (pclass) *pclass = NULL;
    return TRUE;
}

static BOOL get_function_qualifier(struct parsed_symbol *sym, const char** qualif)
{
    struct datatype_t   xdt;

    if (!get_qualifier(sym, &xdt, NULL)) return FALSE;
    *qualif = (xdt.left || xdt.right) ?
        str_printf(sym, "%s%s%s", xdt.left, (xdt.flags & DT_NO_LRSEP_WS) ? "" : " ", xdt.right) : NULL;
    return TRUE;
}

static BOOL get_qualified_type(struct datatype_t *ct, struct parsed_symbol* sym,
                               char qualif, enum datatype_flags flags)
{
    struct datatype_t xdt1;
    struct datatype_t xdt2;
    const char* ref;
    const char* str_qualif;
    const char* class;

    get_extended_qualifier(sym, &xdt1);

    switch (qualif)
    {
    case 'A': ref = " &";  str_qualif = NULL;              break;
    case 'B': ref = " &";  str_qualif = " volatile";       break;
    case 'P': ref = " *";  str_qualif = NULL;              break;
    case 'Q': ref = " *";  str_qualif = " const";          break;
    case 'R': ref = " *";  str_qualif = " volatile";       break;
    case 'S': ref = " *";  str_qualif = " const volatile"; break;
    case '?': ref = NULL;  str_qualif = NULL;              break;
    case '$': ref = " &&"; str_qualif = NULL;              break;
    default: return FALSE;
    }
    ct->right = NULL;
    ct->flags = 0;

    /* parse managed handle information */
    if (sym->current[0] == '$' && sym->current[1] == 'A')
    {
        sym->current += 2;

        switch (qualif)
        {
        case 'A':
        case 'B':
            ref = " %";
            break;
        case 'P':
        case 'Q':
        case 'R':
        case 'S':
            ref = " ^";
            break;
        default:
            return FALSE;
        }
    }

    if (get_qualifier(sym, &xdt2, &class))
    {
        unsigned            mark = sym->stack.num;
        struct datatype_t   sub_ct;

        if (ref || str_qualif || xdt1.left || xdt1.right)
        {
            if (class)
                ct->left = str_printf(sym, "%s%s%s%s::%s%s%s",
                                      xdt1.left ? " " : NULL, xdt1.left,
                                      class ? " " : NULL, class, ref ? ref + 1 : NULL,
                                      xdt1.right ? " " : NULL, xdt1.right, str_qualif);
            else
                ct->left = str_printf(sym, "%s%s%s%s%s%s",
                                      xdt1.left ? " " : NULL, xdt1.left, ref,
                                      xdt1.right ? " " : NULL, xdt1.right, str_qualif);
        }
        else
            ct->left = NULL;
        /* multidimensional arrays */
        if (*sym->current == 'Y')
        {
            const char* n1;
            int num;

            sym->current++;
            if (!(n1 = get_number(sym))) return FALSE;
            num = atoi(n1);

            ct->left = str_printf(sym, " (%s%s", xdt2.left, ct->left && !xdt2.left ? ct->left + 1 : ct->left);
            ct->right = ")";
            xdt2.left = NULL;

            while (num--)
                ct->right = str_printf(sym, "%s[%s]", ct->right, get_number(sym));
        }

        /* Recurse to get the referred-to type */
        if (!demangle_datatype(sym, &sub_ct, 0))
            return FALSE;
        if (sub_ct.flags & DT_NO_LEADING_WS)
            ct->left++;
        ct->left = str_printf(sym, "%s%s%s%s%s", sub_ct.left, xdt2.left ? " " : NULL,
                              xdt2.left, ct->left,
                              ((xdt2.left || str_qualif) && (flags & WS_AFTER_QUAL_IF)) ? " " : NULL);
        if (sub_ct.right) ct->right = str_printf(sym, "%s%s", ct->right, sub_ct.right);
        sym->stack.num = mark;
    }
    else if (ref || str_qualif || xdt1.left || xdt1.right)
        ct->left = str_printf(sym, "%s%s%s%s%s%s",
                              xdt1.left ? " " : NULL, xdt1.left, ref,
                              xdt1.right ? " " : NULL, xdt1.right, str_qualif);
    else
        ct->left = NULL;
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
              *sym->current == '_' || *sym->current == '$' ||
              *sym->current == '<' || *sym->current == '>')) {
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
    unsigned args_mark = sym->args.num;

    sym->names.start = sym->names.num;
    if (!(name = get_literal_string(sym))) {
        sym->names.start = start_mark;
        return FALSE;
    }
    args = get_args(sym, FALSE, '<', '>');
    if (args != NULL)
        name = str_printf(sym, "%s%s", name, args);
    sym->names.num = num_mark;
    sym->names.start = start_mark;
    sym->stack.num = stack_mark;
    sym->args.num = args_mark;
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
/*  case 'Z': (...) variadic function arguments. Handled in get_args() */
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
    case 'Q': type_string = "char8_t"; break;
    case 'S': type_string = "char16_t"; break;
    case 'U': type_string = "char32_t"; break;
    case 'W': type_string = "wchar_t"; break;
    default:  type_string = NULL; break;
    }
    return type_string;
}

struct function_signature
{
    const char*             call_conv;
    const char*             exported;
    struct datatype_t       return_ct;
    const char*             arguments;
};

static BOOL get_function_signature(struct parsed_symbol* sym, struct function_signature* fs)
{
    unsigned mark = sym->stack.num;

    if (!get_calling_convention(*sym->current++,
                                &fs->call_conv, &fs->exported,
                                sym->flags & ~UNDNAME_NO_ALLOCATION_LANGUAGE) ||
        !demangle_datatype(sym, &fs->return_ct, 0))
        return FALSE;

    if (!(fs->arguments = get_args(sym, TRUE, '(', ')')))
        return FALSE;
    sym->stack.num = mark;

    return TRUE;
}

/*******************************************************************
 *         demangle_datatype
 *
 * Attempt to demangle a C++ data type, which may be datatype.
 * a datatype type is made up of a number of simple types. e.g:
 * char** = (pointer to (pointer to (char)))
 */
static BOOL demangle_datatype(struct parsed_symbol* sym, struct datatype_t* ct,
                              enum datatype_flags flags)
{
    char                dt;

    assert(ct);
    ct->left = ct->right = NULL;
    ct->flags = 0;

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
        if (flags & IN_ARGS)
        {
            const char*   ptr;
            if (!(ptr = get_number(sym))) goto done;
            ct->left = str_printf(sym, "`template-parameter-%s'", ptr);
        }
        else
        {
            if (!get_qualified_type(ct, sym, '?', flags)) goto done;
        }
        break;
    case 'A': /* reference */
    case 'B': /* volatile reference */
        if (!get_qualified_type(ct, sym, dt, flags)) goto done;
        break;
    case 'P': /* Pointer */
    case 'Q': /* const pointer */
    case 'R': /* volatile pointer */
    case 'S': /* const volatile pointer */
        if (!(flags & IN_ARGS)) dt = 'P';
        if (isdigit(*sym->current))
        {
            const char *ptr_qualif;
            switch (dt)
            {
            default:
            case 'P': ptr_qualif = NULL; break;
            case 'Q': ptr_qualif = "const"; break;
            case 'R': ptr_qualif = "volatile"; break;
            case 'S': ptr_qualif = "const volatile"; break;
            }
            /* FIXME:
             *   P6 = Function pointer
             *   P8 = Member function pointer
             *   others who knows.. */
            if (*sym->current == '8')
            {
                struct function_signature       fs;
                const char*                     class;
                const char*                     function_qualifier;

                sym->current++;

                if (!(class = get_class_name(sym)))
                    goto done;
                if (!get_function_qualifier(sym, &function_qualifier))
                    goto done;
                if (!get_function_signature(sym, &fs))
                    goto done;

                ct->left  = str_printf(sym, "%s%s (%s %s::*%s",
                                       fs.return_ct.left, fs.return_ct.right, fs.call_conv, class, ptr_qualif);
                ct->right = str_printf(sym, ")%s%s", fs.arguments, function_qualifier);
            }
            else if (*sym->current == '6')
            {
                struct function_signature       fs;

                sym->current++;

                if (!get_function_signature(sym, &fs))
                     goto done;

                ct->left  = str_printf(sym, "%s%s (%s*%s",
                                       fs.return_ct.left, fs.return_ct.right, fs.call_conv, ptr_qualif);
                ct->flags = DT_NO_LEADING_WS;
                ct->right = str_printf(sym, ")%s", fs.arguments);
            }
            else goto done;
	}
	else if (!get_qualified_type(ct, sym, dt, flags)) goto done;
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
        ct->left = str_array_get_ref(&sym->args, (dt - '0') * 2);
        ct->right = str_array_get_ref(&sym->args, (dt - '0') * 2 + 1);
        if (!ct->left) goto done;
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
            if (*sym->current == 'A')
            {
                sym->current++;
                if (*sym->current == '6')
                {
                    struct function_signature fs;

                    sym->current++;

                    if (!get_function_signature(sym, &fs))
                        goto done;
                    ct->left = str_printf(sym, "%s%s %s%s",
                                          fs.return_ct.left, fs.return_ct.right, fs.call_conv, fs.arguments);
                }
            }
            else if (*sym->current == 'B')
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

                if (!demangle_datatype(sym, &sub_ct, 0)) goto done;

                if (arr)
                    ct->left = str_printf(sym, "%s %s", sub_ct.left, arr);
                else
                    ct->left = sub_ct.left;
                ct->right = sub_ct.right;
                sym->stack.num = mark;
            }
            else if (*sym->current == 'C')
            {
                struct datatype_t xdt;

                sym->current++;
                if (!get_qualifier(sym, &xdt, NULL)) goto done;
                if (!demangle_datatype(sym, ct, flags)) goto done;
                ct->left = str_printf(sym, "%s %s", ct->left, xdt.left);
            }
            else if (*sym->current == 'Q')
            {
                sym->current++;
                if (!get_qualified_type(ct, sym, '$', flags)) goto done;
            }
            else if (*sym->current == 'T')
            {
                sym->current++;
                ct->left = str_printf(sym, "std::nullptr_t");
            }
            break;
        }
        break;
    default :
        ERR("Unknown type %c\n", dt);
        break;
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
    struct datatype_t   xdt = {NULL};
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
            const char* class;

            if (!demangle_datatype(sym, &ct, 0)) goto done;
            if (!get_qualifier(sym, &xdt, &class)) goto done; /* class doesn't seem to be displayed */
            if (xdt.left && xdt.right) xdt.left = str_printf(sym, "%s %s", xdt.left, xdt.right);
            else if (!xdt.left) xdt.left = xdt.right;
            sym->stack.num = mark;
        }
        break;
    case '6' : /* compiler generated static */
    case '7' : /* compiler generated static */
        ct.left = ct.right = NULL;
        if (!get_qualifier(sym, &xdt, NULL)) goto done;
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
        xdt.left = ct.left = ct.right = NULL;
        break;
    default: goto done;
    }
    if (sym->flags & UNDNAME_NAME_ONLY) ct.left = ct.right = xdt.left = NULL;

    sym->result = str_printf(sym, "%s%s%s%s%s%s%s%s", access,
                             member_type, ct.left,
                             xdt.left && ct.left ? " " : NULL, xdt.left,
                             xdt.left || ct.left ? " " : NULL, name, ct.right);
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
    const char*         function_qualifier = NULL;
    const char*         exported;
    const char*         args_str = NULL;
    const char*         name = NULL;
    BOOL                ret = FALSE, has_args = TRUE, has_ret = TRUE;
    unsigned            mark;

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
        /* Implicit 'this' pointer */
        if (!get_function_qualifier(sym, &function_qualifier)) goto done;
    }

    if (!get_calling_convention(*sym->current++, &call_conv, &exported,
                                sym->flags))
        goto done;

    /* Return type, or @ if 'void' */
    if (has_ret && *sym->current == '@')
    {
        ct_ret.left = "void";
        ct_ret.right = NULL;
        sym->current++;
    }
    else if (has_ret)
    {
        if (!demangle_datatype(sym, &ct_ret, cast_op ? WS_AFTER_QUAL_IF : 0))
            goto done;
    }
    if (!has_ret || sym->flags & UNDNAME_NO_FUNCTION_RETURNS)
        ct_ret.left = ct_ret.right = NULL;
    if (cast_op)
    {
        name = str_printf(sym, "%s %s%s", name, ct_ret.left, ct_ret.right);
        ct_ret.left = ct_ret.right = NULL;
    }

    mark = sym->stack.num;
    if (has_args && !(args_str = get_args(sym, TRUE, '(', ')'))) goto done;
    if (sym->flags & UNDNAME_NAME_ONLY) args_str = function_qualifier = NULL;
    if (sym->flags & UNDNAME_NO_THISTYPE) function_qualifier = NULL;
    sym->stack.num = mark;

    /* Note: '()' after 'Z' means 'throws', but we don't care here
     * Yet!!! FIXME
     */
    sym->result = str_printf(sym, "%s%s%s%s%s%s%s%s%s%s%s",
                             access, member_type, ct_ret.left,
                             (ct_ret.left && !ct_ret.right) ? " " : NULL,
                             call_conv, call_conv ? " " : NULL, exported,
                             name, args_str, function_qualifier, ct_ret.right);
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
    enum {
        PP_NONE,
        PP_CONSTRUCTOR,
        PP_DESTRUCTOR,
        PP_CAST_OPERATOR,
    } post_process = PP_NONE;

    /* FIXME seems wrong as name, as it demangles a simple data type */
    if (sym->flags & UNDNAME_NO_ARGUMENTS)
    {
        struct datatype_t   ct;

        if (demangle_datatype(sym, &ct, 0))
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
    if (*sym->current == '?')
    {
        const char* function_name = NULL;
        BOOL in_template = FALSE;

        if (sym->current[1] == '$' && sym->current[2] == '?')
        {
            in_template = TRUE;
            sym->current += 2;
        }

        /* C++ operator code (one character, or two if the first is '_') */
        switch (*++sym->current)
        {
        case '0': function_name = ""; post_process = PP_CONSTRUCTOR; break;
        case '1': function_name = ""; post_process = PP_DESTRUCTOR; break;
        case '2': function_name = "operator new"; break;
        case '3': function_name = "operator delete"; break;
        case '4': function_name = "operator="; break;
        case '5': function_name = "operator>>"; break;
        case '6': function_name = "operator<<"; break;
        case '7': function_name = "operator!"; break;
        case '8': function_name = "operator=="; break;
        case '9': function_name = "operator!="; break;
        case 'A': function_name = "operator[]"; break;
        case 'B': function_name = "operator"; post_process = PP_CAST_OPERATOR; break;
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
            case 'C': sym->result = (char*)"`string'"; /* string literal: followed by string encoding (native never undecode it) */
                /* FIXME: should unmangle the whole string for error reporting */
                if (*sym->current && sym->current[strlen(sym->current) - 1] == '@') ret = TRUE;
                goto done;
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

                        sym->current++;
                        if (!demangle_datatype(sym, &ct, 0))
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
            case '_':
                switch (*++sym->current)
                {
                case 'K':
                    sym->current++;
                    function_name = str_printf(sym, "operator \"\" %s", get_literal_string(sym));
                    --sym->current;
                    break;
                default:
                    FIXME("Unknown operator: __%c\n", *sym->current);
                    return FALSE;
                }
                break;
            default:
                ERR("Unknown operator: _%c\n", *sym->current);
                return FALSE;
            }
            break;
        case '$':
            sym->current++;
            if (!(function_name = get_template_name(sym))) goto done;
            --sym->current;
            break;
        default:
            /* FIXME: Other operators */
            ERR("Unknown operator: %c\n", *sym->current);
            return FALSE;
        }
        sym->current++;
        if (in_template)
        {
            unsigned args_mark = sym->args.num;
            const char *args;

            args = get_args(sym, FALSE, '<', '>');
            if (args) function_name = function_name ? str_printf(sym, "%s%s", function_name, args) : args;
            sym->args.num = args_mark;
            sym->names.num = 0;
        }
        if (!str_array_push(sym, function_name, -1, &sym->stack))
            return FALSE;
    }
    else if (*sym->current == '$')
    {
        /* Strange construct, it's a name with a template argument list
           and that's all. */
        sym->current++;
        ret = (sym->result = get_template_name(sym)) != NULL;
        goto done;
    }

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

    switch (post_process)
    {
    case PP_NONE: default: break;
    case PP_CONSTRUCTOR: case PP_DESTRUCTOR:
        /* it's time to set the member name for ctor & dtor */
        if (sym->stack.num <= 1) goto done;
        sym->stack.elts[0] = str_printf(sym, "%s%s%s", post_process == PP_DESTRUCTOR ? "~" : NULL,
                                        sym->stack.elts[1], sym->stack.elts[0]);
        /* ctors and dtors don't have return type */
        sym->flags |= UNDNAME_NO_FUNCTION_RETURNS;
        break;
    case PP_CAST_OPERATOR:
        sym->flags &= ~UNDNAME_NO_FUNCTION_RETURNS;
        break;
    }

    /* Function/Data type and access level */
    if (*sym->current >= '0' && *sym->current <= '9')
        ret = handle_data(sym);
    else if ((*sym->current >= 'A' && *sym->current <= 'Z') || *sym->current == '$')
        ret = handle_method(sym, post_process == PP_CAST_OPERATOR);
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
