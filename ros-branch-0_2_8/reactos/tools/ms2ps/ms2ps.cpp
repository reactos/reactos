/*
 *  ReactOS kernel
 *  Copyright (C) 2004 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id$
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS seh conversion tool
 * FILE:            tools/ms2ps/ms2ps.cpp
 * PURPOSE:         Conversion tool from msvc to pseh style seh
 * PROGRAMMER:      Art Yerkes
 */

#include <iostream>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <deque>
#include <ctype.h>

#define MAYBE(x)

using std::string;
using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using std::istream;

typedef std::list<std::string> sl_t;
typedef sl_t::iterator sl_it;

string TRY_TOKEN = "__try";
string EXCEPT_TOKEN = "__except";
string FINALLY_TOKEN = "__finally";
char *c_operators[] = {
    "{",
    "}",
    "(",
    ")",
    NULL
};

int isident( int c ) {
    return (c != '{') && (c != '}') && (c != '(') && (c != ')') &&
	(c != '\'') && (c != '\"') && !isspace(c);
}

bool areinclass( string str, int (*isclass)(int) ) {
    int i;

    for( i = 0; i < (int)str.size(); i++ ) 
	if( !isclass( str[i] ) ) return false;

    return true;
}

bool areident( string s ) { return areinclass( s, isident ); }

bool isop( string tok ) {
    int i;
    for( i = 0; c_operators[i] && tok != c_operators[i]; i++ );
    if( c_operators[i] ) return true; else return false;
}

enum fstate { EMPTY, INT, FRAC, EXP };

string generic_match( string tok, string la,
			   bool (*mf)( string ) ) {
    if( tok.size() < 1 ) return "";
    if( mf(tok) && !mf(la) ) return tok; else return "";
}

string match_operator( string tok, string la ) {
    return generic_match( tok, la, isop );
}

string match_ident( string tok, string la ) {
    return generic_match( tok, la, areident );
}

string match_quoted_const( string tok, string la, char q ) {
    if( ((tok.size() && tok[0] == q) ||
	 ((tok.size() > 2) && tok[0] == 'L' && tok[1] == q)) &&
	(tok.rfind(q) == (tok.size() - 1)) ) {
	if( (tok.rfind("\\") != (tok.size() - 2)) ||
	    (tok.size() > 3 && tok.rfind("\\\\") == (tok.size() - 3)) )
	    return tok;
	else return "";
    }
    else return "";
}
    
sl_t snarf_tokens( string line ) {
    int i;
    sl_t out;
    string curtok, la, op, ident, qconst;

    line += " ";

    for( i = 0; i < (int)line.size() - 1; i++ ) {
	/*cout << "Char [" << line[i] << "] and [" << line[i+1] << "]"  
	  << endl; */

	if( (!curtok.size() ||
	     (curtok[0] != '\'' && curtok[0] != '\"')) &&
	    (curtok.size() <= 2 || 
	     curtok[0] != 'L' ||
	     (curtok[1] != '\'' && curtok[1] != '\"')) &&
	     isspace(line[i]) ) {
	    if( curtok.size() ) out.push_back( curtok );
	    curtok = "";
	    continue;
	}

	curtok.push_back( line[i] );

	la = curtok + line[i+1];

	op = match_operator( curtok, la );
	
	if( op != "" ) {
	    out.push_back( op MAYBE(+ "O") );
	    curtok = "";
	    continue;
	}

	if( la != "L\"" && la != "L\'" ) {
	    ident = match_ident( curtok, la );
	    
	    if( ident != "" ) {
		out.push_back( ident MAYBE(+ "I") );
		curtok = "";
		continue;
	    }
	}

	qconst = match_quoted_const( curtok, la, '\'' );
	
	if( qconst != "" ) {
	    out.push_back( qconst MAYBE(+ "q") );
	    curtok = "";
	    continue;
	}

	qconst = match_quoted_const( curtok, la, '\"' );
	
	if( qconst != "" ) {
	    out.push_back( qconst MAYBE(+ "Q") );
	    curtok = "";
	    continue;
	}
    }

    return out;
}

istream &getline_no_comments( istream &is, string &line ) {
    string buf;
    int ch;
    int seen_slash = false;

    while( (ch = is.get()) != -1 ) {
	if( seen_slash ) {
	    if( ch == '/' ) {
		do {
		    ch = is.get();
		} while( ch != -1 && ch != '\n' && ch != '\r' );
		break;
	    } else if( ch == '*' ) {
		ch = is.get(); /* Skip one char */
		do {
		    while( ch != '*' )
			ch = is.get();
		    ch = is.get();
		} while( ch != '/' );
		buf += ' ';
	    } else {
		buf += '/'; buf += (char)ch;
	    }
	    seen_slash = false;
	} else {
	    if( ch == '/' ) seen_slash = true;
	    else if( ch == '\r' || ch == '\n' ) break;
	    else buf += (char)ch;
	}
    }

    line = buf;

    return is;
}

bool expand_input( sl_t &tok ) {
    string line;
    sl_t new_tokens;
    bool out = false;

    out = getline_no_comments( cin, line );
    while( line.size() && isspace( line[0] ) )
	line = line.substr( 1 );
    if( line[0] == '#' ) {
	tok.push_back( line );
	while( line[line.size()-1] == '\\' ) {
	    getline_no_comments( cin, line );
	    tok.push_back( line );
	    tok.push_back( "\n" );
	}
	tok.push_back( "\n" );
    } else {
	new_tokens = snarf_tokens( line );
	tok.splice( tok.end(), new_tokens );
	tok.push_back( "\n" );
    }
    
    return out;
}

sl_it
complete_block( sl_it i,
		sl_it end,
		string start_ch, string end_ch) {
    int bc = 1;
    
    for( i++; i != end && bc; i++ ) {
	if( *i == start_ch ) bc++;
	if( *i == end_ch ) bc--;
    }

    return i;
}

string makename( string intro ) {
    static int i = 0;
    char buf[100];

    sprintf( buf, "%s%d", intro.c_str(), i++ );

    return buf;
}

void append_block( sl_t &t, sl_it b, sl_it e ) {
    while( b != e ) {
	t.push_back( *b );
	b++;
    }
}

void error( sl_t &container, sl_it it, string l ) {
    int line = 0; 
    for( sl_it i = container.begin(); i != it; i++ ) 
	if( (*i)[0] == '#' ) {
	    sscanf( i->substr(1).c_str(), "%d", &line );
	    cerr << "*standard-input*:" << line << ": " << l;
	}
}

/* Goal: match and transform one __try { a } __except [ (foo) ] { b } 
 * [ __finally { c } ]
 *
 * into
 *
 * _SEH_FINALLY(name1) { c }
 * _SEH_FILTER(name2) { return (foo | EXCEPTION_EXECUTE_HANDLER); }
 * _SEH_TRY_FILTER_FINALLY(name1,name2) {
 *   a
 * } _SEH_HANDLE {
 *   b
 * } _SEH_END;
 */

void handle_try( sl_t &container, sl_it try_kw, sl_it end ) {
    string temp;
    sl_t pseh_clause, temp_tok;
    string finally_name, filter_name;
    sl_it try_block, try_block_end, except_kw, paren, end_paren,
	except_block, except_block_end, todelete,
	finally_kw, finally_block, finally_block_end, clause_end;

    try_block = try_kw;
    try_block++;
    try_block_end = complete_block( try_block, end, "{", "}" );
    
    if( try_block_end == end ) 
	error( container, try_block, "unclosed try block");

    except_kw = try_block_end;

    if( *except_kw == FINALLY_TOKEN ) {
	finally_kw = except_kw;
	except_kw = end;
	paren = end;
	end_paren = end;
	except_block = end;
	except_block_end = end;
    } else if( *except_kw == EXCEPT_TOKEN ) {
	paren = except_kw;
	paren++;
	if( *paren == "(" ) {
	    end_paren = complete_block( paren, end, "(", ")" );
	    except_block = end_paren;
	} else {
	    except_block = paren;
	    paren = end;
	    end_paren = end;
	}
	except_block_end = complete_block( except_block, end, "{", "}" );
	finally_kw = except_block_end;
    } else {
	except_kw = paren = end_paren = except_block = except_block_end =
	    finally_kw = finally_block = finally_block_end = end;
    }

    if( finally_kw != end && *finally_kw != FINALLY_TOKEN ) {
	finally_kw = end;
	finally_block = end;
	finally_block_end = end;
    } else {
	finally_block = finally_kw;
	finally_block++;
	finally_block_end = complete_block( finally_block, end, "{", "}" );
    }

    if( finally_block_end != end ) clause_end = finally_block_end;
    else if( except_block_end != end ) clause_end = except_block_end;
    else clause_end = try_block_end;

    /* Skip one so that we can do != on clause_end */

    /* Now for the output phase -- we've collected the whole seh clause
     * and it lies between try_kw and clause_end */

    finally_name = makename("_Finally");
    filter_name = makename("_Filter");

    pseh_clause.push_back( "_SEH_FINALLY" );
    pseh_clause.push_back( "(" );
    pseh_clause.push_back( finally_name );
    pseh_clause.push_back( ")" );
    if( finally_kw != end ) 
	append_block( pseh_clause, finally_block, finally_block_end );
    else {
	pseh_clause.push_back( "{" );
	pseh_clause.push_back( "}" );
    }

    pseh_clause.push_back( "_SEH_FILTER" );
    pseh_clause.push_back( "(" );
    pseh_clause.push_back( filter_name );
    pseh_clause.push_back( ")" );
    pseh_clause.push_back( "{" );
    pseh_clause.push_back( "return" );
    if( paren != end )
	append_block( pseh_clause, paren, end_paren );
    else
	pseh_clause.push_back( "EXCEPTION_EXECUTE_HANDLER" );
    pseh_clause.push_back( ";" );
    pseh_clause.push_back( "}" );

    pseh_clause.push_back( "_SEH_TRY_FILTER_FINALLY" );
    pseh_clause.push_back( "(" );
    pseh_clause.push_back( filter_name );
    pseh_clause.push_back( "," );
    pseh_clause.push_back( finally_name );
    pseh_clause.push_back( ")" );
    append_block( pseh_clause, try_block, try_block_end );
    pseh_clause.push_back( "_SEH_HANDLE" );
    pseh_clause.push_back( "{" );
    if( except_block != end )
	append_block( pseh_clause, except_block, except_block_end );
    pseh_clause.push_back( "}" );
    pseh_clause.push_back( "_SEH_END" );
    pseh_clause.push_back( ";" );

    container.splice( try_kw, pseh_clause );
    while( try_kw != clause_end ) {
	todelete = try_kw;
	try_kw++;
	container.erase( todelete );
    }
}

void print_tokens( sl_it begin, sl_it end ) {
    for( sl_it i = begin; i != end; i++ ) 
	if( *i == "\n" ) cout << *i;
	else cout << /*"[" <<*/ *i << /*"]" <<*/ " ";
}

int main( int argc, char **argv ) {
    sl_t tok;
    sl_it try_found;
    int i;

    for( i = 1; i < argc; i++ ) {
	if( string(argv[i]) == "-try" && i < argc - 1 ) {
	    i++;
	    TRY_TOKEN = argv[i];
	} else if( string(argv[i]) == "-except" && i < argc - 1 ) {
	    i++;
	    EXCEPT_TOKEN = argv[i];
	} else if( string(argv[i]) == "-finally" && i < argc - 1 ) {
	    i++;
	    FINALLY_TOKEN = argv[i];
	}
    }

    /* XXX Uses much memory for large files */
    while( expand_input(tok) );

    while( (try_found = find( tok.begin(), tok.end(), TRY_TOKEN )) !=
	   tok.end() ) {
	handle_try( tok, try_found, tok.end() );
    }

    tok.push_front("#include <pseh/framebased.h>\n");
    print_tokens( tok.begin(), tok.end() );
}
