#ifndef TIMERCMP_H
#define TIMERCMP_H

#ifndef timercmp
/* Taken from sys/time.h on linux. */
# define timercmp(a, b, CMP)                                                  \
  (((a)->tv_sec == (b)->tv_sec) ?                                             \
   ((a)->tv_usec CMP (b)->tv_usec) :                                          \
   ((a)->tv_sec CMP (b)->tv_sec))

# define timerclear(a) ((a)->tv_sec = (a)->tv_usec = 0)
#endif

#endif//TIMERCMP_H
