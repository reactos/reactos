#include "../../pch.h"
#include <assert.h>

#include "../../rbuild.h"
#include "mingw.h"
#include "modulehandler.h"

using std::string;
using std::vector;
using std::map;
using std::set;

typedef set<string> set_string;

map<ModuleType,MingwModuleHandler*>*
MingwModuleHandler::handler_map = NULL;
set_string
MingwModuleHandler::directory_set;
int
MingwModuleHandler::ref = 0;

FILE*
MingwModuleHandler::fMakefile = NULL;

string
ReplaceExtension ( const string& filename,
                   const string& newExtension )
{
	size_t index = filename.find_last_of ( '/' );
	if (index == string::npos) index = 0;
	string tmp = filename.substr( index, filename.size() - index );
	size_t ext_index = tmp.find_last_of( '.' );
	if (ext_index != string::npos) 
		return filename.substr ( 0, index + ext_index ) + newExtension;
	return filename + newExtension;
}


MingwModuleHandler::MingwModuleHandler ( ModuleType moduletype )
{
	if ( !ref++ )
		handler_map = new map<ModuleType,MingwModuleHandler*>;
	(*handler_map)[moduletype] = this;
}

MingwModuleHandler::~MingwModuleHandler()
{
	if ( !--ref )
	{
		delete handler_map;
		handler_map = NULL;
	}
}

const string &
MingwModuleHandler::PassThruCacheDirectory ( const string &file ) const 
{
	directory_set.insert ( GetDirectory ( file ) );
	return file;
}

void
MingwModuleHandler::SetMakefile ( FILE* f )
{
	fMakefile = f;
}

MingwModuleHandler*
MingwModuleHandler::LookupHandler ( const string& location,
                                    ModuleType moduletype )
{
	if ( !handler_map )
		throw Exception ( "internal tool error: no registered module handlers" );
	MingwModuleHandler* h = (*handler_map)[moduletype];
	if ( !h )
	{
		throw UnknownModuleTypeException ( location, moduletype );
		return NULL;
	}
	return h;
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
MingwModuleHandler::GetActualSourceFilename ( const string& filename ) const
{
	string extension = GetExtension ( filename );
	if ( extension == ".spec" || extension == "SPEC" )
	{
		string basename = GetBasename ( filename );
		return basename + ".stubs.c";
	}
	else
		return filename;
}

string
MingwModuleHandler::GetModuleArchiveFilename ( const Module& module ) const
{
	return ReplaceExtension ( FixupTargetFilename ( module.GetPath () ),
	                          ".a" );
}

bool
MingwModuleHandler::IsGeneratedFile ( const File& file ) const
{
	string extension = GetExtension ( file.name );
	if ( extension == ".spec" || extension == "SPEC" )
		return true;
	else
		return false;
}

string
MingwModuleHandler::GetImportLibraryDependency ( const Module& importedModule ) const
{
	if ( importedModule.type == ObjectLibrary )
		return GetObjectsMacro ( importedModule );
	else
		return PassThruCacheDirectory ( FixupTargetFilename ( importedModule.GetDependencyPath () ) );
}

string
MingwModuleHandler::GetImportLibraryDependencies ( const Module& module ) const
{
	string dependencies ( "" );
	for ( size_t i = 0; i < module.libraries.size (); i++ )
	{
		if ( dependencies.size () > 0 )
			dependencies += " ";
		const Module* importedModule = module.project.LocateModule ( module.libraries[i]->name );
		assert ( importedModule != NULL );
		dependencies += GetImportLibraryDependency ( *importedModule );
	}
	return dependencies;
}

string
MingwModuleHandler::GetModuleDependencies ( const Module& module ) const
{
	if ( module.dependencies.size () == 0 )
		return "";
	
	string dependencies ( "" );
	for ( size_t i = 0; i < module.dependencies.size (); i++ )
	{
		if ( dependencies.size () > 0 )
			dependencies += " ";
		const Dependency* dependency = module.dependencies[i];
		const Module* dependencyModule = dependency->dependencyModule;
		dependencies += dependencyModule->GetTargets ();
	}
	string definitionDependencies = GetDefinitionDependencies ( module );
	if ( dependencies.length () > 0 && definitionDependencies.length () > 0 )
		dependencies += " " + definitionDependencies;
	else if ( definitionDependencies.length () > 0 )
		dependencies = definitionDependencies;
	return dependencies;
}

string
MingwModuleHandler::GetAllDependencies ( const Module& module ) const
{
	string dependencies = GetImportLibraryDependencies ( module );
	string s = GetModuleDependencies ( module );
	if ( s.length () > 0 )
	{
		dependencies += " ";
		dependencies += s;
	}
	return dependencies;
}

string
MingwModuleHandler::GetSourceFilenames ( const Module& module,
	                                     bool includeGeneratedFiles ) const
{
	size_t i;

	string sourceFilenames ( "" );
	for ( i = 0; i < module.files.size (); i++ )
	{
		if ( includeGeneratedFiles || !IsGeneratedFile ( *module.files[i] ) )
			sourceFilenames += " " + GetActualSourceFilename ( module.files[i]->name );
	}
	vector<If*> ifs = module.ifs;
	for ( i = 0; i < ifs.size (); i++ )
	{
		size_t j;
		If& rIf = *ifs[i];
		for ( j = 0; j < rIf.ifs.size (); j++ )
			ifs.push_back ( rIf.ifs[j] );
		for ( j = 0; j < rIf.files.size (); j++ )
		{
			if ( includeGeneratedFiles || !IsGeneratedFile ( *rIf.files[j] ) )
				sourceFilenames += " " + GetActualSourceFilename ( rIf.files[j]->name );
		}
	}
	return sourceFilenames;
}

string
MingwModuleHandler::GetSourceFilenames ( const Module& module ) const
{
	return GetSourceFilenames ( module,
	                            true );
}

string
MingwModuleHandler::GetSourceFilenamesWithoutGeneratedFiles ( const Module& module ) const
{
	return GetSourceFilenames ( module,
	                            false );
}

string
MingwModuleHandler::GetObjectFilename ( const string& sourceFilename )
{
	string newExtension;
	string extension = GetExtension ( sourceFilename );
	if ( extension == ".rc" || extension == ".RC" )
		newExtension = ".coff";
	else if ( extension == ".spec" || extension == ".SPEC" )
		newExtension = ".stubs.o";
	else
		newExtension = ".o";
	return FixupTargetFilename ( ReplaceExtension ( sourceFilename, newExtension ) );
}

string
MingwModuleHandler::GetObjectFilenames ( const Module& module ) const
{
	if ( module.files.size () == 0 )
		return "";
	
	string objectFilenames ( "" );
	for ( size_t i = 0; i < module.files.size (); i++ )
	{
		if ( objectFilenames.size () > 0 )
			objectFilenames += " ";
		objectFilenames += PassThruCacheDirectory ( MingwModuleHandler::GetObjectFilename ( module.files[i]->name ) );
	}
	return objectFilenames;
}

bool
MingwModuleHandler::IncludeDirectoryTarget ( const string& directory ) const
{
	if ( directory == "$(ROS_INTERMEDIATE)." SSEP "tools")
		return false;
	else
		return true;
}

void
MingwModuleHandler::GenerateDirectoryTargets () const
{
	if ( directory_set.size () == 0 )
		return;
	
	set_string::iterator i;
	fprintf ( fMakefile, "directories::" );

	for ( i = directory_set.begin ();
	      i != directory_set.end ();
	      i++ )
	{
		if ( IncludeDirectoryTarget ( *i ) )
		{
			fprintf ( fMakefile,
			          " %s",
			          i->c_str () );
		}
	}

	fprintf ( fMakefile, "\n\n" );

	for ( i = directory_set.begin ();
	      i != directory_set.end ();
	      i++ )
	{
		if ( IncludeDirectoryTarget ( *i ) )
		{
			fprintf ( fMakefile,
			          "%s ",
			          i->c_str () );
		}
	}

	fprintf ( fMakefile, 
	          "::\n\t${mkdir} $@\n\n" );

	directory_set.clear ();
}

string
MingwModuleHandler::GenerateGccDefineParametersFromVector ( const vector<Define*>& defines ) const
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
MingwModuleHandler::GenerateGccDefineParameters ( const Module& module ) const
{
	string parameters = GenerateGccDefineParametersFromVector ( module.project.defines );
	string s = GenerateGccDefineParametersFromVector ( module.defines );
	if ( s.length () > 0 )
	{
		parameters += " ";
		parameters += s;
	}
	return parameters;
}

string
MingwModuleHandler::ConcatenatePaths ( const string& path1,
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
MingwModuleHandler::GenerateGccIncludeParameters ( const Module& module ) const
{
	string parameters = GenerateGccIncludeParametersFromVector ( module.includes );
	string s = GenerateGccIncludeParametersFromVector ( module.project.includes );
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
MingwModuleHandler::GenerateLinkerParameters ( const Module& module ) const
{
	return GenerateLinkerParametersFromVector ( module.linkerFlags );
}

void
MingwModuleHandler::GenerateMacro ( const char* assignmentOperation,
                                    const string& macro,
                                    const vector<Include*>& includes,
                                    const vector<Define*>& defines,
                                    const vector<CompilerFlag*>* compilerFlags ) const
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

	for ( i = 0; i < includes.size(); i++ )
	{
		fprintf (
			fMakefile,
			" -I%s",
			includes[i]->directory.c_str() );
	}
	for ( i = 0; i < defines.size(); i++ )
	{
		Define& d = *defines[i];
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
	const vector<File*>& files,
	const vector<Include*>& includes,
	const vector<Define*>& defines,
	const vector<CompilerFlag*>* compilerFlags,
	const vector<LinkerFlag*>* linkerFlags,
	const vector<If*>& ifs,
	const string& cflags_macro,
	const string& nasmflags_macro,
	const string& windresflags_macro,
	const string& linkerflags_macro,
	const string& objs_macro) const
{
	size_t i;

	if ( includes.size () > 0 || defines.size () > 0 )
	{
		GenerateMacro ( assignmentOperation,
		                cflags_macro,
		                includes,
		                defines,
		                compilerFlags );
		GenerateMacro ( assignmentOperation,
		                windresflags_macro,
		                includes,
		                defines,
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
				linkerflags_macro.c_str (),
				assignmentOperation,
				linkerParameters.c_str() );
		}
	}
	
	if ( files.size () > 0 )
	{
		for ( i = 0; i < files.size (); i++ )
		{
			if ( files[i]->first )
			{
				fprintf ( fMakefile,
					"%s := %s $(%s)\n",
					objs_macro.c_str(),
					PassThruCacheDirectory ( MingwModuleHandler::GetObjectFilename ( files[i]->name ) ).c_str (),
					objs_macro.c_str() );
			}
		}
		fprintf (
			fMakefile,
			"%s %s",
			objs_macro.c_str (),
			assignmentOperation );
		for ( i = 0; i < files.size(); i++ )
		{
			if ( !files[i]->first )
			{
				fprintf (
					fMakefile,
					"%s%s",
					( i%10 == 9 ? "\\\n\t" : " " ),
					PassThruCacheDirectory ( MingwModuleHandler::GetObjectFilename ( files[i]->name ) ).c_str () );
			}
		}
		fprintf ( fMakefile, "\n" );
	}

	for ( i = 0; i < ifs.size(); i++ )
	{
		If& rIf = *ifs[i];
		if ( rIf.defines.size() || rIf.includes.size() || rIf.files.size() || rIf.ifs.size() )
		{
			fprintf (
				fMakefile,
				"ifeq (\"$(%s)\",\"%s\")\n",
				rIf.property.c_str(),
				rIf.value.c_str() );
			GenerateMacros (
				"+=",
				rIf.files,
				rIf.includes,
				rIf.defines,
				NULL,
				NULL,
				rIf.ifs,
				cflags_macro,
				nasmflags_macro,
				windresflags_macro,
				linkerflags_macro,
				objs_macro );
			fprintf ( 
				fMakefile,
				"endif\n\n" );
		}
	}
}

void
MingwModuleHandler::GenerateMacros (
	const Module& module,
	const string& cflags_macro,
	const string& nasmflags_macro,
	const string& windresflags_macro,
	const string& linkerflags_macro,
	const string& objs_macro) const
{
	GenerateMacros (
		"=",
		module.files,
		module.includes,
		module.defines,
		&module.compilerFlags,
		&module.linkerFlags,
		module.ifs,
		cflags_macro,
		nasmflags_macro,
		windresflags_macro,
		linkerflags_macro,
		objs_macro );
	fprintf ( fMakefile, "\n" );

	fprintf (
		fMakefile,
		"%s += $(PROJECT_CFLAGS)\n\n",
		cflags_macro.c_str () );

	fprintf (
		fMakefile,
		"%s += $(PROJECT_RCFLAGS)\n\n",
		windresflags_macro.c_str () );

	fprintf (
		fMakefile,
		"%s_LFLAGS += $(PROJECT_LFLAGS)\n\n",
		module.name.c_str () );
}

void
MingwModuleHandler::GenerateGccCommand ( const Module& module,
                                         const string& sourceFilename,
                                         const string& cc,
                                         const string& cflagsMacro ) const
{
	string objectFilename = PassThruCacheDirectory ( MingwModuleHandler::GetObjectFilename ( sourceFilename ) );
	fprintf ( fMakefile,
	          "%s: %s\n",
	          objectFilename.c_str (),
	          sourceFilename.c_str () );
	fprintf ( fMakefile,
	         "\t%s -c %s -o %s %s\n",
	         cc.c_str (),
	         sourceFilename.c_str (),
	         objectFilename.c_str (),
	         cflagsMacro.c_str () );
}

void
MingwModuleHandler::GenerateGccAssemblerCommand ( const Module& module,
                                                  const string& sourceFilename,
                                                  const string& cc,
                                                  const string& cflagsMacro ) const
{
	string objectFilename = PassThruCacheDirectory ( MingwModuleHandler::GetObjectFilename ( sourceFilename ) );
	fprintf ( fMakefile,
	          "%s: %s\n",
	          objectFilename.c_str (),
	          sourceFilename.c_str () );
	fprintf ( fMakefile,
	          "\t%s -x assembler-with-cpp -c %s -o %s -D__ASM__ %s\n",
	          cc.c_str (),
	          sourceFilename.c_str (),
	          objectFilename.c_str (),
	          cflagsMacro.c_str () );
}

void
MingwModuleHandler::GenerateNasmCommand ( const Module& module,
                                          const string& sourceFilename,
                                          const string& nasmflagsMacro ) const
{
	string objectFilename = PassThruCacheDirectory ( MingwModuleHandler::GetObjectFilename ( sourceFilename ) );
	fprintf ( fMakefile,
	          "%s: %s\n",
	          objectFilename.c_str (),
	          sourceFilename.c_str () );
	fprintf ( fMakefile,
	          "\t%s -f win32 %s -o %s %s\n",
	          "nasm",
	          sourceFilename.c_str (),
	          objectFilename.c_str (),
	          nasmflagsMacro.c_str () );
}

void
MingwModuleHandler::GenerateWindresCommand ( const Module& module,
                                             const string& sourceFilename,
                                             const string& windresflagsMacro ) const
{
	string objectFilename = PassThruCacheDirectory ( MingwModuleHandler::GetObjectFilename ( sourceFilename ) );
  string rciFilename = ReplaceExtension ( sourceFilename,
                                          ".rci" );
  string resFilename = ReplaceExtension ( sourceFilename,
                                          ".res" );
	fprintf ( fMakefile,
	          "%s: %s\n",
	          objectFilename.c_str (),
	          sourceFilename.c_str () );
	fprintf ( fMakefile,
	         "\t${gcc} -xc -E -DRC_INVOKED ${%s} %s > %s\n",
	         windresflagsMacro.c_str (),
	         sourceFilename.c_str (),
	         rciFilename.c_str () );
	fprintf ( fMakefile,
	         "\t${wrc} ${%s} %s %s\n",
           windresflagsMacro.c_str (),
	         rciFilename.c_str (),
	         resFilename.c_str () );
	fprintf ( fMakefile,
	         "\t${rm} %s\n",
	         rciFilename.c_str () );
	fprintf ( fMakefile,
	         "\t${windres} %s -o %s\n",
	         resFilename.c_str (),
	         objectFilename.c_str () );
	fprintf ( fMakefile,
	         "\t${rm} %s\n",
	         resFilename.c_str () );
}

void
MingwModuleHandler::GenerateWinebuildCommands ( const Module& module,
                                                const string& sourceFilename ) const
{
	string basename = GetBasename ( sourceFilename );
	fprintf ( fMakefile,
	          "%s.spec.def: %s\n",
	          basename.c_str (),
	          sourceFilename.c_str () );
	fprintf ( fMakefile,
	          "\t%s --def=%s -o %s.spec.def\n",
	          "${winebuild}",
	          sourceFilename.c_str (),
	          basename.c_str () );

	fprintf ( fMakefile,
	          "%s.stubs.c: %s\n",
	          basename.c_str (),
	          sourceFilename.c_str () );
	fprintf ( fMakefile,
	          "\t%s --pedll=%s -o %s.stubs.c\n",
	          "${winebuild}",
	          sourceFilename.c_str (),
	          basename.c_str () );
}

void
MingwModuleHandler::GenerateCommands ( const Module& module,
                                       const string& sourceFilename,
                                       const string& cc,
                                       const string& cppc,
                                       const string& cflagsMacro,
                                       const string& nasmflagsMacro,
                                       const string& windresflagsMacro ) const
{
	string extension = GetExtension ( sourceFilename );
	if ( extension == ".c" || extension == ".C" )
	{
		GenerateGccCommand ( module,
		                     sourceFilename,
		                     cc,
		                     cflagsMacro );
		return;
	}
	else if ( extension == ".cc" || extension == ".CC" ||
	          extension == ".cpp" || extension == ".CPP" ||
	          extension == ".cxx" || extension == ".CXX" )
	{
		GenerateGccCommand ( module,
		                     sourceFilename,
		                     cppc,
		                     cflagsMacro );
		return;
	}
	else if ( extension == ".s" || extension == ".S" )
	{
		GenerateGccAssemblerCommand ( module,
		                              sourceFilename,
		                              cc,
		                              cflagsMacro );
		return;
	}
	else if ( extension == ".asm" || extension == ".ASM" )
	{
		GenerateNasmCommand ( module,
		                      sourceFilename,
		                      nasmflagsMacro );
		return;
	}
	else if ( extension == ".rc" || extension == ".RC" )
	{
		GenerateWindresCommand ( module,
		                         sourceFilename,
		                         windresflagsMacro );
		return;
	}
	else if ( extension == ".spec" || extension == ".SPEC" )
	{
		GenerateWinebuildCommands ( module,
		                            sourceFilename );
		GenerateGccCommand ( module,
		                     GetActualSourceFilename ( sourceFilename ),
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
MingwModuleHandler::GenerateLinkerCommand ( const Module& module,
                                            const string& linker,
                                            const string& linkerParameters,
                                            const string& objectFilenames ) const
{
	string targetName ( module.GetTargetName () );
	string target ( FixupTargetFilename ( module.GetPath () ) );
	string importLibraryDependencies = GetImportLibraryDependencies ( module );
	if ( module.importLibrary != NULL )
	{
		static string ros_junk ( "$(ROS_TEMPORARY)" );
		string base_tmp = ros_junk + module.name + ".base.tmp";
		string junk_tmp = ros_junk + module.name + ".junk.tmp";
		string temp_exp = ros_junk + module.name + ".temp.exp";

		fprintf ( fMakefile,
		          "\t%s %s -Wl,--base-file,%s -o %s %s %s %s\n",
		          linker.c_str (),
		          linkerParameters.c_str (),
		          base_tmp.c_str (),
		          junk_tmp.c_str (),
		          objectFilenames.c_str (),
		          importLibraryDependencies.c_str (),
		          GetLinkerMacro ( module ).c_str () );

		fprintf ( fMakefile,
		          "\t${rm} %s\n",
		          junk_tmp.c_str () );

		string killAt = module.mangledSymbols ? "" : "--kill-at";
		fprintf ( fMakefile,
		          "\t${dlltool} --dllname %s --base-file %s --def %s --output-exp %s %s\n",
		          targetName.c_str (),
		          base_tmp.c_str (),
		          ( module.GetBasePath () + SSEP + module.importLibrary->definition ).c_str (),
		          temp_exp.c_str (),
		          killAt.c_str () );

		fprintf ( fMakefile,
		          "\t${rm} %s\n",
		          base_tmp.c_str () );

		fprintf ( fMakefile,
		          "\t%s %s %s -o %s %s %s %s\n\n",
		          linker.c_str (),
		          linkerParameters.c_str (),
		          temp_exp.c_str (),
		          target.c_str (),
		          objectFilenames.c_str (),
		          importLibraryDependencies.c_str (),
		          GetLinkerMacro ( module ).c_str () );

		fprintf ( fMakefile,
		          "\t${rm} %s\n\n",
		          temp_exp.c_str () );
	}
	else
	{
		fprintf ( fMakefile,
		          "\t%s %s -o %s %s %s %s\n",
		          linker.c_str (),
		          linkerParameters.c_str (),
		          target.c_str (),
		          objectFilenames.c_str (),
		          importLibraryDependencies.c_str (),
		          GetLinkerMacro ( module ).c_str () );
	}

	fprintf ( fMakefile,
		      "\t${rsym} %s %s\n\n",
		      target.c_str (),
		      target.c_str () );
}

void
MingwModuleHandler::GenerateObjectFileTargets ( const Module& module,
                                                const vector<File*>& files,
                                                const vector<If*>& ifs,
                                                const string& cc,
                                                const string& cppc,
                                                const string& cflagsMacro,
                                                const string& nasmflagsMacro,
                                                const string& windresflagsMacro ) const
{
	size_t i;

	for ( i = 0; i < files.size (); i++ )
	{
		string sourceFilename = files[i]->name;
		GenerateCommands ( module,
		                   sourceFilename,
		                   cc,
		                   cppc,
		                   cflagsMacro,
		                   nasmflagsMacro,
		                   windresflagsMacro );
		fprintf ( fMakefile,
		          "\n" );
	}

	for ( i = 0; i < ifs.size(); i++ )
	{
		GenerateObjectFileTargets ( module,
		                            ifs[i]->files,
		                            ifs[i]->ifs,
		                            cc,
		                            cppc,
		                            cflagsMacro,
		                            nasmflagsMacro,
		                            windresflagsMacro );
	}
}

void
MingwModuleHandler::GenerateObjectFileTargets ( const Module& module,
                                                const string& cc,
                                                const string& cppc,
                                                const string& cflagsMacro,
                                                const string& nasmflagsMacro,
                                                const string& windresflagsMacro ) const
{
	GenerateObjectFileTargets ( module,
	                            module.files,
	                            module.ifs,
	                            cc,
	                            cppc,
	                            cflagsMacro,
	                            nasmflagsMacro,
	                            windresflagsMacro );
	fprintf ( fMakefile, "\n" );
}

void
MingwModuleHandler::GetCleanTargets ( vector<string>& out,
                                      const vector<File*>& files,
                                      const vector<If*>& ifs ) const
{
	size_t i;

	for ( i = 0; i < files.size(); i++ )
		out.push_back ( PassThruCacheDirectory ( MingwModuleHandler::GetObjectFilename ( files[i]->name ) ) );

	for ( i = 0; i < ifs.size(); i++ )
		GetCleanTargets ( out, ifs[i]->files, ifs[i]->ifs );
}

string
MingwModuleHandler::GenerateArchiveTarget ( const Module& module,
                                            const string& ar,
                                            const string& objs_macro ) const
{
	string archiveFilename = GetModuleArchiveFilename ( module );
	
	fprintf ( fMakefile,
	          "%s: %s\n",
	          archiveFilename.c_str (),
	          objs_macro.c_str ());

	fprintf ( fMakefile,
	          "\t%s -rc %s %s\n\n",
	          ar.c_str (),
	          archiveFilename.c_str (),
	          objs_macro.c_str ());

	return archiveFilename;
}

string
MingwModuleHandler::GetCFlagsMacro ( const Module& module ) const
{
	return ssprintf ( "$(%s_CFLAGS)",
	                  module.name.c_str () );
}

string
MingwModuleHandler::GetObjectsMacro ( const Module& module ) const
{
	return ssprintf ( "$(%s_OBJS)",
	                  module.name.c_str () );
}

string
MingwModuleHandler::GetLinkerMacro ( const Module& module ) const
{
	return ssprintf ( "$(%s_LFLAGS)",
	                  module.name.c_str () );
}
	                                    
void
MingwModuleHandler::GenerateMacrosAndTargets (
	const Module& module,
	const string& cc,
	const string& cppc,
	const string& ar,
	const string* cflags,
	const string* nasmflags ) const
{
	string cflagsMacro = ssprintf ("%s_CFLAGS", module.name.c_str ());
	string nasmflagsMacro = ssprintf ("%s_NASMFLAGS", module.name.c_str ());
	string windresflagsMacro = ssprintf ("%s_RCFLAGS", module.name.c_str ());
	string linkerFlagsMacro = ssprintf ("%s_LFLAGS", module.name.c_str ());
	string objectsMacro = ssprintf ("%s_OBJS", module.name.c_str ());

	GenerateMacros ( module,
	                cflagsMacro,
	                nasmflagsMacro,
	                windresflagsMacro,
	                linkerFlagsMacro,
	                objectsMacro );

	if ( cflags != NULL )
	{
		fprintf ( fMakefile,
		          "%s += %s\n\n",
		          cflagsMacro.c_str (),
		          cflags->c_str () );
	}

	if ( nasmflags != NULL )
	{
		fprintf ( fMakefile,
		          "%s += %s\n\n",
		          nasmflagsMacro.c_str (),
		          nasmflags->c_str () );
	}

	// generate phony target for module name
	fprintf ( fMakefile, ".PHONY: %s\n",
		module.name.c_str () );
	fprintf ( fMakefile, "%s: %s\n\n",
		module.name.c_str (),
		FixupTargetFilename ( module.GetPath () ).c_str () );

	// future references to the macros will be to get their values
	cflagsMacro = ssprintf ("$(%s)", cflagsMacro.c_str ());
	nasmflagsMacro = ssprintf ("$(%s)", nasmflagsMacro.c_str ());
	objectsMacro = ssprintf ("$(%s)", objectsMacro.c_str ());

	string ar_target = GenerateArchiveTarget ( module, ar, objectsMacro );
	GenerateObjectFileTargets ( module,
	                            cc,
	                            cppc,
	                            cflagsMacro,
	                            nasmflagsMacro,
	                            windresflagsMacro );

	vector<string> clean_files;
	clean_files.push_back ( FixupTargetFilename(module.GetPath()) );
	clean_files.push_back ( ar_target );
	GetCleanTargets ( clean_files, module.files, module.ifs );

	fprintf ( fMakefile, ".PHONY: %s_clean\n", module.name.c_str() );
	fprintf ( fMakefile, "%s_clean:\n\t-@$(rm)", module.name.c_str() );
	for ( size_t i = 0; i < clean_files.size(); i++ )
	{
		if ( 9==(i%10) )
			fprintf ( fMakefile, " 2>$(NUL)\n\t-@$(rm)" );
		fprintf ( fMakefile, " %s", clean_files[i].c_str() );
	}
	fprintf ( fMakefile, " 2>$(NUL)\n" );
	fprintf ( fMakefile, "clean: %s_clean\n\n", module.name.c_str() );
}

void
MingwModuleHandler::GenerateMacrosAndTargetsHost ( const Module& module ) const
{
	GenerateMacrosAndTargets ( module,
	                           "${host_gcc}",
	                           "${host_gpp}",
	                           "${host_ar}",
	                           NULL,
	                           NULL );
}

void
MingwModuleHandler::GenerateMacrosAndTargetsTarget ( const Module& module ) const
{
	GenerateMacrosAndTargetsTarget ( module,
	                                 NULL,
	                                 NULL );
}

void
MingwModuleHandler::GenerateMacrosAndTargetsTarget ( const Module& module,
	                                                 const string* cflags,
	                                                 const string* nasmflags ) const
{
	GenerateMacrosAndTargets ( module,
	                          "${gcc}",
	                          "${gpp}",
	                          "${ar}",
	                          cflags,
	                          nasmflags );
}

string
MingwModuleHandler::GetInvocationDependencies ( const Module& module ) const
{
	string dependencies;
	for ( size_t i = 0; i < module.invocations.size (); i++ )
	{
		Invoke& invoke = *module.invocations[i];
		if (invoke.invokeModule == &module)
			/* Protect against circular dependencies */
			continue;
		if ( dependencies.length () > 0 )
			dependencies += " ";
		dependencies += invoke.GetTargets ();
	}
	return dependencies;
}

void
MingwModuleHandler::GenerateInvocations ( const Module& module ) const
{
	if ( module.invocations.size () == 0 )
		return;
	
	for ( size_t i = 0; i < module.invocations.size (); i++ )
	{
		const Invoke& invoke = *module.invocations[i];

		if ( invoke.invokeModule->type != BuildTool )
		{
			throw InvalidBuildFileException ( module.node.location,
			                                  "Only modules of type buildtool can be invoked." );
		}

		string invokeTarget = module.GetInvocationTarget ( i );
		fprintf ( fMakefile,
		          ".PHONY: %s\n\n",
		          invokeTarget.c_str () );
		fprintf ( fMakefile,
		          "%s: %s\n\n",
		          invokeTarget.c_str (),
		          invoke.GetTargets ().c_str () );
		fprintf ( fMakefile,
		          "%s: %s\n",
		          invoke.GetTargets ().c_str (),
		          FixupTargetFilename ( invoke.invokeModule->GetPath () ).c_str () );
		fprintf ( fMakefile,
		          "\t%s %s\n\n",
		          FixupTargetFilename ( invoke.invokeModule->GetPath () ).c_str (),
		          invoke.GetParameters ().c_str () );
	}
}

string
MingwModuleHandler::GetPreconditionDependenciesName ( const Module& module ) const
{
	return ssprintf ( "%s_precondition",
	                  module.name.c_str () );
}

string
MingwModuleHandler::GetDefaultDependencies ( const Module& module ) const
{
	/* Avoid circular dependency */
	if ( module.type == BuildTool || module.name == "zlib" )
		return "$(ROS_INTERMEDIATE)." SSEP "tools $(ROS_INTERMEDIATE)." SSEP "lib" SSEP "zlib";
	else
		return "init";
}

void
MingwModuleHandler::GeneratePreconditionDependencies ( const Module& module ) const
{
	string preconditionDependenciesName = GetPreconditionDependenciesName ( module );
	string sourceFilenames = GetSourceFilenamesWithoutGeneratedFiles ( module );
	string dependencies = GetDefaultDependencies ( module );
	string s = GetModuleDependencies ( module );
	if ( s.length () > 0 )
	{
		if ( dependencies.length () > 0 )
			dependencies += " ";
		dependencies += s;
	}

	s = GetInvocationDependencies ( module );
	if ( s.length () > 0 )
	{
		if ( dependencies.length () > 0 )
			dependencies += " ";
		dependencies += s;
	}
	
	fprintf ( fMakefile,
	          ".PHONY: %s\n\n",
	          preconditionDependenciesName.c_str () );
	fprintf ( fMakefile,
	          "%s: %s\n\n",
	          preconditionDependenciesName.c_str (),
	          dependencies.c_str () );
	const char* p = sourceFilenames.c_str();
	const char* end = p + strlen(p);
	while ( p < end )
	{
		const char* p2 = &p[512];
		if ( p2 > end )
			p2 = end;
		while ( p2 > p && !isspace(*p2) )
			--p2;
		if ( p == p2 )
		{
			p2 = strpbrk ( p, " \t" );
			if ( !p2 )
				p2 = end;
		}
		fprintf ( fMakefile,
		          "%.*s: %s\n",
		          p2-p,
		          p,
		          preconditionDependenciesName.c_str ());
		p = p2;
		p += strspn ( p, " \t" );
	}
	fprintf ( fMakefile, "\n" );
}

void
MingwModuleHandler::GenerateImportLibraryTargetIfNeeded ( const Module& module ) const
{
	if ( module.importLibrary != NULL )
	{
		string definitionDependencies = GetDefinitionDependencies ( module );
		fprintf ( fMakefile, "%s: %s\n",
		          FixupTargetFilename( module.GetDependencyPath () ).c_str (),
		          definitionDependencies.c_str () );

		string killAt = module.mangledSymbols ? "" : "--kill-at";
		fprintf ( fMakefile,
		          "\t${dlltool} --dllname %s --def %s --output-lib %s %s\n\n",
		          module.GetTargetName ().c_str (),
		          ( module.GetBasePath () + SSEP + module.importLibrary->definition ).c_str (),
		          FixupTargetFilename ( module.GetDependencyPath () ).c_str (),
		          killAt.c_str () );
	}
}

string
MingwModuleHandler::GetSpecObjectDependencies ( const string& filename ) const
{
	string basename = GetBasename ( filename );
	return basename + ".spec.def" + " " + basename + ".stubs.c";
}

string
MingwModuleHandler::GetDefinitionDependencies ( const Module& module ) const
{
	string dependencies;
	string dkNkmLibNoFixup = "dk/nkm/lib";
	dependencies += FixupTargetFilename ( dkNkmLibNoFixup );
	PassThruCacheDirectory ( dkNkmLibNoFixup + SSEP );
	for ( size_t i = 0; i < module.files.size (); i++ )
	{
		File& file = *module.files[i];
		string extension = GetExtension ( file.name );
		if ( extension == ".spec" || extension == ".SPEC" )
		{
			if ( dependencies.length () > 0 )
				dependencies += " ";
			dependencies += GetSpecObjectDependencies ( file.name );
		}
	}
	return dependencies;
}

string
MingwModuleHandler::GetLinkingDependencies ( const Module& module ) const
{
	string dependencies = GetImportLibraryDependencies ( module );
	string s = GetDefinitionDependencies ( module );
	if ( s.length () > 0 )
	{
		dependencies += " ";
		dependencies += s;
	}
	return dependencies;
}

bool
MingwModuleHandler::IsCPlusPlusModule ( const Module& module ) const
{
	if ( module.HasFileWithExtensions ( ".cc", ".CC" ) )
		return true;
	if ( module.HasFileWithExtensions ( ".cxx", ".CXX" ) )
		return true;
	if ( module.HasFileWithExtensions ( ".cpp", ".CPP" ) )
		return true;
	return false;
}


static MingwBuildToolModuleHandler buildtool_handler;

MingwBuildToolModuleHandler::MingwBuildToolModuleHandler()
	: MingwModuleHandler ( BuildTool )
{
}

void
MingwBuildToolModuleHandler::Process ( const Module& module )
{
	GeneratePreconditionDependencies ( module );
	GenerateBuildToolModuleTarget ( module );
	GenerateInvocations ( module );
}

void
MingwBuildToolModuleHandler::GenerateBuildToolModuleTarget ( const Module& module )
{
	string target ( FixupTargetFilename ( module.GetPath () ) );
	string objectsMacro = GetObjectsMacro ( module );
	string importLibraryDependencies = GetImportLibraryDependencies ( module );

	GenerateMacrosAndTargetsHost ( module );

	string linker;
	if ( IsCPlusPlusModule ( module ) )
		linker = "${host_gpp}";
	else
		linker = "${host_gcc}";
	
	fprintf ( fMakefile, "%s: %s %s\n",
	          target.c_str (),
	          objectsMacro.c_str (),
	          importLibraryDependencies.c_str () );
	fprintf ( fMakefile,
	          "\t%s %s -o %s %s %s\n\n",
	          linker.c_str (),
	          GetLinkerMacro ( module ).c_str (),
	          target.c_str (),
	          objectsMacro.c_str (),
	          importLibraryDependencies.c_str () );
}


static MingwKernelModuleHandler kernelmodule_handler;

MingwKernelModuleHandler::MingwKernelModuleHandler ()
	: MingwModuleHandler ( Kernel )
{
}

void
MingwKernelModuleHandler::Process ( const Module& module )
{
	GeneratePreconditionDependencies ( module );
	GenerateKernelModuleTarget ( module );
	GenerateInvocations ( module );
}

void
MingwKernelModuleHandler::GenerateKernelModuleTarget ( const Module& module )
{
	static string ros_junk ( "$(ROS_TEMPORARY)" );
	string targetName ( module.GetTargetName () );
	string target ( FixupTargetFilename (module.GetPath ()) );
	string workingDirectory = GetWorkingDirectory ();
	string objectsMacro = GetObjectsMacro ( module );
	string importLibraryDependencies = GetImportLibraryDependencies ( module );
	string base_tmp = ros_junk + module.name + ".base.tmp";
	string junk_tmp = ros_junk + module.name + ".junk.tmp";
	string temp_exp = ros_junk + module.name + ".temp.exp";
	string gccOptions = ssprintf ("-Wl,-T,%s" SSEP "ntoskrnl.lnk -Wl,--subsystem,native -Wl,--entry,%s -Wl,--image-base,%s -Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000 -nostartfiles -mdll",
	                              module.GetBasePath ().c_str (),
	                              module.entrypoint.c_str (),
	                              module.baseaddress.c_str () );

	GenerateMacrosAndTargetsTarget ( module );

	GenerateImportLibraryTargetIfNeeded ( module );

	fprintf ( fMakefile, "%s: %s %s\n",
	          target.c_str (),
	          objectsMacro.c_str (),
	          importLibraryDependencies.c_str () );
	fprintf ( fMakefile,
	          "\t${gcc} %s %s -Wl,--base-file,%s -o %s %s %s\n",
	          GetLinkerMacro ( module ).c_str (),
	          gccOptions.c_str (),
	          base_tmp.c_str (),
	          junk_tmp.c_str (),
	          objectsMacro.c_str (),
	          importLibraryDependencies.c_str () );
	fprintf ( fMakefile,
	          "\t${rm} %s\n",
	          junk_tmp.c_str () );
	string killAt = module.mangledSymbols ? "" : "--kill-at";
	fprintf ( fMakefile,
	          "\t${dlltool} --dllname %s --base-file %s --def ntoskrnl/ntoskrnl.def --output-exp %s %s\n",
	          targetName.c_str (),
	          base_tmp.c_str (),
	          temp_exp.c_str (),
	          killAt.c_str () );
	fprintf ( fMakefile,
	          "\t${rm} %s\n",
	          base_tmp.c_str () );
	fprintf ( fMakefile,
	          "\t${gcc} %s %s -Wl,%s -o %s %s %s\n",
	          GetLinkerMacro ( module ).c_str (),
	          gccOptions.c_str (),
	          temp_exp.c_str (),
	          target.c_str (),
	          objectsMacro.c_str (),
	          importLibraryDependencies.c_str () );
	fprintf ( fMakefile,
	          "\t${rm} %s\n",
	          temp_exp.c_str () );
	fprintf ( fMakefile,
		      "\t${rsym} %s %s\n\n",
		      target.c_str (),
		      target.c_str () );
}


static MingwStaticLibraryModuleHandler staticlibrary_handler;

MingwStaticLibraryModuleHandler::MingwStaticLibraryModuleHandler ()
	: MingwModuleHandler ( StaticLibrary )
{
}

void
MingwStaticLibraryModuleHandler::Process ( const Module& module )
{
	GeneratePreconditionDependencies ( module );
	GenerateStaticLibraryModuleTarget ( module );
	GenerateInvocations ( module );
}

void
MingwStaticLibraryModuleHandler::GenerateStaticLibraryModuleTarget ( const Module& module )
{
	GenerateMacrosAndTargetsTarget ( module );
}


static MingwObjectLibraryModuleHandler objectlibrary_handler;

MingwObjectLibraryModuleHandler::MingwObjectLibraryModuleHandler ()
	: MingwModuleHandler ( ObjectLibrary )
{
}

void
MingwObjectLibraryModuleHandler::Process ( const Module& module )
{
	GeneratePreconditionDependencies ( module );
	GenerateObjectLibraryModuleTarget ( module );
	GenerateInvocations ( module );
}

void
MingwObjectLibraryModuleHandler::GenerateObjectLibraryModuleTarget ( const Module& module )
{
	GenerateMacrosAndTargetsTarget ( module );
}


static MingwKernelModeDLLModuleHandler kernelmodedll_handler;

MingwKernelModeDLLModuleHandler::MingwKernelModeDLLModuleHandler ()
	: MingwModuleHandler ( KernelModeDLL )
{
}

void
MingwKernelModeDLLModuleHandler::Process ( const Module& module )
{
	GeneratePreconditionDependencies ( module );
	GenerateKernelModeDLLModuleTarget ( module );
	GenerateInvocations ( module );
}

void
MingwKernelModeDLLModuleHandler::GenerateKernelModeDLLModuleTarget ( const Module& module )
{
	static string ros_junk ( "$(ROS_TEMPORARY)" );
	string target ( FixupTargetFilename ( module.GetPath () ) );
	string workingDirectory = GetWorkingDirectory ( );
	string objectsMacro = GetObjectsMacro ( module );
	string importLibraryDependencies = GetImportLibraryDependencies ( module );

	GenerateImportLibraryTargetIfNeeded ( module );

	if ( module.files.size () > 0 )
	{
		GenerateMacrosAndTargetsTarget ( module );

		fprintf ( fMakefile, "%s: %s %s\n",
		          target.c_str (),
		          objectsMacro.c_str (),
		          importLibraryDependencies.c_str () );

		string linkerParameters = ssprintf ( "-Wl,--subsystem,native -Wl,--entry,%s -Wl,--image-base,%s -Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000 -nostartfiles -mdll",
		                                     module.entrypoint.c_str (),
		                                     module.baseaddress.c_str () );
		GenerateLinkerCommand ( module,
		                        "${gcc}",
		                        linkerParameters,
		                        objectsMacro );
	}
	else
	{
		fprintf ( fMakefile, ".PHONY: %s\n\n",
		          target.c_str ());
		fprintf ( fMakefile, "%s:\n",
		          target.c_str ());
	}
}


static MingwKernelModeDriverModuleHandler kernelmodedriver_handler;

MingwKernelModeDriverModuleHandler::MingwKernelModeDriverModuleHandler ()
	: MingwModuleHandler ( KernelModeDriver )
{
}

void
MingwKernelModeDriverModuleHandler::Process ( const Module& module )
{
	GeneratePreconditionDependencies ( module );
	GenerateKernelModeDriverModuleTarget ( module );
	GenerateInvocations ( module );
}


void
MingwKernelModeDriverModuleHandler::GenerateKernelModeDriverModuleTarget ( const Module& module )
{
	static string ros_junk ( "$(ROS_TEMPORARY)" );
	string target ( PassThruCacheDirectory( FixupTargetFilename ( module.GetPath () ) ) );
	string workingDirectory = GetWorkingDirectory ();
	string objectsMacro = GetObjectsMacro ( module );
	string importLibraryDependencies = GetImportLibraryDependencies ( module );

	GenerateImportLibraryTargetIfNeeded ( module );

	if ( module.files.size () > 0 )
	{
		string* cflags = new string ( "-D__NTDRIVER__" );
		GenerateMacrosAndTargetsTarget ( module,
		                                 cflags,
		                                 NULL );
		delete cflags;

		fprintf ( fMakefile, "%s: %s %s\n",
		          target.c_str (),
		          objectsMacro.c_str (),
		          importLibraryDependencies.c_str () );

		string linkerParameters = ssprintf ( "-Wl,--subsystem,native -Wl,--entry,%s -Wl,--image-base,%s -Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000 -nostartfiles -mdll",
		                                     module.entrypoint.c_str (),
		                                     module.baseaddress.c_str () );
		GenerateLinkerCommand ( module,
		                        "${gcc}",
		                        linkerParameters,
		                        objectsMacro );
	}
	else
	{
		fprintf ( fMakefile, ".PHONY: %s\n\n",
		          target.c_str ());
		fprintf ( fMakefile, "%s:\n",
		          target.c_str () );
	}
}


static MingwNativeDLLModuleHandler nativedll_handler;

MingwNativeDLLModuleHandler::MingwNativeDLLModuleHandler ()
	: MingwModuleHandler ( NativeDLL )
{
}

void
MingwNativeDLLModuleHandler::Process ( const Module& module )
{
	GeneratePreconditionDependencies ( module );
	GenerateNativeDLLModuleTarget ( module );
	GenerateInvocations ( module );
}

void
MingwNativeDLLModuleHandler::GenerateNativeDLLModuleTarget ( const Module& module )
{
	static string ros_junk ( "$(ROS_TEMPORARY)" );
	string target ( FixupTargetFilename ( module.GetPath () ) );
	string workingDirectory = GetWorkingDirectory ( );
	string objectsMacro = GetObjectsMacro ( module );
	string importLibraryDependencies = GetImportLibraryDependencies ( module );
	
	GenerateImportLibraryTargetIfNeeded ( module );

	if ( module.files.size () > 0 )
	{
		GenerateMacrosAndTargetsTarget ( module );

		fprintf ( fMakefile, "%s: %s %s\n",
		          target.c_str (),
		          objectsMacro.c_str (),
		          importLibraryDependencies.c_str () );

		string linkerParameters = ssprintf ( "-Wl,--subsystem,native -Wl,--entry,%s -Wl,--image-base,%s -Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000 -nostartfiles -nostdlib -mdll",
		                                     module.entrypoint.c_str (),
		                                     module.baseaddress.c_str () );
		GenerateLinkerCommand ( module,
		                        "${gcc}",
		                        linkerParameters,
		                        objectsMacro );
	}
	else
	{
		fprintf ( fMakefile, ".PHONY: %s\n\n",
		          target.c_str ());
		fprintf ( fMakefile, "%s:\n\n",
		          target.c_str ());
	}
}


static MingwNativeCUIModuleHandler nativecui_handler;

MingwNativeCUIModuleHandler::MingwNativeCUIModuleHandler ()
	: MingwModuleHandler ( NativeCUI )
{
}

void
MingwNativeCUIModuleHandler::Process ( const Module& module )
{
	GeneratePreconditionDependencies ( module );
	GenerateNativeCUIModuleTarget ( module );
	GenerateInvocations ( module );
}

void
MingwNativeCUIModuleHandler::GenerateNativeCUIModuleTarget ( const Module& module )
{
	static string ros_junk ( "$(ROS_TEMPORARY)" );
	string target ( FixupTargetFilename ( module.GetPath () ) );
	string workingDirectory = GetWorkingDirectory ( );
	string objectsMacro = GetObjectsMacro ( module );
	string importLibraryDependencies = GetImportLibraryDependencies ( module );
	
	GenerateImportLibraryTargetIfNeeded ( module );

	if ( module.files.size () > 0 )
	{
		string* cflags = new string ( "-D__NTAPP__" );
		GenerateMacrosAndTargetsTarget ( module,
		                                 cflags,
		                                 NULL );
		delete cflags;

		fprintf ( fMakefile, "%s: %s %s\n",
		          target.c_str (),
		          objectsMacro.c_str (),
		          importLibraryDependencies.c_str () );

		string linkerParameters = ssprintf ( "-Wl,--subsystem,native -Wl,--entry,%s -Wl,--image-base,%s -Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000 -nostartfiles -nostdlib",
		                                     module.entrypoint.c_str (),
		                                     module.baseaddress.c_str () );
		GenerateLinkerCommand ( module,
		                        "${gcc}",
		                        linkerParameters,
		                        objectsMacro );
	}
	else
	{
		fprintf ( fMakefile, ".PHONY: %s\n\n",
		          target.c_str ());
		fprintf ( fMakefile, "%s:\n\n",
		          target.c_str ());
	}
}


static MingwWin32DLLModuleHandler win32dll_handler;

MingwWin32DLLModuleHandler::MingwWin32DLLModuleHandler ()
	: MingwModuleHandler ( Win32DLL )
{
}

void
MingwWin32DLLModuleHandler::Process ( const Module& module )
{
	GenerateExtractWineDLLResourcesTarget ( module );
	GeneratePreconditionDependencies ( module );
	GenerateWin32DLLModuleTarget ( module );
	GenerateInvocations ( module );
}

void
MingwWin32DLLModuleHandler::GenerateExtractWineDLLResourcesTarget ( const Module& module )
{
	fprintf ( fMakefile, ".PHONY: %s_extractresources\n\n",
	          module.name.c_str () );
	fprintf ( fMakefile, "%s_extractresources: bin2res\n",
	          module.name.c_str () );
	for ( size_t i = 0; i < module.files.size (); i++ )
	{
		File& file = *module.files[i];
		string extension = GetExtension ( file.name );
		if ( extension == ".rc" || extension == ".RC" )
		{
			string resource = FixupTargetFilename ( file.name );
			fprintf ( fMakefile, "\t@echo ${bin2res} -f -x %s\n",
			          resource.c_str () );
		}
	}
	fprintf ( fMakefile, "\n");
}

void
MingwWin32DLLModuleHandler::GenerateWin32DLLModuleTarget ( const Module& module )
{
	static string ros_junk ( "$(ROS_TEMPORARY)" );
	string target ( FixupTargetFilename ( module.GetPath () ) );
	string workingDirectory = GetWorkingDirectory ( );
	string objectsMacro = GetObjectsMacro ( module );
	string linkingDependencies = GetLinkingDependencies ( module );

	GenerateImportLibraryTargetIfNeeded ( module );
	if ( module.files.size () > 0 )
	{
		GenerateMacrosAndTargetsTarget ( module );
	
		fprintf ( fMakefile, "%s: %s %s\n",
		          target.c_str (),
		          objectsMacro.c_str (),
		          linkingDependencies.c_str () );

		string linker;
		if ( IsCPlusPlusModule ( module ) )
			linker = "${gpp}";
		else
			linker = "${gcc}";

		string linkerParameters = ssprintf ( "-Wl,--subsystem,console -Wl,--entry,%s -Wl,--image-base,%s -Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000 -mdll",
		                                     module.entrypoint.c_str (),
		                                     module.baseaddress.c_str () );
		GenerateLinkerCommand ( module,
		                        linker,
		                        linkerParameters,
		                        objectsMacro );
	}
	else
	{
		fprintf ( fMakefile, ".PHONY: %s\n\n",
		          target.c_str () );
		fprintf ( fMakefile, "%s:\n\n",
		          target.c_str () );
	}
}


static MingwWin32CUIModuleHandler win32cui_handler;

MingwWin32CUIModuleHandler::MingwWin32CUIModuleHandler ()
	: MingwModuleHandler ( Win32CUI )
{
}

void
MingwWin32CUIModuleHandler::Process ( const Module& module )
{
	GeneratePreconditionDependencies ( module );
	GenerateWin32CUIModuleTarget ( module );
	GenerateInvocations ( module );
}

void
MingwWin32CUIModuleHandler::GenerateWin32CUIModuleTarget ( const Module& module )
{
	static string ros_junk ( "$(ROS_TEMPORARY)" );
	string target ( FixupTargetFilename ( module.GetPath () ) );
	string workingDirectory = GetWorkingDirectory ( );
	string objectsMacro = GetObjectsMacro ( module );
	string importLibraryDependencies = GetImportLibraryDependencies ( module );

	GenerateImportLibraryTargetIfNeeded ( module );

	if ( module.files.size () > 0 )
	{
		GenerateMacrosAndTargetsTarget ( module );

		fprintf ( fMakefile, "%s: %s %s\n",
		          target.c_str (),
		          objectsMacro.c_str (),
		          importLibraryDependencies.c_str () );

		string linker;
		if ( IsCPlusPlusModule ( module ) )
			linker = "${gpp}";
		else
			linker = "${gcc}";

		string linkerParameters = ssprintf ( "-Wl,--subsystem,console -Wl,--entry,%s -Wl,--image-base,%s -Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000",
		                                     module.entrypoint.c_str (),
		                                     module.baseaddress.c_str () );
		GenerateLinkerCommand ( module,
		                        linker,
		                        linkerParameters,
		                        objectsMacro );
	}
	else
	{
		fprintf ( fMakefile, ".PHONY: %s\n\n",
		          target.c_str ());
		fprintf ( fMakefile, "%s:\n\n",
		          target.c_str ());
	}
}


static MingwWin32GUIModuleHandler win32gui_handler;

MingwWin32GUIModuleHandler::MingwWin32GUIModuleHandler ()
	: MingwModuleHandler ( Win32GUI )
{
}

void
MingwWin32GUIModuleHandler::Process ( const Module& module )
{
	GeneratePreconditionDependencies ( module );
	GenerateWin32GUIModuleTarget ( module );
	GenerateInvocations ( module );
}

void
MingwWin32GUIModuleHandler::GenerateWin32GUIModuleTarget ( const Module& module )
{
	static string ros_junk ( "$(ROS_TEMPORARY)" );
	string target ( FixupTargetFilename ( module.GetPath () ) );
	string workingDirectory = GetWorkingDirectory ( );
	string objectsMacro = GetObjectsMacro ( module );
	string importLibraryDependencies = GetImportLibraryDependencies ( module );

	GenerateImportLibraryTargetIfNeeded ( module );

	if ( module.files.size () > 0 )
	{
		GenerateMacrosAndTargetsTarget ( module );

		fprintf ( fMakefile, "%s: %s %s\n",
		          target.c_str (),
		          objectsMacro.c_str (),
		          importLibraryDependencies.c_str () );

		string linker;
		if ( IsCPlusPlusModule ( module ) )
			linker = "${gpp}";
		else
			linker = "${gcc}";

		string linkerParameters = ssprintf ( "-Wl,--subsystem,windows -Wl,--entry,%s -Wl,--image-base,%s -Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000",
		                                     module.entrypoint.c_str (),
		                                     module.baseaddress.c_str () );
		GenerateLinkerCommand ( module,
		                        linker,
		                        linkerParameters,
		                        objectsMacro );
	}
	else
	{
		fprintf ( fMakefile, ".PHONY: %s\n\n",
		          target.c_str ());
		fprintf ( fMakefile, "%s:\n\n",
		          target.c_str ());
	}
}


static MingwBootLoaderModuleHandler bootloadermodule_handler;

MingwBootLoaderModuleHandler::MingwBootLoaderModuleHandler ()
	: MingwModuleHandler ( BootLoader )
{
}

void
MingwBootLoaderModuleHandler::Process ( const Module& module )
{
	GeneratePreconditionDependencies ( module );
	GenerateBootLoaderModuleTarget ( module );
	GenerateInvocations ( module );
}

void
MingwBootLoaderModuleHandler::GenerateBootLoaderModuleTarget ( const Module& module )
{
	static string ros_junk ( "$(ROS_TEMPORARY)" );
	string targetName ( module.GetTargetName () );
	string target ( FixupTargetFilename ( module.GetPath () ) );
	string workingDirectory = GetWorkingDirectory ();
	string junk_tmp = ros_junk + module.name + ".junk.tmp";
	string objectsMacro = GetObjectsMacro ( module );
	string importLibraryDependencies = GetImportLibraryDependencies ( module );

	GenerateMacrosAndTargetsTarget ( module );

	fprintf ( fMakefile, "%s: %s %s\n",
	          target.c_str (),
	          objectsMacro.c_str (),
	          importLibraryDependencies.c_str () );

	fprintf ( fMakefile,
	          "\t${ld} %s -N -Ttext=0x8000 -o %s %s %s\n",
	          GetLinkerMacro ( module ).c_str (),
	          junk_tmp.c_str (),
	          objectsMacro.c_str (),
	          importLibraryDependencies.c_str () );
	fprintf ( fMakefile,
	          "\t${objcopy} -O binary %s %s\n",
	          junk_tmp.c_str (),
	          target.c_str () );
	fprintf ( fMakefile,
	          "\t${rm} %s\n",
	          junk_tmp.c_str () );
}


static MingwBootSectorModuleHandler bootsectormodule_handler;

MingwBootSectorModuleHandler::MingwBootSectorModuleHandler ()
	: MingwModuleHandler ( BootSector )
{
}

void
MingwBootSectorModuleHandler::Process ( const Module& module )
{
	GeneratePreconditionDependencies ( module );
	GenerateBootSectorModuleTarget ( module );
	GenerateInvocations ( module );
}

void
MingwBootSectorModuleHandler::GenerateBootSectorModuleTarget ( const Module& module )
{
	string objectsMacro = GetObjectsMacro ( module );

	string* nasmflags = new string ( "-f bin" );
	GenerateMacrosAndTargetsTarget ( module,
	                                 NULL,
	                                 nasmflags);

	fprintf ( fMakefile, ".PHONY: %s\n\n",
		      module.name.c_str ());
	fprintf ( fMakefile,
	          "%s: %s\n",
	          module.name.c_str (),
	          objectsMacro.c_str () );
}


static MingwIsoModuleHandler isomodule_handler;

MingwIsoModuleHandler::MingwIsoModuleHandler ()
	: MingwModuleHandler ( Iso )
{
}

void
MingwIsoModuleHandler::Process ( const Module& module )
{
	GeneratePreconditionDependencies ( module );
	GenerateIsoModuleTarget ( module );
	GenerateInvocations ( module );
}

void
MingwIsoModuleHandler::OutputBootstrapfileCopyCommands ( const string bootcdDirectory,
	                                                     const Module& module ) const
{
	for ( size_t i = 0; i < module.project.modules.size (); i++ )
	{
		const Module& m = *module.project.modules[i];
		if ( m.bootstrap != NULL )
		{
			string targetFilenameNoFixup = bootcdDirectory + SSEP + m.bootstrap->base + SSEP + m.bootstrap->nameoncd;
			string targetFilename = PassThruCacheDirectory ( FixupTargetFilename ( targetFilenameNoFixup ) );
			fprintf ( fMakefile,
			          "\t${cp} %s %s\n",
			          m.GetPath ().c_str (),
			          targetFilename.c_str () );
		}
	}
}

void
MingwIsoModuleHandler::OutputCdfileCopyCommands ( const string bootcdDirectory,
	                                              const Module& module ) const
{
	for ( size_t i = 0; i < module.project.cdfiles.size (); i++ )
	{
		const CDFile& cdfile = *module.project.cdfiles[i];
		string targetFilenameNoFixup = bootcdDirectory + SSEP + cdfile.base + SSEP + cdfile.nameoncd;
		string targetFilename = PassThruCacheDirectory ( FixupTargetFilename ( targetFilenameNoFixup ) );
		fprintf ( fMakefile,
		          "\t${cp} %s %s\n",
		          cdfile.GetPath ().c_str (),
		          targetFilename.c_str () );
	}
}

string
MingwIsoModuleHandler::GetBootstrapCdDirectories ( const string bootcdDirectory,
	                                               const Module& module ) const
{
	string directories;
	for ( size_t i = 0; i < module.project.modules.size (); i++ )
	{
		const Module& m = *module.project.modules[i];
		if ( m.bootstrap != NULL )
		{
			string targetDirecctory = bootcdDirectory + SSEP + m.bootstrap->base;
			if ( directories.size () > 0 )
				directories += " ";
			directories += FixupTargetFilename ( targetDirecctory );
		}
	}
	return directories;
}

string
MingwIsoModuleHandler::GetNonModuleCdDirectories ( const string bootcdDirectory,
	                                               const Module& module ) const
{
	string directories;
	for ( size_t i = 0; i < module.project.cdfiles.size (); i++ )
	{
		const CDFile& cdfile = *module.project.cdfiles[i];
		string targetDirecctory = bootcdDirectory + SSEP + cdfile.base;
		if ( directories.size () > 0 )
			directories += " ";
		directories += FixupTargetFilename ( targetDirecctory );
	}
	return directories;
}

string
MingwIsoModuleHandler::GetCdDirectories ( const string bootcdDirectory,
	                                      const Module& module ) const
{
	string directories = GetBootstrapCdDirectories ( bootcdDirectory,
	                                                 module );
	directories += " " + GetNonModuleCdDirectories ( bootcdDirectory,
	                                                 module );
	return directories;
}

string
MingwIsoModuleHandler::GetBootstrapCdFiles ( const string bootcdDirectory,
	                                         const Module& module ) const
{
	string cdfiles;
	for ( size_t i = 0; i < module.project.modules.size (); i++ )
	{
		const Module& m = *module.project.modules[i];
		if ( m.bootstrap != NULL )
		{
			if ( cdfiles.size () > 0 )
				cdfiles += " ";
			cdfiles += FixupTargetFilename ( m.GetPath () );
		}
	}
	return cdfiles;
}

string
MingwIsoModuleHandler::GetNonModuleCdFiles ( const string bootcdDirectory,
	                                         const Module& module ) const
{
	string cdfiles;
	for ( size_t i = 0; i < module.project.cdfiles.size (); i++ )
	{
		const CDFile& cdfile = *module.project.cdfiles[i];
		if ( cdfiles.size () > 0 )
			cdfiles += " ";
		cdfiles += NormalizeFilename ( cdfile.GetPath () );
	}
	return cdfiles;
}

string
MingwIsoModuleHandler::GetCdFiles ( const string bootcdDirectory,
	                                const Module& module ) const
{
	string cdfiles = GetBootstrapCdFiles ( bootcdDirectory,
	                                       module );
	cdfiles += " " + GetNonModuleCdFiles ( bootcdDirectory,
	                                       module );
	return cdfiles;
}

void
MingwIsoModuleHandler::GenerateIsoModuleTarget ( const Module& module )
{
	string bootcdDirectory = "cd";
	string isoboot = FixupTargetFilename ( "boot/freeldr/bootsect/isoboot.o" );
	string bootcdReactosNoFixup = bootcdDirectory + "/reactos";
	string bootcdReactos = FixupTargetFilename ( bootcdReactosNoFixup );
	PassThruCacheDirectory ( bootcdReactos + SSEP );
	string reactosInf = FixupTargetFilename ( bootcdReactosNoFixup + "/reactos.inf" );
	string reactosDff = NormalizeFilename ( "bootdata/packages/reactos.dff" );
	string cdDirectories = bootcdReactos + " " + GetCdDirectories ( bootcdDirectory,
	                                                                module );
	string cdFiles = GetCdFiles ( bootcdDirectory,
	                              module );

	fprintf ( fMakefile, ".PHONY: %s\n\n",
		      module.name.c_str ());
	fprintf ( fMakefile,
	          "%s: all %s %s %s\n",
	          module.name.c_str (),
	          isoboot.c_str (),
	          cdDirectories.c_str (),
	          cdFiles.c_str () );
	fprintf ( fMakefile,
	          "\t${cabman} -C %s -L %s -I\n",
	          reactosDff.c_str (),
	          bootcdReactos.c_str () );
	fprintf ( fMakefile,
	          "\t${cabman} -C %s -RC %s -L %s -N\n",
	          reactosDff.c_str (),
	          reactosInf.c_str (),
	          bootcdReactos.c_str () );
	fprintf ( fMakefile,
	          "\t- ${rm} %s\n",
	          reactosInf.c_str () );
	OutputBootstrapfileCopyCommands ( bootcdDirectory,
	                                  module );
	OutputCdfileCopyCommands ( bootcdDirectory,
	                           module );
	fprintf ( fMakefile,
	          "\t${cdmake} -v -m -b %s %s REACTOS ReactOS.iso\n",
	          isoboot.c_str (),
	          bootcdDirectory.c_str () );
	fprintf ( fMakefile,
	          "\n" );
}
