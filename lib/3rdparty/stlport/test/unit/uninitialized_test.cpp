#include <memory>
#include <vector>
#include <list>

#include "cppunit/cppunit_proxy.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

//
// TestCase class
//
class UninitializedTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(UninitializedTest);
  CPPUNIT_TEST(copy_test);
  //CPPUNIT_TEST(fill_test);
  //CPPUNIT_TEST(fill_n_test);
  CPPUNIT_TEST_SUITE_END();

protected:
  void copy_test();
  void fill_test();
  void fill_n_test();
};

CPPUNIT_TEST_SUITE_REGISTRATION(UninitializedTest);

struct NotTrivialCopyStruct {
  NotTrivialCopyStruct() : member(0) {}
  NotTrivialCopyStruct(NotTrivialCopyStruct const&) : member(1) {}

  int member;
};

struct TrivialCopyStruct {
  TrivialCopyStruct() : member(0) {}
  TrivialCopyStruct(TrivialCopyStruct const&) : member(1) {}

  int member;
};

struct TrivialInitStruct {
  TrivialInitStruct()
  { ++nbConstructorCalls; }

  static size_t nbConstructorCalls;
};

size_t TrivialInitStruct::nbConstructorCalls = 0;

#if defined (STLPORT)
#  if defined (_STLP_USE_NAMESPACES)
namespace std {
#  endif
  _STLP_TEMPLATE_NULL
  struct __type_traits<TrivialCopyStruct> {
     typedef __false_type has_trivial_default_constructor;
     //This is a wrong declaration just to check that internaly a simple memcpy is called:
     typedef __true_type has_trivial_copy_constructor;
     typedef __true_type has_trivial_assignment_operator;
     typedef __true_type has_trivial_destructor;
     typedef __false_type is_POD_type;
  };

  _STLP_TEMPLATE_NULL
  struct __type_traits<TrivialInitStruct> {
     //This is a wrong declaration just to check that internaly no initialization is done:
     typedef __true_type has_trivial_default_constructor;
     typedef __true_type has_trivial_copy_constructor;
     typedef __true_type has_trivial_assignment_operator;
     typedef __true_type has_trivial_destructor;
     typedef __false_type is_POD_type;
  };
#  if defined (_STLP_USE_NAMESPACES)
}
#  endif
#endif

struct base {};
struct derived : public base {};

//
// tests implementation
//
void UninitializedTest::copy_test()
{
  {
    //Random iterators
    {
      vector<NotTrivialCopyStruct> src(10);
      vector<NotTrivialCopyStruct> dst(10);
      uninitialized_copy(src.begin(), src.end(), dst.begin());
      vector<NotTrivialCopyStruct>::const_iterator it(dst.begin()), end(dst.end());
      for (; it != end; ++it) {
        CPPUNIT_ASSERT( (*it).member == 1 );
      }
    }
    {
      /** Note: we use static arrays here so the iterators are always
      pointers, even in debug mode. */
      size_t const count = 10;
      TrivialCopyStruct src[count];
      TrivialCopyStruct dst[count];

      TrivialCopyStruct* it = src + 0;
      TrivialCopyStruct* end = src + count;
      for (; it != end; ++it) {
        (*it).member = 0;
      }

      uninitialized_copy(src+0, src+count, dst+0);
      for (it = dst+0, end = dst+count; it != end; ++it) {
#if defined (STLPORT)
        /* If the member is 1, it means that library has not found any
        optimization oportunity and called the regular copy-ctor instead. */
        CPPUNIT_ASSERT( (*it).member == 0 );
#else
        CPPUNIT_ASSERT( (*it).member == 1 );
#endif
      }
    }
  }

  {
    //Bidirectional iterator
    {
      vector<NotTrivialCopyStruct> src(10);
      list<NotTrivialCopyStruct> dst(10);

      list<NotTrivialCopyStruct>::iterator it(dst.begin()), end(dst.end());
      for (; it != end; ++it) {
        (*it).member = -1;
      }

      uninitialized_copy(src.begin(), src.end(), dst.begin());

      for (it = dst.begin(); it != end; ++it) {
        CPPUNIT_ASSERT( (*it).member == 1 );
      }
    }

    {
      list<NotTrivialCopyStruct> src(10);
      vector<NotTrivialCopyStruct> dst(10);

      vector<NotTrivialCopyStruct>::iterator it(dst.begin()), end(dst.end());
      for (; it != end; ++it) {
        (*it).member = -1;
      }

      uninitialized_copy(src.begin(), src.end(), dst.begin());

      for (it = dst.begin(); it != end; ++it) {
        CPPUNIT_ASSERT( (*it).member == 1 );
      }
    }
  }

  {
    //Using containers of native types:
#if !defined (STLPORT) || !defined (_STLP_NO_MEMBER_TEMPLATES)
    {
      vector<int> src;
      int i;
      for (i = -5; i < 6; ++i) {
        src.push_back(i);
      }

      //Building a vector result in a uninitialized_copy call internally
      vector<unsigned int> dst(src.begin(), src.end());
      vector<unsigned int>::const_iterator it(dst.begin());
      for (i = -5; i < 6; ++i, ++it) {
        CPPUNIT_ASSERT( *it == (unsigned int)i );
      }
    }

    {
      vector<char> src;
      char i;
      for (i = -5; i < 6; ++i) {
        src.push_back(i);
      }

      //Building a vector result in a uninitialized_copy call internally
      vector<unsigned int> dst(src.begin(), src.end());
      vector<unsigned int>::const_iterator it(dst.begin());
      for (i = -5; i < 6; ++i, ++it) {
        CPPUNIT_ASSERT( *it == (unsigned int)i );
      }
    }

    {
      vector<int> src;
      int i;
      for (i = -5; i < 6; ++i) {
        src.push_back(i);
      }

      //Building a vector result in a uninitialized_copy call internally
      vector<float> dst(src.begin(), src.end());
      vector<float>::const_iterator it(dst.begin());
      for (i = -5; i < 6; ++i, ++it) {
        CPPUNIT_ASSERT( *it == (float)i );
      }
    }

    {
      vector<vector<float>*> src(10);
      vector<vector<float>*> dst(src.begin(), src.end());
    }

    {
      derived d;
      //base *pb = &d;
      derived *pd = &d;
      //base **ppb = &pd;
      vector<derived*> src(10, pd);
      vector<base*> dst(src.begin(), src.end());
      vector<base*>::iterator it(dst.begin()), end(dst.end());
      for (; it != end; ++it) {
        CPPUNIT_ASSERT( (*it) == pd );
      }
    }
#endif
  }

  {
    //Vector initialization:
    vector<TrivialInitStruct> vect(10);
    //Just 1 constructor call for the default value:
    CPPUNIT_ASSERT( TrivialInitStruct::nbConstructorCalls == 1  );
  }
}

/*
void UninitializedTest::fill_test()
{
}

void UninitializedTest::fill_n_test()
{
}
*/
