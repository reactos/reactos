
#include "../pch.h"

#include "../Rbuild.h"
#include "backend.h"

#include "mingw/mingw.h"

using std::string;
using std::vector;

vector<Backend::Factory*>* Backend::Factory::factories = NULL;

Backend::Factory::Factory ( const std::string& name_ )
	: name(name_)
{
	if ( !factories )
		factories = new vector<Factory*>;
	factories->push_back ( this );
}

/*static*/ Backend*
Backend::Factory::Create ( const std::string& name, Project& project )
{
	string sname ( name );
	strlwr ( &sname[0] );
	if ( !factories || !factories->size() )
		throw Exception ( "internal tool error: no registered factories" );
	vector<Backend::Factory*>& fact = *factories;
	for ( size_t i = 0; i < fact.size(); i++ )
	{
		//char* p = *fact[i];
		if ( sname == fact[i]->name )
			return (*fact[i]) ( project );
	}
	throw UnknownBackendException ( sname );
	return NULL;
}

Backend::Backend ( Project& project )
	: ProjectNode ( project )
{
}
