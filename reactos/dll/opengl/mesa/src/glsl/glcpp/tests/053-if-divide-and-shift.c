#if (15 / 2) != 7
failure_1
#else
success_1
#endif
#if (1 << 12) == 4096
success_2
#else
failure_2
#endif
#if (31762 >> 8) != 124
failure_3
#else
success_3
#endif
