#include "test.h"

using std::string;

void ModuleTest::Run()
{
	string projectFilename ( "tests/data/module.xml" );
	Project project ( projectFilename );
	ARE_EQUAL(2, project.modules.size());

	Module& module1 = *project.modules[0];
	IS_TRUE(module1.type == BuildTool);
	ARE_EQUAL(2, module1.files.size());
	ARE_EQUAL("./dir1/file1.c", module1.files[0]->name);
	ARE_EQUAL("./dir1/file2.c", module1.files[1]->name);

	ARE_EQUAL(0, module1.libraries.size());

	Module& module2 = *project.modules[1];
	IS_TRUE(module2.type == KernelModeDLL);
	ARE_EQUAL(2, module2.files.size());
	ARE_EQUAL("./dir2/file3.c", module2.files[0]->name);
	ARE_EQUAL("./dir2/file4.c", module2.files[1]->name);

	ARE_EQUAL(1, module2.libraries.size());
	Library& library1 = *module2.libraries[0];

	ARE_EQUAL("module1", library1.name);
}
