#include "test.h"

using std::string;

void ModuleTest::Run()
{
	string projectFilename ( "tests/data/module.xml" );
	Project* project = new Project( projectFilename );
	ARE_EQUAL(2, project->modules.size());

	Module& module1 = *project->modules[0];
	ARE_EQUAL(2, module1.files.size());
	ARE_EQUAL("./dir1/file1.c", module1.files[0]->name);
	ARE_EQUAL("./dir1/file2.c", module1.files[1]->name);
	
	Module& module2 = *project->modules[1];
	ARE_EQUAL(2, module2.files.size());
	ARE_EQUAL("./dir2/file3.c", module2.files[0]->name);
	ARE_EQUAL("./dir2/file4.c", module2.files[1]->name);
}
