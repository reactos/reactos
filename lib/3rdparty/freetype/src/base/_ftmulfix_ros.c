  FT_EXPORT_DEF( FT_Long )
  FT_MulFix( FT_Long  a,
             FT_Long  b )
  {
    /* use inline assembly to speed up things a bit */

#if defined( __GNUC__ ) && defined( i386 )

    FT_Long  result;


    __asm__ __volatile__ (
      "imul  %%edx\n"
      "movl  %%edx, %%ecx\n"
      "sarl  $31, %%ecx\n"
      "addl  $0x8000, %%ecx\n"
      "addl  %%ecx, %%eax\n"
      "adcl  $0, %%edx\n"
      "shrl  $16, %%eax\n"
      "shll  $16, %%edx\n"
      "addl  %%edx, %%eax\n"
      "mov   %%eax, %0\n"
      : "=r"(result), "=d"(b)
      : "a"(a), "d"(b)
      : "%ecx"
    );
    return result;

#elif 1

    FT_Long   sa, sb;
    FT_ULong  ua, ub;


    if ( a == 0 || b == 0x10000L )
      return a;

    sa = ( a >> ( sizeof ( a ) * 8 - 1 ) );
    a  = ( a ^ sa ) - sa;
    sb = ( b >> ( sizeof ( b ) * 8 - 1 ) );
    b  = ( b ^ sb ) - sb;

    ua = (FT_ULong)a;
    ub = (FT_ULong)b;

    if ( ua <= 2048 && ub <= 1048576L )
      ua = ( ua * ub + 0x8000U ) >> 16;
    else
    {
      FT_ULong  al = ua & 0xFFFFU;


      ua = ( ua >> 16 ) * ub +  al * ( ub >> 16 ) +
           ( ( al * ( ub & 0xFFFFU ) + 0x8000U ) >> 16 );
    }

    sa ^= sb,
    ua  = (FT_ULong)(( ua ^ sa ) - sa);

    return (FT_Long)ua;

#else /* 0 */

    FT_Long   s;
    FT_ULong  ua, ub;


    if ( a == 0 || b == 0x10000L )
      return a;

    s  = a; a = FT_ABS( a );
    s ^= b; b = FT_ABS( b );

    ua = (FT_ULong)a;
    ub = (FT_ULong)b;

    if ( ua <= 2048 && ub <= 1048576L )
      ua = ( ua * ub + 0x8000UL ) >> 16;
    else
    {
      FT_ULong  al = ua & 0xFFFFUL;


      ua = ( ua >> 16 ) * ub +  al * ( ub >> 16 ) +
           ( ( al * ( ub & 0xFFFFUL ) + 0x8000UL ) >> 16 );
    }

    return ( s < 0 ? -(FT_Long)ua : (FT_Long)ua );

#endif /* 0 */

  }
