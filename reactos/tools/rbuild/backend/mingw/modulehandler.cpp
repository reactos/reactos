
#include "../../pch.h"

#include "../../rbuild.h"
#include "mingw.h"
#include "modulehandler.h"

using std::string;

MingwModuleHandler::MingwModuleHandler ( FILE* fMakefile )
	: fMakefile ( fMakefile )
{
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
MingwModuleHandler::GetModuleLibraryDependencies ( Module& module )
{
	if ( module.libraries.size () == 0 )
		return "";
	
	string dependencies ( "" );
	for ( size_t i = 0; i < module.libraries.size (); i++ )
	{
		if ( dependencies.size () > 0 )
			dependencies += " ";
		dependencies += module.libraries[i]->name;
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
		          sourceFilename.c_str (),
		          objectFilename.c_str() );
		fprintf ( fMakefile,
		          "\t${gcc} -c %s -o %s\n",
		          sourceFilename.c_str (),
		          objectFilename.c_str () );
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
	fprintf ( fMakefile, "%s: %s\n",
	          module.GetPath ().c_str (),
	          GetModuleLibraryDependencies ( module ).c_str () );
	fprintf ( fMakefile, "\t\n\n" );
	GenerateArchiveTarget ( module );
	GenerateObjectFileTargets ( module );
}
