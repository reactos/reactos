#ifndef COMPDVR_UTIL_H
#define COMPDVR_UTIL_H

#include <utility>
#include <stdint.h>

void le16write(uint8_t *dataptr, uint16_t value);
void le16write_postinc(uint8_t *&dataptr, uint16_t value);
void le32write(uint8_t *dataptr, uint32_t value);
void le32write_postinc(uint8_t *&dataptr, uint32_t value);
uint16_t le16read(uint8_t *dataptr);
uint16_t le16read_postinc(uint8_t *&dataptr);
uint32_t le32read(uint8_t *dataptr);
uint32_t le32read_postinc(uint8_t *&dataptr);
uint16_t be16read(uint8_t *dataptr);
uint16_t be16read_postinc(uint8_t *&dataptr);
uint32_t be32read(uint8_t *dataptr);
uint32_t be32read_postinc(uint8_t *&dataptr);
typedef std::pair<uint32_t, uint32_t> u32pair_t;
void le32pwrite_postinc(uint8_t *&dataptr, const u32pair_t &pair);
void le32pwrite(uint8_t *dataptr, const u32pair_t &pair);
uint32_t roundup(uint32_t value, int round);

#endif//COMPDVR_UTIL_H
