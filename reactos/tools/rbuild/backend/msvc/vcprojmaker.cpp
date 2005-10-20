/*
 * Copyright (C) 2002 Patrik Stridvall
 * Copyright (C) 2005 Royce Mitchell III
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

#include <string>
#include <vector>

#include <stdio.h>

#include "msvc.h"

using std::string;
using std::vector;

#ifdef OUT
#undef OUT
#endif//OUT

void
MSVCBackend::_generate_vcproj ( const Module& module )
{
	size_t i;
	// TODO FIXME wine hack?
	//const bool wine = false;

	string vcproj_file = VcprojFileName(module);
	printf ( "Creating MSVC.NET project: '%s'\n", vcproj_file.c_str() );
	FILE* OUT = fopen ( vcproj_file.c_str(), "wb" );

	vector<string> imports;
	for ( i = 0; i < module.non_if_data.libraries.size(); i++ )
	{
		imports.push_back ( module.non_if_data.libraries[i]->name );
	}

	string module_type = GetExtension(module.GetTargetName());
	bool lib = (module.type == ObjectLibrary) || (module_type == ".lib") || (module_type == ".a");
	bool dll = (module_type == ".dll") || (module_type == ".cpl");
	bool exe = (module_type == ".exe");
	// TODO FIXME - need more checks here for 'sys' and possibly 'drv'?

	bool console = exe && (module.type == Win32CUI);

	// TODO FIXME - not sure if the count here is right...
	int parts = 0;
	const char* p = strpbrk ( vcproj_file.c_str(), "/\\" );
	while ( p )
	{
		++parts;
		p = strpbrk ( p+1, "/\\" );
	}
	string msvc_wine_dir = "..";
	while ( --parts )
		msvc_wine_dir += "\\..";

	string wine_include_dir = msvc_wine_dir + "\\include";

	//$progress_current++;
	//$output->progress("$dsp_file (file $progress_current of $progress_max)");

	// TODO FIXME - what's diff. betw. 'c_srcs' and 'source_files'?
	string vcproj_path = module.GetBasePath();
	vector<string> c_srcs, source_files, resource_files, includes, libraries, defines;
	vector<const IfableData*> ifs_list;
	ifs_list.push_back ( &module.project.non_if_data );
	ifs_list.push_back ( &module.non_if_data );

	// this is a define in MinGW w32api, but not Microsoft's headers
	defines.push_back ( "STDCALL=__stdcall" );

	while ( ifs_list.size() )
	{
		const IfableData& data = *ifs_list.back();
		ifs_list.pop_back();
		// TODO FIXME - refactor needed - we're discarding if conditions
		for ( i = 0; i < data.ifs.size(); i++ )
			ifs_list.push_back ( &data.ifs[i]->data );
		const vector<File*>& files = data.files;
		for ( i = 0; i < files.size(); i++ )
		{
			// TODO FIXME - do we want the full path of the file here?
			string file = string(".") + &files[i]->name[vcproj_path.size()];

			source_files.push_back ( file );
			if ( !stricmp ( Right(file,2).c_str(), ".c" ) )
				c_srcs.push_back ( file );
			if ( !stricmp ( Right(file,3).c_str(), ".rc" ) )
				resource_files.push_back ( file );
		}
		const vector<Include*>& incs = data.includes;
		for ( i = 0; i < incs.size(); i++ )
		{
			// explicitly omit win32api directories
			if ( !strncmp(incs[i]->directory.c_str(), "w32api", 6 ) )
				continue;

			// explicitly omit include/wine directories
			if ( !strncmp(incs[i]->directory.c_str(), "include\\wine", 12 ) )
				continue;

			string path = Path::RelativeFromDirectory (
				incs[i]->directory,
				module.GetBasePath() );
			includes.push_back ( path );
		}
		const vector<Library*>& libs = data.libraries;
		for ( i = 0; i < libs.size(); i++ )
		{
			libraries.push_back ( libs[i]->name + ".lib" );
		}
		const vector<Define*>& defs = data.defines;
		for ( i = 0; i < defs.size(); i++ )
		{
			if ( defs[i]->value[0] )
				defines.push_back ( defs[i]->name + "=" + defs[i]->value );
			else
				defines.push_back ( defs[i]->name );
		}
	}

	vector<string> header_files;

	bool no_cpp = true;
	bool no_msvc_headers = true;

	std::vector<std::string> cfgs;

	cfgs.push_back ( "Debug" );
	cfgs.push_back ( "Release" );

	if (!no_cpp)
	{
		std::vector<std::string> _cfgs;
		for ( i = 0; i < cfgs.size(); i++ )
		{
			_cfgs.push_back ( cfgs[i] + " C" );
			_cfgs.push_back ( cfgs[i] + " C++" );
		}
		cfgs.resize(0);
		cfgs = _cfgs;
	}

	if (!no_msvc_headers)
	{
		std::vector<std::string> _cfgs;
		for ( i = 0; i < cfgs.size(); i++ )
		{
			_cfgs.push_back ( cfgs[i] + " MSVC Headers" );
			_cfgs.push_back ( cfgs[i] + " Wine Headers" );
		}
		cfgs.resize(0);
		cfgs = _cfgs;
	}

	string default_cfg = cfgs.back();

	fprintf ( OUT, "<?xml version=\"1.0\" encoding = \"Windows-1252\"?>\r\n" );
	fprintf ( OUT, "<VisualStudioProject\r\n" );
	fprintf ( OUT, "\tProjectType=\"Visual C++\"\r\n" );

	if (configuration.VSProjectVersion.empty())
		configuration.VSProjectVersion = MS_VS_DEF_VERSION;

	fprintf ( OUT, "\tVersion=\"%s\"\r\n", configuration.VSProjectVersion.c_str() );
	fprintf ( OUT, "\tName=\"%s\"\r\n", module.name.c_str() );
	fprintf ( OUT, "\tProjectGUID=\"%s\"\r\n", module.guid.c_str() ); 
	fprintf ( OUT, "\tKeyword=\"Win32Proj\">\r\n" );

	fprintf ( OUT, "\t<Platforms>\r\n" );
	fprintf ( OUT, "\t\t<Platform\r\n" );
	fprintf ( OUT, "\t\t\tName=\"Win32\"/>\r\n" );
	fprintf ( OUT, "\t</Platforms>\r\n" );

	int n = 0;

	std::string output_dir;

	fprintf ( OUT, "\t<Configurations>\r\n" );
	for ( size_t icfg = 0; icfg < cfgs.size(); icfg++ )
	{
		std::string& cfg = cfgs[icfg];

		bool debug = !strstr ( cfg.c_str(), "Release" );
		//bool msvc_headers = ( 0 != strstr ( cfg.c_str(), "MSVC Headers" ) );

		fprintf ( OUT, "\t\t<Configuration\r\n" );
		fprintf ( OUT, "\t\t\tName=\"%s|Win32\"\r\n", cfg.c_str() );
		fprintf ( OUT, "\t\t\tOutputDirectory=\"%s\"\r\n", cfg.c_str() );
		fprintf ( OUT, "\t\t\tIntermediateDirectory=\"%s\"\r\n", cfg.c_str() );
		fprintf ( OUT, "\t\t\tConfigurationType=\"%d\"\r\n", exe ? 1 : dll ? 2 : lib ? 4 : -1 );
		fprintf ( OUT, "\t\t\tCharacterSet=\"2\">\r\n" );

		fprintf ( OUT, "\t\t\t<Tool\r\n" );
		fprintf ( OUT, "\t\t\t\tName=\"VCCLCompilerTool\"\r\n" );
		fprintf ( OUT, "\t\t\t\tOptimization=\"%d\"\r\n", debug ? 0 : 2 );

		fprintf ( OUT, "\t\t\t\tAdditionalIncludeDirectories=\"" );
		bool multiple_includes = false;
		fprintf ( OUT, "./;" );
		for ( i = 0; i < includes.size(); i++ )
		{
			const string& include = includes[i];
			if ( strcmp ( include.c_str(), "." ) )
			{
				if ( multiple_includes )
					fprintf ( OUT, ";" );
				fprintf ( OUT, "%s", include.c_str() );
				multiple_includes = true;
			}
		}
		fprintf ( OUT, "\"\r\n " );

		if ( debug )
		{
			defines.push_back ( "_DEBUG" );
			if ( lib || exe )
			{
				defines.push_back ( "_LIB" );
			}
			else
			{
				defines.push_back ( "_WINDOWS" );
				defines.push_back ( "_USRDLL" );
			}
		}
		else
		{
			defines.push_back ( "NDEBUG" );
			if ( lib || exe )
			{
				defines.push_back ( "_LIB" );
			}
			else
			{
				defines.push_back ( "_WINDOWS" );
				defines.push_back ( "_USRDLL" );
			}
		}

		fprintf ( OUT, "\t\t\t\tPreprocessorDefinitions=\"" );
		for ( i = 0; i < defines.size(); i++ )
		{
			if ( i > 0 )
				fprintf ( OUT, ";" );

			defines[i] = _replace_str(defines[i], "\"","&quot;"); 
			fprintf ( OUT, "%s", defines[i].c_str() );
		}
		fprintf ( OUT, "\"\r\n" );

		fprintf ( OUT, "\t\t\t\tMinimalRebuild=\"TRUE\"\r\n" );
		fprintf ( OUT, "\t\t\t\tBasicRuntimeChecks=\"3\"\r\n" );
		fprintf ( OUT, "\t\t\t\tRuntimeLibrary=\"5\"\r\n" );
		fprintf ( OUT, "\t\t\t\tBufferSecurityCheck=\"%s\"\r\n", debug ? "TRUE" : "FALSE" );
		fprintf ( OUT, "\t\t\t\tEnableFunctionLevelLinking=\"%s\"\r\n", debug ? "TRUE" : "FALSE" );
		fprintf ( OUT, "\t\t\t\tUsePrecompiledHeader=\"0\"\r\n" );
		fprintf ( OUT, "\t\t\t\tWarningLevel=\"1\"\r\n" );
		fprintf ( OUT, "\t\t\t\tDetect64BitPortabilityProblems=\"TRUE\"\r\n" );
		fprintf ( OUT, "\t\t\t\tDebugInformationFormat=\"4\"/>\r\n" );

		fprintf ( OUT, "\t\t\t<Tool\r\n" );
		fprintf ( OUT, "\t\t\t\tName=\"VCCustomBuildTool\"/>\r\n" );

		if ( lib )
		{
			fprintf ( OUT, "\t\t\t<Tool\r\n" );
			fprintf ( OUT, "\t\t\t\tName=\"VCLibrarianTool\"\r\n" );
			fprintf ( OUT, "\t\t\t\tOutputFile=\"$(OutDir)/%s%s\"/>\r\n", module.name.c_str(), module_type.c_str() );
		}
		else
		{
			fprintf ( OUT, "\t\t\t<Tool\r\n" );
			fprintf ( OUT, "\t\t\t\tName=\"VCLinkerTool\"\r\n" );

			fprintf ( OUT, "\t\t\t\tAdditionalDependencies=\"" );
			for ( i = 0; i < libraries.size(); i++ )
			{
				if ( i > 0 )
					fprintf ( OUT, " " );
				fprintf ( OUT, "%s", libraries[i].c_str() );
			}
			fprintf ( OUT, "\"\r\n" );

			fprintf ( OUT, "\t\t\t\tOutputFile=\"$(OutDir)/%s%s\"\r\n", module.name.c_str(), module_type.c_str() );
			fprintf ( OUT, "\t\t\t\tLinkIncremental=\"%d\"\r\n", debug ? 2 : 1 );
			fprintf ( OUT, "\t\t\t\tGenerateDebugInformation=\"TRUE\"\r\n" );

			if ( debug )
				fprintf ( OUT, "\t\t\t\tProgramDatabaseFile=\"$(OutDir)/%s.pdb\"\r\n", module.name.c_str() );

			fprintf ( OUT, "\t\t\t\tSubSystem=\"%d\"\r\n", console ? 1 : 2 );
			fprintf ( OUT, "\t\t\t\tTargetMachine=\"%d\"/>\r\n", 1 );
		}
		
		fprintf ( OUT, "\t\t\t<Tool\r\n" );
		fprintf ( OUT, "\t\t\t\tName=\"VCResourceCompilerTool\"\r\n" );
		fprintf ( OUT, "\t\t\t\tAdditionalIncludeDirectories=\"" );
		multiple_includes = false;
		fprintf ( OUT, "./;" );
		for ( i = 0; i < includes.size(); i++ )
		{
			const string& include = includes[i];
			if ( strcmp ( include.c_str(), "." ) )
			{
				if ( multiple_includes )
					fprintf ( OUT, ";" );
				fprintf ( OUT, "%s", include.c_str() );
				multiple_includes = true;
			}
		}
		fprintf ( OUT, "\"/>\r\n " );

		fprintf ( OUT, "\t\t\t<Tool\r\n" );
		fprintf ( OUT, "\t\t\t\tName=\"VCMIDLTool\"/>\r\n" );
		fprintf ( OUT, "\t\t\t<Tool\r\n" );
		fprintf ( OUT, "\t\t\t\tName=\"VCPostBuildEventTool\"/>\r\n" );
		fprintf ( OUT, "\t\t\t<Tool\r\n" );
		fprintf ( OUT, "\t\t\t\tName=\"VCPreBuildEventTool\"/>\r\n" );
		fprintf ( OUT, "\t\t\t<Tool\r\n" );
		fprintf ( OUT, "\t\t\t\tName=\"VCPreLinkEventTool\"/>\r\n" );
		fprintf ( OUT, "\t\t\t<Tool\r\n" );
		fprintf ( OUT, "\t\t\t\tName=\"VCWebServiceProxyGeneratorTool\"/>\r\n" );
		fprintf ( OUT, "\t\t\t<Tool\r\n" );
		fprintf ( OUT, "\t\t\t\tName=\"VCWebDeploymentTool\"/>\r\n" );
		fprintf ( OUT, "\t\t</Configuration>\r\n" );

		n++;
	}
	fprintf ( OUT, "\t</Configurations>\r\n" );

	fprintf ( OUT, "\t<Files>\r\n" );

	// Source files
	fprintf ( OUT, "\t\t<Filter\r\n" );
	fprintf ( OUT, "\t\t\tName=\"Source Files\"\r\n" );
	fprintf ( OUT, "\t\t\tFilter=\"cpp;c;cxx;rc;def;r;odl;idl;hpj;bat\">\r\n" );
	for ( size_t isrcfile = 0; isrcfile < source_files.size(); isrcfile++ )
	{
		const string& source_file = DosSeparator(source_files[isrcfile]);
		fprintf ( OUT, "\t\t\t<File\r\n" );
		fprintf ( OUT, "\t\t\t\tRelativePath=\"%s\">\r\n", source_file.c_str() );
		fprintf ( OUT, "\t\t\t</File>\r\n" );
	}
	fprintf ( OUT, "\t\t</Filter>\r\n" );

	// Header files
	fprintf ( OUT, "\t\t<Filter\r\n" );
	fprintf ( OUT, "\t\t\tName=\"Header Files\"\r\n" );
	fprintf ( OUT, "\t\t\tFilter=\"h;hpp;hxx;hm;inl\">\r\n" );
	for ( i = 0; i < header_files.size(); i++ )
	{
		const string& header_file = header_files[i];
		fprintf ( OUT, "\t\t\t<File\r\n" );
		fprintf ( OUT, "\t\t\t\tRelativePath=\"%s\">\r\n", header_file.c_str() );
		fprintf ( OUT, "\t\t\t</File>\r\n" );
	}
	fprintf ( OUT, "\t\t</Filter>\r\n" );

	// Resource files
	fprintf ( OUT, "\t\t<Filter\r\n" );
	fprintf ( OUT, "\t\t\tName=\"Resource Files\"\r\n" );
	fprintf ( OUT, "\t\t\tFilter=\"ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe\">\r\n" );
	for ( i = 0; i < header_files.size(); i++ )
	{
		const string& resource_file = resource_files[i];
		fprintf ( OUT, "\t\t\t<File\r\n" );
		fprintf ( OUT, "\t\t\t\tRelativePath=\"%s\">\r\n", resource_file.c_str() );
		fprintf ( OUT, "\t\t\t</File>\r\n" );
	}
	fprintf ( OUT, "\t\t</Filter>\r\n" );

	fprintf ( OUT, "\t</Files>\r\n" );
	fprintf ( OUT, "\t<Globals>\r\n" );
	fprintf ( OUT, "\t</Globals>\r\n" );
	fprintf ( OUT, "</VisualStudioProject>\r\n" );
	fclose(OUT);
}

std::string
MSVCBackend::_replace_str(std::string string1, const std::string &find_str, const std::string &replace_str)
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

void
MSVCBackend::_generate_sln_header ( FILE* OUT )
{
    if (configuration.VSProjectVersion.empty())
        configuration.VSProjectVersion = MS_VS_DEF_VERSION;

    string version;

    if (configuration.VSProjectVersion == "7.00")
		version = "7.00";

    if (configuration.VSProjectVersion == "7.10")
		version = "8.00";

    if (configuration.VSProjectVersion == "8.00")
		version = "9.00";

    fprintf ( OUT, "Microsoft Visual Studio Solution File, Format Version %s\r\n", version.c_str() );
    fprintf ( OUT, "# Visual Studio 2005\r\n" );
    fprintf ( OUT, "\r\n" );
}


void
MSVCBackend::_generate_sln_project (
	FILE* OUT,
	const Module& module,
	std::string vcproj_file,
	std::string sln_guid,
	std::string vcproj_guid,
	const std::vector<Dependency*>& dependencies )
{
	vcproj_file = DosSeparator ( std::string(".\\") + vcproj_file );

	fprintf ( OUT, "Project(\"%s\") = \"%s\", \"%s\", \"%s\"\r\n", sln_guid.c_str() , module.name.c_str(), vcproj_file.c_str(), vcproj_guid.c_str() );

	//FIXME: only omit ProjectDependencies in VS 2005 when there are no dependencies
	//NOTE: VS 2002 do not use ProjectSection; it uses GlobalSection instead
	if ((configuration.VSProjectVersion == "7.10") || (dependencies.size() > 0)) {
		fprintf ( OUT, "\tProjectSection(ProjectDependencies) = postProject\r\n" );
		for ( size_t i = 0; i < dependencies.size(); i++ )
		{
			Dependency& dependency = *dependencies[i];
			fprintf ( OUT, "\t\t%s = %s\r\n", dependency.module.guid.c_str(), dependency.module.guid.c_str() );
		}
		fprintf ( OUT, "\tEndProjectSection\r\n" );
	}

	fprintf ( OUT, "EndProject\r\n" );
}


void
MSVCBackend::_generate_sln_footer ( FILE* OUT )
{
	fprintf ( OUT, "Global\r\n" );
	fprintf ( OUT, "\tGlobalSection(SolutionConfiguration) = preSolution\r\n" );
	fprintf ( OUT, "\t\tDebug = Debug\r\n" );
	fprintf ( OUT, "\t\tRelease = Release\r\n" );
	fprintf ( OUT, "\tEndGlobalSection\r\n" );
	fprintf ( OUT, "\tGlobalSection(ProjectConfiguration) = postSolution\r\n" );
	for ( size_t i = 0; i < ProjectNode.modules.size(); i++ )
	{
		Module& module = *ProjectNode.modules[i];
		std::string guid = module.guid;
		_generate_sln_configurations ( OUT, guid.c_str() );
	} 
	fprintf ( OUT, "\tEndGlobalSection\r\n" );
	fprintf ( OUT, "\tGlobalSection(ExtensibilityGlobals) = postSolution\r\n" );
	fprintf ( OUT, "\tEndGlobalSection\r\n" );
	fprintf ( OUT, "\tGlobalSection(ExtensibilityAddIns) = postSolution\r\n" );
	fprintf ( OUT, "\tEndGlobalSection\r\n" );
	
	if (configuration.VSProjectVersion == "7.00") {
		fprintf ( OUT, "\tGlobalSection(ProjectDependencies) = postSolution\r\n" );
		//FIXME: Add dependencies for VS 2002
		fprintf ( OUT, "\tEndGlobalSection\r\n" );
	}

	if (configuration.VSProjectVersion == "8.00") {
		fprintf ( OUT, "\tGlobalSection(SolutionProperties) = preSolution\r\n" );
        	fprintf ( OUT, "\t\tHideSolutionNode = FALSE\r\n" );
		fprintf ( OUT, "\tEndGlobalSection\r\n" );
	}

	fprintf ( OUT, "EndGlobal\r\n" );
	fprintf ( OUT, "\r\n" );
}


void
MSVCBackend::_generate_sln_configurations ( FILE* OUT, std::string vcproj_guid )
{
	fprintf ( OUT, "\t\t%s.Debug.ActiveCfg = Debug|Win32\r\n", vcproj_guid.c_str() );
	fprintf ( OUT, "\t\t%s.Debug.Build.0 = Debug|Win32\r\n", vcproj_guid.c_str() );
	fprintf ( OUT, "\t\t%s.Debug.Release.ActiveCfg = Release|Win32\r\n", vcproj_guid.c_str() );
	fprintf ( OUT, "\t\t%s.Debug.Release.Build.0 = Release|Win32\r\n", vcproj_guid.c_str() );
}

void
MSVCBackend::_generate_sln ( FILE* OUT )
{
	string sln_guid = "{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}";
	vector<string> guids;

	_generate_sln_header(OUT);
	// TODO FIXME - is it necessary to sort them?
	for ( size_t i = 0; i < ProjectNode.modules.size(); i++ )
	{
		Module& module = *ProjectNode.modules[i];
		
		std::string vcproj_file = VcprojFileName ( module );
		_generate_sln_project ( OUT, module, vcproj_file, sln_guid, module.guid, module.dependencies );
	}
	_generate_sln_footer ( OUT );
}

