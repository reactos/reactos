// sdkparse.cpp

#ifdef _MSC_VER
#pragma warning ( disable : 4786 )
#endif//_MSC_VER

#include <string>
#include <vector>
#include <conio.h>

#include "assert.h"
#include "File.h"
#include "binary2cstr.h"
#include "strip_comments.h"
#include "tokenize.h"
#include "skip_ws.h"
#include "iskeyword.h"

using std::string;
using std::vector;

typedef enum
{
	T_UNKNOWN = -1,
	T_MACRO,
	T_DEFINE,
	T_VARIABLE,
	T_FUNCTION,
	T_FUNCTION_PTR,
	T_STRUCT
} Type;

bool import_file ( const char* filename );
char* findend ( char* p );
Type identify ( const vector<string>& tokens, int off = 0 );
Type process ( const string& element, vector<string>& names, bool& isTypedef, vector<string>& dependencies );
int parse_type ( Type t, const vector<string>& tokens, int off, vector<string>& names, vector<string>& dependencies );
int parse_variable ( const vector<string>& tokens, int off, vector<string>& names, vector<string>& dependencies );
int parse_struct ( const vector<string>& tokens, int off, vector<string>& names, vector<string>& dependencies );
int parse_function ( const vector<string>& tokens, int off, vector<string>& names, vector<string>& dependencies );
int parse_function_ptr ( const vector<string>& tokens, int off, vector<string>& names, vector<string>& dependencies );

#ifndef ASSERT
#define ASSERT(x) \
do \
{ \
	if ( !(x) ) \
	{ \
		printf("%s:%i - ASSERTION FAILURE: \"%s\"\n", __FILE__, __LINE__, #x); \
		getch(); \
		exit(0); \
	} \
}while(0)
#endif//ASSERT

void main()
{
	import_file ( "test.h" );
}

bool import_file ( const char* filename )
{
	string s;
	if ( !File::LoadIntoString ( s, filename ) )
	{
		printf ( "Couldn't load \"%s\" for input.\n", filename );
		return false;
	}

	// strip comments from the file...
	strip_comments ( s, true );

	/*{
		string no_comments ( filename );
		no_comments += ".nocom.txt";
		File::SaveFromString ( s, no_comments );
	}*/

	char* p = &s[0];
	while ( p )
	{
		// skip whitespace
		p = skip_ws ( p );
		if ( !*p )
			break;
		// check for pre-processor command
		if ( *p == '#' )
		{
			p = strchr ( p, '\n' );
			if ( p )
				p++;
		}
		else
		{
			char* end = findend ( p );
			if ( !end )
				end = p + strlen(p);
			else if ( *end )
				end++;
			string element ( p, end-p );
			p = end;

			printf ( "\"%s\"\n\n", binary2cstr(element).c_str() );

			vector<string> names, dependencies;
			bool isTypedef;
			Type t = process ( element, names, isTypedef, dependencies );

			printf ( "names: " );
			if ( names.size() )
			{
				printf ( "%s", names[0].c_str() );
				for ( int i = 1; i < names.size(); i++ )
					printf ( ", %s", names[i].c_str() );
			}
			else
				printf ( "(none)" );
			printf ( "\n\n" );

			printf ( "dependencies: " );
			if ( dependencies.size() )
			{
				printf ( "%s", dependencies[0].c_str() );
				for ( int i = 1; i < dependencies.size(); i++ )
					printf ( ", %s", dependencies[i].c_str() );
			}
			else
				printf ( "(none)" );
			printf ( "\n\n" );
		}
	}
	return true;
}

char* skipsemi ( char* p )
{
	if ( *p != '{' ) // }
	{
		ASSERT(0);
	}
	p++;
	for ( ;; )
	{
		char* s = strchr ( p, '{' );
		char* e = strchr ( p, '}' );
		if ( !e )
			e = p + strlen(p);
		if ( !s || s > e )
		{
			// make sure we don't return pointer past null
			if ( *e )
				return e + 1;
			else
				return e;
		}
		p = skipsemi ( s );
	}
}

char* findend ( char* p )
{
	for ( ;; )
	{
		char* end = strchr ( p, ';' );
		if ( !end )
			end = p + strlen(p);
		char* semi = strchr ( p, '{' );
		if ( !semi || semi > end )
			return end;
		p = skipsemi ( semi );
	}
}

Type identify ( const vector<string>& tokens, int off )
{
	int parens = 0;
	for ( int i = off; i < tokens.size(); i++ )
	{
		if ( tokens[i] == "(" )
			parens++;
		else if ( tokens[i] == "struct" && !parens )
			return T_STRUCT;
	}
	if ( parens > 1 )
		return T_FUNCTION_PTR;
	else if ( parens == 1 )
		return T_FUNCTION;
	return T_VARIABLE;
}

Type process ( const string& element, vector<string>& names, bool& isTypedef, vector<string>& dependencies )
{
	names.resize ( 0 );
	isTypedef = false;
	dependencies.resize ( 0 );

	vector<string> tokens;

	tokenize ( element, tokens );

	// now let's do the classification...
	int i = 0;
	if ( tokens[i] == "typedef" )
	{
		isTypedef = true;
		i++;
	}

	Type t = identify ( tokens, i );

	parse_type ( t, tokens, i, names, dependencies );

	return t;
}

int parse_type ( Type t, const vector<string>& tokens, int off, vector<string>& names, vector<string>& dependencies )
{
	switch ( t )
	{
	case T_VARIABLE:
		return parse_variable ( tokens, off, names, dependencies );
	case T_STRUCT:
		return parse_struct ( tokens, off, names, dependencies );
	case T_FUNCTION:
		return parse_function ( tokens, off, names, dependencies );
	case T_FUNCTION_PTR:
		return parse_function_ptr ( tokens, off, names, dependencies );
	default:
		ASSERT(0);
		return 0;
	}
}

void name ( const string& ident, vector<string>& names )
{
	if ( !__iscsymf ( ident[0] ) )
		return;
	if ( iskeyword ( ident ) )
		return;
	for ( int i = 0; i < names.size(); i++ )
	{
		if ( names[i] == ident )
			return;
	}
	names.push_back ( ident );
}

void depend ( const string& ident, vector<string>& dependencies )
{
	if ( !__iscsymf ( ident[0] ) )
		return;
	if ( iskeyword ( ident ) )
		return;
	for ( int i = 0; i < dependencies.size(); i++ )
	{
		if ( dependencies[i] == ident )
			return;
	}
	dependencies.push_back ( ident );
}

int parse_variable ( const vector<string>& tokens, int off, vector<string>& names, vector<string>& dependencies )
{
	// FIXME - handle bitfields properly
	depend ( tokens[off++], dependencies );
	int done = tokens.size();
	while ( off < done && tokens[off] != ";" )
		name ( tokens[off++], names );
	if ( off < done )
		return off + 1;
	else
		return off;
}

int parse_struct ( const vector<string>& tokens, int off, vector<string>& names, vector<string>& dependencies )
{
	int done = tokens.size();

	while ( off < done && tokens[off] != "struct" )
		depend ( tokens[off++], dependencies );

	ASSERT ( tokens[off] == "struct" );
	if ( tokens[off] != "struct" )
		return off;
	off++;

	if ( tokens[off] != "{" )
		name ( tokens[off++], names );

	ASSERT ( tokens[off] == "{" );
	off++;

	// skip through body of struct - noting any dependencies
	int indent = 1;
	while ( tokens[off] != "}" )
	{
		vector<string> fauxnames;
		Type t = identify ( tokens, off );
		off = parse_type ( t, tokens, off, fauxnames, dependencies );
	}
	
	// process any trailing dependencies/names...
	while ( tokens[off] != ";" )
	{
		if ( tokens[off+1] == "," || tokens[off+1] == ";" )
			name ( tokens[off], names );
		else
			depend ( tokens[off], dependencies );
		off++;
	}

	return off;
}

int parse_param ( const vector<string>& tokens, int off, vector<string>& names, vector<string>& dependencies )
{
	while ( tokens[off+1] != "," && tokens[off+1] != ")" )
		depend ( tokens[off++], dependencies );
	name ( tokens[off++], names );
	return off;
}

int parse_function ( const vector<string>& tokens, int off, vector<string>& names, vector<string>& dependencies )
{
	while ( tokens[off+1] != "(" )
		depend ( tokens[off++], dependencies );
	name ( tokens[off++], names );

	ASSERT ( tokens[off] == "(" );

	while ( tokens[off] != ")" )
	{
		off++;
		vector<string> fauxnames;
		off = parse_param ( tokens, off, fauxnames, dependencies );
		ASSERT ( tokens[off] == "," || tokens[off] == ")" );
	}

	off++;

	ASSERT ( tokens[off] == ";" );
	return off;
}

int parse_function_ptr ( const vector<string>& tokens, int off, vector<string>& names, vector<string>& dependencies )
{
	while ( tokens[off] != "(" )
		depend ( tokens[off++], dependencies );

	ASSERT ( tokens[off] == "(" );
	off++;

	while ( tokens[off+1] != ")" )
		depend ( tokens[off++], dependencies );
	name ( tokens[off++], names );

	ASSERT ( tokens[off] == ")" );
	
	off++;

	ASSERT ( tokens[off] == "(" );

	while ( tokens[off] != ")" )
	{
		off++;
		vector<string> fauxnames;
		off = parse_param ( tokens, off, fauxnames, dependencies );
		ASSERT ( tokens[off] == "," || tokens[off] == ")" );
	}

	off++;
	ASSERT ( tokens[off] == ";" );
	return off;
}
