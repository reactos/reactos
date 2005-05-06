#include "pch.h"
#include <assert.h>

#include "rbuild.h"

using std::string;

InstallFile::InstallFile ( const Project& project_,
	                       const XMLElement& installfileNode,
	                       const string& path )
	: project ( project_ ),
	  node ( installfileNode )
{
	const XMLAttribute* att = node.GetAttribute ( "base", false );
	if ( att != NULL )
		base = att->value;
	else
		base = "";

	att = node.GetAttribute ( "newname", false );
	if ( att != NULL )
		newname = att->value;
	else
		newname = node.value;
	name = node.value;
	this->path = path;
}

InstallFile::~InstallFile ()
{
}

string
InstallFile::GetPath () const
{
	return path + SSEP + name;
}

void
InstallFile::ProcessXML()
{
}
