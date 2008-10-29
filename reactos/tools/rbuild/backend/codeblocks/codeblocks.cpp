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

#define IsStaticLibrary( module ) ( ( module.type == StaticLibrary ) || ( module.type == HostStaticLibrary ) )

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
	for( std::map<std::string, Module*>::const_iterator p = ProjectNode.modules.begin(); p != ProjectNode.modules.end(); ++ p )
	{
		Module &module = *p->second;
		MingwAddImplicitLibraries( module );
		_generate_cbproj ( module );
	}
}

static std::string
GetExtension ( const std::string& filename )
{
	size_t index = filename.find_last_of ( '/' );
	if (index == string::npos) index = 0;
	string tmp = filename.substr( index, filename.size() - index );
	size_t ext_index = tmp.find_last_of( '.' );
	if (ext_index != string::npos)
		return filename.substr ( index + ext_index, filename.size() );
	return "";
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
		ReplaceExtension ( module.output->relative_path + "\\" + module.output->name, "_auto.cbp" )
		);
}

std::string
CBBackend::LayoutFileName ( const Module& module ) const
{
	return DosSeparator(
		ReplaceExtension ( module.output->relative_path + "\\" + module.output->name, "_auto.layout" )
		);
}

std::string
CBBackend::DependFileName ( const Module& module ) const
{
	return DosSeparator(
		ReplaceExtension ( module.output->relative_path + "\\" + module.output->name, "_auto.depend" )
		);
}

void
CBBackend::_get_object_files ( const Module& module, vector<string>& out) const
{
	string basepath = module.output->relative_path;
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
			string file = files[i]->file.relative_path + sSep + files[i]->file.name;
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
	for( std::map<std::string, Module*>::const_iterator p = ProjectNode.modules.begin(); p != ProjectNode.modules.end(); ++ p )
	{
		Module& module = *p->second;
		vector<string> out;
		printf("Cleaning project %s %s\n", module.name.c_str (), module.output->relative_path.c_str () );

		string basepath = module.output->relative_path;
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
	for( std::map<std::string, Module*>::const_iterator p = ProjectNode.modules.begin(); p != ProjectNode.modules.end(); ++ p )
	{
		Module& module = *p->second;

		if ((module.type != Iso) &&
			(module.type != LiveIso) &&
			(module.type != IsoRegTest) &&
			(module.type != LiveIsoRegTest))
		{
			std::string Cbp_file = CbpFileName ( module );
			fprintf ( OUT, "\t\t<Project filename=\"%s\">\r\n", Cbp_file.c_str());

			/* dependencies */
			vector<const IfableData*> ifs_list;
			ifs_list.push_back ( &module.project.non_if_data );
			ifs_list.push_back ( &module.non_if_data );
			while ( ifs_list.size() )
			{
				const IfableData& data = *ifs_list.back();
				ifs_list.pop_back();
				const vector<Library*>& libs = data.libraries;
				for ( size_t j = 0; j < libs.size(); j++ )
					fprintf ( OUT, "\t\t\t<Depends filename=\"%s\\%s_auto.cbp\" />\r\n", libs[j]->importedModule->output->relative_path.c_str(), libs[j]->name.c_str() );
			}
			fprintf ( OUT, "\t\t</Project>\r\n" );
		}
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
	string module_type = GetExtension(*module.output);
	string cbproj_path = module.output->relative_path;
	string CompilerVar;
	string baseaddr;
	string windres_defines;
	string widl_options;
	string project_linker_flags = "-Wl,--enable-stdcall-fixup ";
	project_linker_flags += GenerateProjectLinkerFlags();

	bool lib = (module.type == ObjectLibrary) ||
	           (module.type == RpcClient) ||
	           (module.type == RpcServer) ||
	           (module.type == RpcProxy) ||
	           (module_type == ".lib") ||
	           (module_type == ".a");
	bool dll = (module_type == ".dll") || (module_type == ".cpl");
	bool exe = (module_type == ".exe") || (module_type == ".scr");
	bool sys = (module_type == ".sys");

	vector<string> source_files, resource_files, includes, libraries, libpaths;
	vector<string> header_files, common_defines, compiler_flags;
	vector<string> vars, values;

	/* do not create project files for these targets
	   use virtual targets instead */
	switch (module.type)
	{
		case Iso:
		case LiveIso:
		case IsoRegTest:
		case LiveIsoRegTest:
			return;
		default:
			break;
	}

	compiler_flags.push_back ( "-Wall" );

	// Always force disabling of sibling calls optimisation for GCC
	// (TODO: Move to version-specific once this bug is fixed in GCC)
	compiler_flags.push_back ( "-fno-optimize-sibling-calls" );

	if ( module.pch != NULL )
	{
		string pch_path = Path::RelativeFromDirectory (
					module.pch->file->name,
					module.output->relative_path );

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
			string fullpath = files[i]->file.relative_path + sSep + files[i]->file.name;
			string file = string(".") + &fullpath[cbproj_path.size()];

			if ( !stricmp ( Right(file,3).c_str(), ".rc" ) )
				resource_files.push_back ( file );
			else
				source_files.push_back ( file );
		}
		const vector<Include*>& incs = data.includes;
		for ( i = 0; i < incs.size(); i++ )
		{
			string path = Path::RelativeFromDirectory (
				incs[i]->directory->relative_path,
				module.output->relative_path );

			includes.push_back ( path );
			widl_options += "-I" + path + " ";
		}
		const vector<Library*>& libs = data.libraries;
		for ( i = 0; i < libs.size(); i++ )
		{
			string libpath = intdir + "\\" + libs[i]->importedModule->output->relative_path;
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
				windres_defines += "-D" + defs[i]->name + "=" + escaped + " ";
			}
			else
			{
				common_defines.push_back( defs[i]->name );
				windres_defines += "-D" + defs[i]->name + " ";
			}
		}
		/*const vector<Property*>& variables = data.properties;
		for ( i = 0; i < variables.size(); i++ )
		{
			vars.push_back( variables[i]->name );
			values.push_back( variables[i]->value );
		}*/
		for ( std::map<std::string, Property*>::const_iterator p = data.properties.begin(); p != data.properties.end(); ++ p )
		{
			Property& prop = *p->second;
			if ( strstr ( module.baseaddress.c_str(), prop.name.c_str() ) )
				baseaddr = prop.value;
		}
	}

	if ( !module.allowWarnings )
		compiler_flags.push_back ( "-Werror" );

	if ( IsStaticLibrary ( module ) && module.isStartupLib )
		compiler_flags.push_back ( "-Wno-main" );


	FILE* OUT = fopen ( cbproj_file.c_str(), "wb" );

	fprintf ( OUT, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\" ?>\r\n" );
	fprintf ( OUT, "<CodeBlocks_project_file>\r\n" );
	fprintf ( OUT, "\t<FileVersion major=\"1\" minor=\"6\" />\r\n" );
	fprintf ( OUT, "\t<Project>\r\n" );
	fprintf ( OUT, "\t\t<Option title=\"%s\" />\r\n", module.name.c_str() );
	fprintf ( OUT, "\t\t<Option pch_mode=\"2\" />\r\n" );
	fprintf ( OUT, "\t\t<Option default_target=\"\" />\r\n" );
	fprintf ( OUT, "\t\t<Option compiler=\"gcc\" />\r\n" );
	fprintf ( OUT, "\t\t<Option extended_obj_names=\"1\" />\r\n" );
	fprintf ( OUT, "\t\t<Option virtualFolders=\"\" />\r\n" );
	fprintf ( OUT, "\t\t<Build>\r\n" );

	bool console = exe && (module.type == Win32CUI);

	for ( size_t icfg = 0; icfg < m_configurations.size(); icfg++ )
	{
		const CBConfiguration& cfg = *m_configurations[icfg];
		fprintf ( OUT, "\t\t\t<Target title=\"%s\">\r\n", cfg.name.c_str() );

		if ( configuration.UseConfigurationInPath )
		{
			if ( IsStaticLibrary ( module ) ||module.type == ObjectLibrary )
				fprintf ( OUT, "\t\t\t\t<Option output=\"%s\\%s%s\\%s%s\" prefix_auto=\"0\" extension_auto=\"0\" />\r\n", intdir.c_str (), module.output->relative_path.c_str (), cfg.name.c_str(), module.name.c_str(), module_type.c_str());
			else
				fprintf ( OUT, "\t\t\t\t<Option output=\"%s\\%s%s\\%s%s\" prefix_auto=\"0\" extension_auto=\"0\" />\r\n", outdir.c_str (), module.output->relative_path.c_str (), cfg.name.c_str(), module.name.c_str(), module_type.c_str());
			fprintf ( OUT, "\t\t\t\t<Option object_output=\"%s\\%s%s\" />\r\n", intdir.c_str(), module.output->relative_path.c_str (), cfg.name.c_str() );
		}
		else
		{
			if ( IsStaticLibrary ( module ) || module.type == ObjectLibrary )
				fprintf ( OUT, "\t\t\t\t<Option output=\"%s\\%s\\%s%s\" prefix_auto=\"0\" extension_auto=\"0\" />\r\n", intdir.c_str (), module.output->relative_path.c_str (), module.name.c_str(), module_type.c_str() );
			else
				fprintf ( OUT, "\t\t\t\t<Option output=\"%s\\%s\\%s%s\" prefix_auto=\"0\" extension_auto=\"0\" />\r\n", outdir.c_str (), module.output->relative_path.c_str (), module.name.c_str(), module_type.c_str() );
			fprintf ( OUT, "\t\t\t\t<Option object_output=\"%s\\%s\" />\r\n", intdir.c_str(), module.output->relative_path.c_str () );
		}

		if ( lib )
		{
			fprintf ( OUT, "\t\t\t\t<Option type=\"2\" />\r\n" );
		}
		else if ( dll )
			fprintf ( OUT, "\t\t\t\t<Option type=\"3\" />\r\n" );
		else if ( sys )
			fprintf ( OUT, "\t\t\t\t<Option type=\"5\" />\r\n" );
		else if ( exe )
		{
			if ( module.type == Kernel )
				fprintf ( OUT, "\t\t\t\t<Option type=\"5\" />\r\n" );
			else if ( module.type == NativeCUI )
				fprintf ( OUT, "\t\t\t\t<Option type=\"5\" />\r\n" );
			else if ( module.type == Win32CUI || module.type == Win32GUI || module.type == Win32SCR)
			{
				if ( console )
					fprintf ( OUT, "\t\t\t\t<Option type=\"1\" />\r\n" );
				else
					fprintf ( OUT, "\t\t\t\t<Option type=\"0\" />\r\n" );
			}
		}

		fprintf ( OUT, "\t\t\t\t<Option compiler=\"gcc\" />\r\n" );

		if ( module_type == ".cpl" )
		{
			fprintf ( OUT, "\t\t\t\t<Option parameters=\"shell32,Control_RunDLL &quot;$exe_output&quot;,@\" />\r\n" );
			fprintf ( OUT, "\t\t\t\t<Option host_application=\"rundll32.exe\" />\r\n" );
		}
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

		fprintf ( OUT, "\t\t\t\t<Linker>\r\n" );
		fprintf ( OUT, "\t\t\t\t\t<Add option=\"%s\" />\r\n", project_linker_flags.c_str() );

		if ( sys )
		{
			fprintf ( OUT, "\t\t\t\t\t<Add option=\"-Wl,--entry,%s%s\" />\r\n", "_", module.GetEntryPoint(false) == "" ? "DriverEntry@8" : module.GetEntryPoint(false).c_str ());
			fprintf ( OUT, "\t\t\t\t\t<Add option=\"-Wl,--image-base,%s\" />\r\n", baseaddr == "" ? "0x10000" : baseaddr.c_str () );
			fprintf ( OUT, "\t\t\t\t\t<Add option=\"-nostartfiles -Wl,--nostdlib\" />\r\n" );
		}
		else if ( exe )
		{
			if ( module.type == Kernel )
			{
				fprintf ( OUT, "\t\t\t\t\t<Add option=\"-Wl,--entry,_KiSystemStartup\" />\r\n" );
				fprintf ( OUT, "\t\t\t\t\t<Add option=\"-Wl,--image-base,%s\" />\r\n", baseaddr.c_str () );
			}
			else if ( module.type == NativeCUI )
			{
				fprintf ( OUT, "\t\t\t\t\t<Add option=\"-Wl,--entry,_NtProcessStartup@4\" />\r\n" );
				fprintf ( OUT, "\t\t\t\t\t<Add option=\"-Wl,--image-base,%s\" />\r\n", baseaddr.c_str () );
				fprintf ( OUT, "\t\t\t\t\t<Add option=\"-nostartfiles -Wl,--nostdlib\" />\r\n" );
			}
			else
			{
				fprintf ( OUT, "\t\t\t\t\t<Add option=\"%s\" />\r\n", module.cplusplus ? "-nostartfiles" : "-nostartfiles -Wl,--nostdlib" );
				fprintf ( OUT, "\t\t\t\t\t<Add library=\"gcc\" />\r\n" );
			}
		}
		else if ( dll )
		{
			fprintf ( OUT, "\t\t\t\t\t<Add option=\"-Wl,--entry,%s%s\" />\r\n", "_", module.GetEntryPoint(false).c_str () );
			fprintf ( OUT, "\t\t\t\t\t<Add option=\"-Wl,--image-base,%s\" />\r\n", baseaddr == "" ? "0x40000" : baseaddr.c_str () );

			if ( module.type == Win32DLL)
				fprintf ( OUT, "\t\t\t\t\t<Add option=\"-Wl,--shared\" />\r\n" );
			else if ( module.type == NativeDLL)
				fprintf ( OUT, "\t\t\t\t\t<Add option=\"-Wl,--shared\" />\r\n" );
			else if ( module.type == NativeDLL)
				fprintf ( OUT, "\t\t\t\t\t<Add option=\"-nostartfiles -Wl,--shared\" />\r\n" );

			fprintf ( OUT, "\t\t\t\t\t<Add option=\"%s\" />\r\n", module.cplusplus ? "-nostartfiles" : "-nostartfiles -Wl,--nostdlib" );
			fprintf ( OUT, "\t\t\t\t\t<Add library=\"gcc\" />\r\n" );
		}

		fprintf ( OUT, "\t\t\t\t\t<Add option=\"-Wl,--file-alignment,0x1000\" />\r\n" );
		fprintf ( OUT, "\t\t\t\t\t<Add option=\"-Wl,--section-alignment,0x1000\" />\r\n" );

		if ( dll )
			fprintf ( OUT, "\t\t\t\t\t<Add option=\"%s.temp.exp\" />\r\n", module.name.c_str() );

		/* libraries */
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

		fprintf ( OUT, "\t\t\t\t<ExtraCommands>\r\n" );

#if 0
		if ( IsStaticLibrary ( module ) && module.importLibrary )
			fprintf ( OUT, "\t\t\t\t\t<Add after=\"dlltool --dllname %s --def %s --output-lib $exe_output; %s -U\" />\r\n", module.importLibrary->dllname.c_str (), module.importLibrary->definition.c_str(), module.mangledSymbols ? "" : "--kill-at" );
		else if ( module.importLibrary != NULL )
			fprintf ( OUT, "\t\t\t\t\t<Add after=\"dlltool --dllname %s --def %s --output-lib &quot;$(TARGET_OBJECT_DIR)lib$(TARGET_OUTPUT_BASENAME).a&quot; %s\" />\r\n", module.GetTargetName ().c_str(), module.importLibrary->definition.c_str(), module.mangledSymbols ? "" : "--kill-at" );
#endif


		for ( i = 0; i < resource_files.size(); i++ )
		{
			const string& resource_file = resource_files[i];
#ifdef WIN32
			fprintf ( OUT, "\t\t\t\t\t<Add after=\"cmd /c del $(TARGET_OBJECT_DIR)\\%s.rci.tmp 2&gt;NUL\" />\r\n", resource_file.c_str() );
			fprintf ( OUT, "\t\t\t\t\t<Add after=\"cmd /c del $(TARGET_OBJECT_DIR)\\%s.res.tmp 2&gt;NUL\" />\r\n", resource_file.c_str() );
#else
			fprintf ( OUT, "\t\t\t\t\t<Add after=\"rm $(TARGET_OBJECT_DIR)/%s.rci.tmp 2&gt;/dev/null\" />\r\n", resource_file.c_str() );
			fprintf ( OUT, "\t\t\t\t\t<Add after=\"rm $(TARGET_OBJECT_DIR)/%s.res.tmp 2&gt;/dev/null\" />\r\n", resource_file.c_str() );
#endif
		}

#if 0
		if ( dll )
		{
			if (IsSpecDefinitionFile( module ))
				fprintf ( OUT, "\t\t\t\t\t<Add before=\"%s\\tools\\winebuild\\winebuild.exe -o %s --def -E %s.spec\" />\r\n", outdir.c_str(), module.importLibrary->definition.c_str(),  module.name.c_str());
			fprintf ( OUT, "\t\t\t\t\t<Add before=\"dlltool --dllname %s --def %s --output-exp %s.temp.exp %s\" />\r\n", module.GetTargetName ().c_str(), module.importLibrary->definition.c_str(), module.name.c_str(), module.mangledSymbols ? "" : "--kill-at" );
			fprintf ( OUT, "\t\t\t\t\t<Add after=\"%s\\tools\\pefixup $exe_output -exports\" />\r\n", outdir.c_str() );
#ifdef WIN32
			fprintf ( OUT, "\t\t\t\t\t<Add after=\"cmd /c del %s.temp.exp 2&gt;NUL\" />\r\n", module.name.c_str() );
#else
			fprintf ( OUT, "\t\t\t\t\t<Add after=\"rm %s.temp.exp 2&gt;/dev/null\" />\r\n", module.name.c_str() );
#endif
			fprintf ( OUT, "\t\t\t\t\t<Mode after=\"always\" />\r\n" );
		}
#endif

		fprintf ( OUT, "\t\t\t\t</ExtraCommands>\r\n" );

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

#ifdef FORCE_CPP
	CompilerVar = "CPP"
#else
	if ( module.cplusplus )
		CompilerVar = "CPP";
	else
		CompilerVar = "CC";
#endif

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

		string extension = GetExtension ( source_file );
		if ( extension == ".s" || extension == ".S" )
		{
			fprintf ( OUT, "\t\t\t<Option compile=\"1\" />\r\n" );
			fprintf ( OUT, "\t\t\t<Option link=\"1\" />\r\n" );
			fprintf ( OUT, "\t\t\t<Option compiler=\"gcc\" use=\"1\" buildCommand=\"gcc -x assembler-with-cpp -c $file -o $link_objects $includes -D__ASM__ $options\" />\r\n" );
		}
		else if ( extension == ".asm" || extension == ".ASM" )
		{
			fprintf ( OUT, "\t\t\t<Option compile=\"1\" />\r\n" );
			fprintf ( OUT, "\t\t\t<Option link=\"1\" />\r\n" );
			fprintf ( OUT, "\t\t\t<Option compiler=\"gcc\" use=\"1\" buildCommand=\"nasm -f win32 $file -o $link_objects\" />\r\n" );
		}
		else if ( extension == ".idl" || extension == ".IDL" )
		{
			fprintf ( OUT, "\t\t\t<Option compile=\"1\" />\r\n" );
			fprintf ( OUT, "\t\t\t<Option compiler=\"gcc\" use=\"1\" buildCommand=\"%s\\tools\\widl\\widl.exe %s %s -h -H &quot;$(TARGET_OUTPUT_DIR)$filetitle_c.h&quot; -c -C &quot;$(TARGET_OUTPUT_DIR)$filetitle_c.c&quot; $file\\ngcc %s -c &quot;$(TARGET_OUTPUT_DIR)$filetitle_c.c&quot; -o &quot;$(TARGET_OUTPUT_DIR)$file_c.o&quot;\" />\r\n", outdir.c_str(), widl_options.c_str(), windres_defines.c_str(), widl_options.c_str() );
		}
		else if ( extension == ".spec" || extension == ".SPEC" )
		{
			fprintf ( OUT, "\t\t\t<Option compile=\"1\" />\r\n" );
			fprintf ( OUT, "\t\t\t<Option link=\"1\" />\r\n" );
			fprintf ( OUT, "\t\t\t<Option compiler=\"gcc\" use=\"1\" buildCommand=\"%s\\tools\\winebuild\\winebuild.exe -o $file.stubs.c --pedll $file\\n$compiler -c $options $includes $file.stubs.c -o $(TARGET_OBJECT_DIR)\\$file.o\" />\r\n", outdir.c_str() );
		}

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
		string extension = GetExtension ( resource_file );
		fprintf ( OUT, "\t\t\t<Option compiler=\"gcc\" use=\"1\" buildCommand=\"gcc -xc -E -DRC_INVOKED $includes %s $file -o $(TARGET_OBJECT_DIR)\\$file.rci.tmp\\n%s\\tools\\wrc\\wrc.exe $includes %s $(TARGET_OBJECT_DIR)\\$file.rci.tmp $(TARGET_OBJECT_DIR)\\$file.res.tmp\\n$rescomp --output-format=coff $(TARGET_OBJECT_DIR)\\$file.res.tmp -o $resource_output\" />\r\n" , windres_defines.c_str(), outdir.c_str(),  windres_defines.c_str() );
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

std::string
CBBackend::GenerateProjectLinkerFlags() const
{
	std::string lflags;
	for ( size_t i = 0; i < ProjectNode.linkerFlags.size (); i++ )
	{
		LinkerFlag& linkerFlag = *ProjectNode.linkerFlags[i];
		if ( lflags.length () > 0 )
			lflags += " ";
		lflags += linkerFlag.flag;
	}
	return lflags;
}

void
CBBackend::MingwAddImplicitLibraries( Module &module )
{
	Library* pLibrary;

	if ( !module.isDefaultEntryPoint )
		return;

	if ( module.IsDLL () )
	{
		//pLibrary = new Library ( module, "__mingw_dllmain" );
		//module.non_if_data.libraries.insert ( module.non_if_data.libraries.begin(), pLibrary );
	}
	else
	{
		pLibrary = new Library ( module, module.isUnicode ? "mingw_wmain" : "mingw_main" );
		module.non_if_data.libraries.insert ( module.non_if_data.libraries.begin(), pLibrary );
	}

	pLibrary = new Library ( module, "mingw_common" );
	module.non_if_data.libraries.insert ( module.non_if_data.libraries.begin() + 1, pLibrary );

	if ( module.name != "msvcrt" )
	{
		// always link in msvcrt to get the basic routines
		pLibrary = new Library ( module, "msvcrt" );
		module.non_if_data.libraries.push_back ( pLibrary );
	}
}

const Property*
CBBackend::_lookup_property ( const Module& module, const std::string& name ) const
{
	std::map<std::string, Property*>::const_iterator p;

	/* Check local values */
	p = module.non_if_data.properties.find(name);

	if ( p != module.non_if_data.properties.end() )
		return p->second;

	// TODO FIXME - should we check local if-ed properties?
	p = module.project.non_if_data.properties.find(name);

	if ( p != module.project.non_if_data.properties.end() )
		return p->second;

	// TODO FIXME - should we check global if-ed properties?
	return NULL;
}

bool
CBBackend::IsSpecDefinitionFile ( const Module& module ) const
{
	if ( module.importLibrary == NULL)
		return false;

	size_t index = module.importLibrary->source->name.rfind ( ".spec" );
	return ( index != string::npos );
}
