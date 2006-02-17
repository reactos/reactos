// File.cpp
// This file is (C) 2002-2003 Royce Mitchell III and released under the BSD license

#ifdef _MSC_VER
#pragma warning ( disable : 4786 )
#endif//_MSC_VER

#include "File.h"

bool File::open ( const char* filename, const char* mode )
{
	close();
	_f = fopen ( filename, mode );
	return _f != 0;
}

std::string File::getline ( bool strip_crlf /*= false*/ )
{
	std::string s = "";
	char buf[256];
	for ( ;; )
	{
		*buf = 0;
		fgets ( buf, sizeof(buf)-1, _f );
		if ( !*buf )
			break;
		s += buf;
		if ( strchr ( "\r\n", buf[strlen(buf)-1] ) )
			break;
	}
	if ( strip_crlf )
	{
		char* p = strpbrk ( &s[0], "\r\n" );
		if ( p )
		{
			*p = '\0';
			s.resize ( p-&s[0] );
		}
	}
	return s;
}

// this function searches for the next end-of-line and puts all data it
// finds until then in the 'line' parameter.
//
// call continuously until the function returns false ( no more data )
bool File::next_line ( std::string& line, bool strip_crlf )
{
	line = getline(strip_crlf);
	// indicate that we're done *if*:
	// 1) there's no more data, *and*
	// 2) we're at the end of the file
	return line.size()>0 || !eof();
}

/*
example usage:

bool mycallback ( const std::string& line, int line_number, long lparam )
{
	std::cout << line << std::endl;
	return true; // continue enumeration
}

File f ( "file.txt", "rb" ); // open file for binary read-only ( i.e. "rb" )
f.enum_lines ( mycallback, 0, true );
*/

bool File::enum_lines ( bool (*callback)(const std::string& line, int line_number, long lparam), long lparam, bool strip_crlf )
{
	int line_number = 0;
	for ( ;; )
	{
		std::string s = getline(strip_crlf);
		line_number++;
		if ( !s.size() )
		{
			if ( eof() )
				return true;
			else
				continue;
		}
		if ( !(*callback) ( s, line_number, lparam ) )
			return false;
	}
}

size_t File::length()
{
#ifdef WIN32
	return _filelength ( _fileno(_f) );
#elif defined(UNIX)
	struct stat file_stat;
	verify(fstat(fileno(_f), &file_stat) == 0);
	return file_stat.st_size;
#endif
}

void File::close()
{
	if ( _f )
	{
		fclose(_f);
		_f = 0;
	}
}

/*static*/ bool File::LoadIntoString ( std::string& s, const char* filename )
{
	File in ( filename, "rb" );
	if ( !in.isopened() )
		return false;
	size_t len = in.length();
	s.resize ( len + 1 );
	if ( !in.read ( &s[0], len ) )
		return false;
	s[len] = '\0';
	s.resize ( len );
	return true;
}

/*static*/ bool File::SaveFromString ( const char* filename, const std::string& s, bool binary )
{
	File out ( filename, binary ? "wb" : "w" );
	if ( !out.isopened() )
		return false;
	out.write ( s.c_str(), s.size() );
	return true;
}
