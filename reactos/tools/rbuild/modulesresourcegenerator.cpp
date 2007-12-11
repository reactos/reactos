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

ModulesResourceGenerator::ModulesResourceGenerator ( const Project& project )
	: project ( project )
{
}

ModulesResourceGenerator::~ModulesResourceGenerator ()
{
}

void
ModulesResourceGenerator::Generate ()
{
	for ( size_t i = 0; i < project.modules.size (); i++ )
	{
		Module& module = *project.modules[i];

		if (module.autoResource != NULL)
		{
			WriteResourceFile (module);
		}
	}
}

void
ModulesResourceGenerator::WriteResourceFile ( Module& module )
{
	char* buf;
	char* s;

	buf = (char*) malloc ( 512*1024 );
	if ( buf == NULL )
		throw OutOfMemoryException ();
	
	s = buf;
	s = s + sprintf ( s, "/* Auto generated */\n");
	s = s + sprintf ( s, "\n" );
	s = s + sprintf ( s, "#include <windows.h>\n");
	s = s + sprintf ( s, "#include <commctrl.h>\n");
	s = s + sprintf ( s, "\n" );
	s = s + sprintf ( s, "LANGUAGE LANG_NEUTRAL, SUBLANG_NEUTRAL\n");
	s = s + sprintf ( s, "\n" );

	if (module.metadata)
	{
		s = s + sprintf ( s, "#define REACTOS_STR_FILE_DESCRIPTION  \"%s\\0\"\n" , module.metadata->description.c_str());
	}
	else
	{
		s = s + sprintf ( s, "#define REACTOS_STR_FILE_DESCRIPTION   \"%s\\0\"\n" ,module.name.c_str());
	}

	s = s + sprintf ( s, "#define REACTOS_STR_INTERNAL_NAME     \"%s\\0\"\n" , module.name.c_str());
	s = s + sprintf ( s, "#define REACTOS_STR_ORIGINAL_FILENAME \"%s\\0\"\n" , module.output->name.c_str());
	s = s + sprintf ( s, "\n" );
	s = s + sprintf ( s, "#include <reactos/version.rc>\n");
	s = s + sprintf ( s, "\n" );

	if (module.autoManifest != NULL)
	{
		s = s + sprintf ( s, "1 24 DISCARDABLE \"manifest.xml\"\n");
		s = s + sprintf ( s, "\n" );
	}

	/* Include resources for module localizations */
	for ( size_t i = 0; i < module.localizations.size (); i++ )
	{
		Localization& localization = *module.localizations[i];

		/* If this locale is included in our platform ... */
		const PlatformLanguage* platformLanguage = module.project.LocatePlatformLanguage (localization.isoname);
		if (platformLanguage != NULL)
		{
			std::string langFile = NormalizeFilename(localization.file.relative_path + sSep + localization.file.name);

			s = s + sprintf ( s, "#include \"%s\"" , langFile.c_str() );
			s = s + sprintf ( s, "\n" );
		}
	}

	FileSupportCode::WriteIfChanged ( buf, NormalizeFilename ( Environment::GetIntermediatePath () + sSep + module.output->relative_path + sSep + "auto.rc" ) );

	free ( buf );
}
