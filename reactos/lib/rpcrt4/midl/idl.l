%{
#include "midl.h"
#include "idl.tab.h"
#include <string.h>

int lineno = 1;
void yyerror(char* s);
char* current_file;
int nr_errors = 0;

#define TOK(n) { yylval.token = n; return n; }

%}

DIGIT [0-9]
ID    [a-zA-Z_][a-zA-Z0-9_]*
UUID  [a-z0-9]*

%%

uuid                { TOK(UUID_KEYWORD); }
version             { TOK(VERSION_KEYWORD); }
pointer_default     { TOK(POINTER_DEFAULT_KEYWORD); }
unique              { TOK(UNIQUE_KEYWORD); }
interface           { TOK(INTERFACE_KEYWORD); }
aggregatable        { TOK(AGGREGATABLE_KEYWORD); }
allocate            { TOK(ALLOCATE_KEYWORD); }
appobject           { TOK(APPOBJECT_KEYWORD); }


all_nodes           { TOK(ALL_NODES_KEYWORD); }
single_node         { TOK(SINGLE_NODE_KEYWORD); }
free                { TOK(FREE_KEYWORD); }
dont_free           { TOK(DONT_FREE_KEYWORD);  }

typedef             { TOK(TYPEDEF_KEYWORD); }
struct              { TOK(STRUCT_KEYWORD); }
const               { TOK(CONST_KEYWORD); }

signed              { TOK(SIGNED_KEYWORD); }
unsigned            { TOK(UNSIGNED_KEYWORD); }

in                  { TOK(IN_KEYWORD); }
out                 { TOK(OUT_KEYWORD); }
string              { TOK(STRING_KEYWORD); }
implicit_handle     { TOK(IMPLICIT_HANDLE_KEYWORD); }
auto_handle         { TOK(AUTO_HANDLE_KEYWORD); }
size_is             { TOK(SIZE_IS_KEYWORD); }
length_is           { TOK(LENGTH_IS_KEYWORD); }

{ID}                { int n;
                      if ((n = token_to_type(yytext)) != 0)
		      {
		          yylval.tval = n;
			  return(TYPE_KEYWORD);
		      }
		      else
		      {
		          yylval.id = strdup(yytext); 
			  return(ID_TOKEN); 
		      }
		     }

{DIGIT}+                   { yylval.number = atoi(yytext); 
                             return(NUMBER_TOKEN); }

{DIGIT}+"."{DIGIT}+        { char* ebuf;
                             yylval.version.major = strtoul(yytext, &ebuf, 0);
			     yylval.version.minor = strtoul(ebuf + 1, NULL, 0);
			     return(VERSION_TOKEN); }

{UUID}+"-"{UUID}+"-"{UUID}+"-"{UUID}+"-"{UUID}+ { yylval.uuid = strdup(yytext);
                                                  return(UUID_TOKEN); }

";"                                { TOK(LINE_TERMINATOR); }

"("                                { TOK(LEFT_BRACKET); }

")"                                { TOK(RIGHT_BRACKET); }

","                                { TOK(COMMA); }

"{"                                { TOK(LCURLY_BRACKET); }

"}"                                { TOK(RCURLY_BRACKET); }

"["                                { TOK(LSQBRACKET); }

"]"                                { TOK(RSQBRACKET); }

"*"                                { TOK(STAR); }

"="                                { TOK(ASSIGNMENT); }

\n                       lineno++;

[ \t\r]+                 ;

.                        yyerror("invalid character");

%%

void yyerror(char* s)
{
     nr_errors++;
     printf("%s:%d: %s at %s\n", current_file, lineno, s, yytext);
}

void yyerror1(char* s)
{
     nr_errors++;
     printf("%s:%d: %s\n", current_file, lineno, s);
}

