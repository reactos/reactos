// sdkparse.cpp

#ifdef _MSC_VER
#pragma warning ( disable : 4786 )
#endif//_MSC_VER

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <string>
#include <vector>
#include <conio.h>

#include "EnumFilesImpl.h"

#include "assert.h"
#include "File.h"
#include "binary2cstr.h"
#include "strip_comments.h"
#include "tokenize.h"
#include "skip_ws.h"
#include "iskeyword.h"
#include "Type.h"
#include "Header.h"

#define TOKASSERT(x) \
if(!(x))\
{\
	printf("ASSERT FAILURE: (%s) at %s:%i\n", #x, __FILE__, __LINE__);\
	printf("WHILE PROCESSING: \n");\
	for ( int ajf83pfj = 0; ajf83pfj < tokens.size(); ajf83pfj++ )\
		printf("%s ", tokens[ajf83pfj].c_str() );\
	printf("\n");\
	_CrtDbgBreak();\
}
using std::string;
using std::vector;

vector<Header*> headers;

bool import_file ( const char* filename );
char* findend ( char* p, bool& externc );
Type identify ( const vector<string>& tokens, int off = 0 );
Type process ( const string& element, vector<string>& names, bool& isTypedef, vector<string>& dependencies );
void process_preprocessor ( const char* filename, Header& h, const string& element );
void process_c ( Header& h, const string& element );
int parse_type ( Type t, const vector<string>& tokens, int off, vector<string>& names, vector<string>& dependencies );
int parse_tident ( const vector<string>& tokens, int off, vector<string>& names, vector<string>& dependencies );
int parse_variable ( const vector<string>& tokens, int off, vector<string>& names, vector<string>& dependencies );
int parse_struct ( const vector<string>& tokens, int off, vector<string>& names, vector<string>& dependencies );
int parse_function ( const vector<string>& tokens, int off, vector<string>& names, vector<string>& dependencies );
int parse_function_ptr ( const vector<string>& tokens, int off, vector<string>& names, vector<string>& dependencies );
int parse_ifwhile ( const vector<string>& tokens, int off, vector<string>& names, vector<string>& dependencies );
int parse_do ( const vector<string>& tokens, int off, vector<string>& names, vector<string>& dependencies );

bool is_libc_include ( const string& inc )
{
	string s ( inc );
	strlwr ( &s[0] );
	if ( s == "basetsd.h" )
		return true;
	if ( s == "except.h" )
		return true;
	if ( s == "limits.h" )
		return true;
	if ( s == "stdarg.h" )
		return true;
	return false;
}

BOOL FileEnumProc ( PWIN32_FIND_DATA pwfd, const char* filename, long lParam )
{
	if ( !is_libc_include ( filename ) )
		import_file ( filename );
	return TRUE;
}

void main()
{
	//import_file ( "test.h" );
	EnumFilesInDirectory ( "c:/cvs/reactos/apps/utils/sdkparse/include", "*.h", FileEnumProc, 0, TRUE, FALSE );
}

bool import_file ( const char* filename )
{
	int i;

	for ( i = 0; i < headers.size(); i++ )
	{
		if ( headers[i]->filename == filename )
			return true;
	}

	string s;
	if ( !File::LoadIntoString ( s, filename ) )
	{
		printf ( "Couldn't load \"%s\" for input.\n", filename );
		ASSERT(0);
	}

	printf ( "%s\n", filename );

	// strip comments from the file...
	strip_comments ( s, true );

	/*{
		string no_comments ( filename );
		no_comments += ".nocom.txt";
		File::SaveFromString ( s, no_comments );
	}*/

	Header* h = new Header ( filename );
	headers.push_back ( h );

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
			char* end = strchr ( p, '\n' );
			while ( end && end[-1] == '\\' )
				end = strchr ( end+1, '\n' );
			if ( !end )
				end = p + strlen(p);
			string element ( p, end-p );

			process_preprocessor ( filename, *h, element );

			p = end;
		}
		else if ( *p == '}' && h->externc )
		{
			p++;
			p = skip_ws ( p );

			if ( *p == ';' ) p++;
		}
		else
		{
			bool externc = false;
			char* end = findend ( p, externc );
			ASSERT(end);
			if ( externc )
				h->externc = true;
			else
			{
				string element ( p, end-p );

				process_c ( *h, element );
			}
			p = end;
		}
	}
	h->done = true;
	return true;
}

string get_hdrguardtext ( const char* filename )
{
	string s ( filename );
	char* p = &s[0];
	char* p2;
	while ( (p2 = strchr(p, '\\')) )
		*p2 = '/';
	while ( (p2 = strchr(p,'/')) )
		p = p2 + 1;
	char* end = strchr ( p, '.' );
	ASSERT(end);
	while ( (p2 = strchr(end+1,'.')) )
		end = p2;
	string hdrguardtext ( p, end-p );
	strupr ( &hdrguardtext[0] );
	return hdrguardtext;
}

void process_preprocessor ( const char* filename, Header& h, const string& element )
{
	string hdrguardtext ( get_hdrguardtext ( filename ) );

	const char* p = &element[0];
	ASSERT ( *p == '#' );
	p++;
	p = skip_ws ( p );
	const char* end = p;
	while ( iscsym(*end) )
		end++;
	string preproc ( p, end-p );
	p = end+1;
	p = skip_ws ( p );

	if ( preproc == "include" )
	{
		ASSERT ( *p == '<' || *p == '\"' );
		p++;
		p = skip_ws ( p );
		const char* end = strpbrk ( p, ">\"" );
		if ( !end )
			end = p + strlen(p);
		while ( end > p && isspace(end[-1]) )
			end--;
		string include_filename ( p, end-p );
		if ( is_libc_include ( include_filename ) )
			h.libc_includes.push_back ( include_filename );
		else
		{
			bool loaded = false;
			for ( int i = 0; i < headers.size(); i++ )
			{
				if ( headers[i]->filename == include_filename )
				{
					if ( !headers[i]->done )
					{
						printf ( "circular dependency between '%s' and '%s'\n", filename, include_filename.c_str() );
						ASSERT ( 0 );
					}
					loaded = true;
				}
			}
			if ( !loaded )
			{
				printf ( "(diverting to '%s')\n", include_filename.c_str() );
				import_file ( include_filename.c_str() );
				printf ( "(now back to '%s')\n", filename );
			}
			h.includes.push_back ( include_filename );
		}
	}
	else if ( preproc == "define" )
	{
		size_t len = element.size();
		if ( strstr ( element.c_str(), hdrguardtext.c_str() )
			&& element[len-2] == '_'
			&& element[len-1] == 'H' )
		{
			// header include guard... ignore!
			return;
		}
		Symbol *s = new Symbol;
		s->type = T_DEFINE;

		p += 6;
		p = skip_ws ( p );

		const char* end = p;
		while ( iscsym(*end) )
			end++;

		s->names.push_back ( string(p,end-p) );

		s->definition = element;

		h.symbols.push_back ( s );
	}
	else if ( preproc == "if" || preproc == "ifdef" || preproc == "ifndef" )
	{
		size_t len = element.size();
		// check for header include guard...
		if ( strstr ( element.c_str(), hdrguardtext.c_str() )
			&& element[len-2] == '_'
			&& element[len-1] == 'H' )
			h.ifs.push_back ( string("") );
		else
			h.ifs.push_back ( element );
		h.ifspreproc.push_back ( preproc );
	}
	else if ( preproc == "endif" )
	{
		h.ifs.pop_back();
		h.ifspreproc.pop_back();
	}
	else if ( preproc == "else" )
	{
		string& oldpre = h.ifspreproc.back();
		ASSERT ( oldpre != "else" );
		if ( oldpre == "ifdef" )
			h.ifs.back() = "ifndef";
		else if ( oldpre == "ifndef" )
			h.ifs.back() = "ifdef";
		else if ( oldpre == "if" )
			h.ifs.back() = string("!(") + h.ifs.back() + ")";
		else
		{
			printf ( "unrecognized preproc '%s'\n", oldpre.c_str() );
			ASSERT(0);
			return;
		}
		oldpre = "else";
	}
	else if ( preproc == "include_next" )
	{
		// we can safely ignore this command...
	}
	else if ( preproc == "pragma" )
	{
		h.pragmas.push_back ( element );
	}
	else
	{
		printf ( "process_preprocessor() choked on '%s'\n", preproc.c_str() );
	}
}

void process_c ( Header& h, const string& element )
{
	//printf ( "\"%s\"\n\n", binary2cstr(element).c_str() );

	bool isTypedef;

	Symbol *s = new Symbol;
	s->definition = element;
	s->type = process ( element, s->names, isTypedef, s->dependencies );
	
	for ( int i = 0; i < h.ifs.size(); i++ )
	{
		if ( h.ifs[i].size() )
			s->ifs.push_back ( h.ifs[i] );
	}

	/*printf ( "names: " );
	if ( s->names.size() )
	{
		printf ( "%s", s->names[0].c_str() );
		for ( int i = 1; i < s->names.size(); i++ )
			printf ( ", %s", s->names[i].c_str() );
	}
	else
		printf ( "(none)" );
	printf ( "\n\n" );

	printf ( "dependencies: " );
	if ( s->dependencies.size() )
	{
		printf ( "%s", s->dependencies[0].c_str() );
		for ( int i = 1; i < s->dependencies.size(); i++ )
			printf ( ", %s", s->dependencies[i].c_str() );
	}
	else
		printf ( "(none)" );
	printf ( "\n\n" );*/

	h.symbols.push_back ( s );
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

char* findend ( char* p, bool& externc )
{
	if ( !strncmp ( p, "static inline struct _TEB * NtCurrentTeb", 40 ) )
		_CrtDbgBreak();
	// special-case for 'extern "C"'
	if ( !strncmp ( p, "extern", 6 ) )
	{
		char* p2 = p + 6;
		p2 = skip_ws ( p2 );
		if ( !strncmp ( p2, "\"C\"", 3 ) )
		{
			p2 += 3;
			p2 = skip_ws ( p2 );
			if ( *p2 == '{' )
			{
				externc = true;
				return p2+1;
			}
		}
	}
	// special-case for 'typedef_tident'
	if ( !strncmp ( p, "typedef_tident", 14 ) )
	{
		char* end = strchr ( p, ')' );
		ASSERT(end);
		return end+1;
	}
	externc = false;
	bool isStruct = false;

	char* end = strchr ( p, ';' );
	if ( !end )
		end = p + strlen(p);
	else
		end++;
	char* semi = strchr ( p, '{' );
	if ( !semi || semi > end )
		return end;
	end = skipsemi ( semi );

	const char* structs[] = { "struct", "enum", "class", "union" };
	for ( int i = 0; i < sizeof(structs)/sizeof(structs[0]); i++ )
	{
		char* pStruct = strstr ( p, structs[i] );
		if ( pStruct
			&& pStruct < semi
			&& !__iscsym(pStruct[-1])
			&& !__iscsym(pStruct[strlen(structs[i])]) )
		{
			isStruct = true;
			break;
		}
	}

	if ( isStruct )
	{
		end = strchr ( end, ';' );
		if ( !end )
			end = p + strlen(p);
		else
			end++;
	}
	else
	{
		char* p2 = skip_ws ( end );
		if ( *p2 == ';' )
			end = p2 + 1;
	}
	return end;
}

Type identify ( const vector<string>& tokens, int off )
{
	/*if ( tokens.size() >= 3 )
	{
		if ( tokens[off+2] == "_OBJECTS_AND_SID" )
			_CrtDbgBreak();
	}*/
	if ( tokens[off] == "typedef_tident" )
		return T_TIDENT;
	else if ( tokens[off] == "if" )
		return T_IF;
	else if ( tokens[off] == "while" )
		return T_WHILE;
	else if ( tokens[off] == "do" )
		return T_DO;
	int parens = 0;
	int brackets = 0;
	for ( int i = off; i < tokens.size(); i++ )
	{
		if ( tokens[i] == "(" && !brackets )
			parens++;
		else if ( tokens[i] == "{" )
			brackets++;
		else if ( (tokens[i] == "struct" || tokens[i] == "union") && !parens )
		{
			for ( int j = i + 1; j < tokens.size(); j++ )
			{
				if ( tokens[j] == "{" )
					return T_STRUCT;
				else if ( tokens[j] == "(" || tokens[j] == ";" || tokens[j] == "*" )
					break;
			}
		}
		else if ( tokens[i] == ";" )
			break;
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
	case T_TIDENT:
		return parse_tident ( tokens, off, names, dependencies );
	case T_VARIABLE:
		return parse_variable ( tokens, off, names, dependencies );
	case T_STRUCT:
		return parse_struct ( tokens, off, names, dependencies );
	case T_FUNCTION:
		return parse_function ( tokens, off, names, dependencies );
	case T_FUNCTION_PTR:
		return parse_function_ptr ( tokens, off, names, dependencies );
	case T_IF:
	case T_WHILE:
		return parse_ifwhile ( tokens, off, names, dependencies );
	case T_DO:
		return parse_do ( tokens, off, names, dependencies );
	default:
		TOKASSERT(!"unidentified type in parse_type()");
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

int parse_tident ( const vector<string>& tokens, int off, vector<string>& names, vector<string>& dependencies )
{
	TOKASSERT ( tokens[off] == "typedef_tident" );
	TOKASSERT ( tokens[off+1] == "(" && tokens[off+3] == ")" );
	names.push_back ( tokens[off+2] );
	dependencies.push_back ( "typedef_tident" );
	return off + 4;
}

int parse_variable ( const vector<string>& tokens, int off, vector<string>& names, vector<string>& dependencies )
{
	// NOTE - Test with bitfields, I think this code will actually handle them properly...
	depend ( tokens[off++], dependencies );
	int done = tokens.size();
	while ( tokens[off] != ";" )
		name ( tokens[off++], names );
	TOKASSERT ( tokens[off] == ";" );
	off++;
	return off;
}

int parse_struct ( const vector<string>& tokens, int off, vector<string>& names, vector<string>& dependencies )
{
	int done = tokens.size();

	//if ( tokens[off+1] == "_LARGE_INTEGER" )
	//	_CrtDbgBreak();

	while ( off < done && tokens[off] != "struct" && tokens[off] != "union" )
		depend ( tokens[off++], dependencies );

	TOKASSERT ( tokens[off] == "struct" || tokens[off] == "union" );
	if ( tokens[off] != "struct" && tokens[off] != "union" )
		return off;
	off++;

	if ( tokens[off] != "{" )
		name ( tokens[off++], names );

	TOKASSERT ( tokens[off] == "{" );
	off++;

	// skip through body of struct - noting any dependencies
	int indent = 1;
	//if ( off >= done ) _CrtDbgBreak();
	while ( off < done && tokens[off] != "}" )
	{
		vector<string> fauxnames;
		Type t = identify ( tokens, off );
		off = parse_type ( t, tokens, off, fauxnames, dependencies );
		//if ( off >= done ) _CrtDbgBreak();
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

	TOKASSERT ( tokens[off] == ";" );
	off++;

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
	vector<string> fauxnames;

	while ( tokens[off+1] != "(" )
		depend ( tokens[off++], dependencies );
	name ( tokens[off++], names );

	TOKASSERT ( tokens[off] == "(" );

	while ( tokens[off] != ")" )
	{
		off++;
		off = parse_param ( tokens, off, fauxnames, dependencies );
		TOKASSERT ( tokens[off] == "," || tokens[off] == ")" );
	}

	off++;

	// is this just a function *declaration* ?
	if ( tokens[off] == ";" )
		return off;

	// we have a function body...
	TOKASSERT ( tokens[off] == "{" );
	off++;

	while ( tokens[off] != "}" )
	{
		Type t = identify ( tokens, off );
		off = parse_type ( t, tokens, off, fauxnames, dependencies );
	}

	TOKASSERT ( tokens[off] == "}" );
	off++;

	return off;
}

int parse_function_ptr ( const vector<string>& tokens, int off, vector<string>& names, vector<string>& dependencies )
{
	while ( tokens[off] != "(" )
		depend ( tokens[off++], dependencies );

	TOKASSERT ( tokens[off] == "(" );
	off++;

	while ( tokens[off+1] != ")" )
		depend ( tokens[off++], dependencies );
	name ( tokens[off++], names );

	TOKASSERT ( tokens[off] == ")" );
	
	off++;

	TOKASSERT ( tokens[off] == "(" );

	while ( tokens[off] != ")" )
	{
		off++;
		vector<string> fauxnames;
		off = parse_param ( tokens, off, fauxnames, dependencies );
		TOKASSERT ( tokens[off] == "," || tokens[off] == ")" );
	}

	off++;
	TOKASSERT ( tokens[off] == ";" );
	off++;
	return off;
}

int parse_ifwhile ( const vector<string>& tokens, int off, vector<string>& names, vector<string>& dependencies )
{
	TOKASSERT ( tokens[off] == "if" || tokens[off] == "while" );
	off++;

	TOKASSERT ( tokens[off] == "(" );
	off++;

	TOKASSERT ( tokens[off] != ")" );
	while ( tokens[off] != ")" )
		off++;

	if ( tokens[off] == "{" )
	{
		while ( tokens[off] != "}" )
		{
			Type t = identify ( tokens, off );
			off = parse_type ( t, tokens, off, names, dependencies );
		}
		off++;
	}
	return off;
}

int parse_do ( const vector<string>& tokens, int off, vector<string>& names, vector<string>& dependencies )
{
	TOKASSERT ( tokens[off] == "do" );
	off++;

	if ( tokens[off] != "{" )
	{
		Type t = identify ( tokens, off );
		off = parse_type ( t, tokens, off, names, dependencies );
	}
	else
	{
		while ( tokens[off] != "}" )
		{
			Type t = identify ( tokens, off );
			off = parse_type ( t, tokens, off, names, dependencies );
		}
	}

	TOKASSERT ( tokens[off] == "while" );
	off++;

	TOKASSERT ( tokens[off] == "(" );
	while ( tokens[off] != ")" )
		off++;

	TOKASSERT ( tokens[off] == ")" );
	off++;

	TOKASSERT ( tokens[off] == ";" );
	off++;

	return off;
}

