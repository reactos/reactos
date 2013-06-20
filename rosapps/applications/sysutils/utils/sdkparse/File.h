// File.h
// This file is (C) 2002-2003 Royce Mitchell III and released under the BSD license

#ifndef FILE_H
#define FILE_H

#ifdef WIN32
#  include <io.h>
#elif defined(UNIX)
#  include <sys/stat.h>
#  include <unistd.h>
#endif
#include <string>

class File
{
public:
	File() : _f(0)
	{
	}

	File ( const char* filename, const char* mode ) : _f(0)
	{
		open ( filename, mode );
	}

	~File()
	{
		close();
	}

	bool open ( const char* filename, const char* mode );

	bool seek ( long offset )
	{
		return !fseek ( _f, offset, SEEK_SET );
	}

	int get()
	{
		return fgetc ( _f );
	}

	bool put ( int c )
	{
		return fputc ( c, _f ) != EOF;
	}

	std::string getline ( bool strip_crlf = false );

	// this function searches for the next end-of-line and puts all data it
	// finds until then in the 'line' parameter.
	//
	// call continuously until the function returns false ( no more data )
	bool next_line ( std::string& line, bool strip_crlf );

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

	bool enum_lines ( bool (*callback)(const std::string& line, int line_number, long lparam), long lparam, bool strip_crlf );

	bool read ( void* data, unsigned len )
	{
		return len == fread ( data, 1, len, _f );
	}

	bool write ( const void* data, unsigned len )
	{
		return len == fwrite ( data, 1, len, _f );
	}

	size_t length();

	void close();

	bool isopened()
	{
		return _f != 0;
	}

	bool eof()
	{
		return feof(_f) ? true : false;
	}

	FILE* operator * ()
	{
		return _f;
	}

	static bool LoadIntoString ( std::string& s, const char* filename );
	static bool SaveFromString ( const char* filename, const std::string& s, bool binary );

private:
	File(const File&) {}
	const File& operator = ( const File& ) { return *this; }

	FILE * _f;
};

#endif//FILE_H
