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
#ifndef __EXCEPTION_H
#define __EXCEPTION_H

#include "pch.h"
#include "xml.h"

class Exception
{
public:
	Exception ( const std::string& message );
	Exception ( const char* format,
	            ...);
	const std::string& operator *() { return _e; }

protected:
	Exception ();
	void SetMessage ( const char* message, ... );
	void SetMessageV ( const char* message, va_list args );

private:
	std::string _e;
};


class MissingArgumentException : public Exception
{
public:
	MissingArgumentException ( const std::string& argument );
};


class InvalidOperationException : public Exception
{
public:
	InvalidOperationException ( const char* filename,
	                            const int linenumber);
	InvalidOperationException ( const char* filename,
	                            const int linenumber,
	                            const char* message,
	                            ... );
};


class OutOfMemoryException : public Exception
{
public:
	OutOfMemoryException ();
};


class FileNotFoundException : public Exception
{
public:
	FileNotFoundException ( const std::string& filename );
	std::string Filename;
};


class AccessDeniedException : public Exception
{
public:
	AccessDeniedException ( const std::string& filename );
	std::string Filename;
};

class InvalidDateException : public Exception
{
public:
	InvalidDateException ( const std::string& filename );
	std::string Filename;
};

class RequiredAttributeNotFoundException : public XMLInvalidBuildFileException
{
public:
	RequiredAttributeNotFoundException ( const std::string& location,
	                                     const std::string& attributeName,
	                                     const std::string& elementName );
};


class InvalidAttributeValueException : public XMLInvalidBuildFileException
{
public:
	InvalidAttributeValueException ( const std::string& location,
	                                 const std::string& name,
	                                 const std::string& value );
};


class BackendNameConflictException : public Exception
{
public:
	BackendNameConflictException ( const std::string& name );
};


class UnknownBackendException : public Exception
{
public:
	UnknownBackendException ( const std::string& name );
};

class UnknownModuleTypeException : public XMLInvalidBuildFileException
{
public:
	UnknownModuleTypeException ( const std::string& location,
	                             int moduletype );
};


class InvocationFailedException : public Exception
{
public:
	InvocationFailedException ( const std::string& command,
	                            int exitcode );
	std::string Command;
	int ExitCode;
};


class UnsupportedBuildToolException : public Exception
{
public:
	UnsupportedBuildToolException ( const std::string& buildtool,
	                                const std::string& version );
	std::string BuildTool;
	std::string Version;
};

#endif /* __EXCEPTION_H */
