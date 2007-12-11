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


CDFile::CDFile ( const Project& project,
                 const XMLElement& cdfileNode,
                 const string& path )
	: XmlNode ( project, cdfileNode )
{
	const XMLAttribute* att = cdfileNode.GetAttribute ( "installbase", false );
	string target_relative_directory;
	if ( att != NULL )
        target_relative_directory = Environment::ReplaceVariable ( "$(CDOUTPUT)", Environment::GetCdOutputPath (), att->value );
	else
		target_relative_directory = "";

	const XMLAttribute* nameoncd = cdfileNode.GetAttribute ( "nameoncd", false );

	source = new FileLocation ( SourceDirectory,
	                            path,
	                            cdfileNode.value,
	                            &cdfileNode );
	target = new FileLocation ( OutputDirectory,
	                            target_relative_directory,
	                            nameoncd ? att->value : cdfileNode.value,
	                            &cdfileNode );
}

BootstrapFile::BootstrapFile ( const Project& project,
                 const XMLElement& cdfileNode,
                 const string& path )
	: XmlNode ( project, cdfileNode )
{
	const XMLAttribute* att = cdfileNode.GetAttribute ( "installbase", false );
	string target_relative_directory;
	if ( att != NULL )
		target_relative_directory = Environment::ReplaceVariable ( "$(CDOUTPUT)", Environment::GetCdOutputPath (), att->value );
	else
		target_relative_directory = "";

	const XMLAttribute* nameoncd = cdfileNode.GetAttribute ( "nameoncd", false );

	source = new FileLocation ( SourceDirectory,
	                            path,
	                            cdfileNode.value,
	                            &cdfileNode );
	target = new FileLocation ( OutputDirectory,
	                            target_relative_directory,
	                            nameoncd ? att->value : cdfileNode.value,
	                            &cdfileNode );
}
