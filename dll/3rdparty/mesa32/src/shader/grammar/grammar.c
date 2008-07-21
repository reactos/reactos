/*
 * Mesa 3-D graphics library
 * Version:  6.6
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file grammar.c
 * syntax parsing engine
 * \author Michal Krol
 */

#ifndef GRAMMAR_PORT_BUILD
#error Do not build this file directly, build your grammar_XXX.c instead, which includes this file
#endif

/*
*/

/*
    INTRODUCTION
    ------------

    The task is to check the syntax of an input string. Input string is a stream of ASCII
    characters terminated with a null-character ('\0'). Checking it using C language is
    difficult and hard to implement without bugs. It is hard to maintain and make changes when
    the syntax changes.

    This is because of a high redundancy of the C code. Large blocks of code are duplicated with
    only small changes. Even use of macros does not solve the problem because macros cannot
    erase the complexity of the problem.

    The resolution is to create a new language that will be highly oriented to our task. Once
    we describe a particular syntax, we are done. We can then focus on the code that implements
    the language. The size and complexity of it is relatively small than the code that directly
    checks the syntax.

    First, we must implement our new language. Here, the language is implemented in C, but it
    could also be implemented in any other language. The code is listed below. We must take
    a good care that it is bug free. This is simple because the code is simple and clean.

    Next, we must describe the syntax of our new language in itself. Once created and checked
    manually that it is correct, we can use it to check another scripts.

    Note that our new language loading code does not have to check the syntax. It is because we
    assume that the script describing itself is correct, and other scripts can be syntactically
    checked by the former script. The loading code must only do semantic checking which leads us to
    simple resolving references.

    THE LANGUAGE
    ------------

    Here I will describe the syntax of the new language (further called "Synek"). It is mainly a
    sequence of declarations terminated by a semicolon. The declaration consists of a symbol,
    which is an identifier, and its definition. A definition is in turn a sequence of specifiers
    connected with ".and" or ".or" operator. These operators cannot be mixed together in a one
    definition. Specifier can be a symbol, string, character, character range or a special
    keyword ".true" or ".false".

    On the very beginning of the script there is a declaration of a root symbol and is in the form:
        .syntax <root_symbol>;
    The <root_symbol> must be on of the symbols in declaration sequence. The syntax is correct if
    the root symbol evaluates to true. A symbol evaluates to true if the definition associated with
    the symbol evaluates to true. Definition evaluation depends on the operator used to connect
    specifiers in the definition. If ".and" operator is used, definition evaluates to true if and
    only if all the specifiers evaluate to true. If ".or" operator is used, definition evalutes to
    true if any of the specifiers evaluates to true. If definition contains only one specifier,
    it is evaluated as if it was connected with ".true" keyword by ".and" operator.

    If specifier is a ".true" keyword, it always evaluates to true.

    If specifier is a ".false" keyword, it always evaluates to false. Specifier evaluates to false
    when it does not evaluate to true.

    Character range specifier is in the form:
        '<first_character>' - '<second_character>'
    If specifier is a character range, it evaluates to true if character in the stream is greater
    or equal to <first_character> and less or equal to <second_character>. In that situation 
    the stream pointer is advanced to point to next character in the stream. All C-style escape
    sequences are supported although trigraph sequences are not. The comparisions are performed
    on 8-bit unsigned integers.

    Character specifier is in the form:
        '<single_character>'
    It evaluates to true if the following character range specifier evaluates to true:
        '<single_character>' - '<single_character>'

    String specifier is in the form:
        "<string>"
    Let N be the number of characters in <string>. Let <string>[i] designate i-th character in
    <string>. Then the string specifier evaluates to true if and only if for i in the range [0, N)
    the following character specifier evaluates to true:
        '<string>[i]'
    If <string>[i] is a quotation mark, '<string>[i]' is replaced with '\<string>[i]'.

    Symbol specifier can be optionally preceded by a ".loop" keyword in the form:
        .loop <symbol>                  (1)
    where <symbol> is defined as follows:
        <symbol> <definition>;          (2)
    Construction (1) is replaced by the following code:
        <symbol$1>
    and declaration (2) is replaced by the following:
        <symbol$1> <symbol$2> .or .true;
        <symbol$2> <symbol> .and <symbol$1>;
        <symbol> <definition>;

    Synek supports also a register mechanizm. User can, in its SYN file, declare a number of
    registers that can be accessed in the syn body. Each reg has its name and a default value.
    The register is one byte wide. The C code can change the default value by calling
    grammar_set_reg8() with grammar id, register name and a new value. As we know, each rule is
    a sequence of specifiers joined with .and or .or operator. And now each specifier can be
    prefixed with a condition expression in a form ".if (<reg_name> <operator> <hex_literal>)"
    where <operator> can be == or !=. If the condition evaluates to false, the specifier
    evaluates to .false. Otherwise it evalutes to the specifier.

    ESCAPE SEQUENCES
    ----------------

    Synek supports all escape sequences in character specifiers. The mapping table is listed below.
    All occurences of the characters in the first column are replaced with the corresponding
    character in the second column.

        Escape sequence         Represents
    ------------------------------------------------------------------------------------------------
        \a                      Bell (alert)
        \b                      Backspace
        \f                      Formfeed
        \n                      New line
        \r                      Carriage return
        \t                      Horizontal tab
        \v                      Vertical tab
        \'                      Single quotation mark
        \"                      Double quotation mark
        \\                      Backslash
        \?                      Literal question mark
        \ooo                    ASCII character in octal notation
        \xhhh                   ASCII character in hexadecimal notation
    ------------------------------------------------------------------------------------------------

    RAISING ERRORS
    --------------

    Any specifier can be followed by a special construction that is executed when the specifier
    evaluates to false. The construction is in the form:
        .error <ERROR_TEXT>
    <ERROR_TEXT> is an identifier declared earlier by error text declaration. The declaration is
    in the form:
        .errtext <ERROR_TEXT> "<error_desc>"
    When specifier evaluates to false and this construction is present, parsing is stopped
    immediately and <error_desc> is returned as a result of parsing. The error position is also
    returned and it is meant as an offset from the beggining of the stream to the character that
    was valid so far. Example:

        (**** syntax script ****)

        .syntax program;
        .errtext MISSING_SEMICOLON      "missing ';'"
        program         declaration .and .loop space .and ';' .error MISSING_SEMICOLON .and
                        .loop space .and '\0';
        declaration     "declare" .and .loop space .and identifier;
        space           ' ';

        (**** sample code ****)

        declare foo ,

    In the example above checking the sample code will result in error message "missing ';'" and
    error position 12. The sample code is not correct. Note the presence of '\0' specifier to
    assure that there is no code after semicolon - only spaces.
    <error_desc> can optionally contain identifier surrounded by dollar signs $. In such a case,
    the identifier and dollar signs are replaced by a string retrieved by invoking symbol with
    the identifier name. The starting position is the error position. The lenght of the resulting
    string is the position after invoking the symbol.

    PRODUCTION
    ----------

    Synek not only checks the syntax but it can also produce (emit) bytes associated with specifiers
    that evaluate to true. That is, every specifier and optional error construction can be followed
    by a number of emit constructions that are in the form:
        .emit <parameter>
    <paramater> can be a HEX number, identifier, a star * or a dollar $. HEX number is preceded by
    0x or 0X. If <parameter> is an identifier, it must be earlier declared by emit code declaration
    in the form:
        .emtcode <identifier> <hex_number>

    When given specifier evaluates to true, all emits associated with the specifier are output
    in order they were declared. A star means that last-read character should be output instead
    of constant value. Example:

        (**** syntax script ****)

        .syntax foobar;
        .emtcode WORD_FOO       0x01
        .emtcode WORD_BAR       0x02
        foobar      FOO .emit WORD_FOO .or BAR .emit WORD_BAR .or .true .emit 0x00;
        FOO         "foo" .and SPACE;
        BAR         "bar" .and SPACE;
        SPACE       ' ' .or '\0';

        (**** sample text 1 ****)

        foo

        (**** sample text 2 ****)

        foobar

    For both samples the result will be one-element array. For first sample text it will be
    value 1, for second - 0. Note that every text will be accepted because of presence of
    .true as an alternative.

    Another example:

        (**** syntax script ****)

        .syntax declaration;
        .emtcode VARIABLE       0x01
        declaration     "declare" .and .loop space .and
                        identifier .emit VARIABLE .and          (1)
                        .true .emit 0x00 .and                   (2)
                        .loop space .and ';';
        space           ' ' .or '\t';
        identifier      .loop id_char .emit *;                  (3)
        id_char         'a'-'z' .or 'A'-'Z' .or '_';

        (**** sample code ****)

        declare    fubar;

    In specifier (1) symbol <identifier> is followed by .emit VARIABLE. If it evaluates to
    true, VARIABLE constant and then production of the symbol is output. Specifier (2) is used
    to terminate the string with null to signal when the string ends. Specifier (3) outputs
    all characters that make declared identifier. The result of sample code will be the
    following array:
        { 1, 'f', 'u', 'b', 'a', 'r', 0 }

    If .emit is followed by dollar $, it means that current position should be output. Current
    position is a 32-bit unsigned integer distance from the very beginning of the parsed string to
    first character consumed by the specifier associated with the .emit instruction. Current
    position is stored in the output buffer in Little-Endian convention (the lowest byte comes
    first).
*/

#include <stdio.h>

static void mem_free (void **);

/*
    internal error messages
*/
static const byte *OUT_OF_MEMORY =          (byte *) "internal error 1001: out of physical memory";
static const byte *UNRESOLVED_REFERENCE =   (byte *) "internal error 1002: unresolved reference '$'";
static const byte *INVALID_GRAMMAR_ID =     (byte *) "internal error 1003: invalid grammar object";
static const byte *INVALID_REGISTER_NAME =  (byte *) "internal error 1004: invalid register name: '$'";
/*static const byte *DUPLICATE_IDENTIFIER =   (byte *) "internal error 1005: identifier '$' already defined";*/
static const byte *UNREFERENCED_IDENTIFIER =(byte *) "internal error 1006: unreferenced identifier '$'";

static const byte *error_message = NULL;    /* points to one of the error messages above */
static byte *error_param = NULL;        /* this is inserted into error_message in place of $ */
static int error_position = -1;

static byte *unknown = (byte *) "???";

static void clear_last_error (void)
{
    /* reset error message */
    error_message = NULL;

    /* free error parameter - if error_param is a "???" don't free it - it's static */
    if (error_param != unknown)
        mem_free ((void **) (void *) &error_param);
    else
        error_param = NULL;

    /* reset error position */
    error_position = -1;
}

static void set_last_error (const byte *msg, byte *param, int pos)
{
    /* error message can be set only once */
    if (error_message != NULL)
    {
        mem_free ((void **) (void *) &param);
        return;
    }

    error_message = msg;

    /* if param is NULL, set error_param to unknown ("???") */
    /* note: do not try to strdup the "???" - it may be that we are here because of */
    /* out of memory error so strdup can fail */
    if (param != NULL)
        error_param = param;
    else
        error_param = unknown;

    error_position = pos;
}

/*
    memory management routines
*/
static void *mem_alloc (size_t size)
{
    void *ptr = grammar_alloc_malloc (size);
    if (ptr == NULL)
        set_last_error (OUT_OF_MEMORY, NULL, -1);
    return ptr;
}

static void *mem_copy (void *dst, const void *src, size_t size)
{
    return grammar_memory_copy (dst, src, size);
}

static void mem_free (void **ptr)
{
    grammar_alloc_free (*ptr);
    *ptr = NULL;
}

static void *mem_realloc (void *ptr, size_t old_size, size_t new_size)
{
    void *ptr2 = grammar_alloc_realloc (ptr, old_size, new_size);
    if (ptr2 == NULL)
        set_last_error (OUT_OF_MEMORY, NULL, -1);
    return ptr2;
}

static byte *str_copy_n (byte *dst, const byte *src, size_t max_len)
{
    return grammar_string_copy_n (dst, src, max_len);
}

static byte *str_duplicate (const byte *str)
{
    byte *new_str = grammar_string_duplicate (str);
    if (new_str == NULL)
        set_last_error (OUT_OF_MEMORY, NULL, -1);
    return new_str;
}

static int str_equal (const byte *str1, const byte *str2)
{
    return grammar_string_compare (str1, str2) == 0;
}

static int str_equal_n (const byte *str1, const byte *str2, unsigned int n)
{
    return grammar_string_compare_n (str1, str2, n) == 0;
}

static int
str_length (const byte *str)
{
   return (int) (grammar_string_length (str));
}

/*
    useful macros
*/
#define GRAMMAR_IMPLEMENT_LIST_APPEND(_Ty)\
    static void _Ty##_append (_Ty **x, _Ty *nx) {\
        while (*x) x = &(**x).next;\
        *x = nx;\
    }

/*
    string to byte map typedef
*/
typedef struct map_byte_
{
    byte *key;
    byte data;
    struct map_byte_ *next;
} map_byte;

static void map_byte_create (map_byte **ma)
{
    *ma = (map_byte *) mem_alloc (sizeof (map_byte));
    if (*ma)
    {
        (**ma).key = NULL;
        (**ma).data = '\0';
        (**ma).next = NULL;
    }
}

static void map_byte_destroy (map_byte **ma)
{
    if (*ma)
    {
        map_byte_destroy (&(**ma).next);
        mem_free ((void **) &(**ma).key);
        mem_free ((void **) ma);
    }
}

GRAMMAR_IMPLEMENT_LIST_APPEND(map_byte)

/*
    searches the map for the specified key,
    returns pointer to the element with the specified key if it exists
    returns NULL otherwise
*/
static map_byte *map_byte_locate (map_byte **ma, const byte *key)
{
    while (*ma)
    {
        if (str_equal ((**ma).key, key))
            return *ma;

        ma = &(**ma).next;
    }

    set_last_error (UNRESOLVED_REFERENCE, str_duplicate (key), -1);
    return NULL;
}

/*
    searches the map for specified key,
    if the key is matched, *data is filled with data associated with the key,
    returns 0 if the key is matched,
    returns 1 otherwise
*/
static int map_byte_find (map_byte **ma, const byte *key, byte *data)
{
    map_byte *found = map_byte_locate (ma, key);
    if (found != NULL)
    {
        *data = found->data;

        return 0;
    }

    return 1;
}

/*
    regbyte context typedef

    Each regbyte consists of its name and a default value. These are static and created at
    grammar script compile-time, for example the following line:
        .regbyte vertex_blend      0x00
    adds a new regbyte named "vertex_blend" to the static list and initializes it to 0.
    When the script is executed, this regbyte can be accessed by name for read and write. When a
    particular regbyte is written, a new regbyte_ctx entry is added to the top of the regbyte_ctx
    stack. The new entry contains information abot which regbyte it references and its new value.
    When a given regbyte is accessed for read, the stack is searched top-down to find an
    entry that references the regbyte. The first matching entry is used to return the current
    value it holds. If no entry is found, the default value is returned.
*/
typedef struct regbyte_ctx_
{
    map_byte *m_regbyte;
    byte m_current_value;
    struct regbyte_ctx_ *m_prev;
} regbyte_ctx;

static void regbyte_ctx_create (regbyte_ctx **re)
{
    *re = (regbyte_ctx *) mem_alloc (sizeof (regbyte_ctx));
    if (*re)
    {
        (**re).m_regbyte = NULL;
        (**re).m_prev = NULL;
    }
}

static void regbyte_ctx_destroy (regbyte_ctx **re)
{
    if (*re)
    {
        mem_free ((void **) re);
    }
}

static byte regbyte_ctx_extract (regbyte_ctx **re, map_byte *reg)
{
    /* first lookup in the register stack */
    while (*re != NULL)
    {
        if ((**re).m_regbyte == reg)
            return (**re).m_current_value;

        re = &(**re).m_prev;
    }

    /* if not found - return the default value */
    return reg->data;
}

/*
    emit type typedef
*/
typedef enum emit_type_
{
    et_byte,            /* explicit number */
    et_stream,          /* eaten character */
    et_position         /* current position */
} emit_type;

/*
    emit destination typedef
*/
typedef enum emit_dest_
{
    ed_output,          /* write to the output buffer */
    ed_regbyte          /* write a particular regbyte */
} emit_dest;

/*
    emit typedef
*/
typedef struct emit_
{
    emit_dest m_emit_dest;
    emit_type m_emit_type;      /* ed_output */
    byte m_byte;                /* et_byte */
    map_byte *m_regbyte;        /* ed_regbyte */
    byte *m_regname;            /* ed_regbyte - temporary */
    struct emit_ *m_next;
} emit;

static void emit_create (emit **em)
{
    *em = (emit *) mem_alloc (sizeof (emit));
    if (*em)
    {
        (**em).m_emit_dest = ed_output;
        (**em).m_emit_type = et_byte;
        (**em).m_byte = '\0';
        (**em).m_regbyte = NULL;
        (**em).m_regname = NULL;
        (**em).m_next = NULL;
    }
}

static void emit_destroy (emit **em)
{
    if (*em)
    {
        emit_destroy (&(**em).m_next);
        mem_free ((void **) &(**em).m_regname);
        mem_free ((void **) em);
    }
}

static unsigned int emit_size (emit *_E)
{
    unsigned int n = 0;

    while (_E != NULL)
    {
        if (_E->m_emit_dest == ed_output)
        {
            if (_E->m_emit_type == et_position)
                n += 4;     /* position is a 32-bit unsigned integer */
            else
                n++;
        }
        _E = _E->m_next;
    }

    return n;
}

static int emit_push (emit *_E, byte *_P, byte c, unsigned int _Pos, regbyte_ctx **_Ctx)
{
    while (_E != NULL)
    {
        if (_E->m_emit_dest == ed_output)
        {
            if (_E->m_emit_type == et_byte)
                *_P++ = _E->m_byte;
            else if (_E->m_emit_type == et_stream)
                *_P++ = c;
            else /* _Em->type == et_position */
            {
                *_P++ = (byte) (_Pos);
                *_P++ = (byte) (_Pos >> 8);
                *_P++ = (byte) (_Pos >> 16);
                *_P++ = (byte) (_Pos >> 24);
            }
        }
        else
        {
            regbyte_ctx *new_rbc;
            regbyte_ctx_create (&new_rbc);
            if (new_rbc == NULL)
                return 1;

            new_rbc->m_prev = *_Ctx;
            new_rbc->m_regbyte = _E->m_regbyte;
            *_Ctx = new_rbc;

            if (_E->m_emit_type == et_byte)
                new_rbc->m_current_value = _E->m_byte;
            else if (_E->m_emit_type == et_stream)
                new_rbc->m_current_value = c;
        }

        _E = _E->m_next;
    }

    return 0;
}

/*
    error typedef
*/
typedef struct error_
{
    byte *m_text;
    byte *m_token_name;
    struct rule_ *m_token;
} error;

static void error_create (error **er)
{
    *er = (error *) mem_alloc (sizeof (error));
    if (*er)
    {
        (**er).m_text = NULL;
        (**er).m_token_name = NULL;
        (**er).m_token = NULL;
    }
}

static void error_destroy (error **er)
{
    if (*er)
    {
        mem_free ((void **) &(**er).m_text);
        mem_free ((void **) &(**er).m_token_name);
        mem_free ((void **) er);
    }
}

struct dict_;

static byte *
error_get_token (error *, struct dict_ *, const byte *, int);

/*
    condition operand type typedef
*/
typedef enum cond_oper_type_
{
    cot_byte,               /* constant 8-bit unsigned integer */
    cot_regbyte             /* pointer to byte register containing the current value */
} cond_oper_type;

/*
    condition operand typedef
*/
typedef struct cond_oper_
{
    cond_oper_type m_type;
    byte m_byte;            /* cot_byte */
    map_byte *m_regbyte;    /* cot_regbyte */
    byte *m_regname;        /* cot_regbyte - temporary */
} cond_oper;

/*
    condition type typedef
*/
typedef enum cond_type_
{
    ct_equal,
    ct_not_equal
} cond_type;

/*
    condition typedef
*/
typedef struct cond_
{
    cond_type m_type;
    cond_oper m_operands[2];
} cond;

static void cond_create (cond **co)
{
    *co = (cond *) mem_alloc (sizeof (cond));
    if (*co)
    {
        (**co).m_operands[0].m_regname = NULL;
        (**co).m_operands[1].m_regname = NULL;
    }
}

static void cond_destroy (cond **co)
{
    if (*co)
    {
        mem_free ((void **) &(**co).m_operands[0].m_regname);
        mem_free ((void **) &(**co).m_operands[1].m_regname);
        mem_free ((void **) co);
    }
}

/*
    specifier type typedef
*/
typedef enum spec_type_
{
    st_false,
    st_true,
    st_byte,
    st_byte_range,
    st_string,
    st_identifier,
    st_identifier_loop,
    st_debug
} spec_type;

/*
    specifier typedef
*/
typedef struct spec_
{
    spec_type m_spec_type;
    byte m_byte[2];                 /* st_byte, st_byte_range */
    byte *m_string;                 /* st_string */
    struct rule_ *m_rule;           /* st_identifier, st_identifier_loop */
    emit *m_emits;
    error *m_errtext;
    cond *m_cond;
    struct spec_ *next;
} spec;

static void spec_create (spec **sp)
{
    *sp = (spec *) mem_alloc (sizeof (spec));
    if (*sp)
    {
        (**sp).m_spec_type = st_false;
        (**sp).m_byte[0] = '\0';
        (**sp).m_byte[1] = '\0';
        (**sp).m_string = NULL;
        (**sp).m_rule = NULL;
        (**sp).m_emits = NULL;
        (**sp).m_errtext = NULL;
        (**sp).m_cond = NULL;
        (**sp).next = NULL;
    }
}

static void spec_destroy (spec **sp)
{
    if (*sp)
    {
        spec_destroy (&(**sp).next);
        emit_destroy (&(**sp).m_emits);
        error_destroy (&(**sp).m_errtext);
        mem_free ((void **) &(**sp).m_string);
        cond_destroy (&(**sp).m_cond);
        mem_free ((void **) sp);
    }
}

GRAMMAR_IMPLEMENT_LIST_APPEND(spec)

/*
    operator typedef
*/
typedef enum oper_
{
    op_none,
    op_and,
    op_or
} oper;

/*
    rule typedef
*/
typedef struct rule_
{
    oper m_oper;
    spec *m_specs;
    struct rule_ *next;
    int m_referenced;
} rule;

static void rule_create (rule **ru)
{
    *ru = (rule *) mem_alloc (sizeof (rule));
    if (*ru)
    {
        (**ru).m_oper = op_none;
        (**ru).m_specs = NULL;
        (**ru).next = NULL;
        (**ru).m_referenced = 0;
    }
}

static void rule_destroy (rule **ru)
{
    if (*ru)
    {
        rule_destroy (&(**ru).next);
        spec_destroy (&(**ru).m_specs);
        mem_free ((void **) ru);
    }
}

GRAMMAR_IMPLEMENT_LIST_APPEND(rule)

/*
    returns unique grammar id
*/
static grammar next_valid_grammar_id (void)
{
    static grammar id = 0;

    return ++id;
}

/*
    dictionary typedef
*/
typedef struct dict_
{
    rule *m_rulez;
    rule *m_syntax;
    rule *m_string;
    map_byte *m_regbytes;
    grammar m_id;
    struct dict_ *next;
} dict;

static void dict_create (dict **di)
{
    *di = (dict *) mem_alloc (sizeof (dict));
    if (*di)
    {
        (**di).m_rulez = NULL;
        (**di).m_syntax = NULL;
        (**di).m_string = NULL;
        (**di).m_regbytes = NULL;
        (**di).m_id = next_valid_grammar_id ();
        (**di).next = NULL;
    }
}

static void dict_destroy (dict **di)
{
    if (*di)
    {
        rule_destroy (&(**di).m_rulez);
        map_byte_destroy (&(**di).m_regbytes);
        mem_free ((void **) di);
    }
}

GRAMMAR_IMPLEMENT_LIST_APPEND(dict)

static void dict_find (dict **di, grammar key, dict **data)
{
    while (*di)
    {
        if ((**di).m_id == key)
        {
            *data = *di;
            return;
        }

        di = &(**di).next;
    }

    *data = NULL;
}

static dict *g_dicts = NULL;

/*
    byte array typedef
*/
typedef struct barray_
{
    byte *data;
    unsigned int len;
} barray;

static void barray_create (barray **ba)
{
    *ba = (barray *) mem_alloc (sizeof (barray));
    if (*ba)
    {
        (**ba).data = NULL;
        (**ba).len = 0;
    }
}

static void barray_destroy (barray **ba)
{
    if (*ba)
    {
        mem_free ((void **) &(**ba).data);
        mem_free ((void **) ba);
    }
}

/*
    reallocates byte array to requested size,
    returns 0 on success,
    returns 1 otherwise
*/
static int barray_resize (barray **ba, unsigned int nlen)
{
    byte *new_pointer;

    if (nlen == 0)
    {
        mem_free ((void **) &(**ba).data);
        (**ba).data = NULL;
        (**ba).len = 0;

        return 0;
    }
    else
    {
        new_pointer = (byte *) mem_realloc ((**ba).data, (**ba).len * sizeof (byte),
            nlen * sizeof (byte));
        if (new_pointer)
        {
            (**ba).data = new_pointer;
            (**ba).len = nlen;

            return 0;
        }
    }

    return 1;
}

/*
    adds byte array pointed by *nb to the end of array pointed by *ba,
    returns 0 on success,
    returns 1 otherwise
*/
static int barray_append (barray **ba, barray **nb)
{
    const unsigned int len = (**ba).len;

    if (barray_resize (ba, (**ba).len + (**nb).len))
        return 1;

    mem_copy ((**ba).data + len, (**nb).data, (**nb).len);

    return 0;
}

/*
    adds emit chain pointed by em to the end of array pointed by *ba,
    returns 0 on success,
    returns 1 otherwise
*/
static int barray_push (barray **ba, emit *em, byte c, unsigned int pos, regbyte_ctx **rbc)
{
    unsigned int count = emit_size (em);

    if (barray_resize (ba, (**ba).len + count))
        return 1;

    return emit_push (em, (**ba).data + ((**ba).len - count), c, pos, rbc);
}

/*
    byte pool typedef
*/
typedef struct bytepool_
{
    byte *_F;
    unsigned int _Siz;
} bytepool;

static void bytepool_destroy (bytepool **by)
{
    if (*by != NULL)
    {
        mem_free ((void **) &(**by)._F);
        mem_free ((void **) by);
    }
}

static void bytepool_create (bytepool **by, int len)
{
    *by = (bytepool *) (mem_alloc (sizeof (bytepool)));
    if (*by != NULL)
    {
        (**by)._F = (byte *) (mem_alloc (sizeof (byte) * len));
        (**by)._Siz = len;

        if ((**by)._F == NULL)
            bytepool_destroy (by);
    }
}

static int bytepool_reserve (bytepool *by, unsigned int n)
{
    byte *_P;

    if (n <= by->_Siz)
        return 0;

    /* byte pool can only grow and at least by doubling its size */
    n = n >= by->_Siz * 2 ? n : by->_Siz * 2;

    /* reallocate the memory and adjust pointers to the new memory location */
    _P = (byte *) (mem_realloc (by->_F, sizeof (byte) * by->_Siz, sizeof (byte) * n));
    if (_P != NULL)
    {
        by->_F = _P;
        by->_Siz = n;
        return 0;
    }

    return 1;
}

/*
    string to string map typedef
*/
typedef struct map_str_
{
    byte *key;
    byte *data;
    struct map_str_ *next;
} map_str;

static void map_str_create (map_str **ma)
{
    *ma = (map_str *) mem_alloc (sizeof (map_str));
    if (*ma)
    {
        (**ma).key = NULL;
        (**ma).data = NULL;
        (**ma).next = NULL;
    }
}

static void map_str_destroy (map_str **ma)
{
    if (*ma)
    {
        map_str_destroy (&(**ma).next);
        mem_free ((void **) &(**ma).key);
        mem_free ((void **) &(**ma).data);
        mem_free ((void **) ma);
    }
}

GRAMMAR_IMPLEMENT_LIST_APPEND(map_str)

/*
    searches the map for specified key,
    if the key is matched, *data is filled with data associated with the key,
    returns 0 if the key is matched,
    returns 1 otherwise
*/
static int map_str_find (map_str **ma, const byte *key, byte **data)
{
    while (*ma)
    {
        if (str_equal ((**ma).key, key))
        {
            *data = str_duplicate ((**ma).data);
            if (*data == NULL)
                return 1;

            return 0;
        }

        ma = &(**ma).next;
    }

    set_last_error (UNRESOLVED_REFERENCE, str_duplicate (key), -1);
    return 1;
}

/*
    string to rule map typedef
*/
typedef struct map_rule_
{
    byte *key;
    rule *data;
    struct map_rule_ *next;
} map_rule;

static void map_rule_create (map_rule **ma)
{
    *ma = (map_rule *) mem_alloc (sizeof (map_rule));
    if (*ma)
    {
        (**ma).key = NULL;
        (**ma).data = NULL;
        (**ma).next = NULL;
    }
}

static void map_rule_destroy (map_rule **ma)
{
    if (*ma)
    {
        map_rule_destroy (&(**ma).next);
        mem_free ((void **) &(**ma).key);
        mem_free ((void **) ma);
    }
}

GRAMMAR_IMPLEMENT_LIST_APPEND(map_rule)

/*
    searches the map for specified key,
    if the key is matched, *data is filled with data associated with the key,
    returns 0 if the is matched,
    returns 1 otherwise
*/
static int map_rule_find (map_rule **ma, const byte *key, rule **data)
{
    while (*ma)
    {
        if (str_equal ((**ma).key, key))
        {
            *data = (**ma).data;

            return 0;
        }

        ma = &(**ma).next;
    }

    set_last_error (UNRESOLVED_REFERENCE, str_duplicate (key), -1);
    return 1;
}

/*
    returns 1 if given character is a white space,
    returns 0 otherwise
*/
static int is_space (byte c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

/*
    advances text pointer by 1 if character pointed by *text is a space,
    returns 1 if a space has been eaten,
    returns 0 otherwise
*/
static int eat_space (const byte **text)
{
    if (is_space (**text))
    {
        (*text)++;

        return 1;
    }

    return 0;
}

/*
    returns 1 if text points to C-style comment start string,
    returns 0 otherwise
*/
static int is_comment_start (const byte *text)
{
    return text[0] == '/' && text[1] == '*';
}

/*
    advances text pointer to first character after C-style comment block - if any,
    returns 1 if C-style comment block has been encountered and eaten,
    returns 0 otherwise
*/
static int eat_comment (const byte **text)
{
    if (is_comment_start (*text))
    {
        /* *text points to comment block - skip two characters to enter comment body */
        *text += 2;
        /* skip any character except consecutive '*' and '/' */
        while (!((*text)[0] == '*' && (*text)[1] == '/'))
            (*text)++;
        /* skip those two terminating characters */
        *text += 2;

        return 1;
    }

    return 0;
}

/*
    advances text pointer to first character that is neither space nor C-style comment block
*/
static void eat_spaces (const byte **text)
{
    while (eat_space (text) || eat_comment (text))
        ;
}

/*
    resizes string pointed by *ptr to successfully add character c to the end of the string,
    returns 0 on success,
    returns 1 otherwise
*/
static int string_grow (byte **ptr, unsigned int *len, byte c)
{
    /* reallocate the string in 16-byte increments */
    if ((*len & 0x0F) == 0x0F || *ptr == NULL)
    {
        byte *tmp = (byte *) mem_realloc (*ptr, ((*len + 1) & ~0x0F) * sizeof (byte),
            ((*len + 1 + 0x10) & ~0x0F) * sizeof (byte));
        if (tmp == NULL)
            return 1;

        *ptr = tmp;
    }

    if (c)
    {
        /* append given character */
        (*ptr)[*len] = c;
        (*len)++;
    }
    (*ptr)[*len] = '\0';

    return 0;
}

/*
    returns 1 if given character is a valid identifier character a-z, A-Z, 0-9 or _
    returns 0 otherwise
*/
static int is_identifier (byte c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

/*
    copies characters from *text to *id until non-identifier character is encountered,
    assumes that *id points to NULL object - caller is responsible for later freeing the string,
    text pointer is advanced to point past the copied identifier,
    returns 0 if identifier was successfully copied,
    returns 1 otherwise
*/
static int get_identifier (const byte **text, byte **id)
{
    const byte *t = *text;
    byte *p = NULL;
    unsigned int len = 0;

    if (string_grow (&p, &len, '\0'))
        return 1;

    /* loop while next character in buffer is valid for identifiers */
    while (is_identifier (*t))
    {
        if (string_grow (&p, &len, *t++))
        {
            mem_free ((void **) (void *) &p);
            return 1;
        }
    }

    *text = t;
    *id = p;

    return 0;
}

/*
    converts sequence of DEC digits pointed by *text until non-DEC digit is encountered,
    advances text pointer past the converted sequence,
    returns the converted value
*/
static unsigned int dec_convert (const byte **text)
{
    unsigned int value = 0;

    while (**text >= '0' && **text <= '9')
    {
        value = value * 10 + **text - '0';
        (*text)++;
    }

    return value;
}

/*
    returns 1 if given character is HEX digit 0-9, A-F or a-f,
    returns 0 otherwise
*/
static int is_hex (byte c)
{
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

/*
    returns value of passed character as if it was HEX digit
*/
static unsigned int hex2dec (byte c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return c - 'a' + 10;
}

/*
    converts sequence of HEX digits pointed by *text until non-HEX digit is encountered,
    advances text pointer past the converted sequence,
    returns the converted value
*/
static unsigned int hex_convert (const byte **text)
{
    unsigned int value = 0;

    while (is_hex (**text))
    {
        value = value * 0x10 + hex2dec (**text);
        (*text)++;
    }

    return value;
}

/*
    returns 1 if given character is OCT digit 0-7,
    returns 0 otherwise
*/
static int is_oct (byte c)
{
    return c >= '0' && c <= '7';
}

/*
    returns value of passed character as if it was OCT digit
*/
static int oct2dec (byte c)
{
    return c - '0';
}

static byte get_escape_sequence (const byte **text)
{
    int value = 0;

    /* skip '\' character */
    (*text)++;

    switch (*(*text)++)
    {
    case '\'':
        return '\'';
    case '"':
        return '\"';
    case '?':
        return '\?';
    case '\\':
        return '\\';
    case 'a':
        return '\a';
    case 'b':
        return '\b';
    case 'f':
        return '\f';
    case 'n':
        return '\n';
    case 'r':
        return '\r';
    case 't':
        return '\t';
    case 'v':
        return '\v';
    case 'x':
        return (byte) hex_convert (text);
    }

    (*text)--;
    if (is_oct (**text))
    {
        value = oct2dec (*(*text)++);
        if (is_oct (**text))
        {
            value = value * 010 + oct2dec (*(*text)++);
            if (is_oct (**text))
                value = value * 010 + oct2dec (*(*text)++);
        }
    }

    return (byte) value;
}

/*
    copies characters from *text to *str until " or ' character is encountered,
    assumes that *str points to NULL object - caller is responsible for later freeing the string,
    assumes that *text points to " or ' character that starts the string,
    text pointer is advanced to point past the " or ' character,
    returns 0 if string was successfully copied,
    returns 1 otherwise
*/
static int get_string (const byte **text, byte **str)
{
    const byte *t = *text;
    byte *p = NULL;
    unsigned int len = 0;
    byte term_char;

    if (string_grow (&p, &len, '\0'))
        return 1;

    /* read " or ' character that starts the string */
    term_char = *t++;
    /* while next character is not the terminating character */
    while (*t && *t != term_char)
    {
        byte c;

        if (*t == '\\')
            c = get_escape_sequence (&t);
        else
            c = *t++;

        if (string_grow (&p, &len, c))
        {
            mem_free ((void **) (void *) &p);
            return 1;
        }
    }
    /* skip " or ' character that ends the string */
    t++;

    *text = t;
    *str = p;
    return 0;
}

/*
    gets emit code, the syntax is:
    ".emtcode" " " <symbol> " " (("0x" | "0X") <hex_value>) | <dec_value> | <character>
    assumes that *text already points to <symbol>,
    returns 0 if emit code is successfully read,
    returns 1 otherwise
*/
static int get_emtcode (const byte **text, map_byte **ma)
{
    const byte *t = *text;
    map_byte *m = NULL;

    map_byte_create (&m);
    if (m == NULL)
        return 1;

    if (get_identifier (&t, &m->key))
    {
        map_byte_destroy (&m);
        return 1;
    }
    eat_spaces (&t);

    if (*t == '\'')
    {
        byte *c;

        if (get_string (&t, &c))
        {
            map_byte_destroy (&m);
            return 1;
        }

        m->data = (byte) c[0];
        mem_free ((void **) (void *) &c);
    }
    else if (t[0] == '0' && (t[1] == 'x' || t[1] == 'X'))
    {
        /* skip HEX "0x" or "0X" prefix */
        t += 2;
        m->data = (byte) hex_convert (&t);
    }
    else
    {
        m->data = (byte) dec_convert (&t);
    }

    eat_spaces (&t);

    *text = t;
    *ma = m;
    return 0;
}

/*
    gets regbyte declaration, the syntax is:
    ".regbyte" " " <symbol> " " (("0x" | "0X") <hex_value>) | <dec_value> | <character>
    assumes that *text already points to <symbol>,
    returns 0 if regbyte is successfully read,
    returns 1 otherwise
*/
static int get_regbyte (const byte **text, map_byte **ma)
{
    /* pass it to the emtcode parser as it has the same syntax starting at <symbol> */
    return get_emtcode (text, ma);
}

/*
    returns 0 on success,
    returns 1 otherwise
*/
static int get_errtext (const byte **text, map_str **ma)
{
    const byte *t = *text;
    map_str *m = NULL;

    map_str_create (&m);
    if (m == NULL)
        return 1;

    if (get_identifier (&t, &m->key))
    {
        map_str_destroy (&m);
        return 1;
    }
    eat_spaces (&t);

    if (get_string (&t, &m->data))
    {
        map_str_destroy (&m);
        return 1;
    }
    eat_spaces (&t);

    *text = t;
    *ma = m;
    return 0;
}

/*
    returns 0 on success,
    returns 1 otherwise,
*/
static int get_error (const byte **text, error **er, map_str *maps)
{
    const byte *t = *text;
    byte *temp = NULL;

    if (*t != '.')
        return 0;

    t++;
    if (get_identifier (&t, &temp))
        return 1;
    eat_spaces (&t);

    if (!str_equal ((byte *) "error", temp))
    {
        mem_free ((void **) (void *) &temp);
        return 0;
    }

    mem_free ((void **) (void *) &temp);

    error_create (er);
    if (*er == NULL)
        return 1;

    if (*t == '\"')
    {
        if (get_string (&t, &(**er).m_text))
        {
            error_destroy (er);
            return 1;
        }
        eat_spaces (&t);
    }
    else
    {
        if (get_identifier (&t, &temp))
        {
            error_destroy (er);
            return 1;
        }
        eat_spaces (&t);

        if (map_str_find (&maps, temp, &(**er).m_text))
        {
            mem_free ((void **) (void *) &temp);
            error_destroy (er);
            return 1;
        }

        mem_free ((void **) (void *) &temp);
    }

    /* try to extract "token" from "...$token$..." */
    {
        byte *processed = NULL;
        unsigned int len = 0;
      int i = 0;

        if (string_grow (&processed, &len, '\0'))
        {
            error_destroy (er);
            return 1;
        }

        while (i < str_length ((**er).m_text))
        {
            /* check if the dollar sign is repeated - if so skip it */
            if ((**er).m_text[i] == '$' && (**er).m_text[i + 1] == '$')
            {
                if (string_grow (&processed, &len, '$'))
                {
                    mem_free ((void **) (void *) &processed);
                    error_destroy (er);
                    return 1;
                }

                i += 2;
            }
            else if ((**er).m_text[i] != '$')
            {
                if (string_grow (&processed, &len, (**er).m_text[i]))
                {
                    mem_free ((void **) (void *) &processed);
                    error_destroy (er);
                    return 1;
                }

                i++;
            }
            else
            {
                if (string_grow (&processed, &len, '$'))
                {
                    mem_free ((void **) (void *) &processed);
                    error_destroy (er);
                    return 1;
                }

                {
                    /* length of token being extracted */
                    unsigned int tlen = 0;

                    if (string_grow (&(**er).m_token_name, &tlen, '\0'))
                    {
                        mem_free ((void **) (void *) &processed);
                        error_destroy (er);
                        return 1;
                    }

                    /* skip the dollar sign */
                    i++;

                    while ((**er).m_text[i] != '$')
                    {
                        if (string_grow (&(**er).m_token_name, &tlen, (**er).m_text[i]))
                        {
                            mem_free ((void **) (void *) &processed);
                            error_destroy (er);
                            return 1;
                        }

                        i++;
                    }

                    /* skip the dollar sign */
                    i++;
                }
            }
        }

        mem_free ((void **) &(**er).m_text);
        (**er).m_text = processed;
    }

    *text = t;
    return 0;
}

/*
    returns 0 on success,
    returns 1 otherwise,
*/
static int get_emits (const byte **text, emit **em, map_byte *mapb)
{
    const byte *t = *text;
    byte *temp = NULL;
    emit *e = NULL;
    emit_dest dest;

    if (*t != '.')
        return 0;

    t++;
    if (get_identifier (&t, &temp))
        return 1;
    eat_spaces (&t);

    /* .emit */
    if (str_equal ((byte *) "emit", temp))
        dest = ed_output;
    /* .load */
    else if (str_equal ((byte *) "load", temp))
        dest = ed_regbyte;
    else
    {
        mem_free ((void **) (void *) &temp);
        return 0;
    }

    mem_free ((void **) (void *) &temp);

    emit_create (&e);
    if (e == NULL)
        return 1;

    e->m_emit_dest = dest;

    if (dest == ed_regbyte)
    {
        if (get_identifier (&t, &e->m_regname))
        {
            emit_destroy (&e);
            return 1;
        }
        eat_spaces (&t);
    }

    /* 0xNN */
    if (*t == '0' && (t[1] == 'x' || t[1] == 'X'))
    {
        t += 2;
        e->m_byte = (byte) hex_convert (&t);

        e->m_emit_type = et_byte;
    }
    /* NNN */
    else if (*t >= '0' && *t <= '9')
    {
        e->m_byte = (byte) dec_convert (&t);

        e->m_emit_type = et_byte;
    }
    /* * */
    else if (*t == '*')
    {
        t++;

        e->m_emit_type = et_stream;
    }
    /* $ */
    else if (*t == '$')
    {
        t++;

        e->m_emit_type = et_position;
    }
    /* 'c' */
    else if (*t == '\'')
    {
        if (get_string (&t, &temp))
        {
            emit_destroy (&e);
            return 1;
        }
        e->m_byte = (byte) temp[0];

        mem_free ((void **) (void *) &temp);

        e->m_emit_type = et_byte;
    }
    else
    {
        if (get_identifier (&t, &temp))
        {
            emit_destroy (&e);
            return 1;
        }

        if (map_byte_find (&mapb, temp, &e->m_byte))
        {
            mem_free ((void **) (void *) &temp);
            emit_destroy (&e);
            return 1;
        }

        mem_free ((void **) (void *) &temp);

        e->m_emit_type = et_byte;
    }

    eat_spaces (&t);

    if (get_emits (&t, &e->m_next, mapb))
    {
        emit_destroy (&e);
        return 1;
    }

    *text = t;
    *em = e;
    return 0;
}

/*
    returns 0 on success,
    returns 1 otherwise,
*/
static int get_spec (const byte **text, spec **sp, map_str *maps, map_byte *mapb)
{
    const byte *t = *text;
    spec *s = NULL;

    spec_create (&s);
    if (s == NULL)
        return 1;

    /* first - read optional .if statement */
    if (*t == '.')
    {
        const byte *u = t;
        byte *keyword = NULL;

        /* skip the dot */
        u++;

        if (get_identifier (&u, &keyword))
        {
            spec_destroy (&s);
            return 1;
        }

        /* .if */
        if (str_equal ((byte *) "if", keyword))
        {
            cond_create (&s->m_cond);
            if (s->m_cond == NULL)
            {
                spec_destroy (&s);
                return 1;
            }

            /* skip the left paren */
            eat_spaces (&u);
            u++;

            /* get the left operand */
            eat_spaces (&u);
            if (get_identifier (&u, &s->m_cond->m_operands[0].m_regname))
            {
                spec_destroy (&s);
                return 1;
            }
            s->m_cond->m_operands[0].m_type = cot_regbyte;

            /* get the operator (!= or ==) */
            eat_spaces (&u);
            if (*u == '!')
                s->m_cond->m_type = ct_not_equal;
            else
                s->m_cond->m_type = ct_equal;
            u += 2;
            eat_spaces (&u);

            if (u[0] == '0' && (u[1] == 'x' || u[1] == 'X'))
            {
                /* skip the 0x prefix */
                u += 2;

                /* get the right operand */
                s->m_cond->m_operands[1].m_byte = hex_convert (&u);
                s->m_cond->m_operands[1].m_type = cot_byte;
            }
            else /*if (*u >= '0' && *u <= '9')*/
            {
                /* get the right operand */
                s->m_cond->m_operands[1].m_byte = dec_convert (&u);
                s->m_cond->m_operands[1].m_type = cot_byte;
            }

            /* skip the right paren */
            eat_spaces (&u);
            u++;

            eat_spaces (&u);

            t = u;
        }

        mem_free ((void **) (void *) &keyword);
    }

    if (*t == '\'')
    {
        byte *temp = NULL;

        if (get_string (&t, &temp))
        {
            spec_destroy (&s);
            return 1;
        }
        eat_spaces (&t);

        if (*t == '-')
        {
            byte *temp2 = NULL;

            /* skip the '-' character */
            t++;
            eat_spaces (&t);

            if (get_string (&t, &temp2))
            {
                mem_free ((void **) (void *) &temp);
                spec_destroy (&s);
                return 1;
            }
            eat_spaces (&t);

            s->m_spec_type = st_byte_range;
            s->m_byte[0] = *temp;
            s->m_byte[1] = *temp2;

            mem_free ((void **) (void *) &temp2);
        }
        else
        {
            s->m_spec_type = st_byte;
            *s->m_byte = *temp;
        }

        mem_free ((void **) (void *) &temp);
    }
    else if (*t == '"')
    {
        if (get_string (&t, &s->m_string))
        {
            spec_destroy (&s);
            return 1;
        }
        eat_spaces (&t);

        s->m_spec_type = st_string;
    }
    else if (*t == '.')
    {
        byte *keyword = NULL;

        /* skip the dot */
        t++;

        if (get_identifier (&t, &keyword))
        {
            spec_destroy (&s);
            return 1;
        }
        eat_spaces (&t);

        /* .true */
        if (str_equal ((byte *) "true", keyword))
        {
            s->m_spec_type = st_true;
        }
        /* .false */
        else if (str_equal ((byte *) "false", keyword))
        {
            s->m_spec_type = st_false;
        }
        /* .debug */
        else if (str_equal ((byte *) "debug", keyword))
        {
            s->m_spec_type = st_debug;
        }
        /* .loop */
        else if (str_equal ((byte *) "loop", keyword))
        {
            if (get_identifier (&t, &s->m_string))
            {
                mem_free ((void **) (void *) &keyword);
                spec_destroy (&s);
                return 1;
            }
            eat_spaces (&t);

            s->m_spec_type = st_identifier_loop;
        }
        mem_free ((void **) (void *) &keyword);
    }
    else
    {
        if (get_identifier (&t, &s->m_string))
        {
            spec_destroy (&s);
            return 1;
        }
        eat_spaces (&t);

        s->m_spec_type = st_identifier;
    }

    if (get_error (&t, &s->m_errtext, maps))
    {
        spec_destroy (&s);
        return 1;
    }

    if (get_emits (&t, &s->m_emits, mapb))
    {
        spec_destroy (&s);
        return 1;
    }

    *text = t;
    *sp = s;
    return 0;
}

/*
    returns 0 on success,
    returns 1 otherwise,
*/
static int get_rule (const byte **text, rule **ru, map_str *maps, map_byte *mapb)
{
    const byte *t = *text;
    rule *r = NULL;

    rule_create (&r);
    if (r == NULL)
        return 1;

    if (get_spec (&t, &r->m_specs, maps, mapb))
    {
        rule_destroy (&r);
        return 1;
    }

    while (*t != ';')
    {
        byte *op = NULL;
        spec *sp = NULL;

        /* skip the dot that precedes "and" or "or" */
        t++;

        /* read "and" or "or" keyword */
        if (get_identifier (&t, &op))
        {
            rule_destroy (&r);
            return 1;
        }
        eat_spaces (&t);

        if (r->m_oper == op_none)
        {
            /* .and */
            if (str_equal ((byte *) "and", op))
                r->m_oper = op_and;
            /* .or */
            else
                r->m_oper = op_or;
        }

        mem_free ((void **) (void *) &op);

        if (get_spec (&t, &sp, maps, mapb))
        {
            rule_destroy (&r);
            return 1;
        }

        spec_append (&r->m_specs, sp);
    }

    /* skip the semicolon */
    t++;
    eat_spaces (&t);

    *text = t;
    *ru = r;
    return 0;
}

/*
    returns 0 on success,
    returns 1 otherwise,
*/
static int update_dependency (map_rule *mapr, byte *symbol, rule **ru)
{
    if (map_rule_find (&mapr, symbol, ru))
        return 1;

    (**ru).m_referenced = 1;

    return 0;
}

/*
    returns 0 on success,
    returns 1 otherwise,
*/
static int update_dependencies (dict *di, map_rule *mapr, byte **syntax_symbol,
    byte **string_symbol, map_byte *regbytes)
{
    rule *rulez = di->m_rulez;

    /* update dependecies for the root and lexer symbols */
    if (update_dependency (mapr, *syntax_symbol, &di->m_syntax) ||
        (*string_symbol != NULL && update_dependency (mapr, *string_symbol, &di->m_string)))
        return 1;

    mem_free ((void **) syntax_symbol);
    mem_free ((void **) string_symbol);

    /* update dependecies for the rest of the rules */
    while (rulez)
    {
        spec *sp = rulez->m_specs;

        /* iterate through all the specifiers */
        while (sp)
        {
            /* update dependency for identifier */
            if (sp->m_spec_type == st_identifier || sp->m_spec_type == st_identifier_loop)
            {
                if (update_dependency (mapr, sp->m_string, &sp->m_rule))
                    return 1;

                mem_free ((void **) &sp->m_string);
            }

            /* some errtexts reference to a rule */
            if (sp->m_errtext && sp->m_errtext->m_token_name)
            {
                if (update_dependency (mapr, sp->m_errtext->m_token_name, &sp->m_errtext->m_token))
                    return 1;

                mem_free ((void **) &sp->m_errtext->m_token_name);
            }

            /* update dependency for condition */
            if (sp->m_cond)
            {
                int i;
                for (i = 0; i < 2; i++)
                    if (sp->m_cond->m_operands[i].m_type == cot_regbyte)
                    {
                        sp->m_cond->m_operands[i].m_regbyte = map_byte_locate (&regbytes,
                            sp->m_cond->m_operands[i].m_regname);

                        if (sp->m_cond->m_operands[i].m_regbyte == NULL)
                            return 1;

                        mem_free ((void **) &sp->m_cond->m_operands[i].m_regname);
                    }
            }

            /* update dependency for all .load instructions */
            if (sp->m_emits)
            {
                emit *em = sp->m_emits;
                while (em != NULL)
                {
                    if (em->m_emit_dest == ed_regbyte)
                    {
                        em->m_regbyte = map_byte_locate (&regbytes, em->m_regname);

                        if (em->m_regbyte == NULL)
                            return 1;

                        mem_free ((void **) &em->m_regname);
                    }

                    em = em->m_next;
                }
            }

            sp = sp->next;
        }

        rulez = rulez->next;
    }

    /* check for unreferenced symbols */
    rulez = di->m_rulez;
    while (rulez != NULL)
    {
        if (!rulez->m_referenced)
        {
            map_rule *ma = mapr;
            while (ma)
            {
                if (ma->data == rulez)
                {
                    set_last_error (UNREFERENCED_IDENTIFIER, str_duplicate (ma->key), -1);
                    return 1;
                }
                ma = ma->next;
            }
        }
        rulez = rulez->next;
    }

    return 0;
}

static int satisfies_condition (cond *co, regbyte_ctx *ctx)
{
    byte values[2];
    int i;

    if (co == NULL)
        return 1;

    for (i = 0; i < 2; i++)
        switch (co->m_operands[i].m_type)
        {
        case cot_byte:
            values[i] = co->m_operands[i].m_byte;
            break;
        case cot_regbyte:
            values[i] = regbyte_ctx_extract (&ctx, co->m_operands[i].m_regbyte);
            break;
        }

    switch (co->m_type)
    {
    case ct_equal:
        return values[0] == values[1];
    case ct_not_equal:
        return values[0] != values[1];
    }

    return 0;
}

static void free_regbyte_ctx_stack (regbyte_ctx *top, regbyte_ctx *limit)
{
    while (top != limit)
    {
        regbyte_ctx *rbc = top->m_prev;
        regbyte_ctx_destroy (&top);
        top = rbc;
    }
}

typedef enum match_result_
{
    mr_not_matched,     /* the examined string does not match */
    mr_matched,         /* the examined string matches */
    mr_error_raised,    /* mr_not_matched + error has been raised */
    mr_dont_emit,       /* used by identifier loops only */
    mr_internal_error   /* an internal error has occured such as out of memory */
} match_result;

/*
 * This function does the main job. It parses the text and generates output data.
 */
static match_result
match (dict *di, const byte *text, int *index, rule *ru, barray **ba, int filtering_string,
       regbyte_ctx **rbc)
{
   int ind = *index;
    match_result status = mr_not_matched;
    spec *sp = ru->m_specs;
    regbyte_ctx *ctx = *rbc;

    /* for every specifier in the rule */
    while (sp)
    {
      int i, len, save_ind = ind;
        barray *array = NULL;

        if (satisfies_condition (sp->m_cond, ctx))
        {
            switch (sp->m_spec_type)
            {
            case st_identifier:
                barray_create (&array);
                if (array == NULL)
                {
                    free_regbyte_ctx_stack (ctx, *rbc);
                    return mr_internal_error;
                }

                status = match (di, text, &ind, sp->m_rule, &array, filtering_string, &ctx);

                if (status == mr_internal_error)
                {
                    free_regbyte_ctx_stack (ctx, *rbc);
                    barray_destroy (&array);
                    return mr_internal_error;
                }
                break;
            case st_string:
                len = str_length (sp->m_string);

                /* prefilter the stream */
                if (!filtering_string && di->m_string)
                {
                    barray *ba;
               int filter_index = 0;
                    match_result result;
                    regbyte_ctx *null_ctx = NULL;

                    barray_create (&ba);
                    if (ba == NULL)
                    {
                        free_regbyte_ctx_stack (ctx, *rbc);
                        return mr_internal_error;
                    }

                    result = match (di, text + ind, &filter_index, di->m_string, &ba, 1, &null_ctx);

                    if (result == mr_internal_error)
                    {
                        free_regbyte_ctx_stack (ctx, *rbc);
                        barray_destroy (&ba);
                        return mr_internal_error;
                    }

                    if (result != mr_matched)
                    {
                        barray_destroy (&ba);
                        status = mr_not_matched;
                        break;
                    }

                    barray_destroy (&ba);

                    if (filter_index != len || !str_equal_n (sp->m_string, text + ind, len))
                    {
                        status = mr_not_matched;
                        break;
                    }

                    status = mr_matched;
                    ind += len;
                }
                else
                {
                    status = mr_matched;
                    for (i = 0; status == mr_matched && i < len; i++)
                        if (text[ind + i] != sp->m_string[i])
                            status = mr_not_matched;

                    if (status == mr_matched)
                        ind += len;
                }
                break;
            case st_byte:
                status = text[ind] == *sp->m_byte ? mr_matched : mr_not_matched;
                if (status == mr_matched)
                    ind++;
                break;
            case st_byte_range:
                status = (text[ind] >= sp->m_byte[0] && text[ind] <= sp->m_byte[1]) ?
                    mr_matched : mr_not_matched;
                if (status == mr_matched)
                    ind++;
                break;
            case st_true:
                status = mr_matched;
                break;
            case st_false:
                status = mr_not_matched;
                break;
            case st_debug:
                status = ru->m_oper == op_and ? mr_matched : mr_not_matched;
                break;
            case st_identifier_loop:
                barray_create (&array);
                if (array == NULL)
                {
                    free_regbyte_ctx_stack (ctx, *rbc);
                    return mr_internal_error;
                }

                status = mr_dont_emit;
                for (;;)
                {
                    match_result result;

                    save_ind = ind;
                    result = match (di, text, &ind, sp->m_rule, &array, filtering_string, &ctx);

                    if (result == mr_error_raised)
                    {
                        status = result;
                        break;
                    }
                    else if (result == mr_matched)
                    {
                        if (barray_push (ba, sp->m_emits, text[ind - 1], save_ind, &ctx) ||
                            barray_append (ba, &array))
                        {
                            free_regbyte_ctx_stack (ctx, *rbc);
                            barray_destroy (&array);
                            return mr_internal_error;
                        }
                        barray_destroy (&array);
                        barray_create (&array);
                        if (array == NULL)
                        {
                            free_regbyte_ctx_stack (ctx, *rbc);
                            return mr_internal_error;
                        }
                    }
                    else if (result == mr_internal_error)
                    {
                        free_regbyte_ctx_stack (ctx, *rbc);
                        barray_destroy (&array);
                        return mr_internal_error;
                    }
                    else
                        break;
                }
                break;
            }
        }
        else
        {
            status = mr_not_matched;
        }

        if (status == mr_error_raised)
        {
            free_regbyte_ctx_stack (ctx, *rbc);
            barray_destroy (&array);

            return mr_error_raised;
        }

        if (ru->m_oper == op_and && status != mr_matched && status != mr_dont_emit)
        {
            free_regbyte_ctx_stack (ctx, *rbc);
            barray_destroy (&array);

            if (sp->m_errtext)
            {
                set_last_error (sp->m_errtext->m_text, error_get_token (sp->m_errtext, di, text,
                    ind), ind);

                return mr_error_raised;
            }

            return mr_not_matched;
        }

        if (status == mr_matched)
        {
            if (sp->m_emits)
                if (barray_push (ba, sp->m_emits, text[ind - 1], save_ind, &ctx))
                {
                    free_regbyte_ctx_stack (ctx, *rbc);
                    barray_destroy (&array);
                    return mr_internal_error;
                }

            if (array)
                if (barray_append (ba, &array))
                {
                    free_regbyte_ctx_stack (ctx, *rbc);
                    barray_destroy (&array);
                    return mr_internal_error;
                }
        }

        barray_destroy (&array);

        /* if the rule operator is a logical or, we pick up the first matching specifier */
        if (ru->m_oper == op_or && (status == mr_matched || status == mr_dont_emit))
        {
            *index = ind;
            *rbc = ctx;
            return mr_matched;
        }

        sp = sp->next;
    }

    /* everything went fine - all specifiers match up */
    if (ru->m_oper == op_and && (status == mr_matched || status == mr_dont_emit))
    {
        *index = ind;
        *rbc = ctx;
        return mr_matched;
    }

    free_regbyte_ctx_stack (ctx, *rbc);
    return mr_not_matched;
}

static match_result
fast_match (dict *di, const byte *text, int *index, rule *ru, int *_PP, bytepool *_BP,
            int filtering_string, regbyte_ctx **rbc)
{
   int ind = *index;
    int _P = filtering_string ? 0 : *_PP;
    int _P2;
    match_result status = mr_not_matched;
    spec *sp = ru->m_specs;
    regbyte_ctx *ctx = *rbc;

    /* for every specifier in the rule */
    while (sp)
    {
      int i, len, save_ind = ind;

        _P2 = _P + (sp->m_emits ? emit_size (sp->m_emits) : 0);
        if (bytepool_reserve (_BP, _P2))
        {
            free_regbyte_ctx_stack (ctx, *rbc);
            return mr_internal_error;
        }

        if (satisfies_condition (sp->m_cond, ctx))
        {
            switch (sp->m_spec_type)
            {
            case st_identifier:
                status = fast_match (di, text, &ind, sp->m_rule, &_P2, _BP, filtering_string, &ctx);

                if (status == mr_internal_error)
                {
                    free_regbyte_ctx_stack (ctx, *rbc);
                    return mr_internal_error;
                }
                break;
            case st_string:
                len = str_length (sp->m_string);

                /* prefilter the stream */
                if (!filtering_string && di->m_string)
                {
               int filter_index = 0;
                    match_result result;
                    regbyte_ctx *null_ctx = NULL;

                    result = fast_match (di, text + ind, &filter_index, di->m_string, NULL, _BP, 1, &null_ctx);

                    if (result == mr_internal_error)
                    {
                        free_regbyte_ctx_stack (ctx, *rbc);
                        return mr_internal_error;
                    }

                    if (result != mr_matched)
                    {
                        status = mr_not_matched;
                        break;
                    }

                    if (filter_index != len || !str_equal_n (sp->m_string, text + ind, len))
                    {
                        status = mr_not_matched;
                        break;
                    }

                    status = mr_matched;
                    ind += len;
                }
                else
                {
                    status = mr_matched;
                    for (i = 0; status == mr_matched && i < len; i++)
                        if (text[ind + i] != sp->m_string[i])
                            status = mr_not_matched;

                    if (status == mr_matched)
                        ind += len;
                }
                break;
            case st_byte:
                status = text[ind] == *sp->m_byte ? mr_matched : mr_not_matched;
                if (status == mr_matched)
                    ind++;
                break;
            case st_byte_range:
                status = (text[ind] >= sp->m_byte[0] && text[ind] <= sp->m_byte[1]) ?
                    mr_matched : mr_not_matched;
                if (status == mr_matched)
                    ind++;
                break;
            case st_true:
                status = mr_matched;
                break;
            case st_false:
                status = mr_not_matched;
                break;
            case st_debug:
                status = ru->m_oper == op_and ? mr_matched : mr_not_matched;
                break;
            case st_identifier_loop:
                status = mr_dont_emit;
                for (;;)
                {
                    match_result result;

                    save_ind = ind;
                    result = fast_match (di, text, &ind, sp->m_rule, &_P2, _BP, filtering_string, &ctx);

                    if (result == mr_error_raised)
                    {
                        status = result;
                        break;
                    }
                    else if (result == mr_matched)
                    {
                        if (!filtering_string)
                        {
                            if (sp->m_emits != NULL)
                            {
                                if (emit_push (sp->m_emits, _BP->_F + _P, text[ind - 1], save_ind, &ctx))
                                {
                                    free_regbyte_ctx_stack (ctx, *rbc);
                                    return mr_internal_error;
                                }
                            }

                            _P = _P2;
                            _P2 += sp->m_emits ? emit_size (sp->m_emits) : 0;
                            if (bytepool_reserve (_BP, _P2))
                            {
                                free_regbyte_ctx_stack (ctx, *rbc);
                                return mr_internal_error;
                            }
                        }
                    }
                    else if (result == mr_internal_error)
                    {
                        free_regbyte_ctx_stack (ctx, *rbc);
                        return mr_internal_error;
                    }
                    else
                        break;
                }
                break;
            }
        }
        else
        {
            status = mr_not_matched;
        }

        if (status == mr_error_raised)
        {
            free_regbyte_ctx_stack (ctx, *rbc);

            return mr_error_raised;
        }

        if (ru->m_oper == op_and && status != mr_matched && status != mr_dont_emit)
        {
            free_regbyte_ctx_stack (ctx, *rbc);

            if (sp->m_errtext)
            {
                set_last_error (sp->m_errtext->m_text, error_get_token (sp->m_errtext, di, text,
                    ind), ind);

                return mr_error_raised;
            }

            return mr_not_matched;
        }

        if (status == mr_matched)
        {
            if (sp->m_emits != NULL) {
                const byte ch = (ind <= 0) ? 0 : text[ind - 1];
                if (emit_push (sp->m_emits, _BP->_F + _P, ch, save_ind, &ctx))
                {
                    free_regbyte_ctx_stack (ctx, *rbc);
                    return mr_internal_error;
                }

           }
           _P = _P2;
        }

        /* if the rule operator is a logical or, we pick up the first matching specifier */
        if (ru->m_oper == op_or && (status == mr_matched || status == mr_dont_emit))
        {
            *index = ind;
            *rbc = ctx;
            if (!filtering_string)
                *_PP = _P;
            return mr_matched;
        }

        sp = sp->next;
    }

    /* everything went fine - all specifiers match up */
    if (ru->m_oper == op_and && (status == mr_matched || status == mr_dont_emit))
    {
        *index = ind;
        *rbc = ctx;
        if (!filtering_string)
            *_PP = _P;
        return mr_matched;
    }

    free_regbyte_ctx_stack (ctx, *rbc);
    return mr_not_matched;
}

static byte *
error_get_token (error *er, dict *di, const byte *text, int ind)
{
    byte *str = NULL;

    if (er->m_token)
    {
        barray *ba;
      int filter_index = 0;
        regbyte_ctx *ctx = NULL;

        barray_create (&ba);
        if (ba != NULL)
        {
            if (match (di, text + ind, &filter_index, er->m_token, &ba, 0, &ctx) == mr_matched &&
                filter_index)
            {
                str = (byte *) mem_alloc (filter_index + 1);
                if (str != NULL)
                {
                    str_copy_n (str, text + ind, filter_index);
                    str[filter_index] = '\0';
                }
            }
            barray_destroy (&ba);
        }
    }

    return str;
}

typedef struct grammar_load_state_
{
    dict *di;
    byte *syntax_symbol;
    byte *string_symbol;
    map_str *maps;
    map_byte *mapb;
    map_rule *mapr;
} grammar_load_state;

static void grammar_load_state_create (grammar_load_state **gr)
{
    *gr = (grammar_load_state *) mem_alloc (sizeof (grammar_load_state));
    if (*gr)
    {
        (**gr).di = NULL;
        (**gr).syntax_symbol = NULL;
        (**gr).string_symbol = NULL;
        (**gr).maps = NULL;
        (**gr).mapb = NULL;
        (**gr).mapr = NULL;
    }
}

static void grammar_load_state_destroy (grammar_load_state **gr)
{
    if (*gr)
    {
        dict_destroy (&(**gr).di);
        mem_free ((void **) &(**gr).syntax_symbol);
        mem_free ((void **) &(**gr).string_symbol);
        map_str_destroy (&(**gr).maps);
        map_byte_destroy (&(**gr).mapb);
        map_rule_destroy (&(**gr).mapr);
        mem_free ((void **) gr);
    }
}


static void error_msg(int line, const char *msg)
{
   fprintf(stderr, "Error in grammar_load_from_text() at line %d: %s\n", line, msg);
}


/*
    the API
*/
grammar grammar_load_from_text (const byte *text)
{
    grammar_load_state *g = NULL;
    grammar id = 0;

    clear_last_error ();

    grammar_load_state_create (&g);
    if (g == NULL) {
        error_msg(__LINE__, "");
        return 0;
    }

    dict_create (&g->di);
    if (g->di == NULL)
    {
        grammar_load_state_destroy (&g);
        error_msg(__LINE__, "");
        return 0;
    }

    eat_spaces (&text);

    /* skip ".syntax" keyword */
    text += 7;
    eat_spaces (&text);

    /* retrieve root symbol */
    if (get_identifier (&text, &g->syntax_symbol))
    {
        grammar_load_state_destroy (&g);
        error_msg(__LINE__, "");
        return 0;
    }
    eat_spaces (&text);

    /* skip semicolon */
    text++;
    eat_spaces (&text);

    while (*text)
    {
        byte *symbol = NULL;
        int is_dot = *text == '.';

        if (is_dot)
            text++;

        if (get_identifier (&text, &symbol))
        {
            grammar_load_state_destroy (&g);
            error_msg(__LINE__, "");
            return 0;
        }
        eat_spaces (&text);

        /* .emtcode */
        if (is_dot && str_equal (symbol, (byte *) "emtcode"))
        {
            map_byte *ma = NULL;

            mem_free ((void **) (void *) &symbol);

            if (get_emtcode (&text, &ma))
            {
                grammar_load_state_destroy (&g);
                error_msg(__LINE__, "");
                return 0;
            }

            map_byte_append (&g->mapb, ma);
        }
        /* .regbyte */
        else if (is_dot && str_equal (symbol, (byte *) "regbyte"))
        {
            map_byte *ma = NULL;

            mem_free ((void **) (void *) &symbol);

            if (get_regbyte (&text, &ma))
            {
                grammar_load_state_destroy (&g);
                error_msg(__LINE__, "");
                return 0;
            }

            map_byte_append (&g->di->m_regbytes, ma);
        }
        /* .errtext */
        else if (is_dot && str_equal (symbol, (byte *) "errtext"))
        {
            map_str *ma = NULL;

            mem_free ((void **) (void *) &symbol);

            if (get_errtext (&text, &ma))
            {
                grammar_load_state_destroy (&g);
                error_msg(__LINE__, "");
                return 0;
            }

            map_str_append (&g->maps, ma);
        }
        /* .string */
        else if (is_dot && str_equal (symbol, (byte *) "string"))
        {
            mem_free ((void **) (void *) &symbol);

            if (g->di->m_string != NULL)
            {
                grammar_load_state_destroy (&g);
                error_msg(__LINE__, "");
                return 0;
            }

            if (get_identifier (&text, &g->string_symbol))
            {
                grammar_load_state_destroy (&g);
                error_msg(__LINE__, "");
                return 0;
            }

            /* skip semicolon */
            eat_spaces (&text);
            text++;
            eat_spaces (&text);
        }
        else
        {
            rule *ru = NULL;
            map_rule *ma = NULL;

            if (get_rule (&text, &ru, g->maps, g->mapb))
            {
                grammar_load_state_destroy (&g);
                error_msg(__LINE__, "");
                return 0;
            }

            rule_append (&g->di->m_rulez, ru);

            /* if a rule consist of only one specifier, give it an ".and" operator */
            if (ru->m_oper == op_none)
                ru->m_oper = op_and;

            map_rule_create (&ma);
            if (ma == NULL)
            {
                grammar_load_state_destroy (&g);
                error_msg(__LINE__, "");
                return 0;
            }

            ma->key = symbol;
            ma->data = ru;
            map_rule_append (&g->mapr, ma);
        }
    }

    if (update_dependencies (g->di, g->mapr, &g->syntax_symbol, &g->string_symbol,
        g->di->m_regbytes))
    {
        grammar_load_state_destroy (&g);
        error_msg(__LINE__, "update_dependencies() failed");
        return 0;
    }

    dict_append (&g_dicts, g->di);
    id = g->di->m_id;
    g->di = NULL;

    grammar_load_state_destroy (&g);

    return id;
}

int grammar_set_reg8 (grammar id, const byte *name, byte value)
{
    dict *di = NULL;
    map_byte *reg = NULL;

    clear_last_error ();

    dict_find (&g_dicts, id, &di);
    if (di == NULL)
    {
        set_last_error (INVALID_GRAMMAR_ID, NULL, -1);
        return 0;
    }

    reg = map_byte_locate (&di->m_regbytes, name);
    if (reg == NULL)
    {
        set_last_error (INVALID_REGISTER_NAME, str_duplicate (name), -1);
        return 0;
    }

    reg->data = value;
    return 1;
}

/*
    internal checking function used by both grammar_check and grammar_fast_check functions
*/
static int _grammar_check (grammar id, const byte *text, byte **prod, unsigned int *size,
    unsigned int estimate_prod_size, int use_fast_path)
{
    dict *di = NULL;
   int index = 0;

    clear_last_error ();

    dict_find (&g_dicts, id, &di);
    if (di == NULL)
    {
        set_last_error (INVALID_GRAMMAR_ID, NULL, -1);
        return 0;
    }

    *prod = NULL;
    *size = 0;

    if (use_fast_path)
    {
        regbyte_ctx *rbc = NULL;
        bytepool *bp = NULL;
        int _P = 0;

        bytepool_create (&bp, estimate_prod_size);
        if (bp == NULL)
            return 0;

        if (fast_match (di, text, &index, di->m_syntax, &_P, bp, 0, &rbc) != mr_matched)
        {
            bytepool_destroy (&bp);
            free_regbyte_ctx_stack (rbc, NULL);
            return 0;
        }

        free_regbyte_ctx_stack (rbc, NULL);

        *prod = bp->_F;
        *size = _P;
        bp->_F = NULL;
        bytepool_destroy (&bp);
    }
    else
    {
        regbyte_ctx *rbc = NULL;
        barray *ba = NULL;

        barray_create (&ba);
        if (ba == NULL)
            return 0;

        if (match (di, text, &index, di->m_syntax, &ba, 0, &rbc) != mr_matched)
        {
            barray_destroy (&ba);
            free_regbyte_ctx_stack (rbc, NULL);
            return 0;
        }

        free_regbyte_ctx_stack (rbc, NULL);

        *prod = (byte *) mem_alloc (ba->len * sizeof (byte));
        if (*prod == NULL)
        {
            barray_destroy (&ba);
            return 0;
        }

        mem_copy (*prod, ba->data, ba->len * sizeof (byte));
        *size = ba->len;
        barray_destroy (&ba);
    }

    return 1;
}

int grammar_check (grammar id, const byte *text, byte **prod, unsigned int *size)
{
    return _grammar_check (id, text, prod, size, 0, 0);
}

int grammar_fast_check (grammar id, const byte *text, byte **prod, unsigned int *size,
    unsigned int estimate_prod_size)
{
    return _grammar_check (id, text, prod, size, estimate_prod_size, 1);
}

int grammar_destroy (grammar id)
{
    dict **di = &g_dicts;

    clear_last_error ();

    while (*di != NULL)
    {
        if ((**di).m_id == id)
        {
            dict *tmp = *di;
            *di = (**di).next;
            dict_destroy (&tmp);
            return 1;
        }

        di = &(**di).next;
    }

    set_last_error (INVALID_GRAMMAR_ID, NULL, -1);
    return 0;
}

static void append_character (const char x, byte *text, int *dots_made, int *len, int size)
{
    if (*dots_made == 0)
    {
        if (*len < size - 1)
        {
            text[(*len)++] = x;
            text[*len] = '\0';
        }
        else
        {
            int i;
            for (i = 0; i < 3; i++)
                if (--(*len) >= 0)
                    text[*len] = '.';
            *dots_made = 1;
        }
    }
}

void grammar_get_last_error (byte *text, unsigned int size, int *pos)
{
    int len = 0, dots_made = 0;
    const byte *p = error_message;

    *text = '\0';

    if (p)
    {
        while (*p)
        {
            if (*p == '$')
            {
                const byte *r = error_param;

                while (*r)
                {
                    append_character (*r++, text, &dots_made, &len, (int) size);
                }

                p++;
            }
            else
            {
                append_character (*p++, text, &dots_made, &len, size);
            }
        }
    }

    *pos = error_position;
}
