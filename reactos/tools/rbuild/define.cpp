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

Define::Define ( const Project& project,
                 const XMLElement& defineNode )
	: project(project),
	  module(NULL),
	  node(&defineNode)
{
	Initialize();
}

Define::Define ( const Project& project,
                 const Module* module,
                 const XMLElement& defineNode )
	: project(project),
	  module(module),
	  node(&defineNode)
{
	Initialize();
}

Define::Define ( const Project& project,
                 const Module* module,
                 const std::string& name_,
                 const std::string& backend_,
                 bool redefine_)
	: project(project),
	  module(module),
	  node(NULL)
{
	name = name_;
	value = "";
	backend = backend_;
	redefine = redefine_;
}

Define::~Define ()
{
//	if ( node )
//		delete node;
//	if ( module )
//		delete module;
}

void
Define::Initialize()
{
	redefine = node->name == "redefine";

	const XMLAttribute* att = node->GetAttribute ( "name", true );

	att = node->GetAttribute ( "name", true );
	assert(att);

	size_t name_len = att->value.find ( '(' );

	name = att->value.substr ( 0, name_len );

	if ( name_len != std::string::npos )
		arguments = att->value.substr ( att->value.find ( '(' ) );

	value = node->value;

	att = node->GetAttribute ( "backend", false );
	if ( att )
		backend = att->value;

	ParseCompilers ( *node, "cpp" );
}

void
Define::ProcessXML()
{
}
