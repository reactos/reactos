
#include "../pch.h"

#include "../Rbuild.h"
#include "backend.h"

#include "mingw/mingw.h"

using std::string;
using std::vector;

vector<Backend::Factory*> Backend::factories;

/*static*/ void
Backend::InitFactories()
{
	factories.push_back ( new Factory ( "mingw", MingwBackend::Factory ) );
}

/*static*/ Backend*
Backend::Create ( const std::string& name, Project& project )
{
	string sname ( name );
	strlwr ( &sname[0] );
	if ( !factories.size() )
		throw Exception ( "internal tool error: no registered factories" );
	for ( size_t i = 0; i < factories.size(); i++ )
	{
		if ( sname == factories[i]->name )
			return (factories[i]->factory) ( project );
	}
	throw UnknownBackendException ( sname );
	return NULL;
}

Backend::Backend ( Project& project )
	: ProjectNode ( project )
{
}
