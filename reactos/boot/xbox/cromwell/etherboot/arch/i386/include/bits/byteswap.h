#ifndef ETHERBOOT_BITS_BYTESWAP_H
#define ETHERBOOT_BITS_BYTESWAP_H

static inline uint16_t __i386_bswap_16(uint16_t x)
{
	__asm__("xchgb %b0,%h0\n\t"
		: "=q" (x)
		: "0" (x));
	return x;
}

static inline uint32_t __i386_bswap_32(uint32_t x)
{
	__asm__("xchgb %b0,%h0\n\t"
		"rorl $16,%0\n\t"
		"xchgb %b0,%h0"
		: "=q" (x)
		: "0" (x));
	return x;
}


#define __bswap_constant_16(x) \
	((uint16_t)((((uint16_t)(x) & 0x00ff) << 8) | \
		(((uint16_t)(x) & 0xff00) >> 8)))

#define __bswap_constant_32(x) \
	((uint32_t)((((uint32_t)(x) & 0x000000ffU) << 24) | \
		(((uint32_t)(x) & 0x0000ff00U) <<  8) | \
		(((uint32_t)(x) & 0x00ff0000U) >>  8) | \
		(((uint32_t)(x) & 0xff000000U) >> 24)))

#define __bswap_16(x) \
	(__builtin_constant_p(x) ? \
	__bswap_constant_16(x) : \
	__i386_bswap_16(x))


#define __bswap_32(x) \
	(__builtin_constant_p(x) ? \
	__bswap_constant_32(x) : \
	__i386_bswap_32(x))


#endif /* ETHERBOOT_BITS_BYTESWAP_H */
