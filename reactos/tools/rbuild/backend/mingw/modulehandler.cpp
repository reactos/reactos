#include "../../pch.h"
#include <assert.h>

#include "../../rbuild.h"
#include "mingw.h"
#include "modulehandler.h"

using std::string;
using std::vector;

#define CLEAN_FILE(f) clean_files.push_back ( f );

static string ros_temp = "$(TEMPORARY)";
MingwBackend*
MingwModuleHandler::backend = NULL;
FILE*
MingwModuleHandler::fMakefile = NULL;
bool
MingwModuleHandler::use_pch = false;

string
PrefixFilename (
	const string& filename,
	const string& prefix )
{
	if ( !prefix.length() )
		return filename;
	string out;
	const char* pfilename = filename.c_str();
	const char* p1 = strrchr ( pfilename, '/' );
	const char* p2 = strrchr ( pfilename, '\\' );
	if ( p1 || p2 )
	{
		if ( p2 > p1 )
			p1 = p2;
		out += string(pfilename,p1-pfilename) + CSEP;
		pfilename = p1 + 1;
	}
	out += prefix + pfilename;
	return out;
}

string
GetTargetMacro ( const Module& module, bool with_dollar )
{
	string s ( module.name );
	strupr ( &s[0] );
	s += "_TARGET";
	if ( with_dollar )
		return ssprintf ( "$(%s)", s.c_str() );
	return s;
}

MingwModuleHandler::MingwModuleHandler (
	const Module& module_ )

	: module(module_)
{
}

MingwModuleHandler::~MingwModuleHandler()
{
}

/*static*/ void
MingwModuleHandler::SetBackend ( MingwBackend* backend_ )
{
	backend = backend_;
}

/*static*/ void
MingwModuleHandler::SetMakefile ( FILE* f )
{
	fMakefile = f;
}

/*static*/ void
MingwModuleHandler::SetUsePch ( bool b )
{
	use_pch = b;
}

/* static*/ string
MingwModuleHandler::RemoveVariables ( string path)
{
	size_t i = path.find ( '$' );
	if ( i != string::npos )
	{
		size_t j = path.find ( ')', i );
		if ( j != string::npos )
		{
			if ( j + 2 < path.length () && path[j + 1] == CSEP )
				return path.substr ( j + 2);
			else
				return path.substr ( j + 1);
		}
	}
	return path;
}

/*static*/ string
MingwModuleHandler::PassThruCacheDirectory (
	const string &file,
	Directory* directoryTree )
{
	string directory ( GetDirectory ( RemoveVariables ( file ) ) );
	string generatedFilesDirectory = backend->AddDirectoryTarget ( directory,
	                                                               directoryTree );
	if ( directory.find ( generatedFilesDirectory ) != string::npos )
		/* This path already includes the generated files directory variable */
		return file;
	else
	{
		if ( file == "" )
			return generatedFilesDirectory;
		return generatedFilesDirectory + SSEP + file;
	}
}

/*static*/ Directory*
MingwModuleHandler::GetTargetDirectoryTree (
	const Module& module )
{
	if ( module.type == StaticLibrary )
		return backend->intermediateDirectory;
	return backend->outputDirectory;
}

/*static*/ string
MingwModuleHandler::GetTargetFilename (
	const Module& module,
	string_list* pclean_files )
{
	string target = PassThruCacheDirectory (
		NormalizeFilename ( module.GetPath () ),
		GetTargetDirectoryTree ( module ) );
	if ( pclean_files )
	{
		string_list& clean_files = *pclean_files;
		CLEAN_FILE ( target );
	}
	return target;
}

/*static*/ string
MingwModuleHandler::GetImportLibraryFilename (
	const Module& module,
	string_list* pclean_files )
{
	string target = PassThruCacheDirectory (
		NormalizeFilename ( module.GetDependencyPath () ),
		backend->intermediateDirectory );
	if ( pclean_files )
	{
		string_list& clean_files = *pclean_files;
		CLEAN_FILE ( target );
	}
	return target;
}

/*static*/ MingwModuleHandler*
MingwModuleHandler::InstanciateHandler (
	const Module& module,
	MingwBackend* backend )
{
	MingwModuleHandler* handler;
	switch ( module.type )
	{
		case BuildTool:
			handler = new MingwBuildToolModuleHandler ( module );
			break;
		case StaticLibrary:
			handler = new MingwStaticLibraryModuleHandler ( module );
			break;
		case ObjectLibrary:
			handler = new MingwObjectLibraryModuleHandler ( module );
			break;
		case Kernel:
			handler = new MingwKernelModuleHandler ( module );
			break;
		case NativeCUI:
			handler = new MingwNativeCUIModuleHandler ( module );
			break;
		case Win32CUI:
			handler = new MingwWin32CUIModuleHandler ( module );
			break;
		case Win32GUI:
			handler = new MingwWin32GUIModuleHandler ( module );
			break;
		case KernelModeDLL:
			handler = new MingwKernelModeDLLModuleHandler ( module );
			break;
		case NativeDLL:
			handler = new MingwNativeDLLModuleHandler ( module );
			break;
		case Win32DLL:
			handler = new MingwWin32DLLModuleHandler ( module );
			break;
		case KernelModeDriver:
			handler = new MingwKernelModeDriverModuleHandler ( module );
			break;
		case BootLoader:
			handler = new MingwBootLoaderModuleHandler ( module );
			break;
		case BootSector:
			handler = new MingwBootSectorModuleHandler ( module );
			break;
		case Iso:
			handler = new MingwIsoModuleHandler ( module );
			break;
		case LiveIso:
			handler = new MingwLiveIsoModuleHandler ( module );
			break;
		case Test:
			handler = new MingwTestModuleHandler ( module );
			break;
		case RpcServer:
			handler = new MingwRpcServerModuleHandler ( module );
			break;
		case RpcClient:
			handler = new MingwRpcClientModuleHandler ( module );
			break;
		default:
			throw UnknownModuleTypeException (
				module.node.location,
				module.type );
			break;
	}
	return handler;
}

string
MingwModuleHandler::GetWorkingDirectory () const
{
	return ".";
}

string
MingwModuleHandler::GetBasename ( const string& filename ) const
{
	size_t index = filename.find_last_of ( '.' );
	if ( index != string::npos )
		return filename.substr ( 0, index );
	return "";
}

string
MingwModuleHandler::GetActualSourceFilename (
	const string& filename ) const
{
	string extension = GetExtension ( filename );
	if ( extension == ".spec" || extension == ".SPEC" )
	{
		string basename = GetBasename ( filename );
		return PassThruCacheDirectory ( NormalizeFilename ( basename + ".stubs.c" ),
		                                backend->intermediateDirectory );
	}
	else if ( extension == ".idl" || extension == ".IDL" )
	{
		string basename = GetBasename ( filename );
		string newname;
		if ( module.type == RpcServer )
			newname = basename + "_s.c";
		else
			newname = basename + "_c.c";
		return PassThruCacheDirectory ( NormalizeFilename ( newname ),
		                                backend->intermediateDirectory );
	}
	else
		return filename;
}

string
MingwModuleHandler::GetModuleArchiveFilename () const
{
	if ( module.type == StaticLibrary )
		return GetTargetFilename ( module, NULL );
	return PassThruCacheDirectory ( ReplaceExtension (
		NormalizeFilename ( module.GetPath () ),
		".temp.a" ),
		backend->intermediateDirectory );
}

bool
MingwModuleHandler::IsGeneratedFile ( const File& file ) const
{
	string extension = GetExtension ( file.name );
	return ( extension == ".spec" || extension == ".SPEC" );
}

/*static*/ bool
MingwModuleHandler::ReferenceObjects (
	const Module& module )
{
	if ( module.type == ObjectLibrary )
		return true;
	if ( module.type == RpcServer )
		return true;
	if ( module.type == RpcClient )
		return true;
	return false;
}

string
MingwModuleHandler::GetImportLibraryDependency (
	const Module& importedModule )
{
	string dep;
	if ( ReferenceObjects ( importedModule ) )
		dep = GetTargetMacro ( importedModule );
	else
		dep = GetImportLibraryFilename ( importedModule, NULL );
	return dep;
}

void
MingwModuleHandler::GetTargets ( const Module& dependencyModule,
	                             string_list& targets )
{
	if ( dependencyModule.invocations.size () > 0 )
	{
		for ( size_t i = 0; i < dependencyModule.invocations.size (); i++ )
		{
			Invoke& invoke = *dependencyModule.invocations[i];
			invoke.GetTargets ( targets );
		}
	}
	else
		targets.push_back ( GetImportLibraryDependency ( dependencyModule ) );
}

void
MingwModuleHandler::GetModuleDependencies (
	string_list& dependencies )
{
	size_t iend = module.dependencies.size ();

	if ( iend == 0 )
		return;
	
	for ( size_t i = 0; i < iend; i++ )
	{
		const Dependency& dependency = *module.dependencies[i];
		const Module& dependencyModule = *dependency.dependencyModule;
		GetTargets ( dependencyModule,
		             dependencies );
	}
	GetDefinitionDependencies ( dependencies );
}

void
MingwModuleHandler::GetSourceFilenames ( string_list& list,
                                         bool includeGeneratedFiles ) const
{
	size_t i;

	const vector<File*>& files = module.non_if_data.files;
	for ( i = 0; i < files.size (); i++ )
	{
		if ( includeGeneratedFiles || !files[i]->IsGeneratedFile () )
		{
			list.push_back (
				GetActualSourceFilename ( files[i]->name ) );
		}
	}
	// intentionally make a copy so that we can append more work in
	// the middle of processing without having to go recursive
	vector<If*> v = module.non_if_data.ifs;
	for ( i = 0; i < v.size (); i++ )
	{
		size_t j;
		If& rIf = *v[i];
		// check for sub-ifs to add to list
		const vector<If*>& ifs = rIf.data.ifs;
		for ( j = 0; j < ifs.size (); j++ )
			v.push_back ( ifs[j] );
		const vector<File*>& files = rIf.data.files;
		for ( j = 0; j < files.size (); j++ )
		{
			File& file = *files[j];
			if ( includeGeneratedFiles || !file.IsGeneratedFile () )
			{
				list.push_back (
					GetActualSourceFilename ( file.name ) );
			}
		}
	}
}

void
MingwModuleHandler::GetSourceFilenamesWithoutGeneratedFiles (
	string_list& list ) const
{
	GetSourceFilenames ( list, false );
}

string
MingwModuleHandler::GetObjectFilename (
	const string& sourceFilename,
	string_list* pclean_files ) const
{
	Directory* directoryTree;

	string newExtension;
	string extension = GetExtension ( sourceFilename );
	if ( extension == ".rc" || extension == ".RC" )
		newExtension = ".coff";
	else if ( extension == ".spec" || extension == ".SPEC" )
		newExtension = ".stubs.o";
	else if ( extension == ".idl" || extension == ".IDL" )
	{
		if ( module.type == RpcServer )
			newExtension = "_s.o";
		else
			newExtension = "_c.o";
	}
	else
		newExtension = ".o";
	
	if ( module.type == BootSector )
		directoryTree = backend->outputDirectory;
	else
		directoryTree = backend->intermediateDirectory;

	string obj_file = PassThruCacheDirectory (
		NormalizeFilename ( ReplaceExtension (
			RemoveVariables ( sourceFilename ),
			                  newExtension ) ),
			directoryTree );
	if ( pclean_files )
	{
		string_list& clean_files = *pclean_files;
		CLEAN_FILE ( obj_file );
	}
	return obj_file;
}

void
MingwModuleHandler::GenerateCleanTarget () const
{
	if ( 0 == clean_files.size() )
		return;
	fprintf ( fMakefile, ".PHONY: %s_clean\n", module.name.c_str() );
	fprintf ( fMakefile, "%s_clean:\n\t-@${rm}", module.name.c_str() );
	for ( size_t i = 0; i < clean_files.size(); i++ )
	{
		if ( 9==((i+1)%10) )
			fprintf ( fMakefile, " 2>$(NUL)\n\t-@${rm}" );
		fprintf ( fMakefile, " %s", clean_files[i].c_str() );
	}
	fprintf ( fMakefile, " 2>$(NUL)\n" );
	fprintf ( fMakefile, "clean: %s_clean\n\n", module.name.c_str() );
}

void
MingwModuleHandler::GenerateInstallTarget () const
{
	if ( module.installName.length () == 0 )
		return;
	fprintf ( fMakefile, ".PHONY: %s_install\n", module.name.c_str() );
	string normalizedTargetFilename = MingwModuleHandler::PassThruCacheDirectory (
		NormalizeFilename ( module.installBase + SSEP + module.installName ),
		backend->installDirectory );
	fprintf ( fMakefile,
	          "%s_install: %s\n",
	          module.name.c_str (),
	          normalizedTargetFilename.c_str() );
}

string
MingwModuleHandler::GetObjectFilenames ()
{
	const vector<File*>& files = module.non_if_data.files;
	if ( files.size () == 0 )
		return "";
	
	string objectFilenames ( "" );
	for ( size_t i = 0; i < files.size (); i++ )
	{
		if ( objectFilenames.size () > 0 )
			objectFilenames += " ";
		objectFilenames +=
			GetObjectFilename ( files[i]->name, NULL );
	}
	return objectFilenames;
}

/* static */ string
MingwModuleHandler::GenerateGccDefineParametersFromVector (
	const vector<Define*>& defines )
{
	string parameters;
	for ( size_t i = 0; i < defines.size (); i++ )
	{
		Define& define = *defines[i];
		if (parameters.length () > 0)
			parameters += " ";
		parameters += "-D";
		parameters += define.name;
		if (define.value.length () > 0)
		{
			parameters += "=";
			parameters += define.value;
		}
	}
	return parameters;
}

string
MingwModuleHandler::GenerateGccDefineParameters () const
{
	string parameters = GenerateGccDefineParametersFromVector ( module.project.non_if_data.defines );
	string s = GenerateGccDefineParametersFromVector ( module.non_if_data.defines );
	if ( s.length () > 0 )
	{
		parameters += " ";
		parameters += s;
	}
	return parameters;
}

string
MingwModuleHandler::ConcatenatePaths (
	const string& path1,
	const string& path2 ) const
{
	if ( ( path1.length () == 0 ) || ( path1 == "." ) || ( path1 == "./" ) )
		return path2;
	if ( path1[path1.length ()] == CSEP )
		return path1 + path2;
	else
		return path1 + CSEP + path2;
}

/* static */ string
MingwModuleHandler::GenerateGccIncludeParametersFromVector ( const vector<Include*>& includes )
{
	string parameters;
	for ( size_t i = 0; i < includes.size (); i++ )
	{
		Include& include = *includes[i];
		if ( parameters.length () > 0 )
			parameters += " ";
		parameters += "-I" + include.directory;
	}
	return parameters;
}

string
MingwModuleHandler::GenerateGccIncludeParameters () const
{
	string parameters = GenerateGccIncludeParametersFromVector ( module.non_if_data.includes );
	string s = GenerateGccIncludeParametersFromVector ( module.project.non_if_data.includes );
	if ( s.length () > 0 )
	{
		parameters += " ";
		parameters += s;
	}
	return parameters;
}

string
MingwModuleHandler::GenerateCompilerParametersFromVector ( const vector<CompilerFlag*>& compilerFlags ) const
{
	string parameters;
	for ( size_t i = 0; i < compilerFlags.size (); i++ )
	{
		CompilerFlag& compilerFlag = *compilerFlags[i];
		if ( parameters.length () > 0 )
			parameters += " ";
		parameters += compilerFlag.flag;
	}
	return parameters;
}

string
MingwModuleHandler::GenerateLinkerParametersFromVector ( const vector<LinkerFlag*>& linkerFlags ) const
{
	string parameters;
	for ( size_t i = 0; i < linkerFlags.size (); i++ )
	{
		LinkerFlag& linkerFlag = *linkerFlags[i];
		if ( parameters.length () > 0 )
			parameters += " ";
		parameters += linkerFlag.flag;
	}
	return parameters;
}

string
MingwModuleHandler::GenerateImportLibraryDependenciesFromVector (
	const vector<Library*>& libraries )
{
	string dependencies ( "" );
	int wrap_count = 0;
	for ( size_t i = 0; i < libraries.size (); i++ )
	{
		if ( wrap_count++ == 5 )
			dependencies += " \\\n\t\t", wrap_count = 0;
		else if ( dependencies.size () > 0 )
			dependencies += " ";
		dependencies += GetImportLibraryDependency ( *libraries[i]->imported_module );
	}
	return dependencies;
}

string
MingwModuleHandler::GenerateLinkerParameters () const
{
	return GenerateLinkerParametersFromVector ( module.linkerFlags );
}

void
MingwModuleHandler::GenerateMacro (
	const char* assignmentOperation,
	const string& macro,
	const IfableData& data )
{
	size_t i;

	fprintf (
		fMakefile,
		"%s %s",
		macro.c_str(),
		assignmentOperation );
	
	string compilerParameters = GenerateCompilerParametersFromVector ( data.compilerFlags );
	if ( compilerParameters.size () > 0 )
	{
		fprintf (
			fMakefile,
			" %s",
			compilerParameters.c_str () );
	}

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
MingwModuleHandler::GenerateMacros (
	const char* assignmentOperation,
	const IfableData& data,
	const vector<LinkerFlag*>* linkerFlags )
{
	size_t i;

	if ( data.includes.size () > 0 || data.defines.size () > 0 || data.compilerFlags.size () > 0 )
	{
		GenerateMacro ( assignmentOperation,
		                cflagsMacro,
		                data );
		GenerateMacro ( assignmentOperation,
		                windresflagsMacro,
		                data );
	}
	
	if ( linkerFlags != NULL )
	{
		string linkerParameters = GenerateLinkerParametersFromVector ( *linkerFlags );
		if ( linkerParameters.size () > 0 )
		{
			fprintf (
				fMakefile,
				"%s %s %s\n",
				linkerflagsMacro.c_str (),
				assignmentOperation,
				linkerParameters.c_str() );
		}
	}

	if ( data.libraries.size () > 0 )
	{
		string deps = GenerateImportLibraryDependenciesFromVector ( data.libraries );
		if ( deps.size () > 0 )
		{
			fprintf (
				fMakefile,
				"%s %s %s\n",
				libsMacro.c_str(),
				assignmentOperation,
				deps.c_str() );
		}
	}

	const vector<If*>& ifs = data.ifs;
	for ( i = 0; i < ifs.size(); i++ )
	{
		If& rIf = *ifs[i];
		if ( rIf.data.defines.size()
			|| rIf.data.includes.size()
			|| rIf.data.libraries.size()
			|| rIf.data.files.size()
			|| rIf.data.compilerFlags.size()
			|| rIf.data.ifs.size() )
		{
			fprintf (
				fMakefile,
				"ifeq (\"$(%s)\",\"%s\")\n",
				rIf.property.c_str(),
				rIf.value.c_str() );
			GenerateMacros (
				"+=",
				rIf.data,
				NULL );
			fprintf ( 
				fMakefile,
				"endif\n\n" );
		}
	}
}

void
MingwModuleHandler::CleanupFileVector ( vector<File*>& sourceFiles )
{
	for (size_t i = 0; i < sourceFiles.size (); i++)
		delete sourceFiles[i];
}

void
MingwModuleHandler::GetModuleSpecificSourceFiles ( vector<File*>& sourceFiles )
{
}

void
MingwModuleHandler::GenerateObjectMacros (
	const char* assignmentOperation,
	const IfableData& data,
	const vector<LinkerFlag*>* linkerFlags )
{
	size_t i;

	const vector<File*>& files = data.files;
	if ( files.size () > 0 )
	{
		for ( i = 0; i < files.size (); i++ )
		{
			File& file = *files[i];
			if ( file.first )
			{
				fprintf ( fMakefile,
					"%s := %s $(%s)\n",
					objectsMacro.c_str(),
					GetObjectFilename (
						file.name, NULL ).c_str (),
					objectsMacro.c_str() );
			}
		}
		fprintf (
			fMakefile,
			"%s %s",
			objectsMacro.c_str (),
			assignmentOperation );
		for ( i = 0; i < files.size(); i++ )
		{
			File& file = *files[i];
			if ( !file.first )
			{
				fprintf (
					fMakefile,
					"%s%s",
					( i%10 == 9 ? " \\\n\t" : " " ),
					GetObjectFilename (
						file.name, NULL ).c_str () );
			}
		}
		fprintf ( fMakefile, "\n" );
	}

	const vector<If*>& ifs = data.ifs;
	for ( i = 0; i < ifs.size(); i++ )
	{
		If& rIf = *ifs[i];
		if ( rIf.data.defines.size()
			|| rIf.data.includes.size()
			|| rIf.data.libraries.size()
			|| rIf.data.files.size()
			|| rIf.data.compilerFlags.size()
			|| rIf.data.ifs.size() )
		{
			fprintf (
				fMakefile,
				"ifeq (\"$(%s)\",\"%s\")\n",
				rIf.property.c_str(),
				rIf.value.c_str() );
			GenerateObjectMacros (
				"+=",
				rIf.data,
				NULL );
			fprintf ( 
				fMakefile,
				"endif\n\n" );
		}
	}

	vector<File*> sourceFiles;
	GetModuleSpecificSourceFiles ( sourceFiles );
	for ( i = 0; i < sourceFiles.size (); i++ )
	{
		fprintf (
			fMakefile,
			"%s += %s\n",
			objectsMacro.c_str(),
			GetObjectFilename (
				sourceFiles[i]->name, NULL ).c_str () );
	}
	CleanupFileVector ( sourceFiles );
}

void
MingwModuleHandler::GenerateGccCommand (
	const string& sourceFilename,
	const string& cc,
	const string& cflagsMacro )
{
	string dependencies = sourceFilename;
	if ( module.pch && use_pch )
		dependencies += " " + module.pch->file.name + ".gch";
	
	/* WIDL generated headers may be used */
	dependencies += " " + GetLinkingDependenciesMacro ();
	dependencies += " " + NormalizeFilename ( module.xmlbuildFile );

	string objectFilename = GetObjectFilename (
		sourceFilename, &clean_files );
	fprintf ( fMakefile,
	          "%s: %s | %s\n",
	          objectFilename.c_str (),
	          dependencies.c_str (),
	          GetDirectory ( objectFilename ).c_str () );
	fprintf ( fMakefile, "\t$(ECHO_CC)\n" );
	fprintf ( fMakefile,
	         "\t%s -c $< -o $@ %s\n",
	         cc.c_str (),
	         cflagsMacro.c_str () );
}

void
MingwModuleHandler::GenerateGccAssemblerCommand (
	const string& sourceFilename,
	const string& cc,
	const string& cflagsMacro )
{
	string dependencies = sourceFilename;
	dependencies += " " + NormalizeFilename ( module.xmlbuildFile );
	string objectFilename = GetObjectFilename (
		sourceFilename, &clean_files );
	fprintf ( fMakefile,
	          "%s: %s | %s\n",
	          objectFilename.c_str (),
	          dependencies.c_str (),
	          GetDirectory ( objectFilename ).c_str () );
	fprintf ( fMakefile, "\t$(ECHO_GAS)\n" );
	fprintf ( fMakefile,
	          "\t%s -x assembler-with-cpp -c $< -o $@ -D__ASM__ %s\n",
	          cc.c_str (),
	          cflagsMacro.c_str () );
}

void
MingwModuleHandler::GenerateNasmCommand (
	const string& sourceFilename,
	const string& nasmflagsMacro )
{
	string dependencies = sourceFilename;
	dependencies += " " + NormalizeFilename ( module.xmlbuildFile );
	string objectFilename = GetObjectFilename (
		sourceFilename, &clean_files );
	fprintf ( fMakefile,
	          "%s: %s | %s\n",
	          objectFilename.c_str (),
	          dependencies.c_str (),
	          GetDirectory ( objectFilename ).c_str () );
	fprintf ( fMakefile, "\t$(ECHO_NASM)\n" );
	fprintf ( fMakefile,
	          "\t%s -f win32 $< -o $@ %s\n",
	          "$(Q)${nasm}",
	          nasmflagsMacro.c_str () );
}

void
MingwModuleHandler::GenerateWindresCommand (
	const string& sourceFilename,
	const string& windresflagsMacro )
{
	string dependencies = sourceFilename;
	dependencies += " " + NormalizeFilename ( module.xmlbuildFile );
	string objectFilename =
		GetObjectFilename ( sourceFilename, &clean_files );
	string sourceFilenamePart = ReplaceExtension ( GetFilename ( sourceFilename ), "" );
	string rciFilename = ros_temp + module.name + "." + sourceFilenamePart + ".rci.tmp";
	string resFilename = ros_temp + module.name + "." + sourceFilenamePart + ".res.tmp";
	if ( module.useWRC )
	{
		fprintf ( fMakefile,
		          "%s: %s $(WRC_TARGET) | %s\n",
		          objectFilename.c_str (),
		          dependencies.c_str (),
		          GetDirectory ( objectFilename ).c_str () );
		fprintf ( fMakefile, "\t$(ECHO_WRC)\n" );
		fprintf ( fMakefile,
		         "\t${gcc} -xc -E -DRC_INVOKED ${%s} %s > %s\n",
		         windresflagsMacro.c_str (),
		         sourceFilename.c_str (),
		         rciFilename.c_str () );
		fprintf ( fMakefile,
		         "\t$(Q)$(WRC_TARGET) ${%s} %s %s\n",
		         windresflagsMacro.c_str (),
		         rciFilename.c_str (),
		         resFilename.c_str () );
		fprintf ( fMakefile,
		         "\t-@${rm} %s 2>$(NUL)\n",
		         rciFilename.c_str () );
		fprintf ( fMakefile,
		         "\t${windres} %s -o $@\n",
		         resFilename.c_str () );
		fprintf ( fMakefile,
		         "\t-@${rm} %s 2>$(NUL)\n",
		         resFilename.c_str () );
	}
	else
	{
		fprintf ( fMakefile,
		          "%s: %s $(WRC_TARGET) | %s\n",
		          objectFilename.c_str (),
		          dependencies.c_str (),
		          GetDirectory ( objectFilename ).c_str () );
		fprintf ( fMakefile, "\t$(ECHO_WRC)\n" );
		fprintf ( fMakefile,
		         "\t${windres} $(%s) %s -o $@\n",
		         windresflagsMacro.c_str (),
		         sourceFilename.c_str () );
	}
}

void
MingwModuleHandler::GenerateWinebuildCommands (
	const string& sourceFilename )
{
	string dependencies = sourceFilename;
	dependencies += " " + NormalizeFilename ( module.xmlbuildFile );

	string basename = GetBasename ( sourceFilename );
	string def_file = PassThruCacheDirectory (
		basename + ".spec.def",
		backend->intermediateDirectory );
	CLEAN_FILE(def_file);

	string stub_file = PassThruCacheDirectory (
		basename + ".stubs.c",
		backend->intermediateDirectory );
	CLEAN_FILE(stub_file)

	fprintf ( fMakefile,
	          "%s: %s $(WINEBUILD_TARGET)\n",
	          def_file.c_str (),
	          dependencies.c_str () );
	fprintf ( fMakefile, "\t$(ECHO_WINEBLD)\n" );
	fprintf ( fMakefile,
	          "\t%s -o %s --def -E %s\n",
	          "$(Q)$(WINEBUILD_TARGET)",
	          def_file.c_str (),
	          sourceFilename.c_str () );
	fprintf ( fMakefile,
	          "%s: %s $(WINEBUILD_TARGET)\n",
	          stub_file.c_str (),
	          sourceFilename.c_str () );
	fprintf ( fMakefile, "\t$(ECHO_WINEBLD)\n" );
	fprintf ( fMakefile,
	          "\t%s -o %s --pedll %s\n",
	          "$(Q)$(WINEBUILD_TARGET)",
	          stub_file.c_str (),
	          sourceFilename.c_str () );
}

string
MingwModuleHandler::GetWidlFlags ( const File& file )
{
	return file.switches;
}

string
MingwModuleHandler::GetRpcServerHeaderFilename ( string basename ) const
{
	return basename + "_s.h";
}
		
void
MingwModuleHandler::GenerateWidlCommandsServer (
	const File& file,
	const string& widlflagsMacro )
{
	string dependencies = file.name;
	dependencies += " " + NormalizeFilename ( module.xmlbuildFile );

	string basename = GetBasename ( file.name );

	/*string generatedHeaderFilename = PassThruCacheDirectory (
		basename + ".h",
		backend->intermediateDirectory );
	CLEAN_FILE(generatedHeaderFilename);
	*/
	string generatedHeaderFilename = GetRpcServerHeaderFilename ( basename );
	CLEAN_FILE(generatedHeaderFilename);

  	string generatedServerFilename = PassThruCacheDirectory (
		basename + "_s.c",
		backend->intermediateDirectory );
	CLEAN_FILE(generatedServerFilename);

	fprintf ( fMakefile,
	          "%s %s: %s $(WIDL_TARGET) | %s\n",
	          generatedServerFilename.c_str (),
	          generatedHeaderFilename.c_str (),
	          dependencies.c_str (),
	          GetDirectory ( generatedServerFilename ).c_str () );
	fprintf ( fMakefile, "\t$(ECHO_WIDL)\n" );
	fprintf ( fMakefile,
	          "\t%s %s %s -h -H %s -s -S %s %s\n",
	          "$(Q)$(WIDL_TARGET)",
	          GetWidlFlags ( file ).c_str (),
	          widlflagsMacro.c_str (),
	          generatedHeaderFilename.c_str (),
	          generatedServerFilename.c_str (),
	          file.name.c_str () );
}

string
MingwModuleHandler::GetRpcClientHeaderFilename ( string basename ) const
{
	return basename + "_c.h";
}

void
MingwModuleHandler::GenerateWidlCommandsClient (
	const File& file,
	const string& widlflagsMacro )
{
	string dependencies = file.name;
	dependencies += " " + NormalizeFilename ( module.xmlbuildFile );

	string basename = GetBasename ( file.name );

	/*string generatedHeaderFilename = PassThruCacheDirectory (
		basename + ".h",
		backend->intermediateDirectory );
	CLEAN_FILE(generatedHeaderFilename);
	*/
	string generatedHeaderFilename = GetRpcClientHeaderFilename ( basename );
	CLEAN_FILE(generatedHeaderFilename);

  	string generatedClientFilename = PassThruCacheDirectory (
		basename + "_c.c",
		backend->intermediateDirectory );
	CLEAN_FILE(generatedClientFilename);

	fprintf ( fMakefile,
	          "%s %s: %s $(WIDL_TARGET) | %s\n",
	          generatedClientFilename.c_str (),
	          generatedHeaderFilename.c_str (),
	          dependencies.c_str (),
	          GetDirectory ( generatedClientFilename ).c_str () );
	fprintf ( fMakefile, "\t$(ECHO_WIDL)\n" );
	fprintf ( fMakefile,
	          "\t%s %s %s -h -H %s -c -C %s %s\n",
	          "$(Q)$(WIDL_TARGET)",
	          GetWidlFlags ( file ).c_str (),
	          widlflagsMacro.c_str (),
	          generatedHeaderFilename.c_str (),
	          generatedClientFilename.c_str (),
	          file.name.c_str () );
}

void
MingwModuleHandler::GenerateWidlCommands (
	const File& file,
	const string& widlflagsMacro )
{
	if ( module.type == RpcServer )
		GenerateWidlCommandsServer ( file,
		                             widlflagsMacro );
	else
		GenerateWidlCommandsClient ( file,
		                             widlflagsMacro );
}

void
MingwModuleHandler::GenerateCommands (
	const File& file,
	const string& cc,
	const string& cppc,
	const string& cflagsMacro,
	const string& nasmflagsMacro,
	const string& windresflagsMacro,
	const string& widlflagsMacro )
{
	string extension = GetExtension ( file.name );
	if ( extension == ".c" || extension == ".C" )
	{
		GenerateGccCommand ( file.name,
		                     cc,
		                     cflagsMacro );
		return;
	}
	else if ( extension == ".cc" || extension == ".CC" ||
	          extension == ".cpp" || extension == ".CPP" ||
	          extension == ".cxx" || extension == ".CXX" )
	{
		GenerateGccCommand ( file.name,
		                     cppc,
		                     cflagsMacro );
		return;
	}
	else if ( extension == ".s" || extension == ".S" )
	{
		GenerateGccAssemblerCommand ( file.name,
		                              cc,
		                              cflagsMacro );
		return;
	}
	else if ( extension == ".asm" || extension == ".ASM" )
	{
		GenerateNasmCommand ( file.name,
		                      nasmflagsMacro );
		return;
	}
	else if ( extension == ".rc" || extension == ".RC" )
	{
		GenerateWindresCommand ( file.name,
		                         windresflagsMacro );
		return;
	}
	else if ( extension == ".spec" || extension == ".SPEC" )
	{
		GenerateWinebuildCommands ( file.name );
		GenerateGccCommand ( GetActualSourceFilename ( file.name ),
		                     cc,
		                     cflagsMacro );
		return;
	}
	else if ( extension == ".idl" || extension == ".IDL" )
	{
		GenerateWidlCommands ( file,
		                       widlflagsMacro );
		GenerateGccCommand ( GetActualSourceFilename ( file.name ),
		                     cc,
		                     cflagsMacro );
		return;
	}

	throw InvalidOperationException ( __FILE__,
	                                  __LINE__,
	                                  "Unsupported filename extension '%s' in file '%s'",
	                                  extension.c_str (),
	                                  file.name.c_str () );
}

void
MingwModuleHandler::GenerateBuildMapCode ()
{
	fprintf ( fMakefile,
	          "ifeq ($(ROS_BUILDMAP),full)\n" );

	string mapFilename = PassThruCacheDirectory (
		GetBasename ( module.GetPath () ) + ".map",
		backend->outputDirectory );
	CLEAN_FILE ( mapFilename );
	
	fprintf ( fMakefile,
	          "\t$(ECHO_OBJDUMP)\n" );
	fprintf ( fMakefile,
	          "\t$(Q)${objdump} -d -S $@ > %s\n",
	          mapFilename.c_str () );

	fprintf ( fMakefile,
	          "else\n" );
	fprintf ( fMakefile,
	          "ifeq ($(ROS_BUILDMAP),yes)\n" );

	fprintf ( fMakefile,
	          "\t$(ECHO_NM)\n" );
	fprintf ( fMakefile,
	          "\t$(Q)${nm} --numeric-sort $@ > %s\n",
	          mapFilename.c_str () );

	fprintf ( fMakefile,
	          "endif\n" );

	fprintf ( fMakefile,
	          "endif\n" );
}

void
MingwModuleHandler::GenerateBuildNonSymbolStrippedCode ()
{
	fprintf ( fMakefile,
	          "ifeq ($(ROS_BUILDNOSTRIP),yes)\n" );

	string filename = module.GetPath ();
	string outputFilename = PassThruCacheDirectory (
		filename,
		backend->outputDirectory );
	string nostripFilename = PassThruCacheDirectory (
		GetBasename ( filename ) + ".nostrip" + GetExtension ( filename ),
		backend->outputDirectory );
	CLEAN_FILE ( nostripFilename );
	
	fprintf ( fMakefile,
	          "\t$(ECHO_CP)\n" );
	fprintf ( fMakefile,
			  "\t${cp} %s %s 1>$(NUL)\n",
			  outputFilename.c_str (),
	          nostripFilename.c_str () );
	
	fprintf ( fMakefile,
	          "endif\n" );
}

void
MergeStringVector ( const vector<string>& input,
	                vector<string>& output )
{
	int wrap_at = 25;
	string s;
	int wrap_count = -1;
	for ( size_t i = 0; i < input.size (); i++ )
	{
		if ( input[i].size () == 0 )
			continue;
		if ( wrap_count++ == wrap_at )
		{
			output.push_back ( s );
			s = "";
			wrap_count = 0;
		}
		else if ( s.size () > 0)
			s += " ";
		s += input[i];
	}
	if ( s.length () > 0 )
		output.push_back ( s );
}

void
MingwModuleHandler::GetObjectsVector ( const IfableData& data,
                                       vector<string>& objectFiles ) const
{
	for ( size_t i = 0; i < data.files.size (); i++ )
	{
		File& file = *data.files[i];
		objectFiles.push_back ( GetObjectFilename ( file.name, NULL ) );
	}
}

void
MingwModuleHandler::GenerateCleanObjectsAsYouGoCode () const
{
	if ( backend->configuration.CleanAsYouGo )
	{
		vector<string> objectFiles;
		GetObjectsVector ( module.non_if_data,
                           objectFiles );
		vector<string> lines;
		MergeStringVector ( objectFiles,
	                        lines );
		for ( size_t i = 0; i < lines.size (); i++ )
		{
			fprintf ( fMakefile,
			          "\t-@${rm} %s 2>$(NUL)\n",
			          lines[i].c_str () );
		}
	}
}

void
MingwModuleHandler::GenerateRunRsymCode () const
{
	fprintf ( fMakefile,
	          "\t$(ECHO_RSYM)\n" );
	fprintf ( fMakefile,
	          "\t$(Q)$(RSYM_TARGET) $@ $@\n\n" );
}

void
MingwModuleHandler::GenerateLinkerCommand (
	const string& dependencies,
	const string& linker,
	const string& linkerParameters,
	const string& objectsMacro,
	const string& libsMacro )
{
	string target ( GetTargetMacro ( module ) );
	string target_folder ( GetDirectory ( GetTargetFilename ( module, NULL ) ) );
	string def_file = GetDefinitionFilename ();

	fprintf ( fMakefile,
		"%s: %s %s $(RSYM_TARGET) | %s\n",
		target.c_str (),
		def_file.c_str (),
		dependencies.c_str (),
		target_folder.c_str () );
	fprintf ( fMakefile, "\t$(ECHO_LD)\n" );
	string targetName ( module.GetTargetName () );

	if ( module.IsDLL () )
	{
		string base_tmp = ros_temp + module.name + ".base.tmp";
		CLEAN_FILE ( base_tmp );
		string junk_tmp = ros_temp + module.name + ".junk.tmp";
		CLEAN_FILE ( junk_tmp );
		string temp_exp = ros_temp + module.name + ".temp.exp";
		CLEAN_FILE ( temp_exp );
	
		fprintf ( fMakefile,
		          "\t%s %s -Wl,--base-file,%s -o %s %s %s %s\n",
		          linker.c_str (),
		          linkerParameters.c_str (),
		          base_tmp.c_str (),
		          junk_tmp.c_str (),
		          objectsMacro.c_str (),
		          libsMacro.c_str (),
		          GetLinkerMacro ().c_str () );
	
		fprintf ( fMakefile,
		          "\t-@${rm} %s 2>$(NUL)\n",
		          junk_tmp.c_str () );
	
		string killAt = module.mangledSymbols ? "" : "--kill-at";
		fprintf ( fMakefile,
		          "\t${dlltool} --dllname %s --base-file %s --def %s --output-exp %s %s\n",
		          targetName.c_str (),
		          base_tmp.c_str (),
		          def_file.c_str (),
		          temp_exp.c_str (),
		          killAt.c_str () );
	
		fprintf ( fMakefile,
		          "\t-@${rm} %s 2>$(NUL)\n",
		          base_tmp.c_str () );
	
		fprintf ( fMakefile,
		          "\t%s %s %s -o %s %s %s %s\n",
		          linker.c_str (),
		          linkerParameters.c_str (),
		          temp_exp.c_str (),
		          target.c_str (),
		          objectsMacro.c_str (),
		          libsMacro.c_str (),
		          GetLinkerMacro ().c_str () );
	
		fprintf ( fMakefile,
		          "\t-@${rm} %s 2>$(NUL)\n",
		          temp_exp.c_str () );
	}
	else
	{
		fprintf ( fMakefile,
		          "\t%s %s -o %s %s %s %s\n",
		          linker.c_str (),
		          linkerParameters.c_str (),
		          target.c_str (),
		          objectsMacro.c_str (),
		          libsMacro.c_str (),
		          GetLinkerMacro ().c_str () );
	}

	GenerateBuildMapCode ();
	GenerateBuildNonSymbolStrippedCode ();
	GenerateRunRsymCode ();
	GenerateCleanObjectsAsYouGoCode ();
}

void
MingwModuleHandler::GeneratePhonyTarget() const
{
	string targetMacro ( GetTargetMacro ( module ) );
	fprintf ( fMakefile,
	          ".PHONY: %s\n\n",
	          targetMacro.c_str ());
	fprintf ( fMakefile, "%s: | %s\n",
	          targetMacro.c_str (),
	          GetDirectory ( GetTargetFilename ( module, NULL ) ).c_str () );
}

void
MingwModuleHandler::GenerateObjectFileTargets (
	const IfableData& data,
	const string& cc,
	const string& cppc,
	const string& cflagsMacro,
	const string& nasmflagsMacro,
	const string& windresflagsMacro,
	const string& widlflagsMacro )
{
	size_t i;
	
	const vector<File*>& files = data.files;
	for ( i = 0; i < files.size (); i++ )
	{
		GenerateCommands ( *files[i],
		                   cc,
		                   cppc,
		                   cflagsMacro,
		                   nasmflagsMacro,
		                   windresflagsMacro,
		                   widlflagsMacro );
		fprintf ( fMakefile,
		          "\n" );
	}

	const vector<If*>& ifs = data.ifs;
	for ( i = 0; i < ifs.size(); i++ )
	{
		GenerateObjectFileTargets ( ifs[i]->data,
		                            cc,
		                            cppc,
		                            cflagsMacro,
		                            nasmflagsMacro,
		                            windresflagsMacro,
		                            widlflagsMacro );
	}

	vector<File*> sourceFiles;
	GetModuleSpecificSourceFiles ( sourceFiles );
	for ( i = 0; i < sourceFiles.size (); i++ )
	{
		GenerateCommands ( *sourceFiles[i],
		                   cc,
		                   cppc,
		                   cflagsMacro,
		                   nasmflagsMacro,
		                   windresflagsMacro,
		                   widlflagsMacro );
	}
	CleanupFileVector ( sourceFiles );
}

void
MingwModuleHandler::GenerateObjectFileTargets (
	const string& cc,
	const string& cppc,
	const string& cflagsMacro,
	const string& nasmflagsMacro,
	const string& windresflagsMacro,
	const string& widlflagsMacro )
{
	if ( module.pch )
	{
		const string& pch_file = module.pch->file.name;
		string gch_file = pch_file + ".gch";
		CLEAN_FILE(gch_file);
		if ( use_pch )
		{
			fprintf (
				fMakefile,
				"%s: %s\n",
				gch_file.c_str(),
				pch_file.c_str() );
			fprintf ( fMakefile, "\t$(ECHO_PCH)\n" );
			fprintf (
				fMakefile,
				"\t%s -o %s %s -g %s\n\n",
				( module.cplusplus ? cppc.c_str() : cc.c_str() ),
				gch_file.c_str(),
				cflagsMacro.c_str(),
				pch_file.c_str() );
		}
	}

	GenerateObjectFileTargets ( module.non_if_data,
	                            cc,
	                            cppc,
	                            cflagsMacro,
	                            nasmflagsMacro,
	                            windresflagsMacro,
	                            widlflagsMacro );
	fprintf ( fMakefile, "\n" );
}

string
MingwModuleHandler::GenerateArchiveTarget ( const string& ar,
                                            const string& objs_macro ) const
{
	string archiveFilename ( GetModuleArchiveFilename () );
	
	fprintf ( fMakefile,
	          "%s: %s | %s\n",
	          archiveFilename.c_str (),
	          objs_macro.c_str (),
	          GetDirectory(archiveFilename).c_str() );

	fprintf ( fMakefile, "\t$(ECHO_AR)\n" );

	fprintf ( fMakefile,
	          "\t%s -rc $@ %s\n",
	          ar.c_str (),
	          objs_macro.c_str ());

	GenerateCleanObjectsAsYouGoCode ();

	fprintf ( fMakefile, "\n" );

	return archiveFilename;
}

string
MingwModuleHandler::GetCFlagsMacro () const
{
	return ssprintf ( "$(%s_CFLAGS)",
	                  module.name.c_str () );
}

/*static*/ string
MingwModuleHandler::GetObjectsMacro ( const Module& module )
{
	return ssprintf ( "$(%s_OBJS)",
	                  module.name.c_str () );
}

string
MingwModuleHandler::GetLinkingDependenciesMacro () const
{
	return ssprintf ( "$(%s_LINKDEPS)", module.name.c_str () );
}

string
MingwModuleHandler::GetLibsMacro () const
{
	return ssprintf ( "$(%s_LIBS)", module.name.c_str () );
}

string
MingwModuleHandler::GetLinkerMacro () const
{
	return ssprintf ( "$(%s_LFLAGS)",
	                  module.name.c_str () );
}

string
MingwModuleHandler::GetModuleTargets ( const Module& module )
{
	if ( ReferenceObjects ( module ) )
		return GetObjectsMacro ( module );
	else
		return GetTargetFilename ( module, NULL );
}

void
MingwModuleHandler::GenerateObjectMacro ()
{
	objectsMacro = ssprintf ("%s_OBJS", module.name.c_str ());

	GenerateObjectMacros (
		"=",
		module.non_if_data,
		&module.linkerFlags );

	// future references to the macro will be to get its values
	objectsMacro = ssprintf ("$(%s)", objectsMacro.c_str ());
}

void
MingwModuleHandler::GenerateTargetMacro ()
{
	fprintf ( fMakefile,
		"%s := %s\n",
		GetTargetMacro ( module, false ).c_str (),
		GetModuleTargets ( module ).c_str () );
}

void
MingwModuleHandler::GetRpcHeaderDependencies (
	string_list& dependencies ) const
{
	for ( size_t i = 0; i < module.non_if_data.libraries.size (); i++ )
	{
		Library& library = *module.non_if_data.libraries[i];
		if ( library.imported_module->type == RpcServer ||
		     library.imported_module->type == RpcClient )
		{

			for ( size_t j = 0; j < library.imported_module->non_if_data.files.size (); j++ )
			{
				File& file = *library.imported_module->non_if_data.files[j];
				string extension = GetExtension ( file.name );
				if ( extension == ".idl" || extension == ".IDL" )
				{
					string basename = GetBasename ( file.name );
					if ( library.imported_module->type == RpcServer )
						dependencies.push_back ( GetRpcServerHeaderFilename ( basename ) );
					if ( library.imported_module->type == RpcClient )
						dependencies.push_back ( GetRpcClientHeaderFilename ( basename ) );
				}
			}
		}
	}
}

void
MingwModuleHandler::GenerateOtherMacros ()
{
	cflagsMacro = ssprintf ("%s_CFLAGS", module.name.c_str ());
	nasmflagsMacro = ssprintf ("%s_NASMFLAGS", module.name.c_str ());
	windresflagsMacro = ssprintf ("%s_RCFLAGS", module.name.c_str ());
	widlflagsMacro = ssprintf ("%s_WIDLFLAGS", module.name.c_str ());
	linkerflagsMacro = ssprintf ("%s_LFLAGS", module.name.c_str ());
	libsMacro = ssprintf("%s_LIBS", module.name.c_str ());
	linkDepsMacro = ssprintf ("%s_LINKDEPS", module.name.c_str ());

	GenerateMacros (
		"=",
		module.non_if_data,
		&module.linkerFlags );

	string_list s;
	if ( module.importLibrary )
	{
		const vector<File*>& files = module.non_if_data.files;
		for ( size_t i = 0; i < files.size (); i++ )
		{
			File& file = *files[i];
			string extension = GetExtension ( file.name );
			if ( extension == ".spec" || extension == ".SPEC" )
				GetSpecObjectDependencies ( s, file.name );
		}
	}
	GetRpcHeaderDependencies ( s );
	if ( s.size () > 0 )
	{
		fprintf (
			fMakefile,
			"%s +=",
			linkDepsMacro.c_str() );
		for ( size_t i = 0; i < s.size(); i++ )
			fprintf ( fMakefile,
			          " %s",
			          s[i].c_str () );
		fprintf ( fMakefile, "\n" );
	}

	string globalCflags = "-g";
	if ( backend->usePipe )
		globalCflags += " -pipe";
	if ( !module.enableWarnings )
		globalCflags += " -Werror";
	
	fprintf (
		fMakefile,
		"%s += $(PROJECT_CFLAGS) %s\n",
		cflagsMacro.c_str (),
		globalCflags.c_str () );

	fprintf (
		fMakefile,
		"%s += $(PROJECT_RCFLAGS)\n",
		windresflagsMacro.c_str () );

	fprintf (
		fMakefile,
		"%s += $(PROJECT_WIDLFLAGS)\n",
		widlflagsMacro.c_str () );

	fprintf (
		fMakefile,
		"%s_LFLAGS += $(PROJECT_LFLAGS) -g\n",
		module.name.c_str () );

	fprintf (
		fMakefile,
		"%s += $(%s)\n",
		linkDepsMacro.c_str (),
		libsMacro.c_str () );

	string cflags = TypeSpecificCFlags();
	if ( cflags.size() > 0 )
	{
		fprintf ( fMakefile,
		          "%s += %s\n\n",
		          cflagsMacro.c_str (),
		          cflags.c_str () );
	}

	string nasmflags = TypeSpecificNasmFlags();
	if ( nasmflags.size () > 0 )
	{
		fprintf ( fMakefile,
		          "%s += %s\n\n",
		          nasmflagsMacro.c_str (),
		          nasmflags.c_str () );
	}

	string linkerflags = TypeSpecificLinkerFlags();
	if ( linkerflags.size() > 0 )
	{
		fprintf ( fMakefile,
		          "%s += %s\n\n",
		          linkerflagsMacro.c_str (),
		          linkerflags.c_str () );
	}

	fprintf ( fMakefile, "\n\n" );

	// future references to the macros will be to get their values
	cflagsMacro = ssprintf ("$(%s)", cflagsMacro.c_str ());
	nasmflagsMacro = ssprintf ("$(%s)", nasmflagsMacro.c_str ());
	widlflagsMacro = ssprintf ("$(%s)", widlflagsMacro.c_str ());
}

void
MingwModuleHandler::GenerateRules ()
{
	string cc = ( module.host == HostTrue ? "${host_gcc}" : "${gcc}" );
	string cppc = ( module.host == HostTrue ? "${host_gpp}" : "${gpp}" );
	string ar = ( module.host == HostTrue ? "${host_ar}" : "${ar}" );

	if ( module.name != "zlib" ) /* Avoid make warning */
	{
		string proxyMakefile = PassThruCacheDirectory (
			NormalizeFilename ( module.GetBasePath () + SSEP + "makefile" ),
			backend->outputDirectory );
		CLEAN_FILE ( proxyMakefile );
	}

	string targetMacro = GetTargetMacro ( module );
	CLEAN_FILE ( targetMacro );

	// generate phony target for module name
	fprintf ( fMakefile, ".PHONY: %s\n",
		module.name.c_str () );
	string dependencies = GetTargetMacro ( module );
	if ( module.type == Test )
		dependencies += " $(REGTESTS_RUN_TARGET)";
	fprintf ( fMakefile, "%s: %s\n\n",
		module.name.c_str (),
		dependencies.c_str () );
	if ( module.type == Test )
	{
		fprintf ( fMakefile,
		          "\t@%s\n",
		          targetMacro.c_str ());
	}

	if ( !ReferenceObjects ( module ) )
	{
		string ar_target ( GenerateArchiveTarget ( ar, objectsMacro ) );
		if ( targetMacro != ar_target )
			CLEAN_FILE ( ar_target );
	}

	GenerateObjectFileTargets ( cc,
	                            cppc,
	                            cflagsMacro,
	                            nasmflagsMacro,
	                            windresflagsMacro,
	                            widlflagsMacro );
}

void
MingwModuleHandler::GetInvocationDependencies (
	const Module& module,
	string_list& dependencies )
{
	for ( size_t i = 0; i < module.invocations.size (); i++ )
	{
		Invoke& invoke = *module.invocations[i];
		if ( invoke.invokeModule == &module )
			/* Protect against circular dependencies */
			continue;
		invoke.GetTargets ( dependencies );
	}
}

void
MingwModuleHandler::GenerateInvocations () const
{
	if ( module.invocations.size () == 0 )
		return;
	
	size_t iend = module.invocations.size ();
	for ( size_t i = 0; i < iend; i++ )
	{
		const Invoke& invoke = *module.invocations[i];

		if ( invoke.invokeModule->type != BuildTool )
		{
			throw InvalidBuildFileException ( module.node.location,
			                                  "Only modules of type buildtool can be invoked." );
		}

		string invokeTarget = module.GetInvocationTarget ( i );
		string_list invoke_targets;
		assert ( invoke_targets.size() );
		invoke.GetTargets ( invoke_targets );
		fprintf ( fMakefile,
		          ".PHONY: %s\n\n",
		          invokeTarget.c_str () );
		fprintf ( fMakefile,
		          "%s:",
		          invokeTarget.c_str () );
		size_t j, jend = invoke_targets.size();
		for ( j = 0; j < jend; j++ )
		{
			fprintf ( fMakefile,
			          " %s",
			          invoke_targets[i].c_str () );
		}
		fprintf ( fMakefile, "\n\n%s", invoke_targets[0].c_str () );
		for ( j = 1; j < jend; j++ )
			fprintf ( fMakefile,
			          " %s",
			          invoke_targets[i].c_str () );
		fprintf ( fMakefile,
		          ": %s\n",
		          NormalizeFilename ( invoke.invokeModule->GetPath () ).c_str () );
		fprintf ( fMakefile, "\t$(ECHO_INVOKE)\n" );
		fprintf ( fMakefile,
		          "\t%s %s\n\n",
		          NormalizeFilename ( invoke.invokeModule->GetPath () ).c_str (),
		          invoke.GetParameters ().c_str () );
	}
}

string
MingwModuleHandler::GetPreconditionDependenciesName () const
{
	return module.name + "_precondition";
}

void
MingwModuleHandler::GetDefaultDependencies (
	string_list& dependencies ) const
{
	/* Avoid circular dependency */
	if ( module.type != BuildTool
		&& module.name != "zlib"
		&& module.name != "hostzlib" )

		dependencies.push_back ( "$(INIT)" );
}

void
MingwModuleHandler::GeneratePreconditionDependencies ()
{
	string preconditionDependenciesName = GetPreconditionDependenciesName ();
	string_list sourceFilenames;
	GetSourceFilenamesWithoutGeneratedFiles ( sourceFilenames );
	string_list dependencies;
	GetDefaultDependencies ( dependencies );
	GetModuleDependencies ( dependencies );

	GetInvocationDependencies ( module, dependencies );
	
	if ( dependencies.size() )
	{
		fprintf ( fMakefile,
		          "%s =",
		          preconditionDependenciesName.c_str () );
		for ( size_t i = 0; i < dependencies.size(); i++ )
			fprintf ( fMakefile,
			          " %s",
			          dependencies[i].c_str () );
		fprintf ( fMakefile, "\n\n" );
	}

	for ( size_t i = 0; i < sourceFilenames.size(); i++ )
	{
		fprintf ( fMakefile,
		          "%s: ${%s}\n",
		          sourceFilenames[i].c_str(),
		          preconditionDependenciesName.c_str ());
	}
	fprintf ( fMakefile, "\n" );
}

bool
MingwModuleHandler::IsWineModule () const
{
	if ( module.importLibrary == NULL)
		return false;

	size_t index = module.importLibrary->definition.rfind ( ".spec.def" );
	return ( index != string::npos );
}

string
MingwModuleHandler::GetDefinitionFilename () const
{
	if ( module.importLibrary != NULL )
	{
		string defFilename = module.GetBasePath () + SSEP + module.importLibrary->definition;
		if ( IsWineModule () )
			return PassThruCacheDirectory ( NormalizeFilename ( defFilename ),
			                                backend->intermediateDirectory );
		else
			return defFilename;
	}
	else
		return "tools" SSEP "rbuild" SSEP "empty.def";
}

void
MingwModuleHandler::GenerateImportLibraryTargetIfNeeded ()
{
	if ( module.importLibrary != NULL )
	{
		string library_target (
			GetImportLibraryFilename ( module, &clean_files ) );
		string defFilename = GetDefinitionFilename ();
	
		string_list deps;
		GetDefinitionDependencies ( deps );

		fprintf ( fMakefile, "# IMPORT LIBRARY RULE:\n" );

		fprintf ( fMakefile, "%s: %s",
		          library_target.c_str (),
		          defFilename.c_str () );

		size_t i, iend = deps.size();
		for ( i = 0; i < iend; i++ )
			fprintf ( fMakefile, " %s",
			          deps[i].c_str () );

		fprintf ( fMakefile, " | %s\n",
		          GetDirectory ( GetImportLibraryFilename ( module, NULL ) ).c_str () );

		fprintf ( fMakefile, "\t$(ECHO_DLLTOOL)\n" );

		string killAt = module.mangledSymbols ? "" : "--kill-at";
		fprintf ( fMakefile,
		          "\t${dlltool} --dllname %s --def %s --output-lib %s %s\n\n",
		          module.GetTargetName ().c_str (),
		          defFilename.c_str (),
		          library_target.c_str (),
		          killAt.c_str () );
	}
}

void
MingwModuleHandler::GetSpecObjectDependencies (
	string_list& dependencies,
	const string& filename ) const
{
	string basename = GetBasename ( filename );
	string defDependency = PassThruCacheDirectory (
		NormalizeFilename ( basename + ".spec.def" ),
		backend->intermediateDirectory );
	dependencies.push_back ( defDependency );
	string stubsDependency = PassThruCacheDirectory (
		NormalizeFilename ( basename + ".stubs.c" ),
		backend->intermediateDirectory );
	dependencies.push_back ( stubsDependency );
}

void
MingwModuleHandler::GetWidlObjectDependencies (
	string_list& dependencies,
	const string& filename ) const
{
	string basename = GetBasename ( filename );
	string serverDependency = PassThruCacheDirectory (
		NormalizeFilename ( basename + "_s.c" ),
		backend->intermediateDirectory );
	dependencies.push_back ( serverDependency );
}

void
MingwModuleHandler::GetDefinitionDependencies (
	string_list& dependencies ) const
{
	string dkNkmLibNoFixup = "dk/nkm/lib";
	const vector<File*>& files = module.non_if_data.files;
	for ( size_t i = 0; i < files.size (); i++ )
	{
		File& file = *files[i];
		string extension = GetExtension ( file.name );
		if ( extension == ".spec" || extension == ".SPEC" )
		{
			GetSpecObjectDependencies ( dependencies, file.name );
		}
		if ( extension == ".idl" || extension == ".IDL" )
		{
			GetWidlObjectDependencies ( dependencies, file.name );
		}
	}
}


MingwBuildToolModuleHandler::MingwBuildToolModuleHandler ( const Module& module_ )
	: MingwModuleHandler ( module_ )
{
}

void
MingwBuildToolModuleHandler::Process ()
{
	GenerateBuildToolModuleTarget ();
}

void
MingwBuildToolModuleHandler::GenerateBuildToolModuleTarget ()
{
	string targetMacro ( GetTargetMacro (module) );
	string objectsMacro = GetObjectsMacro ( module );
	string linkDepsMacro = GetLinkingDependenciesMacro ();
	string libsMacro = GetLibsMacro ();

	GenerateRules ();

	string linker;
	if ( module.cplusplus )
		linker = "${host_gpp}";
	else
		linker = "${host_gcc}";
	
	fprintf ( fMakefile, "%s: %s %s | %s\n",
	          targetMacro.c_str (),
	          objectsMacro.c_str (),
	          linkDepsMacro.c_str (),
	          GetDirectory(GetTargetFilename(module,NULL)).c_str () );
	fprintf ( fMakefile, "\t$(ECHO_LD)\n" );
	fprintf ( fMakefile,
	          "\t%s %s -o $@ %s %s\n\n",
	          linker.c_str (),
	          GetLinkerMacro ().c_str (),
	          objectsMacro.c_str (),
	          libsMacro.c_str () );
}


MingwKernelModuleHandler::MingwKernelModuleHandler (
	const Module& module_ )

	: MingwModuleHandler ( module_ )
{
}

void
MingwKernelModuleHandler::Process ()
{
	GenerateKernelModuleTarget ();
}

void
MingwKernelModuleHandler::GenerateKernelModuleTarget ()
{
	string targetMacro ( GetTargetMacro ( module ) );
	string workingDirectory = GetWorkingDirectory ( );
	string objectsMacro = GetObjectsMacro ( module );
	string linkDepsMacro = GetLinkingDependenciesMacro ();
	string libsMacro = GetLibsMacro ();

	GenerateImportLibraryTargetIfNeeded ();

	if ( module.non_if_data.files.size () > 0 )
	{
		GenerateRules ();

		string dependencies = linkDepsMacro + " " + objectsMacro;

		string linkerParameters = ssprintf ( "-Wl,-T,%s" SSEP "ntoskrnl.lnk -Wl,--subsystem,native -Wl,--entry,%s -Wl,--image-base,%s -Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000 -nostartfiles -mdll --dll",
		                                     module.GetBasePath ().c_str (),
		                                     module.entrypoint.c_str (),
		                                     module.baseaddress.c_str () );
		GenerateLinkerCommand ( dependencies,
		                        "${gcc}",
		                        linkerParameters,
		                        objectsMacro,
		                        libsMacro );
	}
	else
	{
		GeneratePhonyTarget();
	}
}


MingwStaticLibraryModuleHandler::MingwStaticLibraryModuleHandler (
	const Module& module_ )

	: MingwModuleHandler ( module_ )
{
}

void
MingwStaticLibraryModuleHandler::Process ()
{
	GenerateStaticLibraryModuleTarget ();
}

void
MingwStaticLibraryModuleHandler::GenerateStaticLibraryModuleTarget ()
{
	GenerateRules ();
}


MingwObjectLibraryModuleHandler::MingwObjectLibraryModuleHandler (
	const Module& module_ )

	: MingwModuleHandler ( module_ )
{
}

void
MingwObjectLibraryModuleHandler::Process ()
{
	GenerateObjectLibraryModuleTarget ();
}

void
MingwObjectLibraryModuleHandler::GenerateObjectLibraryModuleTarget ()
{
	GenerateRules ();
}


MingwKernelModeDLLModuleHandler::MingwKernelModeDLLModuleHandler (
	const Module& module_ )

	: MingwModuleHandler ( module_ )
{
}

void
MingwKernelModeDLLModuleHandler::Process ()
{
	GenerateKernelModeDLLModuleTarget ();
}

void
MingwKernelModeDLLModuleHandler::GenerateKernelModeDLLModuleTarget ()
{
	string targetMacro ( GetTargetMacro ( module ) );
	string workingDirectory = GetWorkingDirectory ( );
	string objectsMacro = GetObjectsMacro ( module );
	string linkDepsMacro = GetLinkingDependenciesMacro ();
	string libsMacro = GetLibsMacro ();

	GenerateImportLibraryTargetIfNeeded ();

	if ( module.non_if_data.files.size () > 0 )
	{
		GenerateRules ();

		string dependencies = linkDepsMacro + " " + objectsMacro;

		string linkerParameters = ssprintf ( "-Wl,--subsystem,native -Wl,--entry,%s -Wl,--image-base,%s -Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000 -nostartfiles -mdll --dll",
		                                     module.entrypoint.c_str (),
		                                     module.baseaddress.c_str () );
		GenerateLinkerCommand ( dependencies,
		                        "${gcc}",
		                        linkerParameters,
		                        objectsMacro,
		                        libsMacro );
	}
	else
	{
		GeneratePhonyTarget();
	}
}


MingwKernelModeDriverModuleHandler::MingwKernelModeDriverModuleHandler (
	const Module& module_ )

	: MingwModuleHandler ( module_ )
{
}

void
MingwKernelModeDriverModuleHandler::Process ()
{
	GenerateKernelModeDriverModuleTarget ();
}


void
MingwKernelModeDriverModuleHandler::GenerateKernelModeDriverModuleTarget ()
{
	string targetMacro ( GetTargetMacro (module) );
	string workingDirectory = GetWorkingDirectory ();
	string objectsMacro = GetObjectsMacro ( module );
	string linkDepsMacro = GetLinkingDependenciesMacro ();
	string libsMacro = GetLibsMacro ();

	GenerateImportLibraryTargetIfNeeded ();

	if ( module.non_if_data.files.size () > 0 )
	{
		GenerateRules ();

		string dependencies = linkDepsMacro + " " + objectsMacro;

		string linkerParameters = ssprintf ( "-Wl,--subsystem,native -Wl,--entry,%s -Wl,--image-base,%s -Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000 -nostartfiles -mdll --dll",
		                                     module.entrypoint.c_str (),
		                                     module.baseaddress.c_str () );
		GenerateLinkerCommand ( dependencies,
		                        "${gcc}",
		                        linkerParameters,
		                        objectsMacro,
		                        libsMacro );
	}
	else
	{
		GeneratePhonyTarget();
	}
}


MingwNativeDLLModuleHandler::MingwNativeDLLModuleHandler (
	const Module& module_ )

	: MingwModuleHandler ( module_ )
{
}

void
MingwNativeDLLModuleHandler::Process ()
{
	GenerateNativeDLLModuleTarget ();
}

void
MingwNativeDLLModuleHandler::GenerateNativeDLLModuleTarget ()
{
	string targetMacro ( GetTargetMacro (module) );
	string workingDirectory = GetWorkingDirectory ( );
	string objectsMacro = GetObjectsMacro ( module );
	string linkDepsMacro = GetLinkingDependenciesMacro ();
	string libsMacro = GetLibsMacro ();
	
	GenerateImportLibraryTargetIfNeeded ();

	if ( module.non_if_data.files.size () > 0 )
	{
		GenerateRules ();

		string dependencies = linkDepsMacro + " " + objectsMacro;

		string linkerParameters = ssprintf ( "-Wl,--subsystem,native -Wl,--entry,%s -Wl,--image-base,%s -Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000 -nostartfiles -nostdlib -mdll --dll",
		                                     module.entrypoint.c_str (),
		                                     module.baseaddress.c_str () );
		GenerateLinkerCommand ( dependencies,
		                        "${gcc}",
		                        linkerParameters,
		                        objectsMacro,
		                        libsMacro );
	}
	else
	{
		GeneratePhonyTarget();
	}
}


MingwNativeCUIModuleHandler::MingwNativeCUIModuleHandler (
	const Module& module_ )

	: MingwModuleHandler ( module_ )
{
}

void
MingwNativeCUIModuleHandler::Process ()
{
	GenerateNativeCUIModuleTarget ();
}

void
MingwNativeCUIModuleHandler::GenerateNativeCUIModuleTarget ()
{
	string targetMacro ( GetTargetMacro (module) );
	string workingDirectory = GetWorkingDirectory ( );
	string objectsMacro = GetObjectsMacro ( module );
	string linkDepsMacro = GetLinkingDependenciesMacro ();
	string libsMacro = GetLibsMacro ();
	
	GenerateImportLibraryTargetIfNeeded ();

	if ( module.non_if_data.files.size () > 0 )
	{
		GenerateRules ();

		string dependencies = linkDepsMacro + " " + objectsMacro;

		string linkerParameters = ssprintf ( "-Wl,--subsystem,native -Wl,--entry,%s -Wl,--image-base,%s -Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000 -nostartfiles -nostdlib",
		                                     module.entrypoint.c_str (),
		                                     module.baseaddress.c_str () );
		GenerateLinkerCommand ( dependencies,
		                        "${gcc}",
		                        linkerParameters,
		                        objectsMacro,
		                        libsMacro );
	}
	else
	{
		GeneratePhonyTarget();
	}
}


MingwWin32DLLModuleHandler::MingwWin32DLLModuleHandler (
	const Module& module_ )

	: MingwModuleHandler ( module_ )
{
}

void
MingwWin32DLLModuleHandler::Process ()
{
	GenerateWin32DLLModuleTarget ();
}

void
MingwWin32DLLModuleHandler::GenerateWin32DLLModuleTarget ()
{
	string targetMacro ( GetTargetMacro (module) );
	string workingDirectory = GetWorkingDirectory ( );
	string objectsMacro = GetObjectsMacro ( module );
	string linkDepsMacro = GetLinkingDependenciesMacro ();
	string libsMacro = GetLibsMacro ();

	GenerateImportLibraryTargetIfNeeded ();

	if ( module.non_if_data.files.size () > 0 )
	{
		GenerateRules ();

		string dependencies = linkDepsMacro + " " + objectsMacro;

		string linker;
		if ( module.cplusplus )
			linker = "${gpp}";
		else
			linker = "${gcc}";

		string linkerParameters = ssprintf ( "-Wl,--subsystem,console -Wl,--entry,%s -Wl,--image-base,%s -Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000 -mdll --dll",
		                                     module.entrypoint.c_str (),
		                                     module.baseaddress.c_str () );
		GenerateLinkerCommand ( dependencies,
		                        linker,
		                        linkerParameters,
		                        objectsMacro,
		                        libsMacro );
	}
	else
	{
		GeneratePhonyTarget();
	}
}


MingwWin32CUIModuleHandler::MingwWin32CUIModuleHandler (
	const Module& module_ )

	: MingwModuleHandler ( module_ )
{
}

void
MingwWin32CUIModuleHandler::Process ()
{
	GenerateWin32CUIModuleTarget ();
}

void
MingwWin32CUIModuleHandler::GenerateWin32CUIModuleTarget ()
{
	string targetMacro ( GetTargetMacro (module) );
	string workingDirectory = GetWorkingDirectory ( );
	string objectsMacro = GetObjectsMacro ( module );
	string linkDepsMacro = GetLinkingDependenciesMacro ();
	string libsMacro = GetLibsMacro ();

	GenerateImportLibraryTargetIfNeeded ();

	if ( module.non_if_data.files.size () > 0 )
	{
		GenerateRules ();

		string dependencies = linkDepsMacro + " " + objectsMacro;

		string linker;
		if ( module.cplusplus )
			linker = "${gpp}";
		else
			linker = "${gcc}";

		string linkerParameters = ssprintf ( "-Wl,--subsystem,console -Wl,--entry,%s -Wl,--image-base,%s -Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000",
		                                     module.entrypoint.c_str (),
		                                     module.baseaddress.c_str () );
		GenerateLinkerCommand ( dependencies,
		                        linker,
		                        linkerParameters,
		                        objectsMacro,
		                        libsMacro );
	}
	else
	{
		GeneratePhonyTarget();
	}
}


MingwWin32GUIModuleHandler::MingwWin32GUIModuleHandler (
	const Module& module_ )

	: MingwModuleHandler ( module_ )
{
}

void
MingwWin32GUIModuleHandler::Process ()
{
	GenerateWin32GUIModuleTarget ();
}

void
MingwWin32GUIModuleHandler::GenerateWin32GUIModuleTarget ()
{
	string targetMacro ( GetTargetMacro (module) );
	string workingDirectory = GetWorkingDirectory ( );
	string objectsMacro = GetObjectsMacro ( module );
	string linkDepsMacro = GetLinkingDependenciesMacro ();
	string libsMacro = GetLibsMacro ();

	GenerateImportLibraryTargetIfNeeded ();

	if ( module.non_if_data.files.size () > 0 )
	{
		GenerateRules ();

		string dependencies = linkDepsMacro + " " + objectsMacro;

		string linker;
		if ( module.cplusplus )
			linker = "${gpp}";
		else
			linker = "${gcc}";

		string linkerParameters = ssprintf ( "-Wl,--subsystem,windows -Wl,--entry,%s -Wl,--image-base,%s -Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000",
		                                     module.entrypoint.c_str (),
		                                     module.baseaddress.c_str () );
		GenerateLinkerCommand ( dependencies,
		                        linker,
		                        linkerParameters,
		                        objectsMacro,
		                        libsMacro );
	}
	else
	{
		GeneratePhonyTarget();
	}
}


MingwBootLoaderModuleHandler::MingwBootLoaderModuleHandler (
	const Module& module_ )

	: MingwModuleHandler ( module_ )
{
}

void
MingwBootLoaderModuleHandler::Process ()
{
	GenerateBootLoaderModuleTarget ();
}

void
MingwBootLoaderModuleHandler::GenerateBootLoaderModuleTarget ()
{
	string targetName ( module.GetTargetName () );
	string targetMacro ( GetTargetMacro (module) );
	string workingDirectory = GetWorkingDirectory ();
	string junk_tmp = ros_temp + module.name + ".junk.tmp";
	CLEAN_FILE ( junk_tmp );
	string objectsMacro = GetObjectsMacro ( module );
	string linkDepsMacro = GetLinkingDependenciesMacro ();
	string libsMacro = GetLibsMacro ();

	GenerateRules ();

	fprintf ( fMakefile, "%s: %s %s | %s\n",
	          targetMacro.c_str (),
	          objectsMacro.c_str (),
	          linkDepsMacro.c_str (),
	          GetDirectory(GetTargetFilename(module,NULL)).c_str () );

	fprintf ( fMakefile, "\t$(ECHO_LD)\n" );

	fprintf ( fMakefile,
	          "\t${ld} %s -N -Ttext=0x8000 -o %s %s %s\n",
	          GetLinkerMacro ().c_str (),
	          junk_tmp.c_str (),
	          objectsMacro.c_str (),
	          linkDepsMacro.c_str () );
	fprintf ( fMakefile,
	          "\t${objcopy} -O binary %s $@\n",
	          junk_tmp.c_str () );
	fprintf ( fMakefile,
	          "\t-@${rm} %s 2>$(NUL)\n",
	          junk_tmp.c_str () );
}


MingwBootSectorModuleHandler::MingwBootSectorModuleHandler (
	const Module& module_ )

	: MingwModuleHandler ( module_ )
{
}

void
MingwBootSectorModuleHandler::Process ()
{
	GenerateBootSectorModuleTarget ();
}

void
MingwBootSectorModuleHandler::GenerateBootSectorModuleTarget ()
{
	string objectsMacro = GetObjectsMacro ( module );

	GenerateRules ();

	fprintf ( fMakefile, ".PHONY: %s\n\n",
	          module.name.c_str ());
	fprintf ( fMakefile,
	          "%s: %s\n",
	          module.name.c_str (),
	          objectsMacro.c_str () );
}


MingwIsoModuleHandler::MingwIsoModuleHandler (
	const Module& module_ )

	: MingwModuleHandler ( module_ )
{
}

void
MingwIsoModuleHandler::Process ()
{
	GenerateIsoModuleTarget ();
}

void
MingwIsoModuleHandler::OutputBootstrapfileCopyCommands (
	const string& bootcdDirectory )
{
	for ( size_t i = 0; i < module.project.modules.size (); i++ )
	{
		const Module& m = *module.project.modules[i];
		if ( !m.enabled )
			continue;
		if ( m.bootstrap != NULL )
		{
			string sourceFilename = PassThruCacheDirectory (
				NormalizeFilename ( m.GetPath () ),
				backend->outputDirectory );
			string targetFilenameNoFixup ( bootcdDirectory + SSEP + m.bootstrap->base + SSEP + m.bootstrap->nameoncd );
			string targetFilename = MingwModuleHandler::PassThruCacheDirectory (
				NormalizeFilename ( targetFilenameNoFixup ),
				backend->outputDirectory );
			fprintf ( fMakefile,
			          "\t$(ECHO_CP)\n" );
			fprintf ( fMakefile,
			          "\t${cp} %s %s 1>$(NUL)\n",
			          sourceFilename.c_str (),
			          targetFilename.c_str () );
		}
	}
}

void
MingwIsoModuleHandler::OutputCdfileCopyCommands (
	const string& bootcdDirectory )
{
	for ( size_t i = 0; i < module.project.cdfiles.size (); i++ )
	{
		const CDFile& cdfile = *module.project.cdfiles[i];
		string targetFilenameNoFixup = bootcdDirectory + SSEP + cdfile.base + SSEP + cdfile.nameoncd;
		string targetFilename = MingwModuleHandler::PassThruCacheDirectory (
			NormalizeFilename ( targetFilenameNoFixup ),
			backend->outputDirectory );
		fprintf ( fMakefile,
		          "\t$(ECHO_CP)\n" );
		fprintf ( fMakefile,
		          "\t${cp} %s %s 1>$(NUL)\n",
		          cdfile.GetPath ().c_str (),
		          targetFilename.c_str () );
	}
}

string
MingwIsoModuleHandler::GetBootstrapCdDirectories ( const string& bootcdDirectory )
{
	string directories;
	for ( size_t i = 0; i < module.project.modules.size (); i++ )
	{
		const Module& m = *module.project.modules[i];
		if ( !m.enabled )
			continue;
		if ( m.bootstrap != NULL )
		{
			string targetDirectory ( bootcdDirectory + SSEP + m.bootstrap->base );
			if ( directories.size () > 0 )
				directories += " ";
			directories += PassThruCacheDirectory (
				NormalizeFilename ( targetDirectory ),
				backend->outputDirectory );
		}
	}
	return directories;
}

string
MingwIsoModuleHandler::GetNonModuleCdDirectories ( const string& bootcdDirectory )
{
	string directories;
	for ( size_t i = 0; i < module.project.cdfiles.size (); i++ )
	{
		const CDFile& cdfile = *module.project.cdfiles[i];
		string targetDirectory ( bootcdDirectory + SSEP + cdfile.base );
		if ( directories.size () > 0 )
			directories += " ";
		directories += PassThruCacheDirectory (
			NormalizeFilename ( targetDirectory ),
			backend->outputDirectory );
	}
	return directories;
}

string
MingwIsoModuleHandler::GetCdDirectories ( const string& bootcdDirectory )
{
	string directories = GetBootstrapCdDirectories ( bootcdDirectory );
	directories += " " + GetNonModuleCdDirectories ( bootcdDirectory );
	return directories;
}

void
MingwIsoModuleHandler::GetBootstrapCdFiles (
	vector<string>& out ) const
{
	for ( size_t i = 0; i < module.project.modules.size (); i++ )
	{
		const Module& m = *module.project.modules[i];
		if ( !m.enabled )
			continue;
		if ( m.bootstrap != NULL )
		{
			string filename = PassThruCacheDirectory (
				NormalizeFilename ( m.GetPath () ),
				backend->outputDirectory );
			out.push_back ( filename );
		}
	}
}

void
MingwIsoModuleHandler::GetNonModuleCdFiles (
	vector<string>& out ) const
{
	for ( size_t i = 0; i < module.project.cdfiles.size (); i++ )
	{
		const CDFile& cdfile = *module.project.cdfiles[i];
		out.push_back ( cdfile.GetPath () );
	}
}

void
MingwIsoModuleHandler::GetCdFiles (
	vector<string>& out ) const
{
	GetBootstrapCdFiles ( out );
	GetNonModuleCdFiles ( out );
}

void
MingwIsoModuleHandler::GenerateIsoModuleTarget ()
{
	string bootcdDirectory = "cd";
	string bootcd = PassThruCacheDirectory (
		NormalizeFilename ( bootcdDirectory + SSEP ),
		backend->outputDirectory );
	string isoboot = PassThruCacheDirectory (
		NormalizeFilename ( "boot" SSEP "freeldr" SSEP "bootsect" SSEP "isoboot.o" ),
		backend->outputDirectory );
	string bootcdReactosNoFixup = bootcdDirectory + SSEP "reactos";
	string bootcdReactos = PassThruCacheDirectory (
		NormalizeFilename ( bootcdReactosNoFixup + SSEP ),
		backend->outputDirectory );
	CLEAN_FILE ( bootcdReactos );
	string reactosInf = PassThruCacheDirectory (
		NormalizeFilename ( bootcdReactosNoFixup + SSEP "reactos.inf" ),
		backend->outputDirectory );
	string reactosDff = NormalizeFilename ( "bootdata" SSEP "packages" SSEP "reactos.dff" );
	string cdDirectories = GetCdDirectories ( bootcdDirectory );
	vector<string> vCdFiles;
	GetCdFiles ( vCdFiles );
	string cdFiles = v2s ( vCdFiles, 5 );

	fprintf ( fMakefile, ".PHONY: %s\n\n",
	          module.name.c_str ());
	fprintf ( fMakefile,
	          "%s: all %s %s %s %s $(CABMAN_TARGET) $(CDMAKE_TARGET)\n",
	          module.name.c_str (),
	          isoboot.c_str (),
	          bootcdReactos.c_str (),
	          cdDirectories.c_str (),
	          cdFiles.c_str () );
	fprintf ( fMakefile, "\t$(ECHO_CABMAN)\n" );
	fprintf ( fMakefile,
	          "\t$(Q)$(CABMAN_TARGET) -C %s -L %s -I -P $(OUTPUT)\n",
	          reactosDff.c_str (),
	          bootcdReactos.c_str () );
	fprintf ( fMakefile,
	          "\t$(Q)$(CABMAN_TARGET) -C %s -RC %s -L %s -N -P $(OUTPUT)\n",
	          reactosDff.c_str (),
	          reactosInf.c_str (),
	          bootcdReactos.c_str ());
	fprintf ( fMakefile,
	          "\t-@${rm} %s 2>$(NUL)\n",
	          reactosInf.c_str () );
	OutputBootstrapfileCopyCommands ( bootcdDirectory );
	OutputCdfileCopyCommands ( bootcdDirectory );
	fprintf ( fMakefile, "\t$(ECHO_CDMAKE)\n" );
	fprintf ( fMakefile,
	          "\t$(Q)$(CDMAKE_TARGET) -v -m -b %s %s REACTOS ReactOS.iso\n",
	          isoboot.c_str (),
	          bootcd.c_str () );
	fprintf ( fMakefile,
	          "\n" );
}


MingwLiveIsoModuleHandler::MingwLiveIsoModuleHandler (
	const Module& module_ )

	: MingwModuleHandler ( module_ )
{
}

void
MingwLiveIsoModuleHandler::Process ()
{
	GenerateLiveIsoModuleTarget ();
}

void
MingwLiveIsoModuleHandler::CreateDirectory ( const string& directory )
{
	string normalizedDirectory = MingwModuleHandler::PassThruCacheDirectory (
		NormalizeFilename ( directory ) + SSEP,
		backend->outputDirectory );
}

void
MingwLiveIsoModuleHandler::OutputCopyCommand ( const string& sourceFilename,
                                               const string& targetFilename,
                                               const string& targetDirectory )
{
	string normalizedTargetFilename = MingwModuleHandler::PassThruCacheDirectory (
		NormalizeFilename ( targetDirectory + SSEP + targetFilename ),
		backend->outputDirectory );
	fprintf ( fMakefile,
	          "\t$(ECHO_CP)\n" );
	fprintf ( fMakefile,
	          "\t${cp} %s %s 1>$(NUL)\n",
	          sourceFilename.c_str (),
	          normalizedTargetFilename.c_str () );
}

void
MingwLiveIsoModuleHandler::OutputModuleCopyCommands ( string& livecdDirectory,
                                                      string& reactosDirectory )
{
	for ( size_t i = 0; i < module.project.modules.size (); i++ )
	{
		const Module& m = *module.project.modules[i];
		if ( !m.enabled )
			continue;
		if ( m.installName.length () > 0 )
		{
			string sourceFilename = MingwModuleHandler::PassThruCacheDirectory (
				NormalizeFilename ( m.GetPath () ),
				backend->outputDirectory );
			OutputCopyCommand ( sourceFilename,
		                        m.installName,
		                        livecdDirectory + SSEP + reactosDirectory + SSEP + m.installBase );
		}
	}
}

void
MingwLiveIsoModuleHandler::OutputNonModuleCopyCommands ( string& livecdDirectory,
                                                         string& reactosDirectory )
{
	for ( size_t i = 0; i < module.project.installfiles.size (); i++ )
	{
		const InstallFile& installfile = *module.project.installfiles[i];
		OutputCopyCommand ( installfile.GetPath (),
	                        installfile.newname,
	                        livecdDirectory + SSEP + reactosDirectory + SSEP + installfile.base );
	}
}

void
MingwLiveIsoModuleHandler::OutputProfilesDirectoryCommands ( string& livecdDirectory )
{
	CreateDirectory ( livecdDirectory + SSEP "Profiles" );
	CreateDirectory ( livecdDirectory + SSEP "Profiles" SSEP "All Users") ;
	CreateDirectory ( livecdDirectory + SSEP "Profiles" SSEP "All Users" SSEP "Desktop" );
	CreateDirectory ( livecdDirectory + SSEP "Profiles" SSEP "Default User" );
	CreateDirectory ( livecdDirectory + SSEP "Profiles" SSEP "Default User" SSEP "Desktop" );
	CreateDirectory ( livecdDirectory + SSEP "Profiles" SSEP "Default User" SSEP "My Documents" );

	string livecdIni = "bootdata" SSEP "livecd.ini";
	OutputCopyCommand ( livecdIni,
                        "freeldr.ini",
                        livecdDirectory );
}

void
MingwLiveIsoModuleHandler::OutputLoaderCommands ( string& livecdDirectory )
{
	string freeldr = PassThruCacheDirectory (
		NormalizeFilename ( "boot" SSEP "freeldr" SSEP "freeldr" SSEP "freeldr.sys" ),
		backend->outputDirectory );
	CreateDirectory ( livecdDirectory + SSEP "loader" );
	OutputCopyCommand ( freeldr,
                        "setupldr.sys",
                        livecdDirectory + SSEP + "loader" );
}

void
MingwLiveIsoModuleHandler::OutputRegistryCommands ( string& livecdDirectory )
{
	string reactosSystem32ConfigDirectory = NormalizeFilename (
		MingwModuleHandler::PassThruCacheDirectory (
		livecdDirectory + SSEP "reactos" SSEP "system32" SSEP "config" SSEP,
		backend->outputDirectory ) );
	fprintf ( fMakefile,
	          "\t$(ECHO_MKHIVE)\n" );
	fprintf ( fMakefile,
	          "\t$(MKHIVE_TARGET) bootdata %s bootdata" SSEP "livecd.inf bootdata" SSEP "hiveinst.inf\n",
	          reactosSystem32ConfigDirectory.c_str () );
}

void
MingwLiveIsoModuleHandler::GenerateLiveIsoModuleTarget ()
{
	string livecdDirectory = "livecd";
	string livecd = PassThruCacheDirectory (
		NormalizeFilename ( livecdDirectory + SSEP ),
		backend->outputDirectory );
	string isoboot = PassThruCacheDirectory (
		NormalizeFilename ( "boot" SSEP "freeldr" SSEP "bootsect" SSEP "isoboot.o" ),
		backend->outputDirectory );
	string reactosDirectory = "reactos";
	string livecdReactosNoFixup = livecdDirectory + SSEP + reactosDirectory;
	string livecdReactos = NormalizeFilename ( PassThruCacheDirectory (
		NormalizeFilename ( livecdReactosNoFixup + SSEP ),
		backend->outputDirectory ) );
	CLEAN_FILE ( livecdReactos );

	fprintf ( fMakefile, ".PHONY: %s\n\n",
	          module.name.c_str ());
	fprintf ( fMakefile,
	          "%s: all %s %s $(MKHIVE_TARGET) $(CDMAKE_TARGET)\n",
	          module.name.c_str (),
	          isoboot.c_str (),
	          livecdReactos.c_str () );
	OutputModuleCopyCommands ( livecdDirectory,
	                           reactosDirectory );
	OutputNonModuleCopyCommands ( livecdDirectory,
	                              reactosDirectory );
	OutputProfilesDirectoryCommands ( livecdDirectory );
	OutputLoaderCommands ( livecdDirectory );
	OutputRegistryCommands ( livecdDirectory );
	fprintf ( fMakefile, "\t$(ECHO_CDMAKE)\n" );
	fprintf ( fMakefile,
	          "\t$(Q)$(CDMAKE_TARGET) -v -m -j -b %s %s REACTOS ReactOS-LiveCD.iso\n",
	          isoboot.c_str (),
	          livecd.c_str () );
	fprintf ( fMakefile,
	          "\n" );
}


MingwTestModuleHandler::MingwTestModuleHandler (
	const Module& module_ )

	: MingwModuleHandler ( module_ )
{
}

void
MingwTestModuleHandler::Process ()
{
	GenerateTestModuleTarget ();
}

void
MingwTestModuleHandler::GetModuleSpecificSourceFiles ( vector<File*>& sourceFiles )
{
	string basePath = "$(INTERMEDIATE)" SSEP + module.GetBasePath ();
	sourceFiles.push_back ( new File ( basePath + SSEP "_hooks.c", false, "", false ) );
	sourceFiles.push_back ( new File ( basePath + SSEP "_stubs.S", false, "", false ) );
	sourceFiles.push_back ( new File ( basePath + SSEP "_startup.c", false, "", false ) );
}

void
MingwTestModuleHandler::GenerateTestModuleTarget ()
{
	string targetMacro ( GetTargetMacro ( module ) );
	string workingDirectory = GetWorkingDirectory ( );
	string objectsMacro = GetObjectsMacro ( module );
	string linkDepsMacro = GetLinkingDependenciesMacro ();
	string libsMacro = GetLibsMacro ();

	GenerateImportLibraryTargetIfNeeded ();

	if ( module.non_if_data.files.size () > 0 )
	{
		GenerateRules ();

		string dependencies = linkDepsMacro + " " + objectsMacro;

		string linker;
		if ( module.cplusplus )
			linker = "${gpp}";
		else
			linker = "${gcc}";

		string linkerParameters = ssprintf ( "-Wl,--subsystem,console -Wl,--entry,%s -Wl,--image-base,%s -Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000",
		                                     module.entrypoint.c_str (),
		                                     module.baseaddress.c_str () );
		GenerateLinkerCommand ( dependencies,
		                        linker,
		                        linkerParameters,
		                        objectsMacro,
		                        libsMacro );
	}
	else
	{
		GeneratePhonyTarget();
	}
}


MingwRpcServerModuleHandler::MingwRpcServerModuleHandler (
	const Module& module_ )

	: MingwModuleHandler ( module_ )
{
}

void
MingwRpcServerModuleHandler::Process ()
{
	GenerateRules ();
}

MingwRpcClientModuleHandler::MingwRpcClientModuleHandler (
	const Module& module_ )

	: MingwModuleHandler ( module_ )
{
}

void
MingwRpcClientModuleHandler::Process ()
{
	GenerateRules ();
}
