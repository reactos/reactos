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
struct
{
int minor;
int major;
} version;
}

%token <id> ID_TOKEN
%token <uuid> UUID_TOKEN
%token NUMBER_TOKEN
%token <version> VERSION_TOKEN

%token UUID_KEYWORD
%token VERSION_KEYWORD
%token POINTER_DEFAULT_KEYWORD 
%token <token> UNIQUE_KEYWORD
%token INTERFACE_KEYWORD

%token IN_KEYWORD, OUT_KEYWORD, 

%token LSQBRACKET, RSQBRACKET, LBRACKET, RBRACKET
%token LCURLY_BRACKET, RCURLY_BRACKET, LINE_TERMINATOR, COMMA
%token LEFT_BRACKET, RIGHT_BRACKET

%token <tval> TYPE_KEYWORD

%type <tval> type

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
     | POINTER_DEFAULT_KEYWORD LEFT_BRACKET UNIQUE_KEYWORD RIGHT_BRACKET
                                 { set_pointer_default($3); }
     ;

interface: { start_interface(); } 
           INTERFACE_KEYWORD ID_TOKEN LCURLY_BRACKET functions RCURLY_BRACKET
           { end_interface($3); }

functions:                                               
	| function LINE_TERMINATOR functions
       ;

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

argument:
       LSQBRACKET direction RSQBRACKET type ID_TOKEN { add_argument($4, $5); }
       ;

type:  TYPE_KEYWORD;

direction:
        IN_KEYWORD
	;
