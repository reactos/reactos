#include "../../pch.h"
#include <assert.h>

#include "../../rbuild.h"
#include "mingw.h"
#include "modulehandler.h"

using std::string;
using std::vector;

#define CLEAN_FILE(f) clean_files.push_back ( f ); /*if ( module.name == "crt" ) printf ( "%s(%i): clean: %s\n", __FILE__, __LINE__, f.c_str() )*/

static string ros_temp = "$(ROS_TEMPORARY)";
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
	const string &file, bool out )
{
	string directory ( GetDirectory ( RemoveVariables ( file ) ) );
	string generatedFilesDirectory = backend->AddDirectoryTarget ( directory, out );
	if ( directory.find ( generatedFilesDirectory ) != string::npos )
		/* This path already includes the generated files directory variable */
		return file;
	else
		return generatedFilesDirectory + SSEP + file;
}

/*static*/ string
MingwModuleHandler::GetTargetFilename (
	const Module& module,
	string_list* pclean_files )
{
	string target = PassThruCacheDirectory (
		FixupTargetFilename ( module.GetPath () ),
		true );
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
		FixupTargetFilename ( module.GetDependencyPath () ),
		true );
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
		case Test:
			handler = new MingwTestModuleHandler ( module );
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
		return PassThruCacheDirectory ( FixupTargetFilename ( basename + ".stubs.c" ),
		                                false );
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
		FixupTargetFilename ( module.GetPath () ),
		".temp.a" ), false );
}

bool
MingwModuleHandler::IsGeneratedFile ( const File& file ) const
{
	string extension = GetExtension ( file.name );
	return ( extension == ".spec" || extension == ".SPEC" );
}

string
MingwModuleHandler::GetImportLibraryDependency (
	const Module& importedModule )
{
	string dep;
	if ( importedModule.type == ObjectLibrary )
		dep = GetTargetMacro ( importedModule );
	else
		dep = GetImportLibraryFilename ( importedModule, NULL );
	return dep;
}

void
MingwModuleHandler::GetModuleDependencies (
	string_list& dependencies )
{
	size_t iend = module.dependencies.size ();

	// TODO FIXME - do we *really* not want to call
	// GetDefinitionDependencies() if dependencies.size() == 0 ???
	if ( iend == 0 )
		return;
	
	for ( size_t i = 0; i < iend; i++ )
	{
		const Dependency& dependency = *module.dependencies[i];
		const Module& dependencyModule = *dependency.dependencyModule;
		dependencyModule.GetTargets ( dependencies );
	}
	GetDefinitionDependencies ( dependencies );
}

void
MingwModuleHandler::GetSourceFilenames (
	string_list& list,
	bool includeGeneratedFiles ) const
{
	size_t i;

	const vector<File*>& files = module.non_if_data.files;
	for ( i = 0; i < files.size (); i++ )
	{
		if ( includeGeneratedFiles || !IsGeneratedFile ( *files[i] ) )
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
			if ( includeGeneratedFiles || !IsGeneratedFile ( file ) )
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
	string newExtension;
	string extension = GetExtension ( sourceFilename );
	if ( extension == ".rc" || extension == ".RC" )
		newExtension = ".coff";
	else if ( extension == ".spec" || extension == ".SPEC" )
		newExtension = ".stubs.o";
	else
		newExtension = ".o";
	string obj_file = PassThruCacheDirectory (
		FixupTargetFilename ( ReplaceExtension (
			RemoveVariables ( sourceFilename ),
			                  newExtension ) ),
			false );
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
	fprintf ( fMakefile, "%s_clean:\n\t-@$(rm)", module.name.c_str() );
	for ( size_t i = 0; i < clean_files.size(); i++ )
	{
		if ( 9==((i+1)%10) )
			fprintf ( fMakefile, " 2>$(NUL)\n\t-@$(rm)" );
		fprintf ( fMakefile, " %s", clean_files[i].c_str() );
	}
	fprintf ( fMakefile, " 2>$(NUL)\n" );
	fprintf ( fMakefile, "clean: %s_clean\n\n", module.name.c_str() );
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

string
MingwModuleHandler::GenerateGccDefineParametersFromVector (
	const vector<Define*>& defines ) const
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

string
MingwModuleHandler::GenerateGccIncludeParametersFromVector ( const vector<Include*>& includes ) const
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
	const IfableData& data,
	const vector<CompilerFlag*>* compilerFlags )
{
	size_t i;

	fprintf (
		fMakefile,
		"%s %s",
		macro.c_str(),
		assignmentOperation );
	
	if ( compilerFlags != NULL )
	{
		string compilerParameters = GenerateCompilerParametersFromVector ( *compilerFlags );
		if ( compilerParameters.size () > 0 )
		{
			fprintf (
				fMakefile,
				" %s",
				compilerParameters.c_str () );
		}
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
	const vector<CompilerFlag*>* compilerFlags,
	const vector<LinkerFlag*>* linkerFlags )
{
	size_t i;

	if ( data.includes.size () > 0 || data.defines.size () > 0 )
	{
		GenerateMacro ( assignmentOperation,
		                cflagsMacro,
		                data,
		                compilerFlags );
		GenerateMacro ( assignmentOperation,
		                windresflagsMacro,
		                data,
		                compilerFlags );
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
				NULL,
				NULL );
			fprintf ( 
				fMakefile,
				"endif\n\n" );
		}
	}
}

void
MingwModuleHandler::GenerateObjectMacros (
	const char* assignmentOperation,
	const IfableData& data,
	const vector<CompilerFlag*>* compilerFlags,
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
				NULL,
				NULL );
			fprintf ( 
				fMakefile,
				"endif\n\n" );
		}
	}
}

void
MingwModuleHandler::GenerateGccCommand (
	const string& sourceFilename,
	const string& cc,
	const string& cflagsMacro )
{
	string deps = sourceFilename;
	if ( module.pch && use_pch )
		deps += " " + module.pch->header + ".gch";
	string objectFilename = GetObjectFilename (
		sourceFilename, NULL );
	fprintf ( fMakefile,
	          "%s: %s %s\n",
	          objectFilename.c_str (),
	          deps.c_str (),
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
	string objectFilename = GetObjectFilename (
		sourceFilename, &clean_files );
	fprintf ( fMakefile,
	          "%s: %s %s\n",
	          objectFilename.c_str (),
	          sourceFilename.c_str (),
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
	string objectFilename = GetObjectFilename (
		sourceFilename, &clean_files );
	fprintf ( fMakefile,
	          "%s: %s %s\n",
	          objectFilename.c_str (),
	          sourceFilename.c_str (),
	          GetDirectory ( objectFilename ).c_str () );
	fprintf ( fMakefile, "\t$(ECHO_NASM)\n" );
	fprintf ( fMakefile,
	          "\t%s -f win32 $< -o $@ %s\n",
	          "$(Q)nasm",
	          nasmflagsMacro.c_str () );
}

void
MingwModuleHandler::GenerateWindresCommand (
	const string& sourceFilename,
	const string& windresflagsMacro )
{
	string objectFilename =
		GetObjectFilename ( sourceFilename, &clean_files );
	string rciFilename = ros_temp +
		ReplaceExtension ( sourceFilename, ".rci" );
	string resFilename = ros_temp +
		ReplaceExtension ( sourceFilename, ".res" );
	fprintf ( fMakefile,
	          "%s: %s %s $(WRC_TARGET)\n",
	          objectFilename.c_str (),
	          sourceFilename.c_str (),
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

void
MingwModuleHandler::GenerateWinebuildCommands (
	const string& sourceFilename )
{
	string basename = GetBasename ( sourceFilename );

	string def_file = PassThruCacheDirectory (
		basename + ".spec.def",
		false );
	CLEAN_FILE(def_file);

	string stub_file = PassThruCacheDirectory (
		basename + ".stubs.c",
		false );
	CLEAN_FILE(stub_file)

	fprintf ( fMakefile,
	          "%s: %s $(WINEBUILD_TARGET)\n",
	          def_file.c_str (),
	          sourceFilename.c_str () );
	fprintf ( fMakefile, "\t$(ECHO_WINEBLD)\n" );
	fprintf ( fMakefile,
	          "\t%s --def=%s -o %s\n",
	          "$(Q)$(WINEBUILD_TARGET)",
	          sourceFilename.c_str (),
	          def_file.c_str () );

	fprintf ( fMakefile,
	          "%s: %s $(WINEBUILD_TARGET)\n",
	          stub_file.c_str (),
	          sourceFilename.c_str () );
	fprintf ( fMakefile, "\t$(ECHO_WINEBLD)\n" );
	fprintf ( fMakefile,
	          "\t%s --pedll=%s -o %s\n",
	          "$(Q)$(WINEBUILD_TARGET)",
	          sourceFilename.c_str (),
	          stub_file.c_str () );
}

void
MingwModuleHandler::GenerateCommands (
	const string& sourceFilename,
	const string& cc,
	const string& cppc,
	const string& cflagsMacro,
	const string& nasmflagsMacro,
	const string& windresflagsMacro )
{
	string extension = GetExtension ( sourceFilename );
	if ( extension == ".c" || extension == ".C" )
	{
		GenerateGccCommand ( sourceFilename,
		                     cc,
		                     cflagsMacro );
		return;
	}
	else if ( extension == ".cc" || extension == ".CC" ||
	          extension == ".cpp" || extension == ".CPP" ||
	          extension == ".cxx" || extension == ".CXX" )
	{
		GenerateGccCommand ( sourceFilename,
		                     cppc,
		                     cflagsMacro );
		return;
	}
	else if ( extension == ".s" || extension == ".S" )
	{
		GenerateGccAssemblerCommand ( sourceFilename,
		                              cc,
		                              cflagsMacro );
		return;
	}
	else if ( extension == ".asm" || extension == ".ASM" )
	{
		GenerateNasmCommand ( sourceFilename,
		                      nasmflagsMacro );
		return;
	}
	else if ( extension == ".rc" || extension == ".RC" )
	{
		GenerateWindresCommand ( sourceFilename,
		                         windresflagsMacro );
		return;
	}
	else if ( extension == ".spec" || extension == ".SPEC" )
	{
		GenerateWinebuildCommands ( sourceFilename );
		GenerateGccCommand ( GetActualSourceFilename ( sourceFilename ),
		                     cc,
		                     cflagsMacro );
		return;
	}

	throw InvalidOperationException ( __FILE__,
	                                  __LINE__,
	                                  "Unsupported filename extension '%s' in file '%s'",
	                                  extension.c_str (),
	                                  sourceFilename.c_str () );
}

void
MingwModuleHandler::GenerateBuildMapCode ()
{
	fprintf ( fMakefile,
	          "ifeq ($(ROS_BUILDMAP),full)\n" );

	string mapFilename = PassThruCacheDirectory (
		GetBasename ( module.GetPath () ) + ".map",
		true );
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
MingwModuleHandler::GenerateLinkerCommand (
	const string& dependencies,
	const string& linker,
	const string& linkerParameters,
	const string& objectsMacro,
	const string& libsMacro )
{
	string target ( GetTargetMacro ( module ) );
	string target_folder ( GetDirectory ( GetTargetFilename ( module, NULL ) ) );

	fprintf ( fMakefile,
		"%s: %s %s $(RSYM_TARGET)\n",
		target.c_str (),
		dependencies.c_str (),
		target_folder.c_str () );
	fprintf ( fMakefile, "\t$(ECHO_LD)\n" );
	string targetName ( module.GetTargetName () );
	if ( module.importLibrary != NULL )
	{
		string base_tmp = ros_temp + module.name + ".base.tmp";
		CLEAN_FILE ( base_tmp );
		string junk_tmp = ros_temp + module.name + ".junk.tmp";
		CLEAN_FILE ( junk_tmp );
		string temp_exp = ros_temp + module.name + ".temp.exp";
		CLEAN_FILE ( temp_exp );
		string def_file = GetDefinitionFilename ();

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

	fprintf ( fMakefile,
	          "\t$(ECHO_RSYM)\n" );
	fprintf ( fMakefile,
	          "\t$(Q)$(RSYM_TARGET) $@ $@\n\n" );
}

void
MingwModuleHandler::GeneratePhonyTarget() const
{
	string targetMacro ( GetTargetMacro(module) );
	fprintf ( fMakefile, ".PHONY: %s\n\n",
	          targetMacro.c_str ());
	fprintf ( fMakefile, "%s: %s\n",
	          targetMacro.c_str (),
	          GetDirectory(GetTargetFilename(module,NULL)).c_str () );
}

void
MingwModuleHandler::GenerateObjectFileTargets (
	const IfableData& data,
	const string& cc,
	const string& cppc,
	const string& cflagsMacro,
	const string& nasmflagsMacro,
	const string& windresflagsMacro )
{
	size_t i;

	const vector<File*>& files = data.files;
	for ( i = 0; i < files.size (); i++ )
	{
		string sourceFilename = files[i]->name;
		GenerateCommands ( sourceFilename,
		                   cc,
		                   cppc,
		                   cflagsMacro,
		                   nasmflagsMacro,
		                   windresflagsMacro );
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
		                            windresflagsMacro );
	}
}

void
MingwModuleHandler::GenerateObjectFileTargets (
	const string& cc,
	const string& cppc,
	const string& cflagsMacro,
	const string& nasmflagsMacro,
	const string& windresflagsMacro )
{
	if ( module.pch )
	{
		const string& pch_file = module.pch->header;
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
	                            windresflagsMacro );
	fprintf ( fMakefile, "\n" );
}

string
MingwModuleHandler::GenerateArchiveTarget ( const string& ar,
                                            const string& objs_macro ) const
{
	string archiveFilename ( GetModuleArchiveFilename () );
	
	fprintf ( fMakefile,
	          "%s: %s %s\n",
	          archiveFilename.c_str (),
	          objs_macro.c_str (),
	          GetDirectory(archiveFilename).c_str() );

	fprintf ( fMakefile, "\t$(ECHO_AR)\n" );

	fprintf ( fMakefile,
	          "\t%s -rc $@ %s\n\n",
	          ar.c_str (),
	          objs_macro.c_str ());

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
	if ( module.type == ObjectLibrary )
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
		&module.compilerFlags,
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
MingwModuleHandler::GenerateOtherMacros ()
{
	cflagsMacro = ssprintf ("%s_CFLAGS", module.name.c_str ());
	nasmflagsMacro = ssprintf ("%s_NASMFLAGS", module.name.c_str ());
	windresflagsMacro = ssprintf ("%s_RCFLAGS", module.name.c_str ());
	linkerflagsMacro = ssprintf ("%s_LFLAGS", module.name.c_str ());
	libsMacro = ssprintf("%s_LIBS", module.name.c_str ());
	linkDepsMacro = ssprintf ("%s_LINKDEPS", module.name.c_str ());

	GenerateMacros (
		"=",
		module.non_if_data,
		&module.compilerFlags,
		&module.linkerFlags );

	if ( module.importLibrary )
	{
		string_list s;
		const vector<File*>& files = module.non_if_data.files;
		for ( size_t i = 0; i < files.size (); i++ )
		{
			File& file = *files[i];
			string extension = GetExtension ( file.name );
			if ( extension == ".spec" || extension == ".SPEC" )
				GetSpecObjectDependencies ( s, file.name );
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
	}

	string globalCflags = "-g";
	if ( backend->usePipe )
		globalCflags += " -pipe";
	
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

	fprintf ( fMakefile, "\n\n" );

	// future references to the macros will be to get their values
	cflagsMacro = ssprintf ("$(%s)", cflagsMacro.c_str ());
	nasmflagsMacro = ssprintf ("$(%s)", nasmflagsMacro.c_str ());
}

void
MingwModuleHandler::GenerateRules ()
{
	string cc = ( module.host == HostTrue ? "${host_gcc}" : "${gcc}" );
	string cppc = ( module.host == HostTrue ? "${host_gpp}" : "${gpp}" );
	string ar = ( module.host == HostTrue ? "${host_ar}" : "${ar}" );

	string targetMacro = GetTargetMacro ( module );

	CLEAN_FILE ( targetMacro );

	// generate phony target for module name
	fprintf ( fMakefile, ".PHONY: %s\n",
		module.name.c_str () );
	fprintf ( fMakefile, "%s: %s\n\n",
		module.name.c_str (),
		GetTargetMacro ( module ).c_str () );

	if ( module.type != ObjectLibrary )
	{
		string ar_target ( GenerateArchiveTarget ( ar, objectsMacro ) );
		if ( targetMacro != ar_target )
		{
			CLEAN_FILE ( ar_target );
		}
	}

	GenerateObjectFileTargets ( cc,
								cppc,
								cflagsMacro,
								nasmflagsMacro,
								windresflagsMacro );
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
		          FixupTargetFilename ( invoke.invokeModule->GetPath () ).c_str () );
		fprintf ( fMakefile, "\t$(ECHO_INVOKE)\n" );
		fprintf ( fMakefile,
		          "\t%s %s\n\n",
		          FixupTargetFilename ( invoke.invokeModule->GetPath () ).c_str (),
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
	string defFilename = module.GetBasePath () + SSEP + module.importLibrary->definition;
	if ( IsWineModule () )
		return PassThruCacheDirectory ( defFilename, false );
	else
		return defFilename;
}

void
MingwModuleHandler::GenerateImportLibraryTargetIfNeeded ()
{
	if ( module.importLibrary != NULL )
	{
		string library_target (
			GetImportLibraryFilename ( module, &clean_files ) );

		string_list deps;
		GetDefinitionDependencies ( deps );

		fprintf ( fMakefile, "# IMPORT LIBRARY RULE:\n" );

		fprintf ( fMakefile, "%s:",
		          library_target.c_str () );

		size_t i, iend = deps.size();
		for ( i = 0; i < iend; i++ )
			fprintf ( fMakefile, " %s",
			          deps[i].c_str () );

		fprintf ( fMakefile, " %s\n",
		          GetDirectory ( GetTargetFilename ( module, NULL ) ).c_str () );

		fprintf ( fMakefile, "\t$(ECHO_DLLTOOL)\n" );

		string killAt = module.mangledSymbols ? "" : "--kill-at";
		fprintf ( fMakefile,
		          "\t${dlltool} --dllname %s --def %s --output-lib %s %s\n\n",
		          module.GetTargetName ().c_str (),
		          GetDefinitionFilename ().c_str (),
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
	string defDependency = PassThruCacheDirectory ( FixupTargetFilename ( basename + ".spec.def" ), false );
	dependencies.push_back ( defDependency );
	string stubsDependency = PassThruCacheDirectory ( FixupTargetFilename ( basename + ".stubs.c" ), false );
	dependencies.push_back ( stubsDependency );
}

void
MingwModuleHandler::GetDefinitionDependencies (
	string_list& dependencies ) const
{
	string dkNkmLibNoFixup = "dk/nkm/lib";
	// TODO FIXME - verify this is the correct dependency...
	// easiest way to tell is to remove it and see what breaks
	/*dependencies += PassThruCacheDirectory (
		FixupTargetFilename ( dkNkmLibNoFixup ),
		false, NULL );*/
	const vector<File*>& files = module.non_if_data.files;
	for ( size_t i = 0; i < files.size (); i++ )
	{
		File& file = *files[i];
		string extension = GetExtension ( file.name );
		if ( extension == ".spec" || extension == ".SPEC" )
		{
			GetSpecObjectDependencies ( dependencies, file.name );
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
	
	fprintf ( fMakefile, "%s: %s %s %s\n",
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
	string targetName ( module.GetTargetName () ); // i.e. "ntoskrnl.exe"
	string targetMacro ( GetTargetMacro (module) ); // i.e. "$(NTOSKRNL_TARGET)"
	string workingDirectory = GetWorkingDirectory ();
	string objectsMacro = GetObjectsMacro ( module );
	string linkDepsMacro = GetLinkingDependenciesMacro ();
	string libsMacro = GetLibsMacro ();
	string base_tmp = ros_temp + module.name + ".base.tmp";
	CLEAN_FILE ( base_tmp );
	string junk_tmp = ros_temp + module.name + ".junk.tmp";
	CLEAN_FILE ( junk_tmp );
	string temp_exp = ros_temp + module.name + ".temp.exp";
	CLEAN_FILE ( temp_exp );
	string gccOptions = ssprintf ("-Wl,-T,%s" SSEP "ntoskrnl.lnk -Wl,--subsystem,native -Wl,--entry,%s -Wl,--image-base,%s -Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000 -nostartfiles -mdll",
	                              module.GetBasePath ().c_str (),
	                              module.entrypoint.c_str (),
	                              module.baseaddress.c_str () );

	GenerateRules ();

	GenerateImportLibraryTargetIfNeeded ();

	fprintf ( fMakefile, "%s: %s %s %s $(RSYM_TARGET)\n",
	          targetMacro.c_str (),
	          objectsMacro.c_str (),
	          linkDepsMacro.c_str (),
	          GetDirectory(GetTargetFilename(module,NULL)).c_str () );
	fprintf ( fMakefile, "\t$(ECHO_LD)\n" );
	fprintf ( fMakefile,
	          "\t${gcc} %s %s -Wl,--base-file,%s -o %s %s %s\n",
	          GetLinkerMacro ().c_str (),
	          gccOptions.c_str (),
	          base_tmp.c_str (),
	          junk_tmp.c_str (),
	          objectsMacro.c_str (),
	          linkDepsMacro.c_str () );
	fprintf ( fMakefile,
	          "\t-@${rm} %s 2>$(NUL)\n",
	          junk_tmp.c_str () );
	string killAt = module.mangledSymbols ? "" : "--kill-at";
	fprintf ( fMakefile,
	          "\t${dlltool} --dllname %s --base-file %s --def ntoskrnl/ntoskrnl.def --output-exp %s %s\n",
	          targetName.c_str (),
	          base_tmp.c_str (),
	          temp_exp.c_str (),
	          killAt.c_str () );
	fprintf ( fMakefile,
	          "\t-@${rm} %s 2>$(NUL)\n",
	          base_tmp.c_str () );
	fprintf ( fMakefile,
	          "\t${gcc} %s %s -Wl,%s -o $@ %s %s\n",
	          GetLinkerMacro ().c_str (),
	          gccOptions.c_str (),
	          temp_exp.c_str (),
	          objectsMacro.c_str (),
	          linkDepsMacro.c_str () );
	fprintf ( fMakefile,
	          "\t-@${rm} %s 2>$(NUL)\n",
	          temp_exp.c_str () );
	fprintf ( fMakefile, "\t$(ECHO_RSYM)\n" );
	fprintf ( fMakefile,
	          "\t$(Q)$(RSYM_TARGET) $@ $@\n\n" );
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
	string targetMacro ( GetTargetMacro (module) );
	string workingDirectory = GetWorkingDirectory ( );
	string objectsMacro = GetObjectsMacro ( module );
	string linkDepsMacro = GetLinkingDependenciesMacro ();
	string libsMacro = GetLibsMacro ();

	GenerateImportLibraryTargetIfNeeded ();

	if ( module.non_if_data.files.size () > 0 )
	{
		GenerateRules ();

		string dependencies =
			objectsMacro + " " + linkDepsMacro;

		string linkerParameters = ssprintf ( "-Wl,--subsystem,native -Wl,--entry,%s -Wl,--image-base,%s -Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000 -nostartfiles -mdll",
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

		string dependencies = objectsMacro + " " + linkDepsMacro;

		string linkerParameters = ssprintf ( "-Wl,--subsystem,native -Wl,--entry,%s -Wl,--image-base,%s -Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000 -nostartfiles -mdll",
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

		string dependencies =
			objectsMacro + " " + linkDepsMacro;

		string linkerParameters = ssprintf ( "-Wl,--subsystem,native -Wl,--entry,%s -Wl,--image-base,%s -Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000 -nostartfiles -nostdlib -mdll",
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

		string dependencies =
			objectsMacro + " " + linkDepsMacro;

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
	GenerateExtractWineDLLResourcesTarget ();
	GenerateWin32DLLModuleTarget ();
}

void
MingwWin32DLLModuleHandler::GenerateExtractWineDLLResourcesTarget ()
{
	fprintf ( fMakefile, ".PHONY: %s_extractresources\n\n",
	          module.name.c_str () );
	fprintf ( fMakefile, "%s_extractresources: $(BIN2RES_TARGET)\n",
	          module.name.c_str () );
	const vector<File*>& files = module.non_if_data.files;
	for ( size_t i = 0; i < files.size (); i++ )
	{
		File& file = *files[i];
		string extension = GetExtension ( file.name );
		if ( extension == ".rc" || extension == ".RC" )
		{
			string resource = FixupTargetFilename ( file.name );
			fprintf ( fMakefile, "\t$(ECHO_BIN2RES)\n" );
			fprintf ( fMakefile, "\t@:echo ${bin2res} -f -x %s\n",
			          resource.c_str () );
		}
	}
	fprintf ( fMakefile, "\n");
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

		string dependencies = objectsMacro + " " + linkDepsMacro;

		string linker;
		if ( module.cplusplus )
			linker = "${gpp}";
		else
			linker = "${gcc}";

		string linkerParameters = ssprintf ( "-Wl,--subsystem,console -Wl,--entry,%s -Wl,--image-base,%s -Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000 -mdll",
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

		string dependencies =
			objectsMacro + " " + linkDepsMacro;

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

		string dependencies =
			objectsMacro + " " + linkDepsMacro;

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

	fprintf ( fMakefile, "%s: %s %s %s\n",
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
		if ( m.bootstrap != NULL )
		{
			string targetFilenameNoFixup ( bootcdDirectory + SSEP + m.bootstrap->base + SSEP + m.bootstrap->nameoncd );
			string targetFilename ( GetTargetMacro ( module ) );
			fprintf ( fMakefile,
			          "\t$(ECHO_CP)\n" );
			fprintf ( fMakefile,
			          "\t${cp} %s %s 1>$(NUL)\n",
			          m.GetPath ().c_str (),
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
			FixupTargetFilename ( targetFilenameNoFixup ),
			true );
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
		if ( m.bootstrap != NULL )
		{
			string targetDirectory ( bootcdDirectory + SSEP + m.bootstrap->base );
			if ( directories.size () > 0 )
				directories += " ";
			directories += PassThruCacheDirectory (
				FixupTargetFilename ( targetDirectory ),
				true );
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
			FixupTargetFilename ( targetDirectory ),
			true );
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
		if ( m.bootstrap != NULL )
			out.push_back ( FixupTargetFilename ( m.GetPath () ) );
	}
}

void
MingwIsoModuleHandler::GetNonModuleCdFiles (
	vector<string>& out ) const
{
	for ( size_t i = 0; i < module.project.cdfiles.size (); i++ )
	{
		const CDFile& cdfile = *module.project.cdfiles[i];
		out.push_back ( NormalizeFilename ( cdfile.GetPath () ) );
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
	string isoboot = FixupTargetFilename ( "boot/freeldr/bootsect/isoboot.o" );
	string bootcdReactosNoFixup = bootcdDirectory + "/reactos";
	string bootcdReactos = PassThruCacheDirectory (
		FixupTargetFilename ( bootcdReactosNoFixup ),
		true );
	CLEAN_FILE ( bootcdReactos );
	string reactosInf = ros_temp + FixupTargetFilename ( bootcdReactosNoFixup + "/reactos.inf" );
	string reactosDff = NormalizeFilename ( "bootdata/packages/reactos.dff" );
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
	          "\t$(Q)$(CABMAN_TARGET) -C %s -L %s -I\n",
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
	          bootcdDirectory.c_str () );
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
MingwTestModuleHandler::GenerateTestModuleTarget ()
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

		string dependencies = objectsMacro + " " + linkDepsMacro;

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
