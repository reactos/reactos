#ifndef _IEEE_H
#define _IEEE_H

typedef struct {
  unsigned int mantissa:23;
  unsigned int exponent:8;
  unsigned int sign:1;
} float_t;

typedef struct {
  unsigned int mantissal:32;
  unsigned int mantissah:20;
  unsigned int exponent:11;
  unsigned int sign:1;
} double_t;

typedef struct {
  unsigned int mantissal:32;
  unsigned int mantissah:32;
  unsigned int exponent:15;
  unsigned int sign:1;
} long_double_t;

#endif