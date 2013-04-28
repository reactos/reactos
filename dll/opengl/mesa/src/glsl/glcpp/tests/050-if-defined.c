#if defined foo
failure_1
#else
success_1
#endif
#define foo
#if defined foo
success_2
#else
failure_2
#endif
#undef foo
#if defined foo
failure_3
#else
success_3
#endif
