#include "pch.h"
#include <assert.h>

#include "rbuild.h"

using std::string;
using std::vector;

Define::Define ( Project* project,
                 const XMLElement& defineNode )
	: project(project),
	  module(NULL),
	  node(defineNode)
{
	Initialize (defineNode);
}

Define::Define ( Project* project,
	             Module* module,
                 const XMLElement& defineNode )
	: project(project),
	  module(module),
	  node(defineNode)
{
	Initialize (defineNode);
}

Define::~Define ()
{
}

void
Define::Initialize ( const XMLElement& defineNode )
{
	const XMLAttribute* att = defineNode.GetAttribute ( "name", true );
	assert(att);
	name = att->value;
	value = defineNode.value;
}

void
Define::ProcessXML ( const XMLElement& e )
{
}
