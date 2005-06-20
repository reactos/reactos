
#include "pch.h"
#include <assert.h>

#include "rbuild.h"

using std::string;
using std::vector;

/* static */ string
Environment::GetVariable ( const string& name )
{
	char* value = getenv ( name.c_str () );
	if ( value != NULL && strlen ( value ) > 0 )
		return ssprintf ( "%s",
		                  value );
	else
		return "";
}

/* static */ string
Environment::GetEnvironmentVariablePathOrDefault ( const string& name,
                                                   const string& defaultValue )
{
	const string& environmentVariableValue = Environment::GetVariable ( name );
	if ( environmentVariableValue.length () > 0 )
		return NormalizeFilename ( environmentVariableValue );
	else
		return defaultValue;
}

/* static */ string
Environment::GetIntermediatePath ()
{
	return GetEnvironmentVariablePathOrDefault ( "ROS_INTERMEDIATE",
	                                             "obj-i386" );
}

/* static */ string
Environment::GetOutputPath ()
{
	return GetEnvironmentVariablePathOrDefault ( "ROS_OUTPUT",
	                                             "output-i386" );
}

/* static */ string
Environment::GetInstallPath ()
{
	return GetEnvironmentVariablePathOrDefault ( "ROS_INSTALL",
	                                             "reactos" );
}


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
	for ( i = 0; i < linkerFlags.size (); i++ )
		delete linkerFlags[i];
	for ( i = 0; i < cdfiles.size (); i++ )
		delete cdfiles[i];
	for ( i = 0; i < installfiles.size (); i++ )
		delete installfiles[i];
	delete head;
}

const Property*
Project::LookupProperty ( const string& name ) const
{
	for ( size_t i = 0; i < non_if_data.properties.size (); i++ )
	{
		const Property* property = non_if_data.properties[i];
		if ( property->name == name )
			return property;
	}
	return NULL;
}

string
Project::ResolveNextProperty ( string& s ) const
{
	size_t i = s.find ( "${" );
	if ( i == string::npos )
		i = s.find ( "$(" );
	if ( i != string::npos )
	{
		string endCharacter;
		if ( s[i + 1] == '{' )
			endCharacter = "}";
		else
			endCharacter = ")";
		size_t j = s.find ( endCharacter );
		if ( j != string::npos )
		{
			int propertyNameLength = j - i - 2;
			string propertyName = s.substr ( i + 2, propertyNameLength );
			const Property* property = LookupProperty ( propertyName );
			if ( property != NULL )
				return s.replace ( i, propertyNameLength + 3, property->value );
		}
	}
	return s;
}

string
Project::ResolveProperties ( const string& s ) const
{
	string s2 = s;
	string s3;
	do
	{
		s3 = s2;
		s2 = ResolveNextProperty ( s3 );
	} while ( s2 != s3 );
	return s2;
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

	FileSupportCode::WriteIfChanged ( buf, "include" SSEP "roscfg.h" );

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
	head = XMLLoadFile ( xmlfile, path, xmlbuildfiles );
	node = NULL;
	for ( size_t i = 0; i < head->subElements.size (); i++ )
	{
		if ( head->subElements[i]->name == "project" )
		{
			node = head->subElements[i];
			string path;
			this->ProcessXML ( path );
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
	for ( i = 0; i < node->subElements.size (); i++ )
		ProcessXMLSubElement ( *node->subElements[i], path );
	for ( i = 0; i < modules.size (); i++ )
		modules[i]->ProcessXML ();
	for ( i = 0; i < linkerFlags.size (); i++ )
		linkerFlags[i]->ProcessXML ();
	non_if_data.ProcessXML ();
	for ( i = 0; i < cdfiles.size (); i++ )
		cdfiles[i]->ProcessXML ();
	for ( i = 0; i < installfiles.size (); i++ )
		installfiles[i]->ProcessXML ();
}

void
Project::ProcessXMLSubElement ( const XMLElement& e,
                                const string& path,
                                If* pIf )
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
	else if ( e.name == "cdfile" )
	{
		CDFile* cdfile = new CDFile ( *this, e, path );
		cdfiles.push_back ( cdfile );
		subs_invalid = true;
	}
	else if ( e.name == "installfile" )
	{
		InstallFile* installfile = new InstallFile ( *this, e, path );
		installfiles.push_back ( installfile );
		subs_invalid = true;
	}
	else if ( e.name == "directory" )
	{
		const XMLAttribute* att = e.GetAttribute ( "name", true );
		assert(att);
		subpath = GetSubPath ( e.location, path, att->value );
	}
	else if ( e.name == "include" )
	{
		Include* include = new Include ( *this, e );
		if ( pIf )
			pIf->data.includes.push_back ( include );
		else
			non_if_data.includes.push_back ( include );
		subs_invalid = true;
	}
	else if ( e.name == "define" )
	{
		Define* define = new Define ( *this, e );
		if ( pIf )
			pIf->data.defines.push_back ( define );
		else
			non_if_data.defines.push_back ( define );
		subs_invalid = true;
	}
	else if ( e.name == "compilerflag" )
	{
		CompilerFlag* pCompilerFlag = new CompilerFlag ( *this, e );
		if ( pIf )
			pIf->data.compilerFlags.push_back ( pCompilerFlag );
		else
			non_if_data.compilerFlags.push_back ( pCompilerFlag );
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
			pOldIf->data.ifs.push_back ( pIf );
		else
			non_if_data.ifs.push_back ( pIf );
		subs_invalid = false;
	}
	else if ( e.name == "ifnot" )
	{
		If* pOldIf = pIf;
		pIf = new If ( e, *this, NULL, true );
		if ( pOldIf )
			pOldIf->data.ifs.push_back ( pIf );
		else
			non_if_data.ifs.push_back ( pIf );
		subs_invalid = false;
	}
	else if ( e.name == "property" )
	{
		Property* property = new Property ( e, *this, NULL );
		if ( pIf )
			pIf->data.properties.push_back ( property );
		else
			non_if_data.properties.push_back ( property );
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

std::string
Project::GetProjectFilename () const
{
	return xmlfile;
}

	
