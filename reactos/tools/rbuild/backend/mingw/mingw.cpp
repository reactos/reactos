
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
typedef map<string,Directory*> directory_map;


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


class Directory
{
public:
	string name;
	directory_map subdirs;
	Directory ( const string& name );
	void Add ( const char* subdir );
	void GenerateTree ( const string& parent,
	                    bool verbose );
private:
	bool mkdir_p ( const char* path );
	string ReplaceVariable ( string name,
	                         string value,
	                         string path );
	string GetIntermediatePath ();
	string GetOutputPath ();
	void ResolveVariablesInPath ( char* buf,
	                              string path );
	bool CreateDirectory ( string path );
};

Directory::Directory ( const string& name_ )
	: name(name_)
{
}

void Directory::Add ( const char* subdir )
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
Directory::GetIntermediatePath ()
{
	return "obj-i386";
}

string
Directory::GetOutputPath ()
{
	return "output-i386";
}

void
Directory::ResolveVariablesInPath ( char* buf,
	                                string path )
{
	string s = ReplaceVariable ( "$(INTERMEDIATE)", GetIntermediatePath (), path );
	s = ReplaceVariable ( "$(OUTPUT)", GetOutputPath (), s );
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
	Backend* operator() ( Project& project, bool verbose )
	{
		return new MingwBackend ( project, verbose );
	}
} factory;


MingwBackend::MingwBackend ( Project& project, bool verbose )
	: Backend ( project, verbose ),
	  int_directories ( new Directory("$(INTERMEDIATE)") ),
	  out_directories ( new Directory("$(OUTPUT)") )
{
}

MingwBackend::~MingwBackend()
{
	delete int_directories;
	delete out_directories;
}

string
MingwBackend::AddDirectoryTarget ( const string& directory, bool out )
{
	const char* dir_name = "$(INTERMEDIATE)";
	Directory* dir = int_directories;
	if ( out )
	{
		dir_name = "$(OUTPUT)";
		dir = out_directories;
	}
	dir->Add ( directory.c_str() );
	return dir_name;
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
	fprintf ( fMakefile, "PROJECT_RCFLAGS = $(PROJECT_CFLAGS)\n" );
	fprintf ( fMakefile, "PROJECT_LFLAGS = %s\n",
	          GenerateProjectLFLAGS ().c_str () );
	fprintf ( fMakefile, "\n" );
}

bool
MingwBackend::IncludeInAllTarget ( const Module& module ) const
{
	if ( module.type == ObjectLibrary )
		return false;
	if ( module.type == BootSector )
		return false;
	if ( module.type == Iso )
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
	int_directories->GenerateTree ( "", verbose );
	out_directories->GenerateTree ( "", verbose );
	printf ( "done\n" );
}

string
FixupTargetFilename ( const string& targetFilename )
{
	return NormalizeFilename ( targetFilename );
}

void
MingwBackend::DetectPipeSupport ()
{
	printf ( "Detecting compiler -pipe support..." );

	string pipe_detection = "tools" SSEP "rbuild" SSEP "backend" SSEP "mingw" SSEP "pipe_detection.c";
	string pipe_detectionObjectFilename = ReplaceExtension ( pipe_detection,
	                                                         ".o" );
	string command = ssprintf (
		"gcc -pipe -c %s -o %s 2>%s",
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

	// TODO FIXME - eventually check for ROS_USE_PCH env var and
	// allow that to override use_pch if true
}

void
MingwBackend::DetectPCHSupport ()
{
	printf ( "Detecting compiler pre-compiled header support..." );

	string path = "tools" SSEP "rbuild" SSEP "backend" SSEP "mingw" SSEP "pch_detection.h";
	string cmd = ssprintf (
		"gcc -c %s 2>%s",
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

	// TODO FIXME - eventually check for ROS_USE_PCH env var and
	// allow that to override use_pch if true
}

string
MingwBackend::GetNonModuleInstallDirectories ( const string& installDirectory )
{
	string directories;
	for ( size_t i = 0; i < ProjectNode.installfiles.size (); i++ )
	{
		const InstallFile& installfile = *ProjectNode.installfiles[i];
		string targetDirectory ( installDirectory + SSEP + installfile.base );
		if ( directories.size () > 0 )
			directories += " ";
		directories += MingwModuleHandler::PassThruCacheDirectory (
			FixupTargetFilename ( targetDirectory ),
			true );
	}
	return directories;
}

string
MingwBackend::GetInstallDirectories ( const string& installDirectory )
{
	return GetNonModuleInstallDirectories ( installDirectory );
}

void
MingwBackend::GetNonModuleInstallFiles (
	vector<string>& out ) const
{
	for ( size_t i = 0; i < ProjectNode.installfiles.size (); i++ )
	{
		const InstallFile& installfile = *ProjectNode.installfiles[i];
		out.push_back ( NormalizeFilename ( installfile.GetPath () ) );
	}
}

void
MingwBackend::GetInstallFiles (
	vector<string>& out ) const
{
	GetNonModuleInstallFiles ( out );
}

void
MingwBackend::GetNonModuleInstallTargetFiles (
	string installDirectory,
	vector<string>& out ) const
{
	for ( size_t i = 0; i < ProjectNode.installfiles.size (); i++ )
	{
		const InstallFile& installfile = *ProjectNode.installfiles[i];
		string targetFilenameNoFixup = installDirectory + SSEP + installfile.base + SSEP + installfile.newname;
		string targetFilename = MingwModuleHandler::PassThruCacheDirectory (
			FixupTargetFilename ( targetFilenameNoFixup ),
			true );
		out.push_back ( targetFilename );
	}
}

void
MingwBackend::GetInstallTargetFiles (
	string installDirectory,
	vector<string>& out ) const
{
	GetNonModuleInstallTargetFiles ( installDirectory,
	                                 out );
}

void
MingwBackend::OutputInstallfileTargets ( const string& installDirectory )
{
	for ( size_t i = 0; i < ProjectNode.installfiles.size (); i++ )
	{
		const InstallFile& installfile = *ProjectNode.installfiles[i];
		string targetFilenameNoFixup = installDirectory + SSEP + installfile.base + SSEP + installfile.newname;
		string targetFilename = MingwModuleHandler::PassThruCacheDirectory (
			FixupTargetFilename ( targetFilenameNoFixup ),
			true );
		string targetDirectory = MingwModuleHandler::PassThruCacheDirectory (
			FixupTargetFilename ( installDirectory + SSEP + installfile.base ),
			true );
		fprintf ( fMakefile,
		          "%s: %s %s\n",
		          targetFilename.c_str (),
		          installfile.GetPath ().c_str (),
		          targetDirectory.c_str () );
		fprintf ( fMakefile,
		          "\t$(ECHO_CP)\n" );
		fprintf ( fMakefile,
		          "\t${cp} %s %s\n",
		          installfile.GetPath ().c_str (),
		          targetFilename.c_str () );
	}
}

void
MingwBackend::GenerateInstallTarget ()
{
	string installDirectoryNoFixup = "reactos";
	string installDirectory = MingwModuleHandler::PassThruCacheDirectory (
		NormalizeFilename ( installDirectoryNoFixup ),
		true );
	vector<string> vInstallTargetFiles;
	GetInstallTargetFiles ( installDirectoryNoFixup,
	                        vInstallTargetFiles );
	string installTargetFiles = v2s ( vInstallTargetFiles, 5 );

	fprintf ( fMakefile,
	          "install: %s %s\n",
	          installDirectory.c_str (),
	          installTargetFiles.c_str () );
	OutputInstallfileTargets ( installDirectoryNoFixup );
	fprintf ( fMakefile,
	          "\n" );
}
