// File.h
// (C) 2002-2004 Royce Mitchell III
// Dually licensed under BSD & LGPL

#ifndef FILE_H
#define FILE_H

#ifdef WIN32
#  include <stdio.h> // fgetc
#  include <io.h>
#elif defined(UNIX)
#  include <sys/stat.h>
#  include <unistd.h>
#endif
#include <assert.h>
#include <string>

class File
{
public:
#ifdef WIN32
	typedef __int64 fileoff_t;
	typedef unsigned __int64 filesize_t;
#else//_MSC_VER
	typedef __off64_t fileoff_t;
	typedef __size64_t filesize_t;
#endif//_MSC_VER

	File() : _f(0)
	{
	}

	File ( const std::string& filename, const char* mode ) : _f(0)
	{
		open ( filename, mode );
	}

	File ( const std::wstring& filename, const wchar_t* mode ) : _f(0)
	{
		open ( filename, mode );
	}

	File ( const char* filename, const char* mode ) : _f(0)
	{
		open ( filename, mode );
	}

	File ( const wchar_t* filename, const wchar_t* mode ) : _f(0)
	{
		open ( filename, mode );
	}

	~File()
	{
		close();
	}

	bool open ( const std::string& filename, const char* mode )
	{
		assert ( !_f );
		return ( _f = fopen ( filename.c_str(), mode ) ) != 0;
	}

	bool open ( const std::wstring& filename, const wchar_t* mode )
	{
		assert ( !_f );
		return ( _f = _wfopen ( filename.c_str(), mode ) ) != 0;
	}

	bool open ( const char* filename, const char* mode )
	{
		assert ( !_f );
		return ( _f = fopen ( filename, mode ) ) != 0;
	}

	bool open ( const wchar_t* filename, const wchar_t* mode )
	{
		assert ( !_f );
		return ( _f = _wfopen ( filename, mode ) ) != 0;
	}

	fileoff_t seek ( fileoff_t offset );

	int get()
	{
		return fgetc ( _f );
	}

	bool put ( int c )
	{
		return fputc ( c, _f ) != EOF;
	}

	std::string getline ( bool strip_crlf = false );
	std::wstring wgetline ( bool strip_crlf = false );

	// this function searches for the next end-of-line and puts all data it
	// finds until then in the 'line' parameter.
	//
	// call continuously until the function returns false ( no more data )
	bool next_line ( std::string& line, bool strip_crlf );
	bool next_line ( std::wstring& line, bool strip_crlf );

	bool read ( void* data, unsigned len )
	{
		return len == fread ( data, 1, len, _f );
	}

	bool write ( const void* data, unsigned len )
	{
		return len == fwrite ( data, 1, len, _f );
	}

	bool write ( const std::string& data )
	{
		return data.length() == fwrite ( data.c_str(), 1, data.length(), _f );
	}

	bool write ( const std::wstring& data )
	{
		return data.length() == fwrite ( data.c_str(), sizeof(data[0]), data.length(), _f );
	}

	filesize_t length() const;

	void close();

	bool isopened() const
	{
		return _f != 0;
	}

	bool is_open() const
	{
		return _f != 0;
	}

	bool eof() const
	{
		return feof(_f) ? true : false;
	}

	FILE* operator * ()
	{
		return _f;
	}

	operator FILE*()
	{
		return _f;
	}

	void printf ( const char* fmt, ... );
	void printf ( const wchar_t* fmt, ... );

	static bool LoadIntoString ( std::string& s, const std::string& filename );
	static bool LoadIntoString ( std::string& s, const std::wstring& filename );
	static bool SaveFromString ( const std::string& filename, const std::string& s, bool binary );
	static bool SaveFromString ( const std::wstring& filename, const std::string& s, bool binary );
	static bool SaveFromBuffer ( const std::string& filename, const char* buf, size_t buflen, bool binary );
	static bool SaveFromBuffer ( const std::wstring& filename, const char* buf, size_t buflen, bool binary );

	static std::string TempFileName ( const std::string& prefix );
	static std::wstring TempFileName ( const std::wstring& prefix );

private:
	File(const File&) {}
	const File& operator = ( const File& ) { return *this; }

	FILE * _f;
};

#endif//FILE_H
