%{
#include "midl.h"
%}

%union {
int tval;
int token;
char* id;
int number;
char* uuid;
char operator;
char* string;
struct
{
int minor;
int major;
} version;
}

%token <id> ID_LITERAL
%token <uuid> UUID_LITERAL
%token NUMBER_LITERAL
%token <version> VERSION_LITERAL
%token STRING_LITERAL

%token ENDPOINT_KEYWORD
%token EXCEPTIONS_KEYWORD
%token LOCAL_KEYWORD
%token IMPORT_KEYWORD

%token UUID_KEYWORD
%token VERSION_KEYWORD
%token POINTER_DEFAULT_KEYWORD 
%token <token> UNIQUE_KEYWORD
%token INTERFACE_KEYWORD
%token IMPLICIT_HANDLE_KEYWORD
%token AUTO_HANDLE_KEYWORD
%token AGGREGATABLE_KEYWORD
%token ALLOCATE_KEYWORD
%token APPOBJECT_KEYWORD

%token ALL_NODES_KEYWORD
%token SINGLE_NODE_KEYWORD
%token FREE_KEYWORD
%token DONT_FREE_KEYWORD

%token TYPEDEF_KEYWORD
%token STRUCT_KEYWORD
%token CONST_KEYWORD

%token IN_KEYWORD
%token OUT_KEYWORD
%token STRING_KEYWORD
%token SIZE_IS_KEYWORD
%token LENGTH_IS_KEYWORD

%token UNSIGNED_KEYWORD
%token SIGNED_KEYWORD

%token LSQBRACKET, RSQBRACKET, LBRACKET, RBRACKET
%token LCURLY_BRACKET, RCURLY_BRACKET, LINE_TERMINATOR, COMMA
%token LEFT_BRACKET, RIGHT_BRACKET
%token STAR
%token ASSIGNMENT

%token <tval> TYPE_KEYWORD

%type <tval> type
%type <tval> sign
%type <tval> struct_def

%%

idl_file:
       LSQBRACKET options RSQBRACKET interface
       ;

options:
       option
     | option COMMA options
     ;

option:
       UUID_KEYWORD LEFT_BRACKET UUID_TOKEN RIGHT_BRACKET 
                                 { set_uuid($3); }
     | VERSION_KEYWORD LEFT_BRACKET VERSION_TOKEN RIGHT_BRACKET
                            { set_version($3.major, $3.minor); }
     | ENDPOINT_KEYWORD LEFT_BRACKET port_specs RIGHT_BRACKET
     | EXCEPTIONS_KEYWORD LEFT_BRACKET excep_names RIGHT_BRACKET
     | LOCAL_KEYWORD
     | POINTER_DEFAULT_KEYWORD LEFT_BRACKET UNIQUE_KEYWORD RIGHT_BRACKET
                        { set_pointer_default($3); }
     ;

port_specs:
     | STRING_TOKEN COMMA port_specs
     ;

excep_names: ID_TOKEN             { }
     | ID_TOKEN COMMA excep_names { }
     ;

interface: { start_interface(); } 
           INTERFACE_KEYWORD ID_TOKEN LCURLY_BRACKET interface_components 
	   RCURLY_BRACKET
           { end_interface($3); }

interface_components: 
        | interface_component LINE_TERMINATOR interface_components

interface_component:
        | IMPORT_KEYWORD import_list
	| function
	| TYPEDEF_KEYWORD typedef
	| CONST_KEYWORD type ID_TOKEN ASSIGNMENT const_expr
	| STRUCT_KEYWORD struct_def RCURLY_BRACKET 
	;

import_list: STRING_TOKEN
          |  STRING_TOKEN COMMA import_list
	  ;

const_expr: NUMBER_TOKEN 
          | STRING_TOKEN
	  ;



typedef: type ID_TOKEN { add_typedef($2, $1); };

function: { start_function(); }
          type ID_TOKEN LEFT_BRACKET argument_list RIGHT_BRACKET  
          { end_function($2, $3); }
       ;

argument_list:                    
      | TYPE_KEYWORD           { if ($1 != VOID_TYPE) 
                                 {
				   yyerror("parameter name ommitted");
				 }
			        }
      | argument                   
      | argument COMMA argument_list
      ;

argument: arg_attrs type ID_TOKEN { add_argument($2, $3); }
       |  type ID_TOKEN           { add_argument($1, $2); }
       ;

type:  sign TYPE_KEYWORD STAR     { $$ = $2 | POINTER_TYPE_OPTION | $1; }
     | sign TYPE_KEYWORD          { $$ = $2 | $1; }
     | TYPE_KEYWORD               { $$ = $1; }
     | TYPE_KEYWORD STAR          { $$ = $1 | POINTER_TYPE_OPTION; }
     | STRUCT_KEYWORD struct_def RCURLY_BRACKET { $$ = $2; }
     | STRUCT_KEYWORD ID_TOKEN    { $$ = struct_to_type($2); }
     ;

struct_def: ID_TOKEN { start_struct($1); } LCURLY_BRACKET 
          struct_members { $$ = end_struct(); } 
       	| { start_struct(NULL); } LCURLY_BRACKET
          struct_members { $$ = end_struct(); } 
       ;

struct_members: 
           | type ID_TOKEN LINE_TERMINATOR struct_members 
	     { add_struct_member($2, $1); }
	   ;
       
/*
 * Rules for the optional sign for an integer type
 */
sign: UNSIGNED_KEYWORD            { $$ = UNSIGNED_TYPE_OPTION; }
    | SIGNED_KEYWORD              { $$ = SIGNED_TYPE_OPTION; }
	  ;
	  
arg_attrs: LSQBRACKET arg_attr_list RSQBRACKET
       ;
       
/*
 * Rules for the list of attributes for arguments
 */
arg_attr_list:  arg_attr
       | arg_attr COMMA arg_attr_list
       ;

/*
 * Rules for the various attributes for arguments
 */
arg_attr: IN_KEYWORD
        | OUT_KEYWORD
	| STRING_KEYWORD
	| LENGTH_IS_KEYWORD BRACKETED_QUANTITY	
	| SIZE_IS_KEYWORD BRACKETED_QUANTITY
	;

/*
 * 
 */
BRACKETED_QUANTITY: LEFT_BRACKET NUMBER_TOKEN RIGHT_BRACKET
                  | LEFT_BRACKET ID_TOKEN RIGHT_BRACKET
		  ;
