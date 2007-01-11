/*
 * Copyright (C) 2007 Christoph von Wittich
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
#ifdef _MSC_VER
#pragma warning ( disable : 4786 )
#endif//_MSC_VER

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include <stdio.h>

#include "dependencymap.h"
#include "../mingw/mingw.h"

using std::string;
using std::vector;
using std::ifstream;

#ifdef OUT
#undef OUT
#endif//OUT


static class DepMapFactory : public Backend::Factory
{
	public:

		DepMapFactory() : Factory("DepMap", "Dependency Map") {}
		Backend *operator() (Project &project,
		                     Configuration& configuration)
		{
			return new DepMapBackend(project, configuration);
		}
		
} factory;


DepMapBackend::DepMapBackend(Project &project,
	Configuration& configuration) : Backend(project, configuration)
{

}

void DepMapBackend::Process()
{
	string filename_depmap ( "dependencymap.xml" );
	printf ( "Creating dependecy map: %s\n", filename_depmap.c_str() );

	m_DepMapFile = fopen ( filename_depmap.c_str(), "wb" );

	if ( !m_DepMapFile )
	{
		printf ( "Could not create file '%s'.\n", filename_depmap.c_str() );
		return;
	}

	_generate_depmap ( m_DepMapFile );

	fclose ( m_DepMapFile );
	printf ( "Done.\n" );
}

void
DepMapBackend::_clean_project_files ( void )
{
	remove ( "dependencymap.xml" );
}

void
DepMapBackend::_generate_depmap ( FILE* OUT )
{

	/* add dependencies */
	for ( size_t i = 0; i < ProjectNode.modules.size(); i++ )
	{
		Module& module = *ProjectNode.modules[i];

		if ((module.type != Iso) && 
			(module.type != LiveIso) &&
			(module.type != IsoRegTest) &&
			(module.type != LiveIsoRegTest))
		{
			vector<const IfableData*> ifs_list;
			ifs_list.push_back ( &module.project.non_if_data );
			ifs_list.push_back ( &module.non_if_data );
			while ( ifs_list.size() )
			{
				const IfableData& data = *ifs_list.back();
				ifs_list.pop_back();
				const vector<Library*>& libs = data.libraries;
				for ( size_t j = 0; j < libs.size(); j++ )
				{
					//add module.name and libs[j]->name 
				}
			}
		}	
	}

/* save data to file
	fprintf ( OUT, "\r\n" );
*/
}


DepMapConfiguration::DepMapConfiguration ( const std::string &name )
{
	/* nothing to do here */
}


