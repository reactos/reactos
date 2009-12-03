/*
 * Copyright (C) 2002 Patrik Stridvall
 * Copyright (C) 2005 Royce Mitchell III
 * Copyright (C) 2006 Hervé Poussineau
 * Copyright (C) 2006 Christoph von Wittich
 * Copyright (C) 2009 Ged Murphy
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef _MSC_VER
#pragma warning ( disable : 4786 )
#endif//_MSC_VER

#include <string>
#include <vector>
#include <set>
#include <algorithm>
#include <fstream>
#include <iostream>

#include <stdio.h>

#include "msvc.h"

using std::string;
using std::vector;
using std::set;

typedef set<string> StringSet;

#ifdef OUT
#undef OUT
#endif//OUT


VCXProjMaker::VCXProjMaker ( )
{
	vcproj_file = "";
}

VCXProjMaker::VCXProjMaker ( Configuration& buildConfig,
							 const std::vector<MSVCConfiguration*>& msvc_configs,
							 std::string filename )
{
	configuration = buildConfig;
	m_configurations = msvc_configs;
	vcproj_file = filename;

	OUT = fopen ( vcproj_file.c_str(), "wb" );

	if ( !OUT )
	{
		printf ( "Could not create file '%s'.\n", vcproj_file.c_str() );
	}
}

VCXProjMaker::~VCXProjMaker()
{
	fclose ( OUT );
}

void
VCXProjMaker::_generate_proj_file ( const Module& module )
{
	// TODO: Implement me
	ProjMaker::_generate_proj_file ( module );
}

void
VCXProjMaker::_generate_user_configuration ()
{
	// Call base implementation
	ProjMaker::_generate_user_configuration ();
}

void
VCXProjMaker::_generate_standard_configuration( const Module& module, const MSVCConfiguration& cfg, BinaryType binaryType )
{
	// TODO: Implement me
	ProjMaker::_generate_standard_configuration ( module, cfg, binaryType );
}

void
VCXProjMaker::_generate_makefile_configuration( const Module& module, const MSVCConfiguration& cfg )
{
	// TODO: Implement me
	ProjMaker::_generate_makefile_configuration ( module, cfg );
}
