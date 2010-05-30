/*
 * Copyright (C) 2002 Patrik Stridvall
 * Copyright (C) 2005 Royce Mitchell III
 * Copyright (C) 2006 Hervé Poussineau
 * Copyright (C) 2006 Christoph von Wittich
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

#ifdef _MSC_VER
#pragma warning ( disable : 4786 )
#endif//_MSC_VER

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


VCXProjMaker::VCXProjMaker ( )
{
	vcproj_file = "";
}

VCXProjMaker::VCXProjMaker ( Configuration& buildConfig,
							 const std::vector<MSVCConfiguration*>& msvc_configs,
							 std::string filename )
{
	configuration = buildConfig;
	m_configurations = msvc_configs;
	vcproj_file = filename;

	OUT = fopen ( vcproj_file.c_str(), "wb" );

	if ( !OUT )
	{
		printf ( "Could not create file '%s'.\n", vcproj_file.c_str() );
	}
}

VCXProjMaker::~VCXProjMaker()
{
	fclose ( OUT );
}

void
VCXProjMaker::_generate_proj_file ( const Module& module )
{
	size_t i;

	string computername;
	string username;

	// make sure the containers are empty
	header_files.clear();
	includes.clear();
	includes_ros.clear();
	libraries.clear();
	common_defines.clear();

	if (getenv ( "USERNAME" ) != NULL)
		username = getenv ( "USERNAME" );
	if (getenv ( "COMPUTERNAME" ) != NULL)
		computername = getenv ( "COMPUTERNAME" );
	else if (getenv ( "HOSTNAME" ) != NULL)
		computername = getenv ( "HOSTNAME" );

	string vcproj_file_user = "";

	if ((computername != "") && (username != ""))
		vcproj_file_user = vcproj_file + "." + computername + "." + username + ".user";

	printf ( "Creating MSVC project: '%s'\n", vcproj_file.c_str() );

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

	// Set the binary type
	string module_type = GetExtension(*module.output);
	BinaryType binaryType;
	if ((module.type == ObjectLibrary) || (module.type == RpcClient) ||(module.type == RpcServer) || (module_type == ".lib") || (module_type == ".a"))
		binaryType = Lib;
	else if ((module_type == ".dll") || (module_type == ".cpl"))
		binaryType = Dll;
	else if ((module_type == ".exe") || (module_type == ".scr"))
		binaryType = Exe;
	else if (module_type == ".sys")
		binaryType = Sys;
	else
		binaryType = BinUnknown;

	string include_string;

	fprintf ( OUT, "<?xml version=\"1.0\" encoding = \"utf-8\"?>\r\n" );
	fprintf ( OUT, "<Project " );
	fprintf ( OUT, "DefaultTargets=\"Build\" " ); //FIXME: what's Build??
	fprintf ( OUT, "ToolsVersion=\"4.0\" " ); //FIXME: Is it always 4.0??
	fprintf ( OUT, "xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\r\n" );

	if (configuration.VSProjectVersion.empty())
		configuration.VSProjectVersion = "10.00";

	// Write out the configurations
	fprintf ( OUT, "\t<ItemGroup Label=\"ProjectConfigurations\">\r\n" );
	for ( size_t icfg = 0; icfg < m_configurations.size(); icfg++ )
	{
		const MSVCConfiguration& cfg = *m_configurations[icfg];

		if ( cfg.optimization == RosBuild )
		{
			_generate_makefile_configuration( module, cfg );
		}
		else
		{
			_generate_standard_configuration( module, cfg, binaryType );
		}
	}
	fprintf ( OUT, "\t</ItemGroup>\r\n" );

	// Write out the global info
	fprintf ( OUT, "\t<PropertyGroup Label=\"Globals\">\r\n" );
	fprintf ( OUT, "\t\t<ProjectGuid>{%s}</ProjectGuid>\r\n", module.guid.c_str() );
	fprintf ( OUT, "\t\t<Keyword>%s</Keyword>\r\n", "Win32Proj" ); //FIXME: Win32Proj???
	fprintf ( OUT, "\t\t<RootNamespace>%s</RootNamespace>\r\n", module.name.c_str() ); //FIXME: shouldn't this be the soltion name?
	fprintf ( OUT, "\t</PropertyGroup>\r\n" );
	fprintf ( OUT, "</Project>" );


}

void
VCXProjMaker::_generate_user_configuration ()
{
	// Call base implementation
	ProjMaker::_generate_user_configuration ();
}

void
VCXProjMaker::_generate_standard_configuration( const Module& module, const MSVCConfiguration& cfg, BinaryType binaryType )
{
	fprintf ( OUT, "\t\t<ProjectConfiguration Include=\"%s|Win32\">\r\n", cfg.name.c_str() );
	fprintf ( OUT, "\t\t<Configuration>%s</Configuration>\r\n", cfg.name.c_str() );
	fprintf ( OUT, "\t\t<Platform>Win32</Platform>\r\n" );
	fprintf ( OUT, "\t</ProjectConfiguration>\r\n" );
}

void
VCXProjMaker::_generate_makefile_configuration( const Module& module, const MSVCConfiguration& cfg )
{
	// TODO: Implement me
	ProjMaker::_generate_makefile_configuration ( module, cfg );
}
