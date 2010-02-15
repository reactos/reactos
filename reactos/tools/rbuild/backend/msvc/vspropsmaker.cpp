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

PropsMaker::PropsMaker ( Configuration& buildConfig, 
			 Project* ProjectNode,
			 std::string filename_props,
			 MSVCConfiguration* msvc_configs)
{
	m_configuration = buildConfig;
	m_ProjectNode = ProjectNode;

	m_msvc_config = msvc_configs;

	debug = ( m_msvc_config->optimization == Debug );
	release = ( m_msvc_config->optimization == Release );
	speed = ( m_msvc_config->optimization == Speed );
	use_ros_headers = (m_msvc_config->headers == ReactOSHeaders);

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
	fprintf ( OUT, "<?xml version=\"1.0\" encoding = \"Windows-1252\"?>\r\n" );
	fprintf ( OUT, "<VisualStudioPropertySheet\r\n" );
	fprintf ( OUT, "\tProjectType=\"Visual C++\"\r\n" );
	
	//Both visual studio 2005 and 2008 use vsprops files with version 8.00 
	fprintf ( OUT, "\tVersion=\"%s\"\r\n", "8.00" ); 
	fprintf ( OUT, "\tName=\"%s\">\r\n", m_msvc_config->name.c_str() );
}

void
PropsMaker::_generate_tools_defaults()
{
	fprintf ( OUT, "\t<Tool\r\n");
	fprintf ( OUT, "\t\tName=\"VCCLCompilerTool\"\r\n");
	fprintf ( OUT, "\t\tAdditionalIncludeDirectories=\"$(globalIncludes)\"\r\n");
	fprintf ( OUT, "\t\tPreprocessorDefinitions=\"$(globalDefines)\"\r\n");
	if (use_ros_headers)
		fprintf ( OUT, "\t\tIgnoreStandardIncludePath=\"true\"\r\n");
	fprintf ( OUT, "\t\tBufferSecurityCheck=\"false\"\r\n");
	if(use_ros_headers)   //this works only with reactos headers
		fprintf ( OUT, "\t\tForcedIncludeFiles=\"warning.h\"\r\n");
	fprintf ( OUT, "\t\tMinimalRebuild=\"%s\"\r\n", speed ? "TRUE" : "FALSE" );
	fprintf ( OUT, "\t\tBasicRuntimeChecks=\"0\"\r\n" );
	fprintf ( OUT, "\t\tRuntimeLibrary=\"%d\"\r\n", debug ? 3 : 2 );	// 3=/MDd 2=/MD
	fprintf ( OUT, "\t\tEnableFunctionLevelLinking=\"FALSE\"\r\n" );
	fprintf ( OUT, "\t\tUsePrecompiledHeader=\"0\"\r\n" );
	fprintf ( OUT, "\t\tWholeProgramOptimization=\"%s\"\r\n", release ? "FALSE" : "FALSE");
	fprintf ( OUT, "\t\tOptimization=\"%d\"\r\n", release ? 2 : 0 );
	if ( release )
	{
		fprintf ( OUT, "\t\tFavorSizeOrSpeed=\"1\"\r\n" );
		fprintf ( OUT, "\t\tStringPooling=\"true\"\r\n" );
	}
	fprintf ( OUT, "\t\tWarningLevel=\"%s\"\r\n", speed ? "0" : "3" );
	fprintf ( OUT, "\t\tDetect64BitPortabilityProblems=\"%s\"\r\n", "FALSE");
	fprintf ( OUT, "\t\tCallingConvention=\"%d\"\r\n", 0 );	// 0=__cdecl
	fprintf ( OUT, "\t\tDebugInformationFormat=\"%s\"\r\n", speed ? "0" : release ? "3": "4");	// 3=/Zi 4=ZI
	fprintf ( OUT, "\t\tCompileAs=\"1\"\r\n" );
	fprintf ( OUT, "\t/>\r\n");

	//Linker
	fprintf ( OUT, "\t<Tool\r\n" );
	fprintf ( OUT, "\t\tName=\"VCLinkerTool\"\r\n" );
	fprintf ( OUT, "\t\tLinkIncremental=\"%d\"\r\n", debug ? 2 : 1 );
	fprintf ( OUT, "\t\tGenerateDebugInformation=\"%s\"\r\n", speed ? "FALSE" : "TRUE" );
	fprintf ( OUT, "\t\tLinkTimeCodeGeneration=\"%d\"\r\n", release? 0 : 0);	// whole program optimization
	fprintf ( OUT, "\t\tTargetMachine=\"%d\"\r\n", 1 );
	if ( debug )
		fprintf ( OUT, "\t\tProgramDatabaseFile=\"$(OutDir)/$(ProjectName).pdb\"\r\n");
	fprintf ( OUT, "\t/>\r\n");

	//Librarian
	fprintf ( OUT, "\t<Tool\r\n" );
	fprintf ( OUT, "\t\tName=\"VCLibrarianTool\"\r\n" );
	fprintf ( OUT, "\t\tOutputFile=\"$(OutDir)\\$(ProjectName).lib\"\r\n");
	fprintf ( OUT, "\t/>\r\n");

	//Resource compiler
	fprintf ( OUT, "\t\t\t<Tool\r\n" );
	fprintf ( OUT, "\t\tName=\"VCResourceCompilerTool\"\r\n" );
	fprintf ( OUT, "\t\tAdditionalIncludeDirectories=\"$(globalIncludes)\"\n" );
	fprintf ( OUT, "\t/>\r\n");
}

void
PropsMaker::_generate_macro(std::string Name,
							std::string Value,
							bool EvairomentVariable)
{
	fprintf ( OUT, "\t<UserMacro\r\n");
	fprintf ( OUT, "\t\tName=\"%s\"\r\n", Name.c_str());
	fprintf ( OUT, "\t\tValue=\"%s\"\r\n", Value.c_str());
	if(EvairomentVariable)
		fprintf ( OUT, "\t\tPerformEnvironmentSet=\"%s\"\r\n", "true");
	fprintf( OUT, "\t/>\r\n");
}

void
PropsMaker::_generate_global_includes()
{
	//Generate global includes
	//they will be used by the c compiler, the resource compiler
	//and the preprocessor for .s and .pspec files

	fprintf ( OUT, "\t<UserMacro\r\n");
	fprintf ( OUT, "\t\tName=\"globalIncludes\"\r\n");
	fprintf ( OUT, "\t\tValue=\"");

	const IfableData& data = m_ProjectNode->non_if_data;
	//const vector<File*>& files = data.files;
	size_t i;
	const vector<Include*>& incs = data.includes;
	for ( i = 0; i < incs.size(); i++ )
	{
		if ((strncmp(incs[i]->directory->relative_path.c_str(), "include\\crt", 11 ) ||
		     strncmp(incs[i]->directory->relative_path.c_str(), "include\\ddk", 11 ) ||
		     strncmp(incs[i]->directory->relative_path.c_str(), "include\\GL", 10 ) ||
		     strncmp(incs[i]->directory->relative_path.c_str(), "include\\psdk", 12 ) ||
		     strncmp(incs[i]->directory->relative_path.c_str(), "include\\reactos\\wine", 20 )) &&
			 ! use_ros_headers)
		{
			continue;
		}

		if(incs[i]->directory->directory == SourceDirectory)
			fprintf ( OUT, "&quot;$(src)\\");
		else if (incs[i]->directory->directory == IntermediateDirectory)
			fprintf ( OUT, "&quot;$(obj)\\");
		else if (incs[i]->directory->directory == OutputDirectory)
			fprintf ( OUT, "&quot;$(out)\\");
		else 
			continue;

		fprintf ( OUT, incs[i]->directory->relative_path.c_str()); 
		fprintf ( OUT, "&quot; ; ");
	}

	fprintf ( OUT, "&quot;$(obj)\\include&quot; ; ");
	fprintf ( OUT, "&quot;$(obj)\\include\\reactos&quot; ; ");

	if ( !use_ros_headers )
	{
		// Add WDK or PSDK paths, if user provides them
		if (getenv ( "BASEDIR" ) != NULL)
		{
			string WdkBase = getenv ( "BASEDIR" );
			fprintf ( OUT, "&quot;%s\\inc\\api&quot; ; ", WdkBase.c_str());
			fprintf ( OUT, "&quot;%s\\inc\\crt&quot; ; ", WdkBase.c_str());
			fprintf ( OUT, "&quot;%s\\inc\\ddk&quot; ; ", WdkBase.c_str());
		}
	}
	fprintf ( OUT, "\"\r\n");
	fprintf ( OUT, "\t\tPerformEnvironmentSet=\"true\"\r\n");
	fprintf( OUT, "\t/>\r\n");
}

void
PropsMaker::_generate_global_definitions()
{

	string global_defines = "";

	fprintf ( OUT, "\t<UserMacro\r\n");
	fprintf ( OUT, "\t\tName=\"globalDefines\"\r\n");
	fprintf ( OUT, "\t\tValue=\"");

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

		if ( defs[i]->value[0] )
			fprintf ( OUT, "%s=%s",defs[i]->name.c_str(), defs[i]->value.c_str());
		else
			fprintf ( OUT, defs[i]->name.c_str());
		fprintf ( OUT, " ; ");
	}

	fprintf ( OUT, "\"\r\n");
	fprintf ( OUT, "\t\tPerformEnvironmentSet=\"true\"\r\n");
	fprintf( OUT, "\t/>\r\n");
}

void
PropsMaker::_generate_footer()
{
	fprintf ( OUT, "</VisualStudioPropertySheet>\r\n");
}


void 
PropsMaker::_generate_props ( std::string solution_version,
							  std::string studio_version )
{
	_generate_header();
	_generate_tools_defaults();

	string srcdir = Environment::GetSourcePath();
	string intdir = Environment::GetIntermediatePath ();
	string outdir = Environment::GetOutputPath ();
	string rosbedir = Environment::GetVariable("_ROSBE_BASEDIR");

	if ( intdir == "obj-i386" )
		intdir = srcdir + "\\obj-i386"; /* append relative dir from project dir */

	if ( outdir == "output-i386" )
		outdir = srcdir + "\\output-i386";

	//Generate global macros
	_generate_macro("src", srcdir, true);
	_generate_macro("out", outdir, true);
	_generate_macro("obj", intdir, true);
	_generate_macro("Tools", "$(out)\\tools", true);
	_generate_macro("RosBE", rosbedir, true);

	_generate_global_includes();
    _generate_global_definitions();
	_generate_footer();
}
