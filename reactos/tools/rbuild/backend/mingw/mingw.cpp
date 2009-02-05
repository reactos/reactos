/*
 * Copyright (C) 2005 Casper S. Hornstrup
 *               2006 Christoph von Wittich
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
#include "../../pch.h"

#include "mingw.h"
#include <assert.h>
#include "modulehandler.h"

#ifdef _MSC_VER
#define popen _popen
#define pclose _pclose
#endif//_MSC_VER

using std::string;
using std::vector;
using std::set;
using std::map;

typedef set<string> set_string;

const struct ModuleHandlerInformations ModuleHandlerInformations[] = {
	{ HostTrue, "", "", "" }, // BuildTool
	{ HostFalse, "", "", "" }, // StaticLibrary
	{ HostFalse, "", "", "" }, // ObjectLibrary
	{ HostFalse, "", "", "" }, // Kernel
	{ HostFalse, "", "", "" }, // KernelModeDLL
	{ HostFalse, "-D__NTDRIVER__", "", "" }, // KernelModeDriver
	{ HostFalse, "", "", "" }, // NativeDLL
	{ HostFalse, "-D__NTAPP__", "", "" }, // NativeCUI
	{ HostFalse, "", "", "" }, // Win32DLL
	{ HostFalse, "", "", "" }, // Win32OCX
	{ HostFalse, "", "", "" }, // Win32CUI
	{ HostFalse, "", "", "" }, // Win32GUI
	{ HostFalse, "", "", "-nostartfiles -nostdlib" }, // BootLoader
	{ HostFalse, "", "-f bin", "" }, // BootSector
	{ HostFalse, "", "", "" }, // Iso
	{ HostFalse, "", "", "" }, // LiveIso
	{ HostFalse, "", "", "" }, // Test
	{ HostFalse, "", "", "" }, // RpcServer
	{ HostFalse, "", "", "" }, // RpcClient
	{ HostFalse, "", "", "" }, // Alias
	{ HostFalse, "", "", "-nostartfiles -nostdlib" }, // BootProgram
	{ HostFalse, "", "", "" }, // Win32SCR
	{ HostFalse, "", "", "" }, // IdlHeader
	{ HostFalse, "", "", "" }, // IsoRegTest
	{ HostFalse, "", "", "" }, // LiveIsoRegTest
	{ HostFalse, "", "", "" }, // EmbeddedTypeLib
	{ HostFalse, "", "", "" }, // ElfExecutable
	{ HostFalse, "", "", "" }, // RpcProxy
	{ HostTrue, "", "", "" }, // HostStaticLibrary
	{ HostFalse, "", "", "" }, // Cabinet
	{ HostFalse, "", "", "" }, // KeyboardLayout
	{ HostFalse, "", "", "" }, // MessageHeader
};

string
MingwBackend::GetFullPath ( const FileLocation& file ) const
{
	MingwModuleHandler::PassThruCacheDirectory ( &file );

	string directory;
	switch ( file.directory )
	{
		case SourceDirectory:
			directory = "";
			break;
		case IntermediateDirectory:
			directory = "$(INTERMEDIATE)";
			break;
		case OutputDirectory:
			directory = "$(OUTPUT)";
			break;
		case InstallDirectory:
			directory = "$(INSTALL)";
			break;
		case TemporaryDirectory:
			directory = "$(TEMPORARY)";
			break;
		default:
			throw InvalidOperationException ( __FILE__,
			                                  __LINE__,
			                                  "Invalid directory %d.",
			                                  file.directory );
	}

	if ( file.relative_path.length () > 0 )
	{
		if ( directory.length () > 0 )
			directory += sSep;
		directory += file.relative_path;
	}
	return directory;
}

string
MingwBackend::GetFullName ( const FileLocation& file ) const
{
	string directory;
	switch ( file.directory )
	{
		case SourceDirectory:
			directory = "";
			break;
		case IntermediateDirectory:
			directory = "$(INTERMEDIATE)";
			break;
		case OutputDirectory:
			directory = "$(OUTPUT)";
			break;
		case InstallDirectory:
			directory = "$(INSTALL)";
			break;
		case TemporaryDirectory:
			directory = "$(TEMPORARY)";
			break;
		default:
			throw InvalidOperationException ( __FILE__,
			                                  __LINE__,
			                                  "Invalid directory %d.",
			                                  file.directory );
	}

	if ( file.relative_path.length () > 0 )
	{
		if ( directory.length () > 0 )
			directory += sSep;
		directory += file.relative_path;
	}

	if ( directory.length () > 0 )
		directory += sSep;

	return directory + file.name;
}

string
v2s ( const Backend* backend, const vector<FileLocation>& files, int wrap_at )
{
	if ( !files.size() )
		return "";
	string s;
	int wrap_count = 0;
	for ( size_t i = 0; i < files.size(); i++ )
	{
		const FileLocation& file = files[i];
		if ( wrap_at > 0 && wrap_count++ == wrap_at )
			s += " \\\n\t\t";
		else if ( s.size() )
			s += " ";
		s += backend->GetFullName ( file );
	}
	return s;
}


string
v2s ( const string_list& v, int wrap_at )
{
	if ( !v.size() )
		return "";
	string s;
	int wrap_count = 0;
	for ( size_t i = 0; i < v.size(); i++ )
	{
		if ( !v[i].size() )
			continue;
		if ( wrap_at > 0 && wrap_count++ == wrap_at )
			s += " \\\n\t\t";
		else if ( s.size() )
			s += " ";
		s += v[i];
	}
	return s;
}


static class MingwFactory : public Backend::Factory
{
public:
	MingwFactory() : Factory ( "mingw", "Minimalist GNU Win32" ) {}
	Backend* operator() ( Project& project,
	                      Configuration& configuration )
	{
		return new MingwBackend ( project,
		                          configuration );
	}
} factory;


MingwBackend::MingwBackend ( Project& project,
                             Configuration& configuration )
	: Backend ( project, configuration ),
	  manualBinutilsSetting( false ),
	  intermediateDirectory ( new Directory ( "" ) ),
	  outputDirectory ( new Directory ( "" ) ),
	  installDirectory ( new Directory ( "" ) )
{
	compilerPrefix = "";
}

MingwBackend::~MingwBackend()
{
	delete intermediateDirectory;
	delete outputDirectory;
	delete installDirectory;
}

string
MingwBackend::AddDirectoryTarget ( const string& directory,
                                   Directory* directoryTree )
{
	if ( directory.length () > 0)
		directoryTree->Add ( directory.c_str() );
	return directoryTree->name;
}

bool
MingwBackend::CanEnablePreCompiledHeaderSupportForModule ( const Module& module )
{
	if ( !configuration.CompilationUnitsEnabled )
		return true;

	const vector<CompilationUnit*>& compilationUnits = module.non_if_data.compilationUnits;
	size_t i;
	for ( i = 0; i < compilationUnits.size (); i++ )
	{
		CompilationUnit& compilationUnit = *compilationUnits[i];
		if ( compilationUnit.GetFiles ().size () != 1 )
			return false;
	}
	return true;
}

void
MingwBackend::ProcessModules ()
{
	printf ( "Processing modules..." );

	vector<MingwModuleHandler*> v;
	size_t i;

	for ( std::map<std::string, Module*>::iterator p = ProjectNode.modules.begin (); p != ProjectNode.modules.end (); ++ p )
	{
		Module& module = *p->second;
		if ( !module.enabled )
			continue;
		MingwModuleHandler* h = MingwModuleHandler::InstanciateHandler (
			module,
			this );
		h->AddImplicitLibraries ( module );
		if ( use_pch && CanEnablePreCompiledHeaderSupportForModule ( module ) )
			h->EnablePreCompiledHeaderSupport ();
		v.push_back ( h );
	}

	size_t iend = v.size ();

	for ( i = 0; i < iend; i++ )
		v[i]->GenerateSourceMacro();
	for ( i = 0; i < iend; i++ )
		v[i]->GenerateObjectMacro();
	fprintf ( fMakefile, "\n" );
	for ( i = 0; i < iend; i++ )
		v[i]->GenerateTargetMacro();
	fprintf ( fMakefile, "\n" );

	GenerateAllTarget ( v );
	GenerateRegTestsRunTarget ();

	for ( i = 0; i < iend; i++ )
		v[i]->GenerateOtherMacros();

	for ( i = 0; i < iend; i++ )
	{
		MingwModuleHandler& h = *v[i];
		h.GeneratePreconditionDependencies ();
		h.Process ();
		h.GenerateInvocations ();
		h.GenerateCleanTarget ();
		h.GenerateInstallTarget ();
		h.GenerateDependsTarget ();
		delete v[i];
	}

	printf ( "done\n" );
}

void
MingwBackend::Process ()
{
	if ( configuration.CheckDependenciesForModuleOnly )
		CheckAutomaticDependenciesForModuleOnly ();
	else
		ProcessNormal ();
}

void
MingwBackend::CheckAutomaticDependenciesForModuleOnly ()
{
	if ( configuration.Dependencies == AutomaticDependencies )
	{
		Module* module = ProjectNode.LocateModule ( configuration.CheckDependenciesForModuleOnlyModule );
		if ( module == NULL )
		{
			printf ( "Module '%s' does not exist\n",
			        configuration.CheckDependenciesForModuleOnlyModule.c_str () );
			return;
		}

		printf ( "Checking automatic dependencies for module '%s'...",
		         module->name.c_str () );
		AutomaticDependency automaticDependency ( ProjectNode );
		automaticDependency.CheckAutomaticDependenciesForModule ( *module,
		                                                          configuration.Verbose );
		printf ( "done\n" );
	}
}

void
MingwBackend::ProcessNormal ()
{
	assert(sizeof(ModuleHandlerInformations)/sizeof(ModuleHandlerInformations[0]) == TypeDontCare);

	DetectCompiler ();
	DetectBinutils ();
	DetectNetwideAssembler ();
	DetectPipeSupport ();
	DetectPCHSupport ();
	CreateMakefile ();
	GenerateHeader ();
	GenerateGlobalVariables ();
	GenerateXmlBuildFilesMacro ();
	ProcessModules ();
	GenerateInstallTarget ();
	GenerateTestTarget ();
	GenerateDirectoryTargets ();
	GenerateDirectories ();
	GenerateTestSupportCode ();
	GenerateCompilationUnitSupportCode ();
	GenerateSysSetup ();
	GenerateProxyMakefiles ();
	CheckAutomaticDependencies ();
	CloseMakefile ();
}

void
MingwBackend::CreateMakefile ()
{
	fMakefile = fopen ( ProjectNode.makefile.c_str (), "w" );
	if ( !fMakefile )
		throw AccessDeniedException ( ProjectNode.makefile );
	MingwModuleHandler::SetBackend ( this );
	MingwModuleHandler::SetMakefile ( fMakefile );
}

void
MingwBackend::CloseMakefile () const
{
	if (fMakefile)
		fclose ( fMakefile );
}

void
MingwBackend::GenerateHeader () const
{
	fprintf ( fMakefile, "# THIS FILE IS AUTOMATICALLY GENERATED, EDIT '%s' INSTEAD\n\n",
		ProjectNode.GetProjectFilename ().c_str () );
}

void
MingwBackend::GenerateProjectCFlagsMacro ( const char* assignmentOperation,
                                           const IfableData& data ) const
{
	set<string> used_defs;

	if ( data.includes.size () > 0 )
		fprintf (
			fMakefile,
			"PROJECT_CINCLUDES %s %s\n",
			assignmentOperation,
			MingwModuleHandler::GenerateGccIncludeParametersFromVector ( data.includes ).c_str ());

	if ( data.defines.size () > 0 )
		fprintf (
			fMakefile,
			"PROJECT_CDEFINES %s %s\n",
			assignmentOperation,
			MingwModuleHandler::GenerateGccDefineParametersFromVector ( data.defines, used_defs ).c_str ());
}

void
MingwBackend::GenerateGlobalCFlagsAndProperties (
	const char* assignmentOperation,
	const IfableData& data ) const
{
	for ( std::map<std::string, Property*>::const_iterator p = data.properties.begin(); p != data.properties.end(); ++ p )
	{
		Property& prop = *p->second;

		if (!prop.isInternal)
		{
			fprintf ( fMakefile, "%s := %s\n",
				prop.name.c_str(),
				prop.value.c_str() );
		}
	}

	if ( data.includes.size() || data.defines.size() )
	{
		GenerateProjectCFlagsMacro ( assignmentOperation,
		                             data );
	}
}

void
MingwBackend::GenerateProjectGccOptionsMacro ( const char* assignmentOperation,
                                               IfableData& data ) const
{
	size_t i;

	fprintf (
		fMakefile,
		"PROJECT_GCCOPTIONS %s",
		assignmentOperation );

	for ( i = 0; i < data.compilerFlags.size(); i++ )
	{
		if ( data.compilerFlags[i]->compiler == CompilerTypeDontCare )
		{
			fprintf (
				fMakefile,
				" %s",
				data.compilerFlags[i]->flag.c_str() );
		}
	}

	fputs ( "\n", fMakefile );

	// TODO: reference these from somewhere
	fprintf (
		fMakefile,
		"PROJECT_GCC_CFLAGS %s",
		assignmentOperation );

	for ( i = 0; i < data.compilerFlags.size(); i++ )
	{
		if ( data.compilerFlags[i]->compiler == CompilerTypeCC )
		{
			fprintf (
				fMakefile,
				" %s",
				data.compilerFlags[i]->flag.c_str() );
		}
	}

	fputs ( "\n", fMakefile );

	fprintf (
		fMakefile,
		"PROJECT_GCC_CXXFLAGS %s",
		assignmentOperation );

	for ( i = 0; i < data.compilerFlags.size(); i++ )
	{
		if ( data.compilerFlags[i]->compiler == CompilerTypeCPP )
		{
			fprintf (
				fMakefile,
				" %s",
				data.compilerFlags[i]->flag.c_str() );
		}
	}

	fputs ( "\n", fMakefile );
}

void
MingwBackend::GenerateProjectGccOptions (
	const char* assignmentOperation,
	IfableData& data ) const
{
	if ( data.compilerFlags.size() )
	{
		GenerateProjectGccOptionsMacro ( assignmentOperation,
		                                 data );
	}
}

string
MingwBackend::GenerateProjectLFLAGS () const
{
	string lflags;
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
MingwBackend::GenerateGlobalVariables () const
{
	fputs ( "include tools$(SEP)rbuild$(SEP)backend$(SEP)mingw$(SEP)rules.mak\n", fMakefile );

	if ( configuration.Dependencies == FullDependencies )
	{
		fprintf ( fMakefile,
				  "ifeq ($(ROS_BUILDDEPS),)\n"
				  "ROS_BUILDDEPS:=%s\n"
				  "endif\n",
				  "full" );
	}

	fprintf ( fMakefile,
	          "PREFIX := %s\n",
	          compilerPrefix.c_str () );
	fprintf ( fMakefile,
	          "nasm := %s\n",
	          nasmCommand.c_str () );

	GenerateGlobalCFlagsAndProperties ( "=", ProjectNode.non_if_data );
	GenerateProjectGccOptions ( "=", ProjectNode.non_if_data );

	fprintf ( fMakefile, "PROJECT_RCFLAGS := $(PROJECT_CINCLUDES) $(PROJECT_CDEFINES)\n" );
	fprintf ( fMakefile, "PROJECT_WIDLFLAGS := $(PROJECT_CINCLUDES) $(PROJECT_CDEFINES)\n" );
	fprintf ( fMakefile, "PROJECT_LFLAGS := '$(shell ${TARGET_CC} -print-libgcc-file-name)' %s\n", GenerateProjectLFLAGS ().c_str () );
	fprintf ( fMakefile, "PROJECT_LPPFLAGS := '$(shell ${TARGET_CPP} -print-file-name=libstdc++.a)' '$(shell ${TARGET_CPP} -print-file-name=libgcc.a)' '$(shell ${TARGET_CPP} -print-file-name=libmingw32.a)' '$(shell ${TARGET_CPP} -print-file-name=libmingwex.a)'\n" );
	/* hack to get libgcc_eh.a, should check mingw version or something */
	if (Environment::GetArch() == "amd64")
	{
	    fprintf ( fMakefile, "PROJECT_LPPFLAGS += '$(shell ${TARGET_CPP} -print-file-name=libgcc_eh.a)'\n" );
	}
	fprintf ( fMakefile, "PROJECT_GCCOPTIONS += -Wall\n" );
	fprintf ( fMakefile, "ifneq ($(OARCH),)\n" );
	fprintf ( fMakefile, "PROJECT_GCCOPTIONS += -march=$(OARCH)\n" );
	fprintf ( fMakefile, "endif\n" );
	fprintf ( fMakefile, "ifneq ($(TUNE),)\n" );
	fprintf ( fMakefile, "PROJECT_GCCOPTIONS += -mtune=$(TUNE)\n" );
	fprintf ( fMakefile, "endif\n" );
	fprintf ( fMakefile, "PROJECT_CFLAGS = $(PROJECT_GCCOPTIONS) $(PROJECT_GCC_CFLAGS)\n" );
	fprintf ( fMakefile, "PROJECT_CXXFLAGS = $(PROJECT_GCCOPTIONS) $(PROJECT_GCC_CXXFLAGS)\n" );
	fprintf ( fMakefile, "\n" );
}

bool
MingwBackend::IncludeInAllTarget ( const Module& module ) const
{
	if ( MingwModuleHandler::ReferenceObjects ( module ) )
		return false;
	if ( module.type == BootSector )
		return false;
	if ( module.type == Iso )
		return false;
	if ( module.type == LiveIso )
		return false;
	if ( module.type == IsoRegTest )
		return false;
	if ( module.type == LiveIsoRegTest )
		return false;
	if ( module.type == Test )
		return false;
	if ( module.type == Alias )
		return false;
	return true;
}

void
MingwBackend::GenerateAllTarget ( const vector<MingwModuleHandler*>& handlers ) const
{
	fprintf ( fMakefile, "all:" );
	int wrap_count = 0;
	size_t iend = handlers.size ();
	for ( size_t i = 0; i < iend; i++ )
	{
		const Module& module = handlers[i]->module;
		if ( IncludeInAllTarget ( module ) )
		{
			if ( wrap_count++ == 5 )
				fprintf ( fMakefile, " \\\n\t\t" ), wrap_count = 0;
			fprintf ( fMakefile,
			          " %s",
			          GetTargetMacro(module).c_str () );
		}
	}
	fprintf ( fMakefile, "\n\t\n\n" );
}

void
MingwBackend::GenerateRegTestsRunTarget () const
{
	fprintf ( fMakefile,
	          "REGTESTS_RUN_TARGET = regtests.dll\n" );
	fprintf ( fMakefile,
	          "$(REGTESTS_RUN_TARGET): $(REGTESTS_TARGET)\n" );
	fprintf ( fMakefile,
	          "\t$(cp) $(REGTESTS_TARGET) $(REGTESTS_RUN_TARGET)\n" );
	fprintf ( fMakefile, "\n" );
}

void
MingwBackend::GenerateXmlBuildFilesMacro() const
{
	fprintf ( fMakefile,
	          "XMLBUILDFILES = %s \\\n",
	          ProjectNode.GetProjectFilename ().c_str () );
	string xmlbuildFilenames;
	int numberOfExistingFiles = 0;
	struct stat statbuf;
	time_t SystemTime, lastWriteTime;

	for ( size_t i = 0; i < ProjectNode.xmlbuildfiles.size (); i++ )
	{
		XMLInclude& xmlbuildfile = *ProjectNode.xmlbuildfiles[i];
		if ( !xmlbuildfile.fileExists )
			continue;
		numberOfExistingFiles++;
		if ( xmlbuildFilenames.length () > 0 )
			xmlbuildFilenames += " ";

		FILE* f = fopen ( xmlbuildfile.topIncludeFilename.c_str (), "rb" );
		if ( !f )
		throw FileNotFoundException ( NormalizeFilename ( xmlbuildfile.topIncludeFilename ) );

		if ( fstat ( fileno ( f ), &statbuf ) != 0 )
		{
			fclose ( f );
			throw AccessDeniedException ( NormalizeFilename ( xmlbuildfile.topIncludeFilename ) );
		}

		lastWriteTime = statbuf.st_mtime;
		SystemTime = time(NULL);

		if (SystemTime != -1)
		{
			if (difftime (lastWriteTime, SystemTime) > 0)
				throw InvalidDateException ( NormalizeFilename ( xmlbuildfile.topIncludeFilename ) );
		}

		fclose ( f );

		xmlbuildFilenames += NormalizeFilename ( xmlbuildfile.topIncludeFilename );
		if ( numberOfExistingFiles % 5 == 4 || i == ProjectNode.xmlbuildfiles.size () - 1 )
		{
			fprintf ( fMakefile,
			          "\t%s",
			          xmlbuildFilenames.c_str ());
			if ( i == ProjectNode.xmlbuildfiles.size () - 1 )
			{
				fprintf ( fMakefile, "\n" );
			}
			else
			{
				fprintf ( fMakefile,
				          " \\\n" );
			}
			xmlbuildFilenames.resize ( 0 );
		}
		numberOfExistingFiles++;
	}
	fprintf ( fMakefile, "\n" );
}

void
MingwBackend::GenerateTestSupportCode ()
{
	printf ( "Generating test support code..." );
	TestSupportCode testSupportCode ( ProjectNode );
	testSupportCode.GenerateTestSupportCode ( configuration.Verbose );
	printf ( "done\n" );
}

void
MingwBackend::GenerateCompilationUnitSupportCode ()
{
	if ( configuration.CompilationUnitsEnabled )
	{
		printf ( "Generating compilation unit support code..." );
		CompilationUnitSupportCode compilationUnitSupportCode ( ProjectNode );
		compilationUnitSupportCode.Generate ( configuration.Verbose );
		printf ( "done\n" );
	}
}

void
MingwBackend::GenerateSysSetup ()
{
	printf ( "Generating syssetup.inf..." );
	SysSetupGenerator sysSetupGenerator ( ProjectNode );
	sysSetupGenerator.Generate ();
	printf ( "done\n" );
}

string
MingwBackend::GetProxyMakefileTree () const
{
	if ( configuration.GenerateProxyMakefilesInSourceTree )
		return "";
	else
		return Environment::GetOutputPath ();
}

void
MingwBackend::GenerateProxyMakefiles ()
{
	printf ( "Generating proxy makefiles..." );
	ProxyMakefile proxyMakefile ( ProjectNode );
	proxyMakefile.GenerateProxyMakefiles ( configuration.Verbose,
	                                       GetProxyMakefileTree () );
	printf ( "done\n" );
}

void
MingwBackend::CheckAutomaticDependencies ()
{
	if ( configuration.Dependencies == AutomaticDependencies )
	{
		printf ( "Checking automatic dependencies..." );
		AutomaticDependency automaticDependency ( ProjectNode );
		automaticDependency.CheckAutomaticDependencies ( configuration.Verbose );
		printf ( "done\n" );
	}
}

void
MingwBackend::GenerateDirectories ()
{
	printf ( "Creating directories..." );
	intermediateDirectory->GenerateTree ( IntermediateDirectory, configuration.Verbose );
	outputDirectory->GenerateTree ( OutputDirectory, configuration.Verbose );
	if ( !configuration.MakeHandlesInstallDirectories )
		installDirectory->GenerateTree ( InstallDirectory, configuration.Verbose );
	printf ( "done\n" );
}

bool
MingwBackend::TryToDetectThisCompiler ( const string& compiler )
{
	string command = ssprintf (
		"%s -v 1>%s 2>%s",
		FixSeparatorForSystemCommand(compiler).c_str (),
		NUL,
		NUL );
	int exitcode = system ( command.c_str () );
	return (bool) (exitcode == 0);
}

void
MingwBackend::DetectCompiler ()
{
	printf ( "Detecting compiler..." );

	bool detectedCompiler = false;
	const string& ROS_PREFIXValue = Environment::GetVariable ( "ROS_PREFIX" );
	if ( ROS_PREFIXValue.length () > 0 )
	{
		compilerPrefix = ROS_PREFIXValue;
		compilerCommand = compilerPrefix + "-gcc";
		detectedCompiler = TryToDetectThisCompiler ( compilerCommand );
	}
#if defined(WIN32)
	if ( !detectedCompiler )
	{
		compilerPrefix = "";
		compilerCommand = "gcc";
		detectedCompiler = TryToDetectThisCompiler ( compilerCommand );
	}
#endif
	if ( !detectedCompiler )
	{
		compilerPrefix = "mingw32";
		compilerCommand = compilerPrefix + "-gcc";
		detectedCompiler = TryToDetectThisCompiler ( compilerCommand );
	}

	if ( detectedCompiler )
	{
		string compilerVersion = GetCompilerVersion ( compilerCommand );
		if ( IsSupportedCompilerVersion ( compilerVersion ) )
			printf ( "detected (%s %s)\n", compilerCommand.c_str (), compilerVersion.c_str() );
		else
		{
			printf ( "detected (%s), but with unsupported version (%s)\n",
			         compilerCommand.c_str (),
			         compilerVersion.c_str () );
			throw UnsupportedBuildToolException ( compilerCommand, compilerVersion );
		}
	}
	else
		printf ( "not detected\n" );

}

bool
MingwBackend::TryToDetectThisNetwideAssembler ( const string& assembler )
{
	string command = ssprintf (
		"%s -h 1>%s 2>%s",
		FixSeparatorForSystemCommand(assembler).c_str (),
		NUL,
		NUL );
	int exitcode = system ( command.c_str () );
	return (bool) (exitcode == 0);
}

string
MingwBackend::GetVersionString ( const string& versionCommand )
{
	FILE *fp;
	int ch, i;
	size_t newline;
	char buffer[81];

	fp = popen ( versionCommand.c_str () , "r" );
	for( i = 0;
	     ( i < 80 ) &&
	       ( feof ( fp ) == 0 &&
	         ( ( ch = fgetc( fp ) ) != -1 ) );
	     i++ )
	{
		buffer[i] = (char) ch;
	}
	buffer[i] = '\0';
	pclose ( fp );

	char separators[] = " ()";
	char *token;
	char *prevtoken = NULL;

	string version;

	token = strtok ( buffer, separators );
	while ( token != NULL )
	{
		prevtoken = token;
		version = string( prevtoken );
		if ( (newline = version.find('\n')) != std::string::npos )
			version.erase(newline, 1);
		if ( version.find('.') != std::string::npos )
			break;
		token = strtok ( NULL, separators );
	}
	return version;
}

string
MingwBackend::GetNetwideAssemblerVersion ( const string& nasmCommand )
{
	string versionCommand;
	if ( nasmCommand.find("yasm") != std::string::npos )
	{
		versionCommand = ssprintf ( "%s --version",
		                            nasmCommand.c_str (),
		                            NUL,
		                            NUL );
	}
	else
	{
		versionCommand = ssprintf ( "%s -v",
		                            nasmCommand.c_str (),
		                            NUL,
		                            NUL );
	}
	return GetVersionString( versionCommand );
}

string
MingwBackend::GetCompilerVersion ( const string& compilerCommand )
{
	string versionCommand = ssprintf ( "%s --version gcc",
	                                   compilerCommand.c_str (),
	                                   NUL,
	                                   NUL );
	return GetVersionString( versionCommand );
}

string
MingwBackend::GetBinutilsVersion ( const string& binutilsCommand )
{
	string versionCommand = ssprintf ( "%s -v",
	                                   binutilsCommand.c_str (),
	                                   NUL,
	                                   NUL );
	return GetVersionString( versionCommand );
}

bool
MingwBackend::IsSupportedCompilerVersion ( const string& compilerVersion )
{
	if ( strcmp ( compilerVersion.c_str (), "3.4.2") < 0 )
		return false;
	else
		return true;
}

bool
MingwBackend::TryToDetectThisBinutils ( const string& binutils )
{
	string command = ssprintf (
		"%s -v 1>%s 2>%s",
		FixSeparatorForSystemCommand(binutils).c_str (),
		NUL,
		NUL );
	int exitcode = system ( command.c_str () );
	return (exitcode == 0);
}

string
MingwBackend::GetBinutilsVersionDate ( const string& binutilsCommand )
{
	FILE *fp;
	int ch, i;
	char buffer[81];

	string versionCommand = ssprintf ( "%s -v",
	                                   binutilsCommand.c_str (),
	                                   NUL,
	                                   NUL );
	fp = popen ( versionCommand.c_str () , "r" );
	for( i = 0;
	     ( i < 80 ) &&
	         ( feof ( fp ) == 0 &&
	           ( ( ch = fgetc( fp ) ) != -1 ) );
	     i++ )
	{
		buffer[i] = (char) ch;
	}
	buffer[i] = '\0';
	pclose ( fp );

	char separators[] = " ";
	char *token;
	char *prevtoken = NULL;

	token = strtok ( buffer, separators );
	while ( token != NULL )
	{
		prevtoken = token;
		token = strtok ( NULL, separators );
	}
	string version = string ( prevtoken );
	int lastDigit = version.find_last_not_of ( "\t\r\n" );
	if ( lastDigit != -1 )
		return string ( version, 0, lastDigit+1 );
	else
		return version;
}

bool
MingwBackend::IsSupportedBinutilsVersion ( const string& binutilsVersion )
{
	if ( manualBinutilsSetting ) return true;

	/* linux */
	if ( binutilsVersion.find('.') != std::string::npos )
	{
		/* TODO: blacklist versions on version number instead of date */
		return true;
	}

	/*
	 * - Binutils older than 2003/10/01 have broken windres which can't handle
	 *   icons with alpha channel.
	 * - Binutils between 2004/09/02 and 2004/10/08 have broken handling of
	 *   forward exports in dlltool.
	 */
	if ( ( ( strcmp ( binutilsVersion.c_str (), "20040902") >= 0 ) &&
	       ( strcmp ( binutilsVersion.c_str (), "20041008") <= 0 ) ) ||
	       ( strcmp ( binutilsVersion.c_str (), "20031001") < 0 ) )
		return false;
	else
		return true;
}

void
MingwBackend::DetectBinutils ()
{
	printf ( "Detecting binutils..." );

	bool detectedBinutils = false;
	const string& ROS_PREFIXValue = Environment::GetVariable ( "ROS_PREFIX" );

	if ( ROS_PREFIXValue.length () > 0 )
	{
		binutilsPrefix = ROS_PREFIXValue;
		binutilsCommand = binutilsPrefix + "-ld";
		manualBinutilsSetting = true;
		detectedBinutils = true;
	}
#if defined(WIN32)
	if ( !detectedBinutils )
	{
		binutilsPrefix = "";
		binutilsCommand = "ld";
		detectedBinutils = TryToDetectThisBinutils ( binutilsCommand );
	}
#endif
	if ( !detectedBinutils )
	{
		binutilsPrefix = "mingw32";
		binutilsCommand = binutilsPrefix + "-ld";
		detectedBinutils = TryToDetectThisBinutils ( binutilsCommand );
	}
	if ( detectedBinutils )
	{
		string binutilsVersion = GetBinutilsVersionDate ( binutilsCommand );
		if ( IsSupportedBinutilsVersion ( binutilsVersion ) )
			printf ( "detected (%s %s)\n", binutilsCommand.c_str (), GetBinutilsVersion( binutilsCommand ).c_str() );
		else
		{
			printf ( "detected (%s), but with unsupported version (%s)\n",
			         binutilsCommand.c_str (),
			         binutilsVersion.c_str () );
			throw UnsupportedBuildToolException ( binutilsCommand, binutilsVersion );
		}
	}
	else
		printf ( "not detected\n" );

}

void
MingwBackend::DetectNetwideAssembler ()
{
	printf ( "Detecting netwide assembler..." );

	nasmCommand = "nasm";
	bool detectedNasm = TryToDetectThisNetwideAssembler ( nasmCommand );
#if defined(WIN32)
	if ( !detectedNasm )
	{
		nasmCommand = "nasmw";
		detectedNasm = TryToDetectThisNetwideAssembler ( nasmCommand );
	}
#endif
	if ( !detectedNasm )
	{
		nasmCommand = "yasm";
		detectedNasm = TryToDetectThisNetwideAssembler ( nasmCommand );
	}
	if ( detectedNasm )
		printf ( "detected (%s %s)\n", nasmCommand.c_str (), GetNetwideAssemblerVersion( nasmCommand ).c_str() );
	else
		printf ( "not detected\n" );
}

void
MingwBackend::DetectPipeSupport ()
{
	printf ( "Detecting compiler -pipe support..." );

	string pipe_detection = "tools" + sSep + "rbuild" + sSep + "backend" + sSep + "mingw" + sSep + "pipe_detection.c";
	string pipe_detectionObjectFilename = ReplaceExtension ( pipe_detection,
	                                                         ".o" );
	string command = ssprintf (
		"%s -pipe -c %s -o %s 1>%s 2>%s",
		FixSeparatorForSystemCommand(compilerCommand).c_str (),
		pipe_detection.c_str (),
		pipe_detectionObjectFilename.c_str (),
		NUL,
		NUL );
	int exitcode = system ( command.c_str () );
	FILE* f = fopen ( pipe_detectionObjectFilename.c_str (), "rb" );
	if ( f )
	{
		usePipe = (exitcode == 0);
		fclose ( f );
		unlink ( pipe_detectionObjectFilename.c_str () );
	}
	else
		usePipe = false;

	if ( usePipe )
		printf ( "detected\n" );
	else
		printf ( "not detected\n" );
}

void
MingwBackend::DetectPCHSupport ()
{
	printf ( "Detecting compiler pre-compiled header support..." );

	if ( configuration.PrecompiledHeadersEnabled )
	{
		string path = "tools" + sSep + "rbuild" + sSep + "backend" + sSep + "mingw" + sSep + "pch_detection.h";
		string cmd = ssprintf (
			"%s -c %s 1>%s 2>%s",
			FixSeparatorForSystemCommand(compilerCommand).c_str (),
			path.c_str (),
			NUL,
			NUL );
		system ( cmd.c_str () );
		path += ".gch";

		FILE* f = fopen ( path.c_str (), "rb" );
		if ( f )
		{
			use_pch = true;
			fclose ( f );
			unlink ( path.c_str () );
		}
		else
			use_pch = false;

		if ( use_pch )
			printf ( "detected\n" );
		else
			printf ( "not detected\n" );
	}
	else
	{
		use_pch = false;
		printf ( "disabled\n" );
	}
}

void
MingwBackend::GetNonModuleInstallTargetFiles (
	vector<FileLocation>& out ) const
{
	for ( size_t i = 0; i < ProjectNode.installfiles.size (); i++ )
	{
		const InstallFile& installfile = *ProjectNode.installfiles[i];
		out.push_back ( *installfile.target );
	}
}

void
MingwBackend::GetModuleInstallTargetFiles (
	vector<FileLocation>& out ) const
{
	for ( std::map<std::string, Module*>::const_iterator p = ProjectNode.modules.begin (); p != ProjectNode.modules.end (); ++ p )
	{
		const Module& module = *p->second;
		if ( !module.enabled )
			continue;
		if ( module.install )
			out.push_back ( *module.install );
	}
}

void
MingwBackend::GetInstallTargetFiles (
	vector<FileLocation>& out ) const
{
	GetNonModuleInstallTargetFiles ( out );
	GetModuleInstallTargetFiles ( out );
}

void
MingwBackend::OutputInstallTarget ( const FileLocation& source,
                                    const FileLocation& target )
{
	fprintf ( fMakefile,
	          "%s: %s | %s\n",
	          GetFullName ( target ).c_str (),
	          GetFullName ( source ).c_str (),
	          GetFullPath ( target ).c_str () );
	fprintf ( fMakefile,
	          "\t$(ECHO_CP)\n" );
	fprintf ( fMakefile,
	          "\t${cp} %s %s 1>$(NUL)\n",
	          GetFullName ( source ).c_str (),
	          GetFullName ( target ).c_str () );
}

void
MingwBackend::OutputNonModuleInstallTargets ()
{
	for ( size_t i = 0; i < ProjectNode.installfiles.size (); i++ )
	{
		const InstallFile& installfile = *ProjectNode.installfiles[i];
		OutputInstallTarget ( *installfile.source, *installfile.target );
	}
}

const Module&
MingwBackend::GetAliasedModuleOrModule ( const Module& module ) const
{
	if ( module.aliasedModuleName.size () > 0 )
	{
		const Module* aliasedModule = ProjectNode.LocateModule ( module.aliasedModuleName );
		assert ( aliasedModule );
		return *aliasedModule;
	}
	else
		return module;
}

void
MingwBackend::OutputModuleInstallTargets ()
{
	for ( std::map<std::string, Module*>::const_iterator p = ProjectNode.modules.begin (); p != ProjectNode.modules.end (); ++ p )
	{
		const Module& module = *p->second;
		if ( !module.enabled )
			continue;
		if ( module.install )
		{
			const Module& aliasedModule = GetAliasedModuleOrModule ( module );
			OutputInstallTarget ( *aliasedModule.output, *module.install );
		}
	}
}

string
MingwBackend::GetRegistrySourceFiles ()
{
	return "boot" + sSep + "bootdata" + sSep + "hivecls_" + Environment::GetArch() + ".inf "
		"boot" + sSep + "bootdata" + sSep + "hivedef_" + Environment::GetArch() + ".inf "
		"boot" + sSep + "bootdata" + sSep + "hiveinst_" + Environment::GetArch() + ".inf "
		"boot" + sSep + "bootdata" + sSep + "hivesft_" + Environment::GetArch() + ".inf "
		"boot" + sSep + "bootdata" + sSep + "hivesys_" + Environment::GetArch() + ".inf ";
}

string
MingwBackend::GetRegistryTargetFiles ()
{
	string system32ConfigDirectory = "system32" + sSep + "config";
	FileLocation system32 ( InstallDirectory, system32ConfigDirectory, "" );

	vector<FileLocation> registry_files;
	registry_files.push_back ( FileLocation ( InstallDirectory, system32ConfigDirectory, "default" ) );
	registry_files.push_back ( FileLocation ( InstallDirectory, system32ConfigDirectory, "sam" ) );
	registry_files.push_back ( FileLocation ( InstallDirectory, system32ConfigDirectory, "security" ) );
	registry_files.push_back ( FileLocation ( InstallDirectory, system32ConfigDirectory, "software" ) );
	registry_files.push_back ( FileLocation ( InstallDirectory, system32ConfigDirectory, "system" ) );

	return v2s( this, registry_files, 6 );
}

void
MingwBackend::OutputRegistryInstallTarget ()
{
	FileLocation system32 ( InstallDirectory, "system32" + sSep + "config", "" );

	string registrySourceFiles = GetRegistrySourceFiles ();
	string registryTargetFiles = GetRegistryTargetFiles ();
	fprintf ( fMakefile,
	          "install_registry: %s\n",
	          registryTargetFiles.c_str () );
	fprintf ( fMakefile,
	          "%s: %s %s $(MKHIVE_TARGET)\n",
	          registryTargetFiles.c_str (),
	          registrySourceFiles.c_str (),
	          GetFullPath ( system32 ).c_str () );
	fprintf ( fMakefile,
	          "\t$(ECHO_MKHIVE)\n" );
	fprintf ( fMakefile,
	          "\t$(MKHIVE_TARGET) boot%cbootdata %s $(ARCH) boot%cbootdata%chiveinst_$(ARCH).inf\n",
	          cSep, GetFullPath ( system32 ).c_str (),
	          cSep, cSep );
	fprintf ( fMakefile,
	          "\n" );
}

void
MingwBackend::GenerateInstallTarget ()
{
	vector<FileLocation> vInstallTargetFiles;
	GetInstallTargetFiles ( vInstallTargetFiles );
	string installTargetFiles = v2s ( this, vInstallTargetFiles, 5 );
	string registryTargetFiles = GetRegistryTargetFiles ();

	fprintf ( fMakefile,
	          "install: %s %s\n",
	          installTargetFiles.c_str (),
	          registryTargetFiles.c_str () );
	OutputNonModuleInstallTargets ();
	OutputModuleInstallTargets ();
	OutputRegistryInstallTarget ();
	fprintf ( fMakefile,
	          "\n" );
}

void
MingwBackend::GetModuleTestTargets (
	vector<string>& out ) const
{
	for ( std::map<std::string, Module*>::const_iterator p = ProjectNode.modules.begin (); p != ProjectNode.modules.end (); ++ p )
	{
		const Module& module = *p->second;
		if ( !module.enabled )
			continue;
		if ( module.type == Test )
			out.push_back ( module.name );
	}
}

void
MingwBackend::GenerateTestTarget ()
{
	vector<string> vTestTargets;
	GetModuleTestTargets ( vTestTargets );
	string testTargets = v2s ( vTestTargets, 5 );

	fprintf ( fMakefile,
	          "test: %s\n",
		  testTargets.c_str () );
	fprintf ( fMakefile,
	          "\n" );
}

void
MingwBackend::GenerateDirectoryTargets ()
{
	intermediateDirectory->CreateRule ( fMakefile, "$(INTERMEDIATE)" );
	outputDirectory->CreateRule ( fMakefile, "$(OUTPUT)" );
	installDirectory->CreateRule ( fMakefile, "$(INSTALL)" );
}
