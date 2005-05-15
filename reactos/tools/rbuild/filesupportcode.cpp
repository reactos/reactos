#include "pch.h"
#include <assert.h>

#include "rbuild.h"

using std::string;

/* static */ void
FileSupportCode::WriteIfChanged ( char* outbuf,
                                  string filename )
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
			throw AccessDeniedException ( filename );
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
