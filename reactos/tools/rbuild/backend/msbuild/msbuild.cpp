/*
 * Copyright (C) 2007 Christoph von Wittich
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
#ifdef _MSC_VER
#pragma warning ( disable : 4786 )
#endif//_MSC_VER

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include <stdio.h>

#include "msbuild.h"
#include "../mingw/mingw.h"

using std::string;
using std::vector;
using std::map;
using std::ifstream;

#ifdef OUT
#undef OUT
#endif//OUT


static class MsBuildFactory : public Backend::Factory
{
	public:

		MsBuildFactory() : Factory("MsBuild", "Ms Build") {}
		Backend *operator() (Project &project,
		                     Configuration& configuration)
		{
			return new MsBuildBackend(project, configuration);
		}
		
} factory;


MsBuildBackend::MsBuildBackend(Project &project,
	Configuration& configuration) : Backend(project, configuration)
{

}

void MsBuildBackend::Process()
{
	if ( configuration.CleanAsYouGo ) {
		_clean_project_files();
		return;
	}

	ProcessModules();
}

void
MsBuildBackend::_generate_makefile ( const Module& module )
{
	string makefile = module.output->relative_path + "\\makefile";
	FILE* OUT = fopen ( makefile.c_str(), "wb" );
	fprintf ( OUT, "!INCLUDE $(NTMAKEENV)\\makefile.def\r\n" );
	fclose ( OUT );
}

void
MsBuildBackend::_generate_sources ( const Module& module )
{
	size_t i;

	string module_type = GetExtension(*module.output);
	vector<string> source_files, resource_files, includes, libraries;
	vector<string> header_files, common_defines, compiler_flags;
	vector<string> vars, values;
	string sourcesfile = module.output->relative_path + "\\sources";
	string proj_path = module.output->relative_path;

	FILE* OUT = fopen ( sourcesfile.c_str(), "wb" );
	fprintf ( OUT, "TARGETNAME=%s\r\n", module.name.c_str() );

	vector<const IfableData*> ifs_list;
	ifs_list.push_back ( &module.project.non_if_data );
	ifs_list.push_back ( &module.non_if_data );
	while ( ifs_list.size() )
	{
		const IfableData& data = *ifs_list.back();
		ifs_list.pop_back();
		for ( i = 0; i < data.ifs.size(); i++ )
		{
			const Property* property = _lookup_property( module, data.ifs[i]->property );
			if ( property != NULL )
			{
				if ( data.ifs[i]->value == property->value && data.ifs[i]->negated == false ||
					data.ifs[i]->value != property->value && data.ifs[i]->negated)
					ifs_list.push_back ( &data.ifs[i]->data );
			}
		}
		const vector<File*>& files = data.files;
		for ( i = 0; i < files.size(); i++ )
		{
			source_files.push_back ( files[i]->file.name );
		}
		const vector<Include*>& incs = data.includes;
		for ( i = 0; i < incs.size(); i++ )
		{
			string path = Path::RelativeFromDirectory (
				incs[i]->directory->relative_path,
				module.output->relative_path );

			includes.push_back ( path );
		}
		const vector<Library*>& libs = data.libraries;
		for ( i = 0; i < libs.size(); i++ )
		{
			libraries.push_back ( libs[i]->name );
		}
		const vector<CompilerFlag*>& cflags = data.compilerFlags;
		for ( i = 0; i < cflags.size(); i++ )
		{
			compiler_flags.push_back ( cflags[i]->flag );
		}
		const vector<Define*>& defs = data.defines;
		for ( i = 0; i < defs.size(); i++ )
		{
			if ( defs[i]->value[0] )
			{
				const string& escaped = _replace_str(defs[i]->value, "\"","&quot;");
				common_defines.push_back( defs[i]->name + "=" + escaped );
			}
			else
			{
				common_defines.push_back( defs[i]->name );
			}
		}
	}


	if (module_type == ".sys")
		fprintf ( OUT, "TARGETTYPE=DRIVER\r\n" );

	fprintf ( OUT, "\r\nMSC_WARNING_LEVEL=/W3 /WX\r\n\r\n" );

	/* Disable deprecated function uage warnings */
	fprintf ( OUT, "C_DEFINES=$(C_DEFINES) /wd4996\r\n" );
	

	/* includes */
	fprintf ( OUT, "INCLUDES=.; \\\r\n" );
	for ( i = 1; i < includes.size() -1; i++ )
	{
		const string& include = includes[i];
		
		/* don't include psdk / ddk */
		std::string::size_type pos = include.find("ddk");
		std::string::size_type pos2 = include.find("psdk");
		if ((std::string::npos == pos) && (std::string::npos == pos2))
			fprintf ( OUT, "\t%s; \\\r\n", include.c_str() );
	}
	if (includes.size() > 1)
	{
		const string& include = includes[includes.size()-1];
		fprintf ( OUT, "\t%s \r\n\r\n", include.c_str() );
	}

	fprintf ( OUT, "TARGETLIBS= $(DDK_LIB_PATH)\\ntstrsafe.lib\r\n\r\n" );

	string source_file = "";
	if (source_files.size() > 0)
	{
		source_file = DosSeparator(source_files[0]);
		fprintf ( OUT, "SOURCES=%s \\\r\n", source_file.c_str() );
	
		for ( size_t isrcfile = 1; isrcfile < source_files.size()-1; isrcfile++ )
		{
			source_file = DosSeparator(source_files[isrcfile]);
			fprintf ( OUT, "\t%s \\\r\n", source_file.c_str() );
		}
	}
	if (source_files.size() > 1)
	{
		source_file = DosSeparator(source_files[source_files.size()-1]);
		fprintf ( OUT, "\t%s \r\n", source_file.c_str() );
	}

	fprintf ( OUT, "TARGET_DESTINATION=retail\r\n" );
	

	fclose ( OUT );
}

void MsBuildBackend::ProcessModules()
{
	for(size_t i = 0; i < ProjectNode.modules.size(); i++)
	{
		Module &module = *ProjectNode.modules[i];
		_generate_makefile ( module );
		_generate_sources ( module );
	}
}

void
MsBuildBackend::_clean_project_files ( void )
{
	for ( size_t i = 0; i < ProjectNode.modules.size(); i++ )
	{
		Module& module = *ProjectNode.modules[i];
		printf("Cleaning project %s %s\n", module.name.c_str (), module.output->relative_path.c_str () );
		
		string makefile = module.output->relative_path + "\\makefile";
		string sourcesfile = module.output->relative_path + "\\sources";

		string basepath = module.output->relative_path;
		remove ( makefile.c_str() );
		remove ( sourcesfile.c_str() );
	}
}

MsBuildConfiguration::MsBuildConfiguration ( const std::string &name )
{
	/* nothing to do here */
}

const Property* 
MsBuildBackend::_lookup_property ( const Module& module, const std::string& name ) const
{
	/* Check local values */
	for ( size_t i = 0; i < module.non_if_data.properties.size(); i++ )
	{
		const Property& property = *module.non_if_data.properties[i];
		if ( property.name == name )
			return &property;
	}
	// TODO FIXME - should we check local if-ed properties?
	for ( size_t i = 0; i < module.project.non_if_data.properties.size(); i++ )
	{
		const Property& property = *module.project.non_if_data.properties[i];
		if ( property.name == name )
			return &property;
	}
	// TODO FIXME - should we check global if-ed properties?
	return NULL;
}

std::string
MsBuildBackend::_replace_str(std::string string1, const std::string &find_str, const std::string &replace_str)
{
	std::string::size_type pos = string1.find(find_str, 0);
	int intLen = find_str.length();

	while(std::string::npos != pos)
	{
		string1.replace(pos, intLen, replace_str);
		pos = string1.find(find_str, intLen + pos);
	}

	return string1;
}

