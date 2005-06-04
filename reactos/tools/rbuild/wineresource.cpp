#include "pch.h"
#include <assert.h>

#include "rbuild.h"

using std::string;
using std::vector;

WineResource::WineResource ( const Project& project,
                             string bin2res )
	: project ( project ),
	  bin2res ( bin2res )
{
}

WineResource::~WineResource ()
{
}

bool
WineResource::IsSpecFile ( const File& file )
{
	string extension = GetExtension ( file.name );
	if ( extension == ".spec" || extension == ".SPEC" )
		return true;
	return false;
}

bool
WineResource::IsWineModule ( const Module& module )
{
	const vector<File*>& files = module.non_if_data.files;
	for ( size_t i = 0; i < files.size (); i++ )
	{
		if ( IsSpecFile ( *files[i] ) )
			return true;
	}
	return false;
}

bool
WineResource::IsResourceFile ( const File& file )
{
	string extension = GetExtension ( file.name );
	if ( extension == ".rc" || extension == ".RC" )
		return true;
	return false;
}

string
WineResource::GetResourceFilename ( const Module& module )
{
	const vector<File*>& files = module.non_if_data.files;
	for ( size_t i = 0; i < files.size (); i++ )
	{
		if ( IsResourceFile ( *files[i] ) )
			return files[i]->name;
	}
	return "";
}

void
WineResource::UnpackResources ( bool verbose )
{
	for ( size_t i = 0; i < project.modules.size (); i++ )
	{
		if ( IsWineModule ( *project.modules[i] ) )
		{
			UnpackResourcesInModule ( *project.modules[i],
			                          verbose );
		}
	}
}

void
WineResource::UnpackResourcesInModule ( Module& module,
                                        bool verbose )
{
	string resourceFilename = GetResourceFilename ( module );
	if ( resourceFilename.length () == 0 )
		return;

	if ( verbose )
	{
		printf ( "\nUnpacking resources for %s",
		         module.name.c_str () );
	}

	string outputDirectory = module.GetBasePath ();
	string parameters = ssprintf ( "-b %s -f -x %s",
	                               NormalizeFilename ( outputDirectory ).c_str (),
	                               NormalizeFilename ( resourceFilename ).c_str () );
	string command = bin2res + " " + parameters;
	int exitcode = system ( command.c_str () );
	if ( exitcode != 0 )
	{
		throw InvocationFailedException ( command,
		                                  exitcode );
	}
}
