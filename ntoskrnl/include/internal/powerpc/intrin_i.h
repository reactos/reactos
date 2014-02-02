#pragma once

#define Ke386SaveFlags(x) __asm__ __volatile__("mfmsr %0" : "=r" (x) :)

/* EOF */
