#include "test.h"

using std::string;

void IncludeTest::Run()
{
	string projectFilename ( "tests/data/include.xml" );
	Project project ( projectFilename );
	ARE_EQUAL(1, project.includes.size());
	Include& include1 = *project.includes[0];
	ARE_EQUAL("include1", include1.directory);

	ARE_EQUAL(1, project.modules.size());
	Module& module1 = *project.modules[0];

	ARE_EQUAL(1, module1.includes.size());
	Include& include2 = *module1.includes[0];
	ARE_EQUAL("include2", include2.directory);
}
