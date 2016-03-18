#ifndef _LINUX_ATOMIC_H
#define _LINUX_ATOMIC_H

#include <linux/types.h>

//
// atomic
//

typedef struct {
    volatile int counter;
} atomic_t;

#define ATOMIC_INIT(i)	(i)

/**
 * atomic_read - read atomic variable
 * @v: pointer of type atomic_t
 *
 * Atomically reads the value of @v.  Note that the guaranteed
 * useful range of an atomic_t is only 24 bits.
 */
#define atomic_read(v)		((v)->counter)

/**
 * atomic_set - set atomic variable
 * @v: pointer of type atomic_t
 * @i: required value
 *
 * Atomically sets the value of @v to @i.  Note that the guaranteed
 * useful range of an atomic_t is only 24 bits.
 */
#define atomic_set(v,i) InterlockedExchange((PLONG)(&(v)->counter), (LONG)(i))

/**
 * atomic_add - add integer to atomic variable
 * @i: integer value to add
 * @v: pointer of type atomic_t
 *
 * Atomically adds @i to @v.  Note that the guaranteed useful range
 * of an atomic_t is only 24 bits.
 */
static inline void atomic_add(int volatile i, atomic_t volatile *v)
{
    InterlockedExchangeAdd((PLONG)(&v->counter), (LONG) i);
}

/**
 * atomic_sub - subtract the atomic variable
 * @i: integer value to subtract
 * @v: pointer of type atomic_t
 *
 * Atomically subtracts @i from @v.  Note that the guaranteed
 * useful range of an atomic_t is only 24 bits.
 */
static inline void atomic_sub(int volatile i, atomic_t volatile *v)
{
    InterlockedExchangeAdd((PLONG)(&v->counter), (LONG) (-1*i));
}

/**
 * atomic_sub_and_test - subtract value from variable and test result
 * @i: integer value to subtract
 * @v: pointer of type atomic_t
 *
 * Atomically subtracts @i from @v and returns
 * true if the result is zero, or false for all
 * other cases.  Note that the guaranteed
 * useful range of an atomic_t is only 24 bits.
 */
static inline int atomic_sub_and_test(int volatile i, atomic_t volatile *v)
{
    int counter, result;

    do {

        counter = v->counter;
        result = counter - i;

    } while ( InterlockedCompareExchange(
                  (PLONG) (&v->counter),
                  (LONG) result,
                  (LONG) counter) != counter);

    return (result == 0);
}

/**
 * atomic_inc - increment atomic variable
 * @v: pointer of type atomic_t
 *
 * Atomically increments @v by 1.  Note that the guaranteed
 * useful range of an atomic_t is only 24 bits.
 */
static inline void atomic_inc(atomic_t volatile *v)
{
    InterlockedIncrement((PLONG)(&v->counter));
}

/**
 * atomic_dec - decrement atomic variable
 * @v: pointer of type atomic_t
 *
 * Atomically decrements @v by 1.  Note that the guaranteed
 * useful range of an atomic_t is only 24 bits.
 */
static inline void atomic_dec(atomic_t volatile *v)
{
    InterlockedDecrement((PLONG)(&v->counter));
}

/**
 * atomic_dec_and_test - decrement and test
 * @v: pointer of type atomic_t
 *
 * Atomically decrements @v by 1 and
 * returns true if the result is 0, or false for all other
 * cases.  Note that the guaranteed
 * useful range of an atomic_t is only 24 bits.
 */
static inline int atomic_dec_and_test(atomic_t volatile *v)
{
    return (0 == InterlockedDecrement((PLONG)(&v->counter)));
}

/**
 * atomic_inc_and_test - increment and test
 * @v: pointer of type atomic_t
 *
 * Atomically increments @v by 1
 * and returns true if the result is zero, or false for all
 * other cases.  Note that the guaranteed
 * useful range of an atomic_t is only 24 bits.
 */
static inline int atomic_inc_and_test(atomic_t volatile *v)
{
    return (0 == InterlockedIncrement((PLONG)(&v->counter)));
}

/**
 * atomic_add_negative - add and test if negative
 * @v: pointer of type atomic_t
 * @i: integer value to add
 *
 * Atomically adds @i to @v and returns true
 * if the result is negative, or false when
 * result is greater than or equal to zero.  Note that the guaranteed
 * useful range of an atomic_t is only 24 bits.
 */
static inline int atomic_add_negative(int volatile i, atomic_t volatile *v)
{
    return (InterlockedExchangeAdd((PLONG)(&v->counter), (LONG) i) + i);
}

#endif /* LINUX_ATOMIC_H */