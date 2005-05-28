#include "pch.h"
#include <assert.h>

#include "rbuild.h"

using std::string;
using std::vector;

Define::Define ( const Project& project,
                 const XMLElement& defineNode )
	: project(project),
	  module(NULL),
	  node(defineNode)
{
	Initialize();
}

Define::Define ( const Project& project,
	             const Module* module,
                 const XMLElement& defineNode )
	: project(project),
	  module(module),
	  node(defineNode)
{
	Initialize();
}

Define::~Define ()
{
}

void
Define::Initialize()
{
	const XMLAttribute* att = node.GetAttribute ( "name", true );
	assert(att);
	name = att->value;
	value = node.value;
}

void
Define::ProcessXML()
{
}
