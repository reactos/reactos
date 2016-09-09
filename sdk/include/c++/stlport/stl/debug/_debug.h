/*
 *
 * Copyright (c) 1997
 * Moscow Center for SPARC Technology
 *
 * Copyright (c) 1999
 * Boris Fomitchev
 *
 * This material is provided "as is", with absolutely no warranty expressed
 * or implied. Any use is at your own risk.
 *
 * Permission to use or copy this software for any purpose is hereby granted
 * without fee, provided the above notices are retained on all copies.
 * Permission to modify the code and to distribute modified code is granted,
 * provided the above notices are retained, and a notice that the code was
 * modified is included with the above copyright notice.
 *
 */

#ifndef _STLP_DEBUG_H
#define _STLP_DEBUG_H

#if (defined (_STLP_DEBUG) || defined (_STLP_DEBUG_ALLOC)) && \
    !defined (_STLP_ASSERTIONS)
#  define _STLP_ASSERTIONS 1
#endif

#if defined (_STLP_ASSERTIONS)

#  if !defined (_STLP_FILE__)
#    define _STLP_FILE__ __FILE__
#  endif

_STLP_BEGIN_NAMESPACE

_STLP_MOVE_TO_PRIV_NAMESPACE

enum {
  //General errors
  _StlFormat_ERROR_RETURN,
  _StlFormat_ASSERTION_FAILURE,
  _StlFormat_VERBOSE_ASSERTION_FAILURE,
  _StlMsg_INVALID_ARGUMENT,
  //Container/Iterator related errors
  _StlMsg_INVALID_CONTAINER,
  _StlMsg_EMPTY_CONTAINER,
  _StlMsg_ERASE_PAST_THE_END,
  _StlMsg_OUT_OF_BOUNDS,
  _StlMsg_NOT_OWNER,
  _StlMsg_SHOULD_NOT_OWNER,
  _StlMsg_INVALID_ITERATOR,
  _StlMsg_INVALID_LEFTHAND_ITERATOR,
  _StlMsg_INVALID_RIGHTHAND_ITERATOR,
  _StlMsg_DIFFERENT_OWNERS     ,
  _StlMsg_NOT_DEREFERENCEABLE  ,
  _StlMsg_INVALID_RANGE        ,
  _StlMsg_NOT_IN_RANGE_1       ,
  _StlMsg_NOT_IN_RANGE_2       ,
  _StlMsg_INVALID_ADVANCE      ,
  _StlMsg_SINGULAR_ITERATOR    ,
  //Bad predicate for sorting
  _StlMsg_INVALID_STRICT_WEAK_PREDICATE,
  _StlMsg_INVALID_EQUIVALENT_PREDICATE,
  // debug alloc messages
  _StlMsg_DBA_DELETED_TWICE    ,
  _StlMsg_DBA_NEVER_ALLOCATED  ,
  _StlMsg_DBA_TYPE_MISMATCH    ,
  _StlMsg_DBA_SIZE_MISMATCH    ,
  _StlMsg_DBA_UNDERRUN         ,
  _StlMsg_DBA_OVERRUN          ,
  // auto_ptr messages
  _StlMsg_AUTO_PTR_NULL    ,
  //Memory alignent message
  _StlMsg_WRONG_MEMORY_ALIGNMENT,
  _StlMsg_UNKNOWN
  /* _StlMsg_MAX */
};

/* have to hardcode that ;() */
#  define _StlMsg_MAX 31

class __owned_link;
class __owned_list;

#  if defined (_STLP_DEBUG_MODE_THROWS)
#    define _STLP_MESSAGE_NORETURN _STLP_FUNCTION_THROWS
#  else
#    define _STLP_MESSAGE_NORETURN
#  endif

template <class _Dummy>
class __stl_debug_engine {
public:
  // Basic routine to report any debug message
  // Use _STLP_DEBUG_MESSAGE to override
  static void _STLP_MESSAGE_NORETURN _STLP_CALL _Message(const char * format_str, ...);

  // Micsellanous function to report indexed error message
  static void _STLP_CALL  _IndexedError(int __ind, const char* __f, int __l);

  // Basic assertion report mechanism.
  // Reports failed assertion via __stl_debug_message and calls _Terminate
  // if _STLP_DEBUG_TERMINATE is specified, calls __stl_debug_terminate instead
  static void _STLP_CALL  _Assert(const char* __expr, const char* __f, int __l);

  // The same, with additional diagnostics
  static void _STLP_CALL  _VerboseAssert(const char* __expr, int __error_ind, const char* __f, int __l);

  // If exceptions are present, sends unique exception
  // If not, calls _STLP_ABORT() to terminate
  // Use _STLP_DEBUG_TERMINATE to override
  static void _STLP_CALL  _Terminate();

#  if defined (_STLP_DEBUG)
  // owned_list/link delegate non-inline functions here

  static bool _STLP_CALL  _Check_same_owner( const __owned_link& __i1,
                                             const __owned_link& __i2);
  static bool _STLP_CALL  _Check_same_or_null_owner( const __owned_link& __i1,
                                                     const __owned_link& __i2);
  static bool _STLP_CALL  _Check_if_owner( const __owned_list*, const __owned_link&);

  static bool _STLP_CALL  _Check_if_not_owner( const __owned_list*, const __owned_link&);

  static void _STLP_CALL  _Verify(const __owned_list*);

  static void _STLP_CALL  _Swap_owners(__owned_list&, __owned_list&);

  static void _STLP_CALL  _Invalidate_all(__owned_list*);

  static void _STLP_CALL  _Set_owner(__owned_list& /*src*/, __owned_list& /*dst*/);

  static void _STLP_CALL  _Stamp_all(__owned_list*, __owned_list*);

  static void _STLP_CALL  _M_detach(__owned_list*, __owned_link*);

  static void _STLP_CALL  _M_attach(__owned_list*, __owned_link*);

  // accessor : check and get pointer to the container
  static void* _STLP_CALL  _Get_container_ptr(const __owned_link*);
#  endif

  // debug messages and formats
  static const char* _Message_table[_StlMsg_MAX];
};

#  undef _STLP_MESSAGE_NORETURN

#  if defined (_STLP_USE_TEMPLATE_EXPORT)
_STLP_EXPORT_TEMPLATE_CLASS __stl_debug_engine<bool>;
#  endif

typedef __stl_debug_engine<bool> __stl_debugger;

_STLP_MOVE_TO_STD_NAMESPACE

_STLP_END_NAMESPACE

#  if !defined (_STLP_ASSERT)
#    define _STLP_ASSERT(expr) \
       if (!(expr)) { _STLP_PRIV __stl_debugger::_Assert( # expr, _STLP_FILE__, __LINE__); }
#  endif

#else
#  define _STLP_ASSERT(expr)
#endif

// this section is for _STLP_DEBUG only
#if defined (_STLP_DEBUG)

#  if !defined (_STLP_VERBOSE_ASSERT)
// fbp : new form not requiring ";"
#    define _STLP_VERBOSE_ASSERT(expr, __diag_num) \
       if (!(expr)) { _STLP_PRIV __stl_debugger::_VerboseAssert\
                               ( # expr,  _STLP_PRIV __diag_num, _STLP_FILE__, __LINE__ ); \
          }
#  endif

#  define _STLP_DEBUG_CHECK(expr) _STLP_ASSERT(expr)

#  if (_STLP_DEBUG_LEVEL == _STLP_STANDARD_DBG_LEVEL)
#    define _STLP_STD_DEBUG_CHECK(expr) _STLP_DEBUG_CHECK(expr)
#  else
#    define _STLP_STD_DEBUG_CHECK(expr)
#  endif

#  if !defined (_STLP_VERBOSE_RETURN)
#    define _STLP_VERBOSE_RETURN(__expr,__diag_num) if (!(__expr)) { \
         _STLP_PRIV __stl_debugger::_IndexedError(__diag_num, _STLP_FILE__ , __LINE__); \
         return false; }
#  endif

#  if !defined (_STLP_VERBOSE_RETURN_0)
#    define _STLP_VERBOSE_RETURN_0(__expr,__diag_num) if (!(__expr)) { \
         _STLP_PRIV __stl_debugger::_IndexedError(__diag_num, _STLP_FILE__, __LINE__); \
         return 0; }
#  endif

#  ifndef _STLP_INTERNAL_THREADS_H
#    include <stl/_threads.h>
#  endif

#  ifndef _STLP_INTERNAL_ITERATOR_BASE_H
#    include <stl/_iterator_base.h>
#  endif

#  ifndef _STLP_TYPE_TRAITS_H
#    include <stl/type_traits.h>
#  endif

_STLP_BEGIN_NAMESPACE

_STLP_MOVE_TO_PRIV_NAMESPACE

/*
 * Special debug iterator traits having an additionnal static member
 * method _Check. It is used by the slist debug implementation to check
 * the special before_begin iterator.
 */
template <class _Traits>
struct _DbgTraits : _Traits {
  typedef _DbgTraits<typename _Traits::_ConstTraits> _ConstTraits;
  typedef _DbgTraits<typename _Traits::_NonConstTraits> _NonConstTraits;

  template <class _Iterator>
  static bool _Check(const _Iterator&) {return true;}
};

//=============================================================
template <class _Iterator>
inline bool  _STLP_CALL __valid_range(const _Iterator& __i1 ,const _Iterator& __i2,
                                      const random_access_iterator_tag&)
{ return (__i1 < __i2) || (__i1 == __i2); }

template <class _Iterator>
inline bool  _STLP_CALL __valid_range(const _Iterator& __i1 ,const _Iterator& __i2,
                                      const bidirectional_iterator_tag&) {
  // check if comparable
  bool __dummy(__i1==__i2);
  return (__dummy==__dummy);
}

template <class _Iterator>
inline bool  _STLP_CALL __valid_range(const _Iterator& __i1 ,const _Iterator& __i2,
                                      const forward_iterator_tag&) {
  // check if comparable
  bool __dummy(__i1==__i2);
  return (__dummy==__dummy);
}

template <class _Iterator>
inline bool  _STLP_CALL __valid_range(const _Iterator&,const _Iterator&,
                                      const input_iterator_tag&)
{ return true; }

template <class _Iterator>
inline bool  _STLP_CALL __valid_range(const _Iterator&,const _Iterator&,
                                      const output_iterator_tag&)
{ return true; }

template <class _Iterator>
inline bool _STLP_CALL __valid_range(const _Iterator& __i1, const _Iterator& __i2)
{ return __valid_range(__i1,__i2,_STLP_ITERATOR_CATEGORY(__i1, _Iterator)); }

// Note : that means in range [i1, i2].
template <class _Iterator>
inline bool  _STLP_CALL stlp_in_range(const _Iterator& _It,
                                      const _Iterator& __i1, const _Iterator& __i2)
{ return __valid_range(__i1,_It) && __valid_range(_It,__i2); }

template <class _Iterator>
inline bool  _STLP_CALL stlp_in_range(const _Iterator& __first, const _Iterator& __last,
                                      const _Iterator& __start, const _Iterator& __finish)
{ return __valid_range(__first,__last) && __valid_range(__start,__first) && __valid_range(__last,__finish); }

//==========================================================
class _STLP_CLASS_DECLSPEC __owned_link {
public:
  // Note: This and the following special defines for compiling under Windows CE under ARM
  // is needed for correctly using _STLP_DEBUG mode. This comes from a bug in the ARM
  // compiler where checked iterators that are passed by value call _M_attach with the wrong
  // this pointer and calling _M_detach can't find the correct pointer to the __owned_link.
  // This is circumvented by managing a _M_self pointer that points to the correct value.
  // Ugly but works.
#if defined(_STLP_WCE) && defined(_ARM_)
  __owned_link() : _M_self(this), _M_owner(0) {}
  __owned_link(const __owned_list* __c) : _M_self(this), _M_owner(0), _M_next(0)
  { __stl_debugger::_M_attach(__CONST_CAST(__owned_list*,__c), this); }
  __owned_link(const __owned_link& __rhs): _M_self(this), _M_owner(0)
  { __stl_debugger::_M_attach(__CONST_CAST(__owned_list*,__rhs._M_owner), this); }
#else
  __owned_link() : _M_owner(0) {}
  __owned_link(const __owned_list* __c) : _M_owner(0), _M_next(0)
  { __stl_debugger::_M_attach(__CONST_CAST(__owned_list*,__c), this); }
  __owned_link(const __owned_link& __rhs): _M_owner(0)
  { __stl_debugger::_M_attach(__CONST_CAST(__owned_list*,__rhs._M_owner), this); }
#endif
  __owned_link& operator=(const __owned_link& __rhs) {
    __owned_list* __new_owner = __CONST_CAST(__owned_list*,__rhs._M_owner);
    __owned_list* __old_owner = _M_owner;
    if ( __old_owner != __new_owner ) {
      __stl_debugger::_M_detach(__old_owner, this);
      __stl_debugger::_M_attach(__new_owner, this);
    }
    return *this;
  }
#if defined(_STLP_WCE) && defined(_ARM_)
  ~__owned_link() {
    __stl_debugger::_M_detach(_M_owner, _M_self);
    _Invalidate();
  }
#else
  ~__owned_link() {
    __stl_debugger::_M_detach(_M_owner, this);
    _Invalidate();
  }
#endif

  const __owned_list* _Owner() const { return _M_owner; }
  __owned_list* _Owner() { return _M_owner; }
  void _Set_owner(const __owned_list* __o) { _M_owner= __CONST_CAST(__owned_list*,__o); }
  bool _Valid() const { return _M_owner != 0; }
  void _Invalidate() { _M_owner = 0; _M_next = 0; }
  void _Link_to_self() { _M_next = 0; }

  __owned_link* _Next() { return _M_next; }
  const __owned_link* _Next() const { return _M_next; }

public:
#if defined(_STLP_WCE) && defined(_ARM_)
  __owned_link* _M_self;
#endif

  __owned_list* _M_owner;
  __owned_link* _M_next;
};


class _STLP_CLASS_DECLSPEC __owned_list {
public:
  __owned_list(void* __o) {
    //    fprintf(stderr, "__owned_list(): %p\n",(void*)this);
    _M_node._M_owner = __REINTERPRET_CAST(__owned_list*,__o);
    _M_node._M_next = 0;
  }
  ~__owned_list() {
    //    fprintf(stderr, "~__owned_list(): %p\n",(void*)this);
    _Invalidate_all();
    // that prevents detach
    _M_node._Invalidate();
  }
  const void* _Owner() const { return (const void*)_M_node._M_owner; }
  void* _Owner() { return (void*)_M_node._M_owner; }
  bool  _Valid() const { return _M_node._M_owner != 0; }
  void _Invalidate() { _M_node._M_owner = 0; }

  __owned_link* _First() { return _M_node._Next(); }
  __owned_link* _Last() { return 0 ; }

  const __owned_link* _First() const { return (__owned_link*)_M_node._M_next; }
  const __owned_link* _Last() const { return 0 ;}

  void _Verify() const { __stl_debugger::_Verify(this); }
  void _Swap_owners(__owned_list& __y) { __stl_debugger::_Swap_owners(*this, __y); }
  void _Invalidate_all() { __stl_debugger::_Invalidate_all(this); }
  void _Set_owner(__owned_list& __y) { __stl_debugger::_Set_owner(*this, __y); }

  mutable __owned_link _M_node;
  mutable _STLP_mutex  _M_lock;

private:
  // should never be called, should be left not implemented,
  // but some compilers complain about it ;(
  __owned_list(const __owned_list&){}
  __owned_list& operator = (const __owned_list&) { return *this; }

  friend class __owned_link;
  friend class __stl_debug_engine<bool>;
};


//==========================================================

// forward declaratioins

template <class _Iterator>
bool _STLP_CALL __check_range(const _Iterator&, const _Iterator&);
template <class _Iterator>
bool _STLP_CALL __check_range(const _Iterator&,
                              const _Iterator&, const _Iterator&);
template <class _Iterator>
bool _STLP_CALL __check_range(const _Iterator&, const _Iterator& ,
                              const _Iterator&, const _Iterator& );
template <class _Tp>
bool _STLP_CALL __check_ptr_range(const _Tp*, const _Tp*);

template <class _Iterator>
void _STLP_CALL __invalidate_range(const __owned_list* __base,
                                   const _Iterator& __first,
                                   const _Iterator& __last);

template <class _Iterator>
void _STLP_CALL __invalidate_iterator(const __owned_list* __base,
                                      const _Iterator& __it);

template <class _Iterator>
void _STLP_CALL __change_range_owner(const _Iterator& __first,
                                     const _Iterator& __last,
                                     const __owned_list* __dst);

template <class _Iterator>
void  _STLP_CALL __change_ite_owner(const _Iterator& __it,
                                    const __owned_list* __dst);

//============================================================
inline bool _STLP_CALL
__check_same_owner(const __owned_link& __i1, const __owned_link& __i2)
{ return __stl_debugger::_Check_same_owner(__i1,__i2); }

inline bool _STLP_CALL
__check_same_or_null_owner(const __owned_link& __i1, const __owned_link& __i2)
{ return __stl_debugger::_Check_same_or_null_owner(__i1,__i2); }

template <class _Iterator>
inline bool _STLP_CALL  __check_if_owner( const __owned_list* __owner,
                                          const _Iterator& __it)
{ return __stl_debugger::_Check_if_owner(__owner, (const __owned_link&)__it); }

template <class _Iterator>
inline bool _STLP_CALL __check_if_not_owner( const __owned_list* __owner,
                                             const _Iterator& __it)
{ return __stl_debugger::_Check_if_not_owner(__owner, (const __owned_link&)__it); }

_STLP_MOVE_TO_STD_NAMESPACE

_STLP_END_NAMESPACE

#else
#  define _STLP_VERBOSE_ASSERT(expr, diagnostic)
#  define _STLP_DEBUG_CHECK(expr)
#endif /* _STLP_DEBUG */

#if defined (_STLP_ASSERTIONS)

#  if !defined (_STLP_ASSERT_MSG_TRAILER)
#    define _STLP_ASSERT_MSG_TRAILER
#  endif

// dwa 12/30/98 - if _STLP_DEBUG_MESSAGE is defined, the user can supply own definition.
#  if !defined (_STLP_DEBUG_MESSAGE)
#    define __stl_debug_message __stl_debugger::_Message
#  else
extern  void __stl_debug_message(const char * format_str, ...);
#  endif

// fbp: if _STLP_DEBUG_TERMINATE is defined, the user can supply own definition.
#  if !defined (_STLP_DEBUG_TERMINATE)
#    define __stl_debug_terminate __stl_debugger::_Terminate
#  else
extern  void __stl_debug_terminate();
#  endif

#endif

#if defined (_STLP_ASSERTIONS) && !defined (_STLP_LINK_TIME_INSTANTIATION)
#  include <stl/debug/_debug.c>
#endif

#endif /* DEBUG_H */

// Local Variables:
// mode:C++
// End:
