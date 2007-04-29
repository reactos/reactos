#include "util.h"

uint32_t roundup(uint32_t value, int round)
{
    round--;
    return (value + round) & ~round;
}

void le16write(uint8_t *dataptr, uint16_t value)
{
    dataptr[0] = value;
    dataptr[1] = value >> 8;
}

void le16write_postinc(uint8_t *&dataptr, uint16_t value)
{
    le16write(dataptr, value); dataptr += 2;
}

void le32write(uint8_t *dataptr, uint32_t value)
{
    le16write(dataptr, value);
    le16write(dataptr + 2, value >> 16);
}

void le32write_postinc(uint8_t *&dataptr, uint32_t value)
{
    le32write(dataptr, value); dataptr += 4;
}

void le32pwrite_postinc(uint8_t *&dataptr, const u32pair_t &value)
{
    le32write_postinc(dataptr, value.first);
    le32write_postinc(dataptr, value.second);
}
