// rbuild.cpp

#ifdef _MSC_VER
#pragma warning ( disable : 4786 ) // identifier was truncated to '255' characters in the debug information
#endif//_MSC_VER

#include <stdio.h>
#include <io.h>
#include <assert.h>
#include "rbuild.h"

using std::string;
using std::vector;

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
	InitWorkingDirectory();

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
