// rbuild.cpp

#include "pch.h"
#include <typeinfo>

#include <stdio.h>
#include <io.h>
#include <assert.h>

#include "rbuild.h"
#include "backend/backend.h"
#include "backend/mingw/mingw.h"

using std::string;
using std::vector;

int
main ( int argc, char** argv )
{
	if ( argc != 2 )
	{
		printf ( "syntax: rbuild {buildtarget}\n" );
		return 1;
	}
	string buildtarget ( argv[1] );
	strlwr ( &buildtarget[0] );
	try
	{
		string projectFilename ( "ReactOS.xml" );
		Project project ( projectFilename );
		Backend* backend = Backend::Factory::Create ( buildtarget, project );
		backend->Process ();
		delete backend;

		return 0;
	}
	catch (Exception& ex)
	{
		printf ( "%s: %s\n",
		         typeid(ex).name(), ex.Message.c_str() );
		return 1;
	}
}
