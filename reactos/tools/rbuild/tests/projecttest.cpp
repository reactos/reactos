#include "test.h"

void ProjectTest::Run()
{
	string projectFilename ( "tests/data/project.xml" );
	Project* project = new Project( projectFilename );
	ARE_EQUAL(2, project->modules.size());
}
