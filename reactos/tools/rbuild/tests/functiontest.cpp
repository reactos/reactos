#include "test.h"

using std::string;

void FunctionTest::Run ()
{
	string fixedupFilename = NormalizeFilename ( "." SSEP "dir1" SSEP "dir2" SSEP ".." SSEP "filename.txt" );
	ARE_EQUAL ( "dir1" SSEP "filename.txt", fixedupFilename );
	ARE_EQUAL ( "file.txt", GetFilename ( "file.txt" ) );
	ARE_EQUAL ( "file.txt", GetFilename ( "dir" SSEP "file.txt" ) );
}
