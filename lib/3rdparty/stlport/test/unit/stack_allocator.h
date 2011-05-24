#ifndef STLPORT_UNIT_TEST_STACK_ALLOCATOR_H
#define STLPORT_UNIT_TEST_STACK_ALLOCATOR_H

#include <algorithm>

#if !defined (STLPORT) || defined (_STLP_USE_EXCEPTIONS)
//For bad_alloc:
#  include <new>
#endif

#undef __STD
#if !defined (STLPORT) || defined (_STLP_USE_NAMESPACES)
#  define __STD std::
#else
#  define __STD
#endif

struct State {
  char *m_beg, *m_end, *m_cur;
  bool m_isOk, m_swaped;
  int m_nbAlloc;

  //The following members are shared among all StackAllocator instance created from
  //a reference StackAllocator instance:
  char **m_sharedCur;
  bool *m_sharedOk;
  int *m_sharedNbAlloc;

#if defined (__DMC__)
  State(){}
#endif

  State(char *beg, char *end)
    : m_beg(beg), m_end(end), m_cur(m_beg), m_isOk(true), m_swaped(false), m_nbAlloc(0),
      m_sharedCur(&m_cur), m_sharedOk(&m_isOk), m_sharedNbAlloc(&m_nbAlloc) {}

  State(const State& other)
  : m_beg(other.m_beg), m_end(other.m_end), m_cur(0),
    m_isOk(true), m_swaped(other.m_swaped), m_nbAlloc(0),
    m_sharedCur(other.m_sharedCur), m_sharedOk(other.m_sharedOk),
    m_sharedNbAlloc(other.m_sharedNbAlloc) {}
};

/* This allocator is not thread safe:
 */
template <class _Tp>
struct StackAllocator
#if defined (STLPORT) && \
    defined (_STLP_USE_PARTIAL_SPEC_WORKAROUND) && !defined (_STLP_FUNCTION_TMPL_PARTIAL_ORDER)
  //Special Borland workaround that have problem with function
  //overloading when one of the overloaded version is a template
  //one. This is the case for the std::swap function.
  : public __STD __stlport_class<StackAllocator<_Tp> >
#endif
{
  typedef _Tp        value_type;
  typedef value_type *       pointer;
  typedef const _Tp* const_pointer;
  typedef _Tp&       reference;
  typedef const _Tp& const_reference;
  typedef size_t     size_type;
  typedef ptrdiff_t  difference_type;

#if defined (__DMC__)
  StackAllocator(){}
#endif

  StackAllocator(char *beg, char *end)
    : m_state(beg, end) {}

  const State& getState() const { return m_state; }
#if !defined (STLPORT) || defined (_STLP_MEMBER_TEMPLATES)
  template <class _OtherTp>
  StackAllocator(StackAllocator<_OtherTp> const& other)
    : m_state(other.getState()) {}
#else
  StackAllocator(const State& state)
    : m_state(state) {}
#endif

#if !defined (STLPORT) || defined (_STLP_MEMBER_TEMPLATE_CLASSES)
  template <class _Other>
  struct rebind {
    typedef StackAllocator<_Other> other;
  };
#endif

  _Tp* allocate(size_type n, void* = 0) {
    if (n == 0)
      return 0;

    ++(*m_state.m_sharedNbAlloc);

    if (*m_state.m_sharedCur + (n * sizeof(_Tp)) < m_state.m_end) {
      char *ret = *m_state.m_sharedCur;
      *m_state.m_sharedCur += n * sizeof(_Tp);
      return reinterpret_cast<_Tp*>(ret);
    }
#if !defined (STLPORT) || defined (_STLP_USE_EXCEPTIONS)
    throw __STD bad_alloc();
#  if defined (__DMC__)
    return 0;
#  endif
#else
    return 0;
#endif
  }

#if defined (STLPORT) && \
    defined (_STLP_USE_PARTIAL_SPEC_WORKAROUND) && !defined (_STLP_FUNCTION_TMPL_PARTIAL_ORDER)
  //Necessary extension to make StackAllocator a real STLport allocator
  //implementation:
  _Tp* _M_allocate(size_type n, size_type &new_n) {
    new_n = n;
    return allocate(n);
  }
#endif

  void deallocate(pointer p, size_type n) {
    if (p == 0)
      return;

    --(*m_state.m_sharedNbAlloc);

    if ((char*)p == (*m_state.m_sharedCur - n * sizeof(_Tp))) {
      *m_state.m_sharedCur -= n * sizeof(_Tp);
    }

    if ((char*)p < m_state.m_beg || (char*)p >= m_state.m_end) {
      //An object has been returned to the bad allocator instance:
      *m_state.m_sharedOk = false;
    }
  }

  pointer address(reference __x) const {return &__x;}
  const_pointer address(const_reference __x) const { return &__x; }
  size_type max_size() const { return m_state.m_end - *m_state.m_sharedCur; }
  void construct(pointer __p, const_reference __val) { new(__p) _Tp(__val);  }
  void destroy(pointer __p) { __p->~_Tp(); }

  bool ok() const { return m_state.m_isOk && (m_state.m_nbAlloc == 0); }
  void reset () {
    m_state.m_cur = m_state.m_beg;
    m_state.m_isOk = true;
    m_state.m_swaped = false;
  }
  bool swaped() const { return m_state.m_swaped; }
  void swap(StackAllocator &other) {
    __STD swap(m_state, other.m_state);
    m_state.m_swaped = true;
    other.m_state.m_swaped = true;
  }
#if defined (STLPORT) && \
    defined (_STLP_USE_PARTIAL_SPEC_WORKAROUND) && !defined (_STLP_FUNCTION_TMPL_PARTIAL_ORDER)
  void _M_swap_workaround(StackAllocator& __x) { swap(__x); }
#endif

  //2 StackAllocator instance are identical if they are built on top
  //of the same buffer.
  bool operator == (StackAllocator const& other) const
  { return m_state.m_beg == other.m_state.m_beg; }

  bool operator != (StackAllocator const& other) const
  { return !(*this == other); }

private:
  State m_state;
};

#if !defined (STLPORT) || defined (_STLP_USE_NAMESPACES)
namespace std {
#endif

#  if defined (STLPORT) && (defined (_STLP_DONT_SUPPORT_REBIND_MEMBER_TEMPLATE) || !defined (_STLP_MEMBER_TEMPLATES))
template <class _Tp1, class _Tp2>
inline StackAllocator<_Tp2>&
__stl_alloc_rebind(StackAllocator<_Tp1>& __a, const _Tp2*) {  return (StackAllocator<_Tp2>&)(__a); }
template <class _Tp1, class _Tp2>
inline StackAllocator<_Tp2>
__stl_alloc_create(const StackAllocator<_Tp1>& __a, const _Tp2*) { return StackAllocator<_Tp2>(__a.getState()); }
#  endif

#  if !defined (STLPORT) || defined (_STLP_FUNCTION_TMPL_PARTIAL_ORDER)
  template <class _Tp>
  inline void swap(StackAllocator<_Tp>& __a, StackAllocator<_Tp>& __b)
  { __a.swap(__b); }
#  elif !defined (_STLP_USE_PARTIAL_SPEC_WORKAROUND)
//The following overloads depends on instanciation, if new unit tests are written
//with new StackAllocator instanciations associated swap overload should also be
//written
inline void swap(StackAllocator<int>& __a, StackAllocator<int>& __b)
{ __a.swap(__b); }
inline void swap(StackAllocator<char>& __a, StackAllocator<char>& __b)
{ __a.swap(__b); }
inline void swap(StackAllocator<pair<const int, int> >& __a,
                 StackAllocator<pair<const int, int> >& __b)
{ __a.swap(__b); }
#  endif

#if !defined (STLPORT) || defined (_STLP_USE_NAMESPACES)
}
#endif

#undef __STD

#endif //STLPORT_UNIT_TEST_STACK_ALLOCATOR_H
