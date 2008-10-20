/*
 * Copyright (C) 2007 Marc Piulachs (marc.piulachs [at] codexchange [dot] net)
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

#include "versionreport.h"
#include "../mingw/mingw.h"

using std::string;
using std::vector;
using std::map;
using std::ifstream;

#ifdef OUT
#undef OUT
#endif//OUT


static class VReportFactory : public Backend::Factory
{
	public:

		VReportFactory() : Factory("VReport", "Version Report") {}
		Backend *operator() (Project &project,
		                     Configuration& configuration)
		{
			return new VReportBackend(project, configuration);
		}

} factory;


VReportBackend::VReportBackend(Project &project,
	Configuration& configuration) : Backend(project, configuration)
{

}

void VReportBackend::Process()
{
	string filename_depmap ( "versionreport.xml" );
	printf ( "Creating version report: %s\n", filename_depmap.c_str() );

	m_VReportFile = fopen ( filename_depmap.c_str(), "wb" );

	if ( !m_VReportFile )
	{
		printf ( "Could not create file '%s'.\n", filename_depmap.c_str() );
		return;
	}

	GenerateReport ( m_VReportFile );

	fclose ( m_VReportFile );
	printf ( "Done.\n" );
}

void
VReportBackend::CleanFiles ( void )
{
	remove ( "versionreport.xml" );
}

void
VReportBackend::GenerateReport ( FILE* OUT )
{
	fprintf ( m_VReportFile, "<?xml version=\"1.0\" encoding=\"iso-8859-1\" ?>\r\n" );
	fprintf ( m_VReportFile, "<?xml-stylesheet type=\"text/xsl\" href=\"vreport.xsl\"?>\r\n" );
	fprintf ( m_VReportFile, "<components>\r\n" );

	for( std::map<std::string, Module*>::const_iterator p = ProjectNode.modules.begin(); p != ProjectNode.modules.end(); ++ p )
	{
		Module& module = *p->second;
		if ((module.type != Iso) &&
			(module.type != LiveIso) &&
			(module.type != IsoRegTest) &&
			(module.type != LiveIsoRegTest))
		{
			Module& module = *p->second;

			if (module.metadata)
			{
				if (module.metadata->version.length() > 0)
				{
					fprintf ( m_VReportFile, "\t<component>\r\n" );
					fprintf ( m_VReportFile, "\t\t<name>%s</name>\r\n", module.name.c_str () );
					fprintf ( m_VReportFile, "\t\t<base>%s</base>\r\n", module.output->relative_path.c_str () );
					fprintf ( m_VReportFile, "\t\t<version>%s</version>\r\n", module.metadata->version.c_str () );
					fprintf ( m_VReportFile, "\t\t<date>%s</date>\r\n", module.metadata->date.c_str () );
					fprintf ( m_VReportFile, "\t\t<owner>%s</owner>\r\n", module.metadata->owner.c_str () );
					fprintf ( m_VReportFile, "\t</component>\r\n" );
				}
			}
		}
	}

	fprintf ( m_VReportFile, "</components>" );
}


VReportConfiguration::VReportConfiguration ( const std::string &name )
{
	/* nothing to do here */
}
