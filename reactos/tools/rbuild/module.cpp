// module.cpp

#include "pch.h"

#include "rbuild.h"

using std::string;
using std::vector;

Module::Module ( const XMLElement& moduleNode,
                 const string& moduleName,
                 const string& modulePath)
	: node(moduleNode),
	  name(moduleName),
	  path(modulePath)
{
}

Module::~Module()
{
	for ( size_t i = 0; i < files.size(); i++ )
		delete files[i];
}

void
Module::ProcessXML ( const XMLElement& e, const string& path )
{
	string subpath ( path );
	if ( e.name == "file" && e.value.size() )
	{
		files.push_back ( new File(path + "/" + e.value) );
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

File::File ( const std::string& _name )
	: name(_name)
{
}
