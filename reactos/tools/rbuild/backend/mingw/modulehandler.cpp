
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

FILE*
MingwModuleHandler::fMakefile = NULL;

MingwModuleHandler::MingwModuleHandler ( ModuleType moduletype )
{
	if ( !handler_map )
		handler_map = new map<ModuleType,MingwModuleHandler*>;
	(*handler_map)[moduletype] = this;
}

/*static*/ void
MingwModuleHandler::SetMakefile ( FILE* f )
{
	fMakefile = f;
}

/*static*/ MingwModuleHandler*
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
	return ReplaceExtension ( FixupTargetFilename ( module.GetPath () ).c_str (),
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
		dependencies += FixupTargetFilename ( importedModule->GetPath () ).c_str ();
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
	if ( module.files.size () == 0 )
		return "";
	
	string sourceFilenames ( "" );
	for ( size_t i = 0; i < module.files.size (); i++ )
	{
		if ( sourceFilenames.size () > 0 )
			sourceFilenames += " ";
		sourceFilenames += module.files[i]->name;
	}
	return sourceFilenames;
}

string
MingwModuleHandler::GetObjectFilename ( const string& sourceFilename ) const
{
	return FixupTargetFilename ( ReplaceExtension ( sourceFilename,
	                                                ".o" ) );
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

void
MingwModuleHandler::GenerateGccModuleIncludeVariable ( const Module& module ) const
{
	string name ( module.name + "_INCLUDES" );
	fprintf ( fMakefile,
	          "%s := %s\n",
	          name.c_str(),
	          GenerateGccIncludeParameters(module).c_str() );
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
MingwModuleHandler::GenerateGccParameters ( const Module& module ) const
{
	string parameters = GenerateGccDefineParameters ( module );
	parameters += ssprintf(" $(%s_INCLUDES)",module.name.c_str());
	return parameters;
}

void
MingwModuleHandler::GenerateObjectFileTargets ( const Module& module,
	                                            const string& cc) const
{
	if ( module.files.size () == 0 )
		return;
	
	GenerateGccModuleIncludeVariable ( module );

	for ( size_t i = 0; i < module.files.size (); i++ )
	{
		string sourceFilename = module.files[i]->name;
		string objectFilename = GetObjectFilename ( sourceFilename );
		fprintf ( fMakefile,
		          "%s: %s\n",
		          objectFilename.c_str (),
		          sourceFilename.c_str () );
		fprintf ( fMakefile,
		          "\t%s -c %s -o %s %s\n",
		          cc.c_str (),
		          sourceFilename.c_str (),
		          objectFilename.c_str (),
		          GenerateGccParameters ( module ).c_str () );
	}
	
	fprintf ( fMakefile, "\n" );
}

void
MingwModuleHandler::GenerateObjectFileTargetsHost ( const Module& module ) const
{
	GenerateObjectFileTargets ( module,
	                            "${host_gcc}" );
}

void
MingwModuleHandler::GenerateObjectFileTargetsTarget ( const Module& module ) const
{
	GenerateObjectFileTargets ( module,
	                            "${gcc}" );
}

void
MingwModuleHandler::GenerateArchiveTarget ( const Module& module,
	                                        const string& ar ) const
{
	string archiveFilename = GetModuleArchiveFilename ( module );
	string sourceFilenames = GetSourceFilenames ( module );
	string objectFilenames = GetObjectFilenames ( module );
	
	fprintf ( fMakefile,
	          "%s: %s\n",
	          archiveFilename.c_str (),
	          objectFilenames.c_str ());

	fprintf ( fMakefile,
	          "\t%s -rc %s %s\n\n",
	          ar.c_str (),
	          archiveFilename.c_str (),
	          objectFilenames.c_str ());
}

void
MingwModuleHandler::GenerateArchiveTargetHost ( const Module& module ) const
{
	GenerateArchiveTarget ( module,
	                        "${host_ar}" );
}

void
MingwModuleHandler::GenerateArchiveTargetTarget ( const Module& module ) const
{
	GenerateArchiveTarget ( module,
	                        "${ar}" );
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
	          "%s: %s\n\n",
	          preconditionDependenciesName.c_str (),
	          dependencies.c_str () );
	fprintf ( fMakefile,
	          "%s: %s\n\n",
	          sourceFilenames.c_str (),
	          preconditionDependenciesName.c_str ());
	fprintf ( fMakefile,
	          ".PHONY: %s\n\n",
	          preconditionDependenciesName.c_str () );
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
	fprintf ( fMakefile, "%s: %s\n",
	          target.c_str (),
	          archiveFilename.c_str () );
	fprintf ( fMakefile,
	          "\t${host_gcc} -o %s %s\n",
	          target.c_str (),
	          archiveFilename.c_str () );
	GenerateArchiveTargetHost ( module );
	GenerateObjectFileTargetsHost ( module );
}

static MingwKernelModuleHandler kernelmodule_handler;

MingwKernelModuleHandler::MingwKernelModuleHandler ()
	: MingwModuleHandler ( KernelModeDLL )
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
	//static string ros_output ( "$(ROS_INTERMEDIATE)" );
	string target ( FixupTargetFilename(module.GetPath()) );
	string workingDirectory = GetWorkingDirectory ( );
	string archiveFilename = GetModuleArchiveFilename ( module );
	string importLibraryDependencies = GetImportLibraryDependencies ( module );
	string base_tmp = ros_junk + module.name + ".base.tmp";
	string junk_tmp = ros_junk + module.name + ".junk.tmp";
	string temp_exp = ros_junk + module.name + ".temp.exp";
	fprintf ( fMakefile, "%s: %s %s\n",
	          target.c_str (),
	          archiveFilename.c_str (),
	          importLibraryDependencies.c_str () );
	fprintf ( fMakefile,
	          "\t${gcc} -Wl,--base-file,%s -o %s %s %s\n",
	          base_tmp.c_str (),
	          junk_tmp.c_str (),
	          archiveFilename.c_str (),
	          importLibraryDependencies.c_str () );
	fprintf ( fMakefile,
	          "\t${rm} %s\n",
	          junk_tmp.c_str () );
	fprintf ( fMakefile,
	          "\t${dlltool} --dllname %s --base-file %s --output-exp %s --kill-at\n",
	          target.c_str (),
	          base_tmp.c_str (),
	          temp_exp.c_str ());
	fprintf ( fMakefile,
	          "\t${rm} %s\n",
	          base_tmp.c_str () );
	fprintf ( fMakefile,
	          "\t${ld} -Wl,%s -o %s %s %s\n",
	          temp_exp.c_str (),
	          target.c_str (),
	          archiveFilename.c_str (),
	          importLibraryDependencies.c_str () );
	fprintf ( fMakefile,
	          "\t${rm} %s\n",
	          temp_exp.c_str () );
	
	GenerateArchiveTargetTarget ( module );
	GenerateObjectFileTargetsTarget ( module );
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
	GenerateArchiveTargetTarget ( module );
	GenerateObjectFileTargetsTarget ( module );
}
