/*
 * Copyright (C) 2005 Casper S. Hornstrup
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

using std::string;
using std::vector;

WineResource::WineResource ( const Project& project,
                             string bin2res )
	: project ( project ),
	  bin2res ( bin2res )
{
}

WineResource::~WineResource ()
{
}

bool
WineResource::IsSpecFile ( const File& file )
{
	string extension = GetExtension ( file.file.name );
	if ( extension == ".spec" || extension == ".SPEC" )
		return true;
	return false;
}

bool
WineResource::IsWineModule ( const Module& module )
{
	const vector<File*>& files = module.non_if_data.files;
	for ( size_t i = 0; i < files.size (); i++ )
	{
		if ( IsSpecFile ( *files[i] ) )
			return true;
	}
	return false;
}

bool
WineResource::IsResourceFile ( const File& file )
{
	string extension = GetExtension ( file.file.name );
	if ( extension == ".rc" || extension == ".RC" )
		return true;
	return false;
}

string
WineResource::GetResourceFilename ( const Module& module )
{
	const vector<File*>& files = module.non_if_data.files;
	for ( size_t i = 0; i < files.size (); i++ )
	{
		if ( IsResourceFile ( *files[i] ) )
			return files[i]->file.relative_path + sSep + files[i]->file.name;
	}
	return "";
}

void
WineResource::UnpackResources ( bool verbose )
{
	for ( size_t i = 0; i < project.modules.size (); i++ )
	{
		if ( IsWineModule ( *project.modules[i] ) )
		{
			UnpackResourcesInModule ( *project.modules[i],
			                          verbose );
		}
	}
}

void
WineResource::UnpackResourcesInModule ( Module& module,
                                        bool verbose )
{
	string resourceFilename = GetResourceFilename ( module );
	if ( resourceFilename.length () == 0 )
		return;

	if ( verbose )
	{
		printf ( "\nUnpacking resources for %s",
		         module.name.c_str () );
	}

	string relativeDirectory = module.output->relative_path;
	string outputDirectory = Environment::GetIntermediatePath() + sSep + module.output->relative_path;
	string parameters = ssprintf ( "-b %s -O %s -f -x %s",
	                               NormalizeFilename ( relativeDirectory ).c_str (),
	                               NormalizeFilename ( outputDirectory ).c_str (),
	                               NormalizeFilename ( resourceFilename ).c_str () );
	string command = FixSeparatorForSystemCommand(bin2res) + " " + parameters;

	Directory( relativeDirectory ).GenerateTree( Environment::GetIntermediatePath(), false );

	int exitcode = system ( command.c_str () );
	if ( exitcode != 0 )
	{
		throw InvocationFailedException ( command,
		                                  exitcode );
	}
	module.non_if_data.includes.push_back( new Include ( module.project,
	                                                     IntermediateDirectory,
	                                                     module.output->relative_path ) );
}
