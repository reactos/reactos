#ifndef	_TFTP_H
#define	_TFTP_H

#define TFTP_PORT	69
#define	TFTP_DEFAULTSIZE_PACKET	512
#define	TFTP_MAX_PACKET		1432 /* 512 */

#define TFTP_RRQ	1
#define TFTP_WRQ	2
#define TFTP_DATA	3
#define TFTP_ACK	4
#define TFTP_ERROR	5
#define TFTP_OACK	6

#define TFTP_CODE_EOF	1
#define TFTP_CODE_MORE	2
#define TFTP_CODE_ERROR	3
#define TFTP_CODE_BOOT	4
#define TFTP_CODE_CFG	5

struct tftp_t {
	struct iphdr ip;
	struct udphdr udp;
	uint16_t opcode;
	union {
		uint8_t rrq[TFTP_DEFAULTSIZE_PACKET];
		struct {
			uint16_t block;
			uint8_t  download[TFTP_MAX_PACKET];
		} data;
		struct {
			uint16_t block;
		} ack;
		struct {
			uint16_t errcode;
			uint8_t  errmsg[TFTP_DEFAULTSIZE_PACKET];
		} err;
		struct {
			uint8_t  data[TFTP_DEFAULTSIZE_PACKET+2];
		} oack;
	} u;
};

/* define a smaller tftp packet solely for making requests to conserve stack
   512 bytes should be enough */
struct tftpreq_t {
	struct iphdr ip;
	struct udphdr udp;
	uint16_t opcode;
	union {
		uint8_t rrq[512];
		struct {
			uint16_t block;
		} ack;
		struct {
			uint16_t errcode;
			uint8_t  errmsg[512-2];
		} err;
	} u;
};

#define TFTP_MIN_PACKET	(sizeof(struct iphdr) + sizeof(struct udphdr) + 4)

#endif	/* _TFTP_H */
