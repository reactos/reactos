
#include "../../pch.h"
#include <assert.h>

#include "../../rbuild.h"
#include "mingw.h"
#include "modulehandler.h"

using std::string;
using std::vector;
using std::map;

map<ModuleType,MingwModuleHandler*>*
MingwModuleHandler::handler_map = NULL;
int
MingwModuleHandler::ref = 0;

FILE*
MingwModuleHandler::fMakefile = NULL;

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
MingwModuleHandler::GetExtension ( const string& filename ) const
{
	size_t index = filename.find_last_of ( '.' );
	if (index != string::npos)
		return filename.substr ( index );
	return "";
}

string
MingwModuleHandler::ReplaceExtension ( const string& filename,
	                                   const string& newExtension ) const
{
	size_t index = filename.find_last_of ( '.' );
	if (index != string::npos)
		return filename.substr ( 0, index ) + newExtension;
	return filename;
}

string
MingwModuleHandler::GetModuleArchiveFilename ( const Module& module ) const
{
	return ReplaceExtension ( FixupTargetFilename ( module.GetPath () ),
	                          ".a" );
}

string
MingwModuleHandler::GetImportLibraryDependencies ( const Module& module ) const
{
	if ( module.libraries.size () == 0 )
		return "";
	
	string dependencies ( "" );
	for ( size_t i = 0; i < module.libraries.size (); i++ )
	{
		if ( dependencies.size () > 0 )
			dependencies += " ";
		const Module* importedModule = module.project.LocateModule ( module.libraries[i]->name );
		assert ( importedModule != NULL );
		dependencies += FixupTargetFilename ( importedModule->GetDependencyPath () ).c_str ();
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
	return dependencies;
}

string
MingwModuleHandler::GetAllDependencies ( const Module& module ) const
{
	string dependencies = GetImportLibraryDependencies ( module );
	string s = GetModuleDependencies ( module );
	if (s.length () > 0)
	{
		dependencies += " ";
		dependencies += s;
	}
	return dependencies;
}

string
MingwModuleHandler::GetSourceFilenames ( const Module& module ) const
{
	size_t i;

	string sourceFilenames ( "" );
	for ( i = 0; i < module.files.size (); i++ )
		sourceFilenames += " " + module.files[i]->name;
	vector<If*> ifs = module.ifs;
	for ( i = 0; i < ifs.size(); i++ )
	{
		size_t j;
		If& rIf = *ifs[i];
		for ( j = 0; j < rIf.ifs.size(); j++ )
			ifs.push_back ( rIf.ifs[j] );
		for ( j = 0; j < rIf.files.size(); j++ )
			sourceFilenames += " " + rIf.files[j]->name;
	}
	return sourceFilenames;
}

string
MingwModuleHandler::GetObjectFilename ( const string& sourceFilename ) const
{
	string newExtension;
	string extension = GetExtension ( sourceFilename );
	if ( extension == ".rc" || extension == ".RC" )
		newExtension = ".coff";
	else
		newExtension = ".o";
	return FixupTargetFilename ( ReplaceExtension ( sourceFilename,
		                                            newExtension ) );
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
		objectFilenames += GetObjectFilename ( module.files[i]->name );
	}
	return objectFilenames;
}

string
MingwModuleHandler::GenerateGccDefineParametersFromVector ( const vector<Define*>& defines ) const
{
	string parameters;
	for (size_t i = 0; i < defines.size (); i++)
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
	if (s.length () > 0)
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
		if (parameters.length () > 0)
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
                                    const vector<Define*>& defines ) const
{
	size_t i;
	
	fprintf (
		fMakefile,
		"%s %s",
		macro.c_str(),
		assignmentOperation );
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
	const vector<LinkerFlag*>* linkerFlags,
	const vector<If*>& ifs,
	const string& cflags_macro,
	const string& nasmflags_macro,
	const string& windresflags_macro,
	const string& linkerflags_macro,
	const string& objs_macro) const
{
	size_t i;

	if ( includes.size() || defines.size() )
	{
		GenerateMacro ( assignmentOperation,
		                cflags_macro,
		                includes,
		                defines );
		GenerateMacro ( assignmentOperation,
		                windresflags_macro,
		                includes,
		                defines );
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
	
	if ( files.size() )
	{
		fprintf (
			fMakefile,
			"%s %s",
			objs_macro.c_str(),
			assignmentOperation );
		for ( i = 0; i < files.size(); i++ )
		{
			fprintf (
				fMakefile,
				"%s%s",
				( i%10 == 9 ? "\\\n\t" : " " ),
				GetObjectFilename(files[i]->name).c_str() );
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

string
MingwModuleHandler::GenerateGccCommand ( const Module& module,
                                         const string& sourceFilename,
                                         const string& cc,
                                         const string& cflagsMacro ) const
{
	string objectFilename = GetObjectFilename ( sourceFilename );
	return ssprintf ( "%s -c %s -o %s %s\n",
		              cc.c_str (),
		              sourceFilename.c_str (),
		              objectFilename.c_str (),
		              cflagsMacro.c_str () );
}

string
MingwModuleHandler::GenerateGccAssemblerCommand ( const Module& module,
                                                  const string& sourceFilename,
                                                  const string& cc,
                                                  const string& cflagsMacro ) const
{
	string objectFilename = GetObjectFilename ( sourceFilename );
	return ssprintf ( "%s -x assembler-with-cpp -c %s -o %s -D__ASM__ %s\n",
	                  cc.c_str (),
	                  sourceFilename.c_str (),
	                  objectFilename.c_str (),
	                  cflagsMacro.c_str () );
}

string
MingwModuleHandler::GenerateNasmCommand ( const Module& module,
                                          const string& sourceFilename,
                                          const string& nasmflagsMacro ) const
{
	string objectFilename = GetObjectFilename ( sourceFilename );
	return ssprintf ( "%s -f win32 %s -o %s %s\n",
		              "nasm",
		              sourceFilename.c_str (),
		              objectFilename.c_str (),
		              nasmflagsMacro.c_str () );
}

string
MingwModuleHandler::GenerateWindresCommand ( const Module& module,
                                             const string& sourceFilename,
                                             const string& windresflagsMacro ) const
{
	string objectFilename = GetObjectFilename ( sourceFilename );
	return ssprintf ( "%s %s -o %s ${%s}\n",
		              "${windres}",
		              sourceFilename.c_str (),
		              objectFilename.c_str (),
		              windresflagsMacro.c_str () );
}

string
MingwModuleHandler::GenerateCommand ( const Module& module,
                                      const string& sourceFilename,
                                      const string& cc,
                                      const string& cflagsMacro,
                                      const string& nasmflagsMacro,
                                      const string& windresflagsMacro ) const
{
	string extension = GetExtension ( sourceFilename );
	if ( extension == ".c" || extension == ".C" )
		return GenerateGccCommand ( module,
		                            sourceFilename,
		                            cc,
		                            cflagsMacro );
	else if ( extension == ".s" || extension == ".S" )
		return GenerateGccAssemblerCommand ( module,
		                                     sourceFilename,
		                                     cc,
		                                     cflagsMacro );
	else if ( extension == ".asm" || extension == ".ASM" )
		return GenerateNasmCommand ( module,
		                             sourceFilename,
		                             nasmflagsMacro );
	else if ( extension == ".rc" || extension == ".RC" )
		return GenerateWindresCommand ( module,
		                                sourceFilename,
		                                windresflagsMacro );

	throw InvalidOperationException ( __FILE__,
	                                  __LINE__,
	                                  "Unsupported filename extension '%s' in file '%s'",
	                                  extension.c_str (),
	                                  sourceFilename.c_str () );
}

string
MingwModuleHandler::GenerateLinkerCommand ( const Module& module,
                                            const string& linker,
                                            const string& linkerParameters,
                                            const string& objectFilenames ) const
{
	string target ( FixupTargetFilename ( module.GetPath () ) );
	string importLibraryDependencies = GetImportLibraryDependencies ( module );
	return ssprintf ( "%s %s -o %s %s %s %s\n",
	                  linker.c_str (),
		              linkerParameters.c_str (),
                      target.c_str (),
                      objectFilenames.c_str (),
                      importLibraryDependencies.c_str (),
                      GetLinkerMacro ( module ).c_str () );
}

void
MingwModuleHandler::GenerateObjectFileTargets ( const Module& module,
                                                const vector<File*>& files,
                                                const vector<If*>& ifs,
                                                const string& cc,
                                                const string& cflagsMacro,
                                                const string& nasmflagsMacro,
                                                const string& windresflagsMacro ) const
{
	size_t i;

	for ( i = 0; i < files.size (); i++ )
	{
		string sourceFilename = files[i]->name;
		string objectFilename = GetObjectFilename ( sourceFilename );
		fprintf ( fMakefile,
		          "%s: %s\n",
		          objectFilename.c_str (),
		          sourceFilename.c_str () );
		fprintf ( fMakefile,
		          "\t%s\n",
		          GenerateCommand ( module,
		                            sourceFilename,
		                            cc,
		                            cflagsMacro,
		                            nasmflagsMacro,
		                            windresflagsMacro ).c_str () );
	}

	for ( i = 0; i < ifs.size(); i++ )
		GenerateObjectFileTargets ( module,
		                            ifs[i]->files,
		                            ifs[i]->ifs,
		                            cc,
		                            cflagsMacro,
		                            nasmflagsMacro,
		                            windresflagsMacro );
}

void
MingwModuleHandler::GenerateObjectFileTargets ( const Module& module,
                                                const string& cc,
                                                const string& cflagsMacro,
                                                const string& nasmflagsMacro,
                                                const string& windresflagsMacro ) const
{
	GenerateObjectFileTargets ( module,
	                            module.files,
	                            module.ifs,
	                            cc,
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
		out.push_back ( GetObjectFilename(files[i]->name) );

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
	const string& ar,
	const string* cflags ) const
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
	
	// generate phony target for module name
	fprintf ( fMakefile, ".PHONY: %s\n",
		module.name.c_str () );
	fprintf ( fMakefile, "%s: %s\n\n",
		module.name.c_str (),
		module.GetPath ().c_str () );

	// future references to the macros will be to get their values
	cflagsMacro = ssprintf ("$(%s)", cflagsMacro.c_str ());
	nasmflagsMacro = ssprintf ("$(%s)", nasmflagsMacro.c_str ());
	objectsMacro = ssprintf ("$(%s)", objectsMacro.c_str ());

	string ar_target = GenerateArchiveTarget ( module, ar, objectsMacro );
	GenerateObjectFileTargets ( module,
	                            cc,
	                            cflagsMacro,
	                            nasmflagsMacro,
	                            windresflagsMacro );

	vector<string> clean_files;
	clean_files.push_back ( FixupTargetFilename(module.GetPath()) );
	clean_files.push_back ( ar_target );
	GetCleanTargets ( clean_files, module.files, module.ifs );

	fprintf ( fMakefile, "clean::\n\t-@$(rm)" );
	for ( size_t i = 0; i < clean_files.size(); i++ )
	{
		if ( 9==(i%10) )
			fprintf ( fMakefile, " 2>NUL\n\t-@$(rm)" );
		fprintf ( fMakefile, " %s", clean_files[i].c_str() );
	}
	fprintf ( fMakefile, " 2>NUL\n\n" );
}

void
MingwModuleHandler::GenerateMacrosAndTargetsHost ( const Module& module ) const
{
	GenerateMacrosAndTargets ( module, "${host_gcc}", "${host_ar}", NULL );
}

void
MingwModuleHandler::GenerateMacrosAndTargetsTarget ( const Module& module ) const
{
	GenerateMacrosAndTargetsTarget ( module,
	                                 NULL );
}

void
MingwModuleHandler::GenerateMacrosAndTargetsTarget ( const Module& module,
	                                                 const string* clags ) const
{
	GenerateMacrosAndTargets ( module, "${gcc}", "${ar}", clags );
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

string
MingwModuleHandler::GetInvocationParameters ( const Invoke& invoke ) const
{
	string parameters ( "" );
	size_t i;
	for (i = 0; i < invoke.output.size (); i++)
	{
		if (parameters.length () > 0)
			parameters += " ";
		InvokeFile& invokeFile = *invoke.output[i];
		if (invokeFile.switches.length () > 0)
		{
			parameters += invokeFile.switches;
			parameters += " ";
		}
		parameters += invokeFile.name;
	}

	for (i = 0; i < invoke.input.size (); i++)
	{
		if (parameters.length () > 0)
			parameters += " ";
		InvokeFile& invokeFile = *invoke.input[i];
		if (invokeFile.switches.length () > 0)
		{
			parameters += invokeFile.switches;
			parameters += " ";
		}
		parameters += invokeFile.name;
	}

	return parameters;
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
			throw InvalidBuildFileException ( module.node.location,
		                                      "Only modules of type buildtool can be invoked." );

		string invokeTarget = module.GetInvocationTarget ( i );
		fprintf ( fMakefile,
		          "%s: %s\n\n",
		          invoke.GetTargets ().c_str (),
		          invokeTarget.c_str () );
		fprintf ( fMakefile,
		          "%s: %s\n",
		          invokeTarget.c_str (),
		          FixupTargetFilename ( invoke.invokeModule->GetPath () ).c_str () );
		fprintf ( fMakefile,
		          "\t%s %s\n\n",
		          FixupTargetFilename ( invoke.invokeModule->GetPath () ).c_str (),
		          GetInvocationParameters ( invoke ).c_str () );
		fprintf ( fMakefile,
		          ".PNONY: %s\n\n",
		          invokeTarget.c_str () );
	}
}

string
MingwModuleHandler::GetPreconditionDependenciesName ( const Module& module ) const
{
	return ssprintf ( "%s_precondition",
	                  module.name.c_str () );
}

void
MingwModuleHandler::GeneratePreconditionDependencies ( const Module& module ) const
{
	string preconditionDependenciesName = GetPreconditionDependenciesName ( module );
	string sourceFilenames = GetSourceFilenames ( module );
	string dependencies = GetModuleDependencies ( module );
	string s = GetInvocationDependencies ( module );
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
		fprintf ( fMakefile, "%s:\n",
		          module.GetDependencyPath ().c_str () );

		fprintf ( fMakefile,
		          "\t${dlltool} --dllname %s --def %s --output-lib %s --kill-at\n\n",
		          module.GetTargetName ().c_str (),
		          FixupTargetFilename ( module.GetBasePath () + SSEP + module.importLibrary->definition ).c_str (),
		          FixupTargetFilename ( module.GetDependencyPath () ).c_str () );
	}
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
	string archiveFilename = GetModuleArchiveFilename ( module );

	GenerateMacrosAndTargetsHost ( module );

	fprintf ( fMakefile, "%s: %s\n",
	          target.c_str (),
	          archiveFilename.c_str () );
	fprintf ( fMakefile,
	          "\t${host_gcc} %s -o %s %s\n\n",
	          GetLinkerMacro ( module ).c_str (),
	          target.c_str (),
	          archiveFilename.c_str () );
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
	string gccOptions = ssprintf ("-Wl,-T,%s" SSEP "ntoskrnl.lnk -Wl,--subsystem,native -Wl,--entry,_NtProcessStartup -Wl,--image-base,0xC0000000 -Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000 -nostartfiles -mdll",
	                              module.GetBasePath ().c_str () );

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
	fprintf ( fMakefile,
	          "\t${dlltool} --dllname %s --base-file %s --def ntoskrnl/ntoskrnl.def --output-exp %s --kill-at\n",
	          targetName.c_str (),
	          base_tmp.c_str (),
	          temp_exp.c_str () );
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
	          "\t${rm} %s\n\n",
	          temp_exp.c_str () );
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
	string archiveFilename = GetModuleArchiveFilename ( module );
	string importLibraryDependencies = GetImportLibraryDependencies ( module );

	GenerateImportLibraryTargetIfNeeded ( module );

	if ( module.files.size () > 0 )
	{
		GenerateMacrosAndTargetsTarget ( module );

		fprintf ( fMakefile, "%s: %s %s\n",
		          target.c_str (),
		          archiveFilename.c_str (),
		          importLibraryDependencies.c_str () );

		string linkerParameters ( "-Wl,--subsystem,native -Wl,--entry,_DriverEntry@8 -Wl,--image-base,0x10000 -Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000 -nostartfiles -mdll" );
		string linkerCommand = GenerateLinkerCommand ( module,
                                                       "${gcc}",
                                                       linkerParameters,
                                                       archiveFilename );
		fprintf ( fMakefile,
		          "\t%s\n\n",
		          linkerCommand.c_str () );
	}
	else
	{
		fprintf ( fMakefile, "%s:\n",
		          target.c_str ());
		fprintf ( fMakefile, ".PHONY: %s\n\n",
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
	string target ( FixupTargetFilename ( module.GetPath () ) );
	string workingDirectory = GetWorkingDirectory ( );
	string archiveFilename = GetModuleArchiveFilename ( module );
	string importLibraryDependencies = GetImportLibraryDependencies ( module );

	GenerateImportLibraryTargetIfNeeded ( module );

	if ( module.files.size () > 0 )
	{
		string* cflags = new string ( "-D__NTDRIVER__" );
		GenerateMacrosAndTargetsTarget ( module,
		                                 cflags );
		delete cflags;

		fprintf ( fMakefile, "%s: %s %s\n",
		          target.c_str (),
		          archiveFilename.c_str (),
		          importLibraryDependencies.c_str () );

		string linkerParameters ( "-Wl,--subsystem,native -Wl,--entry,_DriverEntry@8 -Wl,--image-base,0x10000 -Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000 -nostartfiles -mdll" );
		string linkerCommand = GenerateLinkerCommand ( module,
                                                       "${gcc}",
                                                       linkerParameters,
                                                       archiveFilename );
		fprintf ( fMakefile,
		          "\t%s\n\n",
		          linkerCommand.c_str () );
	}
	else
	{
		fprintf ( fMakefile, "%s:\n",
		          target.c_str ());
		fprintf ( fMakefile, ".PHONY: %s\n\n",
		          target.c_str ());
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
	string archiveFilename = GetModuleArchiveFilename ( module );
	string importLibraryDependencies = GetImportLibraryDependencies ( module );
	
	GenerateImportLibraryTargetIfNeeded ( module );

	if ( module.files.size () > 0 )
	{
		GenerateMacrosAndTargetsTarget ( module );

		fprintf ( fMakefile, "%s: %s %s\n",
		          target.c_str (),
		          archiveFilename.c_str (),
		          importLibraryDependencies.c_str () );

		string linkerParameters ( "-Wl,--subsystem,native -Wl,--entry,_DllMainCRTStartup@12 -Wl,--image-base,0x10000 -Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000 -nostartfiles -nostdlib -mdll" );
		string linkerCommand = GenerateLinkerCommand ( module,
                                                       "${gcc}",
                                                       linkerParameters,
                                                       archiveFilename );
		fprintf ( fMakefile,
		          "\t%s\n\n",
		          linkerCommand.c_str () );
	}
	else
	{
		fprintf ( fMakefile, "%s:\n\n",
		          target.c_str ());
		fprintf ( fMakefile, ".PHONY: %s\n\n",
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
	GeneratePreconditionDependencies ( module );
	GenerateWin32DLLModuleTarget ( module );
	GenerateInvocations ( module );
}

void
MingwWin32DLLModuleHandler::GenerateWin32DLLModuleTarget ( const Module& module )
{
	static string ros_junk ( "$(ROS_TEMPORARY)" );
	string target ( FixupTargetFilename ( module.GetPath () ) );
	string workingDirectory = GetWorkingDirectory ( );
	string archiveFilename = GetModuleArchiveFilename ( module );
	string importLibraryDependencies = GetImportLibraryDependencies ( module );

	GenerateImportLibraryTargetIfNeeded ( module );

	if ( module.files.size () > 0 )
	{
		GenerateMacrosAndTargetsTarget ( module );

		fprintf ( fMakefile, "%s: %s %s\n",
		          target.c_str (),
		          archiveFilename.c_str (),
		          importLibraryDependencies.c_str () );

		string linkerParameters ( "-Wl,--subsystem,console -Wl,--entry,_DllMain@12 -Wl,--image-base,0x10000 -Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000 -mdll" );
		string linkerCommand = GenerateLinkerCommand ( module,
                                                       "${gcc}",
                                                       linkerParameters,
                                                       archiveFilename );
		fprintf ( fMakefile,
		          "\t%s\n\n",
		          linkerCommand.c_str () );
	}
	else
	{
		fprintf ( fMakefile, "%s:\n\n",
		          target.c_str ());
		fprintf ( fMakefile, ".PHONY: %s\n\n",
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
	string objectFilenames = GetObjectFilenames ( module );
	string importLibraryDependencies = GetImportLibraryDependencies ( module );

	GenerateImportLibraryTargetIfNeeded ( module );

	if ( module.files.size () > 0 )
	{
		GenerateMacrosAndTargetsTarget ( module );

		fprintf ( fMakefile, "%s: %s %s\n",
		          target.c_str (),
		          objectFilenames.c_str (),
		          importLibraryDependencies.c_str () );

		string linkerParameters ( "-Wl,--subsystem,windows -Wl,--entry,_WinMainCRTStartup -Wl,--image-base,0x00400000 -Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000" );
		string linkerCommand = GenerateLinkerCommand ( module,
                                                       "${gcc}",
                                                       linkerParameters,
                                                       objectFilenames );
		fprintf ( fMakefile,
		          "\t%s\n\n",
		          linkerCommand.c_str () );
	}
	else
	{
		fprintf ( fMakefile, "%s:\n\n",
		          target.c_str ());
		fprintf ( fMakefile, ".PHONY: %s\n\n",
		          target.c_str ());
	}
}
