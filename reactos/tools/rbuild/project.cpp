
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
	delete node;
}

void
Project::ReadXml ()
{
	Path path;

	do
	{
		node = XMLParse ( xmlfile, path );
		if ( !node )
			throw InvalidBuildFileException (
				node->location,
				"Document contains no 'project' tag." );
	} while ( node->name != "project" );

	this->ProcessXML ( "." );
}

void
Project::ProcessXML ( const string& path )
{
	const XMLAttribute *att;
	if ( node->name != "project" )
		throw Exception ( "internal tool error: Project::ProcessXML() called with non-<project> node" );

	att = node->GetAttribute ( "name", false );
	if ( !att )
		name = "Unnamed";
	else
		name = att->value;

	att = node->GetAttribute ( "makefile", true );
	assert(att);
	makefile = att->value;

	size_t i;
	for ( i = 0; i < node->subElements.size(); i++ )
		ProcessXMLSubElement ( *node->subElements[i], path );
	for ( i = 0; i < modules.size(); i++ )
		modules[i]->ProcessXML();
	for ( i = 0; i < includes.size(); i++ )
		includes[i]->ProcessXML();
	for ( i = 0; i < defines.size(); i++ )
		defines[i]->ProcessXML();
}

void
Project::ProcessXMLSubElement ( const XMLElement& e, const string& path )
{
	bool subs_invalid = false;
	string subpath(path);
	if ( e.name == "module" )
	{
		Module* module = new Module ( *this, e, path );
		if ( LocateModule ( module->name ) )
			throw InvalidBuildFileException (
				node->location,
				"module name conflict: '%s' (originally defined at %s)",
				module->name.c_str(),
				module->node.location.c_str() );
		modules.push_back ( module );
		return; // defer processing until later
	}
	else if ( e.name == "directory" )
	{
		const XMLAttribute* att = e.GetAttribute ( "name", true );
		assert(att);
		subpath = path + CSEP + att->value;
	}
	else if ( e.name == "include" )
	{
		includes.push_back ( new Include ( *this, e ) );
		subs_invalid = true;
	}
	else if ( e.name == "define" )
	{
		defines.push_back ( new Define ( *this, e ) );
		subs_invalid = true;
	}
	if ( subs_invalid && e.subElements.size() )
		throw InvalidBuildFileException (
			e.location,
			"<%s> cannot have sub-elements",
			e.name.c_str() );
	for ( size_t i = 0; i < e.subElements.size (); i++ )
		ProcessXMLSubElement ( *e.subElements[i], subpath );
}

Module*
Project::LocateModule ( const string& name )
{
	for ( size_t i = 0; i < modules.size (); i++ )
	{
		if (modules[i]->name == name)
			return modules[i];
	}

	return NULL;
}

const Module*
Project::LocateModule ( const string& name ) const
{
	for ( size_t i = 0; i < modules.size (); i++ )
	{
		if (modules[i]->name == name)
			return modules[i];
	}

	return NULL;
}
