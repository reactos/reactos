
#include "pch.h"
#include <assert.h>

#include "rbuild.h"

using std::string;
using std::vector;

Project::Project()
{
}

Project::Project ( const string& filename )
{
	if ( !xmlfile.open ( filename ) )
		throw FileNotFoundException ( filename );
	ReadXml();
}

Project::~Project ()
{
	for ( size_t i = 0; i < modules.size (); i++ )
		delete modules[i];
	delete head;
}

void
Project::ReadXml ()
{
	Path path;

	do
	{
		head = XMLParse ( xmlfile, path );
		if ( !head )
			throw InvalidBuildFileException ( "Document contains no 'project' tag." );
	} while ( head->name != "project" );

	this->ProcessXML ( *head, "." );
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

		att = e.GetAttribute ( "makefile", true );
		assert(att);
		makefile = att->value;
	}
	else if ( e.name == "module" )
	{
		att = e.GetAttribute ( "name", true );
		assert(att);
		Module* module = new Module ( this, e, att->value, path );
		modules.push_back ( module );
		module->ProcessXML ( e, path );
		return;
	}
	else if ( e.name == "directory" )
	{
		const XMLAttribute* att = e.GetAttribute ( "name", true );
		assert(att);
		subpath = path + CSEP + att->value;
	}
	for ( size_t i = 0; i < e.subElements.size (); i++ )
		ProcessXML ( *e.subElements[i], subpath );
}

Module*
Project::LocateModule ( string name )
{
	for ( size_t i = 0; i < modules.size (); i++ )
	{
		if (modules[i]->name == name)
			return modules[i];
	}

	return NULL;
}
