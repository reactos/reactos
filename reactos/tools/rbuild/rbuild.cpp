/*
 * Copyright (C) 2005 Casper S. Hornstrup
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
#include "pch.h"
#include <typeinfo>
#include <algorithm>

#include <stdio.h>
#ifdef WIN32
#include <io.h>
#endif
#include <assert.h>

#include "rbuild.h"
#include "backend/backend.h"
#include "backend/mingw/mingw.h"

using std::string;
using std::vector;

static string BuildSystem;
static string RootXmlFile;
static Configuration configuration;
static std::map<string, string> properties;

bool
ParseAutomaticDependencySwitch (
	char switchChar2,
	char* switchStart )
{
	switch ( switchChar2 )
	{
		case 'd':
			configuration.AutomaticDependencies = false;
			break;
		case 'm':
			if ( strlen ( switchStart ) <= 3 )
			{
				printf ( "Switch -dm requires a module name\n" );
				return false;
			}
			configuration.CheckDependenciesForModuleOnly = true;
			configuration.CheckDependenciesForModuleOnlyModule = string(&switchStart[3]);
			break;
		default:
			printf ( "Unknown switch -d%c\n",
			         switchChar2 );
			return false;
	}
	return true;
}

bool
ParseCompilationUnitSwitch (
	char switchChar2,
	char* switchStart )
{
	switch ( switchChar2 )
	{
		case 'd':
			configuration.CompilationUnitsEnabled = false;
			break;
		default:
			printf ( "Unknown switch -u%c\n",
			         switchChar2 );
			return false;
	}
	return true;
}

bool
ParseVCProjectSwitch (
	char switchChar2,
	char* switchStart )
{
	string temp;

	switch ( switchChar2 )
	{
		case 's':
			if ( strlen ( switchStart ) <= 3 )
			{
				printf ( "Switch -dm requires a module name\n" );
				return false;
			}
			configuration.VSProjectVersion = string(&switchStart[3]);

			if (configuration.VSProjectVersion.at(0) == '{') {
				printf ( "Error: invalid char {\n" );
				return false;
			}

			if (configuration.VSProjectVersion.length() == 1) //7,8
				configuration.VSProjectVersion.append(".00");

			if (configuration.VSProjectVersion.length() == 3) //7.1
				configuration.VSProjectVersion.append("0");

			break;
		case 'c':
			configuration.VSConfigurationType = string (&switchStart[3]);
			configuration.InstallFiles = true;
			break;
		case 'o':
			if ( strlen ( switchStart ) <= 3 )
			{
				printf ( "Invalid switch\n" );
				return false;
			}
			temp = string (&switchStart[3]);
			if ( temp.find ("configuration") != string::npos )
				configuration.UseConfigurationInPath = true;
			
			if ( temp.find ("version") != string::npos )
				configuration.UseVSVersionInPath = true;
			break;
		default:
			printf ( "Unknown switch -d%c\n",
			         switchChar2 );
			return false;
	}
	return true;
}

bool
ParseMakeSwitch ( char switchChar2 )
{
	switch ( switchChar2 )
	{
		case 'i':
			configuration.MakeHandlesInstallDirectories = true;
			break;
		default:
			printf ( "Unknown switch -m%c\n",
			         switchChar2 );
			return false;
	}
	return true;
}

bool
ParseProxyMakefileSwitch ( char switchChar2 )
{
	switch ( switchChar2 )
	{
		case 's':
			configuration.GenerateProxyMakefilesInSourceTree = true;
			break;
		default:
			printf ( "Unknown switch -p%c\n",
			         switchChar2 );
			return false;
	}
	return true;
}

bool
ParseDefineSwitch ( char* switchStart )
{
	string s = string ( switchStart + 2 );
	string::size_type separator =  s.find ( '=' );
	if ( separator == string::npos || separator == 0 )
	{
		printf ( "Invalid define switch: '%s'\n", switchStart );
		return false;
	}
	if ( s.find ( '=', separator + 1 ) != string::npos )
	{
		printf ( "Invalid define switch: '%s'\n", switchStart );
		return false;
	}

	string var = s.substr ( 0, separator );
	string val = s.substr ( separator + 1 );
	properties.insert ( std::pair<string, string> ( var, val ) );
	return true;
}

bool
ParseSwitch ( int argc, char** argv, int index )
{
	char switchChar = strlen ( argv[index] ) > 1 ? argv[index][1] : ' ';
	char switchChar2 = strlen ( argv[index] ) > 2 ? argv[index][2] : ' ';
	switch ( switchChar )
	{
		case 'v':
			if (switchChar2 == 's' || switchChar2 == 'c' || switchChar2 == 'o')
			{
				return ParseVCProjectSwitch (
					switchChar2,
					argv[index] );
			}
			else
				configuration.Verbose = true;
			break;
		case 'c':
			configuration.CleanAsYouGo = true;
			break;
		case 'd':
			return ParseAutomaticDependencySwitch (
				switchChar2,
				argv[index] );
		case 'u':
			return ParseCompilationUnitSwitch (
				switchChar2,
				argv[index] );
		case 'r':
			RootXmlFile = string(&argv[index][2]);
			break;
		case 'm':
			return ParseMakeSwitch ( switchChar2 );
		case 'p':
			return ParseProxyMakefileSwitch ( switchChar2 );
		case 'D':
			return ParseDefineSwitch ( argv[index] );
		default:
			printf (
				"Unknown switch -%c\n",
				switchChar );
			return false;
	}
	return true;
}

bool
ParseArguments ( int argc, char** argv )
{
	if ( argc < 2 )
		return false;
	
	for ( int i = 1; i < argc; i++ )
	{
		if ( argv[i][0] == '-' )
		{
			if ( !ParseSwitch ( argc, argv, i ) )
				return false;
		}
		else
			BuildSystem = argv[i];
	}
	
	return true;
}

int
main ( int argc, char** argv )
{
	InitializeEnvironment ();

	if ( !ParseArguments ( argc, argv ) )
	{
		printf ( "Generates project files for buildsystems\n\n" );
		printf ( "  rbuild [switches] -r{rootfile.rbuild} buildsystem\n\n" );
		printf ( "Switches:\n" );
		printf ( "  -v            Be verbose.\n" );
		printf ( "  -c            Clean as you go. Delete generated files as soon as they are not\n" );
		printf ( "                needed anymore.\n" );
		printf ( "  -dd           Disable automatic dependencies.\n" );
		printf ( "  -dm{module}   Check only automatic dependencies for this module.\n" );
		printf ( "  -ud           Disable multiple source files per compilation unit.\n" );
		printf ( "  -mi           Let make handle creation of install directories. Rbuild will\n" );
		printf ( "                not generate the directories.\n" );
		printf ( "  -ps           Generate proxy makefiles in source tree instead of the output.\n" );
		printf ( "                tree.\n" );
		printf ( "  -vs{version}  Version of MS VS project files. Default is %s.\n", MS_VS_DEF_VERSION );
		printf ( "  -vo{version|configuration} Adds subdirectory path to the default Intermediate-Outputdirectory.\n" );
		printf ( "  -Dvar=val     Set the value of 'var' variable to 'val'.\n" );
		printf ( "\n" );
		printf ( "  buildsystem   Target build system. Can be one of:\n" );

		std::map<std::string,Backend::Factory*>::iterator iter;
		for (iter = Backend::Factory::map_begin(); iter != Backend::Factory::map_end(); iter++)
		{
			Backend::Factory *factory = iter->second;
			printf ( "                %-10s %s\n", factory->Name(), factory->Description());
		}
		return 1;
	}
	try
	{
		if ( RootXmlFile.length () == 0 )
			throw MissingArgumentException ( "-r" );

		string projectFilename ( RootXmlFile );

		printf ( "Reading build files..." );
		Project project ( configuration, projectFilename, &properties );
		printf ( "done\n" );

		project.SetBackend ( Backend::Factory::Create (
			BuildSystem,
			project,
			configuration ) );

		project.WriteConfigurationFile ();
		project.ExecuteInvocations ();
		project.GetBackend().Process();

		return 0;
	}
	catch ( Exception& ex )
	{
		printf ( "%s\n", (*ex).c_str () );
		return 1;
	}
	catch ( XMLException& ex )
	{
		printf ( "%s\n", (*ex).c_str () );
		return 1;
	}
}
