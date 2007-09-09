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
#include <set>

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
MSVCBackend::_generate_dsp ( const Module& module )
{
	size_t i;
	// TODO FIXME wine hack?
	const bool wine = false;

	string dsp_file = DspFileName(module);
	printf ( "Creating MSVC project: '%s'\n", dsp_file.c_str() );
	FILE* OUT = fopen ( dsp_file.c_str(), "wb" );

	vector<string> imports;
	for ( i = 0; i < module.non_if_data.libraries.size(); i++ )
	{
		imports.push_back ( module.non_if_data.libraries[i]->name );
	}

	string module_type = GetExtension(module.output->name);
	bool lib = (module_type == ".lib") || (module_type == ".a");
	bool dll = (module_type == ".dll") || (module_type == ".cpl");
	bool exe = (module_type == ".exe") || (module_type == ".scr");
	// TODO FIXME - need more checks here for 'sys' and possibly 'drv'?

	bool console = exe && (module.type == Win32CUI);

	// TODO FIXME - not sure if the count here is right...
	int parts = 0;
	const char* p = strpbrk ( dsp_file.c_str(), "/\\" );
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
	string dsp_path = module.output->relative_path;
	vector<string> c_srcs, source_files, header_files, resource_files, includes, libraries;
	StringSet common_defines;
	vector<const IfableData*> ifs_list;
	ifs_list.push_back ( &module.project.non_if_data );
	ifs_list.push_back ( &module.non_if_data );

	// this is a define in MinGW w32api, but not Microsoft's headers
	common_defines.insert ( "STDCALL=__stdcall" );

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
			string file = string(".") + &files[i]->name[dsp_path.size()];

			source_files.push_back ( file );
			if ( !stricmp ( Right(file,2).c_str(), ".c" ) )
				c_srcs.push_back ( file );
			if ( !stricmp ( Right(file,2).c_str(), ".h" ) )
				header_files.push_back ( file );
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
				module.output->relative_path );
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
				common_defines.insert( defs[i]->name + "=" + defs[i]->value );
			else
				common_defines.insert( defs[i]->name );
		}
	}
	// TODO FIXME - we don't include header files in our build system
	//my @header_files = @{module->{header_files}};
	//vector<string> header_files;

	// TODO FIXME - wine hack?
	/*if (module.name !~ /^wine(?:_unicode|build|runtests|test)?$/ &&
		module.name !~ /^(?:gdi32)_.+?$/ &&
		Right ( module.name, 5 ) == "_test" )
	{
		source_files.push_back ( module.name + ".spec" );
		@source_files = sort(@source_files);
	}*/

	bool no_cpp = true;
	bool no_msvc_headers = true;
	// TODO FIXME - wine hack?
	/*if (module.name =~ /^wine(?:runtests|test)$/
		|| Right ( module.name, 5 ) == "_test" )
	{
		no_msvc_headers = false;
	}*/

	std::vector<std::string> cfgs;

	cfgs.push_back ( module.name + " - Win32" );

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

	// TODO FIXME - wine hack?
	/*if (!no_release)
	{
		std::vector<std::string> _cfgs;
		for ( i = 0; i < cfgs.size(); i++ )
		{
			_cfgs.push_back ( cfgs[i] + " Debug" );
			_cfgs.push_back ( cfgs[i] + " Release" );
		}
		cfgs.resize(0);
		cfgs = _cfgs;
	}*/

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

	fprintf ( OUT, "# Microsoft Developer Studio Project File - Name=\"%s\" - Package Owner=<4>\r\n", module.name.c_str() );
	fprintf ( OUT, "# Microsoft Developer Studio Generated Build File, Format Version 6.00\r\n" );
	fprintf ( OUT, "# ** DO NOT EDIT **\r\n" );
	fprintf ( OUT, "\r\n" );

	if ( lib )
	{
		fprintf ( OUT, "# TARGTYPE \"Win32 (x86) Static Library\" 0x0104\r\n" );
	}
	else if ( dll )
	{
		fprintf ( OUT, "# TARGTYPE \"Win32 (x86) Dynamic-Link Library\" 0x0102\r\n" );
	}
	else
	{
		fprintf ( OUT, "# TARGTYPE \"Win32 (x86) Console Application\" 0x0103\r\n" );
	}
	fprintf ( OUT, "\r\n" );

	fprintf ( OUT, "CFG=%s\r\n", default_cfg.c_str() );
	fprintf ( OUT, "!MESSAGE This is not a valid makefile. To build this project using NMAKE,\r\n" );
	fprintf ( OUT, "!MESSAGE use the Export Makefile command and run\r\n" );
	fprintf ( OUT, "!MESSAGE \r\n" );
	fprintf ( OUT, "!MESSAGE NMAKE /f \"%s.mak\".\r\n", module.name.c_str() );
	fprintf ( OUT, "!MESSAGE \r\n" );
	fprintf ( OUT, "!MESSAGE You can specify a configuration when running NMAKE\r\n" );
	fprintf ( OUT, "!MESSAGE by defining the macro CFG on the command line. For example:\r\n" );
	fprintf ( OUT, "!MESSAGE \r\n" );
	fprintf ( OUT, "!MESSAGE NMAKE /f \"%s.mak\" CFG=\"%s\"\r\n", module.name.c_str(), default_cfg.c_str() );
	fprintf ( OUT, "!MESSAGE \r\n" );
	fprintf ( OUT, "!MESSAGE Possible choices for configuration are:\r\n" );
	fprintf ( OUT, "!MESSAGE \r\n" );
	for ( i = 0; i < cfgs.size(); i++ )
	{
		const string& cfg = cfgs[i];
		if ( lib )
		{
			fprintf ( OUT, "!MESSAGE \"%s\" (based on \"Win32 (x86) Static Library\")\r\n", cfg.c_str() );
		}
		else if ( dll )
		{
			fprintf ( OUT, "!MESSAGE \"%s\" (based on \"Win32 (x86) Dynamic-Link Library\")\r\n", cfg.c_str() );
		}
		else
		{
			fprintf ( OUT, "!MESSAGE \"%s\" (based on \"Win32 (x86) Console Application\")\r\n", cfg.c_str() );
		}
	}
	fprintf ( OUT, "!MESSAGE \r\n" );
	fprintf ( OUT, "\r\n" );

	fprintf ( OUT, "# Begin Project\r\n" );
	fprintf ( OUT, "# PROP AllowPerConfigDependencies 0\r\n" );
	fprintf ( OUT, "# PROP Scc_ProjName \"\"\r\n" );
	fprintf ( OUT, "# PROP Scc_LocalPath \"\"\r\n" );
	fprintf ( OUT, "CPP=cl.exe\r\n" );
	if ( !lib && !exe ) fprintf ( OUT, "MTL=midl.exe\r\n" );
	fprintf ( OUT, "RSC=rc.exe\r\n" );

	std::string output_dir;
	for ( size_t icfg = 0; icfg < cfgs.size(); icfg++ )
	{
		std::string& cfg = cfgs[icfg];
		if ( icfg == 0 )
		{
			fprintf ( OUT, "!IF  \"$(CFG)\" == \"%s\"\r\n", cfg.c_str() );
			fprintf ( OUT, "\r\n" );
		}
		else
		{
			fprintf ( OUT, "\r\n" );
			fprintf ( OUT, "!ELSEIF  \"$(CFG)\" == \"%s\"\r\n", cfg.c_str() );
			fprintf ( OUT, "\r\n" );
		}

		bool debug = !strstr ( cfg.c_str(), "Release" );
		bool msvc_headers = ( 0 != strstr ( cfg.c_str(), "MSVC Headers" ) );

		fprintf ( OUT, "# PROP BASE Use_MFC 0\r\n" );

		if ( debug )
		{
			fprintf ( OUT, "# PROP BASE Use_Debug_Libraries 1\r\n" );
		}
		else
		{
			fprintf ( OUT, "# PROP BASE Use_Debug_Libraries 0\r\n" );
		}

		output_dir = Replace(cfg,module.name + " - ","");
		output_dir = Replace(output_dir," ","_");
		output_dir = Replace(output_dir,"C++","Cxx");
		// TODO FIXME - wine hack?
		//if ( output_prefix_dir.size() )
		//	output_dir = output_prefix_dir + "\\" + output_dir;

		fprintf ( OUT, "# PROP BASE Output_Dir \"%s\"\r\n", output_dir.c_str() );
		fprintf ( OUT, "# PROP BASE Intermediate_Dir \"%s\"\r\n", output_dir.c_str() );

		fprintf ( OUT, "# PROP BASE Target_Dir \"\"\r\n" );

		fprintf ( OUT, "# PROP Use_MFC 0\r\n" );
		if ( debug )
		{
			fprintf ( OUT, "# PROP Use_Debug_Libraries 1\r\n" );
		}
		else
		{
			fprintf ( OUT, "# PROP Use_Debug_Libraries 0\r\n" );
		}
		fprintf ( OUT, "# PROP Output_Dir \"%s\"\r\n", output_dir.c_str() );
		fprintf ( OUT, "# PROP Intermediate_Dir \"%s\"\r\n", output_dir.c_str() );

		if ( dll ) fprintf ( OUT, "# PROP Ignore_Export_Lib 0\r\n" );
		fprintf ( OUT, "# PROP Target_Dir \"\"\r\n" );

		StringSet defines = common_defines;

		if ( debug )
		{
			defines.insert ( "_DEBUG" );
			if ( lib || exe )
			{
				fprintf ( OUT, "# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od" );
				defines.insert ( "_LIB" );
			}
			else
			{
				fprintf ( OUT, "# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od" );
				defines.insert ( "_WINDOWS" );
				defines.insert ( "_USRDLL" );
				// TODO FIXME - wine hack?
				//defines.insert ( string("\U") + module.name + "\E_EXPORTS" );
			}
		}
		else
		{
			defines.insert ( "NDEBUG" );
			if ( lib || exe )
			{
				fprintf ( OUT, "# ADD BASE CPP /nologo /W3 /GX /O2" );
				defines.insert ( "_LIB" );
			}
			else
			{
				fprintf ( OUT, "# ADD BASE CPP /nologo /MT /W3 /GX /O2" );
				defines.insert ( "_WINDOWS" );
				defines.insert ( "_USRDLL" );
				// TODO FIXME - wine hack?
				//defines.insert ( string("\U") + module.name + "\E_EXPORTS" );
			}
		}

		for ( StringSet::const_iterator it1=defines.begin(); it1!=defines.end(); it1++ )
		{
			fprintf ( OUT, " /D \"%s\"", it1->c_str() );
		}
		if ( lib || exe ) fprintf ( OUT, " /YX" );
		fprintf ( OUT, " /FD" );
		if ( debug )
		{
			fprintf ( OUT, " /GZ" );
			if ( lib || exe ) fprintf ( OUT, " " );
		}
		fprintf ( OUT, " /c" );
		fprintf ( OUT, "\r\n" );

		if ( debug )
		{
			defines.insert ( "_DEBUG" );
			if(lib)
			{
				fprintf ( OUT, "# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od" );
				defines.insert ( "_LIB" );
			}
			else
			{
				fprintf ( OUT, "# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od" );
				defines.insert ( "_USRDLL" );
			}
		}
		else
		{
			defines.insert ( "NDEBUG" );
			if(lib)
			{
				fprintf ( OUT, "# ADD CPP /nologo /MT /W3 /GX /O2" );
				defines.insert ( "_LIB" );
			}
			else
			{
				fprintf ( OUT, "# ADD CPP /nologo /MT /W3 /GX /O2" );
				defines.insert ( "_USRDLL" );
			}
		}

		// TODO FIXME - wine hack?
		if ( wine )
		{
			// TODO FIXME - wine hack?
			//defines.insert ( string("_\U") + module.name + "\E_" );
			// TODO FIXME - wine hack?
			/*if ( module.name !~ /^(?:wine(?:build|test)|.*?_test)$/ )
				defines.insert ( "__WINESRC__" );*/
			if ( msvc_headers )
				defines.insert ( "__WINE_USE_NATIVE_HEADERS" );
			string output_dir2 = Replace(output_dir,"\\","\\\\");
			defines.insert ( ssprintf("__WINETEST_OUTPUT_DIR=\\\"%s\\\"",output_dir.c_str()) );
			defines.insert ( "__i386__" );
			defines.insert ( "_X86_" );

			// TODO FIXME - wine hacks?
			/*if(module.name =~ /^gdi32_(?:enhmfdrv|mfdrv)$/) {
				push @includes, ".." );
			}

			if ( strstr ( module.name.c_str(), "_test" )
			{
				include.push_back ( msvc_wine_dir + "\\" + output_dir );
			}

			if (!msvc_headers || module.name == "winetest")
			{
				includes.push_back ( wine_include_dir );
			}*/
		}

		//if ( wine )
		{
			for ( i = 0; i < includes.size(); i++ )
			{
				const string& include = includes[i];
				if ( strpbrk ( include.c_str(), "[\\\"]" ) || !strncmp ( include.c_str(), "../", 3 ) )
				{
					fprintf ( OUT, " /I \"%s\"", include.c_str() );
				}
				else
				{
					fprintf ( OUT, " /I %s", include.c_str() );
				}
			}
		}

		fprintf ( OUT, " /I \".\"" );
		for ( StringSet::const_iterator it2=defines.begin(); it2!=defines.end(); it2++ )
		{
			const string& define = *it2;
			if ( strpbrk ( define.c_str(), "[\\\"]" ) )
			{
				fprintf ( OUT, " /D \"%s\"", define.c_str() );
			}
			else
			{
				fprintf ( OUT, " /D %s", define.c_str() );
			}
		}
		if ( wine ) fprintf ( OUT, " /D inline=__inline" );
		if ( 0 && wine ) fprintf ( OUT, " /D \"__STDC__\"" );

		fprintf ( OUT, lib ? " /YX" : " /FR" );
		fprintf ( OUT, " /FD" );
		if ( debug ) fprintf ( OUT, " /GZ" );
		if ( debug && lib ) fprintf ( OUT, " " );
		fprintf ( OUT, " /c" );
		if ( !no_cpp ) fprintf ( OUT, " /TP" );
		fprintf ( OUT, "\r\n" );

		if ( debug )
		{
			if ( dll )
			{
				fprintf ( OUT, "# SUBTRACT CPP /X /YX\r\n" );
				fprintf ( OUT, "# ADD BASE MTL /nologo /D \"_DEBUG\" /mktyplib203 /win32\r\n" );
				fprintf ( OUT, "# ADD MTL /nologo /D \"_DEBUG\" /mktyplib203 /win32\r\n" );
			}
			fprintf ( OUT, "# ADD BASE RSC /l 0x41d /d \"_DEBUG\"\r\n" );
			fprintf ( OUT, "# ADD RSC /l 0x41d" );
			/*if ( wine )*/
			{
				for ( i = 0; i < includes.size(); i++ )
				{
					fprintf ( OUT, " /i \"%s\"", includes[i].c_str() );
				}
			}

			for ( StringSet::const_iterator it3=defines.begin(); it3!=defines.end(); it3++ )
			{
				fprintf ( OUT, " /D \"%s\"", it3->c_str() );
			}
			fprintf ( OUT, " /d \"_DEBUG\"\r\n" );
		}
		else
		{
			if ( dll )
			{
				fprintf ( OUT, "# SUBTRACT CPP /YX\r\n" );
				fprintf ( OUT, "# ADD BASE MTL /nologo /D \"NDEBUG\" /mktyplib203 /win32\r\n" );
				fprintf ( OUT, "# ADD MTL /nologo /D \"NDEBUG\" /mktyplib203 /win32\r\n" );
			}
			fprintf ( OUT, "# ADD BASE RSC /l 0x41d /d \"NDEBUG\"\r\n" );
			fprintf ( OUT, "# ADD RSC /l 0x41d" );
			if ( wine )
			{
				for ( i = 0; i < includes.size(); i++ )
					fprintf ( OUT, " /i \"%s\"", includes[i].c_str() );
			}

			for ( StringSet::const_iterator it4=defines.begin(); it4!=defines.end(); it4++ )
			{
				fprintf ( OUT, " /D \"%s\"", it4->c_str() );
			}


			fprintf ( OUT, "/d \"NDEBUG\"\r\n" );
		}
		fprintf ( OUT, "BSC32=bscmake.exe\r\n" );
		fprintf ( OUT, "# ADD BASE BSC32 /nologo\r\n" );
		fprintf ( OUT, "# ADD BSC32 /nologo\r\n" );

		if ( exe || dll )
		{
			fprintf ( OUT, "LINK32=link.exe\r\n" );
			fprintf ( OUT, "# ADD BASE LINK32 " );

			for ( i = 0; i < libraries.size(); i++ )
			{
				fprintf ( OUT, "%s ", libraries[i].c_str() );
			}
			fprintf ( OUT, " /nologo" );
			if ( dll ) fprintf ( OUT, " /dll" );
			if ( console ) fprintf ( OUT, " /subsystem:console" );
			if ( debug ) fprintf ( OUT, " /debug" );
			fprintf ( OUT, " /machine:I386" );
			if ( debug ) fprintf ( OUT, " /pdbtype:sept" );
			fprintf ( OUT, "\r\n" );

			fprintf ( OUT, "# ADD LINK32" );
			fprintf ( OUT, " /nologo" );
			// TODO FIXME - do we need their kludge?
			//if ( module.name == "ntdll" ) fprintf ( OUT, " libcmt.lib" ); // FIXME: Kludge
			for ( i = 0; i < imports.size(); i++ )
			{
				const string& import = imports[i];
				if ( import != "msvcrt" )
					fprintf ( OUT, " %s.lib", import.c_str() );
			}
			if ( dll ) fprintf ( OUT, " /dll" );
			if ( console ) fprintf ( OUT, " /subsystem:console" );
			if ( debug ) fprintf ( OUT, " /debug" );
			fprintf ( OUT, " /machine:I386" );
			// TODO FIXME - do we need their kludge?
			//if ( module.name == "ntdll" ) fprintf ( OUT, " /nodefaultlib" ); // FIXME: Kludge
			if ( dll ) fprintf ( OUT, " /def:\"%s.def\"", module.name.c_str() );
			if (( dll ) && ( module_type == ".cpl")) fprintf ( OUT, " /out:\"Win32\\%s%s\"", module.name.c_str(), module_type.c_str() );
			if ( debug ) fprintf ( OUT, " /pdbtype:sept" );
			fprintf ( OUT, "\r\n" );
		}
		else
		{
			fprintf ( OUT, "LIB32=link.exe -lib\r\n" );
			fprintf ( OUT, "# ADD BASE LIB32 /nologo\r\n" );
			fprintf ( OUT, "# ADD LIB32 /nologo\r\n" );
		}
	}

	if ( cfgs.size() != 0 )
	{
		fprintf ( OUT, "\r\n" );
		fprintf ( OUT, "!ENDIF \r\n" );
		fprintf ( OUT, "\r\n" );
	}
#if 0
	if ( module.name == "winebuild" )
	{
		fprintf ( OUT, "# Begin Special Build Tool\r\n" );
		fprintf ( OUT, "SOURCE=\"$(InputPath)\"\r\n" );
		fprintf ( OUT, "PostBuild_Desc=Copying wine.dll and wine_unicode.dll ...\r\n" );
		fprintf ( OUT, "PostBuild_Cmds=" );
		fprintf ( OUT, "copy ..\\..\\library\\%s\\wine.dll $(OutDir)\t",
			output_dir.c_str() );
		fprintf ( OUT, "copy ..\\..\\unicode\\%s\\wine_unicode.dll $(OutDir)\r\n",
			output_dir.c_str() );
		fprintf ( OUT, "# End Special Build Tool\r\n" );
	}
#endif
	fprintf ( OUT, "# Begin Target\r\n" );
	fprintf ( OUT, "\r\n" );
	for ( i = 0; i < cfgs.size(); i++ )
	{
		fprintf ( OUT, "# Name \"%s\"\r\n", cfgs[i].c_str() );
	}

	fprintf ( OUT, "# Begin Group \"Source Files\"\r\n" );
	fprintf ( OUT, "\r\n" );
	fprintf ( OUT, "# PROP Default_Filter \"cpp;c;cxx;rc;def;r;odl;idl;hpj;bat\"\r\n" );

	for ( size_t isrcfile = 0; isrcfile < source_files.size(); isrcfile++ )
	{
		string source_file = DosSeparator(source_files[isrcfile]);

		if ( strncmp ( source_file.c_str(), ".\\", 2 ) )
		{
			source_file = string(".\\") + source_file;
		}
#if 0
		if ( !strcmp ( &source_file[source_file.size()-5], ".spec" ) )
		{
			string basename = string ( source_file.c_str(), source_file.size() - 5 );

			// TODO FIXME - not sure what this is doing? wine hack maybe?
			//if ( basename !~ /\..{1,3}$/; ) basename += string(".dll");
			string dbg_c_file = basename + ".dbg.c";

			fprintf ( OUT, "# Begin Source File\r\n" );
			fprintf ( OUT, "\r\n" );
			fprintf ( OUT, "SOURCE=%s\r\n", dbg_c_file.c_str() );
			fprintf ( OUT, "# End Source File\r\n" );
		}
#endif
		fprintf ( OUT, "# Begin Source File\r\n" );
		fprintf ( OUT, "\r\n" );

		fprintf ( OUT, "SOURCE=%s\r\n", source_file.c_str() );

		if ( !strcmp ( &source_file[source_file.size()-5], ".spec" ) )
		{
#if 0
			string basename = string ( source_file.c_str(), source_file.size() - 5 );

			string spec_file = source_file;
			string def_file = basename + ".def";

			// TODO FIXME - not sure what this is doing? wine hack maybe?
			//if ( basename !~ /\..{1,3}$/; ) basename += ".dll";
			string dbg_file = basename + ".dbg";
			string dbg_c_file = basename + ".dbg.c";

			string srcdir = "."; // FIXME: Is this really always correct?

			fprintf ( OUT, "# Begin Custom Build\r\n" );
			fprintf ( OUT, "InputPath=%s\r\n", spec_file.c_str() );
			fprintf ( OUT, "\r\n" );
			fprintf ( OUT, "BuildCmds= \\\r\n" );
			fprintf ( OUT, "\t..\\..\\tools\\winebuild\\%s\\winebuild.exe --def %s > %s \\\r\n",
				output_dir.c_str(),
				spec_file.c_str(),
				def_file.c_str() );
			
			if ( module.name == "ntdll" )
			{
				int n = 0;
				for ( i = 0; i < c_srcs.size(); i++ )
				{
					const string& c_src = c_srcs[i];
					if(n++ > 0)
					{
						fprintf ( OUT, "\techo %s >> %s \\\r\n", c_src.c_str(), dbg_file.c_str() );
					}
					else
					{
						fprintf ( OUT, "\techo %s > %s \\\r\n", c_src.c_str(), dbg_file.c_str() );
					}
				}
				fprintf ( OUT, "\t..\\..\\tools\\winebuild\\%s\\winebuild.exe",
					output_dir.c_str() );
				fprintf ( OUT, " -o %s --debug -C%s %s \\\r\n",
					dbg_c_file.c_str(),
					srcdir.c_str(),
					dbg_file.c_str() );
			}
			else
			{
				string sc_srcs;
				for ( i = 0; i < c_srcs.size(); i++ )
				{
					const string& c_src = c_srcs[i];
					if ( !strcmp ( &c_src[c_src.size()-2], ".c" ) )
					{
						if ( sc_srcs.size() )
							sc_srcs += " ";
						sc_srcs += c_src;
					}
				}

				fprintf ( OUT, "\t..\\..\\tools\\winebuild\\%s\\winebuild.exe",
					output_dir.c_str() );
				fprintf ( OUT, " -o %s --debug -C%s %s \\\r\n",
					dbg_c_file.c_str(),
					srcdir.c_str(),
					sc_srcs.c_str() );
			}

			fprintf ( OUT, "\t\r\n" );
			fprintf ( OUT, "\r\n" );
			fprintf ( OUT, "\"%s\" : $(SOURCE) \"$(INTDIR)\" \"$(OUTDIR)\"\r\n", def_file.c_str() );
			fprintf ( OUT, "   $(BuildCmds)\r\n" );
			fprintf ( OUT, "\r\n" );
			fprintf ( OUT, "\"%s\" : $(SOURCE) \"$(INTDIR)\" \"$(OUTDIR)\"\r\n", dbg_c_file.c_str() );
			fprintf ( OUT, "   $(BuildCmds)\r\n" );
			fprintf ( OUT, "# End Custom Build\r\n" );
#endif
		}
		/*else if ( source_file =~ /([^\\]*?\.h)$/ )
		{
			my $h_file = $1;

			foreach my $cfg (@cfgs) {
				if($#cfgs == 0) {
					# Nothing
				} elsif($n == 0) {
					fprintf ( OUT, "!IF  \"$(CFG)\" == \"$cfg\"\r\n" );
					fprintf ( OUT, "\r\n" );
				} else {
					fprintf ( OUT, "\r\n" );
					fprintf ( OUT, "!ELSEIF  \"$(CFG)\" == \"$cfg\"\r\n" );
					fprintf ( OUT, "\r\n" );
				}

				$output_dir = $cfg;
				$output_dir =~ s/^$project - //;
				$output_dir =~ s/ /_/g;
				$output_dir =~ s/C\+\+/Cxx/g;
				if($output_prefix_dir) {
					$output_dir = "$output_prefix_dir\\$output_dir" );
				}

				fprintf ( OUT, "# Begin Custom Build\r\n" );
				fprintf ( OUT, "OutDir=%s\r\n", output_dir.c_str() );
				fprintf ( OUT, "InputPath=%s\r\n", source_file.c_str() );
				fprintf ( OUT, "\r\n" );
				fprintf ( OUT, "\"$(OutDir)\\wine\\%s\" : $(SOURCE) \"$(INTDIR)\" \"$(OUTDIR)\"\r\n", h_file.c_str() );
				fprintf ( OUT, "\tcopy \"$(InputPath)\" \"$(OutDir)\\wine\"\r\n" );
				fprintf ( OUT, "\r\n" );
				fprintf ( OUT, "# End Custom Build\r\n" );
			}

			if ( cfgs.size() != 0)
			{
				fprintf ( OUT, "\r\n" );
				fprintf ( OUT, "!ENDIF \r\n" );
				fprintf ( OUT, "\r\n" );
			}
		}*/

		fprintf ( OUT, "# End Source File\r\n" );
	}
	fprintf ( OUT, "# End Group\r\n" );
	fprintf ( OUT, "# Begin Group \"Header Files\"\r\n" );
	fprintf ( OUT, "\r\n" );
	fprintf ( OUT, "# PROP Default_Filter \"h;hpp;hxx;hm;inl\"\r\n" );
	for ( i = 0; i < header_files.size(); i++ )
	{
		const string& header_file = header_files[i];
		fprintf ( OUT, "# Begin Source File\r\n" );
		fprintf ( OUT, "\r\n" );
		fprintf ( OUT, "SOURCE=.\\%s\r\n", header_file.c_str() );
		fprintf ( OUT, "# End Source File\r\n" );
	}
	fprintf ( OUT, "# End Group\r\n" );



	fprintf ( OUT, "# Begin Group \"Resource Files\"\r\n" );
	fprintf ( OUT, "\r\n" );
	fprintf ( OUT, "# PROP Default_Filter \"ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe\"\r\n" );
/*	for ( i = 0; i < resource_files.size(); i++ )
	{
		const string& resource_file = resource_files[i];
		fprintf ( OUT, "# Begin Source File\r\n" );
		fprintf ( OUT, "\r\n" );
		fprintf ( OUT, "SOURCE=.\\%s\r\n", resource_file.c_str() );
		fprintf ( OUT, "# End Source File\r\n" );
	}
*/	fprintf ( OUT, "# End Group\r\n" );

	fprintf ( OUT, "# End Target\r\n" );
	fprintf ( OUT, "# End Project\r\n" );

	fclose(OUT);
}

void
MSVCBackend::_generate_dsw_header ( FILE* OUT )
{
	fprintf ( OUT, "Microsoft Developer Studio Workspace File, Format Version 6.00\r\n" );
	fprintf ( OUT, "# WARNING: DO NOT EDIT OR DELETE THIS WORKSPACE FILE!\r\n" );
	fprintf ( OUT, "\r\n" );
}

void
MSVCBackend::_generate_dsw_project (
	FILE* OUT,
	const Module& module,
	std::string dsp_file,
	const std::vector<Dependency*>& dependencies )
{
	dsp_file = DosSeparator ( std::string(".\\") + dsp_file );

	// TODO FIXME - must they be sorted?
	//@dependencies = sort(@dependencies);

	fprintf ( OUT, "###############################################################################\r\n" );
	fprintf ( OUT, "\r\n" );
	fprintf ( OUT, "Project: \"%s\"=%s - Package Owner=<4>\r\n", module.name.c_str(), dsp_file.c_str() );
	fprintf ( OUT, "\r\n" );
	fprintf ( OUT, "Package=<5>\r\n" );
	fprintf ( OUT, "{{{\r\n" );
	fprintf ( OUT, "}}}\r\n" );
	fprintf ( OUT, "\r\n" );
	fprintf ( OUT, "Package=<4>\r\n" );
	fprintf ( OUT, "{{{\r\n" );
	for ( size_t i = 0; i < dependencies.size(); i++ )
	{
		Dependency& dependency = *dependencies[i];
		fprintf ( OUT, "    Begin Project Dependency\r\n" );
		fprintf ( OUT, "    Project_Dep_Name %s\r\n", dependency.module.name.c_str() );
		fprintf ( OUT, "    End Project Dependency\r\n" );
	}
	fprintf ( OUT, "}}}\r\n" );
	fprintf ( OUT, "\r\n" );
}

void
MSVCBackend::_generate_dsw_footer ( FILE* OUT )
{
	fprintf ( OUT, "###############################################################################\r\n" );
	fprintf ( OUT, "\r\n" );
	fprintf ( OUT, "Global:\r\n" );
	fprintf ( OUT, "\r\n" );
	fprintf ( OUT, "Package=<5>\r\n" );
	fprintf ( OUT, "{{{\r\n" );
	fprintf ( OUT, "}}}\r\n" );
	fprintf ( OUT, "\r\n" );
	fprintf ( OUT, "Package=<3>\r\n" );
	fprintf ( OUT, "{{{\r\n" );
	fprintf ( OUT, "}}}\r\n" );
	fprintf ( OUT, "\r\n" );
	fprintf ( OUT, "###############################################################################\r\n" );
	fprintf ( OUT, "\r\n" );
}

void
MSVCBackend::_generate_wine_dsw ( FILE* OUT )
{
	_generate_dsw_header(OUT);
	// TODO FIXME - is it necessary to sort them?
	for ( size_t i = 0; i < ProjectNode.modules.size(); i++ )
	{
		Module& module = *ProjectNode.modules[i];

		std::string dsp_file = DspFileName ( module );

		// TODO FIXME - more wine hacks?
		/*if ( module.name == "gdi32" )
		{
			for ( size_t idir = 0; idir < gdi32_dirs.size(); idir++ )
			{
				string dir2 = gdi32_dirs[idir];
				$dir2 =~ s%^.*?/([^/]+)$%$1%;

				dependencies.push_back ( Replace ( "gdi32_" + dir2, "/", "_" ) );
			}
		}*/

		_generate_dsw_project ( OUT, module, dsp_file, module.dependencies );
	}
	_generate_dsw_footer ( OUT );
}
