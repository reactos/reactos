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
		
		// REM TODO FIXME actually do something with Project object...
#if 0
		printf ( "Found %d modules:\n", project.modules.size() );
		for ( size_t i = 0; i < project.modules.size(); i++ )
		{
			Module& m = *project.modules[i];
			printf ( "\t%s in folder: %s\n",
			         m.name.c_str(),
			         m.path.c_str() );
			printf ( "\txml dependencies:\n\t\t%s\n",
			         projectFilename.c_str() );
			const XMLElement* e = &m.node;
			while ( e )
			{
				if ( e->name == "xi:include" )
				{
					const XMLAttribute* att = e->GetAttribute("top_href",false);
					if ( att )
					{
						printf ( "\t\t%s\n", att->value.c_str() );
					}
				}
				e = e->parentElement;
			}
			printf ( "\tfiles:\n" );
			for ( size_t j = 0; j < m.files.size(); j++ )
			{
				printf ( "\t\t%s\n", m.files[j]->name.c_str() );
			}
		}
#endif

		return 0;
	}
	catch (Exception& ex)
	{
		printf ( "%s: %s\n",
		         typeid(ex).name(), ex.Message.c_str() );
		return 1;
	}
}
