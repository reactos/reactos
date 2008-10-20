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
#include <map>

#include <stdio.h>

#include "dependencymap.h"
#include "../mingw/mingw.h"

using std::string;
using std::vector;
using std::map;
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

	typedef map<string, module_data*> ModuleMap;
	ModuleMap module_map;

	for( std::map<std::string, Module*>::const_iterator p = ProjectNode.modules.begin(); p != ProjectNode.modules.end(); ++ p )
	{
		Module& module = *p->second;
		if ((module.type != Iso) &&
			(module.type != LiveIso) &&
			(module.type != IsoRegTest) &&
			(module.type != LiveIsoRegTest))
		{
			vector<const IfableData*> ifs_list;
			ifs_list.push_back ( &module.project.non_if_data );
			ifs_list.push_back ( &module.non_if_data );

			module_data * current_data;
			ModuleMap::iterator mod_it = module_map.find ( module.name );
			if (mod_it != module_map.end ())
			{
				current_data = mod_it->second;
			}
			else
			{
				current_data = new module_data();
				if (current_data)
				{
					module_map.insert (std::make_pair<string, module_data*>(module.name, current_data));
				}
			}
			while ( ifs_list.size() )
			{
				const IfableData& data = *ifs_list.back();
				ifs_list.pop_back();
				const vector<Library*>& libs = data.libraries;
				for ( size_t j = 0; j < libs.size(); j++ )
				{
					ModuleMap::iterator it = module_map.find ( libs[j]->name );

					if ( it != module_map.end ())
					{
						module_data * data = it->second;
						data->references.push_back ( module.name );
					}
					else
					{
						module_data * data = new module_data();
						if ( data )
						{
							data->references.push_back ( module.name );
						}
						module_map.insert ( std::make_pair<string, module_data*>( libs[j]->name, data ) );
					}
					current_data->libraries.push_back ( libs[j]->name );
				}
			}
		}
	}

	fprintf ( m_DepMapFile, "<?xml version=\"1.0\" encoding=\"iso-8859-1\" ?>\r\n" );
	fprintf ( m_DepMapFile, "<?xml-stylesheet type=\"text/xsl\" href=\"depmap.xsl\"?>\r\n" );
	fprintf ( m_DepMapFile, "<components>\r\n" );

	for( std::map<std::string, Module*>::const_iterator p = ProjectNode.modules.begin(); p != ProjectNode.modules.end(); ++ p )
	{
		Module& module = *p->second;

		ModuleMap::iterator it = module_map.find ( module.name );
		if ( it != module_map.end () )
		{
			module_data * data = it->second;



			fprintf ( m_DepMapFile, "\t<component>\r\n" );
			fprintf ( m_DepMapFile, "\t\t<name>%s</name>\r\n", module.name.c_str () );
			fprintf ( m_DepMapFile, "\t\t<base>%s</base>\r\n", module.output->relative_path.c_str () );
			fprintf ( m_DepMapFile, "\t\t<ref_count>%u</ref_count>\r\n", (unsigned int)data->references.size () );
			fprintf ( m_DepMapFile, "\t\t<lib_count>%u</lib_count>\r\n", (unsigned int)data->libraries.size () );
#if 0
			if ( data->references.size () )
			{
				fprintf ( m_DepMapFile, "\t<references>\r\n" );
				for ( size_t j = 0; j < data->references.size (); j++ )
				{
					fprintf ( m_DepMapFile, "\t\t<reference name =\"%s\" />\r\n", data->references[j].c_str () );
				}
				fprintf ( m_DepMapFile, "\t</references>\r\n" );
			}

			if ( data->libraries.size () )
			{
				fprintf ( m_DepMapFile, "\t<libraries>\r\n" );
				for ( size_t j = 0; j < data->libraries.size (); j++ )
				{
					fprintf ( m_DepMapFile, "\t\t<library name =\"%s\" />\r\n", data->libraries[j].c_str () );
				}
				fprintf ( m_DepMapFile, "\t</libraries>\r\n" );
			}
#endif
			fprintf ( m_DepMapFile, "\t</component>\r\n" );
		}
	}

	fprintf ( m_DepMapFile, "</components>" );
}


DepMapConfiguration::DepMapConfiguration ( const std::string &name )
{
	/* nothing to do here */
}


