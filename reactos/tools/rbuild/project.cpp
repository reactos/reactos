/*
 * Copyright (C) 2005 Casper S. Hornstrup
 * Copyright (C) 2008 Hervé Poussineau
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include "pch.h"
#include <assert.h>

#include "rbuild.h"
#include "backend/backend.h"

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

string
Environment::GetArch ()
{
	return GetEnvironmentVariablePathOrDefault ( "ROS_ARCH", "i386" );
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
	string defaultIntermediate =
		string( "obj-" ) + GetArch ();
	return GetEnvironmentVariablePathOrDefault ( "ROS_INTERMEDIATE",
	                                             defaultIntermediate );
}

/* static */ string
Environment::GetOutputPath ()
{
	string defaultOutput =
		string( "output-" ) + GetArch ();
	return GetEnvironmentVariablePathOrDefault ( "ROS_OUTPUT",
	                                             defaultOutput );
}

/* static */ string
Environment::GetInstallPath ()
{
	string defaultInstall = GetCdOutputPath ();
	return GetEnvironmentVariablePathOrDefault ( "ROS_INSTALL",
	                                             defaultInstall );
}

/* static */ string
Environment::GetCdOutputPath ()
{
	return GetEnvironmentVariablePathOrDefault ( "ROS_CDOUTPUT",
	                                             "reactos" );
}

/* static */ string
Environment::GetAutomakeFile ( const std::string& defaultFile )
{
	return GetEnvironmentVariablePathOrDefault ( "ROS_AUTOMAKE",
	                                             defaultFile );
}

ParseContext::ParseContext ()
	: compilationUnit (NULL)
{
}


FileLocation::FileLocation ( const DirectoryLocation directory,
                             const std::string& relative_path,
                             const std::string& name,
                             const XMLElement *node )
	: directory ( directory ),
	  relative_path ( NormalizeFilename ( relative_path ) ),
	  name ( name )
{
	if ( relative_path[0] == '/' ||
	     relative_path[0] == '\\' ||
	     relative_path.find ( '$' ) != string::npos ||
	     ( relative_path.length () > 1 && ( relative_path[1] == ':' ||
	                                        relative_path.find_last_of ( "/\\" ) == relative_path.length () - 1 ) ) ||
	     ( relative_path.length () > 3 && relative_path.find ( ':' ) != string::npos )
	     )
	{
		if ( node )
			throw InvalidOperationException ( __FILE__,
			                                  __LINE__,
			                                  "Invalid relative path '%s' at %s",
			                                  relative_path.c_str (),
			                                  node->location.c_str () );
		else
			throw InvalidOperationException ( __FILE__,
			                                  __LINE__,
			                                  "Invalid relative path '%s'",
			                                  relative_path.c_str () );
	}

	if ( name.find_first_of ( "/\\:" ) != string::npos )
	{
		if ( node )
			throw InvalidOperationException ( __FILE__,
			                                  __LINE__,
			                                  "Invalid file name '%s' at %s",
			                                  name.c_str (),
			                                  node->location.c_str () );
		else
			throw InvalidOperationException ( __FILE__,
			                                  __LINE__,
			                                  "Invalid file name '%s'",
			                                  name.c_str () );
	}
}


FileLocation::FileLocation ( const FileLocation& other )
	: directory ( other.directory ),
	  relative_path ( other.relative_path ),
	  name ( other.name )
{
}


Project::Project ( const Configuration& configuration,
                   const string& filename,
                   const std::map<std::string, std::string>* properties )
	: xmlfile (filename),
	  node (NULL),
	  head (NULL),
	  configuration (configuration)
{
	_backend = NULL;

	if ( properties )
	{
		std::map<string, string>::const_iterator it;
		for (it = properties->begin (); it != properties->end (); it++)
		{
			const Property *existing = LookupProperty( it->first );
			if ( !existing )
			{
				Property* property = new Property ( *this, NULL, it->first, it->second );
				non_if_data.properties.insert ( std::make_pair ( property->name, property ) );
			}
		}
	}

	ReadXml();
}

Project::~Project ()
{
	size_t i;
	if ( _backend )
		delete _backend;
#ifdef NOT_NEEDED_SINCE_THESE_ARE_CLEANED_BY_IFABLE_DATA
	for ( i = 0; i < modules.size (); i++ )
		delete modules[i];
#endif
	for ( i = 0; i < linkerFlags.size (); i++ )
		delete linkerFlags[i];
	for ( i = 0; i < cdfiles.size (); i++ )
		delete cdfiles[i];
	for ( i = 0; i < installfiles.size (); i++ )
		delete installfiles[i];
	if ( head )
		delete head;
}

const Property*
Project::LookupProperty ( const string& name ) const
{
	std::map<std::string, Property*>::const_iterator p = non_if_data.properties.find(name);

	if ( p == non_if_data.properties.end () )
		return NULL;

	return p->second;
}

string
Project::ResolveNextProperty ( const string& s ) const
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
				return string ( s ).replace ( i, propertyNameLength + 3, property->value );
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
Project::ExecuteInvocations ()
{
	for( std::map<std::string, Module*>::const_iterator p = modules.begin(); p != modules.end(); ++ p )
		p->second->InvokeModule ();
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
			ProcessXML ( path );
			return;
		}
	}

	if (node == NULL)
		node = head->subElements[0];

	throw XMLInvalidBuildFileException (
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
	makefile = Environment::GetAutomakeFile ( att->value );

	att = node->GetAttribute ( "allowwarnings", false );
	allowWarningsSet = att != NULL;
	if ( att != NULL )
		allowWarnings = att->value == "true";

	size_t i;
	for ( i = 0; i < node->subElements.size (); i++ )
	{
		ParseContext parseContext;
		ProcessXMLSubElement ( *node->subElements[i], path, parseContext );
	}

	non_if_data.ProcessXML ();
	host_non_if_data.ProcessXML ();

	non_if_data.ExtractModules( modules );

	for ( i = 0; i < linkerFlags.size (); i++ )
		linkerFlags[i]->ProcessXML ();
	for( std::map<std::string, Module*>::const_iterator p = modules.begin(); p != modules.end(); ++ p )
		p->second->ProcessXML ();
	for ( i = 0; i < cdfiles.size (); i++ )
		cdfiles[i]->ProcessXML ();
	for ( i = 0; i < installfiles.size (); i++ )
		installfiles[i]->ProcessXML ();
}

void
Project::ProcessXMLSubElement ( const XMLElement& e,
                                const string& path,
                                ParseContext& parseContext )
{
	const XMLAttribute* att;

	att = e.GetAttribute ( "compilerset", false );

	if ( att )
	{
		CompilerSet compilerSet;

		if ( att->value == "msc" )
			compilerSet = MicrosoftC;
		else if ( att->value == "gcc" )
			compilerSet = GnuGcc;
		else
			throw InvalidAttributeValueException (
				e.location,
				"compilerset",
				att->value );

		if ( compilerSet != configuration.Compiler )
			return;
	}

	att = e.GetAttribute ( "linkerset", false );

	if ( att )
	{
		LinkerSet linkerSet;

		if ( att->value == "mslink" )
			linkerSet = MicrosoftLink;
		else if ( att->value == "ld" )
			linkerSet = GnuLd;
		else
			throw InvalidAttributeValueException (
				e.location,
				"linkerset",
				att->value );

		if ( linkerSet != configuration.Linker )
			return;
	}

	bool subs_invalid = false;

	string subpath(path);
	if ( e.name == "module" )
	{
		Module* module = new Module ( *this, e, path );
		if ( LocateModule ( module->name ) )
			throw XMLInvalidBuildFileException (
				node->location,
				"module name conflict: '%s' (originally defined at %s)",
				module->name.c_str(),
				module->node.location.c_str() );
		non_if_data.modules.push_back ( module );
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
		subpath = GetSubPath ( *this, e.location, path, att->value );
	}
	else if ( e.name == "include" )
	{
		const XMLAttribute* host = e.GetAttribute("host", false);
		Include* include = new Include ( *this, &e );

		if(host && host->value == "true")
			host_non_if_data.includes.push_back(include);
		else
			non_if_data.includes.push_back ( include );

		subs_invalid = true;
	}
	else if ( e.name == "define" || e.name == "redefine" )
	{
		const XMLAttribute* host = e.GetAttribute("host", false);
		Define* define = new Define ( *this, e );

		if(host && host->value == "true")
			host_non_if_data.defines.push_back(define);
		else
			non_if_data.defines.push_back ( define );

		subs_invalid = true;
	}
	else if ( e.name == "compilerflag" )
	{
		CompilerFlag* pCompilerFlag = new CompilerFlag ( *this, e );
		non_if_data.compilerFlags.push_back ( pCompilerFlag );
		subs_invalid = true;
	}
	else if ( e.name == "linkerflag" )
	{
		linkerFlags.push_back ( new LinkerFlag ( *this, e ) );
		subs_invalid = true;
	}
	else if ( e.name == "if" || e.name == "ifnot" )
	{
		const XMLAttribute* name;
		name = e.GetAttribute ( "property", true );
		assert( name );
		const Property *property = LookupProperty( name->value );
		const string *PropertyValue;
		const string EmptyString;

		if (property)
		{
			PropertyValue = &property->value;
		}
		else
		{
			// Property does not exist, treat it as being empty
			PropertyValue = &EmptyString;
		}

		const XMLAttribute* value;
		value = e.GetAttribute ( "value", true );
		assert( value );

		bool negate = ( e.name == "ifnot" );
		bool equality = ( *PropertyValue == value->value );
		if ( equality == negate )
		{
			// Failed, skip this element
			if ( configuration.Verbose )
				printf("Skipping 'If' at %s\n", e.location.c_str () );
			return;
		}
		subs_invalid = false;
	}
	else if ( e.name == "property" )
	{
		Property* property = new Property ( e, *this, NULL );
		non_if_data.properties.insert ( std::make_pair ( property->name, property ) );
	}
	if ( subs_invalid && e.subElements.size() )
	{
		throw XMLInvalidBuildFileException (
			e.location,
			"<%s> cannot have sub-elements",
			e.name.c_str() );
	}
	for ( size_t i = 0; i < e.subElements.size (); i++ )
		ProcessXMLSubElement ( *e.subElements[i], subpath, parseContext );
}

Module*
Project::LocateModule ( const string& name )
{
	std::map<std::string, Module*>::const_iterator p = modules.find(name);

	if ( p == modules.end() )
		return NULL;

	return p->second;
}

const Module*
Project::LocateModule ( const string& name ) const
{
	std::map<std::string, Module*>::const_iterator p = modules.find(name);

	if ( p == modules.end() )
		return NULL;

	return p->second;
}

const std::string&
Project::GetProjectFilename () const
{
	return xmlfile;
}

std::string
Project::GetCompilerSet () const
{
	switch ( configuration.Compiler )
	{
	case GnuGcc: return "gcc";
	case MicrosoftC: return "msc";
	default: assert ( false );
	}
}

std::string
Project::GetLinkerSet () const
{
	switch ( configuration.Linker )
	{
	case GnuLd: return "ld";
	case MicrosoftLink: return "mslink";
	default: assert ( false );
	}
}
