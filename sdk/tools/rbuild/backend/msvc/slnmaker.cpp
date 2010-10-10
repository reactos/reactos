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



SlnMaker::SlnMaker ( Configuration& buildConfig,
					 const std::vector<MSVCConfiguration*>& configurations,
					 std::string filename_sln,
					 std::string solution_version, 
					 std::string studio_version)
{
	m_configuration = buildConfig;
	m_configurations = configurations;

	OUT = fopen ( filename_sln.c_str(), "wb" );

	if ( !OUT )
	{
		printf ( "Could not create file '%s'.\n", filename_sln.c_str() );
	}

	_generate_sln_header( solution_version, studio_version);
}

SlnMaker::~SlnMaker()
{
	_generate_sln_footer ( );
	fclose ( OUT );
}

void
SlnMaker::_generate_sln_header ( std::string solution_version, std::string studio_version )
{
	//fprintf ( OUT, "Microsoft Visual Studio Solution File, Format Version %s\r\n", _get_solution_version().c_str() );
	//fprintf ( OUT, "# Visual Studio %s\r\n", _get_studio_version().c_str() );
	fprintf ( OUT, "Microsoft Visual Studio Solution File, Format Version %s\r\n", solution_version.c_str() );
	fprintf ( OUT, "# Visual Studio %s\r\n", studio_version.c_str() );
	fprintf ( OUT, "\r\n" );
}

void
SlnMaker::_generate_sln_footer ( )
{
	fprintf ( OUT, "Global\r\n" );
	fprintf ( OUT, "\tGlobalSection(SolutionConfigurationPlatforms) = preSolution\r\n" );
	for ( size_t i = 0; i < m_configurations.size(); i++ )
		fprintf ( OUT, "\t\t%s = %s\r\n", m_configurations[i]->name.c_str(), m_configurations[i]->name.c_str() );
	fprintf ( OUT, "\tEndGlobalSection\r\n" );

	fprintf ( OUT, "\tGlobalSection(ProjectConfigurationPlatforms) = postSolution\r\n" );
	for ( size_t i = 0; i < modules.size (); i++)
	{
		_generate_sln_configurations ( modules[i]->guid.c_str() );
	}
	fprintf ( OUT, "\tEndGlobalSection\r\n" );
/*
	fprintf ( OUT, "\tGlobalSection(ExtensibilityGlobals) = postSolution\r\n" );
	fprintf ( OUT, "\tEndGlobalSection\r\n" );
	fprintf ( OUT, "\tGlobalSection(ExtensibilityAddIns) = postSolution\r\n" );
	fprintf ( OUT, "\tEndGlobalSection\r\n" );
*/

	if (m_configuration.VSProjectVersion == "7.00") {
		fprintf ( OUT, "\tGlobalSection(ProjectDependencies) = postSolution\r\n" );
		//FIXME: Add dependencies for VS 2002
		fprintf ( OUT, "\tEndGlobalSection\r\n" );
	}
	else {
		fprintf ( OUT, "\tGlobalSection(SolutionProperties) = preSolution\r\n" );
		fprintf ( OUT, "\t\tHideSolutionNode = FALSE\r\n" );
		fprintf ( OUT, "\tEndGlobalSection\r\n" );
	}

	fprintf ( OUT, "EndGlobal\r\n" );
	fprintf ( OUT, "\r\n" );
}


void
SlnMaker::_generate_sln_configurations (  std::string vcproj_guid )
{
	for ( size_t i = 0; i < m_configurations.size (); i++)
	{
		const MSVCConfiguration& cfg = *m_configurations[i];
		fprintf ( OUT, "\t\t%s.%s|Win32.ActiveCfg = %s|Win32\r\n", vcproj_guid.c_str(), cfg.name.c_str(), cfg.name.c_str() );
		fprintf ( OUT, "\t\t%s.%s|Win32.Build.0 = %s|Win32\r\n", vcproj_guid.c_str(), cfg.name.c_str(), cfg.name.c_str() );
	}
}

void 
SlnMaker::_add_project(ProjMaker &project, Module &module)
{
	string sln_guid = "{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}";

	fprintf ( OUT, "Project(\"%s\") = \"%s\", \".\\%s\",\"%s\"\n", sln_guid.c_str(), module.name.c_str() , project.VcprojFileName(module).c_str() , module.guid.c_str());
	fprintf ( OUT, "EndProject\r\n" );

	modules.push_back(&module);
}
