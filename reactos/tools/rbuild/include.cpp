#include "pch.h"
#include <assert.h>

#include "rbuild.h"

using std::string;
using std::vector;

Include::Include ( const Project& project_,
                   const XMLElement& includeNode )
	: project (project_),
	  module (NULL),
	  node (includeNode)
{
	Initialize();
}

Include::Include ( const Project& project_,
	               const Module* module_,
	               const XMLElement& includeNode )
	: project (project_),
	  module (module_),
	  node (includeNode)
{
	Initialize();
}

Include::~Include ()
{
}

void
Include::Initialize()
{
}

void
Include::ProcessXML()
{
	const XMLAttribute* att;
	att = node.GetAttribute ( "base",
	                          false );
	if ( att )
	{
		if ( !module )
			throw InvalidBuildFileException (
				node.location,
				"'base' attribute illegal from global <include>" );
		bool referenceResolved = false;
		if ( att->value == project.name )
		{
			basePath = ".";
			referenceResolved = true;
		}
		else
		{
			const Module* base = project.LocateModule ( att->value );
			if ( base != NULL )
			{
				basePath = base->GetBasePath ();
				referenceResolved = true;
			}
		}
		if ( !referenceResolved )
			throw InvalidBuildFileException (
				node.location,
				"<include> attribute 'base' references non-existant project or module '%s'",
				att->value.c_str() );
		directory = FixSeparator ( basePath + "/" + node.value );
	}
	else
		directory = FixSeparator ( node.value );
}
