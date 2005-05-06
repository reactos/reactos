#include "pch.h"
#include <assert.h>

#include "rbuild.h"

using std::string;

CDFile::CDFile ( const Project& project_,
	             const XMLElement& cdfileNode,
	             const string& path )
	: project ( project_ ),
	  node ( cdfileNode )
{
	const XMLAttribute* att = node.GetAttribute ( "base", false );
	if ( att != NULL )
		base = att->value;
	else
		base = "";

	att = node.GetAttribute ( "nameoncd", false );
	if ( att != NULL )
		nameoncd = att->value;
	else
		nameoncd = node.value;
	name = node.value;
	this->path = path;
}

CDFile::~CDFile ()
{
}

string
CDFile::GetPath () const
{
	return path + SSEP + name;
}

void
CDFile::ProcessXML()
{
}
