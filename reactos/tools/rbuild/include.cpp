#include "pch.h"
#include <assert.h>

#include "rbuild.h"

using std::string;
using std::vector;

Include::Include ( const Project& project_,
                   const XMLElement& includeNode )
	: project(project_),
	  module(NULL),
	  node(includeNode)
{
	Initialize();
}

Include::Include ( const Project& project_,
	               const Module* module_,
	               const XMLElement& includeNode )
	: project(project_),
	  module(module_),
	  node(includeNode)
{
	Initialize();
}

Include::~Include ()
{
}

void
Include::Initialize()
{
	directory = FixSeparator ( node.value );
}

void
Include::ProcessXML()
{
}
