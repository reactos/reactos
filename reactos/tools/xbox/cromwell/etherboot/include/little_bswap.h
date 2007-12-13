#ifndef ETHERBOOT_LITTLE_BSWAP_H
#define ETHERBOOT_LITTLE_BSWAP_H

#define ntohl(x)	__bswap_32(x)
#define htonl(x) 	__bswap_32(x)
#define ntohs(x) 	__bswap_16(x)
#define htons(x) 	__bswap_16(x)
#define cpu_to_le32(x)	(x)
#define cpu_to_le16(x)	(x)
#define cpu_to_be32(x)	__bswap_32(x)
#define cpu_to_be16(x)	__bswap_16(x)
#define le32_to_cpu(x)	(x)
#define le16_to_cpu(x)	(x)
#define be32_to_cpu(x)	__bswap_32(x)
#define be16_to_cpu(x)	__bswap_16(x)

#endif /* ETHERBOOT_LITTLE_BSWAP_H */
