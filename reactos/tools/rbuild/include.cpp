#include "pch.h"
#include <assert.h>

#include "rbuild.h"

using std::string;
using std::vector;

Include::Include ( Project* project,
                   const XMLElement& includeNode )
	: project(project),
	  node(includeNode)
{
	Initialize ( includeNode );
}

Include::Include ( Project* project,
	               Module* module,
	               const XMLElement& includeNode )
	: project(project),
	  node(includeNode)
{
	Initialize ( includeNode );
}

Include::~Include ()
{
}

void
Include::Initialize ( const XMLElement& includeNode )
{
	directory = FixSeparator ( includeNode.value );
}

void
Include::ProcessXML ( const XMLElement& e )
{
}
