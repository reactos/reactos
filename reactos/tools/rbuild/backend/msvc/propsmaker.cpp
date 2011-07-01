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


PropsMaker::PropsMaker (Project* ProjectNode,
						std::string filename_props,
						std::vector<MSVCConfiguration*> configurations)
{
	m_ProjectNode = ProjectNode;
	m_configurations = configurations;

	OUT = fopen ( filename_props.c_str(), "wb" );

	if ( !OUT )
	{
		printf ( "Could not create file '%s'.\n", filename_props.c_str() );
	}
}

PropsMaker::~PropsMaker ( )
{
	fclose ( OUT );
}

void 
PropsMaker::_generate_header()
{
	fprintf ( OUT, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n");
	fprintf ( OUT, "<Project ");
	fprintf ( OUT, "DefaultTargets=\"Build\" ");
	fprintf ( OUT, "ToolsVersion=\"4.0\" ");
	fprintf ( OUT, "xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\r\n");
}

void 
PropsMaker::_generate_footer()
{
	fprintf ( OUT, "</Project>\r\n");
}

void 
PropsMaker::_generate_macro(std::string Name, std::string Value)
{
	fprintf ( OUT, "\t\t<%s>%s</%s>\r\n", Name.c_str(), Value.c_str(), Name.c_str());
}

void 
PropsMaker::_generate_global_includes(bool debug, bool use_ros_headers)
{
	fprintf ( OUT, "\t\t<globalIncludes>");

	const IfableData& data = m_ProjectNode->non_if_data;
	//const vector<File*>& files = data.files;
	size_t i;
	const vector<Include*>& incs = data.includes;
	for ( i = 0; i < incs.size(); i++ )
	{
		if ((incs[i]->directory->relative_path == "include\\crt" ||
		     incs[i]->directory->relative_path == "include\\ddk" ||
		     incs[i]->directory->relative_path == "include\\GL" ||
		     incs[i]->directory->relative_path == "include\\psdk") &&
			 ! use_ros_headers)
		{
			continue;
		}

		if(incs[i]->directory->directory == SourceDirectory)
			fprintf ( OUT, "\"$(RootSrcDir)\\");
		else if (incs[i]->directory->directory == IntermediateDirectory)
			fprintf ( OUT, "\"$(RootIntDir)\\");
		else if (incs[i]->directory->directory == OutputDirectory)
			fprintf ( OUT, "\"$(RootOutDir)\\");
		else
			continue;

		fprintf ( OUT, "%s\" ; ", incs[i]->directory->relative_path.c_str());
	}

	fprintf ( OUT, "\"$(RootIntDir)\\include\" ; ");
	fprintf ( OUT, "\"$(RootIntDir)\\include\\reactos\" ; ");

	if ( !use_ros_headers )
	{
		// Add WDK or PSDK paths, if user provides them
		if (getenv ( "BASEDIR" ) != NULL)
		{
			string WdkBase = getenv ( "BASEDIR" );
			fprintf ( OUT, "\"%s\\inc\\api\" ; ", WdkBase.c_str());
			fprintf ( OUT, "\"%s\\inc\\crt\" ; ", WdkBase.c_str());
			fprintf ( OUT, "\"%s\\inc\\ddk\" ; ", WdkBase.c_str());
		}
	}
	fprintf ( OUT, "\t</globalIncludes>\r\n");
}

void 
PropsMaker::_generate_global_definitions(bool debug, bool use_ros_headers)
{
	fprintf ( OUT, "\t\t<globalDefines>");

	// Always add _CRT_SECURE_NO_WARNINGS to disable warnings about not
	// using the safe functions introduced in MSVC8.
	fprintf ( OUT, "_CRT_SECURE_NO_WARNINGS ; ") ;

	if ( debug )
	{
		fprintf ( OUT, "_DEBUG ; ");
	}

	if ( !use_ros_headers )
	{
		// this is a define in MinGW w32api, but not Microsoft's headers
		fprintf ( OUT, "STDCALL=__stdcall ; ");
	}

	const IfableData& data = m_ProjectNode->non_if_data;
	const vector<Define*>& defs = data.defines;
	size_t i;

	for ( i = 0; i < defs.size(); i++ )
	{
		if ( defs[i]->backend != "" && defs[i]->backend != "msvc" )
			continue;

		if ( defs[i]->value != "" )
			fprintf ( OUT, "%s=%s ; ",defs[i]->name.c_str(), defs[i]->value.c_str());
		else
			fprintf ( OUT, "%s ; ", defs[i]->name.c_str());
	}

	fprintf ( OUT, "\t</globalDefines>\r\n");
}

void 
PropsMaker::_generate_props ( std::string solution_version, std::string studio_version )
{

	string srcdir = Environment::GetSourcePath();
	string intdir = Environment::GetIntermediatePath ();
	string outdir = Environment::GetOutputPath ();
	string rosbedir = Environment::GetVariable("_ROSBE_BASEDIR");

	if ( intdir == "obj-i386" )
		intdir = srcdir + "\\obj-i386"; /* append relative dir from project dir */

	if ( outdir == "output-i386" )
		outdir = srcdir + "\\output-i386";

	_generate_header();

	fprintf ( OUT, "\t<PropertyGroup Label=\"UserMacros\">\r\n");
	_generate_macro("RootSrcDir", srcdir);
	_generate_macro("RootOutDir", outdir);
	_generate_macro("RootIntDir", intdir);
	_generate_macro("Tools", "$(RootOutDir)\\tools");
	_generate_macro("RosBE", rosbedir);
	fprintf ( OUT, "\t</PropertyGroup>\r\n");

	for ( size_t icfg = 0; icfg < m_configurations.size(); icfg++ )
	{
		MSVCConfiguration* cfg = m_configurations[icfg];

		if(cfg->optimization == RosBuild)
			continue;

		fprintf ( OUT, "\t<PropertyGroup Condition=\"'$(Configuration)'=='%s'\">\r\n", cfg->name.c_str() );
		_generate_global_includes(cfg->optimization == Debug, cfg->headers == ReactOSHeaders);
	    _generate_global_definitions(cfg->optimization == Debug, cfg->headers == ReactOSHeaders);
		fprintf ( OUT, "\t</PropertyGroup>\r\n");
	}

	_generate_footer();
}
