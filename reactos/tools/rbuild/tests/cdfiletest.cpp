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

void CDFileTest::Run()
{
	string projectFilename ( RBUILD_BASE "tests/data/cdfile.xml" );
	Configuration configuration;
	Project project ( configuration, projectFilename );
	ARE_EQUAL ( 3, project.cdfiles.size () );

	CDFile& cdfile1 = *project.cdfiles[0];
	ARE_EQUAL("dir1", cdfile1.base);
	ARE_EQUAL("ReadMe1.txt", cdfile1.nameoncd);

	CDFile& cdfile2 = *project.cdfiles[1];
	ARE_EQUAL("dir2", cdfile2.base);
	ARE_EQUAL("readme2.txt", cdfile2.nameoncd);

	CDFile& cdfile3 = *project.cdfiles[2];
	//ARE_EQUAL("", cdfile3.base);
	ARE_EQUAL("readme3.txt", cdfile3.nameoncd);
}
