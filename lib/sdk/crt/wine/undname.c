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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "wine/config.h"
#include "wine/port.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winternl.h"
#include "wine/exception.h"
#include "winnt.h"
#include "excpt.h"
#include "wine/debug.h"
#include <malloc.h>
#include <stdlib.h>

#include <internal/wine/msvcrt.h>
#include <internal/wine/cppexcept.h>
#include <internal/mtdll.h>

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

/* TODO:
 * - document a bit (grammar + fonctions)
 * - back-port this new code into tools/winedump/msmangle.c
 */

#define UNDNAME_COMPLETE                 (0x0000)
#define UNDNAME_NO_LEADING_UNDERSCORES   (0x0001) /* Don't show __ in calling convention */
#define UNDNAME_NO_MS_KEYWORDS           (0x0002) /* Don't show calling convention at all */
#define UNDNAME_NO_FUNCTION_RETURNS      (0x0004) /* Don't show function/method return value */
#define UNDNAME_NO_ALLOCATION_MODEL      (0x0008)
#define UNDNAME_NO_ALLOCATION_LANGUAGE   (0x0010)
#define UNDNAME_NO_MS_THISTYPE           (0x0020)
#define UNDNAME_NO_CV_THISTYPE           (0x0040)
#define UNDNAME_NO_THISTYPE              (0x0060)
#define UNDNAME_NO_ACCESS_SPECIFIERS     (0x0080) /* Don't show access specifier (public/protected/private) */
#define UNDNAME_NO_THROW_SIGNATURES      (0x0100)
#define UNDNAME_NO_MEMBER_TYPE           (0x0200) /* Don't show static/virtual specifier */
#define UNDNAME_NO_RETURN_UDT_MODEL      (0x0400)
#define UNDNAME_32_BIT_DECODE            (0x0800)
#define UNDNAME_NAME_ONLY                (0x1000) /* Only report the variable/method name */
#define UNDNAME_NO_ARGUMENTS             (0x2000) /* Don't show method arguments */
#define UNDNAME_NO_SPECIAL_SYMS          (0x4000)
#define UNDNAME_NO_COMPLEX_TYPE          (0x8000)

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

#define MAX_ARRAY_ELTS  32
struct array
{
    unsigned            start;          /* first valid reference in array */
    unsigned            num;            /* total number of used elts */
    unsigned            max;
    char*               elts[MAX_ARRAY_ELTS];
};

/* Structure holding a parsed symbol */
struct parsed_symbol
{
    unsigned            flags;          /* the UNDNAME_ flags used for demangling */
    malloc_func_t       mem_alloc_ptr;  /* internal allocator */
    free_func_t         mem_free_ptr;   /* internal deallocator */

    const char*         current;        /* pointer in input (mangled) string */
    char*               result;         /* demangled string */

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

/******************************************************************
 *		und_alloc
 *
 * Internal allocator. Uses a simple linked list of large blocks
 * where we use a poor-man allocator. It's fast, and since all
 * allocation is pool, memory management is easy (esp. freeing).
 */
static void*    und_alloc(struct parsed_symbol* sym, size_t len)
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
        sym->mem_free_ptr(sym->alloc_list);
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
    a->start = a->num = a->max = 0;
}

/******************************************************************
 *		str_array_push
 * Adding a new string to an array
 */
static void str_array_push(struct parsed_symbol* sym, const char* ptr, size_t len, 
                           struct array* a)
{
    assert(ptr);
    assert(a);
    assert(a->num < MAX_ARRAY_ELTS);
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
            TRACE("\t%d%c %s\n", i, c, a->elts[i]);
        }
    }
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
          cref, idx, cref->elts[cref->start + idx]);
    return cref->elts[cref->start + idx];
}

/******************************************************************
 *		str_printf
 * Helper for printf type of command (only %s and %c are implemented) 
 * while dynamically allocating the buffer
 */
static char* str_printf(struct parsed_symbol* sym, const char* format, ...)
{
    va_list     args;
    size_t      len = 1, i, sz;
    char*       tmp;
    char*       p;
    char*       t;

    va_start(args, format);
    for (i = 0; format[i]; i++)
    {
        if (format[i] == '%')
        {
            switch (format[++i])
            {
            case 's': t = va_arg(args, char*); if (t) len += strlen(t); break;
            case 'c': (void)va_arg(args, int); len++; break;
            default: i--; /* fall thru */
            case '%': len++; break;
            }
        }
        else len++;
    }
    va_end(args);
    if (!(tmp = (char*)und_alloc(sym, len))) return NULL;
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
            default: i--; /* fall thru */
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
    int                 i;

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
        /* 'void' terminates an argument list */
        if (!strcmp(ct.left, "void"))
        {
            if (!z_term && *sym->current == '@') sym->current++;
            break;
        }
        str_array_push(sym, str_printf(sym, "%s%s", ct.left, ct.right), -1, 
                       &arg_collect);
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

    if (close_char == '>' && args_str && args_str[strlen(args_str) - 1] == '>')
        args_str = str_printf(sym, "%c%s%s %c", 
                              open_char, arg_collect.elts[0], args_str, close_char);
    else
        args_str = str_printf(sym, "%c%s%s%c", 
                              open_char, arg_collect.elts[0], args_str, close_char);
    
    return args_str;
}

/******************************************************************
 *		get_modifier
 * Parses the type modifier. Always returns a static string
 */
static BOOL get_modifier(char ch, const char** ret)
{
    switch (ch)
    {
    case 'A': *ret = NULL; break;
    case 'B': *ret = "const"; break;
    case 'C': *ret = "volatile"; break;
    case 'D': *ret = "const volatile"; break;
    default: return FALSE;
    }
    return TRUE;
}

static const char* get_modified_type(struct parsed_symbol* sym, char modif)
{
    const char* modifier;
    const char* ret = NULL;
    const char* str_modif;

    switch (modif)
    {
    case 'A': str_modif = " &"; break;
    case 'P': str_modif = " *"; break;
    case 'Q': str_modif = " * const"; break;
    case '?': str_modif = ""; break;
    default: return NULL;
    }

    if (get_modifier(*sym->current++, &modifier))
    {
        unsigned            mark = sym->stack.num;
        struct datatype_t   sub_ct;

        /* Recurse to get the referred-to type */
        if (!demangle_datatype(sym, &sub_ct, NULL, FALSE))
            return NULL;
        ret = str_printf(sym, "%s%s%s%s%s", 
                         sub_ct.left, sub_ct.left && modifier ? " " : NULL, 
                         modifier, sub_ct.right, str_modif);
        sym->stack.num = mark;
    }
    return ret;
}

/******************************************************************
 *		get_class
 * Parses class as a list of parent-classes, separated by '@', terminated by '@@'
 * and stores the result in 'a' array. Each parent-classes, as well as the inner
 * element (either field/method name or class name), are stored as allocated
 * strings in the array.
 */
static BOOL get_class(struct parsed_symbol* sym)
{
    const char* ptr;

    while (*sym->current != '@')
    {
        switch (*sym->current)
        {
        case '\0': return FALSE;

        case '0': case '1': case '2': case '3':
        case '4': case '5': case '6': case '7':
        case '8': case '9':
            ptr = str_array_get_ref(&sym->stack, *sym->current++ - '0');
            if (!ptr) return FALSE;
            str_array_push(sym, ptr, -1, &sym->stack);
            break;
        case '?':
            if (*++sym->current == '$') 
            {
                const char*     name = ++sym->current;
                char*           full = NULL;
                char*           args = NULL;
                unsigned        num_mark = sym->stack.num;
                unsigned        start_mark = sym->stack.start;

                while (*sym->current++ != '@');

                sym->stack.start = sym->stack.num;
                str_array_push(sym, name, sym->current - name -1, &sym->stack);
                args = get_args(sym, NULL, FALSE, '<', '>');
                if (args != NULL)
                {
                    full = str_printf(sym, "%s%s", sym->stack.elts[num_mark], args);
                }
                if (!full) return FALSE;
                sym->stack.elts[num_mark] = full;
                sym->stack.num = num_mark + 1;
                sym->stack.start = start_mark;
            }
            break;
        default:
            ptr = sym->current;
            while (*sym->current++ != '@');
            str_array_push(sym, ptr, sym->current - 1 - ptr, &sym->stack);
            break;
        }
    }
    sym->current++;
    return TRUE;
}

/******************************************************************
 *		get_class_string
 * From an array collected by get_class, constructs the corresponding (allocated) 
 * string
 */
static char* get_class_string(struct parsed_symbol* sym, /*const struct array* a, */int start)
{
    int         i;
    size_t      len, sz;
    char*       ret;
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
 *		get_calling_convention
 * Returns a static string corresponding to the calling convention described
 * by char 'ch'. Sets export to TRUE iff the calling convention is exported.
 */
static BOOL get_calling_convention(struct parsed_symbol* sym, char ch, 
                                   const char** call_conv, const char** exported,
                                   unsigned flags)
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
            case 'K': break;
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
            case 'K': break;
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
static const char* get_simple_type(struct parsed_symbol* sym, char c)
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
 *         get_extented_type
 * Return a string containing an allocated string for a simple data type
 */
static const char* get_extended_type(struct parsed_symbol* sym, char c)
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
        ct->left = get_extended_type(sym, *sym->current++);
        break;
    case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'M':
    case 'N': case 'O': case 'X': case 'Z':
        /* Simple data types */
        ct->left = get_simple_type(sym, dt);
        add_pmt = FALSE;
        break;
    case 'T': /* union */
    case 'U': /* struct */
    case 'V': /* class */
        /* Class/struct/union */
        {
            unsigned    mark = sym->stack.num;
            const char* struct_name = NULL;
            const char* type_name = NULL;

            if (!get_class(sym) ||
                !(struct_name = get_class_string(sym, mark))) goto done;
            sym->stack.num = mark;
            if (!(sym->flags & UNDNAME_NO_COMPLEX_TYPE)) 
            {
                switch (dt)
                {
                case 'T': type_name = "union ";  break;
                case 'U': type_name = "struct "; break;
                case 'V': type_name = "class ";  break;
                }
            }
            ct->left = str_printf(sym, "%s%s", type_name, struct_name);
        }
        break;
    case '?':
        /* not all the time is seems */
        if (!(ct->left = get_modified_type(sym, '?'))) goto done;
        break;
    case 'A':
        if (!(ct->left = get_modified_type(sym, 'A'))) goto done;
        break;
    case 'Q':
        if (!(ct->left = get_modified_type(sym, in_args ? 'Q' : 'P'))) goto done;
        break;
    case 'P': /* Pointer */
        if (isdigit(*sym->current))
	{
            /* FIXME: P6 = Function pointer, others who knows.. */
            if (*sym->current++ == '6')
            {
                char*                   args = NULL;
                const char*             call_conv;
                const char*             exported;
                struct datatype_t       sub_ct;
                unsigned                mark = sym->stack.num;

                if (!get_calling_convention(sym, *sym->current++, 
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
	else if (!(ct->left = get_modified_type(sym, 'P'))) goto done;
        break;
    case 'W':
        if (*sym->current == '4')
        {
            char*               enum_name;
            unsigned            mark = sym->stack.num;
            sym->current++;
            if (!get_class(sym) ||
                !(enum_name = get_class_string(sym, mark))) goto done;
            sym->stack.num = mark;
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
        ct->left = str_array_get_ref(pmt_ref, dt - '0');
        if (!ct->left) goto done;
        add_pmt = FALSE;
        break;
    case '$':
        if (sym->current[0] != '0') goto done;
        if (sym->current[1] >= '0' && sym->current[1] <= '9')
        {
            char*       ptr;
            ptr = und_alloc(sym, 2);
            ptr[0] = sym->current[1] + 1;
            ptr[1] = 0;
            ct->left = ptr;
            sym->current += 2;
        }
        else if ((sym->current[1] >= 'A' && sym->current[1] <= 'P') &&
                 sym->current[2] == '@')
        {
            char* ptr;
            ptr = und_alloc(sym, 3);
            if (sym->current[1] <= 'J')
            {
                ptr[0] = '0' + sym->current[1] - 'A';
                ptr[1] = 0;
            }
            else
            {
                ptr[0] = '1';
                ptr[1] = sym->current[1] - 'K' + '0';
                ptr[2] = 0;
            }
            ct->left = ptr;
            sym->current += 3;
        }
        else goto done;
        break;
    default :
        ERR("Unknown type %c\n", dt);
        break;
    }
    if (add_pmt && pmt_ref && in_args)
        str_array_push(sym, str_printf(sym, "%s%s", ct->left, ct->right), 
                       -1, pmt_ref);
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
    struct datatype_t   ct;
    char*               name = NULL;
    BOOL                ret = FALSE;
    char                dt;

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

    switch (dt = *sym->current++)
    {
    case '0': case '1': case '2':
    case '3': case '4': case '5':
        {
            unsigned mark = sym->stack.num;
            if (!demangle_datatype(sym, &ct, NULL, FALSE)) goto done;
            if (!get_modifier(*sym->current++, &modifier)) goto done;
            sym->stack.num = mark;
        }
        break;
    case '6' : /* compiler generated static */
    case '7' : /* compiler generated static */
        ct.left = ct.right = NULL;
        if (!get_modifier(*sym->current++, &modifier)) goto done;
        if (*sym->current != '@')
        {
            unsigned    mark = sym->stack.num;
            char*       cls = NULL;

            if (!get_class(sym) ||
                !(cls = get_class_string(sym, mark))) goto done;
            sym->stack.num = mark;
            ct.right = str_printf(sym, "{for `%s'}", cls);
        }
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
    const char*         access = NULL;
    const char*         member_type = NULL;
    struct datatype_t   ct_ret;
    const char*         call_conv;
    const char*         modifier = NULL;
    const char*         exported;
    const char*         args_str = NULL;
    const char*         name = NULL;
    BOOL                ret = FALSE;
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
     */

    if (!(sym->flags & UNDNAME_NO_ACCESS_SPECIFIERS))
    {
        switch ((*sym->current - 'A') / 8)
        {
        case 0: access = "private: "; break;
        case 1: access = "protected: "; break;
        case 2: access = "public: "; break;
        }
    }
    if (!(sym->flags & UNDNAME_NO_MEMBER_TYPE))
    {
        if (*sym->current >= 'A' && *sym->current <= 'X')
        {
            switch ((*sym->current - 'A') % 8)
            {
            case 2: case 3: member_type = "static "; break;
            case 4: case 5: member_type = "virtual "; break;
            case 6: case 7: member_type = "thunk "; break;
            }
        }
    }

    if (*sym->current >= 'A' && *sym->current <= 'X')
    {
        if (!((*sym->current - 'A') & 2))
        {
            /* Implicit 'this' pointer */
            /* If there is an implicit this pointer, const modifier follows */
            if (!get_modifier(*++sym->current, &modifier)) goto done;
        }
    }
    else if (*sym->current < 'A' || *sym->current > 'Z') goto done;
    sym->current++;

    name = get_class_string(sym, 0);
  
    if (!get_calling_convention(sym, *sym->current++, 
                                &call_conv, &exported, sym->flags))
        goto done;

    str_array_init(&array_pmt);

    /* Return type, or @ if 'void' */
    if (*sym->current == '@')
    {
        ct_ret.left = "void";
        ct_ret.right = NULL;
        sym->current++;
    }
    else
    {
        if (!demangle_datatype(sym, &ct_ret, &array_pmt, FALSE))
            goto done;
    }
    if (sym->flags & UNDNAME_NO_FUNCTION_RETURNS)
        ct_ret.left = ct_ret.right = NULL;
    if (cast_op)
    {
        name = str_printf(sym, "%s%s%s", name, ct_ret.left, ct_ret.right);
        ct_ret.left = ct_ret.right = NULL;
    }

    mark = sym->stack.num;
    if (!(args_str = get_args(sym, &array_pmt, TRUE, '(', ')'))) goto done;
    if (sym->flags & UNDNAME_NAME_ONLY) args_str = modifier = NULL;
    sym->stack.num = mark;

    /* Note: '()' after 'Z' means 'throws', but we don't care here
     * Yet!!! FIXME
     */
    sym->result = str_printf(sym, "%s%s%s%s%s%s%s%s%s%s%s%s",
                             access, member_type, ct_ret.left, 
                             (ct_ret.left && !ct_ret.right) ? " " : NULL,
                             call_conv, call_conv ? " " : NULL, exported,
                             name, args_str, modifier, 
                             modifier ? " " : NULL, ct_ret.right);
    ret = TRUE;
done:
    return ret;
}

/*******************************************************************
 *         demangle_symbol
 * Demangle a C++ linker symbol
 */
static BOOL symbol_demangle(struct parsed_symbol* sym)
{
    BOOL                ret = FALSE;
    unsigned            do_after = 0;

    /* MS mangled names always begin with '?' */
    if (*sym->current != '?') return FALSE;
    
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

    str_array_init(&sym->stack);
    sym->current++;

    /* Then function name or operator code */
    if (*sym->current == '?' && sym->current[1] != '$')
    {
        const char* function_name = NULL;

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
            sym->stack.num = sym->stack.max = 1;
            sym->stack.elts[0] = "--null--";
            break;
        case 4:
            sym->result = (char*)function_name;
            ret = TRUE;
            goto done;
        default:
            str_array_push(sym, function_name, -1, &sym->stack);
            break;
        }
        sym->stack.start = 1;
    }

    /* Either a class name, or '@' if the symbol is not a class member */
    if (*sym->current != '@')
    {
        /* Class the function is associated with, terminated by '@@' */
        if (!get_class(sym)) goto done;
    }
    else sym->current++;

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
    }

    /* Function/Data type and access level */
    if (*sym->current >= '0' && *sym->current <= '7')
        ret = handle_data(sym);
    else if (*sym->current >= 'A' && *sym->current <= 'Z')
        ret = handle_method(sym, do_after == 3);
    else ret = FALSE;
done:
    if (ret) 
	assert(sym->result);
    if (!ret)
	ERR("Failed at %s\n", sym->current);

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
char* __unDNameEx(char* buffer, const char* mangled, int buflen,
                  malloc_func_t memget, free_func_t memfree,
                  void* unknown, unsigned short int flags)
{
    struct parsed_symbol        sym;

    TRACE("(%p,%s,%d,%p,%p,%p,%x) stub!\n",
          buffer, mangled, buflen, memget, memfree, unknown, flags);
    
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

    if (symbol_demangle(&sym))
    {
        if (buffer && buflen)
        {
            memcpy(buffer, sym.result, buflen - 1);
            buffer[buflen - 1] = '\0';
        }
        else
        {
            buffer = memget(strlen(sym.result) + 1);
            if (buffer) strcpy(buffer, sym.result);
        }
    }
    else buffer = NULL;

    und_free_all(&sym);

    return buffer;
}


/*********************************************************************
 *		__unDName (MSVCRT.@)
 */
char* __unDName(char* buffer, const char* mangled, int buflen,
                malloc_func_t memget, free_func_t memfree,
                unsigned short int flags)
{
    return __unDNameEx(buffer, mangled, buflen, memget, memfree, NULL, flags);
}

