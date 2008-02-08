/*
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

#include "../../rbuild.h"
#include "modulehandler.h"
#include "rule.h"

using std::string;

static void
ReplaceVariable( string& str,
                 const string& name,
                 const string& value )
{
	size_t i = str.find ( name );
	while ( i != string::npos )
	{
		str.replace ( i, name.length (), value );
		i = str.find ( name );
	}
}

static std::string
FixString ( const string& str, Backend *backend, const Module& module, const FileLocation *source )
{
	string ret = str;

	if ( source )
	{
		ReplaceVariable ( ret, "$(source_noext)", ReplaceExtension ( backend->GetFullName ( *source ), "" ) );
		ReplaceVariable ( ret, "$(source)", backend->GetFullName ( *source ) );
		ReplaceVariable ( ret, "$(source_dir)", source->relative_path );
		ReplaceVariable ( ret, "$(source_name)", source->name );
		ReplaceVariable ( ret, "$(source_name_noext)", ReplaceExtension ( source->name , "" ) );
		ReplaceVariable ( ret, "$(source_path)", backend->GetFullPath ( *source ) );
	}
	ReplaceVariable ( ret, "$(module_name)", module.name );
	ReplaceVariable ( ret, "$(module_rbuild)", module.xmlbuildFile );
	ReplaceVariable ( ret, "$(module_output)", GetTargetMacro ( module, true ) );
	ReplaceVariable ( ret, "$(SEP)", sSep );

	return ret;
}

Rule::Rule( const std::string& command, const char *generatedFile1, ... )
	: command ( command )
{
	va_list ap;
	const char *s;

	s = generatedFile1;
	va_start ( ap, generatedFile1 );
	while ( s )
	{
		generatedFiles.push_back ( s );
		s = va_arg ( ap, const char* );
	}
	va_end ( ap );
}


void Rule::Execute ( FILE *outputFile,
                     MingwBackend *backend,
                     const Module& module,
                     const FileLocation *source,
                     string_list& clean_files )
{
	string cmd = FixString ( command, backend, module, source );

	fprintf ( outputFile, "%s", cmd.c_str () );

	for ( size_t i = 0; i < generatedFiles.size (); i++ )
	{
		string file = FixString ( generatedFiles[i], backend, module, source );
		if ( file[file.length () - 1] != cSep )
		{
			clean_files.push_back ( file );
			continue;
		}

		if ( file[0] != '$' )
			throw InvalidOperationException ( __FILE__,
			                                  __LINE__,
			                                  "Invalid directory %s.",
			                                  file.c_str () );

		size_t pos = file.find_first_of ( cSep );
		string relative_path = file.substr ( pos + 1, file.length () - pos - 2 );
		string dir = file.substr ( 0, pos );
		if ( dir == "$(INTERMEDIATE)" )
			backend->AddDirectoryTarget ( relative_path, backend->intermediateDirectory );
		else if ( dir == "$(OUTPUT)" )
			backend->AddDirectoryTarget ( relative_path, backend->outputDirectory );
		else if ( dir == "$(INSTALL)" )
			backend->AddDirectoryTarget ( relative_path, backend->installDirectory );
		else
			throw InvalidOperationException ( __FILE__,
			                                  __LINE__,
			                                  "Invalid directory %s.",
			                                  dir.c_str () );
	}
}
