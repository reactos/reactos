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
#ifndef XML_H
#define XML_H

#include <string>
#include <vector>
#include <stdarg.h>

class XMLElement;

extern std::string working_directory;

void
InitWorkingDirectory();

#ifdef _MSC_VER
unsigned __int64
#else
unsigned long long
#endif
filelen ( FILE* f );

class XMLException
{
public:
	XMLException ( const std::string& location, const char* format, ... );
	const std::string& operator *() { return _e; }

protected:
	XMLException() {}
	void SetExceptionV ( const std::string& location, const char* format, va_list args );
	void SetException ( const std::string& location, const char* format, ... );

private:
	std::string _e;
};

class XMLSyntaxErrorException : public XMLException
{
public:
	XMLSyntaxErrorException (
		const std::string& location,
		const char* format, ... )
	{
		va_list args;
		va_start ( args, format );
		SetExceptionV ( location, format, args );
		va_end ( args );
	}
};

class XMLRequiredAttributeNotFoundException : public XMLException
{
public:
	XMLRequiredAttributeNotFoundException (
		const std::string& location,
		const std::string& attributeName,
		const std::string& elementName )
	{
		SetException ( location, "Required attribute '%s' not found in element '%s'",
			attributeName.c_str(),
			elementName.c_str() );
	}
};

class XMLInvalidBuildFileException : public XMLException
{
public:
	XMLInvalidBuildFileException (
		const std::string& location,
		const char* format,
		... )
	{
		va_list args;
		va_start ( args, format );
		SetExceptionV ( location, format, args );
		va_end ( args );
	}
};

class XMLFileNotFoundException : public XMLException
{
public:
	XMLFileNotFoundException (
		const std::string& location,
		const std::string& filename )
	{
		SetException ( location, "Can't open file '%s'", filename.c_str() );
	}
};

class Path
{
	std::vector<std::string> path;
public:
	Path(); // initializes path to getcwd();
	Path ( const Path& cwd, const std::string& filename );
	std::string Fixup ( const std::string& filename, bool include_filename ) const;

	std::string RelativeFromWorkingDirectory ();
	static std::string RelativeFromWorkingDirectory ( const std::string& path );
	static std::string RelativeFromDirectory ( const std::string& path, const std::string& base_directory);

	static void Split (
		std::vector<std::string>& out,
		const std::string& path,
		bool include_last );
};

class XMLInclude
{
public:
	XMLElement *e;
	Path path;
	std::string topIncludeFilename;
	bool fileExists;

	XMLInclude (
		XMLElement* e_,
		const Path& path_,
		const std::string topIncludeFilename_ )
		: e ( e_ ),
		path ( path_ ),
		topIncludeFilename ( topIncludeFilename_ )
	{
	}
};

class XMLIncludes : public std::vector<XMLInclude*>
{
public:
	~XMLIncludes();
};

class XMLFile
{
	friend class XMLElement;
public:
	XMLFile();
	void close();
	bool open(const std::string& filename);
	void next_token();
	bool next_is_text();
	bool more_tokens();
	bool get_token ( std::string& token );
	bool get_token ( std::string& token, std::string& location );
	const std::string& filename() { return _filename; }
	std::string Location() const;

private:
	std::string _buf, _filename;

	const char *_p, *_end;
};


class XMLAttribute
{
public:
	std::string name;
	std::string value;

	XMLAttribute();
	XMLAttribute ( const std::string& name_, const std::string& value_ );
	XMLAttribute ( const XMLAttribute& );
	XMLAttribute& operator = ( const XMLAttribute& );
};


class XMLElement
{
public:
	XMLFile* xmlFile;
	std::string location;
	std::string name;
	std::vector<XMLAttribute*> attributes;
	XMLElement* parentElement;
	std::vector<XMLElement*> subElements;
	std::string value;

	XMLElement (
		XMLFile* xmlFile,
		const std::string& location );

	~XMLElement();

	bool Parse (
		const std::string& token,
		bool& end_tag);

	void AddSubElement ( XMLElement* e );

	XMLAttribute* GetAttribute (
		const std::string& attribute,
		bool required);

	const XMLAttribute* GetAttribute (
		const std::string& attribute,
		bool required ) const;

	int FindElement (
		const std::string& type,
		int prev = -1 ) const;

	int GetElements (
		const std::string& type,
		std::vector<XMLElement*>& v );

	int GetElements (
		const std::string& type,
		std::vector<const XMLElement*>& v ) const;
};

XMLElement*
XMLLoadFile (
	const std::string& filename,
	const Path& path,
	XMLIncludes& includes );

XMLElement*
XMLLoadFile ( const std::string& filename );

#endif // XML_H
