/*
 * Copyright (C) 2009 Ged Murphy
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <string>
#include <vector>
#include <set>
#include <algorithm>
#include <fstream>
#include <iostream>

#include <stdio.h>

#include "msvc.h"

using std::string;
using std::vector;
using std::set;

typedef set<string> StringSet;

#ifdef OUT
#undef OUT
#endif//OUT

void
MSVCBackend::_generate_vcxproj ( const Module& module )
{
	size_t i;

	string vcproj_file = VcprojFileName(module);
	string computername;
	string username;
	string intermediatedir = "";

	if (getenv ( "USERNAME" ) != NULL)
		username = getenv ( "USERNAME" );
	if (getenv ( "COMPUTERNAME" ) != NULL)
		computername = getenv ( "COMPUTERNAME" );
	else if (getenv ( "HOSTNAME" ) != NULL)
		computername = getenv ( "HOSTNAME" );

	printf ( "Creating MSVC project: '%s'\n", vcproj_file.c_str() );
	FILE* OUT = fopen ( vcproj_file.c_str(), "wb" );

	string path_basedir = module.GetPathToBaseDir ();
	string intenv = Environment::GetIntermediatePath ();
	string outenv = Environment::GetOutputPath ();
	string outdir;
	string intdir;
	string vcdir;

	if ( intenv == "obj-i386" )
		intdir = path_basedir + "obj-i386"; /* append relative dir from project dir */
	else
		intdir = intenv;

	if ( outenv == "output-i386" )
		outdir = path_basedir + "output-i386";
	else
		outdir = outenv;

	if ( configuration.UseVSVersionInPath )
	{
		vcdir = DEF_SSEP + _get_vc_dir();
	}



	bool include_idl = false;

	vector<string> source_files, resource_files;
	vector<const IfableData*> ifs_list;
	ifs_list.push_back ( &module.project.non_if_data );
	ifs_list.push_back ( &module.non_if_data );

	while ( ifs_list.size() )
	{
		const IfableData& data = *ifs_list.back();
		ifs_list.pop_back();
		const vector<File*>& files = data.files;
		for ( i = 0; i < files.size(); i++ )
		{
			if (files[i]->file.directory != SourceDirectory)
				continue;

			// We want the full path here for directory support later on
			string path = Path::RelativeFromDirectory (
				files[i]->file.relative_path,
				module.output->relative_path );
			string file = path + std::string("\\") + files[i]->file.name;

			if ( !stricmp ( Right(file,3).c_str(), ".rc" ) )
				resource_files.push_back ( file );
			else if ( !stricmp ( Right(file,2).c_str(), ".h" ) )
				header_files.push_back ( file );
			else
				source_files.push_back ( file );
		}
		const vector<Include*>& incs = data.includes;
		for ( i = 0; i < incs.size(); i++ )
		{
			string path = Path::RelativeFromDirectory (
				incs[i]->directory->relative_path,
				module.output->relative_path );
			if ( module.type != RpcServer && module.type != RpcClient )
			{
				if ( path.find ("/include/reactos/idl") != string::npos)
				{
					include_idl = true;
					continue;
				}
			}
			// switch between general headers and ros headers
			if ( !strncmp(incs[i]->directory->relative_path.c_str(), "include\\crt", 11 ) ||
			     !strncmp(incs[i]->directory->relative_path.c_str(), "include\\ddk", 11 ) ||
			     !strncmp(incs[i]->directory->relative_path.c_str(), "include\\GL", 10 ) ||
			     !strncmp(incs[i]->directory->relative_path.c_str(), "include\\psdk", 12 ) ||
			     !strncmp(incs[i]->directory->relative_path.c_str(), "include\\reactos\\wine", 20 ) )
			{
				if (strncmp(incs[i]->directory->relative_path.c_str(), "include\\crt", 11 ))
					// not crt include
					includes_ros.push_back ( path );
			}
			else
			{
				includes.push_back ( path );
			}
		}
		const vector<Library*>& libs = data.libraries;
		for ( i = 0; i < libs.size(); i++ )
		{
			string libpath = outdir + "\\" + libs[i]->importedModule->output->relative_path + "\\" + _get_vc_dir() + "\\---\\" + libs[i]->name + ".lib";
			libraries.push_back ( libpath );
		}
		const vector<Define*>& defs = data.defines;
		for ( i = 0; i < defs.size(); i++ )
		{
			if ( defs[i]->backend != "" && defs[i]->backend != "msvc" )
				continue;

			if ( defs[i]->value[0] )
				common_defines.insert( defs[i]->name + "=" + defs[i]->value );
			else
				common_defines.insert( defs[i]->name );
		}
		for ( std::map<std::string, Property*>::const_iterator p = data.properties.begin(); p != data.properties.end(); ++ p )
		{
			Property& prop = *p->second;
			if ( strstr ( module.baseaddress.c_str(), prop.name.c_str() ) )
				baseaddr = prop.value;
		}
	}
	/* include intermediate path for reactos.rc */
	string version = intdir + "\\include";
	includes.push_back (version);
	version += "\\reactos";
	includes.push_back (version);

	string include_string;

	fprintf ( OUT, "<?xml version=\"1.0\" encoding = \"Windows-1252\"?>\r\n" );
}
