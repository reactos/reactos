/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    undecsym.cxx

Abstract:

    This is the engine for the C++ name undecorator.


    Syntax for Decorated Names


    cv-decorated-name ::=
            '?' '@' <decorated-name>

    decorated-name ::=
            '?' <symbol-name> [ <scope> ] '@' <type-encoding>


    symbol-name ::=
            <zname>
            <operator-name>


    zname ::=
            <letter> [ { <letter> | <number> } ] '@'
            <zname-replicator>

    letter ::=
            { 'A'..'Z' | 'a'..'z' | '_' | '$' }

    number ::=
            { '0'..'9' }

    zname-replicator ::=
            '0'..'9'                   Corresponding to the first through
                                       tenth 'zname' to have appeared

    The 'zname-replicator' is a compression facility for decorated names.
    Anywhere a 'zname' is expected, a single digit replicator may be used.
    The digits are '0' through '9' and correspond to the first through tenth unique
    'zname's which occur in the decorated name prior to the use of the replicator.

    operator-name ::=
            '?' <operator-code>


    scope ::=
            <zname> [ <scope> ]
            '?' <decorated-name> [ < scope > ]
            '?' <lexical-frame> [ <scope> ]
            '?' '$' <template-name> [ <scope> ]

    The 'scope' is ordered sequentially as a function of lexical scope, with successive enclosing
    scopes appearing to the right of the enclosed scopes.  Thus, the innermost scope is always placed
    first, followed by each successive enclosing scope.

    operator-code ::=
            '0'                Constructor
            '1'                Destructor
            '2'                'new'
            '3'                'delete'
            '4'                '='   Assignment
            '5'                '>>'  Right shift
            '6'                '<<'  Left shift
            '7'                '!'   Boolean NOT
            '8'                '=='  Equality
            '9'                '!='  Inequality
            'A'                '[]'  Indexing
            'B'                User Defined Conversion
            'C'                '->'  Member selection indirect
            'D'                '*'   Dereference or multiply
            'E'                '++'  Pre/Post-increment
            'F'                '--'  Pre/Post-decrement
            'G'                '-'   Two's complement negate, or subtract
            'H'                '+'   Unary plus, or add
            'I'                '&'   Address of, or bitwise AND
            'J'                '->*' Pointer to member selection
            'K'                '/'   Divide
            'L'                '%'   Modulo
            'M'                '<'   Less than
            'N'                '<='  Less than or equal to
            'O'                '>'   Greater than
            'P'                '>='  Greater than or equal to
            'Q'                ','   Sequence
            'R'                '()'  Function call
            'S'                '~'   Bitwise NOT
            'T'                '^'   Bitwise XOR
            'U'                '|'   Bitwise OR
            'V'                '&&'  Boolean AND
            'W'                '||'  Boolean OR
            'X'                '*='  Multiply and assign
            'Y'                '+='  Add and assign
            'Z'                '-='  Subtract and assign
            '_0'               '/='  Divide and assign
            '_1'               '%='  Modulo and assign
            '_2'               '>>=' Right shift and assign
            '_3'               '<<=' Left shift and assign
            '_4'               '&='  Bitwise AND and assign
            '_5'               '|='  Bitwise OR and assign
            '_6'               '^='  Bitwise XOR and assign
            '_7'               VTable
            '_8'               VBase
            '_9'               VCall Thunk
            '_A'               Metaclass
            '_B'               Guard variable for local statics
            '_C'               Ultimate Constructor for Vbases
            '_D'               Ultimate Destructor for Vbases
            '_E'               Vector Deleting Destructor
            '_F'               Default Constructor Closure
            '_G'               Scalar Deleting Destructor
            '_H'               Vector Constructor Iterator
            '_I'               Vector Destructor Iterator
            '_J'               Vector Allocating Constructor


    type-encoding ::=

    Member Functions

            'A'  <member-function-type>                 private near
            'B'  <member-function-type>                 private far
            'C'  <static-member-function-type>          private near
            'D'  <static-member-function-type>          private far
            'G'  <adjustor-thunk-type>                  private near
            'H'  <adjustor-thunk-type>                  private far
            'I'  <member-function-type>                 protected near
            'J'  <member-function-type>                 protected far
            'K'  <static-member-function-type>          protected near
            'L'  <static-member-function-type>          protected far
            'O'  <adjustor-thunk-type>                  protected near
            'P'  <adjustor-thunk-type>                  protected far
            'Q'  <member-function-type>                 public near
            'R'  <member-function-type>                 public far
            'S'  <static-member-function-type>          public near
            'T'  <static-member-function-type>          public far
            'W'  <adjustor-thunk-type>                  public near
            'X'  <adjustor-thunk-type>                  public far
            '$0' <virtual-adjustor-thunk-type>          private near
            '$1' <virtual-adjustor-thunk-type>          private far
            '$2' <virtual-adjustor-thunk-type>          protected near
            '$3' <virtual-adjustor-thunk-type>          protected far
            '$4' <virtual-adjustor-thunk-type>          public near
            '$5' <virtual-adjustor-thunk-type>          public far

    Virtual Member Functions

            'E'  <member-function-type>                 private near
            'F'  <member-function-type>                 private far
            'M'  <member-function-type>                 protected near
            'N'  <member-function-type>                 protected far
            'U'  <member-function-type>                 public near
            'V'  <member-function-type>                 public far

    Non-Member Functions

            'Y'  <external-function-type>               near
            'Z'  <external-function-type>               far
            '$A' <local-static-data-destructor-type>
            '$B' <vcall-thunk-type>

    Non-Functions

            '0'  <static-member-data-type>              private
            '1'  <static-member-data-type>              protected
            '2'  <static-member-data-type>              public
            '3'  <external-data-type>
            '4'  <local-static-data-type>
            '5'  <local-static-data-guard-type>
            '6'  <vtable-type>
            '7'  <vbase-type>
            '8'  <metaclass-type>


    Based variants of the above

    Member Functions

            '_A' <based-member-function-type>            private near
            '_B' <based-member-function-type>            private far
            '_C' <based-static-member-function-type>     private near
            '_D' <based-static-member-function-type>     private far
            '_G' <based-adjustor-thunk-type>             private near
            '_H' <based-adjustor-thunk-type>             private far
            '_I' <based-member-function-type>            protected near
            '_J' <based-member-function-type>            protected far
            '_K' <based-static-member-function-type>     protected near
            '_L' <based-static-member-function-type>     protected far
            '_O' <based-adjustor-thunk-type>             protected near
            '_P' <based-adjustor-thunk-type>             protected far
            '_Q' <based-member-function-type>            public near
            '_R' <based-member-function-type>            public far
            '_S' <based-static-member-function-type>     public near
            '_T' <based-static-member-function-type>     public far
            '_W' <based-adjustor-thunk-type>             public near
            '_X' <based-adjustor-thunk-type>             public far
            '_$0' <based-virtual-adjustor-thunk-type>    private near
            '_$1' <based-virtual-adjustor-thunk-type>    private far
            '_$2' <based-virtual-adjustor-thunk-type>    protected near
            '_$3' <based-virtual-adjustor-thunk-type>    protected far
            '_$4' <based-virtual-adjustor-thunk-type>    public near
            '_$5' <based-virtual-adjustor-thunk-type>    public far

    Virtual Member Functions

            '_E' <based-member-function-type>           private near
            '_F' <based-member-function-type>           private far
            '_M' <based-member-function-type>           protected near
            '_N' <based-member-function-type>           protected far
            '_U' <based-member-function-type>           public near
            '_V' <based-member-function-type>           public far

    Non-Member Functions

            '_Y'  <based-external-function-type>        near
            '_Z'  <based-external-function-type>        far
            '_$B' <based-vcall-thunk-type>


    external-function-type ::=
            <function-type>

    based-external-function-type ::=
            <based-type><external-function-type>

    external-data-type ::=
            <data-type><storage-convention>

    member-function-type ::=
            <this-type><static-member-function-type>

    based-member-function-type ::=
            <based-type><member-function-type>

    static-member-function-type ::=
            <function-type>

    based-static-member-function-type ::=
            <based-type><static-member-function-type>

    static-member-data-type ::=
            <external-data-type>

    local-static-data-type ::=
            <lexical-frame><external-data-type>

    local-static-data-guard-type ::=
            <guard-number>

    local-static-data-destructor-type ::=
            <calling-convention><local-static-data-type>

    vtable-type ::=
            <storage-convention> [ <vpath-name> ] '@'

    vbase-type ::=
            <storage-convention> [ <vpath-name> ] '@'

    metaclass-type ::=
            <storage-convention>

    adjustor-thunk-type ::=
            <displacement><member-function-type>

    based-adjustor-thunk-type ::=
            <based-type><adjustor-thunk-type>

    virtual-adjustor-thunk-type ::=
            <displacement><adjustor-thunk-type>

    based-virtual-adjustor-thunk-type ::=
            <based-type><virtual-adjustor-thunk-type>

    vcall-thunk-type ::=
            <call-index><vcall-model-type>

    based-vcall-thunk-type ::=
            <based-type><vcall-thunk-type>


    function-type ::=
            <calling-convention><return-type><argument-types>
                                                            <throw-types>


    segment-name ::=
            <zname>

    ecsu-name ::=
            <zname> [ <scope> ] '@'
            '?' <template-name> [ <scope> ] '@'


    return-type ::=
            '@'                        No type, for Ctor's and Dtor's
            <data-type>

    data-type ::=
            <primary-data-type>
            'X'                        'void'
            '?' <ecsu-data-indirect-type><ecsu-data-type>


    storage-convention ::=
            <data-indirect-type>

    this-type ::=
            <data-indirect-type>


    lexical-frame ::=
            <dimension>

    displacement ::=
            <dimension>

    call-index ::=
            <dimension>

    guard-number ::=
            <dimension>


    vcall-model-type ::=
            'A'                        near this, near call,  near vfptr
            'B'                        near this,  far call,  near vfptr
            'C'                         far this, near call,  near vfptr
            'D'                         far this,  far call,  near vfptr
            'E'                        near this, near call,   far vfptr
            'F'                        near this,  far call,   far vfptr
            'G'                         far this, near call,   far vfptr
            'H'                         far this,  far call,   far vfptr
            'I' <based-type>           near this, near call, based vfptr
            'JK' <based-type>          near this,  far call, based vfptr
            'KJ' <based-type>           far this, near call, based vfptr
            'L' <based-type>            far this,  far call, based vfptr


    throw-types ::=
            <argument-types>


    template-name ::=
            <zname><argument-list>

    calling-convention ::=
            'A'                        cdecl
            'B'                        cdecl saveregs
            'C'                        pascal/fortran/oldcall
            'D'                        pascal/fortran/oldcall saveregs
            'E'                        syscall
            'F'                        syscall saveregs
            'G'                        stdcall
            'H'                        stdcall saveregs
            'I'                        fastcall
            'J'                        fastcall saveregs
            'K'                        interrupt


    argument-types ::=
            'Z'                        (...)
            'X'                        (void)
            <argument-list> 'Z'        (arglist,...)
            <argument-list> '@'        (arglist)

    argument-list ::=
            <argument-replicator> [ <argument-list> ]
            <primary-data-type> [ <argument-list> ]

    argument-replicator ::=
            '0'..'9'           Corresponding to the first through tenth
                               argument of more than one character type
                               encoding.

    The 'argument-replicator' like the 'zname-replicator' is used to improve the compression of
    information present in decorated names.  In this case however, the 'replicator' allows a single
    digit to be used where an argument type is expected, and to refer to the first through tenth unique
    'argument-type' seen prior to this one.  This replicator refers to ANY argument type seen before,
    even if it was introduced in the recursively generated name for an argument which itself was a
    pointer or reference to a function.  An 'argument-replicator' is used only when the argument encoding
    exceeds one character, otherwise it would represent no actual compression.

    primary-data-type ::=
            'A' <reference-type>       Reference to
            'B' <reference-type>       Volatile reference to
            <basic-data-type>                  Other types


    reference-type ::=
            <data-indirect-type><reference-data-type>
            <function-indirect-type><function-type>


    pointer-type ::=
            <data-indirect-type><pointer-data-type>
            <function-indirect-type><function-type>


    vpath-name ::=
            <scope> '@' [ <vpath-name> ]


    ecsu-data-indirect-type ::=
            'A'                                near
            'B'                                near const
            'C'                                near volatile
            'D'                                near const volatile
            'E'                                far
            'F'                                far const
            'G'                                far volatile
            'H'                                far const volatile
            'I'                                huge
            'J'                                huge const
            'K'                                huge volatile
            'L'                                huge const volatile
            'M' <based-type>                   based
            'N' <based-type>                   based const
            'O' <based-type>                   based volatile
            'P' <based-type>                   based const volatile

    data-indirect-type ::=
            <ecsu-data-indirect-type>
            'Q' <scope> '@'            member near
            'R' <scope> '@'            member near const
            'S' <scope> '@'            member near volatile
            'T' <scope> '@'            member near const volatile
            'U' <scope> '@'            member far
            'V' <scope> '@'            member far const
            'W' <scope> '@'            member far volatile
            'X' <scope> '@'            member far const volatile
            'Y' <scope> '@'            member huge
            'Z' <scope> '@'            member huge const
            '0' <scope> '@'            member huge volatile
            '1' <scope> '@'            member huge const volatile
            '2' <scope> '@' <based-type>       member based
            '3' <scope> '@' <based-type>       member based const
            '4' <scope> '@' <based-type>       member based volatile
            '5' <scope> '@' <based-type>       member based const volatile


    function-indirect-type ::=
            '6'                                                        near
            '7'                                                        far
            '8'  <scope> '@' <this-type>                               member near
            '9'  <scope> '@' <this-type>                               member far
            '_A' <based-type>                                          based near
            '_B' <based-type>                                          based far
            '_C' <scope> '@' <this-type><based-type>                   based member
                                                                       near
            '_D' <scope> '@' <this-type><based-type>                   based member
                                                                       far


    based-type ::=
            '0'                        based on void
            '1'                        based on self
            '2'                        based on near pointer
            '3'                        based on far pointer
            '4'                        based on huge pointer
            '5' <based-type>           based on based pointer (reserved)
            '6'                        based on segment variable
            '7' <segment-name>         based on named segment
            '8'                        based on segment address of var
            '9'                        reserved


    basic-data-type ::=
            'C'                        signed char
            'D'                        char
            'E'                        unsigned char
            'F'                        (signed) short
            'G'                        unsigned short
            'H'                        (signed) int
            'I'                        unsigned int
            'J'                        (signed) long
            'K'                        unsigned long
            'L'                        __segment
            'M'                        float
            'N'                        double
            'O'                        long double
            'P' <pointer-type>         pointer to
            'Q' <pointer-type>         const pointer to
            'R' <pointer-type>         volatile pointer to
            'S' <pointer-type>         const volatile pointer to
            <ecsu-data-type>
            '_A'                       (signed) __int64
            '_B'                       unsigned __int64


    ecsu-data-type ::=
            'T' <ecsu-name>    union
            'U' <ecsu-name>    struct
            'V' <ecsu-name>    class
            'W' <enum-name>    enum


    pointer-data-type ::=
            'X'                        void
            <reference-data-type>

    reference-data-type ::=
            'Y' <array-type>           array of
            <basic-data-type>


    enum-name ::=
            <enum-type><ecsu-name>

    enum-type ::=
            '0'                        signed char enum
            '1'                        unsigned char enum
            '2'                        signed short enum
            '3'                        unsigned short enum
            '4'                        signed int enum
            '5'                        unsigned int enum
            '6'                        signed long enum
            '7'                        unsigned long enum


    array-type ::=
            <number-of-dimensions> { <dimension> } <basic-data-type>

    number-of-dimensions ::=
            <dimension>

    dimension ::=
            '0'..'9'                   Corresponding to 1 to 10 dimensions
            <adjusted-hex-digit> [ { <adjusted-hex-digit> } ] '@'

    adjusted-hex-digit ::=
            'A'..'P'                   Corresponding to values 0x0 to 0xF


Author:

    Wesley Witt (wesw) 09-June-1993   ( this code came from languages, i just ported it )

Revision History:

--*/

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern "C" {
#define _IMAGEHLP_SOURCE_
#include <imagehlp.h>
#include "private.h"
LONGLONG UndecTime;
}

HMODULE hMsvcrt;
PUNDNAME pfUnDname;
BOOL fLoadMsvcrtDLL;

void * __cdecl AllocIt(unsigned int cb)
{
    return (MemAlloc(cb));
}

void __cdecl FreeIt(void * p)
{
    MemFree(p);
}

DWORD
IMAGEAPI
WINAPI
UnDecorateSymbolName(
    LPCSTR name,
    LPSTR outputString,
    DWORD maxStringLength,
    DWORD flags
    )
{
    DWORD rc;

    //
    // can't undecorate into a zero length buffer
    //
    if (maxStringLength < 2) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (!fLoadMsvcrtDLL) {
        // The first time we run, see if we can find the system undname.  Use
        // GetModuleHandle to avoid any additionally overhead.

        hMsvcrt = GetModuleHandle("msvcrt.dll");

        if (hMsvcrt) {
            pfUnDname = (PUNDNAME) GetProcAddress(hMsvcrt, "__unDName");
        }
        fLoadMsvcrtDLL = TRUE;
    }

    rc = 0;     // Assume failure

    __try {
        if (pfUnDname) {
            if (flags & UNDNAME_NO_ARGUMENTS) {
                flags |= UNDNAME_NAME_ONLY;
                flags &= ~UNDNAME_NO_ARGUMENTS;
            }

            if (flags & UNDNAME_NO_SPECIAL_SYMS) {
                flags &= ~UNDNAME_NO_SPECIAL_SYMS;
            }
            if (pfUnDname(outputString, name, maxStringLength-1, AllocIt, FreeIt, (USHORT)flags)) {
                rc = strlen(outputString);
            }
        } else {
            rc = strlen(strncpy(outputString, "Unable to load msvcrt!__unDName", maxStringLength));
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) { }

    if (!rc) {
        SetLastError(ERROR_INVALID_PARAMETER);
    }
    return rc;
}
