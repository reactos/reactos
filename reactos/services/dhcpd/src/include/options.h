/* This software is GPL, see http://www.gnu.org to see exactly what it means */

/* This file contains some useful constant declarations */

#ifndef OPTIONS_H
#define OPTIONS_H

#define  VERSION  "ecl-dhcp-0.0.2-snoopy"

#define  MAX_PROCESS_TIME 15

#define  FALSE  0
#define  TRUE   1

#define  BOOTREQUEST 0x1
#define  BOOTREPLY   0x2

	     /* Message types */

#define  DHCPDISCOVER  0x1
#define  DHCPOFFER      0x2
#define  DHCPREQUEST    0x3
#define  DHCPDECLINE    0x4
#define  DHCPACK        0x5
#define  DHCPNAK        0x6
#define  DHCPRELEASE    0x7
#define  DHCPINFORM     0x8


#define  PAD            0x00
#define  MASK           0x01
#define  TOFFSET        0x02
#define  ROUTER         0x03
#define  TIMESERVER     0x04
#define  NS             0x05
#define  DNS            0x06
#define  LOGSERVER      0x07
#define  COOKIESERVER   0x08
#define  LPRSERVER      0x09
#define  IMPSERVER      0x0A
#define  RESLOCSERVER   0x0B
#define  HOSTNAME       0x0C
#define  BOOTFILESIZE   0x0D
#define  MERITDUMPFILE  0x0E
#define  DOMAINNAME     0x0F
#define  SWAPSERVER     0x10
#define  ROOTPATH       0x11
#define  EXTENSIONPATH  0x12
#define  IPFORWARD      0x13
#define  NONLOCAL       0x14
#define  POLICYFILTER   0x15
#define  MAXIMUMDATAG   0x16
#define  DEFAULTTTL     0x17
#define  PATHMTUATO     0x18
#define  PATHMTUPTO     0x19
#define  IMTU           0x1A
#define  ALLSUBLOCAL    0x1B
#define  BROADCAST      0x1C
#define  PMASKDISCOVERY 0x1D
#define  MASKSUPPLIER   0x1E
#define  PROUTERDISCOVE 0x1F
#define  RSOLICIADDRESS 0x20
#define  STATICROUTE    0x21
#define  TENCAPSULATION 0x22
#define  ARPCACHE       0x23
#define  ETHENCAPSUL    0x24
#define  TCPDEFTTL      0x25
#define  TCPKAI         0x26
#define  TCPKAG         0x27
#define  NISDOMAIN      0x28
#define  NISSERVER      0x29
#define  NTPSERVER      0x2A
#define  VENDORSP       0x2B
#define  NBTCPIPNS      0x2C
#define  NBTCPIPDDS     0x2D
#define  NBTCPIPNT      0x2E
#define  NBTCPIPSC      0x2F
#define  XWINFONTSERVER 0x30
#define  XWINDISPLAY    0x31
#define  IP             0x32
#define  LEASE          0x33
#define  OVERLOAD       0x34
#define  MESSAGETYPE    0x35
#define  SERVER         0x36
#define  PREQUEST       0x37
#define  MESSAGE        0x38
#define  MAXIMUMDHCP    0x39
#define  RENEWALTIME    0x3A
#define  REBINDING      0x3B
#define  VENDORCLASS    0x3C
#define  CLIENT         0x3D
#define  NISPLUSDOMAIN  0x40
#define  NISPLUSSERVER  0x41
#define  TFTPSERVER     0x42
#define  BOOTFILE       0x43
#define  MOBILEIP       0x44
#define  SMTPSERVER     0x45
#define  POP3SERVER     0x46
#define  NNTPSERVER     0x47
#define  HTTPSERVER     0x48
#define  FINGERSERVER   0x49
#define  IRCSERVER      0x4A
#define  STREETTALKSE   0x4B
#define  STREETTALKDA   0x4C
#define  END            0xFF

		 /* Constants */
#define  FREE           0x01
#define  PROCESSING     0x02
#define  BUSY           0x00

#define  DYNAMIC        0x00
#ifdef STATIC
#undef STATIC
#endif
#define  STATIC         0x01

#endif
