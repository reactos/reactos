// Bitmapped Allocator. -*- C++ -*-

// Copyright (C) 2004 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along
// with this library; see the file COPYING.  If not, write to the Free
// Software Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307,
// USA.

// As a special exception, you may use this file as part of a free software
// library without restriction.  Specifically, if other files instantiate
// templates or use macros or inline functions from this file, or you compile
// this file and link it with other files to produce an executable, this
// file does not by itself cause the resulting executable to be covered by
// the GNU General Public License.  This exception does not however
// invalidate any other reasons why the executable file might be covered by
// the GNU General Public License.



#if !defined _BITMAP_ALLOCATOR_H
#define _BITMAP_ALLOCATOR_H 1

#include <cstddef>
//For std::size_t, and ptrdiff_t.
#include <utility>
//For std::pair.
#include <algorithm>
//std::find_if, and std::lower_bound.
#include <vector>
//For the free list of exponentially growing memory blocks. At max,
//size of the vector should be  not more than the number of bits in an
//integer or an unsigned integer.
#include <functional>
//For greater_equal, and less_equal.
#include <new>
//For operator new.
#include <bits/gthr.h>
//For __gthread_mutex_t, __gthread_mutex_lock and __gthread_mutex_unlock.
#include <ext/new_allocator.h>
//For __gnu_cxx::new_allocator for std::vector.

#include <cassert>
#define NDEBUG

//#define CHECK_FOR_ERRORS
//#define __CPU_HAS_BACKWARD_BRANCH_PREDICTION

namespace __gnu_cxx
{
  namespace {
#if defined __GTHREADS
    bool const __threads_enabled = __gthread_active_p();
#endif

  }

#if defined __GTHREADS
  class _Mutex {
    __gthread_mutex_t _M_mut;
    //Prevent Copying and assignment.
    _Mutex (_Mutex const&);
    _Mutex& operator= (_Mutex const&);
  public:
    _Mutex ()
    {
      if (__threads_enabled)
	{
#if !defined __GTHREAD_MUTEX_INIT
	  __GTHREAD_MUTEX_INIT_FUNCTION(&_M_mut);
#else
	  __gthread_mutex_t __mtemp = __GTHREAD_MUTEX_INIT;
	  _M_mut = __mtemp;
#endif
	}
    }
    ~_Mutex ()
    {
      //Gthreads does not define a Mutex Destruction Function.
    }
    __gthread_mutex_t *_M_get() { return &_M_mut; }
  };

  class _Lock {
    _Mutex* _M_pmt;
    bool _M_locked;
    //Prevent Copying and assignment.
    _Lock (_Lock const&);
    _Lock& operator= (_Lock const&);
  public:
    _Lock(_Mutex* __mptr)
      : _M_pmt(__mptr), _M_locked(false)
    { this->_M_lock(); }
    void _M_lock()
    {
      if (__threads_enabled)
	{
	  _M_locked = true;
	  __gthread_mutex_lock(_M_pmt->_M_get());
	}
    }
    void _M_unlock()
    {
      if (__threads_enabled)
	{
	  if (__builtin_expect(_M_locked, true))
	    {
	      __gthread_mutex_unlock(_M_pmt->_M_get());
	      _M_locked = false;
	    }
	}
    }
    ~_Lock() { this->_M_unlock(); }
  };
#endif



  namespace __aux_balloc {
    static const unsigned int _Bits_Per_Byte = 8;
    static const unsigned int _Bits_Per_Block = sizeof(unsigned int) * _Bits_Per_Byte;

    template <typename _Addr_Pair_t>
    inline size_t __balloc_num_blocks (_Addr_Pair_t __ap)
    {
      return (__ap.second - __ap.first) + 1;
    }

    template <typename _Addr_Pair_t>
    inline size_t __balloc_num_bit_maps (_Addr_Pair_t __ap)
    {
      return __balloc_num_blocks(__ap) / _Bits_Per_Block;
    }

    //T should be a pointer type.
    template <typename _Tp>
    class _Inclusive_between : public std::unary_function<typename std::pair<_Tp, _Tp>, bool> {
      typedef _Tp pointer;
      pointer _M_ptr_value;
      typedef typename std::pair<_Tp, _Tp> _Block_pair;

    public:
      _Inclusive_between (pointer __ptr) : _M_ptr_value(__ptr) { }
      bool operator () (_Block_pair __bp) const throw ()
      {
	if (std::less_equal<pointer> ()(_M_ptr_value, __bp.second) && 
	    std::greater_equal<pointer> ()(_M_ptr_value, __bp.first))
	  return true;
	else
	  return false;
      }
    };
  
    //Used to pass a Functor to functions by reference.
    template <typename _Functor>
    class _Functor_Ref : 
      public std::unary_function<typename _Functor::argument_type, typename _Functor::result_type> {
      _Functor& _M_fref;
    
    public:
      typedef typename _Functor::argument_type argument_type;
      typedef typename _Functor::result_type result_type;

      _Functor_Ref (_Functor& __fref) : _M_fref(__fref) { }
      result_type operator() (argument_type __arg) { return _M_fref (__arg); }
    };


    //T should be a pointer type, and A is the Allocator for the vector.
    template <typename _Tp, typename _Alloc>
    class _Ffit_finder 
      : public std::unary_function<typename std::pair<_Tp, _Tp>, bool> {
      typedef typename std::vector<std::pair<_Tp, _Tp>, _Alloc> _BPVector;
      typedef typename _BPVector::difference_type _Counter_type;
      typedef typename std::pair<_Tp, _Tp> _Block_pair;

      unsigned int *_M_pbitmap;
      unsigned int _M_data_offset;

    public:
      _Ffit_finder () 
	: _M_pbitmap (0), _M_data_offset (0)
      { }

      bool operator() (_Block_pair __bp) throw()
      {
	//Set the _rover to the last unsigned integer, which is the
	//bitmap to the first free block. Thus, the bitmaps are in exact
	//reverse order of the actual memory layout. So, we count down
	//the bimaps, which is the same as moving up the memory.

	//If the used count stored at the start of the Bit Map headers
	//is equal to the number of Objects that the current Block can
	//store, then there is definitely no space for another single
	//object, so just return false.
	_Counter_type __diff = __gnu_cxx::__aux_balloc::__balloc_num_bit_maps (__bp);

	assert (*(reinterpret_cast<unsigned int*>(__bp.first) - (__diff + 1)) <= 
		__gnu_cxx::__aux_balloc::__balloc_num_blocks (__bp));

	if (*(reinterpret_cast<unsigned int*>(__bp.first) - (__diff + 1)) == 
	    __gnu_cxx::__aux_balloc::__balloc_num_blocks (__bp))
	  return false;

	unsigned int *__rover = reinterpret_cast<unsigned int*>(__bp.first) - 1;
	for (_Counter_type __i = 0; __i < __diff; ++__i)
	  {
	    _M_data_offset = __i;
	    if (*__rover)
	      {
		_M_pbitmap = __rover;
		return true;
	      }
	    --__rover;
	  }
	return false;
      }
    
      unsigned int *_M_get () { return _M_pbitmap; }
      unsigned int _M_offset () { return _M_data_offset * _Bits_Per_Block; }
    };
  
    //T should be a pointer type.
    template <typename _Tp, typename _Alloc>
    class _Bit_map_counter {
    
      typedef typename std::vector<std::pair<_Tp, _Tp>, _Alloc> _BPVector;
      typedef typename _BPVector::size_type _Index_type;
      typedef _Tp pointer;
    
      _BPVector& _M_vbp;
      unsigned int *_M_curr_bmap;
      unsigned int *_M_last_bmap_in_block;
      _Index_type _M_curr_index;
    
    public:
      //Use the 2nd parameter with care. Make sure that such an entry
      //exists in the vector before passing that particular index to
      //this ctor.
      _Bit_map_counter (_BPVector& Rvbp, int __index = -1) 
	: _M_vbp(Rvbp)
      {
	this->_M_reset(__index);
      }
    
      void _M_reset (int __index = -1) throw()
      {
	if (__index == -1)
	  {
	    _M_curr_bmap = 0;
	    _M_curr_index = (_Index_type)-1;
	    return;
	  }

	_M_curr_index = __index;
	_M_curr_bmap = reinterpret_cast<unsigned int*>(_M_vbp[_M_curr_index].first) - 1;

	assert (__index <= (int)_M_vbp.size() - 1);
	
	_M_last_bmap_in_block = _M_curr_bmap - 
	  ((_M_vbp[_M_curr_index].second - _M_vbp[_M_curr_index].first + 1) / _Bits_Per_Block - 1);
      }
    
      //Dangerous Function! Use with extreme care. Pass to this
      //function ONLY those values that are known to be correct,
      //otherwise this will mess up big time.
      void _M_set_internal_bit_map (unsigned int *__new_internal_marker) throw()
      {
	_M_curr_bmap = __new_internal_marker;
      }
    
      bool _M_finished () const throw()
      {
	return (_M_curr_bmap == 0);
      }
    
      _Bit_map_counter& operator++ () throw()
      {
	if (_M_curr_bmap == _M_last_bmap_in_block)
	  {
	    if (++_M_curr_index == _M_vbp.size())
	      {
		_M_curr_bmap = 0;
	      }
	    else
	      {
		this->_M_reset (_M_curr_index);
	      }
	  }
	else
	  {
	    --_M_curr_bmap;
	  }
	return *this;
      }
    
      unsigned int *_M_get ()
      {
	return _M_curr_bmap;
      }
    
      pointer _M_base () { return _M_vbp[_M_curr_index].first; }
      unsigned int _M_offset ()
      {
	return _Bits_Per_Block * ((reinterpret_cast<unsigned int*>(this->_M_base()) - _M_curr_bmap) - 1);
      }
    
      unsigned int _M_where () { return _M_curr_index; }
    };
  }

  //Generic Version of the bsf instruction.
  typedef unsigned int _Bit_map_type;
  static inline unsigned int _Bit_scan_forward (register _Bit_map_type __num)
  {
    return static_cast<unsigned int>(__builtin_ctz(__num));
  }

  struct _OOM_handler {
    static std::new_handler _S_old_handler;
    static bool _S_handled_oom;
    typedef void (*_FL_clear_proc)(void);
    static _FL_clear_proc _S_oom_fcp;
    
    _OOM_handler (_FL_clear_proc __fcp)
    {
      _S_oom_fcp = __fcp;
      _S_old_handler = std::set_new_handler (_S_handle_oom_proc);
      _S_handled_oom = false;
    }

    static void _S_handle_oom_proc()
    {
      _S_oom_fcp();
      std::set_new_handler (_S_old_handler);
      _S_handled_oom = true;
    }

    ~_OOM_handler ()
    {
      if (!_S_handled_oom)
	std::set_new_handler (_S_old_handler);
    }
  };
  
  std::new_handler _OOM_handler::_S_old_handler;
  bool _OOM_handler::_S_handled_oom = false;
  _OOM_handler::_FL_clear_proc _OOM_handler::_S_oom_fcp = 0;
  

  class _BA_free_list_store {
    struct _LT_pointer_compare {
      template <typename _Tp>
      bool operator() (_Tp* __pt, _Tp const& __crt) const throw()
      {
	return *__pt < __crt;
      }
    };

#if defined __GTHREADS
    static _Mutex _S_bfl_mutex;
#endif
    static std::vector<unsigned int*> _S_free_list;
    typedef std::vector<unsigned int*>::iterator _FLIter;

    static void _S_validate_free_list(unsigned int *__addr) throw()
    {
      const unsigned int __max_size = 64;
      if (_S_free_list.size() >= __max_size)
	{
	  //Ok, the threshold value has been reached.
	  //We determine which block to remove from the list of free
	  //blocks.
	  if (*__addr >= *_S_free_list.back())
	    {
	      //Ok, the new block is greater than or equal to the last
	      //block in the list of free blocks. We just free the new
	      //block.
	      operator delete((void*)__addr);
	      return;
	    }
	  else
	    {
	      //Deallocate the last block in the list of free lists, and
	      //insert the new one in it's correct position.
	      operator delete((void*)_S_free_list.back());
	      _S_free_list.pop_back();
	    }
	}
	  
      //Just add the block to the list of free lists
      //unconditionally.
      _FLIter __temp = std::lower_bound(_S_free_list.begin(), _S_free_list.end(), 
					*__addr, _LT_pointer_compare ());
      //We may insert the new free list before _temp;
      _S_free_list.insert(__temp, __addr);
    }

    static bool _S_should_i_give(unsigned int __block_size, unsigned int __required_size) throw()
    {
      const unsigned int __max_wastage_percentage = 36;
      if (__block_size >= __required_size && 
	  (((__block_size - __required_size) * 100 / __block_size) < __max_wastage_percentage))
	return true;
      else
	return false;
    }

  public:
    typedef _BA_free_list_store _BFL_type;

    static inline void _S_insert_free_list(unsigned int *__addr) throw()
    {
#if defined __GTHREADS
      _Lock __bfl_lock(&_S_bfl_mutex);
#endif
      //Call _S_validate_free_list to decide what should be done with this
      //particular free list.
      _S_validate_free_list(--__addr);
    }
    
    static unsigned int *_S_get_free_list(unsigned int __sz) throw (std::bad_alloc)
    {
#if defined __GTHREADS
      _Lock __bfl_lock(&_S_bfl_mutex);
#endif
      _FLIter __temp = std::lower_bound(_S_free_list.begin(), _S_free_list.end(), 
					__sz, _LT_pointer_compare());
      if (__temp == _S_free_list.end() || !_S_should_i_give (**__temp, __sz))
	{
	  //We hold the lock because the OOM_Handler is a stateless
	  //entity.
	  _OOM_handler __set_handler(_BFL_type::_S_clear);
	  unsigned int *__ret_val = reinterpret_cast<unsigned int*>
	    (operator new (__sz + sizeof(unsigned int)));
	  *__ret_val = __sz;
	  return ++__ret_val;
	}
      else
	{
	  unsigned int* __ret_val = *__temp;
	  _S_free_list.erase (__temp);
	  return ++__ret_val;
	}
    }

    //This function just clears the internal Free List, and gives back
    //all the memory to the OS.
    static void _S_clear()
    {
#if defined __GTHREADS
      _Lock __bfl_lock(&_S_bfl_mutex);
#endif
      _FLIter __iter = _S_free_list.begin();
      while (__iter != _S_free_list.end())
	{
	  operator delete((void*)*__iter);
	  ++__iter;
	}
      _S_free_list.clear();
    }

  };

#if defined __GTHREADS
  _Mutex _BA_free_list_store::_S_bfl_mutex;
#endif
  std::vector<unsigned int*> _BA_free_list_store::_S_free_list;

  template <typename _Tp> class bitmap_allocator;
  // specialize for void:
  template <> class bitmap_allocator<void> {
  public:
    typedef void*       pointer;
    typedef const void* const_pointer;
    //  reference-to-void members are impossible.
    typedef void  value_type;
    template <typename _Tp1> struct rebind { typedef bitmap_allocator<_Tp1> other; };
  };

  template <typename _Tp> class bitmap_allocator : private _BA_free_list_store {
  public:
    typedef size_t    size_type;
    typedef ptrdiff_t difference_type;
    typedef _Tp*        pointer;
    typedef const _Tp*  const_pointer;
    typedef _Tp&        reference;
    typedef const _Tp&  const_reference;
    typedef _Tp         value_type;
    template <typename _Tp1> struct rebind { typedef bitmap_allocator<_Tp1> other; };

  private:
    static const unsigned int _Bits_Per_Byte = 8;
    static const unsigned int _Bits_Per_Block = sizeof(unsigned int) * _Bits_Per_Byte;

    static inline void _S_bit_allocate(unsigned int *__pbmap, unsigned int __pos) throw()
    {
      unsigned int __mask = 1 << __pos;
      __mask = ~__mask;
      *__pbmap &= __mask;
    }
  
    static inline void _S_bit_free(unsigned int *__pbmap, unsigned int __pos) throw()
    {
      unsigned int __mask = 1 << __pos;
      *__pbmap |= __mask;
    }

    static inline void *_S_memory_get(size_t __sz) throw (std::bad_alloc)
    {
      return operator new(__sz);
    }

    static inline void _S_memory_put(void *__vptr) throw ()
    {
      operator delete(__vptr);
    }

    typedef typename std::pair<pointer, pointer> _Block_pair;
    typedef typename __gnu_cxx::new_allocator<_Block_pair> _BPVec_allocator_type;
    typedef typename std::vector<_Block_pair, _BPVec_allocator_type> _BPVector;


#if defined CHECK_FOR_ERRORS
    //Complexity: O(lg(N)). Where, N is the number of block of size
    //sizeof(value_type).
    static void _S_check_for_free_blocks() throw()
    {
      typedef typename __gnu_cxx::__aux_balloc::_Ffit_finder<pointer, _BPVec_allocator_type> _FFF;
      _FFF __fff;
      typedef typename _BPVector::iterator _BPiter;
      _BPiter __bpi = std::find_if(_S_mem_blocks.begin(), _S_mem_blocks.end(), 
				   __gnu_cxx::__aux_balloc::_Functor_Ref<_FFF>(__fff));
      assert(__bpi == _S_mem_blocks.end());
    }
#endif


    //Complexity: O(1), but internally depends upon the complexity of
    //the function _BA_free_list_store::_S_get_free_list. The part
    //where the bitmap headers are written is of worst case complexity:
    //O(X),where X is the number of blocks of size sizeof(value_type)
    //within the newly acquired block. Having a tight bound.
    static void _S_refill_pool() throw (std::bad_alloc)
    {
#if defined CHECK_FOR_ERRORS
      _S_check_for_free_blocks();
#endif

      const unsigned int __num_bit_maps = _S_block_size / _Bits_Per_Block;
      const unsigned int __size_to_allocate = sizeof(unsigned int) + 
	_S_block_size * sizeof(value_type) + __num_bit_maps*sizeof(unsigned int);

      unsigned int *__temp = 
	reinterpret_cast<unsigned int*>(_BA_free_list_store::_S_get_free_list(__size_to_allocate));
      *__temp = 0;
      ++__temp;

      //The Header information goes at the Beginning of the Block.
      _Block_pair __bp = std::make_pair(reinterpret_cast<pointer>(__temp + __num_bit_maps), 
				       reinterpret_cast<pointer>(__temp + __num_bit_maps) 
					+ _S_block_size - 1);

      //Fill the Vector with this information.
      _S_mem_blocks.push_back(__bp);

      unsigned int __bit_mask = 0; //0 Indicates all Allocated.
      __bit_mask = ~__bit_mask; //1 Indicates all Free.

      for (unsigned int __i = 0; __i < __num_bit_maps; ++__i)
	__temp[__i] = __bit_mask;

      //On some implementations, operator new might throw bad_alloc, or
      //malloc might fail if the size passed is too large, therefore, we
      //limit the size passed to malloc or operator new.
      _S_block_size *= 2;
    }

    static _BPVector _S_mem_blocks;
    static unsigned int _S_block_size;
    static __gnu_cxx::__aux_balloc::_Bit_map_counter<pointer, _BPVec_allocator_type> _S_last_request;
    static typename _BPVector::size_type _S_last_dealloc_index;
#if defined __GTHREADS
    static _Mutex _S_mut;
#endif

    //Complexity: Worst case complexity is O(N), but that is hardly ever
    //hit. if and when this particular case is encountered, the next few
    //cases are guaranteed to have a worst case complexity of O(1)!
    //That's why this function performs very well on the average. you
    //can consider this function to be having a complexity refrred to
    //commonly as: Amortized Constant time.
    static pointer _S_allocate_single_object()
    {
#if defined __GTHREADS
      _Lock __bit_lock(&_S_mut);
#endif

      //The algorithm is something like this: The last_requst variable
      //points to the last accessed Bit Map. When such a condition
      //occurs, we try to find a free block in the current bitmap, or
      //succeeding bitmaps until the last bitmap is reached. If no free
      //block turns up, we resort to First Fit method.

      //WARNING: Do not re-order the condition in the while statement
      //below, because it relies on C++'s short-circuit
      //evaluation. The return from _S_last_request->_M_get() will NOT
      //be dereferenceable if _S_last_request->_M_finished() returns
      //true. This would inevitibly lead to a NULL pointer dereference
      //if tinkered with.
      while (_S_last_request._M_finished() == false && (*(_S_last_request._M_get()) == 0))
	{
	  _S_last_request.operator++();
	}

      if (__builtin_expect(_S_last_request._M_finished() == true, false))
	{
	  //Fall Back to First Fit algorithm.
	  typedef typename __gnu_cxx::__aux_balloc::_Ffit_finder<pointer, _BPVec_allocator_type> _FFF;
	  _FFF __fff;
	  typedef typename _BPVector::iterator _BPiter;
	  _BPiter __bpi = std::find_if(_S_mem_blocks.begin(), _S_mem_blocks.end(), 
				      __gnu_cxx::__aux_balloc::_Functor_Ref<_FFF>(__fff));

	  if (__bpi != _S_mem_blocks.end())
	    {
	      //Search was successful. Ok, now mark the first bit from
	      //the right as 0, meaning Allocated. This bit is obtained
	      //by calling _M_get() on __fff.
	      unsigned int __nz_bit = _Bit_scan_forward(*__fff._M_get());
	      _S_bit_allocate(__fff._M_get(), __nz_bit);

	      _S_last_request._M_reset(__bpi - _S_mem_blocks.begin());

	      //Now, get the address of the bit we marked as allocated.
	      pointer __ret_val = __bpi->first + __fff._M_offset() + __nz_bit;
	      unsigned int *__puse_count = reinterpret_cast<unsigned int*>(__bpi->first) - 
		(__gnu_cxx::__aux_balloc::__balloc_num_bit_maps(*__bpi) + 1);
	      ++(*__puse_count);
	      return __ret_val;
	    }
	  else
	    {
	      //Search was unsuccessful. We Add more memory to the pool
	      //by calling _S_refill_pool().
	      _S_refill_pool();

	      //_M_Reset the _S_last_request structure to the first free
	      //block's bit map.
	      _S_last_request._M_reset(_S_mem_blocks.size() - 1);

	      //Now, mark that bit as allocated.
	    }
	}
      //_S_last_request holds a pointer to a valid bit map, that points
      //to a free block in memory.
      unsigned int __nz_bit = _Bit_scan_forward(*_S_last_request._M_get());
      _S_bit_allocate(_S_last_request._M_get(), __nz_bit);

      pointer __ret_val = _S_last_request._M_base() + _S_last_request._M_offset() + __nz_bit;

      unsigned int *__puse_count = reinterpret_cast<unsigned int*>
	(_S_mem_blocks[_S_last_request._M_where()].first) - 
	(__gnu_cxx::__aux_balloc::__balloc_num_bit_maps(_S_mem_blocks[_S_last_request._M_where()]) + 1);
      ++(*__puse_count);
      return __ret_val;
    }

    //Complexity: O(lg(N)), but the worst case is hit quite often! I
    //need to do something about this. I'll be able to work on it, only
    //when I have some solid figures from a few real apps.
    static void _S_deallocate_single_object(pointer __p) throw()
    {
#if defined __GTHREADS
      _Lock __bit_lock(&_S_mut);
#endif

      typedef typename _BPVector::iterator _Iterator;
      typedef typename _BPVector::difference_type _Difference_type;

      _Difference_type __diff;
      int __displacement;

      assert(_S_last_dealloc_index >= 0);

      if (__gnu_cxx::__aux_balloc::_Inclusive_between<pointer>(__p)(_S_mem_blocks[_S_last_dealloc_index]))
	{
	  assert(_S_last_dealloc_index <= _S_mem_blocks.size() - 1);

	  //Initial Assumption was correct!
	  __diff = _S_last_dealloc_index;
	  __displacement = __p - _S_mem_blocks[__diff].first;
	}
      else
	{
	  _Iterator _iter = (std::find_if(_S_mem_blocks.begin(), _S_mem_blocks.end(), 
					  __gnu_cxx::__aux_balloc::_Inclusive_between<pointer>(__p)));
	  assert(_iter != _S_mem_blocks.end());

	  __diff = _iter - _S_mem_blocks.begin();
	  __displacement = __p - _S_mem_blocks[__diff].first;
	  _S_last_dealloc_index = __diff;
	}

      //Get the position of the iterator that has been found.
      const unsigned int __rotate = __displacement % _Bits_Per_Block;
      unsigned int *__bit_mapC = reinterpret_cast<unsigned int*>(_S_mem_blocks[__diff].first) - 1;
      __bit_mapC -= (__displacement / _Bits_Per_Block);
      
      _S_bit_free(__bit_mapC, __rotate);
      unsigned int *__puse_count = reinterpret_cast<unsigned int*>
	(_S_mem_blocks[__diff].first) - 
	(__gnu_cxx::__aux_balloc::__balloc_num_bit_maps(_S_mem_blocks[__diff]) + 1);

      assert(*__puse_count != 0);

      --(*__puse_count);

      if (__builtin_expect(*__puse_count == 0, false))
	{
	  _S_block_size /= 2;
	  
	  //We may safely remove this block.
	  _Block_pair __bp = _S_mem_blocks[__diff];
	  _S_insert_free_list(__puse_count);
	  _S_mem_blocks.erase(_S_mem_blocks.begin() + __diff);

	  //We reset the _S_last_request variable to reflect the erased
	  //block. We do this to protect future requests after the last
	  //block has been removed from a particular memory Chunk,
	  //which in turn has been returned to the free list, and
	  //hence had been erased from the vector, so the size of the
	  //vector gets reduced by 1.
	  if ((_Difference_type)_S_last_request._M_where() >= __diff--)
	    {
	      _S_last_request._M_reset(__diff);
	      //	      assert(__diff >= 0);
	    }

	  //If the Index into the vector of the region of memory that
	  //might hold the next address that will be passed to
	  //deallocated may have been invalidated due to the above
	  //erase procedure being called on the vector, hence we try
	  //to restore this invariant too.
	  if (_S_last_dealloc_index >= _S_mem_blocks.size())
	    {
	      _S_last_dealloc_index =(__diff != -1 ? __diff : 0);
	      assert(_S_last_dealloc_index >= 0);
	    }
	}
    }

  public:
    bitmap_allocator() throw()
    { }

    bitmap_allocator(const bitmap_allocator&) { }

    template <typename _Tp1> bitmap_allocator(const bitmap_allocator<_Tp1>&) throw()
    { }

    ~bitmap_allocator() throw()
    { }

    //Complexity: O(1), but internally the complexity depends upon the
    //complexity of the function(s) _S_allocate_single_object and
    //_S_memory_get.
    pointer allocate(size_type __n)
    {
      if (__builtin_expect(__n == 1, true))
	return _S_allocate_single_object();
      else
	return reinterpret_cast<pointer>(_S_memory_get(__n * sizeof(value_type)));
    }

    //Complexity: Worst case complexity is O(N) where N is the number of
    //blocks of size sizeof(value_type) within the free lists that the
    //allocator holds. However, this worst case is hit only when the
    //user supplies a bogus argument to hint. If the hint argument is
    //sensible, then the complexity drops to O(lg(N)), and in extreme
    //cases, even drops to as low as O(1). So, if the user supplied
    //argument is good, then this function performs very well.
    pointer allocate(size_type __n, typename bitmap_allocator<void>::const_pointer)
    {
      return allocate(__n);
    }

    void deallocate(pointer __p, size_type __n) throw()
    {
      if (__builtin_expect(__n == 1, true))
	_S_deallocate_single_object(__p);
      else
	_S_memory_put(__p);
    }

    pointer address(reference r) const { return &r; }
    const_pointer address(const_reference r) const { return &r; }

    size_type max_size(void) const throw() { return (size_type()-1)/sizeof(value_type); }

    void construct (pointer p, const_reference __data)
    {
      ::new(p) value_type(__data);
    }

    void destroy (pointer p)
    {
      p->~value_type();
    }

  };

  template <typename _Tp>
  typename bitmap_allocator<_Tp>::_BPVector bitmap_allocator<_Tp>::_S_mem_blocks;

  template <typename _Tp>
  unsigned int bitmap_allocator<_Tp>::_S_block_size = bitmap_allocator<_Tp>::_Bits_Per_Block;

  template <typename _Tp>
  typename __gnu_cxx::bitmap_allocator<_Tp>::_BPVector::size_type 
  bitmap_allocator<_Tp>::_S_last_dealloc_index = 0;

  template <typename _Tp>
  __gnu_cxx::__aux_balloc::_Bit_map_counter 
  <typename bitmap_allocator<_Tp>::pointer, typename bitmap_allocator<_Tp>::_BPVec_allocator_type> 
  bitmap_allocator<_Tp>::_S_last_request(_S_mem_blocks);

#if defined __GTHREADS
  template <typename _Tp>
  __gnu_cxx::_Mutex
  bitmap_allocator<_Tp>::_S_mut;
#endif

  template <typename _Tp1, typename _Tp2>
  bool operator== (const bitmap_allocator<_Tp1>&, const bitmap_allocator<_Tp2>&) throw()
  {
    return true;
  }
  
  template <typename _Tp1, typename _Tp2>
  bool operator!= (const bitmap_allocator<_Tp1>&, const bitmap_allocator<_Tp2>&) throw()
  {
    return false;
  }
}


#endif //_BITMAP_ALLOCATOR_H
