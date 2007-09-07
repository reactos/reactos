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

/* static */ void
FileSupportCode::WriteIfChanged ( char* outbuf,
                                  const string& filename,
                                  bool ignoreError )
{
	FILE* out;
	unsigned int end;
	char* cmpbuf;
	unsigned int stat;
	
	out = fopen ( filename.c_str (), "rb" );
	if ( out == NULL )
	{
		out = fopen ( filename.c_str (), "wb" );
		if ( out == NULL )
		{
			if ( ignoreError )
				return;
			throw AccessDeniedException ( filename );
		}
		fputs ( outbuf, out );
		fclose ( out );
		return;
	}
	
	fseek ( out, 0, SEEK_END );
	end = ftell ( out );
	cmpbuf = (char*) malloc ( end );
	if ( cmpbuf == NULL )
	{
		fclose ( out );
		throw OutOfMemoryException ();
	}
	
	fseek ( out, 0, SEEK_SET );
	stat = fread ( cmpbuf, 1, end, out );
	if ( stat != end )
	{
		free ( cmpbuf );
		fclose ( out );
		throw AccessDeniedException ( filename );
	}
	if ( end == strlen ( outbuf ) && memcmp ( cmpbuf, outbuf, end ) == 0 )
	{
		free ( cmpbuf );
		fclose ( out );
		return;
	}
	
	free ( cmpbuf );
	fclose ( out );
	out = fopen ( filename.c_str (), "wb" );
	if ( out == NULL )
	{
		throw AccessDeniedException ( filename );
	}
	
	stat = fwrite ( outbuf, 1, strlen ( outbuf ), out);
	if ( strlen ( outbuf ) != stat )
	{
		fclose ( out );
		throw AccessDeniedException ( filename );
	}

	fclose ( out );
}
