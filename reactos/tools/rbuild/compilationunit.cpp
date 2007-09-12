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

CompilationUnit::CompilationUnit ( File* file )
	: project(NULL),
	  module(NULL),
	  node(NULL)
{
	local_name = file->file.name;
	name = file->file.relative_path + sSep + file->file.name;
	files.push_back ( file );
}

CompilationUnit::CompilationUnit ( const Project* project,
                                   const Module* module,
                                   const XMLElement* node )
	: project(project),
	  module(module),
	  node(node)
{
	const XMLAttribute* att = node->GetAttribute ( "name", true );
	assert(att);
	local_name = att->value;
	name = module->output->relative_path + cSep + att->value;
}

CompilationUnit::~CompilationUnit ()
{
	size_t i;
	for ( i = 0; i < files.size (); i++ )
		delete files[i];
}

void
CompilationUnit::ProcessXML ()
{
	size_t i;
	for ( i = 0; i < files.size (); i++ )
		files[i]->ProcessXML ();
}

bool
CompilationUnit::IsGeneratedFile () const
{
	if ( files.size () != 1 )
		return false;
	File* file = files[0];
	string extension = GetExtension ( file->file );
	return ( extension == ".spec" || extension == ".SPEC" );
}

bool
CompilationUnit::HasFileWithExtension ( const std::string& extension ) const
{
	size_t i;
	for ( i = 0; i < files.size (); i++ )
	{
		File& file = *files[i];
		string fileExtension = GetExtension ( file.file );
		if ( !stricmp ( fileExtension.c_str (), extension.c_str () ) )
			return true;
	}
	return false;
}

bool
CompilationUnit::IsFirstFile () const
{
	if ( files.size () == 0 || files.size () > 1 )
		return false;
	File* file = files[0];
	return file->first;
}


const FileLocation*
CompilationUnit::GetFilename () const
{
	if ( files.size () == 0 || files.size () > 1 )
	{
		return new FileLocation ( IntermediateDirectory,
		                          module ? module->output->relative_path : "",
		                          local_name );
	}

	File* file = files[0];
	return new FileLocation ( file->file );
}

std::string
CompilationUnit::GetSwitches () const
{
	if ( files.size () == 0 || files.size () > 1 )
		return "";
	File* file = files[0];
	return file->switches;
}
