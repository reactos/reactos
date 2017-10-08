#include <list> /* required, to expose allocator */
#include <stdexcept>
#include <stdio.h>

using namespace std;

struct BigStruct
{
  char _data[4096];
};

void bad_alloc_test()
{
  typedef allocator<BigStruct> BigStructAllocType;
  BigStructAllocType bigStructAlloc;

  try {
    //Lets try to allocate almost 4096 Go (on most of the platforms) of memory:
    BigStructAllocType::pointer pbigStruct = bigStructAlloc.allocate(1024 * 1024 * 1024);

    // CPPUNIT_ASSERT( pbigStruct != 0 && "Allocation failed but no exception thrown" );
  }
  catch (bad_alloc const&) {
    printf( "Ok\n" );
  }
  catch (...) {
    //We shouldn't be there:
    // CPPUNIT_ASSERT( false && "Not bad_alloc exception thrown." );
  }
}

void bad_alloc_test1()
{
  try {
    allocator<BigStruct> all;
    BigStruct *bs = all.allocate(1024*1024*1024);

    // throw bad_alloc();
  }
  catch ( bad_alloc const & ) {
    printf( "I am here\n" );
  }
  catch ( ... ) {
  }
}

int main()
{
  bad_alloc_test();
#if 0
  try {
    throw bad_alloc();
  }
  catch ( bad_alloc& ) {
  }
  catch ( ... ) {
  }
#endif
  return 0;
}
