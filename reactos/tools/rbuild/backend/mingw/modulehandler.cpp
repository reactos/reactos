
#include "../../pch.h"
#include <assert.h>

#include "../../rbuild.h"
#include "mingw.h"
#include "modulehandler.h"

using std::string;
using std::vector;

MingwModuleHandler::MingwModuleHandler ( FILE* fMakefile )
	: fMakefile ( fMakefile )
{
}

string
MingwModuleHandler::GetWorkingDirectory ()
{
	return ".";
}

string
MingwModuleHandler::ReplaceExtension ( string filename,
	                                   string newExtension )
{
	size_t index = filename.find_last_of ( '.' );
	if (index != string::npos)
		return filename.substr ( 0, index ) + newExtension;
	return filename;
}

string
MingwModuleHandler::GetModuleArchiveFilename ( Module& module )
{
	return ReplaceExtension ( module.GetPath ().c_str (),
	                          ".a" );
}

string
MingwModuleHandler::GetImportLibraryDependencies ( Module& module )
{
	if ( module.libraries.size () == 0 )
		return "";
	
	string dependencies ( "" );
	for ( size_t i = 0; i < module.libraries.size (); i++ )
	{
		if ( dependencies.size () > 0 )
			dependencies += " ";
		Module* importedModule = module.project->LocateModule ( module.libraries[i]->name );
		assert ( importedModule != NULL );
		dependencies += importedModule->GetPath ().c_str ();
	}
	return dependencies;
}

string
MingwModuleHandler::GetSourceFilenames ( Module& module )
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
MingwModuleHandler::GetObjectFilename ( string sourceFilename )
{
	return ReplaceExtension ( sourceFilename,
		                      ".o" );
}

string
MingwModuleHandler::GetObjectFilenames ( Module& module )
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
MingwModuleHandler::GenerateGccDefineParametersFromVector ( vector<Define*> defines )
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
MingwModuleHandler::GenerateGccDefineParameters ( Module& module )
{
	string parameters = GenerateGccDefineParametersFromVector ( module.project->defines );
	string s = GenerateGccDefineParametersFromVector ( module.defines );
	if (s.length () > 0)
	{
		parameters += " ";
		parameters += s;
	}
	return parameters;
}

string
MingwModuleHandler::GenerateGccIncludeParametersFromVector ( vector<Include*> includes )
{
	string parameters;
	for (size_t i = 0; i < includes.size (); i++)
	{
		Include& include = *includes[i];
		if (parameters.length () > 0)
			parameters += " ";
		parameters += "-I";
		parameters += include.directory;
	}
	return parameters;
}

string
MingwModuleHandler::GenerateGccIncludeParameters ( Module& module )
{
	string parameters = GenerateGccIncludeParametersFromVector ( module.project->includes );
	string s = GenerateGccIncludeParametersFromVector ( module.includes );
	if (s.length () > 0)
	{
		parameters += " ";
		parameters += s;
	}
	return parameters;
}

string
MingwModuleHandler::GenerateGccParameters ( Module& module )
{
	string parameters = GenerateGccDefineParameters ( module );
	string s = GenerateGccIncludeParameters ( module );
	if (s.length () > 0)
	{
		parameters += " ";
		parameters += s;
	}
	return parameters;
}

void
MingwModuleHandler::GenerateObjectFileTargets ( Module& module )
{
	if ( module.files.size () == 0 )
		return;
	
	for ( size_t i = 0; i < module.files.size (); i++ )
	{
		string sourceFilename = module.files[i]->name;
		string objectFilename = GetObjectFilename ( sourceFilename );
		fprintf ( fMakefile,
		          "%s: %s\n",
		          objectFilename.c_str (),
		          sourceFilename.c_str() );
		fprintf ( fMakefile,
		          "\t${gcc} -c %s -o %s %s\n",
		          sourceFilename.c_str (),
		          objectFilename.c_str (),
		          GenerateGccParameters ( module ).c_str () );
	}
	
	fprintf ( fMakefile, "\n" );
}

void
MingwModuleHandler::GenerateArchiveTarget ( Module& module )
{
	string archiveFilename = GetModuleArchiveFilename ( module );
	string sourceFilenames = GetSourceFilenames ( module );
	string objectFilenames = GetObjectFilenames ( module );
	
	fprintf ( fMakefile,
	          "%s: %s\n",
	          archiveFilename.c_str (),
	          sourceFilenames.c_str ());

	fprintf ( fMakefile,
	         "\t${ar} -rc %s %s\n\n",
	         archiveFilename.c_str (),
	         objectFilenames.c_str ());
}


MingwKernelModuleHandler::MingwKernelModuleHandler ( FILE* fMakefile )
	: MingwModuleHandler ( fMakefile )
{
}

bool
MingwKernelModuleHandler::CanHandleModule ( Module& module )
{
	return true;
}

void
MingwKernelModuleHandler::Process ( Module& module )
{
	GenerateKernelModuleTarget ( module );
}

void
MingwKernelModuleHandler::GenerateKernelModuleTarget ( Module& module )
{
	string workingDirectory = GetWorkingDirectory ( );
	string archiveFilename = GetModuleArchiveFilename ( module );
	string importLibraryDependencies = GetImportLibraryDependencies ( module );
	fprintf ( fMakefile, "%s: %s %s\n",
	          module.GetPath ().c_str (),
	          archiveFilename.c_str (),
	          importLibraryDependencies.c_str () );
	fprintf ( fMakefile,
	          "\t${gcc} -Wl,--base-file,%s" SSEP "base.tmp -o %s" SSEP "junk.tmp %s %s\n",
	          workingDirectory.c_str (),
	          workingDirectory.c_str (),
	          archiveFilename.c_str (),
	          importLibraryDependencies.c_str () );
	fprintf ( fMakefile,
	          "\t${rm} %s" SSEP "junk.tmp\n",
	          workingDirectory.c_str () );
	fprintf ( fMakefile,
	          "\t${dlltool} --dllname %s --base-file %s" SSEP "base.tmp --output-exp %s" SSEP "temp.exp --kill-at\n",
	          module.GetPath ().c_str (),
	          workingDirectory.c_str (),
	          workingDirectory.c_str ());
	fprintf ( fMakefile,
	          "\t${rm} %s" SSEP "base.tmp\n",
	          workingDirectory.c_str () );
	fprintf ( fMakefile,
	          "\t${ld} -Wl,%s" SSEP "temp.exp -o %s %s %s\n",
	          workingDirectory.c_str (),
	          module.GetPath ().c_str (),
	          archiveFilename.c_str (),
	          importLibraryDependencies.c_str () );
	fprintf ( fMakefile,
	          "\t${rm} %s" SSEP "temp.exp\n",
	          workingDirectory.c_str () );
	
	GenerateArchiveTarget ( module );
	GenerateObjectFileTargets ( module );
}
