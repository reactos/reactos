#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_X86_) || defined(_AMD64_)
uint32_t __stdcall calc_crc32c_hw(uint32_t seed, uint8_t* msg, uint32_t msglen);
#endif

uint32_t __stdcall calc_crc32c_sw(uint32_t seed, uint8_t* msg, uint32_t msglen);

typedef uint32_t (__stdcall *crc_func)(uint32_t seed, uint8_t* msg, uint32_t msglen);

extern crc_func calc_crc32c;

#ifdef __cplusplus
}
#endif
