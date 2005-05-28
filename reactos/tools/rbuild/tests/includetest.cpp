#include "test.h"

using std::string;

void IncludeTest::Run()
{
	string projectFilename ( RBUILD_BASE "tests/data/include.xml" );
	Project project ( projectFilename );
	ARE_EQUAL(1, project.non_if_data.includes.size());
	Include& include1 = *project.non_if_data.includes[0];
	ARE_EQUAL("include1", include1.directory);

	ARE_EQUAL(2, project.modules.size());
	Module& module1 = *project.modules[0];
	Module& module2 = *project.modules[1];

	ARE_EQUAL(1, module1.non_if_data.includes.size());
	Include& include2 = *module1.non_if_data.includes[0];
	ARE_EQUAL("include2", include2.directory);

	ARE_EQUAL(1, module2.non_if_data.includes.size());
	Include& include3 = *module2.non_if_data.includes[0];
	ARE_EQUAL(FixSeparator("dir1/include3"), include3.directory);
}
