#include "test.h"

using std::string;

void FunctionTest::Run ()
{
	string fixedupFilename = FixupTargetFilename ( "." SSEP "dir1" SSEP "dir2" SSEP ".." SSEP "filename.txt" );
	ARE_EQUAL ( "$(INTERMEDIATE)dir1" SSEP "filename.txt", fixedupFilename );
}
