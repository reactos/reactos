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
using std::vector;

Include::Include ( const Project& project,
                   const XMLElement* includeNode )
	: directory ( NULL ),
	  project ( project ),
	  node ( includeNode ),
	  module ( NULL )
{
}

Include::Include ( const Project& project,
                   const XMLElement* includeNode,
                   const Module* module )
	: directory ( NULL ),
	  project ( project ),
	  node ( includeNode ),
	  module ( module )
{
}

Include::Include ( const Project& project,
                   DirectoryLocation root,
                   const std::string& relative_path )
	: project ( project ),
	  node ( NULL )
{
	directory = new FileLocation ( root, relative_path, "" );
}

Include::~Include()
{
}

void
Include::ProcessXML ()
{
	DirectoryLocation root = SourceDirectory;
	const Module *base = module;

	string relative_path;
	const XMLAttribute* att = node->GetAttribute ( "base", false );
	if ( att )
	{
		if ( !module )
			throw XMLInvalidBuildFileException (
				node->location,
				"'base' attribute illegal from global <include>" );

		if ( att->value == project.name )
			base = NULL;
		else
		{
			base = project.LocateModule ( att->value );
			if ( !base )
				throw XMLInvalidBuildFileException (
					node->location,
					"<include> attribute 'base' references non-existant project or module '%s'",
					att->value.c_str() );
			root = GetDefaultDirectoryTree ( base );
		}
	}

	if ( base )
	{
		relative_path = base->output->relative_path;
		if ( node->value.length () > 0 && node->value != "." )
			relative_path += sSep + node->value;
	}
	else
		relative_path = node->value;

	att = node->GetAttribute ( "root", false );
	if ( att )
	{
		if ( att->value == "intermediate" )
			root = IntermediateDirectory;
		else if ( att->value == "output" )
			root = OutputDirectory;
		else
			throw InvalidAttributeValueException ( node->location,
			                                       "root",
			                                       att->value );
	}

	directory = new FileLocation ( root,
	                               relative_path,
	                               "" );
}

DirectoryLocation
Include::GetDefaultDirectoryTree ( const Module* module ) const
{
	if ( module != NULL &&
	     ( module->type == RpcServer ||
	       module->type == RpcClient ||
	       module->type == IdlHeader) )
		return IntermediateDirectory;
	else
		return SourceDirectory;
}
