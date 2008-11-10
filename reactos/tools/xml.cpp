/*
 * Copyright (C) 2005 Casper S. Hornstrup
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef _MSC_VER
#pragma warning ( disable : 4786 )
#endif//_MSC_VER

#ifdef WIN32
    #include <direct.h>
    #include <io.h>
#else
    #include <sys/stat.h>
    #include <unistd.h>

    // Some hosts don't define PATH_MAX in unistd.h
    #if !defined(PATH_MAX)
        #include <limits.h>
    #endif

    #define MAX_PATH PATH_MAX
#endif

#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "xml.h"
#include "ssprintf.h"

#ifndef MAX_PATH
#define MAX_PATH _MAX_PATH
#endif

using std::string;
using std::vector;

#ifdef WIN32
#define getcwd _getcwd
#endif//WIN32

static const char* WS = " \t\r\n";
static const char* WSEQ = " =\t\r\n";

string working_directory;

XMLException::XMLException (
	const std::string& location,
	const char* format, ... )
{
	va_list args;
	va_start ( args, format );
	SetExceptionV ( location, format, args );
	va_end ( args );
}

void XMLException::SetExceptionV ( const std::string& location, const char* format, va_list args )
{
	_e = location + ": " + ssvprintf(format,args);
}

void XMLException::SetException ( const std::string& location, const char* format, ... )
{
	va_list args;
	va_start ( args, format );
	SetExceptionV ( location, format, args );
	va_end ( args );
}

XMLIncludes::~XMLIncludes()
{
	for ( size_t i = 0; i < this->size(); i++ )
		delete (*this)[i];
}

void
InitWorkingDirectory()
{
	// store the current directory for path calculations
	working_directory.resize ( MAX_PATH );
	working_directory[0] = 0;
	getcwd ( &working_directory[0], working_directory.size() );
	working_directory.resize ( strlen ( working_directory.c_str() ) );
}

#ifdef _MSC_VER
unsigned __int64
#else
unsigned long long
#endif
filelen ( FILE* f )
{
#ifdef WIN32
	return _filelengthi64 ( _fileno(f) );
#else
# if defined(__FreeBSD__) || defined(__APPLE__) || defined(__CYGWIN__)
	struct stat file_stat;
	if ( fstat(fileno(f), &file_stat) != 0 )
# else
	struct stat64 file_stat;
	if ( fstat64(fileno(f), &file_stat) != 0 )
# endif // __FreeBSD__
		return 0;
	return file_stat.st_size;
#endif // WIN32
}

Path::Path()
{
	if ( !working_directory.size() )
		InitWorkingDirectory();
	string s ( working_directory );
	const char* p = strtok ( &s[0], "/\\" );
	while ( p )
	{
		if ( *p )
			path.push_back ( p );
		p = strtok ( NULL, "/\\" );
	}
}

Path::Path ( const Path& cwd, const string& file )
{
	string s ( cwd.Fixup ( file, false ) );
	const char* p = strtok ( &s[0], "/\\" );
	while ( p )
	{
		if ( *p )
			path.push_back ( p );
		p = strtok ( NULL, "/\\" );
	}
}

string
Path::Fixup ( const string& file, bool include_filename ) const
{
	if ( strchr ( "/\\", file[0] )
#ifdef WIN32
		// this squirreliness is b/c win32 has drive letters and *nix doesn't...
		|| file[1] == ':'
#endif//WIN32
		)
	{
		return file;
	}
	vector<string> pathtmp ( path );
	string tmp ( file );
	const char* prev = strtok ( &tmp[0], "/\\" );
	const char* p = strtok ( NULL, "/\\" );
	while ( p )
	{
		if ( !strcmp ( prev, "." ) )
			; // do nothing
		else if ( !strcmp ( prev, ".." ) )
		{
			// this squirreliness is b/c win32 has drive letters and *nix doesn't...
#ifdef WIN32
			if ( pathtmp.size() > 1 )
#else
			if ( pathtmp.size() )
#endif
				pathtmp.resize ( pathtmp.size() - 1 );
		}
		else
			pathtmp.push_back ( prev );
		prev = p;
		p = strtok ( NULL, "/\\" );
	}
	if ( include_filename )
		pathtmp.push_back ( prev );

	// reuse tmp variable to return recombined path
	tmp.resize(0);
	for ( size_t i = 0; i < pathtmp.size(); i++ )
	{
		// this squirreliness is b/c win32 has drive letters and *nix doesn't...
#ifdef WIN32
		if ( i ) tmp += "/";
#else
		tmp += "/";
#endif
		tmp += pathtmp[i];
	}
	return tmp;
}

string
Path::RelativeFromWorkingDirectory ()
{
	string out = "";
	for ( size_t i = 0; i < path.size(); i++ )
	{
		out += "/" + path[i];
	}
	return RelativeFromWorkingDirectory ( out );
}

string
Path::RelativeFromWorkingDirectory ( const string& path )
{
	return Path::RelativeFromDirectory ( path, working_directory );
}

string
Path::RelativeFromDirectory (
	const string& path,
	const string& base_directory )
{
	vector<string> vbase, vpath, vout;
	Path::Split ( vbase, base_directory, true );
	Path::Split ( vpath, path, true );
#ifdef WIN32
	// this squirreliness is b/c win32 has drive letters and *nix doesn't...
	// not possible to do relative across different drive letters
	{
		char path_driveletter = (path[1] == ':') ? toupper(path[0]) : 0;
		char base_driveletter = (base_directory[1] == ':') ? toupper(base_directory[0]) : 0;
		if ( path_driveletter != base_driveletter )
			return path;
	}
#endif
	size_t i = 0;
	while ( i < vbase.size() && i < vpath.size() && vbase[i] == vpath[i] )
		++i;

	// did we go through all of the path?
	if ( vbase.size() == vpath.size() && i == vpath.size() )
		return ".";

	if ( i < vbase.size() )
	{
		// path goes above our base directory, we will need some ..'s
		for ( size_t j = i; j < vbase.size(); j++ )
			vout.push_back ( ".." );
	}

	while ( i < vpath.size() )
		vout.push_back ( vpath[i++] );

	// now merge vout into a string again
	string out = vout[0];
	for ( i = 1; i < vout.size(); i++ )
	{
		out += "/" + vout[i];
	}
	return out;
}

void
Path::Split (
	vector<string>& out,
	const string& path,
	bool include_last )
{
	string s ( path );
	const char* prev = strtok ( &s[0], "/\\" );
	const char* p = strtok ( NULL, "/\\" );
	out.resize ( 0 );
	while ( p )
	{
		if ( strcmp ( prev, "." ) )
			out.push_back ( prev );
		prev = p;
		p = strtok ( NULL, "/\\" );
	}
	if ( include_last && strcmp ( prev, "." ) )
		out.push_back ( prev );
	// special-case where path only has "."
	// don't move this check up higher as it might miss
	// some funny paths...
	if ( !out.size() && !strcmp ( prev, "." ) )
		out.push_back ( "." );
}

XMLFile::XMLFile()
{
}

void
XMLFile::close()
{
	_buf.resize(0);
	_p = _end = NULL;
}

bool
XMLFile::open ( const string& filename_ )
{
	close();
	FILE* f = fopen ( filename_.c_str(), "rb" );
	if ( !f )
		return false;
	unsigned long len = (unsigned long)filelen(f);
	_buf.resize ( len );
	fread ( &_buf[0], 1, len, f );
	fclose ( f );
	_p = _buf.c_str();
	_end = _p + len;
	_filename = filename_;
	next_token();
	return true;
}

// next_token() moves the pointer to next token, which may be
// an xml element or a text element, basically it's a glorified
// skipspace, normally the user of this class won't need to call
// this function
void
XMLFile::next_token()
{
	_p += strspn ( _p, WS );
}

bool
XMLFile::next_is_text()
{
	return *_p != '<';
}

bool
XMLFile::more_tokens ()
{
	return _p != _end;
}

// get_token() is used to return a token, and move the pointer
// past the token
bool
XMLFile::get_token ( string& token )
{
	const char* tokend;
	if ( !strncmp ( _p, "<!--", 4 ) )
	{
		tokend = strstr ( _p, "-->" );
		if ( !tokend )
			tokend = _end;
		else
			tokend += 3;
	}
	else if ( !strncmp ( _p, "<?", 2 ) )
	{
		tokend = strstr ( _p, "?>" );
		if ( !tokend )
			tokend = _end;
		else
			tokend += 2;
	}
	else if ( *_p == '<' )
	{
		tokend = strchr ( _p, '>' );
		if ( !tokend )
			tokend = _end;
		else
			++tokend;
	}
	else
	{
		tokend = strchr ( _p, '<' );
		if ( !tokend )
			tokend = _end;
		while ( tokend > _p && isspace(tokend[-1]) )
			--tokend;
	}
	if ( tokend == _p )
		return false;
	token = string ( _p, tokend-_p );
	_p = tokend;
	next_token();
	return true;
}

bool
XMLFile::get_token ( string& token, string& location )
{
	location = Location();
	return get_token ( token );
}

string
XMLFile::Location() const
{
	int line = 1;
	const char* p = strchr ( _buf.c_str(), '\n' );
	while ( p && p < _p )
	{
		++line;
		p = strchr ( p+1, '\n' );
	}
	return ssprintf ( "%s(%i)",_filename.c_str(), line );
}

XMLAttribute::XMLAttribute()
{
}

XMLAttribute::XMLAttribute(
	const string& name_,
	const string& value_ )
	: name(name_), value(value_)
{
}

XMLAttribute::XMLAttribute ( const XMLAttribute& src )
	: name(src.name), value(src.value)
{

}

XMLAttribute& XMLAttribute::operator = ( const XMLAttribute& src )
{
	name = src.name;
	value = src.value;
	return *this;
}

XMLElement::XMLElement (
	XMLFile* xmlFile,
	const string& location )
	: xmlFile ( xmlFile ),
	  location ( location ),
	  parentElement ( NULL )
{
}

XMLElement::~XMLElement()
{
	size_t i;
	for ( i = 0; i < attributes.size(); i++ )
		delete attributes[i];
	for ( i = 0; i < subElements.size(); i++ )
		delete subElements[i];
}

void
XMLElement::AddSubElement ( XMLElement* e )
{
	subElements.push_back ( e );
	e->parentElement = this;
}

// Parse()
// This function takes a single xml tag ( i.e. beginning with '<' and
// ending with '>', and parses out it's tag name and constituent
// attributes.
// Return Value: returns true if you need to look for a </tag> for
// the one it just parsed...
bool
XMLElement::Parse (
	const string& token,
	bool& end_tag )
{
	const char* p = token.c_str();
	assert ( *p == '<' );
	++p;
	p += strspn ( p, WS );

	// check if this is a comment
	if ( !strncmp ( p, "!--", 3 ) )
	{
		name = "!--";
		end_tag = false;
		return false; // never look for end tag to a comment
	}

	end_tag = ( *p == '/' );
	if ( end_tag )
	{
		++p;
		p += strspn ( p, WS );
	}
	const char* end = strpbrk ( p, WS );
	if ( !end )
	{
		end = strpbrk ( p, "/>" );
		assert ( end );
	}
	name = string ( p, end-p );
	p = end;
	p += strspn ( p, WS );
	while ( *p != '>' && *p != '/' )
	{
		end = strpbrk ( p, WSEQ );
		if ( !end )
		{
			end = strpbrk ( p, "/>" );
			assert ( end );
		}
		string attribute ( p, end-p ), value;
		p = end;
		p += strspn ( p, WS );
		if ( *p == '=' )
		{
			++p;
			p += strspn ( p, WS );
			char quote = 0;
			if ( strchr ( "\"'", *p ) )
			{
				quote = *p++;
				end = strchr ( p, quote );
			}
			else
			{
				end = strpbrk ( p, WS );
			}
			if ( !end )
			{
				end = strchr ( p, '>' );
				assert(end);
				if ( end[-1] == '/' )
					end--;
			}
			value = string ( p, end-p );
			p = end;
			if ( quote && *p == quote )
				p++;
			p += strspn ( p, WS );
		}
		else if ( name[0] != '!' )
		{
			throw XMLSyntaxErrorException (
				location,
				"attributes must have values" );
		}
		attributes.push_back ( new XMLAttribute ( attribute, value ) );
	}
	return !( *p == '/' ) && !end_tag;
}

XMLAttribute*
XMLElement::GetAttribute (
	const string& attribute,
	bool required )
{
	// this would be faster with a tree-based container, but our attribute
	// lists are likely to stay so short as to not be an issue.
	for ( size_t i = 0; i < attributes.size(); i++ )
	{
		if ( attribute == attributes[i]->name )
			return attributes[i];
	}
	if ( required )
	{
		throw XMLRequiredAttributeNotFoundException (
			location,
			attribute,
			name );
	}
	return NULL;
}

const XMLAttribute*
XMLElement::GetAttribute (
	const string& attribute,
	bool required ) const
{
	// this would be faster with a tree-based container, but our attribute
	// lists are likely to stay so short as to not be an issue.
	for ( size_t i = 0; i < attributes.size(); i++ )
	{
		if ( attribute == attributes[i]->name )
			return attributes[i];
	}
	if ( required )
	{
		throw XMLRequiredAttributeNotFoundException (
			location,
			attribute,
			name );
	}
	return NULL;
}

int
XMLElement::FindElement ( const std::string& type, int prev ) const
{
	int done = subElements.size();
	while ( ++prev < done )
	{
		XMLElement* e = subElements[prev];
		if ( e->name == type )
			return prev;
	}
	return -1;
}

int
XMLElement::GetElements (
	const std::string& type,
	std::vector<XMLElement*>& v )
{
	int find = FindElement ( type );
	v.resize ( 0 );
	while ( find != -1 )
	{
		v.push_back ( subElements[find] );
		find = FindElement ( type, find );
	}
	return v.size();
}

int
XMLElement::GetElements (
	const std::string& type,
	std::vector<const XMLElement*>& v ) const
{
	int find = FindElement ( type );
	v.resize ( 0 );
	while ( find != -1 )
	{
		v.push_back ( subElements[find] );
		find = FindElement ( type, find );
	}
	return v.size();
}

// XMLParse()
// This function reads a "token" from the file loaded in XMLFile
// if it finds a tag that is non-singular, it parses sub-elements and/or
// inner text into the XMLElement that it is building to return.
// Return Value: an XMLElement allocated via the new operator that contains
// it's parsed data. Keep calling this function until it returns NULL
// (no more data)
XMLElement*
XMLParse (
	XMLFile& f,
	XMLIncludes* includes,
	const Path& path,
	bool* pend_tag = NULL )
{
	string token, location;
	if ( !f.get_token(token,location) )
		return NULL;
	bool end_tag, is_include = false;

	while
	(
		token[0] != '<'
		|| !strncmp ( token.c_str (), "<!--", 4 )
		|| !strncmp ( token.c_str (), "<?", 2 )
	)
	{
		if ( token[0] != '<' )
		{
			throw XMLSyntaxErrorException (
				location,
				"expecting xml tag, not '%s'",
				token.c_str () );
		}
		if ( !f.get_token ( token, location ) )
			return NULL;
	}

	XMLElement* e = new XMLElement (
		&f,
		location );
	bool bNeedEnd = e->Parse ( token, end_tag );

	if ( e->name == "xi:include" && includes )
	{
		XMLAttribute* att;
		att = e->GetAttribute ( "href", true );
		assert ( att );
		string includeFile ( path.Fixup ( att->value, true ) );
		string topIncludeFile (
			Path::RelativeFromWorkingDirectory ( includeFile ) );
		includes->push_back (
			new XMLInclude ( e, path, topIncludeFile ) );
		is_include = true;
	}

	if ( !bNeedEnd )
	{
		if ( pend_tag )
			*pend_tag = end_tag;
		else if ( end_tag )
		{
			delete e;
			throw XMLSyntaxErrorException (
				location,
				"end tag '%s' not expected",
				token.c_str() );
			return NULL;
		}
		return e;
	}
	bool bThisMixingErrorReported = false;
	while ( f.more_tokens () )
	{
		if ( f.next_is_text () )
		{
			if ( !f.get_token ( token, location ) || token.size () == 0 )
			{
				throw XMLInvalidBuildFileException (
					location,
					"internal tool error - get_token() failed when more_tokens() returned true" );
				break;
			}
			if ( e->subElements.size() && !bThisMixingErrorReported )
			{
				throw XMLSyntaxErrorException (
					location,
					"mixing of inner text with sub elements" );
				bThisMixingErrorReported = true;
			}
			if ( strchr ( token.c_str (), '>' ) )
			{
				throw XMLSyntaxErrorException (
					location,
					"invalid symbol '>'" );
			}
			if ( e->value.size() > 0 )
			{
				throw XMLSyntaxErrorException (
					location,
					"multiple instances of inner text" );
				e->value += " " + token;
			}
			else
				e->value = token;
		}
		else
		{
			XMLElement* e2 = XMLParse (
				f, is_include ? NULL : includes, path, &end_tag );
			if ( !e2 )
			{
				string e_location = e->location;
				string e_name = e->name;
				delete e;
				throw XMLInvalidBuildFileException (
					e_location,
					"end of file found looking for end tag: </%s>",
					e_name.c_str() );
				break;
			}
			if ( end_tag )
			{
				if ( e->name != e2->name )
				{
					string e2_location = e2->location;
					string e_name = e->name;
					string e2_name = e2->name;
					delete e;
					delete e2;
					throw XMLSyntaxErrorException (
						e2_location,
						"end tag name mismatch - found </%s> but was expecting </%s>",
						e2_name.c_str(),
						e_name.c_str() );
					break;
				}
				delete e2;
				break;
			}
			if ( e->value.size () > 0 && !bThisMixingErrorReported )
			{
				string e_location = e->location;
				delete e;
				throw XMLSyntaxErrorException (
					e_location,
					"mixing of inner text with sub elements" );
				bThisMixingErrorReported = true;
			}
			e->AddSubElement ( e2 );
		}
	}
	return e;
}

void
XMLReadFile (
	XMLFile& f,
	XMLElement& head,
	XMLIncludes& includes,
	const Path& path )
{
	for ( ;; )
	{
		XMLElement* e = XMLParse ( f, &includes, path );
		if ( !e )
			return;
		head.AddSubElement ( e );
	}
}

XMLElement*
XMLLoadInclude (
	XMLInclude& include,
	XMLIncludes& includes )
{
	XMLAttribute* att;
	att = include.e->GetAttribute("href", true);
	assert(att);

	string file ( include.path.Fixup(att->value, true) );
	string top_file ( Path::RelativeFromWorkingDirectory ( file ) );
	include.e->attributes.push_back ( new XMLAttribute ( "top_href", top_file ) );
	XMLFile* fInc = new XMLFile();
	if ( !fInc->open ( file ) )
	{
		include.fileExists = false;
		// look for xi:fallback element
		for ( size_t i = 0; i < include.e->subElements.size (); i++ )
		{
			XMLElement* e2 = include.e->subElements[i];
			if ( e2->name == "xi:fallback" )
			{
				// now look for xi:include below...
				for ( i = 0; i < e2->subElements.size (); i++ )
				{
					XMLElement* e3 = e2->subElements[i];
					if ( e3->name == "xi:include" )
					{
						att = e3->GetAttribute ( "href", true );
						assert ( att );
						string includeFile (
							include.path.Fixup ( att->value, true ) );
						string topIncludeFile (
							Path::RelativeFromWorkingDirectory ( includeFile ) );
						XMLInclude* fallbackInclude =
							new XMLInclude ( e3, include.path, topIncludeFile );

						XMLElement* value = XMLLoadInclude (*fallbackInclude, includes );
						delete fallbackInclude;
						return value;
					}
				}
				throw XMLInvalidBuildFileException (
					e2->location,
					"<xi:fallback> must have a <xi:include> sub-element" );
				return NULL;
			}
		}
		return NULL;
	}
	else
	{
		include.fileExists = true;
		XMLElement* new_e = new XMLElement (
			fInc,
			include.e->location );
		new_e->name = "xi:included";
		Path path2 ( include.path, att->value );
		XMLReadFile ( *fInc, *new_e, includes, path2 );
		return new_e;
	}
}

XMLElement*
XMLLoadFile (
	const string& filename,
	const Path& path,
	XMLIncludes& includes )
{
	XMLFile* f = new XMLFile();

	if ( !f->open ( filename ) )
	{
		delete f;
		throw XMLFileNotFoundException ( "(virtual)", filename );
		return NULL;
	}

	XMLElement* head = new XMLElement ( f, "(virtual)" );

	XMLReadFile ( *f, *head, includes, path );

	for ( size_t i = 0; i < includes.size (); i++ )
	{
		XMLElement* e = includes[i]->e;
		XMLElement* e2 = XMLLoadInclude ( *includes[i], includes );
		if ( !e2 )
		{
			throw XMLFileNotFoundException (
				f->Location(),
				e->GetAttribute ( "top_href", true )->value );
		}
		XMLElement* parent = e->parentElement;
		XMLElement** parent_container = NULL;
		if ( !parent )
		{
			string location = e->location;
			delete e;
			delete f;
			throw XMLException ( location, "internal tool error: xi:include doesn't have a parent" );
			return NULL;
		}
		for ( size_t j = 0; j < parent->subElements.size (); j++ )
		{
			if ( parent->subElements[j] == e )
			{
				parent_container = &parent->subElements[j];
				break;
			}
		}
		if ( !parent_container )
		{
			string location = e->location;
			delete e;
			delete f;
			throw XMLException ( location, "internal tool error: couldn't find xi:include in parent's sub-elements" );
			return NULL;
		}
		// replace inclusion tree with the imported tree
		e2->parentElement = e->parentElement;
		e2->name = e->name;
		e2->attributes = e->attributes;
		*parent_container = e2;
		e->attributes.resize ( 0 );
		delete e;
	}
	delete f;
	return head;
}

XMLElement*
XMLLoadFile ( const string& filename )
{
	Path path;
	XMLIncludes includes;
	return XMLLoadFile ( filename, path, includes );
}
