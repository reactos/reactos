#include "test.h"

using std::string;

void InvokeTest::Run()
{
	string projectFilename ( RBUILD_BASE "tests/data/invoke.xml" );
	Project project ( projectFilename );
	ARE_EQUAL(1, project.modules.size());

	Module& module1 = *project.modules[0];
	ARE_EQUAL(1, module1.invocations.size());

	Invoke& invoke1 = *module1.invocations[0];
	ARE_EQUAL(1, invoke1.output.size());

	InvokeFile& file1 = *invoke1.output[0];
	ARE_EQUAL(FixSeparator("dir1/file1.c"), file1.name);
}
