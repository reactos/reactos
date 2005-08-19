// File.cpp
// (C) 2002-2004 Royce Mitchell III
// Dually licensed under BSD & LGPL

#ifdef _MSC_VER
#pragma warning ( disable : 4786 )
#endif//_MSC_VER

#include <stdio.h>
#include <stdarg.h>
#include "File.h"

#ifndef nelem
#define nelem(x) (  sizeof(x) / sizeof(x[0])  )
#endif//nelem

typedef File::filesize_t filesize_t;
typedef File::fileoff_t fileoff_t;


fileoff_t File::seek ( fileoff_t offset )
{
#ifdef WIN32
	if ( _f->_flag & _IOWRT ) // is there pending output?
		fflush ( _f );

	// reset "buffered input" variables
	_f->_cnt = 0;
	_f->_ptr = _f->_base;

	// make sure we're going forward
	if ( _f->_flag & _IORW )
		_f->_flag &= ~(_IOREAD|_IOWRT);

	return _lseeki64 ( _fileno(_f), offset, SEEK_SET );
#else//UNIX
	return lseek64 ( fileno(_f), offset, SEEK_SET );
#endif
}

std::string File::getline ( bool strip_crlf /*= false*/ )
{
	std::string s = "";
	char buf[256];
	for ( ;; )
	{
		*buf = 0;
		fgets ( buf, nelem(buf)-1, _f );
		if ( !*buf )
			break;
		s += buf;
		if ( strchr ( "\r\n", buf[strlen(buf)-1] ) )
			break;
	}
	if ( strip_crlf && s.size() )
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

std::wstring File::wgetline ( bool strip_crlf /*= false*/ )
{
	std::wstring s = L"";
	wchar_t buf[256];
	for ( ;; )
	{
		*buf = 0;
		fgetws ( buf, nelem(buf)-1, _f );
		if ( !*buf )
			break;
		s += buf;
		if ( wcschr ( L"\r\n", buf[wcslen(buf)-1] ) )
			break;
	}
	if ( strip_crlf && s.size() )
	{
		wchar_t* p = wcspbrk ( &s[0], L"\r\n" );
		if ( p )
		{
			*p = L'\0';
			s.resize ( (p-&s[0])/sizeof(wchar_t) );
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

bool File::next_line ( std::wstring& line, bool strip_crlf )
{
	line = wgetline ( strip_crlf );
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

/*bool File::enum_lines ( bool (*callback)(const std::string& line, int line_number, long lparam), long lparam, bool strip_crlf )
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
}*/

filesize_t File::length() const
{
#ifdef WIN32
	return _filelengthi64 ( _fileno(_f) );
#elif defined(UNIX)
	struct stat64 file_stat;
	verify(fstat64(fileno(_f), &file_stat) == 0);
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

void File::printf ( const char* fmt, ... )
{
	va_list arg;
	int done;

	va_start(arg, fmt);
	assert(_f);
	done = vfprintf ( _f, fmt, arg );
	va_end(arg);
}

/*static*/ bool File::LoadIntoString ( std::string& s, const std::string& filename )
{
	File in ( filename, "rb" );
	if ( !in.isopened() )
		return false;
	filesize_t flen = in.length();
	size_t len = size_t(flen);
	if ( len != flen )
		return false; // file too big...
	s.resize ( len + 1 );
	if ( !in.read ( &s[0], len ) )
		return false;
	s[len] = '\0';
	s.resize ( len );
	return true;
}

/*static*/ bool File::LoadIntoString ( std::string& s, const std::wstring& filename )
{
	File in ( filename, L"rb" );
	if ( !in.isopened() )
		return false;
	filesize_t flen = in.length();
	size_t len = size_t(flen);
	if ( len != flen )
		return false; // file too big...
	s.resize ( len + 1 );
	if ( !in.read ( &s[0], len ) )
		return false;
	s[len] = '\0';
	s.resize ( len );
	return true;
}

/*static*/ bool File::SaveFromString ( const std::string& filename, const std::string& s, bool binary )
{
	File out ( filename, binary ? "wb" : "w" );
	if ( !out.isopened() )
		return false;
	out.write ( s.c_str(), s.size() );
	return true;
}

/*static*/ bool File::SaveFromString ( const std::wstring& filename, const std::string& s, bool binary )
{
	File out ( filename, binary ? L"wb" : L"w" );
	if ( !out.isopened() )
		return false;
	out.write ( s.c_str(), s.size() );
	return true;
}

/*static*/ bool File::SaveFromBuffer ( const std::string& filename, const char* buf, size_t buflen, bool binary )
{
	File out ( filename, binary ? "wb" : "w" );
	if ( !out.isopened() )
		return false;
	out.write ( buf, buflen );
	return true;
}

/*static*/ bool File::SaveFromBuffer ( const std::wstring& filename, const char* buf, size_t buflen, bool binary )
{
	File out ( filename, binary ? L"wb" : L"w" );
	if ( !out.isopened() )
		return false;
	out.write ( buf, buflen );
	return true;
}

/*static*/ std::string File::TempFileName ( const std::string& prefix )
{
#ifdef WIN32
	std::string s ( _tempnam ( ".", prefix.c_str() ) );
	return s;
#else
	// FIXME
	return string("");
#endif
}

/*static*/ std::wstring File::TempFileName ( const std::wstring& prefix )
{
#ifdef WIN32
	std::wstring s ( _wtempnam ( L".", prefix.c_str() ) );
	return s;
#else
	// FIXME
	return std::wstring(L"");
#endif
}
