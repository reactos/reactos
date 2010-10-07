#include <vector>
#include <algorithm>
#include <string>
#if defined (STLPORT) && !defined (_STLP_NO_EXTENSIONS)
#  include <rope>
#endif
#if defined (STLPORT) && !defined (_STLP_NO_EXTENSIONS)
#  include <slist>
#endif
#include <list>
#include <deque>
#include <set>
#include <map>
#if defined (STLPORT)
#  include <unordered_set>
#  include <unordered_map>
#endif
#if defined (STLPORT) && !defined (_STLP_NO_EXTENSIONS)
#  include <hash_set>
#  include <hash_map>
#endif
#include <queue>
#include <stack>

#include "mvctor_test.h"

#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#  if defined (STLPORT)
using namespace std::tr1;
#  endif
#endif

#if defined (STLPORT) && !defined (_STLP_NO_MOVE_SEMANTIC)

#  if defined (__GNUC__) && defined (_STLP_USE_NAMESPACES)
// libstdc++ sometimes exposed its own __true_type in
// global namespace resulting in an ambiguity.
#    define __true_type std::__true_type
#    define __false_type std::__false_type
#  endif

static bool type_to_bool(__true_type)
{ return true; }
static bool type_to_bool(__false_type)
{ return false; }

template <class _Tp>
static bool is_movable(const _Tp&) {
  typedef typename __move_traits<_Tp>::implemented _MovableTp;
  return type_to_bool(_MovableTp());
}

template <class _Tp>
static bool is_move_complete(const _Tp&) {
  typedef __move_traits<_Tp> _TpMoveTraits;
  typedef typename _TpMoveTraits::complete _TpMoveComplete;
  return type_to_bool(_TpMoveComplete());
}

struct specially_allocated_struct {
  bool operator < (const specially_allocated_struct&) const;
#  if defined (__DMC__) // slist<_Tp,_Alloc>::remove error
  bool operator==(const specially_allocated_struct&) const;
#  endif
};

#if defined (__DMC__)
bool specially_allocated_struct::operator < (const specially_allocated_struct&) const
{ return false; }
#endif

struct struct_with_specialized_less {};

#  if defined (_STLP_USE_NAMESPACES)
namespace std {
#  endif
  _STLP_TEMPLATE_NULL
  class allocator<specially_allocated_struct> {
    //This allocator just represent what a STLport could do and in this
    //case the STL containers implemented with it should still be movable
    //but not completely as we cannot do any hypothesis on what is in this
    //allocator.
  public:
    typedef specially_allocated_struct value_type;
    typedef value_type *       pointer;
    typedef const value_type* const_pointer;
    typedef value_type&       reference;
    typedef const value_type& const_reference;
    typedef size_t     size_type;
    typedef ptrdiff_t  difference_type;
#  if defined (_STLP_MEMBER_TEMPLATE_CLASSES)
    template <class _Tp1> struct rebind {
      typedef allocator<_Tp1> other;
    };
#  endif
    allocator() _STLP_NOTHROW {}
#  if defined (_STLP_MEMBER_TEMPLATES)
    template <class _Tp1> allocator(const allocator<_Tp1>&) _STLP_NOTHROW {}
#  endif
    allocator(const allocator&) _STLP_NOTHROW {}
    ~allocator() _STLP_NOTHROW {}
    pointer address(reference __x) const { return &__x; }
    const_pointer address(const_reference __x) const { return &__x; }
    pointer allocate(size_type, const void* = 0) { return 0; }
    void deallocate(pointer, size_type) {}
    size_type max_size() const _STLP_NOTHROW  { return 0; }
    void construct(pointer, const_reference) {}
    void destroy(pointer) {}
  };

  _STLP_TEMPLATE_NULL
  struct less<struct_with_specialized_less> {
    bool operator() (struct_with_specialized_less const&,
                     struct_with_specialized_less const&) const;
  };

#  if defined (_STLP_CLASS_PARTIAL_SPECIALIZATION)
#    if !defined (_STLP_NO_MOVE_SEMANTIC)
#      if defined (__BORLANDC__) && (__BORLANDC__ >= 0x564)
  _STLP_TEMPLATE_NULL
  struct __move_traits<vector<specially_allocated_struct> > {
    typedef __true_type implemented;
    typedef __false_type complete;
  };
  _STLP_TEMPLATE_NULL
  struct __move_traits<deque<specially_allocated_struct> > {
    typedef __true_type implemented;
    typedef __false_type complete;
  };
  _STLP_TEMPLATE_NULL
  struct __move_traits<list<specially_allocated_struct> > {
    typedef __true_type implemented;
    typedef __false_type complete;
  };
  _STLP_TEMPLATE_NULL
  struct __move_traits<slist<specially_allocated_struct> > {
    typedef __true_type implemented;
    typedef __false_type complete;
  };
  _STLP_TEMPLATE_NULL
  struct __move_traits<less<struct_with_specialized_less> > {
    typedef __true_type implemented;
    typedef __false_type complete;
  };
  _STLP_TEMPLATE_NULL
  struct __move_traits<set<specially_allocated_struct> > {
    typedef __true_type implemented;
    typedef __false_type complete;
  };
  _STLP_TEMPLATE_NULL
  struct __move_traits<multiset<specially_allocated_struct> > {
    typedef __true_type implemented;
    typedef __false_type complete;
  };
#      endif
#    endif
#  endif

#  if defined (_STLP_USE_NAMESPACES)
}
#  endif
#endif

void MoveConstructorTest::movable_declaration()
{
#if defined (STLPORT) && !defined (_STLP_DONT_SIMULATE_PARTIAL_SPEC_FOR_TYPE_TRAITS) && \
                         !defined (_STLP_NO_MOVE_SEMANTIC)
  //This test purpose is to check correct detection of the STL movable
  //traits declaration
  {
    //string, wstring:
    CPPUNIT_ASSERT( is_movable(string()) );
#  if defined (_STLP_CLASS_PARTIAL_SPECIALIZATION)
    CPPUNIT_ASSERT( is_move_complete(string()) );
#  else
    CPPUNIT_ASSERT( !is_move_complete(string()) );
#  endif
#  if defined (_STLP_HAS_WCHAR_T)
    CPPUNIT_ASSERT( is_movable(wstring()) );
#    if defined (_STLP_CLASS_PARTIAL_SPECIALIZATION)
    CPPUNIT_ASSERT( is_move_complete(wstring()) );
#    else
    CPPUNIT_ASSERT( !is_move_complete(wstring()) );
#    endif
#  endif
  }

#  if defined (STLPORT) && !defined (_STLP_NO_EXTENSIONS)
  {
    //crope, wrope:
    CPPUNIT_ASSERT( is_movable(crope()) );
#    if defined (_STLP_CLASS_PARTIAL_SPECIALIZATION)
    CPPUNIT_ASSERT( is_move_complete(crope()) );
#    else
    CPPUNIT_ASSERT( !is_move_complete(crope()) );
#    endif
#    if defined (_STLP_HAS_WCHAR_T)
    CPPUNIT_ASSERT( is_movable(wrope()) );
#      if defined (_STLP_CLASS_PARTIAL_SPECIALIZATION)
    CPPUNIT_ASSERT( is_move_complete(wrope()) );
#      else
    CPPUNIT_ASSERT( !is_move_complete(wrope()) );
#      endif
#    endif
  }
#  endif

  {
    //vector:
    CPPUNIT_ASSERT( is_movable(vector<char>()) );
    CPPUNIT_ASSERT( is_movable(vector<specially_allocated_struct>()) );
#  if defined (_STLP_CLASS_PARTIAL_SPECIALIZATION)
    CPPUNIT_ASSERT( is_move_complete(vector<char>()) );
    CPPUNIT_ASSERT( !is_move_complete(vector<specially_allocated_struct>()) );
#  else
    CPPUNIT_ASSERT( !is_move_complete(vector<char>()) );
#  endif
  }

  {
    //deque:
    CPPUNIT_ASSERT( is_movable(deque<char>()) );
    CPPUNIT_ASSERT( is_movable(deque<specially_allocated_struct>()) );
#  if defined (_STLP_CLASS_PARTIAL_SPECIALIZATION)
    CPPUNIT_ASSERT( is_move_complete(deque<char>()) );
    CPPUNIT_ASSERT( !is_move_complete(deque<specially_allocated_struct>()) );
#  else
    CPPUNIT_ASSERT( !is_move_complete(deque<char>()) );
#  endif
  }

  {
    //list:
    CPPUNIT_ASSERT( is_movable(list<char>()) );
    CPPUNIT_ASSERT( is_movable(list<specially_allocated_struct>()) );
#  if defined (_STLP_CLASS_PARTIAL_SPECIALIZATION)
    CPPUNIT_ASSERT( is_move_complete(list<char>()) );
    CPPUNIT_ASSERT( !is_move_complete(list<specially_allocated_struct>()) );
#  else
    CPPUNIT_ASSERT( !is_move_complete(list<char>()) );
#  endif
  }

#  if defined (STLPORT) && !defined (_STLP_NO_EXTENSIONS)
  {
    //slist:
    CPPUNIT_ASSERT( is_movable(slist<char>()) );
    CPPUNIT_ASSERT( is_movable(slist<specially_allocated_struct>()) );
#    if defined (_STLP_CLASS_PARTIAL_SPECIALIZATION)
    CPPUNIT_ASSERT( is_move_complete(slist<char>()) );
    CPPUNIT_ASSERT( !is_move_complete(slist<specially_allocated_struct>()) );
#    else
    CPPUNIT_ASSERT( !is_move_complete(slist<char>()) );
#    endif
  }
#  endif

  {
    //queue:
    CPPUNIT_ASSERT( is_movable(queue<char>()) );
    CPPUNIT_ASSERT( is_movable(queue<specially_allocated_struct>()) );
#  if defined (_STLP_CLASS_PARTIAL_SPECIALIZATION)
    CPPUNIT_ASSERT( is_move_complete(queue<char>()) );
    CPPUNIT_ASSERT( !is_move_complete(queue<specially_allocated_struct>()) );
#  else
    CPPUNIT_ASSERT( !is_move_complete(queue<char>()) );
#  endif
  }

  {
    //stack:
    CPPUNIT_ASSERT( is_movable(stack<char>()) );
    CPPUNIT_ASSERT( is_movable(stack<specially_allocated_struct>()) );
#  if defined (_STLP_CLASS_PARTIAL_SPECIALIZATION)
    CPPUNIT_ASSERT( is_move_complete(stack<char>()) );
    CPPUNIT_ASSERT( !is_move_complete(stack<specially_allocated_struct>()) );
#  else
    CPPUNIT_ASSERT( !is_move_complete(stack<char>()) );
#  endif
  }

#endif
}

void MoveConstructorTest::movable_declaration_assoc()
{
#if defined (STLPORT) && !defined (_STLP_DONT_SIMULATE_PARTIAL_SPEC_FOR_TYPE_TRAITS) && \
                         !defined (_STLP_NO_MOVE_SEMANTIC)
  {
    //associative containers, set multiset, map, multimap:

    //For associative containers it is important that less is correctly recognize as
    //the STLport less or a user specialized less:
#  if defined (_STLP_CLASS_PARTIAL_SPECIALIZATION)
    CPPUNIT_ASSERT( is_move_complete(less<char>()) );
#  endif
    CPPUNIT_ASSERT( !is_move_complete(less<struct_with_specialized_less>()) );

    //set
    CPPUNIT_ASSERT( is_movable(set<char>()) );
    CPPUNIT_ASSERT( is_movable(set<specially_allocated_struct>()) );
#  if defined (_STLP_CLASS_PARTIAL_SPECIALIZATION)
    CPPUNIT_ASSERT( is_move_complete(set<char>()) );
    CPPUNIT_ASSERT( !is_move_complete(set<specially_allocated_struct>()) );
#  else
    CPPUNIT_ASSERT( !is_move_complete(set<char>()) );
#  endif

    //multiset
    CPPUNIT_ASSERT( is_movable(multiset<char>()) );
    CPPUNIT_ASSERT( is_movable(multiset<specially_allocated_struct>()) );
#  if defined (_STLP_CLASS_PARTIAL_SPECIALIZATION)
    CPPUNIT_ASSERT( is_move_complete(multiset<char>()) );
    CPPUNIT_ASSERT( !is_move_complete(multiset<specially_allocated_struct>()) );
#  else
    CPPUNIT_ASSERT( !is_move_complete(multiset<char>()) );
#  endif

    //map
    CPPUNIT_ASSERT( is_movable(map<char, char>()) );
    CPPUNIT_ASSERT( is_movable(map<specially_allocated_struct, char>()) );
#  if defined (_STLP_CLASS_PARTIAL_SPECIALIZATION)
    CPPUNIT_ASSERT( is_move_complete(map<char, char>()) );
    //Here even if allocator has been specialized for specially_allocated_struct
    //this pecialization won't be used in default map instanciation as the default
    //allocator is allocator<pair<specially_allocated_struct, char> >
    CPPUNIT_ASSERT( is_move_complete(map<specially_allocated_struct, char>()) );
#  else
    CPPUNIT_ASSERT( !is_move_complete(map<char, char>()) );
#  endif

    //multimap
    CPPUNIT_ASSERT( is_movable(multimap<char, char>()) );
    CPPUNIT_ASSERT( is_movable(multimap<specially_allocated_struct, char>()) );
#  if defined (_STLP_CLASS_PARTIAL_SPECIALIZATION)
    CPPUNIT_ASSERT( is_move_complete(multimap<char, char>()) );
    //Idem map remark
    CPPUNIT_ASSERT( is_move_complete(multimap<specially_allocated_struct, char>()) );
#  else
    CPPUNIT_ASSERT( !is_move_complete(multimap<char, char>()) );
#  endif
  }
#endif
}

void MoveConstructorTest::movable_declaration_hash()
{
#if defined (STLPORT) && !defined (_STLP_DONT_SIMULATE_PARTIAL_SPEC_FOR_TYPE_TRAITS) && \
                         !defined (_STLP_NO_MOVE_SEMANTIC)
  {
    //hashed containers, unordered_set unordered_multiset, unordered_map, unordered_multimap,
    //                   hash_set, hash_multiset, hash_map, hash_multimap:

    //We only check that they are movable, completness is not yet supported
    CPPUNIT_ASSERT( is_movable(unordered_set<char>()) );
    CPPUNIT_ASSERT( is_movable(unordered_multiset<char>()) );
    CPPUNIT_ASSERT( is_movable(unordered_map<char, char>()) );
    CPPUNIT_ASSERT( is_movable(unordered_multimap<char, char>()) );
#  if defined (STLPORT) && !defined (_STLP_NO_EXTENSIONS)
    CPPUNIT_ASSERT( is_movable(hash_set<char>()) );
    CPPUNIT_ASSERT( is_movable(hash_multiset<char>()) );
    CPPUNIT_ASSERT( is_movable(hash_map<char, char>()) );
    CPPUNIT_ASSERT( is_movable(hash_multimap<char, char>()) );
#  endif
  }
#endif
}

