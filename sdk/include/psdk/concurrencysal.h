/*
 * concurrencysal.h
 *
 * Standard Annotation Language (SAL) definitions for synchronisation
 *
 * This file is part of the ReactOS PSDK package.
 *
 * Contributors:
 *   Timo Kreuzer (timo.kreuzer@reactos.org)
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#if defined(_PREFAST_)

#define _Benign_race_begin_ __pragma(warning(push)) __pragma(warning(disable:26100 26101 26150 26130 26180 26131 26181 28112))
#define _Benign_race_end_ __pragma(warning(pop))
#define _No_competing_thread_begin_ __pragma(warning(push)) __pragma(warning(disable:26100 26101 26150 26101 26151 26110 26160 26130 26180 26131 26181 28112))
#define _No_competing_thread_end_ __pragma(warning(pop))

#define _Acquires_exclusive_lock_(lock)
#define _Acquires_lock_(lock)
#define _Acquires_nonreentrant_lock_(lock)
#define _Acquires_shared_lock_(lock)
#define _Analysis_assume_lock_acquired_(lock)
#define _Analysis_assume_lock_released_(lock)
#define _Analysis_assume_lock_held_(lock)
#define _Analysis_assume_lock_not_held_(lock)
#define _Analysis_assume_same_lock_(lock1, lock2)
#define _Analysis_suppress_lock_checking_(lock)
#define _Create_lock_level_(level)
#define _Csalcat1_(x,y)
#define _Csalcat2_(x,y)
#define _Function_ignore_lock_checking_(lock)
#define _Guarded_by_(lock)
#define _Has_lock_kind_(kind)
#define _Has_lock_level_(level)
#define _Interlocked_
#define _Internal_lock_level_order_(a,b)
#define _Lock_level_order_(a,b)
#define _No_competing_thread_
#define _Post_same_lock_(lock1,lock2)
#define _Releases_exclusive_lock_(lock)
#define _Releases_lock_(lock)
#define _Releases_nonreentrant_lock_(lock)
#define _Releases_shared_lock_(lock)
#define _Requires_exclusive_lock_held_(lock)
#define _Requires_shared_lock_held_(lock)
#define _Requires_lock_held_(lock)
#define _Requires_lock_not_held_(lock)
#define _Requires_no_locks_held_
#define _Write_guarded_by_(lock)


const char _Lock_kind_mutex_[] = "";
const char _Lock_kind_event_[] = "";
const char _Lock_kind_semaphore_[] = "";
const char _Lock_kind_spin_lock_[] = "";
const char _Lock_kind_critical_section_[] = "";

#else /* _PREFAST_ */

#define _Benign_race_begin_ __pragma(warning(push))
#define _Benign_race_end_ __pragma(warning(pop))
#define _No_competing_thread_begin_ __pragma(warning(push))
#define _No_competing_thread_end_ __pragma(warning(pop))

#define _Acquires_exclusive_lock_(lock)
#define _Acquires_lock_(lock)
#define _Acquires_nonreentrant_lock_(lock)
#define _Acquires_shared_lock_(lock)
#define _Analysis_assume_lock_acquired_(lock)
#define _Analysis_assume_lock_released_(lock)
#define _Analysis_assume_lock_held_(lock)
#define _Analysis_assume_lock_not_held_(lock)
#define _Analysis_assume_same_lock_(lock1, lock2)
#define _Analysis_suppress_lock_checking_(lock)
#define _Create_lock_level_(level)
#define _Csalcat1_(x,y)
#define _Csalcat2_(x,y)
#define _Function_ignore_lock_checking_(lock)
#define _Guarded_by_(lock)
#define _Has_lock_kind_(kind)
#define _Has_lock_level_(level)
#define _Interlocked_
#define _Internal_lock_level_order_(a,b)
#define _Lock_level_order_(a,b)
#define _No_competing_thread_
#define _Post_same_lock_(lock1,lock2)
#define _Releases_exclusive_lock_(lock)
#define _Releases_lock_(lock)
#define _Releases_nonreentrant_lock_(lock)
#define _Releases_shared_lock_(lock)
#define _Requires_exclusive_lock_held_(lock)
#define _Requires_shared_lock_held_(lock)
#define _Requires_lock_held_(lock)
#define _Requires_lock_not_held_(lock)
#define _Requires_no_locks_held_
#define _Write_guarded_by_(lock)

#endif /* _PREFAST_ */

#if 0 /* Check these */
#define _Internal_set_lock_count_(lock, count)
#define _Internal_set_lock_count_to_zero_(lock)
#define _Internal_set_lock_count_to_one_(lock)
#endif // 0

