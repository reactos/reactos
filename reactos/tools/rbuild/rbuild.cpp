// rbuild.cpp

#ifdef _MSC_VER
#pragma warning ( disable : 4786 )
#endif//_MSC_VER

#include <stdio.h>
#include <io.h>
#include <assert.h>
#include "rbuild.h"

using std::string;
using std::vector;

#ifdef _MSC_VER
unsigned __int64
#else
unsigned long long
#endif
filelen ( FILE* f )
{
#ifdef WIN32
	return _filelengthi64 ( _fileno(f) );
#elif defined(UNIX)
	struct stat64 file_stat;
	if ( fstat64(fileno(f), &file_stat) != 0 )
		return 0;
	return file_stat.st_size;
#endif
}

static const char* WS = " \t\r\n";
static const char* WSEQ = " =\t\r\n";

XMLFile::XMLFile()
{
}

void XMLFile::close()
{
	while ( _f.size() )
	{
		fclose ( _f.back() );
		_f.pop_back();
	}
	_buf.resize(0);
	_p = _end = NULL;
}

bool XMLFile::open(const char* filename)
{
	close();
	FILE* f = fopen ( filename, "r" );
	if ( !f )
		return false;
	unsigned long len = (unsigned long)filelen(f);
	_buf.resize ( len );
	fread ( &_buf[0], 1, len, f );
	_p = _buf.c_str();
	_end = _p + len;
	_f.push_back ( f );
	next_token();
	return true;
}

// next_token() moves the pointer to next token, which may be
// an xml element or a text element, basically it's a glorified
// skipspace, normally the user of this class won't need to call
// this function
void XMLFile::next_token()
{
	_p += strspn ( _p, WS );
}

bool XMLFile::next_is_text()
{
	return *_p != '<';
}

bool XMLFile::more_tokens()
{
	return _p != _end;
}

// get_token() is used to return a token, and move the pointer
// past the token
bool XMLFile::get_token(string& token)
{
	const char* tokend;
	if ( *_p == '<' )
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

XMLAttribute::XMLAttribute()
{
}

XMLAttribute::XMLAttribute(const string& name_,
                           const string& value_)
: name(name_), value(value_)
{
}

XMLElement::XMLElement()
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

// Parse()
// This function takes a single xml tag ( i.e. beginning with '<' and
// ending with '>', and parses out it's tag name and constituent
// attributes.
// Return Value: returns true if you need to look for a </tag> for
// the one it just parsed...
bool XMLElement::Parse(const string& token,
                       bool& end_tag)
{
	const char* p = token.c_str();
	assert ( *p == '<' );
	p++;
	p += strspn ( p, WS );
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
		attributes.push_back ( new XMLAttribute ( attribute, value ) );
	}
	return !( *p == '/' ) && !end_tag;
}

const XMLAttribute* XMLElement::GetAttribute ( const string& attribute ) const
{
	// this would be faster with a tree-based container, but our attribute
	// lists are likely to stay so short as to not be an issue.
	for ( int i = 0; i < attributes.size(); i++ )
	{
		if ( attribute == attributes[i]->name )
			return attributes[i];
	}
	return NULL;
}

// XMLParse()
// This function reads a "token" from the file loaded in XMLFile
// REM TODO FIXME: At the moment it can't handle comments or non-xml tags.
// if it finds a tag that is non-singular, it parses sub-elements and/or
// inner text into the XMLElement that it is building to return.
// Return Value: an XMLElement allocated via the new operator that contains
// it's parsed data. Keep calling this function until it returns NULL
// (no more data)
XMLElement* XMLParse(XMLFile& f,
                     bool* pend_tag = NULL)
{
	string token;
	if ( !f.get_token(token) )
		return NULL;
	XMLElement* e = new XMLElement;
	bool end_tag;
	if ( !e->Parse ( token, end_tag ) )
	{
		if ( pend_tag )
			*pend_tag = end_tag;
		else if ( end_tag )
		{
			delete e;
			printf ( "syntax error: end tag '%s' not expected\n", token.c_str() );
			return NULL;
		}
		return e;
	}
	while ( f.more_tokens() )
	{
		if ( f.next_is_text() )
		{
			if ( !f.get_token ( token ) )
			{
				printf ( "internal tool error - get_token() failed when more_tokens() returned true\n" );
				break;
			}
			if ( e->subElements.size() )
				printf ( "syntax error: mixing of inner text with sub elements\n" );
			if ( e->value.size() )
			{
				printf ( "syntax error: multiple instances of inner text\n" );
				e->value += " " + token;
			}
			else
				e->value = token;
		}
		else
		{
			XMLElement* e2 = XMLParse ( f, &end_tag );
			if ( end_tag )
			{
				if ( e->name != e2->name )
					printf ( "end tag name mismatch\n" );
				delete e2;
				break;
			}
			e->subElements.push_back ( e2 );
		}
	}
	return e;
}

void Project::ProcessXML ( const XMLElement& e, const string& path )
{
	const XMLAttribute *att;
	string subpath(path);
	if ( e.name == "project" )
	{
		att = e.GetAttribute ( "name" );
		if ( !att )
			name = "Unnamed";
		else
			name = att->value;
	}
	else if ( e.name == "module" )
	{
		att = e.GetAttribute ( "name" );
		if ( !att )
		{
			printf ( "syntax error: 'name' attribute required for <module>\n" );
			return;
		}
		Module* module = new Module ( e, att->value, path );
		modules.push_back ( module );
		return; // REM TODO FIXME no processing of modules... yet
	}
	else if ( e.name == "directory" )
	{
		const XMLAttribute* att = e.GetAttribute("name");
		if ( !att )
		{
			printf ( "syntax error: 'name' attribute required for <directory>\n" );
			return;
		}
		subpath = path + "/" + att->value;
	}
	for ( size_t i = 0; i < e.subElements.size(); i++ )
		ProcessXML ( *e.subElements[i], subpath );
}

int main ( int argc, char** argv )
{
	XMLFile f;
	if ( !f.open ( "ReactOS.xml" ) )
	{
		printf ( "couldn't open ReactOS.xml!\n" );
		return -1;
	}

	for ( ;; )
	{
		XMLElement* head = XMLParse ( f );
		if ( !head )
			break; // end of file

		if ( head->name != "project" )
		{
			printf ( "error: expecting 'project', got '%s'\n", head->name.c_str() );
			continue;
		}

		Project* proj = new Project;
		proj->ProcessXML ( *head, "." );

		// REM TODO FIXME actually do something with Project object...
		printf ( "Found %lu modules:\n", proj->modules.size() );
		for ( size_t i = 0; i < proj->modules.size(); i++ )
		{
			Module& m = *proj->modules[i];
			printf ( "\t%s in folder: %s\n",
			         m.name.c_str(),
			         m.path.c_str() );
		}

		delete proj;
		delete head;
	}

	return 0;
}
