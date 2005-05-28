#include "test.h"

using std::string;

void DefineTest::Run()
{
	string projectFilename ( RBUILD_BASE "tests/data/define.xml" );
	Project project ( projectFilename );
	ARE_EQUAL(1, project.non_if_data.defines.size());
	Define& define1 = *project.non_if_data.defines[0];
	ARE_EQUAL("define1", define1.name);
	ARE_EQUAL("value1", define1.value);

	ARE_EQUAL(1, project.modules.size());
	Module& module1 = *project.modules[0];

	ARE_EQUAL(1, module1.non_if_data.defines.size());
	Define& define2 = *module1.non_if_data.defines[0];
	ARE_EQUAL("define2", define2.name);
	ARE_EQUAL("value2", define2.value);
}
