#define one 1
#define two 2
#define three 3
#define five 5
#if five < two
failure_1
#else
success_1
#endif
#if three >= two
success_2
#else
failure_2
#endif
#if two + three <= five
success_3
#else
failure_3
#endif
#if five - two == three
success_4
#else
failure_4
#endif
#if one > three
failure_5
#else
success_5
#endif
#if one != five
success_6
#else
failure_6
#endif
