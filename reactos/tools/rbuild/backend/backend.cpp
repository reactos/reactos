
#include "../pch.h"

#include "../Rbuild.h"
#include "backend.h"

using std::string;
using std::vector;
using std::map;

map<const char*,Backend::Factory*>* Backend::Factory::factories = NULL;

Backend::Factory::Factory ( const std::string& name_ )
{
	string name(name_);
	strlwr ( &name[0] );
	if ( !factories )
		factories = new map<const char*,Factory*>;
	(*factories)[name.c_str()] = this;
}

/*static*/ Backend*
Backend::Factory::Create ( const std::string& name, Project& project )
{
	string sname ( name );
	strlwr ( &sname[0] );
	if ( !factories || !factories->size() )
		throw Exception ( "internal tool error: no registered factories" );
	Backend::Factory* f = (*factories)[sname.c_str()];
	if ( !f )
	{
		throw UnknownBackendException ( sname );
		return NULL;
	}
	return (*f) ( project );
}

Backend::Backend ( Project& project )
	: ProjectNode ( project )
{
}
