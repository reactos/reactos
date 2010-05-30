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

ProjMaker::ProjMaker ( )
{
	vcproj_file = "";
}

ProjMaker::ProjMaker ( Configuration& buildConfig,
					   const std::vector<MSVCConfiguration*>& msvc_configs,
					   std::string filename )
{
	configuration = buildConfig;
	m_configurations = msvc_configs;
	vcproj_file = filename;
}

void
ProjMaker::_generate_proj_file ( const Module& module )
{
	printf("_generate_proj_file not implemented for the base class\n");
}

void
ProjMaker::_generate_user_configuration()
{
#if 0
	string computername;
	string username;
	string vcproj_file_user = "";

	if (getenv ( "USERNAME" ) != NULL)
		username = getenv ( "USERNAME" );
	if (getenv ( "COMPUTERNAME" ) != NULL)
		computername = getenv ( "COMPUTERNAME" );
	else if (getenv ( "HOSTNAME" ) != NULL)
		computername = getenv ( "HOSTNAME" );

	if ((computername != "") && (username != ""))
		vcproj_file_user = vcproj_file + "." + computername + "." + username + ".user";

	/* User configuration file */
	if (vcproj_file_user != "")
	{
		OUT = fopen ( vcproj_file_user.c_str(), "wb" );
		fprintf ( OUT, "<?xml version=\"1.0\" encoding = \"Windows-1252\"?>\r\n" );
		fprintf ( OUT, "<VisualStudioUserFile\r\n" );
		fprintf ( OUT, "\tProjectType=\"Visual C++\"\r\n" );
		fprintf ( OUT, "\tVersion=\"%s\"\r\n", configuration.VSProjectVersion.c_str() );
		fprintf ( OUT, "\tShowAllFiles=\"false\"\r\n" );
		fprintf ( OUT, "\t>\r\n" );

		fprintf ( OUT, "\t<Configurations>\r\n" );
		for ( size_t icfg = 0; icfg < m_configurations.size(); icfg++ )
		{
			const MSVCConfiguration& cfg = *m_configurations[icfg];
			fprintf ( OUT, "\t\t<Configuration\r\n" );
			fprintf ( OUT, "\t\t\tName=\"%s|Win32\"\r\n", cfg.name.c_str() );
			fprintf ( OUT, "\t\t\t>\r\n" );
			fprintf ( OUT, "\t\t\t<DebugSettings\r\n" );
			if ( module_type == ".cpl" )
			{
				fprintf ( OUT, "\t\t\t\tCommand=\"rundll32.exe\"\r\n" );
				fprintf ( OUT, "\t\t\t\tCommandArguments=\" shell32,Control_RunDLL &quot;$(TargetPath)&quot;,@\"\r\n" );
			}
			else
			{
				fprintf ( OUT, "\t\t\t\tCommand=\"$(TargetPath)\"\r\n" );
				fprintf ( OUT, "\t\t\t\tCommandArguments=\"\"\r\n" );
			}
			fprintf ( OUT, "\t\t\t\tAttach=\"false\"\r\n" );
			fprintf ( OUT, "\t\t\t\tDebuggerType=\"3\"\r\n" );
			fprintf ( OUT, "\t\t\t\tRemote=\"1\"\r\n" );
			string remote_machine = "\t\t\t\tRemoteMachine=\"" + computername + "\"\r\n";
			fprintf ( OUT, remote_machine.c_str() );
			fprintf ( OUT, "\t\t\t\tRemoteCommand=\"\"\r\n" );
			fprintf ( OUT, "\t\t\t\tHttpUrl=\"\"\r\n" );
			fprintf ( OUT, "\t\t\t\tPDBPath=\"\"\r\n" );
			fprintf ( OUT, "\t\t\t\tSQLDebugging=\"\"\r\n" );
			fprintf ( OUT, "\t\t\t\tEnvironment=\"\"\r\n" );
			fprintf ( OUT, "\t\t\t\tEnvironmentMerge=\"true\"\r\n" );
			fprintf ( OUT, "\t\t\t\tDebuggerFlavor=\"\"\r\n" );
			fprintf ( OUT, "\t\t\t\tMPIRunCommand=\"\"\r\n" );
			fprintf ( OUT, "\t\t\t\tMPIRunArguments=\"\"\r\n" );
			fprintf ( OUT, "\t\t\t\tMPIRunWorkingDirectory=\"\"\r\n" );
			fprintf ( OUT, "\t\t\t\tApplicationCommand=\"\"\r\n" );
			fprintf ( OUT, "\t\t\t\tApplicationArguments=\"\"\r\n" );
			fprintf ( OUT, "\t\t\t\tShimCommand=\"\"\r\n" );
			fprintf ( OUT, "\t\t\t\tMPIAcceptMode=\"\"\r\n" );
			fprintf ( OUT, "\t\t\t\tMPIAcceptFilter=\"\"\r\n" );
			fprintf ( OUT, "\t\t\t/>\r\n" );
			fprintf ( OUT, "\t\t</Configuration>\r\n" );
		}
		fprintf ( OUT, "\t</Configurations>\r\n" );
		fprintf ( OUT, "</VisualStudioUserFile>\r\n" );
		fclose ( OUT );
	}
#endif
}


void
ProjMaker::_generate_standard_configuration( const Module& module, const MSVCConfiguration& cfg, BinaryType binaryType )
{
	printf("_generate_standard_configuration not implemented for the base class\n");
}

void
ProjMaker::_generate_makefile_configuration( const Module& module, const MSVCConfiguration& cfg )
{
	printf("_generate_makefile_configuration not implemented for the base class\n");
}

std::string
ProjMaker::_get_vc_dir ( void ) const
{
	if ( configuration.VSProjectVersion == "8.00" )
		return "vc8";
	else if ( configuration.VSProjectVersion == "10.00" )
		return "vc10";
	else /* default to VS2008 */
		return "vc9";
}

std::string
ProjMaker::VcprojFileName ( const Module& module ) const
{
	return FixSeparatorForSystemCommand(
			ReplaceExtension ( module.output->relative_path + "\\" + module.name, "_" + _get_vc_dir() + "_auto.vcproj" )
			);
}

std::string
ProjMaker::_strip_gcc_deffile(std::string Filename, std::string sourcedir, std::string objdir)
{
	std::string NewFilename = Environment::GetIntermediatePath () + "\\" + objdir + "\\" + Filename;
	// we don't like infinite loops - so replace it in two steps
	NewFilename = _replace_str(NewFilename, ".def", "_msvc.de");
	NewFilename = _replace_str(NewFilename, "_msvc.de", "_msvc.def");
	Filename = sourcedir + "\\" + Filename;

	Directory dir(objdir);
	dir.GenerateTree(IntermediateDirectory, false);

	std::fstream in_file(Filename.c_str(), std::ios::in);
	std::fstream out_file(NewFilename.c_str(), std::ios::out);
	std::string::size_type pos;
	DWORD i = 0;

	std::string line;
	while (std::getline(in_file, line))
	{
		pos = line.find("@", 0);
		while (std::string::npos != pos)
		{
			if (pos > 1)
			{
				// make sure it is stdcall and no ordinal
				if (line[pos -1] != ' ')
				{
					i = 0;
					while (true)
					{
						i++;
						if ((line[pos + i] < '0') || (line[pos + i] > '9'))
							break;
					}
					line.replace(pos, i, "");
				}
			}
			pos = line.find("@", pos + 1);
		}

		line += "\n";
		out_file << line;
	}
	in_file.close();
	out_file.close();

	return NewFilename;
}

std::string
ProjMaker::_replace_str(std::string string1, const std::string &find_str, const std::string &replace_str)
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
