/*-
 * Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995, 1996, 1997
 *  The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from the Stanford/CMU enet packet filter,
 * (net/enet.c) distributed as part of 4.3BSD, and code contributed
 * to Berkeley by Steven McCanne and Van Jacobson both of Lawrence 
 * Berkeley Laboratory.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *      @(#)bpf.h       7.1 (Berkeley) 5/7/91
 *
 * @(#) $Header: /cygdrive/c/RCVS/CVS/ReactOS/reactos/drivers/net/npf/win_bpf.h,v 1.1 2003/01/08 19:55:02 robd Exp $ (LBL)
 */

#ifndef BPF_MAJOR_VERSION

/* BSD style release date */
#define BPF_RELEASE 199606

typedef UCHAR u_char;
typedef USHORT u_short;
typedef ULONG u_int;
typedef LONG bpf_int32;
typedef ULONG bpf_u_int32;
typedef ULONG u_int32;

#define BPF_MAXINSNS 512
#define BPF_MAXBUFSIZE 0x8000
#define BPF_MINBUFSIZE 32

#include "time_calls.h"

/*
 * The instruction data structure.
 */
struct bpf_insn {
    u_short code;
    u_char  jt;
    u_char  jf;
    bpf_int32 k;
};

/*
 *  Structure for BIOCSETF.
 */
struct bpf_program {
    u_int bf_len;
    struct bpf_insn *bf_insns;
};
 
/*
 * Struct returned by BIOCGSTATS.
 */
struct bpf_stat {
    u_int bs_recv;      /* number of packets received */
    u_int bs_drop;      /* number of packets dropped */
};

/*
 * Struct return by BIOCVERSION.  This represents the version number of 
 * the filter language described by the instruction encodings below.
 * bpf understands a program iff kernel_major == filter_major &&
 * kernel_minor >= filter_minor, that is, if the value returned by the
 * running kernel has the same major number and a minor number equal
 * equal to or less than the filter being downloaded.  Otherwise, the
 * results are undefined, meaning an error may be returned or packets
 * may be accepted haphazardly.
 * It has nothing to do with the source code version.
 */
struct bpf_version {
    u_short bv_major;
    u_short bv_minor;
};
/* Current version number of filter architecture. */
#define BPF_MAJOR_VERSION 1
#define BPF_MINOR_VERSION 1


/*
 * Structure prepended to each packet.
 */
struct bpf_hdr {
    struct timeval  bh_tstamp;  /* time stamp */
    bpf_u_int32 bh_caplen;  /* length of captured portion */
    bpf_u_int32 bh_datalen; /* original length of packet */
    u_short     bh_hdrlen;  /* length of bpf header (this struct
                       plus alignment padding) */
};

/*
 * Data-link level type codes.
 */

/*
 * These are the types that are the same on all platforms; on other
 * platforms, a <net/bpf.h> should be supplied that defines the additional
 * DLT_* codes appropriately for that platform (the BSDs, for example,
 * should not just pick up this version of "bpf.h"; they should also define
 * the additional DLT_* codes used by their kernels, as well as the values
 * defined here - and, if the values they use for particular DLT_ types
 * differ from those here, they should use their values, not the ones
 * here).
 */
#define DLT_NULL    0   /* no link-layer encapsulation */
#define DLT_EN10MB  1   /* Ethernet (10Mb) */
#define DLT_EN3MB   2   /* Experimental Ethernet (3Mb) */
#define DLT_AX25    3   /* Amateur Radio AX.25 */
#define DLT_PRONET  4   /* Proteon ProNET Token Ring */
#define DLT_CHAOS   5   /* Chaos */
#define DLT_IEEE802 6   /* IEEE 802 Networks */
#define DLT_ARCNET  7   /* ARCNET */
#define DLT_SLIP    8   /* Serial Line IP */
#define DLT_PPP     9   /* Point-to-point Protocol */
#define DLT_FDDI    10  /* FDDI */

/*
 * These are values from the traditional libpcap "bpf.h".
 * Ports of this to particular platforms should replace these definitions
 * with the ones appropriate to that platform, if the values are
 * different on that platform.
 */
#define DLT_ATM_RFC1483 11  /* LLC/SNAP encapsulated atm */
#define DLT_RAW     12  /* raw IP */

/*
 * These are values from BSD/OS's "bpf.h".
 * These are not the same as the values from the traditional libpcap
 * "bpf.h"; however, these values shouldn't be generated by any
 * OS other than BSD/OS, so the correct values to use here are the
 * BSD/OS values.
 *
 * Platforms that have already assigned these values to other
 * DLT_ codes, however, should give these codes the values
 * from that platform, so that programs that use these codes will
 * continue to compile - even though they won't correctly read
 * files of these types.
 */
#define DLT_SLIP_BSDOS  15  /* BSD/OS Serial Line IP */
#define DLT_PPP_BSDOS   16  /* BSD/OS Point-to-point Protocol */

#define DLT_ATM_CLIP    19  /* Linux Classical-IP over ATM */

/*
 * This value is defined by NetBSD; other platforms should refrain from
 * using it for other purposes, so that NetBSD savefiles with a link
 * type of 50 can be read as this type on all platforms.
 */
#define DLT_PPP_SERIAL  50  /* PPP over serial with HDLC encapsulation */

/*
 * This value was defined by libpcap 0.5; platforms that have defined
 * it with a different value should define it here with that value -
 * a link type of 104 in a save file will be mapped to DLT_C_HDLC,
 * whatever value that happens to be, so programs will correctly
 * handle files with that link type regardless of the value of
 * DLT_C_HDLC.
 *
 * The name DLT_C_HDLC was used by BSD/OS; we use that name for source
 * compatibility with programs written for BSD/OS.
 *
 * libpcap 0.5 defined it as DLT_CHDLC; we define DLT_CHDLC as well,
 * for source compatibility with programs written for libpcap 0.5.
 */
#define DLT_C_HDLC  104 /* Cisco HDLC */
#define DLT_CHDLC   DLT_C_HDLC

/*
 * Reserved for future use.
 * Do not pick other numerical value for these unless you have also
 * picked up the tcpdump.org top-of-CVS-tree version of "savefile.c",
 * which will arrange that capture files for these DLT_ types have
 * the same "network" value on all platforms, regardless of what
 * value is chosen for their DLT_ type (thus allowing captures made
 * on one platform to be read on other platforms, even if the two
 * platforms don't use the same numerical values for all DLT_ types).
 */
#define DLT_IEEE802_11  105 /* IEEE 802.11 wireless */

/*
 * Values between 106 and 107 are used in capture file headers as
 * link-layer types corresponding to DLT_ types that might differ
 * between platforms; don't use those values for new DLT_ new types.
 */

/*
 * OpenBSD DLT_LOOP, for loopback devices; it's like DLT_NULL, except
 * that the AF_ type in the link-layer header is in network byte order.
 *
 * OpenBSD defines it as 12, but that collides with DLT_RAW, so we
 * define it as 108 here.  If OpenBSD picks up this file, it should
 * define DLT_LOOP as 12 in its version, as per the comment above -
 * and should not use 108 for any purpose.
 */
#define DLT_LOOP    108

/*
 * Values between 109 and 112 are used in capture file headers as
 * link-layer types corresponding to DLT_ types that might differ
 * between platforms; don't use those values for new DLT_ new types.
 */

/*
 * This is for Linux cooked sockets.
 */
#define DLT_LINUX_SLL   113

/*
 * The instruction encodings.
 */
/* instruction classes */
#define BPF_CLASS(code) ((code) & 0x07)
#define     BPF_LD      0x00
#define     BPF_LDX     0x01
#define     BPF_ST      0x02
#define     BPF_STX     0x03
#define     BPF_ALU     0x04
#define     BPF_JMP     0x05
#define     BPF_RET     0x06
#define     BPF_MISC    0x07

/* ld/ldx fields */
#define BPF_SIZE(code)  ((code) & 0x18)
#define     BPF_W       0x00
#define     BPF_H       0x08
#define     BPF_B       0x10
#define BPF_MODE(code)  ((code) & 0xe0)
#define     BPF_IMM     0x00
#define     BPF_ABS     0x20
#define     BPF_IND     0x40
#define     BPF_MEM     0x60
#define     BPF_LEN     0x80
#define     BPF_MSH     0xa0

/* alu/jmp fields */
#define BPF_OP(code)    ((code) & 0xf0)
#define     BPF_ADD     0x00
#define     BPF_SUB     0x10
#define     BPF_MUL     0x20
#define     BPF_DIV     0x30
#define     BPF_OR      0x40
#define     BPF_AND     0x50
#define     BPF_LSH     0x60
#define     BPF_RSH     0x70
#define     BPF_NEG     0x80
#define     BPF_JA      0x00
#define     BPF_JEQ     0x10
#define     BPF_JGT     0x20
#define     BPF_JGE     0x30
#define     BPF_JSET    0x40
#define BPF_SRC(code)   ((code) & 0x08)
#define     BPF_K       0x00
#define     BPF_X       0x08

/* ret - BPF_K and BPF_X also apply */
#define BPF_RVAL(code)  ((code) & 0x18)
#define     BPF_A       0x10

/* misc */
#define BPF_MISCOP(code) ((code) & 0xf8)
#define     BPF_TAX     0x00
#define     BPF_TXA     0x80

/* TME instructions */
#define     BPF_TME                 0x08

#define     BPF_LOOKUP              0x90   
#define     BPF_EXECUTE             0xa0
#define     BPF_INIT                0xb0
#define     BPF_VALIDATE            0xc0
#define     BPF_SET_ACTIVE          0xd0
#define     BPF_RESET               0xe0
#define     BPF_SET_MEMORY          0x80
#define     BPF_GET_REGISTER_VALUE  0x70
#define     BPF_SET_REGISTER_VALUE  0x60
#define     BPF_SET_WORKING         0x50
#define     BPF_SET_ACTIVE_READ     0x40
#define     BPF_SET_AUTODELETION    0x30
#define     BPF_SEPARATION          0xff

#define     BPF_MEM_EX_IMM  0xc0
#define     BPF_MEM_EX_IND  0xe0
/*used for ST */
#define     BPF_MEM_EX      0xc0


/*
 * Macros for insn array initializers.
 */
#define BPF_STMT(code, k) { (u_short)(code), 0, 0, k }
#define BPF_JUMP(code, k, jt, jf) { (u_short)(code), jt, jf, k }

/*
 * Number of scratch memory words (for BPF_LD|BPF_MEM and BPF_ST).
 */
#define BPF_MEMWORDS 16

#endif
