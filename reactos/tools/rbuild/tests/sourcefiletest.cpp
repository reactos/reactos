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
#include "test.h"

using std::string;

bool
SourceFileTest::IsParentOf ( const SourceFile* parent,
	                         const SourceFile* child )
{
	size_t i;
	for ( i = 0; i < child->parents.size (); i++ )
	{
		if ( child->parents[i] != NULL )
		{
			if ( child->parents[i] == parent )
			{
				return true;
			}
		}
	}
	for ( i = 0; i < child->parents.size (); i++ )
	{
		if ( child->parents[i] != NULL )
		{
			if ( IsParentOf ( parent,
			                  child->parents[i] ) )
			{
				return true;
			}
		}
	}
	return false;
}

void
SourceFileTest::IncludeTest ()
{
	const string projectFilename = RBUILD_BASE "tests" SSEP "data" SSEP "automaticdependency_include.xml";
	Configuration configuration;
	Project project ( configuration, projectFilename );
	AutomaticDependency automaticDependency ( project );
	automaticDependency.ParseFiles ();
	ARE_EQUAL( 4, automaticDependency.sourcefile_map.size () );
	const SourceFile* include = automaticDependency.RetrieveFromCache ( RBUILD_BASE "tests" SSEP "data" SSEP "sourcefile_include.h" );
	IS_NOT_NULL( include );
	const SourceFile* includenext = automaticDependency.RetrieveFromCache ( RBUILD_BASE "tests" SSEP "data" SSEP "sourcefile1" SSEP "sourcefile_includenext.h" );
	IS_NOT_NULL( includenext );
}

void
SourceFileTest::FullParseTest ()
{
	const string projectFilename = RBUILD_BASE "tests" SSEP "data" SSEP "automaticdependency.xml";
	Configuration configuration;
	Project project ( configuration, projectFilename );
	AutomaticDependency automaticDependency ( project );
	automaticDependency.ParseFiles ();
	ARE_EQUAL( 5, automaticDependency.sourcefile_map.size () );
	const SourceFile* header1 = automaticDependency.RetrieveFromCache ( RBUILD_BASE "tests" SSEP "data" SSEP "sourcefile1_header1.h" );
	IS_NOT_NULL( header1 );
	const SourceFile* recurse = automaticDependency.RetrieveFromCache ( RBUILD_BASE "tests" SSEP "data" SSEP "sourcefile1_recurse.h" );
	IS_NOT_NULL( recurse );
	IS_TRUE( IsParentOf ( header1,
	                      recurse ) );
	IS_FALSE( IsParentOf ( recurse,
	                       header1 ) );
	
}

void
SourceFileTest::Run ()
{
	IncludeTest ();
	FullParseTest ();
}
