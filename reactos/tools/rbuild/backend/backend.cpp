
#include "../pch.h"

#include "../rbuild.h"
#include "backend.h"

using std::string;
using std::vector;
using std::map;

map<string,Backend::Factory*>*
Backend::Factory::factories = NULL;
int
Backend::Factory::ref = 0;

Backend::Factory::Factory ( const std::string& name_ )
{
	string name(name_);
	strlwr ( &name[0] );
	if ( !ref++ )
		factories = new map<string,Factory*>;
	(*factories)[name] = this;
}

Backend::Factory::~Factory ()
{
	if ( !--ref )
	{
		delete factories;
		factories = NULL;
	}
}

/*static*/ Backend*
Backend::Factory::Create ( const string& name,
                           Project& project,
                           Configuration& configuration )
{
	string sname ( name );
	strlwr ( &sname[0] );
	if ( !factories || !factories->size () )
		throw InvalidOperationException ( __FILE__,
		                                  __LINE__,
		                                  "No registered factories" );
	Backend::Factory* f = (*factories)[sname];
	if ( !f )
	{
		throw UnknownBackendException ( sname );
		return NULL;
	}
	return (*f) ( project, configuration );
}

Backend::Backend ( Project& project,
                   Configuration& configuration )
	: ProjectNode ( project ),
	  configuration ( configuration )
{
}
