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

InstallFile::InstallFile ( const Project& project,
                           const XMLElement& installfileNode,
                           const string& path )
	: XmlNode(project, installfileNode )
{
	const XMLAttribute* base = node.GetAttribute ( "installbase", false );
	const XMLAttribute* newname = node.GetAttribute ( "newname", false );

	DirectoryLocation source_directory = SourceDirectory;
	const XMLAttribute* att = node.GetAttribute ( "root", false );
	if ( att != NULL)
	{
		if ( att->value == "intermediate" )
			source_directory = IntermediateDirectory;
		else if ( att->value == "output" )
			source_directory = OutputDirectory;
		else
		{
			throw InvalidAttributeValueException (
				node.location,
				"root",
				att->value );
		}
	}

	source = new FileLocation ( source_directory,
	                            path,
	                            node.value,
	                            &node );
	target = new FileLocation ( InstallDirectory,
	                            base && base->value != "."
	                                ? base->value
	                                : "",
	                            newname
	                                ? newname->value
	                                : node.value,
	                            &node );
}
