#ifdef _MSC_VER
#pragma warning ( disable : 4786 ) // identifier was truncated to '255' characters in the debug information
#endif//_MSC_VER

#include "rbuild.h"

using std::string;
using std::vector;

Project::Project()
{
}

Project::Project(const string& filename)
{
	if ( !xmlfile.open ( filename ) )
		throw FileNotFoundException ( filename );
	ReadXml();
}

Project::~Project()
{
	for ( size_t i = 0; i < modules.size(); i++ )
		delete modules[i];
}

void Project::ReadXml()
{
	Path path;
	bool projectFound = false;
	do
	{
		XMLElement* head = XMLParse ( xmlfile, path );
		if ( !head )
			throw InvalidBuildFileException ( "Document contains no 'project' tag." );

		if ( head->name == "!--" )
			continue; // ignore comments

		if ( head->name != "project" )
		{
			throw InvalidBuildFileException ( "Expected 'project', got '%s'.",
			                                  head->name.c_str());
		}

		this->ProcessXML ( *head, "." );
		delete head;
		projectFound = true;
	} while (!projectFound);
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
