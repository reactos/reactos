
#include "pch.h"

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

void Project::ReadXml ()
{
	Path path;

	head = XMLParse ( xmlfile, path );
	if ( !head )
		throw InvalidBuildFileException ( "Document contains no 'project' tag." );

	if ( head->name != "project" )
	{
		throw InvalidBuildFileException ( "Expected 'project', got '%s'.",
			                              head->name.c_str());
	}

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
		makefile = att->value;
	}
	else if ( e.name == "module" )
	{
		att = e.GetAttribute ( "name", true );
		Module* module = new Module ( e, att->value, path );
		modules.push_back ( module );
		module->ProcessXML ( e, path );
		return;
	}
	else if ( e.name == "directory" )
	{
		const XMLAttribute* att = e.GetAttribute ( "name", true );
		subpath = path + "/" + att->value;
	}
	for ( size_t i = 0; i < e.subElements.size (); i++ )
		ProcessXML ( *e.subElements[i], subpath );
}
