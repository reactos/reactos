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

#include "pch.h"

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

class Path
{
	std::vector<std::string> path;
public:
	Path(); // initializes path to getcwd();
	Path ( const Path& cwd, const std::string& filename );
	std::string Fixup ( const std::string& filename, bool include_filename ) const;

	std::string RelativeFromWorkingDirectory ();
	static std::string RelativeFromWorkingDirectory ( const std::string& path );

	static void Split ( std::vector<std::string>& out,
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

	XMLInclude ( XMLElement* e_, const Path& path_, const std::string topIncludeFilename_ )
		: e ( e_ ), path ( path_ ), topIncludeFilename ( topIncludeFilename_ )
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
	bool get_token(std::string& token);
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

	XMLElement ( XMLFile* xmlFile,
	             const std::string& location );
	~XMLElement();
	bool Parse(const std::string& token,
	           bool& end_tag);
	void AddSubElement ( XMLElement* e );
	XMLAttribute* GetAttribute ( const std::string& attribute,
	                             bool required);
	const XMLAttribute* GetAttribute ( const std::string& attribute,
	                                   bool required) const;
};

XMLElement*
XMLLoadFile ( const std::string& filename,
	          const Path& path,
	          XMLIncludes& includes );

#endif // XML_H
