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
	: project ( project ),
	  module ( NULL ),
	  node ( includeNode ),
	  baseModule ( NULL )
{
}

Include::Include ( const Project& project,
                   const Module* module,
                   const XMLElement* includeNode )
	: project ( project ),
	  module ( module ),
	  node ( includeNode ),
	  baseModule ( NULL )
{
}

Include::Include ( const Project& project,
                   string directory,
                   string basePath )
	: project ( project ),
	  module ( NULL ),
	  node ( NULL ),
	  baseModule ( NULL )
{
	this->directory = NormalizeFilename ( basePath + sSep + directory );
	this->basePath = NormalizeFilename ( basePath );
}

Include::~Include ()
{
}

void
Include::ProcessXML()
{
	const XMLAttribute* att;
	att = node->GetAttribute ( "base", false );
	if ( att )
	{
		bool referenceResolved = false;

		if ( !module )
		{
			throw XMLInvalidBuildFileException (
				node->location,
				"'base' attribute illegal from global <include>" );
		}

		if ( !referenceResolved )
		{
			if ( att->value == project.name )
			{
				basePath = ".";
				referenceResolved = true;
			}
			else
			{
				const Module* base = project.LocateModule ( att->value );
				if ( base != NULL )
				{
					baseModule = base;
					basePath = base->GetBasePath ();
					referenceResolved = true;
				}
			}
		}

		if ( !referenceResolved )
			throw XMLInvalidBuildFileException (
				node->location,
				"<include> attribute 'base' references non-existant project or module '%s'",
				att->value.c_str() );
		directory = NormalizeFilename ( basePath + sSep + node->value );
	}
	else
		directory = NormalizeFilename ( node->value );

	att = node->GetAttribute ( "root", false );
	if ( att )
	{
		if ( att->value != "intermediate" && att->value != "output" )
		{
			throw InvalidAttributeValueException (
				node->location,
				"root",
				att->value );
		}

		root = att->value;
	}
}
