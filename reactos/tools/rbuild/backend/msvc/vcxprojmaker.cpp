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
							 std::string filename,
							 const Module& module)
{
	configuration = buildConfig;
	m_configurations = msvc_configs;
	vcproj_file = filename;

	OUT = fopen ( vcproj_file.c_str(), "wb" );

	if ( !OUT )
	{
		printf ( "Could not create file '%s'.\n", vcproj_file.c_str() );
	}

	// Set the binary type
	string module_type = GetExtension(*module.output);
	
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
}

VCXProjMaker::~VCXProjMaker()
{
	fclose ( OUT );
}

void
VCXProjMaker::_generate_item_group (std::vector<std::string> files)
{
	size_t i;
	
	for( i = 0; i<files.size(); i++)
	{
		std::string extension = ToLower(GetExtension(files[i]));

		if( extension == ".c" || extension == ".cpp") 
			fprintf ( OUT, "\t\t<ClCompile Include=\"%s\" />\r\n", files[i].c_str());
		else if( extension == ".s")
			fprintf ( OUT, "\t\t<s_as_mscpp Include=\"%s\" />\r\n", files[i].c_str());
		else if( extension == ".spec")
			fprintf ( OUT, "\t\t<spec Include=\"%s\" />\r\n", files[i].c_str());
		else if( extension == ".pspec")
			fprintf ( OUT, "\t\t<pspec Include=\"%s\" />\r\n", files[i].c_str());
		else if( extension == ".rc")
			fprintf ( OUT, "\t\t<ResourceCompile Include=\"%s\" />\r\n", files[i].c_str());
		else if( extension == ".h")
			fprintf ( OUT, "\t\t<ClInclude Include=\"%s\" />\r\n", files[i].c_str());
		else
			fprintf ( OUT, "\t\t<None Include=\"%s\" />\r\n", files[i].c_str());
	}
}

string
VCXProjMaker::_get_configuration_type ()
{
	switch (binaryType)
	{
	case Exe:
		return "Application";
	case Dll:
	case Sys:
		return "DynamicLibrary";
	case Lib:
		return "StaticLibrary";
	default:
		return "";
	}
}

void
VCXProjMaker::_generate_proj_file ( const Module& module )
{
	string path_basedir = module.GetPathToBaseDir ();
	size_t i;
	string vcdir;

	if ( configuration.UseVSVersionInPath )
	{
		vcdir = DEF_SSEP + _get_vc_dir();
	}

	printf ( "Creating MSVC project: '%s'\n", vcproj_file.c_str() );

	_collect_files(module);

	fprintf ( OUT, "<?xml version=\"1.0\" encoding = \"utf-8\"?>\r\n" );
	fprintf ( OUT, "<Project " );
	fprintf ( OUT, "DefaultTargets=\"Build\" " ); //FIXME: what's Build??
	fprintf ( OUT, "ToolsVersion=\"4.0\" " ); //version 4 is the one bundled with .net framework 4
	fprintf ( OUT, "xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\r\n" );

	if (configuration.VSProjectVersion.empty())
		configuration.VSProjectVersion = "10.00";

	// Write out the configurations
	fprintf ( OUT, "\t<ItemGroup Label=\"ProjectConfigurations\">\r\n" );
	for ( size_t icfg = 0; icfg < m_configurations.size(); icfg++ )
	{
		const MSVCConfiguration& cfg = *m_configurations[icfg];

		if ( cfg.optimization == RosBuild )
			_generate_makefile_configuration( module, cfg );
		else
			_generate_standard_configuration( module, cfg, binaryType );
	}
	fprintf ( OUT, "\t</ItemGroup>\r\n\r\n" );

	// Write out the global info
	fprintf ( OUT, "\t<PropertyGroup Label=\"Globals\">\r\n" );
	fprintf ( OUT, "\t\t<ProjectGuid>{%s}</ProjectGuid>\r\n", module.guid.c_str() );
	fprintf ( OUT, "\t\t<Keyword>%s</Keyword>\r\n", "Win32Proj" ); //FIXME: Win32Proj???
	fprintf ( OUT, "\t\t<RootNamespace>%s</RootNamespace>\r\n", module.name.c_str() ); //FIXME: shouldn't this be the soltion name?
	fprintf ( OUT, "\t</PropertyGroup>\r\n" );
	fprintf ( OUT, "\r\n" );

	fprintf ( OUT, "\t<PropertyGroup Label=\"Configuration\">\r\n");
	if( binaryType != BinUnknown)
		fprintf ( OUT, "\t\t<ConfigurationType>%s</ConfigurationType>\r\n" , _get_configuration_type().c_str());
	fprintf ( OUT, "\t\t<CharacterSet>%s</CharacterSet>\r\n", module.isUnicode ? "Unicode" : "MultiByte");
	fprintf ( OUT, "\t</PropertyGroup>\r\n");

	fprintf ( OUT, "\t<Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.Default.props\" />\r\n" );
	fprintf ( OUT, "\t<Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.props\" />\r\n" );
	fprintf ( OUT, "\t<ImportGroup Label=\"PropertySheets\">\r\n");
	fprintf ( OUT, "\t\t<Import Project=\"%s\\reactos.props\" />\r\n", path_basedir.c_str());
	fprintf ( OUT, "\t\t<Import Project=\"%s\\tools\\rbuild\\backend\\msvc\\rules\\reactos.defaults.props\" />\r\n", path_basedir.c_str());
	fprintf ( OUT, "\t</ImportGroup>\r\n");

	fprintf ( OUT, "\t<PropertyGroup>\r\n");
	fprintf ( OUT, "\t\t<OutDir>$(RootOutDir)\\%s%s\\$(Configuration)\\</OutDir>\r\n", module.output->relative_path.c_str (), vcdir.c_str ());
	fprintf ( OUT, "\t\t<IntDir>$(RootIntDir)\\%s%s\\$(Configuration)\\</IntDir>\r\n", module.output->relative_path.c_str (), vcdir.c_str ());

	if( includes.size() != 0)
	{
		fprintf( OUT, "\t\t<ProjectIncludes>");
		for ( i = 0; i < includes.size(); i++ )
				fprintf ( OUT, "%s;", includes[i].c_str() );
		fprintf( OUT, "</ProjectIncludes>\r\n");
	}

	if(defines.size() != 0)
	{
		fprintf( OUT, "\t\t<ProjectDefines>");
		for ( i = 0; i < defines.size(); i++ )
				fprintf ( OUT, "%s;", defines[i].c_str() );
		fprintf( OUT, "</ProjectDefines>\r\n");
	}

	fprintf ( OUT, "\t</PropertyGroup>\r\n\r\n");
	
	fprintf ( OUT, "\t<ItemDefinitionGroup>\r\n");
	fprintf ( OUT, "\t\t<ClCompile>\r\n");
	if ( module.cplusplus )
		fprintf ( OUT, "\t\t\t<CompileAs>CompileAsCpp</CompileAs>\r\n");
	fprintf ( OUT, "\t\t</ClCompile>\r\n");
	
	fprintf ( OUT, "\t\t<Link>\r\n");
	if(libraries.size() != 0)
	{
		fprintf ( OUT, "\t\t\t<AdditionalDependencies>");
		for ( i = 0; i < libraries.size(); i++ )
		{
			string libpath = libraries[i].c_str();
			libpath = libpath.erase (0, libpath.find_last_of ("\\") + 1 );
			fprintf ( OUT, "%s;", libpath.c_str() );
		}
		fprintf ( OUT, "%%(AdditionalDependencies)</AdditionalDependencies>\r\n");
	
		fprintf ( OUT, "\t\t\t<AdditionalLibraryDirectories>");
		for ( i = 0; i < libraries.size(); i++ )
		{
			string libpath = libraries[i].c_str();
			libpath = libpath.substr (0, libpath.find_last_of ("\\") );
			fprintf ( OUT, "%s;", libpath.c_str() );
		}
		fprintf ( OUT, "%%(AdditionalLibraryDirectories)</AdditionalLibraryDirectories>\r\n");
	}

	if( module.CRT != "msvcrt")
		fprintf ( OUT, "\t\t\t<IgnoreAllDefaultLibraries>true</IgnoreAllDefaultLibraries>\r\n");

	fprintf ( OUT, "\t\t</Link>\r\n");
	fprintf ( OUT, "\t</ItemDefinitionGroup>\r\n");

	fprintf ( OUT, "\t<ItemGroup>\r\n");
	_generate_item_group(header_files);
	_generate_item_group(source_files);
	_generate_item_group(resource_files);
	_generate_item_group(generated_files);
	fprintf ( OUT, "\t</ItemGroup>\r\n\r\n");

	fprintf ( OUT, "\t<Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.targets\" />\r\n");
	fprintf ( OUT, "\t<Import Project=\"%s\\tools\\rbuild\\backend\\msvc\\rules\\reactos.targets\" />\r\n", path_basedir.c_str());

	fprintf ( OUT, "</Project>\r\n");
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
