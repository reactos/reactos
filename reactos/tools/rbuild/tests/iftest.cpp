#include "test.h"

using std::string;

void IfTest::Run()
{
	string projectFilename ( "tests/data/if.xml" );
	Project project ( projectFilename );

	ARE_EQUAL ( 1, project.modules.size () );
	Module& module1 = *project.modules[0];

	ARE_EQUAL ( 1, module1.ifs.size () );
	If& if1 = *module1.ifs[0];
	ARE_EQUAL ( "VAR1", if1.property );
	ARE_EQUAL ( "value1", if1.value );

	ARE_EQUAL ( 1, if1.files.size () );
	File& file1 = *if1.files[0];
	ARE_EQUAL( "." SSEP "file1.c", file1.name );

	ARE_EQUAL ( 1, module1.files.size () );
	File& file2 = *module1.files[0];
	ARE_EQUAL( "." SSEP "file2.c", file2.name );
}
