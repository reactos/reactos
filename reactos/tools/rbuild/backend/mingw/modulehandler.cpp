/*
 * Copyright (C) 2005 Casper S. Hornstrup
 *               2007-2008 Hervé Poussineau
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
#include <algorithm>

#include "../../rbuild.h"
#include "mingw.h"
#include "modulehandler.h"
#include "rule.h"

using std::set;
using std::string;
using std::vector;

#define CLEAN_FILE(f) clean_files.push_back ( (f).name.length () > 0 ? backend->GetFullName ( f ) : backend->GetFullPath ( f ) );
#define IsStaticLibrary( module ) ( ( module.type == StaticLibrary ) || ( module.type == HostStaticLibrary ) )

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

/*static*/ const FileLocation*
MingwModuleHandler::PassThruCacheDirectory (const FileLocation* file )
{
	switch ( file->directory )
	{
		case SourceDirectory:
			break;
		case IntermediateDirectory:
			backend->AddDirectoryTarget ( file->relative_path, backend->intermediateDirectory );
			break;
		case OutputDirectory:
			backend->AddDirectoryTarget ( file->relative_path, backend->outputDirectory );
			break;
		case InstallDirectory:
			backend->AddDirectoryTarget ( file->relative_path, backend->installDirectory );
			break;
		default:
			throw InvalidOperationException ( __FILE__,
			                                  __LINE__,
			                                  "Invalid directory %d.",
			                                  file->directory );
	}

	return file;
}

/* caller needs to delete the returned object */
const FileLocation*
MingwModuleHandler::GetTargetFilename (
	const Module& module,
	string_list* pclean_files )
{
	FileLocation *target = new FileLocation ( *module.output );
	if ( pclean_files )
	{
		string_list& clean_files = *pclean_files;
		CLEAN_FILE ( *target );
	}
	return target;
}

/* caller needs to delete the returned object */
const FileLocation*
MingwModuleHandler::GetImportLibraryFilename (
	const Module& module,
	string_list* pclean_files )
{
	FileLocation *target = new FileLocation ( *module.dependency );
	if ( pclean_files )
	{
		string_list& clean_files = *pclean_files;
		CLEAN_FILE ( *target );
	}
	return target;
}

/* caller needs to delete the returned object */
MingwModuleHandler*
MingwModuleHandler::InstanciateHandler (
	const Module& module,
	MingwBackend* backend )
{
	MingwModuleHandler* handler;
	switch ( module.type )
	{
		case StaticLibrary:
		case HostStaticLibrary:
		case ObjectLibrary:
		case RpcServer:
		case RpcClient:
		case RpcProxy:
		case MessageHeader:
		case IdlHeader:
		case IdlInterface:
		case EmbeddedTypeLib:
		case BootSector:
			handler = new MingwModuleHandler( module );
			break;
		case BuildTool:
			handler = new MingwBuildToolModuleHandler ( module );
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
		case KeyboardLayout:
		case KernelModeDLL:
		case KernelModeDriver:
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
		case BootLoader:
			handler = new MingwBootLoaderModuleHandler ( module );
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
		case Alias:
			handler = new MingwAliasModuleHandler ( module );
			break;
		case Cabinet:
			handler = new MingwCabinetModuleHandler ( module );
			break;
		case ElfExecutable:
			handler = new MingwElfExecutableModuleHandler ( module );
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
MingwModuleHandler::GetCompilationUnitDependencies (
	const CompilationUnit& compilationUnit ) const
{
	if ( compilationUnit.GetFiles ().size () <= 1 )
		return "";
	vector<string> sourceFiles;
	for ( size_t i = 0; i < compilationUnit.GetFiles ().size (); i++ )
	{
		const File& file = *compilationUnit.GetFiles ()[i];
		sourceFiles.push_back ( backend->GetFullName ( file.file ) );
	}
	return string ( " " ) + v2s ( sourceFiles, 10 );
}

/* caller needs to delete the returned object */
const FileLocation*
MingwModuleHandler::GetModuleArchiveFilename () const
{
	if ( IsStaticLibrary ( module ) )
		return GetTargetFilename ( module, NULL );
	return new FileLocation ( IntermediateDirectory,
	                          module.output->relative_path,
	                          ReplaceExtension ( module.name, ".temp.a" ) );
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
	if ( module.type == RpcProxy )
		return true;
	if ( module.type == IdlHeader )
		return true;
	if ( module.type == IdlInterface )
		return true;
	if ( module.type == MessageHeader)
		return true;
	return false;
}

void
MingwModuleHandler::OutputCopyCommand ( const FileLocation& source,
                                        const FileLocation& destination )
{
	fprintf ( fMakefile, "# OUTPUT COPY COMMAND\n" );
	fprintf ( fMakefile,
	          "\t$(ECHO_CP)\n" );
	fprintf ( fMakefile,
	          "\t${cp} %s %s 1>$(NUL)\n",
	          backend->GetFullName ( source ).c_str (),
	          backend->GetFullName ( *PassThruCacheDirectory ( &destination ) ).c_str () );
}

string
MingwModuleHandler::GetImportLibraryDependency (
	const Module& importedModule )
{
	string dep;
	if ( ReferenceObjects ( importedModule ) )
	{
		const vector<CompilationUnit*>& compilationUnits = importedModule.non_if_data.compilationUnits;
		size_t i;

		dep = GetTargetMacro ( importedModule );
		for ( i = 0; i < compilationUnits.size (); i++ )
		{
			CompilationUnit& compilationUnit = *compilationUnits[i];
			const FileLocation& compilationName = compilationUnit.GetFilename ();
			const FileLocation *objectFilename = GetObjectFilename ( &compilationName, importedModule );
			if ( GetExtension ( *objectFilename ) == ".h" )
				dep += ssprintf ( " $(%s_HEADERS)", importedModule.name.c_str () );
			else if ( GetExtension ( *objectFilename ) == ".rc" )
				dep += ssprintf ( " $(%s_MCHEADERS)", importedModule.name.c_str () );
		}
	}
	else
	{
		const FileLocation *library_target = GetImportLibraryFilename ( importedModule, NULL );
		dep = backend->GetFullName ( *library_target );
		delete library_target;
	}

	if ( IsStaticLibrary ( importedModule ) || importedModule.type == ObjectLibrary )
	{
		const std::vector<Library*>& libraries = importedModule.non_if_data.libraries;

		for ( size_t i = 0; i < libraries.size (); ++ i )
		{
			dep += " ";
			dep += GetImportLibraryDependency ( *libraries[i]->importedModule );
		}
	}

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
	vector<FileLocation> v;
	GetDefinitionDependencies ( v );

	for ( size_t i = 0; i < v.size (); i++ )
	{
		const FileLocation& file = v[i];
		dependencies.push_back ( backend->GetFullName ( file ) );
	}
}

/* caller needs to delete the returned object */
const FileLocation*
MingwModuleHandler::GetObjectFilename (
	const FileLocation* sourceFile,
	const Module& module ) const
{
	DirectoryLocation destination_directory;
	string newExtension;
	string extension = GetExtension ( *sourceFile );

	if ( module.type == BootSector )
		return new FileLocation ( *module.output );
	else if (extension == ".rc")
		newExtension = "_" + module.name + ".coff";
	else if (extension == ".mc")
		newExtension = ".rc";
	else if (extension == ".idl")
	{
		if ( module.type == RpcServer )
			newExtension = "_s.o";
		else if ( module.type == RpcClient )
			newExtension = "_c.o";
		else if ( module.type == RpcProxy )
			newExtension = "_p.o";
		else if ( module.type == IdlInterface )
			newExtension = "_i.o";
		else
			newExtension = ".h";
	}
	else
		newExtension = "_" + module.name + ".o";

	if ( module.type == BootSector )
		destination_directory = OutputDirectory;
	else
		destination_directory = IntermediateDirectory;

	const FileLocation *obj_file = new FileLocation(
		destination_directory,
		sourceFile->relative_path,
		ReplaceExtension ( sourceFile->name, newExtension ) );
	PassThruCacheDirectory ( obj_file );

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

	fprintf ( fMakefile, "# CLEAN TARGET\n" );
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
		if ( ( i + 1 ) % 10 == 9 )
			fprintf ( fMakefile, " 2>$(NUL)\n\t-@${rm}" );
		fprintf ( fMakefile, " %s", clean_files[i].c_str() );
	}
	fprintf ( fMakefile, " 2>$(NUL)\n" );

	if( ProxyMakefile::GenerateProxyMakefile(module) )
	{
		DirectoryLocation root;

		if ( backend->configuration.GenerateProxyMakefilesInSourceTree )
			root = SourceDirectory;
		else
			root = OutputDirectory;

		FileLocation proxyMakefile ( root,
		                             module.output->relative_path,
		                            "GNUmakefile" );
		fprintf ( fMakefile, "\t-@${rm} %s 2>$(NUL)\n",
		          backend->GetFullName ( proxyMakefile ).c_str () );
	}

	fprintf ( fMakefile, "clean: %s_clean\n\n", module.name.c_str() );
}

void
MingwModuleHandler::GenerateInstallTarget () const
{
	if ( !module.install )
		return;
	fprintf ( fMakefile, "# INSTALL TARGET\n" );
	fprintf ( fMakefile, ".PHONY: %s_install\n", module.name.c_str() );
	fprintf ( fMakefile,
	          "%s_install: %s\n",
	          module.name.c_str (),
	          backend->GetFullName ( *module.install ).c_str () );
}

void
MingwModuleHandler::GenerateDependsTarget () const
{
	fprintf ( fMakefile, "# DEPENDS TARGET\n" );
	fprintf ( fMakefile,
	          ".PHONY: %s_depends\n",
	          module.name.c_str() );
	fprintf ( fMakefile,
	          "%s_depends: $(RBUILD_TARGET)\n",
	          module.name.c_str () );
	fprintf ( fMakefile,
	          "\t$(ECHO_RBUILD)\n" );
	fprintf ( fMakefile,
	          "\t$(Q)$(RBUILD_TARGET) $(RBUILD_FLAGS) -dm%s mingw\n",
	          module.name.c_str () );
}

static
const char * const CompilerPrefixTable [ CompilerTypesCount ] =
{
	"C",
	"CXX",
	"CPP",
	"AS",
	"MIDL",
	"RC",
	"NASM",
};

/* static */ void
MingwModuleHandler::GenerateParameters (
	const char* prefix,
	const char* assignmentOperation,
	const IfableData& data )
{
	for ( unsigned type = CompilerTypeCC; type < CompilerTypesCount; ++ type )
	{
		CompilerType compiler = static_cast < CompilerType > ( type );

		// Includes
		std::string includes = GenerateIncludeParametersFromVector ( data.includes, compiler );

		if ( includes.size() )
		{
			fprintf ( fMakefile,
					  "%s_%sINCLUDES%s%s\n",
					  prefix,
					  CompilerPrefixTable [ compiler ],
					  assignmentOperation,
					  includes.c_str () );
		}

		// Defines
		std::string defines = GenerateDefineParametersFromVector ( data.defines, compiler );

		if ( defines.size() )
		{
			fprintf ( fMakefile,
					  "%s_%sDEFINES%s%s\n",
					  prefix,
					  CompilerPrefixTable [ compiler ],
					  assignmentOperation,
					  defines.c_str () );
		}

		// Flags
		std::string flags = GenerateCompilerParametersFromVector ( data.compilerFlags, compiler );

		if ( flags.size() )
		{
			fprintf ( fMakefile,
					  "%s_%sFLAGS%s%s\n",
					  prefix,
					  CompilerPrefixTable [ compiler ],
					  assignmentOperation,
					  flags.c_str () );
		}
	}
}

/* static */ string
MingwModuleHandler::GenerateGccDefineParametersFromVector (
	const vector<Define*>& defines,
	set<string>& used_defs)
{
	string parameters;

	for ( size_t i = 0; i < defines.size (); i++ )
	{
		Define& define = *defines[i];
		if (used_defs.find(define.name) != used_defs.end())
			continue;
		if (define.redefine)
		{
			if (parameters.length () > 0)
				parameters += " ";
			parameters += "-U";
			parameters += define.name;
		}
		if (parameters.length () > 0)
			parameters += " ";
		if (define.arguments.length ())
			parameters += "$(QT)";
		parameters += "-D";
		parameters += define.name;
		parameters += define.arguments;
		if (define.value.length () > 0)
		{
			parameters += "=";
			parameters += define.value;
		}
		if (define.arguments.length ())
			parameters += "$(QT)";
		used_defs.insert(used_defs.begin(),define.name);
	}
	return parameters;
}

/* static */ string
MingwModuleHandler::GenerateDefineParametersFromVector (
	const std::vector<Define*>& defines,
	CompilerType compiler )
{
	string parameters;

	for ( size_t i = 0; i < defines.size (); i++ )
	{
		Define& define = *defines[i];
		if (!define.IsCompilerSet (compiler))
			continue;
		if (define.redefine)
		{
			if (parameters.length () > 0)
				parameters += " ";
			parameters += "-U";
			parameters += define.name;
		}
		if (parameters.length () > 0)
			parameters += " ";
		if (define.arguments.length ())
			parameters += "$(QT)";
		parameters += "-D";
		parameters += define.name;
		parameters += define.arguments;
		if (define.value.length () > 0)
		{
			parameters += "=";
			parameters += define.value;
		}
		if (define.arguments.length ())
			parameters += "$(QT)";
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
MingwModuleHandler::GenerateIncludeParametersFromVector ( const vector<Include*>& includes, const CompilerType type )
{
	string parameters, path_prefix;
	for ( size_t i = 0; i < includes.size (); i++ )
	{
		Include& include = *includes[i];
		if ( include.IsCompilerSet( type ) )
			parameters += " -I" + backend->GetFullPath ( *include.directory );
	}
	return parameters;
}

/* static */ string
MingwModuleHandler::GenerateCompilerParametersFromVector ( const vector<CompilerFlag*>& compilerFlags, const CompilerType type )
{
	string parameters;
	for ( size_t i = 0; i < compilerFlags.size (); i++ )
	{
		CompilerFlag& compilerFlag = *compilerFlags[i];
		if ( compilerFlag.IsCompilerSet( type ) )
			parameters += " " + compilerFlag.flag;
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
MingwModuleHandler::GenerateMacros (
	const char* assignmentOperation,
	const IfableData& data,
	const vector<LinkerFlag*>* linkerFlags,
	set<const Define *>& used_defs )
{
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
		// Check if host and target modules are not mixed up
		HostType current = ModuleHandlerInformations[module.type].DefaultHost;
		std::vector<Library*>::const_iterator it;
		for ( it = data.libraries.begin(); it != data.libraries.end(); ++it )
		{
			HostType imported = ModuleHandlerInformations[(*it)->importedModule->type].DefaultHost;
			if (current != imported)
			{
				throw InvalidOperationException ( __FILE__,
				                                  __LINE__,
				                                  "Module '%s' imports module '%s', which is not of the right type",
				                                  module.name.c_str (),
				                                  (*it)->importedModule->name.c_str () );
			}
		}

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
}

void
MingwModuleHandler::CleanupCompilationUnitVector ( vector<CompilationUnit*>& compilationUnits )
{
	for ( size_t i = 0; i < compilationUnits.size (); i++ )
		delete compilationUnits[i];
}

void
MingwModuleHandler::GetModuleSpecificCompilationUnits ( vector<CompilationUnit*>& compilationUnits )
{
}

void
MingwModuleHandler::GenerateSourceMacros (
	const IfableData& data )
{
	size_t i;

	const vector<CompilationUnit*>& compilationUnits = data.compilationUnits;
	vector<const FileLocation *> headers;
	if ( compilationUnits.size () > 0 )
	{
		fprintf (
			fMakefile,
			"%s =",
			sourcesMacro.c_str () );
		for ( i = 0; i < compilationUnits.size(); i++ )
		{
			CompilationUnit& compilationUnit = *compilationUnits[i];
			const FileLocation& compilationName = compilationUnit.GetFilename ();
			fprintf (
				fMakefile,
				"%s%s",
				( i%10 == 9 ? " \\\n\t" : " " ),
				backend->GetFullName ( compilationName ).c_str () );
		}
		fprintf ( fMakefile, "\n" );
	}

	vector<CompilationUnit*> sourceCompilationUnits;
	GetModuleSpecificCompilationUnits ( sourceCompilationUnits );
	for ( i = 0; i < sourceCompilationUnits.size (); i++ )
	{
		const FileLocation& compilationName = sourceCompilationUnits[i]->GetFilename ();
		fprintf (
			fMakefile,
			"%s += %s\n",
			sourcesMacro.c_str(),
			backend->GetFullName ( compilationName ).c_str () );
	}
	CleanupCompilationUnitVector ( sourceCompilationUnits );
}

void
MingwModuleHandler::GenerateObjectMacros (
	const IfableData& data )
{
	size_t i;
	const char* assignmentOperation = "=";

	const vector<CompilationUnit*>& compilationUnits = data.compilationUnits;
	vector<const FileLocation *> headers;
	vector<const FileLocation *> mcheaders;
	vector<const FileLocation *> mcresources;
	if ( compilationUnits.size () > 0 )
	{
		for ( i = 0; i < compilationUnits.size (); i++ )
		{
			CompilationUnit& compilationUnit = *compilationUnits[i];
			if ( compilationUnit.IsFirstFile () )
			{
				const FileLocation& compilationName = compilationUnit.GetFilename ();
				const FileLocation *object_file = GetObjectFilename ( &compilationName, module );
				fprintf ( fMakefile,
					"%s := %s\n",
					objectsMacro.c_str(),
					backend->GetFullName ( *object_file ).c_str () );
				delete object_file;
				assignmentOperation = "+=";
				break;
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
				const FileLocation& compilationName = compilationUnit.GetFilename ();
				const FileLocation *objectFilename = GetObjectFilename ( &compilationName, module );
				if ( GetExtension ( *objectFilename ) == ".h" )
					headers.push_back ( objectFilename );
				else if ( GetExtension ( *objectFilename ) == ".rc" )
				{
					const FileLocation *headerFilename = GetMcHeaderFilename ( &compilationUnit.GetFilename () );
					mcheaders.push_back ( headerFilename );
					mcresources.push_back ( objectFilename );
				}
				else
				{
					fprintf (
						fMakefile,
						"%s%s",
						( i%10 == 9 ? " \\\n\t" : " " ),
						backend->GetFullName ( *objectFilename ).c_str () );
					delete objectFilename;
				}
			}
		}
		fprintf ( fMakefile, "\n" );
	}
	if ( headers.size () > 0 )
	{
		fprintf (
			fMakefile,
			"%s_HEADERS %s",
			module.name.c_str (),
			assignmentOperation );
		for ( i = 0; i < headers.size (); i++ )
		{
			fprintf (
				fMakefile,
				"%s%s",
				( i%10 == 9 ? " \\\n\t" : " " ),
				backend->GetFullName ( *headers[i] ).c_str () );
			delete headers[i];
		}
		fprintf ( fMakefile, "\n" );
	}

	if ( mcheaders.size () > 0 )
	{
		fprintf (
			fMakefile,
			"%s_MCHEADERS %s",
			module.name.c_str (),
			assignmentOperation );
		for ( i = 0; i < mcheaders.size (); i++ )
		{
			fprintf (
				fMakefile,
				"%s%s",
				( i%10 == 9 ? " \\\n\t" : " " ),
				backend->GetFullName ( *mcheaders[i] ).c_str () );
			delete mcheaders[i];
		}
		fprintf ( fMakefile, "\n" );
	}

	if ( mcresources.size () > 0 )
	{
		fprintf (
			fMakefile,
			"%s_RESOURCES %s",
			module.name.c_str (),
			assignmentOperation );
		for ( i = 0; i < mcresources.size (); i++ )
		{
			fprintf (
				fMakefile,
				"%s%s",
				( i%10 == 9 ? " \\\n\t" : " " ),
				backend->GetFullName ( *mcresources[i] ).c_str () );
			delete mcresources[i];
		}
		fprintf ( fMakefile, "\n" );
	}

	vector<CompilationUnit*> sourceCompilationUnits;
	GetModuleSpecificCompilationUnits ( sourceCompilationUnits );
	for ( i = 0; i < sourceCompilationUnits.size (); i++ )
	{
		const FileLocation& compilationName = sourceCompilationUnits[i]->GetFilename ();
		const FileLocation *object_file = GetObjectFilename ( &compilationName, module );
		fprintf (
			fMakefile,
			"%s += %s\n",
			objectsMacro.c_str(),
			backend->GetFullName ( *object_file ).c_str () );
		delete object_file;
	}
	CleanupCompilationUnitVector ( sourceCompilationUnits );

	if ( IsSpecDefinitionFile() )
	{
		const FileLocation *stubs_file = new FileLocation(
			IntermediateDirectory,
			module.importLibrary->source->relative_path,
			ReplaceExtension ( module.importLibrary->source->name, "_" + module.name + ".stubs.o" ) );

		fprintf (
			fMakefile,
			"%s += %s\n",
			objectsMacro.c_str(),
			backend->GetFullName ( *stubs_file ).c_str () );

		delete stubs_file;
	}

	if ( module.type == RpcProxy )
	{
		const FileLocation *dlldata_file = GetDlldataFilename();

		fprintf (
			fMakefile,
			"%s += %s\n",
			objectsMacro.c_str(),
			ReplaceExtension ( backend->GetFullName ( *dlldata_file ), ".o" ).c_str() );

		delete dlldata_file;
	}
}

const FileLocation*
MingwModuleHandler::GetDlldataFilename() const
{
	std::string dlldata_path = "";
	size_t dlldata_path_len = module.xmlbuildFile.find_last_of(cSep);

	if ( dlldata_path_len != std::string::npos && dlldata_path_len != 0 )
		dlldata_path = module.xmlbuildFile.substr(0, dlldata_path_len);

	return new FileLocation( IntermediateDirectory, dlldata_path, module.name + ".dlldata.c" );
}

const FileLocation*
MingwModuleHandler::GetPrecompiledHeaderPath () const
{
	if ( !module.pch || !use_pch )
		return NULL;
	return new FileLocation ( IntermediateDirectory,
	                          module.pch->file->relative_path,
	                          ".gch_" + module.name );
}

const FileLocation*
MingwModuleHandler::GetPrecompiledHeaderFilename () const
{
	if ( !module.pch || !use_pch )
		return NULL;
	return new FileLocation ( IntermediateDirectory,
	                          module.pch->file->relative_path + "/.gch_" + module.name,
	                          module.pch->file->name + ".gch" );
}

Rule windresRule ( "$(eval $(call RBUILD_WRC_RULE,$(module_name),$(source),$(dependencies),$(compiler_flags)))\n",
                   "$(intermediate_path_unique).coff",
                   "$(intermediate_path_unique).res",
                   "$(intermediate_path_unique).res.d",
                   "$(intermediate_dir)$(SEP)", NULL );
Rule winebuildPRule ( "$(eval $(call RBUILD_WINEBUILD_WITH_CPP_RULE,$(module_name),$(source),$(dependencies),$(compiler_flags),$(module_dllname)))\n",
                      "$(intermediate_path_unique).spec",
                      "$(intermediate_path_unique).spec.d",
                      "$(intermediate_path_unique).auto.def",
                      "$(intermediate_path_unique).stubs.c",
                      "$(intermediate_path_unique).stubs.o",
                      "$(intermediate_path_unique).stubs.o.d",
                      "$(intermediate_dir)$(SEP)", NULL );
Rule winebuildRule ( "$(eval $(call RBUILD_WINEBUILD_RULE,$(module_name),$(source),$(dependencies),$(compiler_flags),$(module_dllname)))\n",
                     "$(intermediate_path_unique).auto.def",
                     "$(intermediate_path_unique).stubs.c",
                     "$(intermediate_path_unique).stubs.o",
                     "$(intermediate_path_unique).stubs.o.d",
                     "$(intermediate_dir)$(SEP)", NULL );
Rule gasRule ( "$(eval $(call RBUILD_GAS_RULE,$(module_name),$(source),$(dependencies),$(compiler_flags)))\n",
               "$(intermediate_path_unique).o",
               "$(intermediate_path_unique).o.d", NULL );
Rule gccRule ( "$(eval $(call RBUILD_CC_RULE,$(module_name),$(source),$(dependencies),$(compiler_flags)))\n",
               "$(intermediate_path_unique).o",
               "$(intermediate_path_unique).o.d", NULL );
Rule gccHostRule ( "$(eval $(call RBUILD_HOST_GCC_RULE,$(module_name),$(source),$(dependencies),$(compiler_flags)))\n",
                   "$(intermediate_path_unique).o", NULL );
Rule gppRule ( "$(eval $(call RBUILD_CXX_RULE,$(module_name),$(source),$(dependencies),$(compiler_flags)))\n",
               "$(intermediate_path_unique).o",
               "$(intermediate_path_unique).o.d", NULL );
Rule gppHostRule ( "$(eval $(call RBUILD_HOST_GPP_RULE,$(module_name),$(source),$(dependencies),$(compiler_flags)))\n",
                   "$(intermediate_path_unique).o", NULL );
Rule widlHeaderRule ( "$(eval $(call RBUILD_WIDL_HEADER_RULE,$(module_name),$(source),$(dependencies),$(compiler_flags)))\n",
                      "$(intermediate_path_noext).h",
                      "$(intermediate_dir)$(SEP)", NULL );
Rule widlServerRule ( "$(eval $(call RBUILD_WIDL_SERVER_RULE,$(module_name),$(source),$(dependencies),$(compiler_flags)))\n",
                      "$(intermediate_path_noext)_s.h",
                      "$(intermediate_path_noext)_s.c",
                      "$(intermediate_path_noext)_s.o",
                      "$(intermediate_dir)$(SEP)", NULL );
Rule widlClientRule ( "$(eval $(call RBUILD_WIDL_CLIENT_RULE,$(module_name),$(source),$(dependencies),$(compiler_flags)))\n",
                      "$(intermediate_path_noext)_c.h",
                      "$(intermediate_path_noext)_c.c",
                      "$(intermediate_path_noext)_c.o",
                      "$(intermediate_dir)$(SEP)", NULL );
Rule widlProxyRule ( "$(eval $(call RBUILD_WIDL_PROXY_RULE,$(module_name),$(source),$(dependencies),$(compiler_flags)))\n",
                     "$(intermediate_path_noext)_p.h",
                     "$(intermediate_path_noext)_p.c",
                     "$(intermediate_path_noext)_p.o",
                     "$(intermediate_dir)$(SEP)", NULL );
Rule widlInterfaceRule ( "$(eval $(call RBUILD_WIDL_INTERFACE_RULE,$(module_name),$(source),$(dependencies),$(compiler_flags)))\n",
                       "$(intermediate_path_noext)_i.c",
                       "$(intermediate_path_noext)_i.o",
                       "$(intermediate_dir)$(SEP)", NULL );
Rule widlDlldataRule ( "$(eval $(call RBUILD_WIDL_DLLDATA_RULE,$(module_name),$(source),$(dependencies),$(compiler_flags),$(bare_dependencies)))\n",
                       "$(intermediate_path_noext).o", NULL );
Rule widlTlbRule ( "$(eval $(call RBUILD_WIDL_TLB_RULE,$(module_name),$(source),$(dependencies),$(compiler_flags)))\n",
                   "$(intermediate_dir)$(SEP)", NULL );
Rule pchRule ( "$(eval $(call RBUILD_CC_PCH_RULE,$(module_name),$(source),$(dependencies),$(compiler_flags)))\n",
			   "$(intermediate_dir)$(SEP).gch_$(module_name)$(SEP)$(source_name).gch",
			   "$(intermediate_dir)$(SEP).gch_$(module_name)$(SEP)$(source_name).gch.d",
			   "$(intermediate_dir)$(SEP).gch_$(module_name)$(SEP)", NULL );
Rule pchCxxRule ( "$(eval $(call RBUILD_CXX_PCH_RULE,$(module_name),$(source),$(dependencies),$(compiler_flags)))\n",
			      "$(intermediate_dir)$(SEP).gch_$(module_name)$(SEP)$(source_name).gch",
			      "$(intermediate_dir)$(SEP).gch_$(module_name)$(SEP)$(source_name).gch.d",
			      "$(intermediate_dir)$(SEP).gch_$(module_name)$(SEP)", NULL );
Rule bootRule ( "$(eval $(call RBUILD_NASM,$(module_name),$(source),$(dependencies),,$(module_output)))\n",
                "$(module_output)",
                "$(OUTPUT)$(SEP)$(source_dir)$(SEP)", NULL );
Rule nasmRule ( "$(eval $(call RBUILD_NASM,$(module_name),$(source),$(dependencies),,$(intermediate_path_unique).o))\n",
                "$(intermediate_path_unique).o",
                "$(intermediate_dir)$(SEP)", NULL );

/* TODO: move these to rules.mak */
Rule wmcRule ( "$(intermediate_path_noext).rc $(INTERMEDIATE)$(SEP)include$(SEP)reactos$(SEP)$(source_name_noext).h: $(WMC_TARGET) $(source) | $(intermediate_dir)\n"
               "\t$(ECHO_WMC)\n"
               "\t$(Q)$(WMC_TARGET) -i -H $(INTERMEDIATE)$(SEP)include$(SEP)reactos$(SEP)$(source_name_noext).h -o $(intermediate_path_noext).rc $(source)\n",
               "$(intermediate_path_noext).rc",
               "$(INTERMEDIATE)$(SEP)include$(SEP)reactos$(SEP)$(source_name_noext).h",
               "$(intermediate_dir)$(SEP)", NULL );
/* TODO: if possible, move these to rules.mak */
Rule arRule1 ( "$(intermediate_path_noext).a: $($(module_name)_OBJS)  $(dependencies) | $(intermediate_dir)\n",
               "$(intermediate_path_noext).a",
               "$(intermediate_dir)$(SEP)", NULL );
Rule arRule2 ( "\t$(ECHO_AR)\n"
              "\t${ar} -rc $@ $($(module_name)_OBJS)\n",
              NULL );
Rule arHostRule2 ( "\t$(ECHO_HOSTAR)\n"
                   "\t${host_ar} -rc $@ $($(module_name)_OBJS)\n",
                   NULL );

Rule emptyRule ( "", NULL );

void
MingwModuleHandler::GenerateGccCommand (
	const FileLocation* sourceFile,
	const Rule *rule,
	const string& extraDependencies )
{
	const FileLocation *pchFilename = GetPrecompiledHeaderFilename ();
	string dependencies = extraDependencies;

	if ( pchFilename )
	{
		dependencies += " " + backend->GetFullName ( *pchFilename );
		delete pchFilename;
	}

	/* WIDL generated headers may be used */
	vector<FileLocation> rpcDependencies;
	GetRpcHeaderDependencies ( rpcDependencies );
	if ( rpcDependencies.size () > 0 )
		dependencies += " " + v2s ( backend, rpcDependencies, 5 );

	rule->Execute ( fMakefile, backend, module, sourceFile, clean_files, dependencies );
}

string
MingwModuleHandler::GetPropertyValue ( const Module& module, const std::string& name )
{
	const Property* property = module.project.LookupProperty(name);

	if (property)
		return property->value;
	else
		return string ( "" );
}

/* caller needs to delete the returned object */
const FileLocation*
MingwModuleHandler::GetRpcServerHeaderFilename ( const FileLocation *base ) const
{
	string newname = GetBasename ( base->name ) + "_s.h";
	return new FileLocation ( IntermediateDirectory, base->relative_path, newname );
}

/* caller needs to delete the returned object */
const FileLocation*
MingwModuleHandler::GetRpcClientHeaderFilename ( const FileLocation *base ) const
{
	string newname = GetBasename ( base->name ) + "_c.h";
	return new FileLocation ( IntermediateDirectory, base->relative_path, newname );
}

/* caller needs to delete the returned object */
const FileLocation*
MingwModuleHandler::GetRpcProxyHeaderFilename ( const FileLocation *base ) const
{
	string newname = GetBasename ( base->name ) + "_p.h";
	return new FileLocation ( IntermediateDirectory, base->relative_path, newname );
}

/* caller needs to delete the returned object */
const FileLocation*
MingwModuleHandler::GetIdlHeaderFilename ( const FileLocation *base ) const
{
	string newname = GetBasename ( base->name ) + ".h";
	return new FileLocation ( IntermediateDirectory, base->relative_path, newname );
}

/* caller needs to delete the returned object */
const FileLocation*
MingwModuleHandler::GetMcHeaderFilename ( const FileLocation *base ) const
{
	string newname = GetBasename ( base->name ) + ".h";
	return new FileLocation ( IntermediateDirectory, "include/reactos" , newname );
}

void
MingwModuleHandler::GenerateCommands (
	const CompilationUnit& compilationUnit,
	const string& extraDependencies )
{
	const FileLocation& sourceFile = compilationUnit.GetFilename ();
	string extension = GetExtension ( sourceFile );
	std::transform ( extension.begin (), extension.end (), extension.begin (), tolower );

	struct
	{
		HostType host;
		ModuleType type;
		string extension;
		Rule* rule;
	} rules[] = {
		{ HostDontCare, TypeDontCare, ".s", &gasRule },
		{ HostDontCare, BootSector, ".asm", &bootRule },
		{ HostDontCare, TypeDontCare, ".asm", &nasmRule },
		{ HostDontCare, TypeDontCare, ".rc", &windresRule },
		{ HostDontCare, TypeDontCare, ".mc", &wmcRule },
		{ HostDontCare, RpcServer, ".idl", &widlServerRule },
		{ HostDontCare, RpcClient, ".idl", &widlClientRule },
		{ HostDontCare, RpcProxy, ".idl", &widlProxyRule },
		{ HostDontCare, IdlInterface, ".idl", &widlInterfaceRule },
		{ HostDontCare, EmbeddedTypeLib, ".idl", &widlTlbRule },
		{ HostDontCare, TypeDontCare, ".idl", &widlHeaderRule },
		{ HostTrue, TypeDontCare, ".c", &gccHostRule },
		{ HostTrue, TypeDontCare, ".cc", &gppHostRule },
		{ HostTrue, TypeDontCare, ".cpp", &gppHostRule },
		{ HostTrue, TypeDontCare, ".cxx", &gppHostRule },
		{ HostFalse, TypeDontCare, ".c", &gccRule },
		{ HostFalse, TypeDontCare, ".cc", &gppRule },
		{ HostFalse, TypeDontCare, ".cpp", &gppRule },
		{ HostFalse, TypeDontCare, ".cxx", &gppRule },
		{ HostFalse, Cabinet, ".*", &emptyRule }
	};
	size_t i;
	Rule *customRule = NULL;

	for ( i = 0; i < sizeof ( rules ) / sizeof ( rules[0] ); i++ )
	{
		if ( rules[i].host != HostDontCare && rules[i].host != ModuleHandlerInformations[module.type].DefaultHost )
			continue;
		if ( rules[i].type != TypeDontCare && rules[i].type != module.type )
			continue;
		if ( rules[i].extension != extension && rules[i].extension != ".*")
			continue;
		customRule = rules[i].rule;
		break;
	}

	if ( extension == ".c" || extension == ".cc" || extension == ".cpp" || extension == ".cxx" )
	{
		GenerateGccCommand ( &sourceFile,
		                     customRule,
		                     GetCompilationUnitDependencies ( compilationUnit ) + extraDependencies );
	}
	else if ( customRule )
		customRule->Execute ( fMakefile, backend, module, &sourceFile, clean_files );
	else
	{
		throw InvalidOperationException ( __FILE__,
		                                  __LINE__,
		                                  "Unsupported filename extension '%s' in file '%s'",
		                                  extension.c_str (),
		                                  backend->GetFullName ( sourceFile ).c_str () );
	}
}

void
MingwModuleHandler::GenerateBuildMapCode ( const FileLocation *mapTarget )
{
	fprintf ( fMakefile, "# BUILD MAP CODE\n" );

	fprintf ( fMakefile,
	          "ifeq ($(ROS_BUILDMAP),full)\n" );

	FileLocation mapFilename ( OutputDirectory,
	                           module.output->relative_path,
	                           GetBasename ( module.output->name ) + ".map" );
	CLEAN_FILE ( mapFilename );

	fprintf ( fMakefile,
	          "\t$(ECHO_OBJDUMP)\n" );
	fprintf ( fMakefile,
	          "\t$(Q)${objdump} -d -S %s > %s\n",
	          mapTarget ? backend->GetFullName ( *mapTarget ).c_str () :  "$@",
	          backend->GetFullName ( mapFilename ).c_str () );

	fprintf ( fMakefile,
	          "else\n" );
	fprintf ( fMakefile,
	          "ifeq ($(ROS_BUILDMAP),yes)\n" );

	fprintf ( fMakefile,
	          "\t$(ECHO_NM)\n" );
	fprintf ( fMakefile,
	          "\t$(Q)${nm} --numeric-sort %s > %s\n",
	          mapTarget ? backend->GetFullName ( *mapTarget ).c_str () :  "$@",
	          backend->GetFullName ( mapFilename ).c_str () );

	fprintf ( fMakefile,
	          "endif\n" );

	fprintf ( fMakefile,
	          "endif\n" );
}

void
MingwModuleHandler::GenerateBuildNonSymbolStrippedCode ()
{
	fprintf ( fMakefile, "# BUILD NO STRIP CODE\n" );

	fprintf ( fMakefile,
	          "ifeq ($(ROS_BUILDNOSTRIP),yes)\n" );

	FileLocation nostripFilename ( OutputDirectory,
	                               module.output->relative_path,
	                               GetBasename ( module.output->name ) + ".nostrip" + GetExtension ( *module.output ) );
	CLEAN_FILE ( nostripFilename );

	OutputCopyCommand ( *module.output, nostripFilename );

	fprintf ( fMakefile,
	          "endif\n" );
}

void
MergeStringVector ( const Backend* backend,
                    const vector<FileLocation>& input,
                    vector<string>& output )
{
	int wrap_at = 25;
	string s;
	int wrap_count = -1;
	for ( size_t i = 0; i < input.size (); i++ )
	{
		if ( wrap_count++ == wrap_at )
		{
			output.push_back ( s );
			s = "";
			wrap_count = 0;
		}
		else if ( s.size () > 0)
			s += " ";
		s += backend->GetFullName ( input[i] );
	}
	if ( s.length () > 0 )
		output.push_back ( s );
}

void
MingwModuleHandler::GetObjectsVector ( const IfableData& data,
                                       vector<FileLocation>& objectFiles ) const
{
	for ( size_t i = 0; i < data.compilationUnits.size (); i++ )
	{
		CompilationUnit& compilationUnit = *data.compilationUnits[i];
		const FileLocation& compilationName = compilationUnit.GetFilename ();
		const FileLocation *object_file = GetObjectFilename ( &compilationName, module );
		objectFiles.push_back ( *object_file );
		delete object_file;
	}
}

void
MingwModuleHandler::GenerateCleanObjectsAsYouGoCode () const
{
	if ( backend->configuration.CleanAsYouGo )
	{
		vector<FileLocation> objectFiles;
		GetObjectsVector ( module.non_if_data,
		                   objectFiles );
		vector<string> lines;
		MergeStringVector ( backend,
		                    objectFiles,
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
	fprintf ( fMakefile, "# RUN RSYM CODE\n" );
	fprintf ( fMakefile,
             "ifneq ($(ROS_GENERATE_RSYM),no)\n" );
	fprintf ( fMakefile,
	          "\t$(ECHO_RSYM)\n" );
	fprintf ( fMakefile,
	          "\t$(Q)$(RSYM_TARGET) $@ $@\n\n" );
	fprintf ( fMakefile,
             "endif\n" );
}

void
MingwModuleHandler::GenerateRunStripCode () const
{
	fprintf ( fMakefile, "# RUN STRIP CODE\n" );
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
	const string& linkerParameters,
	const string& pefixupParameters )
{
	const FileLocation *target_file = GetTargetFilename ( module, NULL );
	const FileLocation *definitionFilename = GetDefinitionFilename ();
	string linker = "${ld}";
	string objectsMacro = GetObjectsMacro ( module );
	string libsMacro = GetLibsMacro ();

	fprintf ( fMakefile, "# LINKER COMMAND\n" );

	string target_macro ( GetTargetMacro ( module ) );
	string target_folder ( backend->GetFullPath ( *target_file ) );

	string linkerScriptArgument;
	if ( module.linkerScript != NULL )
		linkerScriptArgument = ssprintf ( " -T %s", backend->GetFullName ( *module.linkerScript->file ).c_str () );
	else
		linkerScriptArgument = "";

	/* check if we need to add default C++ libraries, ie if we have
	 * a C++ user-mode module without the -nostdlib linker flag
	 */
	bool link_defaultlibs = module.cplusplus &&
	                        linkerParameters.find ("-nostdlib") == string::npos &&
	                        !(module.type == KernelModeDLL || module.type == KernelModeDriver);

	if ( !module.HasImportLibrary() )
	{
		fprintf ( fMakefile,
			"%s: %s %s $(RSYM_TARGET) $(PEFIXUP_TARGET) | %s\n",
			target_macro.c_str (),
			definitionFilename ? backend->GetFullName ( *definitionFilename ).c_str () : "",
			dependencies.c_str (),
			target_folder.c_str () );
		fprintf ( fMakefile, "\t$(ECHO_LD)\n" );

		fprintf ( fMakefile,
		          "\t%s %s%s %s %s %s %s -o %s\n",
		          linker.c_str (),
		          linkerParameters.c_str (),
		          linkerScriptArgument.c_str (),
		          objectsMacro.c_str (),
		          link_defaultlibs ? "$(PROJECT_LPPFLAGS) " : "",
		          libsMacro.c_str (),
		          GetLinkerMacro ().c_str (),
		          target_macro.c_str () );
	}
	else
	{
		FileLocation temp_exp ( IntermediateDirectory,
		                        module.output->relative_path,
		                        module.name + ".exp" );
		CLEAN_FILE ( temp_exp );

		fprintf ( fMakefile,
			"%s: %s | %s\n",
			backend->GetFullName ( temp_exp ).c_str (),
			definitionFilename ? backend->GetFullName ( *definitionFilename ).c_str () : "",
			backend->GetFullPath ( temp_exp ).c_str () );
		fprintf ( fMakefile, "\t$(ECHO_DLLTOOL)\n" );

		fprintf ( fMakefile,
		          "\t${dlltool} --dllname %s --def %s --output-exp $@%s%s\n",
		          module.GetDllName ().c_str (),
		          definitionFilename ? backend->GetFullName ( *definitionFilename ).c_str () : "",
		          module.mangledSymbols ? "" : " --kill-at",
		          module.underscoreSymbols ? " --add-underscore" : "" );

		fprintf ( fMakefile,
			"%s: %s %s $(RSYM_TARGET) $(PEFIXUP_TARGET) | %s\n",
			target_macro.c_str (),
			backend->GetFullName ( temp_exp ).c_str (),
			dependencies.c_str (),
			target_folder.c_str () );
		fprintf ( fMakefile, "\t$(ECHO_LD)\n" );

		fprintf ( fMakefile,
		          "\t%s %s%s %s %s %s %s %s -o %s\n",

		          linker.c_str (),
		          linkerParameters.c_str (),
		          linkerScriptArgument.c_str (),
		          backend->GetFullName ( temp_exp ).c_str (),
		          objectsMacro.c_str (),
		          link_defaultlibs ? "$(PROJECT_LPPFLAGS) " : "",
		          libsMacro.c_str (),
		          GetLinkerMacro ().c_str (),
		          target_macro.c_str () );

		fprintf ( fMakefile,
		          "\t$(Q)$(PEFIXUP_TARGET) %s -exports%s\n",
		          target_macro.c_str (),
		          pefixupParameters.c_str() );
	}

	GenerateBuildMapCode ();
	GenerateBuildNonSymbolStrippedCode ();
	GenerateRunRsymCode ();
	GenerateRunStripCode ();
	GenerateCleanObjectsAsYouGoCode ();

	if ( definitionFilename )
		delete definitionFilename;
	delete target_file;
}

void
MingwModuleHandler::GeneratePhonyTarget() const
{
	string targetMacro ( GetTargetMacro ( module ) );
	const FileLocation *target_file = GetTargetFilename ( module, NULL );

	fprintf ( fMakefile, "# PHONY TARGET\n" );
	fprintf ( fMakefile,
	          ".PHONY: %s\n\n",
	          targetMacro.c_str ());
	fprintf ( fMakefile, "%s: | %s\n",
	          targetMacro.c_str (),
	          backend->GetFullPath ( *target_file ).c_str () );

	delete target_file;
}

void
MingwModuleHandler::GenerateObjectFileTargets ( const IfableData& data )
{
	size_t i;
	string moduleDependencies;

	fprintf ( fMakefile, "# OBJECT FILE TARGETS\n" );

	const vector<CompilationUnit*>& compilationUnits = data.compilationUnits;
	for ( i = 0; i < compilationUnits.size (); i++ )
	{
		CompilationUnit& compilationUnit = *compilationUnits[i];
		const FileLocation& compilationName = compilationUnit.GetFilename ();
		const FileLocation *objectFilename = GetObjectFilename ( &compilationName, module );
		if ( GetExtension ( *objectFilename ) == ".h" )
			moduleDependencies += ssprintf ( " $(%s_HEADERS)", module.name.c_str () );
		else if ( GetExtension ( *objectFilename ) == ".rc" )
			moduleDependencies += ssprintf ( " $(%s_RESOURCES)", module.name.c_str () );
		delete objectFilename;
	}

	for ( i = 0; i < compilationUnits.size (); i++ )
	{
		GenerateCommands ( *compilationUnits[i],
		                   moduleDependencies );
	}

	vector<CompilationUnit*> sourceCompilationUnits;
	GetModuleSpecificCompilationUnits ( sourceCompilationUnits );
	for ( i = 0; i < sourceCompilationUnits.size (); i++ )
	{
		GenerateCommands ( *sourceCompilationUnits[i],
		                   moduleDependencies );
	}
	CleanupCompilationUnitVector ( sourceCompilationUnits );

	if ( module.type == RpcProxy )
	{
		widlDlldataRule.Execute ( fMakefile,
								  backend,
								  module,
								  GetDlldataFilename(),
								  clean_files,
								  ssprintf ( "$(%s_SOURCES)", module.name.c_str ()) );
	}
}

void
MingwModuleHandler::GenerateObjectFileTargets ()
{
	fprintf ( fMakefile, "# OBJECT FILE TARGETS\n" );

	if ( module.pch && use_pch )
	{

		std::map<string, string> vars;

		/* WIDL generated headers may be used */
		string dependencies;
		vector<FileLocation> rpcDependencies;
		GetRpcHeaderDependencies ( rpcDependencies );
		if ( rpcDependencies.size () > 0 )
			dependencies = " " + v2s ( backend, rpcDependencies, 5 );

		if ( module.cplusplus )
			pchCxxRule.Execute ( fMakefile, backend, module, module.pch->file, clean_files, dependencies );
		else
			pchRule.Execute ( fMakefile, backend, module, module.pch->file, clean_files, dependencies );

		fprintf ( fMakefile, "\n" );
	}

	GenerateObjectFileTargets ( module.non_if_data );
	fprintf ( fMakefile, "\n" );
}

/* caller needs to delete the returned object */
const FileLocation*
MingwModuleHandler::GenerateArchiveTarget ()
{
	const FileLocation *archiveFilename = GetModuleArchiveFilename ();
	const FileLocation *definitionFilename = GetDefinitionFilename ();

	fprintf ( fMakefile, "# ARCHIVE TARGET\n" );

	if ( IsStaticLibrary ( module ) && definitionFilename )
	{
		arRule1.Execute ( fMakefile,
						  backend,
						  module,
						  archiveFilename,
						  clean_files,
						  backend->GetFullName ( *definitionFilename ).c_str () );

		fprintf ( fMakefile,
		          "\t${dlltool} --dllname %s --def %s --output-lib $@%s%s\n",
		          module.GetDllName ().c_str (),
		          backend->GetFullName ( *definitionFilename ).c_str (),
		          module.mangledSymbols ? "" : " --kill-at",
		          module.underscoreSymbols ? " --add-underscore" : "" );
	}
	else
		arRule1.Execute ( fMakefile, backend, module, archiveFilename, clean_files );

	if ( definitionFilename )
		delete definitionFilename;

	if(module.type == HostStaticLibrary)
		arHostRule2.Execute ( fMakefile, backend, module, archiveFilename, clean_files );
	else
		arRule2.Execute ( fMakefile, backend, module, archiveFilename, clean_files );

	GenerateCleanObjectsAsYouGoCode ();

	fprintf ( fMakefile, "\n" );

	return archiveFilename;
}

/*static*/ string
MingwModuleHandler::GetObjectsMacro ( const Module& module )
{
	return ssprintf ( "$(%s_OBJS)",
	                  module.name.c_str () );
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
MingwModuleHandler::GetDebugFormat ()
{
    if (Environment::GetArch() == "amd64")
    {
        return "dwarf-2";
    }
    return "stabs+";
}

string
MingwModuleHandler::GetModuleTargets ( const Module& module )
{
	if ( ReferenceObjects ( module ) )
		return GetObjectsMacro ( module );
	else
	{
		const FileLocation *target_file = GetTargetFilename ( module, NULL );
		string target = backend->GetFullName ( *target_file ).c_str ();
		delete target_file;
		return target;
	}
}

void
MingwModuleHandler::GenerateSourceMacro ()
{
	sourcesMacro = ssprintf ( "%s_SOURCES", module.name.c_str ());

	if ( module.type == RpcProxy || module.type == Cabinet )
		GenerateSourceMacros ( module.non_if_data );

	// future references to the macro will be to get its values
	sourcesMacro = ssprintf ("$(%s)", sourcesMacro.c_str ());
}

void
MingwModuleHandler::GenerateObjectMacro ()
{
	objectsMacro = ssprintf ("%s_OBJS", module.name.c_str ());

	GenerateObjectMacros ( module.non_if_data );

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
	vector<FileLocation>& dependencies ) const
{
	for ( size_t i = 0; i < module.non_if_data.libraries.size (); i++ )
	{
		Library& library = *module.non_if_data.libraries[i];
		if ( library.importedModule->type == RpcServer ||
		     library.importedModule->type == RpcClient ||
		     library.importedModule->type == RpcProxy ||
		     library.importedModule->type == IdlHeader )
		{
			for ( size_t j = 0; j < library.importedModule->non_if_data.compilationUnits.size (); j++ )
			{
				CompilationUnit& compilationUnit = *library.importedModule->non_if_data.compilationUnits[j];
				const FileLocation& sourceFile = compilationUnit.GetFilename ();
				string extension = GetExtension ( sourceFile );
				if ( extension == ".idl" || extension == ".IDL" )
				{
					string basename = GetBasename ( sourceFile.name );
					if ( library.importedModule->type == RpcServer )
					{
						const FileLocation *header = GetRpcServerHeaderFilename ( &sourceFile );
						dependencies.push_back ( *header );
						delete header;
					}
					if ( library.importedModule->type == RpcClient )
					{
						const FileLocation *header = GetRpcClientHeaderFilename ( &sourceFile );
						dependencies.push_back ( *header );
						delete header;
					}
					if ( library.importedModule->type == RpcProxy )
					{
						const FileLocation *header = GetRpcProxyHeaderFilename ( &sourceFile );
						dependencies.push_back ( *header );
						delete header;
					}
					if ( library.importedModule->type == IdlHeader )
					{
						const FileLocation *header = GetIdlHeaderFilename ( &sourceFile );
						dependencies.push_back ( *header );
						delete header;
					}
				}
			}
		}
	}
}

void
MingwModuleHandler::GenerateOtherMacros ()
{
	set<const Define *> used_defs;

	linkerflagsMacro = ssprintf ("%s_LFLAGS", module.name.c_str ());
	libsMacro = ssprintf("%s_LIBS", module.name.c_str ());

	const FileLocation * pchPath = GetPrecompiledHeaderPath ();

	if ( pchPath )
	{
		string pchPathStr = backend->GetFullName ( *pchPath );
		delete pchPath;

		fprintf ( fMakefile,
				  "%s_%sINCLUDES+= -I%s\n",
				  module.name.c_str(),
				  CompilerPrefixTable[CompilerTypeCC],
				  pchPathStr.c_str() );

		fprintf ( fMakefile,
				  "%s_%sINCLUDES+= -I%s\n",
				  module.name.c_str(),
				  CompilerPrefixTable[CompilerTypeCXX],
				  pchPathStr.c_str() );
	}

	const char * toolPrefix = "";

	if ( ModuleHandlerInformations[module.type].DefaultHost == HostTrue )
		toolPrefix = "HOST_";

	// FIXME: this is very ugly and generates lots of useless entries
	for ( unsigned type = CompilerTypeCC; type < CompilerTypesCount; ++ type )
	{
		string flags;

		if ( module.dynamicCRT )
			flags += ssprintf ( " $(%s%sFLAG_CRTDLL)", toolPrefix, CompilerPrefixTable[type] );

		// FIXME: this duplicates the flag for CPP and C/CXX
		if ( !module.allowWarnings )
			flags += ssprintf ( " $(%s%sFLAG_WERROR)", toolPrefix, CompilerPrefixTable[type] );

		if ( module.isUnicode )
			flags += ssprintf ( " $(%s%sFLAG_UNICODE)", toolPrefix, CompilerPrefixTable[type] );

		if ( flags.size() )
		{
			fprintf ( fMakefile,
					  "%s_%sFLAGS+=%s\n",
					  module.name.c_str(),
					  CompilerPrefixTable[type],
					  flags.c_str() );
		}
	}

	GenerateParameters ( module.name.c_str(), "+=", module.non_if_data );

	const char *linkerflags = ModuleHandlerInformations[module.type].linkerflags;
	if ( strlen( linkerflags ) > 0 )
	{
		fprintf ( fMakefile,
		          "%s += %s\n\n",
		          linkerflagsMacro.c_str (),
		          linkerflags );
	}

	// FIXME: make rules for linker, move standard flags there
	if ( ModuleHandlerInformations[module.type].DefaultHost == HostFalse )
	{
		if ( module.cplusplus )
			fprintf ( fMakefile,
					  "%s+= $(PROJECT_LPPFLAGS)\n\n",
					  linkerflagsMacro.c_str () );
		else
			fprintf ( fMakefile,
					  "%s+= $(PROJECT_LFLAGS)\n\n",
					  linkerflagsMacro.c_str () );
	}

	GenerateMacros (
		"+=",
		module.non_if_data,
		&module.linkerFlags,
		used_defs );

	fprintf ( fMakefile, "\n\n" );
}

void
MingwModuleHandler::GenerateRules ()
{
    SpecFileType spec;

	fprintf ( fMakefile, "# RULES\n" );
	string targetMacro = GetTargetMacro ( module );
	//CLEAN_FILE ( targetMacro );
	CLEAN_FILE ( FileLocation ( SourceDirectory, "", targetMacro ) );

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
		const FileLocation* ar_target = GenerateArchiveTarget ();
		delete ar_target;
	}


    spec = IsSpecDefinitionFile();

    if(spec)
	{
		Rule * defRule;

		if (spec == PSpec)
			defRule = &winebuildPRule;
		else
			defRule = &winebuildRule;

		defRule->Execute ( fMakefile, backend, module, module.importLibrary->source, clean_files );
	}

	GenerateObjectFileTargets ();
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

	fprintf ( fMakefile, "# INVOCATIONS\n" );

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
		          NormalizeFilename ( backend->GetFullName ( *invoke.invokeModule->output ) ).c_str () );
		fprintf ( fMakefile, "\t$(ECHO_INVOKE)\n" );
		fprintf ( fMakefile,
		          "\t%s %s\n\n",
		          NormalizeFilename ( backend->GetFullName ( *invoke.invokeModule->output ) ).c_str (),
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
	if ( ModuleHandlerInformations[module.type].DefaultHost == HostTrue )
		return;

	if (module.name != "psdk" &&
		module.name != "dxsdk")
	{
		dependencies.push_back ( "$(PSDK_TARGET) $(psdk_HEADERS)" );
		dependencies.push_back ( "$(DXSDK_TARGET) $(dxsdk_HEADERS)" );
	}

	if (module.name != "errcodes" &&
		module.name != "bugcodes" &&
		module.name != "ntstatus")
	{
		dependencies.push_back ( "$(ERRCODES_TARGET) $(ERRCODES_MCHEADERS)" );
		dependencies.push_back ( "$(BUGCODES_TARGET) $(BUGCODES_MCHEADERS)" );
		dependencies.push_back ( "$(NTSTATUS_TARGET) $(NTSTATUS_MCHEADERS)" );
	}

	///* Check if any dependent library relies on the generated headers */
	//for ( size_t i = 0; i < module.project.modules.size (); i++ )
	//{
	//	const Module& m = *module.project.modules[i];
	//	for ( size_t j = 0; j < m.non_if_data.compilationUnits.size (); j++ )
	//	{
	//		CompilationUnit& compilationUnit = *m.non_if_data.compilationUnits[j];
	//		const FileLocation& sourceFile = compilationUnit.GetFilename ();
	//		string extension = GetExtension ( sourceFile );
	//		if (extension == ".mc" || extension == ".MC" )
	//		{
	//			string dependency = ssprintf ( "$(%s_MCHEADERS)", m.name.c_str () );
	//			dependencies.push_back ( dependency );
	//		}
	//	}
	//}
}

void
MingwModuleHandler::GeneratePreconditionDependencies ()
{
	fprintf ( fMakefile, "# PRECONDITION DEPENDENCIES\n" );
	string preconditionDependenciesName = GetPreconditionDependenciesName ();
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

	fprintf ( fMakefile, "\n" );
}

SpecFileType
MingwModuleHandler::IsSpecDefinitionFile () const
{
    if(!module.importLibrary)
        return None;

	std::string ext = GetExtension ( *module.importLibrary->source );

    if ( ext == ".spec" )
        return Spec;

    if ( ext == ".pspec" )
        return PSpec;

    return None;
}

/* caller needs to delete the returned object */
const FileLocation*
MingwModuleHandler::GetDefinitionFilename () const
{
	if ( module.importLibrary == NULL )
		return NULL;

	if ( IsSpecDefinitionFile () )
	{
		return new FileLocation ( IntermediateDirectory,
								  module.importLibrary->source->relative_path,
								  GetBasename ( module.importLibrary->source->name ) + "_" + module.name + ".auto.def" );
	}
	else
	{
		return new FileLocation ( SourceDirectory,
								  module.importLibrary->source->relative_path,
								  module.importLibrary->source->name );
	}
}

void
MingwModuleHandler::GenerateImportLibraryTargetIfNeeded ()
{
	if ( module.importLibrary != NULL )
	{
		const FileLocation *library_target = GetImportLibraryFilename ( module, &clean_files );
		const FileLocation *defFilename = GetDefinitionFilename ();
		string empty = "tools" + sSep + "rbuild" + sSep + "empty.def";

		fprintf ( fMakefile, "# IMPORT LIBRARY RULE\n" );

		fprintf ( fMakefile, "%s:",
		          backend->GetFullName ( *library_target ).c_str () );

		if ( defFilename )
		{
			fprintf ( fMakefile, " %s",
			          backend->GetFullName ( *defFilename ).c_str () );
		}

		fprintf ( fMakefile, " | %s\n",
		          backend->GetFullPath ( *library_target ).c_str () );

		fprintf ( fMakefile, "\t$(ECHO_DLLTOOL)\n" );

		fprintf ( fMakefile,
		          "\t${dlltool} --dllname %s --def %s --output-lib %s%s%s\n\n",
		          module.GetDllName ().c_str (),
		          defFilename ? backend->GetFullName ( *defFilename ).c_str ()
		                      : empty.c_str (),
		          backend->GetFullName ( *library_target ).c_str (),
		          module.mangledSymbols ? "" : " --kill-at",
		          module.underscoreSymbols ? " --add-underscore" : "" );

		if ( defFilename )
			delete defFilename;
		delete library_target;
	}
}

void
MingwModuleHandler::GetSpecObjectDependencies (
	vector<FileLocation>& dependencies,
	const FileLocation *file ) const
{
	dependencies.push_back ( FileLocation ( IntermediateDirectory,
											file->relative_path,
											GetBasename ( file->name ) + "_" + module.name + ".stubs.c" ) );
}

void
MingwModuleHandler::GetMcObjectDependencies (
	vector<FileLocation>& dependencies,
	const FileLocation *file ) const
{
	string basename = GetBasename ( file->name );

	FileLocation defDependency ( IntermediateDirectory,
	                             "include/reactos",
	                             basename + ".h" );
	dependencies.push_back ( defDependency );

	FileLocation stubsDependency ( IntermediateDirectory,
	                               file->relative_path,
	                             basename + ".rc" );
	dependencies.push_back ( stubsDependency );
}

void
MingwModuleHandler::GetWidlObjectDependencies (
	vector<FileLocation>& dependencies,
	const FileLocation *file ) const
{
	string basename = GetBasename ( file->name );
	const FileLocation *generatedHeaderFilename = GetRpcServerHeaderFilename ( file );

	FileLocation serverSourceDependency ( IntermediateDirectory,
	                                      file->relative_path,
	                                      basename + "_s.c" );
	dependencies.push_back ( serverSourceDependency );
	dependencies.push_back ( *generatedHeaderFilename );

	delete generatedHeaderFilename;
}

void
MingwModuleHandler::GetDefinitionDependencies (
	vector<FileLocation>& dependencies ) const
{
	const vector<CompilationUnit*>& compilationUnits = module.non_if_data.compilationUnits;
	for ( size_t i = 0; i < compilationUnits.size (); i++ )
	{
		const CompilationUnit& compilationUnit = *compilationUnits[i];
		const FileLocation& sourceFile = compilationUnit.GetFilename ();
		string extension = GetExtension ( sourceFile );

		if (extension == ".spec" || extension == ".pspec")
			GetSpecObjectDependencies ( dependencies, &sourceFile );

		if (extension == ".idl")
		{
			if ( ( module.type == RpcServer ) || ( module.type == RpcClient ) || ( module.type == RpcProxy ) )
				GetWidlObjectDependencies ( dependencies, &sourceFile );
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

static void
MingwAddCRTLibrary( Module &module )
{
	const char * crtAttr = module.CRT.c_str ();
	const char * crtLib = NULL;

	if ( stricmp ( crtAttr, "libc" ) == 0 )
		crtLib = "crt";
	else if ( stricmp ( crtAttr, "msvcrt" ) == 0 )
		crtLib = "msvcrt";
	else if ( stricmp ( crtAttr, "libcntpr" ) == 0 )
		crtLib = "libcntpr";
	else if ( stricmp ( crtAttr, "ntdll" ) == 0 )
		crtLib = "ntdll";

	if ( crtLib )
	{
		Library* pLibrary = new Library ( module, std::string ( crtLib ) );

		if ( pLibrary->importedModule == NULL)
		{
			throw XMLInvalidBuildFileException (
				module.node.location,
				"module '%s' trying to import non-existant C runtime module '%s'",
				module.name.c_str(),
				crtLib );
		}

		module.non_if_data.libraries.push_back ( pLibrary );
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
	string libsMacro = GetLibsMacro ();

	GenerateRules ();

	fprintf ( fMakefile, "# BUILD TOOL MODULE TARGET\n" );

	string linker;
	if ( module.cplusplus )
		linker = "${host_gpp}";
	else
		linker = "${host_gcc}";

	const FileLocation *target_file = GetTargetFilename ( module, NULL );
	fprintf ( fMakefile, "%s: %s %s | %s\n",
	          targetMacro.c_str (),
	          objectsMacro.c_str (),
	          libsMacro.c_str (),
	          backend->GetFullPath ( *target_file ).c_str () );
	fprintf ( fMakefile, "\t$(ECHO_HOSTLD)\n" );
	fprintf ( fMakefile,
	          "\t%s %s -o $@ %s %s\n\n",
	          linker.c_str (),
	          GetLinkerMacro ().c_str (),
	          objectsMacro.c_str (),
	          libsMacro.c_str () );

	delete target_file;
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
	string libsMacro = GetLibsMacro ();

	GenerateImportLibraryTargetIfNeeded ();

	if ( module.non_if_data.compilationUnits.size () > 0 )
	{
		GenerateRules ();

		string dependencies = libsMacro + " " + objectsMacro;

		string linkerParameters = ssprintf ( "-subsystem=native -entry=%s -image-base=%s",
		                                     module.GetEntryPoint(!(Environment::GetArch() == "arm")).c_str (),
		                                     module.baseaddress.c_str () );

		GenerateLinkerCommand ( dependencies,
		                        linkerParameters + " $(NTOSKRNL_SHARED)",
		                        " -sections" );
	}
	else
	{
		GeneratePhonyTarget();
	}
}


MingwKernelModeDLLModuleHandler::MingwKernelModeDLLModuleHandler (
	const Module& module_ )

	: MingwModuleHandler ( module_ )
{
}


void
MingwKernelModeDLLModuleHandler::AddImplicitLibraries ( Module& module )
{
	MingwAddCRTLibrary ( module );
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
	string workingDirectory = GetWorkingDirectory ();
	string libsMacro = GetLibsMacro ();

	GenerateImportLibraryTargetIfNeeded ();

	if ( module.non_if_data.compilationUnits.size () > 0 )
	{
		GenerateRules ();

		string dependencies = libsMacro + " " + objectsMacro;

		string linkerParameters = ssprintf ( "-subsystem=native -entry=%s -image-base=%s -file-alignment=0x1000 -section-alignment=0x1000 -shared",
		                                     module.GetEntryPoint(!(Environment::GetArch() == "arm")).c_str (),
		                                     module.baseaddress.c_str () );
		GenerateLinkerCommand ( dependencies,
		                        linkerParameters,
		                        " -sections" );
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
	MingwAddCRTLibrary ( module );
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
	string libsMacro = GetLibsMacro ();

	GenerateImportLibraryTargetIfNeeded ();

	if ( module.non_if_data.compilationUnits.size () > 0 )
	{
		GenerateRules ();

		string dependencies = libsMacro + " " + objectsMacro;

		string linkerParameters = ssprintf ( "-subsystem=native -entry=%s -image-base=%s -file-alignment=0x1000 -section-alignment=0x1000 -shared",
		                                     module.GetEntryPoint(!(Environment::GetArch() == "arm")).c_str (),
		                                     module.baseaddress.c_str () );
		GenerateLinkerCommand ( dependencies,
		                        linkerParameters,
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
	MingwAddCRTLibrary ( module );
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
	string libsMacro = GetLibsMacro ();

	GenerateImportLibraryTargetIfNeeded ();

	if ( module.non_if_data.compilationUnits.size () > 0 )
	{
		GenerateRules ();

		string dependencies = libsMacro + " " + objectsMacro;

		string linkerParameters = ssprintf ( "-subsystem=native -entry=%s -image-base=%s -file-alignment=0x1000 -section-alignment=0x1000",
		                                     module.GetEntryPoint(!(Environment::GetArch() == "arm")).c_str (),
		                                     module.baseaddress.c_str () );
		GenerateLinkerCommand ( dependencies,
		                        linkerParameters,
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

	if ( module.type != Win32DLL
	  && module.type != Win32OCX
	  && module.type != Win32CUI
	  && module.type != Win32GUI
	  && module.type != Win32SCR)
	{
		return;
	}

	if ( module.isDefaultEntryPoint )
	{
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
	}

	pLibrary = new Library ( module, "mingw_common" );
	module.non_if_data.libraries.push_back ( pLibrary );

	MingwAddCRTLibrary ( module );
	MingwAddDebugSupportLibraries ( module, DebugUserMode );
}

void
MingwWin32DLLModuleHandler::AddImplicitLibraries ( Module& module )
{
	MingwAddImplicitLibraries ( module );
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
	string libsMacro = GetLibsMacro ();

	GenerateImportLibraryTargetIfNeeded ();

	if ( module.non_if_data.compilationUnits.size () > 0 )
	{
		GenerateRules ();

		string dependencies = libsMacro + " " + objectsMacro;

		string linkerParameters = ssprintf ( "-subsystem=console -entry=%s -image-base=%s -file-alignment=0x1000 -section-alignment=0x1000 -shared",
		                                     module.GetEntryPoint(!(Environment::GetArch() == "arm")).c_str (),
		                                     module.baseaddress.c_str () );
		GenerateLinkerCommand ( dependencies,
		                        linkerParameters,
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
	string libsMacro = GetLibsMacro ();

	GenerateImportLibraryTargetIfNeeded ();

	if ( module.non_if_data.compilationUnits.size () > 0 )
	{
		GenerateRules ();

		string dependencies = libsMacro + " " + objectsMacro;

		string linkerParameters = ssprintf ( "-subsystem=console -entry=%s -image-base=%s -file-alignment=0x1000 -section-alignment=0x1000 -shared",
		                                     module.GetEntryPoint(!(Environment::GetArch() == "arm")).c_str (),
		                                     module.baseaddress.c_str () );
		GenerateLinkerCommand ( dependencies,
		                        linkerParameters,
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
	string libsMacro = GetLibsMacro ();

	GenerateImportLibraryTargetIfNeeded ();

	if ( module.non_if_data.compilationUnits.size () > 0 )
	{
		GenerateRules ();

		string dependencies = libsMacro + " " + objectsMacro;

		string linkerParameters = ssprintf ( "-subsystem=console -entry=%s -image-base=%s -file-alignment=0x1000 -section-alignment=0x1000",
		                                     module.GetEntryPoint(!(Environment::GetArch() == "arm")).c_str (),
		                                     module.baseaddress.c_str () );
		GenerateLinkerCommand ( dependencies,
		                        linkerParameters,
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
	string libsMacro = GetLibsMacro ();

	GenerateImportLibraryTargetIfNeeded ();

	if ( module.non_if_data.compilationUnits.size () > 0 )
	{
		GenerateRules ();

		string dependencies = libsMacro + " " + objectsMacro;

		string linkerParameters = ssprintf ( "-subsystem=windows -entry=%s -image-base=%s -file-alignment=0x1000 -section-alignment=0x1000",
		                                     module.GetEntryPoint(!(Environment::GetArch() == "arm")).c_str (),
		                                     module.baseaddress.c_str () );
		GenerateLinkerCommand ( dependencies,
		                        linkerParameters,
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
	fprintf ( fMakefile, "# BOOT LOADER MODULE TARGET\n" );
	string targetName ( module.output->name );
	string targetMacro ( GetTargetMacro (module) );
	string workingDirectory = GetWorkingDirectory ();
	FileLocation junk_tmp ( TemporaryDirectory,
	                        "",
	                        module.name + ".junk.tmp" );
	CLEAN_FILE ( junk_tmp );
	string objectsMacro = GetObjectsMacro ( module );
	string libsMacro = GetLibsMacro ();

	GenerateRules ();

	const FileLocation *target_file = GetTargetFilename ( module, NULL );
	fprintf ( fMakefile, "%s: %s %s | %s\n",
	          targetMacro.c_str (),
	          objectsMacro.c_str (),
	          libsMacro.c_str (),
	          backend->GetFullPath ( *target_file ).c_str () );

	fprintf ( fMakefile, "\t$(ECHO_LD)\n" );

	if (Environment::GetArch() == "arm")
	{
		fprintf ( fMakefile,
		         "\t${gcc} -Wl,--subsystem,native -o %s %s %s %s\n",
		         backend->GetFullName ( junk_tmp ).c_str (),
		         objectsMacro.c_str (),
		         libsMacro.c_str (),
		         GetLinkerMacro ().c_str ());
	}
	else
	{
		fprintf ( fMakefile,
		         "\t${gcc} -Wl,--subsystem,native -Wl,-Ttext,0x8000 -o %s %s %s %s\n",
		         backend->GetFullName ( junk_tmp ).c_str (),
		         objectsMacro.c_str (),
		         libsMacro.c_str (),
		         GetLinkerMacro ().c_str ());
	}
	fprintf ( fMakefile,
	          "\t${objcopy} -O binary %s $@\n",
	          backend->GetFullName ( junk_tmp ).c_str () );
	GenerateBuildMapCode ( &junk_tmp );
	fprintf ( fMakefile,
	          "\t-@${rm} %s 2>$(NUL)\n",
	          backend->GetFullName ( junk_tmp ).c_str () );

	delete target_file;
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
	fprintf ( fMakefile, "# BOOT PROGRAM MODULE TARGET\n" );

	string targetName ( module.output->name );
	string targetMacro ( GetTargetMacro (module) );
	string workingDirectory = GetWorkingDirectory ();
	FileLocation junk_tmp ( TemporaryDirectory,
	                        "",
	                        module.name + ".junk.tmp" );
	FileLocation junk_elf ( TemporaryDirectory,
	                        "",
	                        module.name + ".junk.elf" );
	FileLocation junk_cpy ( TemporaryDirectory,
	                        "",
	                        module.name + ".junk.elf" );
	CLEAN_FILE ( junk_tmp );
	CLEAN_FILE ( junk_elf );
	CLEAN_FILE ( junk_cpy );
	string objectsMacro = GetObjectsMacro ( module );
	string libsMacro = GetLibsMacro ();
	const Module *payload = module.project.LocateModule ( module.payload );

	GenerateRules ();

	const FileLocation *target_file = GetTargetFilename ( module, NULL );
	fprintf ( fMakefile, "%s: %s %s %s | %s\n",
	          targetMacro.c_str (),
	          objectsMacro.c_str (),
	          libsMacro.c_str (),
	          payload->name.c_str (),
	          backend->GetFullPath ( *target_file ).c_str () );

	fprintf ( fMakefile, "\t$(ECHO_BOOTPROG)\n" );

	fprintf ( fMakefile, "\t$(%s_PREPARE) $(OUTPUT)$(SEP)%s %s\n",
		module.buildtype.c_str (),
		NormalizeFilename( backend->GetFullName ( *payload->output ) ).c_str (),
		backend->GetFullName ( junk_cpy ).c_str () );

	fprintf ( fMakefile, "\t${objcopy} $(%s_FLATFORMAT) %s %s\n",
		module.buildtype.c_str (),
		backend->GetFullName ( junk_cpy ).c_str (),
		backend->GetFullName ( junk_tmp ).c_str () );

	fprintf ( fMakefile, "\t${ld} $(%s_LINKFORMAT) %s %s -o %s\n",
		module.buildtype.c_str (),
		libsMacro.c_str (),
		backend->GetFullName ( junk_tmp ).c_str (),
		backend->GetFullName ( junk_elf ).c_str () );

	fprintf ( fMakefile, "\t${objcopy} $(%s_COPYFORMAT) %s $(INTERMEDIATE)$(SEP)%s\n",
		module.buildtype.c_str (),
		backend->GetFullName ( junk_elf ).c_str (),
		backend->GetFullName ( *module.output ) .c_str () );

	fprintf ( fMakefile,
	          "\t-@${rm} %s %s %s 2>$(NUL)\n",
	          backend->GetFullName ( junk_tmp ).c_str (),
	          backend->GetFullName ( junk_elf ).c_str (),
	          backend->GetFullName ( junk_cpy ).c_str () );

	delete target_file;
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
	for ( std::map<std::string, Module*>::const_iterator p = module.project.modules.begin (); p != module.project.modules.end (); ++ p )
	{
		const Module& m = *p->second;
		if ( !m.enabled )
			continue;
		if ( m.bootstrap != NULL )
		{
			FileLocation targetFile ( OutputDirectory,
			                          m.bootstrap->base.length () > 0
			                                   ? bootcdDirectory + sSep + m.bootstrap->base
			                                   : bootcdDirectory,
			                          m.bootstrap->nameoncd );
			OutputCopyCommand ( *m.output, targetFile );
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
		FileLocation targetFile ( OutputDirectory,
		                          cdfile.target->relative_path.length () > 0
		                              ? bootcdDirectory + sSep + cdfile.target->relative_path
		                              : bootcdDirectory,
		                          cdfile.target->name );
		OutputCopyCommand ( *cdfile.source, targetFile );
	}
}

void
MingwIsoModuleHandler::GetBootstrapCdDirectories ( vector<FileLocation>& out,
                                                   const string& bootcdDirectory )
{
	for ( std::map<std::string, Module*>::const_iterator p = module.project.modules.begin (); p != module.project.modules.end (); ++ p )
	{
		const Module& m = *p->second;
		if ( !m.enabled )
			continue;
		if ( m.bootstrap != NULL )
		{
			FileLocation targetDirectory ( OutputDirectory,
			                               m.bootstrap->base.length () > 0
			                                   ? bootcdDirectory + sSep + m.bootstrap->base
			                                   : bootcdDirectory,
			                               "" );
			out.push_back ( targetDirectory );
		}
	}
}

void
MingwIsoModuleHandler::GetNonModuleCdDirectories ( vector<FileLocation>& out,
                                                   const string& bootcdDirectory )
{
	for ( size_t i = 0; i < module.project.cdfiles.size (); i++ )
	{
		const CDFile& cdfile = *module.project.cdfiles[i];
		FileLocation targetDirectory ( OutputDirectory,
		                               cdfile.target->relative_path.length () > 0
		                                   ? bootcdDirectory + sSep + cdfile.target->relative_path
		                                   : bootcdDirectory,
		                               "" );
		out.push_back( targetDirectory );
	}
}

void
MingwIsoModuleHandler::GetCdDirectories ( vector<FileLocation>& out,
                                          const string& bootcdDirectory )
{
	GetBootstrapCdDirectories ( out, bootcdDirectory );
	GetNonModuleCdDirectories ( out, bootcdDirectory );
}

void
MingwIsoModuleHandler::GetBootstrapCdFiles (
	vector<FileLocation>& out ) const
{
	for ( std::map<std::string, Module*>::const_iterator p = module.project.modules.begin (); p != module.project.modules.end (); ++ p )
	{
		const Module& m = *p->second;
		if ( !m.enabled )
			continue;
		if ( m.bootstrap != NULL )
		{
			out.push_back ( *m.output );
		}
	}
}

void
MingwIsoModuleHandler::GetNonModuleCdFiles (
	vector<FileLocation>& out ) const
{
	for ( size_t i = 0; i < module.project.cdfiles.size (); i++ )
	{
		const CDFile& cdfile = *module.project.cdfiles[i];
		out.push_back ( *cdfile.source );
	}
}

void
MingwIsoModuleHandler::GetCdFiles (
	vector<FileLocation>& out ) const
{
	GetBootstrapCdFiles ( out );
	GetNonModuleCdFiles ( out );
}

void
MingwIsoModuleHandler::GenerateIsoModuleTarget ()
{
	fprintf ( fMakefile, "# ISO MODULE TARGET\n" );
	string bootcdDirectory = "cd";
	FileLocation bootcd ( OutputDirectory,
	                      bootcdDirectory,
	                      "" );
	FileLocation bootcdReactos ( OutputDirectory,
	                             bootcdDirectory + sSep + Environment::GetCdOutputPath (),
	                             "" );
	vector<FileLocation> vSourceFiles, vCdFiles;
	vector<FileLocation> vCdDirectories;

	// unattend.inf
	FileLocation srcunattend ( SourceDirectory,
	                           "boot" + sSep + "bootdata" + sSep + "bootcdregtest",
	                           "unattend.inf" );
	FileLocation tarunattend ( bootcdReactos.directory,
	                           bootcdReactos.relative_path,
	                           "unattend.inf" );
	if (module.type == IsoRegTest)
		vSourceFiles.push_back ( srcunattend );

	// bootsector
	const Module* bootModule = module.bootSector->bootSectorModule;

	if (!bootModule)
	{
		throw InvalidOperationException ( module.node.location.c_str(),
										  0,
										  "Invalid bootsector. module '%s' requires <bootsector>",
										  module.name.c_str ());
	}

	const FileLocation *isoboot = bootModule->output;
	vSourceFiles.push_back ( *isoboot );

	// prepare reactos.dff and reactos.inf
	FileLocation reactosDff ( SourceDirectory,
	                          "boot" + sSep + "bootdata" + sSep + "packages",
	                          "reactos.dff" );
	FileLocation reactosInf ( bootcdReactos.directory,
	                          bootcdReactos.relative_path,
	                          "reactos.inf" );

	vSourceFiles.push_back ( reactosDff );

	/*
		We use only the name and not full FileLocation(ouput) because Iso/LiveIso are an exception to the general rule.
		Iso/LiveIso outputs are generated in code base root
	*/
	string IsoName = module.output->name;

	string sourceFiles = v2s ( backend, vSourceFiles, 5 );

	// fill cdrom
	GetCdDirectories ( vCdDirectories, bootcdDirectory );
	GetCdFiles ( vCdFiles );
	string cdDirectories = "";//v2s ( vCdDirectories, 5 );
	string cdFiles = v2s ( backend, vCdFiles, 5 );

	fprintf ( fMakefile, ".PHONY: %s\n\n",
	          module.name.c_str ());
	fprintf ( fMakefile,
	          "%s: all %s %s %s $(CABMAN_TARGET) $(CDMAKE_TARGET) %s\n",
	          module.name.c_str (),
	          backend->GetFullName ( *isoboot ).c_str (),
	          sourceFiles.c_str (),
	          cdFiles.c_str (),
	          cdDirectories.c_str () );
	fprintf ( fMakefile,
	          "\t$(Q)$(CABMAN_TARGET) -C %s -L %s -I -P $(OUTPUT)\n",
	          backend->GetFullName ( reactosDff ).c_str (),
	          backend->GetFullPath ( bootcdReactos ).c_str () );
	fprintf ( fMakefile,
	          "\t$(Q)$(CABMAN_TARGET) -C %s -RC %s -L %s -N -P $(OUTPUT)\n",
	          backend->GetFullName ( reactosDff ).c_str (),
	          backend->GetFullName ( reactosInf ).c_str (),
	          backend->GetFullPath ( bootcdReactos ).c_str ());
	fprintf ( fMakefile,
	          "\t-@${rm} %s 2>$(NUL)\n",
	          backend->GetFullName ( reactosInf ).c_str () );
	OutputBootstrapfileCopyCommands ( bootcdDirectory );
	OutputCdfileCopyCommands ( bootcdDirectory );

	if (module.type == IsoRegTest)
		OutputCopyCommand ( srcunattend, tarunattend );

	fprintf ( fMakefile, "\t$(ECHO_CDMAKE)\n" );
	fprintf ( fMakefile,
	          "\t$(Q)$(CDMAKE_TARGET) -v -j -m -b %s %s REACTOS %s\n",
	          backend->GetFullName ( *isoboot ).c_str (),
	          backend->GetFullPath ( bootcd ).c_str (),
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
	FileLocation dir ( OutputDirectory,
	                   directory,
	                   "" );
	MingwModuleHandler::PassThruCacheDirectory ( &dir );
}

void
MingwLiveIsoModuleHandler::OutputModuleCopyCommands ( string& livecdDirectory,
                                                      string& reactosDirectory )
{
	for ( std::map<std::string, Module*>::const_iterator p = module.project.modules.begin (); p != module.project.modules.end (); ++ p )
	{
		const Module& m = *p->second;
		if ( !m.enabled )
			continue;
		if ( m.install )
		{
			const Module& aliasedModule = backend->GetAliasedModuleOrModule ( m  );
			FileLocation destination ( OutputDirectory,
			                           m.install->relative_path.length () > 0
			                               ? livecdDirectory + sSep + reactosDirectory + sSep + m.install->relative_path
			                               : livecdDirectory + sSep + reactosDirectory,
			                           m.install->name );
			OutputCopyCommand ( *aliasedModule.output,
			                    destination);
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
		FileLocation target ( OutputDirectory,
		                      installfile.target->relative_path.length () > 0
		                          ? livecdDirectory + sSep + reactosDirectory + sSep + installfile.target->relative_path
		                          : livecdDirectory + sSep + reactosDirectory,
		                      installfile.target->name );
		OutputCopyCommand ( *installfile.source, target );
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

	FileLocation livecdIni ( SourceDirectory,
	                         "boot" + sSep + "bootdata",
	                         "livecd.ini" );
	FileLocation destination ( OutputDirectory,
	                           livecdDirectory,
	                           "freeldr.ini" );
	OutputCopyCommand ( livecdIni,
	                    destination );
}

void
MingwLiveIsoModuleHandler::OutputLoaderCommands ( string& livecdDirectory )
{
	FileLocation freeldr ( OutputDirectory,
	                       "boot" + sSep + "freeldr" + sSep + "freeldr",
	                       "freeldr.sys" );
	FileLocation destination ( OutputDirectory,
	                           livecdDirectory + sSep + "loader",
	                           "setupldr.sys" );
	OutputCopyCommand ( freeldr,
	                    destination );
}

void
MingwLiveIsoModuleHandler::OutputRegistryCommands ( string& livecdDirectory )
{
	fprintf ( fMakefile, "# REGISTRY COMMANDS\n" );
	FileLocation reactosSystem32ConfigDirectory ( OutputDirectory,
	                                              livecdDirectory + sSep + "reactos" + sSep + "system32" + sSep + "config",
	                                              "" );
	fprintf ( fMakefile,
	          "\t$(ECHO_MKHIVE)\n" );
	fprintf ( fMakefile,
	          "\t$(MKHIVE_TARGET) boot%cbootdata %s $(ARCH) boot%cbootdata%clivecd.inf boot%cbootdata%chiveinst_$(ARCH).inf\n",
	          cSep, backend->GetFullPath ( reactosSystem32ConfigDirectory ).c_str (),
	          cSep, cSep, cSep, cSep );
}

void
MingwLiveIsoModuleHandler::GenerateLiveIsoModuleTarget ()
{
	fprintf ( fMakefile, "# LIVE ISO MODULE TARGET\n" );
	string livecdDirectory = module.name;
	FileLocation livecd ( OutputDirectory, livecdDirectory, "" );

	string IsoName;

	// bootsector
	const Module* bootModule = module.bootSector->bootSectorModule;

	if (!bootModule)
	{
		throw InvalidOperationException ( module.node.location.c_str(),
										  0,
										  "Invalid bootsector. module '%s' requires <bootsector>",
										  module.name.c_str ());
	}

	const FileLocation *isoboot = bootModule->output;

	/*
		We use only the name and not full FileLocation(ouput) because Iso/LiveIso are an exception to the general rule.
		Iso/LiveIso outputs are generated in code base root
	*/
	IsoName = module.output->name;

	string reactosDirectory = "reactos";
	string livecdReactosNoFixup = livecdDirectory + sSep + reactosDirectory;
	FileLocation livecdReactos ( OutputDirectory,
	                             livecdReactosNoFixup,
	                             "" );
	CLEAN_FILE ( livecdReactos );

	fprintf ( fMakefile, ".PHONY: %s\n\n",
	          module.name.c_str ());
	fprintf ( fMakefile,
	          "%s: all %s %s $(MKHIVE_TARGET) $(CDMAKE_TARGET)\n",
	          module.name.c_str (),
	          backend->GetFullName ( *isoboot) .c_str (),
	          backend->GetFullPath ( livecdReactos ).c_str () );
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
	          backend->GetFullName( *isoboot ).c_str (),
	          backend->GetFullPath ( livecd ).c_str (),
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

/* caller needs to delete the returned object */
void
MingwTestModuleHandler::GetModuleSpecificCompilationUnits ( vector<CompilationUnit*>& compilationUnits )
{
	compilationUnits.push_back ( new CompilationUnit ( new File ( IntermediateDirectory, module.output->relative_path + sSep + "..", module.name + "_hooks.c", false, "", false ) ) );
	compilationUnits.push_back ( new CompilationUnit ( new File ( IntermediateDirectory, module.output->relative_path + sSep + "..", module.name + "_stubs.S", false, "", false ) ) );
	compilationUnits.push_back ( new CompilationUnit ( new File ( IntermediateDirectory, module.output->relative_path + sSep + "..", module.name + "_startup.c", false, "", false ) ) );
}

void
MingwTestModuleHandler::GenerateTestModuleTarget ()
{
	string targetMacro ( GetTargetMacro ( module ) );
	string workingDirectory = GetWorkingDirectory ( );
	string libsMacro = GetLibsMacro ();

	GenerateImportLibraryTargetIfNeeded ();

	if ( module.non_if_data.compilationUnits.size () > 0 )
	{
		GenerateRules ();

		string dependencies = libsMacro + " " + objectsMacro;

		string linkerParameters = ssprintf ( "-subsystem=console -entry=%s -image-base=%s -file-alignment=0x1000 -section-alignment=0x1000",
		                                     module.GetEntryPoint(!(Environment::GetArch() == "arm")).c_str (),
		                                     module.baseaddress.c_str () );
		GenerateLinkerCommand ( dependencies,
		                        linkerParameters,
		                        "" );
	}
	else
	{
		GeneratePhonyTarget();
	}
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


MingwCabinetModuleHandler::MingwCabinetModuleHandler (
	const Module& module_ )

	: MingwModuleHandler ( module_ )
{
}

void
MingwCabinetModuleHandler::Process ()
{
	fprintf ( fMakefile, "# CABINET MODULE TARGET\n" );
	string targetMacro ( GetTargetMacro (module) );

	GenerateRules ();

	const FileLocation *target_file = GetTargetFilename ( module, NULL );
	fprintf ( fMakefile, "%s: $(CABMAN_TARGET) | %s\n",
	          targetMacro.c_str (),
	          backend->GetFullPath ( *target_file ).c_str () );

	fprintf ( fMakefile, "\t$(ECHO_CABMAN)\n" );
	fprintf ( fMakefile,
	          "\t$(Q)$(CABMAN_TARGET) -M raw -S %s $(%s_SOURCES)\n",      // Escape the asterisk for Make
	          targetMacro.c_str (),
			  module.name.c_str());
}

MingwElfExecutableModuleHandler::MingwElfExecutableModuleHandler (
	const Module& module_ )

	: MingwModuleHandler ( module_ )
{
}

void
MingwElfExecutableModuleHandler::Process ()
{
	string targetName ( module.output->name );
	string targetMacro ( GetTargetMacro (module) );
	string workingDirectory = GetWorkingDirectory ();
	string objectsMacro = GetObjectsMacro ( module );
	string libsMacro = GetLibsMacro ();
	string debugFormat = GetDebugFormat ();

	fprintf ( fMakefile, "# ELF EXECUTABLE TARGET\n" );
	GenerateRules ();

	const FileLocation *target_file = GetTargetFilename ( module, NULL );
	fprintf ( fMakefile, "%s: %s %s | %s\n",
	          targetMacro.c_str (),
	          objectsMacro.c_str (),
	          libsMacro.c_str (),
	          backend->GetFullPath ( *target_file ).c_str () );

	fprintf ( fMakefile, "\t$(ECHO_BOOTPROG)\n" );

	fprintf ( fMakefile, "\t${gcc} $(%s_LINKFORMAT) %s %s -g%s -o %s\n",
	          module.buildtype.c_str(),
	          objectsMacro.c_str(),
	          libsMacro.c_str(),
	          debugFormat.c_str(),
	          targetMacro.c_str () );

	delete target_file;
	fprintf ( fMakefile, "#/ELF EXECUTABLE TARGET\n" );
}
