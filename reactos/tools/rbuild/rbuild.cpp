// rbuild.cpp

#ifdef _MSC_VER
#pragma warning ( disable : 4786 ) // identifier was truncated to '255' characters in the debug information
#endif//_MSC_VER

#include <stdio.h>
#include <io.h>
#include <assert.h>
#include <direct.h>
#include "rbuild.h"

using std::string;
using std::vector;

#ifdef WIN32
#define getcwd _getcwd
#endif//WIN32
string working_directory;

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

Path::Path()
{
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

/*static*/ string
Path::RelativeFromWorkingDirectory ( const string& path )
{
	vector<string> vwork, vpath, vout;
	Path::Split ( vwork, working_directory, true );
	Path::Split ( vpath, path, true );
#ifdef WIN32
	// this squirreliness is b/c win32 has drive letters and *nix doesn't...
	// not possible to do relative across different drive letters
	if ( vwork[0] != vpath[0] )
		return path;
#endif
	size_t i = 0;
	while ( i < vwork.size() && i < vpath.size() && vwork[i] == vpath[i] )
		++i;
	if ( i < vwork.size() )
	{
		// path goes above our working directory, we will need some ..'s
		for ( size_t j = 0; j < i; j++ )
			vout.push_back ( ".." );
	}
	while ( i < vpath.size() )
		vout.push_back ( vpath[i++] );

	// now merge vout into a string again
	string out;
	for ( i = 0; i < vout.size(); i++ )
	{
		// this squirreliness is b/c win32 has drive letters and *nix doesn't...
#ifdef WIN32
		if ( i ) out += "/";
#else
		out += "/";
#endif
		out += vout[i];
	}
	return out;
}

/*static*/ void
Path::Split ( vector<string>& out,
              const string& path,
              bool include_last )
{
	string s ( path );
	const char* prev = strtok ( &s[0], "/\\" );
	const char* p = strtok ( NULL, "/\\" );
	out.resize ( 0 );
	while ( p )
	{
		out.push_back ( prev );
		prev = p;
		p = strtok ( NULL, "/\\" );
	}
	if ( include_last )
		out.push_back ( prev );
}

XMLFile::XMLFile()
{
}

void
XMLFile::close()
{
	while ( _f.size() )
	{
		fclose ( _f.back() );
		_f.pop_back();
	}
	_buf.resize(0);
	_p = _end = NULL;
}

bool
XMLFile::open(const string& filename)
{
	close();
	FILE* f = fopen ( filename.c_str(), "rb" );
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
XMLFile::more_tokens()
{
	return _p != _end;
}

// get_token() is used to return a token, and move the pointer
// past the token
bool
XMLFile::get_token(string& token)
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

XMLAttribute::XMLAttribute()
{
}

XMLAttribute::XMLAttribute(const string& name_,
                           const string& value_)
	: name(name_), value(value_)
{
}

XMLElement::XMLElement()
	: parentElement(NULL)
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
XMLElement::Parse(const string& token,
                  bool& end_tag)
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
		attributes.push_back ( new XMLAttribute ( attribute, value ) );
	}
	return !( *p == '/' ) && !end_tag;
}

XMLAttribute*
XMLElement::GetAttribute ( const string& attribute,
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
		printf ( "syntax error: attribute '%s' required for <%s>\n",
			attribute.c_str(), name.c_str() );
	}
	return NULL;
}

const XMLAttribute*
XMLElement::GetAttribute ( const string& attribute,
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
		printf ( "syntax error: attribute '%s' required for <%s>\n",
			attribute.c_str(), name.c_str() );
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
XMLElement*
XMLParse(XMLFile& f,
         const Path& path,
         bool* pend_tag = NULL)
{
	string token;
	if ( !f.get_token(token) )
		return NULL;
	bool end_tag;

	while ( token[0] != '<' )
	{
		printf ( "syntax error: expecting xml tag, not '%s'\n", token.c_str() );
		if ( !f.get_token(token) )
			return NULL;
	}

	XMLElement* e = new XMLElement;
	bool bNeedEnd = e->Parse ( token, end_tag );

	if ( e->name == "xi:include" )
	{
		XMLAttribute* att;
		att = e->GetAttribute("href",true);
		if ( att )
		{
			string file ( path.Fixup(att->value,true) );
			string top_file ( Path::RelativeFromWorkingDirectory ( file ) );
			e->attributes.push_back ( new XMLAttribute ( "top_href", top_file ) );
			XMLFile fInc;
			if ( !fInc.open ( file ) )
				printf ( "xi:include error, couldn't find file '%s'\n", file.c_str() );
			else
			{
				Path path2 ( path, att->value );
				for ( ;; )
				{
					XMLElement* e2 = XMLParse ( fInc, path2 );
					if ( !e2 )
						break;
					e->AddSubElement ( e2 );
				}
			}
		}
	}

	if ( !bNeedEnd )
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
	bool bThisMixingErrorReported = false;
	while ( f.more_tokens() )
	{
		if ( f.next_is_text() )
		{
			if ( !f.get_token ( token ) || !token.size() )
			{
				printf ( "internal tool error - get_token() failed when more_tokens() returned true\n" );
				break;
			}
			if ( e->subElements.size() && !bThisMixingErrorReported )
			{
				printf ( "syntax error: mixing of inner text with sub elements\n" );
				bThisMixingErrorReported = true;
			}
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
			XMLElement* e2 = XMLParse ( f, path, &end_tag );
			if ( end_tag )
			{
				if ( e->name != e2->name )
					printf ( "end tag name mismatch\n" );
				delete e2;
				break;
			}
			if ( e->value.size() && !bThisMixingErrorReported )
			{
				printf ( "syntax error: mixing of inner text with sub elements\n" );
				bThisMixingErrorReported = true;
			}
			e->AddSubElement ( e2 );
		}
	}
	return e;
}

Project::~Project()
{
	for ( size_t i = 0; i < modules.size(); i++ )
		delete modules[i];
}

void
Project::ProcessXML ( const XMLElement& e, const string& path )
{
	const XMLAttribute *att;
	string subpath(path);
	if ( e.name == "project" )
	{
		att = e.GetAttribute ( "name", false );
		if ( !att )
			name = "Unnamed";
		else
			name = att->value;
	}
	else if ( e.name == "module" )
	{
		att = e.GetAttribute ( "name", true );
		if ( !att )
			return;
		Module* module = new Module ( e, att->value, path );
		modules.push_back ( module );
		module->ProcessXML ( e, path );
		return;
	}
	else if ( e.name == "directory" )
	{
		// this code is duplicated between Project::ProcessXML() and Module::ProcessXML() :(
		const XMLAttribute* att = e.GetAttribute ( "name", true );
		if ( !att )
			return;
		subpath = path + "/" + att->value;
	}
	for ( size_t i = 0; i < e.subElements.size(); i++ )
		ProcessXML ( *e.subElements[i], subpath );
}

int
main ( int argc, char** argv )
{
	// store the current directory for path calculations
	working_directory.resize ( _MAX_PATH );
	working_directory[0] = 0;
	getcwd ( &working_directory[0], working_directory.size() );
	working_directory.resize ( strlen ( working_directory.c_str() ) );

	XMLFile f;
	Path path;
	string xml_file ( "ReactOS.xml" );
	if ( !f.open ( xml_file ) )
	{
		printf ( "couldn't open ReactOS.xml!\n" );
		return -1;
	}

	vector<string> xml_dependencies;
	xml_dependencies.push_back ( xml_file );
	for ( ;; )
	{
		XMLElement* head = XMLParse ( f, path );
		if ( !head )
			break; // end of file

		if ( head->name == "!--" )
			continue; // ignore comments

		if ( head->name != "project" )
		{
			printf ( "error: expecting 'project', got '%s'\n", head->name.c_str() );
			continue;
		}

		Project* proj = new Project;
		proj->ProcessXML ( *head, "." );

		// REM TODO FIXME actually do something with Project object...
		printf ( "Found %d modules:\n", proj->modules.size() );
		for ( size_t i = 0; i < proj->modules.size(); i++ )
		{
			Module& m = *proj->modules[i];
			printf ( "\t%s in folder: %s\n",
			         m.name.c_str(),
			         m.path.c_str() );
			printf ( "\txml dependencies:\n\t\tReactOS.xml\n" );
			const XMLElement* e = &m.node;
			while ( e )
			{
				if ( e->name == "xi:include" )
				{
					const XMLAttribute* att = e->GetAttribute("top_href",false);
					if ( att )
					{
						printf ( "\t\t%s\n", att->value.c_str() );
					}
				}
				e = e->parentElement;
			}
			printf ( "\tfiles:\n" );
			for ( size_t j = 0; j < m.files.size(); j++ )
			{
				printf ( "\t\t%s\n", m.files[j]->name.c_str() );
			}
		}

		delete proj;
		delete head;
	}

	return 0;
}
