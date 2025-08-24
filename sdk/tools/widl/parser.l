/* -*-C-*-
 * IDL Compiler
 *
 * Copyright 2002 Ove Kaaven
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

%option bison-bridge
%option bison-locations
%option stack
%option noinput nounput noyy_top_state
%option noyywrap
%option 8bit never-interactive prefix="parser_"

ws    [ \f\t\r]
hd    [0-9a-fA-F]
uuid  {hd}{8}-{hd}{4}-{hd}{4}-{hd}{4}-{hd}{12}

%x ATTR
%x PP_LINE
%x PP_FILE
%x PP_PRAGMA

%{

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>
#include <limits.h>
#define YY_NO_UNISTD_H

#include "widl.h"
#include "utils.h"
#include "parser.h"
#include "wpp_private.h"

#define YYerror PARSER_error
#define YYSTYPE PARSER_STYPE
#define YYLTYPE PARSER_LTYPE
#define YYUNDEF PARSER_UNDEF
#define yyerror parser_error

#include "parser.tab.h"

static void reset_location( struct location *where, const char *input_name );
static void update_location( struct location *where, const char *yytext );
static void end_of_line( struct location *where );

#define YY_USER_INIT    reset_location( yylloc, input_name )
#define YY_USER_ACTION  update_location( yylloc, yytext );

static void switch_to_acf(void);

static warning_list_t *disabled_warnings = NULL;

struct import_state
{
    YY_BUFFER_STATE buffer;
    char *input_name;
    struct location where;
    struct list entry;
};
static struct list import_stack = LIST_INIT( import_stack );
int parse_only = 0;

struct import
{
    const char *name;
    struct list entry;
};
static struct list imports = LIST_INIT( imports );
static struct location previous_location;

/* converts an integer in string form to an unsigned long and prints an error
 * on overflow */
static unsigned int xstrtoul(const char *nptr, char **endptr, int base)
{
    unsigned long val;

    errno = 0;
    val = strtoul(nptr, endptr, base);
    if ((val == ULONG_MAX && errno == ERANGE) || ((unsigned int)val != val))
        error_loc("integer constant %s is too large\n", nptr);
    return val;
}

static int token_uuid( const char *str, YYSTYPE *yylval )
{
    struct uuid *uuid;
    char tmp[3] = {0};

    if (*str == '\"') str++;

    uuid = xmalloc( sizeof(*uuid) );
    uuid->Data1 = strtoul( str , NULL, 16 );
    uuid->Data2 = strtoul( str + 9, NULL, 16 );
    uuid->Data3 = strtoul( str + 14, NULL, 16 );
    memcpy( tmp, str + 19, 2 );
    uuid->Data4[0] = strtoul( tmp, NULL, 16 );
    memcpy( tmp, str + 21, 2 );
    uuid->Data4[1] = strtoul( tmp, NULL, 16 );
    memcpy( tmp, str + 24, 2 );
    uuid->Data4[2] = strtoul( tmp, NULL, 16 );
    memcpy( tmp, str + 26, 2 );
    uuid->Data4[3] = strtoul( tmp, NULL, 16 );
    memcpy( tmp, str + 28, 2 );
    uuid->Data4[4] = strtoul( tmp, NULL, 16 );
    memcpy( tmp, str + 30, 2 );
    uuid->Data4[5] = strtoul( tmp, NULL, 16 );
    memcpy( tmp, str + 32, 2 );
    uuid->Data4[6] = strtoul( tmp, NULL, 16 );
    memcpy( tmp, str + 34, 2 );
    uuid->Data4[7] = strtoul( tmp, NULL, 16 );

    yylval->uuid = uuid;
    return aUUID;
}

static int token_str( int token, const char *str, YYSTYPE *yylval )
{
    char *tmp = xstrdup( str );

    if (token == aWSTRING || token == aSTRING || token == aSQSTRING)
    {
        char *src, *dst;
        src = dst = ++tmp; /* skip first quote */
        while (*src)
        {
            if (*src == '\\') src++;
            *dst++ = *src++;
        }
        dst[-1] = 0; /* strip last quote */
    }

    yylval->str = tmp;
    return token;
}

static int token_num( const char *yytext, YYSTYPE *yylval, int is_hex )
{
    yylval->integer.value = xstrtoul( yytext, NULL, 0 );
    yylval->integer.is_hex = is_hex;
    yylval->integer.is_long = !!strchr(yytext, 'l');
    yylval->integer.is_unsigned = !!strchr(yytext, 'u');
    return is_hex ? aHEXNUM : aNUM;
}

static int token_ident( const char *str, YYSTYPE *yylval )
{
    return token_str( is_type( str ) ? aKNOWNTYPE : aIDENTIFIER, str, yylval );
}

static int token_winrt( int token, const char *str, YYSTYPE *yylval )
{
    if (winrt_mode) return token;
    return token_ident( str, yylval );
}

static void winrt_enable( int ns_prefix )
{
    if (!list_empty( &import_stack ) && !winrt_mode) error_loc( "WinRT IDL file imported in non-winrt mode.\n" );

    use_abi_namespace = ns_prefix;
    winrt_mode = TRUE;
}

%}

/*
 **************************************************************************
 * The flexer starts here
 **************************************************************************
 */
%%
<PP_PRAGMA>{
    midl_echo/"("                               {
                                                    yy_pop_state();
                                                    yylloc->first_line -= 1;
                                                    return tCPPQUOTE;
                                                }
    winrt{ws}+ns_prefix[^\n]*                   {
                                                    yy_pop_state();
                                                    yylloc->first_line -= 1;
                                                    winrt_enable( TRUE );
                                                }
    winrt[^\n]*                                 {
                                                    yy_pop_state();
                                                    yylloc->first_line -= 1;
                                                    winrt_enable( FALSE );
                                                }
    [^\n]*                                      {
                                                    yy_pop_state();
                                                    yylloc->first_line -= 1;
                                                    return token_str( aPRAGMA, yytext, yylval );
                                                }
}
<PP_LINE>[0-9]+{ws}*                            {
                                                    yylloc->first_line = strtoul( yytext, NULL, 10 ) - 1;
                                                    yylloc->last_line = yylloc->first_line;
                                                    yy_pop_state();
                                                    yy_push_state(PP_FILE);
                                                }
<PP_FILE>\"(\\[^n]|[^"\\\n])*\"{ws}*            {
                                                    input_name = xstrdup( yytext + 1 );
                                                    *strchr( input_name, '"' ) = 0;
                                                    yylloc->input_name = input_name;
                                                }
<PP_FILE>[^"][^\n]*                             { yy_pop_state(); }

<ATTR>{
    \]                                          { yy_pop_state(); return ']'; }

    ({uuid}|\"{uuid}\")                         { return token_uuid( yytext, yylval ); }
    activatable                                 { return token_winrt( tACTIVATABLE, yytext, yylval ); }
    aggregatable                                { return tAGGREGATABLE; }
    agile                                       { return token_winrt( tAGILE, yytext, yylval ); }
    all_nodes                                   { return tALLNODES; }
    allocate                                    { return tALLOCATE; }
    annotation                                  { return tANNOTATION; }
    apartment                                   { return tAPARTMENT; }
    appobject                                   { return tAPPOBJECT; }
    async                                       { return tASYNC; }
    async_uuid                                  { return tASYNCUUID; }
    auto_handle                                 { return tAUTOHANDLE; }
    bindable                                    { return tBINDABLE; }
    both                                        { return tBOTH; }
    broadcast                                   { return tBROADCAST; }
    byte_count                                  { return tBYTECOUNT; }
    call_as                                     { return tCALLAS; }
    callback                                    { return tCALLBACK; }
    code                                        { return tCODE; }
    comm_status                                 { return tCOMMSTATUS; }
    composable                                  { return token_winrt( tCOMPOSABLE, yytext, yylval ); }
    context_handle                              { return tCONTEXTHANDLE; }
    context_handle_noserialize                  { return tCONTEXTHANDLENOSERIALIZE; }
    context_handle_serialize                    { return tCONTEXTHANDLENOSERIALIZE; }
    contract                                    { return token_winrt( tCONTRACT, yytext, yylval ); }
    contractversion                             { return token_winrt( tCONTRACTVERSION, yytext, yylval ); }
    control                                     { return tCONTROL; }
    custom                                      { return tCUSTOM; }
    decode                                      { return tDECODE; }
    default_overload                            { return tDEFAULT_OVERLOAD; }
    defaultbind                                 { return tDEFAULTBIND; }
    defaultcollelem                             { return tDEFAULTCOLLELEM; }
    defaultvalue                                { return tDEFAULTVALUE; }
    defaultvtable                               { return tDEFAULTVTABLE; }
    deprecated                                  { return token_winrt( tDEPRECATED, yytext, yylval ); }
    disable_consistency_check                   { return tDISABLECONSISTENCYCHECK; }
    displaybind                                 { return tDISPLAYBIND; }
    dllname                                     { return tDLLNAME; }
    dont_free                                   { return tDONTFREE; }
    dual                                        { return tDUAL; }
    enable_allocate                             { return tENABLEALLOCATE; }
    encode                                      { return tENCODE; }
    endpoint                                    { return tENDPOINT; }
    entry                                       { return tENTRY; }
    eventadd                                    { return token_winrt( tEVENTADD, yytext, yylval ); }
    eventremove                                 { return token_winrt( tEVENTREMOVE, yytext, yylval ); }
    exclusiveto                                 { return token_winrt( tEXCLUSIVETO, yytext, yylval ); }
    explicit_handle                             { return tEXPLICITHANDLE; }
    fault_status                                { return tFAULTSTATUS; }
    flags                                       { return token_winrt( tFLAGS, yytext, yylval ); }
    force_allocate                              { return tFORCEALLOCATE; }
    free                                        { return tFREE; }
    handle                                      { return tHANDLE; }
    helpcontext                                 { return tHELPCONTEXT; }
    helpfile                                    { return tHELPFILE; }
    helpstring                                  { return tHELPSTRING; }
    helpstringcontext                           { return tHELPSTRINGCONTEXT; }
    helpstringdll                               { return tHELPSTRINGDLL; }
    hidden                                      { return tHIDDEN; }
    id                                          { return tID; }
    idempotent                                  { return tIDEMPOTENT; }
    ignore                                      { return tIGNORE; }
    iid_is                                      { return tIIDIS; }
    immediatebind                               { return tIMMEDIATEBIND; }
    implicit_handle                             { return tIMPLICITHANDLE; }
    in                                          { return tIN; }
    in_line                                     { return tIN_LINE; }
    input_sync                                  { return tINPUTSYNC; }
    lcid                                        { return tLCID; }
    length_is                                   { return tLENGTHIS; }
    licensed                                    { return tLICENSED; }
    local                                       { return tLOCAL; }
    marshaling_behavior                         { return token_winrt( tMARSHALINGBEHAVIOR, yytext, yylval ); }
    maybe                                       { return tMAYBE; }
    message                                     { return tMESSAGE; }
    mta                                         { return tMTA; }
    neutral                                     { return tNEUTRAL; }
    nocode                                      { return tNOCODE; }
    nonbrowsable                                { return tNONBROWSABLE; }
    noncreatable                                { return tNONCREATABLE; }
    none                                        { return token_winrt( tNONE, yytext, yylval ); }
    nonextensible                               { return tNONEXTENSIBLE; }
    notify                                      { return tNOTIFY; }
    notify_flag                                 { return tNOTIFYFLAG; }
    object                                      { return tOBJECT; }
    odl                                         { return tODL; }
    oleautomation                               { return tOLEAUTOMATION; }
    optimize                                    { return tOPTIMIZE; }
    optional                                    { return tOPTIONAL; }
    out                                         { return tOUT; }
    overload                                    { return tOVERLOAD; }
    partial_ignore                              { return tPARTIALIGNORE; }
    pointer_default                             { return tPOINTERDEFAULT; }
    progid                                      { return tPROGID; }
    propget                                     { return tPROPGET; }
    propput                                     { return tPROPPUT; }
    propputref                                  { return tPROPPUTREF; }
    protected                                   { return tPROTECTED; }
    proxy                                       { return tPROXY; }
    ptr                                         { return tPTR; }
    public                                      { return tPUBLIC; }
    range                                       { return tRANGE; }
    readonly                                    { return tREADONLY; }
    ref                                         { return tREF; }
    represent_as                                { return tREPRESENTAS; }
    requestedit                                 { return tREQUESTEDIT; }
    restricted                                  { return tRESTRICTED; }
    retval                                      { return tRETVAL; }
    single                                      { return tSINGLE; }
    single_node                                 { return tSINGLENODE; }
    size_is                                     { return tSIZEIS; }
    source                                      { return tSOURCE; }
    standard                                    { return token_winrt( tSTANDARD, yytext, yylval ); }
    static                                      { return token_winrt( tSTATIC, yytext, yylval ); }
    strict_context_handle                       { return tSTRICTCONTEXTHANDLE; }
    string                                      { return tSTRING; }
    switch_is                                   { return tSWITCHIS; }
    switch_type                                 { return tSWITCHTYPE; }
    threading                                   { return tTHREADING; }
    transmit_as                                 { return tTRANSMITAS; }
    uidefault                                   { return tUIDEFAULT; }
    unique                                      { return tUNIQUE; }
    user_marshal                                { return tUSERMARSHAL; }
    usesgetlasterror                            { return tUSESGETLASTERROR; }
    uuid                                        { return tUUID; }
    v1_enum                                     { return tV1ENUM; }
    vararg                                      { return tVARARG; }
    version                                     { return tVERSION; }
    vi_progid                                   { return tVIPROGID; }
    wire_marshal                                { return tWIREMARSHAL; }
}

<INITIAL>{
    ^{ws}*\#{ws}*pragma{ws}+                    { yy_push_state( PP_PRAGMA ); }
    ^{ws}*midl_pragma{ws}+warning               { return tPRAGMA_WARNING; }

    [0-9]+\.[0-9]+([eE][+-]?[0-9]+)*            {
                                                    yylval->dbl = strtod( yytext, NULL );
                                                    return aDOUBLE;
                                                }
}

SAFEARRAY{ws}*/\(	return tSAFEARRAY;

<INITIAL,ATTR>{
    ^{ws}*\#{ws}*                               { yy_push_state(PP_LINE); }
    \[                                          { yy_push_state(ATTR); return '['; }

    FALSE                                       { return tFALSE; }
    NULL                                        { return tNULL; }
    TRUE                                        { return tTRUE; }
    _?_?cdecl                                   { return token_str( tCDECL, "__cdecl", yylval ); }
    _?_?pascal                                  { return token_str( tPASCAL, "__pascal", yylval ); }
    _?_?stdcall                                 { return token_str( tSTDCALL, "__stdcall", yylval ); }
    __?fastcall                                 { return token_str( tFASTCALL, "__fastcall", yylval ); }
    __int32                                     { return tINT32; }
    __int3264                                   { return tINT3264; }
    __int64                                     { return tINT64; }
    apicontract                                 { return token_winrt( tAPICONTRACT, yytext, yylval ); }
    boolean                                     { return tBOOLEAN; }
    byte                                        { return tBYTE; }
    case                                        { return tCASE; }
    char                                        { return tCHAR; }
    coclass                                     { return tCOCLASS; }
    const                                       { return tCONST; }
    cpp_quote                                   { return tCPPQUOTE; }
    declare                                     { return token_winrt( tDECLARE, yytext, yylval ); }
    default                                     { return tDEFAULT; }
    delegate                                    { return token_winrt( tDELEGATE, yytext, yylval ); }
    dispinterface                               { return tDISPINTERFACE; }
    double                                      { return tDOUBLE; }
    enum                                        { return tENUM; }
    error_status_t                              { return tERRORSTATUST; }
    extern                                      { return tEXTERN; }
    float                                       { return tFLOAT; }
    handle_t                                    { return tHANDLET; }
    hyper                                       { return tHYPER; }
    import                                      { return tIMPORT; }
    importlib                                   { return tIMPORTLIB; }
    inline                                      { return tINLINE; }
    int                                         { return tINT; }
    interface                                   { return tINTERFACE; }
    library                                     { return tLIBRARY; }
    long                                        { return tLONG; }
    methods                                     { return tMETHODS; }
    module                                      { return tMODULE; }
    namespace                                   { return token_winrt( tNAMESPACE, yytext, yylval ); }
    properties                                  { return tPROPERTIES; }
    register                                    { return tREGISTER; }
    requires                                    { return token_winrt( tREQUIRES, yytext, yylval ); }
    runtimeclass                                { return token_winrt( tRUNTIMECLASS, yytext, yylval ); }
    short                                       { return tSHORT; }
    signed                                      { return tSIGNED; }
    sizeof                                      { return tSIZEOF; }
    small                                       { return tSMALL; }
    static                                      { return tSTATIC; }
    struct                                      { return tSTRUCT; }
    switch                                      { return tSWITCH; }
    typedef                                     { return tTYPEDEF; }
    union                                       { return tUNION; }
    unsigned                                    { return tUNSIGNED; }
    void                                        { return tVOID; }
    wchar_t                                     { return tWCHAR; }

    [a-zA-Z_][0-9a-zA-Z_]*                      { return token_ident( yytext, yylval ); }

    0[xX]{hd}+[uU]?[lL]?                        { return token_num( yytext, yylval, TRUE ); }
    [0-9]+[uU]?[lL]?                            { return token_num( yytext, yylval, FALSE ); }

    L\"(\\.|[^"\\])*\"                          { return token_str( aWSTRING, yytext + 1, yylval ); }
    \"(\\.|[^"\\])*\"                           { return token_str( aSTRING, yytext, yylval ); }
    \'(\\.|[^'\\])*\'                           { return token_str( aSQSTRING, yytext, yylval ); }

    \n                                          { end_of_line( yylloc ); }
    {ws}                                        {}
    \<\<                                        { return SHL; }
    \>\>                                        { return SHR; }
    \-\>                                        { return MEMBERPTR; }
    ==                                          { return EQUALITY; }
    !=                                          { return INEQUALITY; }
    \>=                                         { return GREATEREQUAL; }
    \<=                                         { return LESSEQUAL; }
    \|\|                                        { return LOGICALOR; }
    &&                                          { return LOGICALAND; }
    \.\.\.                                      { return ELLIPSIS; }
    .                                           { return yytext[0]; }
}

<<EOF>>			{
                            if (!list_empty( &import_stack ))
                                return aEOF;
                            if (acf_name)
                            {
                                switch_to_acf();
                                return aACF;
                            }
                            yyterminate();
			}
%%

static void print_imports(void)
{
    struct import_state *state, *next;

    if (list_empty( &import_stack )) return;

    fprintf( stderr, "In file included from " );
    LIST_FOR_EACH_ENTRY_SAFE_REV( state, next, &import_stack, struct import_state, entry )
    {
        if (&next->entry == &import_stack) break;
        fprintf( stderr, "%s:%d,\n", state->input_name, state->where.first_line );
        fprintf( stderr, "                 from ");
    }
    fprintf( stderr, "%s:%d:\n", state->input_name, state->where.first_line );
}

struct location pop_import(void)
{
    struct list *entry = list_head( &import_stack );
    struct import_state *state;
    struct location where;
    assert( entry );

    state = LIST_ENTRY( entry, struct import_state, entry );
    list_remove( &state->entry );
    parse_only = !list_empty( &import_stack );

    if (yyin) fclose( yyin );
    yy_delete_buffer( YY_CURRENT_BUFFER );
    yy_switch_to_buffer( state->buffer );

    input_name = state->input_name;
    where = state->where;
    free( state );
    return where;
}

void push_import( const char *import_name, struct location *where )
{
    struct import_state *state;
    struct import *import;
    FILE *file;

    state = xmalloc( sizeof(struct import_state ));
    list_add_head( &import_stack, &state->entry );
    parse_only = !list_empty( &import_stack );

    state->buffer = YY_CURRENT_BUFFER;
    state->input_name = input_name;
    state->where = *where;
    input_name = NULL;

    /* reset buffer for <<EOF>>, in case import fails or already imported */
    yy_scan_string( "" );

    LIST_FOR_EACH_ENTRY( import, &imports, struct import, entry )
        if (!strcmp( import->name, import_name )) return;  /* already imported */
    if (!strcmp( idl_name, import_name )) return;  /* already imported */

    import = xmalloc( sizeof(struct import) );
    import->name = xstrdup( import_name );
    list_add_tail( &imports, &import->entry );

    input_name = find_input_file( import_name, state->input_name );
    file = open_input_file( input_name );
    reset_location( where, input_name );

    yy_switch_to_buffer( yy_create_buffer( file, YY_BUF_SIZE ) );
}

static void switch_to_acf(void)
{
    FILE *file;

    if (yyin) fclose( yyin );
    yy_delete_buffer( YY_CURRENT_BUFFER );

    input_name = xstrdup( acf_name );
    file = open_input_file( input_name );
    acf_name = NULL;

    yy_switch_to_buffer( yy_create_buffer( file, YY_BUF_SIZE ) );
}

void close_all_inputs(void)
{
    while (!list_empty( &import_stack )) pop_import();
    if (yyin) fclose( yyin );
}

static void reset_location( struct location *where, const char *input_name )
{
    where->first_line = 1;
    where->last_line = 1;
    where->first_column = 1;
    where->last_column = 1;
    where->input_name = xstrdup( input_name );
}

static void update_location( struct location *where, const char *yytext )
{
    int len = strlen( yytext );
    previous_location = *where;
    where->first_column = where->last_column;
    where->last_column += len;
}

static void end_of_line( struct location *where )
{
    where->first_line++;
    where->last_line++;
    where->first_column = 1;
    where->last_column = 1;
}

void init_location( struct location *where, const struct location *begin, const struct location *end )
{
    if (!begin) begin = &previous_location;
    *where = *begin;

    if (end)
    {
        where->last_line   = end->last_line;
        where->last_column = end->last_column;
    }
    else
    {
        where->first_line   = begin->last_line;
        where->first_column = begin->last_column;
    }
}

static void diagnostic( const struct location *where, const char *type, const char *message )
{
    char buffer[1024], *line = NULL;
    FILE *file;
    int i;

    if (!where) where = &previous_location;

    print_imports();

    fprintf( stderr, "%s:%d:%d: %s: %s\n", where->input_name, where->first_line, where->first_column, type, message );

    if (!where->input_name || !(file = fopen( where->input_name, "r" ))) return;
    for (i = 0; i < where->first_line; i++) if (!(line = fgets( buffer, sizeof(buffer), file ))) break;
    fclose( file );
    if (!line) return;
    fprintf( stderr, "%s", line );

    line = buffer;
    for (i = 0; i < where->first_column - 1; i++) *line++ = ' ';
    *line++ = '^';
    for (i = where->first_column + 1; i < where->last_column; i++) *line++ = '~';
    *line = '\0';
    fprintf( stderr, "%s\n", buffer );
}

void parser_error( const struct location *where, const char *message )
{
    diagnostic( where, "error", message );
}

void parser_warning( const struct location *where, const char *message )
{
    diagnostic( where, "warning", message );
}

static void warning_disable(int warning)
{
    warning_t *warning_entry;
    LIST_FOR_EACH_ENTRY(warning_entry, disabled_warnings, warning_t, entry)
        if(warning_entry->num == warning)
            return;
    warning_entry = xmalloc( sizeof(*warning_entry) );
    warning_entry->num = warning;
    list_add_tail(disabled_warnings, &warning_entry->entry);
}

static void warning_enable(int warning)
{
    warning_t *warning_entry;
    LIST_FOR_EACH_ENTRY(warning_entry, disabled_warnings, warning_t, entry)
        if(warning_entry->num == warning)
        {
            list_remove(&warning_entry->entry);
            free(warning_entry);
            break;
        }
}

int do_warning(const char *toggle, warning_list_t *wnum)
{
    warning_t *warning, *next;
    int ret = 1;
    if(!disabled_warnings)
    {
        disabled_warnings = xmalloc( sizeof(*disabled_warnings) );
        list_init( disabled_warnings );
    }

    if(!strcmp(toggle, "disable"))
        LIST_FOR_EACH_ENTRY(warning, wnum, warning_t, entry)
            warning_disable(warning->num);
    else if(!strcmp(toggle, "enable") || !strcmp(toggle, "default"))
        LIST_FOR_EACH_ENTRY(warning, wnum, warning_t, entry)
            warning_enable(warning->num);
    else
        ret = 0;

    LIST_FOR_EACH_ENTRY_SAFE(warning, next, wnum, warning_t, entry)
        free(warning);
    return ret;
}

int is_warning_enabled(int warning)
{
    warning_t *warning_entry;
    if(!disabled_warnings)
        return 1;
    LIST_FOR_EACH_ENTRY(warning_entry, disabled_warnings, warning_t, entry)
        if(warning_entry->num == warning)
            return 0;
    return 1;
}
