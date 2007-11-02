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


CreditsGenerator::CreditsGenerator ( const Project& project )
	: project ( project )
{
}

CreditsGenerator::~CreditsGenerator ()
{
}

void
CreditsGenerator::Generate ()
{
	char* buf;
	char* s;

	buf = (char*) malloc ( 512*1024 );
	if ( buf == NULL )
		throw OutOfMemoryException ();
	
	s = buf;
	s = s + sprintf ( s, "ReactOS is available thanks to the work of:\n\n");

    for ( size_t i = 0; i < project.contributors.size (); i++ )
	{
        Contributor& contributor = *project.contributors[i];

	    s = s + sprintf ( s, "\t%s %s (%s)\n" , 
            contributor.firstName.c_str() , 
            contributor.lastName.c_str() , 
            contributor.alias.c_str());
        
        s = s + sprintf ( s, "\t\t%s\n" , contributor.mail.c_str());
        
        if (strlen(contributor.city.c_str()) > 0 &&
            strlen(contributor.country.c_str()) > 0)
        {
            s = s + sprintf ( s, "\t\t%s,%s\n\n" , 
                contributor.city.c_str() , 
                contributor.country.c_str());
        }
	}


	FileSupportCode::WriteIfChanged ( buf, NormalizeFilename ( Environment::GetIntermediatePath () + sSep + "CREDITS" ) );

	free ( buf );
}
