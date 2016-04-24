#include <memory>
#include <vector>

#include <cstdio>

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class AllocatorTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(AllocatorTest);
  CPPUNIT_TEST(zero_allocation);
#if !defined (STLPORT) || defined (_STLP_USE_EXCEPTIONS)
  CPPUNIT_TEST(bad_alloc_test);
#endif
#if defined (STLPORT) && defined (_STLP_THREADS) && defined (_STLP_USE_PERTHREAD_ALLOC)
  CPPUNIT_TEST(per_thread_alloc);
#endif
  CPPUNIT_TEST_SUITE_END();

protected:
  void zero_allocation();
  void bad_alloc_test();
  void per_thread_alloc();
};

CPPUNIT_TEST_SUITE_REGISTRATION(AllocatorTest);

//
// tests implementation
//
void AllocatorTest::zero_allocation()
{
  typedef allocator<char> CharAllocator;
  CharAllocator charAllocator;

  char* buf = charAllocator.allocate(0);
  charAllocator.deallocate(buf, 0);

  charAllocator.deallocate(0, 0);
}

#if !defined (STLPORT) || defined (_STLP_USE_EXCEPTIONS)

struct BigStruct
{
  char _data[4096];
};

void AllocatorTest::bad_alloc_test()
{
  typedef allocator<BigStruct> BigStructAllocType;
  BigStructAllocType bigStructAlloc;

  try {
    //Lets try to allocate almost 4096 Go (on most of the platforms) of memory:
    BigStructAllocType::pointer pbigStruct = bigStructAlloc.allocate(1024 * 1024 * 1024);

    //Allocation failed but no exception thrown
    CPPUNIT_ASSERT( pbigStruct != 0 );

    // Just it case it succeeds:
    bigStructAlloc.deallocate(pbigStruct, 1024 * 1024 * 1024);
  }
  catch (bad_alloc const&) {
  }
  catch (...) {
    //We shouldn't be there:
    //Not bad_alloc exception thrown.
    CPPUNIT_FAIL;
  }
}
#endif

#if defined (STLPORT) && defined (_STLP_THREADS) && defined (_STLP_USE_PERTHREAD_ALLOC)
#  include <pthread.h>

class SharedDatas
{
public:
  typedef vector<int, per_thread_allocator<int> > thread_vector;

  SharedDatas(size_t nbElems) : threadVectors(nbElems, (thread_vector*)0) {
    pthread_mutex_init(&mutex, 0);
    pthread_cond_init(&condition, 0);
  }

  ~SharedDatas() {
    for (size_t i = 0; i < threadVectors.size(); ++i) {
      delete threadVectors[i];
    }
  }

  size_t initThreadVector() {
    size_t ret;

    pthread_mutex_lock(&mutex);

    for (size_t i = 0; i < threadVectors.size(); ++i) {
      if (threadVectors[i] == 0) {
        threadVectors[i] = new thread_vector();
        ret = i;
        break;
      }
    }

    if (ret != threadVectors.size() - 1) {
      //We wait for other thread(s) to call this method too:
      printf("Thread %d wait\n", ret);
      pthread_cond_wait(&condition, &mutex);
    }
    else {
      //We are the last thread calling this method, we signal this
      //to the other thread(s) that might be waiting:
      printf("Thread %d signal\n", ret);
      pthread_cond_signal(&condition);
    }

    pthread_mutex_unlock(&mutex);

    return ret;
  }

  thread_vector& getThreadVector(size_t index) {
    //We return other thread thread_vector instance:
    return *threadVectors[(index + 1 == threadVectors.size()) ? 0 : index + 1];
  }

private:
  pthread_mutex_t mutex;
  pthread_cond_t condition;
  vector<thread_vector*> threadVectors;
};

void* f(void* pdatas) {
  SharedDatas *psharedDatas = (SharedDatas*)pdatas;

  int threadIndex = psharedDatas->initThreadVector();

  for (int i = 0; i < 100; ++i) {
    psharedDatas->getThreadVector(threadIndex).push_back(i);
  }

  return 0;
}

void AllocatorTest::per_thread_alloc()
{
  const size_t nth = 2;
  SharedDatas datas(nth);
  pthread_t t[nth];

  size_t i;
  for (i = 0; i < nth; ++i) {
    pthread_create(&t[i], 0, f, &datas);
  }

  for (i = 0; i < nth; ++i ) {
    pthread_join(t[i], 0);
  }
}
#endif
