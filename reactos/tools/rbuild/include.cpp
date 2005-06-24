#include "pch.h"
#include <assert.h>

#include "rbuild.h"

using std::string;
using std::vector;

Include::Include ( const Project& project,
                   const XMLElement* includeNode )
	: project ( project ),
	  module ( NULL ),
	  node ( includeNode )
{
}

Include::Include ( const Project& project,
                   const Module* module,
                   const XMLElement* includeNode )
	: project ( project ),
	  module ( module ),
	  node ( includeNode )
{
}

Include::Include ( const Project& project,
                   string directory,
                   string basePath )
	: project ( project ),
	  module ( NULL ),
	  node ( NULL )
{
	this->directory = NormalizeFilename ( basePath + SSEP + directory );
	this->basePath = NormalizeFilename ( basePath );
}

Include::~Include ()
{
}

void
Include::ProcessXML()
{
	const XMLAttribute* att;
	att = node->GetAttribute ( "base",
	                           false );
	if ( att )
	{
		if ( !module )
			throw InvalidBuildFileException (
				node->location,
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
				node->location,
				"<include> attribute 'base' references non-existant project or module '%s'",
				att->value.c_str() );
		directory = NormalizeFilename ( basePath + SSEP + node->value );
	}
	else
		directory = NormalizeFilename ( node->value );
}
