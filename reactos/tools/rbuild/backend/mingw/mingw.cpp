
#include "../../pch.h"

#include "mingw.h"
#include <assert.h>
#include <dirent.h>
#include "modulehandler.h"

#ifdef WIN32
#define MKDIR(s) mkdir(s)
#else
#define MKDIR(s) mkdir(s, 0755)
#endif

using std::string;
using std::vector;
using std::set;
using std::map;

typedef set<string> set_string;


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


/* static */ string
Environment::GetVariable ( const string& name )
{
	char* value = getenv ( name.c_str () );
	if ( value != NULL && strlen ( value ) > 0 )
		return ssprintf ( "%s",
		                  value );
	else
		return "";
}


Directory::Directory ( const string& name_ )
	: name(name_)
{
}

void
Directory::Add ( const char* subdir )
{
	size_t i;
	string s1 = string ( subdir );
	if ( ( i = s1.find ( '$' ) ) != string::npos )
	{
		throw InvalidOperationException ( __FILE__,
		                                  __LINE__,
		                                  "No environment variables can be used here. Path was %s",
		                                  subdir );
	}

	const char* p = strpbrk ( subdir, "/\\" );
	if ( !p )
		p = subdir + strlen(subdir);
	string s ( subdir, p-subdir );
	if ( subdirs.find(s) == subdirs.end() )
		subdirs[s] = new Directory(s);
	if ( *p && *++p )
		subdirs[s]->Add ( p );
}

bool
Directory::mkdir_p ( const char* path )
{
	DIR *directory;
	directory = opendir ( path );
	if ( directory != NULL )
	{
		closedir ( directory );
		return false;
	}

	if ( MKDIR ( path ) != 0 )
		throw AccessDeniedException ( string ( path ) );
	return true;
}

bool
Directory::CreateDirectory ( string path )
{
	size_t index = 0;
	size_t nextIndex;
	if ( isalpha ( path[0] ) && path[1] == ':' && path[2] == CSEP )
	{
		nextIndex = path.find ( CSEP, 3);
	}
	else
		nextIndex = path.find ( CSEP );

	bool directoryWasCreated = false;
	while ( nextIndex != string::npos )
	{
		nextIndex = path.find ( CSEP, index + 1 );
		directoryWasCreated = mkdir_p ( path.substr ( 0, nextIndex ).c_str () );
		index = nextIndex;
	}
	return directoryWasCreated;
}

string
Directory::ReplaceVariable ( string name,
	                         string value,
	                         string path )
{
	size_t i = path.find ( name );
	if ( i != string::npos )
		return path.replace ( i, name.length (), value );
	else
		return path;
}

string
Directory::GetEnvironmentVariablePathOrDefault ( const string& name,
	                                             const string& defaultValue )
{
	const string& environmentVariableValue = Environment::GetVariable ( name );
	if ( environmentVariableValue.length () > 0 )
		return NormalizeFilename ( environmentVariableValue );
	else
		return defaultValue;
}

string
Directory::GetIntermediatePath ()
{
	return GetEnvironmentVariablePathOrDefault ( "ROS_INTERMEDIATE",
	                                             "obj-i386" );
}

string
Directory::GetOutputPath ()
{
	return GetEnvironmentVariablePathOrDefault ( "ROS_OUTPUT",
	                                             "output-i386" );
}

string
Directory::GetInstallPath ()
{
	return GetEnvironmentVariablePathOrDefault ( "ROS_INSTALL",
	                                             "reactos" );
}

void
Directory::ResolveVariablesInPath ( char* buf,
	                                string path )
{
	string s = ReplaceVariable ( "$(INTERMEDIATE)", GetIntermediatePath (), path );
	s = ReplaceVariable ( "$(OUTPUT)", GetOutputPath (), s );
	s = ReplaceVariable ( "$(INSTALL)", GetInstallPath (), s );
	strcpy ( buf, s.c_str () );
}

void
Directory::GenerateTree ( const string& parent,
	                      bool verbose )
{
	string path;

	if ( parent.size() )
	{
		char buf[256];
		
		path = parent + SSEP + name;
		ResolveVariablesInPath ( buf, path );
		if ( CreateDirectory ( buf ) && verbose )
			printf ( "Created %s\n", buf );
	}
	else
		path = name;

	for ( directory_map::iterator i = subdirs.begin();
		i != subdirs.end();
		++i )
	{
		i->second->GenerateTree ( path, verbose );
	}
}


static class MingwFactory : public Backend::Factory
{
public:
	MingwFactory() : Factory ( "mingw" ) {}
	Backend* operator() ( Project& project,
	                      bool verbose,
	                      bool cleanAsYouGo )
	{
		return new MingwBackend ( project,
		                          verbose,
		                          cleanAsYouGo );
	}
} factory;


MingwBackend::MingwBackend ( Project& project,
                             bool verbose,
                             bool cleanAsYouGo )
	: Backend ( project, verbose, cleanAsYouGo ),
	  intermediateDirectory ( new Directory ("$(INTERMEDIATE)" ) ),
	  outputDirectory ( new Directory ( "$(OUTPUT)" ) ),
	  installDirectory ( new Directory ( "$(INSTALL)" ) )
{
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
	directoryTree->Add ( directory.c_str() );
	return directoryTree->name;
}

void
MingwBackend::ProcessModules ()
{
	printf ( "Processing modules..." );

	vector<MingwModuleHandler*> v;
	size_t i;
	for ( i = 0; i < ProjectNode.modules.size (); i++ )
	{
		Module& module = *ProjectNode.modules[i];
		MingwModuleHandler* h = MingwModuleHandler::InstanciateHandler (
			module,
			this );
		if ( module.host == HostDefault )
		{
			module.host = h->DefaultHost();
			assert ( module.host != HostDefault );
		}
		v.push_back ( h );
	}

	size_t iend = v.size ();

	for ( i = 0; i < iend; i++ )
		v[i]->GenerateObjectMacro();
	fprintf ( fMakefile, "\n" );
	for ( i = 0; i < iend; i++ )
		v[i]->GenerateTargetMacro();
	fprintf ( fMakefile, "\n" );

	GenerateAllTarget ( v );
	GenerateInitTarget ();

	for ( i = 0; i < iend; i++ )
		v[i]->GenerateOtherMacros();

	for ( i = 0; i < iend; i++ )
	{
		MingwModuleHandler& h = *v[i];
		h.GeneratePreconditionDependencies ();
		h.Process ();
		h.GenerateInvocations ();
		h.GenerateCleanTarget ();
		delete v[i];
	}

	printf ( "done\n" );
}
	
void
MingwBackend::Process ()
{
	DetectCompiler ();
	DetectPipeSupport ();
	DetectPCHSupport ();
	CreateMakefile ();
	GenerateHeader ();
	GenerateGlobalVariables ();
	GenerateXmlBuildFilesMacro ();
	ProcessModules ();
	GenerateInstallTarget ();
	GenerateDirectories ();
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
	MingwModuleHandler::SetUsePch ( use_pch );
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
	fprintf ( fMakefile, "# THIS FILE IS AUTOMATICALLY GENERATED, EDIT 'ReactOS.xml' INSTEAD\n\n" );
}

void
MingwBackend::GenerateProjectCFlagsMacro ( const char* assignmentOperation,
                                           IfableData& data ) const
{
	size_t i;

	fprintf (
		fMakefile,
		"PROJECT_CFLAGS %s",
		assignmentOperation );
	for ( i = 0; i < data.includes.size(); i++ )
	{
		fprintf (
			fMakefile,
			" -I%s",
			data.includes[i]->directory.c_str() );
	}

	for ( i = 0; i < data.defines.size(); i++ )
	{
		Define& d = *data.defines[i];
		fprintf (
			fMakefile,
			" -D%s",
			d.name.c_str() );
		if ( d.value.size() )
			fprintf (
				fMakefile,
				"=%s",
				d.value.c_str() );
	}
	fprintf ( fMakefile, "\n" );
}

void
MingwBackend::GenerateGlobalCFlagsAndProperties (
	const char* assignmentOperation,
	IfableData& data ) const
{
	size_t i;

	for ( i = 0; i < data.properties.size(); i++ )
	{
		Property& prop = *data.properties[i];
		fprintf ( fMakefile, "%s := %s\n",
			prop.name.c_str(),
			prop.value.c_str() );
	}

	if ( data.includes.size() || data.defines.size() )
	{
		GenerateProjectCFlagsMacro ( assignmentOperation,
		                             data );
	}

	for ( i = 0; i < data.ifs.size(); i++ )
	{
		If& rIf = *data.ifs[i];
		if ( rIf.data.defines.size()
			|| rIf.data.includes.size()
			|| rIf.data.ifs.size() )
		{
			fprintf (
				fMakefile,
				"ifeq (\"$(%s)\",\"%s\")\n",
				rIf.property.c_str(),
				rIf.value.c_str() );
			GenerateGlobalCFlagsAndProperties (
				"+=",
				rIf.data );
			fprintf (
				fMakefile,
				"endif\n\n" );
		}
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
	GenerateGlobalCFlagsAndProperties (
		"=",
		ProjectNode.non_if_data );

	fprintf ( fMakefile, "PROJECT_RCFLAGS := $(PROJECT_CFLAGS)\n" );
	fprintf ( fMakefile, "PROJECT_WIDLFLAGS := $(PROJECT_CFLAGS)\n" );
	fprintf ( fMakefile, "PROJECT_LFLAGS := %s\n",
	          GenerateProjectLFLAGS ().c_str () );
	fprintf ( fMakefile, "PROJECT_CFLAGS += -Wall\n" );
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
	if ( module.type == Test )
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

string
MingwBackend::GetBuildToolDependencies () const
{
	string dependencies;
	for ( size_t i = 0; i < ProjectNode.modules.size (); i++ )
	{
		Module& module = *ProjectNode.modules[i];
		if ( module.type == BuildTool )
		{
			if ( dependencies.length () > 0 )
				dependencies += " ";
			dependencies += module.GetDependencyPath ();
		}
	}
	return dependencies;
}

void
MingwBackend::GenerateInitTarget () const
{
	fprintf ( fMakefile,
	          "INIT = %s\n",
	          GetBuildToolDependencies ().c_str () );
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
	for ( size_t i = 0; i < ProjectNode.xmlbuildfiles.size (); i++ )
	{
		XMLInclude& xmlbuildfile = *ProjectNode.xmlbuildfiles[i];
		if ( !xmlbuildfile.fileExists )
			continue;
		numberOfExistingFiles++;
		if ( xmlbuildFilenames.length () > 0 )
			xmlbuildFilenames += " ";
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
				          " \\\n",
				          xmlbuildFilenames.c_str () );
			}
			xmlbuildFilenames.resize ( 0 );
		}
		numberOfExistingFiles++;
	}
	fprintf ( fMakefile, "\n" );
}

void
MingwBackend::CheckAutomaticDependencies ()
{
	printf ( "Checking automatic dependencies..." );
	AutomaticDependency automaticDependency ( ProjectNode );
	automaticDependency.Process ();
	automaticDependency.CheckAutomaticDependencies ( verbose );
	printf ( "done\n" );
}

bool
MingwBackend::IncludeDirectoryTarget ( const string& directory ) const
{
	if ( directory == "$(INTERMEDIATE)" SSEP "tools")
		return false;
	else
		return true;
}

void
MingwBackend::GenerateDirectories ()
{
	printf ( "Creating directories..." );
	intermediateDirectory->GenerateTree ( "", verbose );
	outputDirectory->GenerateTree ( "", verbose );
	installDirectory->GenerateTree ( "", verbose );
	printf ( "done\n" );
}

bool
MingwBackend::TryToDetectThisCompiler ( const string& compiler )
{
	string command = ssprintf (
		"%s -v 2>%s",
		compiler.c_str (),
		NUL );
	int exitcode = system ( command.c_str () );
	return (exitcode == 0);
}

void
MingwBackend::DetectCompiler ()
{
	printf ( "Detecting compiler..." );

	bool detectedCompiler = false;
	const string& ROS_PREFIXValue = Environment::GetVariable ( "ROS_PREFIX" );
	if ( ROS_PREFIXValue.length () > 0 )
	{
		compilerCommand = ROS_PREFIXValue + "-gcc";
		detectedCompiler = TryToDetectThisCompiler ( compilerCommand );
	}
#if defined(WIN32)
	if ( !detectedCompiler )
	{
		compilerCommand = "gcc";
		detectedCompiler = TryToDetectThisCompiler ( compilerCommand );
	}
#endif
	if ( !detectedCompiler )
	{
		compilerCommand = "mingw32-gcc";
		detectedCompiler = TryToDetectThisCompiler ( compilerCommand );
	}
	if ( detectedCompiler )
		printf ( "detected (%s)\n", compilerCommand.c_str () );
	else
		printf ( "not detected\n" );
}

void
MingwBackend::DetectPipeSupport ()
{
	printf ( "Detecting compiler -pipe support..." );

	string pipe_detection = "tools" SSEP "rbuild" SSEP "backend" SSEP "mingw" SSEP "pipe_detection.c";
	string pipe_detectionObjectFilename = ReplaceExtension ( pipe_detection,
	                                                         ".o" );
	string command = ssprintf (
		"%s -pipe -c %s -o %s 2>%s",
		compilerCommand.c_str (),
		pipe_detection.c_str (),
		pipe_detectionObjectFilename.c_str (),
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

	string path = "tools" SSEP "rbuild" SSEP "backend" SSEP "mingw" SSEP "pch_detection.h";
	string cmd = ssprintf (
		"%s -c %s 2>%s",
		compilerCommand.c_str (),
		path.c_str (),
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

void
MingwBackend::GetNonModuleInstallTargetFiles (
	vector<string>& out ) const
{
	for ( size_t i = 0; i < ProjectNode.installfiles.size (); i++ )
	{
		const InstallFile& installfile = *ProjectNode.installfiles[i];
		string targetFilenameNoFixup = installfile.base + SSEP + installfile.newname;
		string targetFilename = MingwModuleHandler::PassThruCacheDirectory (
			NormalizeFilename ( targetFilenameNoFixup ),
			installDirectory );
		out.push_back ( targetFilename );
	}
}

void
MingwBackend::GetModuleInstallTargetFiles (
	vector<string>& out ) const
{
	for ( size_t i = 0; i < ProjectNode.modules.size (); i++ )
	{
		const Module& module = *ProjectNode.modules[i];
		if ( module.installName.length () > 0 )
		{
			string targetFilenameNoFixup = module.installBase + SSEP + module.installName;
			string targetFilename = MingwModuleHandler::PassThruCacheDirectory (
				NormalizeFilename ( targetFilenameNoFixup ),
				installDirectory );
			out.push_back ( targetFilename );
		}
	}
}

void
MingwBackend::GetInstallTargetFiles (
	vector<string>& out ) const
{
	GetNonModuleInstallTargetFiles ( out );
	GetModuleInstallTargetFiles ( out );
}

void
MingwBackend::OutputInstallTarget ( const string& sourceFilename,
	                                const string& targetFilename,
	                                const string& targetDirectory )
{
	string normalizedTargetFilename = MingwModuleHandler::PassThruCacheDirectory (
		NormalizeFilename ( targetDirectory + SSEP + targetFilename ),
		installDirectory );
	string normalizedTargetDirectory = MingwModuleHandler::PassThruCacheDirectory (
		NormalizeFilename ( targetDirectory ),
		installDirectory );
	fprintf ( fMakefile,
	          "%s: %s %s\n",
	          normalizedTargetFilename.c_str (),
	          sourceFilename.c_str (),
	          normalizedTargetDirectory.c_str () );
	fprintf ( fMakefile,
	          "\t$(ECHO_CP)\n" );
	fprintf ( fMakefile,
	          "\t${cp} %s %s 1>$(NUL)\n",
	          sourceFilename.c_str (),
	          normalizedTargetFilename.c_str () );
}

void
MingwBackend::OutputNonModuleInstallTargets ()
{
	for ( size_t i = 0; i < ProjectNode.installfiles.size (); i++ )
	{
		const InstallFile& installfile = *ProjectNode.installfiles[i];
		OutputInstallTarget ( installfile.GetPath (),
	                          installfile.newname,
	                          installfile.base );
	}
}

void
MingwBackend::OutputModuleInstallTargets ()
{
	for ( size_t i = 0; i < ProjectNode.modules.size (); i++ )
	{
		const Module& module = *ProjectNode.modules[i];
		if ( module.installName.length () > 0 )
		{
			string sourceFilename = MingwModuleHandler::PassThruCacheDirectory (
				NormalizeFilename ( module.GetPath () ),
				outputDirectory );
			OutputInstallTarget ( sourceFilename,
		                          module.installName,
		                          module.installBase );
		}
	}
}

string
MingwBackend::GetRegistrySourceFiles ()
{
	return "bootdata" SSEP "hivecls.inf "
		"bootdata" SSEP "hivedef.inf "
		"bootdata" SSEP "hiveinst.inf "
		"bootdata" SSEP "hivesft.inf "
		"bootdata" SSEP "hivesys.inf";
}

string
MingwBackend::GetRegistryTargetFiles ()
{
	string system32ConfigDirectory = NormalizeFilename (
		MingwModuleHandler::PassThruCacheDirectory (
		"system32" SSEP "config" SSEP,
		installDirectory ) );
	return system32ConfigDirectory + SSEP "default " +
		system32ConfigDirectory + SSEP "sam " +
		system32ConfigDirectory + SSEP "security " +
		system32ConfigDirectory + SSEP "software " +
		system32ConfigDirectory + SSEP "system";
}

void
MingwBackend::OutputRegistryInstallTarget ()
{
	string system32ConfigDirectory = NormalizeFilename (
		MingwModuleHandler::PassThruCacheDirectory (
		"system32" SSEP "config" SSEP,
		installDirectory ) );

	string registrySourceFiles = GetRegistrySourceFiles ();
	string registryTargetFiles = GetRegistryTargetFiles ();
	fprintf ( fMakefile,
	          "install_registry: %s\n",
	          registryTargetFiles.c_str () );
	fprintf ( fMakefile,
	          "%s: %s %s $(MKHIVE_TARGET)\n",
	          registryTargetFiles.c_str (),
	          registrySourceFiles.c_str (),
	          system32ConfigDirectory.c_str () );
	fprintf ( fMakefile,
	          "\t$(ECHO_MKHIVE)\n" );
	fprintf ( fMakefile,
	          "\t$(MKHIVE_TARGET) bootdata %s bootdata" SSEP "hiveinst.inf\n",
	          system32ConfigDirectory.c_str () );
	fprintf ( fMakefile,
	          "\n" );
}

void
MingwBackend::GenerateInstallTarget ()
{
	vector<string> vInstallTargetFiles;
	GetInstallTargetFiles ( vInstallTargetFiles );
	string installTargetFiles = v2s ( vInstallTargetFiles, 5 );

	fprintf ( fMakefile,
	          "install: %s install_registry\n",
	          installTargetFiles.c_str () );
	OutputNonModuleInstallTargets ();
	OutputModuleInstallTargets ();
	OutputRegistryInstallTarget ();
	fprintf ( fMakefile,
	          "\n" );
}
