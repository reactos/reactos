
#include "../../pch.h"

#include "../../rbuild.h"
#include "mingw.h"
#include "modulehandler.h"

using std::string;

MingwModuleHandler::MingwModuleHandler ( FILE* fMakefile )
	: fMakefile ( fMakefile )
{
}

string MingwModuleHandler::GetModuleDependencies ( Module& module )
{
	string dependencies ( "" );
	
	for ( size_t i = 0; i < module.libraries.size(); i++ )
	{
		if (dependencies.size () > 0)
			dependencies += " ";
		dependencies += module.libraries[i]->name;
	}
	return dependencies;
}


MingwKernelModuleHandler::MingwKernelModuleHandler ( FILE* fMakefile )
	: MingwModuleHandler ( fMakefile )
{
}

bool MingwKernelModuleHandler::CanHandleModule ( Module& module )
{
	return true;
}

void MingwKernelModuleHandler::Process ( Module& module )
{
	GenerateKernelModuleTarget ( module );
}

void MingwKernelModuleHandler::GenerateKernelModuleTarget ( Module& module )
{
	fprintf ( fMakefile, "%s: %s",
	          module.name.c_str (),
	          GetModuleDependencies ( module ).c_str () );
	fprintf ( fMakefile, "\n" );
	fprintf ( fMakefile, "\t" );
	fprintf ( fMakefile, "\n\n" );
}
