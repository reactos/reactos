#include "pch.h"
#include <assert.h>

#include "rbuild.h"

using std::string;
using std::vector;

CompilerFlag::CompilerFlag ( const Project& project_,
                             const XMLElement& compilerFlagNode )
	: project(project_),
	  module(NULL),
	  node(compilerFlagNode)
{
	Initialize();
}

CompilerFlag::CompilerFlag ( const Project& project_,
	                         const Module* module_,
	                         const XMLElement& compilerFlagNode )
	: project(project_),
	  module(module_),
	  node(compilerFlagNode)
{
	Initialize();
}

CompilerFlag::~CompilerFlag ()
{
}

void
CompilerFlag::Initialize ()
{
	if (node.value.size () == 0)
	{
		throw InvalidBuildFileException (
			node.location,
			"<compilerflag> is empty." );
	}
	flag = node.value;
}

void
CompilerFlag::ProcessXML ()
{
}
