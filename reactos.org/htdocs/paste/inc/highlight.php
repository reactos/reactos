<?php

// download-link: http://rafb.net/paste/highlight.phps


/* This software is licensed through a BSD-style License.
 * http://www.opensource.org/licenses/bsd-license.php

Copyright (c) 2003, 2004, Jacob D. Cohen
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
Neither the name of Jacob D. Cohen nor the names of his contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

function keyword_replace($keywords, $text, $ncs = false)
{
    $cm = ($ncs)? "i" : "";
    foreach ($keywords as $keyword)
    {
        $search[]  = "/(\\b$keyword\\b)/" . $cm;
        $replace[] = '<span class="keyword">\\0</span>';
    }

    $search[]  = "/(\\bclass\s)/";
    $replace[] = '<span class="keyword">\\0</span>';

    return preg_replace($search, $replace, $text);
}


function preproc_replace($preproc, $text)
{
    foreach ($preproc as $proc)
    {
        $search[] = "/(\\s*#\s*$proc\\b)/";
        $replace[] = '<span class="keyword">\\0</span>';
    }

    return preg_replace($search, $replace, $text);
}


function sch_syntax_helper($text)
{
    return $text;
}


function syntax_highlight_helper($text, $language)
{
    $preproc = array();
    $preproc["C++"] = array(
    "if",    "ifdef",   "ifndef", "elif",  "else",
    "endif", "include", "define", "undef", "line",
    "error", "pragma");
    $preproc["C89"] = & $preproc["C++"];
    $preproc["C"] = & $preproc["C89"];

    $keywords = array(
    "C++" => array(
    "asm",          "auto",      "bool",     "break",            "case",
    "catch",        "char",      /*class*/   "const",            "const_cast",
    "continue",     "default",   "delete",   "do",               "double",
    "dynamic_cast", "else",      "enum",     "explicit",         "export",
    "extern",       "false",     "float",    "for",              "friend",
    "goto",         "if",        "inline",   "int",              "long",
    "mutable",      "namespace", "new",      "operator",         "private",
    "protected",    "public",    "register", "reinterpret_cast", "return",
    "short",        "signed",    "sizeof",   "static",           "static_cast",
    "struct",       "switch",    "template", "this",             "throw", 
    "true",         "try",       "typedef",  "typeid",           "typename",
    "union",        "unsigned",  "using",    "virtual",          "void",
    "volatile",     "wchar_t",   "while"),

    "C89" => array(
    "auto",     "break",    "case",     "char",     "const",
    "continue", "default",  "do",       "double",   "else",
    "enum",     "extern",   "float",    "for",      "goto",
    "if",       "int",      "long",     "register", "return",
    "short",    "signed",   "sizeof",   "static",   "struct",
    "switch",   "typedef",  "union",    "unsigned", "void",
    "volatile", "while"),

    "C" => array(
    "auto",     "break",    "case",     "char",     "const",
    "continue", "default",  "do",       "double",   "else",
    "enum",     "extern",   "float",    "for",      "goto",
    "if",       "int",      "long",     "register", "return",
    "short",    "signed",   "sizeof",   "static",   "struct",
    "switch",   "typedef",  "union",    "unsigned", "void",
    "volatile", "while",    "__restrict","_Bool"),

    "PHP" => array(
    "and",          "or",           "xor",      "__FILE__",     "__LINE__",
    "array",        "as",           "break",    "case",         "cfunction",
    /*class*/       "const",        "continue", "declare",      "default",
    "die",          "do",           "echo",     "else",         "elseif",
    "empty",        "enddeclare",   "endfor",   "endforeach",   "endif",
    "endswitch",    "endwhile",     "eval",     "exit",         "extends",
    "for",          "foreach",      "function", "global",       "if",
    "include",      "include_once", "isset",    "list",         "new",
    "old_function", "print",        "require",  "require_once", "return",
    "static",       "switch",       "unset",    "use",          "var",
    "while",        "__FUNCTION__", "__CLASS__"),

    "Perl" => array(
    "-A",           "-B",           "-C",       "-M",           "-O",
    "-R",           "-S",           "-T",       "-W",           "-X",
    "-b",           "-c",           "-d",       "-e",           "-f",
    "-g",           "-k",           "-l",       "-o",           "-p",
    "-r",           "-s",           "-t",       "-u",           "-w",
    "-x",           "-z",           "ARGV",     "DATA",         "ENV",
    "SIG",          "STDERR",       "STDIN",    "STDOUT",       "atan2",
    "bind",         "binmode",      "bless",    "caller",       "chdir",
    "chmod",        "chomp",        "chop",     "chown",        "chr",
    "chroot",       "close",        "closedir", "cmp",          "connect",
    "continue",     "cos",          "crypt",    "dbmclose",     "dbmopen",
    "defined",      "delete",       "die",      "do",           "dump",
    "each",         "else",         "elsif",    "endgrent",     "endhostent",
    "endnetent",    "endprotent",   "endpwent", "endservent",   "eof",
    "eq",           "eval",         "exec",     "exists",       "exit",
    "exp",          "fcntl",        "fileno",   "flock",        "for",
    "foreach",      "fork",         "format",   "formline",     "ge",
    "getc",         "getgrent",     "getgrid",  "getgrnam",     "gethostbyaddr",
    "gethostbyname","gethostent",   "getlogin", "getnetbyaddr", "getnetbyname",
    "getnetent",    "getpeername",  "getpgrp",  "getppid",      "getpriority",
    "getprotobyname","getprotobynumber","getprotoent","getpwent","getpwnam",
    "getpwuid",     "getservbyname","getservbyport","getservent","getsockname",
    "getsockopt",   "glob",         "gmtime",   "goto",         "grep",
    /*gt*/          "hex",          "if",       "import",       "index",
    "int",          "ioctl",        "join",     "keys",         "kill",
    "last",         "lc",           "lcfirst",  "le",           "length",
    "link",         "listen",       "local",    "localtime",    "log",
    "lstat",        /*lt*/          "m",        "map",          "mkdir",
    "msgctl",       "msgget",       "msgrcv",   "msgsnd",       "my",
    "ne",           "next",         "no",       "oct",          "open",
    "opendir",      "ord",          "pack",     "package",      "pipe",
    "pop",          "pos",          "print",    "printf",       "push",
    "q",            "qq",           "quotemeta","qw",           "qx",
    "rand",         "read",         "readdir",  "readlink",     "recv",
    "redo",         "ref",          "refname",  "require",      "reset",
    "return",       "reverse",      "rewinddir","rindex",       "rmdir",
    "s",            "scalar",       "seek",     "seekdir",      "select",
    "semctl",       "semget",       "semop",    "send",         "setgrent",
    "sethostent",   "setnetent",    "setpgrp",  "setpriority",  "setprotoent",
    "setpwent",     "setservent",   "setsockopt","shift",       "shmctl",
    "shmget",       "shmread",      "shmwrite", "shutdown",     "sin",
    "sleep",        "socket",       "socketpair","sort",        "splice",
    "split",        "sprintf",      "sqrt",     "srand",        "stat",
    "study",        "sub",          "substr",   "symlink",      "syscall",
    "sysopen",      "sysread",      "system",   "syswrite",     "tell",
    "telldir",      "tie",          "tied",     "time",         "times",
    "tr",           "truncate",     "uc",       "ucfirst",      "umask",
    "undef",        "unless",       "unlink",   "unpack",       "unshift",
    "untie",        "until",        "use",      "utime",        "values",
    "vec",          "wait",         "waitpid",  "wantarray",    "warn", 
    "while",        "write",        "y",        "or",           "and",
    "not"),

    "Java" => array(
    "abstract",     "boolean",      "break",    "byte",         "case",
    "catch",        "char",         /*class*/   "const",        "continue",
    "default",      "do",           "double",   "else",         "extends",
    "final",        "finally",      "float",    "for",          "goto",
    "if",           "implements",   "import",   "instanceof",   "int",
    "interface",    "long",         "native",   "new",          "package",
    "private",      "protected",    "public",   "return",       "short",
    "static",       "strictfp",     "super",    "switch",       "synchronized",
    "this",         "throw",        "throws",   "transient",    "try",
    "void",         "volatile",     "while"),

    "VB" => array(
    "AddressOf",    "Alias",        "And",      "Any",          "As",
    "Binary",       "Boolean",      "ByRef",    "Byte",         "ByVal",
    "Call",         "Case",         "CBool",    "CByte",        "CCur",
    "CDate",        "CDbl",         "CInt",     "CLng",         "Close",
    "Const",        "CSng",         "CStr",     "Currency",     "CVar",
    "CVErr",        "Date",         "Debug",    "Declare",      "DefBool",
    "DefByte",      "DefCur",       "DefDate",  "DefDbl",       "DefInt",
    "DefLng",       "DefObj",       "DefSng",   "DefStr",       "DefVar",
    "Dim",          "Do",           "Double",   "Each",         "Else",
    "End",          "Enum",         "Eqv",      "Erase",        "Error",
    "Event",        "Exit",         "For",      "Friend",       "Function",
    "Get",          "Get",          "Global",   "GoSub",        "GoTo",
    "If",           "Imp",          "Implements","In",          "Input",
    "Integer",      "Is",           "LBound",   "Len",          "Let",
    "Lib",          "Like",         "Line",     "Lock",         "Long",
    "Loop",         "LSet",         "Mod",      "Name",         "Next",
    "Not",          "Nothing",      "Null",     "Object",       "On",
    "Open",         "Option Base 1","Option Compare Binary",
    "Option Compare Database", "Option Compare Text", "Option Explicit",
    "Option Private Module", "Optional",        "Or",           "Output",
    "ParamArray",   "Preserve",     "Print",    "Private",      "Property",
    "Public",       "Put",          "RaiseEvent","Random",      "Read",
    "ReDim",        "Resume",       "Return",   "RSet",         "Seek",
    "Select",       "Set",          "Single",   "Spc",          "Static",
    "Step",         "Stop",         "String",   "Sub",          "Tab",
    "Then",         "To",           "Type",     "UBound",       "Unlock",
    "Variant",      "Wend",         "While",    "With",         "WithEvents",
    "Write",        "Xor"),

    "C#" => array(
    "abstract",     "as",           "base",     "bool",         "break",
    "byte",         "case",         "catch",    "char",         "checked",
    /*class*/       "const",        "continue", "decimal",      "default",
    "delegate",     "do",           "double",   "else",         "enum",
    "event",        "explicit",     "extern",   "false",        "finally",
    "fixed",        "float",        "for",      "foreach",      "goto",
    "if",           "implicit",     "in",       "int",          "interface",
    "internal",     "is",           "lock",     "long",         "namespace",
    "new",          "null",         "object",   "operator",     "out",
    "override",     "params",       "private",  "protected",    "public",
    "readonly",     "ref",          "return",   "sbyte",        "sealed",
    "short",        "sizeof",       "stackalloc","static",      "string",
    "struct",       "switch",       "this",     "throw",        "true",
    "try",          "typeof",       "uint",     "ulong",        "unchecked",
    "unsafe",       "ushort",       "using",    "virtual",      "volatile",
    "void",         "while"),
    
    "Ruby" => array(
    "alias",        "and",          "begin",    "break",        "case",
    /*class*/       "def",          "defined",  "do",           "else",
    "elsif",        "end",          "ensure",   "false",        "for",
    "if",           "in",           "module",   "next",         "module",
    "next",         "nil",          "not",      "or",           "redo",
    "rescue",       "retry",        "return",   "self",         "super",
    "then",         "true",         "undef",    "unless",       "until",
    "when",         "while",        "yield"),

    "Python" => array(
    "and",          "assert",       "break",    /*"class",*/    "continue",
    "def",          "del",          "elif",     "else",         "except",
    "exec",         "finally",      "for",      "from",         "global",
    "if",           "import",       "in",       "is",           "lambda",
    "not",          "or",           "pass",     "print",        "raise",
    "return",       "try",          "while",    "yield"),

    "Pascal" => array(
    "Absolute",     "Abstract",     "All",      "And",          "And_then",
    "Array",        "Asm",          "Begin",    "Bindable",     "Case",
    /*"Class",*/    "Const",        "Constructor","Destructor", "Div",
    "Do",           "Downto",       "Else",     "End",          "Export",
    "File",         "For",          "Function", "Goto",         "If",
    "Import",       "Implementation","Inherited","In",          "Inline",
    "Interface",    "Is",           "Label",    "Mod",          "Module",
    "Nil",          "Not",          "Object",   "Of",           "Only",
    "Operator",     "Or",           "Or_else",  "Otherwise",    "Packed",
    "Pow",          "Procedure",    "Program",  "Property",     "Protected",
    "Qualified",    "Record",       "Repeat",   "Restricted",   "Set",
    "Shl",          "Shr",          "Then",     "To",           "Type",
    "Unit",         "Until",        "Uses",     "Value",        "Var",
    "View",         "Virtual",      "While",    "With",         "Xor"),

    "mIRC" => array(
        ),

    "PL/I" => array(
    "A",            "ABS",            "ACOS",        "%ACTIVATE",    "ACTUALCOUNT", 
    "ADD",            "ADDR",            "ADDREL",    "ALIGNED",        "ALLOCATE", 
    "ALLOC",        "ALLOCATION",    "ALLOCN",    "ANY",            "ANYCONDITION", 
    "APPEND",        "AREA",            "ASIN",        "ATAN",            "ATAND", 
    "ATANH",        "AUTOMATIC",    "AUTO",        "B",            "B1", 
    "B2",            "B3",            "B4",        "BACKUP_DATE",    "BASED", 
    "BATCH",        "BEGIN",        "BINARY",    "BIN",            "BIT",
    "BLOCK_BOUNDARY_FORMAT",        "BLOCK_IO",    "BLOCK_SIZE",    "BOOL",
    "BUCKET_SIZE",    "BUILTIN",        "BY",        "BYTE",            "BYTESIZE",
    "CALL",            "CANCEL_CONTROL_O",            "CARRIAGE_RETURN_FORMAT",
    "CEIL",            "CHAR", "CHARACTER",    "CLOSE",    "COLLATE",        "COLUMN",
    "CONDITION",    "CONTIGUOUS",    "CONTIGUOUS_BEST_TRY",        "CONTROLLED",
    "CONVERSION",    "COPY",            "COS",        "COSD",            "COSH",
    "CREATION_DATE",                "CURRENT_POSITION",            "DATE",
    "DATETIME",        "%DEACTIVATE",    "DECIMAL",    "DEC",            "%DECLARE",
    "%DCL",            "DECLARE",        "DCL",        "DECODE",        "DEFAULT_FILE_NAME",
    "DEFERRED_WRITE",                "DEFINED",    "DEF",            "DELETE",
    "DESCRIPTOR",    "%DICTIONARY",    "DIMENSION","DIM",           "DIRECT",
    "DISPLAY",        "DIVIDE",        "%DO",        "DO",            "E",
    "EDIT",            "%ELSE",        "ELSE",        "EMPTY",        "ENCODE",
    "%END",            "END",            "ENDFILE",    "ENDPAGE",        "ENTRY",
    "ENVIRONMENT",    "ENV",            "%ERROR",    "ERROR",        "EVERY",
    "EXP",            "EXPIRATION_DATE",            "EXTEND",        "EXTENSION_SIZE",
    "EXTERNAL",        "EXT",            "F",        "FAST_DELETE",    "%FATAL",
    "FILE",            "FILE_ID",        "FILE_ID_TO",                "FILE_SIZE",
    "FINISH",        "FIXED",        "FIXEDOVERFLOW",            "FOFL",
    "FIXED_CONTROL_FROM",            "FIXED_CONTROL_SIZE",        "FIXED_CONTROL_SIZE_TO",
    "FIXED_CONTROL_TO",                "FIXED_LENGTH_RECORDS",        "FLOAT",
    "FLOOR",        "FLUSH",        "FORMAT",    "FREE",            "FROM",
    "GET",            "GLOBALDEF",    "GLOBALREF",                "%GOTO",
    "GOTO",            "GO", "TO",        "GROUP_PROTETION",            "HBOUND",
    "HIGH",            "INDENT",        "%IF",        "IF",            "IGNORE_LINE_MARKS",
    "IN",            "%INCLUDE",        "INDEX",    "INDEXED",        "INDEX_NUMBER",
    "%INFORM",        "INFORM",        "INITIAL",    "INIT",            "INITIAL_FILL",
    "INPUT",        "INT",            "INTERNAL",    "INTO",            "KEY",
    "KEYED",        "KEYFROM",        "KEYTO",    "LABEL",        "LBOUND",
    "LEAVE",        "LENGTH",        "LIKE",        "LINE",            "LINENO",
    "LINESIZE",        "%LIST",        "LIST",        "LOCK_ON_READ",    "LOCK_ON_WRITE",
    "LOG",            "LOG10",        "LOG2",        "LOW",            "LTRIM",
    "MAIN",            "MANUAL_UNLOCKING",            "MATCH_GREATER",
    "MATCH_GREATER_EQUAL",            "MATCH_NEXT",                "MATCH_NEXT_EQUAL",
    "MAX",            "MAXIMUM_RECORD_NUMBER",    "MAXIMUM_RECORD_SIZE",
    "MAXLENGTH",    "MEMBER",        "MIN",        "MOD",            "MULTIBLOCK_COUNT",
    "MULTIBUFFER_COUNT",            "MULTIPLY",    "NEXT_VOLUME",    "%NOLIST",
    "NOLOCK",        "NONEXISTENT_RECORD",        "NONRECURSIVE",    "NONVARYING",
    "NONVAR",        "NORESCAN",        "NO_ECHO",    "NO_FILTER",    "NO_SHARE",
    "NULL",            "OFFSET",        "ON",        "ONARGSLIST",    "ONCHAR",
    "ONCODE",        "ONFILE",        "ONKEY",    "ONSOURCE",        "OPEN",
    "OPTIONAL",        "OPTIONS",        "OTHERWISE","OTHER",        "OUTPUT",
    "OVERFLOW",        "OFL",            "OWNER_GROUP",                "OWNER_ID",
    "OWNER_MEMBER",    "OWNER_PROTECTION",            "P",            "%PAGE",
    "PAGE",            "PAGENO",        "PAGESIZE",    "PARAMETER",    "PARM",
    "PICTURE",        "PIC",            "POINTER",    "PTR",            "POSINT",
    "POSITION",        "POS",            "PRECISION","PREC",            "PRESENT",
    "PRINT",        "PRINTER_FORMAT",            "%PROCEDURE",    "%PROC",
    "PROCEDURE",    "PROC",            "PROD",        "PROMPT",        "PURGE_TYPE_AHEAD",
    "PUT",            "R",            "RANK",        "READ",            "READONLY",
    "READ_AHEAD",    "READ_CHECK",    "READ_REGARDLESS",            "RECORD",
    "RECORD_ID",    "RECORD_ID_ACCESS",            "RECORD_ID_TO",    "RECURSIVE",
    "REFER",        "REFERENCE",    "RELEASE",    "REPEAT",        "%REPLACE",
    "RESCAN",        "RESIGNAL",        "RETRIEVAL_POINTERS",        "%RETURN",
    "RETURN",        "RETURNS",        "REVERSE",    "REVERT",        "REVISION_DATE",
    "REWIND",        "REWIND_ON_CLOSE",            "REWIND_ON_OPEN",
    "REWRITE",        "ROUND",        "RTRIM",    "%SBTTL",        "SCALARVARYING",
    "SEARCH",        "SELECT",        "SEQUENTIAL",                "SEQL",
    "SET",            "SHARED_READ",    "SHARED_WRITE",                "SIGN",
    "SIGNAL",        "SIN",            "SIND",        "SINH",            "SIZE",
    "SKIP",            "SNAP",            "SOME",        "SPACEBLOCK",    "SPOOL",
    "SQRT",            "STATEMENT",    "STATIC",    "STOP",            "STORAGE",
    "STREAM",        "STRING",        "STRINGRANGE",                "STRG",
    "STRUCTURE",    "SUBSCRIPTRANGE",            "SUBRG",        "SUBSTR",
    "SUBTRACT",        "SUM",            "SUPERCEDE","SYSIN",        "SYSPRINT",
    "SYSTEM",        "SYSTEM_PROTECTION",        "TAB",            "TAN",
    "TAND",            "TANH",            "TEMPORARY","%THEN",        "THEN",
    "TIME",            "TIMEOUT_PERIOD",            "%TITLE",        "TITLE",
    "TO",            "TRANSLATE",    "TRIM",        "TRUNC",        "TRUNCATE",
    "UNALIGNED",    "UNAL",            "UNDEFINED","UNDF",            "UNDERFLOW",
    "UFL",            "UNION",        "UNSPEC",    "UNTIL",        "UPDATE",
    "USER_OPEN",    "VALID",        "VALUE",    "VAL",            "VARIABLE",
    "VARIANT",        "VARYING",        "VAR",        "VAXCONDITION",    "VERIFY",
    "WAIT_FOR_RECORD",                "%WARN",    "WARN",            "WHEN",    
    "WHILE",        "WORLD_PROTECTION",            "WRITE",        "WRITE_BEHIND",
    "WRITE_CHECK",    "X",            "ZERODIVIDE"),

    "SQL" => array(
    "abort", "abs", "absolute", "access",
    "action", "ada", "add", "admin",
    "after", "aggregate", "alias", "all",
    "allocate", "alter", "analyse", "analyze",
    "and", "any", "are", "array",
    "as", "asc", "asensitive", "assertion",
    "assignment", "asymmetric", "at", "atomic",
    "authorization", "avg", "backward", "before",
    "begin", "between", "bigint", "binary",
    "bit", "bitvar", "bit_length", "blob",
    "boolean", "both", "breadth", "by",
    "c", "cache", "call", "called",
    "cardinality", "cascade", "cascaded", "case",
    "cast", "catalog", "catalog_name", "chain",
    "char", "character", "characteristics", "character_length",
    "character_set_catalog", "character_set_name", "character_set_schema", "char_length",
    "check", "checked", "checkpoint", /* "class", */
    "class_origin", "clob", "close", "cluster",
    "coalesce", "cobol", "collate", "collation",
    "collation_catalog", "collation_name", "collation_schema", "column",
    "column_name", "command_function", "command_function_code", "comment",
    "commit", "committed", "completion", "condition_number",
    "connect", "connection", "connection_name", "constraint",
    "constraints", "constraint_catalog", "constraint_name", "constraint_schema",
    "constructor", "contains", "continue", "conversion",
    "convert", "copy", "corresponding", "count",
    "create", "createdb", "createuser", "cross",
    "cube", "current", "current_date", "current_path",
    "current_role", "current_time", "current_timestamp", "current_user",
    "cursor", "cursor_name", "cycle", "data",
    "database", "date", "datetime_interval_code", "datetime_interval_precision",
    "day", "deallocate", "dec", "decimal",
    "declare", "default", "defaults", "deferrable",
    "deferred", "defined", "definer", "delete",
    "delimiter", "delimiters", "depth", "deref",
    "desc", "describe", "descriptor", "destroy",
    "destructor", "deterministic", "diagnostics", "dictionary",
    "disconnect", "dispatch", "distinct", "do",
    "domain", "double", "drop", "dynamic",
    "dynamic_function", "dynamic_function_code", "each", "else",
    "encoding", "encrypted", "end", "end-exec",
    "equals", "escape", "every", "except",
    "exception", "excluding", "exclusive", "exec",
    "execute", "existing", "exists", "explain",
    "external", "extract", "false", "fetch",
    "final", "first", "float", "for",
    "force", "foreign", "fortran", "forward",
    "found", "free", "freeze", "from",
    "full", "function", "g", "general",
    "generated", "get", "global", "go",
    "goto", "grant", "granted", "group",
    "grouping", "handler", "having", "hierarchy",
    "hold", "host", "hour", "identity",
    "ignore", "ilike", "immediate", "immutable",
    "implementation", "implicit", "in", "including",
    "increment", "index", "indicator", "infix",
    "inherits", "initialize", "initially", "inner",
    "inout", "input", "insensitive", "insert",
    "instance", "instantiable", "instead", "int",
    "integer", "intersect", "interval", "into",
    "invoker", "is", "isnull", "isolation",
    "iterate", "join", "k", "key",
    "key_member", "key_type", "lancompiler", "language",
    "large", "last", "lateral", "leading",
    "left", "length", "less", "level",
    "like", "limit", "listen", "load",
    "local", "localtime", "localtimestamp", "location",
    "locator", "lock", "lower", "m",
    "map", "match", "max", "maxvalue",
    "message_length", "message_octet_length", "message_text", "method",
    "min", "minute", "minvalue", "mod",
    "mode", "modifies", "modify", "module",
    "month", "more", "move", "mumps",
    "name", "names", "national", "natural",
    "nchar", "nclob", "new", "next",
    "no", "nocreatedb", "nocreateuser", "none",
    "not", "nothing", "notify", "notnull",
    "null", "nullable", "nullif", "number",
    "numeric", "object", "octet_length", "of",
    "off", "offset", "oids", "old",
    "on", "only", "open", "operation",
    "operator", "option", "options", "or",
    "order", "ordinality", "out", "outer",
    "output", "overlaps", "overlay", "overriding",
    "owner", "pad", "parameter", "parameters",
    "parameter_mode", "parameter_name", "parameter_ordinal_position", "parameter_specific_catalog",
    "parameter_specific_name", "parameter_specific_schema", "partial", "pascal",
    "password", "path", "pendant", "placing",
    "pli", "position", "postfix", "precision",
    "prefix", "preorder", "prepare", "preserve",
    "primary", "prior", "privileges", "procedural",
    "procedure", "public", "read", "reads",
    "real", "recheck", "recursive", "ref",
    "references", "referencing", "reindex", "relative",
    "rename", "repeatable", "replace", "reset",
    "restart", "restrict", "result", "return",
    "returned_length", "returned_octet_length", "returned_sqlstate", "returns",
    "revoke", "right", "role", "rollback",
    "rollup", "routine", "routine_catalog", "routine_name",
    "routine_schema", "row", "rows", "row_count",
    "rule", "savepoint", "scale", "schema",
    "schema_name", "scope", "scroll", "search",
    "second", "section", "security", "select",
    "self", "sensitive", "sequence", "serializable",
    "server_name", "session", "session_user", "set",
    "setof", "sets", "share", "show",
    "similar", "simple", "size", "smallint",
    "some", "source", "space", "specific",
    "specifictype", "specific_name", "sql", "sqlcode",
    "sqlerror", "sqlexception", "sqlstate", "sqlwarning",
    "stable", "start", "state", "statement",
    "static", "statistics", "stdin", "stdout",
    "storage", "strict", "structure", "style",
    "subclass_origin", "sublist", "substring", "sum",
    "symmetric", "sysid", "system", "system_user",
    "table", "table_name", "temp", "template",
    "temporary", "terminate", "text", "than", "then",
    "time", "timestamp", "timezone_hour", "timezone_minute",
    "to", "toast", "trailing", "transaction",
    "transactions_committed", "transactions_rolled_back", "transaction_active", "transform",
    "transforms", "translate", "translation", "treat",
    "trigger", "trigger_catalog", "trigger_name", "trigger_schema",
    "trim", "true", "truncate", "trusted",
    "type", "uncommitted", "under", "unencrypted",
    "union", "unique", "unknown", "unlisten",
    "unnamed", "unnest", "until", "update",
    "upper", "usage", "user", "user_defined_type_catalog",
    "user_defined_type_name", "user_defined_type_schema", "using", "vacuum",
    "valid", "validator", "value", "values",
    "varchar", "variable", "varying", "verbose",
    "version", "view", "volatile", "when",
    "whenever", "where", "with", "without",
    "work", "write", "year", "zone"),
	
	
	"Bash" => array(
	"alias", "break", "case", "continue",
	"do", "done", "elif", "else",
	"esac", "exit", "export", "fi",
	"for", "if", "in", "return",
	"set", "then", "unalias", "unset",
	"while", "halt", "ifconfig", "lsmod",
	"modprobe", "reboot", "rmmod", "route",
	"shutdown", "traceroute", "awk", "basename",
	"cat", "cp", "echo", "egrep",
	"fgrep", "gawk", "grep", "gzip",
	"kill", "killall", "less", "md",
	"mkdir", "mv", "nice", "pidof",
	"ps", "rd", "read", "rm",
	"rmdir", "sed", "sleep", "test",
	"touch", "ulimit", "uname", "usleep",
	"zcat", "zless"
	),
	
	"Batch" => array(
	"do", "else", "end", "errorlevel",
	"exist", "exit", "for", "goto",
	"if", "not", "pause", "return",
	"say", "select", "then", "when",
	"ansi", "append", "assign", "attrib",
	"autofail", "backup", "basedev", "boot",
	"break", "buffers", "cache", "call",
	"cd", "chcp", "chdir", "chkdsk",
	"choice", "cls", "cmd", "codepage",
	"command", "comp", "copy", "country",
	"date", "ddinstal", "debug", "del",
	"detach", "device", "devicehigh", "devinfo",
	"dir", "diskcoache", "diskcomp", "diskcopy",
	"doskey", "dpath", "dumpprocess", "eautil",
	"endlocal", "erase", "exit_vdm", "extproc",
	"fcbs", "fdisk", "fdiskpm", "files",
	"find", "format", "fsaccess", "fsfilter",
	"graftabl",	"iopl",	"join",	"keyb", "keys",
	"label", "lastdrive", "libpath", "lh", "loadhigh",
	"makeini", "maxwait", "md", "mem", "memman", "mkdir", "mode", "move",
	"net",	"patch", "path", "pauseonerror", "picview", "pmrexx", "print",
	"printmonbufsize", "priority", "priority_disk_io", "prompt", "protectonly",
	"protshell", "pstat", "rd", "recover", "reipl", "ren", "rename", "replace",
	"restore", "rmdir", "rmsize", "run", "set", "setboot", "setlocal", "shell", 
	"shift", "sort", "spool", "start", "subst", "suppresspopups", "swappath", 
	"syslevel", "syslog", "threads", "time", "timeslice", "trace", "tracebuf", 
	"tracefmt", "trapdump", "tree", "type",	"undelete", "unpack", "use",
	"ver", "verify", "view", "vmdisk", "vol",
	"xcopy", "xcopy32", "xdfcopy",
	"echo", "off", "on"
	)

    );

    $case_insensitive = array(
        "VB" => true,
        "Pascal" => true,
        "PL/I"   => true,
        "SQL"    => true
    );
    $ncs = false;
    if (array_key_exists($language, $case_insensitive))
        $ncs = true;

    $text = (array_key_exists($language, $preproc))?
        preproc_replace($preproc[$language], $text) :
        $text;
    $text = (array_key_exists($language, $keywords))?
        keyword_replace($keywords[$language], $text, $ncs) :
        $text;

    return $text;
}


function rtrim1($span, $lang, $ch)
{
    return syntax_highlight_helper(substr($span, 0, -1), $lang);
}


function rtrim1_htmlesc($span, $lang, $ch)
{
    return htmlspecialchars(substr($span, 0, -1));
}


function sch_rtrim1($span, $lang, $ch)
{
    return sch_syntax_helper(substr($span, 0, -1));
}


function rtrim2($span, $lang, $ch)
{
    return substr($span, 0, -2);
}


function syn_proc($span, $lang, $ch)
{
    return syntax_highlight_helper($span, $lang);
}

function dash_putback($span, $lang, $ch)
{
    return syntax_highlight_helper('-' . $span, $lang);
}

function slash_putback($span, $lang, $ch)
{
    return syntax_highlight_helper('/' . $span, $lang);
}

function slash_putback_rtrim1($span, $lang, $ch)
{
    return rtrim1('/' . $span, $lang, $ch);
}

function lparen_putback($span, $lang, $ch)
{
    return syntax_highlight_helper('(' . $span, $lang);
}

function lparen_putback_rtrim1($span, $lang, $ch)
{
    return rtrim1('(' . $span, $lang, $ch);
}

function prepend_xml_opentag($span, $lang, $ch) 
{                                               
    return '<span class="xml_tag">&lt;' . $span;
}                                               

function proc_void($span, $lang, $ch)
{
    return $span;
}


/**
 * Syntax highlight function
 * Does the bulk of the syntax highlighting by lexing the input
 * string, then calling the helper function to highlight keywords.
 */
function syntax_highlight($text, $language)
{
    if ($language == "Plain Text") return $text;

    define("normal_text",   1, true);
    define("dq_literal",    2, true);
    define("dq_escape",     3, true);
    define("sq_literal",    4, true);
    define("sq_escape",     5, true);
    define("slash_begin",   6, true);
    define("star_comment",  7, true);
    define("star_end",      8, true);
    define("line_comment",  9, true);
    define("html_entity",  10, true);
    define("lc_escape",    11, true);
    define("block_comment",12, true);
    define("paren_begin",  13, true);
    define("dash_begin",   14, true);
    define("bt_literal",   15, true);
    define("bt_escape",    16, true);
    define("xml_tag_begin",17, true);
    define("xml_tag",      18, true);
    define("xml_pi",       19, true);
    define("sch_normal",   20, true);
    define("sch_stresc",    21, true);
    define("sch_idexpr",   22, true);
    define("sch_numlit",   23, true);
    define("sch_chrlit",   24, true);
    define("sch_strlit",   25, true);

    $initial_state["Scheme"] = sch_normal;

    $sch[sch_normal][0]     = sch_normal;
    $sch[sch_normal]['"']   = sch_strlit;
    $sch[sch_normal]["#"]   = sch_chrlit;
    $sch[sch_normal]["0"]   = sch_numlit;
    $sch[sch_normal]["1"]   = sch_numlit;
    $sch[sch_normal]["2"]   = sch_numlit;
    $sch[sch_normal]["3"]   = sch_numlit;
    $sch[sch_normal]["4"]   = sch_numlit;
    $sch[sch_normal]["5"]   = sch_numlit;
    $sch[sch_normal]["6"]   = sch_numlit;
    $sch[sch_normal]["7"]   = sch_numlit;
    $sch[sch_normal]["8"]   = sch_numlit;
    $sch[sch_normal]["9"]   = sch_numlit;

    $sch[sch_strlit]['"']   = sch_normal;
    $sch[sch_strlit]["\n"]  = sch_normal;
    $sch[sch_strlit]["\\"]  = sch_stresc;
    $sch[sch_strlit][0]     = sch_strlit;

    $sch[sch_chrlit][" "]   = sch_normal;
    $sch[sch_chrlit]["\t"]  = sch_normal;
    $sch[sch_chrlit]["\n"]  = sch_normal;
    $sch[sch_chrlit]["\r"]  = sch_normal;
    $sch[sch_chrlit][0]     = sch_chrlit;

    $sch[sch_numlit][" "]   = sch_normal;
    $sch[sch_numlit]["\t"]  = sch_normal;
    $sch[sch_numlit]["\n"]  = sch_normal;
    $sch[sch_numlit]["\r"]  = sch_normal;
    $sch[sch_numlit][0]     = sch_numlit;

    //
    // State transitions for C
    //
    $c89[normal_text]["\""] = dq_literal;
    $c89[normal_text]["'"]  = sq_literal;
    $c89[normal_text]["/"]  = slash_begin;
    $c89[normal_text][0]    = normal_text;

    $c89[dq_literal]["\""]  = normal_text;
    $c89[dq_literal]["\n"]  = normal_text;
    $c89[dq_literal]["\\"]  = dq_escape;
    $c89[dq_literal][0]     = dq_literal;

    $c89[dq_escape][0]      = dq_literal;

    $c89[sq_literal]["'"]   = normal_text;
    $c89[sq_literal]["\n"]  = normal_text;
    $c89[sq_literal]["\\"]  = sq_escape;
    $c89[sq_literal][0]     = sq_literal;

    $c89[sq_escape][0]      = sq_literal;

    $c89[slash_begin]["*"]  = star_comment;
    $c89[slash_begin][0]    = normal_text;

    $c89[star_comment]["*"] = star_end;
    $c89[star_comment][0]   = star_comment;

    $c89[star_end]["/"]     = normal_text;
    $c89[star_end]["*"]     = star_end;
    $c89[star_end][0]       = star_comment;

    //
    // State transitions for C++
    // Inherit transitions from C, and add line comment support
    //
    $cpp = $c89;
    $cpp[slash_begin]["/"]   = line_comment;
    $cpp[line_comment]["\n"] = normal_text;
    $cpp[line_comment]["\\"] = lc_escape;
    $cpp[line_comment][0]    = line_comment;

    $cpp[lc_escape]["\r"]    = lc_escape;
    $cpp[lc_escape][0]       = line_comment;

    //
    // State transitions for C99.
    // C99 supports line comments like C++
    //
    $c99 = $cpp;

    // State transitions for PL/I
    // Kinda like C
    $pli = $c89;

    //
    // State transitions for PHP
    // Inherit transitions from C++, and add perl-style line comment support
    $php = $cpp;
    $php[normal_text]["#"]   = line_comment;
    $php[sq_literal]["\n"]   = sq_literal;
    $php[dq_literal]["\n"]   = dq_literal;

    //
    // State transitions for Perl
    $perl[normal_text]["#"]  = line_comment;
    $perl[normal_text]["\""] = dq_literal;
    $perl[normal_text]["'"]  = sq_literal;
    $perl[normal_text][0]    = normal_text;

    $perl[dq_literal]["\""]  = normal_text;
    $perl[dq_literal]["\\"]  = dq_escape;
    $perl[dq_literal][0]     = dq_literal;

    $perl[dq_escape][0]      = dq_literal;

    $perl[sq_literal]["'"]   = normal_text;
    $perl[sq_literal]["\\"]  = sq_escape;
    $perl[sq_literal][0]     = sq_literal;

    $perl[sq_escape][0]      = sq_literal;

    $perl[line_comment]["\n"] = normal_text;
    $perl[line_comment][0]    = line_comment;

    $mirc[normal_text]["\""] = dq_literal;
    $mirc[normal_text][";"]  = line_comment;
    $mirc[normal_text][0]    = normal_text;

    $mirc[dq_literal]["\""]  = normal_text;
    $mirc[dq_literal]["\\"]  = dq_escape;
    $mirc[dq_literal][0]     = dq_literal;

    $mirc[dq_escape][0]      = dq_literal;

    $mirc[line_comment]["\n"] = normal_text;
    $mirc[line_comment][0]   = line_comment;
	
    $ruby = $perl;

    $python = $perl;

    $java = $cpp;

    $vb = $perl;
    $vb[normal_text]["#"] = normal_text;
    $vb[normal_text]["'"] = line_comment;

    $cs = $java;

    $pascal = $c89;
    $pascal[normal_text]["("]  = paren_begin;
    $pascal[normal_text]["/"]  = slash_begin;
    $pascal[normal_text]["{"]  = block_comment;

    $pascal[paren_begin]["*"]  = star_comment;
    $pascal[paren_begin]["'"]  = sq_literal;
    $pascal[paren_begin]['"']  = dq_literal;
    $pascal[paren_begin][0]    = normal_text;

    $pascal[slash_begin]["'"]  = sq_literal;
    $pascal[slash_begin]['"']  = dq_literal;
    $pascal[slash_begin]['/']  = line_comment;
    $pascal[slash_begin][0]    = normal_text;

    $pascal[star_comment]["*"] = star_end;
    $pascal[star_comment][0]   = star_comment;

    $pascal[block_comment]["}"] = normal_text;
    $pascal[block_comment][0]   = block_comment;

    $pascal[line_comment]["\n"] = normal_text;
    $pascal[line_comment][0]    = line_comment;

    $pascal[star_end][")"]     = normal_text;
    $pascal[star_end]["*"]     = star_end;
    $pascal[star_end][0]       = star_comment;

    $sql[normal_text]['"']     = dq_literal;
    $sql[normal_text]["'"]     = sq_literal;
    $sql[normal_text]['`']     = bt_literal;
    $sql[normal_text]['-']     = dash_begin;
    $sql[normal_text][0]       = normal_text;

    $sql[dq_literal]['"']      = normal_text;
    $sql[dq_literal]['\\']     = dq_escape;
    $sql[dq_literal][0]        = dq_literal;

    $sql[sq_literal]["'"]      = normal_text;
    $sql[sq_literal]['\\']     = sq_escape;
    $sql[sq_literal][0]        = sq_literal;

    $sql[bt_literal]['`']      = normal_text;
    $sql[bt_literal]['\\']     = bt_escape;
    $sql[bt_literal][0]        = bt_literal;

    $sql[dq_escape][0]         = dq_literal;
    $sql[sq_escape][0]         = sq_literal;
    $sql[bt_escape][0]         = bt_literal;

    $sql[dash_begin]["-"]      = line_comment;
    $sql[dash_begin][0]        = normal_text;

    $sql[line_comment]["\n"]   = normal_text;
    $sql[line_comment]["\\"]   = lc_escape;
    $sql[line_comment][0]      = line_comment;

    $sql[lc_escape]["\r"]      = lc_escape;
    $sql[lc_escape][0]         = line_comment;

    $xml[normal_text]["<"]     = xml_tag_begin;               
    $xml[normal_text]["&"]     = html_entity;                 
    $xml[normal_text][0]       = normal_text;                 
    $xml[html_entity][";"]     = normal_text;                 
    $xml[html_entity]["<"]     = xml_tag_begin;
    $xml[html_entity][0]       = html_entity;                 
    $xml[xml_tag_begin]["?"]   = xml_pi;                      
    $xml[xml_tag_begin]["!"]   = line_comment;                
    $xml[xml_tag_begin][0]     = xml_tag;                     
    $xml[xml_tag][">"]         = normal_text;                 
    $xml[xml_tag]["\""]        = dq_literal;           
    $xml[xml_tag]["'"]         = sq_literal;                   
    $xml[xml_tag][0]           = xml_tag;                     
    $xml[xml_pi][">"]          = normal_text;                 
    $xml[xml_pi][0]            = xml_tag;                     
    $xml[line_comment][">"]    = normal_text;                 
    $xml[line_comment][0]      = line_comment;                
    $xml[dq_literal]["\""]     = xml_tag;                     
    $xml[dq_literal]["&"]      = dq_escape;                   
    $xml[dq_literal][0]        = dq_literal;                  
    $xml[sq_literal]["'"]      = xml_tag;                     
    $xml[sq_literal]["&"]      = sq_escape;                   
    $xml[sq_literal][0]        = sq_literal;                  
    $xml[dq_escape][";"]       = dq_literal;                  
    $xml[dq_escape][0]         = dq_escape;                   
    
	$bash = $perl;
	$batch = $c89;;
	
    //
    // Main state transition table
    //
    $states = array(
        "C89"   => $c89,
        "C" => $c99,
        "C++" => $cpp,
        "PHP" => $php,
        "Perl" => $perl,
        "Java" => $java,
        "VB" => $vb,
        "C#" => $cs,
        "Ruby" => $ruby,
        "Python" => $python,
        "Pascal" => $pascal,
        "mIRC" => $mirc,
        "PL/I" => $pli,
        "SQL"  => $sql,
        "XML"  => $xml,
        "Scheme" => $sch,
        "Bash" => $bash,
        "Batch" => $batch
    );


    //
    // Process functions
    //
    $process["C89"][normal_text][sq_literal] = "rtrim1";
    $process["C89"][normal_text][dq_literal] = "rtrim1";
    $process["C89"][normal_text][slash_begin] = "rtrim1";
    $process["C89"][normal_text][0] = "syn_proc";

    $process["C89"][slash_begin][star_comment] = "rtrim1";
    $process["C89"][slash_begin][0] = "slash_putback";

    $process["Scheme"][sch_normal][sch_strlit] = "sch_rtrim1";
    $process["Scheme"][sch_normal][sch_chrlit] = "sch_rtrim1";
    $process["Scheme"][sch_normal][sch_numlit] = "sch_rtrim1";

    $process["SQL"][normal_text][sq_literal] = "rtrim1";
    $process["SQL"][normal_text][dq_literal] = "rtrim1";
    $process["SQL"][normal_text][bt_literal] = "rtrim1";
    $process["SQL"][normal_text][dash_begin] = "rtrim1";
    $process["SQL"][normal_text][0] = "syn_proc";

    $process["SQL"][dash_begin][line_comment] = "rtrim1";
    $process["SQL"][dash_begin][0] = "dash_putback";

    $process["PL/I"] = $process["C89"];

    $process["C++"] = $process["C89"];
    $process["C++"][slash_begin][line_comment] = "rtrim1";

    $process["C"] = $process["C++"];

    $process["PHP"] = $process["C++"];
    $process["PHP"][normal_text][line_comment] = "rtrim1";

    $process["Perl"][normal_text][sq_literal] = "rtrim1";
    $process["Perl"][normal_text][dq_literal] = "rtrim1";
    $process["Perl"][normal_text][line_comment] = "rtrim1";
    $process["Perl"][normal_text][0] = "syn_proc";

    $process["Ruby"] = $process["Perl"];
    $process["Python"] = $process["Perl"];

    $process["mIRC"][normal_text][dq_literal] = "rtrim1";
    $process["mIRC"][normal_text][line_comment] = "rtrim1";
    $process["mIRC"][normal_text][0] = "syn_proc";

    $process["VB"] = $process["Perl"];

    $process["Java"] = $process["C++"];

    $process["C#"] = $process["Java"];

    $process["Pascal"] = $process["C++"];
    $process["Pascal"][normal_text][line_comment] = "rtrim1";
    $process["Pascal"][normal_text][block_comment] = "rtrim1";
    $process["Pascal"][normal_text][paren_begin] = "rtrim1";
    $process["Pascal"][slash_begin][sq_literal] = "slash_putback_rtrim1";
    $process["Pascal"][slash_begin][dq_literal] = "slash_putback_rtrim1";
    $process["Pascal"][slash_begin][0] = "slash_putback";
    $process["Pascal"][paren_begin][sq_literal] = "lparen_putback_rtrim1";
    $process["Pascal"][paren_begin][dq_literal] = "lparen_putback_rtrim1";
    $process["Pascal"][paren_begin][star_comment] = "rtrim1";
    $process["Pascal"][paren_begin][0] = "lparen_putback";

    $process["XML"][normal_text][xml_tag_begin] = "rtrim1";
    $process["XML"][normal_text][html_entity] = "rtrim1";
    $process["XML"][html_entity][xml_tag_begin] = "rtrim1";
    $process["XML"][html_entity][0] = "proc_void";
    $process["XML"][xml_tag_begin][xml_tag] = "prepend_xml_opentag";
    $process["XML"][xml_tag_begin][xml_pi] = "rtrim1";
    $process["XML"][xml_tag_begin][line_comment] = "rtrim1";
    $process["XML"][line_comment][normal_text] = "rtrim1_htmlesc";
    $process["XML"][xml_tag][normal_text] = "rtrim1";
    $process["XML"][xml_tag][dq_literal] = "rtrim1";
    $process["XML"][dq_literal][xml_tag] = "rtrim1";
    $process["XML"][dq_literal][dq_escape] = "rtrim1";
	
	$process["Bash"] = $process["Perl"];
	$process["Batch"] = $process["C++"];


    $process_end["C89"] = "syntax_highlight_helper";
    $process_end["C++"] = $process_end["C89"];
    $process_end["C"] = $process_end["C89"];
    $process_end["PHP"] = $process_end["C89"];
    $process_end["Perl"] = $process_end["C89"];
    $process_end["Java"] = $process_end["C89"];
    $process_end["VB"] = $process_end["C89"];
    $process_end["C#"] = $process_end["C89"];
    $process_end["Ruby"] = $process_end["C89"];
    $process_end["Python"] = $process_end["C89"];
    $process_end["Pascal"] = $process_end["C89"];
    $process_end["mIRC"] = $process_end["C89"];
    $process_end["PL/I"] = $process_end["C89"];
    $process_end["SQL"] = $process_end["C89"];
    $process_end["Scheme"] = "sch_syntax_helper";
    $process_end["Bash"] = $process_end["C89"];
    $process_end["Batch"] = $process_end["C89"];

    $edges["C89"][normal_text .",". dq_literal]   = '<span class="literal">"';
    $edges["C89"][normal_text .",". sq_literal]   = '<span class="literal">\'';
    $edges["C89"][slash_begin .",". star_comment] = '<span class="comment">/*';
    $edges["C89"][dq_literal .",". normal_text]   = '</span>';
    $edges["C89"][sq_literal .",". normal_text]   = '</span>';
    $edges["C89"][star_end .",". normal_text]     = '</span>';

    $edges["Scheme"][sch_normal .",". sch_strlit] = '<span class="sch_str">"';
    $edges["Scheme"][sch_normal .",". sch_numlit] = '<span class="sch_num">';
    $edges["Scheme"][sch_normal .",". sch_chrlit] = '<span class="sch_chr">#';
    $edges["Scheme"][sch_strlit .",". sch_normal] = '</span>';
    $edges["Scheme"][sch_numlit .",". sch_normal] = '</span>';
    $edges["Scheme"][sch_chrlit .",". sch_normal] = '</span>';

    $edges["SQL"][normal_text .",". dq_literal]   = '<span class="literal">"';
    $edges["SQL"][normal_text .",". sq_literal]   = '<span class="literal">\'';
    $edges["SQL"][dash_begin .",". line_comment] = '<span class="comment">--';
    $edges["SQL"][normal_text .",". bt_literal]   = '`';
    $edges["SQL"][dq_literal .",". normal_text]   = '</span>';
    $edges["SQL"][sq_literal .",". normal_text]   = '</span>';
    $edges["SQL"][line_comment .",". normal_text] = '</span>';

    $edges["PL/I"] = $edges["C89"];

    $edges["C++"] = $edges["C89"];
    $edges["C++"][slash_begin .",". line_comment] = '<span class="comment">//';
    $edges["C++"][line_comment .",". normal_text] = '</span>';

    $edges["C"] = $edges["C++"];

    $edges["PHP"] = $edges["C++"];
    $edges["PHP"][normal_text .",". line_comment] = '<span class="comment">#';

    $edges["Perl"][normal_text .",". dq_literal]   = '<span class="literal">"';
    $edges["Perl"][normal_text .",". sq_literal]   = '<span class="literal">\'';
    $edges["Perl"][dq_literal .",". normal_text]   = '</span>';
    $edges["Perl"][sq_literal .",". normal_text]   = '</span>';
    $edges["Perl"][normal_text .",". line_comment] = '<span class="comment">#';
    $edges["Perl"][line_comment .",". normal_text] = '</span>';

    $edges["Ruby"] = $edges["Perl"];

    $edges["Python"] = $edges["Perl"];

    $edges["mIRC"][normal_text .",". dq_literal] = '<span class="literal">"';
    $edges["mIRC"][normal_text .",". line_comment] = '<span class="comment">;';
    $edges["mIRC"][dq_literal .",". normal_text] = '</span>';
    $edges["mIRC"][line_comment .",". normal_text] = '</span>';

    $edges["VB"] = $edges["Perl"];
    $edges["VB"][normal_text .",". line_comment] = '<span class="comment">\'';

    $edges["Java"] = $edges["C++"];

    $edges["C#"] = $edges["Java"];

    $edges["Pascal"] = $edges["C89"];
    $edges["Pascal"][paren_begin .",". star_comment] = '<span class="comment">(*';
    $edges["Pascal"][paren_begin .",". dq_literal]   = '<span class="literal">"';
    $edges["Pascal"][paren_begin .",". sq_literal]   = '<span class="literal">\'';
    $edges["Pascal"][slash_begin .",". dq_literal]   = '<span class="literal">"';
    $edges["Pascal"][slash_begin .",". sq_literal]   = '<span class="literal">\'';
    $edges["Pascal"][slash_begin .",". line_comment] = '<span class="comment">//';
    $edges["Pascal"][normal_text . "," . block_comment] = '<span class="comment">{';
    $edges["Pascal"][line_comment . "," . normal_text] = '</span>';
    $edges["Pascal"][block_comment . "," . normal_text] = '</span>';

    $edges["XML"][normal_text . "," . html_entity] = '<span class="html_entity">&amp;';
    $edges["XML"][html_entity . "," . normal_text] = '</span>';
    $edges["XML"][html_entity . "," . xml_tag_begin] = '</span>';
    $edges["XML"][xml_tag . "," . normal_text] = '&gt;</span>';
    $edges["XML"][xml_tag_begin . "," . xml_pi] = '<span class="xml_pi">&lt;?';
    $edges["XML"][xml_tag_begin . "," . line_comment] = '<span class="comment">&lt;!';
    $edges["XML"][line_comment . "," . normal_text] = '&gt;</span>'; 
    $edges["XML"][xml_tag .",". dq_literal]   = '<span class="literal">"';
    $edges["XML"][dq_literal . "," . xml_tag] = '"</span>';
    $edges["XML"][dq_literal . "," . dq_escape] = '<span class="html_entity">&amp;';
    $edges["XML"][dq_escape . "," . dq_literal] = '</span>';
    $edges["XML"][xml_tag .",". sq_literal]   = '<span class="literal">\'';
    $edges["XML"][sq_literal . "," . xml_tag] = '\'</span>';
    $edges["XML"][sq_literal . "," . sq_escape] = '<span class="html_entity">&amp;'; 
    $edges["XML"][sq_escape . "," . sq_literal] = '</span>'; 

    $edges["Bash"] = $edges["Perl"];
    $edges["Batch"] = $edges["C89"];
	

    //
    // The State Machine
    //
    if (array_key_exists($language, $initial_state))
        $state = $initial_state[$language];
    else
        $state = normal_text;
    $output = "";
    $span = "";
    while (strlen($text) > 0)
    {
        $ch = substr($text, 0, 1);
        $text = substr($text, 1);

        $oldstate = $state;
        $state = (array_key_exists($ch, $states[$language][$state]))?
            $states[$language][$state][$ch] :
            $states[$language][$state][0];

        $span .= $ch;

        if ($oldstate != $state)
        {
            if (array_key_exists($language, $process) &&
                array_key_exists($oldstate, $process[$language]))
            {
                if (array_key_exists($state, $process[$language][$oldstate]))
                {
                    $pf = $process[$language][$oldstate][$state];
                    $output .= $pf($span, $language, $ch);
                }
                else
                {
                    $pf = $process[$language][$oldstate][0];
                    $output .= $pf($span, $language, $ch);
                }
            }
            else
            {
                $output .= $span;
            }

            if (array_key_exists($language, $edges) &&
                array_key_exists("$oldstate,$state", $edges[$language]))
                $output .= $edges[$language]["$oldstate,$state"];

            $span = "";
        }
    }

    if (array_key_exists($language, $process_end) && $state == normal_text)
        $output .= $process_end[$language]($span, $language);
    else
        $output .= $span;

    if ($state != normal_text)
    {
        if (array_key_exists($language, $edges) &&
            array_key_exists("$state," . normal_text, $edges[$language]))
            $output .= $edges[$language]["$state," . normal_text];
    }
                
    return $output;
}

?>
