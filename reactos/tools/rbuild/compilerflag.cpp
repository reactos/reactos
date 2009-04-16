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

CompilerFlag::CompilerFlag ( const Project& project_,
                             const XMLElement& compilerFlagNode )
	: project(project_),
	  module(NULL),
	  node(compilerFlagNode)
{
	Initialize();
}

CompilerFlag::CompilerFlag ( const Project& project_,
	                         const Module* module_,
	                         const XMLElement& compilerFlagNode )
	: project(project_),
	  module(module_),
	  node(compilerFlagNode)
{
	Initialize();
}

CompilerFlag::~CompilerFlag ()
{
}

void
CompilerFlag::Initialize ()
{
	if (node.value.size () == 0)
	{
		throw XMLInvalidBuildFileException (
			node.location,
			"<compilerflag> is empty." );
	}

	flag = node.value;

	ParseCompilers ( node, "cc,cxx" );
}

void
CompilerFlag::ProcessXML ()
{
}
