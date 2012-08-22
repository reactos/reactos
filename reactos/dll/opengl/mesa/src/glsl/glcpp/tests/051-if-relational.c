#if 3 < 2
failure_1
#else
success_1
#endif

#if 3 >= 2
success_2
#else
failure_2
#endif

#if 2 + 3 <= 5
success_3
#else
failure_3
#endif

#if 3 - 2 == 1
success_3
#else
failure_3
#endif

#if 1 > 3
failure_4
#else
success_4
#endif

#if 1 != 5
success_5
#else
failure_5
#endif
