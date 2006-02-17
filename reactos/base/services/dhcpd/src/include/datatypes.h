#ifndef  DATATYPES_H
#define  DATATYPES_H

typedef unsigned char  u8b;
typedef unsigned short u16b;
typedef unsigned int   u32b;

typedef struct{
  u8b  op;
  u8b  htype;
  u8b  hlen;
  u8b  hops;
  u32b xid;
  u16b secs;
  u16b flags;
  u32b ciaddr;
  u32b yiaddr;
  u32b siaddr;
  u32b giaddr;
  u8b  chaddr[16];
  u8b  sname[64];
  u8b  file[128];
  u8b  options[312];
} DHCPMESSAGE;

typedef struct{
  u8b  type;
  u32b r_ip;
  u32b r_mask;
  u32b r_router;
  u32b r_lease;
  char *hostname;
} DHCPOPTIONS;

typedef struct{
  u32b ip;
  u32b router;
  u32b mask;
  u32b lease;
  u32b siaddr;
} DHCPLEASE;

struct _DHCPLIST{
  u8b  available;
  u32b xid;
  u8b  chaddr[16];
  u8b type;
  u32b ltime;
  DHCPLEASE data;
  struct _DHCPLIST *next;
  struct _DHCPLIST *back;
};

typedef struct _DHCPLIST DHCPLIST;

#endif
