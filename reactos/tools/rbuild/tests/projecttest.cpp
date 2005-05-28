#include "test.h"

using std::string;

void ProjectTest::Run()
{
	string projectFilename ( RBUILD_BASE "tests/data/project.xml" );
	Project project( projectFilename );
	ARE_EQUAL(2, project.modules.size());
}
