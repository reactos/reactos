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
#include "rbuild.h"

using std::string;

Exception::Exception ()
{
}

Exception::Exception ( const string& message )
{
	_e = message;
}

Exception::Exception ( const char* format, ...)
{
	va_list args;
	va_start ( args, format);
	_e = ssvprintf ( format, args);
	va_end ( args );
}

void Exception::SetMessage ( const char* format, ...)
{
	va_list args;
	va_start ( args, format);
	_e = ssvprintf ( format, args);
	va_end ( args );
}

void Exception::SetMessageV ( const char* message, va_list args )
{
	_e = ssvprintf ( message, args);
}


MissingArgumentException::MissingArgumentException ( const string& argument )
	: Exception ( "Argument '%s' is missing",
	              argument.c_str () )
{
}


OutOfMemoryException::OutOfMemoryException ()
	: Exception ( "Out of memory" )
{
}


InvalidOperationException::InvalidOperationException (
	const char* filename,
	const int linenumber )
	: Exception ( "%s:%d", filename, linenumber )
{
}

InvalidDateException::InvalidDateException ( const string& filename)
	: Exception ( "File '%s' has an invalid date.",
	             filename.c_str() )
{
	Filename = filename;
}

InvalidOperationException::InvalidOperationException (
	const char* filename,
	const int linenumber,
	const char* message,
	... )
{
	string errorMessage;
	va_list args;
	va_start ( args, message );
	errorMessage = ssvprintf ( message, args );
	va_end ( args );
	SetMessage (
		"%s:%d %s",
		filename,
		linenumber,
		errorMessage.c_str () );
}


FileNotFoundException::FileNotFoundException ( const string& filename )
	: Exception ( "File '%s' not found.",
	              filename.c_str() )
{
	Filename = filename;
}


AccessDeniedException::AccessDeniedException ( const string& filename)
	: Exception ( "Access denied to file or directory '%s'.",
	             filename.c_str() )
{
	Filename = filename;
}


RequiredAttributeNotFoundException::RequiredAttributeNotFoundException (
	const string& location,
	const string& attributeName,
	const string& elementName )
	: XMLInvalidBuildFileException (
		location,
		"Required attribute '%s' not found on '%s'.",
		attributeName.c_str (),
		elementName.c_str ())
{
}

InvalidAttributeValueException::InvalidAttributeValueException (
	const string& location,
	const string& name,
	const string& value )
	: XMLInvalidBuildFileException (
		location,
		"Attribute '%s' has an invalid value '%s'.",
		name.c_str (),
		value.c_str () )
{
	
}

BackendNameConflictException::BackendNameConflictException ( const string& name )
	: Exception ( "Backend name conflict: '%s'",
	             name.c_str() )
{
}


UnknownBackendException::UnknownBackendException ( const string& name )
	: Exception ( "Unknown Backend requested: '%s'",
	              name.c_str() )
{
}


UnknownModuleTypeException::UnknownModuleTypeException ( const string& location,
                                                         int moduletype )
	: XMLInvalidBuildFileException (
		location,
		"module type requested: %i",
		moduletype )
{
}


InvocationFailedException::InvocationFailedException ( const std::string& command,
                                                       int exitcode )
	: Exception ( "Failed to execute '%s' (exit code %d)",
	              command.c_str (),
	              exitcode )
{
	Command = command;
	ExitCode = exitcode;
}


UnsupportedBuildToolException::UnsupportedBuildToolException ( const std::string& buildTool,
                                                               const std::string& version )
	: Exception ( "Build tool '%s' with version '%s' is unsupported. Please upgrade your build tool.",
	              buildTool.c_str (),
	              version.c_str () )
{
	BuildTool = buildTool;
	Version  = version;
}
