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

void InvokeTest::Run()
{
	string projectFilename ( RBUILD_BASE "tests/data/invoke.xml" );
	Configuration configuration;
	Project project ( configuration, projectFilename );
	ARE_EQUAL(1, project.modules.size());

	Module& module1 = *project.modules[0];
	ARE_EQUAL(1, module1.invocations.size());

	Invoke& invoke1 = *module1.invocations[0];
	ARE_EQUAL(1, invoke1.output.size());

	InvokeFile& file1 = *invoke1.output[0];
	ARE_EQUAL(FixSeparator("dir1/file1.c"), file1.name);
}
