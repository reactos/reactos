#include "test.h"

using std::string;

void LinkerFlagTest::Run()
{
	string projectFilename ( RBUILD_BASE "tests/data/linkerflag.xml" );
	Project project ( projectFilename );
	ARE_EQUAL(1, project.linkerFlags.size());
	LinkerFlag& linkerFlag1 = *project.linkerFlags[0];
	ARE_EQUAL("-lgcc1", linkerFlag1.flag);

	ARE_EQUAL(1, project.modules.size());
	Module& module1 = *project.modules[0];

	ARE_EQUAL(1, module1.linkerFlags.size());
	LinkerFlag& linkerFlag2 = *module1.linkerFlags[0];
	ARE_EQUAL("-lgcc2", linkerFlag2.flag);
}
