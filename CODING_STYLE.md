# Coding Style

This article describes general coding style guidelines, which should be used for new ReactOS code. These guidelines apply exclusively to C and C++ source files. The Members of ReactOS agreed on this document in the October 2013 meeting.

As much existing ReactOS code as possible should be converted to this style unless there are reasons against doing this (like if the code is going to be rewritten from scratch in the near future). See [Notes on reformatting existing code](#notes-on-reformatting-existing-code) for more details.

Code synchronized with other sources (like Wine) must not be rewritten. [3rd Party Files.txt](https://github.com/reactos/reactos/blob/master/media/doc/3rd%20Party%20Files.txt) and [WINESYNC.txt](https://github.com/reactos/reactos/blob/master/media/doc/WINESYNC.txt) files can be used for tracking synchronized files.

## File Structure

1. Every ReactOS source code file should include a file header like this:

```
/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Does cool things like Memory Management
 * COPYRIGHT:   Copyright 2017 Arno Nymous <abc@mailaddress.com>
 *              Copyright 2017 Mike Blablabla <mike@blabla.com>
 */
```

Please use SPDX license identifiers available at https://spdx.org/licenses.
This makes our source file parseable by licensing tools!

You should add yourself to the `COPYRIGHT` section of a file if you did a major contribution to it and could take responsibility for the whole file or a part of it. Not more than 3 people shall be in that list for each file.

`FILE` line of the old header should be removed.

2. [Doxygen](https://doxygen.reactos.org/) documentation generator is used for ReactOS codebase, so use a proper header for functions, see [API Documentation](https://reactos.org/wiki/Documentation_Guidelines#API_Documentation) for details.

## Indentation and line width

1. Line width must be at most **100 characters**.
2. Do not add a space or tab at the end of any line.
3. Indent with **4 spaces**, don't use tabs!
4. Indent both a case label and the case statement of a switch statement.

**Right:**
```c
switch (Condition)
{
    case 1:
        DoSomething();
        break;

    case 2:
    {
        DoMany();
        ManyMore();
        OtherThings();
        break;
    }
}
```

**Wrong:**
```c
switch(Condition)
{
case 1:
     DoSomething();
     break;
case 2:
    DoMany();
    ManyMore();
    OtherThings();
    break;
}
```

5. When a function call does not fit onto a line, align arguments like this:
```c
FunctionCall(arg1,
             arg2,
             arg3);
```

6. Function headers should have this format (preserving the order as in the example):
```c
static    // scope identifier
CODE_SEG("PAGE")    // section placement
// other attributes
BOOLEAN    // return type
FASTCALL    // calling convention
IsOdd(
    _In_ UINT32 Number);
```

## Spacing

1. Do not use spaces around unary operators.  
**Right:** `i++;`  
**Wrong:** `i ++;`

2. Place spaces around binary and ternary operators.  
**Right:** `a = b + c;`  
**Wrong:** `a=b+c;`

3. Do not place spaces before comma and semicolon.

**Right:**
```c
for (int i = 0; i < 5; i++)
    DoSomething();

func1(a, b);
```

**Wrong:**
```c
for (int i = 0; i < 5 ; i++)
    DoSomething();

func1(a , b) ;
```

4. Place spaces between control statements and their parentheses.

**Right:**
```c
if (Condition)
    DoSomething();
```

**Wrong:**
```c
if(Condition)
    DoSomething();
```

5. Do not place spaces between a function and its parentheses, or between a parenthesis and its content.

**Right:**
```c
func(a, b);
```

**Wrong:**
```c
func (a, b);
func( a, b );
```

## Line breaking

1. Each statement should get its own line.

**Right:**
```c
x++;
y++;

if (Condition)
    DoSomething();
```

**Wrong:**
```c
x++; y++;

if (Condition) DoSomething();
```

## Braces

1. Always put braces (`{` and `}`) on their own lines.
2. One-line control clauses may use braces, but this is not a requirement. An exception are one-line control clauses including additional comments.

**Right:**
```c
if (Condition)
    DoSomething();

if (Condition)
{
    DoSomething();
}

if (Condition)
{
    // This is a comment
    DoSomething();
}

if (A_Very || (Very && Long || Condition) &&
    On_Many && Lines)
{
    DoSomething();
}

if (Condition)
    DoSomething();
else
    DoSomethingElse();

if (Condition)
{
    DoSomething();
}
else
{
    DoSomethingElse();
    YetAnother();
}
```

**Wrong:**
```c
if (Condition) {
    DoSomething();
}

if (Condition)
    // This is a comment
    DoSomething();

if (A_Very || (Very && Long || Condition) &&
    On_Many && Lines)
    DoSomething();

if (Condition)
    DoSomething();
else {
    DoSomethingElse();
    YetAnother();
}
```

## Control structures

1. Don't use inverse logic in control clauses.  
**Right:** `if (i == 1)`  
**Wrong:** `if (1 == i)`

2. Avoid too many levels of cascaded control structures. Prefer a "linear style" over a "tree style". Use `goto` when it helps to make the code cleaner (e.g. for cleanup paths).

**Right:**
```c
if (!func1())
    return;

i = func2();
if (i == 0)
    return;

j = func3();
if (j == 1)
    return;
...
```

**Wrong:**
```c
if (func1())
{
    i = func2();
    if (func2())
    {
        j = func3();
        if (func3())
        {
            ...
        }
    }
}
```

## Naming

1. Capitalize names of variables and functions. Hungarian Notation may be used when developing for Win32, but it is not required. If you don't use it, the first letter of a name must be a capital too (no lowerCamelCase). Do not use underscores as separators either.

**Right:**
```c
PLIST_ENTRY FirstEntry;
VOID NTAPI IopDeleteIoCompletion(PVOID ObjectBody);
PWSTR pwszTest;
```

**Wrong:**
```c
PLIST_ENTRY first_entry;
VOID NTAPI iop_delete_io_completion(PVOID objectBody);
PWSTR pwsztest;
```

2. Avoid abbreviating function and variable names, use descriptive verbs where possible.

3. Precede boolean values with meaningful verbs like "is" and "did" if possible and if it fits the usage.

**Right:**
```c
BOOLEAN IsValid;
BOOLEAN DidSendData;
```

**Wrong:**
```c
BOOLEAN Valid;
BOOLEAN SentData;
```

## Commenting

1. Avoid line-wasting comments, which could fit into a single line.

**Right:**
```c
// This is a one-line comment

/* This is a C-style comment */

// This is a comment over multiple lines.
// We don't define any strict rules for it.
```

**Wrong:**
```c
//
// This comment wastes two lines
//
```

## Null, false and 0

1. The null pointer should be written as `NULL`. In the rare case that your environment recommends a different null pointer (e.g. C++11 `nullptr`), you may use this one of course. Just don't use the value `0`.

2. Win32/NT Boolean values should be written as `TRUE` and `FALSE`. In the rare case that you use C/C++ `bool` variables, you should write them as `true` and `false`.

3. When you need to terminate ANSI or OEM string, or check for its terminator, use `ANSI_NULL`. If the string is Unicode or Wide string, use `UNICODE_NULL`.

## Notes on reformatting existing code

- Never totally reformat a file and put a code change into it. Do this in separate commits.
- If a commit only consists of formatting changes, say this clearly in the commit message by preceding it with *[FORMATTING]*.

## Other points

- Do not use `LARGE_INTEGER`/`ULARGE_INTEGER` unless needed for using APIs. Use `INT64`/`UINT64` instead
- Use `#pragma once` instead of guard defines in headers
- Don't specify a calling convention for a function unless required (usually for APIs or exported symbols)

## Using an automatic code style tool

TO BE ADDED BY User:Zefklop

## Points deliberately left out

Additional ideas were suggested during the discussion of this document, but a consensus couldn't be reached on them. Therefore we refrain from enforcing any rules on these points:

- TO BE ADDED BY User:Hbelusca

## See also

- [Kernel Coding Style](https://reactos.org/wiki/Kernel_Coding_Style)
- [GNU Indent](https://reactos.org/wiki/GNU_Indent)
