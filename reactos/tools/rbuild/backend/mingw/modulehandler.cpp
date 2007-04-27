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
		out += string(pfilename,p1-pfilename) + cSep;
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
	use_pch = false;
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

void
MingwModuleHandler::EnablePreCompiledHeaderSupport ()
{
	use_pch = true;
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
			if ( j + 2 < path.length () && path[j + 1] == cSep )
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
	if ( directoryTree == NULL )
		return file;
	string generatedFilesDirectory = backend->AddDirectoryTarget ( directory,
	                                                               directoryTree );
	if ( directory.find ( generatedFilesDirectory ) != string::npos )
		/* This path already includes the generated files directory variable */
		return file;
	else
	{
		if ( file == "" )
			return generatedFilesDirectory;
		return generatedFilesDirectory + sSep + file;
	}
}

/*static*/ string
MingwModuleHandler::PassThruCacheDirectory (const FileLocation* fileLocation )
{
	return PassThruCacheDirectory ( fileLocation->filename,
	                                fileLocation->directory );
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
		case Win32SCR:
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
		case Win32OCX: 
			handler = new MingwWin32OCXModuleHandler ( module );
			break;
		case KernelModeDriver:
        case ExportDriver: // maybe change this later
			handler = new MingwKernelModeDriverModuleHandler ( module );
			break;
		case BootLoader:
			handler = new MingwBootLoaderModuleHandler ( module );
			break;
		case BootSector:
			handler = new MingwBootSectorModuleHandler ( module );
			break;
		case BootProgram:
			handler = new MingwBootProgramModuleHandler ( module );
			break;
		case Iso:
			handler = new MingwIsoModuleHandler ( module );
			break;
		case LiveIso:
			handler = new MingwLiveIsoModuleHandler ( module );
			break;
		case IsoRegTest:
			handler = new MingwIsoModuleHandler ( module );
			break;
		case LiveIsoRegTest:
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
		case Alias:
			handler = new MingwAliasModuleHandler ( module );
			break;
		case IdlHeader:
			handler = new MingwIdlHeaderModuleHandler ( module );
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

FileLocation*
MingwModuleHandler::GetActualSourceFilename (
	const FileLocation* fileLocation ) const
{
	string filename = fileLocation->filename;
	string extension = GetExtension ( filename );
	if ( extension == ".spec" || extension == ".SPEC" )
	{
		string basename = GetBasename ( filename );
		PassThruCacheDirectory ( NormalizeFilename ( basename + ".stubs.c" ),
		                         backend->intermediateDirectory );
		return new FileLocation ( backend->intermediateDirectory, NormalizeFilename ( basename + ".stubs.c" ) );
	}
	else if ( extension == ".idl" || extension == ".IDL" )
	{
		string basename = GetBasename ( filename );
		string newname;
		if ( module.type == RpcServer )
		{
			newname = basename + "_s.c";
			PassThruCacheDirectory ( NormalizeFilename ( newname ),
				                     backend->intermediateDirectory );
			return new FileLocation ( backend->intermediateDirectory, NormalizeFilename ( newname ) );
		}
		else if ( module.type == RpcClient )
		{
			newname = basename + "_c.c";
			PassThruCacheDirectory ( NormalizeFilename ( newname ),
				                     backend->intermediateDirectory );
			return new FileLocation ( backend->intermediateDirectory, NormalizeFilename ( newname ) );
		}
		else //if ( module.type == IdlHeader )
		{
			newname = basename + ".h";
			return new FileLocation ( fileLocation->directory, filename );
		}
	}
	else
		return new FileLocation ( fileLocation->directory, filename );
}

string
MingwModuleHandler::GetExtraDependencies (
	const string& filename ) const
{
	string extension = GetExtension ( filename );
	if ( extension == ".idl" || extension == ".IDL" )
	{
		string basename = GetBasename ( filename );
		if ( (module.type == RpcServer) || (module.type == RpcClient) )
			return GetRpcServerHeaderFilename ( basename ) + " " + GetRpcClientHeaderFilename ( basename );
		else
			return GetIdlHeaderFilename ( basename );			
	}
	else
		return "";
}

string
MingwModuleHandler::GetCompilationUnitDependencies (
	const CompilationUnit& compilationUnit ) const
{
	if ( compilationUnit.files.size () <= 1 )
		return "";
	vector<string> sourceFiles;
	for ( size_t i = 0; i < compilationUnit.files.size (); i++ )
	{
		File& file = *compilationUnit.files[i];
		sourceFiles.push_back ( NormalizeFilename ( file.name ) );
	}
	return v2s ( sourceFiles, 10 );
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
	if ( module.type == IdlHeader )
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

	const vector<CompilationUnit*>& compilationUnits = module.non_if_data.compilationUnits;
	for ( i = 0; i < compilationUnits.size (); i++ )
	{
		if ( includeGeneratedFiles || !compilationUnits[i]->IsGeneratedFile () )
		{
			FileLocation* sourceFileLocation = GetActualSourceFilename (
				compilationUnits[i]->GetFilename ( backend->intermediateDirectory ) );
			list.push_back ( PassThruCacheDirectory ( sourceFileLocation->filename,
		                                                  sourceFileLocation->directory ) );
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
		const vector<CompilationUnit*>& compilationUnits = rIf.data.compilationUnits;
		for ( j = 0; j < compilationUnits.size (); j++ )
		{
			CompilationUnit& compilationUnit = *compilationUnits[j];
			if ( includeGeneratedFiles || !compilationUnit.IsGeneratedFile () )
			{
				FileLocation* sourceFileLocation = GetActualSourceFilename (
					compilationUnit.GetFilename ( backend->intermediateDirectory ) );
				list.push_back ( PassThruCacheDirectory ( sourceFileLocation->filename,
				                                          sourceFileLocation->directory ) );
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
	const FileLocation* sourceFileLocation,
	string_list* pclean_files ) const
{
	string sourceFilename = sourceFileLocation->filename;
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
		else if ( module.type == RpcClient )
			newExtension = "_c.o";
		else
			newExtension = ".h";
	}
	else
		newExtension = ".o";

	if ( module.type == BootSector )
		directoryTree = backend->outputDirectory;
	else
		directoryTree = backend->intermediateDirectory;

	if (newExtension == ".h")
		directoryTree = NULL;

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

string
MingwModuleHandler::GetModuleCleanTarget ( const Module& module ) const
{
	return module.name + "_clean";
}

void
MingwModuleHandler::GetReferencedObjectLibraryModuleCleanTargets ( vector<string>& moduleNames ) const
{
	for ( size_t i = 0; i < module.non_if_data.libraries.size (); i++ )
	{
		Library& library = *module.non_if_data.libraries[i];
		if ( library.importedModule->type == ObjectLibrary )
			moduleNames.push_back ( GetModuleCleanTarget ( *library.importedModule ) );
	}
}

void
MingwModuleHandler::GenerateCleanTarget () const
{
	if ( module.type == Alias )
		return;

	fprintf ( fMakefile,
	          ".PHONY: %s_clean\n",
	          module.name.c_str() );
	vector<string> referencedModuleNames;
	GetReferencedObjectLibraryModuleCleanTargets ( referencedModuleNames );
	fprintf ( fMakefile,
	          "%s: %s\n\t-@${rm}",
	          GetModuleCleanTarget ( module ).c_str(),
	          v2s ( referencedModuleNames, 10 ).c_str () );
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
		NormalizeFilename ( module.installBase + sSep + module.installName ),
		backend->installDirectory );
	fprintf ( fMakefile,
	          "%s_install: %s\n",
	          module.name.c_str (),
	          normalizedTargetFilename.c_str() );
}

void
MingwModuleHandler::GenerateDependsTarget () const
{
	fprintf ( fMakefile,
	          ".PHONY: %s_depends\n",
	          module.name.c_str() );
	fprintf ( fMakefile,
	          "%s_depends: $(RBUILD_TARGET)\n",
	          module.name.c_str () );
	fprintf ( fMakefile,
	          "\t$(ECHO_RBUILD)\n" );
	fprintf ( fMakefile,
	          "\t$(Q)$(RBUILD_TARGET) -dm%s mingw\n",
	          module.name.c_str () );
}

string
MingwModuleHandler::GetObjectFilenames ()
{
	const vector<CompilationUnit*>& compilationUnits = module.non_if_data.compilationUnits;
	if ( compilationUnits.size () == 0 )
		return "";

	string objectFilenames ( "" );
	for ( size_t i = 0; i < compilationUnits.size (); i++ )
	{
		if ( objectFilenames.size () > 0 )
			objectFilenames += " ";
		objectFilenames += GetObjectFilename ( compilationUnits[i]->GetFilename ( backend->intermediateDirectory ), NULL );
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
	if ( path1[path1.length ()] == cSep )
		return path1 + path2;
	else
		return path1 + cSep + path2;
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
		dependencies += GetImportLibraryDependency ( *libraries[i]->importedModule );
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
	bool generateAssignment;

	generateAssignment = (use_pch && module.pch != NULL ) || data.includes.size () > 0 || data.defines.size () > 0 || data.compilerFlags.size () > 0;
	if ( generateAssignment )
	{
		fprintf ( fMakefile,
		          "%s %s",
		          macro.c_str(),
		          assignmentOperation );
	}

	if ( use_pch && module.pch != NULL )
	{
		fprintf ( fMakefile,
		          " -I%s",
		          GetDirectory ( GetPrecompiledHeaderFilename () ).c_str () );
	}

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
		const Include& include = *data.includes[i];
		string includeDirectory;
		if ( include.baseModule != NULL &&
		     ( include.baseModule->type == RpcServer ||
		       include.baseModule->type == RpcClient ||
		       include.baseModule->type == IdlHeader) )
			includeDirectory = PassThruCacheDirectory ( NormalizeFilename ( include.directory ),
	                                                            backend->intermediateDirectory );
		else
			includeDirectory = include.directory;
		fprintf (
			fMakefile,
			" -I%s",
			includeDirectory.c_str() );
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
	if ( generateAssignment )
	{
		fprintf ( fMakefile, "\n" );
	}
}

void
MingwModuleHandler::GenerateMacros (
	const char* assignmentOperation,
	const IfableData& data,
	const vector<LinkerFlag*>* linkerFlags )
{
	size_t i;

	GenerateMacro ( assignmentOperation,
	                cflagsMacro,
	                data );
	GenerateMacro ( assignmentOperation,
	                windresflagsMacro,
	                data );

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
			|| rIf.data.compilationUnits.size()
			|| rIf.data.compilerFlags.size()
			|| rIf.data.ifs.size() )
		{
			fprintf (
				fMakefile,
				"%s (\"$(%s)\",\"%s\")\n",
				rIf.negated ? "ifneq" : "ifeq",
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
MingwModuleHandler::CleanupCompilationUnitVector ( vector<CompilationUnit*>& compilationUnits )
{
	for (size_t i = 0; i < compilationUnits.size (); i++)
		delete compilationUnits[i];
}

void
MingwModuleHandler::GetModuleSpecificCompilationUnits ( vector<CompilationUnit*>& compilationUnits )
{
}

void
MingwModuleHandler::GenerateObjectMacros (
	const char* assignmentOperation,
	const IfableData& data,
	const vector<LinkerFlag*>* linkerFlags )
{
	size_t i;

	const vector<CompilationUnit*>& compilationUnits = data.compilationUnits;
	if ( compilationUnits.size () > 0 )
	{
		for ( i = 0; i < compilationUnits.size (); i++ )
		{
			CompilationUnit& compilationUnit = *compilationUnits[i];
			if ( compilationUnit.IsFirstFile () )
			{
				fprintf ( fMakefile,
					"%s := %s $(%s)\n",
					objectsMacro.c_str(),
					GetObjectFilename ( compilationUnit.GetFilename ( backend->intermediateDirectory ), NULL ).c_str (),
					objectsMacro.c_str() );
			}
		}
		fprintf (
			fMakefile,
			"%s %s",
			objectsMacro.c_str (),
			assignmentOperation );
		for ( i = 0; i < compilationUnits.size(); i++ )
		{
			CompilationUnit& compilationUnit = *compilationUnits[i];
			if ( !compilationUnit.IsFirstFile () )
			{
				fprintf (
					fMakefile,
					"%s%s",
					( i%10 == 9 ? " \\\n\t" : " " ),
					GetObjectFilename ( compilationUnit.GetFilename ( backend->intermediateDirectory ), NULL ).c_str () );
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
			|| rIf.data.compilationUnits.size()
			|| rIf.data.compilerFlags.size()
			|| rIf.data.ifs.size() )
		{
			fprintf (
				fMakefile,
				"%s (\"$(%s)\",\"%s\")\n",
				rIf.negated ? "ifneq" : "ifeq",
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

	vector<CompilationUnit*> sourceCompilationUnits;
	GetModuleSpecificCompilationUnits ( sourceCompilationUnits );
	for ( i = 0; i < sourceCompilationUnits.size (); i++ )
	{
		fprintf (
			fMakefile,
			"%s += %s\n",
			objectsMacro.c_str(),
			GetObjectFilename ( sourceCompilationUnits[i]->GetFilename ( backend->intermediateDirectory ), NULL ).c_str () );
	}
	CleanupCompilationUnitVector ( sourceCompilationUnits );
}

string
MingwModuleHandler::GetPrecompiledHeaderFilename () const
{
	const string& basePchFilename = module.pch->file.name + ".gch";
	return PassThruCacheDirectory ( NormalizeFilename ( basePchFilename ),
	                                backend->intermediateDirectory );
}

void
MingwModuleHandler::GenerateGccCommand (
	const FileLocation* sourceFileLocation,
	const string& extraDependencies,
	const string& cc,
	const string& cflagsMacro )
{
	string sourceFilename = PassThruCacheDirectory ( sourceFileLocation );
	string dependencies = sourceFilename;
	if ( extraDependencies != "" )
		dependencies += " " + extraDependencies;
	if ( module.pch && use_pch )
		dependencies += " " + GetPrecompiledHeaderFilename ();

	/* WIDL generated headers may be used */
	vector<string> rpcDependencies;
	GetRpcHeaderDependencies ( rpcDependencies );
	dependencies += " " + v2s ( rpcDependencies, 5 );
	dependencies += " " + NormalizeFilename ( module.xmlbuildFile );

	string objectFilename = GetObjectFilename (
		sourceFileLocation, &clean_files );
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
	const FileLocation* sourceFileLocation,
	const string& cc,
	const string& cflagsMacro )
{
	string sourceFilename = PassThruCacheDirectory ( sourceFileLocation );
	string dependencies = sourceFilename;
	dependencies += " " + NormalizeFilename ( module.xmlbuildFile );
	string objectFilename = GetObjectFilename (
		sourceFileLocation, &clean_files );
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
	const FileLocation* sourceFileLocation,
	const string& nasmflagsMacro )
{
	string sourceFilename = PassThruCacheDirectory ( sourceFileLocation );
	string dependencies = sourceFilename;
	dependencies += " " + NormalizeFilename ( module.xmlbuildFile );
	string objectFilename = GetObjectFilename (
		sourceFileLocation, &clean_files );
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
	const FileLocation* sourceFileLocation,
	const string& windresflagsMacro )
{
	string sourceFilename = PassThruCacheDirectory ( sourceFileLocation );
	string dependencies = sourceFilename;
	dependencies += " " + NormalizeFilename ( module.xmlbuildFile );
	string objectFilename = GetObjectFilename ( sourceFileLocation, &clean_files );
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
	const FileLocation* sourceFileLocation )
{
	string sourceFilename = PassThruCacheDirectory ( sourceFileLocation );
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
	          "%s: %s $(WINEBUILD_TARGET) | %s\n",
	          def_file.c_str (),
	          dependencies.c_str (),
	          GetDirectory ( def_file ).c_str () );
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
MingwModuleHandler::GetWidlFlags ( const CompilationUnit& compilationUnit )
{
	return compilationUnit.GetSwitches ();
}

string
MingwModuleHandler::GetRpcServerHeaderFilename ( string basename ) const
{
	return PassThruCacheDirectory ( basename + "_s.h",
	                                backend->intermediateDirectory );
}

void
MingwModuleHandler::GenerateWidlCommandsServer (
	const CompilationUnit& compilationUnit,
	const string& widlflagsMacro )
{
	FileLocation* sourceFileLocation = compilationUnit.GetFilename ( backend->intermediateDirectory );
	string filename = sourceFileLocation->filename;
	string dependencies = filename;
	dependencies += " " + NormalizeFilename ( module.xmlbuildFile );

	string basename = GetBasename ( filename );

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
	          GetWidlFlags ( compilationUnit ).c_str (),
	          widlflagsMacro.c_str (),
	          generatedHeaderFilename.c_str (),
	          generatedServerFilename.c_str (),
	          filename.c_str () );
}

string
MingwModuleHandler::GetRpcClientHeaderFilename ( string basename ) const
{
	return PassThruCacheDirectory ( basename + "_c.h",
	                                backend->intermediateDirectory );
}

string
MingwModuleHandler::GetIdlHeaderFilename ( string basename ) const
{
	return basename + ".h";
}

void
MingwModuleHandler::GenerateWidlCommandsClient (
	const CompilationUnit& compilationUnit,
	const string& widlflagsMacro )
{
	FileLocation* sourceFileLocation = compilationUnit.GetFilename ( backend->intermediateDirectory );
	string filename = sourceFileLocation->filename;
	string dependencies = filename;
	dependencies += " " + NormalizeFilename ( module.xmlbuildFile );

	string basename = GetBasename ( filename );

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
	          GetWidlFlags ( compilationUnit ).c_str (),
	          widlflagsMacro.c_str (),
	          generatedHeaderFilename.c_str (),
	          generatedClientFilename.c_str (),
	          filename.c_str () );
}

void
MingwModuleHandler::GenerateWidlCommandsIdlHeader (
	const CompilationUnit& compilationUnit,
	const string& widlflagsMacro )
{
	FileLocation* sourceFileLocation = compilationUnit.GetFilename ( backend->intermediateDirectory );
	string filename = sourceFileLocation->filename;
	string dependencies = filename;
	dependencies += " " + NormalizeFilename ( module.xmlbuildFile );

	string basename = GetBasename ( filename );

	string generatedHeaderFilename = GetIdlHeaderFilename ( basename );
	CLEAN_FILE(generatedHeaderFilename);

	fprintf ( fMakefile,
	          "%s: %s $(WIDL_TARGET) | %s\n",
	          generatedHeaderFilename.c_str (),
	          dependencies.c_str (),
	          GetDirectory ( generatedHeaderFilename ).c_str () );
	fprintf ( fMakefile, "\t$(ECHO_WIDL)\n" );
	fprintf ( fMakefile,
	          "\t%s %s %s -h -H %s %s\n",
	          "$(Q)$(WIDL_TARGET)",
	          GetWidlFlags ( compilationUnit ).c_str (),
	          widlflagsMacro.c_str (),
	          generatedHeaderFilename.c_str (),
	          filename.c_str () );
}

void
MingwModuleHandler::GenerateWidlCommands (
	const CompilationUnit& compilationUnit,
	const string& widlflagsMacro )
{
	if ( module.type == RpcServer )
		GenerateWidlCommandsServer ( compilationUnit,
		                             widlflagsMacro );
	else if ( module.type == RpcClient )
		GenerateWidlCommandsClient ( compilationUnit,
		                             widlflagsMacro );
	else
		GenerateWidlCommandsIdlHeader ( compilationUnit,
		                                widlflagsMacro );
}

void
MingwModuleHandler::GenerateCommands (
	const CompilationUnit& compilationUnit,
	const string& cc,
	const string& cppc,
	const string& cflagsMacro,
	const string& nasmflagsMacro,
	const string& windresflagsMacro,
	const string& widlflagsMacro )
{
	FileLocation* sourceFileLocation = compilationUnit.GetFilename ( backend->intermediateDirectory );
	string filename = sourceFileLocation->filename;
	string extension = GetExtension ( filename );
	if ( extension == ".c" || extension == ".C" )
	{
		GenerateGccCommand ( sourceFileLocation,
		                     GetCompilationUnitDependencies ( compilationUnit ),
		                     cc,
		                     cflagsMacro );
		return;
	}
	else if ( extension == ".cc" || extension == ".CC" ||
	          extension == ".cpp" || extension == ".CPP" ||
	          extension == ".cxx" || extension == ".CXX" )
	{
		GenerateGccCommand ( sourceFileLocation,
		                     GetCompilationUnitDependencies ( compilationUnit ),
		                     cppc,
		                     cflagsMacro );
		return;
	}
	else if ( extension == ".s" || extension == ".S" )
	{
		GenerateGccAssemblerCommand ( sourceFileLocation,
		                              cc,
		                              cflagsMacro );
		return;
	}
	else if ( extension == ".asm" || extension == ".ASM" )
	{
		GenerateNasmCommand ( sourceFileLocation,
		                      nasmflagsMacro );
		return;
	}
	else if ( extension == ".rc" || extension == ".RC" )
	{
		GenerateWindresCommand ( sourceFileLocation,
		                         windresflagsMacro );
		return;
	}
	else if ( extension == ".spec" || extension == ".SPEC" )
	{
		GenerateWinebuildCommands ( sourceFileLocation );
		GenerateGccCommand ( GetActualSourceFilename ( sourceFileLocation ),
		                     "",
		                     cc,
		                     cflagsMacro );
		return;
	}
	else if ( extension == ".idl" || extension == ".IDL" )
	{
		GenerateWidlCommands ( compilationUnit,
		                       widlflagsMacro );
		if ( (module.type == RpcServer) || (module.type == RpcClient) )
		{
			GenerateGccCommand ( GetActualSourceFilename ( sourceFileLocation ),
			                     GetExtraDependencies ( filename ),
		    	                 cc,
		        	             cflagsMacro );
		}
		return;
	}

	throw InvalidOperationException ( __FILE__,
	                                  __LINE__,
	                                  "Unsupported filename extension '%s' in file '%s'",
	                                  extension.c_str (),
	                                  filename.c_str () );
}

void
MingwModuleHandler::GenerateBuildMapCode ( const char *mapTarget )
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
	          "\t$(Q)${objdump} -d -S %s > %s\n",
			  mapTarget ? mapTarget :  "$@",
	          mapFilename.c_str () );

	fprintf ( fMakefile,
	          "else\n" );
	fprintf ( fMakefile,
	          "ifeq ($(ROS_BUILDMAP),yes)\n" );

	fprintf ( fMakefile,
	          "\t$(ECHO_NM)\n" );
	fprintf ( fMakefile,
	          "\t$(Q)${nm} --numeric-sort %s > %s\n",
			  mapTarget ? mapTarget :  "$@",
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
	for ( size_t i = 0; i < data.compilationUnits.size (); i++ )
	{
		CompilationUnit& compilationUnit = *data.compilationUnits[i];
		objectFiles.push_back ( GetObjectFilename ( compilationUnit.GetFilename ( backend->intermediateDirectory ), NULL ) );
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
MingwModuleHandler::GenerateRunStripCode () const
{
	fprintf ( fMakefile,
	          "ifeq ($(ROS_LEAN_AND_MEAN),yes)\n" );
	fprintf ( fMakefile,
	          "\t$(ECHO_STRIP)\n" );
	fprintf ( fMakefile,
	          "\t${strip} -s -x -X $@\n\n" );
	fprintf ( fMakefile,
	          "endif\n" );
}

void
MingwModuleHandler::GenerateLinkerCommand (
	const string& dependencies,
	const string& linker,
	const string& linkerParameters,
	const string& objectsMacro,
	const string& libsMacro,
	const string& pefixupParameters )
{
	string target ( GetTargetMacro ( module ) );
	string target_folder ( GetDirectory ( GetTargetFilename ( module, NULL ) ) );
	string definitionFilename = GetDefinitionFilename ();

	string linkerScriptArgument;
	if ( module.linkerScript != NULL )
		linkerScriptArgument = ssprintf ( "-Wl,-T,%s", module.linkerScript->directory.c_str () );
	else
		linkerScriptArgument = "";

	fprintf ( fMakefile,
		"%s: %s %s $(RSYM_TARGET) $(PEFIXUP_TARGET) | %s\n",
		target.c_str (),
		definitionFilename.c_str (),
		dependencies.c_str (),
		target_folder.c_str () );
	fprintf ( fMakefile, "\t$(ECHO_LD)\n" );
	string targetName ( module.GetTargetName () );

	if ( module.IsDLL () )
	{
		string temp_exp = ros_temp + module.name + ".temp.exp";
		CLEAN_FILE ( temp_exp );

		fprintf ( fMakefile,
		          "\t${dlltool} --dllname %s --def %s --output-exp %s %s %s\n",
		          targetName.c_str (),
		          definitionFilename.c_str (),
		          temp_exp.c_str (),
		          module.mangledSymbols ? "" : "--kill-at",
		          module.underscoreSymbols ? "--add-underscore" : "" );

		fprintf ( fMakefile,
		          "\t%s %s %s %s -o %s %s %s %s\n",
		          linker.c_str (),
		          linkerParameters.c_str (),
		          linkerScriptArgument.c_str (),
		          temp_exp.c_str (),
		          target.c_str (),
		          objectsMacro.c_str (),
		          libsMacro.c_str (),
		          GetLinkerMacro ().c_str () );

		fprintf ( fMakefile,
		          "\t$(Q)$(PEFIXUP_TARGET) %s -exports %s\n",
		          target.c_str (),
		          pefixupParameters.c_str() );

		fprintf ( fMakefile,
		          "\t-@${rm} %s 2>$(NUL)\n",
		          temp_exp.c_str () );
	}
	else
	{
		fprintf ( fMakefile,
		          "\t%s %s %s -o %s %s %s %s\n",
		          linker.c_str (),
		          linkerParameters.c_str (),
		          linkerScriptArgument.c_str (),
		          target.c_str (),
		          objectsMacro.c_str (),
		          libsMacro.c_str (),
		          GetLinkerMacro ().c_str () );

#if 0 // causes crashes sometimes
		fprintf ( fMakefile,
		          "\t${objcopy} -R .edata %s\n",
		          target.c_str () );
#endif
	}

	GenerateBuildMapCode ();
	GenerateBuildNonSymbolStrippedCode ();
	GenerateRunRsymCode ();
	GenerateRunStripCode ();
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

	const vector<CompilationUnit*>& compilationUnits = data.compilationUnits;
	for ( i = 0; i < compilationUnits.size (); i++ )
	{
		GenerateCommands ( *compilationUnits[i],
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

	vector<CompilationUnit*> sourceCompilationUnits;
	GetModuleSpecificCompilationUnits ( sourceCompilationUnits );
	for ( i = 0; i < sourceCompilationUnits.size (); i++ )
	{
		GenerateCommands ( *sourceCompilationUnits[i],
		                   cc,
		                   cppc,
		                   cflagsMacro,
		                   nasmflagsMacro,
		                   windresflagsMacro,
		                   widlflagsMacro );
	}
	CleanupCompilationUnitVector ( sourceCompilationUnits );
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
	if ( module.pch && use_pch )
	{
		const string& baseHeaderFilename = module.pch->file.name;
		const string& pchFilename = GetPrecompiledHeaderFilename ();
		CLEAN_FILE(pchFilename);
		string dependencies = baseHeaderFilename;
		/* WIDL generated headers may be used */
		vector<string> rpcDependencies;
		GetRpcHeaderDependencies ( rpcDependencies );
		dependencies += " " + v2s ( rpcDependencies, 5 );
		fprintf ( fMakefile,
		          "%s: %s\n",
		          pchFilename.c_str(),
		          dependencies.c_str() );
		fprintf ( fMakefile, "\t$(ECHO_PCH)\n" );
		fprintf ( fMakefile,
		          "\t%s -o %s %s -g %s\n\n",
		          module.cplusplus ? cppc.c_str() : cc.c_str(),
		          pchFilename.c_str(),
		          cflagsMacro.c_str(),
		          baseHeaderFilename.c_str() );
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

	if ( module.type == StaticLibrary && module.importLibrary )
	{
		string archiveFilename ( GetModuleArchiveFilename () );
		string definitionFilename ( GetDefinitionFilename () );

		fprintf ( fMakefile,
		          "\t${dlltool} --dllname %s --def %s --output-lib $@ %s %s\n",
		          module.importLibrary->dllname.c_str (),
		          definitionFilename.c_str (),
		          module.mangledSymbols ? "" : "--kill-at",
		          module.underscoreSymbols ? "--add-underscore" : "" );
	}

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
	vector<string>& dependencies ) const
{
	for ( size_t i = 0; i < module.non_if_data.libraries.size (); i++ )
	{
		Library& library = *module.non_if_data.libraries[i];
		if ( library.importedModule->type == RpcServer ||
		     library.importedModule->type == RpcClient ||
		     library.importedModule->type == IdlHeader )
		{
			for ( size_t j = 0; j < library.importedModule->non_if_data.compilationUnits.size (); j++ )
			{
				CompilationUnit& compilationUnit = *library.importedModule->non_if_data.compilationUnits[j];
				FileLocation* sourceFileLocation = compilationUnit.GetFilename ( backend->intermediateDirectory );
				string extension = GetExtension ( sourceFileLocation->filename );
				if ( extension == ".idl" || extension == ".IDL" )
				{
					string basename = GetBasename ( sourceFileLocation->filename );
					if ( library.importedModule->type == RpcServer )
						dependencies.push_back ( GetRpcServerHeaderFilename ( basename ) );
					if ( library.importedModule->type == RpcClient )
						dependencies.push_back ( GetRpcClientHeaderFilename ( basename ) );
					if ( library.importedModule->type == IdlHeader )
						dependencies.push_back ( GetIdlHeaderFilename ( basename ) );
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

	vector<string> s;
	if ( module.importLibrary )
	{
		const vector<CompilationUnit*>& compilationUnits = module.non_if_data.compilationUnits;
		for ( size_t i = 0; i < compilationUnits.size (); i++ )
		{
			CompilationUnit& compilationUnit = *compilationUnits[i];
			FileLocation* sourceFileLocation = compilationUnit.GetFilename ( backend->intermediateDirectory );
			string extension = GetExtension ( sourceFileLocation->filename );
			if ( extension == ".spec" || extension == ".SPEC" )
				GetSpecObjectDependencies ( s, sourceFileLocation->filename );
		}
	}
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
	if ( !module.allowWarnings )
		globalCflags += " -Werror";

	// Always force disabling of sibling calls optimisation for GCC
	// (TODO: Move to version-specific once this bug is fixed in GCC)
	globalCflags += " -fno-optimize-sibling-calls";

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

	if ( module.type == StaticLibrary && module.isStartupLib )
	{
		fprintf ( fMakefile,
		          "%s += -Wno-main\n\n",
		          cflagsMacro.c_str () );
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
			NormalizeFilename ( module.GetBasePath () + sSep + "makefile" ),
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
			throw XMLInvalidBuildFileException (
				module.node.location,
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
		string defFilename = module.GetBasePath () + sSep + module.importLibrary->definition;
		if ( IsWineModule () )
			return PassThruCacheDirectory ( NormalizeFilename ( defFilename ),
			                                backend->intermediateDirectory );
		else
			return defFilename;
	}
	else
		return "tools" + sSep + "rbuild" + sSep + "empty.def";
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

		fprintf ( fMakefile,
		          "\t${dlltool} --dllname %s --def %s --output-lib %s %s %s\n\n",
		          module.GetTargetName ().c_str (),
		          defFilename.c_str (),
		          library_target.c_str (),
		          module.mangledSymbols ? "" : "--kill-at",
		          module.underscoreSymbols ? "--add-underscore" : "" );
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
	string serverSourceDependency = PassThruCacheDirectory (
		NormalizeFilename ( basename + "_s.c" ),
		backend->intermediateDirectory );
	dependencies.push_back ( serverSourceDependency );
	dependencies.push_back ( GetRpcServerHeaderFilename ( basename ) );
}

void
MingwModuleHandler::GetDefinitionDependencies (
	string_list& dependencies ) const
{
	string dkNkmLibNoFixup = "dk/nkm/lib";
	const vector<CompilationUnit*>& compilationUnits = module.non_if_data.compilationUnits;
	for ( size_t i = 0; i < compilationUnits.size (); i++ )
	{
		CompilationUnit& compilationUnit = *compilationUnits[i];
		FileLocation* sourceFileLocation = compilationUnit.GetFilename ( backend->intermediateDirectory );
		string extension = GetExtension ( sourceFileLocation->filename );
		if ( extension == ".spec" || extension == ".SPEC" )
			GetSpecObjectDependencies ( dependencies, sourceFileLocation->filename );
		if ( extension == ".idl" || extension == ".IDL" )
		{
			if ( ( module.type == RpcServer ) || ( module.type == RpcClient ) )
				GetWidlObjectDependencies ( dependencies, sourceFileLocation->filename );
		}
	}
}

enum DebugSupportType
{
	DebugKernelMode,
	DebugUserMode
};

static void
MingwAddDebugSupportLibraries ( Module& module, DebugSupportType type )
{
	Library* pLibrary;

	switch(type)
	{
		case DebugKernelMode:
			pLibrary = new Library ( module, "debugsup_ntoskrnl" );
			break;

		case DebugUserMode:
			pLibrary = new Library ( module, "debugsup_ntdll" );
			break;

		default:
			assert(0);
	}

	module.non_if_data.libraries.push_back(pLibrary);
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

	if ( module.non_if_data.compilationUnits.size () > 0 )
	{
		GenerateRules ();

		string dependencies = linkDepsMacro + " " + objectsMacro;

		string linkerParameters = ssprintf ( "-Wl,-T,%s%cntoskrnl.lnk -Wl,--subsystem,native -Wl,--entry,%s -Wl,--image-base,%s -Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000 -nostartfiles -shared",
		                                     module.GetBasePath ().c_str (),
		                                     cSep,
		                                     module.GetEntryPoint(true).c_str (),
		                                     module.baseaddress.c_str () );
		GenerateLinkerCommand ( dependencies,
		                        "${gcc}",
		                        linkerParameters,
		                        objectsMacro,
		                        libsMacro,
		                        "-sections" );
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
MingwKernelModeDLLModuleHandler::AddImplicitLibraries ( Module& module )
{
	MingwAddDebugSupportLibraries ( module, DebugKernelMode );
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

	if ( module.non_if_data.compilationUnits.size () > 0 )
	{
		GenerateRules ();

		string dependencies = linkDepsMacro + " " + objectsMacro;

		string linkerParameters = ssprintf ( "-Wl,--subsystem,native -Wl,--entry,%s -Wl,--image-base,%s -Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000 -nostartfiles -shared",
		                                     module.GetEntryPoint(true).c_str (),
		                                     module.baseaddress.c_str () );
		GenerateLinkerCommand ( dependencies,
		                        "${gcc}",
		                        linkerParameters,
		                        objectsMacro,
		                        libsMacro,
		                        "-sections" );
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
MingwKernelModeDriverModuleHandler::AddImplicitLibraries ( Module& module )
{
	MingwAddDebugSupportLibraries ( module, DebugKernelMode );
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

	if ( module.non_if_data.compilationUnits.size () > 0 )
	{
		GenerateRules ();

		string dependencies = linkDepsMacro + " " + objectsMacro;

		string linkerParameters = ssprintf ( "-Wl,--subsystem,native -Wl,--entry,%s -Wl,--image-base,%s -Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000 -nostartfiles -shared",
		                                     module.GetEntryPoint(true).c_str (),
		                                     module.baseaddress.c_str () );
		GenerateLinkerCommand ( dependencies,
		                        "${gcc}",
		                        linkerParameters,
		                        objectsMacro,
		                        libsMacro,
		                        "-sections" );
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
MingwNativeDLLModuleHandler::AddImplicitLibraries ( Module& module )
{
	MingwAddDebugSupportLibraries ( module, DebugUserMode );
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

	if ( module.non_if_data.compilationUnits.size () > 0 )
	{
		GenerateRules ();

		string dependencies = linkDepsMacro + " " + objectsMacro;

		string linkerParameters = ssprintf ( "-Wl,--subsystem,native -Wl,--entry,%s -Wl,--image-base,%s -Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000 -nostartfiles -nostdlib -shared",
		                                     module.GetEntryPoint(true).c_str (),
		                                     module.baseaddress.c_str () );
		GenerateLinkerCommand ( dependencies,
		                        "${gcc}",
		                        linkerParameters,
		                        objectsMacro,
		                        libsMacro,
		                        "" );
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
MingwNativeCUIModuleHandler::AddImplicitLibraries ( Module& module )
{
	MingwAddDebugSupportLibraries ( module, DebugUserMode );
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

	if ( module.non_if_data.compilationUnits.size () > 0 )
	{
		GenerateRules ();

		string dependencies = linkDepsMacro + " " + objectsMacro;

		string linkerParameters = ssprintf ( "-Wl,--subsystem,native -Wl,--entry,%s -Wl,--image-base,%s -Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000 -nostartfiles -nostdlib",
		                                     module.GetEntryPoint(true).c_str (),
		                                     module.baseaddress.c_str () );
		GenerateLinkerCommand ( dependencies,
		                        "${gcc}",
		                        linkerParameters,
		                        objectsMacro,
		                        libsMacro,
		                        "" );
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

MingwWin32OCXModuleHandler::MingwWin32OCXModuleHandler (
	const Module& module_ )

	: MingwModuleHandler ( module_ )
{
}

static void
MingwAddImplicitLibraries( Module &module )
{
	Library* pLibrary;

	if ( !module.isDefaultEntryPoint )
		return;

	if ( module.IsDLL () )
	{
		//pLibrary = new Library ( module, "__mingw_dllmain" );
		//module.non_if_data.libraries.insert ( module.non_if_data.libraries.begin(), pLibrary );
	}
	else
	{
		pLibrary = new Library ( module, module.isUnicode ? "mingw_wmain" : "mingw_main" );
		module.non_if_data.libraries.insert ( module.non_if_data.libraries.begin(), pLibrary );
	}

	pLibrary = new Library ( module, "mingw_common" );
	module.non_if_data.libraries.insert ( module.non_if_data.libraries.begin() + 1, pLibrary );

	if ( module.name != "msvcrt" )
	{
		// always link in msvcrt to get the basic routines
		pLibrary = new Library ( module, "msvcrt" );
		module.non_if_data.libraries.push_back ( pLibrary );
	}
}

void
MingwWin32DLLModuleHandler::AddImplicitLibraries ( Module& module )
{
	MingwAddImplicitLibraries ( module );
	MingwAddDebugSupportLibraries ( module, DebugUserMode );
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

	if ( module.non_if_data.compilationUnits.size () > 0 )
	{
		GenerateRules ();

		string dependencies = linkDepsMacro + " " + objectsMacro;

		string linker;
		if ( module.cplusplus )
			linker = "${gpp}";
		else
			linker = "${gcc}";

		string linkerParameters = ssprintf ( "-Wl,--subsystem,console -Wl,--entry,%s -Wl,--image-base,%s -Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000 -shared",
		                                     module.GetEntryPoint(true).c_str (),
		                                     module.baseaddress.c_str () );
		GenerateLinkerCommand ( dependencies,
		                        linker,
		                        linkerParameters,
		                        objectsMacro,
		                        libsMacro,
		                        "" );
	}
	else
	{
		GeneratePhonyTarget();
	}
}


void
MingwWin32OCXModuleHandler::AddImplicitLibraries ( Module& module )
{
	MingwAddImplicitLibraries ( module );
	MingwAddDebugSupportLibraries ( module, DebugUserMode );
}

void
MingwWin32OCXModuleHandler::Process ()
{
	GenerateWin32OCXModuleTarget ();
}

void
MingwWin32OCXModuleHandler::GenerateWin32OCXModuleTarget ()
{
	string targetMacro ( GetTargetMacro (module) );
	string workingDirectory = GetWorkingDirectory ( );
	string objectsMacro = GetObjectsMacro ( module );
	string linkDepsMacro = GetLinkingDependenciesMacro ();
	string libsMacro = GetLibsMacro ();

	GenerateImportLibraryTargetIfNeeded ();

	if ( module.non_if_data.compilationUnits.size () > 0 )
	{
		GenerateRules ();

		string dependencies = linkDepsMacro + " " + objectsMacro;

		string linker;
		if ( module.cplusplus )
			linker = "${gpp}";
		else
			linker = "${gcc}";

		string linkerParameters = ssprintf ( "-Wl,--subsystem,console -Wl,--entry,%s -Wl,--image-base,%s -Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000 -shared",
		                                     module.GetEntryPoint(true).c_str (),
		                                     module.baseaddress.c_str () );
		GenerateLinkerCommand ( dependencies,
		                        linker,
		                        linkerParameters,
		                        objectsMacro,
		                        libsMacro,
		                        "" );
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
MingwWin32CUIModuleHandler::AddImplicitLibraries ( Module& module )
{
	MingwAddImplicitLibraries ( module );
	MingwAddDebugSupportLibraries ( module, DebugUserMode );
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

	if ( module.non_if_data.compilationUnits.size () > 0 )
	{
		GenerateRules ();

		string dependencies = linkDepsMacro + " " + objectsMacro;

		string linker;
		if ( module.cplusplus )
			linker = "${gpp}";
		else
			linker = "${gcc}";

		string linkerParameters = ssprintf ( "-Wl,--subsystem,console -Wl,--entry,%s -Wl,--image-base,%s -Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000",
		                                     module.GetEntryPoint(true).c_str (),
		                                     module.baseaddress.c_str () );
		GenerateLinkerCommand ( dependencies,
		                        linker,
		                        linkerParameters,
		                        objectsMacro,
		                        libsMacro,
		                        "" );
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
MingwWin32GUIModuleHandler::AddImplicitLibraries ( Module& module )
{
	MingwAddImplicitLibraries ( module );
	MingwAddDebugSupportLibraries ( module, DebugUserMode );
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

	if ( module.non_if_data.compilationUnits.size () > 0 )
	{
		GenerateRules ();

		string dependencies = linkDepsMacro + " " + objectsMacro;

		string linker;
		if ( module.cplusplus )
			linker = "${gpp}";
		else
			linker = "${gcc}";

		string linkerParameters = ssprintf ( "-Wl,--subsystem,windows -Wl,--entry,%s -Wl,--image-base,%s -Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000",
		                                     module.GetEntryPoint(true).c_str (),
		                                     module.baseaddress.c_str () );
		GenerateLinkerCommand ( dependencies,
		                        linker,
		                        linkerParameters,
		                        objectsMacro,
		                        libsMacro,
		                        "" );
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
	GenerateBuildMapCode ( junk_tmp.c_str() );
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


MingwBootProgramModuleHandler::MingwBootProgramModuleHandler (
	const Module& module_ )
	: MingwModuleHandler ( module_ )
{
}

void
MingwBootProgramModuleHandler::Process ()
{
	GenerateBootProgramModuleTarget ();
}

void
MingwBootProgramModuleHandler::GenerateBootProgramModuleTarget ()
{
	string targetName ( module.GetTargetName () );
	string targetMacro ( GetTargetMacro (module) );
	string workingDirectory = GetWorkingDirectory ();
	string junk_tmp = ros_temp + module.name + ".junk.tmp";
	string junk_elf = ros_temp + module.name + ".junk.elf";
	string junk_cpy = ros_temp + module.name + ".junk.cpy";
	CLEAN_FILE ( junk_tmp );
	CLEAN_FILE ( junk_elf );
	CLEAN_FILE ( junk_cpy );
	string objectsMacro = GetObjectsMacro ( module );
	string linkDepsMacro = GetLinkingDependenciesMacro ();
	string libsMacro = GetLibsMacro ();
	const Module *payload = module.project.LocateModule ( module.payload );

	GenerateRules ();

	fprintf ( fMakefile, "%s: %s %s %s | %s\n",
	          targetMacro.c_str (),
	          objectsMacro.c_str (),
	          linkDepsMacro.c_str (),
		  payload->name.c_str (),
	          GetDirectory(GetTargetFilename(module,NULL)).c_str () );

	fprintf ( fMakefile, "\t$(ECHO_BOOTPROG)\n" );

	fprintf ( fMakefile, "\t$(BOOTPROG_PREPARE) $(OUTPUT)$(SEP)%s %s\n",
		  NormalizeFilename( payload->GetPath() ).c_str (),
		junk_cpy.c_str () );

	fprintf ( fMakefile, "\t${objcopy} $(BOOTPROG_FLATFORMAT) %s %s\n",
		junk_cpy.c_str (),
		junk_tmp.c_str () );

	fprintf ( fMakefile, "\t${ld} $(BOOTPROG_LINKFORMAT) %s %s -g -o %s\n",
		linkDepsMacro.c_str (),
		junk_tmp.c_str (),
		junk_elf.c_str () );

	fprintf ( fMakefile, "\t${objcopy} $(BOOTPROG_COPYFORMAT) %s %s\n",
		junk_elf.c_str (),
		module.GetPath().c_str () );

	fprintf ( fMakefile,
	          "\t-@${rm} %s %s %s 2>$(NUL)\n",
	          junk_tmp.c_str (), junk_elf.c_str (), junk_cpy.c_str () );
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
			string targetFilenameNoFixup ( bootcdDirectory + sSep + m.bootstrap->base + sSep + m.bootstrap->nameoncd );
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
		string targetFilenameNoFixup = bootcdDirectory + sSep + cdfile.base + sSep + cdfile.nameoncd;
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
			string targetDirectory ( bootcdDirectory + sSep + m.bootstrap->base );
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
		string targetDirectory ( bootcdDirectory + sSep + cdfile.base );
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
		NormalizeFilename ( bootcdDirectory + sSep ),
		backend->outputDirectory );

	string bootloader;
	string IsoName;

	if (module.name == "bootcdregtest")
	{
		bootloader = "isobtrt.o";
		IsoName = "ReactOS-RegTest.iso";
	}
	else
	{
		bootloader = "isoboot.o";
		IsoName = "ReactOS.iso";
	}

	string isoboot = PassThruCacheDirectory (
		NormalizeFilename ( "boot" + sSep + "freeldr" + sSep + "bootsect" + sSep + bootloader.c_str() ),
		backend->outputDirectory );

	string bootcdReactosNoFixup = bootcdDirectory + sSep + Environment::GetCdOutputPath ();
	string bootcdReactos = PassThruCacheDirectory (
		NormalizeFilename ( bootcdReactosNoFixup + sSep ),
		backend->outputDirectory );
	CLEAN_FILE ( bootcdReactos );
	string reactosInf = PassThruCacheDirectory (
		NormalizeFilename ( bootcdReactosNoFixup + sSep + "reactos.inf" ),
		backend->outputDirectory );
	string reactosDff = NormalizeFilename ( "boot" + sSep + "bootdata" + sSep + "packages" + sSep + "reactos.dff" );
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
	          "\t$(Q)$(CDMAKE_TARGET) -v -j -m -b %s %s REACTOS %s\n",
	          isoboot.c_str (),
	          bootcd.c_str (),
			  IsoName.c_str() );
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
		NormalizeFilename ( directory ) + sSep,
		backend->outputDirectory );
}

void
MingwLiveIsoModuleHandler::OutputCopyCommand ( const string& sourceFilename,
                                               const string& targetFilename,
                                               const string& targetDirectory )
{
	string normalizedTargetFilename = MingwModuleHandler::PassThruCacheDirectory (
		NormalizeFilename ( targetDirectory + sSep + targetFilename ),
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
			const Module& aliasedModule = backend->GetAliasedModuleOrModule ( m  );
			string sourceFilename = MingwModuleHandler::PassThruCacheDirectory (
				NormalizeFilename ( aliasedModule.GetPath () ),
				backend->outputDirectory );
			OutputCopyCommand ( sourceFilename,
			                    m.installName,
			                    livecdDirectory + sSep + reactosDirectory + sSep + m.installBase );
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
	                        livecdDirectory + sSep + reactosDirectory + sSep + installfile.base );
	}
}

void
MingwLiveIsoModuleHandler::OutputProfilesDirectoryCommands ( string& livecdDirectory )
{
	CreateDirectory ( livecdDirectory + sSep + "Profiles" );
	CreateDirectory ( livecdDirectory + sSep + "Profiles" + sSep + "All Users") ;
	CreateDirectory ( livecdDirectory + sSep + "Profiles" + sSep + "All Users" + sSep + "Desktop" );
	CreateDirectory ( livecdDirectory + sSep + "Profiles" + sSep + "Default User" );
	CreateDirectory ( livecdDirectory + sSep + "Profiles" + sSep + "Default User" + sSep + "Desktop" );
	CreateDirectory ( livecdDirectory + sSep + "Profiles" + sSep + "Default User" + sSep + "My Documents" );

	string livecdIni = "boot" + sSep + "bootdata" + sSep + "livecd.ini";
	OutputCopyCommand ( livecdIni,
	                    "freeldr.ini",
	                    livecdDirectory );
}

void
MingwLiveIsoModuleHandler::OutputLoaderCommands ( string& livecdDirectory )
{
	string freeldr = PassThruCacheDirectory (
		NormalizeFilename ( "boot" + sSep + "freeldr" + sSep + "freeldr" + sSep + "freeldr.sys" ),
		backend->outputDirectory );
	CreateDirectory ( livecdDirectory + sSep + "loader" );
	OutputCopyCommand ( freeldr,
	                   "setupldr.sys",
	                   livecdDirectory + sSep + "loader" );
}

void
MingwLiveIsoModuleHandler::OutputRegistryCommands ( string& livecdDirectory )
{
	string reactosSystem32ConfigDirectory = NormalizeFilename (
		MingwModuleHandler::PassThruCacheDirectory (
		livecdDirectory + sSep + "reactos" + sSep + "system32" + sSep + "config" + sSep,
		backend->outputDirectory ) );
	fprintf ( fMakefile,
	          "\t$(ECHO_MKHIVE)\n" );
	fprintf ( fMakefile,
	          "\t$(MKHIVE_TARGET) boot%cbootdata %s boot%cbootdata%clivecd.inf boot%cbootdata%chiveinst.inf\n",
	          cSep, reactosSystem32ConfigDirectory.c_str (),
	          cSep, cSep, cSep, cSep );
}

void
MingwLiveIsoModuleHandler::GenerateLiveIsoModuleTarget ()
{
	string livecdDirectory = module.name;
	string livecd = PassThruCacheDirectory (
		NormalizeFilename ( livecdDirectory + sSep ),
		backend->outputDirectory );

	string bootloader;
	string IsoName;

	if (module.name == "livecdregtest")
	{
		bootloader = "isobtrt.o";
		IsoName = "ReactOS-LiveCD-RegTest.iso";
	}
	else
	{
		bootloader = "isoboot.o";
		IsoName = "ReactOS-LiveCD.iso";
	}

	string isoboot = PassThruCacheDirectory (
		NormalizeFilename ( "boot" + sSep + "freeldr" + sSep + "bootsect" + sSep + bootloader.c_str() ),
		backend->outputDirectory );

	string reactosDirectory = "reactos";
	string livecdReactosNoFixup = livecdDirectory + sSep + reactosDirectory;
	string livecdReactos = NormalizeFilename ( PassThruCacheDirectory (
		NormalizeFilename ( livecdReactosNoFixup + sSep ),
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
	          "\t$(Q)$(CDMAKE_TARGET) -v -m -j -b %s %s REACTOS %s\n",
	          isoboot.c_str (),
	          livecd.c_str (),
			  IsoName.c_str() );
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
MingwTestModuleHandler::GetModuleSpecificCompilationUnits ( vector<CompilationUnit*>& compilationUnits )
{
	string basePath = "$(INTERMEDIATE)" + sSep + module.GetBasePath ();
	compilationUnits.push_back ( new CompilationUnit ( new File ( basePath + sSep + "_hooks.c", false, "", false ) ) );
	compilationUnits.push_back ( new CompilationUnit ( new File ( basePath + sSep + "_stubs.S", false, "", false ) ) );
	compilationUnits.push_back ( new CompilationUnit ( new File ( basePath + sSep + "_startup.c", false, "", false ) ) );
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

	if ( module.non_if_data.compilationUnits.size () > 0 )
	{
		GenerateRules ();

		string dependencies = linkDepsMacro + " " + objectsMacro;

		string linker;
		if ( module.cplusplus )
			linker = "${gpp}";
		else
			linker = "${gcc}";

		string linkerParameters = ssprintf ( "-Wl,--subsystem,console -Wl,--entry,%s -Wl,--image-base,%s -Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000",
		                                     module.GetEntryPoint(true).c_str (),
		                                     module.baseaddress.c_str () );
		GenerateLinkerCommand ( dependencies,
		                        linker,
		                        linkerParameters,
		                        objectsMacro,
		                        libsMacro,
		                        "" );
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


MingwAliasModuleHandler::MingwAliasModuleHandler (
	const Module& module_ )

	: MingwModuleHandler ( module_ )
{
}

void
MingwAliasModuleHandler::Process ()
{
}

MingwIdlHeaderModuleHandler::MingwIdlHeaderModuleHandler (
	const Module& module_ )

	: MingwModuleHandler ( module_ )
{
}

void
MingwIdlHeaderModuleHandler::Process ()
{
	GenerateRules ();
}
