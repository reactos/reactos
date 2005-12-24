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

void
IfTest::TestProjectIf ( Project& project )
{
	ARE_EQUAL ( 1, project.non_if_data.ifs.size () );
	If& if1 = *project.non_if_data.ifs[0];
	ARE_EQUAL ( "VAR1", if1.property );
	ARE_EQUAL ( "value1", if1.value );

	ARE_EQUAL ( 1, if1.data.compilerFlags.size () );
	CompilerFlag& compilerFlag1 = *if1.data.compilerFlags[0];
	ARE_EQUAL( "compilerflag1", compilerFlag1.flag );
}

void
IfTest::TestModuleIf ( Project& project )
{
	ARE_EQUAL ( 1, project.modules.size () );
	Module& module1 = *project.modules[0];

	ARE_EQUAL ( 1, module1.non_if_data.ifs.size () );
	If& if1 = *module1.non_if_data.ifs[0];
	ARE_EQUAL ( "VAR2", if1.property );
	ARE_EQUAL ( "value2", if1.value );

	ARE_EQUAL ( 1, if1.data.files.size () );
	File& file1 = *if1.data.files[0];
	ARE_EQUAL( SSEP "file1.c", file1.name );

	ARE_EQUAL ( 1, module1.non_if_data.files.size () );
	File& file2 = *module1.non_if_data.files[0];
	ARE_EQUAL( SSEP "file2.c", file2.name );

	ARE_EQUAL ( 1, if1.data.compilerFlags.size () );
	CompilerFlag& compilerFlag2 = *if1.data.compilerFlags[0];
	ARE_EQUAL( "compilerflag2", compilerFlag2.flag );
}

void
IfTest::Run ()
{
	string projectFilename ( RBUILD_BASE "tests/data/if.xml" );
	Configuration configuration;
	Project project ( configuration, projectFilename );

	TestProjectIf ( project );
	TestModuleIf ( project );
}
