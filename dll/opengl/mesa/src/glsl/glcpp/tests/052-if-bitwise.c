#if (0xaaaaaaaa | 0x55555555) != 4294967295
failure_1
#else
success_1
#endif
#if (0x12345678 ^ 0xfdecba98) == 4023971040
success_2
#else
failure_2
#endif
#if (~ 0xdeadbeef) != -3735928560
failure_3
#else
success_3
#endif
#if (0667 & 0733) == 403
success_4
#else
failure_4
#endif
