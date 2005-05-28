#include "test.h"

using std::string;

void CDFileTest::Run()
{
	string projectFilename ( RBUILD_BASE "tests/data/cdfile.xml" );
	Project project( projectFilename );
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
