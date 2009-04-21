/*
 * Copyright (C) 2009 KJK::Hyperion
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

#include <locale>

void
CompilerDirective::SetCompiler ( CompilerType compiler )
{
	assert ( compiler < CompilerTypesCount );
	compilersSet.set ( compiler );
}

void
CompilerDirective::UnsetCompiler ( CompilerType compiler )
{
	assert ( compiler < CompilerTypesCount );
	compilersSet.reset ( compiler );
}

void
CompilerDirective::SetAllCompilers ()
{
	compilersSet.set ();
}

void
CompilerDirective::UnsetAllCompilers ()
{
	compilersSet.reset ();
}

bool
CompilerDirective::IsCompilerSet ( CompilerType compiler ) const
{
	assert ( compiler < CompilerTypesCount );
	return compilersSet.test ( compiler );
}

template < std::size_t N >
static
bool
CompareStringElement ( const std::string::const_iterator& begin, const std::string::const_iterator& end, const char (& compareTo)[(N)] )
{
	return !std::use_facet < std::collate < char > > ( std::locale::classic () )
		.compare ( &(*begin),
				   &(*(end - 1)),
				   &compareTo[0],
				   &compareTo[(N) - 2] );
}

void
CompilerDirective::ParseCompilers ( const XMLElement& node, const std::string& defaultValue )
{
	const XMLAttribute* att = node.GetAttribute ( "compiler", false );
	const std::string& value = att ? att->value : defaultValue;

	if ( value == "*" )
		SetAllCompilers ();
	else
	{
		UnsetAllCompilers ();

		std::string::const_iterator beginString;

		beginString = value.begin();

		do
		{
			for ( ; beginString != value.end(); ++ beginString )
			{
				if ( *beginString != ',' )
					break;
			}

			if ( beginString == value.end() )
				break;

			std::string::const_iterator endString = beginString;

			for ( ; endString != value.end(); ++ endString )
			{
				if ( *endString == ',' )
					break;
			}

			assert ( endString != beginString );

			CompilerType compiler;

			if ( CompareStringElement ( beginString, endString, "cc" ) )
				compiler = CompilerTypeCC;
			else if ( CompareStringElement ( beginString, endString, "cxx" ) )
				compiler = CompilerTypeCXX;
			else if ( CompareStringElement ( beginString, endString, "cpp" ) )
				compiler = CompilerTypeCPP;
			else if ( CompareStringElement ( beginString, endString, "as" ) )
				compiler = CompilerTypeAS;
			else if ( CompareStringElement ( beginString, endString, "midl" ) )
				compiler = CompilerTypeMIDL;
			else if ( CompareStringElement ( beginString, endString, "rc" ) )
				compiler = CompilerTypeRC;
			else if ( CompareStringElement ( beginString, endString, "nasm" ) )
				compiler = CompilerTypeNASM;
			else
			{
				throw InvalidAttributeValueException (
					node.location,
					"compiler",
					value );
			}

			SetCompiler ( compiler );

			beginString = endString;
		}
		while ( beginString != value.end() );

		if ( !compilersSet.any() )
		{
			throw InvalidAttributeValueException (
				node.location,
				"compiler",
				value );
		}
	}
}
