
// Currently, SUN CC requires object file

#if defined (__GNUC__)

/*
**  int _STLP_atomic_exchange (__stl_atomic_t *pvalue, __stl_atomic_t value)
*/

#  if defined(__sparc_v9__) || defined (__sparcv9) 

#    ifdef __arch64__

#      define _STLP_EXCH_ASM  asm volatile ("casx [%3], %4, %0 ;  membar  #LoadLoad | #LoadStore " : \
                   "=r" (_L_value2), "=m" (*_L_pvalue1) : \
                   "m" (*_L_pvalue1), "r" (_L_pvalue1), "r" (_L_value1), "0" (_L_value2) )

#    else /* __arch64__ */

#      define _STLP_EXCH_ASM  asm volatile ("cas [%3], %4, %0" : \
                   "=r" (_L_value2), "=m" (*_L_pvalue1) : \
                   "m" (*_L_pvalue1), "r" (_L_pvalue1), "r" (_L_value1), "0" (_L_value2) )
#    endif

#  else /* __sparc_v9__ */

#    define _STLP_EXCH_ASM asm volatile ("swap [%3], %0 " : \
                                       "=r" (_L_value2), "=m" (*_L_pvalue1) : \
                                       "m" (*_L_pvalue1), "r" (_L_pvalue1),  "0" (_L_value2) )
#  endif


#  define _STLP_ATOMIC_EXCHANGE(__pvalue1, __value2) \
 ({  register volatile __stl_atomic_t *_L_pvalue1 = __pvalue1; \
     register __stl_atomic_t _L_value1, _L_value2 =  __value2 ; \
     do { _L_value1 = *_L_pvalue1; _STLP_EXCH_ASM; } while ( _L_value1 != _L_value2 ) ; \
     _L_value1; })

#  define _STLP_ATOMIC_INCREMENT(__pvalue1) \
 ({  register volatile __stl_atomic_t *_L_pvalue1 = __pvalue1; \
    register __stl_atomic_t _L_value1, _L_value2; \
    do { _L_value1 = *_L_pvalue1;  _L_value2 = _L_value1+1; _STLP_EXCH_ASM; } while ( _L_value1 != _L_value2 ) ; \
    (_L_value2 + 1); })

#  define _STLP_ATOMIC_DECREMENT(__pvalue1) \
 ({  register volatile __stl_atomic_t *_L_pvalue1 = __pvalue1; \
    register __stl_atomic_t _L_value1, _L_value2; \
    do { _L_value1 = *_L_pvalue1;  _L_value2 = _L_value1-1; _STLP_EXCH_ASM; } while ( _L_value1 != _L_value2 ) ; \
    (_L_value2 - 1); })

#  elif ! defined (_STLP_NO_EXTERN_INLINE)

extern "C" __stl_atomic_t _STLP_atomic_exchange(__stl_atomic_t * __x, __stl_atomic_t __v);
extern "C" void _STLP_atomic_decrement(__stl_atomic_t* i);
extern "C" void _STLP_atomic_increment(__stl_atomic_t* i);

#    define _STLP_ATOMIC_INCREMENT(__x)           _STLP_atomic_increment((__stl_atomic_t*)__x)
#    define _STLP_ATOMIC_DECREMENT(__x)           _STLP_atomic_decrement((__stl_atomic_t*)__x)
#    define _STLP_ATOMIC_EXCHANGE(__x, __y)       _STLP_atomic_exchange((__stl_atomic_t*)__x, (__stl_atomic_t)__y)

#endif
