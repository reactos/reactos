#include "test.h"

using std::string;

void SourceFileTest::Run()
{
	const Project project ( "tests/data/automaticdependency.xml" );
	AutomaticDependency automaticDependency ( project );
	automaticDependency.Process ();
	ARE_EQUAL(3, automaticDependency.sourcefile_map.size());
}
