
#include "pch.h"
#include <assert.h>

#include "rbuild.h"

using std::string;
using std::vector;

Project::Project ( const string& filename )
	: xmlfile (filename),
	  node (NULL),
	  head (NULL)
{
	ReadXml();
}

Project::~Project ()
{
	size_t i;
	for ( i = 0; i < modules.size (); i++ )
		delete modules[i];
	for ( i = 0; i < includes.size (); i++ )
		delete includes[i];
	for ( i = 0; i < defines.size (); i++ )
		delete defines[i];
	for ( i = 0; i < linkerFlags.size (); i++ )
		delete linkerFlags[i];
	for ( i = 0; i < properties.size (); i++ )
		delete properties[i];
	for ( i = 0; i < ifs.size (); i++ )
		delete ifs[i];
	delete head;
}

const Property*
Project::LookupProperty ( const string& name ) const
{
	for ( size_t i = 0; i < properties.size (); i++ )
	{
		const Property* property = properties[i];
		if ( property->name == name )
			return property;
	}
	return NULL;
}

void
Project::WriteIfChanged ( char* outbuf,
	                      string filename )
{
	FILE* out;
	unsigned int end;
	char* cmpbuf;
	unsigned int stat;
	
	out = fopen ( filename.c_str (), "rb" );
	if ( out == NULL )
	{
		out = fopen ( filename.c_str (), "wb" );
		if ( out == NULL )
			throw AccessDeniedException ( filename );
		fputs ( outbuf, out );
		fclose ( out );
		return;
	}
	
	fseek ( out, 0, SEEK_END );
	end = ftell ( out );
	cmpbuf = (char*) malloc ( end );
	if ( cmpbuf == NULL )
	{
		fclose ( out );
		throw OutOfMemoryException ();
	}
	
	fseek ( out, 0, SEEK_SET );
	stat = fread ( cmpbuf, 1, end, out );
	if ( stat != end )
	{
		free ( cmpbuf );
		fclose ( out );
		throw AccessDeniedException ( filename );
	}
	if ( end == strlen ( outbuf ) && memcmp ( cmpbuf, outbuf, end ) == 0 )
	{
		free ( cmpbuf );
		fclose ( out );
		return;
	}
	
	free ( cmpbuf );
	fclose ( out );
	out = fopen ( filename.c_str (), "wb" );
	if ( out == NULL )
	{
		throw AccessDeniedException ( filename );
	}
	
	stat = fwrite ( outbuf, 1, strlen ( outbuf ), out);
	if ( strlen ( outbuf ) != stat )
	{
		fclose ( out );
		throw AccessDeniedException ( filename );
	}

	fclose ( out );
}

void
Project::SetConfigurationOption ( char* s,
	                              string name,
	                              string* alternativeName )
{
	const Property* property = LookupProperty ( name );
	if ( property != NULL && property->value.length () > 0 )
	{
		s = s + sprintf ( s,
		                  "#define %s=%s\n",
		                  property->name.c_str (),
		                  property->value.c_str () );
	}
	else if ( property != NULL )
	{
		s = s + sprintf ( s,
		                  "#define %s\n",
		                  property->name.c_str () );
	}
	else if ( alternativeName != NULL )
	{
		s = s + sprintf ( s,
		                  "#define %s\n",
		                  alternativeName->c_str () );
	}
}

void
Project::SetConfigurationOption ( char* s,
	                              string name )
{
	SetConfigurationOption ( s, name, NULL );
}

void
Project::WriteConfigurationFile ()
{
	char* buf;
	char* s;

	buf = (char*) malloc ( 10*1024 );
	if ( buf == NULL )
		throw OutOfMemoryException ();
	
	s = buf;
	s = s + sprintf ( s, "/* Automatically generated. " );
	s = s + sprintf ( s, "Edit config.xml to change configuration */\n" );
	s = s + sprintf ( s, "#ifndef __INCLUDE_CONFIG_H\n" );
	s = s + sprintf ( s, "#define __INCLUDE_CONFIG_H\n" );

	SetConfigurationOption ( s, "ARCH" );
	SetConfigurationOption ( s, "OPTIMIZED" );
	SetConfigurationOption ( s, "MP", new string ( "UP" ) );
	SetConfigurationOption ( s, "ACPI" );
	SetConfigurationOption ( s, "_3GB" );

	s = s + sprintf ( s, "#endif /* __INCLUDE_CONFIG_H */\n" );

	WriteIfChanged ( buf, "include" SSEP "roscfg.h" );

	free ( buf );
}

void
Project::ExecuteInvocations ()
{
	for ( size_t i = 0; i < modules.size (); i++ )
		modules[i]->InvokeModule ();
}

void
Project::ReadXml ()
{
	Path path;
	head = XMLLoadFile ( xmlfile, path );
	node = NULL;
	for ( size_t i = 0; i < head->subElements.size (); i++ )
	{
		if ( head->subElements[i]->name == "project" )
		{
			node = head->subElements[i];
			this->ProcessXML ( "." );
			return;
		}
	}

	throw InvalidBuildFileException (
		node->location,
		"Document contains no 'project' tag." );
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
	for ( i = 0; i < linkerFlags.size(); i++ )
		linkerFlags[i]->ProcessXML();
	for ( i = 0; i < properties.size(); i++ )
		properties[i]->ProcessXML();
	for ( i = 0; i < ifs.size(); i++ )
		ifs[i]->ProcessXML();
}

void
Project::ProcessXMLSubElement ( const XMLElement& e,
                                const string& path,
                                If* pIf /*= NULL*/ )
{
	bool subs_invalid = false;
	string subpath(path);
	if ( e.name == "module" )
	{
		if ( pIf )
			throw InvalidBuildFileException (
				e.location,
				"<module> is not a valid sub-element of <if>" );
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
		Include* include = new Include ( *this, e );
		if ( pIf )
			pIf->includes.push_back ( include );
		else
			includes.push_back ( include );
		subs_invalid = true;
	}
	else if ( e.name == "define" )
	{
		Define* define = new Define ( *this, e );
		if ( pIf )
			pIf->defines.push_back ( define );
		else
			defines.push_back ( define );
		subs_invalid = true;
	}
	else if ( e.name == "linkerflag" )
	{
		linkerFlags.push_back ( new LinkerFlag ( *this, e ) );
		subs_invalid = true;
	}
	else if ( e.name == "if" )
	{
		If* pOldIf = pIf;
		pIf = new If ( e, *this, NULL );
		if ( pOldIf )
			pOldIf->ifs.push_back ( pIf );
		else
			ifs.push_back ( pIf );
		subs_invalid = false;
	}
	else if ( e.name == "property" )
	{
		Property* property = new Property ( e, *this, NULL );
		if ( pIf )
			pIf->properties.push_back ( property );
		else
			properties.push_back ( property );
	}
	if ( subs_invalid && e.subElements.size() )
		throw InvalidBuildFileException (
			e.location,
			"<%s> cannot have sub-elements",
			e.name.c_str() );
	for ( size_t i = 0; i < e.subElements.size (); i++ )
		ProcessXMLSubElement ( *e.subElements[i], subpath, pIf );
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
		if ( modules[i]->name == name )
			return modules[i];
	}

	return NULL;
}
