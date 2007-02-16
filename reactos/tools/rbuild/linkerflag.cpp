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

LinkerFlag::LinkerFlag ( const Project& project_,
                         const XMLElement& linkerFlagNode )
	: project(project_),
	  module(NULL),
	  node(linkerFlagNode)
{
	Initialize();
}

LinkerFlag::LinkerFlag ( const Project& project_,
	                     const Module* module_,
	                     const XMLElement& linkerFlagNode )
	: project(project_),
	  module(module_),
	  node(linkerFlagNode)
{
	Initialize();
}

LinkerFlag::~LinkerFlag ()
{
}

void
LinkerFlag::Initialize ()
{
}

void
LinkerFlag::ProcessXML ()
{
	if ( node.value.size () == 0 )
	{
		throw XMLInvalidBuildFileException (
			node.location,
			"<linkerflag> is empty." );
	}
	flag = node.value;
}
