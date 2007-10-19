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

void CompilationUnitTest::Run()
{
	string projectFilename ( RBUILD_BASE "tests/data/compilationunit.xml" );
	Configuration configuration;
	Project project ( configuration, projectFilename );
	ARE_EQUAL ( 1, project.modules.size () );

	Module& module1 = *project.modules[0];
	IS_TRUE ( module1.type == BuildTool );

	ARE_EQUAL ( 3, module1.non_if_data.files.size());
	ARE_EQUAL ( "dir1" SSEP "file1.c", module1.non_if_data.files[0]->name );
	ARE_EQUAL ( "dir1" SSEP "file2.c", module1.non_if_data.files[1]->name );
	ARE_EQUAL ( "dir1" SSEP "file3.c", module1.non_if_data.files[2]->name );

	ARE_EQUAL ( 2, module1.non_if_data.compilationUnits.size () );

	CompilationUnit& compilationUnit1 = *module1.non_if_data.compilationUnits[0];
	ARE_EQUAL ( 2, compilationUnit1.files.size () );

	ARE_EQUAL ( "dir1" SSEP "file1.c", compilationUnit1.files[0]->name );
	ARE_EQUAL ( "dir1" SSEP "file2.c", compilationUnit1.files[1]->name );

	CompilationUnit& compilationUnit2 = *module1.non_if_data.compilationUnits[1];
	ARE_EQUAL ( 1, compilationUnit2.files.size () );

	ARE_EQUAL ( "dir1" SSEP "file3.c", compilationUnit2.files[0]->name );
}
