/*
 * Copyright (C) 2005 Casper S. Hornstrup
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include "pch.h"
#include <assert.h>

#include "rbuild.h"

using std::string;

string
CDFile::ReplaceVariable ( const string& name,
                          const string& value,
                          string path )
{
	size_t i = path.find ( name );
	if ( i != string::npos )
		return path.replace ( i, name.length (), value );
	else
		return path;
}

CDFile::CDFile ( const Project& project_,
	             const XMLElement& cdfileNode,
	             const string& path )
	: project ( project_ ),
	  node ( cdfileNode )
{
	const XMLAttribute* att = node.GetAttribute ( "base", false );
	if ( att != NULL )
		base = ReplaceVariable ( "$(CDOUTPUT)", Environment::GetCdOutputPath (), att->value );
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
	return path + sSep + name;
}

void
CDFile::ProcessXML()
{
}
