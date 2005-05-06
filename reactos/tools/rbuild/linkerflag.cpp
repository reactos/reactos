#include "pch.h"
#include <assert.h>

#include "rbuild.h"

using std::string;
using std::vector;

LinkerFlag::LinkerFlag ( const Project& project_,
                         const XMLElement& linkerFlagNode )
	: project(project_),
	  module(NULL),
	  node(linkerFlagNode)
{
	Initialize();
}

LinkerFlag::LinkerFlag ( const Project& project_,
	                     const Module* module_,
	                     const XMLElement& linkerFlagNode )
	: project(project_),
	  module(module_),
	  node(linkerFlagNode)
{
	Initialize();
}

LinkerFlag::~LinkerFlag ()
{
}

void
LinkerFlag::Initialize ()
{
}

void
LinkerFlag::ProcessXML ()
{
	if ( node.value.size () == 0 )
	{
		throw InvalidBuildFileException (
			node.location,
			"<linkerflag> is empty." );
	}
	flag = node.value;
}
