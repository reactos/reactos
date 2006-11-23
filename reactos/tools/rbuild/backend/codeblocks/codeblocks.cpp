/*
 * Copyright (C) 2006 Christoph von Wittich
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

#include "codeblocks.h"
#include "../mingw/mingw.h"

using std::string;
using std::vector;
using std::ifstream;

#ifdef OUT
#undef OUT
#endif//OUT


static class CBFactory : public Backend::Factory
{
	public:

		CBFactory() : Factory("CB", "Code::Blocks") {}
		Backend *operator() (Project &project,
		                     Configuration& configuration)
		{
			return new CBBackend(project, configuration);
		}
		
} factory;


CBBackend::CBBackend(Project &project,
	Configuration& configuration) : Backend(project, configuration)
{
	m_unitCount = 0;
}

void CBBackend::Process()
{

	while ( m_configurations.size () > 0 )
	{
		const CBConfiguration* cfg = m_configurations.back();
		m_configurations.pop_back();
		delete cfg;
	}

	m_configurations.push_back ( new CBConfiguration( Debug ));
	m_configurations.push_back ( new CBConfiguration( Release ));

	string filename_wrkspace ( ProjectNode.name );
	filename_wrkspace += "_auto.workspace";

	printf ( "Creating Code::Blocks workspace: %s\n", filename_wrkspace.c_str() );
	
	ProcessModules();
	m_wrkspaceFile = fopen ( filename_wrkspace.c_str(), "wb" );

	if ( !m_wrkspaceFile )
	{
		printf ( "Could not create file '%s'.\n", filename_wrkspace.c_str() );
		return;
	}

	_generate_workspace ( m_wrkspaceFile );

	fclose ( m_wrkspaceFile );
	printf ( "Done.\n" );
}

void CBBackend::ProcessModules()
{
	for(size_t i = 0; i < ProjectNode.modules.size(); i++)
	{
		Module &module = *ProjectNode.modules[i];
		_generate_cbproj ( module );
	}
}

static bool FileExists(string &filename)
{
	ifstream file(filename.c_str());

	if(!file.is_open())
		return false;

	file.close();
	return true;
}

void CBBackend::ProcessFile(string &filepath)
{
	// Remove the .\ at the start of the filenames
	if ( filepath[0] == '.' && strchr ( "/\\", filepath[1] ) )
		filepath.erase(0, 2);

	if(!FileExists(filepath))
		return;

	// Change the \ to /
	for(size_t i = 0; i < filepath.length(); i++)
	{
		if(filepath[i] == '\\')
			filepath[i] = '/';
	}

	// Remove the filename from the path
	string folder = "";

	size_t pos = filepath.rfind(string("/"), filepath.length() - 1);

	if(pos != string::npos)
	{
		folder = filepath;
		folder.erase(pos, folder.length() - pos);
	}
	
	FileUnit fileUnit;
	fileUnit.filename = filepath;
	fileUnit.folder = folder;

	m_fileUnits.push_back(fileUnit);

	if(folder != "")
		AddFolders(folder);

	m_unitCount++;
}

bool CBBackend::CheckFolderAdded(string &folder)
{
	for(size_t i = 0; i < m_folders.size(); i++)
	{
		if(m_folders[i] == folder)
			return true;
	}

	return false;
}

void CBBackend::AddFolders(string &folder)
{
	// Check if this folder was already added. true if it was, false otherwise.
	if(CheckFolderAdded(folder))
		return;
	
	m_folders.push_back(folder);
	
	size_t pos = folder.rfind(string("/"), folder.length() - 1);

	if(pos == string::npos)
		return;

	folder.erase(pos, folder.length() - pos);
	AddFolders(folder);
}

void CBBackend::OutputFolders()
{
#if 0
	m_devFile << "Folders=";

	for(size_t i = 0; i < m_folders.size(); i++)
	{
		if(i > 0)
			m_devFile << ",";

		m_devFile << m_folders[i];
	}
#endif
}

std::string
CBBackend::CbpFileName ( const Module& module ) const
{
	return DosSeparator(
		ReplaceExtension ( module.GetPath(), + "_auto.cbp" )
		);
}

std::string
CBBackend::LayoutFileName ( const Module& module ) const
{
	return DosSeparator(
		ReplaceExtension ( module.GetPath(), + "_auto.layout" )
		);
}

std::string
CBBackend::DependFileName ( const Module& module ) const
{
	return DosSeparator(
		ReplaceExtension ( module.GetPath(), + "_auto.depend" )
		);
}

void 
CBBackend::_get_object_files ( const Module& module, vector<string>& out) const
{
	string basepath = module.GetBasePath ();
	size_t i;
	string intenv = Environment::GetIntermediatePath () + "\\" + basepath + "\\";
	string outenv = Environment::GetOutputPath () + "\\" + basepath + "\\";

	vector<string> cfgs;

	if ( configuration.UseConfigurationInPath )
	{
		cfgs.push_back ( intenv + "Debug" );
		cfgs.push_back ( intenv + "Release" );
		cfgs.push_back ( outenv + "Debug" );
		cfgs.push_back ( outenv + "Release" );
	}
	else
	{
		cfgs.push_back ( intenv );
		cfgs.push_back ( outenv );
	}

	vector<const IfableData*> ifs_list;
	ifs_list.push_back ( &module.project.non_if_data );
	ifs_list.push_back ( &module.non_if_data );
	while ( ifs_list.size () )
	{
		const IfableData& data = *ifs_list.back();
		ifs_list.pop_back();
		const vector<File*>& files = data.files;
		for ( i = 0; i < files.size (); i++ )
		{
			string file = files[i]->name;
			string::size_type pos = file.find_last_of ("\\");
			if ( pos != string::npos )
				file.erase ( 0, pos+1 );
			if ( !stricmp ( Right(file,3).c_str(), ".rc" ) )
				file = ReplaceExtension ( file, ".res" );
			else
				file = ReplaceExtension ( file, ".obj" );
			for ( size_t j = 0; j < cfgs.size () / 2; j++ )
				out.push_back ( cfgs[j] + "\\" + file );
		}

	}
}

void
CBBackend::_clean_project_files ( void )
{
	for ( size_t i = 0; i < ProjectNode.modules.size(); i++ )
	{
		Module& module = *ProjectNode.modules[i];
		vector<string> out;
		printf("Cleaning project %s %s\n", module.name.c_str (), module.GetBasePath ().c_str () );
		
		string basepath = module.GetBasePath ();
		remove ( CbpFileName ( module ).c_str () );	
		remove ( DependFileName ( module ).c_str () );	
		remove ( LayoutFileName ( module ).c_str () );	

		_get_object_files ( module, out );
		for ( size_t j = 0; j < out.size (); j++)
		{
			//printf("Cleaning file %s\n", out[j].c_str () );
			remove ( out[j].c_str () );
		}
	}

	string filename_wrkspace = ProjectNode.name + ".workspace";

	remove ( filename_wrkspace.c_str () );
}

void
CBBackend::_generate_workspace ( FILE* OUT )
{
	fprintf ( OUT, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\" ?>\r\n" );
	fprintf ( OUT, "<CodeBlocks_workspace_file>\r\n" );
	fprintf ( OUT, "\t<Workspace title=\"ReactOS\">\r\n" );
	for ( size_t i = 0; i < ProjectNode.modules.size(); i++ )
	{
		Module& module = *ProjectNode.modules[i];
		
		std::string Cbp_file = CbpFileName ( module );
		fprintf ( OUT, "\t\t<Project filename=\"%s\" />\r\n", Cbp_file.c_str());
		
	}
	fprintf ( OUT, "\t</Workspace>\r\n" );
	fprintf ( OUT, "</CodeBlocks_workspace_file>\r\n" );
}

void
CBBackend::_generate_cbproj ( const Module& module )
{

	size_t i;

	string cbproj_file = CbpFileName(module);
	string outdir;
	string intdir;
	string path_basedir = module.GetPathToBaseDir ();
	string intenv = Environment::GetIntermediatePath ();
	string outenv = Environment::GetOutputPath ();
	string module_type = GetExtension(module.GetTargetName());
	string cbproj_path = module.GetBasePath();	
	string CompilerVar;

	bool lib = (module.type == ObjectLibrary) || (module.type == RpcClient) ||(module.type == RpcServer) || (module_type == ".lib") || (module_type == ".a");
	bool dll = (module_type == ".dll") || (module_type == ".cpl");
	bool exe = (module_type == ".exe") || (module_type == ".scr");
	bool sys = (module_type == ".sys");

	vector<string> source_files, resource_files, includes, libraries, libpaths;
	vector<string> header_files, common_defines, compiler_flags;
	vector<string> vars, values;
	
	compiler_flags.push_back ( "-Wall" );

	// Always force disabling of sibling calls optimisation for GCC
	// (TODO: Move to version-specific once this bug is fixed in GCC)
	compiler_flags.push_back ( "-fno-optimize-sibling-calls" );

	if ( module.pch != NULL )
	{
		string pch_path = Path::RelativeFromDirectory (
					module.pch->file.name,
					module.GetBasePath() );

		header_files.push_back ( pch_path );
	}

	if ( intenv == "obj-i386" )
		intdir = path_basedir + "obj-i386"; /* append relative dir from project dir */
	else
		intdir = intenv;

	if ( outenv == "output-i386" )
		outdir = path_basedir + "output-i386";
	else
		outdir = outenv;

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
			string file = string(".") + &files[i]->name[cbproj_path.size()];

			if ( !stricmp ( Right(file,3).c_str(), ".rc" ) )
				resource_files.push_back ( file );
			else
				source_files.push_back ( file );
		}
		const vector<Include*>& incs = data.includes;
		for ( i = 0; i < incs.size(); i++ )
		{
			string path = Path::RelativeFromDirectory (
				incs[i]->directory,
				module.GetBasePath() );

			includes.push_back ( path );
		}
		const vector<Library*>& libs = data.libraries;
		for ( i = 0; i < libs.size(); i++ )
		{
			string libpath = outdir + "\\" + libs[i]->importedModule->GetBasePath() + "\\" + libs[i]->name;
			libraries.push_back ( libs[i]->name );
			libpaths.push_back ( libpath );
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
		/*const vector<Property*>& variables = data.properties;
		for ( i = 0; i < variables.size(); i++ )
		{
			vars.push_back( variables[i]->name );
			values.push_back( variables[i]->value );
		}*/
	}

	if ( !module.allowWarnings )
		compiler_flags.push_back ( "-Werror" );

	FILE* OUT = fopen ( cbproj_file.c_str(), "wb" );

	fprintf ( OUT, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\" ?>\r\n" );
	fprintf ( OUT, "<CodeBlocks_project_file>\r\n" );
	fprintf ( OUT, "\t<FileVersion major=\"1\" minor=\"5\" />\r\n" );
	fprintf ( OUT, "\t<Project>\r\n" );
	fprintf ( OUT, "\t\t<Option title=\"%s\" />\r\n", module.name.c_str() );
	fprintf ( OUT, "\t\t<Option pch_mode=\"2\" />\r\n" );
	fprintf ( OUT, "\t\t<Option default_target=\"\" />\r\n" );
	fprintf ( OUT, "\t\t<Option compiler=\"gcc\" />\r\n" );
	fprintf ( OUT, "\t\t<Option virtualFolders=\"\" />\r\n" );
	fprintf ( OUT, "\t\t<Build>\r\n" );

	bool console = exe && (module.type == Win32CUI);

	for ( size_t icfg = 0; icfg < m_configurations.size(); icfg++ )
	{
		const CBConfiguration& cfg = *m_configurations[icfg];
		fprintf ( OUT, "\t\t\t<Target title=\"%s\">\r\n", cfg.name.c_str() );

		if ( configuration.UseConfigurationInPath )
		{
			fprintf ( OUT, "\t\t\t\t<Option output=\"%s\\%s%s\\%s%s\" prefix_auto=\"0\" extension_auto=\"0\" />\r\n", outdir.c_str (), module.GetBasePath ().c_str (), cfg.name.c_str(), module.name.c_str(), module_type.c_str());
			fprintf ( OUT, "\t\t\t\t<Option object_output=\"%s\\%s%s\" />\r\n", intdir.c_str(), module.GetBasePath ().c_str (), cfg.name.c_str() );
		}
		else
		{
			fprintf ( OUT, "\t\t\t\t<Option output=\"%s\\%s\\%s%s\" prefix_auto=\"0\" extension_auto=\"0\" />\r\n", outdir.c_str (), module.GetBasePath ().c_str (), module.name.c_str(), module_type.c_str() );
			fprintf ( OUT, "\t\t\t\t<Option object_output=\"%s\\%s\" />\r\n", intdir.c_str(), module.GetBasePath ().c_str () );
		}

		if ( lib )
			fprintf ( OUT, "\t\t\t\t<Option type=\"2\" />\r\n" );
		else if ( dll )		
			fprintf ( OUT, "\t\t\t\t<Option type=\"3\" />\r\n" );
		else if ( sys )
			fprintf ( OUT, "\t\t\t\t<Option type=\"?\" />\r\n" ); /*FIXME*/
		else if ( exe )
		{
			if ( module.type == Kernel )
				fprintf ( OUT, "\t\t\t\t<Option type=\"?\" />\r\n" ); /*FIXME*/
			else if ( module.type == NativeCUI )
				fprintf ( OUT, "\t\t\t\t<Option type=\"?\" />\r\n" ); /*FIXME*/
			else if ( module.type == Win32CUI || module.type == Win32GUI || module.type == Win32SCR)
			{
				if ( console )
					fprintf ( OUT, "\t\t\t\t<Option type=\"1\" />\r\n" );
				else
					fprintf ( OUT, "\t\t\t\t<Option type=\"0\" />\r\n" );
			}
		}
		
			
		fprintf ( OUT, "\t\t\t\t<Option compiler=\"gcc\" />\r\n" );
		fprintf ( OUT, "\t\t\t\t<Compiler>\r\n" );
		
		bool debug = ( cfg.optimization == Debug );

		if ( debug )
			fprintf ( OUT, "\t\t\t\t\t<Add option=\"-g\" />\r\n" );

		/* compiler flags */
		for ( i = 0; i < compiler_flags.size(); i++ )
		{
			const string& cflag = compiler_flags[i];
			fprintf ( OUT, "\t\t\t\t\t<Add option=\"%s\" />\r\n", cflag.c_str() );
		}

		/* defines */
		for ( i = 0; i < common_defines.size(); i++ )
		{
			const string& define = common_defines[i];
			fprintf ( OUT, "\t\t\t\t\t<Add option=\"-D%s\" />\r\n", define.c_str() );
		}
		/* includes */
		for ( i = 0; i < includes.size(); i++ )
		{
			const string& include = includes[i];
			fprintf ( OUT, "\t\t\t\t\t<Add directory=\"%s\" />\r\n", include.c_str() );
		}
		fprintf ( OUT, "\t\t\t\t</Compiler>\r\n" );

		/* includes */
		fprintf ( OUT, "\t\t\t\t<ResourceCompiler>\r\n" );
		for ( i = 0; i < includes.size(); i++ )
		{
			const string& include = includes[i];
			fprintf ( OUT, "\t\t\t\t\t<Add directory=\"%s\" />\r\n", include.c_str() );
		}
		fprintf ( OUT, "\t\t\t\t</ResourceCompiler>\r\n" );

		/* libraries */
		fprintf ( OUT, "\t\t\t\t<Linker>\r\n" );
		for ( i = 0; i < libraries.size(); i++ )
		{
			const string& lib = libraries[i];
			fprintf ( OUT, "\t\t\t\t\t<Add library=\"%s\" />\r\n", lib.c_str() );
		}
		for ( i = 0; i < libpaths.size(); i++ )
		{
			const string& lib = libpaths[i];
			fprintf ( OUT, "\t\t\t\t\t<Add directory=\"%s\" />\r\n", lib.c_str() );
		}
		fprintf ( OUT, "\t\t\t\t</Linker>\r\n" );

		fprintf ( OUT, "\t\t\t</Target>\r\n" );

	}

	/* vars
	fprintf ( OUT, "\t\t\t<Environment>\r\n" );
	for ( i = 0; i < vars.size(); i++ )
	{
		const string& var = vars[i];
		const string& value = values[i];
		fprintf ( OUT, "\t\t\t\t<Variable name=\"%s\" value=\"%s\" />\r\n", var.c_str(), value.c_str()  );
	}
	fprintf ( OUT, "\t\t\t</Environment>\r\n" ); */

	fprintf ( OUT, "\t\t</Build>\r\n" );

	if ( module.cplusplus )
		CompilerVar = "CPP";
	else
		CompilerVar = "CC";

	/* header files */
	for ( i = 0; i < header_files.size(); i++ )
	{
		const string& header_file = header_files[i];
		fprintf ( OUT, "\t\t<Unit filename=\"%s\">\r\n", header_file.c_str() );
		fprintf ( OUT, "\t\t\t<Option compilerVar=\"%s\" />\r\n", CompilerVar.c_str() );
		fprintf ( OUT, "\t\t\t<Option compile=\"0\" />\r\n" );
		fprintf ( OUT, "\t\t\t<Option link=\"0\" />\r\n" );
		for ( size_t icfg = 0; icfg < m_configurations.size(); icfg++ )
		{
			const CBConfiguration& cfg = *m_configurations[icfg];
			fprintf ( OUT, "\t\t\t<Option target=\"%s\" />\r\n" , cfg.name.c_str() );
		}
		fprintf ( OUT, "\t\t</Unit>\r\n" );
	}

	/* source files */
	for ( size_t isrcfile = 0; isrcfile < source_files.size(); isrcfile++ )
	{
		string source_file = DosSeparator(source_files[isrcfile]);
		fprintf ( OUT, "\t\t<Unit filename=\"%s\">\r\n", source_file.c_str() );
		fprintf ( OUT, "\t\t\t<Option compilerVar=\"%s\" />\r\n", CompilerVar.c_str() );
		for ( size_t icfg = 0; icfg < m_configurations.size(); icfg++ )
		{
			const CBConfiguration& cfg = *m_configurations[icfg];
			fprintf ( OUT, "\t\t\t<Option target=\"%s\" />\r\n" , cfg.name.c_str() );
		}
		fprintf ( OUT, "\t\t</Unit>\r\n" );
	}

	/* resource files */
	for ( i = 0; i < resource_files.size(); i++ )
	{
		const string& resource_file = resource_files[i];
		fprintf ( OUT, "\t\t<Unit filename=\"%s\">\r\n", resource_file.c_str() );
		fprintf ( OUT, "\t\t\t<Option compilerVar=\"WINDRES\" />\r\n" );
		for ( size_t icfg = 0; icfg < m_configurations.size(); icfg++ )
		{
			const CBConfiguration& cfg = *m_configurations[icfg];
			fprintf ( OUT, "\t\t\t<Option target=\"%s\" />\r\n" , cfg.name.c_str() );
		}
		fprintf ( OUT, "\t\t</Unit>\r\n" );
	}

	fprintf ( OUT, "\t\t<Extensions />\r\n" );
	fprintf ( OUT, "\t</Project>\r\n" );
	fprintf ( OUT, "</CodeBlocks_project_file>\r\n" );


	fclose ( OUT );
}

CBConfiguration::CBConfiguration ( const OptimizationType optimization, const std::string &name )
{
	this->optimization = optimization;
	if ( name != "" )
		this->name = name;
	else
	{
		if ( optimization == Debug )
			this->name = "Debug";
		else if ( optimization == Release )
			this->name = "Release";
		else
			this->name = "Unknown";
	}
}

std::string
CBBackend::_replace_str(std::string string1, const std::string &find_str, const std::string &replace_str)
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

