// module.cpp

#include "pch.h"
#include <assert.h>

#include "rbuild.h"

using std::string;
using std::vector;

#ifdef WIN32
#define EXEPOSTFIX ".exe"
#define SEP "\\"
string
FixSeparator ( const string& s )
{
	string s2(s);
	char* p = strchr ( &s2[0], '/' );
	while ( p )
	{
		*p++ = '\\';
		p = strchr ( p, '/' );
	}
	return s2;
}
#else
#define EXEPOSTFIX
#define SEP "/"
string
FixSeparator ( const string& s )
{
	string s2(s);
	char* p = strchr ( &s2[0], '\\' );
	while ( p )
	{
		*p++ = '/';
		p = strchr ( p, '\\' );
	}
	return s2;
}
#endif

Module::Module ( const XMLElement& moduleNode,
                 const string& moduleName,
                 const string& modulePath )
	: node(moduleNode),
	  name(moduleName),
	  path(modulePath)
{
	type = GetModuleType ( *moduleNode.GetAttribute ( "type", true ) );
}

Module::~Module ()
{
	size_t i;
	for ( i = 0; i < files.size(); i++ )
		delete files[i];
	for ( i = 0; i < libraries.size(); i++ )
		delete libraries[i];
}

void
Module::ProcessXML ( const XMLElement& e,
                     const string& path )
{
	string subpath ( path );
	if ( e.name == "file" && e.value.size () )
	{
		files.push_back ( new File ( path + "/" + e.value ) );
	}
	else if ( e.name == "library" && e.value.size () )
	{
		libraries.push_back ( new Library ( e.value ) );
	}
	else if ( e.name == "directory" )
	{
		const XMLAttribute* att = e.GetAttribute ( "name", true );
		assert(att);
		subpath = path + "/" + att->value;
	}
	for ( size_t i = 0; i < e.subElements.size (); i++ )
		ProcessXML ( *e.subElements[i], subpath );
}

ModuleType
Module::GetModuleType ( const XMLAttribute& attribute )
{
	if ( attribute.value == "buildtool" )
		return BuildTool;
	if ( attribute.value == "kernelmodedll" )
		return KernelModeDLL;
	throw InvalidAttributeValueException ( attribute.name,
	                                       attribute.value );
}

string
Module::GetPath ()
{
	return FixSeparator (path) + SEP + name + EXEPOSTFIX;
}


File::File ( const string& _name )
	: name(_name)
{
}


Library::Library ( const string& _name )
	: name(_name)
{
}
