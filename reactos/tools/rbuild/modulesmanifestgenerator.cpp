/*
 * Copyright (C) 2007 Marc Piulachs
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
#include <assert.h>
#include "rbuild.h"

using std::string;
using std::vector;

ModulesManifestGenerator::ModulesManifestGenerator ( const Project& project )
	: project ( project )
{
}

ModulesManifestGenerator::~ModulesManifestGenerator ()
{
}

void
ModulesManifestGenerator::Generate ()
{
	for ( size_t i = 0; i < project.modules.size (); i++ )
	{
		Module& module = *project.modules[i];

		if (module.autoManifest != NULL)
		{
			WriteManifestFile (module);
		}
	}
}

void
ModulesManifestGenerator::WriteManifestFile ( Module& module )
{
	char* buf;
	char* s;

	buf = (char*) malloc ( 512*1024 );
	if ( buf == NULL )
		throw OutOfMemoryException ();
	
	s = buf;
	s = s + sprintf ( s, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>");
	s = s + sprintf ( s, "<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">");
	s = s + sprintf ( s, " <assemblyIdentity");
	s = s + sprintf ( s, "  version=\"1.0.0.0\"");
	s = s + sprintf ( s, "  processorArchitecture=\"x86\"");
	s = s + sprintf ( s, "  name=\"ReactOS.System.Module\"");
	s = s + sprintf ( s, "  type=\"win32\"");
	s = s + sprintf ( s, " />");
	
	if (module.metadata != NULL)
	{
		s = s + sprintf ( s, " <description>%s</description>" , module.metadata->description.c_str());
	}
	else
	{
		s = s + sprintf ( s, " <description>%s<description>" , module.name.c_str());
	}
	
	s = s + sprintf ( s, " <dependency>");
	s = s + sprintf ( s, "  <dependentAssembly>");
 	s = s + sprintf ( s, "  <assemblyIdentity");
 	s = s + sprintf ( s, "   type=\"win32\"");
 	s = s + sprintf ( s, "   name=\"Microsoft.Windows.Common-Controls\"");
 	s = s + sprintf ( s, "   version=\"6.0.0.0\"");
 	s = s + sprintf ( s, "   processorArchitecture=\"x86\"");
 	s = s + sprintf ( s, "   publicKeyToken=\"6595b64144ccf1df\"");
 	s = s + sprintf ( s, "   language=\"*\"");
	s = s + sprintf ( s, "   />");
	s = s + sprintf ( s, "  </dependentAssembly>");
	s = s + sprintf ( s, " </dependency>");
	s = s + sprintf ( s, "</assembly>");

	FileSupportCode::WriteIfChanged ( buf, NormalizeFilename ( Environment::GetIntermediatePath () + sSep + module.output->relative_path + sSep + "manifest.xml" ) );

	free ( buf );
}
