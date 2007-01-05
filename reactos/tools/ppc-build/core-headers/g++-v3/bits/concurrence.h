// Support for concurrent programing -*- C++ -*-

// Copyright (C) 2003, 2004
// Free Software Foundation, Inc.
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

#ifndef _CONCURRENCE_H
#define _CONCURRENCE_H 1

// GCC's thread abstraction layer
#include "bits/gthr.h"

#if __GTHREADS

# ifdef __GTHREAD_MUTEX_INIT
#  define __glibcxx_mutex_type __gthread_mutex_t
#  define __glibcxx_mutex_define_initialized(NAME) \
__gthread_mutex_t NAME = __GTHREAD_MUTEX_INIT
#  define __glibcxx_mutex_lock(NAME) \
__gthread_mutex_lock(&NAME)
# else
// Implies __GTHREAD_MUTEX_INIT_FUNCTION
struct __glibcxx_mutex : public __gthread_mutex_t
{
   __glibcxx_mutex() { __GTHREAD_MUTEX_INIT_FUNCTION(this); }
};

#  define __glibcxx_mutex_type __glibcxx_mutex
#  define __glibcxx_mutex_define_initialized(NAME) \
__glibcxx_mutex NAME
# define __glibcxx_mutex_lock(NAME) \
__gthread_mutex_lock(&NAME)
# endif

# define __glibcxx_mutex_unlock(NAME) __gthread_mutex_unlock(&NAME)

#else

# define __glibcxx_mutex_type __gthread_mutex_t
# define __glibcxx_mutex_define_initialized(NAME) __gthread_mutex_t NAME
# define __glibcxx_mutex_lock(NAME)
# define __glibcxx_mutex_unlock(NAME)

#endif

namespace __gnu_cxx
{
  typedef __glibcxx_mutex_type mutex_type;
  
  // Scoped lock idiom.
  // Acquire the mutex here with a constructor call, then release with
  // the destructor call in accordance with RAII style.
   class lock
  {
    // Externally defined and initialized.
    mutex_type& device;

  public:
    explicit lock(mutex_type& name) : device(name)
    { __glibcxx_mutex_lock(device); }

    ~lock() throw()
    { __glibcxx_mutex_unlock(device); }

  private:
    lock(const lock&);
    lock& operator=(const lock&);
  };
}

#endif
